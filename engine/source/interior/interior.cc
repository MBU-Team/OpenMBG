//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interior.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"

#include "dgl/dgl.h"
#include "dgl/gBitmap.h"
#include "dgl/gTexManager.h"
#include "math/mMatrix.h"
#include "math/mRect.h"
#include "dgl/materialList.h"
#include "dgl/materialPropertyMap.h"
#include "interior/interiorSubObject.h"
#include "core/bitVector.h"
#include "sim/frameAllocator.h"
#include "sceneGraph/sgUtil.h"
#include "platform/profiler.h"

U32  Interior::smRenderMode            = 0;
bool Interior::smFocusedDebug          = false;
bool Interior::smRenderEnvironmentMaps = true;
bool Interior::smUseVertexLighting     = false;
bool Interior::smUseTexturedFog        = false;
bool Interior::smLockArrays            = true;


// These are setup by setupActivePolyList
U16*            sgActivePolyList      = NULL;
U32             sgActivePolyListSize  = 0;
U16*            sgEnvironPolyList     = NULL;
U32             sgEnvironPolyListSize = 0;
U16*            sgFogPolyList         = NULL;
U32             sgFogPolyListSize     = 0;
bool            sgFogActive           = false;

// Always the same size as the mPoints array
Point2F*        sgFogTexCoords        = NULL;

class PlaneRange
{
  public:
   U32 start;
   U32 count;
};

namespace {

struct PortalRenderInfo
{
   bool  render;

   F64   frustum[4];
   RectI viewport;
};

//-------------------------------------- Rendering state variables.
Point3F sgCamPoint;
F64     sgStoredFrustum[6];
RectI   sgStoredViewport;

Vector<PortalRenderInfo> sgZoneRenderInfo(__FILE__, __LINE__);

// Takes OS coords to clip space...
MatrixF sgWSToOSMatrix;
MatrixF sgProjMatrix;

PlaneF  sgOSPlaneFar;
PlaneF  sgOSPlaneXMin;
PlaneF  sgOSPlaneXMax;
PlaneF  sgOSPlaneYMin;
PlaneF  sgOSPlaneYMax;

struct ZoneRect {
   RectD rect;
   bool  active;
};

Vector<ZoneRect> sgZoneRects(__FILE__, __LINE__);

//-------------------------------------- Little utility functions
RectD outlineRects(const Vector<RectD>& rects)
{
   F64 minx =  1e10;
   F64 maxx = -1e10;
   F64 miny =  1e10;
   F64 maxy = -1e10;

   for (U32 i = 0; i < rects.size(); i++) {
      if (rects[i].point.x < minx)
         minx = rects[i].point.x;
      if (rects[i].point.y < miny)
         miny = rects[i].point.y;

      if (rects[i].point.x + rects[i].extent.x > maxx)
         maxx = rects[i].point.x + rects[i].extent.x;
      if (rects[i].point.y + rects[i].extent.y > maxy)
         maxy = rects[i].point.y + rects[i].extent.y;
   }

   return RectD(minx, miny, maxx - minx, maxy - miny);
}

void insertZoneRects(ZoneRect& rZoneRect, const RectD* rects, const U32 numRects)
{
   F64 minx =  1e10;
   F64 maxx = -1e10;
   F64 miny =  1e10;
   F64 maxy = -1e10;

   for (U32 i = 0; i < numRects; i++) {
      if (rects[i].point.x < minx)
         minx = rects[i].point.x;
      if (rects[i].point.y < miny)
         miny = rects[i].point.y;

      if (rects[i].point.x + rects[i].extent.x > maxx)
         maxx = rects[i].point.x + rects[i].extent.x;
      if (rects[i].point.y + rects[i].extent.y > maxy)
         maxy = rects[i].point.y + rects[i].extent.y;
   }

   if (rZoneRect.active == false && numRects != 0) {
      rZoneRect.rect = RectD(minx, miny, maxx - minx, maxy - miny);
      rZoneRect.active = true;
   } else {
      if (rZoneRect.rect.point.x < minx)
         minx = rZoneRect.rect.point.x;
      if (rZoneRect.rect.point.y < miny)
         miny = rZoneRect.rect.point.y;

      if (rZoneRect.rect.point.x + rZoneRect.rect.extent.x > maxx)
         maxx = rZoneRect.rect.point.x + rZoneRect.rect.extent.x;
      if (rZoneRect.rect.point.y + rZoneRect.rect.extent.y > maxy)
         maxy = rZoneRect.rect.point.y + rZoneRect.rect.extent.y;

      rZoneRect.rect = RectD(minx, miny, maxx - minx, maxy - miny);
   }
}



void fixupViewport(PortalRenderInfo& rInfo)
{
   F64 widthV  = rInfo.frustum[1] - rInfo.frustum[0];
   F64 heightV = rInfo.frustum[3] - rInfo.frustum[2];

   F64 fx0 = (rInfo.frustum[0] - sgStoredFrustum[0]) / (sgStoredFrustum[1] - sgStoredFrustum[0]);
   F64 fx1 = (sgStoredFrustum[1] - rInfo.frustum[1]) / (sgStoredFrustum[1] - sgStoredFrustum[0]);

   F64 dV0 = F64(sgStoredViewport.point.x) + fx0 * F64(sgStoredViewport.extent.x);
   F64 dV1 = F64(sgStoredViewport.point.x +
                 sgStoredViewport.extent.x) - fx1 * F64(sgStoredViewport.extent.x);

   F64 fdV0 = getMax(mFloorD(dV0), F64(sgStoredViewport.point.x));
   F64 cdV1 = getMin(mCeilD(dV1), F64(sgStoredViewport.point.x + sgStoredViewport.extent.x));

   // If the width is 1 pixel, we need to widen it up a bit...
   if ((cdV1 - fdV0) <= 1.0)
      cdV1 = fdV0 + 1;
   AssertFatal((fdV0 >= sgStoredViewport.point.x &&
                cdV1 <= sgStoredViewport.point.x + sgStoredViewport.extent.x),
               "Out of bounds viewport bounds");
   
   F64 new0 = rInfo.frustum[0] - ((dV0 - fdV0) * (widthV / F64(sgStoredViewport.extent.x)));
   F64 new1 = rInfo.frustum[1] + ((cdV1 - dV1) * (widthV / F64(sgStoredViewport.extent.x)));

   rInfo.frustum[0] = new0;
   rInfo.frustum[1] = new1;

   rInfo.viewport.point.x  = S32(fdV0);
   rInfo.viewport.extent.x = S32(cdV1) - rInfo.viewport.point.x;

   F64 fy0 = (sgStoredFrustum[3] - rInfo.frustum[3]) / (sgStoredFrustum[3] - sgStoredFrustum[2]);
   F64 fy1 = (rInfo.frustum[2] - sgStoredFrustum[2]) / (sgStoredFrustum[3] - sgStoredFrustum[2]);

   dV0 = F64(sgStoredViewport.point.y) + fy0 * F64(sgStoredViewport.extent.y);
   dV1 = F64(sgStoredViewport.point.y +
             sgStoredViewport.extent.y) - fy1 * F64(sgStoredViewport.extent.y);
   fdV0 = getMax(mFloorD(dV0), F64(sgStoredViewport.point.y));
   cdV1 = getMin(mCeilD(dV1), F64(sgStoredViewport.point.y + sgStoredViewport.extent.y));

   // If the width is 1 pixel, we need to widen it up a bit...
   if ((cdV1 - fdV0) <= 1.0)
      cdV1 = fdV0 + 1;
   AssertFatal((fdV0 >= sgStoredViewport.point.y &&
                cdV1 <= sgStoredViewport.point.y + sgStoredViewport.extent.y),
               "Out of bounds viewport bounds");
   
   new0 = rInfo.frustum[2] - ((cdV1 - dV1) * (heightV / F64(sgStoredViewport.extent.y)));
   new1 = rInfo.frustum[3] + ((dV0 - fdV0) * (heightV / F64(sgStoredViewport.extent.y)));
   rInfo.frustum[2] = new0;
   rInfo.frustum[3] = new1;

   rInfo.viewport.point.y  = S32(fdV0);
   rInfo.viewport.extent.y = S32(cdV1) - rInfo.viewport.point.y;
}

RectD convertToRectD(const F64 inResult[4])
{
   F64 minx = ((inResult[0] + 1.0f) / 2.0f) * (sgStoredFrustum[1] - sgStoredFrustum[0]) + sgStoredFrustum[0];
   F64 maxx = ((inResult[2] + 1.0f) / 2.0f) * (sgStoredFrustum[1] - sgStoredFrustum[0]) + sgStoredFrustum[0];

   F64 miny = ((inResult[1] + 1.0f) / 2.0f) * (sgStoredFrustum[3] - sgStoredFrustum[2]) + sgStoredFrustum[2];
   F64 maxy = ((inResult[3] + 1.0f) / 2.0f) * (sgStoredFrustum[3] - sgStoredFrustum[2]) + sgStoredFrustum[2];

   return RectD(minx, miny, (maxx - minx), (maxy - miny));
}

void convertToFrustum(PortalRenderInfo& zrInfo, const RectD& finalRect)
{
   zrInfo.frustum[0] = finalRect.point.x;                      // left
   zrInfo.frustum[1] = finalRect.point.x + finalRect.extent.x; // right
   zrInfo.frustum[2] = finalRect.point.y;                      // bottom
   zrInfo.frustum[3] = finalRect.point.y + finalRect.extent.y; // top

   fixupViewport(zrInfo);
}

} // namespace {}


