//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIOR_H_
#define _INTERIOR_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _COLLISION_H_
#include "collision/collision.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif
#ifndef _MPLANE_H_
#include "math/mPlane.h"
#endif
#ifndef _MBOX_H_
#include "math/mBox.h"
#endif
#ifndef _MSPHERE_H_
#include "math/mSphere.h"
#endif
#ifndef _CONVEX_H_
#include "collision/convex.h"
#endif
#ifndef _INTERIORLMMANAGER_H_
#include "interior/interiorLMManager.h"
#endif
#include "interiorSimpleMesh.h"


//-------------------------------------- Forward declarations
class Stream;
class EditGeometry;
class InteriorInstance;
class GBitmap;
class TextureHandle;
class RectD;
class SphereF;
class MatrixF;
class SceneState;
class MaterialList;
class AbstractPolyList;
class InteriorSubObject;
class TranslucentSubObject;
class BitVector;
struct RayInfo;
struct EdgeList;
class SurfaceHash;
class InteriorPolytope;
class FloorPlan;
class LightInfo;
class PlaneRange;
class EditInteriorResource;

//--------------------------------------------------------------------------
class InteriorConvex : public Convex
{
   typedef Convex Parent;
   friend class Interior;
   friend class InteriorInstance;

  protected:
   Interior* pInterior;
  public:
   S32       hullId;
   Box3F     box;

  public:
   InteriorConvex() { mType = InteriorConvexType; }
   InteriorConvex(const InteriorConvex& cv) {
      mObject   = cv.mObject;
      pInterior = cv.pInterior;
      hullId    = cv.hullId;
      box       = box;
   }

   Box3F getBoundingBox() const;
   Box3F getBoundingBox(const MatrixF& mat, const Point3F& scale) const;
   Point3F      support(const VectorF& v) const;
   void         getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf);
   void         getPolyList(AbstractPolyList* list);
};

class ZoneVisDeterminer
{
   enum Mode {
      FromState,
      FromRects
   };

   Mode mMode;

   SceneState* mState;
   U32         mZoneRangeOffset;
   U32         mParentZone;

  public:
   ZoneVisDeterminer() : mMode(FromRects), mState(NULL) { }

   void runFromState(SceneState*, U32, U32);
   void runFromRects(SceneState*, U32, U32);

   bool isZoneVisible(const U32) const;
};


struct ItrPaddedPoint
{
   Point3F point;
   union {
      F32   fogCoord;
      U8    fogColor[4];
   };
};


//------------------------------------------------------------------------------
//-------------------------------------- CLASS NOTES
// Interior: Base for all interior geometries.  Contains all lighting, poly,
//             portal zone, bsp info, etc. to render an interior.
//
// Internal Structure Notes:
//    IBSPNode:
//     planeIndex: Obv.
//     frontIndex/backIndex: Top bit indicates if children are leaves.
//                            Next bit indicates if leaf children are solid.
//
//    IBSPLeafSolid:
//     planeIndex: obv.
//     surfaceIndex/surfaceCount: Polys that are on the faces of this leaf.  Only
//                                 used for collision/surface info detection.
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
class Interior
{
   friend class FloorPlan;
   friend class EditGeometry;
   friend class InteriorInstance;
   friend class SceneLighting;
   friend class InteriorProxy;
   friend class TranslucentSubObject;
   friend class MirrorSubObject;
   friend class InteriorConvex;
   friend class InteriorLMManager;
   friend class EditInteriorResource;
   friend class PathedInterior;

   //-------------------------------------- Public interfaces
  public:
   Interior();
   ~Interior();

   // Misc
   U32          getDetailLevel() const;
   U32          getMinPixels() const;
   const Box3F& getBoundingBox() const;
   S32          getNumZones() const;

   // Rendering
   bool prepForRendering(const char* path);
   void rebuildVertexColors(LM_HANDLE instanceHandle,
                            Vector<ColorI>* normal,
                            Vector<ColorI>* alarm);

