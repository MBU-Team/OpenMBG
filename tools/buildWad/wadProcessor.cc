//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "buildWad/wadProcessor.h"

#include "platform/platformAssert.h"
#include "core/fileStream.h"
#include "dgl/gPalette.h"
#include "math/mPoint.h"

#include "buildWad/palQuantization.h"
#include <stdarg.h>

//------------------------------------------------------------------------------
// globals
//------------------------------------------------------------------------------
namespace {
   //
   Vector<char *>    ErrorMessages;
   const char * addErrorMessage(const char * message, ...)
   {
      static char buffer[1024];
      va_list args;
      va_start(args, message);
      dVsprintf(buffer, sizeof(buffer), message, args);

      // copy this string into our vector...
      char * errorCopy = new char[dStrlen(buffer) + 1];
      dStrcpy(errorCopy, buffer);
      ErrorMessages.push_back(errorCopy);

      // spam a message now...
      dPrintf(errorCopy);
      return(errorCopy);
   }

   bool getBaseName(StringTableEntry src, char * dest, U32 destLen)
   {
      AssertFatal(src && dest, "getBaseName: invalid params");

      const char * end = dStrrchr(src, '.');
      AssertISV(end, avar("getBaseName: invalid src file '%s'", src));

      const char * start = dStrrchr(src, '/');
      if (!start)
         start = dStrrchr(src, '\\');
      start ? start++ : start = src;

      if((end - start) >= destLen)
      {
         addErrorMessage("  *** BaseFileName too long for %s [max %d].\n", src, destLen);
         return(false);
      }
      
      U32 count = 0;
      while(start != end)
         dest[count++] = *start++;
      
      dest[count] = '\0';
      return(true);         
   }
   
   //---------------------------------------------------------------------------
   
   bool getPath(StringTableEntry src, char * dest, U32 destLen)
   {
      AssertFatal(src && dest, "getPath: invalid params");
      
      const char * end = dStrrchr(src, '/');
      if (!end)
         end = dStrrchr(src, '\\');
      if(!end)
      {
         AssertFatal(destLen <= 3, "getPath - dest buffer too small.");
         dest[0] = '.';
         dest[1] = '\0';
      }
      
      const char * start = src;
      
      if((end - start) >= destLen)
      {
         addErrorMessage("  *** getPath: path too long for %s [max %d].\n", src, destLen);
         return(false);
      }

      U32 count = 0;
      while(start != end)
         dest[count++] = *start++;
         
      dest[count] = '\0';
      return(true);         
   }

   //---------------------------------------------------------------------------
   GBitmap* loadBitmap(StringTableEntry file)
   {
      const char *dot = dStrrchr(file, '.');
      if (!dot)
         return 0;

      FileStream loadStream;
      loadStream.open(file, FileStream::Read);
      if (loadStream.getStatus() != Stream::Ok)
      {
         addErrorMessage("  *** Unable to load texture: %s.\n", file);
         return 0;
      }

      GBitmap * bitmap = new GBitmap;
      if (!dStricmp(dot, ".png"))
      {
         if(!bitmap->readPNG(loadStream))
         {
            addErrorMessage("  *** Bad PNG file: %s.\n", file);
            delete bitmap;
            return 0;
         }
      }
      else
         if (!dStricmp(dot, ".jpg"))
         {
            if(!bitmap->readJPEG(loadStream))
            {
               addErrorMessage("  *** Bad JPEG file: %s.\n", file);
               delete bitmap;
               return 0;
            }
         }
      return bitmap;
   }
}

//------------------------------------------------------------------------------
// Class WadProcessor
//------------------------------------------------------------------------------

WadProcessor::WadProcessor()
{
   // fill in the header info
   mHeader.mNumLumps = 0;
   mHeader.mInfoTableOffset = 0;
   mHeader.mID = U32('3') << 24 | U32('D') << 16 | U32('A') << 8 | U32('W');
}

WadProcessor::~WadProcessor()
{
   // destroy the error message list...
   for(U32 i = 0; i < ErrorMessages.size(); i++)
      delete [] ErrorMessages[i];
   ErrorMessages.clear();
}

//------------------------------------------------------------------------------

