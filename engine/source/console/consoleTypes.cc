//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/stringTable.h"
#include "core/color.h"
#include "console/simBase.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static const char *getDataTypeString(void *dptr, EnumTable *, BitSet32)
{
   return *((const char **)(dptr));
}

static void setDataTypeString(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
      *((const char **) dptr) = StringTable->insert(argv[0]);
   else
      Con::printf("(TypeString) Cannot set multiple args to a single string.");
}

static void setDataTypeCaseString(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
      *((const char **) dptr) = StringTable->insert(argv[0], true);
   else
      Con::printf("(TypeCaseString) Cannot set multiple args to a single string.");
}

static void setDataTypeFilename(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
   {
      char buffer[1024];
      if (Con::expandScriptFilename(buffer, 1024, argv[0]))
         *((const char **) dptr) = StringTable->insert(buffer);
      else
         Con::warnf("(TypeFilename) illegal filename detected: %s", argv[0]);
   }
   else
      Con::printf("(TypeFilename) Cannot set multiple args to a single filename.");
}

static const char *getDataTypeCharArray(void *dptr, EnumTable *, BitSet32)
{
   return (const char *)(dptr);
}

//----------------------------------------------------------------------------

static const char *getDataTypeU8(void *dptr, EnumTable *, BitSet32)
{
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d", *((U8 *) dptr) );
   return returnBuffer;
}

static void setDataTypeU8(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
      *((U8 *) dptr) = dAtoi(argv[0]);
   else
      Con::printf("(TypeU8) Cannot set multiple args to a single S32.");
}

static const char *getDataTypeS32(void *dptr, EnumTable *, BitSet32)
{
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d", *((S32 *) dptr) );
   return returnBuffer;
}

static void setDataTypeS32(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
      *((S32 *) dptr) = dAtoi(argv[0]);
   else
      Con::printf("(TypeS32) Cannot set multiple args to a single S32.");
}

//-----------------------------------------------------------------------------
static const char *getDataTypeS32Vector(void *dptr, EnumTable *, BitSet32)
{
   Vector<S32> *vec = (Vector<S32> *)dptr;
   char* returnBuffer = Con::getReturnBuffer(1024);
   S32 maxReturn = 1024;
   returnBuffer[0] = '\0';
   S32 returnLeng = 0;
   for (Vector<S32>::iterator itr = vec->begin(); itr != vec->end(); itr++)
   {
      // concatenate the next value onto the return string
      dSprintf(returnBuffer + returnLeng, maxReturn - returnLeng, "%d ", *itr);
      // update the length of the return string (so far)
      returnLeng = dStrlen(returnBuffer);
   }
   // trim off that last extra space
   if (returnLeng > 0 && returnBuffer[returnLeng - 1] == ' ')
      returnBuffer[returnLeng - 1] = '\0';
   return returnBuffer;
}

static void setDataTypeS32Vector(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   Vector<S32> *vec = (Vector<S32> *)dptr;
   // we assume the vector should be cleared first (not just appending)
   vec->clear();
   if(argc == 1)
   {
      const char *values = argv[0];
      const char *endValues = values + dStrlen(values);
      S32 value;
      // advance through the string, pulling off S32's and advancing the pointer
      while (values < endValues && dSscanf(values, "%d", &value) != 0)
      {
         vec->push_back(value);
         const char *nextValues = dStrchr(values, ' ');
         if (nextValues != 0 && nextValues < endValues)
            values = nextValues + 1;
         else
            break;
      }
   }
   else if (argc > 1)
   {
      for (S32 i = 0; i < argc; i++)
         vec->push_back(dAtoi(argv[i]));
   }
   else
      Con::printf("Vector<S32> must be set as { a, b, c, ... } or \"a b c ...\"");
}

//-----------------------------------------------------------------------------
static const char *getDataTypeF32(void *dptr, EnumTable *, BitSet32)
{
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%g", *((F32 *) dptr) );
   return returnBuffer;
}

static void setDataTypeF32(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
      *((F32 *) dptr) = dAtof(argv[0]);
   else
      Con::printf("(TypeF32) Cannot set multiple args to a single F32.");
}

//-----------------------------------------------------------------------------
static const char *getDataTypeF32Vector(void *dptr, EnumTable *, BitSet32)
{
   Vector<F32> *vec = (Vector<F32> *)dptr;
   char* returnBuffer = Con::getReturnBuffer(1024);
   S32 maxReturn = 1024;
   returnBuffer[0] = '\0';
   S32 returnLeng = 0;
   for (Vector<F32>::iterator itr = vec->begin(); itr != vec->end(); itr++)
   {
      // concatenate the next value onto the return string
      dSprintf(returnBuffer + returnLeng, maxReturn - returnLeng, "%f ", *itr);
      // update the length of the return string (so far)
      returnLeng = dStrlen(returnBuffer);
   }
   // trim off that last extra space
   if (returnLeng > 0 && returnBuffer[returnLeng - 1] == ' ')
      returnBuffer[returnLeng - 1] = '\0';
   return returnBuffer;
}

