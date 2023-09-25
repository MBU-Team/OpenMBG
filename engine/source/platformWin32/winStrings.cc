//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"

#ifdef HAS_VSSCANF
#  undef HAS_VSSCANF
#endif

#if defined(TORQUE_COMPILER_CODEWARRIOR)
#  define HAS_VSSCANF
#  define __vsscanf vsscanf
#endif

#if defined(TORQUE_COMPILER_GCC)
#  include <stdlib.h>
#  include <ctype.h>
#endif


char *dStrdup_r(const char *src, const char *fileName, dsize_t lineNumber)
{
   char *buffer = (char *) dMalloc_r(dStrlen(src) + 1, fileName, lineNumber);
   dStrcpy(buffer, src);
   return buffer;
}

char* dStrcat(char *dst, const char *src)
{
   return strcat(dst,src);
}   

char* dStrncat(char *dst, const char *src, dsize_t len)
{
   return strncat(dst,src,len);
}

// concatenates a list of src's onto the end of dst
// the list of src's MUST be terminated by a NULL parameter
// dStrcatl(dst, sizeof(dst), src1, src2, NULL);
char* dStrcatl(char *dst, dsize_t dstSize, ...)
{
   const char* src;
   char *p = dst;

   AssertFatal(dstSize > 0, "dStrcatl: destination size is set zero");
   dstSize--;  // leave room for string termination

   // find end of dst
   while (dstSize && *p++)                    
      dstSize--;   
   
   va_list args;
   va_start(args, dstSize);

   // concatenate each src to end of dst
   while ( (src = va_arg(args, const char*)) != NULL )
      while( dstSize && *src )
      {
         *p++ = *src++;
         dstSize--;   
      }

   va_end(args);

   // make sure the string is terminated 
   *p = 0;

   return dst;
}   


// copy a list of src's into dst
// the list of src's MUST be terminated by a NULL parameter
// dStrccpyl(dst, sizeof(dst), src1, src2, NULL);
char* dStrcpyl(char *dst, dsize_t dstSize, ...)
{
   const char* src;
   char *p = dst;

   AssertFatal(dstSize > 0, "dStrcpyl: destination size is set zero");
   dstSize--;  // leave room for string termination

   va_list args;
   va_start(args, dstSize);

   // concatenate each src to end of dst
   while ( (src = va_arg(args, const char*)) != NULL )
      while( dstSize && *src )
      {
         *p++ = *src++;
         dstSize--;   
      }

   va_end(args);

   // make sure the string is terminated 
   *p = 0;

   return dst;
}   


int dStrcmp(const char *str1, const char *str2)
{
   return strcmp(str1, str2);   
}  
 
int dStricmp(const char *str1, const char *str2)
{
   return stricmp(str1, str2);   
}  

int dStrncmp(const char *str1, const char *str2, dsize_t len)
{
   return strncmp(str1, str2, len);   
}  
 
int dStrnicmp(const char *str1, const char *str2, dsize_t len)
{
   return strnicmp(str1, str2, len);   
}   

char* dStrcpy(char *dst, const char *src)
{
   return strcpy(dst,src);
}   

char* dStrncpy(char *dst, const char *src, dsize_t len)
{
   return strncpy(dst,src,len);
}   

dsize_t dStrlen(const char *str)
{
   return (dsize_t)strlen(str);
}   


char* dStrupr(char *str)
{
#if defined(TORQUE_COMPILER_CODEWARRIOR)
   // metrowerks strupr is broken
   _strupr(str);
   return str;
#else
   return strupr(str);
#endif
}   


char* dStrlwr(char *str)
{
   return strlwr(str);
}   


char* dStrchr(char *str, S32 c)
{
   return strchr(str,c);
}   


const char* dStrchr(const char *str, S32 c)
{
   return strchr(str,c);
}   


const char* dStrrchr(const char *str, S32 c)
{
   return strrchr(str,c);
}   

char* dStrrchr(char *str, S32 c)
{
   return strrchr(str,c);
}   

dsize_t dStrspn(const char *str, const char *set)
{
   return (dsize_t)strspn(str, set);
}

dsize_t dStrcspn(const char *str, const char *set)
{
   return (dsize_t)strcspn(str, set);
}   


char* dStrstr(const char *str1, const char *str2)
{
   return (char*)strstr(str1,str2);
}   