//------------------------------------------------------------------------------
//-------------------------------------- IMPLEMENTATION
//
Interior::Interior()
{
   mMaterialList  = NULL;
   mWhite         = NULL;
   mWhiteRGB      = NULL;
   mLightFalloff  = NULL;

   // By default, no alarm state, no animated light states
   mHasAlarmState = false;

   mNumLightStateEntries = 0;
   mNumTriggerableLights = 0;

   mLMHandle = LM_HANDLE(-1);

   mPreppedForRender = false;;

   mSearchTag = 0;

   // Bind our vectors
   VECTOR_SET_ASSOCIATION(mPlanes);
   VECTOR_SET_ASSOCIATION(mPoints);
   VECTOR_SET_ASSOCIATION(mBSPNodes);
   VECTOR_SET_ASSOCIATION(mBSPSolidLeaves);
   VECTOR_SET_ASSOCIATION(mEnvironMaps);
   VECTOR_SET_ASSOCIATION(mEnvironFactors);
   VECTOR_SET_ASSOCIATION(mWindings);
   VECTOR_SET_ASSOCIATION(mTexGenEQs);
   VECTOR_SET_ASSOCIATION(mLMTexGenEQs);
   VECTOR_SET_ASSOCIATION(mWindingIndices);
   VECTOR_SET_ASSOCIATION(mSurfaces);
   VECTOR_SET_ASSOCIATION(mNullSurfaces);
   VECTOR_SET_ASSOCIATION(mSolidLeafSurfaces);
   VECTOR_SET_ASSOCIATION(mZones);
   VECTOR_SET_ASSOCIATION(mZonePlanes);
   VECTOR_SET_ASSOCIATION(mZoneSurfaces);
   VECTOR_SET_ASSOCIATION(mZonePortalList);
   VECTOR_SET_ASSOCIATION(mPortals);
   VECTOR_SET_ASSOCIATION(mSubObjects);
   VECTOR_SET_ASSOCIATION(mLightmaps);
   VECTOR_SET_ASSOCIATION(mLightmapKeep);
   VECTOR_SET_ASSOCIATION(mNormalLMapIndices);
   VECTOR_SET_ASSOCIATION(mAlarmLMapIndices);
   VECTOR_SET_ASSOCIATION(mAnimatedLights);
   VECTOR_SET_ASSOCIATION(mLightStates);
   VECTOR_SET_ASSOCIATION(mStateData);
   VECTOR_SET_ASSOCIATION(mStateDataBuffer);
   VECTOR_SET_ASSOCIATION(mNameBuffer);
   VECTOR_SET_ASSOCIATION(mConvexHulls);
   VECTOR_SET_ASSOCIATION(mConvexHullEmitStrings);
   VECTOR_SET_ASSOCIATION(mHullIndices);
   VECTOR_SET_ASSOCIATION(mHullEmitStringIndices);
   VECTOR_SET_ASSOCIATION(mHullSurfaceIndices);
   VECTOR_SET_ASSOCIATION(mHullPlaneIndices);
   VECTOR_SET_ASSOCIATION(mPolyListPlanes);
   VECTOR_SET_ASSOCIATION(mPolyListPoints);
   VECTOR_SET_ASSOCIATION(mPolyListStrings);
   VECTOR_SET_ASSOCIATION(mCoordBinIndices);

   VECTOR_SET_ASSOCIATION(mVehicleConvexHulls);
   VECTOR_SET_ASSOCIATION(mVehicleConvexHullEmitStrings);
   VECTOR_SET_ASSOCIATION(mVehicleHullIndices);
   VECTOR_SET_ASSOCIATION(mVehicleHullEmitStringIndices);
   VECTOR_SET_ASSOCIATION(mVehicleHullSurfaceIndices);
   VECTOR_SET_ASSOCIATION(mVehicleHullPlaneIndices);
   VECTOR_SET_ASSOCIATION(mVehiclePolyListPlanes);
   VECTOR_SET_ASSOCIATION(mVehiclePolyListPoints);
   VECTOR_SET_ASSOCIATION(mVehiclePolyListStrings);
   VECTOR_SET_ASSOCIATION(mVehiclePoints);
   VECTOR_SET_ASSOCIATION(mVehicleNullSurfaces);
   VECTOR_SET_ASSOCIATION(mVehiclePlanes);
}

Interior::~Interior()
{
   U32 i;
   delete mMaterialList;
   mMaterialList = NULL;
   delete mWhite;
   mWhite = NULL;
   delete mWhiteRGB;
   mWhiteRGB = NULL;
   delete mLightFalloff;
   mLightFalloff = NULL;

   // remove from lightmap manager
   if(mLMHandle != LM_HANDLE(-1))
      gInteriorLMManager.removeInterior(mLMHandle);

   for (i = 0; i < mLightmaps.size(); i++) {
      delete mLightmaps[i];
      mLightmaps[i] = NULL;
   }

   for (i = 0; i < mEnvironMaps.size(); i++) {
      delete mEnvironMaps[i];
      mEnvironMaps[i] = NULL;
   }

   for (i = 0; i < mSubObjects.size(); i++) {
      delete mSubObjects[i];
      mSubObjects[i] = NULL;
   }
}


//--------------------------------------------------------------------------
bool Interior::prepForRendering(const char* path)
{
   if (mPreppedForRender == true)
      return true;

   // Load the material list
   bool matListSuccess = mMaterialList->load(InteriorTexture, path, false);

   if(!matListSuccess)
   {
      matListSuccess = true;
      for(U32 i = 0; i < mSurfaces.size(); i++)
      {
         if(!mMaterialList->getMaterial(mSurfaces[i].textureIndex))
         {
            matListSuccess = false;
            break;
         }
      }
   }
   if(!matListSuccess)
      return false;

   // lightmap manager steals the lightmaps here...
   gInteriorLMManager.addInterior(mLMHandle, mLightmaps.size(), this);
   AssertFatal(!mLightmaps.size(), "Failed to process lightmaps");
   
   // And the environment maps...
   MaterialPropertyMap* pMatMap = static_cast<MaterialPropertyMap*>(Sim::findObject("MaterialPropertyMap"));

   mEnvironMaps.setSize(mMaterialList->getMaterialCount());
   mEnvironFactors.setSize(mMaterialList->getMaterialCount());
   mValidEnvironMaps = 0;
   for (U32 i = 0; i < mMaterialList->getMaterialCount(); i++) {
      mEnvironFactors[i] = 1.0f;

      const char* pName = mMaterialList->getMaterialName(i);
      const MaterialPropertyMap::MapEntry* pEntry = pMatMap->getMapEntry(pName);
      if (pEntry != NULL) {
         if (pEntry->environMapName != NULL) {
            mEnvironMaps[i]    = new TextureHandle(pEntry->environMapName, MeshTexture);
            mEnvironFactors[i] = pEntry->environMapFactor;
            mValidEnvironMaps += mEnvironMaps[i] != NULL ? 1 : 0;
         } else {
            mEnvironMaps[i] = NULL;
         }
      } else {
         mEnvironMaps[i] = NULL;
      }
   }
   mWhite        = new TextureHandle("common/lighting/whiteAlpha255", MeshTexture);
   mWhiteRGB     = new TextureHandle("common/lighting/whiteNoAlpha", MeshTexture);
   mLightFalloff = new TextureHandle("common/lighting/lightFalloffMono", BitmapTexture, true);

   mPreppedForRender = matListSuccess;

   // Setup the average texgen length...
   setupAveTexGenLength();
   
   return matListSuccess;
}


void Interior::setupAveTexGenLength()
{
   F32 len = 0;
   for (U32 i = 0; i < mSurfaces.size(); i++)
   {
      // We're going to assume that most textures don't have separate scales for
      //  x and y...
      F32 lenx = mTexGenEQs[mSurfaces[i].texGenIndex].planeX.len();
      len += F32((*mMaterialList)[mSurfaces[i].textureIndex].getWidth()) * lenx;
   }
   len /= F32(mSurfaces.size());
   mAveTexGenLength = len;
}


//--------------------------------------------------------------------------
bool Interior::prepRender(SceneState*    state,
                          S32            containingZone,
                          S32            baseZone,
                          U32            zoneOffset,
                          const MatrixF& OSToWS,
                          const Point3F& objScale,
                          const bool     modifyBaseState,
                          const bool     dontRestrictOutside,
                          const bool     flipClipPlanes)
{
   // Store off the viewport and frustum
   if (modifyBaseState || dontRestrictOutside ) {
      sgStoredViewport   = state->getBaseZoneState().viewport;
      sgStoredFrustum[0] = state->getBaseZoneState().frustum[0];
      sgStoredFrustum[1] = state->getBaseZoneState().frustum[1];
      sgStoredFrustum[2] = state->getBaseZoneState().frustum[2];
      sgStoredFrustum[3] = state->getBaseZoneState().frustum[3];
      sgStoredFrustum[4] = state->getNearPlane();
      sgStoredFrustum[5] = state->getFarPlane();
   } else {
      sgStoredViewport   = state->getZoneState(containingZone).viewport;
      sgStoredFrustum[0] = state->getZoneState(containingZone).frustum[0];
      sgStoredFrustum[1] = state->getZoneState(containingZone).frustum[1];
      sgStoredFrustum[2] = state->getZoneState(containingZone).frustum[2];
      sgStoredFrustum[3] = state->getZoneState(containingZone).frustum[3];
      sgStoredFrustum[4] = state->getNearPlane();
      sgStoredFrustum[5] = state->getFarPlane();
   }

   // Camera point is given by the state.  We need the projection matrix.
   //  OS->WS and scale are given.  This is an ugly way to do this...
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   dglSetFrustum(sgStoredFrustum[0], sgStoredFrustum[1],
                 sgStoredFrustum[2], sgStoredFrustum[3],
                 sgStoredFrustum[4], sgStoredFrustum[5]);
   dglGetProjection(&sgProjMatrix);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);

   MatrixF finalModelView;
   dglGetModelview(&finalModelView);
   finalModelView.mul(OSToWS);
   finalModelView.scale(Point3F(objScale.x, objScale.y, objScale.z));
   sgProjMatrix.mul(finalModelView);

   finalModelView.inverse();
   finalModelView.mulP(Point3F(0, 0, 0), &sgCamPoint);
   sgWSToOSMatrix = finalModelView;
   
   // do the zone traversal
   sgZoneRenderInfo.setSize(mZones.size());
   zoneTraversal(baseZone, flipClipPlanes);

   // Copy out the information for all zones but the outside zone.
   for (U32 i = 1; i < mZones.size(); i++) {
      AssertFatal(zoneOffset != 0xFFFFFFFF, "Error, this should never happen!");
      U32 globalIndex = i + zoneOffset - 1;

      SceneState::ZoneState& rState = state->getZoneStateNC(globalIndex);
      rState.render = sgZoneRenderInfo[i].render;
      if (rState.render) {
         rState.frustum[0] = sgZoneRenderInfo[i].frustum[0];
         rState.frustum[1] = sgZoneRenderInfo[i].frustum[1];
         rState.frustum[2] = sgZoneRenderInfo[i].frustum[2];
         rState.frustum[3] = sgZoneRenderInfo[i].frustum[3];
         rState.viewport   = sgZoneRenderInfo[i].viewport;
      }
   }

   if (modifyBaseState) {
      // Need to modify the state's baseZoneState based on the outside zone's (0),
      //  parameters.
      if (sgZoneRenderInfo[0].render == true) {
         SceneState::ZoneState& rState = state->getBaseZoneStateNC();
         rState.frustum[0] = sgZoneRenderInfo[0].frustum[0];
         rState.frustum[1] = sgZoneRenderInfo[0].frustum[1];
         rState.frustum[2] = sgZoneRenderInfo[0].frustum[2];
         rState.frustum[3] = sgZoneRenderInfo[0].frustum[3];
         rState.viewport   = sgZoneRenderInfo[0].viewport;
      } 
   }
   destroyZoneRectVectors();

   // If zone 0 is rendered, then we return true...
   return sgZoneRenderInfo[0].render;
}