void WadProcessor::addBitmaps(StringTableEntry source)
{
   AssertFatal(source, "WadProcessor::addBitmaps - invalid 'source' param");
   AssertISV(dStrlen(source) < (WadMaxFile-1), avar("WadProcess::addBitmaps - 'source' param too long - %s", source));
   
   // check if a response file and add each line of it
   if(source[0] == '@')
   {
      //....
      return;
   }

   // check if file or path...
   const char * dot = dStrrchr(source, '.');
   if(dot)
   {
      if(dStricmp(dot, ".png") && dStricmp(dot, ".jpg"))
      {
         addErrorMessage("  *** Invalid extension for file '%s'.", source);
         return;
      }
      mSrcBitmaps.push_back(StringTable->insert(source));
   }
   else
   {
      char path[WadMaxFile+1];
      dStrcpy(path, source);
      
      Vector<Platform::FileInfo> srcFiles;
      Platform::dumpPath(path, srcFiles);

      for(U32 i = 0; i < srcFiles.size(); i++)
      {
         dot = dStrrchr(srcFiles[i].pFileName, '.');
         if(!dot || (dStricmp(dot, ".png") && dStricmp(dot, ".jpg")))
            continue;

         AssertISV(srcFiles[i].pFileName && srcFiles[i].pFullPath, "WadProcess::addBitmaps - bad filename encountered");         
         AssertISV((dStrlen(srcFiles[i].pFileName) + dStrlen(srcFiles[i].pFullPath)) < (WadMaxFile-1),
            avar("WadProcess::addBitmaps - filename too long %s%s", srcFiles[i].pFullPath, srcFiles[i].pFileName));

         // add to list of bitmaps...
         mSrcBitmaps.push_back(StringTable->insert(avar("%s/%s", srcFiles[i].pFullPath, srcFiles[i].pFileName)));
      }
   }      
}

//------------------------------------------------------------------------------

void WadProcessor::processBitmaps()
{
   for(U32 i = 0; i < mSrcBitmaps.size(); i++)
   {
      dPrintf(" Adding lump: %s\n", mSrcBitmaps[i]);
      Lump * lump = new Lump;
      if(!lump->load(mSrcBitmaps[i]))
      {
         delete lump;
         continue;
      }

      // looks like worldcraft does not use the mip levels - no need to create them...
      // lump->process();
   
      lump->write(mFileStream, mLumps);
      delete lump;
   }
}

//------------------------------------------------------------------------------

void WadProcessor::processSinglePalette()
{
   Vector<Lump *> lumps;
   PalQuantizer * quantizer = new PalQuantizer;
   
   U32 numColors = 256;
   
   // add the colors
   for(U32 i = 0; i < mSrcBitmaps.size(); i++)
   {
      dPrintf(" Processing lump: %s\n", mSrcBitmaps[i]);
      Lump * lump = new Lump;
      if(!lump->open(mSrcBitmaps[i]))
      {
         delete lump;
         continue;
      }
      
      quantizer->addToTree(lump->mBitmap, numColors);
      lumps.push_back(lump);
   }
   
   // create the master palette
   quantizer->mNumColors = 0;
   quantizer->fillPalette(quantizer->mRoot, &quantizer->mNumColors);
   
   // do the palette
   for(U32 i = 0; i < lumps.size(); i++)
   {
      dPrintf(" Adding lump: %s\n", lumps[i]->mMipInfo.mName);
      lumps[i]->colorBits(quantizer);
      lumps[i]->write(mFileStream, mLumps);
      delete lumps[i];
   }
   
   delete quantizer;
}

//------------------------------------------------------------------------------

bool WadProcessor::open(StringTableEntry wadFile)
{
   mFileStream.close();
   AssertFatal(wadFile, "WadProcessor::open - invalid args");
   if(!mFileStream.open(wadFile, FileStream::Write))
      return(false);
   
   // move ahead of the header - 12bytes!
   for(U32 i = 0; i < sizeof(HeaderInfo); i++)
      mFileStream.write(U8(0));
      
   AssertFatal(mFileStream.getPosition() == sizeof(HeaderInfo), "WadProcessor::open - failed to offset by header");
   return(true);
}

