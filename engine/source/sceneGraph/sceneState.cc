//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "sceneGraph/sceneState.h"
#include "sceneGraph/sgUtil.h"
#include "sim/sceneObject.h"
#include "dgl/dgl.h"
#include "sceneGraph/sceneGraph.h"
#include "terrain/sky.h"
#include "platform/profiler.h"

namespace {

S32 FN_CDECL
cmpImageFunc(const void* p1, const void* p2)
{
   const SceneRenderImage* psri1 = *((const SceneRenderImage**)p1);
   const SceneRenderImage* psri2 = *((const SceneRenderImage**)p2);
   // Compares only non-transcluent images
   AssertFatal(psri1->isTranslucent == false && psri2->isTranslucent == false,
               "Error, only non-translucent images allowed here.");
   
   if (psri1->sortType != psri2->sortType)
   {
      // Normal render images are setup in such a way that increasing order
      //  renders
      return S32(psri1->sortType) - S32(psri2->sortType);
   }
   else
   {
      // Otherwise, sort on primary texture, as set by the sort key
      return S32(psri1->textureSortKey) - S32(psri2->textureSortKey);
   }
}

S32 FN_CDECL
cmpTPImageFunc(const void* p1, const void* p2)
{
   const SceneRenderImage* psri1 = *((const SceneRenderImage**)p1);
   const SceneRenderImage* psri2 = *((const SceneRenderImage**)p2);
   // Compares only non-transcluent images
   AssertFatal(psri1->isTranslucent == true && psri2->isTranslucent == true,
               "Error, only non-translucent images allowed here.");
   
   return S32(psri1->textureSortKey) - S32(psri2->textureSortKey);
}

S32 FN_CDECL
cmpPlaneImageFunc(const void* p1, const void* p2)
{
   const SceneRenderImage** psri1 = (const SceneRenderImage**)p1;
   const SceneRenderImage** psri2 = (const SceneRenderImage**)p2;

   // Normal render images are setup in such a way that increasing order
   //  renders
   if (((*psri2)->polyArea - (*psri1)->polyArea) < 0.0)
      return -1;
   else if (((*psri2)->polyArea - (*psri1)->polyArea) == 0.0)
      return 0;
   else
      return 1;
}


S32 FN_CDECL
cmpPointImageFunc(const void* p1, const void* p2)
{
   const SceneRenderImage* psri1 = *((const SceneRenderImage**)p1);
   const SceneRenderImage* psri2 = *((const SceneRenderImage**)p2);

   if (psri1->pointDistSq != psri2->pointDistSq)
   {
      if (psri1->pointDistSq > psri2->pointDistSq)
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
   else
   {
      if (psri1->tieBreaker == true)
      {
         return -1;
      }
      else
      {
         return 1;
      }
   }
}

inline void renderImage(SceneState* state, SceneRenderImage* image)
{
   PROFILE_START(SceneStateRenderImage);
#if defined(TORQUE_DEBUG)
   S32 m, p, t0, t1, v[4];
   F32 t0m[16], t1m[16];
   dglGetTransformState(&m, &p, &t0, t0m, &t1, t1m, v);
#endif
   image->obj->renderObject(state, image);

#if defined(TORQUE_DEBUG)
   if (dglCheckState(m, p, t0, t0m, t1, t1m, v) == false) {
      S32 bm, bp, bt0, bt1, bv[4];
      F32 bt0m[16], bt1m[16];
      dglGetTransformState(&bm, &bp, &bt0, bt0m, &bt1, bt1m, bv);
      AssertFatal(false,
                  avar("Error, object of class %s either unbalanced the xform stacks, or didn't reset the viewport!"
                       " mv(%d %d) proj(%d %d) t0(%d %d), t1(%d %d) (%d %d %d %d: %d %d %d %d)",
                       image->obj->getClassName(),
                       m, bm, p, bp, t0, bt0, t1, bt1, v[0], v[1], v[2], v[3], bv[0], bv[1], bv[2], bv[3]));
   }
#endif
   PROFILE_END();
}



} // namespace {}


// MM/JF: Added for mirrorSubObject fix.
void SceneState::setupClipPlanes(ZoneState& rState)
{
	F32 farOverNear = getFarPlane() / getNearPlane();
	Point3F farPosLeftUp = Point3F(rState.frustum[0] * farOverNear, getFarPlane(), rState.frustum[3] * farOverNear);
	Point3F farPosLeftDown = Point3F(rState.frustum[0] * farOverNear, getFarPlane(), rState.frustum[2] * farOverNear);
	Point3F farPosRightUp = Point3F(rState.frustum[1] * farOverNear, getFarPlane(), rState.frustum[3] * farOverNear);
	Point3F farPosRightDown = Point3F(rState.frustum[1] * farOverNear, getFarPlane(), rState.frustum[2] * farOverNear);
	MatrixF temp = mModelview;
	temp.inverse();
	temp.mulP(farPosLeftUp);
	temp.mulP(farPosLeftDown);
	temp.mulP(farPosRightUp);
	temp.mulP(farPosRightDown);
	sgOrientClipPlanes(&rState.clipPlanes[0], getCameraPosition(), farPosLeftUp, farPosLeftDown, farPosRightUp, farPosRightDown);
	rState.clipPlanesValid = true;
}


//--------------------------------------------------------------------------
//--------------------------------------
SceneState::SceneState(SceneState*    parent,
                       const U32      numZones,
                       F64            left,
                       F64            right,
                       F64            bottom,
                       F64            top,
                       F64            nearPlane,
                       F64            farPlane,
                       RectI          viewport,
                       const Point3F& camPos,
                       const MatrixF& modelview,
                       F32            fogDistance,
                       F32            visibleDistance,
                       ColorF         fogColor,
                       U32            numFogVolumes,
                       FogVolume*     fogVolumes,
                       TextureHandle  envMap,
                       F32            visFactor)
{
   mVisFactor = visFactor;

   mParent   = parent;
   mFlipCull = false;

   mBaseZoneState.render          = false;
   mBaseZoneState.clipPlanesValid = false;
   mBaseZoneState.frustum[0] = left;
   mBaseZoneState.frustum[1] = right;
   mBaseZoneState.frustum[2] = bottom;
   mBaseZoneState.frustum[3] = top;
   mBaseZoneState.viewport = viewport;
#if defined(TORQUE_DEBUG)
   // Avoid FPU exceptions in ZoneState constructors
   dMemset(mBaseZoneState.clipPlanes, 0, (sizeof mBaseZoneState.clipPlanes));
#endif

   mNearPlane   = nearPlane;
   mFarPlane    = farPlane;

   mModelview   = modelview;
   mCamPosition = camPos;
   mFogDistance = fogDistance;
   mVisibleDistance = visibleDistance;
   mFogColor = fogColor;

   mZoneStates.setSize(numZones);
   for (U32 i = 0; i < numZones; i++)
   {
      mZoneStates[i].render          = false;
      mZoneStates[i].clipPlanesValid = false;
   }

   mPortalOwner = NULL;
   mPortalIndex = 0xFFFFFFFF;
   
   mNumFogVolumes = numFogVolumes;
   mFogVolumes = fogVolumes;
   setupFog();

   mTerrainOverride = false;

   mEnvironmentMap  = envMap;

   mRenderImages.reserve(128);
   mTranslucentPlaneImages.reserve(128);
   mTranslucentPointImages.reserve(128);
   mTranslucentBeginImages.reserve(32);
   mTranslucentEndImages.reserve(32);
   mTranslucentBSP.reserve(64);
   mTranslucentBSP.setSize(1);
   mTranslucentBSP[0].riList = NULL;
   mTranslucentBSP[0].frontIndex = 0xFFFF;
   mTranslucentBSP[0].backIndex = 0xFFFF;
   mTranslucentBSP[0].rimage = NULL;
}

SceneState::~SceneState()
{
   U32 i;
   for (i = 0; i < mSubsidiaries.size(); i++)
      delete mSubsidiaries[i];

   for (i = 0; i < mRenderImages.size(); i++)
      delete mRenderImages[i];
   for (i = 0; i < mTranslucentPlaneImages.size(); i++)
      delete mTranslucentPlaneImages[i];
   for (i = 0; i < mTranslucentPointImages.size(); i++)
      delete mTranslucentPointImages[i];
   for (i = 0; i < mTranslucentEndImages.size(); i++)
      delete mTranslucentEndImages[i];
   for (i = 0; i < mTranslucentBeginImages.size(); i++)
      delete mTranslucentBeginImages[i];
}

void SceneState::setPortal(SceneObject* owner, const U32 index)
{
   mPortalOwner = owner;
   mPortalIndex = index;
}

void SceneState::insertRenderImage(SceneRenderImage* ri)
{
   if (ri->isTranslucent == false)
      mRenderImages.push_back(ri);
   else
   {
      if (ri->sortType == SceneRenderImage::Plane)
      {
         mTranslucentPlaneImages.push_back(ri);
      }
      else if (ri->sortType == SceneRenderImage::Point)
      {
         mTranslucentPointImages.push_back(ri);
      }
      else if (ri->sortType == SceneRenderImage::BeginSort)
      {
         mTranslucentBeginImages.push_back(ri);
      }
      else
      {
         AssertFatal(ri->sortType == SceneRenderImage::EndSort, "Error, bad transcluent sortType");
         mTranslucentEndImages.push_back(ri);
      }
   }
}

void SceneState::insertTransformPortal(SceneObject* owner, U32 portalIndex,
                                       U32 globalZone,     const Point3F& traversalStartPoint,
                                       const bool flipCull)
{
   mTransformPortals.increment();
   mTransformPortals.last().owner         = owner;
   mTransformPortals.last().portalIndex   = portalIndex;
   mTransformPortals.last().globalZone    = globalZone;
   mTransformPortals.last().traverseStart = traversalStartPoint;
   mTransformPortals.last().flipCull      = flipCull;
}

void SceneState::sortRenderImages()
{
   dQsort(mRenderImages.address(), mRenderImages.size(), sizeof(SceneRenderImage*), cmpImageFunc);
   dQsort(mTranslucentPointImages.address(), mTranslucentPointImages.size(), sizeof(SceneRenderImage*), cmpTPImageFunc);
   dQsort(mTranslucentPlaneImages.address(), mTranslucentPlaneImages.size(), sizeof(SceneRenderImage*), cmpPlaneImageFunc);
}

void SceneState::insertIntoNode(RenderBSPNode& rNode, SceneRenderImage* pImage, bool rendered)
{
   if (rNode.frontIndex == 0xFFFF)
   {
      // Split the node
      rNode.plane = pImage->plane;
      rNode.frontIndex = mTranslucentBSP.size() + 0;
      rNode.backIndex  = mTranslucentBSP.size() + 1;
      if (rendered)
         rNode.rimage = pImage;

      mTranslucentBSP.increment(2);
      mTranslucentBSP[rNode.frontIndex].riList     = NULL;
      mTranslucentBSP[rNode.frontIndex].frontIndex = 0xFFFF;
      mTranslucentBSP[rNode.frontIndex].backIndex  = 0xFFFF;
      mTranslucentBSP[rNode.frontIndex].rimage     = NULL;
      mTranslucentBSP[rNode.backIndex].riList      = NULL;
      mTranslucentBSP[rNode.backIndex].frontIndex  = 0xFFFF;
      mTranslucentBSP[rNode.backIndex].backIndex   = 0xFFFF;
      mTranslucentBSP[rNode.backIndex].rimage      = NULL;
      
      return;
   }

   // Determine which side we're on...
   U32 mask = 0;
   F32 dist = 0.0f;
   for (U32 i = 0; i < 4; i++)
   {
      F32 d = rNode.plane.distToPlane(pImage->poly[i]);
      if (d >= 0.0f)
      {
         mask |= (1 << i);
      }
      dist += d;
   }

   if (mask == 0xF)
   {
      // Front only
      insertIntoNode(mTranslucentBSP[rNode.frontIndex], pImage);
   }
   else if (mask == 0)
   {
      // Back only
      insertIntoNode(mTranslucentBSP[rNode.backIndex], pImage);
   }
   else
   {
      // Both
      if (dist >= 0.0f)
      {
         // Render front
         insertIntoNode(mTranslucentBSP[rNode.frontIndex], pImage, true);
         insertIntoNode(mTranslucentBSP[rNode.backIndex], pImage, false);
      }
      else
      {
         // Render back
         insertIntoNode(mTranslucentBSP[rNode.frontIndex], pImage, false);
         insertIntoNode(mTranslucentBSP[rNode.backIndex], pImage, true);
      }
   }
}


void SceneState::buildTranslucentBSP()
{
   U32 i;
   for (i = 0; i < mTranslucentPlaneImages.size(); i++)
   {
      SceneRenderImage* pImage = mTranslucentPlaneImages[i];
      AssertFatal(pImage->sortType == SceneRenderImage::Plane, "Error, bad sort type on plane list!");

      insertIntoNode(mTranslucentBSP[0], pImage);
   }

   for (i = 0; i < mTranslucentPointImages.size(); i++)
   {
      SceneRenderImage* pImage = mTranslucentPointImages[i];
      AssertFatal(pImage->sortType == SceneRenderImage::Point, "Error, bad sort type on point list!");

      RenderBSPNode* pNode = &mTranslucentBSP[0];
      while (true)
      {
         if (pNode->frontIndex != 0xFFFF)
         {
            if (pNode->plane.distToPlane(pImage->poly[0]) >= 0)
               pNode = &mTranslucentBSP[pNode->frontIndex];
            else
               pNode = &mTranslucentBSP[pNode->backIndex];
         }
         else
         {
            pImage->pNext = pNode->riList;
            pNode->riList = pImage;
            break;
         }
      }
   }
}

void SceneState::renderNode(RenderBSPNode& rNode)
{
   if (rNode.frontIndex != 0xFFFF)
   {
      if (rNode.plane.distToPlane(mCamPosition) >= 0)
      {
         renderNode(mTranslucentBSP[rNode.backIndex]);
         if (rNode.rimage != NULL)
            renderImage(this, rNode.rimage);

         renderNode(mTranslucentBSP[rNode.frontIndex]);
      }
      else
      {
         renderNode(mTranslucentBSP[rNode.frontIndex]);
         if (rNode.rimage != NULL)
            renderImage(this, rNode.rimage);

         renderNode(mTranslucentBSP[rNode.backIndex]);
      }
   }
   else
   {
      Vector<SceneRenderImage*> imageList(128);
      SceneRenderImage* pImage = rNode.riList;
      while (pImage != NULL)
      {
         pImage->pointDistSq = (mCamPosition - pImage->poly[0]).lenSquared();
         imageList.push_back(pImage);
         pImage = pImage->pNext;
      }

      dQsort(imageList.address(), imageList.size(), sizeof(SceneRenderImage*), cmpPointImageFunc);
      
      for (U32 i = 0; i < imageList.size(); i++)
      {
         renderImage(this, imageList[i]);
      }
   }
}


void SceneState::renderCurrentImages()
{
   sortRenderImages();
   buildTranslucentBSP();
   
   if (mPortalOwner != NULL) {
      // If we're a portalized object, we need to setup a user clip plane...
      PlaneF clipPlane;
      mPortalOwner->getWSPortalPlane(mPortalIndex, &clipPlane);

      if (mFlipCull)
         clipPlane.neg();

      GLdouble planeEQ[4];
      planeEQ[0] = clipPlane.x;
      planeEQ[1] = clipPlane.y;
      planeEQ[2] = clipPlane.z;
      planeEQ[3] = clipPlane.d;
      glClipPlane(GL_CLIP_PLANE0, planeEQ);
      glEnable(GL_CLIP_PLANE0);
   }

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   dglLoadMatrix(&mModelview);
   
   U32 i;
   for (i = 0; i < mRenderImages.size(); i++)
      renderImage(this, mRenderImages[i]);


   for (i = 0; i < mTranslucentBeginImages.size(); i++)
      renderImage(this, mTranslucentBeginImages[i]);

   renderNode(mTranslucentBSP[0]);

   
   for (i = 0; i < mTranslucentEndImages.size(); i++)
      renderImage(this, mTranslucentEndImages[i]);

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   if (mPortalOwner != NULL)
      glDisable(GL_CLIP_PLANE0);
}

void SceneState::setupZoneProjection(const U32 zone)
{
   const ZoneState& rState = getZoneState(zone);
   AssertFatal(rState.render == true, "Error, should never set up a non-rendering zone!");

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   dglSetFrustum(rState.frustum[0], rState.frustum[1],
                 rState.frustum[2], rState.frustum[3],
                 getNearPlane(), getFarPlane());
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(rState.viewport);
}

void SceneState::setupObjectProjection(const SceneObject* obj)
{
   RectI viewport;
   F64   frustum[4] = { 1e10, -1e10, 1e10, -1e10 };

   bool init = false;
   SceneObjectRef* pWalk = obj->mZoneRefHead;
   AssertFatal(pWalk != NULL, "Error, object must exist in at least one zone to call this!");
   while (pWalk) {
      const ZoneState& rState = getZoneState(pWalk->zone);
      if (rState.render == true) {
         // frustum
         if (rState.frustum[0] < frustum[0]) frustum[0] = rState.frustum[0];
         if (rState.frustum[1] > frustum[1]) frustum[1] = rState.frustum[1];
         if (rState.frustum[2] < frustum[2]) frustum[2] = rState.frustum[2];
         if (rState.frustum[3] > frustum[3]) frustum[3] = rState.frustum[3];

         // viewport
         if (init == false)
            viewport = rState.viewport;
         else
            viewport.unionRects(rState.viewport); 

         init = true;
      }
      pWalk = pWalk->nextInObj;
   }
   AssertFatal(init, "Error, at least one zone must be rendered here!");

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   dglSetFrustum(frustum[0], frustum[1],
                 frustum[2], frustum[3],
                 getNearPlane(), getFarPlane());
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(viewport);
}

void SceneState::setupBaseProjection()
{
   const ZoneState& rState = getBaseZoneState();

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   dglSetFrustum(rState.frustum[0], rState.frustum[1],
                 rState.frustum[2], rState.frustum[3],
                 getNearPlane(), getFarPlane());
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(rState.viewport);
}


bool SceneState::isObjectRendered(const SceneObject* obj)
{
   const SceneObjectRef* pWalk = obj->mZoneRefHead;

   static F32 darkToOGLCoord[16] = { 1, 0,  0, 0,
                                     0, 0, -1, 0,
                                     0, 1,  0, 0,
                                     0, 0,  0, 1 };
   static MatrixF darkToOGLMatrix;
   static bool matrixInitialized = false;
   if (matrixInitialized == false)
   {
      F32* m = darkToOGLMatrix;
      for (U32 i = 0; i < 16; i++)
         m[i] = darkToOGLCoord[i];
      darkToOGLMatrix.transpose();
      matrixInitialized = true;
   }

   while (pWalk != NULL) {
      if (getZoneState(pWalk->zone).render == true)
      {
         ZoneState& rState = getZoneStateNC(pWalk->zone);
         if (rState.clipPlanesValid == false)
         {
            setupClipPlanes(rState);
         }

         const Box3F& rObjBox = obj->getObjBox();
         const Point3F& rScale = obj->getScale();

         Point3F center;
         rObjBox.getCenter(&center);
         center.convolve(rScale);

         Point3F xRad((rObjBox.max.x - rObjBox.min.x) * 0.5 * rScale.x, 0, 0);
         Point3F yRad(0, (rObjBox.max.y - rObjBox.min.y) * 0.5 * rScale.y, 0);
         Point3F zRad(0, 0, (rObjBox.max.z - rObjBox.min.z) * 0.5 * rScale.z);

         obj->getRenderTransform().mulP(center);
         obj->getRenderTransform().mulV(xRad);
         obj->getRenderTransform().mulV(yRad);
         obj->getRenderTransform().mulV(zRad);

         bool render = true;
         for (U32 i = 0; i < 5; i++) {
            if (rState.clipPlanes[i].whichSideBox(center, xRad, yRad, zRad, Point3F(0, 0, 0)) == PlaneF::Back) {
               render = false;
               break;
            }
         }

         if (render)
            return true;
      }

      pWalk = pWalk->nextInObj;
   }

   return false;
}

//--------------------------------------------------------------------------
//--------------------------------------

bool checkFogBandBoxVisible(F32 dist, F32 haze, F32 low, F32 high, Vector<SceneState::FogBand> &fb)
{
   // if there are no fog bands, no fog - it's visible
   if(!fb.size())
      return true;
   // if the first fog band is unfogged and the box
   // is inside the band, it's visible
   if(fb[0].isFog == false && low < fb[0].cap)
      return true;

   // check the case of the camera in a fog band
   if(fb[0].isFog)
   {
      // if the low point is in the fog, we check that
      
      if(low < fb[0].cap)   
      {
         if(haze + dist * fb[0].factor < 1)
            return true;
         // if low and high are both in the fog band
         // and low isn't visible, neither is high
         if(high < fb[0].cap)
            return false;
         // check the high point...
         F32 highDist = mSqrt(high * high + dist * dist - low * low);
         return haze + (fb[0].cap / high) * highDist * fb[0].factor < 1;
      }
      // ok, both low and high are above the cap of the plane
      // so we have to check only the high point (bigger triangle means less fog
      // applied (higher top means steeper slope on the hypotenuse))

      F32 highDist = mSqrt(high * high + dist * dist - low * low);
      return haze + (fb[0].cap / high) * highDist * fb[0].factor < 1;
   }
   // ok, fb[0] is not fogged, meaning there is an empty layer
   // followed by a fog plane, followed by the box.

   // we only test the one fog volume for visibility of the box...
   F32 fogStart = fb[0].cap;
   F32 fogEnd = fogStart + fb[1].cap;

   // if the low is in the fog band, we have to check
   // low, followed by possibly high
   // if low is above the fog band we only have to check high point
   if(low > fogEnd)
   {
      // only check the high point through the fog
      F32 highDist = mSqrt(high * high + dist * dist - low * low);
      return haze + (fb[1].cap / high) * highDist * fb[1].factor < 1;
   }
   // last case, low is in the fog band
   // check low vis:
   if(haze + fb[1].factor * dist * (low - fogStart) / low < 1)
      return true;
   // if the high point is in the same fog band, it's not visible
   if(high < fogEnd)
      return false;
   // ok, check the high point
   F32 highDist = mSqrt(high * high + dist * dist - low * low);
   return haze + (fb[1].cap / high) * highDist * fb[1].factor < 1;
}

bool SceneState::isBoxFogVisible(F32 dist, F32 top, F32 bottom)
{
   F32 camZ = mCamPosition.z;
   float haze = 0;
   if(dist > mFogDistance) {
      float distFactor = (dist - mFogDistance) * mFogScale - 1.0;
      haze = 1.0 - distFactor * distFactor;
   }
   F32 distSq = dist * dist;
   // the object is below:
   if(top < camZ)
   {
      return checkFogBandBoxVisible(dist, haze, camZ - top, camZ - bottom, mNegFogBands);
   }
   else if(bottom > camZ)
   {
      return checkFogBandBoxVisible(dist, haze, bottom - camZ, top - camZ, mPosFogBands);
   }
   else
   {
      // spans the fog...
      if(!mNegFogBands.size() || !mPosFogBands.size() || !mPosFogBands[0].isFog)
         return true;
      // ok, we know there is at least one fog band and the camera is in it.
      // check if the object is visible through the fog...
      if(haze + dist * mPosFogBands[0].factor < 1)
         return true;

      // ok, check the top stretch...
      // we know now that since the box spans the horizontal,
      // that dist is a horizontal (deltaZ = 0)
      // so we want the segment of the hypotenuse that goes through
      // the fog.

      F32 ht = top - camZ;
      // don't do it if the top is in the fog
      if(ht > mPosFogBands[0].cap)
      {
         if(haze + (mPosFogBands[0].cap / ht) * mSqrt(dist * dist + ht * ht) * mPosFogBands[0].factor < 1)
            return true;
      }

      // ok, last chance, check the bottom segment
      ht = camZ - bottom;
      if(ht < mNegFogBands[0].cap)
         return false;
      return haze + (mNegFogBands[0].cap / ht) * mSqrt(dist * dist + ht * ht) * mNegFogBands[0].factor < 1;
   }
}

void SceneState::setupFog()
{

   if( mVisibleDistance == mFogDistance ) {
      // FIXME: arbitrary large constant
      mFogScale = 1000.0f;
   } else {
      mFogScale = 1.0 / (mVisibleDistance - mFogDistance);
   }

   // construct positive fog volumes
   mPosFogBands.clear();
   F32 camZ = mCamPosition.z;
   
   S32 i;
   for(i = 0; i < mNumFogVolumes; i++)
   {
      if(camZ < mFogVolumes[i].maxHeight)
         break;
   }
   if(i < mNumFogVolumes)
   {
      float prevHeight = camZ;
      for(;i < mNumFogVolumes; i++)
      {
         if(prevHeight < mFogVolumes[i].minHeight)
         {
            FogBand fb;
            fb.isFog = false;
            fb.color.set(mFogVolumes[i].color.red,
                         mFogVolumes[i].color.green,
                         mFogVolumes[i].color.blue,
                         mFogVolumes[i].color.alpha);
            fb.cap     = mFogVolumes[i].minHeight - prevHeight;
            prevHeight = mFogVolumes[i].minHeight;
            mPosFogBands.push_back(fb);
         }
         FogBand fb;
         fb.isFog = true;
         fb.cap = mFogVolumes[i].maxHeight - prevHeight;
         fb.color.set(mFogVolumes[i].color.red,
                      mFogVolumes[i].color.green,
                      mFogVolumes[i].color.blue,
                      mFogVolumes[i].color.alpha);
         fb.factor = (1 / (mFogVolumes[i].visibleDistance * mVisFactor)) * mFogVolumes[i].percentage;
         prevHeight = mFogVolumes[i].maxHeight;
         mPosFogBands.push_back(fb);
      }
   }
   // construct negative fog volumes
   mNegFogBands.clear();
   for(i = mNumFogVolumes - 1; i >= 0; i--)
   {
      if(camZ > mFogVolumes[i].minHeight)
         break;
   }
   if(i >= 0)
   {
      float prevHeight = camZ;
      for(;i >= 0; i--)
      {
         if(prevHeight > mFogVolumes[i].maxHeight)
         {
            FogBand fb;
            fb.isFog = false;
            fb.cap = prevHeight - mFogVolumes[i].maxHeight;
            prevHeight = mFogVolumes[i].maxHeight;
            fb.color.set(mFogVolumes[i].color.red,
                         mFogVolumes[i].color.green,
                         mFogVolumes[i].color.blue,
                         mFogVolumes[i].color.alpha);
            mNegFogBands.push_back(fb);
         }
         FogBand fb;
         fb.isFog = true;
         fb.cap = prevHeight - mFogVolumes[i].minHeight;
         fb.factor = (1 / (mFogVolumes[i].visibleDistance * mVisFactor)) * mFogVolumes[i].percentage;
         prevHeight = mFogVolumes[i].minHeight;
         fb.color.set(mFogVolumes[i].color.red,
                      mFogVolumes[i].color.green,
                      mFogVolumes[i].color.blue,
                      mFogVolumes[i].color.alpha);
         mNegFogBands.push_back(fb);
      }
   }
}

void SceneState::getFogs(float dist, float deltaZ, ColorF *array, U32 &numFogs)
{
   numFogs = 0;
   Vector<FogBand> *band;
   if(deltaZ < 0)
   {
      deltaZ = -deltaZ;
      band = &mNegFogBands;
   }
   else
      band = &mPosFogBands;

   float ht = deltaZ;
   for(int i = 0; i < band->size(); i++)
   {
      FogBand &bnd = (*band)[i];
      
      if(ht < bnd.cap)
      {
         if(bnd.isFog)
            array[numFogs++] = ColorF(bnd.color.red, bnd.color.green, bnd.color.blue, dist * bnd.factor);
         break;
      }
      float subDist = dist * bnd.cap / ht;
      if(bnd.isFog)
      {
         array[numFogs++] = ColorF(bnd.color.red,
                                   bnd.color.green,
                                   bnd.color.blue,
                                   subDist * bnd.factor);
      }
      dist -= subDist;
      ht   -= bnd.cap;
   }
}

F32 SceneState::getFog(float dist, float deltaZ, S32 volKey)
{
   float haze = 0;
   Vector<FogBand> *band;
   if(deltaZ < 0)
   {
      deltaZ = -deltaZ;
      band = &mNegFogBands;
   }
   else
      band = &mPosFogBands;

   if(band->size() < 1)
      return haze;
   
   float ht = deltaZ;
   FogBand &bnd = (*band)[volKey];
   
   if(ht < bnd.cap)
   {
      if(bnd.isFog)
         haze += dist * bnd.factor;
   }
   else
   {
      float subDist = dist * bnd.cap / ht;
      if(bnd.isFog)
         haze += subDist * bnd.factor;
   }
 
   return haze;
}

F32 SceneState::getFog(float dist, float deltaZ)
{
   float haze = 0;
   Vector<FogBand> *band;
   if(deltaZ < 0)
   {
      deltaZ = -deltaZ;
      band = &mNegFogBands;
   }
   else
      band = &mPosFogBands;

   float ht = deltaZ;
   for(int i = 0; i < band->size(); i++)
   {
      FogBand &bnd = (*band)[i];
      
      if(ht < bnd.cap)
      {
         if(bnd.isFog)
            haze += dist * bnd.factor;
         break;
      }
      float subDist = dist * bnd.cap / ht;
      if(bnd.isFog)
         haze += subDist * bnd.factor;
      dist -= subDist;
      ht -= bnd.cap;
   }
   return haze;
}

F32 SceneState::getHazeAndFog(float dist, float deltaZ)
{
   float haze = 0;
      
   if(dist > mFogDistance) {
      if (dist > mVisibleDistance)
         return 1.0;

      float distFactor = (dist - mFogDistance) * mFogScale - 1.0;
      haze = 1.0 - distFactor * distFactor;
   }

   Vector<FogBand> *band; 
   if(deltaZ < 0)
   {
      deltaZ = -deltaZ;
      band = &mNegFogBands;
   }
   else
      band = &mPosFogBands;

   float ht = deltaZ;
   for(int i = 0; i < band->size(); i++)
   {
      FogBand &bnd = (*band)[i];
      
      if(ht < bnd.cap)
      {
         if(bnd.isFog)
            haze += dist * bnd.factor;
         break;
      }
      float subDist = dist * bnd.cap / ht;
      if(bnd.isFog)
         haze += subDist * bnd.factor;
      dist -= subDist;
      ht -= bnd.cap;
   }
   if(haze > 1)
      return 1;
   return haze;
}


void SceneState::setImageRefPoint(SceneObject* obj, SceneRenderImage* image) const
{
   const Box3F& rBox = obj->getObjBox();
   Point3F objSpaceCamPosition = mCamPosition;
   obj->getRenderWorldTransform().mulP(objSpaceCamPosition);
   objSpaceCamPosition.convolveInverse(obj->getScale());
   image->poly[0] = rBox.getClosestPoint(objSpaceCamPosition);
   image->poly[0].convolve(obj->getScale());
   obj->getRenderTransform().mulP(image->poly[0]);
}

//--------------------------------------------------------------------------
//--------------------------------------
SceneRenderImage::~SceneRenderImage()
{
   
}

