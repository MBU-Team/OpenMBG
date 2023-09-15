//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mPoint.h"
#include "dgl/gTexManager.h"
#include "dgl/dgl.h"
#include "core/color.h"
#include "math/mPoint.h"
#include "math/mRect.h"
#include "dgl/gFont.h"
#include "console/console.h"
#include "math/mMatrix.h"
#include "sim/frameAllocator.h"
#include "platform/profiler.h"

namespace {

ColorI sg_bitmapModulation(255, 255, 255, 255);
ColorI sg_textAnchorColor(255, 255, 255, 255);
ColorI sg_stackColor(255, 255, 255, 255);
RectI sgCurrentClipRect;

} // namespace {}


//--------------------------------------------------------------------------
void dglSetBitmapModulation(const ColorF& in_rColor)
{
   ColorF c = in_rColor;
   c.clamp();
   sg_bitmapModulation = c;
   sg_textAnchorColor = sg_bitmapModulation;
}

void dglGetBitmapModulation(ColorF* color)
{
   *color = sg_bitmapModulation;
}

void dglGetBitmapModulation(ColorI* color)
{
   *color = sg_bitmapModulation;
}

void dglClearBitmapModulation()
{
   sg_bitmapModulation.set(255, 255, 255, 255);
}

void dglSetTextAnchorColor(const ColorF& in_rColor)
{
   ColorF c = in_rColor;
   c.clamp();
   sg_textAnchorColor = c;
}  


//--------------------------------------------------------------------------
void dglDrawBitmapStretchSR(TextureObject* texture,
                       const RectI&   dstRect,
                       const RectI&   srcRect,
                       const U32   in_flip)
{
   AssertFatal(texture != NULL, "GSurface::drawBitmapStretchSR: NULL Handle");
   if(!dstRect.isValidRect())
      return;
   AssertFatal(srcRect.isValidRect() == true,
               "GSurface::drawBitmapStretchSR: routines assume normal rects");

   glDisable(GL_LIGHTING);

   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texture->texGLName);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   F32 texLeft   = F32(srcRect.point.x)                    / F32(texture->texWidth);
   F32 texRight  = F32(srcRect.point.x + srcRect.extent.x) / F32(texture->texWidth);
   F32 texTop    = F32(srcRect.point.y)                    / F32(texture->texHeight);
   F32 texBottom = F32(srcRect.point.y + srcRect.extent.y) / F32(texture->texHeight);
   F32 screenLeft   = dstRect.point.x;
   F32 screenRight  = dstRect.point.x + dstRect.extent.x;
   F32 screenTop    = dstRect.point.y;
   F32 screenBottom = dstRect.point.y + dstRect.extent.y;

   if(in_flip & GFlip_X)
   {
      F32 temp = texLeft;
      texLeft = texRight;
      texRight = temp;
   }
   if(in_flip & GFlip_Y)
   {
      F32 temp = texTop;
      texTop = texBottom;
      texBottom = temp;
   }
   
   glColor4ub(sg_bitmapModulation.red,
             sg_bitmapModulation.green,
             sg_bitmapModulation.blue,
             sg_bitmapModulation.alpha);

   glBegin(GL_TRIANGLE_FAN);
      glTexCoord2f(texLeft, texBottom);
      glVertex2f(screenLeft, screenBottom);
   
      glTexCoord2f(texRight, texBottom);
      glVertex2f(screenRight, screenBottom);
   
      glTexCoord2f(texRight, texTop);
      glVertex2f(screenRight, screenTop);

      glTexCoord2f(texLeft, texTop);
      glVertex2f(screenLeft, screenTop);
   glEnd();

   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
}

void dglDrawBitmap(TextureObject* texture, const Point2I& in_rAt, const U32 in_flip)
{
   AssertFatal(texture != NULL, "GSurface::drawBitmap: NULL Handle");

   // All non-StretchSR bitmaps are transformed into StretchSR calls...
   //
   RectI subRegion(0, 0,
                   texture->bitmapWidth,
                   texture->bitmapHeight);
   RectI stretch(in_rAt.x, in_rAt.y,
                   texture->bitmapWidth,
                   texture->bitmapHeight);
   dglDrawBitmapStretchSR(texture,
                          stretch,
                          subRegion,
                          in_flip);
}

