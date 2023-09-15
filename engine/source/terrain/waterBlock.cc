//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/waterBlock.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "core/bitStream.h"
#include "math/mBox.h"
#include "dgl/dgl.h"
#include "core/color.h"
#include "terrain/terrData.h"
#include "terrain/terrRender.h"
#include "math/mathIO.h"
#include "sceneGraph/sgUtil.h"
#include "audio/audioDataBlock.h"

//==============================================================================

IMPLEMENT_CO_NETOBJECT_V1(WaterBlock);

//==============================================================================

bool           WaterBlock::mCameraSubmerged = false;
U32            WaterBlock::mSubmergedType   = 0;
TextureHandle  WaterBlock::mSubmergeTexture[WC_NUM_SUBMERGE_TEX];

//==============================================================================
// I know this is a bit of a hack.  I've done this in order to avoid a coupling
// between the fluid and the rest of the system.
// 
static SceneState* pSceneState = NULL;

static F32 FogFunction( F32 Distance, F32 DeltaZ )
{
   return( pSceneState->getHazeAndFog( Distance, DeltaZ ) );
}

//==============================================================================

WaterBlock::WaterBlock()
{
   mNetFlags.set(Ghostable | ScopeAlways);
   mTypeMask = WaterObjectType;

   mObjBox.min.set( 0, 0, 0 );
   mObjBox.max.set( 1, 1, 1 );

    mLiquidType				= eOceanWater;
    mDensity				=  1; 
    mViscosity				= 15; 
    mWaveMagnitude			= 1.0f; 
    mSurfaceTexture			= TextureHandle();
    mSpecMaskTex           = TextureHandle();
    mSurfaceOpacity			= 0.75f; 
    mEnvMapOverTexture		= TextureHandle();		// MM: Added Over/Under Env Texture Support.
    mEnvMapUnderTexture		= TextureHandle();		// MM: Added Over/Under Env Texture Support.
    mEnvMapIntensity		= 0.4f;					// MM: Added Over/Under Env Texture Support.
	mShoreTexture			= TextureHandle();		// MM: Added Shore Texture.
    mRemoveWetEdges			= false;
    mAudioEnvironment		= 0;
    
    // lets be good little programmers and initialize our data!
    mSurfaceName = NULL;
    mSpecMaskName = NULL;
    mEnvMapOverName = NULL;		// MM: Added Over/Under Env Texture Support.
    mEnvMapUnderName = NULL;	// MM: Added Over/Under Env Texture Support.
	mShoreName = NULL;			// MM: Added Shore Name.

    dMemset( mSubmergeName, 0, sizeof( mSubmergeName ) );

    mpTerrain        = NULL;
    mSurfaceZ        = 0.0f;
  
	// MM: Reset Depth-Map Controls.
	mEditorApplied			= false;
	mShoreDepth				= 20.0f;
	mMinAlpha				= 0.03f;
	mMaxAlpha				= 1.0f;
	mDepthGradient			= 1.0f;
	mUseDepthMap			= true;
	mTessellationSurface	= 50;
	mTessellationShore		= 60;
	mSurfaceParallax		= 0.5f;
	mFlowAngle				= 0.0f;
	mFlowRate				= 0.0f;
	mDistortGridScale		= 0.1f;
	mDistortMagnitude		= 0.05f;
	mDistortTime			= 0.5f;
	
   mFluid.SetFogFn( FogFunction );
   mSpecColor.set( 1.0, 1.0, 1.0, 1.0 );
   mSpecPower = 6;
}

//==============================================================================

WaterBlock::~WaterBlock()
{
}

//==============================================================================

void WaterBlock::SnagTerrain( SceneObject* sceneObj, void * key )
{
    WaterBlock* pWater = (WaterBlock*)key;
    pWater->mpTerrain  = dynamic_cast<TerrainBlock*>(sceneObj);
}

//==============================================================================

void WaterBlock::UpdateFluidRegion( void )
{
    MatrixF M;
    Point3F P;

    P    = mObjToWorld.getPosition();
    P.x += 1024.0f;
    P.y += 1024.0f;

    mSurfaceZ = P.z + mObjScale.z;

   mFluid.SetInfo( P.x,
      P.y,
                    mObjScale.x, mObjScale.y,
                    mSurfaceZ,
                    mWaveMagnitude,
                    mSurfaceOpacity,
                    mEnvMapIntensity,                    
                    mRemoveWetEdges,
					mUseDepthMap,
					mTessellationSurface,
					mTessellationShore,
					mSurfaceParallax,
					mFlowAngle,
					mFlowRate,
					mDistortGridScale,
					mDistortMagnitude,
					mDistortTime,
               mSpecColor,
               mSpecPower );		// MM: Added Various Parameters.

    P.x -= 1024.0f;
    P.y -= 1024.0f;

    M.identity();
    M.setPosition( P );

    Parent::setTransform( M );

    resetWorldBox();

    if( isServerObject() )
        setMaskBits(1);
}

