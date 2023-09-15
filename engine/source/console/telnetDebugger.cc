//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "console/telnetDebugger.h"
#include "platform/event.h"
#include "core/stringTable.h"
#include "console/consoleInternal.h"
#include "console/ast.h"
#include "console/compiler.h"
#include "platform/gameInterface.h"

//------------------------------------------------------------

// debugger commands:
// CEVAL console line - evaluate the console line
//    output: none
// BRKVARSET varName passct expr
//    output: none
// BRKVARCLR varName
//    output: none
// BRKSET file line clear passct expr - set a breakpoint on the file,line
//        it must pass passct times for it to break and if clear is true, it
//        clears when hit
//    output: none
// BRKCLR file line - clear a breakpoint on the file,line
//    output: none
// BRKCLRALL - clear all breakpoints
//    output: none
// CONTINUE - continue execution
//    output: RUNNING
// STEPIN - run until next statement
//    output: RUNNING
// STEPOVER - run until next break <= current frame
//    output: RUNNING
// STEPOUT - run until next break <= current frame - 1
//    output: RUNNING
// EVAL tag frame expr - evaluate the expr in the console, on the frame'th stack frame
//    output: EVALOUT tag exprResult 
// FILELIST - list script files loaded
//    output: FILELISTOUT file1 file2 file3 file4 ...
// BREAKLIST file - get a list of breakpoint-able lines in the file
//    output: BREAKLISTOUT file skipBreakPairs skiplinecount breaklinecount skiplinecount breaklinecount ...
//
// other output:
//
// when the debugger hits a breakpoint, it lists out:
// BREAK file1 line1 fn1 file2 line2 fn2 file3 line3 fn3 file4 line4 fn4 etc.
//       where file1 line1 fn1 ... etc is the current call stack.
// COUT echo out a console line

ConsoleFunction( dbgSetParameters, void, 3, 3, "(int port, string password)"
                "Open a debug server port on the specified port, requiring the specified password.")
{
   if (TelDebugger)
      TelDebugger->setDebugParameters(dAtoi(argv[1]), argv[2]);
}

static void debuggerConsumer(ConsoleLogEntry::Level level, const char *line)
{
   level;
   if (TelDebugger)
      TelDebugger->processConsoleLine(line);
}

TelnetDebugger::TelnetDebugger()
{
   Con::addConsumer(debuggerConsumer);
   
   mAcceptPort = -1;
   mAcceptSocket = InvalidSocket;
   mDebugSocket = InvalidSocket;
   
   mState = NotConnected;
   mCurPos = 0;

   mBreakpoints = NULL;
   mBreakOnNextStatement = false;
   mStackPopBreakIndex = -1;
   mProgramPaused = false;
}

TelnetDebugger::Breakpoint **TelnetDebugger::findBreakpoint(StringTableEntry fileName, S32 lineNumber)
{
   Breakpoint **walk = &mBreakpoints;
   Breakpoint *cur;
   while((cur = *walk) != NULL)
   {
      if(cur->code->name == fileName && cur->lineNumber == U32(lineNumber))
         return walk;
      walk = &cur->next;
   }
   return NULL;
}


TelnetDebugger::~TelnetDebugger()
{
   Con::removeConsumer(debuggerConsumer);
   
   if(mAcceptSocket != InvalidSocket)
      Net::closeSocket(mAcceptSocket);
   if(mDebugSocket != InvalidSocket)
      Net::closeSocket(mDebugSocket);
}

TelnetDebugger *TelDebugger = NULL;

void TelnetDebugger::create()
{
   TelDebugger = new TelnetDebugger;
}

void TelnetDebugger::destroy()
{
   delete TelDebugger;
   TelDebugger = NULL;
}

void TelnetDebugger::send(const char *str)
{
   Net::send(mDebugSocket, (const unsigned char*)str, dStrlen(str));
}

void TelnetDebugger::setDebugParameters(S32 port, const char *password)
{
   // Don't bail if same port... we might just be wanting to change
   // the password.
//   if(port == mAcceptPort)
//      return;
   
   if(mAcceptSocket != InvalidSocket)
   {
      Net::closeSocket(mAcceptSocket);
      mAcceptSocket = InvalidSocket;
   }
   mAcceptPort = port;
   if(mAcceptPort != -1 && mAcceptPort != 0)
   {
      mAcceptSocket = Net::openSocket();
      Net::bind(mAcceptSocket, mAcceptPort);
      Net::listen(mAcceptSocket, 4);
      
      Net::setBlocking(mAcceptSocket, false);
   }
   dStrncpy(mDebuggerPassword, password, PasswordMaxLength);
}