void dglDrawBitmapStretch(TextureObject* texture, const RectI& dstRect, const U32 in_flip)
{
   AssertFatal(texture != NULL, "GSurface::drawBitmapStretch: NULL Handle");
   AssertFatal(dstRect.isValidRect() == true,
               "GSurface::drawBitmapStretch: routines assume normal rects");

   RectI subRegion(0, 0,
                   texture->bitmapWidth,
                   texture->bitmapHeight);
   dglDrawBitmapStretchSR(texture,
                          dstRect,
                          subRegion,
                          in_flip);
}

void dglDrawBitmapSR(TextureObject *texture, const Point2I& in_rAt, const RectI& srcRect, const U32 in_flip)
{
   AssertFatal(texture != NULL, "GSurface::drawBitmapSR: NULL Handle");
   AssertFatal(srcRect.isValidRect() == true,
               "GSurface::drawBitmapSR: routines assume normal rects");

   RectI stretch(in_rAt.x, in_rAt.y,
                 srcRect.len_x(),
                 srcRect.len_y());
   dglDrawBitmapStretchSR(texture,
                          stretch,
                          srcRect,
                          in_flip);
}

U32 dglDrawText(GFont*   font,
          const Point2I& ptDraw,
          const void*    in_string,
          const ColorI*  colorTable,
          const U32      maxColorIndex)
{
   return dglDrawTextN(font, ptDraw, in_string, dStrlen((const char *) in_string), colorTable, maxColorIndex);
}

struct TextVertex
{
   Point3F p;
   Point2F t;
   ColorI c;
   void set(F32 x, F32 y, F32 tx, F32 ty, ColorI color)
   {
      p.x = x;
      p.y = y;
      p.z = 0;
      t.x = tx;
      t.y = ty;
      c = color;
   }
};

