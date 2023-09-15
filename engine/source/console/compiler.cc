//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "console/telnetDebugger.h"
#include "platform/event.h"

#include "console/ast.h"
#include "core/tAlgorithm.h"
#include "core/resManager.h"

#include "core/findMatch.h"
#include "console/consoleInternal.h"
#include "core/fileStream.h"
#include "console/compiler.h"
#include "console/gram.h"

#include "console/simBase.h"

extern const char *CMDGetCurrentFile();
extern S32 CMDGetCurrentLine();

DataChunker consoleAllocator;
extern void SetScanBuffer(const char *sb, const char *fn);
extern S32 CMDparse();
extern void CMD_reset();

CodeBlock* CodeBlock::sCurrentCodeBlock = NULL;

static U32 evalSTEtoU32(StringTableEntry ste, U32)
{
   return *((U32 *) &ste);
}

static CompilerIdentTable gIdentTable;

static U32 compileSTEtoU32(StringTableEntry ste, U32 ip)
{
   if(ste)
      gIdentTable.add(ste, ip);
   return 0;
}

U32 (*STEtoU32)(StringTableEntry ste, U32 ip) = evalSTEtoU32;


//------------------------------------------------------------

const StringTableEntry CodeBlock::getCurrentCodeBlockName()
{
   if (sCurrentCodeBlock)
      return sCurrentCodeBlock->name;
   else
      return NULL;
}   


CodeBlock *CodeBlock::find(StringTableEntry name)
{
   for(CodeBlock *walk = codeBlockList; walk; walk = walk->nextFile)
      if(walk->name == name)
         return walk;
   return NULL;
}

CodeBlock::CodeBlock()
{
   globalStrings = NULL;
   functionStrings = NULL;
   globalFloats = NULL;
   functionFloats = NULL;
   lineBreakPairs = NULL;
   breakList = NULL;
   breakListSize = 0;
   
   refCount = 0;
   code = NULL;
   name = NULL;
   mRoot = StringTable->insert("");
}

CodeBlock::~CodeBlock()
{
   if(name)
      removeFromCodeList();
   delete[] const_cast<char*>(globalStrings);
   delete[] const_cast<char*>(functionStrings);
   delete[] globalFloats;
   delete[] functionFloats;
   delete[] code;
   delete[] breakList;
}

static bool inFunction;
static U32 breakLineCount;
static CodeBlock *curCode = NULL;
CodeBlock *codeBlockList = NULL;

void CodeBlock::addToCodeList()
{
   // remove any code blocks with my name
   for(CodeBlock **walk = &codeBlockList; *walk;walk = &((*walk)->nextFile))
   {
      if((*walk)->name == name)
      {
         *walk = (*walk)->nextFile;
         break;
      }
   }
   nextFile = codeBlockList;
   codeBlockList = this;
}

void CodeBlock::clearAllBreaks()
{
   if(!lineBreakPairs)
      return;
   for(U32 i = 0; i < lineBreakPairCount; i++)
   {
      U32 *p = lineBreakPairs + i * 2;
      code[p[1]] = p[0] & 0xFF;
   }
}

void CodeBlock::clearBreakpoint(U32 lineNumber)
{
   if(!lineBreakPairs)
      return;
   for(U32 i = 0; i < lineBreakPairCount; i++)
   {
      U32 *p = lineBreakPairs + i * 2;
      if((p[0] >> 8) == lineNumber)
      {
         code[p[1]] = p[0] & 0xFF;
         return;
      }
   }
}

void CodeBlock::setAllBreaks()
{
   if(!lineBreakPairs)
      return;
   for(U32 i = 0; i < lineBreakPairCount; i++)
   {
      U32 *p = lineBreakPairs + i * 2;
      code[p[1]] = OP_BREAK;
   }
}

void CodeBlock::setBreakpoint(U32 lineNumber)
{
   if(!lineBreakPairs)
      return;
   for(U32 i = 0; i < lineBreakPairCount; i++)
   {
      U32 *p = lineBreakPairs + i * 2;
      if((p[0] >> 8) == lineNumber)
      {
         code[p[1]] = OP_BREAK;
         return;
      }
   }
}

struct LinePair
{
   U32 instLine;
   U32 ip;
};

void CodeBlock::findBreakLine(U32 ip, U32 &line, U32 &instruction)
{
   U32 min = 0;
   U32 max = lineBreakPairCount - 1;
   LinePair *p = (LinePair *) lineBreakPairs;

   U32 found;
   if(!lineBreakPairCount || p[min].ip > ip || p[max].ip < ip)
   {
      line = 0;
      instruction = OP_INVALID;
      return;
   }
   else if(p[min].ip == ip)
      found = min;
   else if(p[max].ip == ip)
      found = max;
   else
   {
      for(;;)
      {
         if(min == max - 1)
         {
            found = min;
            break;
         }
         U32 mid = (min + max) >> 1;
         if(p[mid].ip == ip)
         {
            found = mid;
            break;
         }
         else if(p[mid].ip > ip)
            max = mid;
         else
            min = mid;
      }
   }
   instruction = p[found].instLine & 0xFF;
   line = p[found].instLine >> 8;
}

const char *CodeBlock::getFileLine(U32 ip)
{
   static char nameBuffer[256];
   U32 line, inst;
   findBreakLine(ip, line, inst);
   
   dSprintf(nameBuffer, sizeof(nameBuffer), "%s (%d)", name ? name : "<input>", line);
   return nameBuffer;
}

void CodeBlock::removeFromCodeList()
{
   for(CodeBlock **walk = &codeBlockList; *walk; walk = &((*walk)->nextFile))
   {
      if(*walk == this)
      {
         *walk = nextFile;
         // clear out all breakpoints
         clearAllBreaks();
         return;
      }
   }
}

void CodeBlock::calcBreakList()
{
   U32 size = 0;
   S32 line = -1;
   U32 seqCount = 0;
   U32 i;
   for(i = 0; i < lineBreakPairCount; i++)
   {
      U32 lineNumber = lineBreakPairs[i * 2];
      if(lineNumber == U32(line + 1))
         seqCount++;
      else
      {
         if(seqCount)
            size++;
         size++;
         seqCount = 1;
      }
      line = lineNumber;
   }
   if(seqCount)
      size++;
      
   breakList = new U32[size];
   breakListSize = size;
   line = -1;
   seqCount = 0;
   size = 0;
   for(i = 0; i < lineBreakPairCount; i++)
   {
      U32 lineNumber = lineBreakPairs[i * 2];
      if(lineNumber == U32(line + 1))
         seqCount++;
      else
      {
         if(seqCount)
            breakList[size++] = seqCount;
         breakList[size++] = lineNumber - getMax(0, line) - 1;
         seqCount = 1;
      }
      line = lineNumber;
   }
   if(seqCount)
      breakList[size++] = seqCount;
   for(i = 0; i < lineBreakPairCount; i++)
   {
      U32 *p = lineBreakPairs + i * 2;
      p[0] = (p[0] << 8) | code[p[1]];
   }
}

bool CodeBlock::read(StringTableEntry fileName, Stream &st)
{
   name = fileName;
   //
   if (name)
   {
      if (const char *slash = dStrchr(this->name, '/'))
      {
         char root[512];
         dStrncpy(root, this->name, slash-this->name);
         root[slash-this->name] = 0;
         mRoot = StringTable->insert(root);
      }
   }
   //
   addToCodeList();

   U32 globalSize,size,i;
   st.read(&size);
   if(size)
   {
      globalSize = size;
      globalStrings = new char[size];
      st.read(size, globalStrings);
   }
   st.read(&size);
   if(size)
   {
      globalFloats = new F64[size];
      for(U32 i = 0; i < size; i++)
         st.read(&globalFloats[i]);
   }
   st.read(&size);
   if(size)
   {
      functionStrings = new char[size];
      st.read(size, functionStrings);
   }
   st.read(&size);
   if(size)
   {
      functionFloats = new F64[size];
      for(U32 i = 0; i < size; i++)
         st.read(&functionFloats[i]);
   }
   U32 codeSize;
   st.read(&codeSize);
   st.read(&lineBreakPairCount);
   
   U32 totSize = codeSize + lineBreakPairCount * 2;
   code = new U32[totSize];

   for(i = 0; i < codeSize; i++)
   {
      U8 b;
      st.read(&b);
      if(b == 0xFF)
         st.read(&code[i]);
      else
         code[i] = b;
   }
      
   for(i = codeSize; i < totSize; i++)
      st.read(&code[i]);

   lineBreakPairs = code + codeSize;
   
   U32 identCount;
   st.read(&identCount);
   while(identCount--)
   {
      U32 offset;
      st.read(&offset);
      StringTableEntry ste;
      if(offset < globalSize)
	 ste = StringTable->insert(globalStrings + offset);
      else
	 ste = StringTable->insert("");
      U32 count;
      st.read(&count);
      while(count--)
      {
         U32 ip;
         st.read(&ip);
         code[ip] = *((U32 *) &ste);
      }
   }
   if(lineBreakPairCount)
      calcBreakList();
   return true;
}

bool gConsoleSyntaxError;