void WadProcessor::close()
{
   U32 tablePos = mFileStream.getPosition();
   
   // write out all the lump info's
   for(U32 i = 0; i < mLumps.size(); i++)
   {
      mFileStream.write(mLumps[i].mFilePos);
      mFileStream.write(mLumps[i].mDiskSize);
      mFileStream.write(mLumps[i].mSize);
      mFileStream.write(mLumps[i].mType);
      mFileStream.write(mLumps[i].mCompression);
      mFileStream.write(mLumps[i].mPad1);
      mFileStream.write(mLumps[i].mPad2);
      for(U32 j = 0; j < 16; j++)
         mFileStream.write(U8(dToupper(mLumps[i].mName[j])));
   }
   
   // set back to beginning and write out the header...
   mFileStream.setPosition(0);
   mFileStream.write(mHeader.mID);
   mFileStream.write(mLumps.size());
   mFileStream.write(tablePos);

   mFileStream.close();

   // dump any errors
   if(ErrorMessages.size())
   {
      dPrintf("\n  ***************************************************\n");
      dPrintf("  *** ERRORS ENCOUNTERED WHILE CREATING WAD FILE! ***\n");
      dPrintf("  ***************************************************\n\n");

      for(U32 i = 0; i < ErrorMessages.size(); i++)
         dPrintf(ErrorMessages[i]);

      dPrintf("\n");
   }
}

//------------------------------------------------------------------------------
// Class WadProcessor::Lump
//------------------------------------------------------------------------------

WadProcessor::Lump::Lump() :
   mPaletteEntries(0),
   mPalQuantizer(0),
   mBitmap(0)
{
   for(U32 i = 0; i < 4; i++)
      mBits[i] = 0;
}

WadProcessor::Lump::~Lump()
{
   for(U32 i = 0; i < 4; i++)
      delete [] mBits[i];
   delete mPalQuantizer;
   delete mBitmap;
}

//------------------------------------------------------------------------------

bool WadProcessor::Lump::load(StringTableEntry file)
{
   AssertFatal(file, "WadProcessor::Lump::load - invalid argument");
   
   char baseName[16];
   if(!getBaseName(file, baseName, 16))
      return(false);

   char * path = new char[WadMaxFile];
   if(!getPath(file, path, WadMaxFile))
   {
      delete path;
      return(false);
   }
   
   GBitmap * bitmap = loadBitmap(file);
   if (!bitmap)
      return(false);
   
   // fill in the lump info...
   for(U32 i = 0; i < 16; i++)
      mMipInfo.mName[i] = 0;
      
   dStrcpy((char*)mMipInfo.mName, baseName);
   for(U32 j = 0; j < 16; j++)
      mMipInfo.mName[j] = U8(dTolower(mMipInfo.mName[j]));

   mMipInfo.mWidth = bitmap->getWidth();
   mMipInfo.mHeight = bitmap->getHeight();
   
   // check the width/height of the bitmap
   bool wp = false;
   bool hp = false;
   
   // 16x16 -> 512x512
   for(U32 i = 4; i < 10; i++)
   {
      if(mMipInfo.mWidth == 1<<i)
         wp = true;
      if(mMipInfo.mHeight == 1<<i)
         hp = true;
   }
   
   if(!wp || !hp)
   {
      addErrorMessage("  *** Improper ditmap dimension: %s [%d/%d].\n", 
         file, mMipInfo.mWidth, mMipInfo.mHeight);
      delete bitmap;
      return(false);
   }

   // copy the bits... 
   switch(bitmap->getFormat())
   {
      case GBitmap::Palettized:
      {
         mBits[0] = new U8[mMipInfo.mWidth * mMipInfo.mHeight];
      
         for(U32 y = 0; y < mMipInfo.mHeight; y++)
            for(U32 x = 0; x < mMipInfo.mWidth; x++)
               mBits[0][x+mMipInfo.mWidth * y] = *bitmap->getAddress(x,y);
         
         
         // grab the palette info...
         const GPalette * pal = bitmap->getPalette();
         mPaletteEntries = 256;
         for(U32 i = 0; i < 256; i++)
            mPalette[i] = pal->getColor(i);

         break;
      }

      case GBitmap::RGB:
      case GBitmap::RGBA:
      {
         // need to create a palette for this guy...
         mPalQuantizer = new PalQuantizer;
         mPalQuantizer->buildTree(bitmap);
         
         mBits[0] = new U8[mMipInfo.mWidth * mMipInfo.mHeight];
         
         for(U32 y = 0; y < mMipInfo.mHeight; y++)
            for(U32 x = 0; x < mMipInfo.mWidth; x++)
            {
               ColorI col;
               bitmap->getColor(x, y, col);
               mBits[0][x+mMipInfo.mWidth * y] = (U8)mPalQuantizer->getColorIndex(col);
            }
         
         mPaletteEntries = mPalQuantizer->mNumColors;
         AssertFatal(mPaletteEntries <= 256, "WadProcessor::Lump::Load - failed to quantize colors.");
         for(U32 i = 0; i < mPaletteEntries; i++)
            mPalette[i] = mPalQuantizer->mPalette[i];
         
         break;
      }
         
      default:
      {
         addErrorMessage("  *** Bitmap format not supported [%d] for: %s.\n", bitmap->getFormat(), file);
         delete bitmap;
         return(false);
         break;
      }
   }
   
   delete bitmap;
   
   return(true);
}

