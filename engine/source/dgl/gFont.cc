//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "core/stream.h"
#include "dgl/gFont.h"
#include "dgl/gBitmap.h"
#include "core/fileStream.h"
#include "dgl/gTexManager.h"

S32 GFont::smSheetIdCount = 0;

ResourceInstance* constructFont(Stream& stream)
{
   GFont *ret = new GFont;
   if(!ret->read(stream))
   {
      delete ret;
      return NULL;
   }

   return ret;
}
const U32 GFont::csm_fileVersion = 1;

Resource<GFont> GFont::create(const char *faceName, U32 size, const char *cacheDirectory)
{
   char buf[256];
   dSprintf(buf, sizeof(buf), "%s/%s_%d.gft", cacheDirectory, faceName, size);
   
   Resource<GFont> ret = ResourceManager->load(buf);
   if(bool(ret))
      return ret;
   
   GFont *resFont = createFont(faceName, size);
   if (resFont == NULL) 
   {
      AssertISV(dStricmp(faceName, "Arial") != 0, "Error, The Arial Font must always be available!");

      // Need to handle this case better.  For now, let's just return a font that we're
      //  positive exists, in the correct size...
      return create("Arial", size, cacheDirectory);
   }

   FileStream stream;
   if(ResourceManager->openFileForWrite(stream, buf)) 
   {
      resFont->write(stream);
      stream.close();
   }
   ResourceManager->add(buf, resFont, false);
   return ResourceManager->load(buf);
}

GFont::GFont()
{
   VECTOR_SET_ASSOCIATION(mCharInfoList);

   for (U32 i = 0; i < 256; i++)
      mRemapTable[i] = -1;

   mTextureSheets = NULL;
}

GFont::~GFont()
{
   delete [] mTextureSheets;
   mTextureSheets = NULL;
}

void GFont::insertBitmap(U16 index, U8 *src, U32 stride, U32 width, U32 height, S32 xOrigin, S32 yOrigin, S32 xIncrement)
{
   CharInfo c;
   c.bitmapIndex = -1;
   c.xOffset = 0;
   c.yOffset = 0;
   c.width = width;
   c.height = height;
   
   c.xOrigin = xOrigin;
   c.yOrigin = yOrigin;
   c.xIncrement = xIncrement;
   
   c.bitmapData = new U8[c.width * c.height];

   for(U32 y = 0; S32(y) < c.height; y++)
   {
      U32 x;
      for(x = 0; x < width; x++)
         c.bitmapData[y * c.width + x] = src[y * stride + x];
   }
   mRemapTable[index] = mCharInfoList.size();
   mCharInfoList.push_back(c);
}

static S32 QSORT_CALLBACK CharInfoCompare(const void *a, const void *b)
{
   S32 ha = (*((GFont::CharInfo **) a))->height;
   S32 hb = (*((GFont::CharInfo **) b))->height;
   
   return hb - ha;
}

void GFont::pack(U32 inFontHeight, U32 inBaseLine)
{
   mFontHeight = inFontHeight;
   mBaseLine = inBaseLine;
   
   // pack all the bitmap data into sheets.
   Vector<CharInfo *> vec;
   
   U32 size = mCharInfoList.size();
   U32 i;
   
   for(i = 0; i < size; i++)
   {
      CharInfo *ch = &mCharInfoList[i];
      vec.push_back(ch);
   }

   dQsort(vec.address(), size, sizeof(CharInfo *), CharInfoCompare);
   // sorted by height

   Vector<Point2I> sheetSizes;
   Point2I curSheetSize(256, 256);
   
   S32 curY = 0;
   S32 curX = 0;
   S32 curLnHeight = 0;
   for(i = 0; i < size; i++)
   {
      CharInfo *ci = vec[i];
      
      if(curX + ci->width > curSheetSize.x)
      {
         curY += curLnHeight;
         curX = 0;
         curLnHeight = 0;
      }
      if(curY + ci->height > curSheetSize.y)
      {
         sheetSizes.push_back(curSheetSize);
         curX = 0;
         curY = 0;
         curLnHeight = 0;
      }
      if(ci->height > curLnHeight)
         curLnHeight = ci->height;
      ci->bitmapIndex = sheetSizes.size();
      ci->xOffset = curX;
      ci->yOffset = curY;
      curX += ci->width;
   }
   curY += curLnHeight;
   
   if(curY < 64)
      curSheetSize.y = 64;
   else if(curY < 128)
      curSheetSize.y = 128;
      
   sheetSizes.push_back(curSheetSize);

   Vector<GBitmap *> bitmapArray;

   mNumSheets = sheetSizes.size();
   mTextureSheets = new TextureHandle[mNumSheets];
   
   for(i = 0; i < mNumSheets; i++)
      bitmapArray.push_back(new GBitmap(sheetSizes[i].x, sheetSizes[i].y, false, GBitmap::Alpha));

   for(i = 0; i < size; i++)
   {
      CharInfo *ci = vec[i];
      GBitmap *bmp = bitmapArray[ci->bitmapIndex];
      S32 x, y;
      for(y = 0; y < ci->height; y++)
         for(x = 0; x < ci->width; x++)
            *bmp->getAddress(x + ci->xOffset, y + ci->yOffset) =
                  ci->bitmapData[y * ci->width + x];
      delete[] ci->bitmapData;
   }
   for(i = 0; i < mNumSheets; i++)
	{
      assignSheet(i, bitmapArray[i]);
		mTextureSheets[i].setFilterNearest();
	}
}

