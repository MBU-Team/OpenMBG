//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interior.h"
#include "console/console.h"
#include "core/color.h"
#include "dgl/gTexManager.h"
#include "dgl/dgl.h"
#include "math/mMatrix.h"
#include "dgl/materialList.h"
#include "dgl/gBitmap.h"

extern U16* sgActivePolyList;
extern U32  sgActivePolyListSize;

namespace {

void lineLoopFromStrip(Vector<ItrPaddedPoint>& points,
                       Vector<U32>&            windings,
                       U32                     windingStart,
                       U32                     windingCount)
{
   glBegin(GL_LINE_LOOP);
   glVertex3fv(points[windings[windingStart]].point);

   S32 skip = windingStart + 1;
   while (skip < (windingStart + windingCount)) {
      glVertex3fv(points[windings[skip]].point);
      skip += 2;
   }

   skip -= 1;
   while (skip > windingStart) {
      if (skip < (windingStart + windingCount))
         glVertex3fv(points[windings[skip]].point);
      skip -= 2;
   }
   glEnd();
}

void lineStrip(Vector<ItrPaddedPoint>& points,
               Vector<U32>&            windings,
               U32                     windingStart,
               U32                     windingCount)
{
   U32 end = 2;

   while (end < windingCount) {
      // Even
      glBegin(GL_LINE_LOOP);
      glVertex3fv(points[windings[windingStart + end - 2]].point);
      glVertex3fv(points[windings[windingStart + end - 1]].point);
      glVertex3fv(points[windings[windingStart + end - 0]].point);
      glEnd();

      end++;
      if (end >= windingCount)
         break;

      glBegin(GL_LINE_LOOP);
      glVertex3fv(points[windings[windingStart + end - 1]].point);
      glVertex3fv(points[windings[windingStart + end - 2]].point);
      glVertex3fv(points[windings[windingStart + end - 0]].point);
      glEnd();
      end++;
   }
}

} // namespace {}


void Interior::debugRender(MaterialList* pMaterials, LM_HANDLE instanceHandle)
{
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(ItrPaddedPoint), mPoints.address());

   switch (smRenderMode) {
     case NormalRenderLines:
      debugNormalRenderLines();
      break;

     case ShowDetail:
      debugShowDetail();
      break;

     case ShowAmbiguous:
      debugShowAmbiguous();
      break;

     case ShowLightmaps:
      debugShowLightmaps(instanceHandle);
      break;

     case ShowPortalZones:
      debugShowPortalZones();
      break;

     case ShowCollisionFans:
      debugShowCollisionFans();
      break;

     case ShowOrphan:
      debugShowOrphan();
      break;

     case ShowStrips:
      debugShowStrips();
      break;

     case ShowTexturesOnly:
      debugShowTexturesOnly(pMaterials);
      break;

     case ShowNullSurfaces:
      debugShowNullSurfaces(pMaterials);
      break;

     case ShowLargeTextures:
      debugShowLargeTextures(pMaterials);
      break;

     case ShowOutsideVisible:
      debugShowOutsideVisible();
      break;

     case ShowHullSurfaces:
      debugShowHullSurfaces();
      break;

     case ShowVehicleHullSurfaces:
      debugShowVehicleHullSurfaces(pMaterials);
      break;

     case ShowVertexColors:
//      debugShowVertexColors(pMaterials);
      break;

     case ShowDetailLevel:
      debugShowDetailLevel();
      break;

     default:
      AssertWarn(false, "Warning!  Misunderstood debug render mode.  Defaulting to ShowDetail");
      debugShowDetail();
      break;
   }

   glDisableClientState(GL_VERTEX_ARRAY);
}

void Interior::debugNormalRenderLines()
{
   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound = U32(-1);
   U32 currentTexGen  = U32(-1);

   // Base textures
   glBlendFunc(GL_ONE, GL_ZERO);
   glDisable(GL_TEXTURE_2D);
   glColor3f(1, 0, 1);
   for (U32 i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
      lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
   }
   glEnable(GL_TEXTURE_2D);
}


