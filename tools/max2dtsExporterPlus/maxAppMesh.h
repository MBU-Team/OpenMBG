//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSMAXAPPMESH_H_
#define DTSMAXAPPMESH_H_

#include "appMesh.h"
#include "appNode.h"

class INode;

namespace DTS
{

   class MaxAppMesh : public AppMesh
   {
      typedef AppMesh Parent;

      protected:

      // Don't hold onto anything mesh-like in max, keep the node instead
      INode * mMaxNode;

      // Hold this in addition just for convenience of a few methods that
      // use app node methods
      AppNode * mAppNode;

      void getSkinData();

      public:

      MaxAppMesh(INode * maxNode, AppNode * appNode);

      const char * getName() { return mAppNode->getName(); }
      bool getFloat(const char * propName, F32  & defaultVal) { return mAppNode->getFloat(propName,defaultVal); }
      bool   getInt(const char * propName, S32  & defaultVal) { return mAppNode->getInt(propName,defaultVal); }
      bool  getBool(const char * propName, bool & defaultVal) { return mAppNode->getBool(propName,defaultVal); }

      Matrix<4,4,F32> getMeshTransform(const AppTime & time);
      F32 getVisValue(const AppTime & time);

      bool getMaterial(S32 matIdx, Material &);

      S32 getNumBones();
      AppNode * getBone(S32 idx);
      F32 getWeight(S32 boneIdx, S32 vertIdx);

      bool animatesMatFrame(const AppSequenceData & seqData);
      bool animatesFrame(const AppSequenceData & seqData);

      AppMeshLock lockMesh(const AppTime & time, const Matrix<4,4,F32> & objectOffset);
      void unlockMesh();
   };

};

#endif // DTSMAXAPPMESH_H_