//==============================================================================

void WaterBlock::CalculateDepthMaps(void)
{
	// Generate Depth Textures.
	GenerateDepthTextures(mDepthBitmap, mDepthTexture, false);
	GenerateDepthTextures(mShoreDepthBitmap, mShoreDepthTexture, true);
}

//==============================================================================

///   Generate depth textures for water interaction with terrain.
///
///   What we are doing here is sample the terrain height at it's own resolution over the area of the
///   defined waterblock.  With these height points we can calculate an appropriate alpha value for the
///   depth-map texture texel.  This results in a *very* coarse alpha-map but this is resolved by passing
///   the texture through an iterative blur-convolution.
///
///   The resultant texture is used to alpha-blend in the surfaces and environment map using multi-texturing.
///   @author Melv May
void WaterBlock::GenerateDepthTextures(GBitmap* pBitmap, TextureHandle& mTexture, bool ShoreFlag)
{
	// Is this a client object?
	if (isClientObject())
	{
		// Yes, so we should have a depth-map bitmap.
		AssertWarn(pBitmap != NULL, "Waterblock: We should have a depth-map bitmap!");
		
		// Do we have the terrain yet?
		if (mpTerrain)
		{
			F32		DepthPoint;
			U32		FluidPointColour;
			U32*	pTexelBlockOutput;

			// Fetch the Bitmaps Data.
			U32* pDepthBits = (U32*)pBitmap->getAddress(0,0,0);

			// Calculate Fluid Min/Max Colour-Alpha.
			const U32 mFluidColour	= 0xffffff;
			const U32 mMinColour	= (mFluidColour + (((U32)(mMinAlpha * 255)) << 24));
			const U32 mMaxColour	= (mFluidColour + (((U32)(mMaxAlpha * 255)) << 24));

			// Calculate the Depth Bitmap Resolutions.
			F32		DepthSqrResX = pBitmap->getWidth(0) / mFluid.m_SquaresInX;
			F32		DepthSqrResY = pBitmap->getHeight(0) / mFluid.m_SquaresInY;

			// Calculate the Inverse Depth Bitmap Resolutions.
			F32		InvDepthSqrResX = 1.0f / DepthSqrResX;
			F32		InvDepthSqrResY = 1.0f / DepthSqrResY;

			// Calculate the Depth Bitmap Texel Resolutions.
			F32 DepthTexResX = mObjScale.x / eDepthMapResolution;
			F32 DepthTexResY = mObjScale.y / eDepthMapResolution;


			// Calculate the Depth Bitmap Strides.
			U32		XStride = pBitmap->getWidth(0);
			U32		YStride = pBitmap->getHeight(0);

			// Calculate the FluidLevel.
			F32		FluidLevel = mSurfaceZ;
			F32		Height;

			// Fetch the Waterblock Position.
			Point3F	TerrainPos = mObjToWorld.getPosition();
			// Change Spaces.
			TerrainPos.x += 1024.0f;
			TerrainPos.y += 1024.0f;

			// Terrain Height Offsets.
			F32 TerrainOffsetX;
			F32 TerrainOffsetY = 0;

			// Calculate Alpha Range.
			F32 AlphaRange = mMaxAlpha - mMinAlpha;

			// Calculate depth Alpha-Texel Map.
			for (s32 t = 0; t < eDepthMapResolution; t++)
			{
				// Reset Terrain Height Offset X.
				TerrainOffsetX = 0;

				for (s32 s = 0; s < eDepthMapResolution; s++)
				{
					// Fetch Terrain Height at current position.
					if ( mpTerrain->getHeight(	Point2F(TerrainPos.x + TerrainOffsetX , TerrainPos.y + TerrainOffsetY),
												&Height) )
					{
						// Got a valid height so ...

						// Calculate Depth Point.
						DepthPoint = FluidLevel - Height;

						// Are we below the minimum depth?
						if (DepthPoint <= 0)
						{
							// Yes, so set to minimum colour/alpha.
							FluidPointColour = mMinColour;
						}
						// ... so are we below the maximum depth?
						else if (DepthPoint < mShoreDepth)
						{
							// Normalise Range.
							F32 NormRange = mClampF(DepthPoint / mShoreDepth, 0.0f, 1.0f);
					
							// Calculate Depth Alpha.
							F32 DepthAlpha = (AlphaRange * mPow(NormRange, 1.0f / mDepthGradient)) + mMinAlpha;

							// Calculate Resultant Fluid Colour/Alpha.
							FluidPointColour = ((U32)(DepthAlpha*255)) << 24 | mFluidColour;
						}
						// ... out of our depth range ...
						else
						{
							// Are we doing a shoreline depth-map?
							if (ShoreFlag)
							{
								// Yesm so normalise Range.
								F32 NormRange = mClampF((DepthPoint-mShoreDepth) / mShoreDepth, 0.0f, 1.0f);
						
								// Calculate Depth Alpha.
								F32 DepthAlpha = mMaxAlpha - ((AlphaRange * mPow(NormRange, 1.0f / mDepthGradient)) + mMinAlpha);

								// Calculate Resultant Fluid Colour/Alpha.
								FluidPointColour = ((U32)(255*DepthAlpha)) << 24 | mFluidColour;
							}
							else
							{
								// No so clip to maximum colour/alpha.
								FluidPointColour = mMaxColour;
							}
						}
					}
					else
					{
						// Eeek, empty grid square ...
						FluidPointColour = mFluidColour;
					}

					// Calculate Position of Texel Block Origin.
					pTexelBlockOutput = pDepthBits + (U32)(s + (t * YStride));

					// Write Point Height to Texel.
					*pTexelBlockOutput = FluidPointColour;

					// Move to next Terrain Height Point.
					TerrainOffsetX += DepthTexResX;
				}

				// Move to next Terrain Height Point.
				TerrainOffsetY += DepthTexResY;
			}

			// Update the texture.
			mTexture.refresh();
#if 0
			// --------------------------------------------------------------------------------------
			// DEBUG CODE:  Dump the Depth Alpha-Map as a PNG file.
			// --------------------------------------------------------------------------------------

			// Output file.
			FileStream fStream;
			if(!fStream.open("depthmap.png", FileStream::Write))
			{   
			  Con::printf("Waterblock: Failed to open debug depth-map file!");
			  return;
			}
			else
			{
			   mDepthTexture.getBitmap()->writePNG(fStream);
			   fStream.close();
			}
#endif
			// --------------------------------------------------------------------------------------
		}
	}
}