//------------------------------------------------------------------------------

bool WadProcessor::Lump::open(StringTableEntry file)
{
   AssertFatal(file, "WadProcessor::Lump::open - invalid argument");
   
   char baseName[16];
   if(!getBaseName(file, baseName, 16))
      return(false);

   char * path = new char[WadMaxFile];
   if(!getPath(file, path, WadMaxFile))
   {
      delete path;
      return(false);
   }
   
   GBitmap * bitmap = loadBitmap(file);
   if (!bitmap)
      return(false);
   
   // fill in the lump info...
   for(U32 i = 0; i < 16; i++)
      mMipInfo.mName[i] = 0;
      
   dStrcpy((char*)mMipInfo.mName, baseName);
   for(U32 j = 0; j < 16; j++)
      mMipInfo.mName[j] = U8(dTolower(mMipInfo.mName[j]));

   mMipInfo.mWidth = bitmap->getWidth();
   mMipInfo.mHeight = bitmap->getHeight();
   
   // check the width/height of the bitmap
   bool wp = false;
   bool hp = false;
   
   // 16x16 -> 512x512
   for(U32 i = 4; i < 10; i++)
   {
      if(mMipInfo.mWidth == 1<<i)
         wp = true;
      if(mMipInfo.mHeight == 1<<i)
         hp = true;
   }
   
   if(!wp || !hp)
   {
      addErrorMessage("  *** Improper ditmap dimension: %s [%d/%d].\n", 
         file, mMipInfo.mWidth, mMipInfo.mHeight);
      delete bitmap;
      return(false);
   }

   switch(bitmap->getFormat())
   {
      case GBitmap::RGB:
      case GBitmap::RGBA:
         break;
         
      case GBitmap::Palettized:
         addErrorMessage("  *** Palettized format unsupported for global palette in: %s.\n", file);
         delete bitmap;
         return(false);
         break;
         
      default:
         addErrorMessage("  *** Bitmap format not supported [%d] for: %s.\n", bitmap->getFormat(), file);
         delete bitmap;
         return(false);
         break;
   }

   mBitmap = bitmap;
   return(true);
}
   
//------------------------------------------------------------------------------

void WadProcessor::Lump::colorBits(PalQuantizer * quantizer)
{
   AssertFatal(quantizer, "WadProcessor::Lump::colorBits - invalid args");
   AssertFatal(mBitmap, "WadProcessor::Lump::colorBits - bitmap not loaded");

   mBits[0] = new U8[mMipInfo.mWidth * mMipInfo.mHeight];
   
   for(U32 y = 0; y < mMipInfo.mHeight; y++)
      for(U32 x = 0; x < mMipInfo.mWidth; x++)
      {
         ColorI col;
         mBitmap->getColor(x, y, col);
         mBits[0][x+mMipInfo.mWidth * y] = (U8)quantizer->getColorIndex(col);
      }   

   mPaletteEntries = quantizer->mNumColors;
   AssertFatal(mPaletteEntries <= 256, "WadProcessor::Lump::colorBits - failed to quantize colors.");
   for(U32 i = 0; i < mPaletteEntries; i++)
      mPalette[i] = quantizer->mPalette[i];
}