U32 dglDrawTextN(GFont*          font, 
                 const Point2I&  ptDraw, 
                 const void*     in_string, 
                 U32             n, 
                 const ColorI*   colorTable,
                 const U32       maxColorIndex)
{
   // return on zero length strings
   if( n < 1 )
      return ptDraw.x;
   PROFILE_START(DrawText);

   Point2I     pt;
   U8          c;
   const U8    *str     = (const U8*)in_string;
   const U8    *endStr  = str + n;
   pt.x                 = ptDraw.x;
   
   ColorI                  currentColor;
   S32                     currentPt = 0;
   U32                     storedWaterMark; 
   
   TextureObject *lastTexture = NULL;

   storedWaterMark   = FrameAllocator::getWaterMark();
   currentColor      = sg_bitmapModulation;

   TextVertex *vert = (TextVertex *) FrameAllocator::alloc(4 * n * sizeof(TextVertex));

   glDisable(GL_LIGHTING);
   
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);
   
   glEnableClientState ( GL_VERTEX_ARRAY );
   glVertexPointer     ( 3, GL_FLOAT, sizeof(TextVertex), &(vert[0].p) );

   glEnableClientState ( GL_COLOR_ARRAY );
   glColorPointer      ( 4, GL_UNSIGNED_BYTE, sizeof(TextVertex), &(vert[0].c) );

   glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
   glTexCoordPointer   ( 2, GL_FLOAT, sizeof(TextVertex), &(vert[0].t) );
   
   // first build the point, color, and coord arrays
   for (c = *str; str < endStr; c = *(++str))
   {
      // We have to do a little dance here since \t = 0x9, \n = 0xa, and \r = 0xd
      if ((c >=  1 && c <=  7) ||
          (c >= 11 && c <= 12) ||
          (c == 14)) 
      {
         // Color code
         if (colorTable) 
         {
            static U8 remap[15] = 
            { 
               0x0,
               0x0, 
               0x1, 
               0x2, 
               0x3, 
               0x4, 
               0x5, 
               0x6, 
               0x0, 
               0x0,
               0x0, 
               0x7, 
               0x8,
               0x0,
               0x9 
            };

            U8 remapped = remap[c];
            // Ignore if the color is greater than the specified max index:
            if ( remapped <= maxColorIndex )
            {
               const ColorI &clr = colorTable[remapped];
               currentColor = sg_bitmapModulation = clr;
            }
         }
         continue;
      }

      // reset color?
      if ( c == 15 )
      {
         currentColor = sg_textAnchorColor;
         sg_bitmapModulation = sg_textAnchorColor;
         continue;
      }

      // push color:
      if ( c == 16 )
      {
         sg_stackColor = sg_bitmapModulation;
         continue;
      }

      // pop color:
      if ( c == 17 )
      {
         currentColor = sg_stackColor;
         sg_bitmapModulation = sg_stackColor;
         continue;
      }

      // Tab character
      if( !font->isValidChar( c ) ) 
      {
         if ( c == '\t' ) 
         {
            const GFont::CharInfo &ci = font->getCharInfo( ' ' );
            pt.x += ci.xIncrement * GFont::TabWidthInSpaces;
         }
         continue;
      }

      const GFont::CharInfo &ci = font->getCharInfo(c);
      TextureObject *newObj = font->getTextureHandle(ci.bitmapIndex);
      if(newObj != lastTexture)
      {
         if(currentPt)
         {
            glBindTexture(GL_TEXTURE_2D, lastTexture->texGLName);
            glDrawArrays( GL_QUADS, 0, currentPt );
            currentPt = 0;
         }
         lastTexture = newObj;
      }
      if(ci.width != 0 && ci.height != 0)
      {
         pt.y = ptDraw.y + font->getBaseline() - ci.yOrigin;
         pt.x += ci.xOrigin;
         
         F32 texLeft   = F32(ci.xOffset)             / F32(lastTexture->texWidth);
         F32 texRight  = F32(ci.xOffset + ci.width)  / F32(lastTexture->texWidth);
         F32 texTop    = F32(ci.yOffset)             / F32(lastTexture->texHeight);
         F32 texBottom = F32(ci.yOffset + ci.height) / F32(lastTexture->texHeight);

         F32 screenLeft   = pt.x;
         F32 screenRight  = pt.x + ci.width;
         F32 screenTop    = pt.y;
         F32 screenBottom = pt.y + ci.height;
         vert[currentPt++].set(screenLeft, screenBottom, texLeft, texBottom, currentColor);
         vert[currentPt++].set(screenRight, screenBottom, texRight, texBottom, currentColor);
         vert[currentPt++].set(screenRight, screenTop, texRight, texTop, currentColor);
         vert[currentPt++].set(screenLeft, screenTop, texLeft, texTop, currentColor);
         pt.x += ci.xIncrement - ci.xOrigin;
      }
      else
         pt.x += ci.xIncrement;
   }
/*      if (gOpenGLNoDrawArraysAlpha)
      {
			glBegin(GL_QUADS);
			for (S32 i = 0; i < rd[page].count; ++i)
			{
				glColor4fv((float *) &colArray[rd[page].start+i]);
				glTexCoord2f(texArray[rd[page].start+i].x, texArray[rd[page].start+i].y);
				glVertex2f(ptArray[rd[page].start+i].x, ptArray[rd[page].start+i].y);
			}
			glEnd();
		}
		else*/
   if(currentPt)
   {
      glBindTexture(GL_TEXTURE_2D, lastTexture->texGLName);
      glDrawArrays( GL_QUADS, 0, currentPt );
   }
   
   glDisableClientState ( GL_VERTEX_ARRAY );
   glDisableClientState ( GL_COLOR_ARRAY );
   glDisableClientState ( GL_TEXTURE_COORD_ARRAY );
   
   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
   
   // restore the FrameAllocator
   FrameAllocator::setWaterMark(storedWaterMark);

   AssertFatal(pt.x >= ptDraw.x, "How did this happen?");
   PROFILE_END();

   return pt.x - ptDraw.x;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Drawing primitives

