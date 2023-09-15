//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPNODE_H_
#define DTSAPPNODE_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "appMesh.h"

namespace DTS
{
   class AppNode
   {
      // add attached meshes and child nodes to app node
      // the reason these are tracked by AppNode is that
      // AppNode is responsible for deleting all it's children
      // and attached meshes.
      virtual void buildMeshList() = 0;
      virtual void buildChildList() = 0;

      protected:

      std::vector<AppMesh*> mMeshes;
      std::vector<AppNode*> mChildNodes;
      char * mName;
      char * mParentName;

      public:

      AppNode();
      virtual ~AppNode();

      S32 getNumMesh();
      AppMesh * getMesh(S32 idx);

      S32 getNumChildNodes();
      AppNode * getChildNode(S32 idx);


      virtual Matrix<4,4,F32> getNodeTransform(const AppTime & time) = 0;

      virtual bool isEqual(AppNode *) = 0;

      virtual bool animatesTransform(const AppSequenceData & seqData) = 0;

      virtual const char * getName() = 0;
      virtual const char * getParentName() = 0;

      virtual bool getFloat(const char * propName, F32  & defaultVal) = 0;
      virtual bool   getInt(const char * propName, S32  & defaultVal) = 0;
      virtual bool  getBool(const char * propName, bool & defaultVal) = 0;

      virtual bool isParentRoot() = 0;
      virtual bool isDummy();
      virtual bool isBounds();
      virtual bool isRoot();
   };

};

#endif // DTSAPPNODE_H_

