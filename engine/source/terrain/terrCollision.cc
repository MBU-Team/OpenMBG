
//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/terrData.h"
#include "dgl/dgl.h"
#include "editor/editor.h"

const F32 TerrainThickness = 0.5f;
static const U32 MaxExtent = 100;
#define MAX_FLOAT 1e20f


//----------------------------------------------------------------------------

Convex sTerrainConvexList;

// Number of vertices followed by point index
S32 sVertexList[5][5] = {
   { 3, 1,2,3 },  // 135 B
   { 3, 0,1,3 },  // 135 A
   { 3, 0,2,3 },  // 45 B
   { 3, 0,1,2 },  // 45 A
   { 4, 0,1,2,3 } // Convex square
};

#if 1
// This section (as opposed to the #else down below) has been modified
// to remove the left and bottom edges, and right diagonal edges, so
// that the tiling of the terrain does not produce overlapping edges.

// Number of edges followed by edge index pairs
S32 sEdgeList45[16][11] = {
   { 0 },                  // 
   { 0 },
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0 },
   { 1, 0,1 },             // 0-2
   { 1, 0,1 },             // 1-2
   { 3, 0,1,1,2,2,0 },     // 0-1,1-2,2-0
   { 0 },
   { 0,},                  // 
   { 0 },
   { 1, 0,1 },             // 0-1,
   { 0, },                 // 
   { 1, 0,1 },             // 0-2,
   { 1, 0,1 },             // 1-2
   { 3, 0,1,1,2,0,2 },
};

S32 sEdgeList135[16][11] = {
   { 0 },
   { 0 },
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0 },
   { 0 },
   { 1, 0,1 },             // 1-2
   { 2, 0,1,1,2 },         // 0-1,1-2
   { 0 },
   { 0, },                 // 
   { 1, 0,1 },             // 1-3
   { 2, 0,1,1,2 },         // 0-1,1-3,
   { 0 },                  // 
   { 0 },                  // 
   { 2, 0,1,2,0 },         // 1-2,3-1
   { 3, 0,1,1,2,1,3 },
};

// On split squares, the FaceA diagnal is also removed
S32 sEdgeList45A[16][11] = {
   { 0 },                  // 
   { 0 },
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0 },
   { 0 },                  //
   { 1, 0,1 },             // 1-2
   { 2, 0,1,1,2 },         // 0-1,1-2
   { 0 },
   { 0,},                  // 
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0, },                 // 
   { 0, 0,1 },             //
   { 1, 0,1 },             // 1-2
   { 3, 0,1,1,2 },
};

S32 sEdgeList135A[16][11] = {
   { 0 },
   { 0 },
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0 },
   { 0 },
   { 1, 0,1 },             // 1-2
   { 2, 0,1,1,2 },         // 0-1,1-2
   { 0 },
   { 0 },                  // 
   { 0 },                  //
   { 1, 0,1 },             // 0-1
   { 0 },                  // 
   { 0 },                  // 
   { 1, 0,1 },             // 1-2
   { 3, 0,1,1,2 },
};

#else
// Number of edges followed by edge index pairs
S32 sEdgeList45[16][11] = {
   { 0 },                  // 
   { 0 },
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0 },
   { 1, 0,1 },             // 0-2
   { 1, 0,1 },             // 1-2
   { 3, 0,1,1,2,2,0 },     // 0-1,1-2,2-0
   { 0 },
   { 1, 0,1 },             // 0-3
   { 0 },
   { 2, 0,1,0,2 },         // 0-1,0-3
   { 1, 0,1 },             // 2-3
   { 3, 0,1,1,2,2,0 },     // 0-2,2-3,3-0
   { 2, 0,1,1,2 },         // 1-2,2-3
   { 5, 0,1,1,2,2,3,3,0,0,2 },
};

S32 sEdgeList135[16][11] = {
   { 0 },
   { 0 },
   { 0 },
   { 1, 0,1 },             // 0-1
   { 0 },
   { 0 },
   { 1, 0,1 },             // 1-2
   { 2, 0,1,1,2 },         // 0-1,1-2
   { 0 },
   { 1, 0,1 },             // 0-3
   { 1, 0,1 },             // 1-3
   { 3, 0,1,1,2,2,0 },     // 0-1,1-3,3-0
   { 1, 0,1 },             // 2-3
   { 2, 0,2,1,2 },         // 0-3,2-3
   { 3, 0,1,1,2,2,0 },     // 1-2,2-3,3-1
   { 5, 0,1,1,2,2,3,3,0,1,3 },
};
#endif

// Number of faces followed by normal index and vertices
S32 sFaceList45[16][9] = {
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 1, 0,0,1,2 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 1, 1,0,1,2 },
   { 0 },
   { 2, 0,0,1,2, 1,0,2,3 },
};