void dglDrawLine(S32 x1, S32 y1, S32 x2, S32 y2, const ColorI &color)
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_TEXTURE_2D);

   glColor4ub(color.red, color.green, color.blue, color.alpha);
   glBegin(GL_LINES);
   glVertex2f((F32)x1 + 0.5,  (F32)y1 + 0.5);
   glVertex2f((F32)x2 + 0.5,    (F32)y2 + 0.5);
   glEnd();
	glBegin(GL_POINTS);
	glVertex2f((F32)x2 + 0.5, (F32)y2 + 0.5);
	glEnd();
}

void dglDrawLine(const Point2I &startPt, const Point2I &endPt, const ColorI &color)
{
	dglDrawLine(startPt.x, startPt.y, endPt.x, endPt.y, color);
}

void dglDrawRect(const Point2I &upperL, const Point2I &lowerR, const ColorI &color)
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_TEXTURE_2D);

   glColor4ub(color.red, color.green, color.blue, color.alpha);
   glBegin(GL_LINE_LOOP);
      glVertex2f((F32)upperL.x + 0.5, (F32)upperL.y + 0.5);
      glVertex2f((F32)lowerR.x + 0.5, (F32)upperL.y + 0.5);
      glVertex2f((F32)lowerR.x + 0.5, (F32)lowerR.y + 0.5);
      glVertex2f((F32)upperL.x + 0.5, (F32)lowerR.y + 0.5);
   glEnd();
}

// the fill convention for lined rects is that they outline the rectangle border of the
// filled region specified.

void dglDrawRect(const RectI &rect, const ColorI &color)
{
   Point2I lowerR(rect.point.x + rect.extent.x - 1, rect.point.y + rect.extent.y - 1);
   dglDrawRect(rect.point, lowerR, color);
}

// the fill convention says that pixel at upperL will be filled and
// that pixel at lowerR will NOT be filled.

void dglDrawRectFill(const Point2I &upperL, const Point2I &lowerR, const ColorI &color)
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_TEXTURE_2D);

   glColor4ub(color.red, color.green, color.blue, color.alpha);
   glRecti((S32)upperL.x, (S32)upperL.y, (S32)lowerR.x, (S32)lowerR.y);
}
void dglDrawRectFill(const RectI &rect, const ColorI &color)
{
   Point2I lowerR(rect.point.x + rect.extent.x, rect.point.y + rect.extent.y);
   dglDrawRectFill(rect.point, lowerR, color);
}

void dglDraw2DSquare( const Point2F &screenPoint, F32 width, F32 spinAngle )
{
   width *= 0.5;

   MatrixF rotMatrix( EulerF( 0.0, 0.0, spinAngle ) );

   Point3F offset( screenPoint.x, screenPoint.y, 0.0 );
   Point3F points[4];
   
   points[0] = Point3F(-width, -width, 0.0);
   points[1] = Point3F(-width,  width, 0.0);
   points[2] = Point3F( width,  width, 0.0);
   points[3] = Point3F( width, -width, 0.0);

   for( int i=0; i<4; i++ )
   {
      rotMatrix.mulP( points[i] );
      points[i] += offset;
   }

   glBegin(GL_TRIANGLE_FAN);
      glTexCoord2f(0.0, 0.0);
      glVertex2fv(points[0]);

      glTexCoord2f(0.0, 1.0);
      glVertex2fv(points[1]);

      glTexCoord2f(1.0, 1.0);
      glVertex2fv(points[2]);

      glTexCoord2f(1.0, 0.0);
      glVertex2fv(points[3]);
   glEnd();
}

void dglDrawBillboard( const Point3F &position, F32 width, F32 spinAngle )
{
   MatrixF modelview;
   dglGetModelview( &modelview );
   modelview.transpose();


   width *= 0.5;
   Point3F points[4];
   points[0] = Point3F(-width, 0.0, -width);
   points[1] = Point3F(-width, 0.0,  width);
   points[2] = Point3F( width, 0.0,  width);
   points[3] = Point3F( width, 0.0, -width);


   MatrixF rotMatrix( EulerF( 0.0, spinAngle, 0.0 ) );

   for( int i=0; i<4; i++ )
   {
      rotMatrix.mulP( points[i] );
      modelview.mulP( points[i] );
      points[i] += position;
   }

   glBegin(GL_TRIANGLE_FAN);
      glTexCoord2f(0.0, 0.0);
      glVertex3fv(points[0]);

      glTexCoord2f(0.0, 1.0);
      glVertex3fv(points[1]);

      glTexCoord2f(1.0, 1.0);
      glVertex3fv(points[2]);

      glTexCoord2f(1.0, 0.0);
      glVertex3fv(points[3]);
   glEnd();
   

}



