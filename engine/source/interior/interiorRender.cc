//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interior.h"
#include "sceneGraph/sceneState.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/lightManager.h"

#include "dgl/dgl.h"
#include "dgl/gBitmap.h"
#include "dgl/gTexManager.h"
#include "math/mMatrix.h"
#include "math/mRect.h"
#include "dgl/materialList.h"
#include "dgl/materialPropertyMap.h"
#include "interior/interiorSubObject.h"
#include "core/bitVector.h"
#include "dgl/stripCache.h"
#include "platform/profiler.h"

//!!!!!!!TBD -- there should be a platform fn called memMove!
#include <string.h>


extern bool sgFogActive;
extern U16* sgActivePolyList;
extern U32  sgActivePolyListSize;
extern U16* sgFogPolyList;
extern U32  sgFogPolyListSize;
extern U16* sgEnvironPolyList;
extern U32  sgEnvironPolyListSize;

Point3F sgOSCamPosition;


#include "interior/itf.h" // render data structs and processing fns.

OutputPoint sgRenderBuffer[512];
U32         sgRenderIndices[2048];
U32         csgNumAllowedPoints = 256;

extern "C" {
   F32   texGen0[8];
   F32   texGen1[8];
   Point2F *fogCoordinatePointer;
}

void emitPrimitive(OutputPoint*                  renderBuffer,
                   U32*                          renderIndices,
                   const U32*                    winding,
                   const U32                     numPoints,
                   const U32                     offset,
                   ItrPaddedPoint*               srcPoints)
{
   U32 currIndex = 0;
   U32 last = 2;
   while (last < numPoints) {
      // First
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 0;
      last++;

      if (last == numPoints)
         break;

      // Second
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 0;
      last++;
   }

   processTriFan(renderBuffer,
                 srcPoints,
                 winding,
                 numPoints);
}

void emitPrimitiveFC_VB(OutputPointFC_VB*	renderBuffer,
                        U32*					renderIndices,
                        const U32*			winding,
                        const U32			numPoints,
                        const U32			offset,
                        ItrPaddedPoint*	srcPoints)
{
   U32 currIndex = 0;
   U32 last = 2;
   while (last < numPoints) {
      // First
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 0;
      last++;

      if (last == numPoints)
         break;

      // Second
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 0;
      last++;
   }

   processTriFanFC_VB(renderBuffer,
                      srcPoints,
                      winding,
                      numPoints);
}

void emitPrimitiveSP(OutputPoint*                  renderBuffer,
                     U32*                          renderIndices,
                     const U32*                    winding,
                     const U32                     numPoints,
                     const U32                     offset,
                     ItrPaddedPoint*               srcPoints,
                     const ColorI*                 srcColors)
{
   U32 currIndex = 0;
   U32 last = 2;
   while (last < numPoints) {
      // First
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 0;
      last++;

      if (last == numPoints)
         break;

      // Second
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 0;
      last++;
   }

   processTriFanSP(renderBuffer,
                   srcPoints,
                   winding, numPoints,
                   srcColors);
}


void emitPrimitiveVC_TF(OutputPoint*                  renderBuffer,
                        U32*                          renderIndices,
                        const U32*                    winding,
                        const U32                     numPoints,
                        const U32                     offset,
                        ItrPaddedPoint*               srcPoints,
                        const ColorI*                 srcColors)
{
   U32 currIndex = 0;
   U32 last = 2;
   while (last < numPoints) {
      // First
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 0;
      last++;

      if (last == numPoints)
         break;

      // Second
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 0;
      last++;
   }

   processTriFanVC_TF(renderBuffer,
                      srcPoints,
                      winding,
                      numPoints,
                      srcColors);
}

void emitPrimitiveSP_FC(OutputPoint*                  renderBuffer,
                        U32*                          renderIndices,
                        const U32*                    winding,
                        const U32                     numPoints,
                        const U32                     offset,
                        ItrPaddedPoint*               srcPoints,
                        const ColorI*                 srcColors)
{
   U32 currIndex = 0;
   U32 last = 2;
   while (last < numPoints) {
      // First
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 0;
      last++;

      if (last == numPoints)
         break;

      // Second
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 0;
      last++;
   }

   processTriFanSP_FC(renderBuffer,
                      srcPoints,
                      winding, numPoints,
                      srcColors);
}

void emitPrimitiveSP_FC_VB(OutputPointSP_FC_VB*	renderBuffer,
                           U32*			renderIndices,
                           const U32*		winding,
                           const U32		numPoints,
                           const U32		offset,
                           ItrPaddedPoint*	srcPoints,
                           const ColorI*        srcColors)
{
   U32 currIndex = 0;
   U32 last = 2;
   while (last < numPoints) {
      // First
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 0;
      last++;

      if (last == numPoints)
         break;

      // Second
      renderIndices[currIndex++] = offset + last - 1;
      renderIndices[currIndex++] = offset + last - 2;
      renderIndices[currIndex++] = offset + last - 0;
      last++;
   }

   processTriFanSP_FC_VB(renderBuffer,
                         srcPoints,
                         winding,
                         numPoints,
                         srcColors);
}

