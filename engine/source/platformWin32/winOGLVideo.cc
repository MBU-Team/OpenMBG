//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformGL.h"
#include "platformWin32/platformWin32.h"
#include "platform/platformAudio.h"
#include "platformWin32/winOGLVideo.h"
#include "platformWin32/winD3DVideo.h"
#include "console/console.h"
#include "math/mPoint.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "console/consoleInternal.h"
#include "console/ast.h"
#include "core/fileStream.h"

//------------------------------------------------------------------------------

bool OpenGLDevice::smCanSwitchBitDepth = true;

struct CardProfile
{
   const char *vendor;     // manufacturer
   const char *renderer;   // driver name

   bool safeMode;          // destroy rendering context for resolution change
   bool lockArray;         // allow compiled vertex arrays
   bool subImage;          // allow glTexSubImage*
   bool fogTexture;        // require bound texture for combine extension
   bool noEnvColor;        // no texture environment color
   bool clipHigh;          // clip high resolutions
	bool deleteContext;		// delete rendering context
	bool texCompress;			// allow texture compression
	bool interiorLock;		// lock arrays for Interior render
	bool skipFirstFog;		// skip first two-pass fogging (dumb 3Dfx hack)
	bool only16;				// inhibit 32-bit resolutions
	bool noArraysAlpha;	// don't use glDrawArrays with a GL_ALPHA texture

	const char *proFile;		// explicit profile of graphic settings
};

struct OSCardProfile
{
   const char *vendor;     // manufacturer
   const char *renderer;   // driver name

   bool allowOpenGL;			// allow OpenGL driver
	bool allowD3D;				// allow D3D driver
	bool preferOpenGL;		// prefer OpenGL driver
};    

static Vector<CardProfile> sCardProfiles(__FILE__, __LINE__);
static Vector<OSCardProfile> sOSCardProfiles(__FILE__, __LINE__);

struct ProcessorProfile
{
    U16 clock;  // clock range max
    U16 adjust; // CPU adjust   
};

static U8 sNumProcessors = 4;
static ProcessorProfile sProcessorProfiles[] =
{ 
    {  400,  0 },
    {  600,  5 },
    {  800, 10 },
    { 1000, 15 },
};

struct SettingProfile
{
    U16 performance;        // metric range max
    const char *settings;   // default file
};    

static U8 sNumSettings = 3;
static SettingProfile sSettingProfiles[] =
{
    {  33, "LowProfile.cs" },
    {  66, "MediumProfile.cs" },
    { 100, "HighProfile.cs" }, 
};

//------------------------------------------------------------------------------
ConsoleFunction( addCardProfile, void, 16, 16, "(string vendor, string renderer,"
                "bool safeMode, bool lockArray, bool subImage, bool fogTexture,"
                "bool noEnvColor, bool clipHigh, bool deleteContext, bool texCompress"
                "bool interiorLock, bool skipFirstFog, bool only16,"
                "bool noArraysAlpha, string proFile)"
                ""
                "Register a card profile with the card profile manager.\n\n"
                "Most of the parameters are fairly self-explanatory and very internal"
                " to the rendering code; however there are a few of note:\n"
                "@param vendor   The vendor string the card claims.\n"
                "@param renderer The renderer string the card claims.\n"
                "@param proFile  Name of the file where further configuration information is kept."
                )
{
   CardProfile profile;

   profile.vendor = dStrdup(argv[1]);
   profile.renderer = dStrdup(argv[2]);

   profile.safeMode = dAtob(argv[3]);   
   profile.lockArray = dAtob(argv[4]);
   profile.subImage = dAtob(argv[5]);
   profile.fogTexture = dAtob(argv[6]);
   profile.noEnvColor = dAtob(argv[7]);
   profile.clipHigh = dAtob(argv[8]);
	profile.deleteContext = dAtob(argv[9]);
	profile.texCompress = dAtob(argv[10]);
	profile.interiorLock = dAtob(argv[11]);
	profile.skipFirstFog = dAtob(argv[12]);
	profile.only16 = dAtob(argv[13]);
	profile.noArraysAlpha = dAtob(argv[14]);

	if (strcmp(argv[15],""))
		profile.proFile = dStrdup(argv[15]);
	else
		profile.proFile = NULL;

   sCardProfiles.push_back(profile);
}