void dglSetClipRect(const RectI &clipRect)
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   U32 screenWidth  = Platform::getWindowSize().x;
   U32 screenHeight = Platform::getWindowSize().y;

   glOrtho(clipRect.point.x, clipRect.point.x + clipRect.extent.x,
           clipRect.extent.y, 0,
           0, 1);
   glTranslatef(0, -clipRect.point.y, 0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glViewport(clipRect.point.x, screenHeight - (clipRect.point.y + clipRect.extent.y),
              clipRect.extent.x, clipRect.extent.y);

   sgCurrentClipRect = clipRect;
}

const RectI& dglGetClipRect()
{
   return sgCurrentClipRect;
}

bool dglPointToScreen( Point3F &point3D, Point3F &screenPoint )
{
   GLdouble       glMV[16];
   GLdouble       glPR[16];
   GLint          glVP[4];


   glGetDoublev(GL_PROJECTION_MATRIX, glPR);
   glGetDoublev(GL_MODELVIEW_MATRIX, glMV);

   RectI viewport;
   dglGetViewport(&viewport);

   glVP[0] = viewport.point.x;
   glVP[1] = viewport.point.y + viewport.extent.y;
   glVP[2] = viewport.extent.x;
   glVP[3] = -viewport.extent.y;

   MatrixF mv;
   dglGetModelview(&mv);
   MatrixF pr;
   dglGetProjection(&pr);

   F64 x, y, z;
   int result = gluProject( (GLdouble)point3D.x, (GLdouble)point3D.y, (GLdouble)point3D.z, (const F64 *)&glMV, (const F64 *)&glPR, (const GLint *)&glVP, &x, &y, &z );
   screenPoint.x = x;
   screenPoint.y = y;
   screenPoint.z = z;
      

   return (result == GL_TRUE);

}



bool dglIsInCanonicalState()
{
   bool ret = true;

   // Canonical state:
   //  BLEND disabled
   //  TEXTURE_2D disabled on both texture units.
   //  ActiveTexture set to 0
   //  LIGHTING off
   //  winding : clockwise ?
   //  cullface : disabled

   ret &= glIsEnabled(GL_BLEND) == GL_FALSE;
   ret &= glIsEnabled(GL_CULL_FACE) == GL_FALSE;
   GLint temp;

   if (dglDoesSupportARBMultitexture() == true) {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      ret &= glIsEnabled(GL_TEXTURE_2D) == GL_FALSE;
      glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &temp);
      ret &= temp == GL_REPLACE;

      glActiveTextureARB(GL_TEXTURE0_ARB);
      ret &= glIsEnabled(GL_TEXTURE_2D) == GL_FALSE;
      glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &temp);
      ret &= temp == GL_REPLACE;

      glClientActiveTextureARB(GL_TEXTURE1_ARB);
      ret &= glIsEnabled(GL_TEXTURE_COORD_ARRAY) == GL_FALSE;
      glClientActiveTextureARB(GL_TEXTURE0_ARB);
      ret &= glIsEnabled(GL_TEXTURE_COORD_ARRAY) == GL_FALSE;
   } else {
      ret &= glIsEnabled(GL_TEXTURE_2D) == GL_FALSE;
      glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &temp);
      ret &= temp == GL_REPLACE;

      ret &= glIsEnabled(GL_TEXTURE_COORD_ARRAY) == GL_FALSE;
   }

   ret &= glIsEnabled(GL_LIGHTING) == GL_FALSE;

   ret &= glIsEnabled(GL_COLOR_ARRAY)         == GL_FALSE;
   ret &= glIsEnabled(GL_VERTEX_ARRAY)        == GL_FALSE;
   ret &= glIsEnabled(GL_NORMAL_ARRAY)        == GL_FALSE;
   if (dglDoesSupportFogCoord())
      ret &= glIsEnabled(GL_FOG_COORDINATE_ARRAY_EXT) == GL_FALSE;

   return ret;
}