//==============================================================================

bool WaterBlock::onAdd()
{
    if( !isClientObject() ) 
    {
        // Make sure that the Fluid has had a chance to tweak the values.
        UpdateFluidRegion();
    }

    if( !Parent::onAdd() )
        return false;

    if(!mRemoveWetEdges)
    {
       mObjBox.min.set(-1e4, -1e4, 0);
       mObjBox.max.set( 1e4,  1e4, 1);
    }

    if( isClientObject() ) 
    { 
        // load textures
        bool success = true;

        mSurfaceTexture		= TextureHandle( mSurfaceName,  MeshTexture );
        mSpecMaskTex		= TextureHandle( mSpecMaskName,  MeshTexture );
        mEnvMapOverTexture	= TextureHandle( mEnvMapOverName,   MeshTexture );	// MM: Added Over/Under Env Texture Support.
        mEnvMapUnderTexture	= TextureHandle( mEnvMapUnderName,   MeshTexture );	// MM: Added Over/Under Env Texture Support.
        mShoreTexture		= TextureHandle( mShoreName,	 MeshTexture );		// MM: Added Shore Texture.

		// MM: Removed this as it was causing a resource failure.
        //if(!mSurfaceTexture || !mEnvMapOverTexture || !mEnvMapUnderTexture)
        //   success = false;

        for( int i=0; i<WC_NUM_SUBMERGE_TEX; i++ )
        {
           if( mSubmergeName[i] && mSubmergeName[i][0] )
           {
              mLocalSubmergeTexture[i] = TextureHandle( mSubmergeName[i], MeshTexture );
              mSubmergeTexture[i] = mLocalSubmergeTexture[i];
              if(!mLocalSubmergeTexture[i])
                 success = false;
           }
        }
        if(!success && !bool(mServerObject))
           return false;
        
		// MM: Create a Depth-map bitmap.
		mDepthBitmap = new GBitmap(eDepthMapResolution, eDepthMapResolution, false, GBitmap::RGBA);
		if(!mDepthBitmap) return false;
		// MM: Create an anonymous Depth-map texture.
		mDepthTexture = TextureHandle(NULL, mDepthBitmap, false);

		// MM: Create a Shore-map bitmap.
		mShoreDepthBitmap = new GBitmap(eDepthMapResolution, eDepthMapResolution, false, GBitmap::RGBA);
		if(!mShoreDepthBitmap) return false;
		// MM: Create an anonymous Shore-Depth-map texture.
		mShoreDepthTexture = TextureHandle(NULL, mShoreDepthBitmap, false);

        // MM: Register these textures with the Fluid.
        mFluid.SetTextures( mSurfaceTexture,
                            mEnvMapOverTexture,
                            mEnvMapUnderTexture,
							mShoreTexture,
							mDepthTexture,
							mShoreDepthTexture,
                     mSpecMaskTex);
    }
    resetWorldBox();
    addToScene();

    return( true );
}

//==============================================================================

void WaterBlock::onRemove()
{

   // clear static texture handles
   for( int i=0; i<WC_NUM_SUBMERGE_TEX; i++ )
   {
      mSubmergeTexture[i] = NULL;
   }

   removeFromScene();
   Parent::onRemove();
}

