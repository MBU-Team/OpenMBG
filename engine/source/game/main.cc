//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/platformVideo.h"
#include "platform/platformInput.h"
#include "platform/platformAudio.h"
#include "platform/event.h"
#include "platform/gameInterface.h"
#include "core/tVector.h"
#include "math/mMath.h"
#include "dgl/dgl.h"
#include "dgl/gBitmap.h"
#include "core/resManager.h"
#include "core/fileStream.h"
#include "dgl/gTexManager.h"
#include "dgl/gFont.h"
#include "console/console.h"
#include "console/simBase.h"
#include "gui/guiCanvas.h"
#include "sim/actionMap.h"
#include "core/dnet.h"
#include "game/game.h"
#include "core/bitStream.h"
#include "console/telnetConsole.h"
#include "console/telnetDebugger.h"
#include "console/consoleTypes.h"
#include "math/mathTypes.h"
#include "dgl/gTexManager.h"
#include "core/resManager.h"
#include "interior/interiorRes.h"
#include "interior/interiorInstance.h"
#include "ts/tsShapeInstance.h"
#include "terrain/terrData.h"
#include "terrain/terrRender.h"
#include "editor/terraformer.h"
#include "sceneGraph/sceneGraph.h"
#include "dgl/materialList.h"
#include "sceneGraph/sceneRoot.h"
#include "game/moveManager.h"
#include "platform/platformVideo.h"
#include "dgl/materialPropertyMap.h"
#include "sim/netStringTable.h"
#include "sim/pathManager.h"
#include "game/gameFunctions.h"
#include "game/fx/particleEngine.h"
#include "platform/platformRedBook.h"
#include "game/demoGame.h"
#include "sim/decalManager.h"
#include "sim/frameAllocator.h"
#include "sceneGraph/detailManager.h"
#include "interior/interiorLMManager.h"
#include "game/version.h"
#include "platform/profiler.h"
#include "game/shapeBase.h"
#include "game/objectTypes.h"
#include "game/net/serverQuery.h"
#include "game/ambientAudioManager.h"
#include "game/gameConnection.h"

#ifndef BUILD_TOOLS
DemoGame GameObject;
DemoNetInterface GameNetInterface;
#endif

extern ResourceInstance *constructTerrainFile(Stream &stream);
extern ResourceInstance *constructTSShape(Stream &);

IMPLEMENT_CONOBJECT(Terraformer);

ConsoleFunctionGroupBegin( Platform , "General platform functions.");

ConsoleFunction( lockMouse, void, 2, 2, "(bool isLocked)"
                "Lock the mouse (or not, depending on the argument's value) to the window.")
{
   Platform::setWindowLocked(dAtob(argv[1]));
}

ConsoleFunction( setNetPort, bool, 2, 2, "(int port)"
                "Set the network port for the game to use.")
{
   return Net::openPort(dAtoi(argv[1]));
}

ConsoleFunction( createCanvas, bool, 2, 2, "(string windowTitle)"
                "Create the game window/canvas, with the specified window title.")
{
   AssertISV(!Canvas, "cCreateCanvas: canvas has already been instantiated");

#if !defined(TORQUE_OS_MAC) // macs can only run one instance in general.
#if !defined(TORQUE_DEBUG) && !defined(INTERNAL_RELEASE)
   if(!Platform::excludeOtherInstances("TorqueTest"))
      return false;
#endif
#endif
   Platform::initWindow(Point2I(800, 600), argv[1]);

   // create the canvas, and add it to the manager
   Canvas = new GuiCanvas();
   Canvas->registerObject("Canvas"); // automatically adds to GuiGroup
   return true;
}

ConsoleFunction( saveJournal, void, 2, 2, "(string filename)"
                "Save the journal to the specified file.")
{
   Game->saveJournal(argv[1]);
}

ConsoleFunction( playJournal, void, 2, 3, "(string filename, bool break=false)"
                "Begin playback of a journal from a specified field, optionally breaking at the start.")
{
   bool jBreak = (argc > 2)? dAtob(argv[2]): false;
   Game->playJournal(argv[1],jBreak);
}

extern void netInit();
extern void processConnectedReceiveEvent( ConnectedReceiveEvent * event );
extern void processConnectedNotifyEvent( ConnectedNotifyEvent * event );
extern void processConnectedAcceptEvent( ConnectedAcceptEvent * event );
extern void ShowInit();