//------------------------------------------------------------------------------
// * needs to generate the other mip levels for this bitmap(4total)
// * just average pixels for mip's
// * WorldCraft does not appear to use the mip's.. just ignore those...
void WadProcessor::Lump::process()
{
   AssertFatal(mBits[0], "WadProcessor::Lump::process: lump not loaded");

   for(U32 mipLevel = 1; mipLevel < 4; mipLevel++)
   {
      U32 size = (mMipInfo.mWidth>>mipLevel) * (mMipInfo.mHeight>>mipLevel);
      mBits[mipLevel] = new U8[size];
      
      // walk through and average the pixels for this mip
      U32 mipStep = 1<<mipLevel;
      for(U32 y = 0; y < mMipInfo.mHeight; y+=mipStep)
         for(U32 x = 0; x < mMipInfo.mWidth; x+=mipStep)
         {
            ColorF colSum(0,0,0);
            U8 palIndex = 0;
            
            // average them..
            for(U32 yy=0; yy < mipStep; yy++)
               for(U32 xx=0; xx < mipStep; xx++)
               {
                  AssertFatal((x+xx+mMipInfo.mWidth*(y+yy)) < (mMipInfo.mWidth * mMipInfo.mHeight), "WadProcess::Lump::process - bad index");
                  U8 index = mBits[0][x + xx + mMipInfo.mWidth * (y + yy)];
                  
                  // add the color from this guy...
                  colSum.red += F64(mPalette[index].red) / 255.f;
                  colSum.green += F64(mPalette[index].green) / 255.f;
                  colSum.blue += F64(mPalette[index].blue) / 255.f;
               }

            colSum /= F32(mipStep * mipStep);
            colSum.clamp();
            
            ColorI col;
            col.red = U8(colSum.red * 255.f);
            col.green = U8(colSum.green * 255.f);
            col.blue = U8(colSum.blue * 255.f);
            
            F64 minDist = 1e10;
            S32 minIndex = -1;
            Point3D src(col.red, col.green, col.blue);
            
            // just walk through and grab the closest one... lame and slow :) Bonus!
            for(U32 i =0; i < 256; i++)
            {
               Point3D dest(mPalette[i].red, mPalette[i].green, mPalette[i].blue);
               dest -= src;
               F64 dist = dest.len();
               if(dist < minDist)
               {
                  minDist = dist;
                  minIndex = i;
               }
            }
            
            AssertFatal(minIndex != -1, "WadProcessor::Lump::process: bad palette!");
            
            // set the index...
            AssertFatal((x>>mipLevel)+(y>>mipLevel)*(mMipInfo.mWidth>>mipLevel) < size, 
               "WadProcessor::Lump::Processs - bad mip index");
            mBits[mipLevel][(x>>mipLevel)+(y>>mipLevel)*(mMipInfo.mWidth>>mipLevel)] = minIndex;
         }
   }   
}

//------------------------------------------------------------------------------
#if defined(TORQUE_DEBUG)
void WadProcessor::dumpHeader(HeaderInfo & header)
{
   dPrintf("------ HeaderInfo\n");
   dPrintf("tag:           %d\n", header.mID);
   dPrintf("numLumps:      %d\n", header.mNumLumps);
   dPrintf("tableOffset:   %d\n", header.mInfoTableOffset);
}

void WadProcessor::dumpLumpInfo(LumpInfo & info)
{
   dPrintf("\t------ LumpInfo\n");
   dPrintf("\tfilePos:     %d\n", info.mFilePos);
   dPrintf("\tdiskSize:    %d\n", info.mDiskSize);
   dPrintf("\tsize:        %d\n", info.mSize);
   dPrintf("\ttype:        %d\n", info.mType);
   dPrintf("\tcompression: %d\n", info.mCompression);
   dPrintf("\tpad1:        %d\n", info.mPad1);
   dPrintf("\tpad2:        %d\n", info.mPad2);
   dPrintf("\tname:        %s\n", info.mName);
}

void WadProcessor::dumpMipInfo(MipTexInfo & info)
{
   dPrintf("\t------ MipTexInfo\n");
   dPrintf("\tname:        %s\n", info.mName);
   dPrintf("\twidth:       %d\n", info.mWidth);
   dPrintf("\theight:      %d\n", info.mHeight);
   dPrintf("\tmOffsets:    %d %d %d %d\n", info.mOffsets[0], info.mOffsets[1], info.mOffsets[2], info.mOffsets[3]);
}