bool CodeBlock::compile(const char *codeFileName, StringTableEntry fileName, const char *script)
{
   gConsoleSyntaxError = false;

   consoleAllocator.freeBlocks();

   STEtoU32 = compileSTEtoU32;

   statementList = NULL;
   SetScanBuffer(script, fileName);
   CMD_reset();
   CMDparse();
   if(gConsoleSyntaxError)
   {
      consoleAllocator.freeBlocks();
      return false;
   }   

   FileStream st;
   if(!ResourceManager->openFileForWrite(st, codeFileName)) 
      return false;
   st.write(U32(Con::DSOVersion));
      
   currentStringTable = &gGlobalStringTable;
   currentFloatTable = &gGlobalFloatTable;
   gGlobalFloatTable.reset();
   gGlobalStringTable.reset();
   gFunctionFloatTable.reset();
   gFunctionStringTable.reset();
   gIdentTable.reset();
   
   inFunction = false;
   breakLineCount = 0;
   curCode = this;
   if(statementList)
      codeSize = precompileBlock(statementList, 0) + 1;
   else
      codeSize = 1;
      
   lineBreakPairCount = breakLineCount;
   code = new U32[codeSize + breakLineCount * 2];
   lineBreakPairs = code + codeSize;
   
   gGlobalStringTable.write(st);
   gGlobalFloatTable.write(st);
   gFunctionStringTable.write(st);
   gFunctionFloatTable.write(st);
   
   breakLineCount = 0;
   U32 lastIp;
   if(statementList)
      lastIp = compileBlock(statementList, code, 0, 0, 0);
   else
      lastIp = 0;
      
   if(lastIp != codeSize - 1)
      Con::errorf(ConsoleLogEntry::General, "precompile size mismatch");

   code[lastIp++] = OP_RETURN;
   U32 totSize = codeSize + breakLineCount * 2;
   st.write(codeSize);
   st.write(lineBreakPairCount);

   U32 i;   
   for(i = 0; i < codeSize; i++)
   {
      if(code[i] < 0xFF)
         st.write(U8(code[i]));
      else
      {
         st.write(U8(0xFF));
         st.write(code[i]);
      }
   }
   for(i = codeSize; i < totSize; i++)
      st.write(code[i]);
   
   gIdentTable.write(st);
   
   consoleAllocator.freeBlocks();
   st.close();
   
   return true;
}

const char *CodeBlock::compileExec(StringTableEntry fileName, const char *string, bool noCalls)
{
   STEtoU32 = evalSTEtoU32;
   consoleAllocator.freeBlocks();
   name = fileName;
   if(name)
      addToCodeList();
   
   statementList = NULL;
   SetScanBuffer(string, fileName);
   CMD_reset();
   CMDparse();

   if(!statementList)
   {
      delete this;
      return "";
   }

   currentStringTable = &gGlobalStringTable;
   currentFloatTable = &gGlobalFloatTable;
   gGlobalFloatTable.reset();
   gGlobalStringTable.reset();
   gFunctionFloatTable.reset();
   gFunctionStringTable.reset();
   
   inFunction = false;
   breakLineCount = 0;
   curCode = this;
   
   codeSize = precompileBlock(statementList, 0) + 1;
   
   lineBreakPairCount = breakLineCount;
   
   globalStrings = gGlobalStringTable.build();
   functionStrings = gFunctionStringTable.build();
   globalFloats = gGlobalFloatTable.build();
   functionFloats = gFunctionFloatTable.build();
   
   code = new U32[codeSize + lineBreakPairCount * 2];
   lineBreakPairs = code + codeSize;
   
   breakLineCount = 0;
   U32 lastIp = compileBlock(statementList, code, 0, 0, 0);
   code[lastIp++] = OP_RETURN;
   consoleAllocator.freeBlocks();
   if(lineBreakPairCount && fileName)
      calcBreakList();

   if(lastIp != codeSize)
      Con::warnf(ConsoleLogEntry::General, "precompile size mismatch");
   return exec(0, fileName, NULL, 0, 0, noCalls, NULL);
}

void CodeBlock::incRefCount()
{
   refCount++;
}

void CodeBlock::decRefCount()
{
   refCount--;
   if(!refCount)
      delete this;
}

void StmtNode::addBreakCount()
{
   if(inFunction)
      breakLineCount++;
}

void StmtNode::addBreakLine(U32 ip)
{
   if(inFunction)
   {
      U32 line = breakLineCount * 2;
      breakLineCount++;
      if(curCode->lineBreakPairs)
      {
         curCode->lineBreakPairs[line] = dbgLineNumber;
         curCode->lineBreakPairs[line+1] = ip;
      }
   }
}

//------------------------------------------------------------

U32 CompilerStringTable::add(const char *str, bool caseSens, bool tag)
{
   Entry **walk;
   for(walk = &list; *walk; walk = &((*walk)->next))
   {
      if((*walk)->tag != tag)
         continue;
      if(caseSens)
      {
         if(!dStrcmp((*walk)->string, str))
            return (*walk)->start;
      }
      else
      {
         if(!dStricmp((*walk)->string, str))
            return (*walk)->start;
      }
   }
   Entry *newStr = (Entry *) consoleAllocator.alloc(sizeof(Entry));
   *walk = newStr;
   newStr->next = NULL;
   newStr->start = totalLen;
   U32 len = dStrlen(str) + 1;
   if(tag && len < 7) // alloc space for the numeric tag 1 for tag, 5 for # and 1 for nul
      len = 7;
   totalLen += len;
   newStr->string = (char *) consoleAllocator.alloc(len);
   newStr->len = len;
   newStr->tag = tag;
   dStrcpy(newStr->string, str);
   return newStr->start;
}

U32 CompilerStringTable::addIntString(U32 value)
{
   dSprintf(buf, sizeof(buf), "%d", value);
   return add(buf);
}

U32 CompilerStringTable::addFloatString(F64 value)
{
   dSprintf(buf, sizeof(buf), "%g", value);
   return add(buf);   
}
void CompilerStringTable::reset()
{
   list = NULL;
   totalLen = 0;
}
char *CompilerStringTable::build()
{
   char *ret = new char[totalLen];
   for(Entry *walk = list; walk; walk = walk->next)
      dStrcpy(ret + walk->start, walk->string);
   return ret;
}

void CompilerStringTable::write(Stream &st)
{
   st.write(totalLen);
   for(Entry *walk = list; walk; walk = walk->next)
      st.write(walk->len, walk->string);
}

CompilerStringTable *currentStringTable, gGlobalStringTable, gFunctionStringTable;

//------------------------------------------------------------

U32 CompilerFloatTable::add(F64 value)
{
   Entry **walk;
   U32 i = 0;
   for(walk = &list; *walk; walk = &((*walk)->next), i++)
      if(value == (*walk)->val)
         return i;
   Entry *newFloat = (Entry *) consoleAllocator.alloc(sizeof(Entry));
   newFloat->val = value;
   newFloat->next = NULL;
   count++;
   *walk = newFloat;
   return count-1;
}
void CompilerFloatTable::reset()
{
   list = NULL;
   count = 0;
}
F64 *CompilerFloatTable::build()
{
   F64 *ret = new F64[count];
   U32 i = 0;
   for(Entry *walk = list; walk; walk = walk->next, i++)
      ret[i] = walk->val;
   return ret;
}

void CompilerFloatTable::write(Stream &st)
{
   st.write(count);
   for(Entry *walk = list; walk; walk = walk->next)
      st.write(walk->val);
}

CompilerFloatTable *currentFloatTable, gGlobalFloatTable, gFunctionFloatTable;

//------------------------------------------------------------

static void precompileIdent(StringTableEntry ident)
{
   if(ident)
      gGlobalStringTable.add(ident);
}

void CompilerIdentTable::reset()
{
   list = NULL;
}

void CompilerIdentTable::add(StringTableEntry ste, U32 ip)
{
   U32 index = gGlobalStringTable.add(ste, false);
   Entry *newEntry = (Entry *) consoleAllocator.alloc(sizeof(Entry));
   newEntry->offset = index;
   newEntry->ip = ip;
   for(Entry *walk = list; walk; walk = walk->next)
   {
      if(walk->offset == index)
      {
         newEntry->nextIdent = walk->nextIdent;
         walk->nextIdent = newEntry;
         return;
      }
   }
   newEntry->next = list;
   list = newEntry;
   newEntry->nextIdent = NULL;
}

void CompilerIdentTable::write(Stream &st)
{
   U32 count = 0;
   Entry * walk;
   for(walk = list; walk; walk = walk->next)
      count++;
   st.write(count);
   for(walk = list; walk; walk = walk->next)
   {
      U32 ec = 0;
      Entry * el;
      for(el = walk; el; el = el->nextIdent)
         ec++;
      st.write(walk->offset);
      st.write(ec);
      for(el = walk; el; el = el->nextIdent)
         st.write(el->ip);
   }
}

//------------------------------------------------------------

extern const char *CMDgetFileLine(int &lineNumber);

StmtNode::StmtNode()
{
   next = NULL;
   dbgFileName = CMDGetCurrentFile();
   dbgLineNumber = CMDGetCurrentLine();
}

void StmtNode::setPackage(StringTableEntry)
{
}

U32 precompileBlock(StmtNode *block, U32 loopCount)
{
   U32 sum = 0;
   for(StmtNode *walk = block; walk; walk = walk->getNext())
      sum += walk->precompileStmt(loopCount);
   return sum;
}