/// Initalizes the components of the game like the TextureManager, ResourceManager
/// console...etc.
static bool initLibraries()
{
   if(!Net::init())
   {
      Platform::AlertOK("Network Error", "Unable to initialize the network... aborting.");
      return false;
   }

   // asserts should be created FIRST
   PlatformAssert::create();

   FrameAllocator::init(3 << 20);      // 3 meg frame allocator buffer

//   // Cryptographic pool next
//   CryptRandomPool::init();

   _StringTable::create();
   TextureManager::create();
   ResManager::create();

   // Register known file types here
   ResourceManager->registerExtension(".jpg", constructBitmapJPEG);
   ResourceManager->registerExtension(".png", constructBitmapPNG);
   ResourceManager->registerExtension(".gif", constructBitmapGIF);
   ResourceManager->registerExtension(".dbm", constructBitmapDBM);
   ResourceManager->registerExtension(".bmp", constructBitmapBMP);
   ResourceManager->registerExtension(".bm8", constructBitmapBM8);
   ResourceManager->registerExtension(".gft", constructFont);
   ResourceManager->registerExtension(".dif", constructInteriorDIF);
   ResourceManager->registerExtension(".ter", constructTerrainFile);
   ResourceManager->registerExtension(".dts", constructTSShape);
   ResourceManager->registerExtension(".dml", constructMaterialList);

   RegisterCoreTypes();
   RegisterMathTypes();
   RegisterGuiTypes();
   
   Con::init();
   NetStringTable::create();
   
   TelnetConsole::create();
   TelnetDebugger::create();

   Processor::init();
   Math::init();
   Platform::init();    // platform specific initialization
   InteriorLMManager::init();
   InteriorInstance::init();
   TSShapeInstance::init();
   RedBook::init();
   
   return true;
}


/// Destroys all the things initalized in initLibraries
static void shutdownLibraries()
{
   // Purge any resources on the timeout list...
   if (ResourceManager)
      ResourceManager->purge();

   RedBook::destroy();
   TSShapeInstance::destroy();
   InteriorInstance::destroy();
   InteriorLMManager::destroy();

   TextureManager::preDestroy();
   
   Platform::shutdown();
   TelnetDebugger::destroy();
   TelnetConsole::destroy();

   NetStringTable::destroy();
   Con::shutdown();

   ResManager::destroy();
   TextureManager::destroy();

   _StringTable::destroy();

   // asserts should be destroyed LAST
   FrameAllocator::destroy();

   PlatformAssert::destroy();
   Net::shutdown();
}

ConsoleFunction( setModPaths, void, 2, 2, "(string paths)"
                "Set the mod paths the resource manager is using. These are semicolon delimited.")
{
   char buf[512];
   dStrncpy(buf, argv[1], sizeof(buf) - 1);
   buf[511] = '\0';

   Vector<char*> paths;
   char* temp = dStrtok( buf, ";" );
   while ( temp )
   {
      if ( temp[0] )
         paths.push_back( temp );
      temp = dStrtok( NULL, ";" );
   }

   ResourceManager->setModPaths( paths.size(), (const char**) paths.address() );
}

ConsoleFunction( getModPaths, const char*, 1, 1, "Return the mod paths the resource manager is using.")
{
   return( ResourceManager->getModPaths() );
}

ConsoleFunction( getVersionNumber, S32, 1, 1, "Get the version of the build, as a string.")
{
   return getVersionNumber();
}

ConsoleFunction( getVersionString, const char*, 1, 1, "Get the version of the build, as a string.")
{
   return getVersionString();
}

ConsoleFunction( getCompileTimeString, const char*, 1, 1, "Get the time of compilation.")
{
   return getCompileTimeString();
}

ConsoleFunction( getBuildString, const char*, 1, 1, "Get the type of build, \"Debug\" or \"Release\".")
{
#ifdef TORQUE_DEBUG
   return "Debug";
#else
   return "Release";
#endif
}

ConsoleFunction( getSimTime, S32, 1, 1, "Return the current sim time in milliseconds.\n\n"
                "Sim time is time since the game started.")
{
   return Sim::getCurrentTime();
}

ConsoleFunction( getRealTime, S32, 1, 1, "Return the current real time in milliseconds.\n\n"
                "Real time is platform defined; typically time since the computer booted.")
{
   return Platform::getRealMilliseconds();
}

ConsoleFunctionGroupEnd(Platform);

