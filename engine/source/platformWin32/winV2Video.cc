//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformGL.h"
#include "platformWin32/platformWin32.h"
#include "platform/platformAudio.h"
#include "platformWin32/winV2Video.h"
#include "platform/3Dfx.h"
#include "console/console.h"
#include "math/mPoint.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "console/consoleInternal.h"
#include "console/ast.h"
#include "core/fileStream.h"


static U8 sResCode;  // Used for initializing the resolution list only

static void execScript(const char *scriptFile)
{
	// execute the script
   FileStream str;

   if (!str.open(scriptFile, FileStream::Read))
      return;

   U32 size = str.getStreamSize();
   char *script = new char[size + 1];

   str.read(size, script);
   str.close();

   script[size] = 0;
   Con::executef(2, "eval", script);
   delete[] script;
}

//------------------------------------------------------------------------------
Voodoo2Device::Voodoo2Device()
{
   initDevice();
}


//------------------------------------------------------------------------------
void Voodoo2Device::initDevice()
{
   // Set the device name:
   mDeviceName = "Voodoo2";

   // Set some initial conditions:
   mResolutionList.clear();
	mFullScreenOnly = true;

   // Enumerate all available resolutions:
   Resolution newRes;
   newRes = Resolution( 640, 480, 16 );
   mResolutionList.push_back( newRes );

   if ( sResCode & 1 )
   {
      newRes = Resolution( 800, 600, 16 );
      mResolutionList.push_back( newRes );
   }

   if ( sResCode & 2 )
   {
      Con::printf( "SLI detected." );

      newRes = Resolution( 960, 720, 16 );
      mResolutionList.push_back( newRes );

      newRes = Resolution( 1024, 768, 16 );
      mResolutionList.push_back( newRes );
   }
}