//==============================================================================

bool WaterBlock::onSceneAdd( SceneGraph* pGraph )
{
   if( Parent::onSceneAdd(pGraph) )
   {
      // Attempt to get the terrain.
      if( (mpTerrain == NULL) && (mContainer != NULL) )
      {
         mContainer->findObjects( mWorldBox, (U32)TerrainObjectType, SnagTerrain, this );
         if( mpTerrain )
         {
            mFluid.SetTerrainData( mpTerrain->heightMap );
            // MM: Calculate Depth Maps.
            if (isClientObject()) CalculateDepthMaps();
         }
      }

      return( true );
   }
   return( false );
}

//==============================================================================
// The incoming matrix transforms the water into the WORLD.  This includes any
// offset in the terrain, so the translation can be negative.  We need to get 
// the translation to be expressed in "terrain square" terms.  The terrain 
// offset is always -1024,-1024.

void WaterBlock::setTransform( const MatrixF &mat )
{
   mObjToWorld = mat;
   UpdateFluidRegion();
}

//==============================================================================

void WaterBlock::setScale( const VectorF & scale )
{
   mObjScale = scale;
   UpdateFluidRegion();
}

//==============================================================================

bool WaterBlock::prepRenderImage( SceneState* state, 
                                 const U32   stateKey,
                                 const U32, 
                                 const bool )
{
   // Attempt to get the terrain.
   if( (mpTerrain == NULL) && (mContainer != NULL) )
   {
      mContainer->findObjects( mWorldBox, (U32)TerrainObjectType, SnagTerrain, this );
      if( mpTerrain )
      {
         mFluid.SetTerrainData( mpTerrain->heightMap );

         // MM: Calculate Depth Maps.
         if (isClientObject()) CalculateDepthMaps();
      }
   }


   if (isLastState(state, stateKey))
      return false;
   setLastState(state, stateKey);

   // This should be sufficient for most objects that don't manage zones, and
   //  don't need to return a specialized RenderImage...
   if (state->isObjectRendered(this)) {
      SceneRenderImage* image = new SceneRenderImage;
      image->obj = this;
      image->isTranslucent = true;

      image->sortType = SceneRenderImage::Plane;
      image->plane    = PlaneF(0, 0, 1, -mSurfaceZ);
      image->poly[0]  = Point3F(mObjBox.min.x, mObjBox.min.y, 1);
      image->poly[1]  = Point3F(mObjBox.min.x, mObjBox.max.y, 1);
      image->poly[2]  = Point3F(mObjBox.max.x, mObjBox.max.y, 1);
      image->poly[3]  = Point3F(mObjBox.max.x, mObjBox.min.y, 1);
      for (U32 i = 0; i < 4; i++)
      {
         image->poly[i].convolve(mObjScale);
         getTransform().mulP(image->poly[i]);
      }

      // Calc the area of this poly
      Point3F intermed;
      mCross(image->poly[2] - image->poly[0], image->poly[3] - image->poly[1], &intermed);
      image->polyArea = intermed.len() * 0.5;

      state->insertRenderImage(image);
   }

   return false;
}

//==============================================================================

