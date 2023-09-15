//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SUN_H_
#define _SUN_H_

#ifndef _NETOBJECT_H_
#include "sim/netObject.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif

class Sun : public NetObject
{
   private:
      typedef NetObject Parent;

      LightInfo      mLight;
      
      void conformLight();

   public:

      Sun();

      // SimObject
      bool onAdd();
      void registerLights(LightManager *lm, bool lightingScene);

      //
      void inspectPostApply();

      static void initPersistFields();

      // NetObject
      enum NetMaskBits {
         UpdateMask     = BIT(0)
      };

      U32  packUpdate  (NetConnection *conn, U32 mask, BitStream * stream);
      void unpackUpdate(NetConnection *conn,           BitStream * stream);

	F32		mSunAzimuth;
	F32		mSunElevation;


      DECLARE_CONOBJECT(Sun);
};

#endif