void Interior::prepTempRender(SceneState*    state,
                              S32            containingZone,
                              S32            baseZone,
                              const MatrixF& OSToWS,
                              const Point3F& objScale,
                              const bool     flipClipPlanes)
{
   PROFILE_START(InteriorPrepTempRender);
   sgStoredViewport   = state->getZoneState(containingZone).viewport;
   sgStoredFrustum[0] = state->getZoneState(containingZone).frustum[0];
   sgStoredFrustum[1] = state->getZoneState(containingZone).frustum[1];
   sgStoredFrustum[2] = state->getZoneState(containingZone).frustum[2];
   sgStoredFrustum[3] = state->getZoneState(containingZone).frustum[3];
   sgStoredFrustum[4] = state->getNearPlane();
   sgStoredFrustum[5] = state->getFarPlane();

   // Camera point is given by the state.  We need the projection matrix.
   //  OS->WS and scale are given.  This is an ugly way to do this...
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   dglSetFrustum(sgStoredFrustum[0], sgStoredFrustum[1],
                 sgStoredFrustum[2], sgStoredFrustum[3],
                 sgStoredFrustum[4], sgStoredFrustum[5]);
   dglGetProjection(&sgProjMatrix);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);

   MatrixF finalModelView;
   dglGetModelview(&finalModelView);
   finalModelView.mul(OSToWS);
   finalModelView.scale(Point3F(objScale.x, objScale.y, objScale.z));
   sgProjMatrix.mul(finalModelView);

   finalModelView.inverse();
   finalModelView.mulP(Point3F(0, 0, 0), &sgCamPoint);
   sgWSToOSMatrix = finalModelView;
   
   // do the zone traversal
   sgZoneRenderInfo.setSize(mZones.size());
   zoneTraversal(baseZone, flipClipPlanes);
   destroyZoneRectVectors();
   PROFILE_END();
}


//------------------------------------------------------------------------------
S32 Interior::getZoneForPoint(const Point3F& rPoint) const
{
   const IBSPNode* pNode = &mBSPNodes[0];

   while (true) {
      F32 dist = getPlane(pNode->planeIndex).distToPlane(rPoint);
      if (planeIsFlipped(pNode->planeIndex))
         dist = -dist;

      U16 traverseIndex;
      if (dist >= 0)
         traverseIndex = pNode->frontIndex;
      else
         traverseIndex = pNode->backIndex;

      if (isBSPLeafIndex(traverseIndex)) {
         if (isBSPSolidLeaf(traverseIndex)) {
            return -1;
         } else {
            U16 zone = getBSPEmptyLeafZone(traverseIndex);
            if (zone == 0x0FFF)
               return -1;
            else
               return zone;
         }
      }

      pNode = &mBSPNodes[traverseIndex];
   }
}


//--------------------------------------------------------------------------
static void itrClipToPlane(Point3F* points, U32& rNumPoints, const PlaneF& rPlane)
{
   S32 start = -1;
   for (U32 i = 0; i < rNumPoints; i++) {
      if (rPlane.whichSide(points[i]) == PlaneF::Front) {
         start = i;
         break;
      }
   }

   // Nothing was in front of the plane...
   if (start == -1) {
      rNumPoints = 0;
      return;
   }

   static Point3F finalPoints[128];
   U32  numFinalPoints = 0;

   U32 baseStart = start;
   U32 end       = (start + 1) % rNumPoints;

   while (end != baseStart) {
      const Point3F& rStartPoint = points[start];
      const Point3F& rEndPoint   = points[end];

      PlaneF::Side fSide = rPlane.whichSide(rStartPoint);
      PlaneF::Side eSide = rPlane.whichSide(rEndPoint);

      S32 code = fSide * 3 + eSide;
      switch (code) {
         case 4:   // f f
         case 3:   // f o
         case 1:   // o f
         case 0:   // o o
            // No Clipping required
            finalPoints[numFinalPoints++] = points[start];
            start = end;
            end   = (end + 1) % rNumPoints;
            break;


         case 2: // f b
         { 
            // In this case, we emit the front point, Insert the intersection,
            //  and advancing to point to first point that is in front or on...
            //
            finalPoints[numFinalPoints++] = points[start];

            Point3F vector = rEndPoint - rStartPoint;
            F32 t        = -(rPlane.distToPlane(rStartPoint) / mDot(rPlane, vector));

            Point3F intersection = rStartPoint + (vector * t);
            finalPoints[numFinalPoints++] = intersection;

            U32 endSeek = (end + 1) % rNumPoints;
            while (rPlane.whichSide(points[endSeek]) == PlaneF::Back)
               endSeek = (endSeek + 1) % rNumPoints;

            end   = endSeek;
            start = (end + (rNumPoints - 1)) % rNumPoints;

            const Point3F& rNewStartPoint = points[start];
            const Point3F& rNewEndPoint   = points[end];

            vector = rNewEndPoint - rNewStartPoint;
            t = -(rPlane.distToPlane(rNewStartPoint) / mDot(rPlane, vector));
            
            intersection = rNewStartPoint + (vector * t);
            points[start] = intersection;
         }
         break;

         case -1: // o b
         {
            // In this case, we emit the front point, and advance to point to first
            //  point that is in front or on...
            //
            finalPoints[numFinalPoints++] = points[start];

            U32 endSeek = (end + 1) % rNumPoints;
            while (rPlane.whichSide(points[endSeek]) == PlaneF::Back)
               endSeek = (endSeek + 1) % rNumPoints;

            end   = endSeek;
            start = (end + (rNumPoints - 1)) % rNumPoints;

            const Point3F& rNewStartPoint = points[start];
            const Point3F& rNewEndPoint   = points[end];

            Point3F vector = rNewEndPoint - rNewStartPoint;
            F32 t        = -(rPlane.distToPlane(rNewStartPoint) / mDot(rPlane, vector));
            
            Point3F intersection = rNewStartPoint + (vector * t);
            points[start] = intersection;
         }
         break;

         case -2:  // b f
         case -3:  // b o
         case -4:  // b b
            // In the algorithm used here, this should never happen...
            AssertISV(false, "CSGPlane::clipWindingToPlaneFront: error in polygon clipper");
            break;

         default:
            AssertFatal(false, "CSGPlane::clipWindingToPlaneFront: bad outcode");
            break;
      }

   }
   
   // Emit the last point.
   finalPoints[numFinalPoints++] = points[start];
   AssertFatal(numFinalPoints >= 3, avar("Error, this shouldn't happen!  Invalid winding in itrClipToPlane: %d", numFinalPoints));

   // Copy the new rWinding, and we're set!
   //
   dMemcpy(points, finalPoints, numFinalPoints * sizeof(Point3F));
   rNumPoints = numFinalPoints;
   AssertISV(rNumPoints <= 128, "Increase maxWindingPoints.  Talk to DMoore");
}

bool Interior::projectClipAndBoundFan(U32 fanIndex, F64* pResult)
{
   const TriFan& rFan = mWindingIndices[fanIndex];

   static Point3F windingPoints[128];
   U32 numPoints = rFan.windingCount;
   U32 i;
   for (i = 0; i < numPoints; i++)
      windingPoints[i] = mPoints[mWindings[rFan.windingStart + i]].point;

   itrClipToPlane(windingPoints, numPoints, sgOSPlaneFar);
   if (numPoints != 0)
      itrClipToPlane(windingPoints, numPoints, sgOSPlaneXMin);
   if (numPoints != 0)
      itrClipToPlane(windingPoints, numPoints, sgOSPlaneXMax);
   if (numPoints != 0)
      itrClipToPlane(windingPoints, numPoints, sgOSPlaneYMin);
   if (numPoints != 0)
      itrClipToPlane(windingPoints, numPoints, sgOSPlaneYMax);

   if (numPoints == 0) {
      pResult[0] =
         pResult[1] =
         pResult[2] =
         pResult[3] = 0.0f;
      return false;
   }

   F32 minX = 1e10;
   F32 maxX = -1e10;
   F32 minY = 1e10;
   F32 maxY = -1e10;
   static Point4F projPoints[128];
   for (i = 0; i < numPoints; i++) {
      projPoints[i].set(windingPoints[i].x, windingPoints[i].y, windingPoints[i].z, 1.0);
      sgProjMatrix.mul(projPoints[i]);

      AssertFatal(projPoints[i].w != 0.0, "Error, that's bad!");
      projPoints[i].x /= projPoints[i].w;
      projPoints[i].y /= projPoints[i].w;

      if (projPoints[i].x < minX)
         minX = projPoints[i].x;
      if (projPoints[i].x > maxX)
         maxX = projPoints[i].x;
      if (projPoints[i].y < minY)
         minY = projPoints[i].y;
      if (projPoints[i].y > maxY)
         maxY = projPoints[i].y;
   }

   if (minX < -1.0f) minX = -1.0f;
   if (minY < -1.0f) minY = -1.0f;
   if (maxX > 1.0f)  maxX = 1.0f;
   if (maxY > 1.0f)  maxY = 1.0f;

   pResult[0] = minX;
   pResult[1] = minY;
   pResult[2] = maxX;
   pResult[3] = maxY;
   return true;
}

void Interior::createZoneRectVectors()
{
   sgZoneRects.setSize(mZones.size());
   for (U32 i = 0; i < mZones.size(); i++)
      sgZoneRects[i].active = false;
}