void dglSetCanonicalState()
{
   glDisable(GL_BLEND);
   glDisable(GL_CULL_FACE);
   glBlendFunc(GL_ONE, GL_ZERO);
   glDisable(GL_LIGHTING);
   if (dglDoesSupportARBMultitexture() == true) {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   } else {
      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   }

   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_NORMAL_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   if (dglDoesSupportFogCoord())
      glDisableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
}

void dglGetTransformState(S32* mvDepth,
                          S32* pDepth,
                          S32* t0Depth,
                          F32* t0Matrix,
                          S32* t1Depth,
                          F32* t1Matrix,
                          S32* vp)
{
   glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, (GLint*)mvDepth);
   glGetIntegerv(GL_PROJECTION_STACK_DEPTH, (GLint*)pDepth);

   glGetIntegerv(GL_TEXTURE_STACK_DEPTH, (GLint*)t0Depth);
   glGetFloatv(GL_TEXTURE_MATRIX, t0Matrix);
   if (dglDoesSupportARBMultitexture())
   {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glGetIntegerv(GL_TEXTURE_STACK_DEPTH, (GLint*)t1Depth);
      glGetFloatv(GL_TEXTURE_MATRIX, t1Matrix);
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }
   else
   {
      *t1Depth = 0;
      for (U32 i = 0; i < 16; i++)
         t1Matrix[i] = 0;
   }
   
   RectI v;
   dglGetViewport(&v);
   vp[0] = v.point.x;
   vp[1] = v.point.y;
   vp[2] = v.extent.x;
   vp[3] = v.extent.y;
}


bool dglCheckState(const S32 mvDepth, const S32 pDepth,
                   const S32 t0Depth, const F32* t0Matrix,
                   const S32 t1Depth, const F32* t1Matrix,
                   const S32* vp)
{
   GLint md, pd;
   RectI v;

   glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,  &md);
   glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &pd);

   GLint t0d, t1d;
   GLfloat t0m[16], t1m[16];
   glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &t0d);
   glGetFloatv(GL_TEXTURE_MATRIX, t0m);
   if (dglDoesSupportARBMultitexture())
   {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glGetIntegerv(GL_TEXTURE_STACK_DEPTH, &t1d);
      glGetFloatv(GL_TEXTURE_MATRIX, t1m);
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }
   else
   {
      t1d = 0;
      for (U32 i = 0; i < 16; i++)
         t1m[i] = 0;
   }

   dglGetViewport(&v);

   return ((md == mvDepth) &&
           (pd == pDepth) &&
           (t0d == t0Depth) &&
           (dMemcmp(t0m, t0Matrix, sizeof(F32) * 16) == 0) &&
           (t1d == t1Depth) &&
           (dMemcmp(t1m, t1Matrix, sizeof(F32) * 16) == 0) &&
           ((v.point.x  == vp[0]) &&
            (v.point.y  == vp[1]) &&
            (v.extent.x == vp[2]) &&
            (v.extent.y == vp[3])));
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Advanced hardware functionality.

// now owns the global stuff for NPatch, FSAA, rather than sceneGraph.

// define the globals first.  don't bracket them, as it doesn't hurt to have
// them defined always...

bool gNPatch = 0;
S32 gNPatchLevel = 2; ///< 2 seems optimal curve vs little-noticeable-performance-drop tradeoff.

S32 gFSAASamples = 1; ///< 1==no FSAA.

#if ENABLE_NPATCH
//------------------------------------------------------------------------------
ConsoleFunctionGroupBegin( NPatch, "Functions controlling NPatch feature.");

