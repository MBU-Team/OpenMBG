//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "appSceneEnum.h"
#include "appSequence.h"

namespace DTS
{

   AppSequence * AppSceneEnum::getSequence(AppNode * node)
   {
      const char * prefix = "Sequence::";
      const char * name = node->getName();
      if (!strnicmp(name,prefix,strlen(prefix)))
         return new AppSequenceNode(node);
      prefix = "Sequence_";
      if (!strnicmp(name,prefix,strlen(prefix)))
         return new AppSequenceNode(node);

      return NULL;
   }

   bool AppSceneEnum::processNode(AppNode * node)
   {
      // Helper method to help rot nodes that we find in the scene.

      // At this stage we do not need to collect all the nodes
      // because the tree structure will still be there when we
      // build the shape.  What we need to do right now is grab
      // the top of all the subtrees, any meshes hanging on the
      // root level (these will be lower detail levels, we don't
      // need to grab meshes on the sub-trees because they will
      // be found when we recurse into the sub-tree), the bounds
      // node, and any sequences.

      const char * name  = node->getName();
      const char * pname = node->getParentName();

      AppConfig::PrintDump(PDPass1,avar("Processing Node %s with parent %s\r\n", name, pname));

      AppSequence * seq = getSequence(node);
      if (seq)
      {         
         sequences.push_back(seq);
         return true;
      }

      if (node->isDummy())
         return false;

      if (isSubtree(node))
      {
         // Add this node to the subtree list...
         AppConfig::PrintDump(PDPass1,avar("Found subtree starting at Node \"%s\"\r\n",name));
         subtrees.push_back(node);
         return true;
      }

      // See if it is a bounding box.  If so, save it as THE bounding
      // box for the scene
      if (node->isBounds())
      {
         if (boundsNode)
         {
            setExportError("More than one bounds node found.");
            AppConfig::PrintDump(PDPass1,"More than one bounds node found.\r\n");
         }
         else
            AppConfig::PrintDump(PDPass1,"Bounding box found\r\n");
         boundsNode = node;
         return true;
      }

      // if we use this node, then be sure to return true so the caller doesn't delete it
      bool used = false;

      if (node->getNumMesh()!=0)
      {
         for (S32 i=0; i<node->getNumMesh(); i++)
         {
            AppMesh * mesh = node->getMesh(i);
            if (mesh->isSkin())
            {
               AppConfig::PrintDump(PDPass1,avar("Skin \"%s\" with parent \"%s\" added to entry list\r\n",mesh->getName(),pname));
               skins.push_back(mesh);
               used = true;
            }
            else
            {
               if (node->isParentRoot())
               {
                  AppConfig::PrintDump(PDPass1,avar("Mesh \"%s\" with parent \"%s\" added to entry list\r\n",mesh->getName(),pname));
                  meshNodes.push_back(node);
                  meshes.push_back(mesh);
                  used = true;
               }
            }
         }
         if (used)
            usedNodes.push_back(node);
      }
      return used;
   }

   bool AppSceneEnum::isSubtree(AppNode * node)
   {
      return node->isParentRoot() && node->getNumMesh() == 0;
   }

   AppSceneEnum::AppSceneEnum()
   {
      boundsNode = NULL;
   }

   AppSceneEnum::~AppSceneEnum()
   {
      S32 i;
      for (i=0; i<usedNodes.size(); i++)
         delete usedNodes[i];
      for (i=0; i<subtrees.size(); i++)
         delete subtrees[i];
      for (i=0; i<sequences.size(); i++)
         delete sequences[i];
      delete boundsNode;
   }

   Shape * AppSceneEnum::processScene()
   {
      setExportError(NULL);

      AppConfig::PrintDump(PDPass1,"First pass:  enumerate scene...\r\n\r\n");

      enumScene();

      if (!boundsNode)
         AppConfig::SetExportError("No bounds found");

      if (AppConfig::IsExportError())
         return NULL;

      AppConfig::PrintDump(PDPass2,"\r\nSecond pass:  put shape structure together...\r\n\r\n");

      // set up bounds node
      shapeMimic.addBounds(boundsNode);

      // add other subtrees
      S32 i;
      for (i=0; i<subtrees.size(); i++)
         shapeMimic.addSubtree(subtrees[i]);

      // add meshes
      for (i=0; i<meshes.size(); i++)
         shapeMimic.addMesh(meshNodes[i],meshes[i]);

      // add skin
      for (i=0; i<skins.size(); i++)
         shapeMimic.addSkin(skins[i]);

      // add sequences
      for (i=0; i<sequences.size(); i++)
         shapeMimic.addSequence(sequences[i]);

      // generate the shape
      return shapeMimic.generateShape();
   }

}; // namespace DTS
