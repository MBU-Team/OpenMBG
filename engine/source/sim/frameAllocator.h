//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _FRAMEALLOCATOR_H_
#define _FRAMEALLOCATOR_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// Temporary memory pool for per-frame allocations.
///
/// In the course of rendering a frame, it is often necessary to allocate
/// many small chunks of memory, then free them all in a batch. For instance,
/// say we're allocating storage for some vertex calculations:
///
/// @code
///   // Get FrameAllocator memory...
///   U32 waterMark = FrameAllocator::getWaterMark();
///   F32 * ptr = (F32*)FrameAllocator::alloc(sizeof(F32)*2*targetMesh->vertsPerFrame);
///
///   ... calculations ...
///
///   // Free frameAllocator memory
///   FrameAllocator::setWaterMark(waterMark);
/// @endcode
class FrameAllocator
{
   static U8*   smBuffer;
   static U32   smHighWaterMark;
   static U32   smWaterMark;

  public:
   inline static void init(const U32 frameSize);
   inline static void destroy();

   inline static void* alloc(const U32 allocSize);

   inline static void setWaterMark(const U32);
   inline static U32  getWaterMark();
   inline static U32  getHighWaterMark();
};

#if defined(TORQUE_DEBUG)
static S32 sgMaxFrameAllocation = 0;
#endif

void FrameAllocator::init(const U32 frameSize)
{
   AssertFatal(smBuffer == NULL, "Error, already initialized");
   smBuffer = new U8[frameSize];
   smWaterMark = 0;
   smHighWaterMark = frameSize;
}

void FrameAllocator::destroy()
{
   AssertFatal(smBuffer != NULL, "Error, not initialized");
   
   delete [] smBuffer;
   smBuffer = NULL;
   smWaterMark = 0;
   smHighWaterMark = 0;
}


void* FrameAllocator::alloc(const U32 allocSize)
{
   AssertFatal(smBuffer != NULL, "Error, no buffer!");
   AssertFatal(smWaterMark + allocSize <= smHighWaterMark, "Error alloc too large, increase frame size!");

   U8* p = &smBuffer[smWaterMark];
   smWaterMark += allocSize;

#if defined(TORQUE_DEBUG)
   if (smWaterMark > sgMaxFrameAllocation)
      sgMaxFrameAllocation = smWaterMark;
#endif
   
   return p;
}


void FrameAllocator::setWaterMark(const U32 waterMark)
{
   AssertFatal(waterMark < smHighWaterMark, "Error, invalid waterMark");

   smWaterMark = waterMark;
}

U32 FrameAllocator::getWaterMark()
{
   return smWaterMark;
}

U32 FrameAllocator::getHighWaterMark()
{
   return smHighWaterMark;
}


#endif  // _H_FRAMEALLOCATOR_
