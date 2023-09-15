//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/lensFlare.h"
#include "dgl/dgl.h"


//**************************************************************************
// Lens Flare
//**************************************************************************

//--------------------------------------------------------------------------
// Clean up
//--------------------------------------------------------------------------
LensFlare::~LensFlare()
{
   for( int i=0; i<mFlareList.size(); i++ )
   {
      delete mFlareList[i];
   }
}

//--------------------------------------------------------------------------
// Add Flare
//--------------------------------------------------------------------------
void LensFlare::addFlare( LFlare &flare )
{
   LFlare *newFlare = new LFlare;
   *newFlare = flare;

   mFlareList.push_back( newFlare );
}

//--------------------------------------------------------------------------
// Render
//--------------------------------------------------------------------------
void LensFlare::render( const MatrixF &camTrans, const Point3F &lightPos )
{

   Point3F camPos = camTrans.getPosition();
   Point3F camDir;
   camTrans.getRow( 1, &camDir );


   Point3F camAdjust = camPos + camDir;

   Point3F lensFlareLine = lightPos - ( camAdjust );
   lensFlareLine.normalize();

   Point3F startPoint = camAdjust - lensFlareLine;
   lensFlareLine *= 2.0;


   for( int i=0; i<mFlareList.size(); i++ )
   {
      const LFlare *flare = mFlareList[i];
      Point3F flarePos = startPoint + lensFlareLine * flare->offset;
      renderFlare( flarePos, *flare );
   }

}

//--------------------------------------------------------------------------
// Render flare
//--------------------------------------------------------------------------
void LensFlare::renderFlare( Point3F &pos, const LFlare &flare )
{

   Point3F screenPoint;
   if( !dglPointToScreen( pos, screenPoint ) )
   {
      return;
   }

   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();

   // set ortho mode
   RectI viewport;
   dglGetViewport(&viewport);
   dglSetClipRect( viewport );

   glColor4fv( flare.color );
   glBindTexture(GL_TEXTURE_2D, flare.tex.getGLName());

   dglDraw2DSquare( Point2F( screenPoint.x, screenPoint.y ), flare.size, 0 );


   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
}