void Interior::destroyZoneRectVectors()
{
   
}

void Interior::traverseZone(const RectD* inputRects, const U32 numInputRects, U32 currZone, Vector<U32>& zoneStack)
{
   PROFILE_START(InteriorTraverseZone);
   // First, we push onto our rect list all the inputRects...
   insertZoneRects(sgZoneRects[currZone], inputRects, numInputRects);

   // A portal is a valid traversal if the camera point is on the
   //  same side of it's plane as the zone.  It must then pass the
   //  clip/project test.
   U32 i;
   const Zone& rZone = mZones[currZone];
   for (i = rZone.portalStart; i < U32(rZone.portalStart + rZone.portalCount); i++) {
      const Portal& rPortal = mPortals[mZonePortalList[i]];
      AssertFatal(U32(rPortal.zoneFront) == currZone || U32(rPortal.zoneBack) == currZone,
                  "Portal doesn't reference this zone?");

      S32 camSide = getPlane(rPortal.planeIndex).whichSide(sgCamPoint);
      if (planeIsFlipped(rPortal.planeIndex))
         camSide = -camSide;
      S32  zoneSide  = (U32(rPortal.zoneFront) == currZone) ? 1 : -1;
      U16 otherZone = (U32(rPortal.zoneFront) == currZone) ? rPortal.zoneBack : rPortal.zoneFront;

      // Make sure this isn't a free floating portal...
      if (otherZone == currZone)
         continue;

      // Make sure we haven't encountered this zone already in this traversal
      bool onStack = false;
      for (U32 i = 0; i < zoneStack.size(); i++) {
         if (otherZone == zoneStack[i]) {
            onStack = true;
            break;
         }
      }
      if (onStack == true)
         continue;

      if (camSide == zoneSide) {
         // Can traverse.  Note: special case PlaneF::On
         //  here to prevent possible w == 0 problems and infinite recursion
//          Vector<RectD> newRects;
//          VECTOR_SET_ASSOCIATION(newRects);

         // We're abusing the heck out of the allocator here.
         U32 waterMark = FrameAllocator::getWaterMark();
         RectD* newRects = (RectD*)FrameAllocator::alloc(1);
         U32 numNewRects = 0;
         
         for (S32 j = 0; j < rPortal.triFanCount; j++) {
            F64 result[4];
            if (projectClipAndBoundFan(rPortal.triFanStart + j, result)) {
               // Have a good rect from this.
               RectD possible = convertToRectD(result);

               for (U32 k = 0; k < numInputRects; k++) {
                  RectD copy = possible;
                  if (copy.intersect(inputRects[k]))
                     newRects[numNewRects++] = copy;
               }
            }
         }
         
         if (numNewRects != 0) {
            FrameAllocator::alloc((sizeof(RectD) * numNewRects) - 1);

            U32 prevStackSize = zoneStack.size();
            zoneStack.push_back(currZone);
            traverseZone(newRects, numNewRects, otherZone, zoneStack);
            zoneStack.pop_back();
            AssertFatal(zoneStack.size() == prevStackSize, "Error, stack size changed!");
         }
         FrameAllocator::setWaterMark(waterMark);
      }
      else if (camSide == PlaneF::On) {
         U32 waterMark = FrameAllocator::getWaterMark();
         RectD* newRects = (RectD*)FrameAllocator::alloc(numInputRects * sizeof(RectD));
         dMemcpy(newRects, inputRects, sizeof(RectD) * numInputRects);

         U32 prevStackSize = zoneStack.size();
         zoneStack.push_back(currZone);
         traverseZone(newRects, numInputRects, otherZone, zoneStack);
         zoneStack.pop_back();
         AssertFatal(zoneStack.size() == prevStackSize, "Error, stack size changed!");
         FrameAllocator::setWaterMark(waterMark);
      }
   }
   PROFILE_END();
}

void Interior::zoneTraversal(S32 baseZone, const bool flipClip)
{
   PROFILE_START(InteriorZoneTraversal);
   // If we're in solid, render everything...
   if (baseZone == -1) {
      for (U32 i = 0; i < mZones.size(); i++) {
         sgZoneRenderInfo[i].render = true;

         sgZoneRenderInfo[i].frustum[0] = sgStoredFrustum[0];
         sgZoneRenderInfo[i].frustum[1] = sgStoredFrustum[1];
         sgZoneRenderInfo[i].frustum[2] = sgStoredFrustum[2];
         sgZoneRenderInfo[i].frustum[3] = sgStoredFrustum[3];
         sgZoneRenderInfo[i].viewport   = sgStoredViewport;
      }
      PROFILE_END();
      return;
   }

   // Otherwise, we're going to have to do some work...
   createZoneRectVectors();
   U32 i;
   for (i = 0; i < mZones.size(); i++)
      sgZoneRenderInfo[i].render = false;

   // Create the object space clipping planes...
   sgComputeOSFrustumPlanes(sgStoredFrustum,
                            sgWSToOSMatrix,
                            sgCamPoint,
                            sgOSPlaneFar,
                            sgOSPlaneXMin,
                            sgOSPlaneXMax,
                            sgOSPlaneYMin,
                            sgOSPlaneYMax);
                            
   if (flipClip == true) {
      sgOSPlaneXMin.neg();
      sgOSPlaneXMax.neg();
      sgOSPlaneYMin.neg();
      sgOSPlaneYMax.neg();
   }
   // First, the current zone gets the full clipRect, and marked as rendering...
   static const F64 fullResult[4] = { -1, -1, 1, 1 };
   
   static Vector<U32> zoneStack;
   zoneStack.clear();
   VECTOR_SET_ASSOCIATION(zoneStack);

   RectD baseRect = convertToRectD(fullResult);
   traverseZone(&baseRect, 1, baseZone, zoneStack);

   for (i = 0; i < mZones.size(); i++) {
      if (sgZoneRects[i].active == true) {
         sgZoneRenderInfo[i].render = true;
         convertToFrustum(sgZoneRenderInfo[i], sgZoneRects[i].rect);
      }
   }
   PROFILE_END();
}


void mergeSurfaceVectors(const U16* from0,
                         const U32  size0,
                         const U16* from1,
                         const U32  size1,
                         U16*       output,
                         U32*       outputSize)
{
   U32 pos0 = 0;
   U32 pos1 = 0;
   U32 outputCount = 0;
   while (pos0 < size0 && pos1 < size1) {
      if (from0[pos0] < from1[pos1]) {
         output[outputCount++] = from0[pos0++];
      } else if (from0[pos0] == from1[pos1]) {
         // Equal, output one, and inc both counts
         output[outputCount++] = from0[pos0++];
         pos1++;
      } else {
         output[outputCount++] = from1[pos1++];
      }
   }
   AssertFatal(pos0 == size0 || pos1 == size1, "Error, one of these must have reached the end!");

   // Copy the dregs...
   if (pos0 != size0) {
      dMemcpy(&output[outputCount], &from0[pos0], sizeof(U16) * (size0 - pos0));
      outputCount += size0 - pos0;
   } else if (pos1 != size1) {
      dMemcpy(&output[outputCount], &from1[pos1], sizeof(U16) * (size1 - pos1));
      outputCount += size1 - pos1;
   }

   *outputSize = outputCount;
}

struct ItrMergeStruct
{
   U16* array;
   U32  size;
};

bool Interior::useFogCoord()
{
   if (dglDoesSupportFogCoord() && smUseTexturedFog == false)
      return true;
   return false;
}

