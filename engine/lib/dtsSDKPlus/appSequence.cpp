//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "appSequence.h"
#include "appNode.h"
#include "appConfig.h"
#include "DTSShape.h"

namespace DTS
{

   const char * AppSequenceNode::getName()
   {
      const char * prefix = "Sequence::";
      const char * name = mAppNode->getName();
      if (!strnicmp(name,prefix,strlen(prefix)))
         name += strlen(prefix);
      return name;
   }

   void AppSequenceNode::getSequenceData(AppSequenceData * seqData)
   {
      seqData->cyclic = true;
      seqData->blend = false;
      seqData->ignoreGround = false;

      seqData->enableMorph = false;
      seqData->enableTVert = false;
      seqData->enableVis = true;
      seqData->enableTransform = true;
      seqData->enableScale = true;
      seqData->enableUniformScale = true;
      seqData->enableAlignedScale = true;
      seqData->enableArbitraryScale = true;
      seqData->enableIFL = true;

      seqData->forceMorph = false;
      seqData->forceTVert = false;
      seqData->forceVis = false;
      seqData->forceTransform = false;
      seqData->forceScale = false;

      seqData->frameRate = 30;
      seqData->groundFrameRate = 10;
      seqData->numFrames = 0;
      seqData->groundNumFrames = 0;

      seqData->overrideDuration = -1.0f;

      seqData->priority = 5.0f;

      S32 startFrame = 0;
      S32 endFrame = 0;
      S32 blendReferenceFrame = 0;

      mAppNode->getBool("cyclic",seqData->cyclic);
      mAppNode->getBool("blend",seqData->blend);
      mAppNode->getBool("ignoreGround",seqData->ignoreGround);
      mAppNode->getBool("enableMorph",seqData->enableMorph);
      mAppNode->getBool("enableTVert",seqData->enableTVert);
      mAppNode->getBool("enableVis",seqData->enableVis);
      mAppNode->getBool("enableTransform",seqData->enableTransform);
      mAppNode->getBool("enableScale",seqData->enableScale);
      mAppNode->getBool("enableUniformScale",seqData->enableUniformScale);
      mAppNode->getBool("enableAlignedScale",seqData->enableAlignedScale);
      mAppNode->getBool("enableArbitraryScale",seqData->enableArbitraryScale);
      mAppNode->getBool("enableIFL",seqData->enableIFL);

      mAppNode->getBool("forceMorph",seqData->forceMorph);
      mAppNode->getBool("forceTVert",seqData->forceTVert);
      mAppNode->getBool("forceVis",seqData->forceVis);
      mAppNode->getBool("forceTransform",seqData->forceTransform);
      mAppNode->getBool("forceScale",seqData->forceScale);

      mAppNode->getFloat("priority",seqData->priority);

      mAppNode->getInt("startFrame",startFrame);
      mAppNode->getInt("endFrame",endFrame);
      mAppNode->getInt("blendReferenceFrame",blendReferenceFrame);
      mAppNode->getFloat("overrideDuration",seqData->overrideDuration);

      mAppNode->getFloat("frameRate",seqData->frameRate);
      if (seqData->frameRate<0.000001f)
         seqData->frameRate = 1.0f/30.0f;

      mAppNode->getFloat("groundFrameRate",seqData->groundFrameRate);
      if (seqData->groundFrameRate<0.000001f)
         seqData->groundFrameRate = 1.0f/10.0f;

      // convert from frames to times
      F32 appFPS = AppConfig::AppFramesPerSec(); // not necessarily same as exported fps
      F32 startTime = F32(startFrame) / appFPS;
      F32 endTime = F32(endFrame) / appFPS;
      F32 blendReferenceTime = F32(blendReferenceFrame) / appFPS;

      seqData->startTime.set(startTime,0);
      seqData->endTime.set(endTime,0);
      seqData->blendReferenceTime.set(blendReferenceTime,0);

      F32 duration = seqData->endTime.getF32() - seqData->startTime.getF32();
      if (seqData->cyclic)
         // This is required to match up with what is displayed by 3dsMax when 
         // playing animation cyclically.  Set to zero for apps that don't need it.
         duration += AppConfig::CyclicSequencePadding();
      F32 delta = 0.0f;
      F32 groundDelta = 0.0f;

      // Get sequence timing information
      if (mAppNode->getInt("numFrames",seqData->numFrames) && seqData->numFrames>0)
         seqData->numFrames--;
      else
         seqData->numFrames = (S32) ((duration + 0.25f/seqData->frameRate) * seqData->frameRate);
      delta = seqData->numFrames ? duration / F32(seqData->numFrames) : duration;
      if (!seqData->cyclic)
         seqData->numFrames++;

      // Get sequence ground timing information
      if (mAppNode->getInt("groundNumFrames",seqData->groundNumFrames) && seqData->groundNumFrames>1)
      {
         groundDelta = duration / (F32)(seqData->groundNumFrames-1);
      }
      else
      {
         seqData->groundNumFrames = (S32)((duration + 0.25f/seqData->groundFrameRate) * seqData->groundFrameRate);
         groundDelta = seqData->groundNumFrames ? duration / (F32) seqData->groundNumFrames : duration;
         seqData->groundNumFrames++;
      }

      seqData->duration.set(duration,0);
      seqData->delta.set(delta,0);
      seqData->groundDelta.set(groundDelta,0);
   }

   S32 AppSequenceNode::getNumTriggers()
   {
      S32 num = 0;
      mAppNode->getInt("numTriggers",num);
      return num;
   }

   Trigger AppSequenceNode::getTrigger(S32 idx)
   {
      char buffer[256];

      sprintf(buffer,"triggerFrame%i",idx);
      S32 frame = 0;
      mAppNode->getInt(buffer,frame);

      sprintf(buffer,"triggerState%i",idx);
      S32 state = 0;
      mAppNode->getInt(buffer,state);

      AppSequenceData seqData;
      getSequenceData(&seqData);
      F32 appFPS = AppConfig::AppFramesPerSec(); // not necessarily same as exported fps

      Trigger ret;
      ret.state = state;
      if (seqData.duration.getF32()<0.001f)
         ret.pos=0;
      else
         ret.pos = F32(frame/appFPS-seqData.startTime.getF32())/seqData.duration.getF32();
      return ret;
   }

   void AppSequence::setTSSequence(Sequence * seq)
   {
      AppSequenceData seqData;
      getSequenceData(&seqData);

     // fill in some sequence data
     seq->flags = 0;
     if (seqData.cyclic)
      seq->flags |= Sequence::Cyclic;
     if (seqData.blend)
      seq->flags |= Sequence::Blend;
     seq->priority = seqData.priority;
     seq->numKeyFrames = seqData.numFrames;
     seq->duration = seqData.overrideDuration>0 ? seqData.overrideDuration : seqData.duration.getF32();
     seq->toolBegin = seqData.startTime.getF32();
     seq->numTriggers = 0;
     seq->firstTrigger = 0;
   }


}; // namespace DTS