void WaterBlock::renderObject( SceneState* state, SceneRenderImage* )
{
   AssertFatal( dglIsInCanonicalState(), 
      "Error, GL not in canonical state on entry" );

   RectI   viewport;
   Point3F Eye;    // Camera in water space.
   bool    CameraSubmergedFlag = false;

   dglGetViewport  ( &viewport );
   glMatrixMode    ( GL_PROJECTION );
   glPushMatrix    ();
   state->setupObjectProjection( this );

   /****
   // Debug assist.
   // Render a wire outline around the base of the water block.
   if( 0 )
   {
   glMatrixMode    ( GL_MODELVIEW );
   glPushMatrix    ();
   dglMultMatrix   ( &mObjToWorld );

   F32 X0 = 0;
   F32 Y0 = 0;
   F32 X1 = mObjScale.x;
   F32 Y1 = mObjScale.y;
   F32 Z  = 0;

   glDisable       ( GL_TEXTURE_2D );
   glDisable       ( GL_DEPTH_TEST );
   glEnable        ( GL_BLEND );
   glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
   glPolygonMode   ( GL_FRONT_AND_BACK, GL_LINE );
   glColor4f       ( 0.6f, 0.6f, 0.0f, 0.5f );
   glBegin         ( GL_QUADS );

   glVertex3f      ( X0, Y0, Z );
   glVertex3f      ( X1, Y0, Z );
   glVertex3f      ( X1, Y1, Z );
   glVertex3f      ( X0, Y1, Z );

   glEnd           ();
   glEnable        ( GL_DEPTH_TEST );
   glColor4f       ( 0.8f, 0.8f, 0.8f, 0.8f );
   glBegin         ( GL_QUADS );

   glVertex3f      ( X0, Y0, Z );
   glVertex3f      ( X1, Y0, Z );
   glVertex3f      ( X1, Y1, Z );
   glVertex3f      ( X0, Y1, Z );

   glEnd           ();
   glPolygonMode   ( GL_FRONT_AND_BACK, GL_FILL );

   glPopMatrix     ();
   glMatrixMode    ( GL_MODELVIEW );
   }
   ****/

   // Handle the fluid.
   {
      // The water block lives in terrain space which is -1024,-1024.
      // To get into world space, we just add 1024,1024.
      // The fluid lives in world space.

      Point3F W2Lv(  1024.0f,  1024.0f, 0.0f );   // World to Local vector
      Point3F L2Wv( -1024.0f, -1024.0f, 0.0f );   // Local to World vector
      MatrixF L2Wm;                               // Local to World matrix

      L2Wm.identity();
      L2Wm.setPosition( L2Wv );

      glMatrixMode    ( GL_MODELVIEW );
      glPushMatrix    ();
      dglMultMatrix   ( &L2Wm );


      // We need the eye in water space.
      {
         Eye = state->getCameraPosition() + W2Lv;
         mFluid.SetEyePosition( Eye.x, Eye.y, Eye.z );
      }

      // We need the frustrum in water space.
      {
         MatrixF L2Cm;
         dglGetModelview( &L2Cm );
         L2Cm.inverse();

         Point3F Dummy;
         Dummy = L2Cm.getPosition();

         F64 frustumParam[6];
         dglGetFrustum(&frustumParam[0], &frustumParam[1],
            &frustumParam[2], &frustumParam[3],
            &frustumParam[4], &frustumParam[5]);

         sgComputeOSFrustumPlanes(frustumParam,
            L2Cm,
            Dummy,
            mClipPlane[1],
            mClipPlane[2],
            mClipPlane[3],
            mClipPlane[4],
            mClipPlane[5]);

         // near plane is needed as well...
         PlaneF p(0, 1, 0, -frustumParam[4]);
         mTransformPlane(L2Cm, Point3F(1,1,1), p, &mClipPlane[0]);

         //            F64     left, right, bottom, top, near, far;
         //             MatrixF L2Cm;

         //             dglGetFrustum( &left, &right, &bottom, &top, &near, &far );

         //             far = state->getVisibleDistance();

         //             Point3F Origin      ( 0,     0,    0      );
         //             Point3F FarOrigin   ( 0,     far,  0      );
         //             Point3F UpperLeft   ( left,  near, top    );
         //             Point3F UpperRight  ( right, near, top    );
         //             Point3F LowerLeft   ( left,  near, bottom );
         //             Point3F LowerRight  ( right, near, bottom );

         //             dglGetModelview( &L2Cm );
         // 	        L2Cm.inverse();

         //             Point3F Dummy;
         // 	        Dummy = L2Cm.getPosition();

         //             L2Cm.mulP( Origin     );
         //             L2Cm.mulP( FarOrigin  );
         //             L2Cm.mulP( UpperLeft  );
         //             L2Cm.mulP( UpperRight );
         //             L2Cm.mulP( LowerLeft  );
         //             L2Cm.mulP( LowerRight );

         //             // Frustrum clip planes: 0=T 1=B 2=L 3=R 4=N 5=F

         //             mClipPlane[0].set( UpperLeft,  Origin,    UpperRight ); 
         //             mClipPlane[1].set( LowerRight, Origin,    LowerLeft  ); 
         //             mClipPlane[2].set( LowerLeft,  Origin,    UpperLeft  ); 
         //             mClipPlane[3].set( UpperRight, Origin,    LowerRight ); 
         //             mClipPlane[4].set( UpperRight, UpperLeft, LowerRight ); 

         //             // far plane is reverse of near plane vector
         //             mClipPlane[5].x = -mClipPlane[4].x;
         //             mClipPlane[5].y = -mClipPlane[4].y;
         //             mClipPlane[5].z = -mClipPlane[4].z;
         //             mClipPlane[5].d = -mClipPlane[4].d + far;

         mFluid.SetFrustrumPlanes( (F32*)mClipPlane );
      }

      // Fog stuff.
      {
         pSceneState = state;
         ColorF FogColor = state->getFogColor();
         mFluid.SetFogParameters( FogColor.red, 
            FogColor.green, 
            FogColor.blue,
            state->getVisibleDistance() );
      }

      // And RENDER!
      {
         mFluid.Render( CameraSubmergedFlag );
      }

      // Clean up.
      glPopMatrix     ();
      glMatrixMode    ( GL_MODELVIEW );
   }

   //
   // And now the closing ceremonies...
   //
   glMatrixMode    ( GL_PROJECTION );    
   glPopMatrix     ();
   glTexEnvi       ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
   dglSetViewport  ( viewport );

   //
   // Oh yes.  We have to set some state information for the scene...
   //

   if( CameraSubmergedFlag )
   {
      mCameraSubmerged = true;
      mSubmergedType   = mLiquidType;
   }

   AssertFatal( dglIsInCanonicalState(), 
      "Error, GL not in canonical state on exit" );
}