U32 compileBlock(StmtNode *block, U32 *codeStream, U32 ip, U32 continuePoint, U32 breakPoint)
{
   for(StmtNode *walk = block; walk; walk = walk->getNext())
      ip = walk->compileStmt(codeStream, ip, continuePoint, breakPoint);
   return ip;
}

void StmtNode::append(StmtNode *next)
{
   StmtNode *walk = this;
   while(walk->next)
      walk = walk->next;
   walk->next = next;
}

U32 ExprNode::compileX(U32 *codeStream, U32 ip, TypeReq type)
{
   U32 size = precompile(type);
   U32 start = ip;
   U32 ret = compile(codeStream, ip, type);
   if(ret - start != size)
   {
      precompile(type);
      compile(codeStream, ip, type);
   }
   return ret;
}

//------------------------------------------------------------
//
// Console language node allocators
//
//------------------------------------------------------------

BreakStmtNode *BreakStmtNode::alloc()
{
   BreakStmtNode *ret = (BreakStmtNode *) consoleAllocator.alloc(sizeof(BreakStmtNode));
   constructInPlace(ret);
   return ret;
}

ContinueStmtNode *ContinueStmtNode::alloc()
{
   ContinueStmtNode *ret = (ContinueStmtNode *) consoleAllocator.alloc(sizeof(ContinueStmtNode));
   constructInPlace(ret);
   return ret;
}

ReturnStmtNode *ReturnStmtNode::alloc(ExprNode *expr)
{
   ReturnStmtNode *ret = (ReturnStmtNode *) consoleAllocator.alloc(sizeof(ReturnStmtNode));
   constructInPlace(ret);
   ret->expr = expr;
   
   return ret;
}

IfStmtNode *IfStmtNode::alloc(S32 lineNumber, ExprNode *testExpr, StmtNode *ifBlock, StmtNode *elseBlock, bool propagate)
{
   IfStmtNode *ret = (IfStmtNode *) consoleAllocator.alloc(sizeof(IfStmtNode));
   constructInPlace(ret);
   ret->dbgLineNumber = lineNumber;
   
   ret->testExpr = testExpr;
   ret->ifBlock = ifBlock;
   ret->elseBlock = elseBlock;
   ret->propagate = propagate;
   
   return ret;
}

LoopStmtNode *LoopStmtNode::alloc(S32 lineNumber, ExprNode *initExpr, ExprNode *testExpr, ExprNode *endLoopExpr, StmtNode *loopBlock, bool isDoLoop)
{
   LoopStmtNode *ret = (LoopStmtNode *) consoleAllocator.alloc(sizeof(LoopStmtNode));
   constructInPlace(ret);
   ret->dbgLineNumber = lineNumber;
   ret->testExpr = testExpr;
   ret->initExpr = initExpr;
   ret->endLoopExpr = endLoopExpr;
   ret->loopBlock = loopBlock;
   ret->isDoLoop = isDoLoop;
   
   return ret;
}

FloatBinaryExprNode *FloatBinaryExprNode::alloc(S32 op, ExprNode *left, ExprNode *right)
{
   FloatBinaryExprNode *ret = (FloatBinaryExprNode *) consoleAllocator.alloc(sizeof(FloatBinaryExprNode));
   constructInPlace(ret);

   ret->op = op;
   ret->left = left;
   ret->right = right;
   
   return ret;
}

IntBinaryExprNode *IntBinaryExprNode::alloc(S32 op, ExprNode *left, ExprNode *right)
{
   IntBinaryExprNode *ret = (IntBinaryExprNode *) consoleAllocator.alloc(sizeof(IntBinaryExprNode));
   constructInPlace(ret);

   ret->op = op;
   ret->left = left;
   ret->right = right;
   
   return ret;
}
   
StreqExprNode *StreqExprNode::alloc(ExprNode *left, ExprNode *right, bool eq)
{
   StreqExprNode *ret = (StreqExprNode *) consoleAllocator.alloc(sizeof(StreqExprNode));
   constructInPlace(ret);
   ret->left = left;
   ret->right = right;
   ret->eq = eq;
   
   return ret;
}
   
StrcatExprNode *StrcatExprNode::alloc(ExprNode *left, ExprNode *right, int appendChar)
{
   StrcatExprNode *ret = (StrcatExprNode *) consoleAllocator.alloc(sizeof(StrcatExprNode));
   constructInPlace(ret);
   ret->left = left;
   ret->right = right;
   ret->appendChar = appendChar;
   
   return ret;
}
   
CommaCatExprNode *CommaCatExprNode::alloc(ExprNode *left, ExprNode *right)
{
   CommaCatExprNode *ret = (CommaCatExprNode *) consoleAllocator.alloc(sizeof(CommaCatExprNode));
   constructInPlace(ret);
   ret->left = left;
   ret->right = right;

   return ret;
}
   
IntUnaryExprNode *IntUnaryExprNode::alloc(S32 op, ExprNode *expr)
{
   IntUnaryExprNode *ret = (IntUnaryExprNode *) consoleAllocator.alloc(sizeof(IntUnaryExprNode));
   constructInPlace(ret);
   ret->op = op;
   ret->expr = expr;
   return ret;
}
   
FloatUnaryExprNode *FloatUnaryExprNode::alloc(S32 op, ExprNode *expr)
{
   FloatUnaryExprNode *ret = (FloatUnaryExprNode *) consoleAllocator.alloc(sizeof(FloatUnaryExprNode));
   constructInPlace(ret);
   ret->op = op;
   ret->expr = expr;
   return ret;
}
   
VarNode *VarNode::alloc(StringTableEntry varName, ExprNode *arrayIndex)
{
   VarNode *ret = (VarNode *) consoleAllocator.alloc(sizeof(VarNode));
   constructInPlace(ret);
   ret->varName = varName;
   ret->arrayIndex = arrayIndex;
   return ret;
}
   
IntNode *IntNode::alloc(S32 value)
{
   IntNode *ret = (IntNode *) consoleAllocator.alloc(sizeof(IntNode));
   constructInPlace(ret);
   ret->value = value;
   return ret;
}
   
ConditionalExprNode *ConditionalExprNode::alloc(ExprNode *testExpr, ExprNode *trueExpr, ExprNode *falseExpr)
{
   ConditionalExprNode *ret = (ConditionalExprNode *) consoleAllocator.alloc(sizeof(ConditionalExprNode));
   constructInPlace(ret);
   ret->testExpr = testExpr;
   ret->trueExpr = trueExpr;
   ret->falseExpr = falseExpr;
   ret->integer = false;
   return ret;
}

FloatNode *FloatNode::alloc(F64 value)
{
   FloatNode *ret = (FloatNode *) consoleAllocator.alloc(sizeof(FloatNode));
   constructInPlace(ret);
   ret->value = value;
   return ret;
}
   
StrConstNode *StrConstNode::alloc(char *str, bool tag)
{
   StrConstNode *ret = (StrConstNode *) consoleAllocator.alloc(sizeof(StrConstNode));
   constructInPlace(ret);
   ret->str = (char *) consoleAllocator.alloc(dStrlen(str) + 1);
   ret->tag = tag;
   dStrcpy(ret->str, str);
   
   return ret;
}
   
ConstantNode *ConstantNode::alloc(StringTableEntry value)
{
   ConstantNode *ret = (ConstantNode *) consoleAllocator.alloc(sizeof(ConstantNode));
   constructInPlace(ret);
   ret->value = value;
   return ret;
}
   
AssignExprNode *AssignExprNode::alloc(StringTableEntry varName, ExprNode *arrayIndex, ExprNode *expr)
{
   AssignExprNode *ret = (AssignExprNode *) consoleAllocator.alloc(sizeof(AssignExprNode));
   constructInPlace(ret);
   ret->varName = varName;
   ret->expr = expr;
   ret->arrayIndex = arrayIndex;
   
   return ret;
}
   
AssignOpExprNode *AssignOpExprNode::alloc(StringTableEntry varName, ExprNode *arrayIndex, ExprNode *expr, S32 op)
{
   AssignOpExprNode *ret = (AssignOpExprNode *) consoleAllocator.alloc(sizeof(AssignOpExprNode));
   constructInPlace(ret);
   ret->varName = varName;
   ret->expr = expr;
   ret->arrayIndex = arrayIndex;
   ret->op = op;
   return ret;
}
   
TTagSetStmtNode *TTagSetStmtNode::alloc(StringTableEntry tag, ExprNode *valueExpr, ExprNode *stringExpr)
{
   TTagSetStmtNode *ret = (TTagSetStmtNode *) consoleAllocator.alloc(sizeof(TTagSetStmtNode));
   constructInPlace(ret);
   ret->tag = tag;
   ret->valueExpr = valueExpr;
   ret->stringExpr = stringExpr;
   return ret;
}
   
TTagDerefNode *TTagDerefNode::alloc(ExprNode *expr)
{
   TTagDerefNode *ret = (TTagDerefNode *) consoleAllocator.alloc(sizeof(TTagDerefNode));
   constructInPlace(ret);
   ret->expr = expr;
   return ret;
}
   
TTagExprNode *TTagExprNode::alloc(StringTableEntry tag)
{
   TTagExprNode *ret = (TTagExprNode *) consoleAllocator.alloc(sizeof(TTagExprNode));
   constructInPlace(ret);
   ret->tag = tag;
   return ret;
}
   
