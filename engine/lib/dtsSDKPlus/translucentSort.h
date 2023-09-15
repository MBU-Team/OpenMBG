//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _TRANS_SORT
#define _TRANS_SORT

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "DTSUtil.h"

namespace DTS
{

   typedef std::vector<bool> IntegerSet;

   class TranslucentSort
   {
      std::vector<IntegerSet*> frontClusters;
      std::vector<IntegerSet*> backClusters;
      std::vector<S32> middleCluster;
   
      Point3D splitNormal;
      F32 splitK;

      S32 mNumBigFaces;
      S32 mMaxDepth;
      bool mZLayerUp;
      bool mZLayerDown;

      S32 currentDepth;
      
      TranslucentSort * frontSort;
      TranslucentSort * backSort;

      struct FaceInfo
      {
         bool used;
         S32 priority;
         S32 parentFace;
         S32 childFace1;
         S32 childFace2;
         S32 childFace3;
         Point3D normal;
         F32 k;
         IntegerSet isInFrontOfMe;
         IntegerSet isBehindMe;
         IntegerSet isCutByMe;
         IntegerSet isCoplanarWithMe;
      };
      std::vector<FaceInfo*> faceInfoList;
      std::vector<FaceInfo*> saveFaceInfoList;

      std::vector<Primitive> & mFaces;
      std::vector<U16> & mIndices;
      std::vector<Point3D> & mVerts;
      std::vector<Point3D> & mNorms;
      std::vector<Point2D> & mTVerts;

      void initFaces();
      void initFaceInfo(Primitive & face, FaceInfo & faceInfo, bool setPriority = true);
      void setFaceInfo(Primitive & face, FaceInfo & faceInfo);
      void clearFaces(IntegerSet &);
      void saveFaceInfo();
      void restoreFaceInfo();
      void addFaces(IntegerSet *, std::vector<Primitive> & faces, std::vector<U16> & indices, bool continueLast = false);
      void addFaces(std::vector<IntegerSet *> &, std::vector<Primitive> & faces, std::vector<U16> & indices, bool continueLast = false);
      void addOrderedFaces(std::vector<S32> &, std::vector<Primitive> &, std::vector<U16> & indices, bool continueLast = false);
      void splitFace(S32 faceIndex, Point3D normal, F32 k);
      void splitFace2(S32 faceIndex, Point3D normal, F32 k);
      void sort();
   
      // routines for sorting faces when there is no perfect solution for all cases
      void copeSort(std::vector<S32> &);
      void layerSort(std::vector<S32> &, bool upFirst);

      // these are for debugging
      bool anyInFrontOfPlane(Point3D normal, F32 k);
      bool anyBehindPlane(Point3D normal, F32 k);

      //
      void generateClusters(std::vector<Cluster> & clusters, std::vector<Primitive> & faces, std::vector<U16> & indices, S32 retIndex = -1);

      TranslucentSort(TranslucentSort *);
      TranslucentSort(std::vector<Primitive> & faces,
                      std::vector<U16> & indices,
                      std::vector<Point3D> & verts,
                      std::vector<Point3D> & norms,
                      std::vector<Point2D> & tverts,
                      S32 numBigFaces, S32 maxDepth, bool zLayerUp, bool zLayerDown);

      ~TranslucentSort();

   public:

      static void generateSortedMesh(Mesh * mesh, S32 numBigFaces, S32 maxDepth, bool zLayerUp, bool zLayerDown);
   };
};


#endif // _TRANS_SORT

