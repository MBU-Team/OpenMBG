//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformGL.h"
#include "platformWin32/platformWin32.h"
#include "platform/platformAudio.h"
#include "platformWin32/winD3DVideo.h"
#include "console/console.h"
#include "math/mPoint.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "console/consoleInternal.h"
#include "console/ast.h"

#if defined(TORQUE_DEBUG)
#  define OPENGL2D3D_DLL "opengl2d3d_DEBUG.dll"
#  define GLU2D3D_DLL "glu2d3d_DEBUG.dll"
#else
#  define OPENGL2D3D_DLL "opengl2d3d.dll"
#  define GLU2D3D_DLL "glu2d3d.dll"
#endif

//------------------------------------------------------------------------------

bool D3DDevice::smCanSwitchBitDepth = true;
bool D3DDevice::smStay16 = false;

static void profileSystem(const char *vendor, const char *renderer)
{
	Con::setBoolVariable("$pref::Video::safeModeOn", true);
	Con::setBoolVariable("$pref::OpenGL::disableEXTCompiledVertexArray", false);
	Con::setBoolVariable("$pref::OpenGL::disableSubImage", false); 
	Con::setBoolVariable("$pref::OpenGL::noEnvColor", true);
	Con::setBoolVariable("$pref::OpenGL::disableARBTextureCompression", false);
	Con::setBoolVariable("$pref::Interior::lockArrays", true);
	Con::setBoolVariable("$pref::TS::skipFirstFog", false);
	Con::setBoolVariable("$pref::OpenGL::disableEXTFogCoord", false);

   Con::setVariable("$pref::Video::profiledVendor", vendor);
   Con::setVariable("$pref::Video::profiledRenderer", renderer);
}    


//------------------------------------------------------------------------------
D3DDevice::D3DDevice()
{
   initDevice();
}


//------------------------------------------------------------------------------
void D3DDevice::initDevice()
{
   // Set the device name:
   mDeviceName = "D3D";

   // Set some initial conditions:
   mResolutionList.clear();

   // Enumerate all available resolutions:
   DEVMODE devMode;
   U32 modeNum = 0;
   U32 stillGoing = true;
   while ( stillGoing )
   {
      dMemset( &devMode, 0, sizeof( devMode ) );
      devMode.dmSize = sizeof( devMode );

      stillGoing = EnumDisplaySettings( NULL, modeNum++, &devMode );

      if ( devMode.dmPelsWidth >= 640 && devMode.dmPelsHeight >= 480
			  && ( devMode.dmBitsPerPel == 16 || (devMode.dmBitsPerPel == 32 && !smStay16) ) &&
			  ( smCanSwitchBitDepth || devMode.dmBitsPerPel == winState.desktopBitsPixel ) )
      {
         // Only add this resolution if it is not already in the list:
         bool alreadyInList = false;
         for ( U32 i = 0; i < mResolutionList.size(); i++ )
         {
            if ( devMode.dmPelsWidth == mResolutionList[i].w
              && devMode.dmPelsHeight == mResolutionList[i].h
              && devMode.dmBitsPerPel == mResolutionList[i].bpp )
            {
               alreadyInList = true;
               break;
            }
         }

         if ( !alreadyInList )
         {
            Resolution newRes( devMode.dmPelsWidth, devMode.dmPelsHeight, devMode.dmBitsPerPel );
            mResolutionList.push_back( newRes );
         }
      }
   }
}


