//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#include "terrain/terrData.h"
#include "math/mMath.h"
#include "dgl/dgl.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "dgl/gBitmap.h"
#include "terrain/terrRender.h"
#include "dgl/materialList.h"
#include "sceneGraph/sceneState.h"
#include "terrain/waterBlock.h"
#include "terrain/blender.h"
#include "sim/frameAllocator.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sgUtil.h"
#include "platform/profiler.h"

#if ENABLE_NPATCH
#if TERRAIN_STORE_NORMALS
// terrain-local NPatch control.
bool gNPatchTerrain = false;
S32 gNPatchTerrainLevel = -1; // means use global setting.
#endif
#endif

struct LightTriangle {
   ColorF  color;

   Point2F texco1;
   Point3F point1;
   Point2F texco2;
   Point3F point2;
   Point2F texco3;
   Point3F point3;

   LightTriangle* next;
   U32            flags;  // 0 if inactive
};

static LightTriangle* sgCurrLightTris = NULL;


GBitmap* TerrainRender::mBlendBitmap = NULL;

S32 TerrainRender::mTextureMinSquareSize;

bool TerrainRender::mEnableTerrainDetails       = true;
// CW - stuff with bump maps
// So, this is how it works...
// There are 3 fields:
// 1) bumpScale: This is the size of the bumps.  It determines how much the bump texture is stretched
// 2) bumpOffset: How much the two textures are offset from each other.  This should be VERY close to
//    0.  An appropriate value is usually around 0.004 and 0.01.  Play around with it and see what
//    works best for your implementation.
// 3) zeroBumpScale: This is a bitshift value for how far the bumps are drawn.  The LESS the value
//    the FARTHER they will exist.  If you go above 7-8, the bumps tend to go a little wacky.
//    8 should be the max.  6-7 are good values.
bool TerrainRender::mEnableTerrainEmbossBumps   = true;
// This is only for win32
#ifdef TORQUE_OS_WIN32
bool TerrainRender::mRenderGL = false;
#endif
// CW - end bump map stuff
bool TerrainRender::mEnableTerrainDynLights     = true;

U32 TerrainRender::mNumClipPlanes = 4;
MatrixF TerrainRender::mCameraToObject;

AllocatedTexture TerrainRender::mTextureFrameListHead;
AllocatedTexture TerrainRender::mTextureFrameListTail;
AllocatedTexture TerrainRender::mTextureFreeListHead;
AllocatedTexture TerrainRender::mTextureFreeListTail;
AllocatedTexture TerrainRender::mTextureFreeBigListHead;
AllocatedTexture TerrainRender::mTextureFreeBigListTail;
AllocatedTexture *TerrainRender::mTextureGrid[AllocatedTextureCount];
AllocatedTexture **TerrainRender::mTextureGridPtr[5];
AllocatedTexture *TerrainRender::mCurrentTexture = NULL;
#if defined(TORQUE_OS_LINUX) || defined(TORQUE_OS_OPENBSD)
// Texture slop isn't necessary on Linux
U32 TerrainRender::mTextureSlopSize = 512;
#else
U32 TerrainRender::mTextureSlopSize = 220;
#endif

static bool sgTextureFreeListPrimed = false;
static U32  sgFreeListPrimeCount    = 32;
Vector<TextureHandle> TerrainRender::mTextureFreeList(__FILE__, __LINE__);

SceneState* TerrainRender::mSceneState;

TerrainBlock* TerrainRender::mCurrentBlock;
S32 TerrainRender::mSquareSize;
F32 TerrainRender::mScreenSize;
U32 TerrainRender::mFrameIndex;

Point2F TerrainRender::mBlockPos;
Point2I TerrainRender::mBlockOffset;
Point2I TerrainRender::mTerrainOffset;
PlaneF  TerrainRender::mClipPlane[MaxClipPlanes];
Point3F TerrainRender::mCamPos;

TextureHandle* TerrainRender::mGrainyTexture = NULL;
U32            TerrainRender::mDynamicLightCount;

F32 TerrainRender::mPixelError;

TerrLightInfo TerrainRender::mTerrainLights[MaxTerrainLights];
bool TerrainRender::mRenderingCommander = false;


F32 TerrainRender::mScreenError;
F32 TerrainRender::mMinSquareSize;
F32 TerrainRender::mFarDistance;
S32 TerrainRender::mDynamicTextureCount;
S32 TerrainRender::mStaticTextureCount;
S32 maxTerrPoints = 100;

ColorF TerrainRender::mFogColor;

bool TerrainRender::mRenderOutline;

U32 TerrainRender::mMaterialCount;

namespace {

Point4F sgTexGenS;
Point4F sgTexGenT;
Point4F sgLMGenS;
Point4F sgLMGenT;
Point4F bumpTexGenS;
Point4F bumpTexGenT;
Point2F bumpTextureOffset;
} // namespace {}


static S32 getPower(S32 x)
{
    // Returns 2^n (the highest bit).
    S32 i = 0;
    if (x)
        do
            i++;
        while (x >>= 1);
    return i;
}


void TerrainRender::init()
{
   S32 i;
   mTextureMinSquareSize = 0;

   mFrameIndex = 0;

   mScreenError = 4;
   mScreenSize = 45;
   mMinSquareSize = 4;
   mFarDistance = 500;
   mRenderOutline = false;
   mTextureFrameListHead.next = &mTextureFrameListTail;
   mTextureFrameListHead.previous = NULL;
   mTextureFrameListTail.next = NULL;
   mTextureFrameListTail.previous = &mTextureFrameListHead;

   mTextureFreeListHead.next = &mTextureFreeListTail;
   mTextureFreeListHead.previous = NULL;
   mTextureFreeListTail.next = NULL;
   mTextureFreeListTail.previous = &mTextureFreeListHead;

   mTextureFreeBigListHead.next = &mTextureFreeBigListTail;
   mTextureFreeBigListHead.previous = NULL;
   mTextureFreeBigListTail.next = NULL;
   mTextureFreeBigListTail.previous = &mTextureFreeBigListHead;

   for(i = 0; i < AllocatedTextureCount; i++)
      mTextureGrid[i] = 0;

   mTextureGridPtr[0] = mTextureGrid;
   mTextureGridPtr[1] = mTextureGrid + 4096;
   mTextureGridPtr[2] = mTextureGrid + 4096 + 1024;
   mTextureGridPtr[3] = mTextureGrid + 4096 + 1024 + 256;
   mTextureGridPtr[4] = mTextureGrid + 4096 + 1024 + 256 + 64;

/*   for(i = 0; i < 256; i++)
   {
      mSquareSeqAdd[i] = 0;
      for(S32 val = 0; val < 9; val++)
      {
         if(i & (1 << val))
            mSquareSeqAdd[i] += (1 << val) * (1 << val);
      }
   } */

   mBlendBitmap = new GBitmap(TerrainTextureSize, TerrainTextureSize, true, GBitmap::RGB5551);

   Con::addVariable("T2::dynamicTextureCount", TypeS32, &mDynamicTextureCount);
   Con::addVariable("T2::staticTextureCount", TypeS32, &mStaticTextureCount);

   Con::addVariable("screenSize", TypeF32, &mScreenSize);
   Con::addVariable("farDistance", TypeF32, &mFarDistance);
   Con::addVariable("pref::Terrain::texDetail", TypeS32, &mTextureMinSquareSize);
   Con::addVariable("pref::Terrain::enableDetails", TypeBool, &mEnableTerrainDetails);
// CW - stuff with bump maps
    Con::addVariable("pref::Terrain::enableEmbossBumps", TypeBool, &mEnableTerrainEmbossBumps);
    // CW - end bump map stuff
   Con::addVariable("pref::Terrain::dynamicLights", TypeBool, &mEnableTerrainDynLights);
   Con::addVariable("pref::Terrain::screenError", TypeF32, &mScreenError);
   Con::addVariable("pref::Terrain::textureCacheSize", TypeS32, &mTextureSlopSize);

#if ENABLE_NPATCH && TERRAIN_STORE_NORMALS
   Con::addVariable("pref::Terrain::nPatch", TypeBool, &gNPatchTerrain);
   Con::addVariable("pref::Terrain::nPatchLevel", TypeS32, &gNPatchTerrainLevel);
#endif
}

void TerrainRender::shutdown()
{
   delete mBlendBitmap;
   mBlendBitmap = NULL;
   flushCache();
}

void TerrainRender::buildClippingPlanes(bool flipClipPlanes)
{
   F64 frustumParam[6];
   dglGetFrustum(&frustumParam[0], &frustumParam[1],
                 &frustumParam[2], &frustumParam[3],
                 &frustumParam[4], &frustumParam[5]);

   Point3F osCamPoint(0, 0, 0);
   mCameraToObject.mulP(osCamPoint);
   sgComputeOSFrustumPlanes(frustumParam,
                            mCameraToObject,
                            osCamPoint,
                            mClipPlane[4],
                            mClipPlane[0],
                            mClipPlane[1],
                            mClipPlane[2],
                            mClipPlane[3]);
   // no need
   mNumClipPlanes = 4;
   // near plane is needed as well...
   //PlaneF p(0, 1, 0, -frustumParam[4]);
   //mTransformPlane(mCameraToObject, Point3F(1,1,1), p, &mClipPlane[0]);

   if (flipClipPlanes) {
      mClipPlane[0].neg();
      mClipPlane[1].neg();
      mClipPlane[2].neg();
      mClipPlane[3].neg();
      mClipPlane[4].neg();
      mClipPlane[5].neg();
   }
}

S32 TerrainRender::TestSquareVisibility(Point3F &min, Point3F &max, S32 mask, F32 expand)
{
   S32 retMask = 0;
   Point3F minPoint, maxPoint;
   for(S32 i = 0; i < mNumClipPlanes; i++)
   {
      if(mask & (1 << i))
      {
         if(mClipPlane[i].x > 0)
         {
            maxPoint.x = max.x;
            minPoint.x = min.x;
         }
         else
         {
            maxPoint.x = min.x;
            minPoint.x = max.x;
         }
         if(mClipPlane[i].y > 0)
         {
            maxPoint.y = max.y;
            minPoint.y = min.y;
         }
         else
         {
            maxPoint.y = min.y;
            minPoint.y = max.y;
         }
         if(mClipPlane[i].z > 0)
         {
            maxPoint.z = max.z;
            minPoint.z = min.z;
         }
         else
         {
            maxPoint.z = min.z;
            minPoint.z = max.z;
         }
         F32 maxDot = mDot(maxPoint, mClipPlane[i]);
         F32 minDot = mDot(minPoint, mClipPlane[i]);
         F32 planeD = mClipPlane[i].d;
         if(maxDot <= -(planeD + expand))
            return -1;
         if(minDot <= -planeD)
            retMask |= (1 << i);
      }
   }
   return retMask;
}

ChunkCornerPoint *TerrainRender::allocInitialPoint(Point3F pos)
{

   ChunkCornerPoint *ret = (ChunkCornerPoint *) FrameAllocator::alloc(sizeof(ChunkCornerPoint));
   ret->x = pos.x;
   ret->y = pos.y;
   ret->z = pos.z;
   ret->distance = (*ret - mCamPos).len();
   gClientSceneGraph->getFogCoordPair(ret->distance, ret->z, ret->fogRed, ret->fogGreen);
   ret->xfIndex = 0;
   return ret;
}

