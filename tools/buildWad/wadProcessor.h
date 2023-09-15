//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INC_WADPROCESSOR
#define _INC_WADPROCESSOR

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif
#ifndef _GBITMAP_H_
#include "dgl/gBitmap.h"
#endif
#ifndef _FILESTREAM_H_
#include "core/fileStream.h"
#endif

#define WAD_MAXFILE     1024

//------------------------------------------------------------------------------
class PalQuantizer;
class WadProcessor
{
   public:
   
      enum {
         WadMaxFile = 1024
      };
      
      enum LumpType {
         NoType = 0,
         Label =  1,
         Lumpy =  64,          //64 + grab code..
      };
      
      enum GrabCode {
         Palette =   0,
         ColorMap =  1,
         Pic =       2,
         MipTex =    3,       //type that WorldCraft uses and we are interested in
         Raw =       4,
         ColorMap2 = 5,   
      };
      
      enum CompType {
         NoComp = 0,
         LZSS =   1,
      };
      
      struct MipTexInfo
      {
         U8  mName[16];       // must be null terminated
         U32 mWidth;
         U32 mHeight;
         U32 mOffsets[4];     // 4 mip level offsets...
      };
      
      struct HeaderInfo
      {
         U32 mID;                // 'WAD3'   
         U32 mNumLumps;
         U32 mInfoTableOffset;   // offset to table of lumpInfo's
      };
      
      struct LumpInfo
      {
         U32 mFilePos;        // pos in file for this lump
         U32 mDiskSize;       // size of lump   
         U32 mSize;           // same as mDiskSize
         U8  mType;           // lump type
         U8  mCompression;    // compression type
         U8  mPad1, mPad2;
         U8  mName[16];       // must be null terminated
      };

      // only concerned with miptex lumps...
      class Lump
      {
         public:
            U8 *                 mBits[4];
            U32                  mPaletteEntries;
            ColorI               mPalette[256];
         
            MipTexInfo           mMipInfo;

            GBitmap *            mBitmap;
            
            //
            PalQuantizer *       mPalQuantizer;
            Lump();
            ~Lump();
            
            bool load(StringTableEntry file);
            void process();
            void write(FileStream & stream, Vector<LumpInfo> & lumps);

            bool open(StringTableEntry file);
            void addColors(PalQuantizer * quantizer);
            void colorBits(PalQuantizer * quantizer);
      };
      
   private:
   
      //   
      Vector<StringTableEntry>   mSrcBitmaps;
      Vector<LumpInfo>           mLumps;
      HeaderInfo                 mHeader;
      FileStream                 mFileStream;
      
   public:
   
      void addBitmaps(StringTableEntry source);
      void processBitmaps();
      void processSinglePalette();
      
      // these functions take care of writing the header/table of contents...   
      bool open(StringTableEntry wadFile);
      void close();

   #if defined(TORQUE_DEBUG)
      void dumpHeader(HeaderInfo & header);
      void dumpLumpInfo(LumpInfo & info);
      void dumpMipInfo(MipTexInfo & info);
      void dumpWad(StringTableEntry name);
   #endif
      
      WadProcessor();
      ~WadProcessor();
};

#endif