ConsoleFunction( setNPatch, void, 3, 3, "setFSAA(bool, int);")
{
   argc;
   gNPatch = dglDoesSupportNPatch();
   if (gNPatch)
   {
      gNPatch = dAtob(argv[1]);
      Con::setBoolVariable("$pref::OpenGL::nPatch", gNPatch?true:false);

      gNPatchLevel = dAtoi(argv[2]);
      if (gNPatchLevel<0)
         gNPatchLevel = 0;
      else if (gNPatchLevel>dglGetMaxNPatchLevel())
         gNPatchLevel = dglGetMaxNPatchLevel();
      dglSetNPatchLevel(gNPatchLevel);

      Con::printf("> NPatch is now %s. [%d]", gNPatch?"enabled":"disabled", gNPatchLevel);
   }

   if (Con::getIntVariable("$pref::TradeshowDemo", 0))
      Con::executef(2, "setNPatchBadge", gNPatch?"true":"false");
}

//------------------------------------------------------------------------------
ConsoleFunction( toggleNPatch, void, 1, 1, "toggleNPatch()" )
{
   if (dglDoesSupportNPatch())
   {
      gNPatch = !gNPatch;
      Con::printf("> NPatch is now %s. [%d]", gNPatch?"enabled":"disabled", gNPatchLevel);
      Con::setBoolVariable("$pref::OpenGL::nPatch", gNPatch?true:false);

      if (Con::getIntVariable("$pref::TradeshowDemo", 0))
         Con::executef(2, "setNPatchBadge", gNPatch?"true":"false");
   }
   else
   {
      Con::printf("> Current graphic driver does not support NPatch tessellation.");
   }
}

//------------------------------------------------------------------------------
ConsoleFunction( increaseNPatch, void, 1, 1, "increaseNPatch()" )
{
   if (gNPatchLevel<dglGetMaxNPatchLevel())
   {
      gNPatchLevel++;
      dglSetNPatchLevel(gNPatchLevel);
      Con::setIntVariable("$pref::OpenGL::nPatchLevel", gNPatchLevel);
      Con::printf("> NPatch LOD is now [%d].", gNPatchLevel);
   }
}

//------------------------------------------------------------------------------
ConsoleFunction( decreaseNPatch, void, 1, 1, "decreaseNPatch()" )
{
   if (gNPatchLevel>0)
   {
      gNPatchLevel--;
      dglSetNPatchLevel(gNPatchLevel);
      Con::setIntVariable("$pref::OpenGL::nPatchLevel", gNPatchLevel);
      Con::printf("> NPatch LOD is now [%d].", gNPatchLevel);
   }
}

ConsoleFunctionGroupEnd( NPatch );
#endif // ENABLE_NPATCH

#if defined(TORQUE_OS_MAC)
// new FSAA simple-method handling

ConsoleFunctionGroupBegin( MacFSAA, "Mac-specific FSAA control functions.");

//------------------------------------------------------------------------------
ConsoleFunction( setFSAA, void, 2, 2, "setFSAA(int);")
{
   argc;
   gFSAASamples = dAtoi(argv[1]);
   if (gFSAASamples<1)
      gFSAASamples = 1;
   else if (gFSAASamples>gGLState.maxFSAASamples)
      gFSAASamples = gGLState.maxFSAASamples;
   dglSetFSAASamples(gFSAASamples);
}

//------------------------------------------------------------------------------
ConsoleFunction( increaseFSAA, void, 1, 1, "increaseFSAA()" )
{
   if (gFSAASamples<gGLState.maxFSAASamples)
   {
      gFSAASamples<<=1;
      dglSetFSAASamples(gFSAASamples);
   }
}

//------------------------------------------------------------------------------
ConsoleFunction( decreaseFSAA, void, 1, 1, "decreaseFSAA()" )
{
   if (gFSAASamples>1)
   {
      gFSAASamples>>=1;
      dglSetFSAASamples(gFSAASamples);
   }
}

ConsoleFunctionGroupEnd( MacFSAA );

#endif