TextureHandle GFont::getTextureHandle(S32 index)
{
   return mTextureSheets[index];
}
 
void GFont::assignSheet(S32 sheetNum, GBitmap *bmp)
{
   char buf[30];
   dSprintf(buf, sizeof(buf), "font_%d", smSheetIdCount++);
   mTextureSheets[sheetNum] = TextureHandle(buf, bmp);
}

U32 GFont::getStrWidth(const char* in_pString) const
{
   AssertFatal(in_pString != NULL, "GFont::getStrWidth: String is NULL, height is undefined");
   // If we ain't running debug...
   if (in_pString == NULL)
      return 0;

   return getStrNWidth(in_pString, dStrlen(in_pString));
}

U32 GFont::getStrWidthPrecise(const char* in_pString) const
{
   AssertFatal(in_pString != NULL, "GFont::getStrWidth: String is NULL, height is undefined");
   // If we ain't running debug...
   if (in_pString == NULL)
      return 0;

   return getStrNWidthPrecise(in_pString, dStrlen(in_pString));
}

//-----------------------------------------------------------------------------
U32 GFont::getStrNWidth(const char *str, U32 n) const
{
   AssertFatal(str != NULL, "GFont::getStrNWidth: String is NULL");

   if (str == NULL)   
      return(0);
      
   U32 totWidth = 0;
   const char *curChar;
   const char *endStr;
   for (curChar = str, endStr = str + n; curChar < endStr; curChar++)
   {
      if(isValidChar(*curChar))
      {
         const CharInfo& rChar = getCharInfo(*curChar);
         totWidth += rChar.xIncrement;
      }
      else if (*curChar == '\t')
      {
         const CharInfo& rChar = getCharInfo(' ');
         totWidth += rChar.xIncrement * TabWidthInSpaces;
      }
   }

   return(totWidth);
}

U32 GFont::getStrNWidthPrecise(const char *str, U32 n) const
{
   AssertFatal(str != NULL, "GFont::getStrNWidth: String is NULL");

   if (str == NULL)   
      return(0);
      
   U32 totWidth = 0;
   const char *curChar;
   const char *endStr;
   for (curChar = str, endStr = str + n; curChar < endStr; curChar++)
   {
      if(isValidChar(*curChar))
      {
         const CharInfo& rChar = getCharInfo(*curChar);
         totWidth += rChar.xIncrement;
      }
      else if (*curChar == '\t')
      {
         const CharInfo& rChar = getCharInfo(' ');
         totWidth += rChar.xIncrement * TabWidthInSpaces;
      }
   }

   if (n != 0) {
      // Need to check the last char to see if it has some slop...
      char endChar = str[n-1];
      if (isValidChar(endChar)) {
         const CharInfo& rChar = getCharInfo(endChar);
         if (rChar.width > rChar.xIncrement)
            totWidth += (rChar.width - rChar.xIncrement);
      }
   }

   return(totWidth);
}

U32 GFont::getBreakPos(const char *string, U32 slen, U32 width, bool breakOnWhitespace)
{
   U32 ret = 0;
   U32 lastws = 0;
   while(ret < slen)
   {
      char c = string[ret];
      if(c == '\t')
         c = ' ';
      if(!isValidChar(c))
      {
         ret++;
         continue;
      }
      if(c == ' ')
         lastws = ret+1;
      const CharInfo& rChar = getCharInfo(c);
      if(rChar.width > width || rChar.xIncrement > width)
      {
         if(lastws && breakOnWhitespace)
            return lastws;
         return ret;
      }
      width -= rChar.xIncrement;
      
      ret++;
   }
   return ret;
}