static F32 gTimeScale = 1.0;
static U32 gTimeAdvance = 0;
static U32 gFrameSkip = 0;
static U32 gFrameCount = 0;
static bool gGamePaused = false;

/// Initalize game, run main.cs startup script
bool initGame()
{
   Con::setFloatVariable("Video::texResidentPercentage", -1.0f);
   Con::setIntVariable("Video::textureCacheMisses", -1);
   Con::addVariable("gamePaused", TypeBool, &gGamePaused);
   Con::addVariable("timeScale", TypeF32, &gTimeScale);
   Con::addVariable("timeAdvance", TypeS32, &gTimeAdvance);
   Con::addVariable("frameSkip", TypeS32, &gFrameSkip);

   // Stuff game types into the console
   Con::setIntVariable("$TypeMasks::StaticObjectType",         StaticObjectType);
   Con::setIntVariable("$TypeMasks::EnvironmentObjectType",    EnvironmentObjectType);
   Con::setIntVariable("$TypeMasks::TerrainObjectType",        TerrainObjectType);
   Con::setIntVariable("$TypeMasks::InteriorObjectType",       InteriorObjectType);
   Con::setIntVariable("$TypeMasks::WaterObjectType",          WaterObjectType);
   Con::setIntVariable("$TypeMasks::TriggerObjectType",        TriggerObjectType);
   Con::setIntVariable("$TypeMasks::MarkerObjectType",         MarkerObjectType);
   Con::setIntVariable("$TypeMasks::GameBaseObjectType",       GameBaseObjectType);
   Con::setIntVariable("$TypeMasks::ShapeBaseObjectType",      ShapeBaseObjectType);
   Con::setIntVariable("$TypeMasks::CameraObjectType",         CameraObjectType);
   Con::setIntVariable("$TypeMasks::StaticShapeObjectType",    StaticShapeObjectType);
   Con::setIntVariable("$TypeMasks::PlayerObjectType",         PlayerObjectType);
   Con::setIntVariable("$TypeMasks::ItemObjectType",           ItemObjectType);
   Con::setIntVariable("$TypeMasks::VehicleObjectType",        VehicleObjectType);
   Con::setIntVariable("$TypeMasks::VehicleBlockerObjectType", VehicleBlockerObjectType);
   Con::setIntVariable("$TypeMasks::ProjectileObjectType",     ProjectileObjectType);
   Con::setIntVariable("$TypeMasks::ExplosionObjectType",      ExplosionObjectType);
   Con::setIntVariable("$TypeMasks::CorpseObjectType",         CorpseObjectType);
   Con::setIntVariable("$TypeMasks::DebrisObjectType",         DebrisObjectType);
   Con::setIntVariable("$TypeMasks::PhysicalZoneObjectType",   PhysicalZoneObjectType);
   Con::setIntVariable("$TypeMasks::StaticTSObjectType",       StaticTSObjectType);
   Con::setIntVariable("$TypeMasks::StaticRenderedObjectType", StaticRenderedObjectType);
   Con::setIntVariable("$TypeMasks::DamagableItemObjectType",  DamagableItemObjectType);

   //
#ifdef GATHER_METRICS
   Con::addVariable("Video::numTexelsLoaded", TypeS32, &TextureManager::smTextureSpaceLoaded);
#else
   static U32 sBogusNTL = 0;
   Con::addVariable("Video::numTexelsLoaded", TypeS32, &sBogusNTL);
#endif

   TerrainRender::init();
   netInit();
   GameInit();
   ShowInit();
   MoveManager::init();

   Sim::init();

   ActionMap* globalMap = new ActionMap;
   globalMap->registerObject("GlobalActionMap");
   Sim::getActiveActionMapSet()->pushObject(globalMap);

   MaterialPropertyMap *map = new MaterialPropertyMap;

   map->registerObject("MaterialPropertyMap");
   Sim::getRootGroup()->addObject(map);

   gClientSceneGraph = new SceneGraph(true);
   gClientSceneRoot  = new SceneRoot;
   gClientSceneGraph->addObjectToScene(gClientSceneRoot);
   gServerSceneGraph = new SceneGraph(false);
   gServerSceneRoot  = new SceneRoot;
   gServerSceneGraph->addObjectToScene(gServerSceneRoot);
   gDecalManager = new DecalManager;
   gClientContainer.addObject(gDecalManager);
   gClientSceneGraph->addObjectToScene(gDecalManager);

   DetailManager::init();
   PathManager::init();
   ParticleEngine::init();
 
   // Execute the main.cs script, the script is not compiled,
   // and is loaded here directly because access to the "root"
   // directory through the resource system is restricted.
   FileStream str;
   const char* mainCS = "main.cs";
   if(!str.open(mainCS, FileStream::Read))
   {
      char msg[1024];
      dSprintf(msg, sizeof(msg), "Failed to open \"%s\".", mainCS);
      Platform::AlertOK("Error", msg);
      return false;
   }

   U32 size = str.getStreamSize();
   char *script = new char[size + 1];
   str.read(size, script);
   str.close();

   script[size] = 0;
   Con::executef(2, "eval", script);
   delete[] script;
   return true;
}

