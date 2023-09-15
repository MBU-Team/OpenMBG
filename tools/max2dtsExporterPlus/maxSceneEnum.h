//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSMAXSCENEENUM_H_
#define DTSMAXSCENEENUM_H_

#include "appSceneEnum.h"

class INode;
class IScene;

namespace DTS
{

   class MaxSceneEnum : public AppSceneEnum
   {
   protected:
      IScene * mTheScene;

   public:
      MaxSceneEnum(IScene*);
      ~MaxSceneEnum();

      S32 callback(INode * node);
      void enumScene();      
   };

}; // namespace DTS

#endif // #define DTSMAXSCENEENUM_H_