S32 sFaceList135[16][9] = {
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 0 },
   { 1, 0,0,1,2 },
   { 0 },
   { 0 },
   { 1, 1,0,1,2 },
   { 2, 0,0,1,3, 1,1,2,3 },
};


//----------------------------------------------------------------------------

Box3F TerrainConvex::getBoundingBox() const
{
   return box;
}

Box3F TerrainConvex::getBoundingBox(const MatrixF&, const Point3F& ) const
{
   // Function should not be called....
   return box;
}

Point3F TerrainConvex::support(const VectorF& v) const
{
   S32 *vp;
   if (halfA)
      vp = square ? sVertexList[(split45 << 1) | 1]: sVertexList[4];
   else
      vp = square ? sVertexList[(split45 << 1)]    : sVertexList[4];
      
   S32 *ve = vp + vp[0] + 1;
   const Point3F *bp = &point[vp[1]];
   F32 bd = mDot(*bp,v);
   for (vp += 2; vp < ve; vp++) {
      const Point3F* cp = &point[*vp];
      F32 dd = mDot(*cp,v);
      if (dd > bd) {
         bd = dd;
         bp = cp;
      }
   }
   return *bp;
}

inline bool isOnPlane(Point3F& p,PlaneF& plane)
{
   F32 dist = mDot(plane,p) + plane.d;
   return dist < 0.1 && dist > -0.1;
}

void TerrainConvex::getFeatures(const MatrixF& mat,const VectorF& n, ConvexFeature* cf)
{
   U32 i;
   cf->material = 0;
   cf->object = mObject;

   // Plane is normal n + support point
   PlaneF plane;
   plane.set(support(n),n);
   S32 vertexCount = cf->mVertexList.size();

   // Emit vertices on the plane
   S32* vertexListPointer;
   if (halfA)
      vertexListPointer = square ? sVertexList[(split45 << 1) | 1]: sVertexList[4];
   else
      vertexListPointer = square ? sVertexList[(split45 << 1)]    : sVertexList[4];
      
   S32 pm = 0;
   S32 numVerts = *vertexListPointer;
   vertexListPointer += 1;
   for (i = 0; i < numVerts; i++)
   {
      Point3F& cp = point[vertexListPointer[i]];
      cf->mVertexList.increment();
      mat.mulP(cp,&cf->mVertexList.last());
      pm |= 1 << vertexListPointer[i];
   }

   // Emit Edges
   S32* ep = (square && halfA)?
      (split45 ? sEdgeList45A[pm]: sEdgeList135A[pm]):
      (split45 ? sEdgeList45[pm]: sEdgeList135[pm]);

   S32 numEdges = *ep;
   S32 edgeListStart = cf->mEdgeList.size();
   cf->mEdgeList.increment(numEdges);
   ep += 1;
   for (i = 0; i < numEdges; i++)
   {
      cf->mEdgeList[edgeListStart + i].vertex[0] = vertexCount + ep[i * 2 + 0];
      cf->mEdgeList[edgeListStart + i].vertex[1] = vertexCount + ep[i * 2 + 1];
   }

   // Emit faces
   S32* fp = split45 ? sFaceList45[pm]: sFaceList135[pm];
   S32 numFaces = *fp;
   fp += 1;
   S32 faceListStart = cf->mFaceList.size();
   cf->mFaceList.increment(numFaces);
   for (i = 0; i < numFaces; i++)
   {
      cf->mFaceList[faceListStart + i].normal    = normal[fp[i * 4 + 0]];
      cf->mFaceList[faceListStart + i].vertex[0] = vertexCount + fp[i * 4 + 1];
      cf->mFaceList[faceListStart + i].vertex[1] = vertexCount + fp[i * 4 + 2];
      cf->mFaceList[faceListStart + i].vertex[2] = vertexCount + fp[i * 4 + 3];
   }
}