void GFont::wrapString(const char *txt, U32 lineWidth, Vector<U32> &startLineOffset, Vector<U32> &lineLen)
{
   startLineOffset.clear();
   lineLen.clear();

   if (!txt || !txt[0] || lineWidth < getCharWidth('W')) //make sure the line width is greater then a single character
      return;

   U32 len = dStrlen(txt);

   U32 startLine; 

   for (U32 i = 0; i < len;)
   {
      startLine = i;
      startLineOffset.push_back(startLine);

      // loop until the string is too large
      bool needsNewLine = false;
      U32 lineStrWidth = 0;
      for (; i < len; i++)
      {
         if(isValidChar(txt[i]))
         {
            lineStrWidth += getCharInfo(txt[i]).xIncrement;
            if ( txt[i] == '\n' || lineStrWidth > lineWidth )
            {
               needsNewLine = true;
               break;      
            }
         }
      }

      if (!needsNewLine)
      {
         // we are done!
         lineLen.push_back(i - startLine);
         return;
      }

      // now determine where to put the newline
      // else we need to backtrack until we find a either space character 
      // or \\ character to break up the line. 
      S32 j;
      for (j = i - 1; j >= startLine; j--)
      {
         if (dIsspace(txt[j]))
            break;
      }

      if (j < startLine)
      {
         // the line consists of a single word!              
         // So, just break up the word
         j = i - 1;
      }
      lineLen.push_back(j - startLine);
      i = j;

      // now we need to increment through any space characters at the
      // beginning of the next line
      for (i++; i < len; i++)
      {
         if (!dIsspace(txt[i]) || txt[i] == '\n')
            break;
      }
   }
}

//------------------------------------------------------------------------------
//-------------------------------------- Persist functionality
//
static const U32 csm_fileVersion = 1;

bool GFont::read(Stream& io_rStream)
{
   // Handle versioning
   U32 version;
   io_rStream.read(&version);
   if(version != csm_fileVersion)
      return false;
      
   // Read Font Information
   io_rStream.read(&mFontHeight);
   io_rStream.read(&mBaseLine);

   U32 size = 0;
   io_rStream.read(&size);
   mCharInfoList.setSize(size);
   U32 i;
   for(i = 0; i < size; i++)
   {
      CharInfo *ci = &mCharInfoList[i];
      io_rStream.read(&ci->bitmapIndex);
      io_rStream.read(&ci->xOffset);
      io_rStream.read(&ci->yOffset);
      io_rStream.read(&ci->width);
      io_rStream.read(&ci->height);
      io_rStream.read(&ci->xOrigin);
      io_rStream.read(&ci->yOrigin);
      io_rStream.read(&ci->xIncrement);
   }
   io_rStream.read(&mNumSheets);

   mTextureSheets = new TextureHandle[mNumSheets];
   for(i = 0; i < mNumSheets; i++)
   {
      GBitmap *bmp = new GBitmap;
      if(!bmp->readPNG(io_rStream))
      {
         delete bmp;
         return false;
      }
      assignSheet(i, bmp);
		mTextureSheets[i].setFilterNearest();
   }

   // Read character remap table
   for(i = 0; i < 256; i++)
      io_rStream.read(&mRemapTable[i]);

   return (io_rStream.getStatus() == Stream::Ok);
}

bool
GFont::write(Stream& stream) const
{
   // Handle versioning
   stream.write(csm_fileVersion);

   // Write Font Information
   stream.write(mFontHeight);
   stream.write(mBaseLine);

   stream.write(U32(mCharInfoList.size()));
   U32 i;
   for(i = 0; i < mCharInfoList.size(); i++)
   {
      const CharInfo *ci = &mCharInfoList[i];
      stream.write(ci->bitmapIndex);
      stream.write(ci->xOffset);
      stream.write(ci->yOffset);
      stream.write(ci->width);
      stream.write(ci->height);
      stream.write(ci->xOrigin);
      stream.write(ci->yOrigin);
      stream.write(ci->xIncrement);
   }
   stream.write(mNumSheets);
   for(i = 0; i < mNumSheets; i++)
      mTextureSheets[i].getBitmap()->writePNG(stream);

   for(i = 0; i < 256; i++)
      stream.write(mRemapTable[i]);

   return (stream.getStatus() == Stream::Ok);
}
