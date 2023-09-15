//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WATERBLOCK_H_
#define _WATERBLOCK_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif

#ifndef _FLUID_H_
#include "terrain/fluid.h"
#endif

//==============================================================================
class AudioEnvironment;

class WaterBlock : public SceneObject
{
    typedef SceneObject Parent;

public:

    enum EWaterType
    {
        eWater            = 0,
        eOceanWater       = 1,
        eRiverWater       = 2,
        eStagnantWater    = 3,
        eLava             = 4,
        eHotLava          = 5,
        eCrustyLava       = 6,
        eQuicksand        = 7,
    };

    enum WaterConst
    {
       WC_NUM_SUBMERGE_TEX = 2,
    };

   // MM: Depth-map Resolution.
   enum WaterAttributes
   {
      eDepthMapResolution   = 512,
   };


private:
    fluid               mFluid;
    TextureHandle       mSurfaceTexture;
    TextureHandle       mSpecMaskTex;           ///< Specular mask texture.
    TextureHandle       mEnvMapOverTexture;     ///< Overhead environment map texture handle.
    TextureHandle       mEnvMapUnderTexture;    ///< Undersea environment map texture handle.
    TextureHandle       mShoreTexture;          ///< Shore texture handle.
    PlaneF              mClipPlane[6];          ///< Frustrum clip planes: 0=T 1=B 2=L 3=R 4=N 5=F
    TerrainBlock*       mpTerrain;              ///< Terrain block.
    F32                 mSurfaceZ;              ///< Height of surface (approx.)

    // Fields exposed to the editor.
    EWaterType          mLiquidType;            ///< Type of liquid: Water?  Lava?  What?
    F32                 mDensity;               ///< Density of liquid.
    F32                 mViscosity;             ///< Viscosity of liquid.
    F32                 mWaveMagnitude;         ///< Size of waves.
    StringTableEntry    mSurfaceName;           ///< Surface texture.
    StringTableEntry    mSpecMaskName;          ///< Specular mask texture.
    F32                 mSurfaceOpacity;        ///< Opacity of surface texture.
    StringTableEntry    mEnvMapOverName;      ///< Overhead environment map texture name.
    StringTableEntry    mEnvMapUnderName;      ///< Undersea environment maptexture name
    F32                 mEnvMapIntensity;      ///< Intensity of environment maps.
    StringTableEntry    mShoreName;             ///< Shore texture name.
    StringTableEntry    mSubmergeName[WC_NUM_SUBMERGE_TEX]; ///< Name of submerge texture.
    bool                mRemoveWetEdges;        ///< Remove wet edges?
    AudioEnvironment *  mAudioEnvironment;      ///< Audio environment handle.

   bool                mEditorApplied;         ///< Editor Applied Flag.
   GBitmap*            mDepthBitmap;           ///< Depth Bitmap.
   TextureHandle       mDepthTexture;          ///< Depth Texture.
   GBitmap*            mShoreDepthBitmap;      ///< Shore Bitmap.
   TextureHandle       mShoreDepthTexture;     ///< Shore Texture.
   bool                mUseDepthMap;           ///< Use Depth-Map Flag.
   F32                 mShoreDepth;            ///< Shore Depth.
   F32                 mMinAlpha;              ///< Minimum Alpha.
   F32                   mMaxAlpha;              ///< Maximum Alpha.
   F32                 mDepthGradient;         ///< Depth Gradient.
   F32                 mTessellationSurface;   ///< Tessellation Surface.
   F32                 mTessellationShore;     ///< Tessellation Shore.
   F32                 mSurfaceParallax;       ///< Surface Parallax.
   F32                 mFlowAngle;               ///< Flow Angle.
   F32                 mFlowRate;              ///< Flow Rate.
   F32                 mDistortGridScale;      ///< Distort Grid Scale.
   F32                 mDistortMagnitude;      ///< Distort Magnitude.
   F32                 mDistortTime;           ///< Distortion Time. 
   ColorF            mSpecColor;
   F32               mSpecPower;

    TextureHandle mLocalSubmergeTexture[WC_NUM_SUBMERGE_TEX];

    static TextureHandle mSubmergeTexture[WC_NUM_SUBMERGE_TEX];

public:

    WaterBlock();
    ~WaterBlock();

    bool onAdd                  ( void );
    void onRemove               ( void );
    bool onSceneAdd             ( SceneGraph* graph );
    void setTransform           ( const MatrixF& mat );
    void setScale               ( const VectorF& scale );
    bool prepRenderImage        ( SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState=false);
    void renderObject           ( SceneState *state, SceneRenderImage *image);
    void inspectPostApply       ( void );
    F32  getSurfaceHeight       ( void ) { return mSurfaceZ; }

    void UpdateFluidRegion      ( void );
    static void SnagTerrain     ( SceneObject* sceneObj, void * key );

    static void cToggleWireFrame( SimObject*, S32, const char** );
	void toggleWireFrame();
	
    DECLARE_CONOBJECT(WaterBlock);

    static void initPersistFields();

    EWaterType getLiquidType() const                 { return mLiquidType; }
    static bool isWater      ( U32 liquidType );
    static bool isLava       ( U32 liquidType );
    static bool isQuicksand  ( U32 liquidType );

    F32 getViscosity() const { return mViscosity; }
    F32 getDensity()   const { return mDensity;   }

    U32  packUpdate  ( NetConnection* conn, U32 mask, BitStream *stream );
    void unpackUpdate( NetConnection* conn,           BitStream *stream );

    bool isPointSubmerged      ( const Point3F &pos, bool worldSpace = true ) const;
    bool isPointSubmergedSimple( const Point3F &pos, bool worldSpace = true ) const;

    AudioEnvironment * getAudioEnvironment() { return(mAudioEnvironment); }

    static bool             mCameraSubmerged;
    static U32              mSubmergedType;

    static TextureHandle getSubmergeTexture( U32 index ){ return mSubmergeTexture[index]; }

protected:
   bool castRay( const Point3F& start, const Point3F& end, RayInfo* info );
   void CalculateDepthMaps(void);                                                ///< Calculate Depth Map.
   void GenerateDepthTextures(GBitmap* pBitmap, TextureHandle& mTexture, bool ShoreFlag);      ///< Generate Depth.
};

#endif
