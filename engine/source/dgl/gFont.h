//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GFONT_H_
#define _GFONT_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _GBITMAP_H_
#include "dgl/gBitmap.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

extern ResourceInstance* constructFont(Stream& stream);

class TextureHandle;

class GFont : public ResourceInstance
{
   static const U32 csm_fileVersion;
   static S32 smSheetIdCount;
   
   // Enumerations and structs available to everyone...
public:
   // A justification consists of a horizontal type | a vertical type.
   //  Note that a justification of 0 evalutes to left/top, the default.
   //  The robustness of the rendering functions should be considered
   //  suspect for a while, especially the justified versions...
   //
   struct CharInfo {
      S16 bitmapIndex;    // Note: -1 indicates character is NOT to be
                          //  rendered, i.e., \n, \r, etc.
      U8  xOffset;        // x offset into bitmap sheet
      U8  yOffset;        // y offset into bitmap sheet
      U8  width;          // width of character (pixels)
      U8  height;         // height of character (pixels)
      S8  xOrigin;
      S8  yOrigin;
      S8  xIncrement;
      U8  *bitmapData;    // temp storage for bitmap data
   };
   enum Constants {
      TabWidthInSpaces = 3
   };


   // Enumerations and structures available to derived classes
private:
   U32 mNumSheets;
   TextureHandle *mTextureSheets;
   
   U32 mFontHeight;   // ascent + descent of the font
   U32 mBaseLine;     // ascent of the font (pixels above the baseline of any character in the font)
   
   Vector<CharInfo>  mCharInfoList;      // - List of character info structures, must
                                          //    be accessed through the getCharInfo(U32)
                                          //    function to account for remapping...
   S16             mRemapTable[256];    // - Index remapping

   S16 getActualIndex(const U8 in_charIndex) const;
   void assignSheet(S32 sheetNum, GBitmap *bmp);

public:
   GFont();
   virtual ~GFont();

   // Queries about this font
public:
   TextureHandle getTextureHandle(S32 index);
   U32   getCharHeight(const U8 in_charIndex) const;
   U32   getCharWidth(const U8 in_charIndex)  const;
   U32   getCharXIncrement(const U8 in_charIndex)  const;

   bool            isValidChar(const U8 in_charIndex) const;
   const CharInfo& getCharInfo(const U8 in_charIndex) const;


   // Rendering assistance functions...
public:
   U32 getBreakPos(const char *string, U32 strlen, U32 width, bool breakOnWhitespace);
   
   U32 getStrWidth(const char*)  const;   // Note: ignores c/r
   U32 getStrNWidth(const char*, U32 n) const;
   U32 getStrWidthPrecise(const char*)  const;   // Note: ignores c/r
   U32 getStrNWidthPrecise(const char*, U32 n) const;
   void wrapString(const char *string, U32 width, Vector<U32> &startLineOffset, Vector<U32> &lineLen);

   bool read(Stream& io_rStream);
   bool write(Stream& io_rStream) const;

   U32 getHeight()   { return mFontHeight; }
   U32 getBaseline() { return mBaseLine; }
   U32 getAscent()   { return mBaseLine; }
   U32 getDescent()  { return mFontHeight - mBaseLine; }

   void insertBitmap(U16 index, U8 *src, U32 stride, U32 width, U32 height, S32 xOrigin, S32 yOrigin, S32 xIncrement);
   void pack(U32 fontHeight, U32 baseLine);
   
   static Resource<GFont> create(const char *face, U32 size, const char *cacheDirectory);
};

inline bool GFont::isValidChar(const U8 in_charIndex) const
{
   return mRemapTable[in_charIndex] != -1;
}

inline S16 GFont::getActualIndex(const U8 in_charIndex) const
{
   AssertFatal(isValidChar(in_charIndex) == true,
               avar("GFont::getActualIndex: invalid character: 0x%x",
                    in_charIndex));

   return mRemapTable[in_charIndex];
}

inline const GFont::CharInfo& GFont::getCharInfo(const U8 in_charIndex) const
{
   S16 remap = getActualIndex(in_charIndex);
   AssertFatal(remap != -1, "No remap info for this character");

   return mCharInfoList[remap];
}

inline U32 GFont::getCharXIncrement(const U8 in_charIndex) const
{
   const CharInfo& rChar = getCharInfo(in_charIndex);
   return rChar.xIncrement;
}

inline U32 GFont::getCharWidth(const U8 in_charIndex) const
{
   const CharInfo& rChar = getCharInfo(in_charIndex);
   return rChar.width;
}

inline U32 GFont::getCharHeight(const U8 in_charIndex) const
{
   const CharInfo& rChar = getCharInfo(in_charIndex);
   return rChar.height;
}

#endif //_GFONT_H_