void Interior::debugShowDetail()
{
   glDisable(GL_TEXTURE_2D);

   glBlendFunc(GL_ONE, GL_ZERO);
   for (U32 i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      if (rSurface.surfaceFlags & SurfaceDetail)
         glColor3f(1.0f, 0, 0);
      else {
         if (smFocusedDebug == true)
            continue;
         else
            glColor3f(1.0f, 1.0f, 1.0f);
      }

      glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   if (smFocusedDebug == false) {
      glColor3f(0, 0, 0);
      for (U32 i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
      }
   }

   glEnable(GL_TEXTURE_2D);
}


void Interior::debugShowAmbiguous()
{
   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);

   for (U32 i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      if (rSurface.surfaceFlags & SurfaceAmbiguous)
         glColor3f(0, 1.0f, 0);
      else {
         if (smFocusedDebug == true)
            continue;
         else
            glColor3f(1.0f, 1.0f, 1.0f);
      }

      glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   if (smFocusedDebug == false) {
      glColor3f(0, 0, 0);
      for (U32 i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
      }
   }

   glEnable(GL_TEXTURE_2D);
}


void Interior::debugShowLightmaps(LM_HANDLE instanceHandle)
{
   glBlendFunc(GL_ONE, GL_ZERO);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   U32 currentlyBound = U32(-1);

   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      if (mNormalLMapIndices[sgActivePolyList[i]] != currentlyBound) {
         glBindTexture(GL_TEXTURE_2D, gInteriorLMManager.getHandle(mLMHandle, instanceHandle, mNormalLMapIndices[sgActivePolyList[i]])->getGLName());
         currentlyBound = mNormalLMapIndices[sgActivePolyList[i]];
      }

      // Draw the poly
      glBegin(GL_TRIANGLE_STRIP);
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
         glTexCoord2f(mLMTexGenEQs[sgActivePolyList[i]].planeX.distToPlane(mPoints[mWindings[j]].point),
                      mLMTexGenEQs[sgActivePolyList[i]].planeY.distToPlane(mPoints[mWindings[j]].point));
         glVertex3fv(mPoints[mWindings[j]].point);
      }
      glEnd();
   }

   glDisable(GL_TEXTURE_2D);
   glColor3f(0, 0, 0);
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
   }
   glEnable(GL_TEXTURE_2D);
}


void Interior::debugShowPortalZones()
{
   static U8 colors[14][3] = {
      { 0xFF, 0xFF, 0xFF },
      { 0x00, 0x00, 0xFF },
      { 0x00, 0xFF, 0x00 },
      { 0xFF, 0x00, 0x00 },
      { 0xFF, 0xFF, 0x00 },
      { 0xFF, 0x00, 0xFF },
      { 0x00, 0xFF, 0xFF },
      { 0x80, 0x80, 0x80 },
      { 0xFF, 0x80, 0x80 },
      { 0x80, 0xFF, 0x80 },
      { 0x80, 0x80, 0xFF },
      { 0x80, 0xFF, 0xFF },
      { 0xFF, 0x80, 0xFF },
      { 0xFF, 0x80, 0x80 }
   };
	
   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);

   for (U32 i = 0; i < mZones.size(); i++) {
      U8* color;
      if (i == 0)
         color = colors[0];
      else
         color = colors[(i % 13) + 1];

      for (U32 j = mZones[i].surfaceStart; j < mZones[i].surfaceStart + mZones[i].surfaceCount; j++) {
         const Surface& rSurface = mSurfaces[mZoneSurfaces[j]];

         glColor3ub(color[0], color[1], color[2]);
         glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
         glColor3ub(0, 0, 0);
         lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
      }
   }

   glEnable(GL_TEXTURE_2D);
   debugRenderPortals();
}

void Interior::debugRenderPortals()
{
   //-------------------------------------- Render portals...
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   for (U32 i = 0; i < mPortals.size(); i++) {
      const Portal& rPortal = mPortals[i];

      for (U16 j = 0; j < rPortal.triFanCount; j++) {
         const TriFan& rFan = mWindingIndices[rPortal.triFanStart + j];
         U32 k;

         glColor4f(0.75, 0.5, 0.75, 0.45);
         glBegin(GL_TRIANGLE_FAN);
         for (k = 0; k < rFan.windingCount; k++)
            glVertex3fv(mPoints[mWindings[rFan.windingStart + k]].point);
         glEnd();
         glColor4f(0, 0, 1, 1);
         glBegin(GL_LINE_LOOP);
         for (k = 0; k < rFan.windingCount; k++)
            glVertex3fv(mPoints[mWindings[rFan.windingStart + k]].point);
         glEnd();
      }
   }
}