   bool prepRender(SceneState*    state,
                   S32            containingZone,
                   S32            baseZone,
                   U32            zoneOffset,
                   const MatrixF& OSToWS,
                   const Point3F& objScale,
                   const bool     modifyBaseState,
                   const bool     dontRestrictOutside,
                   const bool     flipClipPlanes);
   void prepTempRender(SceneState*    state,
                       S32            containingZone,
                       S32            baseZone,
                       const MatrixF& OSToWS,
                       const Point3F& objScale,
                       const bool     flipClipPlanes);

   void render(const bool useAlarmLighting, MaterialList* pMaterials,
               const LM_HANDLE instanceHandle,
               const Vector<ColorI>* normalVLights,
               const Vector<ColorI>* alarmVLights);
   void render_vc_tf(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle,
                     const Vector<ColorI>* normalVLights,
                     const Vector<ColorI>* alarmVLights);
   void render_vc_fc(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle,
                     const Vector<ColorI>* normalVLights,
                     const Vector<ColorI>* alarmVLights);
   void renderARB_vc_tf(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle,
                        const Vector<ColorI>* normalVLights,
                        const Vector<ColorI>* alarmVLights);
   void renderARB(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle);
   void renderARB_FC(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle);
   void renderLights(LightInfo*     pInfo,
                     const MatrixF& transform,
                     const Point3F& scale,
                     U32*           lightSurfaces,
                     U32            numLightSurfaces);

   void renderAsShape();
   bool useFogCoord();
   
   bool scopeZones(const S32            baseZone,
                   const Point3F&       interiorRoot,
                   bool*                interiorScopingState);

   //-------------------------------------- Collision Interface and zone scans
  public:
   bool scanZones(const Box3F&, const MatrixF&, U16* zones, U32* numZones);
   bool castRay(const Point3F&, const Point3F&, RayInfo*);
   bool buildPolyList(AbstractPolyList*, const Box3F&, const MatrixF&, const Point3F&);
   bool buildLightPolyList(U32* lightSurfaces, U32* numLightSurfaces,
                           const Box3F&, const MatrixF&, const Point3F&);

   bool getIntersectingHulls(const Box3F&, U16* hulls, U32* numHulls);
   bool getIntersectingVehicleHulls(const Box3F&, U16* hulls, U32* numHulls);

  protected:
   bool castRay_r(const U16, const U16, const Point3F&, const Point3F&, RayInfo*);
   void buildPolyList_r(InteriorPolytope& polytope,
                        SurfaceHash& hash);
   void scanZone_r(const U16      node,
                   const Point3F& center,
                   const Point3F& axisx,
                   const Point3F& axisy,
                   const Point3F& axisz,
                   U16*           zones,
                   U32*           numZones);
   void scanZoneNew(InteriorPolytope& polytope,
                    U16*           zones,
                    U32*           numZones);

   void scopeZone(const U32           currZone,
                  bool*               interiorScopingState,
                  const Point3F&      interiorRoot,
                  Vector<U32>&        zoneStack,
                  Vector<PlaneF>&     planeStack,
                  Vector<PlaneRange>& planeRangeStack);

   //-------------------------------------- Global rendering control
  public:
   enum RenderModes {
      NormalRender            = 0,
      NormalRenderLines       = 1,
      ShowDetail              = 2,
      ShowAmbiguous           = 3,
      ShowOrphan              = 4,
      ShowLightmaps           = 5,
      ShowTexturesOnly        = 6,
      ShowPortalZones         = 7,
      ShowOutsideVisible      = 8,
      ShowCollisionFans       = 9,
      ShowStrips              = 10,
      ShowNullSurfaces        = 11,
      ShowLargeTextures       = 12,
      ShowHullSurfaces        = 13,
      ShowVehicleHullSurfaces = 14,
      ShowVertexColors        = 15,
      ShowDetailLevel         = 16
   };
   enum Constants {
      NumCoordBins   = 16,

      BinsXY         = 0,
      BinsXZ         = 1,
      BinsYZ         = 2
   };