void Interior::setupActivePolyList(ZoneVisDeterminer& zoneDeterminer,
                                   SceneState*        state,
                                   const Point3F&     rPoint,
                                   const Point3F&     osCamVector,
                                   const Point3F&     osZVec,
                                   const F32          worldZ,
                                   const Point3F&     scale)
{
   PROFILE_START(InteriorSetupActivePolyList);
   U32 i;
   // Here's the deal.  We loop through each of the zones, and create a merged master
   //  list of polygons that are the union of all the zones render sets.  While we're
   //  doing this, we'll be setting up each zone's list of planes.  I've got these
   //  processes separated out for now, but they could be merged.  After we have the
   //  master list of polys, and the list of active planes, we'll copy the list
   //  (culling the backfaces) into the ActivePolyList.

   // There's some trickiness here.  We use the high bit of this U16 to test against the
   //  flip bit in the surfaces planeindex.
   U16* planeSides = (U16*)FrameAllocator::alloc(sizeof(U16) * mPlanes.size());
   
   Point3F worldP = state->getCameraPosition();

   // We'll never need more than twice the number of zones for merging...
   ItrMergeStruct* mergeArray = (ItrMergeStruct*)FrameAllocator::alloc((mZones.size() * 2) * sizeof(ItrMergeStruct));
   U32 numMergeStructs = 0;
   
   PROFILE_START(ISAPL_Merge);
   for (i = 0; i < mZones.size(); i++)
   {
      if (zoneDeterminer.isZoneVisible(i) == false)
         continue;

      // Setup the plane directionals
      for (U32 j = mZones[i].planeStart; j < mZones[i].planeStart + mZones[i].planeCount; j++) {
         if (getPlane(mZonePlanes[j]).distToPlane(rPoint) >= 0.0f)
            planeSides[mZonePlanes[j]] = 0x8000;
         else
            planeSides[mZonePlanes[j]] = 0x0000;
      }

      // Create a merge struct for this zone
      ItrMergeStruct& rMerge = mergeArray[numMergeStructs++];
      rMerge.array = &mZoneSurfaces[mZones[i].surfaceStart];
      rMerge.size  = mZones[i].surfaceCount;
   }
   AssertFatal(numMergeStructs > 0, "Error, no rendered zones, big problem.");

   // Merge the arrays into the final version
   U32 finalArray = 0xFFFFFFFF;
   {
      U32 begin  = 0;
      U32 end    = numMergeStructs;
      while ((end - begin) > 1)
      {
         U32 newEnd = end;

         U32 i;
         for (i = begin; (i + 1) < end; i += 2)
         {
            ItrMergeStruct& rMerge0 = mergeArray[i];
            ItrMergeStruct& rMerge1 = mergeArray[i+1];

            // Create the new structure to merge into
            ItrMergeStruct& rMergeOut = mergeArray[newEnd++];
            rMergeOut.array = (U16*)FrameAllocator::alloc(mSurfaces.size() * sizeof(U16));

            mergeSurfaceVectors(rMerge0.array, rMerge0.size,
                                rMerge1.array, rMerge1.size,
                                rMergeOut.array,
                                &rMergeOut.size);
         }

         begin = i;
         end   = newEnd;
      }

      finalArray = begin;
   }
   AssertFatal(finalArray < mZones.size() * 2, "Error, final array out of bounds!");

   U16* output     = mergeArray[finalArray].array;
   U32 outputCount = mergeArray[finalArray].size;
   PROFILE_END();
   
   // Before we go and fog this object, we'll test the points of our bounding box.  If
   //  they are all unfogged, then we have no need to do any fogging.  If there are
   //  all fogged though, we cannot turn off rendering of the object, as it's possible
   //  that they extend into fog planes.
   Point3F temp;

   PlaneF distPlane;
   F32    distOffset;
   // Setup the dist plane
   Point3F closest = getBoundingBox().getClosestPoint(rPoint);
   Point3F n       = closest - rPoint;
   n.convolve(scale);
   distOffset      = n.len();
   if (distOffset != 0)
   {
      distPlane.set(closest, n);
   }
   else
   {
      // Oops, we're inside the bounding box.  distnormal is the view vector in object space
      distPlane.set(closest, osCamVector);
   }
   distPlane.x /= scale.x;
   distPlane.y /= scale.y;
   distPlane.z /= scale.z;
   
   F32 maxFog = -1;
   Point3F fp[2];
   fp[0] = getBoundingBox().min;
   fp[1] = getBoundingBox().max;
   for (i = 0; i < 8; i++) {
      Point3F test;

      if (i & 0x1) test.x = fp[0].x;
      else         test.x = fp[1].x;
      if (i & 0x2) test.y = fp[0].y;
      else         test.y = fp[1].y;
      if (i & 0x4) test.z = fp[0].z;
      else         test.z = fp[1].z;

      F32 hazeVal  = state->getHazeAndFog(mFabs(distPlane.distToPlane(test)) + distOffset,
                                          (mDot(test, osZVec) + worldZ) - worldP.z);
      if (hazeVal > maxFog)
         maxFog = hazeVal;
   }

   if (maxFog < 1.0/255.0f) {
      // Unfogged.  We can turn off this part of the setup...
      sgFogActive = false;
   } else {
      // Sigh.  Gotta do it
      sgFogActive = true;
   }
   
   PROFILE_START(ISAPL_Setup);

   // Point setup
   U8* activePoints = (U8*)FrameAllocator::alloc(mPoints.size());
   dMemset(activePoints, 0, mPoints.size());

   sgActivePolyList  = (U16*)FrameAllocator::alloc(mSurfaces.size() * sizeof(16));
   sgEnvironPolyList = (U16*)FrameAllocator::alloc(mSurfaces.size() * sizeof(16));
   sgFogPolyList     = (U16*)FrameAllocator::alloc(mSurfaces.size() * sizeof(16));
   sgFogTexCoords    = (Point2F*)FrameAllocator::alloc(mPoints.size() * sizeof(Point2F));
   sgActivePolyListSize  = 0;
   sgEnvironPolyListSize = 0;
   sgFogPolyListSize     = 0;

   // Move conditionals outside to reduce loop branches
   bool environmentActive = (dglDoesSupportARBMultitexture() &&
                             smRenderEnvironmentMaps &&
                             mValidEnvironMaps != 0);

   // Totally bare
   if (sgFogActive && useFogCoord())
   {
      if (environmentActive) {
         // Environ, fc fog
         for (i = 0; i < outputCount; i++) {
            const Surface& rSurface = mSurfaces[output[i]];

            // Not back faced?  Add it to the list
            if (((planeSides[getPlaneIndex(rSurface.planeIndex)] ^ rSurface.planeIndex) & 0x8000) == 0)
               continue;
            sgActivePolyList[sgActivePolyListSize++] = output[i];

            if (mEnvironMaps[rSurface.textureIndex] != NULL)
               sgEnvironPolyList[sgEnvironPolyListSize++] = output[i];

            for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
               U32 index = mWindings[j];
               if (activePoints[index] == 0) {
                  activePoints[index] = 1;

                  mPoints[index].fogCoord = state->getHazeAndFog(mFabs(distPlane.distToPlane(mPoints[index].point)) + distOffset,
                                                                 (mDot(mPoints[index].point, osZVec) + worldZ) - worldP.z);
//                   mPoints[index].fogCoord =
//                      gClientSceneGraph->getFogCoord(distPlane.distToPlane(mPoints[index].point) + distOffset,
//                                                     worldZ + mDot(mPoints[index].point, osZVec));
                  AssertFatal(mPoints[index].fogCoord >= 0.0f, "Error, neg fog coord!");
               }
            }
         }
      }
      else {
         // No environ, FC fog
         for (i = 0; i < outputCount; i++) {
            const Surface& rSurface = mSurfaces[output[i]];
            // Not back faced?  Add it to the list
            if (((planeSides[getPlaneIndex(rSurface.planeIndex)] ^ rSurface.planeIndex) & 0x8000) == 0)
               continue;
            sgActivePolyList[sgActivePolyListSize++] = output[i];

            for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
               U32 index = mWindings[j];
               if (activePoints[index] == 0) {
                  activePoints[index] = 1;

                  mPoints[index].fogCoord = state->getHazeAndFog(mFabs(distPlane.distToPlane(mPoints[index].point)) + distOffset,
                                                                 (mDot(mPoints[index].point, osZVec) + worldZ) - worldP.z);
//                   mPoints[index].fogCoord =
//                      gClientSceneGraph->getFogCoord(distPlane.distToPlane(mPoints[index].point) + distOffset,
//                                                     worldZ + mDot(mPoints[index].point, osZVec));
                  AssertFatal(mPoints[index].fogCoord >= 0.0f, "Error, neg fog coord!");
               }
            }
         }
      }
   }
   else if (sgFogActive) {
      // Environment, textured fog
      if (environmentActive) {
         for (i = 0; i < outputCount; i++) {
            const Surface& rSurface = mSurfaces[output[i]];
            // Not back faced?  Add it to the list
            if (((planeSides[getPlaneIndex(rSurface.planeIndex)] ^ rSurface.planeIndex) & 0x8000) == 0)
               continue;
            sgActivePolyList[sgActivePolyListSize++] = output[i];

            if (mEnvironMaps[rSurface.textureIndex] != NULL)
               sgEnvironPolyList[sgEnvironPolyListSize++] = output[i];

            // Fog the unfogged points...
            for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
               U32 pIndex = mWindings[j];

               if (activePoints[pIndex] == 0) {
                  activePoints[pIndex] = 1;

                  // fog.
                  gClientSceneGraph->getFogCoordPair(distPlane.distToPlane(mPoints[pIndex].point) + distOffset,
                                                     worldZ + mDot(mPoints[pIndex].point, osZVec),
                                                     sgFogTexCoords[pIndex].x,
                                                     sgFogTexCoords[pIndex].y);
               }
            }
         }
      } else {
         // no environment, textured fog
         for (i = 0; i < outputCount; i++) {
            const Surface& rSurface = mSurfaces[output[i]];
            // Not back faced?  Add it to the list
            if (((planeSides[getPlaneIndex(rSurface.planeIndex)] ^ rSurface.planeIndex) & 0x8000) == 0)
               continue;
            sgActivePolyList[sgActivePolyListSize++] = output[i];

            // Fog the unfogged points...
            for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
               U32 pIndex = mWindings[j];

               if (activePoints[pIndex] == 0) {
                  activePoints[pIndex] = 1;

                  // fog.
                  gClientSceneGraph->getFogCoordPair(distPlane.distToPlane(mPoints[pIndex].point) + distOffset,
                                                     worldZ + mDot(mPoints[pIndex].point, osZVec),
                                                     sgFogTexCoords[pIndex].x,
                                                     sgFogTexCoords[pIndex].y);
               }
            }
         }
      }
   } else {
      // No Fog
      if (environmentActive) {
         // Environ
         for (i = 0; i < outputCount; i++) {
            const Surface& rSurface = mSurfaces[output[i]];
            // Not back faced?  Add it to the list
            if (((planeSides[getPlaneIndex(rSurface.planeIndex)] ^ rSurface.planeIndex) & 0x8000) == 0)
               continue;
            sgActivePolyList[sgActivePolyListSize++] = output[i];

            if (mEnvironMaps[mSurfaces[output[i]].textureIndex] != NULL)
               sgEnvironPolyList[sgEnvironPolyListSize++] = output[i];
         }
      }
      else
      {
         for (i = 0; i < outputCount; i++) {
            const Surface& rSurface = mSurfaces[output[i]];
            // Not back faced?  Add it to the list
            if (((planeSides[getPlaneIndex(rSurface.planeIndex)] ^ rSurface.planeIndex) & 0x8000) == 0)
               continue;
            sgActivePolyList[sgActivePolyListSize++] = output[i];
         }
      }
   }
   PROFILE_END();
   
   PROFILE_END();
}