//------------------------------------------------------------------------------
bool Voodoo2Device::activate( U32 width, U32 height, U32 /*bpp*/, bool /*fullScreen*/ )
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
         AssertFatal( false, "Voodoo2Device::activate\nqwglMakeCurrent( NULL, NULL ) failed!" );
         return false;
      }

      Con::printf( "Deleting the rendering context..." );
      if ( !qwglDeleteContext( winState.hGLRC ) )
      {
         AssertFatal( false, "Voodoo2Device::activate\nqwglDeleteContext failed!" );
         return false;
      }
      winState.hGLRC = NULL;
   }

   // If window already exists, kill it so we can start fresh:
   if ( winState.appWindow )
   {
      Con::printf( "Releasing the device context..." );
      ReleaseDC( winState.appWindow, winState.appDC );
      winState.appDC = NULL;

      Con::printf( "Destroying the window..." );
      DestroyWindow( winState.appWindow );
      winState.appWindow = NULL;
   }

   // If OpenGL library already loaded, shut it down and reload the 3Dfx standalone driver:
   if ( winState.hinstOpenGL )
      QGL_Shutdown();

   QGL_Init( "3dfxvgl", "glu32" );

   // This device only supports full-screen, so force it:
   smIsFullScreen = true;
   Con::setVariable( "$pref::Video::fullScreen", "true" );

   // Create the new window:
   Con::printf( "Creating a new full-screen window..." );
   winState.appWindow = CreateOpenGLWindow( width, height, true );
   if ( !winState.appWindow )
   {
      AssertFatal( false, "Voodoo2Device::activate\nFailed to create a window!" );
      return false;
   }

   // Get a device context from the new window:
   HDC tempDC = GetDC( winState.appWindow );
   if ( !tempDC )
   {
      AssertFatal( false, "Voodoo2Device::activate\nFailed to get a device context!" );
      return false;
   }

   // Set the pixel format of the new window:
   PIXELFORMATDESCRIPTOR pfd;
   CreatePixelFormat( &pfd, 16, 16, 8, false );
   S32 chosenFormat = ChooseBestPixelFormat( tempDC, &pfd );
   qwglDescribePixelFormat( tempDC, chosenFormat, sizeof( pfd ), &pfd );
   bool test = qwglSetPixelFormat( tempDC, chosenFormat, &pfd );
   ReleaseDC( winState.appWindow, tempDC );
   if ( !test )
   {
      AssertFatal( false, "Voodoo2Device::activate\nFailed to set the pixel format of the device context!" );
      return false;
   }
   Con::printf( "Pixel format set:" );
   Con::printf( "  %d color bits, %d depth bits, %d stencil bits", pfd.cColorBits, pfd.cDepthBits, pfd.cStencilBits );

   // Set the resolution:
   smCurrentRes.bpp = 16; // Force 16-bit color.
   if ( !setScreenMode( width, height, 16, true, true, false ) )
      return false;

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

   // Set a few variables:
   if ( Con::getIntVariable( "$pref::OpenGL::mipReduction" ) < 1 )
   	  Con::setIntVariable( "$pref::OpenGL::mipReduction", 1 );
	if ( Con::getIntVariable( "$pref::OpenGL::interiorMipReduction" ) < 1 )
   	  Con::setIntVariable( "$pref::OpenGL::interiorMipReduction", 1 );
	if ( Con::getIntVariable( "$pref::OpenGL::skyMipReduction" ) < 1 )
   	  Con::setIntVariable( "$pref::OpenGL::skyMipReduction", 1 );

   if (dStrcmp(vendorString, Con::getVariable("$pref::Video::profiledVendor")) ||
       dStrcmp(rendererString, Con::getVariable("$pref::Video::profiledRenderer")))
   {
      // Voodoo2 defaults
      Con::setBoolVariable("$pref::OpenGL::disableEXTCompiledVertexArray", false);
      Con::setBoolVariable("$pref::OpenGL::disableSubImage", false);
      Con::setBoolVariable("$pref::OpenGL::noEnvColor", true);
      Con::setBoolVariable("$pref::OpenGL::disableARBTextureCompression", false);
		Con::setBoolVariable("$pref::Interior::lockArrays", true);
      Con::setBoolVariable("$pref::TS::skipFirstFog", false);
      Con::setBoolVariable("$pref::OpenGL::noDrawArraysAlpha", true); 
   }

   if ( needResurrect )
   {
      // Reload the textures:
      Con::printf( "Resurrecting the texture manager..." );
      Game->textureResurrect();
   }

   // Set the new window to the foreground:
   ShowWindow( winState.appWindow, SW_SHOW );
   SetForegroundWindow( winState.appWindow );
   SetFocus( winState.appWindow );

   QGL_EXT_Init();

   Con::setBoolVariable( "$SwapIntervalSupported", false );
   Con::setVariable( "$pref::Video::displayDevice", mDeviceName );
	Con::setBoolVariable("$pref::OpenGL::allowTexGen", true);

   return true;
}


//------------------------------------------------------------------------------
void Voodoo2Device::shutdown()
{
   if ( winState.hGLRC )
   {
      qwglMakeCurrent( NULL, NULL );
      qwglDeleteContext( winState.hGLRC );
      winState.hGLRC = NULL;
   }

   if ( winState.appDC )
   {
      ReleaseDC( winState.appWindow, winState.appDC );
      winState.appDC = NULL;
   }

   ChangeDisplaySettings( NULL, 0 );
}


