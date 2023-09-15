//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPSEQUENCE_H_
#define DTSAPPSEQUENCE_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "appTime.h"


namespace DTS
{
   // Enum for Triggers...
   namespace TriggerState
   {
      enum { StateOn = 1 << 31, InvertOnReverse = 1 << 30, StateMask = 0x1f };
   };

   class AppSequence;
   class AppNode;

   struct AppSequenceData 
   {
      bool cyclic;
      bool blend;
      bool ignoreGround;

      bool enableMorph;
      bool enableTVert;
      bool enableVis;
      bool enableTransform;
      bool enableScale;
      bool enableUniformScale;
      bool enableAlignedScale;
      bool enableArbitraryScale;
      bool enableIFL;

      bool forceMorph;
      bool forceTVert;
      bool forceVis;
      bool forceTransform;
      bool forceScale;

      AppTime duration;
      AppTime delta;
      AppTime startTime;
      AppTime endTime;
      AppTime blendReferenceTime;
      AppTime groundDelta;

      S32 numFrames;
      S32 groundNumFrames;

      F32 frameRate;
      F32 groundFrameRate;

      F32 overrideDuration;

      F32  priority;
   };

   class AppSequence
   {
      public:
         virtual const char * getName() = 0;

         virtual void getSequenceData(AppSequenceData *) = 0;

         virtual S32 getNumTriggers() = 0;
         virtual Trigger getTrigger(S32 idx) = 0;

         virtual void setTSSequence(Sequence *);
   };

   // Create a sequence from an appropriately outfitted node
   // Uses property values to figure everything out.
   // This system is adequate for all dts features and will
   // work in any 3d package which supports naming nodes and adding
   // property values to nodes.  Specific 3d apps can add support
   // guis to manipulate these sequence objects.
   class AppSequenceNode : public AppSequence
   {
         AppNode * mAppNode;

      public:

         AppSequenceNode(AppNode * node) { mAppNode=node; }

         const char * getName();
         void getSequenceData(AppSequenceData *);
         S32 getNumTriggers();
         Trigger getTrigger(S32 idx);
   };

};

#endif // DTSAPPSEQUENCE_H_