void TerrainConvex::getPolyList(AbstractPolyList* list)
{
   list->setTransform(&mObject->getTransform(), mObject->getScale());
   list->setObject(mObject);

   // Emit vertices
   U32 array[4];
   U32 curr = 0;

   S32 numVerts;
   S32* vertsStart;
   if (halfA)
   {
      numVerts   = square ?  sVertexList[(split45 << 1) | 1][0] :  sVertexList[4][0];
      vertsStart = square ? &sVertexList[(split45 << 1) | 1][1] : &sVertexList[4][1];
   }
   else
   {
      numVerts   = square ?  sVertexList[(split45 << 1)][0] :  sVertexList[4][0];
      vertsStart = square ? &sVertexList[(split45 << 1)][1] : &sVertexList[4][1];
   }

   S32 pointMask = 0;
   for (U32 i = 0; i < numVerts; i++) {
      const Point3F& cp = point[vertsStart[i]];
      array[curr++] = list->addPoint(cp);
      pointMask |= (1 << vertsStart[i]);
   }

   S32  numFaces  = split45 ?  sFaceList45[pointMask][0] :  sFaceList135[pointMask][0];
   S32* faceStart = split45 ? &sFaceList45[pointMask][1] : &sFaceList135[pointMask][1];
   for (U32 j = 0; j < numFaces; j++) {
      S32 plane = faceStart[0];
      S32 v0    = faceStart[1];
      S32 v1    = faceStart[2];
      S32 v2    = faceStart[3];

      list->begin(0, plane);
      list->vertex(array[v0]);
      list->vertex(array[v1]);
      list->vertex(array[v2]);
      list->plane(array[v0], array[v1], array[v2]);
      list->end();

      faceStart += 4;
   }
}


//----------------------------------------------------------------------------

void TerrainBlock::buildConvex(const Box3F& box,Convex* convex)
{
   sTerrainConvexList.collectGarbage();

   //
   if (box.max.z < -TerrainThickness || box.min.z > fixedToFloat(gridMap[BlockShift]->maxHeight))
      return;

   // Transform the bounding sphere into the object's coord space.  Note that this
   // not really optimal.
   Box3F osBox = box;
   mWorldToObj.mul(osBox);
   AssertWarn(mObjScale == Point3F(1, 1, 1), "Error, handle the scale transform on the terrain");

   S32 xStart = (S32)mFloor( osBox.min.x / squareSize );
   S32 xEnd   = (S32)mCeil ( osBox.max.x / squareSize );
   S32 yStart = (S32)mFloor( osBox.min.y / squareSize );
   S32 yEnd   = (S32)mCeil ( osBox.max.y / squareSize );
   S32 xExt = xEnd - xStart;
   if (xExt > MaxExtent)
      xExt = MaxExtent;

   mHeightMax = floatToFixed(osBox.max.z);
   mHeightMin = (osBox.min.z < 0)? 0: floatToFixed(osBox.min.z);

   for (S32 y = yStart; y < yEnd; y++) {
      S32 yi = y & BlockMask;

      // 
      for (S32 x = xStart; x < xEnd; x++) {
         S32 xi = x & BlockMask;
         GridSquare *gs = findSquare(0, Point2I(xi, yi));

         // holes only in the primary terrain block
         if (((gs->flags & GridSquare::Empty) && x == xi && y == yi) ||
             gs->minHeight > mHeightMax || gs->maxHeight < mHeightMin)
            continue;

         U32 sid = (x << 16) + (y & ((1 << 16) - 1));
         Convex* cc = 0;

         // See if the square already exists as part of the working set.
         CollisionWorkingList& wl = convex->getWorkingList();
         for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext)
            if (itr->mConvex->getType() == TerrainConvexType &&
                static_cast<TerrainConvex*>(itr->mConvex)->squareId == sid) {
               cc = itr->mConvex;
               break;
            }
         if (cc)
            continue;

         // Create a new convex.
         TerrainConvex* cp = new TerrainConvex;
         sTerrainConvexList.registerObject(cp);
         convex->addToWorkingList(cp);
         cp->halfA = true;
         cp->square = 0;
         cp->mObject = this;
         cp->squareId = sid;
         cp->material = getMaterial(xi,yi)->index;
         cp->box.min.set(x * squareSize,y * squareSize,fixedToFloat(gs->minHeight));
         cp->box.max.x = cp->box.min.x + squareSize;
         cp->box.max.y = cp->box.min.y + squareSize;
         cp->box.max.z = fixedToFloat(gs->maxHeight);
         mObjToWorld.mul(cp->box);

         // Build points
         Point3F* pos = cp->point;
         for (int i = 0; i < 4 ; i++,pos++) {
            S32 dx = i >> 1;
            S32 dy = dx ^ (i & 1);
            pos->x = (x + dx) * squareSize;
            pos->y = (y + dy) * squareSize;
            pos->z = fixedToFloat(getHeight(xi + dx, yi + dy));
         }

         // Build normals, then split into two Convex objects if the
         // square is concave
         if ((cp->split45 = gs->flags & GridSquare::Split45) == true) {
            VectorF *vp = cp->point;
            mCross(vp[0] - vp[1],vp[2] - vp[1],&cp->normal[0]);
            cp->normal[0].normalize();
            mCross(vp[2] - vp[3],vp[0] - vp[3],&cp->normal[1]);
            cp->normal[1].normalize();
            if (mDot(vp[3] - vp[1],cp->normal[0]) > 0) {
               TerrainConvex* nc = new TerrainConvex(*cp);
               sTerrainConvexList.registerObject(nc);
               convex->addToWorkingList(nc);
               nc->halfA = false;
               nc->square = cp;
               cp->square = nc;
            }
         }
         else {
            VectorF *vp = cp->point;
            mCross(vp[3] - vp[0],vp[1] - vp[0],&cp->normal[0]);
            cp->normal[0].normalize();
            mCross(vp[1] - vp[2],vp[3] - vp[2],&cp->normal[1]);
            cp->normal[1].normalize();
            if (mDot(vp[2] - vp[0],cp->normal[0]) > 0) {
               TerrainConvex* nc = new TerrainConvex(*cp);
               sTerrainConvexList.registerObject(nc);
               convex->addToWorkingList(nc);
               nc->halfA = false;
               nc->square = cp;
               cp->square = nc;
            }
         }
      }
   }
}