/// Shutdown the game and delete core objects
void shutdownGame()
{
   //exec the script onExit() function
   Con::executef(1, "onExit");

   ParticleEngine::destroy();
   PathManager::destroy();
   DetailManager::shutdown();

   // Note: tho the SceneGraphs are created after the Manager, delete them after, rather
   //  than before to make sure that all the objects are removed from the graph.
   Sim::shutdown();

   gClientSceneGraph->removeObjectFromScene(gDecalManager);
   gClientContainer.removeObject(gDecalManager);
   gClientSceneGraph->removeObjectFromScene(gClientSceneRoot);
   gServerSceneGraph->removeObjectFromScene(gServerSceneRoot);
   delete gClientSceneRoot;
   delete gServerSceneRoot;
   delete gClientSceneGraph;
   delete gServerSceneGraph;
   delete gDecalManager;
   gClientSceneRoot = NULL;
   gServerSceneRoot = NULL;
   gClientSceneGraph = NULL;
   gServerSceneGraph = NULL;
   gDecalManager = NULL;

   TerrainRender::shutdown();
}

extern bool gDGLRender;
bool gShuttingDown   = false;

/// Main loop of the game
int DemoGame::main(int argc, const char **argv)
{
//   if (argc == 1) {
//      static const char* argvFake[] = { "dtest.exe", "-jload", "test.jrn" };
//      argc = 3;                                                               
//      argv = argvFake;
//   }

//   Memory::enableLogging("testMem.log");
//   Memory::setBreakAlloc(104717);

   if(!initLibraries())
      return 0;

#ifdef IHVBUILD
   char* pVer = new char[sgVerStringLen + 1];
   U32 hi;
   for (hi = 0; hi < sgVerStringLen; hi++)
      pVer[hi] = sgVerString[hi] ^ 0xFF;
   pVer[hi] = '\0';

   SHA1Context hashCTX;
   hashCTX.init();
   hashCTX.hashBytes(pVer, sgVerStringLen);
   hashCTX.finalize();

   U8 hash[20];
   hashCTX.getHash(hash);

   for (hi = 0; hi < 20; hi++)
      if (U8(hash[hi]) != U8(sgHashVer[hi]))
         return 0;
#endif

   // Set up the command line args for the console scripts...
   Con::setIntVariable("Game::argc", argc);
   U32 i;
   for (i = 0; i < argc; i++)
      Con::setVariable(avar("Game::argv%d", i), argv[i]);
   if (initGame() == false)
   {
      Platform::AlertOK("Error", "Failed to initialize game, shutting down.");
      shutdownGame();
      shutdownLibraries();
      gShuttingDown = true;
      return 0;
   }

#ifdef IHVBUILD
   char* pPrint = new char[dStrlen(sgVerPrintString) + 1];
   for (U32 pi = 0; pi < dStrlen(sgVerPrintString); pi++)
      pPrint[pi] = sgVerPrintString[pi] ^ 0xff;
   pPrint[dStrlen(sgVerPrintString)] = '\0';

   Con::printf("");
   Con::errorf(ConsoleLogEntry::General, pPrint, pVer);
   delete [] pVer;
#endif

   while(Game->isRunning())
   {
      PROFILE_START(MainLoop);
      Game->journalProcess();
      Net::process();      // read in all events
      Platform::process(); // keys, etc.
      TelConsole->process();
      TelDebugger->process();
      TimeManager::process(); // guaranteed to produce an event
      PROFILE_END();
   }
   shutdownGame();
   shutdownLibraries();

   gShuttingDown = true;

#if 0
// tg: Argh! This OS version check should be part of Platform, not here...
//
   // check os
   OSVERSIONINFO osInfo;
   dMemset(&osInfo, 0, sizeof(OSVERSIONINFO));
   osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

   // see if osversioninfoex fails
   if(!GetVersionEx((OSVERSIONINFO*)&osInfo))
   {
      osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
      if(!GetVersionEx((OSVERSIONINFO*)&osInfo))
         return 0;
   }

   // terminate the process if win95 only!
   if((osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&      // 95, 98, ME
      (osInfo.dwMajorVersion == 4) &&                             // 95, 98, ME, NT
      (osInfo.dwMinorVersion == 0))                               // 95
   {
      AssertWarn(0, "Forcing termination of app (Win95)!  Upgrade your OS now!");
      TerminateProcess(GetCurrentProcess(), 0xffffffff);
   }
#endif

   return 0;
}


static bool serverTick = false;


static F32 fpsRealStart;
static F32 fpsRealLast;
//static F32 fpsRealTotal;
static F32 fpsReal;
static F32 fpsVirtualStart;
static F32 fpsVirtualLast;
//static F32 fpsVirtualTotal;
static F32 fpsVirtual;
static F32 fpsFrames;
static F32 fpsNext;
static bool fpsInit = false;
const F32 UPDATE_INTERVAL = 0.25f;

//--------------------------------------

/// Resets the FPS variables
void fpsReset()
{
   fpsRealStart    = (F32)Platform::getRealMilliseconds()/1000.0f;      // Real-World Tick Count
   fpsVirtualStart = (F32)Platform::getVirtualMilliseconds()/1000.0f;   // Engine Tick Count (does not vary between frames)
   fpsNext         = fpsRealStart + UPDATE_INTERVAL;

//   fpsRealTotal= 0.0f;
   fpsRealLast = 0.0f;
   fpsReal     = 0.0f;
//   fpsVirtualTotal = 0.0f;
   fpsVirtualLast  = 0.0f;
   fpsVirtual      = 0.0f;
   fpsFrames = 0;
   fpsInit   = true;
}   

//--------------------------------------

/// Updates the FPS variables
void fpsUpdate()
{
   if (!fpsInit)
      fpsReset();

   const float alpha  = 0.07f;
   F32 realSeconds    = (F32)Platform::getRealMilliseconds()/1000.0f;
   F32 virtualSeconds = (F32)Platform::getVirtualMilliseconds()/1000.0f;

   fpsFrames++;
   if (fpsFrames > 1)
   {
      fpsReal    = fpsReal*(1.0-alpha) + (realSeconds-fpsRealLast)*alpha;     
      fpsVirtual = fpsVirtual*(1.0-alpha) + (virtualSeconds-fpsVirtualLast)*alpha;     
   }
//   fpsRealTotal    = fpsFrames/(realSeconds - fpsRealStart);
//   fpsVirtualTotal = fpsFrames/(virtualSeconds - fpsVirtualStart);

   fpsRealLast    = realSeconds;
   fpsVirtualLast = virtualSeconds;

   // update variables every few frames
   F32 update = fpsRealLast - fpsNext;
   if (update > 0.5f)
   {
//      Con::setVariable("fps::realTotal",    avar("%4.1f", fpsRealTotal));
//      Con::setVariable("fps::virtualTotal", avar("%4.1f", fpsVirtualTotal));
      Con::setVariable("fps::real",    avar("%4.1f", 1.0f/fpsReal));
      Con::setVariable("fps::virtual", avar("%4.1f", 1.0f/fpsVirtual));
      if (update > UPDATE_INTERVAL)
         fpsNext  = fpsRealLast + UPDATE_INTERVAL;
      else
         fpsNext += UPDATE_INTERVAL;
   }
}   


/// Process a mouse movement event, essentially pass to the canvas for handling
void DemoGame::processMouseMoveEvent(MouseMoveEvent * mEvent)
{
   if (Canvas) 
      Canvas->processMouseMoveEvent(mEvent);
}

/// Process an input event, pass to canvas for handling
void DemoGame::processInputEvent(InputEvent *event)
{
   PROFILE_START(ProcessInputEvent);
   if (!ActionMap::handleEventGlobal(event))
   {
      // Other input consumers here...
      if (!(Canvas && Canvas->processInputEvent(event)))
         ActionMap::handleEvent(event);
   }
   PROFILE_END();
}

/// Process a quit event
void DemoGame::processQuitEvent()
{
   setRunning(false);
}

/// Refresh the game window, ask the canvas to set all regions to dirty (need to be updated)
void DemoGame::refreshWindow()
{
   if(Canvas)
      Canvas->resetUpdateRegions();
}

/// Process a console event
void DemoGame::processConsoleEvent(ConsoleEvent *event)
{
   char *argv[2];
   argv[0] = "eval";
   argv[1] = event->data;
   Sim::postCurrentEvent(Sim::getRootGroup(), new SimConsoleEvent(2, const_cast<const char**>(argv), false));
}

const U32 AudioUpdatePeriod = 125;  ///< milliseconds between audio updates.

Move gFirstMove;
Move gNextMove;

/// Process a time event and update all sub-processes
void DemoGame::processTimeEvent(TimeEvent *event)
{
    U32 elapsedTime = event->elapsedTime;
    // cap the elapsed time to one second
    // if it's more than that we're probably in a bad catch-up situation

    if (elapsedTime > 1024)
        elapsedTime = 1024;

    S32 timeRemaining = elapsedTime;

   // alxUpdate is somewhat expensive and does not need to be updated constantly,
    // though it does need to be updated in real time
   static U32 lastAudioUpdate = 0;
   U32 realTime = Platform::getRealMilliseconds();
   if ((realTime - lastAudioUpdate) >= AudioUpdatePeriod)
   {
       alxUpdate();
       gAmbientAudioManager.update();
       lastAudioUpdate = realTime;
   }
   if (!gGamePaused)
   {
       gFirstMove = gNextMove = NullMove;
       if (NetConnection::getServerConnection() != NULL)
       {
           GameConnection* gc = dynamic_cast<GameConnection*>(NetConnection::getServerConnection());
           if (gc)
           {
               gc->getNextMove(gFirstMove);
               gc->getNextMove(gNextMove);
           }
       }

       if (mDemoWriteStream)
       {
           static char writeBuffer[256];

           BitStream bs(writeBuffer, 256);
           bool areMovesEqual = gFirstMove.x == mLastMove.x
               && gFirstMove.y == mLastMove.y
               && gFirstMove.z == mLastMove.z
               && gFirstMove.yaw == mLastMove.yaw
               && gFirstMove.pitch == mLastMove.pitch
               && gFirstMove.roll == mLastMove.roll
               && (gFirstMove.freeLook > 0) == (mLastMove.freeLook > 0)
               && gFirstMove.trigger[3] == mLastMove.trigger[3]
               && gFirstMove.trigger[1] == mLastMove.trigger[1];
           if (bs.writeFlag(!areMovesEqual))
           {
               mLastMove = gFirstMove;
               mLastMove.pack(&bs);
           }
           areMovesEqual = gNextMove.x == mLastMove.x
               && gNextMove.y == mLastMove.y
               && gNextMove.z == mLastMove.z
               && gNextMove.yaw == mLastMove.yaw
               && gNextMove.pitch == mLastMove.pitch
               && gNextMove.roll == mLastMove.roll
               && (gNextMove.freeLook > 0) == (mLastMove.freeLook > 0)
               && gNextMove.trigger[3] == mLastMove.trigger[3]
               && gNextMove.trigger[1] == mLastMove.trigger[1];
           if (bs.writeFlag(!areMovesEqual))
           {
               mLastMove = gNextMove;
               mLastMove.pack(&bs);
           }
           bs.writeInt(elapsedTime, 10);
           U8 size = bs.getPosition();
           mDemoWriteStream->write(size);
           mDemoWriteStream->write(size, writeBuffer);
           dMemset(writeBuffer, 0, size);
       }
       if (mDemoReadStream)
       {
           static char readBuffer[256];

           U32 demoDelta = mDemoTimeDelta;
           while (timeRemaining > demoDelta)
           {
               timeRemaining -= demoDelta;
               U8 size;
               mDemoReadStream->read(&size);
               mDemoReadStream->read(size, &readBuffer);
               if (mDemoReadStream->getStatus() != Stream::Ok)
               {
                   if (mDemoReadStream)
                       ResourceManager->closeStream(mDemoReadStream);
                   mDemoReadStream = NULL;
                   Con::executef(1, "onDemoPlayDone");
                   break;
               }
               BitStream bs(readBuffer, 256);
               if (bs.readFlag())
                   mCurrentMove.unpack(&bs);
               gFirstMove = mCurrentMove;
               if (bs.readFlag())
                   mCurrentMove.unpack(&bs);
               gNextMove = mCurrentMove;
               U32 timeDelta = bs.readInt(10);
               processElapsedTime(timeDelta);
               mDemoTimeDelta = timeDelta;
               demoDelta = timeDelta;
               if (timeRemaining <= demoDelta)
                   break;

           }
           mDemoTimeDelta = demoDelta - timeRemaining;
       }
       else
       {
           processElapsedTime(elapsedTime);
       }
       GNet->checkTimeouts();
   }
   if (Canvas && gDGLRender)
   {
       bool preRenderOnly = false;
       if (gFrameSkip && gFrameCount % gFrameSkip)
           preRenderOnly = true;

       PROFILE_START(RenderFrame);
       ShapeBase::incRenderFrame();
       Canvas->renderFrame(preRenderOnly);
       PROFILE_END();
       gFrameCount++;
   }
   fpsUpdate();
   // Update the console time
   Con::setFloatVariable("Sim::Time",F32(Platform::getVirtualMilliseconds()) / 1000);
}

void DemoGame::processElapsedTime(U32 elapsedTime)
{
    Platform::advanceTime(elapsedTime);
    serverProcess(elapsedTime);
    GNet->processServer();
    Sim::advanceTime(elapsedTime);
    clientProcess(elapsedTime);
    GNet->processClient();
}

/// Re-activate the game from, say, a minimized state
void GameReactivate()
{
   if ( !Input::isEnabled() )
      Input::enable();

   if ( !Input::isActive() )
      Input::reactivate();

   gDGLRender = true;
   if ( Canvas )
      Canvas->resetUpdateRegions();
}

/// De-activate the game in responce to, say, a minimize event
void GameDeactivate( bool noRender )
{
   if ( Input::isActive() )
      Input::deactivate();

   if ( Input::isEnabled() )
      Input::disable();

   if ( noRender )
      gDGLRender = false;
}

/// Invalidate all the textures
void DemoGame::textureKill()
{
   TextureManager::makeZombie();
   SimGroup* root = Sim::getRootGroup();
   for (SimSetIterator itr(root); *itr; ++itr)
   {
       (*itr)->onVideoKill();
   }
}

/// Reaquire all textures
void DemoGame::textureResurrect()
{
   TextureManager::resurrect();
   SimGroup* root = Sim::getRootGroup();
   for (SimSetIterator itr(root); *itr; ++itr)
   {
       (*itr)->onVideoResurrect();
   }
}

/// Process recieved net-packets
void DemoGame::processPacketReceiveEvent(PacketReceiveEvent * prEvent)
{
   GNet->processPacketReceiveEvent(prEvent);
}

void DemoGame::playDemo(const char* path)
{
    Stream* demoStream = ResourceManager->openStream(path);
    if (demoStream)
    {
        mDemoReadStream = demoStream;
        
        gClientProcessList.timeReset();
        gServerProcessList.timeReset();
        mCurrentMove = NullMove;
        char misPath[256];
        demoStream->readString(misPath);
        Con::executef(2, "onDemoPlay", misPath);
    }
}

void DemoGame::stopDemo()
{
    if (mDemoReadStream)
        ResourceManager->closeStream(mDemoReadStream);
    if (mDemoWriteStream)
        ResourceManager->closeStream(mDemoWriteStream);
    mDemoReadStream = NULL;
    mDemoWriteStream = NULL;
}

void DemoGame::recordDemo(const char* path, const char* mispath)
{
    FileStream* fs = new FileStream();
    if (ResourceManager->openFileForWrite(*fs, path))
    {
        mLastMove = NullMove;
        mDemoWriteStream = fs;
        gClientProcessList.timeReset();
        gServerProcessList.timeReset();
        mDemoWriteStream->writeString(mispath);
    }
}

ConsoleFunction(playDemo, void, 2, 2, "playDemo(path)")
{
    char filePath[1024];
    Con::expandScriptFilename(filePath, 1024, argv[1]);
    GameObject.playDemo(filePath);
}

ConsoleFunction(recordDemo, void, 3, 3, "recordDemo(path)")
{
    char filePath[1024];
    Con::expandScriptFilename(filePath, 1024, argv[1]);
    GameObject.recordDemo(filePath, argv[2]);
}

ConsoleFunction(stopDemo, void, 1, 1, "stopDemo()")
{
    GameObject.stopDemo();
}