void Interior::scopeZone(const U32           currZone,
                         bool*               interiorScopingState,
                         const Point3F&      interiorRoot,
                         Vector<U32>&        zoneStack,
                         Vector<PlaneF>&     planeStack,
                         Vector<PlaneRange>& planeRangeStack)
{
   // First, if we're here, this zone is scoped...
   interiorScopingState[currZone] = true;

   // A portal is a valid traversal if the camera point is on the
   //  same side of it's plane as the zone.  It must then pass the
   //  clip/project test.
   const Zone& rZone = mZones[currZone];
   for (S32 i = rZone.portalStart; i < U32(rZone.portalStart + rZone.portalCount); i++) {
      const Portal& rPortal = mPortals[mZonePortalList[i]];
      AssertFatal(U32(rPortal.zoneFront) == currZone || U32(rPortal.zoneBack) == currZone,
                  "Portal doesn't reference this zone?");

      S32 camSide = getPlane(rPortal.planeIndex).whichSide(interiorRoot);
      if (planeIsFlipped(rPortal.planeIndex))
         camSide = -camSide;
      S32  zoneSide = (U32(rPortal.zoneFront) == currZone) ? 1 : -1;
      U16 otherZone = (U32(rPortal.zoneFront) == currZone) ? rPortal.zoneBack : rPortal.zoneFront;

      // Make sure this isn't a free floating portal...
      if (otherZone == currZone)
         continue;

      // Make sure we haven't encountered this zone already in this traversal
      bool onStack = false;
      for (U32 i = 0; i < zoneStack.size(); i++) {
         if (otherZone == zoneStack[i]) {
            onStack = true;
            break;
         }
      }
      if (onStack == true)
         continue;

      if (camSide == zoneSide) {
         // Can traverse.  Note: special case PlaneF::On

         // push ourselves onto the zonestack
         zoneStack.push_back(currZone);

         for (S32 j = 0; j < rPortal.triFanCount; j++) {
            U32 k;
            const TriFan& rFan = mWindingIndices[rPortal.triFanStart + j];

            // Create the winding for this portal
            //
            static Point3F windingPoints[128];
            U32 numPoints = rFan.windingCount;
            for (k = 0; k < numPoints; k++)
               windingPoints[k] = mPoints[mWindings[rFan.windingStart + k]].point;

            // Clip the winding against the planes in the current range
            for (k = 0; k < planeRangeStack.last().count; k++) {
               const PlaneF& rPlane = planeStack[planeRangeStack.last().start + k];
               itrClipToPlane(windingPoints, numPoints, rPlane);
               if (numPoints == 0)
                  break;
            }
            // If the winding is now empty, bail
            //
            if (numPoints == 0)
               continue;

            // create new planes and range from the winding that remains.  There is one
            //  plane for each winding point.
            //
            planeRangeStack.increment();
            planeRangeStack.last().start = planeStack.size();
            planeRangeStack.last().count = numPoints;
            planeStack.increment(numPoints);
            for (k = 0; k < numPoints; k++) {
               U32 s = k;
               U32 e = (k + 1) % numPoints;

               planeStack[planeRangeStack.last().start + k].set(interiorRoot,
                                                                windingPoints[e],
                                                                windingPoints[s]);
               if (zoneSide == -1)
                  planeStack[planeRangeStack.last().start + k].neg();
            }

            // traverse into new zone
            scopeZone(otherZone,
                      interiorScopingState,
                      interiorRoot,
                      zoneStack,
                      planeStack,
                      planeRangeStack);

            // pop off range, planes
            planeStack.decrement(planeRangeStack.last().count);
            planeRangeStack.pop_back();
         }

         // Pop ourselves off the stack
         zoneStack.pop_back();
      }
      else if (camSide == PlaneF::On) {
         // Special case.  Have to drill down with the same frustums...
         PlaneRange copy = planeRangeStack.last();
         copy.start += copy.count;
         planeRangeStack.push_back(copy);
         planeStack.increment(copy.count);
         for (U32 i = 0; i < copy.count; i++)
            planeStack[copy.start + i] = planeStack[copy.start + i - copy.count];
         
         zoneStack.push_back(currZone);
         scopeZone(otherZone, interiorScopingState, interiorRoot, zoneStack, planeStack, planeRangeStack);
         planeStack.decrement(copy.count);
         planeRangeStack.decrement();
      }
   }
}


bool Interior::scopeZones(const S32            baseZone,
                          const Point3F&       interiorRoot,
                          bool*                interiorScopingState)
{
   // If we are in solid, scope everything, and return ourselves as continuing out...
   if (baseZone == -1) {
      for (U32 i = 0; i < mZones.size(); i++)
         interiorScopingState[i] = true;
      return true;
   }

   Vector<U32>        zoneStack(64, __FILE__, __LINE__);
   Vector<PlaneF>     planeStack(1024, __FILE__, __LINE__);
   Vector<PlaneRange> planeRangeStack(64, __FILE__, __LINE__);
   
   PlaneRange initial;
   initial.start = 0;
   initial.count = 0;
   planeRangeStack.push_back(initial);
   scopeZone(baseZone, interiorScopingState, interiorRoot, zoneStack, planeStack, planeRangeStack);

   return interiorScopingState[0];
}


// Remove any collision hulls, interval trees, etc...
//
void Interior::purgeLODData()
{
   mConvexHulls.clear();
   mHullIndices.clear();
   mHullEmitStringIndices.clear();
   mHullSurfaceIndices.clear();
   mCoordBinIndices.clear();
   mConvexHullEmitStrings.clear();
   for (U32 i = 0; i < NumCoordBins * NumCoordBins; i++) {
      mCoordBins[i].binStart = 0;
      mCoordBins[i].binCount = 0;
   }
}


struct TempProcSurface {
   U32 numPoints;
   U32 pointIndices[32];
   U16 planeIndex;
   U8  mask;
};

struct PlaneGrouping {
   U32 numPlanes;
   U16 planeIndices[32];
   U8  mask;
};


