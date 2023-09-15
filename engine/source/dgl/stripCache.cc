//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/stripCache.h"
#include "platform/platformGL.h"

void StripCache::emitStrip(const U32 start, const U32 count, const ColorI& color)
{
   if (count < 3) {
      AssertFatal(false, "Strip count < 3");
      return;
   }

   if (currIndex + count >= 1024)
      flushCache();

   // Contiguous index enforcement
   if (currIndex != 0 && start != (stripIndices[currIndex - 1]+1))
      flushCache();

   stripStarts[currStrip]   = currIndex;
   stripColors[currStrip++] = color;
   for (U32 i = start; i < start+count; i++)
      stripIndices[currIndex++] = i;
}

void StripCache::flushCache()
{
   if (currIndex == 0)
      return;

   // We know that (for right now) the first index is the least, and the
   //  last is the greatest.  The commented condition in the emitStrip
   //  call makes sure this range is contiguous...
   U32 first = stripIndices[0];
   U32 last  = stripIndices[currIndex-1];

   stripStarts[currStrip] = currIndex;

   if (dglDoesSupportCompiledVertexArray())
      glLockArraysEXT(first, last - first + 1);

   for (U32 i = 0; i < currStrip; i++) {
      glColor4ubv(stripColors[i]);
      glDrawElements(GL_TRIANGLE_STRIP, stripStarts[i+1] - stripStarts[i],
                     GL_UNSIGNED_INT, &stripIndices[stripStarts[i]]);
   }

   if (dglDoesSupportCompiledVertexArray())
      glUnlockArraysEXT();

   currIndex = 0;
   currStrip = 0;
}

