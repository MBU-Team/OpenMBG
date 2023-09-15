//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "skinHelper.h"
#include "appConfig.h"

#pragma pack(push,8)
#include <Max.h>
#include <decomp.h>
#include <dummy.h>
#include <ISkin.h>
#include <modstack.h>
#pragma pack(pop)

class SkinHelperClassDesc:public ClassDesc {
   public:
   int          IsPublic() {return 1;}
   void *         Create(BOOL loading = FALSE) {return new SkinHelper();}
   const TCHAR *   ClassName() {return "Skin Helper";}
   SClass_ID      SuperClassID() {return OSM_CLASS_ID;}
   Class_ID      ClassID() {return SKINHELPER_CLASS_ID;}
   const TCHAR*    Category() {return "General";}
   void ResetClassParams(BOOL) {}
};

static SkinHelperClassDesc SkinHelperDesc;
ClassDesc* GetSkinHelperDesc() {return &SkinHelperDesc;}

IObjParam * SkinHelper::ip = NULL;
INode * SkinHelper::smTheOnlyOne = NULL;

//--- SkinHelper -------------------------------------------------------
SkinHelper::SkinHelper()
{
}

SkinHelper::~SkinHelper()
{
}

S32 gShouldOnlyBeOne=0;

void SkinHelper::SetReference(S32 i, RefTargetHandle rtarg)
{
   gShouldOnlyBeOne++;
   if (gShouldOnlyBeOne != 1)
      DTS::AppConfig::SetExportError("Assertion failed on skin object -- added two skin helper objects");
}

Interval SkinHelper::LocalValidity(TimeValue t)
{
   // if being edited, return NEVER forces a cache to be built 
   // after previous modifier.
   if (TestAFlag(A_MOD_BEING_EDITED))
      return NEVER;  
   //TODO: Return the validity interval of the modifier
      return NEVER;
}

RefTargetHandle SkinHelper::Clone(RemapDir& remap)
{
   SkinHelper* newmod = new SkinHelper();   
   //TODO: Add the cloning code here
   return(newmod);
}

// This sits in maxAppMesh.cpp
extern void findSkinData(INode * pNode, ISkin **skin, ISkinContextData ** skinData);

void SkinHelper::ModifyObject(TimeValue t, ModContext &mc, ObjectState * os, INode *pNode) 
{
   pNode = smTheOnlyOne;
   if (!pNode)
      return;

   ISkin * skin;
   ISkinContextData * skinData;
   if (!pNode)
      return;
   findSkinData(pNode,&skin,&skinData);
   if (!skin || !skinData)
      return;

   TriObject * triObj = (TriObject*)os->obj->ConvertToType(0,Class_ID(TRIOBJ_CLASS_ID,0));
   if (triObj!=os->obj)
      delete triObj;
   else
      modifyTriObject(triObj,skin,skinData);
   PatchObject * patchObj = (PatchObject*)os->obj->ConvertToType(0,Class_ID(PATCHOBJ_CLASS_ID,0));
   if (patchObj!=os->obj)
      delete patchObj;
   else
      modifyPatchObject(patchObj,skin,skinData);
}

void SkinHelper::modifyTriObject(TriObject * triObj, ISkin * skin, ISkinContextData * skinData)
{
   S32 numBones = skin->GetNumBones();

   Mesh & maxMesh = triObj->mesh;
   S32 numVerts  = maxMesh.getNumVerts();
   S32 numTVerts = maxMesh.getNumMapVerts(1);
   if (numVerts!=skinData->GetNumPoints())
      return;

   S32 numChannels = 2+((numBones+1)>>1);
   UVVert tv(0,0,0);
   S32 i,j;
   for (i=2; i<numChannels; i++)
   {
      maxMesh.setMapSupport(i,true);
      maxMesh.setNumMapVerts(i,numVerts);
      for (j=0; j<numVerts; j++)
         maxMesh.setMapVert(i,j,tv);
      maxMesh.setNumMapFaces(i,maxMesh.getNumFaces());
      // copy map faces from the first channel
      for (j=0; j<maxMesh.getNumFaces(); j++)
      {
         Face & face = maxMesh.faces[j];
         TVFace & tvFace  = maxMesh.mapFaces(i)[j];
         tvFace.t[0] = face.v[0];
         tvFace.t[1] = face.v[1];
         tvFace.t[2] = face.v[2];
      }
   }

   for (S32 v=0; v<numVerts; v++)
   {
      for (i=0; i<skinData->GetNumAssignedBones(v); i++)
      {
         S32 bone = skinData->GetAssignedBone(v,i);
         F32 w = skinData->GetBoneWeight(v,i);
         UVVert tv = maxMesh.mapVerts(2+(bone>>1))[v];
         if (bone&1)
            tv.y = w;
         else
            tv.x = w;
         maxMesh.setMapVert(2+(bone>>1),v,tv);
      }
   }
}