void Interior::debugShowCollisionFans()
{
   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);

   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      U32 numIndices;
      U32 fanIndices[32];
      collisionFanFromSurface(rSurface, fanIndices, &numIndices);

      glColor3f(1.0f, 1.0f, 1.0f);
      glDrawElements(GL_TRIANGLE_FAN, numIndices, GL_UNSIGNED_INT, fanIndices);
   }

   glColor3f(0, 0, 0);
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      U32 numIndices;
      U32 fanIndices[32];
      collisionFanFromSurface(rSurface, fanIndices, &numIndices);
      glBegin(GL_LINE_LOOP);
      for (U32 j = 0; j < numIndices; j++)
         glVertex3fv(mPoints[fanIndices[j]].point);
      glEnd();
   }

   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      U32 numIndices;
      U32 fanIndices[32];
      collisionFanFromSurface(rSurface, fanIndices, &numIndices);

      glColor3f(1, 0, 0);
      glBegin(GL_LINES);
      for (U32 j = 0; j < numIndices; j++) {
         Point3F up   = mPoints[fanIndices[j]].point;
         Point3F norm = getPlane(rSurface.planeIndex);
         if (planeIsFlipped(rSurface.planeIndex))
            up -= norm * 0.4;
         else
            up += norm * 0.4;

         glVertex3fv(mPoints[fanIndices[j]].point);
         glVertex3fv(up);
      }
      glEnd();
   }

   glEnable(GL_TEXTURE_2D);
}

void Interior::debugShowOrphan()
{
   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);

   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      if (rSurface.surfaceFlags & SurfaceOrphan)
         glColor3f(0.0f, 0.0f, 1.0f);
      else {
         if (smFocusedDebug == true)
            continue;
         else
            glColor3f(1.0f, 1.0f, 1.0f);
      }

      glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   if (smFocusedDebug == false) {
      glColor3f(0, 0, 0);
      for (i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
      }
   }

   glEnable(GL_TEXTURE_2D);
}

void Interior::debugShowOrphansFinish()
{
   
}

void Interior::debugShowStrips()
{
   static U8 colors[14][3] = {
      { 0xFF, 0xFF, 0xFF },
      { 0x00, 0x00, 0xFF },
      { 0x00, 0xFF, 0x00 },
      { 0xFF, 0x00, 0x00 },
      { 0xFF, 0xFF, 0x00 },
      { 0xFF, 0x00, 0xFF },
      { 0x00, 0xFF, 0xFF },
      { 0x80, 0x80, 0x80 },
      { 0xFF, 0x80, 0x80 },
      { 0x80, 0xFF, 0x80 },
      { 0x80, 0x80, 0xFF },
      { 0x80, 0xFF, 0xFF },
      { 0xFF, 0x80, 0xFF },
      { 0xFF, 0x80, 0x80 }
   };

   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);

   U32 color = 0;
   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      glColor3ub(colors[sgActivePolyList[i]%14][0],
                 colors[sgActivePolyList[i]%14][1],
                 colors[sgActivePolyList[i]%14][2]);
      color++;
      glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   glColor3f(0, 0, 0);
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
      lineStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
   }

   glEnable(GL_TEXTURE_2D);
}


void Interior::debugShowNullSurfaces(MaterialList* pMaterials)
{
   glBlendFunc(GL_ONE, GL_ZERO);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   // Base textures
   U32 currentlyBound = U32(-1);
   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound) {
         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound = baseName;
      }

      // Draw the poly
      glBegin(GL_TRIANGLE_STRIP);
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
         glTexCoord2f(mTexGenEQs[rSurface.texGenIndex].planeX.distToPlane(mPoints[mWindings[j]].point),
                      mTexGenEQs[rSurface.texGenIndex].planeY.distToPlane(mPoints[mWindings[j]].point));
         glVertex3fv(mPoints[mWindings[j]].point);
      }
      glEnd();
   }

   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);

   glColor3f(1, 0, 0);
   for (i = 0; i < mNullSurfaces.size(); i++) {
      const NullSurface& rSurface = mNullSurfaces[i];

      glDrawElements(GL_TRIANGLE_FAN, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   glEnable(GL_TEXTURE_2D);
}

void Interior::debugShowTexturesOnly(MaterialList* pMaterials)
{
   glBlendFunc(GL_ONE, GL_ZERO);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   // Base textures
   U32 currentlyBound = U32(-1);
   U32 i;

   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      if (pMaterials->getMaterial(rSurface.textureIndex).getGLName() != currentlyBound) {
         glBindTexture(GL_TEXTURE_2D, pMaterials->getMaterial(rSurface.textureIndex).getGLName());
         currentlyBound = mMaterialList->getMaterial(rSurface.textureIndex).getGLName();
      }

      // Draw the poly
      glBegin(GL_TRIANGLE_STRIP);
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
         glTexCoord2f(mTexGenEQs[rSurface.texGenIndex].planeX.distToPlane(mPoints[mWindings[j]].point),
                      mTexGenEQs[rSurface.texGenIndex].planeY.distToPlane(mPoints[mWindings[j]].point));
         glVertex3fv(mPoints[mWindings[j]].point);
      }
      glEnd();
   }
}