ChunkCornerPoint *TerrainRender::allocPoint(Point2I pos)
{
   ChunkCornerPoint *ret = (ChunkCornerPoint *) FrameAllocator::alloc(sizeof(ChunkCornerPoint));
   ret->x = pos.x * mSquareSize + mBlockPos.x;
   ret->y = pos.y * mSquareSize + mBlockPos.y;
   ret->z = fixedToFloat(mCurrentBlock->getHeight(pos.x, pos.y));
   ret->distance = (*ret - mCamPos).len();
   gClientSceneGraph->getFogCoordPair(ret->distance, ret->z, ret->fogRed, ret->fogGreen);

   ret->xfIndex = 0;
   return ret;
}

void TerrainRender::allocRenderEdges(U32 edgeCount, EdgeParent **dest, bool renderEdge)
{
   if(renderEdge)
   {
      for(U32 i = 0; i < edgeCount; i++)
      {
         ChunkEdge *edge = (ChunkEdge *) FrameAllocator::alloc(sizeof(ChunkEdge));
         edge->c1 = NULL;
         edge->c2 = NULL;
         edge->xfIndex = 0;
         dest[i] = edge;
      }
   }
   else
   {
      for(U32 i = 0; i < edgeCount; i++)
      {
         ChunkScanEdge *edge = (ChunkScanEdge *) FrameAllocator::alloc(sizeof(ChunkScanEdge));
         edge->mp = NULL;
         dest[i] = edge;
      }
   }
}

void TerrainRender::subdivideChunkEdge(ChunkScanEdge *e, Point2I pos, bool chunkEdge)
{
   if(!e->mp)
   {
      allocRenderEdges(2, &e->e1, chunkEdge);
      e->mp = allocPoint(pos);

      e->e1->p1 = e->p1;
      e->e1->p2 = e->mp;
      e->e2->p1 = e->mp;
      e->e2->p2 = e->p2;
   }
}

F32 TerrainRender::getSquareDistance(const Point3F& minPoint, const Point3F& maxPoint, F32* zDiff)
{
   Point3F vec;
   if(mCamPos.z < minPoint.z)
      vec.z = minPoint.z - mCamPos.z;
   else if(mCamPos.z > maxPoint.z)
      vec.z = maxPoint.z - mCamPos.z;
   else
      vec.z = 0;

   if(mCamPos.x < minPoint.x)
      vec.x = minPoint.x - mCamPos.x;
   else if(mCamPos.x > maxPoint.x)
      vec.x = mCamPos.x - maxPoint.x;
   else
      vec.x = 0;

   if(mCamPos.y < minPoint.y)
      vec.y = minPoint.y - mCamPos.y;
   else if(mCamPos.y > maxPoint.y)
      vec.y = mCamPos.y - maxPoint.y;
   else
      vec.y = 0;

   *zDiff = vec.z;

   return vec.len();
}

void TerrainRender::emitTerrChunk(SquareStackNode *n, F32 squareDistance, U32 lightMask, bool farClip, bool drawDetails, bool drawBumps)
{
   //if(n->pos.x || n->pos.y)
   //   return;
   GridChunk *gc = mCurrentBlock->findChunk(n->pos);
   EmitChunk *chunk = (EmitChunk *) FrameAllocator::alloc(sizeof(EmitChunk));
   chunk->x = n->pos.x + mBlockOffset.x + mTerrainOffset.x;
   chunk->y = n->pos.y + mBlockOffset.y + mTerrainOffset.y;
   chunk->gridX = n->pos.x;
   chunk->gridY = n->pos.y;
   chunk->lightMask = lightMask;

   chunk->next = mCurrentTexture->list;
   mCurrentTexture->list = chunk;

   if(mRenderingCommander)
      return;

   chunk->edge[0] = (ChunkEdge *) n->top;
   chunk->edge[1] = (ChunkEdge *) n->right;
   chunk->edge[2] = (ChunkEdge *) n->bottom;
   chunk->edge[3] = (ChunkEdge *) n->left;

   chunk->edge[0]->c2 = chunk;
   chunk->edge[1]->c1 = chunk;
   chunk->edge[2]->c1 = chunk;
   chunk->edge[3]->c2 = chunk;


   // holes only in the primary terrain block
   if (gc->emptyFlags && mBlockPos.x == 0 && mBlockPos.y == 0)
      chunk->emptyFlags = gc->emptyFlags;
   else
      chunk->emptyFlags = 0;

   S32 subDivLevel;
   F32 growFactor = 0;

   F32 minSubdivideDistance = 1000000;
   chunk->clip = farClip;

   chunk->renderDetails = drawDetails;
    chunk->renderBumps = drawBumps;

   if(squareDistance < 1)
      subDivLevel = -1;
   else
   {
      for(subDivLevel = 2; subDivLevel >= 0; subDivLevel--)
      {
         F32 subdivideDistance = fixedToFloat(gc->heightDeviance[subDivLevel]) / mPixelError;
         if(subdivideDistance > minSubdivideDistance)
            subdivideDistance = minSubdivideDistance;

         if(squareDistance >= subdivideDistance)
            break;
         F32 clampDistance = subdivideDistance * 0.75;
         if(squareDistance > clampDistance)
         {
            growFactor = (squareDistance - clampDistance) / (0.25 * subdivideDistance);
            subDivLevel--;
            break;
         }
         minSubdivideDistance = clampDistance;
      }
   }
   chunk->subDivLevel = subDivLevel;
   chunk->growFactor = growFactor;
}

void TerrainRender::processCurrentBlock(SceneState*, EdgeParent *topEdge, EdgeParent *rightEdge, EdgeParent *bottomEdge, EdgeParent *leftEdge)
{
   SquareStackNode stack[TerrainBlock::BlockShift*4];
   Point3F minPoint, maxPoint;

   stack[0].level = TerrainBlock::BlockShift;
   stack[0].clipFlags = ((1 << mNumClipPlanes) - 1) | FarSphereMask;  // test all the planes
   stack[0].pos.set(0,0);
   stack[0].top = topEdge;
   stack[0].right = rightEdge;
   stack[0].bottom = bottomEdge;
   stack[0].left = leftEdge;
   stack[0].lightMask = (1 << mDynamicLightCount) - 1; // test all the lights
   stack[0].texAllocated = false;

   Vector<SceneState::FogBand> *posFog = mSceneState->getPosFogBands();
   Vector<SceneState::FogBand> *negFog = mSceneState->getNegFogBands();
   bool clipAbove = posFog->size() > 0 && (*posFog)[0].isFog == false;
   bool clipBelow = negFog->size() > 0 && (*negFog)[0].isFog == false;
   bool clipOn = posFog->size() > 0 && (*posFog)[0].isFog == true;

   if(posFog->size() != 0 || negFog->size() != 0)
      stack[0].clipFlags |= FogPlaneBoxMask;

   S32 curStackSize = 1;
   F32 squareDistance;

   F32 worldToScreenScale   = dglProjectRadius(1,1);
   F32 zeroDetailDistance   = (mSquareSize * worldToScreenScale) / (1 << 6) - (mSquareSize >> 1);
    F32 zeroBumpDistance         = (mSquareSize * worldToScreenScale) / (1 << mCurrentBlock->mZeroBumpScale) - (mSquareSize >> 1);

   while(curStackSize)
   {
      SquareStackNode *n = stack + curStackSize - 1;
      // see if it's visible
      GridSquare *sq = mCurrentBlock->findSquare(n->level, n->pos);

      minPoint.set(mSquareSize * n->pos.x + mBlockPos.x,
                   mSquareSize * n->pos.y + mBlockPos.y,
                   fixedToFloat(sq->minHeight));
      maxPoint.set(minPoint.x + (mSquareSize << n->level),
                   minPoint.y + (mSquareSize << n->level),
                   fixedToFloat(sq->maxHeight));

      // holes only in the primary terrain block
      if ((sq->flags & GridSquare::Empty) && mBlockPos.x == 0 && mBlockPos.y == 0)
      {
         curStackSize--;
         continue;
      }

      F32 zDiff;
      squareDistance = getSquareDistance(minPoint, maxPoint, &zDiff);

      S32 nextClipFlags = 0;

      if(n->clipFlags)
      {
         if(n->clipFlags & FogPlaneBoxMask)
         {
            F32 camZ = mCamPos.z;
            bool boxBelow = camZ > maxPoint.z;
            bool boxAbove = camZ < minPoint.z;
            bool boxOn = !(boxAbove || boxBelow);
            if( clipOn ||
                (clipAbove && boxAbove && (maxPoint.z - camZ > (*posFog)[0].cap)) ||
                (clipBelow && boxBelow && (camZ - minPoint.z > (*negFog)[0].cap)) ||
                (boxOn && (( clipAbove && maxPoint.z - camZ > (*posFog)[0].cap ) ||
                           ( clipBelow && camZ - minPoint.z > (*negFog)[0].cap ))))
            {
               // Using the fxSunLight can cause the "sky" to extend down below the camera.
               // To avoid the sun showing through, the terrain must always be rendered.
               // If the fxSunLight is not being used, the following optimization can be
               // uncommented.
               #if 0
               if(boxBelow && !mSceneState->isBoxFogVisible(squareDistance, maxPoint.z, minPoint.z))
               {
                  // Totally fogged terrain tiles can be thrown out as long as they are
                  // below the camera. If they are ubove, the sky will show through the
                  // fog.
                  curStackSize--;
                  continue;
               }
               #endif
               nextClipFlags |= FogPlaneBoxMask;
            }
         }
         if(n->clipFlags & FarSphereMask)
         {
            if(squareDistance >= mFarDistance)
            {
               curStackSize--;
               continue;
            }

            S32 squareSz = mSquareSize << n->level;
            if(squareDistance + maxPoint.z - minPoint.z + squareSz + squareSz > mFarDistance)
               nextClipFlags |= FarSphereMask;
         }

         // zDelta for screen error height deviance.
         F32 zDelta = squareDistance * mPixelError;
         minPoint.z -= zDelta;
         maxPoint.z += zDelta;

         nextClipFlags |= TestSquareVisibility(minPoint, maxPoint, n->clipFlags, mSquareSize);
         if(nextClipFlags == -1)
         {
            //if(!n->texAllocated)
            //   textureRecurse(n);

            // trivially rejected, so pop it off the stack
            curStackSize--;
            continue;
         }
      }
      if(!n->texAllocated)
      {
         S32 squareSz = mSquareSize << n->level;
         // first check the level - if its 3 or less, we have to just make a bitmap:
         // level 3 == 8x8 square - 8x8 * 16x16 == 128x128
         if(n->level > 6)
            goto notexalloc;

         S32 mipLevel = TerrainTextureMipLevel;
         if(!mRenderingCommander)
         {
            if(n->level > mTextureMinSquareSize + 2)
            {
               // get the mip level of the square and see if we're in range
               if(squareDistance > 0.001)
               {
                  S32 size = S32(dglProjectRadius(squareDistance + (squareSz >> 1), squareSz));
                  mipLevel = getPower((S32)(size * 0.75));
                  if(mipLevel > TerrainTextureMipLevel) // too big for this square
                     goto notexalloc;
               }
               else
                  goto notexalloc;
            }
         }
         allocTerrTexture(n->pos, n->level, mipLevel, true, squareDistance);
         n->texAllocated = true;
         if(mRenderingCommander) // level == 6
         {
            emitTerrChunk(n, 0, 0, 0, 0, 0);
            curStackSize--;
            continue;
         }
      }
notexalloc:
      if(n->lightMask)
         n->lightMask = TestSquareLights(sq, n->level, n->pos, n->lightMask);

      if(n->level == 2)
      {
         AssertFatal(n->texAllocated, "Invalid texture index.");

         bool drawDetails = false;
         if (mEnableTerrainDetails && squareDistance < zeroDetailDistance)
            drawDetails = true;

             //END
            bool drawBumps = false;
// CW - stuff with bump maps
            if (mEnableTerrainEmbossBumps && squareDistance < zeroBumpDistance)
                drawBumps = true;
// CW - end bump map stuff
         emitTerrChunk(n, squareDistance, n->lightMask, nextClipFlags & FarSphereMask, drawDetails, drawBumps);
         curStackSize--;
         continue;
      }
      bool allocChunkEdges = (n->level == 3);

      Point2I pos = n->pos;

      ChunkScanEdge *top = (ChunkScanEdge *) n->top;
      ChunkScanEdge *right = (ChunkScanEdge *) n->right;
      ChunkScanEdge *bottom = (ChunkScanEdge *) n->bottom;
      ChunkScanEdge *left = (ChunkScanEdge *) n->left;

      // subdivide this square and throw it on the stack
      S32 squareOneSize = 1 << n->level;
      S32 squareHalfSize = squareOneSize >> 1;

      ChunkCornerPoint *midPoint = allocPoint(Point2I(pos.x + squareHalfSize, pos.y + squareHalfSize));
      S32 nextLevel = n->level - 1;

      subdivideChunkEdge(top, Point2I(pos.x + squareHalfSize, pos.y + squareOneSize), allocChunkEdges);
      subdivideChunkEdge(right, Point2I(pos.x + squareOneSize, pos.y + squareHalfSize), allocChunkEdges);
      subdivideChunkEdge(bottom, Point2I(pos.x + squareHalfSize, pos.y), allocChunkEdges);
      subdivideChunkEdge(left, Point2I(pos.x, pos.y + squareHalfSize), allocChunkEdges);

      // cross edges go top, right, bottom, left
      EdgeParent *crossEdges[4];
      allocRenderEdges(4, crossEdges, allocChunkEdges);
      crossEdges[0]->p1 = top->mp;
      crossEdges[0]->p2 = midPoint;
      crossEdges[1]->p1 = midPoint;
      crossEdges[1]->p2 = right->mp;
      crossEdges[2]->p1 = midPoint;
      crossEdges[2]->p2 = bottom->mp;
      crossEdges[3]->p1 = left->mp;
      crossEdges[3]->p2 = midPoint;

      n->level = nextLevel;
      n->clipFlags = nextClipFlags;

      for(S32 i = 1; i < 4; i++)
      {
         n[i].level = nextLevel;
         n[i].clipFlags = nextClipFlags;
         n[i].lightMask = n->lightMask;
         n[i].texAllocated = n->texAllocated;
      }
      // push in reverse order of processing.
      n[3].pos = pos;
      n[3].top = crossEdges[3];
      n[3].right = crossEdges[2];
      n[3].bottom = bottom->e1;
      n[3].left = left->e2;

      n[2].pos.set(pos.x + squareHalfSize, pos.y);
      n[2].top = crossEdges[1];
      n[2].right = right->e2;
      n[2].bottom = bottom->e2;
      n[2].left = crossEdges[2];

      n[1].pos.set(pos.x, pos.y + squareHalfSize);
      n[1].top = top->e1;
      n[1].right = crossEdges[0];
      n[1].bottom = crossEdges[3];
      n[1].left = left->e1;

      n[0].pos.set(pos.x + squareHalfSize, pos.y + squareHalfSize);
      n[0].top = top->e2;
      n[0].right = right->e1;
      n[0].bottom = crossEdges[1];
      n[0].left = crossEdges[0];

      curStackSize += 3;
   }
}