//==============================================================================

void WaterBlock::inspectPostApply()
{
   // MM: Flag Editor Applied.
   mEditorApplied = true;

   resetWorldBox();
   setMaskBits(1);
}

//==============================================================================

static EnumTable::Enums gLiquidTypeEnums[] =
{
   { WaterBlock::eWater,         "Water"         },
   { WaterBlock::eOceanWater,    "OceanWater"    },
   { WaterBlock::eRiverWater,    "RiverWater"    },
   { WaterBlock::eStagnantWater, "StagnantWater" },
   { WaterBlock::eLava,          "Lava"          },
   { WaterBlock::eHotLava,       "HotLava"       },
   { WaterBlock::eCrustyLava,    "CrustyLava"    },
   { WaterBlock::eQuicksand,     "Quicksand"     }
};
static EnumTable gLiquidTypeTable( 8, gLiquidTypeEnums );

//------------------------------------------------------------------------------

void WaterBlock::initPersistFields()
{
    Parent::initPersistFields();

	// MM: Add Depth-Map Fields and reorganise fields.
	addGroup( "Debugging" );	// MM: Added Group Header.
    addField( "UseDepthMask",		TypeBool,		Offset( mUseDepthMap,			WaterBlock ) );
	endGroup( "Debugging" );	// MM: Added Group Footer.

	addGroup("Media");	// MM: Added Group Header.
    addField( "surfaceTexture",     TypeFilename,  Offset( mSurfaceName,		    WaterBlock ) );
    addField( "ShoreTexture",		TypeFilename,  Offset( mShoreName,				WaterBlock ) );
    addField( "envMapOverTexture",  TypeFilename,  Offset( mEnvMapOverName,			WaterBlock ) );
    addField( "envMapUnderTexture",	TypeFilename,  Offset( mEnvMapUnderName,		WaterBlock ) );
    addField( "submergeTexture",    TypeFilename,  Offset( mSubmergeName,			WaterBlock ), WC_NUM_SUBMERGE_TEX );
    addField( "specularMaskTex",     TypeFilename,  Offset( mSpecMaskName,		    WaterBlock ) );
	endGroup("Media");	// MM: Added Group Footer.

	addGroup("Fluid");	// MM: Added Group Header.
    addField( "liquidType",         TypeEnum,      Offset( mLiquidType,				WaterBlock ), 1, &gLiquidTypeTable );
    addField( "density",            TypeF32,       Offset( mDensity,				WaterBlock ) );
    addField( "viscosity",          TypeF32,       Offset( mViscosity,				WaterBlock ) );
	endGroup("Fluid");	// MM: Added Group Footer.

	addGroup("Surface");	// MM: Added Group Header.
    addField( "waveMagnitude",      TypeF32,       Offset( mWaveMagnitude,			WaterBlock ) );
    addField( "surfaceOpacity",     TypeF32,       Offset( mSurfaceOpacity,			WaterBlock ) );
    addField( "envMapIntensity",    TypeF32,       Offset( mEnvMapIntensity,		WaterBlock ) );
    addField( "TessSurface",		TypeF32,		Offset( mTessellationSurface,	WaterBlock ) );
    addField( "TessShore",			TypeF32,		Offset( mTessellationShore,		WaterBlock ) );
    addField( "SurfaceParallax",	TypeF32,		Offset( mSurfaceParallax,		WaterBlock ) );
	endGroup("Surface");	// MM: Added Group Footer.

	addGroup("Movement");	// MM: Added Group Header.
    addField( "FlowAngle",			TypeF32,		Offset( mFlowAngle,				WaterBlock ) );
    addField( "FlowRate",			TypeF32,		Offset( mFlowRate,				WaterBlock ) );
    addField( "DistortGridScale",	TypeF32,		Offset( mDistortGridScale,		WaterBlock ) );
    addField( "DistortMag",			TypeF32,		Offset( mDistortMagnitude,		WaterBlock ) );
    addField( "DistortTime",		TypeF32,		Offset( mDistortTime,			WaterBlock ) );
	endGroup("Movement");	// MM: Added Group Footer.

	addGroup("Depth Fx");	// MM: Added Group Header.
    addField( "ShoreDepth",			TypeF32,		Offset( mShoreDepth,			WaterBlock ) );
    addField( "DepthGradient",		TypeF32,		Offset( mDepthGradient,			WaterBlock ) );
    addField( "MinAlpha",			TypeF32,		Offset( mMinAlpha,				WaterBlock ) );
    addField( "MaxAlpha",			TypeF32,		Offset( mMaxAlpha,				WaterBlock ) );
	endGroup("Depth Fx");	// MM: Added Group Footer.

	addGroup("Misc");	// MM: Added Group Header.
    addField( "audioEnvironment",   TypeAudioEnvironmentPtr, Offset( mAudioEnvironment, WaterBlock ) );
    addField( "removeWetEdges",     TypeBool,      Offset( mRemoveWetEdges,			WaterBlock ) );
    addField( "specularColor",     TypeColorF,      Offset( mSpecColor,			WaterBlock ) );
    addField( "specularPower",     TypeF32,      Offset( mSpecPower,			WaterBlock ) );
   endGroup("Misc");	// MM: Added Group Footer.
}     

