//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "appNode.h"
#include "DTSUtil.h"

namespace DTS
{

   AppNode::AppNode()
   {
      mName = NULL;
      mParentName = NULL;
   }

   AppNode::~AppNode()
   {
      S32 i;
      for (i=0; i<mMeshes.size(); i++)
         delete mMeshes[i];
      for (i=0; i<mChildNodes.size(); i++)
         delete mChildNodes[i];
      mMeshes.clear();
      mChildNodes.clear();

      delete [] mName;
      delete [] mParentName;
   }

   S32 AppNode::getNumMesh()
   {
      if (mMeshes.size() == 0)
         buildMeshList();
      return mMeshes.size();
   }

   AppMesh * AppNode::getMesh(S32 idx)
   {
      if (mMeshes.size() == 0)
         buildMeshList();
      if (idx<mMeshes.size() && idx>=0)
         return mMeshes[idx];
      return NULL;
   }

   S32 AppNode::getNumChildNodes()
   {
      if (mChildNodes.size() == 0)
         buildChildList();
      return mChildNodes.size();
   }

   AppNode * AppNode::getChildNode(S32 idx)
   {
      if (mChildNodes.size() == 0)
         buildChildList();
      if (idx<mChildNodes.size() && idx>=0)
         return mChildNodes[idx];
      return NULL;
   }

   bool AppNode::isDummy()
   {
      // naming convention should work well enough...
      // ...but can override this method if one wants more
      return !strnicmp(getName(), "dummy", 5);
   }

   bool AppNode::isBounds()
   {
      // naming convention should work well enough...
      // ...but can override this method if one wants more
      return !stricmp(getName(), "bounds");
   }

   bool AppNode::isRoot()
   {
      // we assume root node isn't added, so this is never true
      // but allow for possibility (by overriding this method)
      // so that isParentRoot still works.
      return false;
   }

}; // namespace DTS