//---------------------------------------------------------------
//---------------------------------------------------------------
// Root block render function
//---------------------------------------------------------------
//---------------------------------------------------------------

void TerrainRender::fixEdge(ChunkEdge *edge, S32 x, S32 y, S32 dx, S32 dy)
{
   S32 minLevel, maxLevel;
   F32 growFactor;

   if(edge->c1)
   {
      minLevel = edge->c1->subDivLevel;
      maxLevel = edge->c1->subDivLevel;
      growFactor = edge->c1->growFactor;
      if(edge->c2)
      {
         if(edge->c2->subDivLevel < minLevel)
            minLevel = edge->c2->subDivLevel;
         else if(edge->c2->subDivLevel > maxLevel)
         {
            maxLevel = edge->c2->subDivLevel;
            growFactor = edge->c2->growFactor;
         }
         else if(edge->c2->growFactor > growFactor)
            growFactor = edge->c2->growFactor;
      }
   }
   else
   {
      minLevel = maxLevel = edge->c2->subDivLevel;
      growFactor = edge->c2->growFactor;
   }
   if(minLevel == 2)
   {
      edge->pointCount = 0;
      return;
   }

   // get the mid heights
   EdgePoint *pmid = &edge->pt[1];
   ChunkCornerPoint *p1 = edge->p1;
   ChunkCornerPoint *p2 = edge->p2;

   pmid->x = (p1->x + p2->x) * 0.5;
   pmid->y = (p1->y + p2->y) * 0.5;

   if(maxLevel == 2)
   {
      // pure interp
      pmid->z = (p1->z + p2->z) * 0.5;
      pmid->distance = (*pmid - mCamPos).len();
      pmid->fogRed = (p1->fogRed + p2->fogRed) * 0.5;
      pmid->fogGreen = (p1->fogGreen + p2->fogGreen) * 0.5;

      if(minLevel >= 0)
      {
         edge->pointCount = 1;
         return;
      }
   }
   else
   {
      pmid->z = fixedToFloat(mCurrentBlock->getHeight(x + dx + dx, y + dy + dy));
      if(maxLevel == 1) // interp the z and haze
         pmid->z = pmid->z + growFactor * (((p1->z + p2->z) * 0.5) - pmid->z);

      pmid->distance = (*pmid - mCamPos).len();
      gClientSceneGraph->getFogCoordPair(pmid->distance, pmid->z, pmid->fogRed, pmid->fogGreen);

      if(maxLevel == 1) // interp the z and haze
      {
         pmid->fogRed = pmid->fogRed + growFactor * (((p1->fogRed + p2->fogRed) * 0.5) - pmid->fogRed);
         pmid->fogGreen = pmid->fogGreen + growFactor * (((p1->fogGreen + p2->fogGreen) * 0.5) - pmid->fogGreen);
      }
      if(minLevel >= 0)
      {
         edge->pointCount = 1;
         return;
      }
   }
   // last case - minLevel == -1, midPoint calc'd
   edge->pointCount = 3;
   EdgePoint *pm1 = &edge->pt[0];
   EdgePoint *pm2 = &edge->pt[2];

   pm1->x = (p1->x + pmid->x) * 0.5;
   pm1->y = (p1->y + pmid->y) * 0.5;
   pm2->x = (p2->x + pmid->x) * 0.5;
   pm2->y = (p2->y + pmid->y) * 0.5;

   if(maxLevel != -1)
   {
      // clamp it:
      pm1->z = (p1->z + pmid->z) * 0.5;
      pm1->distance = (*pm1 - mCamPos).len();
      pm1->fogRed = (p1->fogRed + pmid->fogRed) * 0.5;
      pm1->fogGreen = (p1->fogGreen + pmid->fogGreen) * 0.5;

      pm2->z = (p2->z + pmid->z) * 0.5;
      pm2->distance = (*pm2 - mCamPos).len();
      pm2->fogRed = (p2->fogRed + pmid->fogRed) * 0.5;
      pm2->fogGreen = (p2->fogGreen + pmid->fogGreen) * 0.5;
      return;
   }
   // compute the real deals:
   pm1->z = fixedToFloat(mCurrentBlock->getHeight(x + dx, y + dy));
   pm2->z = fixedToFloat(mCurrentBlock->getHeight(x + dx + dx + dx, y + dy + dy + dy));

   if(growFactor)
   {
      pm1->z = pm1->z + growFactor * (((p1->z + pmid->z) * 0.5) - pm1->z);
      pm2->z = pm2->z + growFactor * (((p2->z + pmid->z) * 0.5) - pm2->z);
   }
   pm1->distance = (*pm1 - mCamPos).len();
   gClientSceneGraph->getFogCoordPair(pm1->distance, pm1->z, pm1->fogRed, pm1->fogGreen);

   pm2->distance = (*pm2 - mCamPos).len();
   gClientSceneGraph->getFogCoordPair(pm2->distance, pm2->z, pm2->fogRed, pm2->fogGreen);

   if(growFactor)
   {
      pm1->fogRed = pm1->fogRed + growFactor * (((p1->fogRed + pmid->fogRed) * 0.5) - pm1->fogRed);
      pm1->fogGreen = pm1->fogGreen + growFactor * (((p1->fogGreen + pmid->fogGreen) * 0.5) - pm1->fogGreen);

      pm2->fogRed = pm2->fogRed + growFactor * (((p2->fogRed + pmid->fogRed) * 0.5) - pm2->fogRed);
      pm2->fogGreen = pm2->fogGreen + growFactor * (((p2->fogGreen + pmid->fogGreen) * 0.5) - pm2->fogGreen);
   }
}

EdgePoint *mXFVertices = NULL;
U16 *mXFIndexBuffer;
U16 *mXFIndexPtr;
U32 mXFIndexCount;

U32 mXFPointCount;
U32 mXFIndex;

inline U32 clipPoint(EdgePoint *p1, EdgePoint *p2, F32 dist)
{
   F32 frac = (dist - p1->distance) / (p2->distance - p1->distance);
   F32 onefrac = 1.0 - frac;
   U32 clipIndex = mXFPointCount++;

   EdgePoint *ip = mXFVertices + clipIndex;
   ip->x = (p2->x * frac) + (p1->x * onefrac);
   ip->y = (p2->y * frac) + (p1->y * onefrac);
   ip->z = (p2->z * frac) + (p1->z * onefrac);
   ip->fogRed = (p2->fogRed * frac) + (p1->fogRed * onefrac);
   ip->fogGreen = (p2->fogGreen * frac) + (p1->fogGreen * onefrac);

#if TERRAIN_STORE_NORMALS
    if (gNPatch && gNPatchTerrain)
    {
       ip->vn.x = (p2->vn.x * frac) + (p1->vn.x * onefrac);
       ip->vn.y = (p2->vn.y * frac) + (p1->vn.y * onefrac);
       ip->vn.z = (p2->vn.z * frac) + (p1->vn.z * onefrac);
       ip->vn.normalize();
    }
#endif

   ip->distance = dist;
   return clipIndex;
}