//----------------------------------------------------------------------------

static inline void swap(U32*& a,U32*& b)
{
   U32* t = b;
   b = a;
   a = t;
}

static void clrbuf(U32* p, U32 s)
{
   U32* e = p + s;
   while (p != e)
      *p++ = U32_MAX;
}


//----------------------------------------------------------------------------

MaterialProperty* TerrainBlock::getMaterialProperty(U32)
{
    return nullptr;
}

bool TerrainBlock::buildPolyList(AbstractPolyList* polyList, const Box3F &box, const SphereF&)
{
   if (box.max.z < -TerrainThickness || box.min.z > fixedToFloat(gridMap[BlockShift]->maxHeight))
      return false;

   // Transform the bounding sphere into the object's coord space.  Note that this
   //  not really optimal.
   Box3F osBox = box;
   mWorldToObj.mul(osBox);
   AssertWarn(mObjScale == Point3F(1, 1, 1), "Error, handle the scale transform on the terrain");

   // Setup collision state data
   polyList->setTransform(&getTransform(), getScale());
   polyList->setObject(this);

   S32 xStart = (S32)mFloor( osBox.min.x / squareSize );
   S32 xEnd   = (S32)mCeil ( osBox.max.x / squareSize );
   S32 yStart = (S32)mFloor( osBox.min.y / squareSize );
   S32 yEnd   = (S32)mCeil ( osBox.max.y / squareSize );
   S32 xExt = xEnd - xStart;
   if (xExt > MaxExtent)
      xExt = MaxExtent;

   mHeightMax = floatToFixed(osBox.max.z);
   mHeightMin = (osBox.min.z < 0)? 0: floatToFixed(osBox.min.z);

   // Index of shared points
   U32 bp[(MaxExtent + 1) * 2],*vb[2];
   vb[0] = &bp[0];
   vb[1] = &bp[xExt + 1];
   clrbuf(vb[1],xExt + 1);

   bool emitted = false;
   for (S32 y = yStart; y < yEnd; y++) {
      S32 yi = y & BlockMask;

      swap(vb[0],vb[1]);
      clrbuf(vb[1],xExt + 1);
      // 
      for (S32 x = xStart; x < xEnd; x++) {
         S32 xi = x & BlockMask;
         GridSquare *gs = findSquare(0, Point2I(xi, yi));

         // holes only in the primary terrain block
         if (((gs->flags & GridSquare::Empty) && x == xi && y == yi) ||
             gs->minHeight > mHeightMax || gs->maxHeight < mHeightMin)
            continue;
         emitted = true;

         // Add the missing points
         U32 vi[5];
         for (int i = 0; i < 4 ; i++) {
            S32 dx = i >> 1;
            S32 dy = dx ^ (i & 1);
            U32* vp = &vb[dy][x - xStart + dx];
            if (*vp == U32_MAX) {
               Point3F pos;
               pos.x = (x + dx) * squareSize;
               pos.y = (y + dy) * squareSize;
               pos.z = fixedToFloat(getHeight(xi + dx, yi + dy));
               *vp = polyList->addPoint(pos);
            }
            vi[i] = *vp;
         }

         U32* vp = &vi[0];
         if (!(gs->flags & GridSquare::Split45))
            vi[4] = vi[0], vp++;

         U32 material = getMaterial(xi,yi)->index;
         U32 surfaceKey = (xi << 16 + yi) << 1;
         polyList->begin(material,surfaceKey);
         polyList->vertex(vp[0]);
         polyList->vertex(vp[1]);
         polyList->vertex(vp[2]);
         polyList->plane(vp[0],vp[1],vp[2]);
         polyList->end();
         polyList->begin(material,surfaceKey + 1);
         polyList->vertex(vp[0]);
         polyList->vertex(vp[2]);
         polyList->vertex(vp[3]);
         polyList->plane(vp[0],vp[2],vp[3]);
         polyList->end();
      }
   }
   return emitted;
}