void flushPrimitives(const U32* indices,
                     const U32  count,
                     const U32  vcount)
{
   if (count == 0)
      return;

   if (Interior::smLockArrays && dglDoesSupportCompiledVertexArray()) {
      glLockArraysEXT(0, vcount);
      glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
      glUnlockArraysEXT();
   } else {
      glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
   }
}

void flushPrimitivesVB(const U32* indices,
                       const U32  count,
                       const U32  vcount,
                       const S32  handle)
{
   if (count == 0)
      return;

   glSetVertexBufferEXT(handle);

   if (Interior::smLockArrays && dglDoesSupportCompiledVertexArray()) {
      glLockArraysEXT(0, vcount);
      glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
      glUnlockArraysEXT();
   } else {
      glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
   }
}

//} // namespace {}

//--------------------------------------------------------------------------
void Interior::setupFog(SceneState* state)
{
   if (sgFogActive) {
      if (useFogCoord()) {
         glEnable(GL_FOG);
         glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
         GLfloat fogColor[4];
         fogColor[0] = state->getFogColor().red;
         fogColor[1] = state->getFogColor().green;
         fogColor[2] = state->getFogColor().blue;
         fogColor[3] = 1.0f;
         glFogfv(GL_FOG_COLOR, fogColor);
         glFogi(GL_FOG_MODE, GL_LINEAR);
         glFogf(GL_FOG_START, 0.0f);
         glFogf(GL_FOG_END, 1.0f);
      }
   }
}

void Interior::clearFog()
{
   if (sgFogActive) {
      if (useFogCoord()) {
         glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FRAGMENT_DEPTH_EXT);
         glDisable(GL_FOG);
      }
   }
}

void Interior::setOSCamPosition(const Point3F& pos)
{
   sgOSCamPosition = pos;
}


//------------------------------------------------------------------------------
void Interior::render(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle,
                      const Vector<ColorI>* normalVLights,
                      const Vector<ColorI>* alarmVLights)
{
	glDisable(GL_LIGHTING);

   if (smRenderMode != 0) {
      PROFILE_START(IRO_DebugRender);
      debugRender(pMaterials, instanceHandle);
      PROFILE_END();
      return;
   }

   if (dglDoesSupportARBMultitexture())
   {
      if (smUseVertexLighting == false)
      {
         if (useFogCoord())
         {
            PROFILE_START(IRO_RenderARB_FC);
            renderARB_FC(useAlarmLighting, pMaterials, instanceHandle);
            PROFILE_END();
         }
         else
         {
            PROFILE_START(IRO_RenderARB);
            renderARB(useAlarmLighting, pMaterials, instanceHandle);
            PROFILE_END();
         }
      }
      else
      {
         if (useFogCoord())
         {
            PROFILE_START(IRO_Render_VC_FC);
            render_vc_fc(useAlarmLighting, pMaterials, instanceHandle, normalVLights, alarmVLights);
            PROFILE_END();
         }
         else
         {
            PROFILE_START(IRO_Render_VC_TF);
            render_vc_tf(useAlarmLighting, pMaterials, instanceHandle, normalVLights, alarmVLights);
            PROFILE_END();
         }
      }
   }
   else
   {
      if (useFogCoord())
      {
         PROFILE_START(IRO_Render_VC_FC);
         render_vc_fc(useAlarmLighting, pMaterials, instanceHandle, normalVLights, alarmVLights);
         PROFILE_END();
      }
      else
      {
         PROFILE_START(Render_VC_TF);
         render_vc_tf(useAlarmLighting, pMaterials, instanceHandle, normalVLights, alarmVLights);
         PROFILE_END();
      }
   }
}


