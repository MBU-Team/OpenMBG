//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMMACCARB_H_
#define _PLATFORMMACCARB_H_

// tried to find a minimal set of includes here.
// this helps work around the Torque-Stupidly-Redefines-New problems.
#if defined(__MWERKS__) && !defined(__MACH__)

  #ifndef _PLATFORM_H_
  #include "platform/platform.h"
  #endif
  #include <Events.h>
  #include <Multiprocessing.h>

#else

  #include <CarbonCore.h>
  #include <QD.h>
  #include <Multiprocessing.h>
  #ifndef _PLATFORM_H_
  #include "platform/platform.h"
  #endif

#endif

class MacCarbPlatState
{
public:
   GDHandle      hDisplay;
   WindowPtr     appWindow;
   
   char          appWindowTitle[256];
   bool          quit;
   
   void *        ctx; // was an AGLContext -- but didn't want to inc AGL.h and all...

   short         volRefNum;   // application volume/drive reference number
   long          dirID;      // application directory id
   char          absAppPath[2048]; // app path - make it big enough!

   S32           desktopBitsPixel;
   S32           desktopWidth;
   S32           desktopHeight;
   U32           currentTime;
   
   U32           osVersion;
   bool          osX;
   
	FSRef		appFSRef;
	FSRef		parFSRef;
	FSSpec		appFSSpec;
   
   MacCarbPlatState();
};

extern MacCarbPlatState platState;

extern bool QGL_EXT_Init();


extern WindowPtr CreateOpenGLWindow( GDHandle hDevice, U32 width, U32 height, bool fullScreen );
extern WindowPtr CreateCurtain( GDHandle hDevice, U32 width, U32 height );

U32 GetMilliseconds();

U8* str2p(const char *str);
U8* str2p(const char *str, U8 *dst_p);

char* p2str(U8 *p);
char* p2str(U8 *p, char *dst_str);

char* uni2str(UniChar *u);
char* uni2str(UniChar *u, char *dst_str);

UniChar* str2uni(const char *str);
UniChar* str2uni(const char *str, UniChar *u);

U8 TranslateOSKeyCode(U8 vcode);

#endif //_PLATFORMMACCARB_H_