//----------------------------------------------------------------------------

BSPNode *TerrainBlock::buildCollisionBSP(BSPTree *tree, const Box3F &box, const SphereF& /*sphere*/)
{
   S32 xStart = (S32)mFloor( box.min.x / squareSize );
   S32 xEnd   = (S32)mCeil ( box.max.x / squareSize );
   S32 yStart = (S32)mFloor( box.min.y / squareSize );
   S32 yEnd   = (S32)mCeil ( box.max.y / squareSize );

   mTree = tree;
   
   S32 yExt = yEnd - yStart;
   S32 xExt = xEnd - xStart;
   
   S32 maxPlaneCount = yExt - 1 + (xExt - 1) * yExt + xExt * yExt * 4;

   if (box.max.z < -TerrainThickness || box.min.z > fixedToFloat(gridMap[BlockShift]->maxHeight))
      return 0;
   mHeightMax = floatToFixed(box.max.z);
   mHeightMin = (box.min.z < 0)? 0: floatToFixed(box.min.z);

   BSPNode *backTree = buildXTree(yStart, xStart, xEnd);
      
   for(S32 y = yStart + 1; y < yEnd; y++)
   {
      BSPNode *branch = mTree->alloc();
      branch->plane.setXZ(y * squareSize);

      branch->frontNode = buildXTree(y, xStart, xEnd);
      branch->backNode = backTree;
      
      if(branch->frontNode == branch->backNode)
      {
         AssertFatal(!branch->frontNode, "TerrainBlock::Internal Error: Bad equal nodes.");
         backTree = branch->frontNode;
      }
      else
         backTree = branch;
   }
   return backTree;
}


BSPNode *TerrainBlock::buildXTree(S32 y, S32 xStart, S32 xEnd)
{
   BSPNode *backTree = buildSquareTree(y, xStart);
   for(S32 x = xStart + 1; x < xEnd; x++)
   {
      BSPNode *branch = mTree->alloc();
      branch->plane.setYZ(x * squareSize);
      branch->frontNode = buildSquareTree(y, x);
      branch->backNode = backTree;
      
      if(branch->frontNode == branch->backNode)
      {
         AssertFatal(!branch->frontNode, "TerrainBlock::Internal Error: Bad equal nodes.");
         backTree = branch->frontNode;
      }
      else
         backTree = branch;
   }
   return backTree;
}


BSPNode *TerrainBlock::buildSquareTree(S32 y, S32 x)
{
   // coords are in, mask off for heightMap lookup:
   S32 xi = x & BlockMask;
   S32 yi = y & BlockMask;

   GridSquare *gs = findSquare(0, Point2I(xi, yi));
   
   // trivial check on min/max heights:
   // we compare > on height max so we can add thickness to the terrain.

   // holes only in the primary terrain block   
   if (((gs->flags & GridSquare::Empty) && x == xi && y == yi) ||
       gs->minHeight > mHeightMax || gs->maxHeight < mHeightMin)
      return 0;

   F32 zBottomLeft = fixedToFloat(getHeight(xi, yi));
   F32 zBottomRight = fixedToFloat(getHeight(xi + 1, yi));
   F32 zTopLeft = fixedToFloat(getHeight(xi, yi + 1));
   F32 zTopRight = fixedToFloat(getHeight(xi + 1, yi + 1));

   // trivial check failed - construct planes.
   
   Point3F normal1, normal2, normal3;
   Point3F squarePos, testPoint;
   
   // test point is point on p2 that is not on the line from p1 to p2.
   
   if(gs->flags & GridSquare::Split45)
   {
      squarePos.set(x * squareSize, y * squareSize, zBottomLeft);
      normal1.set(zBottomLeft - zBottomRight, zBottomRight - zTopRight, squareSize);
      normal2.set(zTopLeft - zTopRight, zBottomLeft - zTopLeft, squareSize);
      normal3.set(-squareSize,squareSize,0);
      testPoint.set(x * squareSize, (y+1) * squareSize, zTopLeft);
   }
   else
   {
      squarePos.set((x + 1) * squareSize, y * squareSize, zBottomRight);
      normal1.set(zTopLeft - zTopRight, zBottomRight - zTopRight, squareSize);
      normal2.set(zBottomLeft - zBottomRight, zBottomLeft - zTopLeft, squareSize);
      normal3.set(-squareSize,-squareSize,0);
      testPoint.set(x * squareSize, y * squareSize, zBottomLeft);
   }

   // Solid terrain node with material type
   BSPNode* solidNode = mTree->alloc();
   solidNode->material = getMaterial(xi,yi)->index;
   solidNode->frontNode = solidNode->backNode = 0;

   // Make plane 1 node:
   BSPNode *p1Node = mTree->alloc();
   p1Node->plane.set(squarePos, normal1);
   p1Node->frontNode = 0;
   p1Node->backNode = solidNode;
   
   F32 dist = p1Node->plane.distToPlane(testPoint);
   if(dist != 0)
   {
      // Square is not flat, we need to build more nodes
      // Make plane 2 node:
      BSPNode *p2Node = mTree->alloc();
      p2Node->plane.set(squarePos, normal2);
      p2Node->frontNode = 0;
      p2Node->backNode = solidNode;

      // Make vertical split node:
      BSPNode *psNode = mTree->alloc();
      psNode->plane.set(squarePos, normal3);
      psNode->frontNode = p2Node;
      psNode->backNode = p1Node;

      p1Node = psNode;
   }

   // Add in a bottom plane if we extend past the bottom of the terrain.
   if(gs->minHeight >= mHeightMin - TerrainThickness)
   {
      BSPNode *belowNode = mTree->alloc();
      belowNode->plane.setXY(fixedToFloat(gs->minHeight) - TerrainThickness);
      belowNode->plane.invert();
      belowNode->frontNode = 0;
      belowNode->backNode = p1Node;
      return belowNode;
   }
   return p1Node;
}