//------------------------------------------------------------------------------
bool D3DDevice::activate( U32 width, U32 height, U32 bpp, bool fullScreen )
{
   bool needResurrect = false;

   // If the rendering context exists, delete it:
   if ( winState.hGLRC )
   {
      Con::printf( "Killing the texture manager..." );
      Game->textureKill();
      needResurrect = true;

      Con::printf( "Making the rendering context not current..." );
      if ( !qwglMakeCurrent( NULL, NULL ) )
      {
         AssertFatal( false, "D3DDevice::activate\nqwglMakeCurrent( NULL, NULL ) failed!" );
         return false;
      }

      Con::printf( "Deleting the rendering context ..." );
      if ( !qwglDeleteContext( winState.hGLRC ) )
      {
         AssertFatal( false, "D3DDevice::activate\nqwglDeleteContext failed!" );
         return false;
      }
      winState.hGLRC = NULL;
   }

   // If the window already exists, kill it so we can start fresh:
   if ( winState.appWindow )
   {
      Con::printf( "Releasing the device context..." );
      ReleaseDC( winState.appWindow, winState.appDC );
      winState.appDC = NULL;

      Con::printf( "Destroying the window..." );
      DestroyWindow( winState.appWindow );
      winState.appWindow = NULL;
   }

   // If OpenGL library already loaded, shut it down and reload it:
   if ( winState.hinstOpenGL )
      QGL_Shutdown();

   QGL_Init(OPENGL2D3D_DLL,GLU2D3D_DLL);

   // Set the resolution:
   if ( !setScreenMode( width, height, bpp, ( fullScreen || mFullScreenOnly ), true, false ) )
      return false;

   // Get original gamma ramp
   mRestoreGamma = GetDeviceGammaRamp(winState.appDC, mOriginalRamp);

   // Output some driver info to the console:
   const char* vendorString   = (const char*) glGetString( GL_VENDOR );
   const char* rendererString = (const char*) glGetString( GL_RENDERER );
   const char* versionString  = (const char*) glGetString( GL_VERSION );
   Con::printf( "OpenGL driver information:" );
   if ( vendorString )
      Con::printf( "  Vendor: %s", vendorString );
   if ( rendererString )
      Con::printf( "  Renderer: %s", rendererString );
   if ( versionString )
      Con::printf( "  Version: %s", versionString );

   if ( needResurrect )
   {
      // Reload the textures:
      Con::printf( "Resurrecting the texture manager..." );
      Game->textureResurrect();
   }

   QGL_EXT_Init();

   Con::setVariable( "$pref::Video::displayDevice", mDeviceName );
   Con::setBoolVariable( "$SwapIntervalSupported", false );
	Con::setBoolVariable( "$pref::OpenGL::allowTexGen", false );
	Con::setBoolVariable( "$pref::environmentMaps", false );

   // only do this for the first session
   if (!Con::getBoolVariable("$DisableSystemProfiling") && (dStrcmp(vendorString, Con::getVariable("$pref::Video::profiledVendor")) ||
       dStrcmp(rendererString, Con::getVariable("$pref::Video::profiledRenderer"))))
      profileSystem(vendorString, rendererString);

   return true;
}


//------------------------------------------------------------------------------
void D3DDevice::shutdown()
{
   if ( winState.hGLRC )
   {
      if (mRestoreGamma)
         SetDeviceGammaRamp(winState.appDC, mOriginalRamp);

      qwglMakeCurrent( NULL, NULL );
      qwglDeleteContext( winState.hGLRC );
      winState.hGLRC = NULL;
   }

   if ( winState.appDC )
   {
      ReleaseDC( winState.appWindow, winState.appDC );
      winState.appDC = NULL;
   }

   if ( smIsFullScreen || (smStay16 && winState.desktopBitsPixel != 16))
      ChangeDisplaySettings( NULL, 0 );

	QGL_Shutdown();
}


