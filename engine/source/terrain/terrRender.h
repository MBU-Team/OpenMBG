//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TERRRENDER_H_
#define _TERRRENDER_H_

#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _WATERBLOCK_H_
#include "terrain/waterBlock.h"
#endif

struct EmitChunk;

struct AllocatedTexture {
   U32 level;
   S32 x, y;
   F32 distance;
   EmitChunk *list;
   TextureHandle handle;
   AllocatedTexture *next;
   AllocatedTexture *previous;
   AllocatedTexture *nextLink;
   U32 mipLevel;

   AllocatedTexture()
   {
      next = previous = NULL;
   }
   inline void unlink()
   {
      AssertFatal(next && previous, "Invalid unlink.");
      next->previous = previous;
      previous->next = next;
      next = previous = NULL;
   }
   inline void linkAfter(AllocatedTexture *t)
   {
      AssertFatal(next == NULL && previous == NULL, "Cannot link a non-null next & prev");

      next = t->next;
      previous = t;
      t->next->previous = this;
      t->next = this;
   }
};

struct Render2Point : public Point3F
{
   F32 d;
};

struct EdgePoint : public Point3F
{
   ColorI detailColor;
   F32 haze;
   F32 distance;
   F32 fogRed;
   F32 fogGreen;

#if TERRAIN_STORE_NORMALS
   Point3F vn;
#endif
};

struct ChunkCornerPoint : public EdgePoint
{
   U32 pointIndex;
   U32 xfIndex;
};

struct EdgeParent
{
   ChunkCornerPoint *p1, *p2;
};

struct ChunkScanEdge : public EdgeParent
{
   ChunkCornerPoint *mp;
   EdgeParent *e1, *e2;
};

struct ChunkEdge : public EdgeParent
{
   U32 xfIndex;
   U32 pointIndex;
   U32 pointCount;
   
   EdgePoint pt[3];
   EmitChunk *c1, *c2;
};

struct EmitChunk
{
   ChunkEdge *edge[4];
   S32 subDivLevel;
   F32 growFactor;
   S32 x, y;
   S32 gridX, gridY;
   U32 emptyFlags;
   bool clip;
   U32 lightMask;
   EmitChunk *next;
   bool renderDetails;
//CW - bump mapping stuff
	bool renderBumps;
//CW - end bump mapping stuff
};

struct SquareStackNode2
{
   U32 clipFlags;
   U32 lightMask;
   Point2I pos;
   U32  level;
   bool texAllocated;
};

struct SquareStackNode
{
   U32 clipFlags;
   U32 lightMask;
   Point2I pos;
   U32  level;
   bool  texAllocated;
   EdgeParent *top, *right, *bottom, *left;
};

struct TerrLightInfo
{
   Point3F pos;       ///< world position
   F32 radius;        ///< radius of the light
   F32 radiusSquared; ///< radius^2
   F32 r, g, b;

   F32 distSquared; // distance to camera
};

enum EmptyFlags {
   SquareEmpty_0_0 = BIT(0),
   SquareEmpty_1_0 = BIT(1),
   SquareEmpty_2_0 = BIT(2),
   SquareEmpty_3_0 = BIT(3),
   SquareEmpty_0_1 = BIT(4),
   SquareEmpty_1_1 = BIT(5),
   SquareEmpty_2_1 = BIT(6),
   SquareEmpty_3_1 = BIT(7),
   SquareEmpty_0_2 = BIT(8),
   SquareEmpty_1_2 = BIT(9),
   SquareEmpty_2_2 = BIT(10),
   SquareEmpty_3_2 = BIT(11),
   SquareEmpty_0_3 = BIT(12),
   SquareEmpty_1_3 = BIT(13),
   SquareEmpty_2_3 = BIT(14),
   SquareEmpty_3_3 = BIT(15),
   CornerEmpty_0_0 = SquareEmpty_0_0 | SquareEmpty_1_0 | SquareEmpty_0_1 | SquareEmpty_1_1,
   CornerEmpty_1_0 = SquareEmpty_2_0 | SquareEmpty_3_0 | SquareEmpty_2_1 | SquareEmpty_3_1,
   CornerEmpty_0_1 = SquareEmpty_0_2 | SquareEmpty_1_2 | SquareEmpty_0_3 | SquareEmpty_1_3,
   CornerEmpty_1_1 = SquareEmpty_2_2 | SquareEmpty_3_2 | SquareEmpty_2_3 | SquareEmpty_3_3,
};   

struct RenderPoint : public Point3F
{
   F32 dist;
   F32 haze; ///< also used as grow factor
};

enum TerrConstants {
   MaxClipPlanes = 8, ///< left, right, top, bottom - don't need far tho...
   MaxTerrainMaterials = 256,

   EdgeStackSize = 1024, ///< value for water/terrain edge stack size.
   MaxWaves = 8,
   MaxDetailLevel = 9,
   MaxMipLevel = 8,
   MaxTerrainLights = 64,
   MaxVisibleLights = 31,
   ClipPlaneMask = (1 << MaxClipPlanes) - 1, 
   FarSphereMask = 0x80000000,
   FogPlaneBoxMask = 0x40000000,
   VertexBufferSize = 65 * 65 + 1000,
   AllocatedTextureCount = 16 + 64 + 256 + 1024 + 4096,

