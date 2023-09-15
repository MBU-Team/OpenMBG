//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "PlatformMacCarb/platformMacCarb.h"
#include "dgl/gFont.h"
#include "dgl/gBitmap.h"
#include "Math/mRect.h"
#include "console/console.h"

#include <QDOffscreen.h>
#include <Fonts.h>

static GWorldPtr fontgw = NULL;
static Rect fontrect = {0,0,256,256};
static short fontdepth = 32;
static U8 rawmap[256*256];


void createFontInit(void);
void createFontShutdown(void);
S32 CopyCharToRaw(U8 *raw, PixMapHandle pm, const Rect &r);

// !!!!! TBD this should be returning error, or raising exception...
void createFontInit()
{
   OSErr err = NewGWorld(&fontgw, fontdepth, &fontrect, NULL, NULL, keepLocal);
   AssertWarn(err==noErr, "Font system failed to initialize GWorld.");
}
 
void createFontShutdown()
{
   DisposeGWorld(fontgw);
   fontgw = NULL;
}

U8 ColorAverage8(RGBColor &rgb)
{
   return ((rgb.red>>8) + (rgb.green>>8) + (rgb.blue>>8)) / 3;
}

S32 CopyCharToRaw(U8 *raw, PixMapHandle pmh, const Rect &r)
{
   // walk the pixmap, copying into raw buffer.
   // we want bg black, fg white, which is opposite 'sign' from the pixmap (bg white, with black text)
   
//   int off = GetPixRowBytes(pmh);
//   U32 *c = (U32*)GetPixBaseAddr(pmh);
//   U32 *p;

   RGBColor rgb;

   S32 i, j;
   U8 val, ca;
   S32 lastRow = -1;
   
   for (i = r.left; i <= r.right; i++)
   {
      for (j = r.top; j <= r.bottom; j++)
      {
//         p = (U32*)(((U8*)c) + (j*off)); // since rowbytes is bytes not pixels, need to convert to byte * before doing the math...
         val = 0;
//         if (((*p)&0x00FFFFFF)==0) // then black pixel in current port, so want white in new buffer.
         GetCPixel(i, j, &rgb);
         if ((ca = ColorAverage8(rgb))<=250) // get's us some grays with a small slop factor.
         {
            val = 255 - ca; // flip sign, basically.
            lastRow = j; // track the last row in the rect that we actually saw an active pixel, finding descenders basically...
         }
         raw[i + (j<<8)] = val;
//         p++;
      }
   }
   
   return(lastRow);
}

GFont *createFont(const char *name, dsize_t size)
{
   if(!name)
      return NULL;
   if(size < 1)
      return NULL;

   bool wantsBold = false;
   short fontid;
   GetFNum(str2p(name), &fontid);
   if (fontid == 0)
   {
      // hmmm... see if it has "Bold" on the end.  if so, remove it and try again.
      int len = dStrlen(name);
      if (len>4 && 0==dStricmp(name+len-4, "bold"))
      {
         char substr[128];
         dStrcpy(substr, name);
         len -= 5;
         substr[len] = 0; // new null termination.
         GetFNum(str2p(substr), &fontid);
         wantsBold = true;
      }

      if (fontid == 0)
      {
         Con::errorf(ConsoleLogEntry::General,"Error creating font [%s (%d)] -- it doesn't exist on this machine.",name, size);
         return(NULL);
      }
   }

   Boolean aaWas;
   S16 aaSize;
   CGrafPtr savePort;
   GDHandle saveGD;
   GetGWorld(&savePort, &saveGD);
   
   aaWas = IsAntiAliasedTextEnabled(&aaSize);
   SetAntiAliasedTextEnabled(true, 6);
   
   RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};
   RGBColor black = {0, 0, 0};
   PixMapHandle pmh;

   SetGWorld(fontgw, NULL);
   PenNormal(); // reset pen attribs.
   // shouldn't really need to do this, right?
   RGBBackColor(&white);
   RGBForeColor(&black);

   // set proper font.
   // mac fonts are coming out a bit bigger than PC - think PC is like 80-90dpi comparatively, vs 72dpi.
   // so, let's tweak here.  20=>16. 16=>13. 12=>9. 10=>7.
   TextSize(size - 2 - (int)((float)size * 0.1));
   TextFont(fontid);
   TextMode(srcCopy);
   if (wantsBold)
      TextFace(bold);

   // get font info
   int cx, cy, ry, my;
   FontInfo fi;
   GetFontInfo(&fi); // gets us basic glyphs.
   cy = fi.ascent + fi.descent + fi.leading + 1; // !!!! HACK.  Not per-char-specific.
   
   pmh = GetGWorldPixMap(fontgw);

   GFont *retFont = new GFont;

   Rect b = {0,0,0,0};
   int drawBase = fi.ascent+1;
   int outBase = fi.ascent+fi.descent-1;
   for(S32 i = 32; i < 256; i++)
   {
      if (!LockPixels(pmh))
      {
         UpdateGWorld(&fontgw, fontdepth, &fontrect, NULL, NULL, keepLocal);
         pmh = GetGWorldPixMap(fontgw);
         // for now, assume we're good to go... TBD!!!!
         LockPixels(pmh);
      }
      
      // clear the port.
      EraseRect(&fontrect);
      // reset position to left edge, bottom of line for this font style.
      MoveTo(0, drawBase);
      // draw char & calc its pixel width.
      DrawChar(i);
      cx = CharWidth(i);      

      b.right = cx+1;
      b.bottom = cy+1; // in case descenders drop too low, we want to catch the chars.
      ry = CopyCharToRaw(rawmap, pmh, b);

      UnlockPixels(pmh);

      if (ry<0) // bitmap was blank.
      {
         Con::printf("Blank character %c", i);
         if (cx)
            retFont->insertBitmap(i, rawmap, 0, 0, 0, 0, 0, cx);
      }
      else
         retFont->insertBitmap(i, rawmap, 256, cx+1, cy+1, 0, outBase, cx);
   }

   retFont->pack(cy, outBase);

//   if (actualChars==0)
//      Con::errorf(ConsoleLogEntry::General,"Error creating font: %s %d",name, size);

   //clean up local vars
   if (aaWas)
      SetAntiAliasedTextEnabled(aaWas, aaSize);
   SetGWorld(savePort, saveGD);
   
   return retFont;
}