   static U32  smRenderMode;
   static bool smFocusedDebug;
   static bool smRenderEnvironmentMaps;
   static bool smUseVertexLighting;
   static bool smUseTexturedFog;
   static bool smLockArrays;

   //-------------------------------------- Persistence interface
  public:
   bool read(Stream& stream);
   bool write(Stream& stream) const;

   bool readVehicleCollision(Stream& stream);
   bool writeVehicleCollision(Stream& stream) const;

  private:
   static const U32 smFileVersion;
   void readCompressedVector(Stream& stream, Vector<U32>& vec);
   bool writePlaneVector(Stream&) const;
   bool readPlaneVector(Stream&);
   bool readLMapTexGen(Stream&, PlaneF&, PlaneF&);
   bool writeLMapTexGen(Stream&, const PlaneF&, const PlaneF&) const;
   void setupTexCoords();
   void setupZonePlanes();

   //-------------------------------------- For morian only...
  public:
   void processHullPolyLists();
   void processVehicleHullPolyLists();

   //-------------------------------------- BSP Structures
  private:
   struct IBSPNode {
      U16 planeIndex;
      U16 frontIndex;
      U16 backIndex;

      U16 terminalZone;   // if high bit set, then the lower 15 bits are the zone
                          //  of any of the subsidiary nodes.  Note that this is
                          //  going to overestimate some, since an object could be
                          //  completely contained in solid, but it's probably
                          //  going to turn out alright.
   };
   struct IBSPLeafSolid {
      U32 surfaceIndex;
      U16 surfaceCount;
   };

   bool isBSPLeafIndex(U16 index) const;
   bool isBSPSolidLeaf(U16 index) const;
   bool isBSPEmptyLeaf(U16 index) const;
   U16  getBSPSolidLeafIndex(U16 index) const;
   U16  getBSPEmptyLeafZone(U16 index) const;

   void setupAveTexGenLength();

   void truncateZoneTree();
   void truncateZoneNode(const U16);
   bool getUnifiedZone(const U16, S32*);

  public:
   static U16  getPlaneIndex(U16 index);
   static bool planeIsFlipped(U16 index);
   const PlaneF& getPlane(U16 index) const;

  private:
   bool areEqualPlanes(U16, U16) const;

   bool isNullSurfaceIndex(const U32 index) const;
   bool isVehicleNullSurfaceIndex(const U32 index) const;
   U32  getNullSurfaceIndex(const U32 index) const;
   U32  getVehicleNullSurfaceIndex(const U32 index) const;

   //-------------------------------------- Portals and Zone structures
  private:
   struct Zone {
      U16 portalStart;
      U16 portalCount;

      U32 surfaceStart;
      U32 planeStart;
      
      U16 surfaceCount;
      U16 planeCount;
      
      U32 staticMeshStart;
      U32 staticMeshCount;

      U16 flags;
      U16 zoneId;       // This is ephemeral, not persisted out.
   };

   struct Portal {
      U16 planeIndex;

      U16 triFanCount;
      U32 triFanStart;    // portals can have multiple windings

      U16 zoneFront;
      U16 zoneBack;
   };

   //-------------------------------------- Poly/Surface structures
  public:
   enum SurfaceFlags {
      SurfaceDetail         = BIT(0),
      SurfaceAmbiguous      = BIT(1),
      SurfaceOrphan         = BIT(2),
      SurfaceSharedLMaps    = BIT(3),     // Indicates that the alarm and normal states share a lightmap (for mission lighter)
      SurfaceOutsideVisible = BIT(4),
      SurfaceFlagMask       = (SurfaceDetail      |
                               SurfaceAmbiguous   |
                               SurfaceOrphan      |
                               SurfaceSharedLMaps |
                               SurfaceOutsideVisible)
   };
   enum ZoneFlags {
      ZoneInside = BIT(0)
   };

   const bool isSurfaceOutsideVisible(U32 surface) const;
      