//------------------------------------------------------------------------------
ConsoleFunction(png2jpg, S32, 2, 3, "png2jpg(pngName,[quality=0-100])")
{
   extern U32 gJpegQuality;
   const char * rgbname = NULL;
   const char * alphaname = NULL;
   const char * basname = NULL;
   const char * bmpname = argv[1];
   if(argc == 3)
      gJpegQuality = dAtoi(argv[2]);
   else
      gJpegQuality = 90;
   bool basOpt = false;

   Con::printf("Converting file: %s", argv[1]);

   if (!rgbname)
   {
      char * buf = new char[dStrlen(bmpname)+32];
      dStrcpy(buf,bmpname);
      char * pos = dStrstr(buf,".png");
      if (!pos)
         pos = buf + dStrlen(buf);
      dStrcpy(pos,".jpg");
      rgbname = buf;
   }
   if (!alphaname)
   {
      char * buf = new char[dStrlen(bmpname)+32];
      dStrcpy(buf,bmpname);
      char * pos = dStrstr(buf,".png");
      if (!pos)
         pos = buf + dStrlen(buf);
      dStrcpy(pos,".alpha.jpg");
      alphaname = buf;
   }
   GBitmap bmp;
   FileStream fs;
   if (fs.open(bmpname, FileStream::Read) == false) {
      Con::printf("Error: unable to open file: %s for reading\n", bmpname);
      return -1;
   }
   if (bmp.readPNG(fs) == false) {
      Con::printf("Error: unable to read %s as a .PNG\n", bmpname);
      return -1;
   }
   fs.close();

   if (bmp.getFormat() != GBitmap::RGB &&
       bmp.getFormat() != GBitmap::RGBA) {
      Con::printf("Error: %s is not a 24 or 32-bit .PNG\n", bmpname);
      return false;
   }

   GBitmap * outRGB = NULL;
   GBitmap * outAlpha = NULL;
   GBitmap workRGB, workAlpha;
   if (bmp.getFormat() == GBitmap::RGB)
      outRGB = &bmp;
   else
   {
      S32 w = bmp.getWidth();
      S32 h = bmp.getHeight();
      workRGB.allocateBitmap(w,h,false,GBitmap::RGB);
      workAlpha.allocateBitmap(w,h,false,GBitmap::Alpha);

      U8 * rgbBits = workRGB.getWritableBits();
      U8 * alphaBits = workAlpha.getWritableBits();
      U8 * bmpBits = bmp.getWritableBits();
      for (S32 i=0; i<w; i++)
      {
         for (S32 j=0; j<h; j++)
         {
            rgbBits[i*3 + j*3*w + 0] = bmpBits[i*4 + j*4*w + 0];
            rgbBits[i*3 + j*3*w + 1] = bmpBits[i*4 + j*4*w + 1];
            rgbBits[i*3 + j*3*w + 2] = bmpBits[i*4 + j*4*w + 2];
            alphaBits[i + j*w]       = bmpBits[i*4 + j*4*w + 3];
         }
      }
      Con::printf("texture: width=%i, height=%i\n",w,h);
      outRGB = &workRGB;
      outAlpha = &workAlpha;
   }

   if (outRGB)
   {
      FileStream fws;
      if (fws.open(rgbname, FileStream::Write) == false)
      {
         Con::printf("Error: unable to open file: %s for writing\n", rgbname);
         return -1;
      }

      if (dStrstr(rgbname,".png"))
      {
         if (outRGB->writePNG(fws) == false)
         {
            fws.close();
            Con::printf("Error: couldn't write RGB as a png\n");
            return -1;
         }
      }
      else if (outRGB->writeJPEG(fws) == false)
      {
         Con::printf("Error: couldn't write RGB as a jpg\n");
         return -1;
      }
      fws.close();
   }
   if (outAlpha)
   {
      gJpegQuality = 60;
      FileStream fws;
      if (fws.open(alphaname, FileStream::Write) == false)
      {
         Con::printf("Error: unable to open file: %s for writing\n", alphaname);
         return -1;
      }

      if (dStrstr(alphaname,".png"))
      {
         if (outAlpha->writePNG(fws) == false)
         {
            fws.close();
            Con::printf("Error: couldn't write alpha as a png\n");
            return -1;
         }
      }
      else if (outAlpha->writeJPEG(fws) == false)
      {
         Con::printf("Error: couldn't write alpha as a jpg\n");
         return -1;
      }
      fws.close();
   }

   return(0);
}