//------------------------------------------------------------------------------
void Interior::render_vc_tf(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE,
                            const Vector<ColorI>* normalVLights,
                            const Vector<ColorI>* alarmVLights)
{
   const Vector<ColorI>* pVertexColors = useAlarmLighting ? alarmVLights : normalVLights;

   OutputPoint* pFirstOutputPoint = (OutputPoint*)sgRenderBuffer;
   U32 currRenderBufferPoint = 0;
   U32 currIndexPoint        = 0;

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glBlendFunc(GL_ONE, GL_ZERO);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->point);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->texCoord);
   glEnableClientState(GL_COLOR_ARRAY);
   glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OutputPoint), &pFirstOutputPoint->fogColor[0]);
   
   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound = U32(-1);
   U32 currentTexGen  = U32(-1);
   
   // Draw the polys!
   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      // Setup the base texture...
      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound = baseName;
      }

      if (currRenderBufferPoint + rSurface.windingCount >= 512 ||
          currIndexPoint        + (rSurface.windingCount - 2) * 3 >= 2048) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;
      }

      if (rSurface.texGenIndex != currentTexGen)
      {
         currentTexGen = rSurface.texGenIndex;
         memcpy(texGen0,  &mTexGenEQs[rSurface.texGenIndex], sizeof(F32)*8);
      }
      
      emitPrimitiveSP(&sgRenderBuffer[currRenderBufferPoint],
                      &sgRenderIndices[currIndexPoint],
                      &mWindings[rSurface.windingStart],
                      rSurface.windingCount,
                      currRenderBufferPoint,
                      &mPoints[0],
                      &((*pVertexColors)[rSurface.windingStart]));
      currRenderBufferPoint += rSurface.windingCount;
      currIndexPoint        += (rSurface.windingCount - 2) * 3;
      AssertFatal(currRenderBufferPoint < 512 && currIndexPoint < 2048, "Aw, crap.1");
   }
   flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
   currIndexPoint        = 0;
   currRenderBufferPoint = 0;

   glDisableClientState(GL_COLOR_ARRAY);
   if (sgFogActive) {
      extern Point2F* sgFogTexCoords;
      glVertexPointer(3, GL_FLOAT, sizeof(ItrPaddedPoint), &mPoints[0].point);
      glTexCoordPointer(2, GL_FLOAT, sizeof(Point2F), sgFogTexCoords);

      glBindTexture(GL_TEXTURE_2D, gClientSceneGraph->getFogTexture().getGLName());
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      extern ColorF gInteriorFogColor;
      glColor4f(gInteriorFogColor.red,
                gInteriorFogColor.green,
                gInteriorFogColor.blue,
                1);

      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(-1,-1);

      if (dglDoesSupportCompiledVertexArray())
         glLockArraysEXT(0, mPoints.size());
      for (U32 i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
      }
      if (dglDoesSupportCompiledVertexArray())
         glUnlockArraysEXT();

      glDisable(GL_POLYGON_OFFSET_FILL);
   }
   
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}


//------------------------------------------------------------------------------
void Interior::render_vc_fc(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE,
                            const Vector<ColorI>* normalVLights,
                            const Vector<ColorI>* alarmVLights)
   
{
   const Vector<ColorI>* pVertexColors = useAlarmLighting ? alarmVLights : normalVLights;

   OutputPoint* pFirstOutputPoint = (OutputPoint*)sgRenderBuffer;
   U32 currRenderBufferPoint = 0;
   U32 currIndexPoint        = 0;
   bool supportBuffers;
   GLint bufferHandle;
   OutputPointSP_FC_VB *vertexBuffer = NULL;

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glBlendFunc(GL_ONE, GL_ZERO);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->point);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->texCoord);
   glEnableClientState(GL_COLOR_ARRAY);
   glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OutputPoint), &pFirstOutputPoint->fogColor[0]);
   if (sgFogActive) {
      // Need the fog coord pointer enabled here...
      glEnableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
      glFogCoordPointerEXT(GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->lmCoord.x);
   }
   
   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound = U32(-1);
   U32 currentTexGen  = U32(-1);

   if ((supportBuffers = dglDoesSupportVertexBuffer()) == true)
   {
      bufferHandle = gInteriorLMManager.getVertexBuffer(GL_V12FTVFMT_EXT);
      // guess we ran out of video memory
      if (bufferHandle == -1)
         supportBuffers = false;
   }
   
   // Draw the polys!
   U32 i;
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      // Setup the base texture...
      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound) {
         if (supportBuffers)
         {
            if (vertexBuffer)
               glUnlockVertexBufferEXT(bufferHandle);
            flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
            vertexBuffer = (OutputPointSP_FC_VB *) glLockVertexBufferEXT(bufferHandle,512);
         }
         else
            flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound = baseName;
      }

      if (currRenderBufferPoint + rSurface.windingCount >= 512 ||
          currIndexPoint        + (rSurface.windingCount - 2) * 3 >= 2048) {
         if (supportBuffers)
         {
            if (vertexBuffer)
               glUnlockVertexBufferEXT(bufferHandle);
            flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
            vertexBuffer = (OutputPointSP_FC_VB *) glLockVertexBufferEXT(bufferHandle,512);
         }
         else
            flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;
      }

      if (rSurface.texGenIndex != currentTexGen)
      {
         currentTexGen = rSurface.texGenIndex;
         memcpy(texGen0,  &mTexGenEQs[rSurface.texGenIndex], sizeof(F32)*8);
      }
      
      if (supportBuffers)
         emitPrimitiveSP_FC_VB(&vertexBuffer[currRenderBufferPoint],
                               &sgRenderIndices[currIndexPoint],
                               &mWindings[rSurface.windingStart],
                               rSurface.windingCount,
                               currRenderBufferPoint,
                               &mPoints[0],
                               &((*pVertexColors)[rSurface.windingStart]));
      else
         emitPrimitiveSP_FC(&sgRenderBuffer[currRenderBufferPoint],
                            &sgRenderIndices[currIndexPoint],
                            &mWindings[rSurface.windingStart],
                            rSurface.windingCount,
                            currRenderBufferPoint,
                            &mPoints[0],
                            &((*pVertexColors)[rSurface.windingStart]));
      currRenderBufferPoint += rSurface.windingCount;
      currIndexPoint        += (rSurface.windingCount - 2) * 3;
      AssertFatal(currRenderBufferPoint < 512 && currIndexPoint < 2048, "Aw, crap.2");
   }
   if (supportBuffers)
   {
      if (vertexBuffer)
         glUnlockVertexBufferEXT(bufferHandle);
      flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
   }
   else
      flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
   currIndexPoint        = 0;
   currRenderBufferPoint = 0;

   if (sgFogActive) {
      // Need the fog coord pointer enabled here...
      glDisableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
   }
   
   glDisableClientState(GL_COLOR_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}