//==============================================================================

// XA: Added a class method for this one too.
void WaterBlock::toggleWireFrame()
{

      mFluid.m_ShowWire = !(mFluid.m_ShowWire);
      if( mFluid.m_ShowWire )
         Con::printf( "WaterBlock wire frame ENABLED" );
      else
         Con::printf( "WaterBlock wire frame DISABLED" );
}

ConsoleMethod( WaterBlock, toggleWireFrame, void, 2, 2, "()")
{
	object->toggleWireFrame();
}

//==============================================================================

U32 WaterBlock::packUpdate( NetConnection* c, U32 mask, BitStream* stream )
{
   U32 retMask = Parent::packUpdate( c, mask, stream );

   // No masking in here now.  
   // There's not too much data, and it doesn't change during normal game play.

   stream->writeAffineTransform( mObjToWorld );
   mathWrite( *stream, mObjScale );

    stream->writeString( mSurfaceName  );
    stream->writeString( mSpecMaskName  );
    stream->writeString( mEnvMapOverName   );		// MM: Added Over/Under Env Texture Support.
    stream->writeString( mEnvMapUnderName   );		// MM: Added Over/Under Env Texture Support.
	stream->writeString( mShoreName	   );			// MM: Added Shore Name.

    for( int i=0; i<WC_NUM_SUBMERGE_TEX; i++ )
    {
       stream->writeString( mSubmergeName[i] );
    }
   
    stream->write( (S32)mLiquidType );
    stream->write( mDensity         );
    stream->write( mViscosity       );
    stream->write( mWaveMagnitude   );
    stream->write( mSurfaceOpacity  );
    stream->write( mEnvMapIntensity );
    stream->write( mRemoveWetEdges  );

    // audio environment:
    if(stream->writeFlag(mAudioEnvironment))
       stream->writeRangedU32(mAudioEnvironment->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

	// MM: Write Depth-Map Controls.
	stream->write( mUseDepthMap );
	stream->write( mShoreDepth );
	stream->write( mMinAlpha );
	stream->write( mMaxAlpha );
	stream->write( mDepthGradient );
	stream->write( mTessellationSurface );
	stream->write( mTessellationShore );
	stream->write( mSurfaceParallax );
	stream->write( mFlowAngle );
	stream->write( mFlowRate );
	stream->write( mDistortGridScale );
	stream->write( mDistortMagnitude );
	stream->write( mDistortTime );


	stream->write( mSpecColor );
	stream->write( mSpecPower );

	// MM: Editor Applied Flag.
	stream->writeFlag(mEditorApplied);
	// MM: Reset Editor Applied Flag.
	mEditorApplied = false;

   return( retMask );
}

//==============================================================================

void WaterBlock::unpackUpdate( NetConnection* c, BitStream* stream )
{
   Parent::unpackUpdate( c, stream );

   U32 LiquidType;

   // No masking in here now. 
   // There's not too much data, and it doesn't change during normal game play.

   stream->readAffineTransform( &mObjToWorld );
   mathRead( *stream, &mObjScale );

    mSurfaceName		= stream->readSTString();
    mSpecMaskName		= stream->readSTString();
    mEnvMapOverName		= stream->readSTString();	// MM: Added Over/Under Env Texture Support.
    mEnvMapUnderName	= stream->readSTString();	// MM: Added Over/Under Env Texture Support.
	mShoreName			= stream->readSTString();	// MM: Added Shore Name.

   for( int i=0; i<WC_NUM_SUBMERGE_TEX; i++ )
   {
      mSubmergeName[i] = stream->readSTString();
   }

    // MM: Register these textures with the Fluid.
    mSurfaceTexture		= TextureHandle( mSurfaceName,  MeshTexture );
    mSpecMaskTex		= TextureHandle( mSpecMaskName,  MeshTexture );
    mEnvMapOverTexture	= TextureHandle( mEnvMapOverName,   MeshTexture );	// MM: Added Over/Under Env Texture Support.
    mEnvMapUnderTexture	= TextureHandle( mEnvMapUnderName,   MeshTexture );	// MM: Added Over/Under Env Texture Support.
    mShoreTexture		= TextureHandle( mShoreName,   MeshTexture );		// MM: Added Shore Texture.
    // MM: Register these textures with the Fluid.
    mFluid.SetTextures( mSurfaceTexture,
                        mEnvMapOverTexture,
                        mEnvMapUnderTexture,
						mShoreTexture,
						mDepthTexture,
						mShoreDepthTexture,
                  mSpecMaskTex); 
    
    for( int i=0; i<WC_NUM_SUBMERGE_TEX; i++ )
    {
       if( mSubmergeName[i] && mSubmergeName[i][0] )
       {
          mLocalSubmergeTexture[i] = TextureHandle( mSubmergeName[i], MeshTexture );
          mSubmergeTexture[i] = mLocalSubmergeTexture[i];
       }
    }

   stream->read( &LiquidType       );
   stream->read( &mDensity         );
   stream->read( &mViscosity       );
   stream->read( &mWaveMagnitude   );
   stream->read( &mSurfaceOpacity  );
   stream->read( &mEnvMapIntensity );
   stream->read( &mRemoveWetEdges  );

   // audio environment:
   if(stream->readFlag())
   {
      U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
      mAudioEnvironment = dynamic_cast<AudioEnvironment*>(Sim::findObject(profileId));
   }
   else
      mAudioEnvironment = 0;

   mLiquidType = (EWaterType)LiquidType;

   // MM: Read Depth-Map Controls.
   stream->read( &mUseDepthMap );
   stream->read( &mShoreDepth );
   stream->read( &mMinAlpha );
   stream->read( &mMaxAlpha );
   stream->read( &mDepthGradient );
   stream->read( &mTessellationSurface );
   stream->read( &mTessellationShore );
   stream->read( &mSurfaceParallax );
   stream->read( &mFlowAngle );
   stream->read( &mFlowRate );
   stream->read( &mDistortGridScale );
   stream->read( &mDistortMagnitude );
   stream->read( &mDistortTime );
   stream->read( &mSpecColor );
   stream->read( &mSpecPower );


   // MM: Calculate Depth Maps if needed.
   if (stream->readFlag()) CalculateDepthMaps();

   // MM: Keep this *after* depth-map reads.
   UpdateFluidRegion();

   if( !isProperlyAdded() )
      return;

   resetWorldBox();
}

//==============================================================================
// This method can take a point in world space, or water block space. The default
// assumes pos is in world space, and therefore must transform it to waterblock
// space.

bool WaterBlock::isPointSubmerged(const Point3F &pos, bool worldSpace) const
{
   return( isPointSubmergedSimple( pos, worldSpace ) );
}

//==============================================================================

bool WaterBlock::isPointSubmergedSimple(const Point3F &pos, bool worldSpace) const
{
   Point3F Pos = pos;

   if( Pos.z > mSurfaceZ )
      return( false );

   if( worldSpace )
   {
      Pos.x += 1024.0f;
      Pos.y += 1024.0f;
   }

   return( mFluid.IsFluidAtXY( Pos.x, Pos.y ) );
}

//==============================================================================

bool WaterBlock::castRay( const Point3F& start, const Point3F& end, RayInfo* info )
{
   F32     t, x, y, X, Y;
   Point3F Pos;

   //
   // Looks like the incoming points are in parametric object space.  Great.
   //

   // The water surface is 1.0.  Bail if the ray does not cross the surface.

   if( (start.z > 1.0f) && (end.z > 1.0f) )        return( false );
   if( (start.z < 1.0f) && (end.z < 1.0f) )        return( false );

   // The ray crosses the surface plane.  Find out where.

   t = (start.z - 1.0f) / (start.z - end.z);
   x = start.x + (end.x - start.x) * t;
   y = start.y + (end.y - start.y) * t;

   Pos = mObjToWorld.getPosition();

   X = (x * mObjScale.x) + Pos.x + 1024.0f;
   Y = (y * mObjScale.y) + Pos.y + 1024.0f;

   if( mFluid.IsFluidAtXY( X, Y ) )
   {
      info->t = t;
      info->point.x  = x;
      info->point.y  = y;
      info->point.z  = 1.0f;
      info->normal.x = 0.0f;
      info->normal.y = 0.0f;
      info->normal.z = 1.0f;
      info->object   = this;
      info->material = 0;
      return( true );
   }

   // Hmm.  Guess we missed!
   return( false );
}

//==============================================================================

bool WaterBlock::isWater( U32 liquidType )
{
   EWaterType wType = EWaterType( liquidType );
   return( wType == eWater      || 
           wType == eOceanWater || 
           wType == eRiverWater || 
           wType == eStagnantWater );
}

//==============================================================================

bool WaterBlock::isLava( U32 liquidType )
{
   EWaterType wType = EWaterType( liquidType );
   return( wType == eLava    || 
           wType == eHotLava || 
           wType == eCrustyLava );
}

//==============================================================================

bool WaterBlock::isQuicksand( U32 liquidType )
{
   EWaterType wType = EWaterType( liquidType );
   return( wType == eQuicksand );
}

//==============================================================================