  public:
   struct TexMatrix
   {
       S32 T;
       S32 N;
       S32 B;
       TexMatrix()
           : T(-1),
           N(-1),
           B(-1)
       {};
   };
   struct Edge
   {
       U32 vertex1;
       U32 vertex2;
       U32 normal1;
       U32 normal2;
       U32 face1;
       U32 face2;
   };
   struct TexGenPlanes {
      PlaneF planeX;
      PlaneF planeY;
   };
   struct TriFan {
      U32 windingStart;
      U32 windingCount;
   };
   struct Surface {
      U32 windingStart;          // 1

      U16 planeIndex;            // 2
      U16 textureIndex;

      U32 texGenIndex;           // 3

      U16 lightCount;            // 4
      U8  surfaceFlags;
      U8  windingCount;

      U32 fanMask;               // 5

      U32 lightStateInfoStart;   // 6

      U8  mapOffsetX;            // 7
      U8  mapOffsetY;
      U8  mapSizeX;
      U8  mapSizeY;

      bool unused;
   };
   struct NullSurface {
      U32 windingStart;

      U16 planeIndex;
      U8  surfaceFlags;
      U8  windingCount;
   };
   struct ItrFastDetail {
        struct Section
        {
            int start;
            int count;
        };
        struct VertexData
        {
            Point3F vertex;
            Point3F normal;
            Point2F texCoord;
            int windingIndex;
            int neighbourCount;
        };
        Vector<Section> mSections;
        Vector<VertexData> mVertexDatas;
   };
   struct ItrShadowVolume {
       int pad;
   };

   //-------------------------------------- Animated lighting structures
   enum LightFlags {
      AnimationAmbient  = BIT(0),
      AnimationLoop     = BIT(1),
      AnimationFlicker  = BIT(2),
      AnimationTypeMask = BIT(3) - 1,

      AlarmLight        = BIT(3)
   };
   enum LightType {
      AmbientLooping     = AnimationAmbient | AnimationLoop,
      AmbientFlicker     = AnimationAmbient | AnimationFlicker,

      TriggerableLoop    = AnimationLoop,
      TriggerableFlicker = AnimationFlicker,
      TriggerableRamp    = 0
   };

  public:
   // this is public because tools/Morian needs this defination
   struct AnimatedLight {
      U32 nameIndex;   // Light's name
      U32 stateIndex;  // start point in the state list

      U16 stateCount;  // number of states in this light
      U16 flags;       // flags (Apply AnimationTypeMask to get type)

      U32 duration;    // total duration of animation (ms)
   };
  private:
   struct LightState {
      U8  red;                // state's color
      U8  green;
      U8  blue;
      U8  _color_padding_;

      U32 activeTime;         // Time (ms) at which this state becomes active

      U32 dataIndex;          // StateData count and index for this state
      U16 dataCount;

      U16 __32bit_padding__;
   };
   struct LightStateData {
      U32   surfaceIndex;     // Surface affected by this data
      U32   mapIndex;         // Index into StateDataBuffer (0xFFFFFFFF indicates none)
      U16   lightStateIndex;  // Entry to modify in InteriorInstance
      U16   __32bit_padding__;
   };

   // convex hull collision structures...
  protected:
   struct ConvexHull {
      F32   minX;
      F32   maxX;
      F32   minY;
      F32   maxY;

      F32   minZ;
      F32   maxZ;
      U32   hullStart;
      U32   surfaceStart;

      U32   planeStart;
      U16   hullCount;
      U16   surfaceCount;
      U32   polyListPlaneStart;

      U32   polyListPointStart;
      U32   polyListStringStart;      
      U16   searchTag;

      bool  staticMesh;
   };

   struct CoordBin {
      U32   binStart;
      U32   binCount;
   };

   //
  private:
   LM_HANDLE               mLMHandle;
  public:
   LM_HANDLE getLMHandle() {return(mLMHandle);}

   // SceneLighting::InteriorProxy interface
   bool readSurface(Stream&, Surface&, TexGenPlanes&, const bool);
   const Surface & getSurface(const U32 surface) const;
   const U32 getSurfaceCount() const;
   const U8 getNormalLMapIndex(const U32 surface) const;
   const U8 getAlarmLMapIndex(const U32 surface) const;
   const U32 getWinding(const U32 index) const;
   const Point3F & getPoint(const U32 index) const;
   const TexGenPlanes & getLMTexGenEQ(const U32 index) const;
   bool hasAlarmState() const;
   const U32 getWindingCount() const;

