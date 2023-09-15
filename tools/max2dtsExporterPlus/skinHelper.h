//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef __SKINHELPER__H
#define __SKINHELPER__H

#pragma pack(push,8)
#include <Max.h>
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <ISkin.h>
#pragma pack(pop)

#include "DTSPlusTypes.h"

// Note: this skinhelper is same as skin helper in original max2dts
// But we give it a different id so that it can co-exist with the old one.
//#define SKINHELPER_CLASS_ID Class_ID(0x20dfdba0, 0x60646cb4)
#define SKINHELPER_CLASS_ID Class_ID(0x449d28a3, 0x4dd3242b)

extern ClassDesc* GetSkinHelperDesc();
extern TCHAR *GetString(S32);

class SkinHelper : public OSModifier
{
   public:
      static IObjParam *ip;         //Access to the interface
      
      // From Animatable
      TCHAR *GetObjectName() { return "SkinHelper"; }

      //From Modifier
      ChannelMask ChannelsUsed()  { return TEXMAP_CHANNEL; }
      ChannelMask ChannelsChanged() { return TEXMAP_CHANNEL; }
      void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
      void modifyTriObject(TriObject *, ISkin *, ISkinContextData *);
      void modifyPatchObject(PatchObject *, ISkin *, ISkinContextData *);

      Class_ID InputType() {return defObjectClassID;}
      Interval LocalValidity(TimeValue t);

      // From BaseObject
      BOOL ChangeTopology() {return FALSE;}      
      
      CreateMouseCallBack* GetCreateMouseCallBack() {return NULL;}
      void BeginEditParams(IObjParam *ip, ULONG flags,Animatable *prev);
      void EndEditParams(IObjParam *ip, ULONG flags,Animatable *next);

      Interval GetValidity(TimeValue t);

      // Automatic texture support
      BOOL HasUVW();
      void SetGenUVW(BOOL sw);
      
      // Loading/Saving
      IOResult Load(ILoad *iload);
      IOResult Save(ISave *isave);

      //From Animatable
      Class_ID ClassID() {return SKINHELPER_CLASS_ID;}      
      SClass_ID SuperClassID() { return OSM_CLASS_ID; }
      void GetClassName(TSTR& s) {s = "SkinHelper"; }
      
      RefTargetHandle Clone( RemapDir &remap );
      RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
         PartID& partID,  RefMessage message);

      int NumSubs() { return 0; }
      TSTR SubAnimName(int i) { return _T(""); }            
      Animatable* SubAnim(int i) { return NULL; }
      int NumRefs() { return 0; }
      RefTargetHandle GetReference(int i) { return NULL; }
      void SetReference(int i, RefTargetHandle rtarg);

      void DeleteThis() { delete this; }      
      //Constructor/Destructor
      SkinHelper();
      ~SkinHelper();      

      static INode * smTheOnlyOne;
};

#endif // __SKINHELPER__H
