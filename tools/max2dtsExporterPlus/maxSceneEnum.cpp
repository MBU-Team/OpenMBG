//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "maxSceneEnum.h"
#include "maxAppNode.h"

#pragma pack(push,8)
#include <max.h>
#include <ISTDPLUG.H>
#pragma pack(pop)


namespace DTS
{

   class PrivateMaxEnum : public ITreeEnumProc 
   {

      MaxSceneEnum * mMaxSceneEnum;

   public:

      PrivateMaxEnum(MaxSceneEnum * sceneEnum) { mMaxSceneEnum = sceneEnum; }
      S32 callback( INode * node ) { return mMaxSceneEnum->callback(node); }
      void enumScene(IScene * scene ) { scene->EnumTree(this); }
   };

   MaxSceneEnum::MaxSceneEnum(IScene * theScene)
   {
      mTheScene = theScene;
   }

   MaxSceneEnum::~MaxSceneEnum()
   {
   }

   S32 MaxSceneEnum::callback(INode * node)
   {
      MaxAppNode * maxAppNode = new MaxAppNode(node);
      if (!processNode(maxAppNode))
         delete maxAppNode;
      return TREE_CONTINUE;
   }

   void MaxSceneEnum::enumScene()
   {
      PrivateMaxEnum pme(this);
      pme.enumScene(mTheScene);
   }

}; // namespace DTS