FuncCallExprNode *FuncCallExprNode::alloc(StringTableEntry funcName, StringTableEntry nameSpace, ExprNode *args, bool dot)
{
   FuncCallExprNode *ret = (FuncCallExprNode *) consoleAllocator.alloc(sizeof(FuncCallExprNode));
   constructInPlace(ret);
   ret->funcName = funcName;
   ret->nameSpace = nameSpace;
   ret->args = args;
   if(dot)
      ret->callType = MethodCall;
   else
   {
      if(nameSpace && !dStricmp(nameSpace, "Parent"))
         ret->callType = ParentCall;
      else
         ret->callType = FunctionCall;
   }
   return ret;
}
   
SlotAccessNode *SlotAccessNode::alloc(ExprNode *objectExpr, ExprNode *arrayExpr, StringTableEntry slotName)
{
   SlotAccessNode *ret = (SlotAccessNode *) consoleAllocator.alloc(sizeof(SlotAccessNode));
   constructInPlace(ret);
   ret->objectExpr = objectExpr;
   ret->arrayExpr = arrayExpr;
   ret->slotName = slotName;
   return ret;
}
   
SlotAssignNode *SlotAssignNode::alloc(ExprNode *objectExpr, ExprNode *arrayExpr, StringTableEntry slotName, ExprNode *valueExpr)
{
   SlotAssignNode *ret = (SlotAssignNode *) consoleAllocator.alloc(sizeof(SlotAssignNode));
   constructInPlace(ret);
   ret->objectExpr = objectExpr;
   ret->arrayExpr = arrayExpr;
   ret->slotName = slotName;
   ret->valueExpr = valueExpr;
   return ret;
}
   
SlotAssignOpNode *SlotAssignOpNode::alloc(ExprNode *objectExpr, StringTableEntry slotName, ExprNode *arrayExpr, S32 op, ExprNode *valueExpr)
{
   SlotAssignOpNode *ret = (SlotAssignOpNode *) consoleAllocator.alloc(sizeof(SlotAssignOpNode));
   constructInPlace(ret);
   ret->objectExpr = objectExpr;
   ret->arrayExpr = arrayExpr;
   ret->slotName = slotName;
   ret->op = op;
   ret->valueExpr = valueExpr;
   return ret;
}
   
ObjectDeclNode *ObjectDeclNode::alloc(ExprNode *classNameExpr, ExprNode *objectNameExpr, ExprNode *argList, StringTableEntry parentObject, SlotAssignNode *slotDecls, ObjectDeclNode *subObjects, bool structDecl)
{
   ObjectDeclNode *ret = (ObjectDeclNode *) consoleAllocator.alloc(sizeof(ObjectDeclNode));
   constructInPlace(ret);
   ret->classNameExpr = classNameExpr;
   ret->objectNameExpr = objectNameExpr;
   ret->argList = argList;
   ret->slotDecls = slotDecls;
   ret->subObjects = subObjects;
   ret->structDecl = structDecl;
   if(parentObject)
      ret->parentObject = parentObject;
   else
      ret->parentObject = StringTable->insert("");
   return ret;
}
   
FunctionDeclStmtNode *FunctionDeclStmtNode::alloc(StringTableEntry fnName, StringTableEntry nameSpace, VarNode *args, StmtNode *stmts)
{
   FunctionDeclStmtNode *ret = (FunctionDeclStmtNode *) consoleAllocator.alloc(sizeof(FunctionDeclStmtNode));
   constructInPlace(ret);
   ret->fnName = fnName;
   ret->args = args;
   ret->stmts = stmts;
   ret->nameSpace = nameSpace;
   ret->package = NULL;
   return ret;
}

void FunctionDeclStmtNode::setPackage(StringTableEntry packageName)
{
   package = packageName;
}

//------------------------------------------------------------
//
// Console language compilers
//
//------------------------------------------------------------

static U32 conversionOp(TypeReq src, TypeReq dst)
{
   if(src == TypeReqString)
   {
      switch(dst)
      {
         case TypeReqUInt:
            return OP_STR_TO_UINT;
         case TypeReqFloat:
            return OP_STR_TO_FLT;
         case TypeReqNone:
            return OP_STR_TO_NONE;
      }
   }
   else if(src == TypeReqFloat)
   {
      switch(dst)
      {
         case TypeReqUInt:
            return OP_FLT_TO_UINT;
         case TypeReqString:
            return OP_FLT_TO_STR;
         case TypeReqNone:
            return OP_FLT_TO_NONE;
      }
   }
   else if(src == TypeReqUInt)
   {
      switch(dst)
      {
         case TypeReqFloat:
            return OP_UINT_TO_FLT;
         case TypeReqString:
            return OP_UINT_TO_STR;
         case TypeReqNone:
            return OP_UINT_TO_NONE;
      }
   }
   return OP_INVALID;
}

//------------------------------------------------------------

U32 BreakStmtNode::precompileStmt(U32 loopCount)
{
   if(loopCount)
   {
      addBreakCount();
      return 2;
   }
   Con::warnf(ConsoleLogEntry::General, "%s (%d): break outside of loop... ignoring.", dbgFileName, dbgLineNumber);
   return 0;
}

U32 BreakStmtNode::compileStmt(U32 *codeStream, U32 ip, U32, U32 breakPoint)
{
   if(breakPoint)
   {
      addBreakLine(ip);
      codeStream[ip++] = OP_JMP;
      codeStream[ip++] = breakPoint;
   }
   return ip;
}

//------------------------------------------------------------

U32 ContinueStmtNode::precompileStmt(U32 loopCount)
{
   if(loopCount)
   {
      addBreakCount();
      return 2;
   }
   Con::warnf(ConsoleLogEntry::General, "%s (%d): continue outside of loop... ignoring.", dbgFileName, dbgLineNumber);
   return 0;
}

U32 ContinueStmtNode::compileStmt(U32 *codeStream, U32 ip, U32 continuePoint, U32)
{
   if(continuePoint)
   {
      addBreakLine(ip);
      codeStream[ip++] = OP_JMP;
      codeStream[ip++] = continuePoint;
   }
   return ip;
}

//------------------------------------------------------------

U32 ExprNode::precompileStmt(U32)
{
   addBreakCount();
   return precompile(TypeReqNone);
}

U32 ExprNode::compileStmt(U32 *codeStream, U32 ip, U32, U32)
{
   addBreakLine(ip);
   return compile(codeStream, ip, TypeReqNone);
}

//------------------------------------------------------------

U32 ReturnStmtNode::precompileStmt(U32)
{
   addBreakCount();
   if(!expr)
      return 1;
   else
      return 1 + expr->precompile(TypeReqString);
}

U32 ReturnStmtNode::compileStmt(U32 *codeStream, U32 ip, U32, U32)
{
   addBreakLine(ip);
   if(!expr)
      codeStream[ip++] = OP_RETURN;
   else
   {
      ip = expr->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_RETURN;
   }
   return ip;
}

//------------------------------------------------------------

ExprNode *IfStmtNode::getSwitchOR(ExprNode *left, ExprNode *list, bool string)
{
   ExprNode *nextExpr = (ExprNode *) list->getNext();
   ExprNode *test;
   if(string)
      test = StreqExprNode::alloc(left, list, true);
   else
      test = IntBinaryExprNode::alloc(opEQ, left, list);
   if(!nextExpr)
      return test;
   return IntBinaryExprNode::alloc(opOR, test, getSwitchOR(left, nextExpr, string));
}

void IfStmtNode::propagateSwitchExpr(ExprNode *left, bool string)
{
   testExpr = getSwitchOR(left, testExpr, string);
   if(propagate && elseBlock)
      ((IfStmtNode *) elseBlock)->propagateSwitchExpr(left, string);
}

U32 IfStmtNode::precompileStmt(U32 loopCount)
{
   U32 exprSize;
   addBreakCount();
   
   if(testExpr->getPreferredType() == TypeReqUInt)
   {
      exprSize = testExpr->precompile(TypeReqUInt);
      integer = true;
   }
   else
   {
      exprSize = testExpr->precompile(TypeReqFloat);
      integer = false;
   }
   // next is the JMPIFNOT or JMPIFFNOT - size of 2
   U32 ifSize = precompileBlock(ifBlock, loopCount);
   if(!elseBlock)
      endifOffset = ifSize + 2 + exprSize;
   else
   {
      elseOffset = exprSize + 2 + ifSize + 2;
      U32 elseSize = precompileBlock(elseBlock, loopCount);
      endifOffset = elseOffset + elseSize;
   }
   return endifOffset;
}

U32 IfStmtNode::compileStmt(U32 *codeStream, U32 ip, U32 continuePoint, U32 breakPoint)
{
   U32 start = ip;
   addBreakLine(ip);

   ip = testExpr->compile(codeStream, ip, integer ? TypeReqUInt : TypeReqFloat);
   codeStream[ip++] = integer ? OP_JMPIFNOT : OP_JMPIFFNOT;
   
   if(elseBlock)
   {
      codeStream[ip++] = start + elseOffset;
      ip = compileBlock(ifBlock, codeStream, ip, continuePoint, breakPoint);
      codeStream[ip++] = OP_JMP;
      codeStream[ip++] = start + endifOffset;
      ip = compileBlock(elseBlock, codeStream, ip, continuePoint, breakPoint);
   }
   else
   {
      codeStream[ip++] = start + endifOffset;
      ip = compileBlock(ifBlock, codeStream, ip, continuePoint, breakPoint);
   }
   return ip;
}

