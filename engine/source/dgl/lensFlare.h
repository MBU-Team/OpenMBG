//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _LENSFLARE_H_
#define _LENSFLARE_H_

#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

//**************************************************************************
// Lens flare data
//**************************************************************************
struct LFlare
{
   ColorF         color;
   TextureHandle  tex;
   F32            size;          // size in screen pixels (scaled to 640x480)
   F32            offset;        // offset of flare along flare line values around 0.0-1.0 are good
   

   LFlare()
   {
      dMemset( this, 0, sizeof( LFlare ) );
      color.set( 1.0, 1.0, 1.0, 1.0 );
   }
};

//**************************************************************************
// Lens Flare
//**************************************************************************
class LensFlare
{
private:
   Vector <LFlare*> mFlareList;

   void renderFlare( Point3F &pos, const LFlare &flare );

public:
   ~LensFlare();
   void addFlare( LFlare &flare );
   void render( const MatrixF &camTrans, const Point3F &lightPos );



};




#endif