void Interior::renderARB_vc_tf(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE,
                               const Vector<ColorI>* normalVLights,
                               const Vector<ColorI>* alarmVLights)
   
{
   // DMMNOTE: NON-FUNCTIONAL ON THE GEFORCE!  DO NOT PUT IN RENDER FUNCTION!!!
   // DMMNOTE: NON-FUNCTIONAL ON THE GEFORCE!  DO NOT PUT IN RENDER FUNCTION!!!
   // DMMNOTE: NON-FUNCTIONAL ON THE GEFORCE!  DO NOT PUT IN RENDER FUNCTION!!!
   U32 i;
   const Vector<ColorI>* pVertexColors = useAlarmLighting ? alarmVLights : normalVLights;

   extern Point2F* sgFogTexCoords;
   fogCoordinatePointer = sgFogTexCoords;
   
   OutputPoint* pFirstOutputPoint = (OutputPoint*)sgRenderBuffer;
   U32 currRenderBufferPoint = 0;
   U32 currIndexPoint        = 0;

   // Base textures
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   glBindTexture(GL_TEXTURE_2D, gClientSceneGraph->getFogTexture().getGLName());
   glActiveTextureARB(GL_TEXTURE0_ARB);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->point);
   glEnableClientState(GL_COLOR_ARRAY);
   glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(OutputPoint), &pFirstOutputPoint->fogColor[0]);

//    // Fog texture
//    if (sgFogActive)
//    {
   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->texCoord);
//      glClientActiveTextureARB(GL_TEXTURE0_ARB);
//    }
//    else
//    {
//       glActiveTextureARB(GL_TEXTURE1_ARB);
//       glDisable(GL_TEXTURE_2D);

//       glClientActiveTextureARB(GL_TEXTURE1_ARB);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//    }
       
   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->lmCoord);

   // Misc setup
   glBlendFunc(GL_ONE, GL_ZERO);
   glActiveTextureARB(GL_TEXTURE0_ARB);

   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound = U32(-1);
   U32 currentTexGen   = U32(-1);
   
   // Draw the polys!
   glColor4f(1, 1, 1, 1);
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      // Setup the base texture...
      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound = baseName;
      }

      if (currRenderBufferPoint + rSurface.windingCount >= 512 ||
          currIndexPoint        + (rSurface.windingCount - 2) * 3 >= 2048) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;
      }
      if (rSurface.texGenIndex != currentTexGen)
      {
         currentTexGen = rSurface.texGenIndex;
         memcpy(texGen0,  &mTexGenEQs[rSurface.texGenIndex], sizeof(F32)*8);
      }

      emitPrimitiveVC_TF(&sgRenderBuffer[currRenderBufferPoint],
                         &sgRenderIndices[currIndexPoint],
                         &mWindings[rSurface.windingStart],
                         rSurface.windingCount,
                         currRenderBufferPoint,
                         &mPoints[0],
                         &((*pVertexColors)[rSurface.windingStart]));
      currRenderBufferPoint += rSurface.windingCount;
      currIndexPoint        += (rSurface.windingCount - 2) * 3;
      AssertFatal(currRenderBufferPoint < 512 && currIndexPoint < 2048, "Aw, crap.3");
   }
   flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
   currIndexPoint        = 0;
   currRenderBufferPoint = 0;

   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);

   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}