//------------------------------------------------------------

U32 LoopStmtNode::precompileStmt(U32 loopCount)
{
   U32 initSize = 0;
   addBreakCount();
   if(initExpr)
      initSize = initExpr->precompile(TypeReqNone);
   U32 testSize;
   if(testExpr->getPreferredType() == TypeReqUInt)
   {
      integer = true;
      testSize = testExpr->precompile(TypeReqUInt);
   }
   else
   {
      integer = false;
      testSize = testExpr->precompile(TypeReqFloat);
   }
   U32 blockSize = precompileBlock(loopBlock, loopCount + 1);
   U32 endLoopSize = 0;
   if(endLoopExpr)
      endLoopSize = endLoopExpr->precompile(TypeReqNone);
   
   // if it's a for loop or a while loop it goes:
   // initExpr
   // testExpr
   // OP_JMPIFNOT to break point
   // loopStartPoint:
   // loopBlock
   // continuePoint:
   // endLoopExpr
   // testExpr
   // OP_JMPIF loopStartPoint
   // breakPoint:
   
   // otherwise if it's a do ... while() it goes:
   // initExpr
   // loopStartPoint:
   // loopBlock
   // continuePoint:
   // endLoopExpr
   // testExpr
   // OP_JMPIF loopStartPoint
   // breakPoint:
   
   if(!isDoLoop)
   {
      loopBlockStartOffset = initSize + testSize + 2;
      continueOffset = loopBlockStartOffset + blockSize;
      breakOffset = continueOffset + endLoopSize + testSize + 2;
   }
   else
   {
      loopBlockStartOffset = initSize;
      continueOffset = initSize + blockSize;
      breakOffset = continueOffset + endLoopSize + testSize + 2;
   }
   return breakOffset;
}

U32 LoopStmtNode::compileStmt(U32 *codeStream, U32 ip, U32, U32)
{
   addBreakLine(ip);
   U32 start = ip;
   if(initExpr)
      ip = initExpr->compile(codeStream, ip, TypeReqNone);
   if(!isDoLoop)
   {
      ip = testExpr->compile(codeStream, ip, integer ? TypeReqUInt : TypeReqFloat);
      codeStream[ip++] = integer ? OP_JMPIFNOT : OP_JMPIFFNOT;
      codeStream[ip++] = start + breakOffset;
   }
   ip = compileBlock(loopBlock, codeStream, ip, start + continueOffset, start + breakOffset);
   if(endLoopExpr)
      ip = endLoopExpr->compile(codeStream, ip, TypeReqNone);
   ip = testExpr->compile(codeStream, ip, integer ? TypeReqUInt : TypeReqFloat);
   codeStream[ip++] = integer ? OP_JMPIF : OP_JMPIFF;
   codeStream[ip++] = start + loopBlockStartOffset;
   return ip;
}

//------------------------------------------------------------

U32 ConditionalExprNode::precompile(TypeReq type)
{
   // code is testExpr
   // JMPIFNOT falseStart
   // trueExpr
   // JMP end
   // falseExpr
   U32 exprSize;
   
   if(testExpr->getPreferredType() == TypeReqUInt)
   {
      exprSize = testExpr->precompile(TypeReqUInt);
      integer = true;
   }
   else
   {
      exprSize = testExpr->precompile(TypeReqFloat);
      integer = false;
   }
   return exprSize + 
          trueExpr->precompile(type) +
          falseExpr->precompile(type) + 4;
}

U32 ConditionalExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = testExpr->compile(codeStream, ip, integer ? TypeReqUInt : TypeReqFloat);
   codeStream[ip++] = integer ? OP_JMPIFNOT : OP_JMPIFFNOT;
   U32 jumpElseIp = ip++;
   ip = trueExpr->compile(codeStream, ip, type);
   codeStream[ip++] = OP_JMP;
   U32 jumpEndIp = ip++;
   codeStream[jumpElseIp] = ip;
   ip = falseExpr->compile(codeStream, ip, type);
   codeStream[jumpEndIp] = ip;
   return ip;
}

TypeReq ConditionalExprNode::getPreferredType()
{
   return trueExpr->getPreferredType();
}

//------------------------------------------------------------

U32 FloatBinaryExprNode::precompile(TypeReq type)
{
   U32 addSize = left->precompile(TypeReqFloat) + right->precompile(TypeReqFloat) + 1;
   if(type != TypeReqFloat)
      addSize++;
   
   return addSize;
}

U32 FloatBinaryExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = right->compile(codeStream, ip, TypeReqFloat);
   ip = left->compile(codeStream, ip, TypeReqFloat);
   U32 operand = OP_INVALID;
   switch(op)
   {
      case '+':
         operand = OP_ADD;
         break;
      case '-':
         operand = OP_SUB;
         break;
      case '/':
         operand = OP_DIV;
         break;
      case '*':
         operand = OP_MUL;
         break;
   }
   codeStream[ip++] = operand;
   if(type != TypeReqFloat)
      codeStream[ip++] =conversionOp(TypeReqFloat, type);
   return ip;
}

TypeReq FloatBinaryExprNode::getPreferredType()
{
   return TypeReqFloat;
}

//------------------------------------------------------------

void IntBinaryExprNode::getSubTypeOperand()
{
   subType = TypeReqUInt;
   switch(op)
   {
      case '^':
         operand = OP_XOR;
         break;
      case '%':
         operand = OP_MOD;
         break;
      case '&':
         operand = OP_BITAND;
         break;
      case '|':
         operand = OP_BITOR;
         break;
      case '<':
         operand = OP_CMPLT;
         subType = TypeReqFloat;
         break;
      case '>':
         operand = OP_CMPGR;
         subType = TypeReqFloat;
         break;
      case opGE:
         operand = OP_CMPGE;
         subType = TypeReqFloat;
         break;
      case opLE:
         operand = OP_CMPLE;
         subType = TypeReqFloat;
         break;
      case opEQ:
         operand = OP_CMPEQ;
         subType = TypeReqFloat;
         break;
      case opNE:
         operand = OP_CMPNE;
         subType = TypeReqFloat;
         break;
      case opOR:
         operand = OP_OR;
         break;
      case opAND:
         operand = OP_AND;
         break;
      case opSHR:
         operand = OP_SHR;
         break;
      case opSHL:
         operand = OP_SHL;
         break;
   }
}

U32 IntBinaryExprNode::precompile(TypeReq type)
{
   getSubTypeOperand();
   U32 addSize = left->precompile(subType) + right->precompile(subType) + 1;
   if(operand == OP_OR || operand == OP_AND)
      addSize++;
      
   if(type != TypeReqUInt)
      addSize++;
   
   return addSize;
}

U32 IntBinaryExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   if(operand == OP_OR || operand == OP_AND)
   {
      ip = left->compile(codeStream, ip, subType);
      codeStream[ip++] = operand == OP_OR ? OP_JMPIF_NP : OP_JMPIFNOT_NP;
      U32 jmpIp = ip++;
      ip = right->compile(codeStream, ip, subType);
      codeStream[jmpIp] = ip;
   }
   else
   {
      ip = right->compile(codeStream, ip, subType);
      ip = left->compile(codeStream, ip, subType);
      codeStream[ip++] = operand;
   }
   if(type != TypeReqUInt)
      codeStream[ip++] =conversionOp(TypeReqUInt, type);
   return ip;
}

TypeReq IntBinaryExprNode::getPreferredType()
{
   return TypeReqUInt;
}

//------------------------------------------------------------

U32 StreqExprNode::precompile(TypeReq type)
{
   // eval str left
   // OP_ADVANCE_STR_NUL
   // eval str right
   // OP_COMPARE_STR
   // optional conversion
   U32 addSize = left->precompile(TypeReqString) + right->precompile(TypeReqString) + 2;
   if(!eq)
      addSize ++;
   if(type != TypeReqUInt)
      addSize ++;
   return addSize;
}

U32 StreqExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = left->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_ADVANCE_STR_NUL;
   ip = right->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_COMPARE_STR;
   if(!eq)
      codeStream[ip++] = OP_NOT;
   if(type != TypeReqUInt)
      codeStream[ip++] = conversionOp(TypeReqUInt, type);
   return ip;
}

TypeReq StreqExprNode::getPreferredType()
{
   return TypeReqUInt;
}

//------------------------------------------------------------
                                 
U32 StrcatExprNode::precompile(TypeReq type)
{
   U32 addSize = left->precompile(TypeReqString) + right->precompile(TypeReqString) + 2;
   if(appendChar)
      addSize++;
      
   if(type != TypeReqString)
      addSize ++;
   return addSize;
}

U32 StrcatExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = left->compile(codeStream, ip, TypeReqString);
   if(!appendChar)
      codeStream[ip++] = OP_ADVANCE_STR;
   else
   {
      codeStream[ip++] = OP_ADVANCE_STR_APPENDCHAR;
      codeStream[ip++] = appendChar;
   }
   ip = right->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_REWIND_STR;
   if(type == TypeReqUInt)
      codeStream[ip++] = OP_STR_TO_UINT;
   else if(type == TypeReqFloat)
      codeStream[ip++] = OP_STR_TO_FLT;
   return ip;
}