void TerrainRender::clip(U32 indexStart)
{
   static U16 dest[16 * 3 * 2];


   U32 vertexCount = mXFIndexBuffer[indexStart + 1];
   U32 centerPoint = mXFIndexBuffer[indexStart + 2];
   U16 *source = mXFIndexBuffer + indexStart + 3;
   EdgePoint *center = mXFVertices + centerPoint;

   U32 destIndex = 0;

   if(mXFVertices[centerPoint].distance > mFarDistance)
   {
      // loop through all the tris and clip em:
      // there are vertexCount - 1 triangles.
      EdgePoint *p1 = mXFVertices + source[0];
      bool p1out = p1->distance >= mFarDistance;
      U32 p1idx = source[0];
      U32 clip1;
      if(!p1out)
         clip1 = clipPoint(p1, center, mFarDistance);
      for(U32 i = 0; i < vertexCount - 2; i++)
      {
         U32 p2idx = source[i+1];
         EdgePoint *p2 = mXFVertices + p2idx;
         bool p2out = p2->distance >= mFarDistance;
         if(!p2out)
         {
            U32 clip2 = clipPoint(p2, center, mFarDistance);
            if(p1out)
            {
               // p2 is the only "in" point:
               dest[destIndex++] = p2idx;
               dest[destIndex++] = clip2;
               dest[destIndex++] = clipPoint(p1, p2, mFarDistance);
            }
            else
            {
               dest[destIndex++] = clip2;
               dest[destIndex++] = clip1;
               dest[destIndex++] = p1idx;
               dest[destIndex++] = p2idx;
               dest[destIndex++] = clip2;
               dest[destIndex++] = p1idx;
            }
            clip1 = clip2;
         }
         else if(!p1out)
         {
            dest[destIndex++] = p1idx;
            dest[destIndex++] = clipPoint(p1, p2, mFarDistance);
            dest[destIndex++] = clip1;
         }
         p1idx = p2idx;
         p1out = p2out;
         p1 = p2;
      }
      if(destIndex)
      {
         // copy this in..
         mXFIndexBuffer[indexStart] = GL_TRIANGLES;
         mXFIndexBuffer[indexStart + 1] = destIndex;
         for(U32 i = 0; i < destIndex; i++)
            mXFIndexBuffer[indexStart + i + 2] = dest[i];
         mXFIndexCount = destIndex + indexStart + 2;
      }
      else
         mXFIndexCount = indexStart;
   }
   else
   {
      EdgePoint *prev = mXFVertices + source[0];
      bool prevIn = prev->distance <= mFarDistance;
      U32 i;

      for(i = 1; i < vertexCount - 1; i++)
      {
         EdgePoint *pt = mXFVertices + source[i];
         bool curIn = pt->distance <= mFarDistance;

         if((curIn && !prevIn) || (!curIn && prevIn))
            dest[destIndex++] = clipPoint(pt, prev, mFarDistance);
         if(curIn)
            dest[destIndex++] = source[i];
         else
            dest[destIndex++] = clipPoint(pt, center, mFarDistance);
         prev = pt;
         prevIn = curIn;
      }
      for(i = 0; i < destIndex; i++)
         mXFIndexBuffer[indexStart + i + 3] = dest[i];
      mXFIndexBuffer[indexStart + destIndex + 3] = dest[0];
      mXFIndexBuffer[indexStart + 1] = destIndex + 2;
      mXFIndexCount = indexStart + destIndex + 4;
   }
}

inline U32 TerrainRender::constructPoint(S32 x, S32 y)
{
   U32 ret = mXFPointCount++;
   EdgePoint *pt = mXFVertices + ret;

   pt->x = x * mSquareSize;
   pt->y = y * mSquareSize;
   pt->z = fixedToFloat(mCurrentBlock->getHeight(x, y));

   pt->distance = (*pt - mCamPos).len();

   gClientSceneGraph->getFogCoordPair(pt->distance, pt->z, pt->fogRed, pt->fogGreen);

#if TERRAIN_STORE_NORMALS
    if (gNPatch && gNPatchTerrain)
    {
        static Point2F p2;
        p2.x = pt->x;
        p2.y = pt->y;
    if (!mCurrentBlock->getNormal(p2, &(pt->vn)))
    {
        pt->vn.x = 0.0;
        pt->vn.y = 0.0;
        pt->vn.z = 1.0;
    }
   }
#endif

   return ret;
}

inline U32 TerrainRender::interpPoint(U32 p1, U32 p2, S32 x, S32 y, F32 growFactor)
{
   U32 ret = mXFPointCount++;
   EdgePoint *pt = mXFVertices + ret;

   pt->x = x * mSquareSize;
   pt->y = y * mSquareSize;
   pt->z = fixedToFloat(mCurrentBlock->getHeight(x, y));
   pt->z = pt->z + growFactor * (((mXFVertices[p1].z + mXFVertices[p2].z) * 0.5) - pt->z);

   pt->distance = (*pt - mCamPos).len();
   gClientSceneGraph->getFogCoordPair(pt->distance, pt->z, pt->fogRed, pt->fogGreen);

   return ret;
}

inline void TerrainRender::addEdge(ChunkEdge *edge)
{
   if(edge->pointCount == 1)
   {
      edge->pointIndex = mXFPointCount;
      mXFVertices[mXFPointCount++] = * ((EdgePoint *) &edge->pt[1]);
   }
   else if(edge->pointCount == 3)
   {
      edge->pointIndex = mXFPointCount;
      mXFVertices[mXFPointCount++] = *((EdgePoint *) &edge->pt[0]);
      mXFVertices[mXFPointCount++] = *((EdgePoint *) &edge->pt[1]);
      mXFVertices[mXFPointCount++] = *((EdgePoint *) &edge->pt[2]);
   }
   edge->xfIndex = mXFIndex;
}

inline void emitTri(U32 i1, U32 i2, U32 i3)
{
   mXFIndexBuffer[mXFIndexCount] = i1;
   mXFIndexBuffer[mXFIndexCount + 1] = i2;
   mXFIndexBuffer[mXFIndexCount + 2] = i3;
   mXFIndexCount += 3;
}

inline U32 emitCornerPoint(ChunkCornerPoint *p)
{
   if(p->xfIndex != mXFIndex)
   {
      p->pointIndex = mXFPointCount;
      p->xfIndex = mXFIndex;
      mXFVertices[mXFPointCount++] = *((EdgePoint *) p);
   }
   return p->pointIndex;
}

void buildLightTri(LightTriangle* pTri, TerrLightInfo* pInfo)
{
   // Get the plane normal
   Point3F normal;
   mCross((pTri->point1 - pTri->point2), (pTri->point3 - pTri->point2), &normal);
   if (normal.lenSquared() < 1e-7)
   {
      pTri->flags = 0;
      return;
   }


   PlaneF plane(pTri->point2, normal);  // Assumes that mPlane.h normalizes incoming point

   Point3F centerPoint;
   F32 d = plane.distToPlane(pInfo->pos);
   centerPoint = pInfo->pos - plane * d;
   d = mFabs(d);
   if (d >= pInfo->radius) {
      pTri->flags = 0;
      return;
   }

   F32 mr = mSqrt(pInfo->radiusSquared - d*d);

   Point3F normalS;
   Point3F normalT;
   mCross(plane, Point3F(0, 1, 0), &normalS);
   mCross(plane, normalS, &normalT);
   PlaneF splane(centerPoint, normalS); // Assumes that mPlane.h normalizes incoming point
   PlaneF tplane(centerPoint, normalT); // Assumes that mPlane.h normalizes incoming point

   pTri->color.red   = pInfo->r;
   pTri->color.green = pInfo->g;
   pTri->color.blue  = pInfo->b;
   pTri->color.alpha = (pInfo->radius - d) / pInfo->radius;

   pTri->texco1.set(((splane.distToPlane(pTri->point1) / mr) + 1.0) / 2.0,
                    ((tplane.distToPlane(pTri->point1) / mr) + 1.0) / 2.0);
   pTri->texco2.set(((splane.distToPlane(pTri->point2) / mr) + 1.0) / 2.0,
                    ((tplane.distToPlane(pTri->point2) / mr) + 1.0) / 2.0);
   pTri->texco3.set(((splane.distToPlane(pTri->point3) / mr) + 1.0) / 2.0,
                    ((tplane.distToPlane(pTri->point3) / mr) + 1.0) / 2.0);

   pTri->flags = 1;
}

void TerrainRender::renderChunkCommander(EmitChunk *chunk)
{
   U32 ll = mXFPointCount;
   for(U32 y = 0; y <= 64; y += 4)
      for(U32 x = (y & 4) ? 4 : 0; x <= 64; x += 8)
         constructPoint(chunk->x + x,chunk->y + y);

   for(U32 y = 0; y < 8; y++)
   {
      for(U32 x = 0; x < 8; x++)
      {
         U16 *ib = mXFIndexBuffer + mXFIndexCount;
         ib[0] = GL_TRIANGLE_FAN;
         ib[1] = 6;
         ib[2] = ll + 9;
         ib[3] = ll;
         ib[4] = ll + 17;
         ib[5] = ll + 18;
         ib[6] = ll + 1;
         ib[7] = ll;
         mXFIndexCount += 8;
         ll++;
      }
      ll += 9;
   }
}

