//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#include "interior/interiorInstance.h"
#include "interior/lightUpdateGrouper.h"
#include "interior/interior.h"
#include "math/mRandom.h"

void InteriorInstance::echoTriggerableLights()
{
   // DMMFIX: Only the first detail for now...
   Interior* pInterior = mInteriorRes->getDetailLevel(0);

   Con::printf("Interior: %s", mInteriorFileName);
   Con::printf("  %d Triggerable lights:", pInterior->mNumTriggerableLights);

   // Triggerable lights are always the first in the array...
   for (U32 i = 0; i < pInterior->mNumTriggerableLights; i++) {
      const char* pName = pInterior->getName(pInterior->mAnimatedLights[i].nameIndex);
      U32 type          = pInterior->mAnimatedLights[i].flags & Interior::AnimationTypeMask;
      float duration    = pInterior->mAnimatedLights[i].duration;
      U32 numStates     = pInterior->mAnimatedLights[i].stateCount;

      Con::printf("  - %s [%s, Duration: %f, NumStates: %d]",
                      pName, Interior::getLightTypeString(Interior::LightType(type)),
                      duration, numStates);
   }
}

void InteriorInstance::activateLight(const char* pLightName)
{
   if (bool(mInteriorRes) == false) {
      AssertWarn(false, "Activating a light on an unloaded interior!");
      return;
   }

   // Now, it's a real pain in the ass to try to keep track of light states on detail
   //  changes as we did in tribes 1.  There, we analyzed the state on a detail change
   //  and tried to duplicate that state on the detail level we were switching to.
   //  Inspiration: forget that, and just animate the lights on all the details all
   //  the time.  Unless the detail is rendering, the lightmap data will never be
   //  downloaded, and the amount of time necessary to keep the lights updated on
   //  a detail level is absolutely miniscule.  Much easier.
   //
   for (U32 i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
      Interior* pInterior = mInteriorRes->getDetailLevel(i);

      for (U32 j = 0; j < pInterior->mNumTriggerableLights; j++) {
         const char* pILightName = pInterior->getName(pInterior->mAnimatedLights[j].nameIndex);
         if (dStricmp(pLightName, pILightName) == 0) {
            activateLight(i, j);
            break;
         }
      }
   }
}

void InteriorInstance::deactivateLight(const char* pLightName)
{
   if (bool(mInteriorRes) == false) {
      AssertWarn(false, "Deactivating a light on an unloaded interior!");
      return;
   }

   for (U32 i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
      Interior* pInterior = mInteriorRes->getDetailLevel(i);

      for (U32 j = 0; j < pInterior->mNumTriggerableLights; j++) {
         const char* pILightName = pInterior->getName(pInterior->mAnimatedLights[j].nameIndex);
         if (dStricmp(pLightName, pILightName) == 0) {
            deactivateLight(i, j);
            break;
         }
      }
   }
}