TypeReq StrcatExprNode::getPreferredType()
{
   return TypeReqString;
}

//------------------------------------------------------------

U32 CommaCatExprNode::precompile(TypeReq type)
{
   U32 addSize = left->precompile(TypeReqString) + right->precompile(TypeReqString) + 2;
   if(type != TypeReqString)
      addSize ++;
   return addSize;
}

U32 CommaCatExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = left->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_ADVANCE_STR_COMMA;
   ip = right->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_REWIND_STR;
   if(type == TypeReqUInt || type == TypeReqFloat)
      Con::warnf(ConsoleLogEntry::General, "%s (%d): converting comma string to a number... probably wrong.", dbgFileName, dbgLineNumber);
   if(type == TypeReqUInt)
      codeStream[ip++] = OP_STR_TO_UINT;
   else if(type == TypeReqFloat)
      codeStream[ip++] = OP_STR_TO_FLT;
   return ip;
}

TypeReq CommaCatExprNode::getPreferredType()
{
   return TypeReqString;
}

//------------------------------------------------------------

U32 IntUnaryExprNode::precompile(TypeReq type)
{
   integer = true;
   TypeReq prefType = expr->getPreferredType();
   if(op == '!' && prefType == TypeReqFloat || prefType == TypeReqString)
      integer = false;
   
   U32 exprSize = expr->precompile(integer ? TypeReqUInt : TypeReqFloat);
   if(type != TypeReqUInt)
      return exprSize + 2;
   else
      return exprSize + 1;
}

U32 IntUnaryExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = expr->compile(codeStream, ip, integer ? TypeReqUInt : TypeReqFloat);
   if(op == '!')
      codeStream[ip++] = integer ? OP_NOT : OP_NOTF;
   else if(op == '~')
      codeStream[ip++] = OP_ONESCOMPLEMENT;
   if(type != TypeReqUInt)
      codeStream[ip++] =conversionOp(TypeReqUInt, type);
   return ip;
}

TypeReq IntUnaryExprNode::getPreferredType()
{
   return TypeReqUInt;
}

//------------------------------------------------------------

U32 FloatUnaryExprNode::precompile(TypeReq type)
{
   U32 exprSize = expr->precompile(TypeReqFloat);
   if(type != TypeReqFloat)
      return exprSize + 2;
   else
      return exprSize + 1;
}

U32 FloatUnaryExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = expr->compile(codeStream, ip, TypeReqFloat);
   codeStream[ip++] = OP_NEG;
   if(type != TypeReqFloat)
      codeStream[ip++] =conversionOp(TypeReqFloat, type);
   return ip;
}

TypeReq FloatUnaryExprNode::getPreferredType()
{
   return TypeReqFloat;
}

//------------------------------------------------------------

U32 VarNode::precompile(TypeReq type)
{
   // if this has an arrayIndex...
   // OP_LOADIMMED_IDENT
   // varName
   // OP_ADVANCE_STR
   // evaluate arrayIndex TypeReqString
   // OP_REWIND_STR
   // OP_SETCURVAR_ARRAY
   // OP_LOADVAR (type)
   
   // else
   // OP_SETCURVAR
   // varName
   // OP_LOADVAR (type)
   if(type == TypeReqNone)
      return 0;

   precompileIdent(varName);
   if(arrayIndex)
      return arrayIndex->precompile(TypeReqString) + 6;
   else
      return 3;
}

U32 VarNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   if(type == TypeReqNone)
      return ip;
      
   codeStream[ip++] = arrayIndex ? OP_LOADIMMED_IDENT : OP_SETCURVAR;
   codeStream[ip] = STEtoU32(varName, ip);
   ip++;
   if(arrayIndex)
   {
      codeStream[ip++] = OP_ADVANCE_STR;
      ip = arrayIndex->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_REWIND_STR;
      codeStream[ip++] = OP_SETCURVAR_ARRAY;
   }
   switch(type)
   {
      case TypeReqUInt:
         codeStream[ip++] = OP_LOADVAR_UINT;
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_LOADVAR_FLT;
         break;
      case TypeReqString:
         codeStream[ip++] = OP_LOADVAR_STR;
         break;
   }
   return ip;
}

TypeReq VarNode::getPreferredType()
{
   return TypeReqNone; // no preferred type
}

//------------------------------------------------------------

U32 IntNode::precompile(TypeReq type)
{
   if(type == TypeReqNone)
      return 0;
   if(type == TypeReqString)
      index = currentStringTable->addIntString(value);
   else if(type == TypeReqFloat)
      index = currentFloatTable->add(value);
   return 2;
}

U32 IntNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   switch(type)
   {
      case TypeReqUInt:
         codeStream[ip++] = OP_LOADIMMED_UINT;
         codeStream[ip++] = value;
         break;
      case TypeReqString:
         codeStream[ip++] = OP_LOADIMMED_STR;
         codeStream[ip++] = index;
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_LOADIMMED_FLT;
         codeStream[ip++] = index;
         break;
   }
   return ip;
}

TypeReq IntNode::getPreferredType()
{
   return TypeReqUInt;
}

//------------------------------------------------------------

U32 FloatNode::precompile(TypeReq type)
{
   if(type == TypeReqNone)
      return 0;
   if(type == TypeReqString)
      index = currentStringTable->addFloatString(value);
   else if(type == TypeReqFloat)
      index = currentFloatTable->add(value);
   return 2;
}

U32 FloatNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   switch(type)
   {
      case TypeReqUInt:
         codeStream[ip++] = OP_LOADIMMED_UINT;
         codeStream[ip++] = U32(value);
         break;
      case TypeReqString:
         codeStream[ip++] = OP_LOADIMMED_STR;
         codeStream[ip++] = index;
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_LOADIMMED_FLT;
         codeStream[ip++] = index;
         break;
   }
   return ip;
}

TypeReq FloatNode::getPreferredType()
{
   return TypeReqFloat;
}

//------------------------------------------------------------

U32 StrConstNode::precompile(TypeReq type)
{
   if(type == TypeReqString)
   {
      index = currentStringTable->add(str, true, tag);
      return 2;
   }
   else if(type == TypeReqNone)
      return 0;

   fVal = consoleStringToNumber(str, dbgFileName, dbgLineNumber);
   if(type == TypeReqFloat)
      index = currentFloatTable->add(fVal);
   return 2;
}

U32 StrConstNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   switch(type)
   {
      case TypeReqString:
         codeStream[ip++] = tag ? OP_TAG_TO_STR : OP_LOADIMMED_STR;
         codeStream[ip++] = index;
         break;
      case TypeReqUInt:
         codeStream[ip++] = OP_LOADIMMED_UINT;
         codeStream[ip++] = U32(fVal);
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_LOADIMMED_FLT;
         codeStream[ip++] = index;
         break;      
   }
   return ip;
}

TypeReq StrConstNode::getPreferredType()
{
   return TypeReqString;
}

//------------------------------------------------------------

U32 ConstantNode::precompile(TypeReq type)
{
   if(type == TypeReqString)
   {
      precompileIdent(value);
      return 2;
   }
   else if(type == TypeReqNone)
      return 0;

   fVal = consoleStringToNumber(value, dbgFileName, dbgLineNumber);
   if(type == TypeReqFloat)
      index = currentFloatTable->add(fVal);
   return 2;
}

U32 ConstantNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   switch(type)
   {
      case TypeReqString:
         codeStream[ip++] = OP_LOADIMMED_IDENT;
         codeStream[ip] = STEtoU32(value, ip);
         ip++;
         break;
      case TypeReqUInt:
         codeStream[ip++] = OP_LOADIMMED_UINT;
         codeStream[ip++] = U32(fVal);
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_LOADIMMED_FLT;
         codeStream[ip++] = index;
         break;      
   }
   return ip;
}

TypeReq ConstantNode::getPreferredType()
{
   return TypeReqString;
}

//------------------------------------------------------------

U32 AssignExprNode::precompile(TypeReq type)
{
   subType = expr->getPreferredType();
   if(subType == TypeReqNone)
      subType = type;
   if(subType == TypeReqNone)
      subType = TypeReqString;
   // if it's an array expr, the formula is:
   // eval expr
   // (push and pop if it's TypeReqString) OP_ADVANCE_STR
   // OP_LOADIMMED_IDENT
   // varName
   // OP_ADVANCE_STR
   // eval array
   // OP_REWIND_STR
   // OP_SETCURVAR_ARRAY_CREATE
   // OP_TERMINATE_REWIND_STR
   // OP_SAVEVAR
   
   //else
   // eval expr
   // OP_SETCURVAR_CREATE
   // varname
   // OP_SAVEVAR
   U32 addSize = 0;
   if(type != subType)
      addSize = 1;
   
   U32 retSize = expr->precompile(subType);
   precompileIdent(varName);
   if(arrayIndex)
   {
      if(subType == TypeReqString)
         return arrayIndex->precompile(TypeReqString) + retSize + addSize + 8;
      else
         return arrayIndex->precompile(TypeReqString) + retSize + addSize + 6;
   }
   else
      return retSize + addSize + 3;
}