//--------------------------------------------------------------------------
void Interior::processHullPolyLists()
{
   Vector<U16>             planeIndices(256, __FILE__, __LINE__);
   Vector<U32>             pointIndices(256, __FILE__, __LINE__);
   Vector<U8>              pointMasks(256, __FILE__, __LINE__);
   Vector<U8>              planeMasks(256, __FILE__, __LINE__);
   Vector<TempProcSurface> tempSurfaces(128, __FILE__, __LINE__);
   Vector<PlaneGrouping>   planeGroups(32, __FILE__, __LINE__);
   
   // Reserve space in the vectors
   {
      mPolyListStrings.setSize(0);
      mPolyListStrings.reserve(128 << 10);

      mPolyListPoints.setSize(0);
      mPolyListPoints.reserve(32 << 10);

      mPolyListPlanes.setSize(0);
      mPolyListPlanes.reserve(16 << 10);
   }

   for (U32 i = 0; i < mConvexHulls.size(); i++) {
      U32 j, k, l, m;

      ConvexHull& rHull = mConvexHulls[i];

      planeIndices.setSize(0);
      pointIndices.setSize(0);
      tempSurfaces.setSize(0);

      // Extract all the surfaces from this hull into our temporary processing format
      {
         for (j = 0; j < rHull.surfaceCount; j++) {
            tempSurfaces.increment();
            TempProcSurface& temp = tempSurfaces.last();

            U32 surfaceIndex = mHullSurfaceIndices[j + rHull.surfaceStart];
            if (isNullSurfaceIndex(surfaceIndex)) {
               const NullSurface& rSurface = mNullSurfaces[getNullSurfaceIndex(surfaceIndex)];

               temp.planeIndex = rSurface.planeIndex;
               temp.numPoints  = rSurface.windingCount;
               for (k = 0; k < rSurface.windingCount; k++)
                  temp.pointIndices[k] = mWindings[rSurface.windingStart + k];
            } else {
               const Surface& rSurface = mSurfaces[surfaceIndex];

               temp.planeIndex = rSurface.planeIndex;
               collisionFanFromSurface(rSurface, temp.pointIndices, &temp.numPoints);
            }
         }
      }

      // First order of business: extract all unique planes and points from
      //  the list of surfaces...
      {
         for (j = 0; j < tempSurfaces.size(); j++) {
            const TempProcSurface& rSurface = tempSurfaces[j];

            bool found = false;
            for (k = 0; k < planeIndices.size() && !found; k++) {
               if (rSurface.planeIndex == planeIndices[k])
                  found = true;
            }
            if (!found)
               planeIndices.push_back(rSurface.planeIndex);

            for (k = 0; k < rSurface.numPoints; k++) {
               found = false;
               for (l = 0; l < pointIndices.size(); l++) {
                  if (pointIndices[l] == rSurface.pointIndices[k])
                     found = true;
               }
               if (!found)
                  pointIndices.push_back(rSurface.pointIndices[k]);
            }
         }
      }

      // Now that we have all the unique points and planes, remap the surfaces in
      //  terms of the offsets into the unique point list...
      {
         for (j = 0; j < tempSurfaces.size(); j++) {
            TempProcSurface& rSurface = tempSurfaces[j];

            // Points
            for (k = 0; k < rSurface.numPoints; k++) {
               bool found = false;
               for (l = 0; l < pointIndices.size(); l++) {
                  if (pointIndices[l] == rSurface.pointIndices[k]) {
                     rSurface.pointIndices[k] = l;
                     found = true;
                     break;
                  }
               }
               AssertISV(found, "Error remapping point indices in interior collision processing");
            }
         }
      }

      // Ok, at this point, we have a list of unique points, unique planes, and the
      //  surfaces all remapped in those terms.  We need to check our error conditions
      //  that will make sure that we can properly encode this hull:
      {
         AssertISV(planeIndices.size() < 256,   "Error, > 256 planes on an interior hull");
         AssertISV(pointIndices.size() < 63356, "Error, > 65536 points on an interior hull");
         AssertISV(tempSurfaces.size() < 256,   "Error, > 256 surfaces on an interior hull");
      }

      // Now we group the planes together, and merge the closest groups until we're left
      //  with <= 8 groups
      {
         planeGroups.setSize(planeIndices.size());
         for (j = 0; j < planeIndices.size(); j++) {
            planeGroups[j].numPlanes       = 1;
            planeGroups[j].planeIndices[0] = planeIndices[j];
         }

         while (planeGroups.size() > 8) {
            // Find the two closest groups.  If mdp(i, j) is the value of the
            //  largest pairwise dot product that can be computed from the vectors
            //  of group i, and group j, then the closest group pair is the one
            //  with the smallest value of mdp.
            F32 currmin = 2;
            S32 firstGroup  = -1;
            S32 secondGroup = -1;

            for (j = 0; j < planeGroups.size(); j++) {
               PlaneGrouping& first = planeGroups[j];
               for (k = j + 1; k < planeGroups.size(); k++) {
                  PlaneGrouping& second = planeGroups[k];

                  F32 max = -2;
                  for (l = 0; l < first.numPlanes; l++) {
                     for (m = 0; m < second.numPlanes; m++) {
                        Point3F firstNormal = getPlane(first.planeIndices[l]);
                        if (planeIsFlipped(first.planeIndices[l]))
                           firstNormal.neg();
                        Point3F secondNormal = getPlane(second.planeIndices[m]);
                        if (planeIsFlipped(second.planeIndices[m]))
                           secondNormal.neg();

                        F32 dot = mDot(firstNormal, secondNormal);
                        if (dot > max)
                           max = dot;
                     }
                  }

                  if (max < currmin) {
                     currmin = max;
                     firstGroup  = j;
                     secondGroup = k;
                  }
               }
            }
            AssertFatal(firstGroup != -1 && secondGroup != -1, "Error, unable to find a suitable pairing?");

            // Merge first and second
            PlaneGrouping& to   = planeGroups[firstGroup];
            PlaneGrouping& from = planeGroups[secondGroup];
            while (from.numPlanes != 0) {
               to.planeIndices[to.numPlanes++] = from.planeIndices[from.numPlanes - 1];
               from.numPlanes--;
            }

            // And remove the merged group
            planeGroups.erase(secondGroup);
         }
         AssertFatal(planeGroups.size() <= 8, "Error, too many plane groupings!");


         // Assign a mask to each of the plane groupings
         for (j = 0; j < planeGroups.size(); j++)
            planeGroups[j].mask = (1 << j);
      }

      // Now, assign the mask to each of the temp polys
      {
         for (j = 0; j < tempSurfaces.size(); j++) {
            bool assigned = false;
            for (k = 0; k < planeGroups.size() && !assigned; k++) {
               for (l = 0; l < planeGroups[k].numPlanes; l++) {
                  if (planeGroups[k].planeIndices[l] == tempSurfaces[j].planeIndex) {
                     tempSurfaces[j].mask = planeGroups[k].mask;
                     assigned = true;
                     break;
                  }
               }
            }
            AssertFatal(assigned, "Error, missed a plane somewhere in the hull poly list!");
         }
      }

      // Copy the appropriate group mask to the plane masks
      {
         planeMasks.setSize(planeIndices.size());
         dMemset(planeMasks.address(), 0, planeMasks.size() * sizeof(U8));

         for (j = 0; j < planeIndices.size(); j++) {
            bool found = false;
            for (k = 0; k < planeGroups.size() && !found; k++) {
               for (l = 0; l < planeGroups[k].numPlanes; l++) {
                  if (planeGroups[k].planeIndices[l] == planeIndices[j]) {
                     planeMasks[j] = planeGroups[k].mask;
                     found = true;
                     break;
                  }
               }
            }
            AssertFatal(planeMasks[j] != 0, "Error, missing mask for plane!");
         }
      }

      // And whip through the points, constructing the total mask for that point
      {
         pointMasks.setSize(pointIndices.size());
         dMemset(pointMasks.address(), 0, pointMasks.size() * sizeof(U8));

         for (j = 0; j < pointIndices.size(); j++) {
            for (k = 0; k < tempSurfaces.size(); k++) {
               for (l = 0; l < tempSurfaces[k].numPoints; l++) {
                  if (tempSurfaces[k].pointIndices[l] == j) {
                     pointMasks[j] |= tempSurfaces[k].mask;
                     break;
                  }
               }
            }
            AssertFatal(pointMasks[j] != 0, "Error, point must exist in at least one surface!");
         }
      }

      // Create the emit strings, and we're done!
      {
         // Set the range of planes
         rHull.polyListPlaneStart = mPolyListPlanes.size();
         mPolyListPlanes.setSize(rHull.polyListPlaneStart + planeIndices.size());
         for (j = 0; j < planeIndices.size(); j++)
            mPolyListPlanes[j + rHull.polyListPlaneStart] = planeIndices[j];

         // Set the range of points
         rHull.polyListPointStart = mPolyListPoints.size();
         mPolyListPoints.setSize(rHull.polyListPointStart + pointIndices.size());
         for (j = 0; j < pointIndices.size(); j++)
            mPolyListPoints[j + rHull.polyListPointStart] = pointIndices[j];

         // Now the emit string.  The emit string goes like: (all fields are bytes)
         //  NumPlanes (PLMask) * NumPlanes
         //  NumPointsHi NumPointsLo (PtMask) * NumPoints
         //  NumSurfaces
         //   (NumPoints SurfaceMask PlOffset (PtOffsetHi PtOffsetLo) * NumPoints) * NumSurfaces
         //
         U32 stringLen  = 1 + planeIndices.size();
         stringLen     += 2 + pointIndices.size();
         stringLen     += 1;
         for (j = 0; j < tempSurfaces.size(); j++)
            stringLen += 1 + 1 + 1 + (tempSurfaces[j].numPoints * 2);

         rHull.polyListStringStart = mPolyListStrings.size();
         mPolyListStrings.setSize(rHull.polyListStringStart + stringLen);

         U8* pString = &mPolyListStrings[rHull.polyListStringStart];
         U32 currPos = 0;

         // Planes
         pString[currPos++] = planeIndices.size();
         for (j = 0; j < planeIndices.size(); j++)
            pString[currPos++] = planeMasks[j];

         // Points
         pString[currPos++] = (pointIndices.size() >> 8) & 0xFF;
         pString[currPos++] = (pointIndices.size() >> 0) & 0xFF;
         for (j = 0; j < pointIndices.size(); j++)
            pString[currPos++] = pointMasks[j];

         // Surfaces
         pString[currPos++] = tempSurfaces.size();
         for (j = 0; j < tempSurfaces.size(); j++) {
            pString[currPos++] = tempSurfaces[j].numPoints;
            pString[currPos++] = tempSurfaces[j].mask;

            bool found = false;
            for (k = 0; k < planeIndices.size(); k++) {
               if (planeIndices[k] == tempSurfaces[j].planeIndex) {
                  pString[currPos++] = k;
                  found = true;
                  break;
               }
            }
            AssertFatal(found, "Error, missing planeindex!");

            for (k = 0; k < tempSurfaces[j].numPoints; k++) {
               pString[currPos++] = (tempSurfaces[j].pointIndices[k] >> 8) & 0xFF;
               pString[currPos++] = (tempSurfaces[j].pointIndices[k] >> 0) & 0xFF;
            }
         }
         AssertFatal(currPos == stringLen, "Error, mismatched string length!");
      }
   } // for (i = 0; i < mConvexHulls.size(); i++)

   // Compact the used vectors
   {
      mPolyListStrings.compact();
      mPolyListPoints.compact();
      mPolyListPlanes.compact();
   }
}

