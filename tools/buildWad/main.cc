//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/platformAssert.h"
#include "math/mMath.h"
#include "core/resManager.h"
#include "console/console.h"
#include "dgl/gBitmap.h"
#include "dgl/gTexManager.h"
#include "console/consoleTypes.h"
#include "math/mathTypes.h"
#include "platform/event.h"
#include "sim/frameAllocator.h"

#include "buildWad/wadProcessor.h"

//------------------------------------------------------------------------------
#include "platform/gameInterface.h"
class WadToolGame : public GameInterface
{
   public:
      S32 main(S32 argc, const char **argv);
} GameObject;

// FOR SILLY LINK DEPENDANCY
bool gEditingMission = false;
void GameHandleNotify(NetConnectionId, bool)
{
}


#if defined(TORQUE_DEBUG)
   const char * const gProgramVersion = "0.900d-beta";
#else
   const char * const gProgramVersion = "0.900r-beta";
#endif

//------------------------------------------------------------------------------

static bool initLibraries()
{
   // asserts should be created FIRST
   PlatformAssert::create();

   FrameAllocator::init(1 << 20); 
   FrameAllocator::setWaterMark(0);

   _StringTable::create();
   TextureManager::create();

   RegisterCoreTypes();
   RegisterMathTypes();
   
   Con::init();

//   Processor::init();
   Math::init();
   Platform::init();    // platform specific initialization
   return(true);
}

//------------------------------------------------------------------------------

static void shutdownLibraries()
{
   // shut down
   Platform::shutdown();
   Con::shutdown();

   TextureManager::destroy();
   _StringTable::destroy();

   FrameAllocator::destroy();

   // asserts should be destroyed LAST
   PlatformAssert::destroy();
}

//------------------------------------------------------------------------------

void (*terrMipBlit)(U16 *dest, U32 destStride, U32 squareSize, const U8 *sourcePtr, U32 sourceStep, U32 sourceRowAdd) = 0;

struct Event;
void GamePostEvent(const Event& /*event*/)
{
   //
}

S32 WadToolGame::main(int argc, const char ** argv)
{
   if(!initLibraries())
      return(0);

   // give the console the command line
   Con::setIntVariable("Game::argc", argc);
   for(S32 i = 0; i < argc; i++)
      Con::setVariable(avar("Game::argv%d", i), argv[i]);

   // so the debugger can actually do something....
//   if(argc<3)
//   {
//      const char* argvFake[] = { "wadtool", "lushTest", "lushtest.wad"};
//      argc = 3;
//      argv = argvFake;
//   }
   
   // info
   if(argc <= 2)
   {
      dPrintf("\nbuildWad - Torque wad file creator\n"
              "  Copyright (C) GarageGames.com, Inc.\n"
              "  Programming by John Folliard\n"
              "  Program version: %s\n"
              "  Built: %s at %s\n", gProgramVersion, __DATE__, __TIME__);
      
      dPrintf("\n  Usage: buildWad -x <[@]source> <outputFile>.wad\n");
      dPrintf("\n         buildWad -x <directory>|<source.png>... <outputFile>.wad\n");
      dPrintf(  "      -x: allow each texture to have a unique palette.\n");
      shutdownLibraries();
      return(0);
   }

   // create the wad processor - have it write a header then allow the lumps 
   // to write themseleves out, then close and fixup the wad   
   WadProcessor * waddy = new WadProcessor();
   if (!waddy->open(argv[argc-1]))  {
      dPrintf(" *** Could not open wad file: %s\n",argv[argc-1]);
      return(1);
   }
   
   U32 startArg = 1;
   bool singlePalette = true;
   
   // check for a switch
   if(argv[1][0] == '-')
   {
      if(U8(dTolower(argv[1][1])) == 'x')
      {
         startArg++;
         singlePalette = false;
      }
   }
   
   // process the list of bitmaps...
   for(S32 i = startArg; i < (argc-1); i++)
      waddy->addBitmaps(argv[i]);

   //
   if(singlePalette)
      waddy->processSinglePalette();
   else
      waddy->processBitmaps();

   waddy->close();
   delete waddy;
   
   shutdownLibraries();
   return(0);
}


void GameReactivate()
{

}

void GameDeactivate( bool )
{

}