void InteriorInstance::updateAllLights(const U32 ms)
{
   if (bool(mInteriorRes) == false)
      return;

   for (U32 i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
      LightInfo& rLightInfo = mLightInfo[i];

      for (U32 j = 0; j < rLightInfo.mLights.size(); j++) {
         if (mAlarmState == Normal) {
            if (rLightInfo.mLights[j].active == true && rLightInfo.mLights[j].alarm == false)
               updateLightTime(i, j, ms);
         } else {
            if (rLightInfo.mLights[j].alarm == true)
               updateLightTime(i, j, ms);
         }
      }
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::activateLight(const U32 detail, const U32 lightIndex)
{
   AssertFatal(bool(mInteriorRes) && detail < mInteriorRes->getNumDetailLevels(), "Error, no interior resource, or out of range detail level");
   AssertFatal(lightIndex < mInteriorRes->getDetailLevel(detail)->mAnimatedLights.size(), "Error, out of bounds light index");

   LightInfo& rLightInfo    = mLightInfo[detail];
   LightInfo::Light& rLight = rLightInfo.mLights[lightIndex];

   if (rLight.active == false) {
      rLight.active   = true;
      rLight.curState = 0;
      rLight.curTime  = 0;

      Interior* pInterior = mInteriorRes->getDetailLevel(detail);
      Interior::LightState& rState = pInterior->mLightStates[pInterior->mAnimatedLights[lightIndex].stateIndex];
      rLight.curColor.set(rState.red, rState.green, rState.blue);

      installLight(detail, lightIndex);

      if (isServerObject() && lightIndex < pInterior->mNumTriggerableLights) {
         U32 key = makeUpdateKey(detail, lightIndex);
         U32 mask = mUpdateGrouper->getKeyMask(key);
         setMaskBits(mask);
      }
   } else {
      // Light is already active, no need to play around further...
      //
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::deactivateLight(const U32 detail, const U32 lightIndex)
{
   AssertFatal(bool(mInteriorRes) && detail < mInteriorRes->getNumDetailLevels(), "Error, no interior resource, or out of range detail level");
   AssertFatal(lightIndex < mInteriorRes->getDetailLevel(detail)->mAnimatedLights.size(), "Error, out of bounds light index");

   LightInfo& rLightInfo    = mLightInfo[detail];
   LightInfo::Light& rLight = rLightInfo.mLights[lightIndex];

   if (rLight.active == true) {
      // DMMFIX

      rLight.active   = false;
      rLight.curState = 0;
      rLight.curTime  = 0;

      Interior* pInterior = mInteriorRes->getDetailLevel(detail);
      Interior::LightState& rState = pInterior->mLightStates[pInterior->mAnimatedLights[lightIndex].stateIndex];
      rLight.curColor.set(rState.red, rState.green, rState.blue);

      installLight(detail, lightIndex);

      if (isServerObject() && lightIndex < pInterior->mNumTriggerableLights) {
         U32 key = makeUpdateKey(detail, lightIndex);
         U32 mask = mUpdateGrouper->getKeyMask(key);
         setMaskBits(mask);
      }
   } else {
      // Light is already inactive, no need to play around further...
      //
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::updateLightTime(const U32 detail, const U32 lightIndex, const U32 ms)
{
   AssertFatal(bool(mInteriorRes) && detail < mInteriorRes->getNumDetailLevels(), "Error, no interior resource, or out of range detail level");
   AssertFatal(lightIndex < mInteriorRes->getDetailLevel(detail)->mAnimatedLights.size(), "Error, out of bounds light index");

   LightInfo& rLightInfo = mLightInfo[detail];
   Interior* pInterior   = mInteriorRes->getDetailLevel(detail);

   LightInfo::Light& rLight         = rLightInfo.mLights[lightIndex];
   Interior::AnimatedLight& rILight = pInterior->mAnimatedLights[lightIndex];

   U32    oldState = rLight.curState;
   ColorI oldColor = rLight.curColor;

   // Ok, now we need to break this down a bit.  We pass the update along to
   //  the specialized updating functions based on lightType.
   switch (rILight.flags & Interior::AnimationTypeMask) {
     case Interior::AmbientLooping:
     case Interior::TriggerableLoop:
      updateLoopingLight(pInterior, rLight, lightIndex, ms);
      break;

     case Interior::AmbientFlicker:
     case Interior::TriggerableFlicker:
      updateFlickerLight(pInterior, rLight, lightIndex, ms);
      break;

     case Interior::TriggerableRamp:
      updateRampLight(pInterior, rLight, lightIndex, ms);
      break;

     default:
      AssertFatal(false, "Bad light type in updateLightTime");
   }

   if (rLight.curState != oldState ||
       rLight.curColor != oldColor) {
      // Need to reinstall the light
      installLight(detail, lightIndex);
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::updateLoopingLight(Interior* interior, LightInfo::Light& light,
                                          const U32 lightIndex, const U32 ms)
{
   AssertISV( lightIndex < interior->mAnimatedLights.size( ), "out of bounds array access in InteriorInstance::updateLoopingLight" );
   Interior::AnimatedLight& rILight = interior->mAnimatedLights[lightIndex];

   light.curTime += ms;
   light.curTime %= rILight.duration;

   // Find the last state that has a active time below this new time...
   light.curState = 0;
   for (U32 i = 1; i < rILight.stateCount; i++) {
      Interior::LightState& rState = interior->mLightStates[rILight.stateIndex + i];
      if (rState.activeTime <= light.curTime)
         light.curState = i;
      else
         break;
   }

   // interpolate the color
   Interior::LightState& rState = interior->mLightStates[rILight.stateIndex + light.curState];
   Interior::LightState* pNextState;

   U32 msIntoState = light.curTime - rState.activeTime;
   U32 msTotal;
   if (light.curState != (rILight.stateCount - 1)) {
      // Have one more good state
      pNextState  = &interior->mLightStates[rILight.stateIndex + light.curState + 1];
      msTotal = pNextState->activeTime - rState.activeTime;
   } else {
      // Have to interpolate against the first state...
      pNextState  = &interior->mLightStates[rILight.stateIndex];
      msTotal = rILight.duration - rState.activeTime;
   }

   F32 interp = F32(msIntoState) / F32(msTotal);
   F32 red    = F32(rState.red)   * (1.0f - interp) + F32(pNextState->red)   * interp;
   F32 green  = F32(rState.green) * (1.0f - interp) + F32(pNextState->green) * interp;
   F32 blue   = F32(rState.blue)  * (1.0f - interp) + F32(pNextState->blue)  * interp;

   light.curColor.set(U8(red + 0.5f), U8(green + 0.5f), U8(blue + 0.5f));
}


//--------------------------------------------------------------------------
void InteriorInstance::updateFlickerLight(Interior* interior, LightInfo::Light& light,
                                          const U32 lightIndex, const U32 ms)
{
   Interior::AnimatedLight& rILight = interior->mAnimatedLights[lightIndex];

   U32 switchPeriod = interior->mLightStates[interior->mAnimatedLights[lightIndex].stateIndex + 1].activeTime;
   U32 oldTime    = light.curTime;
   light.curTime += ms;
   if (light.curTime < switchPeriod)
      return;

   light.curTime = 0;

   // Ok, pick a random number from 0 to the light duration, and find the state that
   //  it falls in.

   static MRandomLCG randomGen;
   U32 pickedTime = randomGen.randI(0, rILight.duration);

   light.curState = 0;
   for (U32 i = 1; i < rILight.stateCount; i++) {
      Interior::LightState& rState = interior->mLightStates[rILight.stateIndex + i];
      if (rState.activeTime <= pickedTime) 
         light.curState = i;
      else
         break;
   }

   Interior::LightState& rState = interior->mLightStates[rILight.stateIndex + light.curState];
   light.curColor.set(rState.red, rState.green, rState.blue);
}


//--------------------------------------------------------------------------
void InteriorInstance::updateRampLight(Interior* interior, LightInfo::Light& light,
                                       const U32 lightIndex, const U32 ms)
{
   Interior::AnimatedLight& rILight = interior->mAnimatedLights[lightIndex];

   light.curTime += ms;
   if (light.curTime > rILight.duration)
      light.curTime = rILight.duration;

   // Find the last state that has a active time below this new time...
   light.curState = 0;
   for (U32 i = 1; i < rILight.stateCount; i++) {
      Interior::LightState& rState = interior->mLightStates[rILight.stateIndex + i];
      if (rState.activeTime <= light.curTime)
         light.curState = i;
      else
         break;
   }

   // interpolate the color
   Interior::LightState& rState = interior->mLightStates[rILight.stateIndex + light.curState];
   Interior::LightState* pNextState;

   U32 msIntoState = light.curTime - rState.activeTime;
   U32 msTotal;
   if (light.curState != (rILight.stateCount - 1)) {
      // Have one more good state
      pNextState  = &interior->mLightStates[rILight.stateIndex + light.curState + 1];
      msTotal = pNextState->activeTime - rState.activeTime;
   } else {
      // A ramp light does NOT NOT NOT interp against the first state
      pNextState = &rState;
      msTotal    = msIntoState;
   }

   F32 interp = F32(msIntoState) / F32(msTotal);
   F32 red    = F32(rState.red)   * (1.0f - interp) + F32(pNextState->red)   * interp;
   F32 green  = F32(rState.green) * (1.0f - interp) + F32(pNextState->green) * interp;
   F32 blue   = F32(rState.blue)  * (1.0f - interp) + F32(pNextState->blue)  * interp;

   light.curColor.set(U8(red + 0.5f), U8(green + 0.5f), U8(blue + 0.5f));
}


//--------------------------------------------------------------------------
void InteriorInstance::installLight(const U32 detail, const U32 lightIndex)
{
   AssertFatal(bool(mInteriorRes) && detail < mInteriorRes->getNumDetailLevels(), "Error, no interior resource, or out of range detail level");
   AssertFatal(lightIndex < mInteriorRes->getDetailLevel(detail)->mAnimatedLights.size(), "Error, out of bounds light index");

   LightInfo& rLightInfo    = mLightInfo[detail];
   LightInfo::Light& rLight = rLightInfo.mLights[lightIndex];

   // All we are allowed to assume is that the light time, color, and state are
   //  correct here. We must install all statedata, and invalidate all surfaces.
   //  First, let's retrieve the actual light from the Interior
   //
   Interior* pInterior = mInteriorRes->getDetailLevel(detail);
   Interior::AnimatedLight& rILight = pInterior->mAnimatedLights[lightIndex];
   Interior::LightState&    rIState = pInterior->mLightStates[rILight.stateIndex + rLight.curState];

   // Ok.  Now, cycle through the light's state data, and install it
   for (U32 i = rIState.dataIndex; i < (rIState.dataIndex + rIState.dataCount); i++) {
      Interior::LightStateData& rIData = pInterior->mStateData[i];
      LightInfo::StateDataInfo& rData  = rLightInfo.mStateDataInfo[rIData.lightStateIndex];

      if (rIData.mapIndex != 0xFFFFFFFF) {
         rData.curMap = &pInterior->mStateDataBuffer[rIData.mapIndex];
      } else {
         rData.curMap = NULL;
      }
      rData.curColor = rLight.curColor;
      rData.alarm    = (rILight.flags & Interior::AlarmLight) != 0;
      rLightInfo.mSurfaceInvalid.set(rIData.surfaceIndex);
   }
}

//--------------------------------------------------------------------------
void InteriorInstance::intensityMapMerge(U8*           lightMap,
                                         const U32     width,
                                         const U32     height,
                                         const U8*     intensityMap,
                                         const ColorI& color)
{
   // lightmap is a 24bit RGB texture, intensitymap is an 8 bit intensity
   //  map.  We want lightmap = [lightmap + (intensityMap * color)]

   // DMMFIX: SLOWSLOWSLOW!  Need MMX version of this at the very least,
   //  this version is only for clarity;
   for (U32 y = 0; y < height; y++) {
      for (U32 x = 0; x < width; x++) {
         U8* data      = &lightMap[(y * width + x) * 3];
         U32 intensity = intensityMap[(y * width + x)];

         U32 newRed   = data[0];
         U32 newGreen = data[1];
         U32 newBlue  = data[2];

         U32 addRed   = (U32(color.red)   * intensity + 0x80) >> 8;
         U32 addGreen = (U32(color.green) * intensity + 0x80) >> 8;
         U32 addBlue  = (U32(color.blue)  * intensity + 0x80) >> 8;

         newRed   += addRed;
         newGreen += addGreen;
         newBlue  += addBlue;

         data[0] = (newRed   <= 255) ? U8(newRed)   : 0xFF;
         data[1] = (newGreen <= 255) ? U8(newGreen) : 0xFF;
         data[2] = (newBlue  <= 255) ? U8(newBlue)  : 0xFF;
      }
   }
}