   //-------------------------------------- Instance Data Members
  private:
   U32                     mFileVersion;
   U32                     mDetailLevel;
   U32                     mMinPixels;
   F32                     mAveTexGenLength;     // Set in Interior::read after loading the texgen planes.
   Box3F                   mBoundingBox;
   SphereF                 mBoundingSphere;

   Vector<PlaneF>          mPlanes;
   Vector<ItrPaddedPoint>  mPoints;
   Vector<U8>              mPointVisibility;
   
   Vector<Point3F>         mNormals;
   Vector<TexMatrix>       mTexMatrices;
   Vector<U32>             mTexMatIndices;

   ColorF                  mBaseAmbient;
   ColorF                  mAlarmAmbient;

   Vector<IBSPNode>        mBSPNodes;
   Vector<IBSPLeafSolid>   mBSPSolidLeaves;

   bool                    mPreppedForRender;
   MaterialList*           mMaterialList;
   TextureHandle*          mWhite;
   TextureHandle*          mWhiteRGB;

   TextureHandle*          mLightFalloff;

   Vector<TextureHandle*>  mEnvironMaps;
   Vector<F32>             mEnvironFactors;
   U32                     mValidEnvironMaps;

   Vector<U32>             mWindings;

   Vector<TexGenPlanes>    mTexGenEQs;
   Vector<TexGenPlanes>    mLMTexGenEQs;

   Vector<TriFan>          mWindingIndices;
   Vector<Surface>         mSurfaces;
   Vector<NullSurface>     mNullSurfaces;
   Vector<U32>             mSolidLeafSurfaces;

   Vector<Edge>            mEdges;

   // Portals and zones
   Vector<Zone>            mZones;
   Vector<U16>             mZonePlanes;
   Vector<U16>             mZoneSurfaces;
   Vector<U16>             mZonePortalList;
   Vector<Portal>          mPortals;
   Vector<U32>             mZoneStaticMeshes;

   // Subobjects: Doors, translucencies, mirrors, etc.
   Vector<InteriorSubObject*> mSubObjects;

   // Lighting info
   bool                    mHasAlarmState;
   U32                     mNumLightStateEntries;

   Vector<U16> mNormalIndices;
   
   Vector<GBitmap*>        mLightmaps;
   Vector<GBitmap*>        mLightDirMaps;
   Vector<bool>            mLightmapKeep;
   Vector<U8>              mNormalLMapIndices;
   Vector<U8>              mAlarmLMapIndices;

   U32                     mNumTriggerableLights;        // Note: not persisted

   // Persistent animated light structures
   Vector<AnimatedLight>   mAnimatedLights;
   Vector<LightState>      mLightStates;
   Vector<LightStateData>  mStateData;
   Vector<U8>              mStateDataBuffer;

   Vector<char>            mNameBuffer;

   Vector<ConvexHull>      mConvexHulls;
   Vector<U8>              mConvexHullEmitStrings;
   Vector<U32>             mHullIndices;
   Vector<U32>             mHullEmitStringIndices;
   Vector<U32>             mHullSurfaceIndices;
   Vector<U16>             mHullPlaneIndices;
   Vector<U16>             mPolyListPlanes;
   Vector<U32>             mPolyListPoints;
   Vector<U8>              mPolyListStrings;
   CoordBin                mCoordBins[NumCoordBins * NumCoordBins];
   Vector<U16>             mCoordBinIndices;
   U32                     mCoordBinMode;