void TerrainRender::renderChunkOutline(EmitChunk *chunk)
{
   U32 startXFIndex = mXFIndexCount;

   ChunkEdge *e0 = chunk->edge[0];
   ChunkEdge *e1 = chunk->edge[1];
   ChunkEdge *e2 = chunk->edge[2];
   ChunkEdge *e3 = chunk->edge[3];

   if(e0->xfIndex != mXFIndex)
   {
      if(!e0->xfIndex)
         fixEdge(e0, chunk->x, chunk->y + 4, 1, 0);
      addEdge(e0);
   }
   if(e1->xfIndex != mXFIndex)
   {
      if(!e1->xfIndex)
         fixEdge(e1, chunk->x + 4, chunk->y + 4, 0, -1);
      addEdge(e1);
   }
   if(e2->xfIndex != mXFIndex)
   {
      if(!e2->xfIndex)
         fixEdge(e2, chunk->x, chunk->y, 1, 0);
      addEdge(e2);
   }
   if(e3->xfIndex != mXFIndex)
   {
      if(!e3->xfIndex)
         fixEdge(e3, chunk->x, chunk->y + 4, 0, -1);
      addEdge(e3);
   }
   U32 p0 = emitCornerPoint(e0->p1);
   U32 p1 = emitCornerPoint(e0->p2);
   U32 p2 = emitCornerPoint(e2->p2);
   U32 p3 = emitCornerPoint(e2->p1);

   // build the interior points:
   U32 ip0 = constructPoint(chunk->x + 2, chunk->y + 2);
   F32 growFactor = chunk->growFactor;

   if(chunk->subDivLevel >= 1)
   {
      // just emit the fan for the whole square:
      S32 i;
      mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLE_FAN;
      U32 indexStart = mXFIndexCount++;
      mXFIndexBuffer[mXFIndexCount++] = ip0;

      mXFIndexBuffer[mXFIndexCount++] = p0;
      for(i = 0; i < e0->pointCount; i++)
         mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex + i;

      mXFIndexBuffer[mXFIndexCount++] = p1;
      for(i = 0; i < e1->pointCount; i++)
         mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex + i;

      mXFIndexBuffer[mXFIndexCount++] = p2;
      for(i = e2->pointCount - 1; i >= 0; i--)
         mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex + i;

      mXFIndexBuffer[mXFIndexCount++] = p3;
      for(i = e3->pointCount - 1; i >= 0; i--)
         mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex + i;

      mXFIndexBuffer[mXFIndexCount++] = p0;

      mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
      if(chunk->clip)
         clip(indexStart - 1);
   }
   else
   {
      if(chunk->subDivLevel == 0)
      {
         U32 ip1 = interpPoint(p0, ip0, chunk->x + 1, chunk->y + 3, growFactor);
         U32 ip2 = interpPoint(p1, ip0, chunk->x + 3, chunk->y + 3, growFactor);
         U32 ip3 = interpPoint(p2, ip0, chunk->x + 3, chunk->y + 1, growFactor);
         U32 ip4 = interpPoint(p3, ip0, chunk->x + 1, chunk->y + 1, growFactor);
         // emit the 4 fans:

         U32 indexStart;

         if((chunk->emptyFlags & CornerEmpty_0_1) != CornerEmpty_0_1)
         {
            mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLE_FAN;
            indexStart = mXFIndexCount++;

            mXFIndexBuffer[mXFIndexCount++] = ip1;
            mXFIndexBuffer[mXFIndexCount++] = p0;
            if(e0->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex;
            else // has to be 3:
            {
               mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex;
               mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex + 1;
            }
            mXFIndexBuffer[mXFIndexCount++] = ip0;
            if(e3->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex + 1;
               mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex;
            }
            mXFIndexBuffer[mXFIndexCount++] = p0;
            mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            if(chunk->clip)
               clip(indexStart - 1);
         }

         if((chunk->emptyFlags & CornerEmpty_1_1) != CornerEmpty_1_1)
         {
            mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLE_FAN;
            indexStart = mXFIndexCount++;
            mXFIndexBuffer[mXFIndexCount++] = ip2;
            mXFIndexBuffer[mXFIndexCount++] = p1;
            if(e1->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex;
               mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex + 1;
            }
            mXFIndexBuffer[mXFIndexCount++] = ip0;
            if(e0->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex + 1;
               mXFIndexBuffer[mXFIndexCount++] = e0->pointIndex + 2;
            }
            mXFIndexBuffer[mXFIndexCount++] = p1;
            mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            if(chunk->clip)
               clip(indexStart - 1);
         }

         if((chunk->emptyFlags & CornerEmpty_1_0) != CornerEmpty_1_0)
         {
            mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLE_FAN;
            indexStart = mXFIndexCount++;
            mXFIndexBuffer[mXFIndexCount++] = ip3;
            mXFIndexBuffer[mXFIndexCount++] = p2;
            if(e2->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex + 2;
               mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex + 1;
            }
            mXFIndexBuffer[mXFIndexCount++] = ip0;
            if(e1->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex + 1;
               mXFIndexBuffer[mXFIndexCount++] = e1->pointIndex + 2;
            }
            mXFIndexBuffer[mXFIndexCount++] = p2;
            mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            if(chunk->clip)
               clip(indexStart - 1);
         }

         if((chunk->emptyFlags & CornerEmpty_0_0) != CornerEmpty_0_0)
         {
            mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLE_FAN;
            indexStart = mXFIndexCount++;
            mXFIndexBuffer[mXFIndexCount++] = ip4;
            mXFIndexBuffer[mXFIndexCount++] = p3;
            if(e3->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex + 2;
               mXFIndexBuffer[mXFIndexCount++] = e3->pointIndex + 1;
            }
            mXFIndexBuffer[mXFIndexCount++] = ip0;
            if(e2->pointCount == 1)
               mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex;
            else
            {
               mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex + 1;
               mXFIndexBuffer[mXFIndexCount++] = e2->pointIndex;
            }
            mXFIndexBuffer[mXFIndexCount++] = p3;
            mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            if(chunk->clip)
               clip(indexStart - 1);
         }
      }
      else
      {
         // subDiv == -1
         U32 ip1 = constructPoint(chunk->x + 1, chunk->y + 3);
         U32 ip2 = constructPoint(chunk->x + 3, chunk->y + 3);
         U32 ip3 = constructPoint(chunk->x + 3, chunk->y + 1);
         U32 ip4 = constructPoint(chunk->x + 1, chunk->y + 1);
         U32 ip5 = interpPoint(e0->pointIndex + 1, ip0, chunk->x + 2, chunk->y + 3, growFactor);
         U32 ip6 = interpPoint(e1->pointIndex + 1, ip0, chunk->x + 3, chunk->y + 2, growFactor);
         U32 ip7 = interpPoint(e2->pointIndex + 1, ip0, chunk->x + 2, chunk->y + 1, growFactor);
         U32 ip8 = interpPoint(e3->pointIndex + 1, ip0, chunk->x + 1, chunk->y + 2, growFactor);

         // now do the squares:
         U16 *ib;

         if(chunk->emptyFlags & CornerEmpty_0_1)
         {
            if((chunk->emptyFlags & CornerEmpty_0_1) != CornerEmpty_0_1)
            {
               mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLES;
               U32 indexStart = mXFIndexCount++;
               if(!(chunk->emptyFlags & SquareEmpty_0_3))
               {
                  emitTri(ip1, e3->pointIndex, p0);
                  emitTri(ip1, p0, e0->pointIndex);
               }
               if(!(chunk->emptyFlags & SquareEmpty_1_3))
               {
                  emitTri(ip1, e0->pointIndex, e0->pointIndex + 1);
                  emitTri(ip1, e0->pointIndex + 1, ip5);
               }
               if(!(chunk->emptyFlags & SquareEmpty_1_2))
               {
                  emitTri(ip1, ip5, ip0);
                  emitTri(ip1, ip0, ip8);
               }
               if(!(chunk->emptyFlags & SquareEmpty_0_2))
               {
                  emitTri(ip1, ip8, e3->pointIndex + 1);
                  emitTri(ip1, e3->pointIndex + 1, e3->pointIndex);
               }
               mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            }
         }
         else
         {
            ib = mXFIndexBuffer + mXFIndexCount + 1;
            ib[-1] = GL_TRIANGLE_FAN;
            ib[0] = 10;
            ib[1] = ip1;
            ib[2] = p0;
            ib[3] = e0->pointIndex;
            ib[4] = e0->pointIndex + 1;
            ib[5] = ip5;
            ib[6] = ip0;
            ib[7] = ip8;
            ib[8] = e3->pointIndex + 1;
            ib[9] = e3->pointIndex;
            ib[10] = ib[2];

            mXFIndexCount += 12;
            if(chunk->clip)
               clip(mXFIndexCount - 12);
         }

         if(chunk->emptyFlags & CornerEmpty_1_1)
         {
            if((chunk->emptyFlags & CornerEmpty_1_1) != CornerEmpty_1_1)
            {
               mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLES;
               U32 indexStart = mXFIndexCount++;

               if(!(chunk->emptyFlags & SquareEmpty_3_3))
               {
                  emitTri(ip2, e0->pointIndex + 2, p1);
                  emitTri(ip2, p1, e1->pointIndex);
               }
               if(!(chunk->emptyFlags & SquareEmpty_3_2))
               {
                  emitTri(ip2, e1->pointIndex, e1->pointIndex + 1);
                  emitTri(ip2, e1->pointIndex + 1, ip6);
               }
               if(!(chunk->emptyFlags & SquareEmpty_2_2))
               {
                  emitTri(ip2, ip6, ip0);
                  emitTri(ip2, ip0, ip5);
               }
               if(!(chunk->emptyFlags & SquareEmpty_2_3))
               {
                  emitTri(ip2, ip5, e0->pointIndex + 1);
                  emitTri(ip2, e0->pointIndex + 1, e0->pointIndex + 2);
               }
               mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            }
         }
         else
         {
            ib = mXFIndexBuffer + mXFIndexCount + 1;
            ib[-1] = GL_TRIANGLE_FAN;
            ib[0] = 10;
            ib[1] = ip2;
            ib[2] = p1;
            ib[3] = e1->pointIndex;
            ib[4] = e1->pointIndex + 1;
            ib[5] = ip6;
            ib[6] = ip0;
            ib[7] = ip5;
            ib[8] = e0->pointIndex + 1;
            ib[9] = e0->pointIndex + 2;
            ib[10] = ib[2];

            mXFIndexCount += 12;
            if(chunk->clip)
               clip(mXFIndexCount - 12);
         }

         if(chunk->emptyFlags & CornerEmpty_1_0)
         {
            if((chunk->emptyFlags & CornerEmpty_1_0) != CornerEmpty_1_0)
            {
               mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLES;
               U32 indexStart = mXFIndexCount++;

               if(!(chunk->emptyFlags & SquareEmpty_3_0))
               {
                  emitTri(ip3, e1->pointIndex + 2, p2);
                  emitTri(ip3, p2, e2->pointIndex + 2);
               }
               if(!(chunk->emptyFlags & SquareEmpty_2_0))
               {
                  emitTri(ip3, e2->pointIndex + 2, e2->pointIndex + 1);
                  emitTri(ip3, e2->pointIndex + 1, ip7);
               }
               if(!(chunk->emptyFlags & SquareEmpty_2_1))
               {
                  emitTri(ip3, ip7, ip0);
                  emitTri(ip3, ip0, ip6);
               }
               if(!(chunk->emptyFlags & SquareEmpty_3_1))
               {
                  emitTri(ip3, ip6, e1->pointIndex + 1);
                  emitTri(ip3, e1->pointIndex + 1, e1->pointIndex + 2);
               }
               mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            }
         }
         else
         {
            ib = mXFIndexBuffer + mXFIndexCount + 1;
            ib[-1] = GL_TRIANGLE_FAN;
            ib[0] = 10;
            ib[1] = ip3;
            ib[2] = p2;
            ib[3] = e2->pointIndex + 2;
            ib[4] = e2->pointIndex + 1;
            ib[5] = ip7;
            ib[6] = ip0;
            ib[7] = ip6;
            ib[8] = e1->pointIndex + 1;
            ib[9] = e1->pointIndex + 2;
            ib[10] = ib[2];

            mXFIndexCount += 12;
            if(chunk->clip)
               clip(mXFIndexCount - 12);
         }

         if(chunk->emptyFlags & CornerEmpty_0_0)
         {
            if((chunk->emptyFlags & CornerEmpty_0_0) != CornerEmpty_0_0)
            {
               mXFIndexBuffer[mXFIndexCount++] = GL_TRIANGLES;
               U32 indexStart = mXFIndexCount++;

               if(!(chunk->emptyFlags & SquareEmpty_0_0))
               {
                  emitTri(ip4, e2->pointIndex, p3);
                  emitTri(ip4, p3, e3->pointIndex + 2);
               }
               if(!(chunk->emptyFlags & SquareEmpty_0_1))
               {
                  emitTri(ip4, e3->pointIndex + 2, e3->pointIndex + 1);
                  emitTri(ip4, e3->pointIndex + 1, ip8);
               }
               if(!(chunk->emptyFlags & SquareEmpty_1_1))
               {
                  emitTri(ip4, ip8, ip0);
                  emitTri(ip4, ip0, ip7);
               }
               if(!(chunk->emptyFlags & SquareEmpty_1_0))
               {
                  emitTri(ip4, ip7, e2->pointIndex + 1);
                  emitTri(ip4, e2->pointIndex + 1, e2->pointIndex);
               }
               mXFIndexBuffer[indexStart] = mXFIndexCount - indexStart - 1;
            }
         }
         else
         {
            ib = mXFIndexBuffer + mXFIndexCount + 1;
            ib[-1] = GL_TRIANGLE_FAN;
            ib[0] = 10;
            ib[1] = ip4;
            ib[2] = p3;
            ib[3] = e3->pointIndex + 2;
            ib[4] = e3->pointIndex + 1;
            ib[5] = ip8;
            ib[6] = ip0;
            ib[7] = ip7;
            ib[8] = e2->pointIndex + 1;
            ib[9] = e2->pointIndex;
            ib[10] = ib[2];

            mXFIndexCount += 12;
            if(chunk->clip)
               clip(mXFIndexCount - 12);
         }

      }
   }

   // Do dynamic lighting here...
   if (mEnableTerrainDynLights && chunk->lightMask != 0) {
      for (U32 i = 0; i < 32; i++) {
         if ((chunk->lightMask & (1 << i)) == 0)
            continue;

         for (U32 start = startXFIndex; start < mXFIndexCount; start++) {
            if (mXFIndexBuffer[start] == GL_TRIANGLE_FAN) {
               start++;
               U32 count = mXFIndexBuffer[start];
               U32 triCount = count - 2;

               LightTriangle* lightTris = (LightTriangle*)FrameAllocator::alloc(sizeof(LightTriangle) * triCount);
               U32 j;
               for (j = 0; j < (triCount-1); j++)
                  lightTris[j].next = &lightTris[j+1];
               lightTris[triCount-1].next = sgCurrLightTris;
               sgCurrLightTris = lightTris;

               // Copy out tri data here...
               for (j = 0; j < triCount; j++) {
                  lightTris[j].point1 = mXFVertices[mXFIndexBuffer[start + 1 + 0]];
                  lightTris[j].point2 = mXFVertices[mXFIndexBuffer[start + 1 + j + 1]];
                  lightTris[j].point3 = mXFVertices[mXFIndexBuffer[start + 1 + j + 2]];

                  buildLightTri(&lightTris[j], &mTerrainLights[i]);
               }

               start += count;
            } else {
               AssertFatal(mXFIndexBuffer[start] == GL_TRIANGLES, "Error, bad start code!");
               start++;
               U32 count = mXFIndexBuffer[start];
               AssertFatal((count / 3) * 3 == count, "Error, vertex count not divisible by 3!");

               U32 triCount = count/3;
               LightTriangle* lightTris = (LightTriangle*)FrameAllocator::alloc(sizeof(LightTriangle) * triCount);
               U32 j;
               for (j = 0; j < (triCount-1); j++)
                  lightTris[j].next = &lightTris[j+1];
               lightTris[triCount-1].next = sgCurrLightTris;
               sgCurrLightTris = lightTris;

               // Copy out tri data here...
               for (j = 0; j <  triCount; j++) {
                  lightTris[j].point1 = mXFVertices[mXFIndexBuffer[start + 1 + (j*3) + 0]];
                  lightTris[j].point2 = mXFVertices[mXFIndexBuffer[start + 1 + (j*3) + 1]];
                  lightTris[j].point3 = mXFVertices[mXFIndexBuffer[start + 1 + (j*3) + 2]];

                  buildLightTri(&lightTris[j], &mTerrainLights[i]);
               }

               start += count;
            }
         }
      }
   }
}

/*void TerrainRender::drawTriFan(U32 vCount, U32 *indexBuffer)
{
   glBegin(GL_LINES);
   U32 cur = vCount - 1;
   for(U32 i = 1; i < vCount; i++)
   {
      glArrayElement(indexBuffer[0]);
      glArrayElement(indexBuffer[cur]);
      glArrayElement(indexBuffer[cur]);
      glArrayElement(indexBuffer[i]);
      cur = i;
   }
   glEnd();
}*/

void TerrainRender::renderXFCache()
{
   U32 count = 0;

   while (count < mXFIndexCount)
   {
      U32 mode = mXFIndexBuffer[count];
      U32 vertexCount = mXFIndexBuffer[count + 1];
      glDrawElements(mode, vertexCount, GL_UNSIGNED_SHORT, mXFIndexBuffer + count + 2);
      count += vertexCount + 2;
   }
}

void doTexGens(Point4F &s, Point4F &t)
{
   EdgePoint *pt = mXFVertices;
   EdgePoint *last = pt + mXFPointCount;
   for(;pt < last; pt++)
   {
      pt->haze = pt->x * s.x + pt->y * s.y + pt->z * s.z + s.w;
      pt->distance = pt->x * t.x + pt->y * t.y + pt->z * t.z + t.w;
   }
}

#ifdef TORQUE_OS_WIN32
void TerrainRender::renderD3DBumps(Point2F bumpTextureOffset)
{
    PROFILE_START(TerrainRenderBumpsD3D);

    glColor3f(1.0,1.0,1.0);
    //First pass - normal bump map, no blending, no offset
    doTexGens(bumpTexGenS, bumpTexGenT);
    glBindTexture(GL_TEXTURE_2D, mCurrentBlock->mBumpTextureHandle.getGLName());
    renderXFCache();

    //Second pass - inverted bump map, additive blending, offset
    Point4F bumpTexGenOS = bumpTexGenS;
    Point4F bumpTexGenOT = bumpTexGenT;
    bumpTexGenOS.w += bumpTextureOffset.x;
    bumpTexGenOT.w += bumpTextureOffset.y;
    doTexGens(bumpTexGenOS, bumpTexGenOT);

    glBindTexture(GL_TEXTURE_2D, mCurrentBlock->mInvertedBumpTextureHandle.getGLName());
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);
    renderXFCache();

    //Setup blending for the terrain pass
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);

    PROFILE_END();
}
#endif