U32 AssignExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = expr->compile(codeStream, ip, subType);
   if(arrayIndex)
   {
      if(subType == TypeReqString)
         codeStream[ip++] = OP_ADVANCE_STR;
      codeStream[ip++] = OP_LOADIMMED_IDENT;
      codeStream[ip] = STEtoU32(varName, ip);
      ip++;
      codeStream[ip++] = OP_ADVANCE_STR;
      ip = arrayIndex->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_REWIND_STR;
      codeStream[ip++] = OP_SETCURVAR_ARRAY_CREATE;
      if(subType == TypeReqString)
         codeStream[ip++] = OP_TERMINATE_REWIND_STR;
   }
   else
   {
      codeStream[ip++] = OP_SETCURVAR_CREATE;
      codeStream[ip] = STEtoU32(varName, ip);
      ip++;
   }
   switch(subType)
   {
      case TypeReqString:
         codeStream[ip++] = OP_SAVEVAR_STR;
         break;
      case TypeReqUInt:
         codeStream[ip++] = OP_SAVEVAR_UINT;
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_SAVEVAR_FLT;
         break;
   }
   if(type != subType)
      codeStream[ip++] = conversionOp(subType, type);
   return ip;
}

TypeReq AssignExprNode::getPreferredType()
{
   return expr->getPreferredType();
}

//------------------------------------------------------------

static void getAssignOpTypeOp(S32 op, TypeReq &type, U32 &operand)
{
   switch(op)
   {
      case '+':
         type = TypeReqFloat;
         operand = OP_ADD;
         break;
      case '-':
         type = TypeReqFloat;
         operand = OP_SUB;
         break;
      case '*':
         type = TypeReqFloat;
         operand = OP_MUL;
         break;
      case '/':
         type = TypeReqFloat;
         operand = OP_DIV;
         break;
      case '%':
         type = TypeReqUInt;
         operand = OP_MOD;
         break;
      case '&':
         type = TypeReqUInt;
         operand = OP_BITAND;
         break;
      case '^':
         type = TypeReqUInt;
         operand = OP_XOR;
         break;
      case '|':
         type = TypeReqUInt;
         operand = OP_BITOR;
         break;
      case opSHL:
         type = TypeReqUInt;
         operand = OP_SHL;
         break;
      case opSHR:
         type = TypeReqUInt;
         operand = OP_SHR;
         break;
   }   
}

U32 AssignOpExprNode::precompile(TypeReq type)
{
   // goes like this...
   // eval expr as float or int
   // if there's an arrayIndex
   
   // OP_LOADIMMED_IDENT
   // varName
   // OP_ADVANCE_STR
   // eval arrayIndex stringwise
   // OP_REWIND_STR
   // OP_SETCURVAR_ARRAY_CREATE
   
   // else
   // OP_SETCURVAR_CREATE
   // varName
   
   // OP_LOADVAR_FLT or UINT
   // operand
   // OP_SAVEVAR_FLT or UINT
   
   // conversion OP if necessary.
   getAssignOpTypeOp(op, subType, operand);
   precompileIdent(varName);
   U32 size = expr->precompile(subType);
   if(type != subType)
      size++;
   if(!arrayIndex)
      return size + 5;
   else
   {
      size += arrayIndex->precompile(TypeReqString);
      return size + 8;
   }
}

U32 AssignOpExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = expr->compile(codeStream, ip, subType);
   if(!arrayIndex)
   {
      codeStream[ip++] = OP_SETCURVAR_CREATE;
      codeStream[ip] = STEtoU32(varName, ip);
      ip++;
   }
   else
   {
      codeStream[ip++] = OP_LOADIMMED_IDENT;
      codeStream[ip] = STEtoU32(varName, ip);
      ip++;
      codeStream[ip++] = OP_ADVANCE_STR;
      ip = arrayIndex->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_REWIND_STR;
      codeStream[ip++] = OP_SETCURVAR_ARRAY_CREATE;
   }
   codeStream[ip++] = (subType == TypeReqFloat) ? OP_LOADVAR_FLT : OP_LOADVAR_UINT;
   codeStream[ip++] = operand;
   codeStream[ip++] = (subType == TypeReqFloat) ? OP_SAVEVAR_FLT : OP_SAVEVAR_UINT;
   if(subType != type)
      codeStream[ip++] = conversionOp(subType, type);
   return ip;
}

TypeReq AssignOpExprNode::getPreferredType()
{
   getAssignOpTypeOp(op, subType, operand);
   return subType;
}

//------------------------------------------------------------

U32 TTagSetStmtNode::precompileStmt(U32 loopCount)
{
   loopCount;
   return 0;
}
   
U32 TTagSetStmtNode::compileStmt(U32*, U32 ip, U32, U32)
{
   return ip;
}

//------------------------------------------------------------

U32 TTagDerefNode::precompile(TypeReq)
{
   return 0;
}

U32 TTagDerefNode::compile(U32*, U32 ip, TypeReq)
{
   return ip;
}

TypeReq TTagDerefNode::getPreferredType()
{
   return TypeReqNone;
}

//------------------------------------------------------------

U32 TTagExprNode::precompile(TypeReq)
{
   return 0;
}

U32 TTagExprNode::compile(U32*, U32 ip, TypeReq)
{
   return ip;
}

TypeReq TTagExprNode::getPreferredType()
{
   return TypeReqNone;
}

//------------------------------------------------------------

U32 FuncCallExprNode::precompile(TypeReq type)
{
   // OP_PUSH_FRAME
   // arg OP_PUSH arg OP_PUSH arg OP_PUSH
   // eval all the args, then call the function.
   
   // OP_CALLFUNC
   // function
   // namespace
   // isDot
   
   U32 size = 0;
   if(type != TypeReqString)
      size++;
   precompileIdent(funcName);
   precompileIdent(nameSpace);
   for(ExprNode *walk = args; walk; walk = (ExprNode *) walk->getNext())
      size += walk->precompile(TypeReqString) + 1;
   return size + 5;
}

U32 FuncCallExprNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   codeStream[ip++] = OP_PUSH_FRAME;
   for(ExprNode *walk = args; walk; walk = (ExprNode *) walk->getNext())
   {
      ip = walk->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_PUSH;
   }
   if(callType == MethodCall || callType == ParentCall)
      codeStream[ip++] = OP_CALLFUNC;
   else
      codeStream[ip++] = OP_CALLFUNC_RESOLVE;
   
   codeStream[ip] = STEtoU32(funcName, ip);
   ip++;
   codeStream[ip] = STEtoU32(nameSpace, ip);
   ip++;
   codeStream[ip++] = callType;
   if(type != TypeReqString)
      codeStream[ip++] = conversionOp(TypeReqString, type);
   return ip;
}

TypeReq FuncCallExprNode::getPreferredType()
{
   return TypeReqString;
}

//------------------------------------------------------------

U32 SlotAccessNode::precompile(TypeReq type)
{
   if(type == TypeReqNone)
      return 0;
   U32 size = 0;
   precompileIdent(slotName);
   if(arrayExpr)
   {
      // eval array
      // OP_ADVANCE_STR
      // evaluate object expression sub (OP_SETCURFIELD)
      // OP_TERMINATE_REWIND_STR
      // OP_SETCURFIELDARRAY
      // total add of 4 + array precomp
      size += 3 + arrayExpr->precompile(TypeReqString);
   }
   // eval object expression sub + 3 (op_setCurField + OP_SETCUROBJECT)
   size += objectExpr->precompile(TypeReqString) + 3;
   
   // get field in desired type:
   return size + 1;
}

U32 SlotAccessNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   if(type == TypeReqNone)
      return ip;
      
   if(arrayExpr)
   {
      ip = arrayExpr->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_ADVANCE_STR;
   }
   ip = objectExpr->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_SETCUROBJECT;
   codeStream[ip++] = OP_SETCURFIELD;
   codeStream[ip] = STEtoU32(slotName, ip);
   ip++;
   if(arrayExpr)
   {
      codeStream[ip++] = OP_TERMINATE_REWIND_STR;
      codeStream[ip++] = OP_SETCURFIELD_ARRAY;
   }
   switch(type)
   {
      case TypeReqUInt:
         codeStream[ip++] = OP_LOADFIELD_UINT;
         break;
      case TypeReqFloat:
         codeStream[ip++] = OP_LOADFIELD_FLT;
         break;
      case TypeReqString:
         codeStream[ip++] = OP_LOADFIELD_STR;
         break;
   }
   return ip;
}

TypeReq SlotAccessNode::getPreferredType()
{
   return TypeReqNone;
}

//------------------------------------------------------------

U32 SlotAssignNode::precompile(TypeReq type)
{
   // first eval the expression TypeReqString
   
   // if it's an array:

   // if OP_ADVANCE_STR 1
   // eval array
   
   // OP_ADVANCE_STR 1
   // evaluate object expr
   // OP_SETCUROBJECT 1
   // OP_SETCURFIELD 1
   // fieldName 1
   // OP_TERMINATE_REWIND_STR 1

   // OP_SETCURFIELDARRAY 1
   // OP_TERMINATE_REWIND_STR 1
   
   // else
   // OP_ADVANCE_STR
   // evaluate object expr
   // OP_SETCUROBJECT
   // OP_SETCURFIELD
   // fieldName
   // OP_TERMINATE_REWIND_STR

   // OP_SAVEFIELD
   // convert to return type if necessary.
   
   U32 size = 0;
   if(type != TypeReqString)
      size++;

   precompileIdent(slotName);

   size += valueExpr->precompile(TypeReqString);
   
   if(objectExpr)
      size += objectExpr->precompile(TypeReqString) + 5;
   else
      size += 5;
   
   if(arrayExpr)
      size += arrayExpr->precompile(TypeReqString) + 3;
   return size + 1;
}

