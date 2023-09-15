//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/gBitmap.h"
#include "math/mathIO.h"
#include "core/bitStream.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "terrain/terrData.h"
#include "math/mathUtils.h"
#include "terrain/sun.h"

IMPLEMENT_CO_NETOBJECT_V1(Sun);

//-----------------------------------------------------------------------------

Sun::Sun()
{
   mNetFlags.set(Ghostable | ScopeAlways);
   mTypeMask = EnvironmentObjectType;

   mLight.mType = LightInfo::Vector;
   mLight.mDirection.set(0.f, 0.707f, -0.707f);
   mLight.mColor.set(0.7f, 0.7f, 0.7f);
   mLight.mAmbient.set(0.3f, 0.3f, 0.3f);

   mSunAzimuth = 0.0f;
   mSunElevation = 35.0f;
}

//-----------------------------------------------------------------------------

void Sun::conformLight()
{
   mLight.mDirection.normalize();
   mLight.mColor.clamp();
   mLight.mAmbient.clamp();
}

//-----------------------------------------------------------------------------

bool Sun::onAdd()
{
   if(!Parent::onAdd())
      return(false);

   if(isClientObject())
      Sim::getLightSet()->addObject(this);
   else
      conformLight();

   return(true);
}

void Sun::registerLights(LightManager * lightManager, bool)
{
   lightManager->addLight(&mLight);
}

//-----------------------------------------------------------------------------

void Sun::inspectPostApply()
{
   conformLight();
   setMaskBits(UpdateMask);
}

void Sun::unpackUpdate(NetConnection *, BitStream * stream)
{
   if(stream->readFlag())
   {
      // direction -> color -> ambient
      mathRead(*stream, &mLight.mDirection);

      stream->read(&mLight.mColor.red);
      stream->read(&mLight.mColor.green);
      stream->read(&mLight.mColor.blue);
      stream->read(&mLight.mColor.alpha);

      stream->read(&mLight.mAmbient.red);
      stream->read(&mLight.mAmbient.green);
      stream->read(&mLight.mAmbient.blue);
      stream->read(&mLight.mAmbient.alpha);
   }
}

U32 Sun::packUpdate(NetConnection *, U32 mask, BitStream * stream)
{
   if(stream->writeFlag(mask & UpdateMask))
   {

		// Calculate Light Direction.
		F32 Yaw = mDegToRad(mClampF(mSunAzimuth,0,359));
		F32 Pitch = mDegToRad(mClampF(mSunElevation,-360,+360));
		VectorF sunvec;
		MathUtils::getVectorFromAngles(sunvec, Yaw, Pitch);
		mLight.mDirection = -sunvec;

      // direction -> color -> ambient
      mathWrite(*stream, mLight.mDirection);

      stream->write(mLight.mColor.red);
      stream->write(mLight.mColor.green);
      stream->write(mLight.mColor.blue);
      stream->write(mLight.mColor.alpha);

      stream->write(mLight.mAmbient.red);
      stream->write(mLight.mAmbient.green);
      stream->write(mLight.mAmbient.blue);
      stream->write(mLight.mAmbient.alpha);
   }
   return(0);
}

//-----------------------------------------------------------------------------

void Sun::initPersistFields()
{
	Parent::initPersistFields();
	addGroup("Misc");	// MM: Added Group Header.
		addField( "azimuth",		TypeF32,		Offset( mSunAzimuth,		Sun ) );
		addField( "elevation",		TypeF32,		Offset( mSunElevation,		Sun ) );
		//addField( "direction",		TypePoint3F,	Offset(mLight.mDirection,	Sun));
		addField( "color",			TypeColorF,		Offset(mLight.mColor,		Sun));
		addField( "ambient",		TypeColorF,		Offset(mLight.mAmbient,		Sun));
	endGroup("Misc");	// MM: Added Group Footer.

}
