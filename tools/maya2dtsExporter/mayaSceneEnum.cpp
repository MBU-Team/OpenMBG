//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "mayaSceneEnum.h"
#include "mayaAppNode.h"
#include <maya/MItDag.h>
#include <maya/MDagPath.h>

namespace DTS
{

   void MayaSceneEnum::enumScene()
   {
      MItDag dagIt(MItDag::kBreadthFirst);
      for (dagIt.next(); !dagIt.isDone(); dagIt.next())
      {
         MDagPath path;
         if (dagIt.getPath (path) != MS::kSuccess)
            continue;

         if (path.hasFn(MFn::kMesh) && !path.hasFn(MFn::kTransform))
            // don't want meshes...
            continue;

         MayaAppNode * mayaNode = new MayaAppNode(path);
         if (!processNode(mayaNode))
            delete mayaNode;

         // could prune here (i.e., not explore depth) but might find
         // bounds in tree so go ahead and plum the depths of the dag
      }
   }
}; // namespace DTS