   Vector<ConvexHull>      mVehicleConvexHulls;
   Vector<U8>              mVehicleConvexHullEmitStrings;
   Vector<U32>             mVehicleHullIndices;
   Vector<U32>             mVehicleHullEmitStringIndices;
   Vector<U32>             mVehicleHullSurfaceIndices;
   Vector<U16>             mVehicleHullPlaneIndices;
   Vector<U16>             mVehiclePolyListPlanes;
   Vector<U32>             mVehiclePolyListPoints;
   Vector<U8>              mVehiclePolyListStrings;
   Vector<ItrPaddedPoint>  mVehiclePoints;
   Vector<NullSurface>     mVehicleNullSurfaces;
   Vector<PlaneF>          mVehiclePlanes;
   Vector<U32>             mVehicleWindings;
   Vector<TriFan>          mVehicleWindingIndices;

   VectorPtr<InteriorSimpleMesh*> mStaticMeshes;
   U32 mLightMapBorderSize;
   
   U16                     mSearchTag;

   //-------------------------------------- Private interface
  private:
   const char* getName(const U32 nameIndex) const;
   static const char* getLightTypeString(const LightType);
   S32  getZoneForPoint(const Point3F&) const;

   void debugRender(MaterialList* pMaterials, LM_HANDLE instanceHandle);
   void debugRenderPortals();
   void debugNormalRenderLines();
   void debugShowDetail();
   void debugShowAmbiguous();
   void debugShowLightmaps(LM_HANDLE instanceHandle);
   void debugShowPortalZones();
   void debugShowCollisionFans();
   void debugShowOrphan();
   void debugShowStrips();
   void debugShowTexturesOnly(MaterialList* pMaterials);
   void debugShowLargeTextures(MaterialList* pMaterials);
   void debugShowNullSurfaces(MaterialList* pMaterials);
   void debugShowOutsideVisible();
   void debugShowHullSurfaces();
   void debugShowVehicleHullSurfaces(MaterialList* pMaterials);
//   void debugShowVertexColors(MaterialList* pMaterials);
   void debugShowDetailLevel();

   void debugShowOrphansFinish();

   void collisionFanFromSurface(const Surface&, U32* fan, U32* numIndices) const;
   void fullWindingFromSurface(const Surface&, U32* fan, U32* numIndices) const;
   bool projectClipAndBoundFan(U32 fanIndex, F64* pResult);
   void zoneTraversal(S32 baseZone, const bool flipClipPlanes);
   void createZoneRectVectors();
   void destroyZoneRectVectors();
   void traverseZone(const RectD* inRects, const U32 numInputRects, U32 currZone, Vector<U32>& zoneStack);
   void setupActivePolyList(ZoneVisDeterminer&, SceneState*,
                            const Point3F&, const Point3F& rViewVector,
                            const Point3F&,
                            const F32 worldz, const Point3F& scale);
   void renderSmooth(MaterialList* pMaterials, ItrFastDetail* fastDetail, bool boolParam, S32 intParam, U32 uintParam);

   void setupFog(SceneState* state);
   void clearFog();
   void setOSCamPosition(const Point3F&);

  public:
   void purgeLODData();
   void computeNormals(ItrFastDetail*);
   void computeShadowVolume(ItrShadowVolume&, Point3F, float, Point3F);
};

//------------------------------------------------------------------------------
inline bool Interior::isBSPLeafIndex(U16 index) const
{
   return (index & 0x8000) != 0;
}

inline bool Interior::isBSPSolidLeaf(U16 index) const
{
   AssertFatal(isBSPLeafIndex(index) == true, "Error, only call for leaves!");
   return (index & 0x4000) != 0;
}

inline bool Interior::isBSPEmptyLeaf(U16 index) const
{
   AssertFatal(isBSPLeafIndex(index) == true, "Error, only call for leaves!");
   return (index & 0x4000) == 0;
}

inline U16 Interior::getBSPSolidLeafIndex(U16 index) const
{
   AssertFatal(isBSPSolidLeaf(index) == true, "Error, only call for leaves!");
   return U16(index & ~0xC000);
}

inline U16 Interior::getBSPEmptyLeafZone(U16 index) const
{
   AssertFatal(isBSPEmptyLeaf(index) == true, "Error, only call for leaves!");
   return U16(index & ~0xC000);
}