void Interior::debugShowLargeTextures(MaterialList* pMaterials)
{
   glBlendFunc(GL_ONE, GL_ZERO);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   // Base textures
   U32 currentlyBound = U32(-1);
   U32 currentTexGen  = U32(-1);
   U32 i;

   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound) {
         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound = baseName;

         U32 width  = pMaterials->getMaterial(rSurface.textureIndex).getWidth();
         U32 height = pMaterials->getMaterial(rSurface.textureIndex).getHeight();
         if (width >= 256 || height >= 256) {
            if (width == 256 && height == 256) {
               // small large
               glColor3f(0.25, 0.25, 1);
            } else if (width != 512 || height != 512) {
               // thin large
               glColor3f(0.25, 1, 0.25);
            } else {
               // oh god.
               glColor3f(1, 0.25, 0.25);
            }
         } else {
            glColor3f(0.35, 0.35, 0.35);
         }
      }

      // Draw the poly
      glBegin(GL_TRIANGLE_STRIP);
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
         glTexCoord2f(mTexGenEQs[rSurface.texGenIndex].planeX.distToPlane(mPoints[mWindings[j]].point),
                      mTexGenEQs[rSurface.texGenIndex].planeY.distToPlane(mPoints[mWindings[j]].point));
         glVertex3fv(mPoints[mWindings[j]].point);
      }
      glEnd();
   }
}


void Interior::debugShowOutsideVisible()
{
   glDisable(GL_TEXTURE_2D);

   glBlendFunc(GL_ONE, GL_ZERO);
   for (U32 i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      if (rSurface.surfaceFlags & SurfaceOutsideVisible)
         glColor3f(1.0f, 0, 0);
      else {
         if (smFocusedDebug == true)
            continue;
         else
            glColor3f(1.0f, 1.0f, 1.0f);
      }

      glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   if (smFocusedDebug == false) {
      glColor3f(0, 0, 0);
      for (U32 i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
      }
   }

   glEnable(GL_TEXTURE_2D);
}

void Interior::debugShowHullSurfaces()
{
   glDisable(GL_TEXTURE_2D);

   static U8 colors[14][3] = {
      { 0xFF, 0xFF, 0xFF },
      { 0x00, 0x00, 0xFF },
      { 0x00, 0xFF, 0x00 },
      { 0xFF, 0x00, 0x00 },
      { 0xFF, 0xFF, 0x00 },
      { 0xFF, 0x00, 0xFF },
      { 0x00, 0xFF, 0xFF },
      { 0x80, 0x80, 0x80 },
      { 0xFF, 0x80, 0x80 },
      { 0x80, 0xFF, 0x80 },
      { 0x80, 0x80, 0xFF },
      { 0x80, 0xFF, 0xFF },
      { 0xFF, 0x80, 0xFF },
      { 0xFF, 0x80, 0x80 }
   };
   U32 color = 0;

   glBlendFunc(GL_ONE, GL_ZERO);
   for (U32 i = 0; i < mConvexHulls.size(); i++) {
      const ConvexHull& rHull = mConvexHulls[i];
      for (U32 j = rHull.surfaceStart; j < rHull.surfaceCount + rHull.surfaceStart; j++) {
         U32 index = mHullSurfaceIndices[j];
         if (isNullSurfaceIndex(index)) {
         } else {
            const Interior::Surface& rSurface = mSurfaces[index];
            U32 fanVerts[32];
            U32 numVerts;
            collisionFanFromSurface(rSurface, fanVerts, &numVerts);

            glColor3ub(colors[(i%13)+1][0], colors[(i%13)+1][1], colors[(i%13)+1][2]);
            color++;
            Point3F center(0, 0, 0);
            glBegin(GL_TRIANGLE_FAN);
            for (U32 k = 0; k < numVerts; k++) {
               glVertex3fv(mPoints[fanVerts[k]].point);
               center += mPoints[fanVerts[k]].point;
            }
            glEnd();
            center /= F32(numVerts);
            glColor3f(0, 0, 0);
            lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);

            PlaneF plane;
            plane.set(mPoints[fanVerts[0]].point, mPoints[fanVerts[1]].point, mPoints[fanVerts[2]].point);
            glBegin(GL_LINES);
               glVertex3fv(center);
               glVertex3fv(center + (plane * 0.25));
            glEnd();
         }
      }
   }

   glEnable(GL_TEXTURE_2D);
}