//------------------------------------------------------------------------------
// This is the real workhorse function of the DisplayDevice...
//
bool D3DDevice::setScreenMode( U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint )
{
   HWND curtain = NULL;
   char errorMessage[256];
	Resolution newRes( width, height, bpp );
	bool newFullScreen = fullScreen;
	bool safeModeOn = Con::getBoolVariable( "$pref::Video::safeModeOn" );

	if ( !newFullScreen && mFullScreenOnly )
	{
		Con::warnf( ConsoleLogEntry::General, "D3DDevice::setScreenMode - device or desktop color depth does not allow windowed mode!" );
		newFullScreen = true;
	}

   if ( !newFullScreen && ( newRes.w >= winState.desktopWidth || newRes.h >= winState.desktopHeight ) )
   {
      Con::warnf( ConsoleLogEntry::General, "D3DDevice::setScreenMode -- can't switch to resolution larger than desktop in windowed mode!" );
		// Find the largest standard resolution that will fit on the desktop:
		U32 resIndex;
		for ( resIndex = mResolutionList.size() - 1; resIndex > 0; resIndex-- )
		{
         if ( mResolutionList[resIndex].w < winState.desktopWidth 
           && mResolutionList[resIndex].h < winState.desktopHeight )
            break;  
		}
		newRes = mResolutionList[resIndex];
   }

   if ( newRes.w < 640 || newRes.h < 480 )
   {
      Con::warnf( ConsoleLogEntry::General, "D3DDevice::setScreenMode -- can't go smaller than 640x480!" );
      return false;
   }

   if ( newFullScreen )
   {
      if (newRes.bpp != 16 && mFullScreenOnly)
         newRes.bpp = 16;
    
      // Match the new resolution to one in the list:
      U32 resIndex = 0;
      U32 bestScore = 0, thisScore = 0;
      for ( int i = 0; i < mResolutionList.size(); i++ )
      {
         if ( newRes == mResolutionList[i] )
         {
            resIndex = i;
            break;
         }
         else
         {
            thisScore = abs( S32( newRes.w ) - S32( mResolutionList[i].w ) ) 
                      + abs( S32( newRes.h ) - S32( mResolutionList[i].h ) ) 
                      + ( newRes.bpp == mResolutionList[i].bpp ? 0 : 1 );

            if ( !bestScore || ( thisScore < bestScore ) )
            {
               bestScore = thisScore;
               resIndex = i;
            }
         }
      }

      newRes = mResolutionList[resIndex];
   }
   else
      // Basically ignore the bit depth parameter:
      if (!smStay16)
      	newRes.bpp = winState.desktopBitsPixel;
		else
			newRes.bpp = 16;

   // Return if already at this resolution:
   if ( !forceIt && newRes == smCurrentRes && newFullScreen == smIsFullScreen )
      return true;

   Con::printf( "Setting screen mode to %dx%dx%d (%s)...", newRes.w, newRes.h, newRes.bpp, ( newFullScreen ? "fs" : "w" ) );

   bool needResurrect = false;

   // oh just always do it
   //if ( ( newRes.bpp != smCurrentRes.bpp ) || ( safeModeOn && ( ( smIsFullScreen != newFullScreen ) || newFullScreen ) ) )
   if (true)
   {
      // Delete the rendering context:
      if ( winState.hGLRC )
      {
         if (!Video::smNeedResurrect)
			{
         	Con::printf( "Killing the texture manager..." );
         	Game->textureKill();
         	needResurrect = true;
			}

         Con::printf( "Making the rendering context not current..." );
         if ( !qwglMakeCurrent( NULL, NULL ) )
         {
            AssertFatal( false, "D3DDevice::setScreenMode\nqwglMakeCurrent( NULL, NULL ) failed!" );
            return false;
         }

         Con::printf( "Deleting the rendering context..." );
         if ( !qwglDeleteContext( winState.hGLRC ) )
         {
            AssertFatal( false, "D3DDevice::setScreenMode\nqwglDeleteContext failed!" );
            return false;
         }
         winState.hGLRC = NULL;
      }

      // Release the device context:
      if ( winState.appDC )
      {
         Con::printf( "Releasing the device context..." );
         ReleaseDC( winState.appWindow, winState.appDC );
         winState.appDC = NULL;
      }

      // Destroy the window:
      if ( winState.appWindow )
      {
         Con::printf( "Destroying the window..." );
         DestroyWindow( winState.appWindow );
         winState.appWindow = NULL;
      }
   }
	else if ( smIsFullScreen != newFullScreen )
	{
		// Change the window style:
		Con::printf( "Changing the window style..." );
      S32 windowStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      if ( newFullScreen )
         windowStyle |= ( WS_POPUP | WS_MAXIMIZE );
      else
         windowStyle |= ( WS_OVERLAPPED | WS_CAPTION );

      if ( !SetWindowLong( winState.appWindow, GWL_STYLE, windowStyle ) )
         Platform::AlertOK( "Error", "Failed to change the window style!" );
	}

   U32 test;
   if ( newFullScreen )
   {
      // Change the display settings:
      DEVMODE devMode;
      dMemset( &devMode, 0, sizeof( devMode ) );
      devMode.dmSize       = sizeof( devMode );
      devMode.dmPelsWidth  = newRes.w;
      devMode.dmPelsHeight = newRes.h;
      devMode.dmBitsPerPel = newRes.bpp;
      devMode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

      Con::printf( "Changing the display settings to %dx%dx%d...", newRes.w, newRes.h, newRes.bpp );
      curtain = CreateCurtain( newRes.w, newRes.h );
      test = ChangeDisplaySettings( &devMode, CDS_FULLSCREEN );
      if ( test != DISP_CHANGE_SUCCESSFUL )
      {
         smIsFullScreen = false;
         Con::setBoolVariable( "$pref::Video::fullScreen", false );
         ChangeDisplaySettings( NULL, 0 );
         Con::errorf( ConsoleLogEntry::General, "D3DDevice::setScreenMode - ChangeDisplaySettings failed." );
         switch( test )
         {
            case DISP_CHANGE_RESTART:
               Platform::AlertOK( "Display Change Failed", "You must restart your machine to get the specified mode." );
               break;

            case DISP_CHANGE_BADMODE:
               Platform::AlertOK( "Display Change Failed", "The specified mode is not supported by this device." );
               break;

            default: 
               Platform::AlertOK( "Display Change Failed", "Hardware failed to switch to the specified mode." );
               break;
         };
         DestroyWindow( curtain );
         return false;
      }
      else
         smIsFullScreen = true;
   }
   else if ( smIsFullScreen || (smStay16 && winState.desktopBitsPixel != 16))
   {
      if (!smStay16 || winState.desktopBitsPixel == 16)
      {
      	Con::printf( "Changing to the desktop display settings (%dx%dx%d)...", winState.desktopWidth, winState.desktopHeight, winState.desktopBitsPixel );
      	ChangeDisplaySettings( NULL, 0 );
		}
		else
		{
			// Change the display settings:
      	DEVMODE devMode;

			dMemset( &devMode, 0, sizeof( devMode ) );
      	devMode.dmSize       = sizeof( devMode );
      	devMode.dmBitsPerPel = newRes.bpp;
      	devMode.dmFields     = DM_BITSPERPEL;

      	Con::printf( "Changing the display settings to %dx%dx%d...", winState.desktopWidth, winState.desktopHeight, newRes.bpp );
      	test = ChangeDisplaySettings( &devMode, 0 );
			if ( test != DISP_CHANGE_SUCCESSFUL )
      	{
         	smIsFullScreen = true;
         	Con::setBoolVariable( "$pref::Video::fullScreen", true );
         	ChangeDisplaySettings( NULL, 0 );
         	Con::errorf( ConsoleLogEntry::General, "D3DDevice::setScreenMode - ChangeDisplaySettings failed." );
         	switch ( test )
         	{
            	case DISP_CHANGE_RESTART:
             		Platform::AlertOK( "Display Change Failed", "You must restart your machine to get the specified mode." );
              		break;

            	case DISP_CHANGE_BADMODE:
              		Platform::AlertOK( "Display Change Failed", "The specified mode is not supported by this device." );
              	 	break;

            	default: 
               	Platform::AlertOK( "Display Change Failed", "Hardware failed to switch to the specified mode." );
               	break;
         	};
         
         	return false;
      	}
		}
		smIsFullScreen = false;
   }
	Con::setBoolVariable( "$pref::Video::fullScreen", smIsFullScreen );

   bool newWindow = false;
   if ( !winState.appWindow )
   {
      Con::printf( "Creating a new %swindow...", ( fullScreen ? "full-screen " : "" ) );
      winState.appWindow = CreateOpenGLWindow( newRes.w, newRes.h, newFullScreen );
      if ( !winState.appWindow )
      {
         AssertFatal( false, "D3DDevice::setScreenMode\nFailed to create a new window!" );
         return false;
      }
      newWindow = true;
   }

   // Move the window:
   if ( !newFullScreen )
   {
      // Adjust the window rect to compensate for the window style:
      RECT windowRect;
      windowRect.left = windowRect.top = 0;
      windowRect.right = newRes.w;
      windowRect.bottom = newRes.h;
      
      AdjustWindowRect( &windowRect, GetWindowLong( winState.appWindow, GWL_STYLE ), false );
      U32 adjWidth = windowRect.right - windowRect.left;
      U32 adjHeight = windowRect.bottom - windowRect.top;

      // Center the window on the desktop:
      U32 xPos = ( winState.desktopWidth - adjWidth ) / 2;
      U32 yPos = ( winState.desktopHeight - adjHeight ) / 2;
      test = SetWindowPos( winState.appWindow, 0, xPos, yPos, adjWidth, adjHeight, SWP_NOZORDER );
      if ( !test )
      {
         dSprintf( errorMessage, 255, "D3DDevice::setScreenMode\nSetWindowPos failed trying to move the window to (%d,%d) and size it to %dx%d.", xPos, yPos, newRes.w, newRes.h ); 
         AssertFatal( false, errorMessage );
         return false;
      }
   }
   else if ( !newWindow )
   {
      // Move and size the window to take up the whole screen:
      if ( !SetWindowPos( winState.appWindow, 0, 0, 0, newRes.w, newRes.h, SWP_NOZORDER ) )
      {
         dSprintf( errorMessage, 255, "D3DDevice::setScreenMode\nSetWindowPos failed to move the window to (0,0) and size it to %dx%d.", newRes.w, newRes.h );
         AssertFatal( false, errorMessage );
         return false;
      }
   }

   if ( !winState.appDC )
   {
      // Get a new device context:
      Con::printf( "Acquiring a new device context..." );
      winState.appDC = GetDC( winState.appWindow );
      if ( !winState.appDC )
      {
         AssertFatal( false, "D3DDevice::setScreenMode\nGetDC failed to get a valid device context!" );
         return false;
      }
   }

   if ( newWindow )
   { 
      // Set the pixel format of the new window:
      PIXELFORMATDESCRIPTOR pfd;
      qwglDescribePixelFormat( winState.appDC, 1, sizeof( pfd ), &pfd );
      if ( !SetPixelFormat( winState.appDC, 1, &pfd ) )
      {
         //AssertFatal( false, "D3DDevice::setScreenMode\nFailed to set the pixel format!" );
         //return false;
      }
      Con::printf( "Pixel format set:" );
      Con::printf( "  %d color bits, %d depth bits, %d stencil bits", pfd.cColorBits, pfd.cDepthBits, pfd.cStencilBits );
   }

   if ( !winState.hGLRC )
   {
      // Create a new rendering context:
      Con::printf( "Creating a new rendering context..." );
      winState.hGLRC = qwglCreateContext( winState.appDC );
      if ( !winState.hGLRC )
      {
         AssertFatal( false, "D3DDevice::setScreenMode\nqwglCreateContext failed to create an OpenGL rendering context!" );
         return false;
      }   

      // Make the new rendering context current:
      Con::printf( "Making the new rendering context current..." );
      if ( !qwglMakeCurrent( winState.appDC, winState.hGLRC ) )
      {
         AssertFatal( false, "D3DDevice::setScreenMode\nqwglMakeCurrent failed to make the rendering context current!" );
         return false;
      }

      if ( needResurrect )
      {
         // Reload the textures:
         Con::printf( "Resurrecting the texture manager..." );
         Game->textureResurrect();
      }
   }

   smCurrentRes = newRes;
   Platform::setWindowSize( newRes.w, newRes.h );
   char tempBuf[15];
   dSprintf( tempBuf, sizeof( tempBuf ), "%d %d %d", smCurrentRes.w, smCurrentRes.h, smCurrentRes.bpp );
   Con::setVariable( "$pref::Video::resolution", tempBuf );

   if ( curtain )
      DestroyWindow( curtain );

	// Doesn't hurt to do this even it isn't necessary:
   ShowWindow( winState.appWindow, SW_SHOW );
   SetForegroundWindow( winState.appWindow );
   SetFocus( winState.appWindow );

   if ( repaint )
      Con::evaluate( "resetCanvas();" );

   return true;
}