ConsoleFunction( addOSCardProfile, void, 6,6, "(string vendor, string renderer, bool allowOpenGL,"
                " bool allowD3D, bool preferOpenGL) "
                "Register with the profile manager what rendering mode is preferred with the "
                "specified card.")
{
   OSCardProfile profile;

   profile.vendor = dStrdup(argv[1]);
   profile.renderer = dStrdup(argv[2]);

   profile.allowOpenGL = dAtob(argv[3]);
	profile.allowD3D = dAtob(argv[4]);
	profile.preferOpenGL = dAtob(argv[5]);

   sOSCardProfiles.push_back(profile);
}

static void clearCardProfiles()
{
   while (sCardProfiles.size())
   {
      dFree((char *) sCardProfiles.last().vendor);
      dFree((char *) sCardProfiles.last().renderer);

		dFree((char *) sCardProfiles.last().proFile);

      sCardProfiles.decrement();
   }
}

static void clearOSCardProfiles()
{
   while (sOSCardProfiles.size())
   {
      dFree((char *) sOSCardProfiles.last().vendor);
      dFree((char *) sOSCardProfiles.last().renderer);

      sOSCardProfiles.decrement();
   }
}

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

static void profileSystem(const char *vendor, const char *renderer)
{

   //Con::executef(2, "exec", "scripts/CardProfiles.cs");
	execScript("CardProfiles.cs");

	const char *arch;
	OSVERSIONINFO OSVersionInfo;
	const char *os = NULL;
	char osProfiles[64];

	if (dStrstr(Platform::SystemInfo.processor.name,"AMD") != NULL)
		arch = "AMD";
	else
		arch = "Intel";

	
	dMemset( &OSVersionInfo, 0, sizeof( OSVERSIONINFO ) );
   OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
   if ( GetVersionEx( &OSVersionInfo )	)
	{
		if (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
			if ( OSVersionInfo.dwMinorVersion == 0 )
			{
				if (dStrcmp(arch,"Intel") == 0)
					os = "W95";
			}
			else if ( OSVersionInfo.dwMinorVersion == 10 )	
				if ( OSVersionInfo.szCSDVersion[1] != 'A' )
					os = "W98";
				else
					os = "W98SE";
			else
				os = "WME";
		else
			if ( OSVersionInfo.dwMajorVersion >= 5 )
				os = "W2K";
		
		if ( os != NULL )
		{
			dSprintf(osProfiles,64,"%s%sCardProfiles.cs",arch,os);
			//Con::executef(2, "exec", osProfiles);
			execScript(osProfiles);
		}
	}

	const char *proFile = NULL;
   U32 i;
   
   for (i = 0; i < sCardProfiles.size(); ++i)
      if (dStrstr(vendor, sCardProfiles[i].vendor) &&
          (!dStrcmp(sCardProfiles[i].renderer, "*") ||
           dStrstr(renderer, sCardProfiles[i].renderer)))
      {         
         Con::setBoolVariable("$pref::Video::safeModeOn", sCardProfiles[i].safeMode);
         Con::setBoolVariable("$pref::OpenGL::disableEXTCompiledVertexArray", !sCardProfiles[i].lockArray);
         Con::setBoolVariable("$pref::OpenGL::disableSubImage", !sCardProfiles[i].subImage);
         Con::setBoolVariable("$pref::TS::fogTexture", sCardProfiles[i].fogTexture);
         Con::setBoolVariable("$pref::OpenGL::noEnvColor", sCardProfiles[i].noEnvColor);
         Con::setBoolVariable("$pref::Video::clipHigh", sCardProfiles[i].clipHigh);
			if (!sCardProfiles[i].deleteContext)
			{
				OSVERSIONINFO OSVersionInfo;

   			// HACK: The Voodoo3/5 on W2K crash when deleting a rendering context
				// So we're not deleting it.
				// Oh, and the Voodoo3 returns a Banshee renderer string under W2K
   			dMemset( &OSVersionInfo, 0, sizeof( OSVERSIONINFO ) );
   			OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
   			if ( GetVersionEx( &OSVersionInfo ) &&
					  OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
					  OSVersionInfo.dwMajorVersion == 5)
					Con::setBoolVariable("$pref::Video::deleteContext", false);
				else
					Con::setBoolVariable("$pref::Video::deleteContext", true);
			}
			else
				Con::setBoolVariable("$pref::Video::deleteContext", true);
			Con::setBoolVariable("$pref::OpenGL::disableARBTextureCompression", !sCardProfiles[i].texCompress);
			Con::setBoolVariable("$pref::Interior::lockArrays", sCardProfiles[i].interiorLock);
			Con::setBoolVariable("$pref::TS::skipFirstFog", sCardProfiles[i].skipFirstFog);
			Con::setBoolVariable("$pref::Video::only16", sCardProfiles[i].only16);
			Con::setBoolVariable("$pref::OpenGL::noDrawArraysAlpha", sCardProfiles[i].noArraysAlpha);

			proFile = sCardProfiles[i].proFile;

         break;   
      }

   // defaults
	U16 glProfile;

   if (!proFile)
   {
      // no driver GL profile -- make one via weighting GL extensions
      glProfile = 25;

      glProfile += gGLState.suppARBMultitexture * 25;
      glProfile += gGLState.suppLockedArrays * 15;
      glProfile += gGLState.suppVertexArrayRange * 10;
      glProfile += gGLState.suppTextureEnvCombine * 5;
      glProfile += gGLState.suppPackedPixels * 5;
      glProfile += gGLState.suppTextureCompression * 5;
      glProfile += gGLState.suppS3TC * 5;
      glProfile += gGLState.suppFXT1 * 5;

      Con::setBoolVariable("$pref::Video::safeModeOn", true);
      Con::setBoolVariable("$pref::OpenGL::disableEXTCompiledVertexArray", false);
      Con::setBoolVariable("$pref::OpenGL::disableSubImage", false); 
      Con::setBoolVariable("$pref::TS::fogTexture", false);
      Con::setBoolVariable("$pref::OpenGL::noEnvColor", false);
      Con::setBoolVariable("$pref::Video::clipHigh", false);
		Con::setBoolVariable("$pref::Video::deleteContext", true);
		Con::setBoolVariable("$pref::OpenGL::disableARBTextureCompression", false);
		Con::setBoolVariable("$pref::Interior::lockArrays", true);
		Con::setBoolVariable("$pref::TS::skipFirstFog", false);
		Con::setBoolVariable("$pref::Video::only16", false);
		Con::setBoolVariable("$pref::OpenGL::noDrawArraysAlpha", false);
   }

	Con::setVariable("$pref::Video::profiledVendor", vendor);
   Con::setVariable("$pref::Video::profiledRenderer", renderer);

	if (!Con::getBoolVariable("$DisableSystemProfiling") &&
		 ( dStrcmp(vendor, Con::getVariable("$pref::Video::defaultsVendor")) ||
       	dStrcmp(renderer, Con::getVariable("$pref::Video::defaultsRenderer")) ))
	{
		if (proFile)
		{
			char settings[64];

			dSprintf(settings,64,"%s.cs",proFile);
			//Con::executef(2, "exec", settings);
			execScript(settings);
		}
		else
		{
   		U16 adjust;

   		// match clock with profile
   		for (i = 0; i < sNumProcessors; ++i)
   		{
      		adjust = sProcessorProfiles[i].adjust;

      		if (Platform::SystemInfo.processor.mhz < sProcessorProfiles[i].clock) break;
   		}

   		const char *settings;

   		// match performance metric with profile
   		for (i = 0; i < sNumSettings; ++i)
   		{
      		settings = sSettingProfiles[i].settings;

      		if (glProfile+adjust <= sSettingProfiles[i].performance) break;
   		}

   		//Con::executef(2, "exec", settings);
			execScript(settings);
		}

		bool match = false;

		for (i = 0; i < sOSCardProfiles.size(); ++i)
			if (dStrstr(vendor, sOSCardProfiles[i].vendor) &&
				 (!dStrcmp(sOSCardProfiles[i].renderer, "*") ||
         	  dStrstr(renderer, sOSCardProfiles[i].renderer)))
			{
				Con::setBoolVariable("$pref::Video::allowOpenGL", sOSCardProfiles[i].allowOpenGL);
				Con::setBoolVariable("$pref::Video::allowD3D", sOSCardProfiles[i].allowD3D);
				Con::setBoolVariable("$pref::Video::preferOpenGL", sOSCardProfiles[i].preferOpenGL);
			
				match = true;

				break;
			}

		if (!match)
		{
			Con::setBoolVariable("$pref::Video::allowOpenGL", true);
			Con::setBoolVariable("$pref::Video::allowD3D", true);
			Con::setBoolVariable("$pref::Video::preferOpenGL", true);
		}

		Con::setVariable("$pref::Video::defaultsVendor", vendor);
   	Con::setVariable("$pref::Video::defaultsRenderer", renderer);
	}

   clearCardProfiles();
	clearOSCardProfiles();
}    

//------------------------------------------------------------------------------
OpenGLDevice::OpenGLDevice()
{
   initDevice();
}


//------------------------------------------------------------------------------
void OpenGLDevice::initDevice()
{
   // Set the device name:
   mDeviceName = "OpenGL";

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
           && ( devMode.dmBitsPerPel == 16 || devMode.dmBitsPerPel == 32 ) &&
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
bool OpenGLDevice::activate( U32 width, U32 height, U32 bpp, bool fullScreen )
{
   Con::printf( "Activating the OpenGL display device..." );

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
         AssertFatal( false, "OpenGLDevice::activate\nqwglMakeCurrent( NULL, NULL ) failed!" );
         return false;
      }

      Con::printf( "Deleting the rendering context ..." );
      if ( !qwglDeleteContext( winState.hGLRC ) )
      {
         AssertFatal( false, "OpenGLDevice::activate\nqwglDeleteContext failed!" );
         return false;
      }
      winState.hGLRC = NULL;
   }

   // If the window already exists, kill it so we can start fresh:
   if ( winState.appWindow )
   {
      if ( winState.appDC )
      {
         Con::printf( "Releasing the device context..." );
         ReleaseDC( winState.appWindow, winState.appDC );
         winState.appDC = NULL;
      }

      Con::printf( "Destroying the window..." );
      DestroyWindow( winState.appWindow );
      winState.appWindow = NULL;
   }

   // If OpenGL library already loaded, shut it down and reload it:
   if ( winState.hinstOpenGL )
      QGL_Shutdown();

   QGL_Init( "opengl32", "glu32" );

	static bool onceAlready = false;
	bool profiled = false;

	if ( !mFullScreenOnly && fullScreen && !onceAlready )
	{	
		OSVERSIONINFO OSVersionInfo;

		// HACK: The Voodoo5 on W2K will only work if the initial rendering
		// context is windowed.  Can you believe this crap?
		dMemset( &OSVersionInfo, 0, sizeof( OSVERSIONINFO ) );
		OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
		if ( GetVersionEx( &OSVersionInfo ) &&
		  	  OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		  	  OSVersionInfo.dwMajorVersion == 5 )
			if ( !setScreenMode(	640, 480, bpp, false, true, false ) )
				return false;
			else
			{
				const char* vendorString   = (const char*) glGetString( GL_VENDOR );
				const char* rendererString = (const char*) glGetString( GL_RENDERER );

				// only do this for the first session
				if (!Con::getBoolVariable("$DisableSystemProfiling") &&
					 ( dStrcmp(vendorString, Con::getVariable("$pref::Video::profiledVendor")) ||
       				dStrcmp(rendererString, Con::getVariable("$pref::Video::profiledRenderer")) ))
				{
       			profileSystem(vendorString, rendererString);
					profiled = true;
				}
			}

		onceAlready = true;
	}
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

   // only do this for the first session
   if (!profiled &&
   	 !Con::getBoolVariable("$DisableSystemProfiling") &&
   	 (	dStrcmp(vendorString, Con::getVariable("$pref::Video::profiledVendor")) ||
       	dStrcmp(rendererString, Con::getVariable("$pref::Video::profiledRenderer")) ))
   {
      profileSystem(vendorString, rendererString);
		profiled = true;
	}

	if (profiled)
	{      
      U32 width, height, bpp;

		if (Con::getBoolVariable("$pref::Video::clipHigh", false))
			for (S32 i = mResolutionList.size()-1; i >= 0; --i)
				if (mResolutionList[i].w > 1152 || mResolutionList[i].h > 864)
					mResolutionList.erase(i);

		if (Con::getBoolVariable("$pref::Video::only16", false))
			for (S32 i = mResolutionList.size()-1; i >= 0; --i)
				if (mResolutionList[i].bpp == 32)
					mResolutionList.erase(i);
      
      dSscanf(Con::getVariable("$pref::Video::resolution"), "%d %d %d", &width, &height, &bpp);
      setScreenMode(width, height, bpp,
                    Con::getBoolVariable("$pref::Video::fullScreen", true), false, false);
   }

   // Do this here because we now know about the extensions:
   if ( gGLState.suppSwapInterval )
      setVerticalSync( !Con::getBoolVariable( "$pref::Video::disableVerticalSync" ) );
	Con::setBoolVariable("$pref::OpenGL::allowTexGen", true);

   return true;
}


