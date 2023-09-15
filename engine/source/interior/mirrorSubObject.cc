//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//
// 09/03/03 MM: Changes made by JF to use a stencil mask for mirrors.
//
//-----------------------------------------------------------------------------

#include "interior/mirrorSubObject.h"
#include "interior/interiorInstance.h"
#include "interior/interior.h"
#include "dgl/materialList.h"
#include "core/stream.h"
#include "dgl/dgl.h"
#include "sceneGraph/sgUtil.h"


IMPLEMENT_CONOBJECT(MirrorSubObject);

#include "platform/platformVideo.h"
#include "sceneGraph/sceneGraph.h"

//--------------------------------------------------------------------------
MirrorSubObject::MirrorSubObject()
{
   mTypeMask = StaticObjectType;

   mInitialized = false;
}

MirrorSubObject::~MirrorSubObject()
{
}

//--------------------------------------------------------------------------
void MirrorSubObject::initPersistFields()
{
   Parent::initPersistFields();

   //
}

//--------------------------------------------------------------------------
void MirrorSubObject::renderObject(SceneState* state, SceneRenderImage* image)
{
   // the surface is rendered when the portal is closed
   return;
}

//--------------------------------------------------------------------------
void MirrorSubObject::transformModelview(const U32 portalIndex, const MatrixF& oldMV,
 MatrixF* pNewMV)
{
   AssertFatal(isInitialized() == true, "Error, we should have been initialized by this point!");
   AssertFatal(portalIndex == 0, "Error, we only have one portal!");

   *pNewMV = oldMV;
   pNewMV->mul(mReflectionMatrix);
}

//--------------------------------------------------------------------------
void MirrorSubObject::transformPosition(const U32 portalIndex, Point3F& ioPosition)
{
   AssertFatal(isInitialized() == true, "Error, we should have been initialized by this point!");
   AssertFatal(portalIndex == 0, "Error, we only have one portal!");

   mReflectionMatrix.mulP(ioPosition);
}

//--------------------------------------------------------------------------
bool MirrorSubObject::computeNewFrustum(const U32      portalIndex,
                                        const F64*     oldFrustum,
                                        const F64      nearPlane,
                                        const F64      farPlane,
                                        const RectI&   oldViewport,
                                        F64*           newFrustum,
                                        RectI&         newViewport,
                                        const bool     flippedMatrix)
{
   AssertFatal(isInitialized() == true, "Error, we should have been initialized by this point!");
   AssertFatal(portalIndex == 0, "Error, mirrortests only have one portal!");

   Interior* interior = getInstance()->getDetailLevel(mDetailLevel);
   
   static Vector<SGWinding> mirrorWindings;
   mirrorWindings.setSize(surfaceCount);

   for (U32 i = 0; i < surfaceCount; i++) {
      SGWinding& rSGWinding             = mirrorWindings[i];
      const Interior::Surface& rSurface = interior->mSurfaces[surfaceStart + i];

      U32 fanIndices[32];
      U32 numFanIndices = 0;
      interior->collisionFanFromSurface(rSurface, fanIndices, &numFanIndices);

      for (U32 j = 0; j < numFanIndices; j++)
         rSGWinding.points[j] = interior->mPoints[fanIndices[j]].point;
      rSGWinding.numPoints = numFanIndices;
   }

   MatrixF finalModelView;
   dglGetModelview(&finalModelView);
   finalModelView.mul(getSOTransform());
   finalModelView.scale(getSOScale());

   return sgComputeNewFrustum(oldFrustum, nearPlane, farPlane,
                              oldViewport,
                              mirrorWindings.address(), mirrorWindings.size(),
                              finalModelView,
                              newFrustum, newViewport,
                              flippedMatrix);
}