void TerrainRender::renderGLBumps(Point2F bumpTextureOffset, U32 hazeName)
{
    PROFILE_START(TerrainRenderBumpsGL);
    //have to set color to white or bumps turn up black on the terrain
    glColor3f(1.0,1.0,1.0);
    doTexGens(bumpTexGenS, bumpTexGenT);

    //Set up TMU #0
    //Normal bump texture
    glBindTexture(GL_TEXTURE_2D, mCurrentBlock->mBumpTextureHandle.getGLName());
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);

    //Set up TMU #1
    //Inverted bump texture
    //Offset the texture matrix
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mCurrentBlock->mInvertedBumpTextureHandle.getGLName());
    glTexCoordPointer(2, GL_FLOAT, sizeof(EdgePoint), &mXFVertices->haze);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_ADD);
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glTranslatef(bumpTextureOffset.x, bumpTextureOffset.y, 0.0);

    //Render!
    renderXFCache();

    //Restore TMU #1
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glBindTexture(GL_TEXTURE_2D, hazeName);
    glTexCoordPointer(2, GL_FLOAT, sizeof(EdgePoint), &mXFVertices->fogRed);

    //Restore TMU #0
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glClientActiveTextureARB(GL_TEXTURE0_ARB);

    //Set up for the bump mapping application
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glEnable(GL_BLEND);
    //Do the rest in the function
    PROFILE_END();
}

void TerrainRender::renderBlock(TerrainBlock *block, SceneState *state)
{
   PROFILE_START(TerrainRender);
   PROFILE_START(TerrainRenderSetup);
   dglSetRenderPrimType(1);
   U32 storedWaterMark = FrameAllocator::getWaterMark();

   if (sgTextureFreeListPrimed == false) {
      sgTextureFreeListPrimed = true;
      AssertFatal(mTextureFreeList.size() == 0, "Error, unprimed free list should always be size 0");

      mTextureFreeList.setSize(sgFreeListPrimeCount);
      for (U32 i = 0; i < sgFreeListPrimeCount; i++) {
         constructInPlace(&mTextureFreeList[i]);
         mTextureFreeList[i] = TextureHandle((const char*)NULL, mBlendBitmap, TerrainTexture, true);
      }
   }

   mFrameIndex++;
   mSceneState = state;
   mFarDistance = state->getVisibleDistance();

   dglGetModelview(&mCameraToObject);

   mCameraToObject.inverse();

   mCameraToObject.getColumn(3, &mCamPos);
   mFogColor = state->getFogColor();

   TextureHandle hazeTexture = gClientSceneGraph->getFogTexture();
   mCurrentBlock = block;

   Point4F detTexGenS = Point4F(1, 0, 0, 0);
   Point4F detTexGenT = Point4F(0, 1, 0, 0);
   if (mEnableTerrainDetails && mCurrentBlock->mDetailTextureHandle.getGLName() != 0) {
      detTexGenS.x *= 62.0 / mCurrentBlock->mDetailTextureHandle.getWidth();
      detTexGenT.y *= 62.0 / mCurrentBlock->mDetailTextureHandle.getHeight();
      detTexGenS.w = -(S32) (mCamPos.x*detTexGenS.x);
      detTexGenT.w = -(S32) (mCamPos.y*detTexGenT.y);
   }
// CW - stuff with bump maps
    //calculate the texture offset
    if (mEnableTerrainEmbossBumps && mCurrentBlock->mBumpTextureHandle.getGLName() != 0)
    {
        PROFILE_START(TerrainRenderBumpBuild);
        //There should be a better way to do this, really...
#ifdef TORQUE_OS_WIN32
        if (dStrcmp(Con::getVariable( "$pref::Video::displayDevice" ), "OpenGL") == 0)
            mRenderGL = true;
        else
            mRenderGL = false;
#endif
        //note: don't multiply light by inverse modelview matrix
        //because it is already in object space (or world or anything, really)
        //The first light in the light manager is always the sun.
        //This is also how players are lit, so if this is broken, that is broken as well.

        Point3F sTangent,tTangent;
        static Vector<LightInfo*> lights;
        lights.clear();
        gClientSceneGraph->getLightManager()->getLights(lights);
        VectorF sunVector = -lights[0]->mDirection; //first light is always sun

        //find s and t tangents
        F32 pHeight[4];
        for (int i = 0; i < 4; i++)
        {
            int x = i == 0 || i == 3 ? 0 : 255;
            int y = i == 1 || i == 2 ? 0 : 255;
            block->getHeight(Point2F(x,y), &pHeight[i]);
        }

        S32 pSquareSize = block->getSquareSize();
        sTangent.x = pSquareSize;
        sTangent.y = pHeight[3] - pHeight[0];
        sTangent.z = 0;
        tTangent.x = 0;
        tTangent.y = pHeight[1] - pHeight[0];
        tTangent.z = pSquareSize;
        sTangent.normalize();
        tTangent.normalize();
        // The above is needed for both emboss and dot3 bump mapping
        // techniques.  However, the below 2 lines are emboss-specific.
        // So, dot3 bump mapping would not be too difficult because
        // I have already computed most of the necessary info.
        bumpTextureOffset.x = mDot(sTangent, sunVector) * mCurrentBlock->mBumpOffset;
        bumpTextureOffset.y = mDot(tTangent, sunVector) * mCurrentBlock->mBumpOffset;
        PROFILE_END();
    }
// CW - end bump map stuff
   mSquareSize = block->getSquareSize();

//   mNewGenTextureCount = 0;

   // compute pixelError
   if(mScreenError >= 0.001)
      mPixelError = 1 / dglProjectRadius(mScreenError, 1);
   else
      mPixelError = 0.000001;

   buildClippingPlanes(state->mFlipCull);
   buildLightArray();

   F32 worldToScreenScale   = dglProjectRadius(1,1);
   F32 zeroDetailDistance   = (mSquareSize * worldToScreenScale) / (1 << 6) - (mSquareSize >> 1);
   F32 zeroBumpDistance      = (mSquareSize * worldToScreenScale) / (1 << mCurrentBlock->mZeroBumpScale) - (mSquareSize >> 1);

   F32 blockSize = mSquareSize * TerrainBlock::BlockSquareWidth;
   S32 xStart = (S32)mFloor( (mCamPos.x - mFarDistance) / blockSize );
   S32 xEnd   = (S32)mCeil ( (mCamPos.x + mFarDistance) / blockSize );
   S32 yStart = (S32)mFloor( (mCamPos.y - mFarDistance) / blockSize );
   S32 yEnd   = (S32)mCeil ( (mCamPos.y + mFarDistance) / blockSize );
   S32 xExt   = (S32)(xEnd - xStart);
   S32 yExt   = (S32)(yEnd - yStart);

   PROFILE_END();
   PROFILE_START(TerrainRenderRecurse);
   F32 height = fixedToFloat(block->getHeight(0,0));

   EdgeParent **bottomEdges = (EdgeParent **) FrameAllocator::alloc(sizeof(ChunkScanEdge *) * xExt);
   TerrainRender::allocRenderEdges(xExt, bottomEdges, false);
   ChunkCornerPoint *prevCorner = TerrainRender::allocInitialPoint(Point3F(xStart * blockSize, yStart * blockSize, height));

   mTerrainOffset.set(xStart * TerrainBlock::BlockSquareWidth, yStart * TerrainBlock::BlockSquareWidth);

   for(S32 x = 0; x < xExt; x++)
   {
      bottomEdges[x]->p1 = prevCorner;
      prevCorner = TerrainRender::allocInitialPoint(Point3F((xStart + x ) * blockSize, yStart * blockSize, height));
      bottomEdges[x]->p2 = prevCorner;
   }
   for(S32 y = 0; y < yExt; y++)
   {
      // allocate the left edge:
      EdgeParent *left;
      TerrainRender::allocRenderEdges(1, &left, false);
      left->p1 = TerrainRender::allocInitialPoint(Point3F(xStart * blockSize, (yStart + y + 1) * blockSize, height));
      left->p2 = bottomEdges[0]->p1;
      for(S32 x = 0; x < xExt; x++)
      {
         EdgeParent *right;
         TerrainRender::allocRenderEdges(1, &right, false);
         right->p1 = TerrainRender::allocInitialPoint(Point3F((xStart + x + 1) * blockSize, (yStart + y + 1) * blockSize, height));
         right->p2 = bottomEdges[x]->p2;
         EdgeParent *top;
         TerrainRender::allocRenderEdges(1, &top, false);
         top->p1 = left->p1;
         top->p2 = right->p1;

         mBlockOffset.set(x << TerrainBlock::BlockShift,
                          y << TerrainBlock::BlockShift);

         mBlockPos.set((xStart + x) * blockSize,
                       (yStart + y) * blockSize);

         TerrainRender::processCurrentBlock(state, top, right, bottomEdges[x], left);
         left = right;
         bottomEdges[x] = top;
      }
   }
   U32 slop = 0;
   AllocatedTexture *fwalk = mTextureFreeListHead.next;
   while(fwalk != &mTextureFreeListTail && slop < mTextureSlopSize)
   {
      fwalk = fwalk->next;
      slop++;
   }
   while(fwalk != &mTextureFreeListTail)
   {
      AllocatedTexture *next = fwalk->next;
      fwalk->unlink();

      S32 x = (fwalk->x & TerrainBlock::BlockMask) >> fwalk->level;
      S32 y = (fwalk->y & TerrainBlock::BlockMask) >> fwalk->level;
      mTextureGridPtr[fwalk->level - 2][x + (y << (8 - fwalk->level))] = NULL;

      freeTerrTexture(fwalk);
      fwalk = next;
   }

   PROFILE_END();
   PROFILE_START(TerrainRenderEmit);

   bool lockArrays = dglDoesSupportCompiledVertexArray() && dglDoesSupportARBMultitexture();
   bool multitextureFog = dglDoesSupportARBMultitexture(); // dc 6-1-01 - this flag is BAD.  The non mtex path doesn't currently work.  !!!!!!TBD
   bool vertexBuffer = dglDoesSupportVertexBuffer() && (block->mVertexBuffer != -1);

   glFrontFace(GL_CW);
   glEnable(GL_CULL_FACE);
   glEnableClientState(GL_VERTEX_ARRAY);

   if(!vertexBuffer)
      mXFVertices = (EdgePoint *) FrameAllocator::alloc(sizeof(EdgePoint) * VertexBufferSize);

    //CW - D3D goes crazy if we stick our bump colors in the EdgePoint structure, so make a new one
    ColorI *bumpColors = (ColorI*)FrameAllocator::alloc(sizeof(ColorI) * VertexBufferSize);

   glVertexPointer(3, GL_FLOAT, sizeof(EdgePoint), mXFVertices);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);