void TelnetDebugger::processConsoleLine(const char *consoleLine)
{
   if(mState == Connected)
   {
      send("COUT ");
      send(consoleLine);
      send("\r\n");
   }
}

void TelnetDebugger::process()
{
   NetAddress address;

   if(mAcceptSocket != InvalidSocket)
   {
      // ok, see if we have any new connections:
      NetSocket newConnection;
      newConnection = Net::accept(mAcceptSocket, &address);

      if(newConnection != InvalidSocket && mDebugSocket == InvalidSocket)
      {
   		Con::printf ("Debugger connection from %i.%i.%i.%i",
   				address.netNum[0], address.netNum[1], address.netNum[2], address.netNum[3]);

         mState = PasswordTry;
         mDebugSocket = newConnection;
         
         Net::setBlocking(newConnection, false);
      }
      else if(newConnection != InvalidSocket)
         Net::closeSocket(newConnection);
   }
   // see if we have any input to process...
   
   if(mDebugSocket == InvalidSocket)
      return;

   checkDebugRecv();
   if(mDebugSocket == InvalidSocket)
      removeAllBreakpoints();
}

void TelnetDebugger::checkDebugRecv()
{   
   S32 checked = false;
   for(;;) {
      // check for and recv one command:
      for(S32 i = 0; i < mCurPos; i++)
      {
         if(mLineBuffer[i] == '\r' || mLineBuffer[i] == '\n')
         {
            if(i == 0)
            {
               mCurPos--;
               dMemmove(mLineBuffer, mLineBuffer + 1, mCurPos);
            }
            else
            {
               mLineBuffer[i] = '\n';
               processLineBuffer(i+1);
               mCurPos -= i + 1;
               dMemmove(mLineBuffer, mLineBuffer + i + 1, mCurPos);
               return;
            }
         }
         else if(mLineBuffer[i] == 0)
            mLineBuffer[i] = '_';
      }
      // found no <CR> or <LF>
      if(mCurPos == MaxCommandSize) // this shouldn't happen
      {
         Net::closeSocket(mDebugSocket);
         mDebugSocket = InvalidSocket;
         mState = NotConnected;
         return;
      }
      if(checked)
         return;
      checked = false;
   
      S32 numBytes;
      Net::Error err = Net::recv(mDebugSocket, (unsigned char*)(mLineBuffer + mCurPos), MaxCommandSize - mCurPos, &numBytes);
      
      if((err != Net::NoError && err != Net::WouldBlock) || numBytes == 0)
      {
         Net::closeSocket(mDebugSocket);
         mDebugSocket = InvalidSocket;
         mState = NotConnected;
         return;
      }
      if(err == Net::WouldBlock)
         return;

      mCurPos += numBytes;
   }
}

void TelnetDebugger::executionStopped(CodeBlock *code, U32 lineNumber)
{
   if(mProgramPaused)
      return;
   if(mBreakOnNextStatement)
   {
      // loop through all the codeblocks clearing the breaks
      for(CodeBlock *walk = codeBlockList; walk; walk = walk->nextFile)
         walk->clearAllBreaks();
      for(Breakpoint *w = mBreakpoints; w; w = w->next)
         w->code->setBreakpoint(w->lineNumber);
      breakProcess();
      return;
   }
   Breakpoint **bp = findBreakpoint(code->name, lineNumber);
   if(!bp)
      return;
   Breakpoint *brk = *bp;
   mProgramPaused = true;
   Con::evaluatef("$dbgResult = %s;", brk->testExpression);
   if(Con::getBoolVariable("$dbgResult"))
   {
      brk->curCount++;
      if(brk->curCount >= brk->passCount)
      {
         brk->curCount = 0;
         if(brk->clearOnHit)
            removeBreakpoint(code->name, lineNumber);
         breakProcess();
      }
   }
   mProgramPaused = false;
}

void TelnetDebugger::popStackFrame()
{
   if(mState == NotConnected)
      return;
   if(U32(mStackPopBreakIndex) == gEvalState.stack.size())
      breakOnNextStatement();
}