void Interior::renderARB(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle)
{
   U32 i;
   Vector<U8>* pLMapIndices = useAlarmLighting ? &mAlarmLMapIndices : &mNormalLMapIndices;

   OutputPoint* pFirstOutputPoint = (OutputPoint*)sgRenderBuffer;
   U32 currRenderBufferPoint = 0;
   U32 currIndexPoint        = 0;

   // Lightmaps
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   // Base textures
   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   // Misc setup
   glBlendFunc(GL_ONE, GL_ZERO);
   glActiveTextureARB(GL_TEXTURE0_ARB);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->point);

   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->texCoord);

   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->lmCoord);

   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound0 = U32(-1);
   U32 currentlyBound1 = U32(-1);
   U32 currentTexGen   = U32(-1);
   
   // Draw the polys!
   glColor4f(1, 1, 1, 1);
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      // Setup the base texture...
      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound1) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         glActiveTextureARB(GL_TEXTURE1_ARB);
         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound1 = baseName;
         glActiveTextureARB(GL_TEXTURE0_ARB);
      }

      // Setup the lightmap
      baseName = (*pLMapIndices)[sgActivePolyList[i]];
      if (baseName != currentlyBound0) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         U32 glName = gInteriorLMManager.getHandle(mLMHandle, instanceHandle, baseName)->getGLName();
         AssertFatal(glName, "Interior::renderARB: invalid glName for texture handle");

         glBindTexture(GL_TEXTURE_2D, glName);
         currentlyBound0 = baseName;
      }

      if (currRenderBufferPoint + rSurface.windingCount >= 512 ||
          currIndexPoint        + (rSurface.windingCount - 2) * 3 >= 2048) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;
      }
      if (rSurface.texGenIndex != currentTexGen)
      {
         currentTexGen = rSurface.texGenIndex;
         memcpy(texGen0,  &mTexGenEQs[rSurface.texGenIndex], sizeof(F32)*8);
      }
      memcpy(texGen1, &mLMTexGenEQs[sgActivePolyList[i]], sizeof(F32)*8);

      emitPrimitive(&sgRenderBuffer[currRenderBufferPoint],
                    &sgRenderIndices[currIndexPoint],
                    &mWindings[rSurface.windingStart],
                    rSurface.windingCount,
                    currRenderBufferPoint,
                    &mPoints[0]);
      currRenderBufferPoint += rSurface.windingCount;
      currIndexPoint        += (rSurface.windingCount - 2) * 3;
      AssertFatal(currRenderBufferPoint < 512 && currIndexPoint < 2048, "Aw, crap.4");
   }
   flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
   currIndexPoint        = 0;
   currRenderBufferPoint = 0;

   if (sgFogActive) {
      glVertexPointer(3, GL_FLOAT, sizeof(ItrPaddedPoint), &mPoints[0].point);

      extern Point2F* sgFogTexCoords;
      glClientActiveTextureARB(GL_TEXTURE1_ARB);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);

      glClientActiveTextureARB(GL_TEXTURE0_ARB);
      glTexCoordPointer(2, GL_FLOAT, sizeof(Point2F), sgFogTexCoords);

      glActiveTextureARB(GL_TEXTURE1_ARB);
      glDisable(GL_TEXTURE_2D);

      glActiveTextureARB(GL_TEXTURE0_ARB);
      glBindTexture(GL_TEXTURE_2D, gClientSceneGraph->getFogTexture().getGLName());
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      extern ColorF gInteriorFogColor;
      glColor4f(gInteriorFogColor.red,
                gInteriorFogColor.green,
                gInteriorFogColor.blue,
                1);

      if (dglDoesSupportCompiledVertexArray())
         glLockArraysEXT(0, mPoints.size());
      for (U32 i = 0; i < sgActivePolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgActivePolyList[i]];
         glDrawElements(GL_TRIANGLE_STRIP, rSurface.windingCount, GL_UNSIGNED_INT, &mWindings[rSurface.windingStart]);
      }
      if (dglDoesSupportCompiledVertexArray())
         glUnlockArraysEXT();

      glActiveTextureARB(GL_TEXTURE1_ARB);
      glEnable(GL_TEXTURE_2D);
   }

   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);

   // Render environment maps...
   if (smRenderEnvironmentMaps && sgEnvironPolyListSize) {
      currentlyBound1 = (U32)-1;
      currentlyBound0 = (U32)-1;
 
      // Base textures
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      // Envmaps textures
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
 
      // Misc setup
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glActiveTextureARB(GL_TEXTURE0_ARB);
      
      for (i = 0; i < sgEnvironPolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgEnvironPolyList[i]];
 
         // Setup the base texture...
         U32 baseName = mMaterialList->getMaterial(rSurface.textureIndex).getGLName();
         if (baseName != currentlyBound0) {
            glBindTexture(GL_TEXTURE_2D, baseName);
            currentlyBound0 = baseName;
         }
 
         // Setup the environment map...
         baseName = mEnvironMaps[rSurface.textureIndex]->getGLName();
         if (baseName != currentlyBound1) {
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glBindTexture(GL_TEXTURE_2D, baseName);
            currentlyBound1 = baseName;
            glActiveTextureARB(GL_TEXTURE0_ARB);
         }
 
         const PlaneF& plane = getPlane(rSurface.planeIndex);
         Point3F normal = plane;
         if (planeIsFlipped(rSurface.planeIndex))
            normal.neg();
 
         // And the colors...
         F32 baseLevel = mEnvironFactors[rSurface.textureIndex];
 
         glBegin(GL_TRIANGLE_STRIP);
         for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
            F32 s = mTexGenEQs[rSurface.texGenIndex].planeX.distToPlane(mPoints[mWindings[j]].point);
            F32 t = mTexGenEQs[rSurface.texGenIndex].planeY.distToPlane(mPoints[mWindings[j]].point);

            Point3F u = mPoints[mWindings[j]].point - sgOSCamPosition;
            Point3F r = u;
            F32 dot = mDot(normal, u) * 2.0f;
            u = normal * dot;
            r -= u;
            F32 m = 2 * mSqrt(r.x*r.x + r.y*r.y + (r.z+1)*(r.z+1));
 
            glColor4f(1, 1, 1, baseLevel);
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB, s, t);
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, r.x/m + 0.5,
                                 r.y/m + 0.5);
            glVertex3fv(mPoints[mWindings[j]].point);
         }
         glEnd();
      }
   } // if (environment render on);

   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}