#if ENABLE_NPATCH && TERRAIN_STORE_NORMALS // test both for safety here.  everything else just causes compute overhead...
   bool wasLighting = glIsEnabled(GL_LIGHTING);
   if (gNPatch && gNPatchTerrain)
   {
      glEnable(GL_LIGHTING);
      glEnableClientState(GL_NORMAL_ARRAY);
      glNormalPointer(GL_FLOAT, sizeof(EdgePoint), &(mXFVertices->vn));
      // and since I'm not guaranteed to have unit-normals:
      glEnable(GL_NORMALIZE);

      dglNPatchBegin(); // before setting custom terrain tess level.
      if (gNPatchTerrainLevel != -1) // custom NPatch tessellation.
         dglSetNPatchLevel(gNPatchTerrainLevel);
   }
   else
      glDisable(GL_LIGHTING);
#endif

  // see if we should setup hazing on the second TMU
   // if not, we'll have to 2-pass the terrain...
   // and that ain't gonna be fast no matter how ya slice it.

   if(multitextureFog)
   {
      // Hazing
      glClientActiveTextureARB(GL_TEXTURE1_ARB);
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, hazeTexture.getGLName()); //handle

         glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT, sizeof(EdgePoint), &mXFVertices->fogRed);

      // Base textures
      glClientActiveTextureARB(GL_TEXTURE0_ARB);
      glActiveTextureARB(GL_TEXTURE0_ARB);
   }
   else
      glTexCoordPointer(2, GL_FLOAT, sizeof(EdgePoint), &mXFVertices->fogRed);

   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(EdgePoint), &mXFVertices->haze);


   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   mXFIndex = 0;
   sgCurrLightTris = NULL;
   AllocatedTexture *walk;
   mXFIndexBuffer = (U16 *) FrameAllocator::alloc(sizeof(U16) * 64 * 64 * 4);

   while((walk = mTextureFrameListHead.next) != &mTextureFrameListTail)
   {
      walk->unlink();
      if(walk->level != 6)
         walk->linkAfter(&mTextureFreeListHead);
      else
         walk->linkAfter(&mTextureFreeBigListHead);
      mStaticTextureCount++;
      if(!walk->handle)
         buildBlendMap(walk);
      AllocatedTexture *step = walk;
      while(step)
      {
         // loop through the list and draw all the squares:
         F32 invLevel = 1 / F32(mSquareSize << step->level);
         sgTexGenS.set(invLevel, 0, 0, -(step->x >> step->level));
         sgTexGenT.set(0, invLevel, 0, -(step->y >> step->level));

         // Bump map texture coordinate "calculation" (hack) for ATi cards -CW
         // Why don't Point4F's have a multiply operator?
         const F32 scale = 32 / mCurrentBlock->mBumpScale;
         bumpTexGenS.x = sgTexGenS.x * scale;
         bumpTexGenS.y = sgTexGenS.y * scale;
         bumpTexGenS.z = sgTexGenS.z * scale;
         bumpTexGenS.w = sgTexGenS.w * scale;
         bumpTexGenT.x = sgTexGenT.x * scale;
         bumpTexGenT.y = sgTexGenT.y * scale;
         bumpTexGenT.z = sgTexGenT.z * scale;
         bumpTexGenT.w = sgTexGenT.w * scale;
         // End -CW

        while(step->list)
         {
            mXFPointCount = 0;
            mXFIndex++;
            mXFIndexPtr = mXFIndexBuffer;
            mXFIndexCount = 0;

            PROFILE_START(TerrainRenderLock);
              if (vertexBuffer)
                  mXFVertices = (EdgePoint *) glLockVertexBufferEXT(block->mVertexBuffer, VertexBufferSize);
            PROFILE_END();

            PROFILE_START(TerrainRenderBuild);

            bool renderDetails = false;
// CW - stuff with bump maps
            bool renderBumps = false;
// CW - end bump map stuff
            EmitChunk *sq;
            for(sq = step->list; sq && mXFPointCount < maxTerrPoints; sq = sq->next)
            {
               if(mRenderingCommander)
               {
                  renderChunkCommander(sq);
               }
               else
               {
                  renderDetails |= sq->renderDetails && mEnableTerrainDetails;
// CW - stuff with bump maps
                  renderBumps |= sq->renderBumps && mEnableTerrainEmbossBumps;
// CW - end bump map stuff
                  renderChunkOutline(sq);
               }
               AssertFatal(mXFPointCount <= VertexBufferSize, "Invalid point count.");
               AssertFatal(mXFIndexCount <= 64 * 64 * 4, "Index count sucks.");
            }
            step->list = sq;

              if (mXFIndexCount && renderDetails && mCurrentBlock->mDetailTextureHandle.getGLName() != 0)
              {
                  for (U32 a = 0; a < mXFPointCount; a++)
                  {
                      EdgePoint& rPoint = mXFVertices[a];

                      F32 factor = state->getHazeAndFog(rPoint.distance, rPoint.z);
                      F32 distFactor = (zeroDetailDistance - rPoint.distance) / zeroDetailDistance;
                      if (distFactor < 0.0)
                          distFactor = 0.0;
                      else if (distFactor > 1.0)
                          distFactor = 1.0f;

                      U8 c = U8((((1.0 - factor) * distFactor) * 255.0f) + 0.5f);

                      rPoint.detailColor.red   = c;
                      rPoint.detailColor.green = c;
                      rPoint.detailColor.blue  = c;
                      rPoint.detailColor.alpha = c;
                  }
              }

                // for bumps, it shouldn't fade out purely linearly
                // instead, only start linear fade out at 3/4 out
              if (mXFIndexCount && renderBumps && mCurrentBlock->mBumpTextureHandle.getGLName() != 0)
              {
                  for (U32 a = 0; a < mXFPointCount; a++)
                  {
                      EdgePoint& rPoint = mXFVertices[a];

                        F32 factor = state->getHazeAndFog(rPoint.distance, rPoint.z);
                      F32 distFactor = rPoint.distance / zeroBumpDistance;
                      if (distFactor < 0.0)
                          distFactor = 0.0;
                      else if (distFactor > 1.0)
                          distFactor = 1.0f;

                      U8 c = 0;
                        if (distFactor > 0.75)
                            c = U8( (distFactor - 0.75) * 4 * 255.0f + 0.5f );
                        else
                            c = 0;

                      bumpColors[a].red   = 127;
                      bumpColors[a].green = 127;
                      bumpColors[a].blue  = 127;
                      bumpColors[a].alpha = c;
                  }
              }
            PROFILE_END();
            PROFILE_START(TerrainRenderUnlock);
            if (vertexBuffer)
               glUnlockVertexBufferEXT(block->mVertexBuffer);
            PROFILE_END();

// Render the bump maps! -CW
            glDisable(GL_BLEND);
            if (renderBumps && mCurrentBlock->mBumpTextureHandle.getGLName() != 0 && mCurrentBlock->mInvertedBumpTextureHandle.getGLName() != 0 && mEnableTerrainEmbossBumps)
            {
#ifdef TORQUE_OS_WIN32
               if ( mRenderGL )
                  renderGLBumps(bumpTextureOffset, hazeTexture.getGLName());
               else
                  // D3D sucks...
                  renderD3DBumps(bumpTextureOffset);
#else
               renderGLBumps(bumpTextureOffset, hazeTexture.getGLName());
#endif
                    //add a little fade-out
                    glEnable(GL_BLEND);
                    glDisable(GL_TEXTURE_2D);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
               glEnableClientState(GL_COLOR_ARRAY);
                    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ColorI), bumpColors);

               if(multitextureFog)
               {
                  glActiveTextureARB(GL_TEXTURE1_ARB);
                  glDisable(GL_TEXTURE_2D);
                  renderXFCache();
                  glEnable(GL_TEXTURE_2D);
                  glActiveTextureARB(GL_TEXTURE0_ARB);
               }
               else
               {
                  renderXFCache();
               }

                    glEnable(GL_TEXTURE_2D);
                    glDisableClientState(GL_COLOR_ARRAY);
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
                    glColor3f(1.0,1.0,1.0);
             }
// CW - end bump map render

            if(mXFIndexCount)
            {
               PROFILE_START(TerrainRenderBind);
//               glBindTexture(GL_TEXTURE_2D, hazeTexture.getGLName()); //test code to see hazetex...
               glBindTexture(GL_TEXTURE_2D, walk->handle.getGLName()); //handle
               //glTexGenfv(GL_S, GL_OBJECT_PLANE, sgTexGenS);
               //glTexGenfv(GL_T, GL_OBJECT_PLANE, sgTexGenT);
               doTexGens(sgTexGenS, sgTexGenT);

               PROFILE_END();
               PROFILE_START(TerrainRenderSetVertexBuffer);

               if (vertexBuffer)
                  glSetVertexBufferEXT(block->mVertexBuffer);
               if (lockArrays)
                  glLockArraysEXT(0, mXFPointCount);
               // lock the array
               PROFILE_END();
               PROFILE_START(TerrainRenderXFRender);
               renderXFCache();
               PROFILE_END();

               if(!multitextureFog)
               {
                  glDisable(GL_TEXTURE_GEN_S);
                  glDisable(GL_TEXTURE_GEN_T);
                  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                  glColor4fv(mFogColor);
                  glBindTexture(GL_TEXTURE_2D, hazeTexture.getGLName()); //handle
                  glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, mFogColor);

                  glEnable(GL_BLEND);
                  renderXFCache();
                  glDisable(GL_BLEND);

                  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                  glEnable(GL_TEXTURE_GEN_S);
                  glEnable(GL_TEXTURE_GEN_T);
               }

               if (renderDetails && mCurrentBlock->mDetailTextureHandle.getGLName() != 0)
               {
                  PROFILE_START(TerrainRenderDetails);
                  glEnableClientState(GL_COLOR_ARRAY);
                      glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(EdgePoint), &mXFVertices->detailColor);
                  glBindTexture(GL_TEXTURE_2D, mCurrentBlock->mDetailTextureHandle.getGLName());
                  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                    if (vertexBuffer)
                        mXFVertices = (EdgePoint *) glLockVertexBufferEXT(block->mVertexBuffer, VertexBufferSize);
                  doTexGens(detTexGenS, detTexGenT);
                    if (vertexBuffer)
                        glUnlockVertexBufferEXT(block->mVertexBuffer);
                  //glTexGenfv(GL_S, GL_OBJECT_PLANE, detTexGenS);
                  //glTexGenfv(GL_T, GL_OBJECT_PLANE, detTexGenT);

                  glEnable(GL_BLEND);
                  glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
                  if(multitextureFog)
                  {
                     glActiveTextureARB(GL_TEXTURE1_ARB);
                     glDisable(GL_TEXTURE_2D);
                     renderXFCache();
                     glEnable(GL_TEXTURE_2D);
                     glActiveTextureARB(GL_TEXTURE0_ARB);
                  }
                  else
                  {
                     renderXFCache();
                  }
                  glDisable(GL_BLEND);
                  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                  glDisableClientState(GL_COLOR_ARRAY);
                  PROFILE_END();
               }

               if (lockArrays)
                  glUnlockArraysEXT();
            }
         }
         AllocatedTexture *nextStep = step->nextLink;
         step->list = 0;
         step->nextLink = 0;
         if(step != walk)
            delete step;
         step = nextStep;
      }
   }

   if(multitextureFog)
   {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glClientActiveTextureARB(GL_TEXTURE1_ARB);
      glDisable(GL_TEXTURE_2D);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);

      glActiveTextureARB(GL_TEXTURE0_ARB);
      glClientActiveTextureARB(GL_TEXTURE0_ARB);
   }
   glDisable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