void TelnetDebugger::breakProcess()
{
   mProgramPaused = true;
   // echo out the break
   send("BREAK");
   char buffer[MaxCommandSize];
   
   for(S32 i = (S32) gEvalState.stack.size() - 1; i >= 0; i--)
   {
      CodeBlock *code = gEvalState.stack[i]->code;
      U32 ip = gEvalState.stack[i]->ip;
      
      const char *file = code->name;
      const char *scope = gEvalState.stack[i]->scopeName;
      if ((! file) || (! file[0]))
         file = "N/A";
      if ((! scope) || (! scope[0]))
         scope = "N/A";
      U32 line, inst;
      code->findBreakLine(ip, line, inst);
      dSprintf(buffer, MaxCommandSize, " %s %d %s", file, line, scope);
      send(buffer);
   }
   send("\r\n");
   while(mProgramPaused)
   {
      checkDebugRecv();
      if(mDebugSocket == InvalidSocket)
      {
         mProgramPaused = false;
         removeAllBreakpoints();
         debugContinue();
         return;
      }
   }
}

void TelnetDebugger::processLineBuffer(S32 cmdLen)
{   
   if (mState == PasswordTry)
   {
      if(dStrncmp(mLineBuffer, mDebuggerPassword, cmdLen-1))
      {
         // failed password:
         send("PASS WrongPassword.\r\n");
         Net::closeSocket(mDebugSocket);
         mDebugSocket = InvalidSocket;
         mState = NotConnected;
      }
      else
      {
         send("PASS Connected.\r\n");
         mState = Connected;
      }
   }
   else
   {
      char evalBuffer[MaxCommandSize];
      char varBuffer[MaxCommandSize];
      char fileBuffer[MaxCommandSize];
      char clear[MaxCommandSize];
      S32 passCount, line, frame;
      
      if(dSscanf(mLineBuffer, "CEVAL %[^\n]", evalBuffer) == 1)
      {
         ConsoleEvent postEvent;
         dStrcpy(postEvent.data, evalBuffer);
         postEvent.size = ConsoleEventHeaderSize + dStrlen(evalBuffer) + 1;
         Game->postEvent(postEvent);
      }
      else if(dSscanf(mLineBuffer, "BRKVARSET %s %d %[^\n]", varBuffer, &passCount, evalBuffer) == 3)
         addVariableBreakpoint(varBuffer, passCount, evalBuffer);
      else if(dSscanf(mLineBuffer, "BRKVARCLR %s", varBuffer) == 1)
         removeVariableBreakpoint(varBuffer);
      else if(dSscanf(mLineBuffer, "BRKSET %s %d %s %d %[^\n]", fileBuffer,&line,&clear,&passCount,evalBuffer) == 5)
         addBreakpoint(fileBuffer, line, dAtob(clear), passCount, evalBuffer);
      else if(dSscanf(mLineBuffer, "BRKCLR %s %d", fileBuffer, &line) == 2)
         removeBreakpoint(fileBuffer, line);
      else if(!dStrncmp(mLineBuffer, "BRKCLRALL\n", cmdLen))
         removeAllBreakpoints();
      else if(!dStrncmp(mLineBuffer, "CONTINUE\n", cmdLen))
         debugContinue();
      else if(!dStrncmp(mLineBuffer, "STEPIN\n", cmdLen))
         debugStepIn();
      else if(!dStrncmp(mLineBuffer, "STEPOVER\n", cmdLen))
         debugStepOver();
      else if(!dStrncmp(mLineBuffer, "STEPOUT\n", cmdLen))
         debugStepOut();
      else if(dSscanf(mLineBuffer, "EVAL %s %d %[^\n]", varBuffer, &frame, evalBuffer) == 3)
         evaluateExpression(varBuffer, frame, evalBuffer);
      else if(!dStrncmp(mLineBuffer, "FILELIST\n", cmdLen))
         dumpFileList();
      else if(dSscanf(mLineBuffer, "BREAKLIST %s", fileBuffer) == 1)
         dumpBreakableList(fileBuffer);
      else
      {
         // invalid stuff.
         send("DBGERR Invalid command!\r\n");
      }
   }
}

void TelnetDebugger::addVariableBreakpoint(const char*, S32, const char*)
{
   send("addVariableBreakpoint\r\n");
}

void TelnetDebugger::removeVariableBreakpoint(const char*)
{
   send("removeVariableBreakpoint\r\n");
}

