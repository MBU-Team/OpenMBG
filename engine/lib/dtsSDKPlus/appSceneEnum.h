//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPSCENEENUM_H_
#define DTSAPPSCENEENUM_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "ShapeMimic.h"
#include "appMesh.h"
#include "appNode.h"
#include "appSequence.h"
#include "appConfig.h"

namespace DTS
{

   class AppSceneEnum
   {
   protected:
      std::vector<AppNode*> usedNodes; // store nodes that need
                                       // to be deleted later but
                                       // not tracked in other lists

      std::vector<AppMesh*> meshes;    // don't delete these...children of nodes in usedNodes
      std::vector<AppMesh*> skins;     // ditto
      std::vector<AppNode*> meshNodes; // don't delete these...might contain dups
      std::vector<AppNode*> subtrees;
      std::vector<AppSequence*> sequences;
      AppNode * boundsNode;

      ShapeMimic shapeMimic;
    
      void setExportError(const char * errStr) { AppConfig::SetExportError(errStr); }
      const char * getError() { return AppConfig::GetExportError(); }
      bool isError() { return AppConfig::IsExportError(); }

      virtual bool isSubtree(AppNode * node);
      virtual AppSequence * getSequence(AppNode *);

      bool processNode(AppNode *);

   public:
      AppSceneEnum();
      ~AppSceneEnum();

      virtual void enumScene() = 0;      
      Shape * processScene();
   };

}; // namespace DTS

#endif // #define DTSAPPSCENEENUM_H_
