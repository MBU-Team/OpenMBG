//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "dgl/gFont.h"
#include "dgl/gBitmap.h"
#include "math/mRect.h"
#include "console/console.h"

static HDC fontHDC = NULL;
static HBITMAP fontBMP = NULL;

void createFontInit(void);
void createFontShutdown(void);
void CopyCharToBitmap(GBitmap *pDstBMP, HDC hSrcHDC, const RectI &r);

void createFontInit()
{
   fontHDC = CreateCompatibleDC(NULL);
   fontBMP = CreateCompatibleBitmap(fontHDC, 256, 256);
}

void createFontShutdown()
{
   DeleteObject(fontBMP);
   DeleteObject(fontHDC);
}

void CopyCharToBitmap(GBitmap *pDstBMP, HDC hSrcHDC, const RectI &r)
{
   for (S32 i = r.point.y; i < r.point.y + r.extent.y; i++)
   {
      for (S32 j = r.point.x; j < r.point.x + r.extent.x; j++)
      {
         COLORREF color = GetPixel(hSrcHDC, j, i);
         if (color)
            *pDstBMP->getAddress(j, i) = 255;
         else
            *pDstBMP->getAddress(j, i) = 0;
      }
   }
}

GFont *createFont(const char *name, dsize_t size)
{
   if(!name)
      return NULL;
   if(size < 1)
      return NULL;
   

   HFONT hNewFont = CreateFontA(size,0,0,0,0,0,0,0,0,0,0,0,0,name);
   if(!hNewFont)
      return NULL;
   
   GFont *retFont = new GFont;
   static U8 scratchPad[65536];
   
   TEXTMETRIC textMetric;
	COLORREF backgroundColorRef = RGB(  0,   0,   0);
	COLORREF foregroundColorRef = RGB(255, 255, 255);
	
	SelectObject(fontHDC, fontBMP);
	SelectObject(fontHDC, hNewFont);
	SetBkColor(fontHDC, backgroundColorRef);
	SetTextColor(fontHDC, foregroundColorRef);
	GetTextMetrics(fontHDC, &textMetric);
   MAT2 matrix;
   GLYPHMETRICS metrics;
   RectI clip;
   
   FIXED zero;
   zero.fract = 0;
   zero.value = 0;
   FIXED one;
   one.fract = 0;
   one.value = 1;

   matrix.eM11 = one;
   matrix.eM12 = zero;
   matrix.eM21 = zero;
   matrix.eM22 = one;
   S32 glyphCount = 0;
   
   for(S32 i = 32; i < 256; i++)
   {
      if(GetGlyphOutline(
         fontHDC,	// handle of device context 
         i,	// character to query 
         GGO_GRAY8_BITMAP,	// format of data to return 
         &metrics,	// address of structure for metrics 
         sizeof(scratchPad),	// size of buffer for data 
         scratchPad,	// address of buffer for data 
         &matrix 	// address of transformation matrix structure  
         ) != GDI_ERROR)
      {
         glyphCount++;
         U32 rowStride = (metrics.gmBlackBoxX + 3) & ~3; // DWORD aligned
     	   U32 size = rowStride * metrics.gmBlackBoxY;
         for(U32 j = 0; j < size; j++)
         {
            U32 pad = U32(scratchPad[j]) << 2;
            if(pad > 255)
               pad = 255;
            scratchPad[j] = pad;
         }
         S32 inc = metrics.gmCellIncX;
         if(inc < 0)
            inc = -inc;
         retFont->insertBitmap(i, scratchPad, rowStride, metrics.gmBlackBoxX, metrics.gmBlackBoxY, metrics.gmptGlyphOrigin.x, metrics.gmptGlyphOrigin.y, metrics.gmCellIncX);
      }
      else
      {
         char b = i;
         SIZE size;
         GetTextExtentPoint32A(fontHDC, &b, 1, &size);
         if(size.cx)
            retFont->insertBitmap(i, scratchPad, 0, 0, 0, 0, 0, size.cx);
      }
   }
   retFont->pack(textMetric.tmHeight, textMetric.tmAscent);
   //clean up local vars      
   DeleteObject(hNewFont);

   if (!glyphCount)
      Con::errorf(ConsoleLogEntry::General,"Error creating font: %s %d",name, size);
   
   return retFont;
}