void Interior::debugShowVehicleHullSurfaces(MaterialList* pMaterials)
{
   glBlendFunc(GL_ONE, GL_ZERO);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   // Base textures
   U32 currentlyBound = U32(-1);
   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound) {
         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound = baseName;
      }

      // Draw the poly
      glBegin(GL_TRIANGLE_STRIP);
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
         glTexCoord2f(mTexGenEQs[rSurface.texGenIndex].planeX.distToPlane(mPoints[mWindings[j]].point),
                      mTexGenEQs[rSurface.texGenIndex].planeY.distToPlane(mPoints[mWindings[j]].point));
         glVertex3fv(mPoints[mWindings[j]].point);
      }
      glEnd();
   }

   glDisable(GL_TEXTURE_2D);
   glBlendFunc(GL_ONE, GL_ZERO);
   glVertexPointer(3, GL_FLOAT, sizeof(ItrPaddedPoint), mVehiclePoints.address());

   glColor3f(1, 0, 0);
   for (i = 0; i < mVehicleNullSurfaces.size(); i++) {
      const NullSurface& rSurface = mNullSurfaces[i];

      glDrawElements(GL_TRIANGLE_FAN, rSurface.windingCount, GL_UNSIGNED_INT, &mVehicleWindings[rSurface.windingStart]);
   }

   glEnable(GL_TEXTURE_2D);
}


// void Interior::debugShowVertexColors(MaterialList* /*pMaterials*/)
// {
//    glDisable(GL_TEXTURE_2D);
//    glBlendFunc(GL_ONE, GL_ZERO);

//    for (U32 i = 0; i < sgActivePolyListSize; i++) {
//       const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

//       glBegin(GL_TRIANGLE_STRIP);
//       for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++)
//       {
//          const ItrPaddedPoint& rPoint = mPoints[mWindings[j]];
//          glColor3ub(mVertexColorsNormal[j].red,
//                     mVertexColorsNormal[j].green,
//                     mVertexColorsNormal[j].blue);
//          glVertex3fv(rPoint.point);
//       }
//       glEnd();
//    }

//    if (smFocusedDebug == false) {
//       glColor3f(0, 0, 0);
//       for (U32 i = 0; i < sgActivePolyListSize; i++) {
//          const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
//          lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
//       }
//    }
    
//    glEnable(GL_TEXTURE_2D);
// }


void Interior::debugShowDetailLevel()
{
   static U8 colors[14][3] = {
      { 0xFF, 0xFF, 0xFF },
      { 0x00, 0x00, 0xFF },
      { 0x00, 0xFF, 0x00 },
      { 0xFF, 0x00, 0x00 },
      { 0xFF, 0xFF, 0x00 },
      { 0xFF, 0x00, 0xFF },
      { 0x00, 0xFF, 0xFF },
      { 0x80, 0x80, 0x80 },
      { 0xFF, 0x80, 0x80 },
      { 0x80, 0xFF, 0x80 },
      { 0x80, 0x80, 0xFF },
      { 0x80, 0xFF, 0xFF },
      { 0xFF, 0x80, 0xFF },
      { 0xFF, 0x80, 0x80 }
   };

   glDisable(GL_TEXTURE_2D);

   glBlendFunc(GL_ONE, GL_ZERO);
   for (U32 i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      glColor3ubv(colors[getDetailLevel()]);
      glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
   }

   if (smFocusedDebug == false) {
      glColor3f(0, 0, 0);
      for (U32 i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         lineLoopFromStrip(mPoints, mWindings, rSurface.windingStart, rSurface.windingCount);
      }
   }

   glEnable(GL_TEXTURE_2D);
}