//   glDisable(GL_TEXTURE_GEN_S);
//   glDisable(GL_TEXTURE_GEN_T);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);

   glDisableClientState(GL_VERTEX_ARRAY);


   if (mEnableTerrainDynLights && sgCurrLightTris) {
      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glBindTexture(GL_TEXTURE_2D, mCurrentBlock->mDynLightTexture.getGLName());
      LightTriangle* walk = sgCurrLightTris;

      glBegin(GL_TRIANGLES);
      while (walk) {
         if (walk->flags) {
            glColor4fv(walk->color);
            glTexCoord2fv(walk->texco1);
            glVertex3fv(walk->point1);
            glColor4fv(walk->color);
            glTexCoord2fv(walk->texco2);
            glVertex3fv(walk->point2);
            glColor4fv(walk->color);
            glTexCoord2fv(walk->texco3);
            glVertex3fv(walk->point3);
         }
         walk = walk->next;
      }
      glEnd();

      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glDisable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ZERO);
   }


#if ENABLE_NPATCH && TERRAIN_STORE_NORMALS
   if (gNPatch && gNPatchTerrain)
   { // then need to reset GL state.
      glDisableClientState(GL_NORMAL_ARRAY);
      glDisable(GL_NORMALIZE); // !!!!!TBD -- should I be saving/restoring state???
      if (gNPatchTerrainLevel != -1) // custom NPatch tessellation.
        dglSetNPatchLevel(gNPatchLevel); // reset to global setting.
      dglNPatchEnd();
   }
   if (!wasLighting) // try to keep the state
      glDisable(GL_LIGHTING);
#endif

   glDisable(GL_CULL_FACE);

   FrameAllocator::setWaterMark(storedWaterMark);
   dglSetRenderPrimType(0);
   PROFILE_END();
   PROFILE_END();
}



//---------------------------------------------------------------

//---------------------------------------------------------------
// Texture allocation / free list management
//---------------------------------------------------------------

void TerrainRender::flushCache()
{
   for(S32 i = 0; i < AllocatedTextureCount; i++)
      mTextureGrid[i] = 0;

   for(S32 i = 0; i < mTextureFreeList.size(); i++)
      mTextureFreeList[i] = NULL;
   mTextureFreeList.clear();
   AllocatedTexture *tex;

   while( (tex = mTextureFreeListHead.next) != &mTextureFreeListTail)
   {
      tex->unlink();
      delete tex;
   }
   while( (tex = mTextureFreeBigListHead.next) != &mTextureFreeBigListTail)
   {
      tex->unlink();
      delete tex;
   }
}

void TerrainRender::flushCacheRect(RectI bRect)
{
   bRect.inset(-1,-1);
   for(S32 i = 0; i < AllocatedTextureCount; i++)
   {
      AllocatedTexture *tex = mTextureGrid[i];
      if(!tex)
         continue;
      RectI texRect(tex->x, tex->y, 1 << tex->level, 1 << tex->level);
      if(texRect.intersect(bRect))
      {
         mTextureGrid[i] = NULL;
         tex->unlink();
         freeTerrTexture(tex);
      }
   }
}

//---------------------------------------------------------------

void TerrainRender::freeTerrTexture(AllocatedTexture *texture)
{
   if(texture->handle)
   {
      mTextureFreeList.increment();
      constructInPlace(&mTextureFreeList.last());
      mTextureFreeList.last() = texture->handle;
   }
   delete texture;
}

//---------------------------------------------------------------

void TerrainRender::allocTerrTexture(Point2I pos, U32 level, U32 mipLevel, bool vis, F32 distance)
{
   vis;
   S32 blockX = mBlockOffset.x + mTerrainOffset.x;
   S32 blockY = mBlockOffset.y + mTerrainOffset.y;

   S32 x = pos.x + blockX;
   S32 y = pos.y + blockY;

   S32 px = (pos.x & TerrainBlock::BlockMask) >> level;
   S32 py = (pos.y & TerrainBlock::BlockMask) >> level;

   AssertFatal(level >= 2 && level <= 6, "Invalid level");

   AllocatedTexture *cur = mTextureGridPtr[level - 2][px + (py << (8 - level))];

   if(!cur)
   {
      cur = new AllocatedTexture;

      cur->list = NULL;
      cur->mipLevel = mipLevel;
      cur->x = x;
      cur->y = y;
      cur->level = level;
      cur->nextLink = NULL;
   }
   else
   {
      AssertFatal(cur->level == level, "Invalid block for this level.");
      if(cur->list && (cur->x != x || cur->y != y))
      {
         // see if the texture is already in the list...
         AllocatedTexture *walk = cur->nextLink;
         while(walk && walk->x != x && walk->y != y)
            walk = walk->nextLink;
         if(walk)
         {
            mCurrentTexture = walk;
            return;
         }
         AllocatedTexture *tail = new AllocatedTexture;
         tail->list = NULL;
         tail->x = x;
         tail->y = y;
         tail->level = level;
         tail->nextLink = cur->nextLink;
         tail->distance = distance;
         cur->nextLink = tail;
         mCurrentTexture = tail;
         return;
      }
      else
      {
         cur->x = x;
         cur->y = y;
         cur->unlink();
      }
   }

   cur->linkAfter(&mTextureFrameListHead);
   cur->distance = distance;

   mCurrentTexture = cur;

   mTextureGridPtr[level - 2][px + (py << (8 - level))] = cur;
}

void TerrainRender::buildBlendMap(AllocatedTexture *tex)
{
   PROFILE_START(TerrainRenderBuildBlendMap);
   GBitmap *bmp = mBlendBitmap;
   S32 x = tex->x;
   S32 y = tex->y;
   S32 level = tex->level;

   AssertFatal(mCurrentBlock->lightMap->getFormat() == GBitmap::RGB5551, "Error, lightmap must be 5551");
   AssertFatal(bmp->getFormat() == GBitmap::RGB5551, "Error, destination must be 565");
   AssertFatal(bmp->getWidth() == TerrainTextureSize && bmp->getHeight() == TerrainTextureSize, avar("Error, bitmaps must be %d high and wide for the terrain", TerrainTextureSize));
   AssertFatal(mCurrentBlock->lightMap->getWidth() == 512 && mCurrentBlock->lightMap->getHeight() == 512,
               "Fast blender requires a 512 lightmap!");

   U16* mips[TerrainTextureMipLevel + 1];
   for (U32 i = 0; i < bmp->getNumMipLevels(); i++)
      mips[i] = (U16*)bmp->getWritableBits(i);

   mCurrentBlock->mBlender->blend(x, y, level, (const U16*)mCurrentBlock->lightMap->getBits(), mips);

   mDynamicTextureCount++;
   if(mTextureFreeList.size())
   {
      tex->handle = mTextureFreeList.last();
      mTextureFreeList.last() = NULL;
      mTextureFreeList.decrement();
      tex->handle.refresh(mBlendBitmap);
   }
   else
   {
      tex->handle = TextureHandle((const char*)NULL, mBlendBitmap, TerrainTexture, true);
   }
   PROFILE_END();
}