//--------------------------------------------------------------------------
void MirrorSubObject::openPortal(const U32   portalIndex,
                                 SceneState* pCurrState,
                                 SceneState* pParentState)
{
   AssertFatal(isInitialized() == true, "Error, we should have been initialized by this point!");
   AssertFatal(portalIndex == 0, "Error, mirrortests only have one portal!");

   if (mZone == 0)
      pParentState->setupZoneProjection(getInstance()->getCurrZone(0));
   else
      pParentState->setupZoneProjection(mZone + getInstance()->getZoneRangeStart() - 
1);

   // setup transformation
   glPushMatrix();
   dglMultMatrix(&getSOTransform());
   glScalef(getSOScale().x, getSOScale().y, getSOScale().z);

   // setup stencil buffer:
   glClearStencil(0x0);
   glStencilMask(~0u);
   glEnable(GL_STENCIL_TEST);

   static U32 lastStateKey = 0;
   U32 stateKey = gClientSceneGraph->getStateKey();

   // dont clear the stencil for every portal   
   if(lastStateKey != stateKey)
   {
      lastStateKey = stateKey;
      glClear(GL_STENCIL_BUFFER_BIT);
   }

   // clear with the fog color
   ColorF fogColor = gClientSceneGraph->getFogColor();
   glColor3f(fogColor.red, fogColor.green, fogColor.blue);

   Interior * interior = getInstance()->getDetailLevel(mDetailLevel);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(ItrPaddedPoint), interior->mPoints.address());


   // render the visible surface into the stencil buffer (also render the fog color
   // since terrain may not be rendered and it assumes a fog clear)
   glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
   glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

   for (U32 i = 0; i < surfaceCount; i++) 
   {
      glDrawElements(GL_TRIANGLE_STRIP,
                     interior->mSurfaces[surfaceStart+i].windingCount,
                     GL_UNSIGNED_INT,
                     &interior->mWindings[interior->mSurfaces[surfaceStart+i].windingStart]);
   }

   // now clear the visible surface depth (use stencil). disable color buffers (already
   // rendered fog in previous step).
   glDepthRange(1, 1);
   glDepthFunc(GL_ALWAYS);
   glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

   glStencilFunc(GL_EQUAL, 1, 0xffffffff);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

   for (U32 i = 0; i < surfaceCount; i++) 
   {
      glDrawElements(GL_TRIANGLE_STRIP,
                     interior->mSurfaces[surfaceStart+i].windingCount,
                     GL_UNSIGNED_INT,
                     &interior->mWindings[interior->mSurfaces[surfaceStart+i].windingStart]);
   }

   // reset states (stencil is setup correctly)
   glDepthFunc(GL_LEQUAL);
   glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
   glDepthRange(0, 1);

   glDisableClientState(GL_VERTEX_ARRAY);

   // reset transform
   glPopMatrix();
   dglSetCanonicalState();
}