//----------------------------------------------------------------------------

static F32 calcInterceptV(F32 vStart, F32 invDeltaV, F32 intercept)
{
   return (intercept - vStart) * invDeltaV;
}

static F32 calcInterceptNone(F32, F32, F32)
{
   return MAX_FLOAT;
}

static F32 (*calcInterceptX)(F32, F32, F32);
static F32 (*calcInterceptY)(F32, F32, F32);

static U32 lineCount;
static Point3F lineStart, lineEnd;

static void drawLineTest(F32 startT, F32 endT, bool collide)
{
   glEnable(GL_DEPTH_TEST);
   if(lineCount & 1)
   {
      if(collide)
         glColor3f(1, 0, 0);
      else   
         glColor3f(1, 1, 1);
   }
   else
   {
      if(collide)
         glColor3f(0, 1, 0);
      else   
         glColor3f(0, 0, 0);
   }
   lineCount++;
   glBegin(GL_LINES);
   Point3F pt;
   pt.interpolate(lineStart, lineEnd, startT);
   glVertex3fv(pt);
   pt.interpolate(lineStart, lineEnd, endT);
   glVertex3fv(pt);
   Point3F ptUp = pt;
   ptUp.z += 4;
   glVertex3fv(pt);
   glVertex3fv(ptUp);
   
   glEnd();
}

bool TerrainBlock::castRay(const Point3F &start, const Point3F &end, RayInfo *info)
{
   return castRayI(start, end, info, false);
}

bool TerrainBlock::castRayI(const Point3F &start, const Point3F &end, RayInfo *info, bool collideEmpty)
{
   lineCount = 0;
   lineStart = start;
   lineEnd = end;

   info->object = this;
      
   if(start.x == end.x && start.y == end.y)
   {
      if (end.z == start.z)
         return false;

      F32 height;
      if(!getNormalAndHeight(Point2F(start.x, start.y), &info->normal, &height, true))
         return false;
      F32 t = (height - start.z) / (end.z - start.z);
      if(t < 0 || t > 1)
         return false;
      info->t = t;

      //
      return true;
   }

   F32 invBlockWorldSize = 1 / F32(squareSize * BlockSquareWidth);

   Point3F pStart(start.x * invBlockWorldSize, start.y * invBlockWorldSize, start.z);
   Point3F pEnd(end.x * invBlockWorldSize, end.y * invBlockWorldSize, end.z);
   
   int blockX = (S32)mFloor(pStart.x);
   int blockY = (S32)mFloor(pStart.y);
   
   int dx, dy;

   F32 invDeltaX;
   if(pEnd.x == pStart.x)
   {
      calcInterceptX = calcInterceptNone;
      invDeltaX = 0;
      dx = 0;
   }
   else
   {
      invDeltaX = 1 / (pEnd.x - pStart.x);
      calcInterceptX = calcInterceptV;
      if(pEnd.x < pStart.x)
         dx = -1;
      else
         dx = 1;
   }
      
   F32 invDeltaY;
   if(pEnd.y == pStart.y)
   {
      calcInterceptY = calcInterceptNone;
      invDeltaY = 0;
      dy = 0;
   }
   else
   {
      invDeltaY = 1 / (pEnd.y - pStart.y);
      calcInterceptY = calcInterceptV;
      if(pEnd.y < pStart.y)
         dy = -1;
      else
         dy = 1;
   }
   F32 startT = 0;
   for(;;)
   {
      F32 nextXInt = calcInterceptX(pStart.x, invDeltaX, blockX + (dx == 1));
      F32 nextYInt = calcInterceptY(pStart.y, invDeltaY, blockY + (dy == 1));
      
      F32 intersectT = 1;
      
      if(nextXInt < intersectT)
         intersectT = nextXInt;
      if(nextYInt < intersectT)
         intersectT = nextYInt;
      
      if(castRayBlock(pStart, pEnd, Point2I(blockX * BlockSquareWidth, blockY * BlockSquareWidth), BlockShift, invDeltaX, invDeltaY, startT, intersectT, info, collideEmpty)) {
         info->normal.z *= BlockSquareWidth * squareSize;
         info->normal.normalize();
         return true;
      }

      startT = intersectT;
      if(intersectT >= 1)
         break;
      if(nextXInt < nextYInt)
         blockX += dx;
      else if(nextYInt < nextXInt)
         blockY += dy;
      else
      {
         blockX += dx;
         blockY += dy;
      }
   }
   return false;
}