void Interior::renderARB_FC(const bool useAlarmLighting, MaterialList* pMaterials, const LM_HANDLE instanceHandle)
{
   U32 i;
   Vector<U8>* pLMapIndices = useAlarmLighting ? &mAlarmLMapIndices : &mNormalLMapIndices;

   OutputPoint* pFirstOutputPoint = (OutputPoint*)sgRenderBuffer;
   U32 currRenderBufferPoint = 0;
   U32 currIndexPoint        = 0;
   bool supportBuffers;
   GLint bufferHandle;
   OutputPointFC_VB *vertexBuffer = NULL;

   // Lightmaps
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   // Base textures
   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   // Misc setup
   glBlendFunc(GL_ONE, GL_ZERO);
   glActiveTextureARB(GL_TEXTURE0_ARB);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->point);
   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->texCoord);
   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->lmCoord);
   if (sgFogActive) {
      // Need the fog coord pointer enabled here...
      glEnableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
      glFogCoordPointerEXT(GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->fogCoord);
   }

   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound0 = U32(-1);
   U32 currentlyBound1 = U32(-1);
   U32 currentTexGen = U32(-1);
   
   if ((supportBuffers = dglDoesSupportVertexBuffer()) == true)
   {
      bufferHandle = gInteriorLMManager.getVertexBuffer(GL_V12FMTVFMT_EXT);
      // guess we ran out of video memory
      if (bufferHandle == -1)
         supportBuffers = false;
   }

   // Draw the polys!
   glColor4f(1, 1, 1, 1);
   for (i = 0; i < sgActivePolyListSize; i++) {
      const Surface& rSurface = mSurfaces[sgActivePolyList[i]];

      // Setup the base texture...
      U32 baseName = pMaterials->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound1) {
         if (supportBuffers)
         {
            if (vertexBuffer)
               glUnlockVertexBufferEXT(bufferHandle);
            flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
            vertexBuffer = (OutputPointFC_VB *) glLockVertexBufferEXT(bufferHandle,512);
         }
         else
            flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);

         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         glActiveTextureARB(GL_TEXTURE1_ARB);
         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound1 = baseName;
         glActiveTextureARB(GL_TEXTURE0_ARB);				
      }

      // Setup the lightmap
      baseName = (*pLMapIndices)[sgActivePolyList[i]];
      if (baseName != currentlyBound0) {
         if (supportBuffers)
         {
            if (vertexBuffer)
               glUnlockVertexBufferEXT(bufferHandle);
            flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
            vertexBuffer = (OutputPointFC_VB *) glLockVertexBufferEXT(bufferHandle,512);
         }
         else
            flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);

         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         U32 glName = gInteriorLMManager.getHandle(mLMHandle, instanceHandle, baseName)->getGLName();
         AssertFatal(glName, "Interior::renderARB_FC: invalid glName for texture handle");

         glBindTexture(GL_TEXTURE_2D, glName);
         currentlyBound0 = baseName;	
      }

      if (currRenderBufferPoint + rSurface.windingCount >= 512 ||
          currIndexPoint        + (rSurface.windingCount - 2) * 3 >= 2048) {
         if (supportBuffers)
         {
            if (vertexBuffer)
               glUnlockVertexBufferEXT(bufferHandle);
            flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
            vertexBuffer = (OutputPointFC_VB *) glLockVertexBufferEXT(bufferHandle,512);
         }
         else
            flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;
      }
      if (rSurface.texGenIndex != currentTexGen)
      {
         currentTexGen = rSurface.texGenIndex;
         memcpy(texGen0,  &mTexGenEQs[rSurface.texGenIndex], sizeof(F32)*8);
      }
      memcpy(texGen1,  &mLMTexGenEQs[sgActivePolyList[i]], sizeof(F32)*8);

      if (supportBuffers)
         emitPrimitiveFC_VB(&vertexBuffer[currRenderBufferPoint],
                            &sgRenderIndices[currIndexPoint],
                            &mWindings[rSurface.windingStart],
                            rSurface.windingCount,
                            currRenderBufferPoint,
                            &mPoints[0]);
      else
         emitPrimitive(&sgRenderBuffer[currRenderBufferPoint],
                       &sgRenderIndices[currIndexPoint],
                       &mWindings[rSurface.windingStart],
                       rSurface.windingCount,
                       currRenderBufferPoint,
                       &mPoints[0]);
      currRenderBufferPoint += rSurface.windingCount;
      currIndexPoint        += (rSurface.windingCount - 2) * 3;
      AssertFatal(currRenderBufferPoint < 512 && currIndexPoint < 2048, "Aw, crap.5");
   }

   if (supportBuffers)
   {
      if (vertexBuffer)
         glUnlockVertexBufferEXT(bufferHandle);
      flushPrimitivesVB(sgRenderIndices, currIndexPoint, currRenderBufferPoint, bufferHandle);
   }
   else
      flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);

   currIndexPoint        = 0;
   currRenderBufferPoint = 0;

   if (sgFogActive)
      glDisableClientState(GL_FOG_COORDINATE_ARRAY_EXT);
   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);

   // Render environment maps...
   if (smRenderEnvironmentMaps && sgEnvironPolyListSize) {
      glDisable(GL_FOG);
      currentlyBound1 = (U32)-1;
      currentlyBound0 = (U32)-1;
 
      // Base textures
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      // Envmaps textures
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
 
      // Misc setup
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(-1,-1);

      for (i = 0; i < sgEnvironPolyListSize; i++) {
         const Surface& rSurface = mSurfaces[sgEnvironPolyList[i]];
 
         // Setup the base texture...
         U32 baseName = mMaterialList->getMaterial(rSurface.textureIndex).getGLName();
         if (baseName != currentlyBound0) {
            glBindTexture(GL_TEXTURE_2D, baseName);
            currentlyBound0 = baseName;
         }
 
         // Setup the environment map...
         baseName = mEnvironMaps[rSurface.textureIndex]->getGLName();
         if (baseName != currentlyBound1) {
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glBindTexture(GL_TEXTURE_2D, baseName);
            currentlyBound1 = baseName;
            glActiveTextureARB(GL_TEXTURE0_ARB);
         }
 
         const PlaneF& plane = getPlane(rSurface.planeIndex);
         Point3F normal = plane;
         if (planeIsFlipped(rSurface.planeIndex))
            normal.neg();
 
         // And the colors...
         F32 baseLevel = mEnvironFactors[rSurface.textureIndex];
 
         glBegin(GL_TRIANGLE_STRIP);
         for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
            F32 s = mTexGenEQs[rSurface.texGenIndex].planeX.distToPlane(mPoints[mWindings[j]].point);
            F32 t = mTexGenEQs[rSurface.texGenIndex].planeY.distToPlane(mPoints[mWindings[j]].point);

            Point3F u = mPoints[mWindings[j]].point - sgOSCamPosition;
            Point3F r = u;
            F32 dot = mDot(normal, u) * 2.0f;
            u = normal * dot;
            r -= u;
            F32 m = 2 * mSqrt(r.x*r.x + r.y*r.y + (r.z+1)*(r.z+1));
 
            glColor4f(1, 1, 1, baseLevel * (1.0 - mPoints[mWindings[j]].fogCoord));
            glMultiTexCoord2fARB(GL_TEXTURE0_ARB, s, t);
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, r.x/m + 0.5,
                                 r.y/m + 0.5);
            glVertex3fv(mPoints[mWindings[j]].point);
         }
         glEnd();
      }
      glDisable(GL_POLYGON_OFFSET_FILL);
   } // if (environment render on);

   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}


