//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "appMesh.h"
#include "appTime.h"
#include "dtsUtil.h"
#include "appIfl.h"
#include "appConfig.h"

namespace DTS
{

   AppMesh::AppMesh()
   {
      mLocked = false;
      mSkinDataFetched = false;
   }

   AppMesh::~AppMesh()
   {
      assert(!mLocked && "Mesh is locked");
      S32 i;
      for (i=0; i<mBones.size(); i++)
         delete mBones[i];
      for (i=0; i<mWeights.size(); i++)
         delete mWeights[i];
      for (i=0; i<mIfls.size(); i++)
         delete mIfls[i];
   }

   S32 __cdecl compareFaces( void const *e1, void const *e2 )
   {
      const Primitive * face1 = (const Primitive*)e1;
      const Primitive * face2 = (const Primitive*)e2;

      if (face1->type < face2->type)
         return -1;
      else if (face2->type < face1->type)
         return 1;
      else return 0;
   }

   void AppMesh::sortFaceList()
   {
      if (mFaces.size() != 0)
         qsort(&mFaces[0],mFaces.size(),sizeof(Primitive),compareFaces);
   }

   Box AppMesh::getBounds(const Matrix<4,4,F32> & objectOffset)
   {
      assert(!mLocked && "Mesh is locked");

      AppMeshLock lock = lockMesh(AppTime::DefaultTime(),objectOffset);

      Box bounds(Point3D( 10E30f, 10E30f, 10E30f),Point3D(-10E30f,-10E30f,-10E30f));

      const Point3D * verts = getVerts();
      for (S32 i=0; i<getNumVerts(); i++)
      {
         if (bounds.min.x() > verts[i].x())
            bounds.min.x(verts[i].x());
         if (bounds.min.y() > verts[i].y())
            bounds.min.y(verts[i].y());
         if (bounds.min.z() > verts[i].z())
            bounds.min.z(verts[i].z());

         if (bounds.max.x() < verts[i].x())
            bounds.max.x(verts[i].x());
         if (bounds.max.y() < verts[i].y())
            bounds.max.y(verts[i].y());
         if (bounds.max.z() < verts[i].z())
            bounds.max.z(verts[i].z());
      }

      return bounds;
   }

   F32 AppMesh::getRadius(const Matrix<4,4,F32> & objectOffset)
   {
      Box bounds = getBounds(objectOffset);
      Point3D diameter = bounds.max-bounds.min;
      return diameter.length() * 0.5f;
   }

   F32 AppMesh::getTubeRadius(const Matrix<4,4,F32> & objectOffset)
   {
      Box bounds = getBounds(objectOffset);
      Point2D diameter(bounds.max.x()-bounds.min.x(),bounds.max.y()-bounds.min.y());
      return diameter.length() * 0.5f;
   }

   bool AppMesh::isBillboard()
   {
      return !strncmp(getName(),"BB::",4) || !strncmp(getName(),"BB_",3);
   }

   bool AppMesh::isBillboardZAxis()
   {
      return !strncmp(getName(),"BBZ::",5) || !strncmp(getName(),"BBZ_",4);
   }

   bool AppMesh::isSorted()
   {
      return !strncmp(getName(),"SORT::",6) || !strncmp(getName(),"SORT_",5);
   }

   bool AppMesh::isDummy()
   {
      return !strnicmp(getName(), "dummy", 5);
   }

   bool AppMesh::animatesVis(const AppSequenceData & seqData)
   {
      F32 defaultVis = getVisValue(AppTime::DefaultTime());
      AppTime time = seqData.startTime;
      for (S32 frame=0; frame<seqData.numFrames; frame++, time += seqData.delta)
         if (!isEqual(defaultVis,getVisValue(time),0.01f))
            return true;
      return false;
   }

   bool AppMesh::animatesMatFrame(const AppSequenceData & seqData)
   {
      // don't necessarily want to support this type of animation anymore
      return false;
   }

   bool AppMesh::animatesFrame(const AppSequenceData & seqData)
   {
      // don't necessarily want to support this type of animation anymore
      return false;
   }

   U16 AppMesh::addVertex(const Point3D & vert, const Point3D & norm, Point2D & tvert, U32 vertId)
   {
      assert(mVerts.size() == mTVerts.size() && "Verts and TVerts unbalanced");
      assert(mVerts.size() == mNormals.size() && "Verts and Normals unbalanced");
      assert(mVerts.size() == mVertId.size() && "Verts and vertIds unbalanced");
      for (S32 i=0; i<mVerts.size(); i++)
      {
         if (vertId != mVertId[i])
            continue;
         if (!isEqual(vert,mVerts[i],AppConfig::SameVertTOL()))
            continue;
         if (!isEqual(tvert,mTVerts[i],AppConfig::SameTVertTOL()))
            continue;
         if (!isEqual(norm,mNormals[i],AppConfig::SameNormTOL()))
            continue;
         return i;
      }
      mVerts.push_back(vert);
      mTVerts.push_back(tvert);
      mNormals.push_back(norm);
      mVertId.push_back(vertId);
      return mVerts.size()-1;
   }

   U16 AppMesh::addVertex(const Point3D & vert, Point2D & tvert, U32 vertId, U32 smooth)
   {
      assert(mVerts.size() == mTVerts.size() && "Verts and TVerts unbalanced");
      assert(mVerts.size() == mSmooth.size() && "Verts and smooth unbalanced");
      assert(mVerts.size() == mVertId.size() && "Verts and vertIds unbalanced");
      for (S32 i=0; i<mVerts.size(); i++)
      {
         if (vertId != mVertId[i])
            continue;
         if (smooth != mSmooth[i])
            continue;
         if (!isEqual(vert,mVerts[i],AppConfig::SameVertTOL()))
            continue;
         if (!isEqual(tvert,mTVerts[i],AppConfig::SameTVertTOL()))
            continue;
         return i;
      }
      mVerts.push_back(vert);
      mTVerts.push_back(tvert);
      mSmooth.push_back(smooth);
      mVertId.push_back(vertId);
      return mVerts.size()-1;
   }

   void AppMesh::generateFaces(std::vector<Primitive> & faces,
                               std::vector<Point3D>   & verts,
                               std::vector<Point2D>   & tverts,
                               std::vector<U16>       & indices,
                               std::vector<U32>       & smooth,
                               std::vector<Point3D>   & normals,
                               std::vector<U32>       * vertId)
   {
      assert(mLocked && "Mesh isn't locked");
      faces = mFaces;
      verts = mVerts;
      tverts = mTVerts;
      indices = mIndices;
      normals = mNormals;
      smooth = mSmooth;
      if (vertId)
         *vertId = mVertId;
   }

   AppMeshLock AppMesh::lockMesh(const AppTime & time, const Matrix<4,4,F32> & objectOffset)
   {
      assert(!mLocked && "Mesh is already locked");

      time,objectOffset;

      // if more than one mat type, make mats consecutive
      sortFaceList();

      mLocked = true;
      return AppMeshLock(this);
   }

   void AppMesh::unlockMesh()
   {
      assert(mLocked && "Mesh isn't locked");

      mFaces.clear();
      mVerts.clear();
      mTVerts.clear();
      mIndices.clear();
      mNormals.clear();
      mSmooth.clear();
      mVertId.clear();

      mLocked = false;
   }

   void AppMeshLock::doit() { if (mObj) mObj->unlockMesh(); }


}; // namespace DTS