void MirrorSubObject::closePortal(const U32   portalIndex,
                                  SceneState* pCurrState,
                                  SceneState* pParentState)
{
   AssertFatal(isInitialized() == true, "Error, we should have been initialized by this point!");
   AssertFatal(portalIndex == 0, "Error, mirrortests only have one portal!");

   // setup transformation
   glPushMatrix();
   dglMultMatrix(&getSOTransform());
   glScalef(getSOScale().x, getSOScale().y, getSOScale().z);

   // update depth over portal
   Interior * interior = getInstance()->getDetailLevel(mDetailLevel);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(ItrPaddedPoint), interior->mPoints.address());


   // want to clear stencil value
   glStencilFunc(GL_EQUAL, 1, 0xffffffff);
   glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

   // render the alpha surface
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


   glActiveTextureARB(GL_TEXTURE0_ARB);
   glEnable(GL_TEXTURE_2D);

   glEnable(GL_TEXTURE_GEN_S);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glEnable(GL_TEXTURE_GEN_T);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable(GL_TEXTURE_2D);

   glEnable(GL_TEXTURE_GEN_S);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glEnable(GL_TEXTURE_GEN_T);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


   glActiveTextureARB(GL_TEXTURE0_ARB);

   glEnableClientState(GL_VERTEX_ARRAY);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


   glBindTexture(GL_TEXTURE_2D, interior->mMaterialList->getMaterial(interior->mSurfaces[surfaceStart].textureIndex).getGLName());


   glActiveTextureARB(GL_TEXTURE1_ARB);
   Vector<U8>* pLMapIndices = &interior->mNormalLMapIndices;
   U8 baseIndex = (*pLMapIndices)[surfaceStart];
   TextureHandle *tex = gInteriorLMManager.getHandle( interior->mLMHandle, getInstance()->getLMHandle(), baseIndex); 
   glBindTexture(GL_TEXTURE_2D, tex->getGLName() );



   glActiveTextureARB(GL_TEXTURE0_ARB);
   glTexGenfv(GL_S, GL_OBJECT_PLANE, (GLfloat*)interior->mTexGenEQs[interior->mSurfaces[surfaceStart].texGenIndex].planeX);
   glTexGenfv(GL_T, GL_OBJECT_PLANE, (GLfloat*)interior->mTexGenEQs[interior->mSurfaces[surfaceStart].texGenIndex].planeY);

   glActiveTextureARB(GL_TEXTURE1_ARB);


   glColor4f(1, 1, 1, mAlphaLevel);

   for (U32 i = 0; i < surfaceCount; i++) {

      glTexGenfv(GL_S, GL_OBJECT_PLANE, (GLfloat*)interior->mLMTexGenEQs[surfaceStart+i].planeX);
      glTexGenfv(GL_T, GL_OBJECT_PLANE, (GLfloat*)interior->mLMTexGenEQs[surfaceStart+i].planeY);


      glDrawElements(GL_TRIANGLE_STRIP,
                     interior->mSurfaces[surfaceStart+i].windingCount,
                     GL_UNSIGNED_INT,
                     &interior->mWindings[interior->mSurfaces[surfaceStart+i].windingStart]);
   }

   glDisableClientState(GL_VERTEX_ARRAY);

   glActiveTextureARB(GL_TEXTURE0_ARB);
   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);

   glActiveTextureARB(GL_TEXTURE1_ARB);
   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);
   glDisable(GL_TEXTURE_2D);

   glActiveTextureARB(GL_TEXTURE0_ARB);

   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);

   glDepthFunc(GL_LEQUAL);

   // reset transform
   glPopMatrix();

   // disable stencil test
   glDisable(GL_STENCIL_TEST);
   
   dglSetCanonicalState();
}

//--------------------------------------------------------------------------

void MirrorSubObject::getWSPortalPlane(const U32 portalIndex, PlaneF* pPlane)
{
   AssertFatal(portalIndex == 0, "Error, mirrortests only have one portal!");

   Interior* interior = getInstance()->getDetailLevel(mDetailLevel);
   const Interior::Surface& rSurface = interior->mSurfaces[surfaceStart];

   PlaneF temp = interior->getPlane(rSurface.planeIndex);
   if (Interior::planeIsFlipped(rSurface.planeIndex))
      temp.neg();

   mTransformPlane(getSOTransform(), getSOScale(), temp, pPlane);
}


//--------------------------------------------------------------------------
U32 MirrorSubObject::getSubObjectKey() const
{
   return InteriorSubObject::MirrorSubObjectKey;
}


bool MirrorSubObject::_readISO(Stream& stream)
{
   AssertFatal(isInitialized() == false, "Error, should not be initialized here!");

   if (Parent::_readISO(stream) == false)
      return false;

   stream.read(&mDetailLevel);
   stream.read(&mZone);
   stream.read(&mAlphaLevel);
   stream.read(&surfaceCount);
   stream.read(&surfaceStart);

   stream.read(&mCentroid.x);
   stream.read(&mCentroid.y);
   stream.read(&mCentroid.z);

   return true;
}


bool MirrorSubObject::_writeISO(Stream& stream) const
{
   if (Parent::_writeISO(stream) == false)
      return false;

   stream.write(mDetailLevel);
   stream.write(mZone);
   stream.write(mAlphaLevel);
   stream.write(surfaceCount);
   stream.write(surfaceStart);

   stream.write(mCentroid.x);
   stream.write(mCentroid.y);
   stream.write(mCentroid.z);

   return true;
}