void Interior::renderLights(LightInfo*     pInfo,                       
                            const MatrixF& transform,
                            const Point3F& scale,
                            U32*           lightSurfaces,
                            U32            numLightSurfaces)
{
   Point3F lightPoint = pInfo->mPos;
   transform.mulP(lightPoint);
   lightPoint.convolveInverse(scale);

   if (dglDoesSupportARBMultitexture()) {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glDisable(GL_TEXTURE_2D);
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }

   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, mLightFalloff->getGLName());
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);

   glEnable(GL_POLYGON_OFFSET_FILL);
   glPolygonOffset(-1,-1);

   for (U32 i = 0; i < numLightSurfaces; i++) {
      const Surface& rSurface = mSurfaces[lightSurfaces[i]];

      const PlaneF& plane = getPlane(rSurface.planeIndex);

      Point3F centerPoint;
      F32 d = plane.distToPlane(lightPoint);
      centerPoint = lightPoint - plane * d;
      d = mFabs(d);
      if (d >= pInfo->mRadius)
         continue;

      F32 mr = mSqrt(pInfo->mRadius*pInfo->mRadius - d*d);

      Point3F normalS;
      Point3F normalT;
      if (mFabs(plane.z) < 0.9)
         mCross(plane, Point3F(0, 0, 1), &normalS);
      else
         mCross(plane, Point3F(0, 1, 0), &normalS);
      mCross(plane, normalS, &normalT);
      normalS.normalize();
      normalT.normalize();
      PlaneF splane(centerPoint, normalS);
      PlaneF tplane(centerPoint, normalT);

      F32 factor = (pInfo->mRadius - d) / pInfo->mRadius;
      glColor4f(pInfo->mColor.red, pInfo->mColor.green, pInfo->mColor.blue, factor);

      glBegin(GL_TRIANGLE_STRIP);
      for (U32 j = rSurface.windingStart; j < rSurface.windingStart + rSurface.windingCount; j++) {
         const Point3F& rPoint = mPoints[mWindings[j]].point;

         glTexCoord2f(((splane.distToPlane(rPoint) / mr) + 1.0) / 2.0,
                      ((tplane.distToPlane(rPoint) / mr) + 1.0) / 2.0);
         glVertex3fv(mPoints[mWindings[j]].point);
      }
      glEnd();
   }

   glDisable(GL_POLYGON_OFFSET_FILL);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}

