//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _STRIPCACHE_H_
#define _STRIPCACHE_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif


class StripCache
{
   U32    stripIndices[1024];
   U32    stripStarts[512];
   ColorI stripColors[512];
   U32 currIndex;
   U32 currStrip;

  public:
   StripCache() { currIndex = 0; currStrip = 0; }

   // Cache manages locking
   void emitStrip(const U32 start, const U32 end, const ColorI& color);
   void flushCache();
};

#endif  // _H_STRIPCACHE_