static void setDataTypeF32Vector(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   Vector<F32> *vec = (Vector<F32> *)dptr;
   // we assume the vector should be cleared first (not just appending)
   vec->clear();
   if(argc == 1)
   {
      const char *values = argv[0];
      const char *endValues = values + dStrlen(values);
      F32 value;
      // advance through the string, pulling off F32's and advancing the pointer
      while (values < endValues && dSscanf(values, "%f", &value) != 0)
      {
         vec->push_back(value);
         const char *nextValues = dStrchr(values, ' ');
         if (nextValues != 0 && nextValues < endValues)
            values = nextValues + 1;
         else
            break;
      }
   }
   else if (argc > 1)
   {
      for (S32 i = 0; i < argc; i++)
         vec->push_back(dAtof(argv[i]));
   }
   else
      Con::printf("Vector<F32> must be set as { a, b, c, ... } or \"a b c ...\"");
}

//-----------------------------------------------------------------------------
static const char *getDataTypeBool(void *dptr, EnumTable *, BitSet32)
{
   return *((bool *) dptr) ? "1" : "0";
}

static void setDataTypeBool(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
      *((bool *) dptr) = dAtob(argv[0]);
   else
      Con::printf("(TypeBool) Cannot set multiple args to a single bool.");
}

static const char *getDataTypeBoolVector(void *dptr, EnumTable *, BitSet32)
{
   Vector<bool> *vec = (Vector<bool>*)dptr;
   char* returnBuffer = Con::getReturnBuffer(1024);
   S32 maxReturn = 1024;
   returnBuffer[0] = '\0';
   S32 returnLeng = 0;
   for (Vector<bool>::iterator itr = vec->begin(); itr < vec->end(); itr++)
   {
      // concatenate the next value onto the return string
      dSprintf(returnBuffer + returnLeng, maxReturn - returnLeng, "%d ", (*itr == true ? 1 : 0));
      returnLeng = dStrlen(returnBuffer);
   }
   // trim off that last extra space
   if (returnLeng > 0 && returnBuffer[returnLeng - 1] == ' ')
      returnBuffer[returnLeng - 1] = '\0';
   return(returnBuffer);
}

static void setDataTypeBoolVector(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   Vector<bool> *vec = (Vector<bool>*)dptr;
   // we assume the vector should be cleared first (not just appending)
   vec->clear();
   if (argc == 1)
   {
      const char *values = argv[0];
      const char *endValues = values + dStrlen(values);
      S32 value;
      // advance through the string, pulling off bool's and advancing the pointer
      while (values < endValues && dSscanf(values, "%d", &value) != 0)
      {
         vec->push_back(value == 0 ? false : true);
         const char *nextValues = dStrchr(values, ' ');
         if (nextValues != 0 && nextValues < endValues)
            values = nextValues + 1;
         else
            break;
      }
   }
   else if (argc > 1)
   {
      for (S32 i = 0; i < argc; i++)
         vec->push_back(dAtob(argv[i]));
   }
   else
      Con::printf("Vector<bool> must be set as { a, b, c, ... } or \"a b c ...\"");
}

static const char *getDataTypeEnum(void *dptr, EnumTable *tbl, BitSet32)
{
   AssertFatal(tbl, "Null enum table passed to getDataTypeEnum()");
   S32 dptrVal = *(S32*)dptr;
   for (S32 i = 0; i < tbl->size; i++)
   {
      if (dptrVal == tbl->table[i].index)
      {
         return tbl->table[i].label;
      }
   }
   
   //not found
   return "";
} 

static void setDataTypeEnum(void *dptr, S32 argc, const char **argv, EnumTable *tbl, BitSet32)
{
   AssertFatal(tbl, "Null enum table passed to setDataTypeEnum()");
   if (argc != 1) return;
   
   S32 val = 0;
   for (S32 i = 0; i < tbl->size; i++)
   {
      if (! dStricmp(argv[0], tbl->table[i].label))
      {
         val = tbl->table[i].index;
         break;
      }
   }
   *((S32 *) dptr) = val;
}

static const char *getDataTypeFlag(void *dptr, EnumTable *, BitSet32 flag)
{
   BitSet32 tempFlags = *(BitSet32 *)dptr;
   if (tempFlags.test(flag)) return "true";
   else return "false";
}

static void setDataTypeFlag(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 flag)
{
   bool value = true;
   if (argc != 1)
   {
      Con::printf("flag must be true or false");
   }
   else
   {
      value = dAtob(argv[0]);
   }
   ((BitSet32 *)dptr)->set(flag, value);
}

static const char *getDataTypeColorF(void *dptr, EnumTable *, BitSet32)
{
   ColorF * color = (ColorF*)dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%f %f %f %f", color->red, color->green, color->blue, color->alpha);
   return(returnBuffer);
}