void Interior::renderAsShape()
{
   OutputPoint* pFirstOutputPoint = (OutputPoint*)sgRenderBuffer;
   U32 currRenderBufferPoint = 0;
   U32 currIndexPoint        = 0;

   glEnable(GL_LIGHTING);

   // Base texture:
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glBlendFunc(GL_ONE, GL_ZERO);

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->point);

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(OutputPoint), &pFirstOutputPoint->texCoord);

   // Ok, our verts are set up, draw our polys.
   U32 currentlyBound0 = U32(-1);
   U32 currentlyBound1 = U32(-1);
   U32 currentTexGen   = U32(-1);
   
   // Draw the polys!
   glColor4f(1, 1, 1, 1);
   for (S32 i = 0; i < mSurfaces.size(); i++)
   {
      const Surface& rSurface = mSurfaces[i];
      glNormal3fv(getPlane(rSurface.planeIndex));
      // Setup the base texture...
      U32 baseName = mMaterialList->getMaterial(rSurface.textureIndex).getGLName();
      if (baseName != currentlyBound1) {
         flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
         currIndexPoint        = 0;
         currRenderBufferPoint = 0;

         glBindTexture(GL_TEXTURE_2D, baseName);
         currentlyBound1 = baseName;
      }


      if (rSurface.texGenIndex != currentTexGen)
      {
         currentTexGen = rSurface.texGenIndex;
         memcpy(texGen0,  &mTexGenEQs[rSurface.texGenIndex], sizeof(F32)*8);
      }

      emitPrimitive(&sgRenderBuffer[currRenderBufferPoint],
                    &sgRenderIndices[currIndexPoint],
                    &mWindings[rSurface.windingStart],
                    rSurface.windingCount,
                    currRenderBufferPoint,
                    &mPoints[0]);
      currRenderBufferPoint += rSurface.windingCount;
      currIndexPoint        += (rSurface.windingCount - 2) * 3;
      flushPrimitives(sgRenderIndices, currIndexPoint, currRenderBufferPoint);
      currIndexPoint        = 0;
      currRenderBufferPoint = 0;

      AssertFatal(currRenderBufferPoint < 512 && currIndexPoint < 2048, "Aw, crap.");
   }
   currIndexPoint        = 0;
   currRenderBufferPoint = 0;

   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);

   glDisable(GL_LIGHTING);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}