//------------------------------------------------------------------------------
void D3DDevice::swapBuffers()
{
   qwglSwapBuffers( winState.appDC );
}


//------------------------------------------------------------------------------
const char* D3DDevice::getDriverInfo()
{
   // Output some driver info to the console:
   const char* vendorString   = (const char*) glGetString( GL_VENDOR );
   const char* rendererString = (const char*) glGetString( GL_RENDERER );
   const char* versionString  = (const char*) glGetString( GL_VERSION );
	const char* extensionsString = (const char*) glGetString( GL_EXTENSIONS );
	
	U32 bufferLen = ( vendorString ? dStrlen( vendorString ) : 0 )
	              + ( rendererString ? dStrlen( rendererString ) : 0 )
                 + ( versionString  ? dStrlen( versionString ) : 0 )
                 + ( extensionsString ? dStrlen( extensionsString ) : 0 )
                 + 4;

	char* returnString = Con::getReturnBuffer( bufferLen );
	dSprintf( returnString, bufferLen, "%s\t%s\t%s\t%s", 
			( vendorString ? vendorString : "" ),
			( rendererString ? rendererString : "" ),
			( versionString ? versionString : "" ),
			( extensionsString ? extensionsString : "" ) );
	
   return( returnString );   
}


//------------------------------------------------------------------------------
bool D3DDevice::getGammaCorrection(F32 &g)
{
   U16 ramp[256*3];

   if (!GetDeviceGammaRamp(winState.appDC, ramp))
      return false;

   F32 csum = 0.0;
   U32 ccount = 0;

   for (U16 i = 0; i < 256; ++i)
   {
      if (i != 0 && ramp[i] != 0 && ramp[i] != 65535)
      {
         F64 b = (F64) i/256.0;
         F64 a = (F64) ramp[i]/65535.0;
         F32 c = (F32) (mLog(a)/mLog(b));
         
         csum += c;
         ++ccount;  
      }
   }
   g = csum/ccount;

   return true;         
}    

