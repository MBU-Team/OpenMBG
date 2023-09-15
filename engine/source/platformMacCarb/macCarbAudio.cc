//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformMacCarb/platformMacCarb.h"
#include "console/console.h"
#include "platform/platformAL.h"

#include <al/altypes.h>
#include <al/alctypes.h>

// Define the AL Stub functions
#define AL_FUNCTION(fn_return, fn_name, fn_args, fn_value) fn_return stub_##fn_name fn_args{ fn_value }
#include <al/al_func.h>
#include <al/alc_func.h>
#undef AL_FUNCTION


// Declare the AL Function pointers
// And initialize them to the stub functions
#define AL_FUNCTION(fn_return,fn_name,fn_args, fn_value) fn_return (*fn_name)fn_args = stub_##fn_name;
#include <al/al_func.h>
#include <al/alc_func.h>
#undef AL_FUNCTION

#if defined(TORQUE_OS_MAC_CARB)
#if defined(TORQUE_OS_MAC_OSX)
#include <CoreFoundation/CFBundle.h>
#else
#include <CFBundle.h>
#endif
extern OSStatus LoadFrameworkBundle(CFStringRef framework, CFBundleRef *bundlePtr);
static CFBundleRef alBundle = NULL;
#endif

#include <CodeFragments.h>
static CFragConnectionID  fragOpenAL = NULL;

// DLL's: ------------------------------------------------------------------
static bool findExtension( const char* name )
{
   bool result = false;
   if (alGetString != NULL)
   {
      result = dStrstr( (const char*)alGetString(AL_EXTENSIONS), name) != NULL;
   }
   return result;
}

static bool bindFunction( void *&fnAddress, const char *name )
{
   OSErr err = noErr;
#if defined(TORQUE_OS_MAC_CARB)
   if (platState.osX)
   {
      CFStringRef str = CFStringCreateWithCString(NULL, name, kCFStringEncodingMacRoman);
      fnAddress = CFBundleGetFunctionPointerForName( alBundle, str );
      if (fnAddress == NULL)
         err = cfragNoSymbolErr;
      CFRelease(str);
   }
   else
#endif
   {
      err = FindSymbol(fragOpenAL, str2p(name), (char**)&fnAddress, NULL);
   }
   
   if (err != noErr || fnAddress == NULL)
      Con::errorf(ConsoleLogEntry::General, " Missing OpenAL function '%s'", name);
   return (fnAddress != NULL);
}

static void bindExtension( void *&fnAddress, const char *name, bool &supported )
{
   if (supported)
   {
      bindFunction(fnAddress, name);
      if (fnAddress == NULL)
         supported = NULL;
   }
   else
      fnAddress = NULL;
}

/*!   Bind the functions in the OpenAL DLL to the al interface functions  
*/
static bool bindDLLFunctions()
{
   bool result = true;
   #define AL_FUNCTION(fn_return, fn_name, fn_args, fn_value) result &= bindFunction( *(void**)&fn_name, #fn_name);
   #include <al/al_func.h>
   #include <al/alc_func.h>
   #undef AL_FUNCTION
   return result;
}

/*!   Bind the stub functions to the al interface functions  
*/
static void bindStubFunctions()
{
   #define AL_FUNCTION(fn_return, fn_name, fn_args, fn_value) fn_name = stub_##fn_name;
   #include <al/al_func.h>
   #include <al/alc_func.h>
   #undef AL_FUNCTION
}

namespace Audio
{

/*!   Shutdown and Unload the OpenAL DLL  
*/
void OpenALDLLShutdown()
{
   if (fragOpenAL)
      CloseConnection(&fragOpenAL);
   fragOpenAL = NULL;
   
   bindStubFunctions();   
}   

/*!   Dynamically Loads the OpenAL DLL if present and binds all the functions.
      If there is no DLL or an unexpected error occurs binding functions the
      stub functions are automatically bound.
*/
bool OpenALDLLInit()
{
   OSErr err = noErr;
   
   OpenALDLLShutdown();

#if defined(TORQUE_OS_MAC_CARB)
   if (platState.osX)
   {
      err = LoadFrameworkBundle(CFSTR("OpenAL.framework"), &alBundle);
   }
   else
#endif
   {
      err = GetSharedLibrary ("\pOpenALLibrary",
                              kAnyCFragArch, // as we want whatever form it is in
                              kReferenceCFrag, // the new name for the kLoadLib flag
                              &fragOpenAL, // holder of codefrag connection identifier 
                              NULL, // we don't care
                              NULL); // we don't care
      if (fragOpenAL==NULL)
         return(false);
   }
   
   if (err == noErr)
   {
      if (bindDLLFunctions())
         return(true);

      OpenALDLLShutdown();
   }

   return(false);
}

} // namespace Audio