//------------------------------------------------------------------------------
void OpenGLDevice::shutdown()
{
   Con::printf( "Shutting down the OpenGL display device..." );

   if ( winState.hGLRC )
   {
      if (mRestoreGamma)
         SetDeviceGammaRamp(winState.appDC, mOriginalRamp);

      Con::printf( "Making the GL rendering context not current..." );
      qwglMakeCurrent( NULL, NULL );
		if ( Con::getBoolVariable("$pref::Video::deleteContext", true) )
      {
         Con::printf( "Deleting the GL rendering context..." );
			qwglDeleteContext( winState.hGLRC );
      }
      winState.hGLRC = NULL;
   }

   if ( winState.appDC )
   {
      Con::printf( "Releasing the device context..." );
      ReleaseDC( winState.appWindow, winState.appDC );
      winState.appDC = NULL;
   }

   if ( smIsFullScreen )
   {
      Con::printf( "Restoring the desktop display settings (%dx%dx%d)...", winState.desktopWidth, winState.desktopHeight, winState.desktopBitsPixel );
      ChangeDisplaySettings( NULL, 0 );
   }
}


//------------------------------------------------------------------------------
// This is the real workhorse function of the DisplayDevice...
//
bool OpenGLDevice::setScreenMode( U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint )
{
   HWND curtain = NULL;
   char errorMessage[256];
   Resolution newRes( width, height, bpp );
   bool newFullScreen = fullScreen;
   bool safeModeOn = Con::getBoolVariable( "$pref::Video::safeModeOn" );

   if ( !newFullScreen && mFullScreenOnly )
   {
      Con::warnf( ConsoleLogEntry::General, "OpenGLDevice::setScreenMode - device or desktop color depth does not allow windowed mode!" );
      newFullScreen = true;
   }

   if ( !newFullScreen && ( newRes.w >= winState.desktopWidth || newRes.h >= winState.desktopHeight ) )
   {
      Con::warnf( ConsoleLogEntry::General, "OpenGLDevice::setScreenMode -- can't switch to resolution larger than desktop in windowed mode!" );
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
      Con::warnf( ConsoleLogEntry::General, "OpenGLDevice::setScreenMode -- can't go smaller than 640x480!" );
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
   {
      // Basically ignore the bit depth parameter:
      newRes.bpp = winState.desktopBitsPixel;
   }

   // Return if already at this resolution:
   if ( !forceIt && newRes == smCurrentRes && newFullScreen == smIsFullScreen )
      return true;

   Con::printf( "Setting screen mode to %dx%dx%d (%s)...", newRes.w, newRes.h, newRes.bpp, ( newFullScreen ? "fs" : "w" ) );

   bool needResurrect = false;

   if ( ( newRes.bpp != smCurrentRes.bpp ) || ( safeModeOn && ( ( smIsFullScreen != newFullScreen ) || newFullScreen ) ) )
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
            AssertFatal( false, "OpenGLDevice::setScreenMode\nqwglMakeCurrent( NULL, NULL ) failed!" );
            return false;
         }

         Con::printf( "Deleting the rendering context..." );
         if ( Con::getBoolVariable("$pref::Video::deleteContext",true) &&
         	  !qwglDeleteContext( winState.hGLRC ) )
         {
            AssertFatal( false, "OpenGLDevice::setScreenMode\nqwglDeleteContext failed!" );
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
         Con::errorf( "SetWindowLong failed to change the window style!" );
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
         Con::errorf( ConsoleLogEntry::General, "OpenGLDevice::setScreenMode - ChangeDisplaySettings failed." );
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
   else if ( smIsFullScreen )
   {
      Con::printf( "Changing to the desktop display settings (%dx%dx%d)...", winState.desktopWidth, winState.desktopHeight, winState.desktopBitsPixel );
      ChangeDisplaySettings( NULL, 0 );
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
         AssertFatal( false, "OpenGLDevice::setScreenMode\nFailed to create a new window!" );
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
         dSprintf( errorMessage, 255, "OpenGLDevice::setScreenMode\nSetWindowPos failed trying to move the window to (%d,%d) and size it to %dx%d.", xPos, yPos, newRes.w, newRes.h ); 
         AssertFatal( false, errorMessage );
         return false;
      }
   }
   else if ( !newWindow )
   {
      // Move and size the window to take up the whole screen:
      if ( !SetWindowPos( winState.appWindow, 0, 0, 0, newRes.w, newRes.h, SWP_NOZORDER ) )
      {
         dSprintf( errorMessage, 255, "OpenGLDevice::setScreenMode\nSetWindowPos failed to move the window to (0,0) and size it to %dx%d.", newRes.w, newRes.h );
         AssertFatal( false, errorMessage );
         return false;
      }
   }

   bool newDeviceContext = false;
   if ( !winState.appDC )
   {
      // Get a new device context:
      Con::printf( "Acquiring a new device context..." );
      winState.appDC = GetDC( winState.appWindow );
      if ( !winState.appDC )
      {
         AssertFatal( false, "OpenGLDevice::setScreenMode\nGetDC failed to get a valid device context!" );
         return false;
      }
      newDeviceContext = true;
   }

   if ( newWindow )
   { 
      // Set the pixel format of the new window:
      PIXELFORMATDESCRIPTOR pfd;
      CreatePixelFormat( &pfd, newRes.bpp, 24, 8, false );
      S32 chosenFormat = ChooseBestPixelFormat( winState.appDC, &pfd );
      if ( !chosenFormat )
      {
         AssertFatal( false, "OpenGLDevice::setScreenMode\nNo valid pixel formats found!" );
         return false;
      }
      qwglDescribePixelFormat( winState.appDC, chosenFormat, sizeof( pfd ), &pfd );
      if ( !SetPixelFormat( winState.appDC, chosenFormat, &pfd ) )
      {
         AssertFatal( false, "OpenGLDevice::setScreenMode\nFailed to set the pixel format!" );
         return false;
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
         AssertFatal( false, "OpenGLDevice::setScreenMode\nqwglCreateContext failed to create an OpenGL rendering context!" );
         return false;
      }   

      // Make the new rendering context current:
      Con::printf( "Making the new rendering context current..." );
      if ( !qwglMakeCurrent( winState.appDC, winState.hGLRC ) )
      {
			AssertFatal( false, "OpenGLDevice::setScreenMode\nqwglMakeCurrent failed to make the rendering context current!" );
			return false;
      }

      // Just for kicks.  Seems a relatively central place to put this...
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

      if ( needResurrect )
      {
         // Reload the textures:
         Con::printf( "Resurrecting the texture manager..." );
         Game->textureResurrect();
      }
   }

   // Just for kicks.  Seems a relatively central place to put this...
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   if ( newDeviceContext && gGLState.suppSwapInterval )
      setVerticalSync( !Con::getBoolVariable( "$pref::Video::disableVerticalSync" ) );

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
void OpenGLDevice::swapBuffers()
{
   qwglSwapBuffers( winState.appDC );
}


//------------------------------------------------------------------------------
const char* OpenGLDevice::getDriverInfo()
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
bool OpenGLDevice::getGammaCorrection(F32 &g)
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
bool OpenGLDevice::setGammaCorrection(F32 g)
{
   U16 ramp[256*3];

   for (U16 i = 0; i < 256; ++i)
      ramp[i] = mPow((F32) i/256.0f, g) * 65535.0f;
   dMemcpy(&ramp[256],ramp,256*sizeof(U16));
   dMemcpy(&ramp[512],ramp,256*sizeof(U16));      

   return SetDeviceGammaRamp(winState.appDC, ramp);
}

//------------------------------------------------------------------------------
bool OpenGLDevice::setVerticalSync( bool on )
{
   if ( !gGLState.suppSwapInterval )
      return( false );

   return( qwglSwapIntervalEXT( on ? 1 : 0 ) );
}

//------------------------------------------------------------------------------
DisplayDevice* OpenGLDevice::create()
{
   bool result = false;
   bool fullScreenOnly = false;

   OSVERSIONINFO OSVersionInfo;
	U32 switchedNT = false;

   dMemset( &OSVersionInfo, 0, sizeof( OSVERSIONINFO ) );
   OSVersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
   if ( GetVersionEx( &OSVersionInfo ) &&
        OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
   {
      DEVMODE devMode;

      dMemset( &devMode, 0, sizeof( devMode ) );
      devMode.dmSize = sizeof( devMode );
      devMode.dmBitsPerPel = winState.desktopBitsPixel == 16 ? 32 : 16;
      devMode.dmFields = DM_BITSPERPEL;
      
      // attempt bit-depth change to test Windows 95B
      if ( ChangeDisplaySettings( &devMode, CDS_TEST ) != DISP_CHANGE_SUCCESSFUL )
         smCanSwitchBitDepth = false;
   }
	// switching NT to 32-bit desktop to determine if we have a 16-bit card
	else if (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
				OSVersionInfo.dwMajorVersion == 4 &&
				winState.desktopBitsPixel != 32)
	{
		DEVMODE devMode;

		dMemset( &devMode, 0, sizeof( devMode ) );
		devMode.dmSize       = sizeof( devMode );
		devMode.dmBitsPerPel = 32;
		devMode.dmFields     = DM_BITSPERPEL;

		switchedNT = ChangeDisplaySettings( &devMode, 0 ) == DISP_CHANGE_SUCCESSFUL;
	}

   // This shouldn't happen, but just to be safe...
   if ( winState.hinstOpenGL )
      QGL_Shutdown();

   if (!QGL_Init( "opengl32", "glu32" ))
	{
		if (switchedNT)
			ChangeDisplaySettings( NULL, 0 );

		return NULL;
	}

   // Create a test window to see if OpenGL hardware acceleration is available:
   WNDCLASSA wc;
   dMemset(&wc, 0, sizeof(wc));   
   wc.style         = CS_OWNDC;
   wc.lpfnWndProc   = DefWindowProc;
   wc.hInstance     = winState.appInstance;
   wc.lpszClassName = "OGLTest";
   RegisterClassA( &wc );

   HWND testWindow = CreateWindowA( 
      "OGLTest", 
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
         CreatePixelFormat( &pfd, 16, 16, 8, false );
         U32 chosenFormat = ChooseBestPixelFormat( testDC, &pfd );
         if ( chosenFormat != 0 )
         {
            qwglDescribePixelFormat( testDC, chosenFormat, sizeof( pfd ), &pfd );

            result = !( pfd.dwFlags & PFD_GENERIC_FORMAT );
  
  				if ( result && (winState.desktopBitsPixel == 16 || pfd.cColorBits == 16) )
					D3DDevice::smStay16 = true;
         
            if ( result && winState.desktopBitsPixel != 16 && !smCanSwitchBitDepth)
            {
               // If Windows 95 cannot switch bit depth, it should only attempt 16-bit cards
               // with a 16-bit desktop

               // See if we can get a 32-bit pixel format:
               PIXELFORMATDESCRIPTOR pfd;

               CreatePixelFormat( &pfd, 32, 24, 8, false );
               S32 chosenFormat = ChooseBestPixelFormat( testDC, &pfd );
               if ( chosenFormat != 0 )
               {
                  qwglDescribePixelFormat( winState.appDC, chosenFormat, sizeof( pfd ), &pfd );
      
                  if (pfd.cColorBits == 16)
                  {
                     Platform::AlertOK("Requires 16-Bit Desktop",
                                       "You must run in 16-bit color to run a Torque game.\nPlease quit the game, set your desktop color depth to 16-bit\nand then restart the application.");

                     result = false;
                  }
               }
            }
            // Don't allow 16-bit cards to do windowed mode on NT
            else if (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
                     OSVersionInfo.dwMajorVersion == 4 &&
                     result && pfd.cColorBits == 16)
               fullScreenOnly = true;
         }
         else if ( (winState.desktopBitsPixel != 16 || switchedNT) && smCanSwitchBitDepth )
         {
            // Try again after changing the display to 16-bit:
            ReleaseDC( testWindow, testDC );
            DestroyWindow( testWindow );

            DEVMODE devMode;
            dMemset( &devMode, 0, sizeof( devMode ) );
            devMode.dmSize       = sizeof( devMode );
            devMode.dmBitsPerPel = 16;
            devMode.dmFields     = DM_BITSPERPEL;

            U32 test = ChangeDisplaySettings( &devMode, 0 );
            if ( test == DISP_CHANGE_SUCCESSFUL )
            {
               testWindow = CreateWindowA( 
                  "OGLTest", 
                  "", 
                  WS_OVERLAPPED | WS_CAPTION, 
                  0, 0, 640, 480, 
                  NULL, 
                  NULL, 
                  winState.appInstance, 
                  NULL );

               if ( testWindow )
               {
                  testDC = GetDC( testWindow );
                  if ( testDC )
                  {
                     CreatePixelFormat( &pfd, 16, 16, 8, false );
                     chosenFormat = ChooseBestPixelFormat( testDC, &pfd );
                     if ( chosenFormat != 0 )
                     {
                        qwglDescribePixelFormat( testDC, chosenFormat, sizeof( pfd ), &pfd );
 
                        result = !( pfd.dwFlags & PFD_GENERIC_FORMAT );
                        if ( result )
                           fullScreenOnly = true;
                     }
                  }
               }
            }
            ChangeDisplaySettings( NULL, 0 );
				switchedNT = false;
         }
         else if ( winState.desktopBitsPixel != 16 && !smCanSwitchBitDepth )
            Platform::AlertOK("Requires 16-Bit Desktop",
                              "You must run in 16-bit color to run a Torque game.\nPlease quit the game, set your desktop color depth to 16-bit\nand then restart the application.");

         ReleaseDC( testWindow, testDC );
      }
      DestroyWindow( testWindow );
   }

   UnregisterClassA( "OGLTest", winState.appInstance );

   QGL_Shutdown();

	// Return NT to previous desktop bit depth
	if (switchedNT)
		ChangeDisplaySettings( NULL, 0 );

   if ( result )
   {
      OpenGLDevice* newOGLDevice = new OpenGLDevice();
      if ( newOGLDevice )
      {
         newOGLDevice->mFullScreenOnly = fullScreenOnly;
         return (DisplayDevice*) newOGLDevice;
      }
      else 
         return NULL;
   }
   else
      return NULL;
}