U32 SlotAssignNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = valueExpr->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_ADVANCE_STR;
   if(arrayExpr)
   {
      ip = arrayExpr->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_ADVANCE_STR;
   }
   if(objectExpr)
   {
      ip = objectExpr->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_SETCUROBJECT;
   }
   else
      codeStream[ip++] = OP_SETCUROBJECT_NEW;
   codeStream[ip++] = OP_SETCURFIELD;
   codeStream[ip] = STEtoU32(slotName, ip);
   ip++;
   if(arrayExpr)
   {
      codeStream[ip++] = OP_TERMINATE_REWIND_STR;
      codeStream[ip++] = OP_SETCURFIELD_ARRAY;
   }
   codeStream[ip++] = OP_TERMINATE_REWIND_STR;
   codeStream[ip++] = OP_SAVEFIELD_STR;
   if(type != TypeReqString)
      codeStream[ip++] = conversionOp(TypeReqString, type);
   return ip;
}

TypeReq SlotAssignNode::getPreferredType()
{
   return TypeReqString;
}

//------------------------------------------------------------

U32 SlotAssignOpNode::precompile(TypeReq type)
{
   // first eval the expression as its type
   
   // if it's an array:
   // eval array
   // OP_ADVANCE_STR
   // evaluate object expr
   // OP_SETCUROBJECT
   // OP_SETCURFIELD
   // fieldName
   // OP_TERMINATE_REWIND_STR
   // OP_SETCURFIELDARRAY
   
   // else
   // evaluate object expr
   // OP_SETCUROBJECT
   // OP_SETCURFIELD
   // fieldName

   // OP_LOADFIELD of appropriate type
   // operand
   // OP_SAVEFIELD of appropriate type
   // convert to return type if necessary.
   
   getAssignOpTypeOp(op, subType, operand);
   precompileIdent(slotName);
   U32 size = valueExpr->precompile(subType);
   if(type != subType)
      size++;
   if(arrayExpr)
      return size + 9 + arrayExpr->precompile(TypeReqString) + objectExpr->precompile(TypeReqString);
   else
      return size + 6 + objectExpr->precompile(TypeReqString);
}

U32 SlotAssignOpNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   ip = valueExpr->compile(codeStream, ip, subType);
   if(arrayExpr)
   {
      ip = arrayExpr->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_ADVANCE_STR;
   }
   ip = objectExpr->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_SETCUROBJECT;
   codeStream[ip++] = OP_SETCURFIELD;
   codeStream[ip] = STEtoU32(slotName, ip);
   ip++;
   if(arrayExpr)
   {
      codeStream[ip++] = OP_TERMINATE_REWIND_STR;
      codeStream[ip++] = OP_SETCURFIELD_ARRAY;
   }
   codeStream[ip++] = (subType == TypeReqFloat) ? OP_LOADFIELD_FLT : OP_LOADFIELD_UINT;
   codeStream[ip++] = operand;
   codeStream[ip++] = (subType == TypeReqFloat) ? OP_SAVEFIELD_FLT : OP_SAVEFIELD_UINT;
   if(subType != type)
      codeStream[ip++] = conversionOp(subType, type);
   return ip;
}

TypeReq SlotAssignOpNode::getPreferredType()
{
   getAssignOpTypeOp(op, subType, operand);
   return subType;
}
 
//------------------------------------------------------------

U32 ObjectDeclNode::precompileSubObject(bool)
{
   // goes

   // OP_PUSHFRAME 1
   // name expr
   // OP_PUSH 1
   // args... PUSH
   // OP_CREATE_OBJECT 1
   // className 1
   // datablock? 1
   // fail point 1

   // for each field, eval
   // OP_ADD_OBJECT (to UINT[0]) 1
   // root? 1
   
   // add all the sub objects.
   // OP_END_OBJECT 1
   // root? 1

   U32 argSize = 0;
   precompileIdent(parentObject);
   for(ExprNode *exprWalk = argList; exprWalk; exprWalk = (ExprNode *) exprWalk->getNext())
      argSize += exprWalk->precompile(TypeReqString) + 1;
   argSize += classNameExpr->precompile(TypeReqString) + 1;

   U32 nameSize = objectNameExpr->precompile(TypeReqString);

   U32 slotSize = 0;
   for(SlotAssignNode *slotWalk = slotDecls; slotWalk; slotWalk = (SlotAssignNode *) slotWalk->getNext())
      slotSize += slotWalk->precompile(TypeReqNone);
   
   // OP_ADD_OBJECT
   U32 subObjSize = 0;
   for(ObjectDeclNode *objectWalk = subObjects; objectWalk; objectWalk = (ObjectDeclNode *) objectWalk->getNext())
      subObjSize += objectWalk->precompileSubObject(false);

   failOffset = 10 + nameSize + argSize + slotSize + subObjSize;
   return failOffset;
}

U32 ObjectDeclNode::precompile(TypeReq type)
{
   // root object decl does:
   
   // push 0 onto the UINT stack OP_LOADIMMED_UINT
   // precompiles the subObject(true)
   // UINT stack now has object id
   // type conv to type
   
   U32 ret = 2 + precompileSubObject(true);
   if(type != TypeReqUInt)
      return ret + 1;
   return ret;
}

U32 ObjectDeclNode::compileSubObject(U32 *codeStream, U32 ip, bool root)
{
   U32 start = ip;
   codeStream[ip++] = OP_PUSH_FRAME;
   ip = classNameExpr->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_PUSH;

   ip = objectNameExpr->compile(codeStream, ip, TypeReqString);
   codeStream[ip++] = OP_PUSH;
   for(ExprNode *exprWalk = argList; exprWalk; exprWalk = (ExprNode *) exprWalk->getNext())
   {
      ip = exprWalk->compile(codeStream, ip, TypeReqString);
      codeStream[ip++] = OP_PUSH;
   }
   codeStream[ip++] = OP_CREATE_OBJECT;
   codeStream[ip] = STEtoU32(parentObject, ip);
   ip++;
   codeStream[ip++] = structDecl;
   codeStream[ip++] = start + failOffset;
   for(SlotAssignNode *slotWalk = slotDecls; slotWalk; slotWalk = (SlotAssignNode *) slotWalk->getNext())
      ip = slotWalk->compile(codeStream, ip, TypeReqNone);
   codeStream[ip++] = OP_ADD_OBJECT;
   codeStream[ip++] = root;
   for(ObjectDeclNode *objectWalk = subObjects; objectWalk; objectWalk = (ObjectDeclNode *) objectWalk->getNext())
      ip = objectWalk->compileSubObject(codeStream, ip, false);
   codeStream[ip++] = OP_END_OBJECT;
   codeStream[ip++] = root || structDecl;
   return ip;
}

U32 ObjectDeclNode::compile(U32 *codeStream, U32 ip, TypeReq type)
{
   codeStream[ip++] = OP_LOADIMMED_UINT;
   codeStream[ip++] = 0;
   ip = compileSubObject(codeStream, ip, true);
   if(type != TypeReqUInt)
      codeStream[ip++] = conversionOp(TypeReqUInt, type);
   return ip;
}   
TypeReq ObjectDeclNode::getPreferredType()
{
   return TypeReqUInt;
}

//------------------------------------------------------------

U32 FunctionDeclStmtNode::precompileStmt(U32)
{
   // OP_FUNC_DECL
   // func name
   // namespace
   // package
   // func end ip
   // argc
   // ident array[argc]
   // code
   // OP_RETURN
   currentStringTable = &gFunctionStringTable;
   currentFloatTable = &gFunctionFloatTable;
   argc = 0;
   for(VarNode *walk = args; walk; walk = (VarNode *)((StmtNode*)walk)->getNext())
      argc++;
   inFunction = true;
   precompileIdent(fnName);
   precompileIdent(nameSpace);
   precompileIdent(package);
   U32 subSize = precompileBlock(stmts, 0);
   inFunction = false;
   
   currentStringTable = &gGlobalStringTable;
   currentFloatTable = &gGlobalFloatTable;
   
   endOffset = argc + subSize + 8;
   return endOffset;
}

U32 FunctionDeclStmtNode::compileStmt(U32 *codeStream, U32 ip, U32, U32)
{
   U32 start = ip;
   codeStream[ip++] = OP_FUNC_DECL;
   codeStream[ip] = STEtoU32(fnName, ip);
   ip++;
   codeStream[ip] = STEtoU32(nameSpace, ip);
   ip++;
   codeStream[ip] = STEtoU32(package, ip);
   ip++;
   codeStream[ip++] = bool(stmts != NULL);
   codeStream[ip++] = start + endOffset;
   codeStream[ip++] = argc;
   for(VarNode *walk = args; walk; walk = (VarNode *)((StmtNode*)walk)->getNext())
   {
      codeStream[ip] = STEtoU32(walk->varName, ip);
      ip++;
   }
   inFunction = true;
   ip = compileBlock(stmts, codeStream, ip, 0, 0);
   inFunction = false;
   codeStream[ip++] = OP_RETURN;
   return ip;
}