void SkinHelper::modifyPatchObject(PatchObject * patchObj, ISkin * skin, ISkinContextData * skinData)
{
   S32 numBones = skin->GetNumBones();
   S32 numPoints = skinData->GetNumPoints();
   S32 i;

   PatchMesh & maxMesh = patchObj->patch;
   S32 numVerts  = maxMesh.getNumVerts();
   if (numVerts>numPoints)
      // points should be more than verts...first set of points are the verts, the rest are control verts
      // we don't do anything with those weights...it limits the surface deformations that can take
      // place, but it's all we can do...
      return;

   if (!numBones)
      return;

   S32 numChannels = 2+((numBones+1)>>1);
   S32 numTVerts = maxMesh.getNumMapVerts(1);
   UVVert tv(0,0,0);

   maxMesh.setNumMaps(numChannels);
   for (i=2; i<numChannels; i++)
   {
      // prepare each channel...
      S32 j;
      maxMesh.setNumMapVerts(i,numVerts);
      for (j=0; j<numVerts; j++)
         maxMesh.getMapVert(i,j) = tv;
      // set up tv patch faces
      maxMesh.setNumMapPatches(i,maxMesh.getNumPatches());
      for (j=0; j<maxMesh.getNumPatches(); j++)
      {
         Patch & patch = maxMesh.patches[j];
         TVPatch & tvPatch  = maxMesh.getMapPatch(i,j);
         tvPatch.tv[0] = patch.v[0];
         tvPatch.tv[1] = patch.v[1];
         tvPatch.tv[2] = patch.v[2];
         tvPatch.tv[3] = patch.v[3];
      }
   }

   for (S32 v=0; v<numVerts; v++)
   {
      for (i=0; i<skinData->GetNumAssignedBones(v); i++)
      {
         S32 bone = skinData->GetAssignedBone(v,i);
         F32 w = skinData->GetBoneWeight(v,i);
         S32 channel = 2 + (bone>>1);
         UVVert & tv = maxMesh.getMapVert(channel,v);
         if (bone&1)
            tv.y = w;
         else
            tv.x = w;
      }
   }
}

void SkinHelper::BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev )
{
   this->ip = ip;
}

void SkinHelper::EndEditParams( IObjParam *ip, ULONG flags,Animatable *next)
{
   this->ip = NULL;
}


//From ReferenceMaker 
RefResult SkinHelper::NotifyRefChanged(
      Interval changeInt, RefTargetHandle hTarget,
      PartID& partID,  RefMessage message) 
{
   //TODO: Add code to handle the various reference changed messages
   return REF_SUCCEED;
}

//From Object
BOOL SkinHelper::HasUVW() 
{ 
   //TODO: Return whether the object has UVW coordinates or not
   return TRUE; 
}

void SkinHelper::SetGenUVW(BOOL sw) 
{  
   if (sw==HasUVW()) return;
   //TODO: Set the plugin internal value to sw            
}

IOResult SkinHelper::Load(ILoad *iload)
{
   //TODO: Add code to allow plugin to load its data
   
   return IO_OK;
}

IOResult SkinHelper::Save(ISave *isave)
{
   //TODO: Add code to allow plugin to save its data
   
   return IO_OK;
}
