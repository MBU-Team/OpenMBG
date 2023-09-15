//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMWIN32_H_
#define _PLATFORMWIN32_H_

// define this so that we can use WM_MOUSEWHEEL messages...
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#if defined(TORQUE_COMPILER_CODEWARRIOR)
#  include <ansi_prefix.win32.h>
#  include <stdio.h>
#  include <string.h>
#else
#  include <stdio.h>
#  include <string.h>
#endif

#if defined(TORQUE_COMPILER_VISUALC) || defined(TORQUE_COMPILER_GCC2)
#define vsnprintf _vsnprintf
#define stricmp _stricmp
#define strnicmp _strnicmp
#define strupr _strupr
#define strlwr _strlwr
#endif

#define NOMINMAX


struct Win32PlatState
{
   FILE *log_fp;
   HINSTANCE hinstOpenGL;
   HINSTANCE hinstGLU;
   HINSTANCE hinstOpenAL;
   HWND appWindow;
   HDC appDC;
   HINSTANCE appInstance;
   HGLRC hGLRC;
   DWORD processId;

   S32 desktopBitsPixel;
   S32 desktopWidth;
   S32 desktopHeight;
   U32 currentTime;
   
   Win32PlatState();
};

extern Win32PlatState winState;

extern bool QGL_Init( const char *dllname_gl, const char *dllname_glu );
extern bool QGL_EXT_Init();
extern void QGL_Shutdown();

extern HWND CreateOpenGLWindow( U32 width, U32 height, bool fullScreen );
extern HWND CreateCurtain( U32 width, U32 height );
extern void CreatePixelFormat( PIXELFORMATDESCRIPTOR *pPFD, S32 colorBits, S32 depthBits, S32 stencilBits, bool stereo );
extern S32  ChooseBestPixelFormat( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD );
extern void setModifierKeys( S32 modKeys );

extern S32 ( WINAPI * qwglSwapIntervalEXT )(S32 interval );
extern BOOL  ( WINAPI * qwglGetDeviceGammaRamp3DFX )(HDC, LPVOID );
extern BOOL  ( WINAPI * qwglSetDeviceGammaRamp3DFX )(HDC, LPVOID );
extern S32   ( WINAPI * qwglChoosePixelFormat )(HDC, CONST PIXELFORMATDESCRIPTOR *);
extern S32   ( WINAPI * qwglDescribePixelFormat) (HDC, S32, UINT, LPPIXELFORMATDESCRIPTOR);
extern S32   ( WINAPI * qwglGetPixelFormat )(HDC);
extern BOOL  ( WINAPI * qwglSetPixelFormat )(HDC, S32, CONST PIXELFORMATDESCRIPTOR *);
extern BOOL  ( WINAPI * qwglSwapBuffers )(HDC);
extern BOOL  ( WINAPI * qwglCopyContext )(HGLRC, HGLRC, UINT);
extern HGLRC ( WINAPI * qwglCreateContext )(HDC);
extern HGLRC ( WINAPI * qwglCreateLayerContext )(HDC, S32);
extern BOOL  ( WINAPI * qwglDeleteContext )(HGLRC);
extern HGLRC ( WINAPI * qwglGetCurrentContext )(VOID);
extern HDC   ( WINAPI * qwglGetCurrentDC )(VOID);
extern PROC  ( WINAPI * qwglGetProcAddress )(LPCSTR);
extern BOOL  ( WINAPI * qwglMakeCurrent )(HDC, HGLRC);
extern BOOL  ( WINAPI * qwglShareLists )(HGLRC, HGLRC);
extern BOOL  ( WINAPI * qwglUseFontBitmaps )(HDC, DWORD, DWORD, DWORD);
extern BOOL  ( WINAPI * qwglUseFontOutlines )(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, S32, LPGLYPHMETRICSFLOAT);
extern BOOL ( WINAPI * qwglDescribeLayerPlane )(HDC, S32, S32, UINT, LPLAYERPLANEDESCRIPTOR);
extern S32  ( WINAPI * qwglSetLayerPaletteEntries )(HDC, S32, S32, S32, CONST COLORREF *);
extern S32  ( WINAPI * qwglGetLayerPaletteEntries )(HDC, S32, S32, S32, COLORREF *);
extern BOOL ( WINAPI * qwglRealizeLayerPalette )(HDC, S32, BOOL);
extern BOOL ( WINAPI * qwglSwapLayerBuffers )(HDC, UINT);


#endif //_PLATFORMWIN32_H_