struct TerrLOSStackNode
{
   F32 startT;
   F32 endT;
   Point2I blockPos;
   U32 level;
};

bool TerrainBlock::castRayBlock(const Point3F &pStart, const Point3F &pEnd, Point2I aBlockPos, U32 aLevel, F32 invDeltaX, F32 invDeltaY, F32 aStartT, F32 aEndT, RayInfo *info, bool collideEmpty)
{
   F32 invBlockSize = 1 / F32(BlockSquareWidth);

   static TerrLOSStackNode stack[BlockShift * 3 + 1];
   U32 stackSize = 1;
   
   stack[0].startT = aStartT;
   stack[0].endT = aEndT;
   stack[0].blockPos = aBlockPos;
   stack[0].level = aLevel;
   
   while(stackSize--)
   {
      TerrLOSStackNode *sn = stack + stackSize;
      U32 level  = sn->level;
      F32 startT = sn->startT;
      F32 endT   = sn->endT;
      Point2I blockPos = sn->blockPos;
      
      GridSquare *sq = findSquare(level, Point2I(blockPos.x & BlockMask, blockPos.y & BlockMask));

      F32 startZ = startT * (pEnd.z - pStart.z) + pStart.z;
      F32 endZ = endT * (pEnd.z - pStart.z) + pStart.z;
   
      F32 minHeight = fixedToFloat(sq->minHeight);
      if(startZ <= minHeight && endZ <= minHeight)
      {
         //drawLineTest(startT, sn->endT, false);
         continue;
      }
      F32 maxHeight = fixedToFloat(sq->maxHeight);
      if(startZ >= maxHeight && endZ >= maxHeight)
      {
         //drawLineTest(startT, endT, false);
         continue;
      }
      if (!collideEmpty && (sq->flags & GridSquare::Empty) &&
      	  blockPos.x == (blockPos.x & BlockMask) && blockPos.y == (blockPos.y & BlockMask))
      {
         //drawLineTest(startT, endT, false);
         continue;
      }
      if(level == 0)
      {
         F32 xs = blockPos.x * invBlockSize;
         F32 ys = blockPos.y * invBlockSize;
      
         F32 zBottomLeft = fixedToFloat(getHeight(blockPos.x, blockPos.y));
         F32 zBottomRight= fixedToFloat(getHeight(blockPos.x + 1, blockPos.y));
         F32 zTopLeft =    fixedToFloat(getHeight(blockPos.x, blockPos.y + 1));
         F32 zTopRight =   fixedToFloat(getHeight(blockPos.x + 1, blockPos.y + 1));
      
         PlaneF p1, p2;
         PlaneF divider;
         Point3F planePoint;
      
         if(sq->flags & GridSquare::Split45)
         {
            p1.set(zBottomLeft - zBottomRight, zBottomRight - zTopRight, invBlockSize);
            p2.set(zTopLeft - zTopRight, zBottomLeft - zTopLeft, invBlockSize);
            planePoint.set(xs, ys, zBottomLeft);
            divider.x = 1;
            divider.y = -1;
            divider.z = 0;
         }
         else
         {
            p1.set(zTopLeft - zTopRight, zBottomRight - zTopRight, invBlockSize);
            p2.set(zBottomLeft - zBottomRight, zBottomLeft - zTopLeft, invBlockSize);
            planePoint.set(xs + invBlockSize, ys, zBottomRight);
            divider.x = 1;
            divider.y = 1;
            divider.z = 0;
         }
         p1.setPoint(planePoint);
         p2.setPoint(planePoint);
         divider.setPoint(planePoint);
      
         F32 t1 = p1.intersect(pStart, pEnd);
         F32 t2 = p2.intersect(pStart, pEnd);
         F32 td = divider.intersect(pStart, pEnd);
         
         F32 dStart = divider.distToPlane(pStart);
         F32 dEnd = divider.distToPlane(pEnd);
         
         // see if the line crosses the divider
         if((dStart >= 0 && dEnd < 0) || (dStart < 0 && dEnd >= 0))
         {
            if(dStart < 0)
            {
               F32 temp = t1;
               t1 = t2;
               t2 = temp;
            }
            if(t1 >= startT && t1 && t1 <= td && t1 <= endT)
            {
               info->t = t1;
               info->normal = p1;
               return true;
            }
            if(t2 >= td && t2 >= startT && t2 <= endT)
            {
               info->t = t2;
               info->normal = p2;
               return true;
            }
         }
         else
         {
            F32 t;
            if(dStart >= 0) {
               t = t1;
               info->normal = p1;
            }
            else {
               t = t2;
               info->normal = p2;
            }
            if(t >= startT && t <= endT)
            {
               info->t = t;
               return true;
            }
         }
         continue;
      }
      int squareWidth = 1 << level;
      int subSqWidth = 1 << (level - 1);
      F32 xIntercept = (blockPos.x + subSqWidth) * invBlockSize;
      F32 xInt = calcInterceptX(pStart.x, invDeltaX, xIntercept);
      F32 yIntercept = (blockPos.y + subSqWidth) * invBlockSize;
      F32 yInt = calcInterceptY(pStart.y, invDeltaY, yIntercept);
   
      F32 startX = startT * (pEnd.x - pStart.x) + pStart.x;
      F32 startY = startT * (pEnd.y - pStart.y) + pStart.y;
   
      if(xInt < startT)
         xInt = MAX_FLOAT;
      if(yInt < startT)
         yInt = MAX_FLOAT;

      U32 x0 = (startX > xIntercept) * subSqWidth;
      U32 y0 = (startY > yIntercept) * subSqWidth;
      U32 x1 = subSqWidth - x0;
      U32 y1 = subSqWidth - y0;
      U32 nextLevel = level - 1;

      // push the items on the stack in reverse order of processing
      if(xInt > endT && yInt > endT)
      {
         // only test the square the point started in:
         stack[stackSize].blockPos.set(blockPos.x + x0, blockPos.y + y0);
         stack[stackSize].level = nextLevel;
         stackSize++;
      }
      else if(xInt < yInt)
      {
         F32 nextIntersect = endT;
         if(yInt <= endT)
         {
            stack[stackSize].blockPos.set(blockPos.x + x1, blockPos.y + y1);
            stack[stackSize].startT = yInt;
            stack[stackSize].endT = endT;
            stack[stackSize].level = nextLevel;
            nextIntersect = yInt;
            stackSize++;
         }
         stack[stackSize].blockPos.set(blockPos.x + x1, blockPos.y + y0);
         stack[stackSize].startT = xInt;
         stack[stackSize].endT = nextIntersect;
         stack[stackSize].level = nextLevel;

         stack[stackSize+1].blockPos.set(blockPos.x + x0, blockPos.y + y0);
         stack[stackSize+1].startT = startT;
         stack[stackSize+1].endT = xInt;
         stack[stackSize+1].level = nextLevel;
         stackSize += 2;
      }
      else if(yInt < xInt)
      {
         F32 nextIntersect = endT;
         if(xInt <= endT)
         {
            stack[stackSize].blockPos.set(blockPos.x + x1, blockPos.y + y1);
            stack[stackSize].startT = xInt;
            stack[stackSize].endT = endT;
            stack[stackSize].level = nextLevel;
            nextIntersect = xInt;
            stackSize++;
         }
         stack[stackSize].blockPos.set(blockPos.x + x0, blockPos.y + y1);
         stack[stackSize].startT = yInt;
         stack[stackSize].endT = nextIntersect;
         stack[stackSize].level = nextLevel;

         stack[stackSize+1].blockPos.set(blockPos.x + x0, blockPos.y + y0);
         stack[stackSize+1].startT = startT;
         stack[stackSize+1].endT = yInt;
         stack[stackSize+1].level = nextLevel;
         stackSize += 2;
      }
      else
      {
         stack[stackSize].blockPos.set(blockPos.x + x1, blockPos.y + y1);
         stack[stackSize].startT = xInt;
         stack[stackSize].endT = endT;
         stack[stackSize].level = nextLevel;

         stack[stackSize+1].blockPos.set(blockPos.x + x0, blockPos.y + y0);
         stack[stackSize+1].startT = startT;
         stack[stackSize+1].endT = xInt;
         stack[stackSize+1].level = nextLevel;
         stackSize += 2;
      }
   }
   return false;
}