char* dStrtok(char *str, const char *sep)
{
   return strtok(str, sep);
}


S32 dAtoi(const char *str)
{
   return atoi(str);   
}  

F32 dAtof(const char *str)
{
   // Warning: metrowerks crashes when strange strings are passed in '0x [enter]' for example!
   return (F32)atof(str);
}   

bool dAtob(const char *str)
{
   return !dStricmp(str, "true") || dAtof(str);
}   


bool dIsalnum(const char c)
{
   return isalnum(c)!=0;
}

bool dIsalpha(const char c)
{
   return isalpha(c)!=0;
}

bool dIsspace(const char c)
{
   return isspace(c)!=0;
}

bool dIsdigit(const char c)
{
   return isdigit(c)!=0;
}

void dPrintf(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vprintf(format, args);
}   

S32 dVprintf(const char *format, void *arglist)
{
   S32 len = vprintf(format, (char*)arglist);
   return (len);
}   

S32 dSprintf(char *buffer, dsize_t bufferSize, const char *format, ...)
{
   va_list args;
   va_start(args, format);

#if defined(TORQUE_COMPILER_CODEWARRIOR)
   S32 len = vsnprintf(buffer, bufferSize, format, args);
#else
   bufferSize;
   S32 len = vsprintf(buffer, format, args);
#endif
   return (len);
}   


S32 dVsprintf(char *buffer, dsize_t bufferSize, const char *format, void *arglist)
{
#if defined(TORQUE_COMPILER_CODEWARRIOR)
   S32 len = vsnprintf(buffer, bufferSize, format, (char*)arglist);
#else
   bufferSize;
   S32 len = vsprintf(buffer, format, (char*)arglist);
#endif
//   S32 len = vsnprintf(buffer, bufferSize, format, (char*)arglist);
   return (len);
}   


S32 dSscanf(const char *buffer, const char *format, ...)
{
   va_list args;
#if defined(HAS_VSSCANF)
   va_start(args, format);
   return __vsscanf(buffer, format, args);   
#else
   va_start(args, format);

   // Boy is this lame.  We have to scan through the format string, and find out how many
   //  arguments there are.  We'll store them off as void*, and pass them to the sscanf
   //  function through specialized calls.  We're going to have to put a cap on the number of args that
   //  can be passed, 8 for the moment.  Sigh.
   static void* sVarArgs[20];
   U32 numArgs = 0;

   for (const char* search = format; *search != '\0'; search++) {
      if (search[0] == '%' && search[1] != '%')
         numArgs++;
   }
   AssertFatal(numArgs <= 20, "Error, too many arguments to lame implementation of dSscanf.  Fix implmentation");

   // Ok, we have the number of arguments...
   for (U32 i = 0; i < numArgs; i++)
      sVarArgs[i] = va_arg(args, void*);
   va_end(args);

   switch (numArgs) {
     case 0: return 0;
     case 1:  return sscanf(buffer, format, sVarArgs[0]);
     case 2:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1]);
     case 3:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2]);
     case 4:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3]);
     case 5:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4]);
     case 6:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5]);
     case 7:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6]);
     case 8:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7]);
     case 9:  return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8]);
     case 10: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9]);
     case 11: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10]);
     case 12: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11]);
     case 13: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12]);
     case 14: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13]);
     case 15: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14]);
     case 16: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15]);
     case 17: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16]);
     case 18: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16], sVarArgs[17]);
     case 19: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16], sVarArgs[17], sVarArgs[18]);
     case 20: return sscanf(buffer, format, sVarArgs[0], sVarArgs[1], sVarArgs[2], sVarArgs[3], sVarArgs[4], sVarArgs[5], sVarArgs[6], sVarArgs[7], sVarArgs[8], sVarArgs[9], sVarArgs[10], sVarArgs[11], sVarArgs[12], sVarArgs[13], sVarArgs[14], sVarArgs[15], sVarArgs[16], sVarArgs[17], sVarArgs[18], sVarArgs[19]);
   }
   return 0;
#endif
}   

S32 dFflushStdout()
{
   return fflush(stdout);
}

S32 dFflushStderr()
{
   return fflush(stderr);
}

void dQsort(void *base, U32 nelem, U32 width, S32 (QSORT_CALLBACK *fcmp)(const void *, const void *))
{
   qsort(base, nelem, width, fcmp);
}   