//--------------------------------------------------------------------------
void Interior::processVehicleHullPolyLists()
{
   Vector<U16>             planeIndices(256, __FILE__, __LINE__);
   Vector<U32>             pointIndices(256, __FILE__, __LINE__);
   Vector<U8>              pointMasks(256, __FILE__, __LINE__);
   Vector<U8>              planeMasks(256, __FILE__, __LINE__);
   Vector<TempProcSurface> tempSurfaces(128, __FILE__, __LINE__);
   Vector<PlaneGrouping>   planeGroups(32, __FILE__, __LINE__);
   
   // Reserve space in the vectors
   {
      mVehiclePolyListStrings.setSize(0);
      mVehiclePolyListStrings.reserve(128 << 10);

      mVehiclePolyListPoints.setSize(0);
      mVehiclePolyListPoints.reserve(32 << 10);

      mVehiclePolyListPlanes.setSize(0);
      mVehiclePolyListPlanes.reserve(16 << 10);
   }

   for (U32 i = 0; i < mVehicleConvexHulls.size(); i++) {
      U32 j, k, l, m;

      ConvexHull& rHull = mVehicleConvexHulls[i];

      planeIndices.setSize(0);
      pointIndices.setSize(0);
      tempSurfaces.setSize(0);

      // Extract all the surfaces from this hull into our temporary processing format
      {
         for (j = 0; j < rHull.surfaceCount; j++) {
            tempSurfaces.increment();
            TempProcSurface& temp = tempSurfaces.last();

            U32 surfaceIndex = mVehicleHullSurfaceIndices[j + rHull.surfaceStart];
            const NullSurface& rSurface = mVehicleNullSurfaces[getVehicleNullSurfaceIndex(surfaceIndex)];

            temp.planeIndex = rSurface.planeIndex;
            temp.numPoints  = rSurface.windingCount;
            for (k = 0; k < rSurface.windingCount; k++)
               temp.pointIndices[k] = mVehicleWindings[rSurface.windingStart + k];
         }
      }

      // First order of business: extract all unique planes and points from
      //  the list of surfaces...
      {
         for (j = 0; j < tempSurfaces.size(); j++) {
            const TempProcSurface& rSurface = tempSurfaces[j];

            bool found = false;
            for (k = 0; k < planeIndices.size() && !found; k++) {
               if (rSurface.planeIndex == planeIndices[k])
                  found = true;
            }
            if (!found)
               planeIndices.push_back(rSurface.planeIndex);

            for (k = 0; k < rSurface.numPoints; k++) {
               found = false;
               for (l = 0; l < pointIndices.size(); l++) {
                  if (pointIndices[l] == rSurface.pointIndices[k])
                     found = true;
               }
               if (!found)
                  pointIndices.push_back(rSurface.pointIndices[k]);
            }
         }
      }

      // Now that we have all the unique points and planes, remap the surfaces in
      //  terms of the offsets into the unique point list...
      {
         for (j = 0; j < tempSurfaces.size(); j++) {
            TempProcSurface& rSurface = tempSurfaces[j];

            // Points
            for (k = 0; k < rSurface.numPoints; k++) {
               bool found = false;
               for (l = 0; l < pointIndices.size(); l++) {
                  if (pointIndices[l] == rSurface.pointIndices[k]) {
                     rSurface.pointIndices[k] = l;
                     found = true;
                     break;
                  }
               }
               AssertISV(found, "Error remapping point indices in interior collision processing");
            }
         }
      }

      // Ok, at this point, we have a list of unique points, unique planes, and the
      //  surfaces all remapped in those terms.  We need to check our error conditions
      //  that will make sure that we can properly encode this hull:
      {
         AssertISV(planeIndices.size() < 256,   "Error, > 256 planes on an interior hull");
         AssertISV(pointIndices.size() < 63356, "Error, > 65536 points on an interior hull");
         AssertISV(tempSurfaces.size() < 256,   "Error, > 256 surfaces on an interior hull");
      }

      // Now we group the planes together, and merge the closest groups until we're left
      //  with <= 8 groups
      {
         planeGroups.setSize(planeIndices.size());
         for (j = 0; j < planeIndices.size(); j++) {
            planeGroups[j].numPlanes       = 1;
            planeGroups[j].planeIndices[0] = planeIndices[j];
         }

         while (planeGroups.size() > 8) {
            // Find the two closest groups.  If mdp(i, j) is the value of the
            //  largest pairwise dot product that can be computed from the vectors
            //  of group i, and group j, then the closest group pair is the one
            //  with the smallest value of mdp.
            F32 currmin = 2;
            S32 firstGroup  = -1;
            S32 secondGroup = -1;

            for (j = 0; j < planeGroups.size(); j++) {
               PlaneGrouping& first = planeGroups[j];
               for (k = j + 1; k < planeGroups.size(); k++) {
                  PlaneGrouping& second = planeGroups[k];

                  F32 max = -2;
                  for (l = 0; l < first.numPlanes; l++) {
                     for (m = 0; m < second.numPlanes; m++) {
                        Point3F firstNormal = mVehiclePlanes[first.planeIndices[l]];
                        Point3F secondNormal = mVehiclePlanes[second.planeIndices[m]];

                        F32 dot = mDot(firstNormal, secondNormal);
                        if (dot > max)
                           max = dot;
                     }
                  }

                  if (max < currmin) {
                     currmin = max;
                     firstGroup  = j;
                     secondGroup = k;
                  }
               }
            }
            AssertFatal(firstGroup != -1 && secondGroup != -1, "Error, unable to find a suitable pairing?");

            // Merge first and second
            PlaneGrouping& to   = planeGroups[firstGroup];
            PlaneGrouping& from = planeGroups[secondGroup];
            while (from.numPlanes != 0) {
               to.planeIndices[to.numPlanes++] = from.planeIndices[from.numPlanes - 1];
               from.numPlanes--;
            }

            // And remove the merged group
            planeGroups.erase(secondGroup);
         }
         AssertFatal(planeGroups.size() <= 8, "Error, too many plane groupings!");


         // Assign a mask to each of the plane groupings
         for (j = 0; j < planeGroups.size(); j++)
            planeGroups[j].mask = (1 << j);
      }

      // Now, assign the mask to each of the temp polys
      {
         for (j = 0; j < tempSurfaces.size(); j++) {
            bool assigned = false;
            for (k = 0; k < planeGroups.size() && !assigned; k++) {
               for (l = 0; l < planeGroups[k].numPlanes; l++) {
                  if (planeGroups[k].planeIndices[l] == tempSurfaces[j].planeIndex) {
                     tempSurfaces[j].mask = planeGroups[k].mask;
                     assigned = true;
                     break;
                  }
               }
            }
            AssertFatal(assigned, "Error, missed a plane somewhere in the hull poly list!");
         }
      }

      // Copy the appropriate group mask to the plane masks
      {
         planeMasks.setSize(planeIndices.size());
         dMemset(planeMasks.address(), 0, planeMasks.size() * sizeof(U8));

         for (j = 0; j < planeIndices.size(); j++) {
            bool found = false;
            for (k = 0; k < planeGroups.size() && !found; k++) {
               for (l = 0; l < planeGroups[k].numPlanes; l++) {
                  if (planeGroups[k].planeIndices[l] == planeIndices[j]) {
                     planeMasks[j] = planeGroups[k].mask;
                     found = true;
                     break;
                  }
               }
            }
            AssertFatal(planeMasks[j] != 0, "Error, missing mask for plane!");
         }
      }

      // And whip through the points, constructing the total mask for that point
      {
         pointMasks.setSize(pointIndices.size());
         dMemset(pointMasks.address(), 0, pointMasks.size() * sizeof(U8));

         for (j = 0; j < pointIndices.size(); j++) {
            for (k = 0; k < tempSurfaces.size(); k++) {
               for (l = 0; l < tempSurfaces[k].numPoints; l++) {
                  if (tempSurfaces[k].pointIndices[l] == j) {
                     pointMasks[j] |= tempSurfaces[k].mask;
                     break;
                  }
               }
            }
            AssertFatal(pointMasks[j] != 0, "Error, point must exist in at least one surface!");
         }
      }

      // Create the emit strings, and we're done!
      {
         // Set the range of planes
         rHull.polyListPlaneStart = mVehiclePolyListPlanes.size();
         mVehiclePolyListPlanes.setSize(rHull.polyListPlaneStart + planeIndices.size());
         for (j = 0; j < planeIndices.size(); j++)
            mVehiclePolyListPlanes[j + rHull.polyListPlaneStart] = planeIndices[j];

         // Set the range of points
         rHull.polyListPointStart = mVehiclePolyListPoints.size();
         mVehiclePolyListPoints.setSize(rHull.polyListPointStart + pointIndices.size());
         for (j = 0; j < pointIndices.size(); j++)
            mVehiclePolyListPoints[j + rHull.polyListPointStart] = pointIndices[j];

         // Now the emit string.  The emit string goes like: (all fields are bytes)
         //  NumPlanes (PLMask) * NumPlanes
         //  NumPointsHi NumPointsLo (PtMask) * NumPoints
         //  NumSurfaces
         //   (NumPoints SurfaceMask PlOffset (PtOffsetHi PtOffsetLo) * NumPoints) * NumSurfaces
         //
         U32 stringLen  = 1 + planeIndices.size();
         stringLen     += 2 + pointIndices.size();
         stringLen     += 1;
         for (j = 0; j < tempSurfaces.size(); j++)
            stringLen += 1 + 1 + 1 + (tempSurfaces[j].numPoints * 2);

         rHull.polyListStringStart = mVehiclePolyListStrings.size();
         mVehiclePolyListStrings.setSize(rHull.polyListStringStart + stringLen);

         U8* pString = &mVehiclePolyListStrings[rHull.polyListStringStart];
         U32 currPos = 0;

         // Planes
         pString[currPos++] = planeIndices.size();
         for (j = 0; j < planeIndices.size(); j++)
            pString[currPos++] = planeMasks[j];

         // Points
         pString[currPos++] = (pointIndices.size() >> 8) & 0xFF;
         pString[currPos++] = (pointIndices.size() >> 0) & 0xFF;
         for (j = 0; j < pointIndices.size(); j++)
            pString[currPos++] = pointMasks[j];

         // Surfaces
         pString[currPos++] = tempSurfaces.size();
         for (j = 0; j < tempSurfaces.size(); j++) {
            pString[currPos++] = tempSurfaces[j].numPoints;
            pString[currPos++] = tempSurfaces[j].mask;

            bool found = false;
            for (k = 0; k < planeIndices.size(); k++) {
               if (planeIndices[k] == tempSurfaces[j].planeIndex) {
                  pString[currPos++] = k;
                  found = true;
                  break;
               }
            }
            AssertFatal(found, "Error, missing planeindex!");

            for (k = 0; k < tempSurfaces[j].numPoints; k++) {
               pString[currPos++] = (tempSurfaces[j].pointIndices[k] >> 8) & 0xFF;
               pString[currPos++] = (tempSurfaces[j].pointIndices[k] >> 0) & 0xFF;
            }
         }
         AssertFatal(currPos == stringLen, "Error, mismatched string length!");
      }
   } // for (i = 0; i < mConvexHulls.size(); i++)

   // Compact the used vectors
   {
      mVehiclePolyListStrings.compact();
      mVehiclePolyListPoints.compact();
      mVehiclePolyListPlanes.compact();
   }
}


void Interior::rebuildVertexColors(LM_HANDLE instanceHandle,
                                   Vector<ColorI>* normal,
                                   Vector<ColorI>* alarm)
{
   normal->setSize(mWindings.size());
   if (mHasAlarmState)
      alarm->setSize(mWindings.size());
   else
      alarm->clear();
      
   for (U32 i = 0; i < mSurfaces.size(); i++)
   {
      TextureHandle* normalLMapHandle = gInteriorLMManager.getHandle(getLMHandle(), instanceHandle,
                                                                     mNormalLMapIndices[i]);
      AssertFatal(normalLMapHandle != NULL, "Bad texture handle in rebuildVertexColors");
      const GBitmap* pNormalLMap = normalLMapHandle->getBitmap();
      AssertFatal(pNormalLMap != NULL, "Bad texture handle in rebuildVertexColors");

      TextureHandle* alarmLMapHandle = NULL;
      const GBitmap* pAlarmLMap  = NULL;
      if (mHasAlarmState)
      {
         alarmLMapHandle = gInteriorLMManager.getHandle(getLMHandle(), instanceHandle,
                                                        mAlarmLMapIndices[i]);
         pAlarmLMap = alarmLMapHandle->getBitmap();
      }
      
      const Surface& rSurface = mSurfaces[i];
      const TexGenPlanes& rPlanes = mLMTexGenEQs[i];
      
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++)
      {
         const ItrPaddedPoint& rPoint = mPoints[mWindings[j]];
         ColorI color;

         F32 s = rPlanes.planeX.distToPlane(rPoint.point);
         F32 t = rPlanes.planeY.distToPlane(rPoint.point);

         s *= pNormalLMap->getWidth();
         t *= pNormalLMap->getHeight();
         AssertFatal(s >= 0.5 && s <= (F32(pNormalLMap->getWidth()) - 0.5), "Error, bad lmap coord!");
         AssertFatal(t >= 0.5 && t <= (F32(pNormalLMap->getHeight()) - 0.5), "Error, bad lmap coord!");
         s = mFloor(s);
         t = mFloor(t);

         pNormalLMap->getColor(U32(s), U32(t), color);
         color.alpha = 0xFF;
         (*normal)[j] = color;

         if (mHasAlarmState)
         {
            pAlarmLMap->getColor(U32(s), U32(t), color);
            color.alpha = 0xFF;
            (*alarm)[j] = color;
         }
      }
   }
}


//--------------------------------------------------------------------------
void ZoneVisDeterminer::runFromState(SceneState* state, U32 offset, U32 parentZone)
{
   mMode            = FromState;
   mState           = state;
   mZoneRangeOffset = offset;
   mParentZone      = parentZone;
}

void ZoneVisDeterminer::runFromRects(SceneState* state, U32 offset, U32 parentZone)
{
   mMode            = FromRects;
   mState           = state;
   mZoneRangeOffset = offset;
   mParentZone      = parentZone;
}

bool ZoneVisDeterminer::isZoneVisible(const U32 zone) const
{
   if (zone == 0)
      return mState->getZoneState(mParentZone).render;

   if (mMode == FromState) {
      return mState->getZoneState(zone + mZoneRangeOffset - 1).render;
   } else {
      return sgZoneRenderInfo[zone].render;
   }
}