static void setDataTypeColorF(void *dptr, S32 argc, const char ** argv, EnumTable *, BitSet32)
{
   ColorF *tmpColor = (ColorF *) dptr;
   if(argc == 1)
   {
      tmpColor->set(0, 0, 0, 1);
      F32 r,g,b,a;
      S32 args = dSscanf(argv[0], "%f %f %f %f", &r, &g, &b, &a);
      tmpColor->red = r;
      tmpColor->green = g;
      tmpColor->blue = b;
      if (args == 4)
         tmpColor->alpha = a;
   }
   else if(argc == 3)
   {
      tmpColor->red    = dAtof(argv[0]);
      tmpColor->green  = dAtof(argv[1]);
      tmpColor->blue   = dAtof(argv[2]);
      tmpColor->alpha  = 1.f;
   }
   else if(argc == 4)
   {
      tmpColor->red    = dAtof(argv[0]);
      tmpColor->green  = dAtof(argv[1]);
      tmpColor->blue   = dAtof(argv[2]);
      tmpColor->alpha  = dAtof(argv[3]);
   }
   else
      Con::printf("Color must be set as { r, g, b [,a] }");
}

static const char *getDataTypeColorI(void *dptr, EnumTable *, BitSet32)
{
   ColorI *color = (ColorI *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d %d %d %d", color->red, color->green, color->blue, color->alpha);
   return returnBuffer;
}

static void setDataTypeColorI(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   ColorI *tmpColor = (ColorI *) dptr;
   if(argc == 1)
   {
      tmpColor->set(0, 0, 0, 255);
      S32 r,g,b,a;
      S32 args = dSscanf(argv[0], "%d %d %d %d", &r, &g, &b, &a);
      tmpColor->red = r;
      tmpColor->green = g;
      tmpColor->blue = b;
      if (args == 4)
         tmpColor->alpha = a;
   }
   else if(argc == 3)
   {
      tmpColor->red    = dAtoi(argv[0]);
      tmpColor->green  = dAtoi(argv[1]);
      tmpColor->blue   = dAtoi(argv[2]);
      tmpColor->alpha  = 255;
   }
   else if(argc == 4)
   {
      tmpColor->red    = dAtoi(argv[0]);
      tmpColor->green  = dAtoi(argv[1]);
      tmpColor->blue   = dAtoi(argv[2]);
      tmpColor->alpha  = dAtoi(argv[3]);
   }
   else
      Con::printf("Color must be set as { r, g, b [,a] }");
}

static void setDataTypeSimObjectPtr(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   if(argc == 1)
   {
      SimObject **obj = (SimObject **)dptr;
      *obj = Sim::findObject(argv[0]);
   }
   else
      Con::printf("(TypeSimObjectPtr) Cannot set multiple args to a single S32.");
}

static const char *getDataTypeSimObjectPtr(void *dptr, EnumTable *, BitSet32)
{
   SimObject **obj = (SimObject**)dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%s", *obj ? (*obj)->getName() : "");
   return returnBuffer;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void RegisterCoreTypes(void)
{
   Con::registerType("char",        TypeS8,           sizeof(U8),             getDataTypeU8,             setDataTypeU8);
   Con::registerType("int",         TypeS32,          sizeof(S32),            getDataTypeS32,            setDataTypeS32);
   Con::registerType("intList",     TypeS32Vector,    sizeof(Vector<S32>),    getDataTypeS32Vector,      setDataTypeS32Vector);
   Con::registerType("bool",        TypeBool,         sizeof(bool),           getDataTypeBool,           setDataTypeBool);
   Con::registerType("boolList",    TypeBoolVector,   sizeof(Vector<bool>),   getDataTypeBoolVector,     setDataTypeBoolVector);
   Con::registerType("float",       TypeF32,          sizeof(F32),            getDataTypeF32,            setDataTypeF32);
   Con::registerType("floatList",   TypeF32Vector,    sizeof(Vector<F32>),    getDataTypeF32Vector,      setDataTypeF32Vector);
   Con::registerType("string",      TypeString,       sizeof(const char *),   getDataTypeString,         setDataTypeString);
   Con::registerType("caseString",  TypeCaseString,   sizeof(const char *),   getDataTypeString,         setDataTypeCaseString);
   Con::registerType("filename",    TypeFilename,     sizeof(const char *),   getDataTypeString,         setDataTypeFilename);
   Con::registerType("enumval",     TypeEnum,         sizeof(S32),            getDataTypeEnum,           setDataTypeEnum);
	Con::registerType("flag",        TypeFlag,         sizeof(S32),            getDataTypeFlag,           setDataTypeFlag);
   Con::registerType("ColorI",      TypeColorI,       sizeof(ColorI),         getDataTypeColorI,         setDataTypeColorI);
   Con::registerType("ColorF",      TypeColorF,       sizeof(ColorF),         getDataTypeColorF,         setDataTypeColorF);
   Con::registerType("SimObjectPtr",TypeSimObjectPtr, sizeof(SimObject*),     getDataTypeSimObjectPtr,   setDataTypeSimObjectPtr);
}