//------------------------------------------------------------------------------
bool D3DDevice::setGammaCorrection(F32 g)
{
   U16 ramp[256*3];

   for (U16 i = 0; i < 256; ++i)
      ramp[i] = mPow((F32) i/256.0f, g) * 65535.0f;
   dMemcpy(&ramp[256],ramp,256*sizeof(U16));
   dMemcpy(&ramp[512],ramp,256*sizeof(U16));      

   return SetDeviceGammaRamp(winState.appDC, ramp);
}

//------------------------------------------------------------------------------
bool D3DDevice::setVerticalSync( bool on )
{
   on;
   return( false );
}

//------------------------------------------------------------------------------
DisplayDevice* D3DDevice::create()
{  
   bool result = false;

	OSVERSIONINFO OSVersionInfo;

	dMemset( &OSVersionInfo, 0, sizeof( OSVERSIONINFO ) );
	OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
	if ( GetVersionEx( &OSVersionInfo ) &&
		  (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
			OSVersionInfo.dwMajorVersion <= 4) )
		return NULL;

	// If we can't do OpenGL, don't attempt D3D either (probably Voodoo2 only)
	if (Video::getDevice("OpenGL") == NULL)
		return NULL;

   // This shouldn't happen, but just to be safe...
   if ( winState.hinstOpenGL )
      QGL_Shutdown();

   if (!QGL_Init(OPENGL2D3D_DLL,GLU2D3D_DLL))
		return NULL;

   // Create a test window to see if OpenGL hardware acceleration is available:
   WNDCLASSA wc;
   dMemset(&wc, 0, sizeof(wc));   
   wc.style         = CS_OWNDC;
   wc.lpfnWndProc   = DefWindowProc;
   wc.hInstance     = winState.appInstance;
   wc.lpszClassName = "D3DTest";
   RegisterClassA( &wc );

   HWND testWindow = CreateWindowA( 
      "D3DTest", 
      "", 
      WS_POPUP, 
      0, 0, 640, 480, 
      NULL, 
      NULL, 
      winState.appInstance, 
      NULL );

   if ( testWindow )
   {
      HDC testDC = GetDC( testWindow );

      if ( testDC )
      {
         PIXELFORMATDESCRIPTOR pfd;

         result = qwglDescribePixelFormat( testDC, 1, sizeof( pfd ), &pfd );
         ReleaseDC( testWindow, testDC );
      }
      DestroyWindow( testWindow );
   }

   UnregisterClassA( "D3DTest", winState.appInstance );

   QGL_Shutdown();

   if ( result )
   {
      D3DDevice* newOGLDevice = new D3DDevice();
      if ( newOGLDevice )
      {
         newOGLDevice->mFullScreenOnly = Video::getDevice("OpenGL")->isFullScreenOnly();
         return (DisplayDevice*) newOGLDevice;
      }
      else 
         return NULL;
   }
   else
      return NULL;
}