inline const PlaneF& Interior::getPlane(U16 index) const
{
   AssertFatal(U32(index & ~0x8000) < mPlanes.size(),
               "Interior::getPlane: planeIndex out of range");

   return mPlanes[index & ~0x8000];
}

inline U16 Interior::getPlaneIndex(U16 index)
{
   return U16(index & ~0x8000);
}

inline bool Interior::planeIsFlipped(U16 index)
{
   return (index >> 15)!=0;
//   return (index & 0x8000) != 0;
}

inline bool Interior::areEqualPlanes(U16 o, U16 t) const
{
   return (o & ~0x8000) == (t & ~0x8000);
}

inline U32 Interior::getDetailLevel() const
{
   return mDetailLevel;
}

inline U32 Interior::getMinPixels() const
{
   return mMinPixels;
}

inline const Box3F& Interior::getBoundingBox() const
{
   return mBoundingBox;
}

inline S32 Interior::getNumZones() const
{
   return mZones.size();
}

inline bool Interior::isNullSurfaceIndex(const U32 index) const
{
   return (index & 0x80000000) != 0;
}

inline bool Interior::isVehicleNullSurfaceIndex(const U32 index) const
{
   return (index & 0x40000000) != 0;
}

inline U32 Interior::getNullSurfaceIndex(const U32 index) const
{
   AssertFatal(isNullSurfaceIndex(index), "Not a proper index!");
   AssertFatal(!isVehicleNullSurfaceIndex(index), "Not a proper index");
   return (index & ~0x80000000);
}

inline U32 Interior::getVehicleNullSurfaceIndex(const U32 index) const
{
   AssertFatal(isVehicleNullSurfaceIndex(index), "Not a proper index!");
   return (index & ~(0x80000000 | 0x40000000));
}

inline const char* Interior::getLightTypeString(const LightType type)
{
   switch (type) {
     case AmbientLooping:
      return "AmbientLooping";
     case AmbientFlicker:
      return "AmbientFlicker";
     case TriggerableLoop:
      return "TriggerableLoop";
     case TriggerableFlicker:
      return "TriggerableFlicker";
     case TriggerableRamp:
      return "TriggerableRamp";

     default:
      return "<UNKNOWN>";
   }
}

inline const char* Interior::getName(const U32 nameIndex) const
{
   return &mNameBuffer[nameIndex];
}

inline const U32 Interior::getSurfaceCount() const
{
   return(mSurfaces.size());
}

inline const Interior::Surface & Interior::getSurface(const U32 surface) const
{
   AssertFatal(surface < mSurfaces.size(), "invalid index");
   return(mSurfaces[surface]);
}

inline const U8 Interior::getNormalLMapIndex(const U32 surface) const
{
   AssertFatal(surface < mNormalLMapIndices.size(), "invalid index");
   return(mNormalLMapIndices[surface]);
}

inline const U8 Interior::getAlarmLMapIndex(const U32 surface) const
{
   AssertFatal(surface < mAlarmLMapIndices.size(), "invalid index");
   return(mAlarmLMapIndices[surface]);
}

inline const U32 Interior::getWinding(const U32 index) const
{
   AssertFatal(index < mWindings.size(), "invalid index");
   return(mWindings[index]);
}

inline const Point3F & Interior::getPoint(const U32 index) const
{
   AssertFatal(index < mPoints.size(), "invalid index");
   return(mPoints[index].point);
}

inline const Interior::TexGenPlanes & Interior::getLMTexGenEQ(const U32 index) const
{
   AssertFatal(index < mLMTexGenEQs.size(), "invalid index");
   return(mLMTexGenEQs[index]);
}

inline bool Interior::hasAlarmState() const
{
   return(mHasAlarmState);
}

inline const bool Interior::isSurfaceOutsideVisible(U32 surface) const
{
   AssertFatal(surface < mSurfaces.size(), "Interior::isSurfaceOutsideVisible: Invalid surface index");
   return ((mSurfaces[surface].surfaceFlags & SurfaceOutsideVisible)!=0);
}

inline const U32 Interior::getWindingCount() const 
{ 
   return(mWindings.size()); 
}

#endif //_INTERIOR_H_