SubObjectRenderImage* MirrorSubObject::getRenderImage(SceneState*    state,
                                                      const Point3F& osPoint)
{
   if (isInitialized() == false)
      setupTransforms();

   // Check to make sure that we&#180;re on the right side of the plane...
   Interior* interior = getInstance()->getDetailLevel(mDetailLevel);
   const Interior::Surface& rSurface = interior->mSurfaces[surfaceStart];

   PlaneF plane = interior->getPlane(rSurface.planeIndex);
   if (Interior::planeIsFlipped(rSurface.planeIndex))
      plane.neg();

   if (plane.whichSide(osPoint) != PlaneF::Front)
      return NULL;

   // On the right side, guess we have to return an image and a portal...
   //
   SubObjectRenderImage* ri = new SubObjectRenderImage;

   ri->obj           = this;
   ri->isTranslucent = false;

   U32 realZone;
   if (getInstance()->getZoneRangeStart() == 0xFFFFFFFF || mZone == 0) {
      realZone = getInstance()->getCurrZone(0);
   } else {
      realZone = getInstance()->getZoneRangeStart() + mZone - 1;
   }

   // Create the WS start point.  this will be the centroid of the first poly in os space,
   //  transformed out for the sceneGraph, with a smidge of our normal added in to pull
   //  it off the surface plane...

   Point3F startPoint = mCentroid;
   PlaneF temp = interior->getPlane(rSurface.planeIndex);
   if (Interior::planeIsFlipped(rSurface.planeIndex))
      temp.neg();
   startPoint += Point3F(temp.x, temp.y, temp.z) * 0.01f;
   getSOTransform().mulP(startPoint);
   startPoint.convolve(getSOScale());

   state->insertTransformPortal(this, 0, realZone, startPoint, true);

   return ri;
}


bool MirrorSubObject::renderDetailDependant() const
{
   return true;
}


U32 MirrorSubObject::getZone() const
{
   return mZone;
}


void MirrorSubObject::setupTransforms()
{
   mInitialized = true;

   Interior* interior = getInstance()->getDetailLevel(mDetailLevel);
   const Interior::Surface& rSurface = interior->mSurfaces[surfaceStart];

   for( U32 i=0; i<surfaceCount; i++ )
   {
      Interior::Surface& surface = interior->mSurfaces[surfaceStart + i];
      //surface.mirrored = true;
   }



   PlaneF plane = interior->getPlane(rSurface.planeIndex);
   if (Interior::planeIsFlipped(rSurface.planeIndex))
      plane.neg();

   Point3F n(plane.x, plane.y, plane.z);
   Point3F q = n;
   q *= -plane.d;

   MatrixF t(true);
   t.scale(getSOScale());
   t.mul(getSOTransform());

   t.mulV(n);
   t.mulP(q);

   F32* ra = mReflectionMatrix;

   ra[0]  = 1.0f - 2.0f*(n.x*n.x); ra[1]  = 0.0f - 2.0f*(n.x*n.y); ra[2]  = 0.0f - 2.0f*(n.x*n.z); ra[3]  = 0.0f;
   ra[4]  = 0.0f - 2.0f*(n.y*n.x); ra[5]  = 1.0f - 2.0f*(n.y*n.y); ra[6]  = 0.0f - 2.0f*(n.y*n.z); ra[7]  = 0.0f;
   ra[8]  = 0.0f - 2.0f*(n.z*n.x); ra[9]  = 0.0f - 2.0f*(n.z*n.y); ra[10] = 1.0f - 2.0f*(n.z*n.z); ra[11] = 0.0f;

   Point3F qnn = n * mDot(n, q);

   ra[12] = qnn.x * 2.0f;
   ra[13] = qnn.y * 2.0f;
   ra[14] = qnn.z * 2.0f;
   ra[15] = 1.0f;

   // Now, the GGems series (as of v1) uses row vectors (arg)
   mReflectionMatrix.transpose();
}

void MirrorSubObject::noteTransformChange()
{
   setupTransforms();
   Parent::noteTransformChange();
}

InteriorSubObject* MirrorSubObject::clone(InteriorInstance* instance) const
{
   MirrorSubObject* pClone = new MirrorSubObject;

   pClone->mDetailLevel = mDetailLevel;
   pClone->mZone        = mZone;
   pClone->mAlphaLevel  = mAlphaLevel;
   pClone->mCentroid    = mCentroid;
   pClone->surfaceCount = surfaceCount;
   pClone->surfaceStart = surfaceStart;

   pClone->mInteriorInstance = instance;

   return pClone;
}