void WadProcessor::dumpWad(StringTableEntry name)
{
   FileStream file;
   file.open(name, FileStream::ReadWrite);
   
   // do the header
   HeaderInfo header;
   file.read(&header.mID);
   file.read(&header.mNumLumps);
   file.read(&header.mInfoTableOffset);
   dumpHeader(header);
   
   // do the lump's
   for(U32 i = 0; i < header.mNumLumps; i++)
   {
      // lump info
      file.setPosition(header.mInfoTableOffset + i * sizeof(LumpInfo));
      LumpInfo info;
      file.read(&info.mFilePos);
      file.read(&info.mDiskSize);
      file.read(&info.mDiskSize);
      file.read(&info.mSize);
      file.read(&info.mType);
      file.read(&info.mCompression);
      file.read(&info.mPad1);
      file.read(&info.mPad2);
      for(U32 j = 0; j < 16; j++)
         file.read(&info.mName[j]);
      dumpLumpInfo(info);
      
      // miptex info
      file.setPosition(info.mFilePos);
      MipTexInfo mipInfo;
      for(U32 j = 0; j < 16; j++)
         file.read(&mipInfo.mName[j]);
      file.read(&mipInfo.mWidth);
      file.read(&mipInfo.mHeight);
      for(U32 j = 0; j < 4; j++)
       file.read(&mipInfo.mOffsets[j]);
      dumpMipInfo(mipInfo);

      // palette
      U32 pos = info.mFilePos + mipInfo.mOffsets[3];
      pos += (mipInfo.mWidth >> 3) * (mipInfo.mHeight >> 3);
      file.setPosition(pos);
      U16 colors;
      file.read(&colors);
      dPrintf("\tcolors:  %d\n", colors);
   }
}
#endif

//------------------------------------------------------------------------------

void WadProcessor::Lump::write(FileStream & stream, Vector<LumpInfo> & lumps)
{
   U32 lumpPos = stream.getPosition();

   U32 size = sizeof(MipTexInfo);
   
   // fill in the header info
   for(U32 i = 0; i < 4; i++)
   {
      mMipInfo.mOffsets[i] = size;
      size += ((mMipInfo.mWidth>>i) * (mMipInfo.mHeight>>i));
   }
   
   // write the mip header
   for(U32 i = 0; i < 16; i++)
      stream.write(mMipInfo.mName[i]);
   stream.write(mMipInfo.mWidth);
   stream.write(mMipInfo.mHeight);
   for(U32 i = 0; i < 4; i++)
      stream.write(mMipInfo.mOffsets[i]);
      
   // just write from the first mip level - worldcraft does not use (but requires!)
   // the mip levels...
   
   // write the data..
   for(U32 i = 0; i < 4; i++)
   {
      U32 len = ((mMipInfo.mWidth>>i) * (mMipInfo.mHeight>>i));
      for(U32 j = 0; j < len; j++)
         stream.write(mBits[0][j]);
//         stream.write(mBits[i][j]);
   }
 
   // write the palette out - dump all possible 256 entries
//   stream.write(U16(mPaletteEntries));
   stream.write(U16(256));
   for(U32 i = 0; i < mPaletteEntries; i++)
   {
      stream.write(U8(mPalette[i].red));
      stream.write(U8(mPalette[i].green));
      stream.write(U8(mPalette[i].blue));
   }
   
   for(U32 i = mPaletteEntries; i < 256; i++)
   {
      stream.write(U8(0x00));
      stream.write(U8(0x00));
      stream.write(U8(0x00));
   }

   // fill in the lump info...
   LumpInfo info;
   for(U32 i = 0; i < 16; i++)
      info.mName[i] = 0;
      
   // --- adjust the size of the lump ---
   // * MipTexInfo size (already in size)
   // * number of mip bytes (already in size)
   // * dword align padding
   // * (short) num of palette entries
   // * palette (3*numEntries)

   // padding   
   while(stream.getPosition()&3)
   {
      stream.write(U8(0));
      size++;
   }
   
   // num palette entries
   size += sizeof(U16);
   
   // palette entries (always 256... stupid)
   size += 768;
   
   info.mFilePos = lumpPos;
   info.mDiskSize = size;
   info.mSize = size;
   info.mType = Lumpy + MipTex;
   info.mCompression = NoComp;
   info.mPad1 = info.mPad2 = 0;
   dStrcpy((char*)info.mName, (char*)mMipInfo.mName);

   // add the info to the table of lumps
   lumps.push_back(info);
}
