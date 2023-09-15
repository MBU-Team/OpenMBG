//-----------------------------/------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPMESH_H_
#define DTSAPPMESH_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "appSequence.h"

namespace DTS
{
   class AppIfl;
   class AppMeshLock;
   class AppNode;

   class AppMesh
   {
      protected:

      std::vector<Primitive>   mFaces;
      std::vector<Point3D>     mVerts;
      std::vector<Point2D>     mTVerts;
      std::vector<U16>         mIndices;
      std::vector<U32>         mVertId;

      // One can supply either the normals directly or
      // 3dsMax style smoothing groups and normals will
      // be computed for you.  Note that one or the other
      // should be supplied -- if both or neither are
      // supplied the behavior is not predictable.
      // In 3dsMax, normals are not trustworthy, so it is
      // necessary to use smoothing groups instead.
      std::vector<Point3D>     mNormals;
      std::vector<U32>         mSmooth;

      // For skinned meshes, we want bones and weights
      // Weights stored by (*mWeights[vertIdx])[boneIdx]
      std::vector<AppNode*>    mBones;
      std::vector<std::vector<F32>*> mWeights;

      // For ifls
      std::vector<AppIfl*>     mIfls;

      // These needs to be maintained by lock/unlock methods
      bool mLocked;
      bool mSkinDataFetched;
 
      void sortFaceList();
      void lookupSkinData();
      virtual void getSkinData() = 0;

      public:

      AppMesh();
      ~AppMesh();

      // Not necessary to use these (don't need to keep verts unique) but can be used for
      // keeping vert list smaller.  Add each vert using one or  the other method.
      U16 addVertex(const Point3D & vert, const Point3D & norm, Point2D & tvert, U32 vertId);
      U16 addVertex(const Point3D & vert, Point2D & tvert, U32 vertId, U32 smooth);

      virtual const char * getName() = 0;

      virtual Matrix<4,4,F32> getMeshTransform(const AppTime & time) = 0;
      virtual F32 getVisValue(const AppTime & time) = 0;

      virtual bool getFloat(const char * propName, F32  & defaultVal) = 0;
      virtual bool   getInt(const char * propName, S32  & defaultVal) = 0;
      virtual bool  getBool(const char * propName, bool & defaultVal) = 0;

      virtual bool getMaterial(S32 matIdx, Material &) = 0;
      AppIfl * getIfl(S32 matIdx);

      S32 getNumBones();
      AppNode * getBone(S32 idx);
      F32 getWeight(S32 boneIdx, S32 vertIdx);

      virtual bool animatesVis(const AppSequenceData & seqData);
      virtual bool animatesMatFrame(const AppSequenceData & seqData);
      virtual bool animatesFrame(const AppSequenceData & seqData);

      virtual Box getBounds(const Matrix<4,4,F32> & objectOffset);
      virtual F32 getRadius(const Matrix<4,4,F32> & objectOffset);
      virtual F32 getTubeRadius(const Matrix<4,4,F32> & objectOffset);
      virtual bool isBillboard();
      virtual bool isBillboardZAxis();
      virtual bool isSorted();
      virtual bool isDummy();
      bool isSkin() { return getNumBones() != 0; }

      virtual AppMeshLock lockMesh(const AppTime & time, const Matrix<4,4,F32> & objectOffset);
      virtual void unlockMesh();

      // before and after accessing following methods,
      // one should always call lock/unlock mesh
      S32 getNumFaces();
      const Primitive * getFaces();
      S32 getFaceMaterial(S32 faceIdx);
      S32 getNumIndices();
      const U16 * getIndices();
      S32 getNumTVerts();
      const Point2D * getTVerts();
      S32 getNumVerts();
      const Point3D * getVerts();
      S32 getNumNormals();
      const Point3D * getNormals();
      void generateFaces(std::vector<Primitive> & faces,
                         std::vector<Point3D>   & verts,
                         std::vector<Point2D>   & tverts,
                         std::vector<U16>       & indices,
                         std::vector<U32>       & smooth,
                         std::vector<Point3D>   & normals,
                         std::vector<U32>       * vertId);

   };

   class AppMeshLock : public OnDestroy<AppMesh>
   {
      protected:
         void doit();
      public:
         AppMeshLock(AppMesh * mesh) : OnDestroy<AppMesh>(mesh) {}
         ~AppMeshLock() { transfer(NULL); }
   };

   inline S32 AppMesh::getNumFaces()
   {
      assert(mLocked && "Mesh isn't locked");
      return mFaces.size();
   }

   inline const Primitive * AppMesh::getFaces()
   {
      assert(mLocked && "Mesh isn't locked");
      return getNumFaces() ? &mFaces[0] : NULL;
   }

   inline S32 AppMesh::getFaceMaterial(S32 faceIdx)
   {
      assert(mLocked && "Mesh isn't locked");
      assert(faceIdx>=0 && faceIdx<mFaces.size() && "Face index out of range");
      if (mFaces[faceIdx].type & Primitive::NoMaterial)
         return -1;
      return mFaces[faceIdx].type & Primitive::MaterialMask;
   }

   inline S32 AppMesh::getNumIndices()
   {
      assert(mLocked && "Mesh isn't locked");
      return mIndices.size();
   }

   inline const U16 * AppMesh::getIndices()
   {
      assert(mLocked && "Mesh isn't locked");
      return getNumIndices() ? &mIndices[0] : NULL;
   }

   inline S32 AppMesh::getNumTVerts()
   {
      assert(mLocked && "Mesh isn't locked");
      return mTVerts.size();
   }

   inline const Point2D * AppMesh::getTVerts()
   {
      assert(mLocked && "Mesh isn't locked");
      return getNumTVerts() ? &mTVerts[0] : NULL;
   }

   inline S32 AppMesh::getNumVerts()
   {
      assert(mLocked && "Mesh isn't locked");
      return mVerts.size();
   }

   inline const Point3D * AppMesh::getVerts()
   {
      assert(mLocked && "Mesh isn't locked");
      return getNumVerts() ? &mVerts[0] : NULL;
   }

   inline S32 AppMesh::getNumNormals()
   {
      assert(mLocked && "Mesh isn't locked");
      return mNormals.size();
   }

   inline const Point3D * AppMesh::getNormals()
   {
      assert(mLocked && "Mesh isn't locked");
      return getNumNormals() ? &mNormals[0] : NULL;
   }

   inline S32 AppMesh::getNumBones()
   {
      lookupSkinData();
      return mBones.size();
   }

   inline AppNode * AppMesh::getBone(S32 idx)
   {
      lookupSkinData();
      assert(idx>=0 && idx<mBones.size() && "Bone index out of range");
      return mBones[idx];
   }

   inline F32 AppMesh::getWeight(S32 boneIdx, S32 vertIdx)
   {
      assert(mLocked && "Mesh isn't locked");
      lookupSkinData();
      assert(boneIdx>=0 && boneIdx<mBones.size() && "Bone index out of range");
      assert(vertIdx>=0 && vertIdx<mVertId.size() && "Vertex index out of range");
      assert(mVertId[vertIdx]<mWeights[boneIdx]->size() && "Vertex id out of range");
      return (*mWeights[boneIdx])[mVertId[vertIdx]];
   }

   inline AppIfl * AppMesh::getIfl(S32 matIdx)
   {
      assert(matIdx>=0 && matIdx<mIfls.size() && "Mat index out of ifl range");
      return mIfls[matIdx];
   }

   inline void AppMesh::lookupSkinData()
   {
      if (mSkinDataFetched)
         return;
      getSkinData();
      mSkinDataFetched = true;
   }

};


#endif // DTSAPPMESH_H_
