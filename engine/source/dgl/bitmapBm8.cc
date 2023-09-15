//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/gBitmap.h"
#include "dgl/gPalette.h"
#include "core/stream.h"
#include "platform/platform.h"


bool GBitmap::readBmp8(Stream& stream)
{
   stream.read(&byteSize);
   stream.read(&width);
   stream.read(&height);
   stream.read(&bytesPerPixel);
   stream.read(&numMipLevels);
   U32 i;
   for (i = 0; i < numMipLevels; i++)
      stream.read(&mipLevelOffsets[i]);
   internalFormat = GBitmap::Palettized;

   pPalette = new GPalette;
   pPalette->read(stream);

   pBits = new U8[byteSize];
   stream.read(byteSize, pBits);

   return true;
}

bool GBitmap::writeBmp8(Stream& stream)
{
   AssertFatal(pPalette != NULL, "Error, must have a palette to write the bmp!");

   stream.write(byteSize);
   stream.write(width);
   stream.write(height);
   stream.write(bytesPerPixel);
   stream.write(numMipLevels);
   U32 i;
   for (i = 0; i < numMipLevels; i++)
      stream.write(mipLevelOffsets[i]);
   
   pPalette->write(stream);
   stream.write(byteSize, pBits);

   return true;
}