//------------------------------------------------------------------------------
bool Voodoo2Device::setScreenMode( U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint )
{
   HWND curtain = NULL;
   bool needResurrect = false;
	Resolution newRes( width, height, bpp );

	if ( !fullScreen )
	{
	   // The 3Dfx Voodoo2 OpenGL driver only runs in full-screen, so ignore:
	   Con::warnf( ConsoleLogEntry::General, "Sorry, the Voodoo 2 display device only supports full-screen!" );
	}

   // Force the res to be one of the ones in the supported list:
   U32 resIndex = 0;
   U32 bestScore = 0, thisScore = 0;
   for ( U32 i = 0; i < mResolutionList.size(); i++ )
   {
      if ( newRes == mResolutionList[i] )
      {
         resIndex = i;
         break;
      }
      else
      {
         thisScore = abs( S32( newRes.w ) - S32( mResolutionList[i].w ) ) + abs( S32( newRes.h ) - S32( mResolutionList[i].h ) );
         if ( !bestScore || thisScore < bestScore )
         {
            bestScore = thisScore;
            resIndex = i;
         }
      }
   }
   newRes = mResolutionList[resIndex];

   // Return if we aren't forcing it and we are already in the desired resolution:
   if ( !forceIt && newRes == smCurrentRes )
      return true;

   Con::printf( "Setting screen mode to %dx%dx16 (fs)...", newRes.w, newRes.h );

   // Delete the rendering context:
   if ( winState.hGLRC )
   {
      Con::printf( "Killing the texture manager..." );
      Game->textureKill();
      needResurrect = true;

      Con::printf( "Making the rendering context not current..." );
      if ( !qwglMakeCurrent( NULL, NULL ) )
      {
         AssertFatal( false, "Voodoo2Device::setScreenMode\nqwglMakeCurrent( NULL, NULL ) failed!" );
         return false;
      }

      Con::printf( "Deleting the rendering context..." );
      if ( !qwglDeleteContext( winState.hGLRC ) )
      {
         AssertFatal( false, "Voodoo2Device::setScreenMode\nqwglDeleteContext failed!" );
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

   // Change the display settings (shouldn't really be necessary, but is):
   U32 test;
   DEVMODE devMode;
   dMemset( &devMode, 0, sizeof( devMode ) );
   devMode.dmSize       = sizeof( devMode );
   devMode.dmPelsWidth  = newRes.w;
   devMode.dmPelsHeight = newRes.h;
   devMode.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

   Con::printf( "Changing the display settings to %dx%dx16...", newRes.w, newRes.h );
   curtain = CreateCurtain( newRes.w, newRes.h );
   test = ChangeDisplaySettings( &devMode, CDS_FULLSCREEN );
   if ( test != DISP_CHANGE_SUCCESSFUL )
   {
      ChangeDisplaySettings( NULL, 0 );
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
      return false;
   }

   // Resize the window:
   if ( !SetWindowPos( winState.appWindow, NULL, 0, 0, newRes.w, newRes.h, SWP_NOZORDER | SWP_NOMOVE ) )
   {
      Con::printf( "Voodoo2Device::setScreenMode - SetWindowPos sizing to %dx%d failed.", newRes.w, newRes.h );
      return false;
   }

   // Get a new device context:
   Con::printf( "Acquiring a new device context..." );
   winState.appDC = GetDC( winState.appWindow );
   if ( !winState.appDC )
   {
      AssertFatal( false, "Voodoo2Device::setScreenMode\nFailed to get a valid device context!" );
      return false;
   }

   // Create a new rendering context:
   Con::printf( "Creating a new rendering context..." );
   winState.hGLRC = qwglCreateContext( winState.appDC );
   if ( !winState.hGLRC )
   {
      AssertFatal( false, "Voodoo2Device::setScreenMode\nFailed to create a GL rendering context!" );
      return false;
   }   

   // Make the new rendering context current:
   Con::printf( "Making the new rendering context current..." );
   if ( !qwglMakeCurrent( winState.appDC, winState.hGLRC ) )
   {
      AssertFatal( false, "Voodoo2Device::setScreenMode\nFailed to make the rendering context current!" );
      return false;
   }

   if ( needResurrect )
   {
      // Reload the textures:
      Con::printf( "Resurrecting the texture manager..." );
      Game->textureResurrect();
   }

   smCurrentRes = newRes;
   Platform::setWindowSize( newRes.w, newRes.h );
   char tempBuf[15];
   dSprintf( tempBuf, sizeof( tempBuf ), "%d %d %d", smCurrentRes.w, smCurrentRes.h, smCurrentRes.bpp );
   Con::setVariable( "$pref::Video::resolution", tempBuf );

   if ( curtain )
      DestroyWindow( curtain );

   if ( repaint )
      Con::evaluate( "resetCanvas();" );

   return true;
}


//------------------------------------------------------------------------------
void Voodoo2Device::swapBuffers()
{
   qwglSwapBuffers( winState.appDC );
}


//------------------------------------------------------------------------------
const char* Voodoo2Device::getDriverInfo()
{
   // Output some driver info to the console:
   const char* vendorString   = (const char*) glGetString( GL_VENDOR );
   const char* rendererString = (const char*) glGetString( GL_RENDERER );
   const char* versionString  = (const char*) glGetString( GL_VERSION );
	const char* extensionsString = (const char*) glGetString( GL_EXTENSIONS );
	
	// TODO - move strings out of code and into script...
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
bool Voodoo2Device::getGammaCorrection(F32 &g)
{
   // apparently the qwlGetDeviceGammaRamp3DFX doesn't actually work.  So, this
	// is a good default.  We don't need to worry about restoring the original
	// gamma.  Anything we do is wiped out once we destroy the Voodoo2 window.
   g = 0.8;

	return true;
   
   U16 ramp[256*3];

   if (!qwglGetDeviceGammaRamp3DFX(winState.appDC, ramp))
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
bool Voodoo2Device::setGammaCorrection(F32 g)
{
   U16 ramp[256*3];

   for (U16 i = 0; i < 256; ++i)
      ramp[i] = mPow((F32) i/256.0f, g) * 65535.0f;
   dMemcpy(&ramp[256],ramp,256*sizeof(U16));
   dMemcpy(&ramp[512],ramp,256*sizeof(U16));      

   return qwglSetDeviceGammaRamp3DFX(winState.appDC, ramp);
}

//------------------------------------------------------------------------------
bool Voodoo2Device::setVerticalSync( bool on )
{
   on;
   return( false );
}

//------------------------------------------------------------------------------
// Stuff for Voodoo 2 detection:
#ifndef GR_HARDWARE
#define GR_HARDWARE  0xa1
#endif

#define GLIDE_NUM_TMU         2
#define MAX_NUM_SST           4

typedef S32 GrSstType;
#define GR_SSTTYPE_VOODOO     0
#define GR_SSTTYPE_SST96      1
#define GR_SSTTYPE_AT3D       2

typedef struct GrTMUConfig_St 
{
   S32    tmuRev;                /* Rev of Texelfx chip */
   S32    tmuRam;                /* 1, 2, or 4 MB */
} GrTMUConfig_t;

typedef struct GrVoodooConfig_St 
{
   S32    fbRam;                             /* 1, 2, or 4 MB */
   S32    fbiRev;                            /* Rev of Pixelfx chip */
   S32    nTexelfx;                          /* How many texelFX chips are there? */
   FxBool sliDetect;                         /* Is it a scan-line interleaved board? */
   GrTMUConfig_t tmuConfig[GLIDE_NUM_TMU];   /* Configuration of the Texelfx chips */
} GrVoodooConfig_t;

typedef struct GrSst96Config_St 
{
   S32   fbRam;                  /* How much? */
   S32   nTexelfx;
   GrTMUConfig_t tmuConfig;
} GrSst96Config_t;

typedef struct GrAT3DConfig_St 
{
   S32   rev;
} GrAT3DConfig_t;


typedef struct 
{
   S32 num_sst;                  /* # of HW units in the system */
   struct 
   {
      GrSstType type;             /* Which hardware is it? */
      union SstBoard_u 
      {
         GrVoodooConfig_t  VoodooConfig;
         GrSst96Config_t   SST96Config;
         GrAT3DConfig_t    AT3DConfig;
      } sstBoard;
   } SSTs[MAX_NUM_SST];          /* configuration for each board */
} GrHwConfiguration;

typedef FxBool( FX_CALL *grSstQueryBoards_fpt )( GrHwConfiguration *hwconfig );
typedef FxBool( FX_CALL *grSstQueryHardware_fpt )( GrHwConfiguration *hwconfig );

typedef void ( FX_CALL *grGlideInit_fpt )( void );
typedef const char* ( FX_CALL *grGetString_fpt )( FxU32 pName );
typedef void( FX_CALL *grGlideShutdown_fpt )( void );


//------------------------------------------------------------------------------
DisplayDevice* Voodoo2Device::create()
{
   // Detect the Voodoo2 OpenGL driver:
   GrHwConfiguration hwConfig;
   bool  result = true;
   
   // Reset the available resolution specifier code:
   sResCode = 0;

   grGlideInit_fpt         grGlideInit = NULL;
   grGlideShutdown_fpt     grGlideShutdown = NULL;
   grSstQueryBoards_fpt    grSstQueryBoards = NULL;
   grSstQueryHardware_fpt  grSstQueryHardware = NULL;
   grGetString_fpt         grGetString = NULL;

   // Let the Glide 2 stuff go first:
   HINSTANCE glide2DLL = LoadLibraryA( "glide2x" );

   if ( !glide2DLL )
      return NULL;

   grSstQueryBoards     = (grSstQueryBoards_fpt) GetProcAddress( glide2DLL, "_grSstQueryBoards@4" );

   if ( grSstQueryBoards )
   {
      grSstQueryBoards( &hwConfig );
      if ( hwConfig.num_sst == 0 )
         result = false;
      else
      {
         grGlideInit          = (grGlideInit_fpt) GetProcAddress( glide2DLL, "_grGlideInit@0" );
         grSstQueryHardware   = (grSstQueryHardware_fpt) GetProcAddress( glide2DLL, "_grSstQueryHardware@4" );
         grGlideShutdown      = (grGlideShutdown_fpt) GetProcAddress( glide2DLL, "_grGlideShutdown@0" );

         if ( grGlideInit && grSstQueryHardware && grGlideShutdown )
         {
            grGlideInit();
            grSstQueryHardware( &hwConfig );

            // Find out what resolutions are available:
            if ( hwConfig.SSTs[0].sstBoard.VoodooConfig.sliDetect == 0 )   // SLI not detected.
            {
               if ( hwConfig.SSTs[0].sstBoard.VoodooConfig.fbRam >= 4 )
                  sResCode = 1;  // low bit indicates 800x600
            }
            else  // SLI detected.
            {
               sResCode = 1;

               if ( hwConfig.SSTs[0].sstBoard.VoodooConfig.fbRam >= 4 )
                  sResCode += 2;
            }

            grGlideShutdown();
         }

         grGlideInit       = NULL;
         grGlideShutdown   = NULL;
      }
   }
   else
      result = false;

   FreeLibrary( glide2DLL );

   if ( result )
   {
      // Glide 3's turn:
      HINSTANCE glide3DLL = LoadLibraryA( "glide3x" );

      if ( !glide3DLL )
         return NULL;

      grGlideInit       = (grGlideInit_fpt) GetProcAddress( glide3DLL, "_grGlideInit@0" );
      grGetString       = (grGetString_fpt) GetProcAddress( glide3DLL, "_grGetString@4" );
      grGlideShutdown   = (grGlideShutdown_fpt) GetProcAddress( glide3DLL, "_grGlideShutdown@0" );

      if ( grGlideInit && grGetString && grGlideShutdown )
      {
         grGlideInit();

         const char* hardware = grGetString( GR_HARDWARE );
         if ( dStrcmp( hardware, "Voodoo2" ) != 0 )
            result = false;

         grGlideShutdown();
      }
      else
         result = false;

      FreeLibrary( glide3DLL );
   }

   if ( result )
      return new Voodoo2Device();
   else
      return NULL;   
}