void TelnetDebugger::addBreakpoint(const char *fileName, S32 line, bool clear, S32 passCount, const char *evalString)
{
   fileName = StringTable->insert(fileName);
   Breakpoint **bp = findBreakpoint(fileName, line);
   
   if(bp)
   {
      // trying to add the same breakpoint...
      Breakpoint *brk = *bp;
      dFree(brk->testExpression);
      brk->testExpression = dStrdup(evalString);
      brk->passCount = passCount;
      brk->clearOnHit = clear;
      brk->curCount = 0;
   }
   else
   {
      CodeBlock *code = CodeBlock::find(fileName);
      if(code)
      {
         Breakpoint *brk = new Breakpoint;
         brk->code = code;
         code->setBreakpoint(line);
         brk->lineNumber = line;
         brk->passCount = passCount;
         brk->clearOnHit = clear;
         brk->curCount = 0;
         brk->testExpression = dStrdup(evalString);
         brk->next = mBreakpoints;
         mBreakpoints = brk;
      }
   }
}

void TelnetDebugger::removeBreakpointsFromCode(CodeBlock *code)
{
   Breakpoint **walk = &mBreakpoints;
   Breakpoint *cur;
   while((cur = *walk) != NULL)
   {
      if(cur->code == code)
      {
         dFree(cur->testExpression);
         *walk = cur->next;
         delete walk;
      }
      else
         walk = &cur->next;
   }
}

void TelnetDebugger::removeBreakpoint(const char *fileName, S32 line)
{
   fileName = StringTable->insert(fileName);
   Breakpoint **bp = findBreakpoint(fileName, line);
   if(bp)
   {
      Breakpoint *brk = *bp;
      *bp = brk->next;
      brk->code->clearBreakpoint(brk->lineNumber);
      dFree(brk->testExpression);
      delete brk;
   }
}

void TelnetDebugger::removeAllBreakpoints()
{
   Breakpoint *walk = mBreakpoints;
   while(walk)
   {
      Breakpoint *temp = walk->next;
      walk->code->clearBreakpoint(walk->lineNumber);
      dFree(walk->testExpression);
      delete walk;
      walk = temp;
   }
   mBreakpoints = NULL;
}

void TelnetDebugger::debugContinue()
{
   mBreakOnNextStatement = false;
   mStackPopBreakIndex = -1;
   mProgramPaused = false;
   send("RUNNING\r\n");
}

void TelnetDebugger::breakOnNextStatement()
{
   for(CodeBlock *walk = codeBlockList; walk; walk = walk->nextFile)
      walk->setAllBreaks();
   mBreakOnNextStatement = true;
}

void TelnetDebugger::debugStepIn()
{
   breakOnNextStatement();
   mStackPopBreakIndex = -1;
   mProgramPaused = false;
   send("RUNNING\r\n");
}

void TelnetDebugger::debugStepOver()
{
   mBreakOnNextStatement = false;
   mStackPopBreakIndex = gEvalState.stack.size();
   mProgramPaused = false;
   send("RUNNING\r\n");
}

void TelnetDebugger::debugStepOut()
{
   mBreakOnNextStatement = false;
   mStackPopBreakIndex = gEvalState.stack.size() - 1;
   mProgramPaused = false;
   send("RUNNING\r\n");
}

void TelnetDebugger::evaluateExpression(const char *tag, S32, const char *evalBuffer)
{
   char buffer[MaxCommandSize];
   Con::evaluatef("$dbgResult = %s;", evalBuffer);
   const char *result = Con::getVariable("$dbgResult");
   dSprintf(buffer, MaxCommandSize, "EVALOUT %s %s\r\n", tag, result[0] ? result : "\"\"");
   send(buffer);
}

void TelnetDebugger::dumpFileList()
{
   send("FILELISTOUT ");
   for(CodeBlock *walk = codeBlockList; walk; walk = walk->nextFile)
   {
      send(walk->name);
      if(walk->nextFile)
         send(" ");
   }
   send("\r\n");
}

void TelnetDebugger::dumpBreakableList(const char *fileName)
{
   fileName = StringTable->insert(fileName);
   CodeBlock *file = CodeBlock::find(fileName);
   char buffer[MaxCommandSize];
   if(file)
   {
      dSprintf(buffer, MaxCommandSize, "BREAKLISTOUT %s %d", fileName, file->breakListSize >> 1);
      send(buffer);
      for(U32 i = 0; i < file->breakListSize; i += 2)
      {
         dSprintf(buffer, MaxCommandSize, " %d %d", file->breakList[i], file->breakList[i+1]);
         send(buffer);      
      }
      send("\r\n");
   }
   else
      send("DBGERR No Such file!");
}