   TerrainTextureMipLevel = 7, ///< mip level of generated textures   
   TerrainTextureSize = 1 << TerrainTextureMipLevel, ///< size of generated textures
   SmallMipLevel = 6
};

struct Color
{
   S32 r, g, b;
   F32 z;
};

class SceneState;

struct TerrainRender
{
   static MatrixF mCameraToObject;
   static AllocatedTexture mTextureFrameListHead;
   static AllocatedTexture mTextureFrameListTail;
   static AllocatedTexture mTextureFreeListHead;
   static AllocatedTexture mTextureFreeListTail;
   static AllocatedTexture mTextureFreeBigListHead;
   static AllocatedTexture mTextureFreeBigListTail;
   static U32 mTextureSlopSize;
   static Vector<TextureHandle> mTextureFreeList;
   static S32 mTextureMinSquareSize;
   
   static SceneState *mSceneState;
   static AllocatedTexture *mCurrentTexture;
   
   static TerrainBlock *mCurrentBlock;
   static S32 mSquareSize;
   static F32 mScreenSize;
   static U32 mFrameIndex;
   static U32 mNumClipPlanes;
   static AllocatedTexture *mTextureGrid[AllocatedTextureCount];
   static AllocatedTexture **mTextureGridPtr[5];
   
   static Point2F mBlockPos;
   static Point2I mBlockOffset;
   static Point2I mTerrainOffset;
   
   static PlaneF mClipPlane[MaxClipPlanes];
   static Point3F mCamPos;
   static TextureHandle* mGrainyTexture;
   static U32 mDynamicLightCount;
   static bool mEnableTerrainDetails;
   static bool mEnableTerrainEmbossBumps;
   static bool mEnableTerrainDynLights;

   static F32 mPixelError;

#ifdef TORQUE_OS_WIN32
   //only need this in win32
   static bool mRenderGL;
#endif

   static TerrLightInfo mTerrainLights[MaxTerrainLights];
   static F32 mScreenError;
   static F32 mMinSquareSize;
   static F32 mFarDistance;
   static S32 mDynamicTextureCount;
   static S32 mStaticTextureCount;
   static bool mRenderingCommander;

   static ColorF mFogColor;

   static bool mRenderOutline;
   static U32  mMaterialCount;

   static GBitmap* mBlendBitmap;

   static void init();
   static void shutdown();

   static void allocRenderEdges(U32 edgeCount, EdgeParent **dest, bool renderEdge);
   static void subdivideChunkEdge(ChunkScanEdge *e, Point2I pos, bool chunkEdge);
   static void processCurrentBlock(SceneState* state, EdgeParent *topEdge, EdgeParent *rightEdge, EdgeParent *bottomEdge, EdgeParent *leftEdge);
   static ChunkCornerPoint *allocInitialPoint(Point3F pos);
   static ChunkCornerPoint *allocPoint(Point2I pos);
   static void emitTerrChunk(SquareStackNode *n, F32 squareDistance, U32 lightMask, bool farClip, bool useDetails, bool useBumps);
   static void renderChunkOutline(EmitChunk *chunk);
   static void renderChunkCommander(EmitChunk *chunk);
   static void fixEdge(ChunkEdge *edge, S32 x, S32 y, S32 dx, S32 dy);
   static U32 constructPoint(S32 x, S32 y);
   static U32 interpPoint(U32 p1, U32 p2, S32 x, S32 y, F32 growFactor);
   static void addEdge(ChunkEdge *edge);
   static void clip(U32 triFanStart);

   static F32 getScreenError()            { return(mScreenError); }
   static void setScreenError(F32 error)  { mScreenError = error; }

   static void flushCache();
   static void flushCacheRect(RectI rect);
   
   static void allocTerrTexture(Point2I pos, U32 level, U32 mipLevel, bool vis, F32 distance);
   static void freeTerrTexture(AllocatedTexture *texture);
   static void buildBlendMap(AllocatedTexture *texture);

   static U32 TestSquareLights(GridSquare *sq, S32 level, Point2I pos, U32 lightMask);
   static S32 TestSquareVisibility(Point3F &min, Point3F &max, S32 clipMask, F32 expand);

   static void subdivideEdge(S32 edge, Point2I pos);
   static F32 getSquareDistance(const Point3F& minPoint, const Point3F& maxPoint,
                                F32* zDiff);

   //GL bumps is faster - uses multitexturing and adds 1 small pass
   static void renderGLBumps(Point2F bumpTextureOffset, U32 hazeName);

#ifdef TORQUE_OS_WIN32
   //D3D bumps is slower - adds 2 small passes
   //no need for a D3D render function when not on windows
   static void renderD3DBumps(Point2F bumpTextureOffset);
#endif

   static void buildLightArray();
   static void buildClippingPlanes(bool flipClipPlanes);
   static void buildDetailTable();
   
   static void renderXFCache();
   static void renderBlock(TerrainBlock *, SceneState *state);
};

#endif
