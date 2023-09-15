//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/interiorInstance.h"
#include "interior/lightUpdateGrouper.h"
#include "interior/interior.h"
#include "interior/interiorSubObject.h"
#include "interior/pathedInterior.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "core/bitStream.h"
#include "dgl/dgl.h"
#include "dgl/gBitmap.h"
#include "math/mathIO.h"
#include "dgl/materialList.h"
#include "editor/editor.h"
#include "interior/interiorResObjects.h"
#include "game/trigger.h"
#include "sim/simPath.h"
#include "interior/forceField.h"
#include "dgl/materialList.h"
#include "sceneGraph/lightManager.h"
#include "collision/convex.h"
#include "audio/audioDataBlock.h"
#include "sim/frameAllocator.h"

#include "sim/netConnection.h"
#include "platform/profiler.h"

//--------------------------------------------------------------------------
//-------------------------------------- Local classes, data, and functions
//
const U32 csgMaxZoneSize = 256;
bool sgScopeBoolArray[256];

class InteriorRenderImage : public SceneRenderImage
{
  public:
   InteriorRenderImage() : mDetailLevel(0) { }

   U32   mDetailLevel;
   U32   mBaseZone;
};

ConsoleFunctionGroupBegin(Interiors, "");

#if defined(TORQUE_DEBUG) || defined (INTERNAL_RELEASE)
ConsoleFunction( setInteriorRenderMode, void, 2, 2, "(int modeNum)")
{
   S32 mode = dAtoi(argv[1]);
   if (mode < 0 || mode > Interior::ShowDetailLevel)
      mode = 0;

   Interior::smRenderMode = mode;
}

ConsoleFunction( setInteriorFocusedDebug, void, 2, 2, "(bool enable)")
{
   if (dAtob(argv[1])) {
      Interior::smFocusedDebug = true;
   } else {
      Interior::smFocusedDebug = false;
   }
}

#endif

ConsoleFunction( isPointInside, bool, 2, 4, "(Point3F pos) or (float x, float y, float z)")
{
   static bool lastValue = false;

   if(!(argc == 2 || argc == 4))
   {
      Con::errorf(ConsoleLogEntry::General, "cIsPointInside: invalid parameters");
      return(lastValue);
   }

   Point3F pos;
   if(argc == 2)
      dSscanf(argv[1], "%f %f %f", &pos.x, &pos.y, &pos.z);
   else
   {
      pos.x = dAtof(argv[1]);
      pos.y = dAtof(argv[2]);
      pos.z = dAtof(argv[3]);
   }

   RayInfo collision;
   if(gClientContainer.castRay(pos, Point3F(pos.x, pos.y, pos.z - 2000.f), InteriorObjectType, &collision))
   {
      if(collision.face == -1)
         Con::errorf(ConsoleLogEntry::General, "cIsPointInside: failed to find hit face on interior");
      else
      {
         InteriorInstance * interior = dynamic_cast<InteriorInstance *>(collision.object);
         if(interior)
            lastValue = !interior->getDetailLevel(0)->isSurfaceOutsideVisible(collision.face);
         else
            Con::errorf(ConsoleLogEntry::General, "cIsPointInside: invalid interior on collision");
      }
   }

   return(lastValue);   
}


ConsoleFunctionGroupEnd(Interiors);

ConsoleMethod( InteriorInstance, setAlarmMode, void, 3, 3, "(string mode) Mode is 'On' or 'Off'")
{
   bool alarm;
   if (dStricmp(argv[2], "On") == 0)
      alarm = true;
   else
      alarm = false;

   if (object->isClientObject()) {
      Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
      return;
   }

   object->setAlarmMode(alarm);
}

ConsoleMethod( InteriorInstance, activateLight, void, 3, 3, "(string lightName)")
{
   if (object->isClientObject()) {
      Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
      return;
   }

   const char* pLightName = argv[2];
   object->activateLight(pLightName);
}

ConsoleMethod( InteriorInstance, deactivateLight, void, 3, 3, "(string lightName)")
{
   if (object->isClientObject()) {
      Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
      return;
   }

   const char* pLightName = argv[2];
   object->deactivateLight(pLightName);
}

ConsoleMethod( InteriorInstance, echoTriggerableLights, void, 2, 2, "List lights usable with activateLight()/deactivateLight().")
{
   if (object->isClientObject()) {
      Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
      return;
   }

   object->echoTriggerableLights();
}

ConsoleMethod( InteriorInstance, magicButton, void, 2, 2, "@deprecated unimplemented")
{
   if (object->isClientObject()) {
      Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
      return;
   }

   object->addChildren();
}

ConsoleMethod( InteriorInstance, setSkinBase, void, 3, 3, "(string basename)")
{
   if (object->isClientObject()) {
      Con::errorf(ConsoleLogEntry::General, "InteriorInstance: client objects may not receive console commands.  Ignored");
      return;
   }

   object->setSkinBase(argv[2]);
}

ConsoleMethod( InteriorInstance, getNumDetailLevels, S32, 2, 2, "")
{
   return(object->getNumDetailLevels());
}

ConsoleMethod( InteriorInstance, setDetailLevel, void, 3, 3, "(int level)")
{
   if(object->isServerObject())
   {
      NetConnection * toServer = NetConnection::getServerConnection();
      NetConnection * toClient = NetConnection::getLocalClientConnection();
      if(!toClient || !toServer)
         return;

      S32 index = toClient->getGhostIndex(object);
      if(index == -1)
         return;

      InteriorInstance * clientInstance = dynamic_cast<InteriorInstance*>(toServer->resolveGhost(index));
      if(clientInstance)
         clientInstance->setDetailLevel(dAtoi(argv[2]));
   }
   else
      object->setDetailLevel(dAtoi(argv[2]));
}
   
//--------------------------------------------------------------------------
//-------------------------------------- Static functions
//
IMPLEMENT_CO_NETOBJECT_V1(InteriorInstance);

//------------------------------------------------------------------------------
//-------------------------------------- InteriorInstance
//
bool InteriorInstance::smDontRestrictOutside = false;
bool InteriorInstance::smRenderDynamicLights = true;
U32  InteriorInstance::smLightUpdatePeriod = 66;  // 66 ms between updates
F32  InteriorInstance::smDetailModification = 1.0f;


InteriorInstance::InteriorInstance()
{
   mAlarmState        = false;

   mInteriorFileName = NULL;
   mTypeMask = InteriorObjectType | StaticObjectType | StaticRenderedObjectType;
   mNetFlags.set(Ghostable | ScopeAlways);

   mUpdateGrouper = NULL;

   mShowTerrainInside = false;

   mSkinBase = StringTable->insert("base");
   mAudioProfile = 0;
   mAudioEnvironment = 0;
   mForcedDetailLevel = -1;

   mConvexList = new Convex;
   mCRC = 0;
}

InteriorInstance::~InteriorInstance()
{
   U32 i;

   for (i = 0; i < mLightInfo.size(); i++)
      destructInPlace(&mLightInfo[i]);

   for (i = 0; i < mInteriorSubObjects.size(); i++) {
      // DMM: Fix for first-class objects...
      for (U32 j = 0; j < mInteriorSubObjects[i].size(); j++)
         delete (mInteriorSubObjects[i])[j];

      destructInPlace(&mInteriorSubObjects[i]);
   }
   delete mUpdateGrouper;

   delete mConvexList;
   mConvexList = NULL;

   for (i = 0; i < mVertexColorsNormal.size(); i++)
   {
      delete mVertexColorsNormal[i];
      mVertexColorsNormal[i] = NULL;
   }
   for (i = 0; i < mVertexColorsAlarm.size(); i++)
   {
      delete mVertexColorsAlarm[i];
      mVertexColorsAlarm[i] = NULL;
   }

   for (i = 0; i < mMaterialMaps.size(); i++)
   {
      delete mMaterialMaps[i];
      mMaterialMaps[i] = NULL;
   }
}


void InteriorInstance::init()
{
   // Does nothing for the moment
}


void InteriorInstance::destroy()
{
   // Also does nothing for the moment
}


//--------------------------------------------------------------------------
// Inspection
static AudioProfile * saveAudioProfile = 0;
static AudioEnvironment * saveAudioEnvironment = 0;
void InteriorInstance::inspectPreApply()
{  
   saveAudioProfile = mAudioProfile;
   saveAudioEnvironment = mAudioEnvironment;
}

void InteriorInstance::inspectPostApply()
{
   if((mAudioProfile != saveAudioProfile) || (mAudioEnvironment != saveAudioEnvironment))
      setMaskBits(AudioMask);

   // Update the Transform on Editor Apply.
   setMaskBits(TransformMask);
}

//--------------------------------------------------------------------------
//-------------------------------------- Console functionality
//
void InteriorInstance::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Media");	// MM: Added Group Header.
   addField("interiorFile",      TypeFilename,              Offset(mInteriorFileName, InteriorInstance));
   endGroup("Media");	// MM: Added Group Footer.

   addGroup("Audio");	// MM: Added Group Header.
   addField("audioProfile",      TypeAudioProfilePtr,       Offset(mAudioProfile, InteriorInstance));
   addField("audioEnvironment",  TypeAudioEnvironmentPtr,   Offset(mAudioEnvironment, InteriorInstance));
   endGroup("Audio");	// MM: Added Group Footer.

   addGroup("Misc");	// MM: Added Group Header.
   addField("showTerrainInside", TypeBool,                  Offset(mShowTerrainInside, InteriorInstance));
   endGroup("Misc");	// MM: Added Group Footer.

}

void InteriorInstance::consoleInit()
{
   //-------------------------------------- Class level variables
   Con::addVariable("pref::Interior::LightUpdatePeriod", TypeS32, &smLightUpdatePeriod);
   Con::addVariable("pref::Interior::ShowEnvironmentMaps", TypeBool, &Interior::smRenderEnvironmentMaps);
   Con::addVariable("pref::Interior::DynamicLights", TypeBool, &smRenderDynamicLights);
   Con::addVariable("pref::Interior::VertexLighting", TypeBool, &Interior::smUseVertexLighting);
   Con::addVariable("pref::Interior::TexturedFog", TypeBool, &Interior::smUseTexturedFog);
   Con::addVariable("pref::Interior::lockArrays", TypeBool, &Interior::smLockArrays);

   Con::addVariable("pref::Interior::detailAdjust", TypeF32, &InteriorInstance::smDetailModification);
   
   // DEBUG ONLY!!!
#ifdef TORQUE_DEBUG
   Con::addVariable("Interior::DontRestrictOutside", TypeBool, &smDontRestrictOutside);
#endif
}

//--------------------------------------------------------------------------
void InteriorInstance::renewOverlays()
{
   StringTableEntry baseName = dStricmp(mSkinBase, "base") == 0 ? "blnd" : mSkinBase;

   for (U32 i = 0; i < mMaterialMaps.size(); i++) {
      MaterialList* pMatList = mMaterialMaps[i];

      for (U32 j = 0; j < pMatList->mMaterialNames.size(); j++) {
         const char* pName     = pMatList->mMaterialNames[j];
         const U32 len = dStrlen(pName);
         if (len < 6)
            continue;

         const char* possible = pName + (len - 5);
         if (dStricmp(".blnd", possible) == 0) {
            char newName[256];
            AssertFatal(len < 200, "InteriorInstance::renewOverlays: Error, len exceeds allowed name length");

            dStrncpy(newName, pName, possible - pName);
            newName[possible - pName] = '\0';
            dStrcat(newName, ".");
            dStrcat(newName, baseName);

            TextureHandle test = TextureHandle(newName, MeshTexture, false);
            if (test.getGLName() != 0) {
               pMatList->mMaterials[j] = test;
            } else {
               pMatList->mMaterials[j] = TextureHandle(pName, MeshTexture, false);
            }
         }
      }
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::setSkinBase(const char* newBase)
{
   if (dStricmp(mSkinBase, newBase) == 0)
      return;

   mSkinBase = StringTable->insert(newBase);
   
   if (isServerObject())
      setMaskBits(SkinBaseMask);
   else
      renewOverlays();
}


//--------------------------------------------------------------------------
// from resManager.cc
extern U32 calculateCRC(void * buffer, S32 len, U32 crcVal );

bool InteriorInstance::onAdd()
{
   U32 i;

   // Load resource
   mInteriorRes = ResourceManager->load(mInteriorFileName, true);
   if (bool(mInteriorRes) == false) {
      Con::errorf(ConsoleLogEntry::General, "Unable to load interior: %s", mInteriorFileName);
      NetConnection::setLastError("Unable to load interior: %s", mInteriorFileName);
      return false;
   }
   if(isClientObject())
   {
      if(mCRC != mInteriorRes.getCRC())
      {
         NetConnection::setLastError("Local interior file '%s' does not match version on server.", mInteriorFileName);
         return false;
      }
      for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
         // ok, if the material list load failed...
         // if this is a local connection, we'll assume that's ok
         // and just have white textures...
         // otherwise we want to return false.
         Interior* pInterior = mInteriorRes->getDetailLevel(i);
         if(!pInterior->prepForRendering(mInteriorRes.getFilePath()) )
               return false;
      }
   }
   else
      mCRC = mInteriorRes.getCRC();

   if(!Parent::onAdd())
      return false;

   mVertexColorsNormal.setSize(mInteriorRes->getNumDetailLevels());
   mVertexColorsAlarm.setSize(mInteriorRes->getNumDetailLevels());
   for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++)
   {
      mVertexColorsNormal[i] = new Vector<ColorI>;
      mVertexColorsAlarm[i]  = new Vector<ColorI>;
      VECTOR_SET_ASSOCIATION((*mVertexColorsNormal[i]));
      VECTOR_SET_ASSOCIATION((*mVertexColorsAlarm[i]));
   }

   // Ok, everything's groovy!  Let's cache our hashed filename for renderimage sorting...
   mInteriorFileHash = _StringTable::hashString(mInteriorFileName);
   
   // Setup bounding information
   mObjBox = mInteriorRes->getDetailLevel(0)->getBoundingBox();
   resetWorldBox();
   setRenderTransform(mObjToWorld);
   
   // Setup mLightInfo structure
   //
   mLightInfo.setSize(mInteriorRes->getNumDetailLevels());
   for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
      Interior* pInterior = mInteriorRes->getDetailLevel(i);
      constructInPlace(&mLightInfo[i]);
      LightInfo& rInfo = mLightInfo[i];

      rInfo.mSurfaceInvalid.setSize(pInterior->mSurfaces.size());
      rInfo.mSurfaceInvalid.clear();

      rInfo.mStateDataInfo.setSize(pInterior->mNumLightStateEntries);
      dMemset(rInfo.mStateDataInfo.address(), 0x00,
              sizeof(LightInfo::StateDataInfo) * rInfo.mStateDataInfo.size());

      rInfo.mLights.setSize(pInterior->mAnimatedLights.size());
      for (U32 j = 0; j < rInfo.mLights.size(); j++) {
         LightInfo::Light& rLight = rInfo.mLights[j];
         rLight.curState = 0;
         rLight.curTime  = 0;

         Interior::LightState& rState = pInterior->mLightStates[pInterior->mAnimatedLights[j].stateIndex];
         rLight.curColor.set(rState.red, rState.green, rState.blue);
         rLight.active   = false;
         rLight.alarm    = (pInterior->mAnimatedLights[j].flags & Interior::AlarmLight) != 0;

         installLight(i, j);
      }
   }

   // Setup lightgrouper
   mUpdateGrouper = new LightUpdateGrouper(LightUpdateBitStart, LightUpdateBitEnd);
   for (U32 d = 0; d < mInteriorRes->getNumDetailLevels(); d++) {
      Interior* pInterior = mInteriorRes->getDetailLevel(d);
      
      for (U32 j = 0; j < pInterior->mNumTriggerableLights; j++)
         mUpdateGrouper->addKey(makeUpdateKey(d, j));
   }

   if (isClientObject()) {
      // create all the subObjects
      mInteriorSubObjects.setSize(mInteriorRes->getNumDetailLevels());
      for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
         constructInPlace(&mInteriorSubObjects[i]);

         Interior* pInterior = mInteriorRes->getDetailLevel(i);
         for (U32 j = 0; j < pInterior->mSubObjects.size(); j++)
            mInteriorSubObjects[i].push_back(pInterior->mSubObjects[j]->clone(this));
      } 
   } else {
      // creates all subobjects
      mInteriorSubObjects.setSize(mInteriorRes->getNumDetailLevels());
      for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
         constructInPlace(&mInteriorSubObjects[i]);

         Interior* pInterior = mInteriorRes->getDetailLevel(i);

         for (U32 j = 0; j < pInterior->mSubObjects.size(); j++)
            mInteriorSubObjects[i].push_back(pInterior->mSubObjects[j]->clone(this));
      } 
   }

   // Do any handle loading, etc. required.

   if (isClientObject()) {
      setLightUpdatedTime(Platform::getVirtualMilliseconds());

      for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++) {
         Interior* pInterior = mInteriorRes->getDetailLevel(i);
         // Force the lightmap manager to download textures if we're
         // running the mission editor.  Normally they are only
         // downloaded after the whole scene is lit.
         gInteriorLMManager.addInstance(pInterior->getLMHandle(), mLMHandle, this);
         if (gEditingMission)  {
            gInteriorLMManager.useBaseTextures(pInterior->getLMHandle(), mLMHandle);
            gInteriorLMManager.downloadGLTextures(pInterior->getLMHandle());
         }

         // Install material list
         mMaterialMaps.push_back(new MaterialList(pInterior->mMaterialList));

         // A client interior starts up it's ambient animations on add.  Ambients
         //  are just past the triggerables.
         for (U32 j = pInterior->mNumTriggerableLights; j < pInterior->mAnimatedLights.size(); j++)
            activateLight(i, j);
      }

      renewOverlays();
   } else {
      
   }

   addToScene();
   return true;
}


void InteriorInstance::onRemove()
{
   mConvexList->nukeList();

   //
   if(isClientObject())
   {
      if(bool(mInteriorRes) && mLMHandle != 0xFFFFFFFF)
      {
         for(U32 i = 0; i < mInteriorRes->getNumDetailLevels(); i++)
         {
            Interior * pInterior = mInteriorRes->getDetailLevel(i);
            if (pInterior->getLMHandle() != 0xFFFFFFFF)
               gInteriorLMManager.removeInstance(pInterior->getLMHandle(), mLMHandle);
         }
      }
   }

   removeFromScene();
   Parent::onRemove();
}


//--------------------------------------------------------------------------
bool InteriorInstance::onSceneAdd(SceneGraph* pGraph)
{
   AssertFatal(mInteriorRes, "Error, should not have been added to the scene if there's no interior!");

   if (Parent::onSceneAdd(pGraph) == false)
      return false;

   if (mInteriorRes->getDetailLevel(0)->mZones.size() > 1) {
      AssertWarn(getNumCurrZones() == 1, "There should be one and only one zone for an interior that manages zones");
      mSceneManager->registerZones(this, (mInteriorRes->getDetailLevel(0)->mZones.size() - 1));
   }

   return true;
}


//--------------------------------------------------------------------------
void InteriorInstance::onSceneRemove()
{
   AssertFatal(mInteriorRes, "Error, should not have been added to the scene if there's no interior!");
   
   if (isManagingZones())
      mSceneManager->unregisterZones(this);

   Parent::onSceneRemove();
}


//--------------------------------------------------------------------------
bool InteriorInstance::getOverlappingZones(SceneObject* obj,
                                           U32*         zones,
                                           U32*         numZones)
{
   MatrixF xForm(true);
   Point3F invScale(1.0f / getScale().x,
                    1.0f / getScale().y,
                    1.0f / getScale().z);
   xForm.scale(invScale);
   xForm.mul(getWorldTransform());
   xForm.mul(obj->getTransform());
   xForm.scale(obj->getScale());

   U32 waterMark = FrameAllocator::getWaterMark();

   U16* zoneVector = (U16*)FrameAllocator::alloc(mInteriorRes->getDetailLevel(0)->mZones.size() * sizeof(U16));
   U32 numRetZones = 0;

   bool outsideToo = mInteriorRes->getDetailLevel(0)->scanZones(obj->getObjBox(),
                                                                xForm,
                                                                zoneVector,
                                                                &numRetZones);
   if (numRetZones > SceneObject::MaxObjectZones) {
      Con::warnf(ConsoleLogEntry::General, "Too many zones returned for query on %s.  Returning first %d",
                 mInteriorFileName, SceneObject::MaxObjectZones);
   }

   for (U32 i = 0; i < getMin(numRetZones, U32(SceneObject::MaxObjectZones)); i++)
      zones[i] = zoneVector[i] + mZoneRangeStart - 1;
   *numZones = numRetZones;

   FrameAllocator::setWaterMark(waterMark);

   return outsideToo;
}


//--------------------------------------------------------------------------
U32 InteriorInstance::getPointZone(const Point3F& p)
{
   AssertFatal(mInteriorRes, "Error, no interior!");

   Point3F osPoint = p;
   mWorldToObj.mulP(osPoint);
   osPoint.convolveInverse(mObjScale);

   S32 zone = mInteriorRes->getDetailLevel(0)->getZoneForPoint(osPoint);
   
   // If we're in solid (-1) or outside, we need to return 0
   if (zone == -1 || zone == 0)
      return 0;

   return (zone-1) + mZoneRangeStart;
}

// does a hack check to determine how much a point is 'inside'.. should have
// portals prebuilt with the transfer energy to each other portal in the zone
// from the neighboring zone.. these values can be used to determine the factor 
// from within an individual zone.. also, each zone could be marked with
// average material property for eax environment audio
// ~0: outside -> 1: inside
bool InteriorInstance::getPointInsideScale(const Point3F & pos, F32 * pScale)
{
   AssertFatal(mInteriorRes, "InteriorInstance::getPointInsideScale: no interior");

   Interior * interior = mInteriorRes->getDetailLevel(0);

   Point3F p = pos;
   mWorldToObj.mulP(p);
   p.convolveInverse(mObjScale);

   U32 zoneIndex = interior->getZoneForPoint(p);
   if(zoneIndex == -1)  // solid?
   { 
      *pScale = 1.f;
      return(true);
   }
   else if(zoneIndex == 0) // outside?
   {
      *pScale = 0.f;
      return(true);
   }
   
   U32 waterMark = FrameAllocator::getWaterMark();
   const Interior::Portal** portals = (const Interior::Portal**)FrameAllocator::alloc(256 * sizeof(const Interior::Portal*));
   U32 numPortals = 0;

   Interior::Zone & zone = interior->mZones[zoneIndex];
   
   U32 i;
   for(i = 0; i < zone.portalCount; i++)
   {
      const Interior::Portal & portal = interior->mPortals[interior->mZonePortalList[zone.portalStart + i]];
      if(portal.zoneBack == 0 || portal.zoneFront == 0) {
         AssertFatal(numPortals < 256, "Error, overflow in temporary portal buffer!");
         portals[numPortals++] = &portal;
      }
   }   

   // inside?
   if(numPortals == 0)
   {
      *pScale = 1.f;

      FrameAllocator::setWaterMark(waterMark);
      return(true);
   }

   Point3F* portalCenters = (Point3F*)FrameAllocator::alloc(numPortals * sizeof(Point3F));
   U32 numPortalCenters = 0;
   
   // scale using the distances to the portals in this zone...
   for(i = 0; i < numPortals; i++)
   {
      const Interior::Portal * portal = portals[i];
      if(!portal->triFanCount)
         continue;

      Point3F center(0, 0, 0);
      for(U32 j = 0; j < portal->triFanCount; j++)
      {
         const Interior::TriFan & fan = interior->mWindingIndices[portal->triFanStart + j];
         U32 numPoints = fan.windingCount;
         
         if(!numPoints)
            continue;
               
         for(U32 k = 0; k < numPoints; k++)
         {
            const Point3F & a = interior->mPoints[interior->mWindings[fan.windingStart + k]].point;
            center += a;
         }

         center /= numPoints;
         portalCenters[numPortalCenters++] = center;
      }
   }

   // 'magic' check here...
   F32 magic = Con::getFloatVariable("Interior::insideDistanceFalloff", 10.f);
   
   F32 val = 0.f;
   for(i = 0; i < numPortalCenters; i++)
      val += 1.f - mClampF(Point3F(portalCenters[i] - p).len() / magic, 0.f, 1.f);

   *pScale = 1.f - mClampF(val, 0.f, 1.f);

   FrameAllocator::setWaterMark(waterMark);
   return(true);
}

//--------------------------------------------------------------------------
ColorF gInteriorFogColor(1, 1, 1);

void InteriorInstance::renderObject(SceneState* state, SceneRenderImage* sceneImage)
{
   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

   if(gEditingMission && isHidden())
      return;
   PROFILE_START(InteriorRenderObject);

   PROFILE_START(IRO_GetZones);
   U32 storedWaterMark = FrameAllocator::getWaterMark();
   dglSetRenderPrimType(2);

   InteriorRenderImage* interiorImage = static_cast<InteriorRenderImage*>(sceneImage);

   Point3F osPoint = state->getCameraPosition();
   mRenderWorldToObj.mulP(osPoint);
   osPoint.convolveInverse(mObjScale);

   // Get the object space y vector
   Point3F osCamVector;
   state->mModelview.getColumn(1, &osCamVector);
   mRenderWorldToObj.mulV(osCamVector);
   osCamVector.convolve(getScale());
   osCamVector.normalize();
   
   Point3F osZVec(0, 0, 1);
   mRenderWorldToObj.mulV(osZVec);
   osZVec.convolve(getScale());

   // First, we want to test the planes and setup the fog...
   U32 zoneOffset = mZoneRangeStart != 0xFFFFFFFF ? mZoneRangeStart : 0;

   Interior* pInterior = mInteriorRes->getDetailLevel(interiorImage->mDetailLevel);
   
   U32 baseZone = 0xFFFFFFFF;
   if (getNumCurrZones() == 1)
   {
      baseZone = getCurrZone(0);
   }
   else
   {
      for (U32 i = 0; i < getNumCurrZones(); i++) {
         if (state->getZoneState(getCurrZone(i)).render == true) {
            if (baseZone == 0xFFFFFFFF) {
               baseZone = getCurrZone(i);
               break;
            }
         }
      }
      if (baseZone == 0xFFFFFFFF)
         baseZone = getCurrZone(0);
   }
   PROFILE_END();
   PROFILE_START(IRO_ComputeActivePolys);
   
   Point3F worldOrigin;
   getRenderTransform().getColumn(3, &worldOrigin);
   
   ZoneVisDeterminer zoneVis;
   if (interiorImage->mDetailLevel == 0) {
      zoneVis.runFromState(state, zoneOffset, baseZone);
      pInterior->setupActivePolyList(zoneVis, state, osPoint, osCamVector, osZVec, worldOrigin.z, getScale());
   } else {
      // Something else...
      pInterior->prepTempRender(state,
                                getCurrZone(0),
                                0,
                                mRenderObjToWorld, mObjScale,
                                state->mFlipCull);
      zoneVis.runFromRects(state, zoneOffset, baseZone);
      pInterior->setupActivePolyList(zoneVis, state, osPoint, osCamVector, osZVec, worldOrigin.z, getScale());
   }
   
   PROFILE_END();
   PROFILE_START(IRO_UpdateAnimatedLights);
   // Update the animated lights...
   if (!Interior::smUseVertexLighting)
   {
      LightInfo& rLightInfo = mLightInfo[interiorImage->mDetailLevel];
      downloadLightmaps(state, pInterior, rLightInfo);
   }
   PROFILE_END();
   PROFILE_START(IRO_RenderSolids);

   // Set up the model view and the global render state...
   RectI viewport;
   dglGetViewport(&viewport);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   dglMultMatrix(&mRenderObjToWorld);
   glScalef(mObjScale.x, mObjScale.y, mObjScale.z);

   glEnable(GL_BLEND);
   glDisable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);

   // We need to decide if we're going to use the low-res textures
   Point3F camPoint = state->getCameraPosition();
   Point3F closestPoint = getRenderWorldBox().getClosestPoint(camPoint);
   F32 dist = (camPoint - closestPoint).len();
   if (dist != 0.0)
   {
      F32 length = dglProjectRadius(dist, 1.0f / pInterior->mAveTexGenLength);
      if (length < (1.0 / 16.0))
      {
         TextureManager::setSmallTexturesActive(true);
      }
   }

   
   pInterior->setupFog(state);
   pInterior->setOSCamPosition(osPoint);
   if (interiorImage->mBaseZone != 0)
      state->setupZoneProjection(interiorImage->mBaseZone + mZoneRangeStart - 1);
   else
      state->setupObjectProjection(this);

   gInteriorFogColor = state->getFogColor();

   // make sure to build vertex color information in case interiors were
   //  added after the relight scene
   if(&((*mVertexColorsNormal[interiorImage->mDetailLevel])[0]) == NULL)
	   rebuildVertexColors();

   pInterior->render(mAlarmState, mMaterialMaps[interiorImage->mDetailLevel], mLMHandle,
                     mVertexColorsNormal[interiorImage->mDetailLevel],
                     mVertexColorsAlarm[interiorImage->mDetailLevel]);
   pInterior->clearFog();

   PROFILE_END();
   PROFILE_START(IRO_RenderDynamicLights);

   if (smRenderDynamicLights == true) {
      S32 numLights = gClientSceneGraph->getLightManager()->getNumLights();
      if (numLights > 0)
      {
         U32 lightWaterMark = FrameAllocator::getWaterMark();
         ::LightInfo** lightArray = (::LightInfo**)FrameAllocator::alloc(sizeof(::LightInfo*) * numLights);
         gClientSceneGraph->getLightManager()->getLights(lightArray);
         for (U32 i = 0; i < numLights; i++) {
            if (lightArray[i]->mType != ::LightInfo::Point)
               continue;

            Point3F lightPoint = lightArray[i]->mPos;
            mRenderWorldToObj.mulP(lightPoint);
            lightPoint.convolveInverse(mObjScale);

            Box3F box;
            box.min = lightPoint;
            box.max = lightPoint;
            box.min -= Point3F(lightArray[i]->mRadius, lightArray[i]->mRadius, lightArray[i]->mRadius);
            box.max += Point3F(lightArray[i]->mRadius, lightArray[i]->mRadius, lightArray[i]->mRadius);

            // TODO: Account for scale...
            if (mObjBox.isOverlapped(box) == false)
               continue;

            // The number of light surfaces cannot exceed the total number of non-null surfaces in
            //  interior...
            U32  subWaterMark = FrameAllocator::getWaterMark();
            U32* lightSurfaces    = (U32*)FrameAllocator::alloc(pInterior->mSurfaces.size() * sizeof(U32));
            U32  numLightSurfaces = 0;
            if (pInterior->buildLightPolyList(lightSurfaces, &numLightSurfaces,
                                              box, mRenderWorldToObj, getScale()) == false) {
               FrameAllocator::setWaterMark(subWaterMark);
               continue;
            }

            pInterior->renderLights(lightArray[i], mRenderWorldToObj, getScale(), lightSurfaces, numLightSurfaces);
            FrameAllocator::setWaterMark(subWaterMark);
         }
         
         FrameAllocator::setWaterMark(lightWaterMark);
      }
   }
   PROFILE_END();

   glDisable(GL_BLEND);
   if (dglDoesSupportARBMultitexture()) {
      glActiveTextureARB(GL_TEXTURE1_ARB);
      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   } else {
      glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   }

   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   dglSetViewport(viewport);

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");

   FrameAllocator::setWaterMark(storedWaterMark);
   dglSetRenderPrimType(0);
   
   // Reset the small textures...
   TextureManager::setSmallTexturesActive(false);

   PROFILE_END();
}


//--------------------------------------------------------------------------
bool InteriorInstance::scopeObject(const Point3F&        rootPosition,
                                   const F32             /*rootDistance*/,
                                   bool*                 zoneScopeState)
{
   AssertFatal(isManagingZones(), "Error, should be a zone manager if we are called on to scope the scene!");
   if (bool(mInteriorRes) == false)
      return false;

   Interior* pInterior = getDetailLevel(0);
   AssertFatal(pInterior->mZones.size() <= csgMaxZoneSize, "Error, too many zones!  Increase max");
   bool* pInteriorScopingState = sgScopeBoolArray;
   dMemset(pInteriorScopingState, 0, sizeof(bool) * pInterior->mZones.size());

   // First, let's transform the point into the interior's space
   Point3F interiorRoot = rootPosition;
   getWorldTransform().mulP(interiorRoot);
   interiorRoot.convolveInverse(getScale());

   S32 realStartZone = getPointZone(rootPosition);
   if (realStartZone != 0)
      realStartZone = realStartZone - mZoneRangeStart + 1;

   bool continueOut = pInterior->scopeZones(realStartZone,
                                            interiorRoot,
                                            pInteriorScopingState);

   // Copy pInteriorScopingState to zoneScopeState
   for (S32 i = 1; i < pInterior->mZones.size(); i++)
      zoneScopeState[i + mZoneRangeStart - 1] = pInteriorScopingState[i];

   return continueOut;
}


//--------------------------------------------------------------------------
U32 InteriorInstance::calcDetailLevel(SceneState* state, const Point3F& wsPoint)
{
   AssertFatal(mInteriorRes, "Error, should not try to calculate the deatil level without a resource to work with!");
   AssertFatal(getNumCurrZones() > 0, "Error, must belong to a zone for this to work");

   if (smDetailModification < 0.3)
      smDetailModification = 0.3;
   if (smDetailModification > 1.0)
      smDetailModification = 1.0;
   
   // Early out for simple interiors
   if (mInteriorRes->getNumDetailLevels() == 1)
      return 0;

   if((mForcedDetailLevel >= 0) && (mForcedDetailLevel < mInteriorRes->getNumDetailLevels()))
      return(mForcedDetailLevel);

   Point3F osPoint = wsPoint;
   mRenderWorldToObj.mulP(osPoint);
   osPoint.convolveInverse(mObjScale);

   // First, see if the point is in the object space bounding box of the highest detail
   //  If it is, then the detail level is zero.
   if (mObjBox.isContained(osPoint))
      return 0;

   // Otherwise, we're going to have to do some ugly trickery to get the projection.
   //  I've stolen the worldToScreenScale from dglMatrix, we'll have to calculate the
   //  projection of the bounding sphere of the lowest detail level.
   //  worldToScreenScale = (near * view.extent.x) / (right - left)
   RectI viewport;
   F64   frustum[4] = { 1e10, -1e10, 1e10, -1e10 };

   bool init = false;
   SceneObjectRef* pWalk = mZoneRefHead;
   AssertFatal(pWalk != NULL, "Error, object must exist in at least one zone to call this!");
   while (pWalk) {
      const SceneState::ZoneState& rState = state->getZoneState(pWalk->zone);
      if (rState.render == true) {
         // frustum
         if (rState.frustum[0] < frustum[0]) frustum[0] = rState.frustum[0];
         if (rState.frustum[1] > frustum[1]) frustum[1] = rState.frustum[1];
         if (rState.frustum[2] < frustum[2]) frustum[2] = rState.frustum[2];
         if (rState.frustum[3] > frustum[3]) frustum[3] = rState.frustum[3];

         // viewport
         if (init == false)
            viewport = rState.viewport;
         else
            viewport.unionRects(rState.viewport); 

         init = true;
      }
      pWalk = pWalk->nextInObj;
   }
   AssertFatal(init, "Error, at least one zone must be rendered here!");

   F32 worldToScreenScale   = (state->getNearPlane() * viewport.extent.x) / (frustum[1] - frustum[0]);
   const SphereF& lowSphere = mInteriorRes->getDetailLevel(mInteriorRes->getNumDetailLevels() - 1)->mBoundingSphere;
   F32 dist                 = (lowSphere.center - osPoint).len();
   F32 projRadius           = (lowSphere.radius / dist) * worldToScreenScale;

   // Scale the projRadius based on the objects maximum scale axis
   projRadius *= getMax(mFabs(mObjScale.x), getMax(mFabs(mObjScale.y), mFabs(mObjScale.z)));

   // Multiply based on detail preference...
   projRadius *= smDetailModification;
   
   // Ok, now we have the projected radius, we need to search through the interiors to
   //  find the largest interior that will support this projection.
   U32 final = mInteriorRes->getNumDetailLevels() - 1;
   for (U32 i = 0; i< mInteriorRes->getNumDetailLevels() - 1; i++) {
      Interior* pDetail = mInteriorRes->getDetailLevel(i);

      if (pDetail->mMinPixels < projRadius) {
         final = i;
         break;
      }
   }

   // Ok, that's it.
   return final;
}


//--------------------------------------------------------------------------
bool InteriorInstance::prepRenderImage(SceneState* state,   const U32 stateKey,
                                       const U32 startZone, const bool modifyBaseState)
{
   if (isLastState(state, stateKey))
      return false;
   PROFILE_START(InteriorPrepRenderImage);

   setLastState(state, stateKey);

   U32 realStartZone;
   if (startZone != 0xFFFFFFFF) {
      AssertFatal(startZone != 0, "Hm.  This really shouldn't happen.  Should only get inside zones here");
      AssertFatal(isManagingZones(), "Must be managing zones if we're here...");

      realStartZone = startZone - mZoneRangeStart + 1;
   } else {
      realStartZone = getPointZone(state->getCameraPosition());
      if (realStartZone != 0)
         realStartZone = realStartZone - mZoneRangeStart + 1;
   }

   bool render = true;
   if (modifyBaseState == false) {
      // Regular query.  We only return a render zone if our parent zone is rendered.
      //  Otherwise, we always render
      if (state->isObjectRendered(this) == false)
      {
         PROFILE_END();
         return false;
      }
   } else {
      if (mShowTerrainInside == true)
         state->enableTerrainOverride();
   }

   U32 detailLevel = 0;
   if (startZone == 0xFFFFFFFF)
      detailLevel = calcDetailLevel(state, state->getCameraPosition());

   if (!Interior::smUseVertexLighting)
   {
      // Since we're rendering: update the lights and the alarm state
      U32 msSinceLightsUpdated = Platform::getVirtualMilliseconds() - getLightUpdatedTime();
      if (msSinceLightsUpdated > smLightUpdatePeriod) {
         setLightUpdatedTime(Platform::getVirtualMilliseconds());
         updateAllLights(msSinceLightsUpdated);
      }
   }
   
   U32 baseZoneForPrep = getCurrZone(0);
   bool multipleZones = false;
   if (getNumCurrZones() > 1) {
      U32 numRenderedZones = 0;
      baseZoneForPrep = 0xFFFFFFFF;
      for (U32 i = 0; i < getNumCurrZones(); i++) {
         if (state->getZoneState(getCurrZone(i)).render == true) {
            numRenderedZones++;
            if (baseZoneForPrep == 0xFFFFFFFF)
               baseZoneForPrep = getCurrZone(i);
         }
      }

      if (numRenderedZones > 1)
         multipleZones = true;
   }
   
   bool continueOut = mInteriorRes->getDetailLevel(0)->prepRender(state,
                                                                  baseZoneForPrep,
                                                                  realStartZone, mZoneRangeStart,
                                                                  mRenderObjToWorld, mObjScale,
                                                                  modifyBaseState & !smDontRestrictOutside,
                                                                  smDontRestrictOutside | multipleZones,
                                                                  state->mFlipCull);
   if (smDontRestrictOutside)
      continueOut = true;
   
   InteriorRenderImage* image = new InteriorRenderImage;
   image->obj = this;
   image->mDetailLevel = detailLevel;
   image->mBaseZone    = realStartZone;
   image->textureSortKey = mInteriorFileHash;
   state->insertRenderImage(image);

   // Add renderimages for any sub-objects, first the independant subs, then the dependants
   //  for this detail...
   Point3F osPoint = state->getCameraPosition();
   mRenderWorldToObj.mulP(osPoint);
   osPoint.convolveInverse(mObjScale);

   U32 i;
   for (i = 0; i < mInteriorSubObjects[0].size(); i++) {
      InteriorSubObject* iso = (mInteriorSubObjects[0])[i];

      if (iso->renderDetailDependant() == false) {
         // We want to check the zone that this object is in.  If we are traversing upwards
         //  though, we have no information about our parent zone, and must return the
         //  render image regardless.
         if (modifyBaseState && iso->getZone() == 0) {
            // Must return
         } else {
            U32 realZone;
            if (iso->getZone() == 0) {
               realZone = getCurrZone(0);
            } else {
               realZone = iso->getZone() + mZoneRangeStart - 1;
            }

            if (state->getZoneState(realZone).render == false) {
               // Nuke it...
               continue;
            }
         }

         SubObjectRenderImage* sri = iso->getRenderImage(state, osPoint);
         if (sri) {
            sri->mDetailLevel = 0;
            state->insertRenderImage(sri);
         }
      }
   }
   for (i = 0; i < mInteriorSubObjects[detailLevel].size(); i++) {
      InteriorSubObject* iso = (mInteriorSubObjects[detailLevel])[i];

      if (iso->renderDetailDependant() == true) {
         // We want to check the zone that this object is in.  If we are traversing upwards
         //  though, we have no information about our parent zone, and must return the
         //  render image regardless.
         if (modifyBaseState && iso->getZone() == 0) {
            // Must return
         } else {
            U32 realZone;
            if (iso->getZone() == 0) {
               realZone = getCurrZone(0);
            } else {
               realZone = iso->getZone() + mZoneRangeStart - 1;
            }

            if (state->getZoneState(realZone).render == false) {
               // Nuke it...
               continue;
            }
         }

         SubObjectRenderImage* sri = iso->getRenderImage(state, osPoint);
         if (sri) {
            sri->mDetailLevel = detailLevel;
            state->insertRenderImage(sri);
         }
      }
   }

   PROFILE_END();
   return continueOut;
}


//--------------------------------------------------------------------------
bool InteriorInstance::castRay(const Point3F& s, const Point3F& e, RayInfo* info)
{
   info->object = this;

   if (mInteriorRes->getDetailLevel(0)->castRay(s, e, info)) {
      PlaneF fakePlane;
      fakePlane.x = info->normal.x;
      fakePlane.y = info->normal.y;
      fakePlane.z = info->normal.z;
      fakePlane.d = 0;

      PlaneF result;
      mTransformPlane(getTransform(), getScale(), fakePlane, &result);
      info->normal = result;
      return true;
   }
   return false;
}


//------------------------------------------------------------------------------
void InteriorInstance::setTransform(const MatrixF & mat)
{
   Parent::setTransform(mat);

   // Since the interior is a static object, it's render transform changes 1 to 1
   //  with it's collision transform
   setRenderTransform(mat);
   
   // Notify subobjects that care about the transform change...
   //
   if (bool(mInteriorRes)) {
      for (U32 i = 0; i < mInteriorSubObjects.size(); i++) {
         for (U32 j = 0; j < mInteriorSubObjects[i].size(); j++)
            (mInteriorSubObjects[i])[j]->noteTransformChange();
      }
   }

   if (isServerObject())
      setMaskBits(TransformMask);
}


//------------------------------------------------------------------------------
bool InteriorInstance::buildPolyList(AbstractPolyList* list, const Box3F& wsBox, const SphereF&)
{
   if (bool(mInteriorRes) == false)
      return false;

   // Setup collision state data
   list->setTransform(&getTransform(), getScale());
   list->setObject(this);

   return mInteriorRes->getDetailLevel(0)->buildPolyList(list, wsBox, mWorldToObj, getScale());
}



void InteriorInstance::buildConvex(const Box3F& box, Convex* convex)
{
   if (bool(mInteriorRes) == false)
      return;

   mConvexList->collectGarbage();

   Box3F realBox = box;
   mWorldToObj.mul(realBox);
   realBox.min.convolveInverse(mObjScale);
   realBox.max.convolveInverse(mObjScale);

   if (realBox.isOverlapped(getObjBox()) == false)
      return;

   U32 waterMark = FrameAllocator::getWaterMark();

   if ((convex->getObject()->getType() & VehicleObjectType) &&
       mInteriorRes->getDetailLevel(0)->mVehicleConvexHulls.size() > 0)
   {
      // Can never have more hulls than there are hulls in the interior...
      U16* hulls = (U16*)FrameAllocator::alloc(mInteriorRes->getDetailLevel(0)->mVehicleConvexHulls.size() * sizeof(U16));
      U32 numHulls = 0;

      Interior* pInterior = mInteriorRes->getDetailLevel(0);
      if (pInterior->getIntersectingVehicleHulls(realBox, hulls, &numHulls) == false) {
         FrameAllocator::setWaterMark(waterMark);
         return;
      }

      for (U32 i = 0; i < numHulls; i++) {
         // See if this hull exists in the working set already...
         Convex* cc = 0;
         CollisionWorkingList& wl = convex->getWorkingList();
         for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
            if (itr->mConvex->getType() == InteriorConvexType &&
                (static_cast<InteriorConvex*>(itr->mConvex)->getObject() == this &&
                 static_cast<InteriorConvex*>(itr->mConvex)->hullId    == -S32(hulls[i] + 1))) {
               cc = itr->mConvex;
               break;
            }
         }
         if (cc)
            continue;

         // Create a new convex.
         InteriorConvex* cp = new InteriorConvex;
         mConvexList->registerObject(cp);
         convex->addToWorkingList(cp);
         cp->mObject   = this;
         cp->pInterior = pInterior;
         cp->hullId    = -S32(hulls[i] + 1);
         cp->box.min.x = pInterior->mVehicleConvexHulls[hulls[i]].minX;
         cp->box.min.y = pInterior->mVehicleConvexHulls[hulls[i]].minY;
         cp->box.min.z = pInterior->mVehicleConvexHulls[hulls[i]].minZ;
         cp->box.max.x = pInterior->mVehicleConvexHulls[hulls[i]].maxX;
         cp->box.max.y = pInterior->mVehicleConvexHulls[hulls[i]].maxY;
         cp->box.max.z = pInterior->mVehicleConvexHulls[hulls[i]].maxZ;
      }
   }
   else
   {
      // Can never have more hulls than there are hulls in the interior...
      U16* hulls = (U16*)FrameAllocator::alloc(mInteriorRes->getDetailLevel(0)->mConvexHulls.size() * sizeof(U16));
      U32 numHulls = 0;

      Interior* pInterior = mInteriorRes->getDetailLevel(0);
      if (pInterior->getIntersectingHulls(realBox, hulls, &numHulls) == false) {
         FrameAllocator::setWaterMark(waterMark);
         return;
      }

      for (U32 i = 0; i < numHulls; i++) {
         // See if this hull exists in the working set already...
         Convex* cc = 0;
         CollisionWorkingList& wl = convex->getWorkingList();
         for (CollisionWorkingList* itr = wl.wLink.mNext; itr != &wl; itr = itr->wLink.mNext) {
            if (itr->mConvex->getType() == InteriorConvexType &&
                (static_cast<InteriorConvex*>(itr->mConvex)->getObject() == this &&
                 static_cast<InteriorConvex*>(itr->mConvex)->hullId    == hulls[i])) {
               cc = itr->mConvex;
               break;
            }
         }
         if (cc)
            continue;

         // Create a new convex.
         InteriorConvex* cp = new InteriorConvex;
         mConvexList->registerObject(cp);
         convex->addToWorkingList(cp);
         cp->mObject   = this;
         cp->pInterior = pInterior;
         cp->hullId    = hulls[i];
         cp->box.min.x = pInterior->mConvexHulls[hulls[i]].minX;
         cp->box.min.y = pInterior->mConvexHulls[hulls[i]].minY;
         cp->box.min.z = pInterior->mConvexHulls[hulls[i]].minZ;
         cp->box.max.x = pInterior->mConvexHulls[hulls[i]].maxX;
         cp->box.max.y = pInterior->mConvexHulls[hulls[i]].maxY;
         cp->box.max.z = pInterior->mConvexHulls[hulls[i]].maxZ;
      }
   }
   FrameAllocator::setWaterMark(waterMark);
}


//------------------------------------------------------------------------------
U32 InteriorInstance::packUpdate(NetConnection* c, U32 mask, BitStream* stream)
{
   U32 retMask = Parent::packUpdate(c, mask, stream);

   if (stream->writeFlag((mask & InitMask) != 0)) {
      // Initial update, write the whole kit and kaboodle
      stream->write(mCRC);

      stream->writeString(mInteriorFileName);
      stream->writeFlag(mShowTerrainInside);

      // Write the transform (do _not_ use writeAffineTransform.  Since this is a static
      //  object, the transform must be RIGHT THE *&)*$&^ ON or it will goof up the
      //  syncronization between the client and the server.
      mathWrite(*stream, mObjToWorld);
      mathWrite(*stream, mObjScale);

      // Write the alarm state
      stream->writeFlag(mAlarmState);

      // Write the skinbase
      stream->writeString(mSkinBase);

      // audio profile
      if(stream->writeFlag(mAudioProfile))
         stream->writeRangedU32(mAudioProfile->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

      // audio environment:
      if(stream->writeFlag(mAudioEnvironment))
         stream->writeRangedU32(mAudioEnvironment->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

   } 
   else 
   {
      if (stream->writeFlag((mask & TransformMask) != 0)) {
         mathWrite(*stream, mObjToWorld);
         mathWrite(*stream, mObjScale);
      }

      stream->writeFlag(mAlarmState);

      // Check the lights to see if we need to update any of their states
      LightUpdateGrouper::BitIterator itr;
      for (itr = mUpdateGrouper->begin(); itr.valid() && itr.getNumKeys(); itr++) {
         if (stream->writeFlag((mask & itr.getMask()) != 0)) {
            LightUpdateGrouper::BitIterator::iterator kItr;
            for (kItr = itr.begin(); kItr != itr.end(); kItr++) {
               U32 key = *kItr;
               U32 detail = detailFromUpdateKey(key);
               U32 index  = indexFromUpdateKey(key);

               stream->writeFlag(mLightInfo[detail].mLights[index].active);
            }
         }
      }

      if (stream->writeFlag(mask & SkinBaseMask))
         stream->writeString(mSkinBase);

      // audio update:
      if(stream->writeFlag(mask & AudioMask))
      {
         // profile:
         if(stream->writeFlag(mAudioProfile))
            stream->writeRangedU32(mAudioProfile->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);

         // environment:
         if(stream->writeFlag(mAudioEnvironment))
            stream->writeRangedU32(mAudioEnvironment->getId(), DataBlockObjectIdFirst, DataBlockObjectIdLast);
      }
   }

   return retMask;
}


//------------------------------------------------------------------------------
void InteriorInstance::unpackUpdate(NetConnection* c, BitStream* stream)
{
   Parent::unpackUpdate(c, stream);

   MatrixF temp;
   Point3F tempScale;

   if (stream->readFlag()) {
      // Initial Update
      // CRC
      stream->read(&mCRC);

      // File
      mInteriorFileName = stream->readSTString();

      // Terrain flag
      mShowTerrainInside = stream->readFlag();

      // Transform
      mathRead(*stream, &temp);
      mathRead(*stream, &tempScale);
      setScale(tempScale);
      setTransform(temp);

      // Alarm state: Note that we handle this ourselves on the initial update
      //  so that the state is always full on or full off...
      mAlarmState = stream->readFlag();

      mSkinBase = stream->readSTString();

      // audio profile:
      if(stream->readFlag())
      {
         U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
         mAudioProfile = dynamic_cast<AudioProfile*>(Sim::findObject(profileId));
      }
      else
         mAudioProfile = 0;

      // audio environment:
      if(stream->readFlag())
      {
         U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
         mAudioEnvironment = dynamic_cast<AudioEnvironment*>(Sim::findObject(profileId));
      }
      else
         mAudioEnvironment = 0;
   } 
   else 
   {
      // Normal update
      if (stream->readFlag()) {
         mathRead(*stream, &temp);
         mathRead(*stream, &tempScale);
         setScale(tempScale);
         setTransform(temp);
      }

      setAlarmMode(stream->readFlag());

      LightUpdateGrouper::BitIterator itr;
      for (itr = mUpdateGrouper->begin(); itr.valid() && itr.getNumKeys(); itr++) {
         if (stream->readFlag()) {
            LightUpdateGrouper::BitIterator::iterator kItr;
            for (kItr = itr.begin(); kItr != itr.end(); kItr++) {
               U32 key = *kItr;
               U32 detail = detailFromUpdateKey(key);
               U32 index  = indexFromUpdateKey(key);

               if (stream->readFlag())
                  activateLight(detail, index);
               else
                  deactivateLight(detail, index);
            }
         }
      }

      if (stream->readFlag()) {
         mSkinBase = stream->readSTString();
         renewOverlays();
      }

      // audio update:
      if(stream->readFlag())
      {
         // profile:
         if(stream->readFlag())
         {
            U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
            mAudioProfile = dynamic_cast<AudioProfile*>(Sim::findObject(profileId));
         }
         else
            mAudioProfile = 0;

         // environment:
         if(stream->readFlag())
         {
            U32 profileId = stream->readRangedU32(DataBlockObjectIdFirst, DataBlockObjectIdLast);
            mAudioEnvironment = dynamic_cast<AudioEnvironment*>(Sim::findObject(profileId));
         }
         else
            mAudioEnvironment = 0;
      }
   }
}


//------------------------------------------------------------------------------
Interior* InteriorInstance::getDetailLevel(const U32 level)
{
   return mInteriorRes->getDetailLevel(level);
}

U32 InteriorInstance::getNumDetailLevels()
{
   return mInteriorRes->getNumDetailLevels();
}

//--------------------------------------------------------------------------
//-------------------------------------- Alarm functionality
//
void InteriorInstance::setAlarmMode(const bool alarm)
{
   if (mInteriorRes->getDetailLevel(0)->mHasAlarmState == false)
      return;

   if (mAlarmState == alarm)
      return;

   mAlarmState = alarm;
   if (isServerObject())
   {
      setMaskBits(AlarmMask);
   }
   else
   {
      // DMMTODO: Invalidate current light state
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::rebuildVertexColors()
{
   U32 i;
   for (i = 0; i < mVertexColorsNormal.size(); i++)
   {
      delete mVertexColorsNormal[i];
      mVertexColorsNormal[i] = NULL;
   }
   for (i = 0; i < mVertexColorsAlarm.size(); i++)
   {
      delete mVertexColorsAlarm[i];
      mVertexColorsAlarm[i] = NULL;
   }

   if (bool(mInteriorRes) == false)
      return;

   mVertexColorsNormal.setSize(mInteriorRes->getNumDetailLevels());
   mVertexColorsAlarm.setSize(mInteriorRes->getNumDetailLevels());
   for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++)
   {
      mVertexColorsNormal[i] = new Vector<ColorI>;
      mVertexColorsAlarm[i]  = new Vector<ColorI>;
      VECTOR_SET_ASSOCIATION((*mVertexColorsNormal[i]));
      VECTOR_SET_ASSOCIATION((*mVertexColorsAlarm[i]));
   }
   for (i = 0; i < mInteriorRes->getNumDetailLevels(); i++)
   {
      Interior* pInterior = mInteriorRes->getDetailLevel(i);
      pInterior->rebuildVertexColors(mLMHandle,
                                     mVertexColorsNormal[i],
                                     mVertexColorsAlarm[i]);
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::updateLightMap(Interior* pInterior, LightInfo& rLightInfo, const U32 surfaceIndex)
{
   AssertFatal(surfaceIndex < pInterior->mSurfaces.size(), "Error, out of range surface index");
   static U8 _newLightMapBuffer[256*256*3];

   const Interior::Surface& rSurface = pInterior->mSurfaces[surfaceIndex];
   if (rSurface.lightCount == 0)
      return;

   // Get the surface's original bitmap
   TextureHandle* originalLMapHandle = ((mAlarmState == false) ? 
                                        gInteriorLMManager.getHandle(pInterior->getLMHandle(), mLMHandle,
                                                                     pInterior->mNormalLMapIndices[surfaceIndex]) :
                                        gInteriorLMManager.getHandle(pInterior->getLMHandle(), mLMHandle,
                                                                     pInterior->mAlarmLMapIndices[surfaceIndex]));

   const GBitmap* pOriginalLMap = originalLMapHandle->getBitmap();
   AssertFatal(pOriginalLMap != NULL, "error, no lightmap on the handle!");
   AssertFatal(pOriginalLMap->getFormat() == GBitmap::RGB, "error, bad lightmap format!");

   // First, we need to create a buffer that will receive the new lightmap
   U32 dimX = rSurface.mapSizeX;
   U32 dimY = rSurface.mapSizeY;
   U8* pNewLightmap = _newLightMapBuffer;

   // copy the original lightmap
   const U8 * src = pOriginalLMap->getAddress(rSurface.mapOffsetX, rSurface.mapOffsetY);
   U8 * dest = pNewLightmap;
   
   U32 runSize = rSurface.mapSizeX * 3;
   U32 srcStep = pOriginalLMap->getWidth() * 3;

   for(U32 y = 0; y < rSurface.mapSizeY; y++)
   {
      dMemcpy(dest, src, runSize);
      dest += runSize;
      src += srcStep;
   }

   // ...now we have the original lightmap, add in the animateds...
   for (U32 i = 0; i < rSurface.lightCount; i++) {
      const LightInfo::StateDataInfo& rInfo = rLightInfo.mStateDataInfo[rSurface.lightStateInfoStart + i];

      // Only add in states that affect this surface...duh.
      if (rInfo.curMap != NULL && rInfo.alarm == (mAlarmState != Normal)) {
         intensityMapMerge(pNewLightmap, dimX, dimY,
                           rInfo.curMap, rInfo.curColor);
      }
   }

   // OK, now we have the final, current lightmap.  subimage it in...
   glBindTexture(GL_TEXTURE_2D, originalLMapHandle->getGLName());
   
   if (Con::getBoolVariable("$pref::OpenGL::disableSubImage", false))
   {
      const U8 *src = pNewLightmap;
      U8 *dest = (U8 *) pOriginalLMap->getAddress(rSurface.mapOffsetX, rSurface.mapOffsetY);
      U32 destStep = pOriginalLMap->getWidth() * 3;

      // copy back into the original lightmap
      for (U32 y = 0; y < rSurface.mapSizeY; y++)
      {
         dMemcpy(dest, src, runSize);
         src += runSize;
         dest += destStep;
      }

      glTexImage2D(GL_TEXTURE_2D,
                   0,
                   GL_RGB,
                   pOriginalLMap->getWidth(), pOriginalLMap->getHeight(),
                   0,
                   GL_RGB, GL_UNSIGNED_BYTE,
                   pOriginalLMap->getBits(0));
   }
   else
      glTexSubImage2D(GL_TEXTURE_2D,
                      0,
                      rSurface.mapOffsetX, rSurface.mapOffsetY,
                      rSurface.mapSizeX,   rSurface.mapSizeY,
                      GL_RGB, GL_UNSIGNED_BYTE,
                      pNewLightmap);
}


//--------------------------------------------------------------------------
void InteriorInstance::downloadLightmaps(SceneState* /*state*/,
                                         Interior*   pInterior,
                                         LightInfo&  rLightInfo)
{
   extern U16* sgActivePolyList;
   extern U32  sgActivePolyListSize;

   for (U32 i = 0; i < sgActivePolyListSize; i++) {
      if (rLightInfo.mSurfaceInvalid.test(sgActivePolyList[i]) == true) {
         updateLightMap(pInterior, rLightInfo, sgActivePolyList[i]);
         rLightInfo.mSurfaceInvalid.clear(sgActivePolyList[i]);
      }
   }
}


//--------------------------------------------------------------------------
void InteriorInstance::addChildren()
{
   if (bool(mInteriorRes) == false)
      return;

   char nameBuffer[256];
   U32 i;

   // First thing to do, add a group with our name
   SimGroup* myGroup = getGroup();

   // Load all the interior game objects...
   for(i = 0; i < mInteriorRes->getNumGameEntities(); i++)
   {
      ItrGameEntity *ent = mInteriorRes->getGameEntity(i);
      ConsoleObject *obj = ConsoleObject::create(ent->mGameClass);
      GameBase *gb = dynamic_cast<GameBase*>(obj);
      if(!gb)
      {
         Con::errorf("Invalid game class for entity: %s", ent->mGameClass);
         delete obj;
         continue;
      }
      gb->setField("dataBlock", ent->mDataBlock);
      gb->setModStaticFields(true);
      for(U32 j = 0; j < ent->mDictionary.size(); j++)
         gb->setDataField(StringTable->insert(ent->mDictionary[j].name), NULL, ent->mDictionary[j].value);
      gb->setModStaticFields(false);
      Point3F origin = ent->mPos;
      origin.convolve(mObjScale);
      getTransform().mulP(origin);
      MatrixF xform(true);
      xform.setColumn(3, origin);
      gb->setTransform(xform);
      if(!gb->registerObject())
      {
         Con::errorf("Failed to register entity: %s: %s", ent->mGameClass, ent->mDataBlock);
         delete gb;
         continue;
      }
      myGroup->addObject(gb);
   }

   // Next, for each door in our resource, we'll be creating a group
   for (i = 0; i < mInteriorRes->getNumInteriorPathFollowers(); i++) 
   {
      InteriorPathFollower* pSource = mInteriorRes->getInteriorPathFollower(i);

      StringTableEntry name = StringTable->insert(pSource->mName);
      PathedInterior* child = new PathedInterior;
      child->mName             = name;
      child->mInteriorResIndex = pSource->mInteriorResIndex;
      child->mPathIndex        = pSource->mPathIndex;
      child->mOffset           = pSource->mOffset;
      child->mInteriorResName  = mInteriorFileName;
      child->setField("dataBlock", pSource->mDataBlock);

      for(U32 i = 0; i < pSource->mDictionary.size(); i++)
         child->setDataField(pSource->mDictionary[i].name, NULL, pSource->mDictionary[i].value);

      // Create a group for the door
      SimGroup* doorGroup = new SimGroup;
      dSprintf(nameBuffer, 255, "%s_g", child->mName);
      doorGroup->registerObject();
      myGroup->addObject(doorGroup, nameBuffer);

      // Ok, now we need to add the path and any triggers that affect the door here...
      Path* pSimPath      = new Path;
      pSimPath->registerObject();
      doorGroup->addObject(pSimPath);

      for (U32 j = 0; j < pSource->mWayPoints.size(); j++) {
         Marker* pMarker    = new Marker;
         pMarker->mSeqNum   = j;
         pMarker->mMSToNext = pSource->mWayPoints[j].msToNext;
         pMarker->mSmoothingType = pSource->mWayPoints[j].smoothingType;

         pMarker->registerObject();

         pSimPath->addObject(pMarker);

         Point3F markerPos = pSource->mWayPoints[j].pos;
         markerPos.convolve(mObjScale);
         getTransform().mulP(markerPos);
         MatrixF xform(true);
         xform.setColumn(3, markerPos);
         pMarker->setTransform(xform);
      }
      //pSimPath->finishPath();

      // Now we need to add any triggers relevant to this door...
      //
      for (U32 k = 0; k < pSource->mTriggerIds.size(); k++) {
         InteriorResTrigger *pITrigger = mInteriorRes->getTrigger(pSource->mTriggerIds[k]);
         // Add the object...
         Trigger* pTrigger = new Trigger;
         pTrigger->setField("dataBlock", pITrigger->mDataBlock);
         for(U32 i = 0; i < pITrigger->mDictionary.size(); i++)
            pTrigger->setDataField(StringTable->insert(pITrigger->mDictionary[i].name), NULL, pITrigger->mDictionary[i].value);

         MatrixF newXForm;
         createTriggerTransform(pITrigger, &newXForm);
         pTrigger->setTriggerPolyhedron(pITrigger->mPolyhedron);
         pTrigger->setScale(mObjScale);
         pTrigger->setTransform(newXForm);

         pTrigger->registerObject();
         doorGroup->addObject(pTrigger, pITrigger->mName);
      }

      // And finally, add the door, taking care to get the transform right...
      MatrixF childTransform(true);
      Point3F childOffset = child->mOffset;
      childOffset.convolve(mObjScale);
      getTransform().mulP(childOffset);
      childOffset.neg();
      childTransform.setColumn(3, childOffset);
      childTransform.mul(getTransform());
      child->mBaseTransform = childTransform;
      child->mBaseScale     = mObjScale;
      doorGroup->addObject(child, child->mName);
      if (!child->registerObject()) {
         Con::warnf(ConsoleLogEntry::General, "Warning, could not register door.  Door skipped!");
         delete child;
         continue;
      }
   }
}


void InteriorInstance::createTriggerTransform(const InteriorResTrigger* trigger, MatrixF* transform)
{
   Point3F offset;
   MatrixF xform = getTransform();
   xform.getColumn(3, &offset);
   
   Point3F triggerOffset = trigger->mOffset;
   triggerOffset.convolve(mObjScale);
   getTransform().mulV(triggerOffset);
   offset += triggerOffset;
   xform.setColumn(3, offset);

   *transform = xform;
}

bool InteriorInstance::readLightmaps(GBitmap**** lightmaps)
{
   AssertFatal(mInteriorRes, "Error, no interior loaded!");
   AssertFatal(lightmaps, "Error, no lightmaps or numdetails result pointers");
   AssertFatal(*lightmaps == NULL, "Error, already have a pointer in the lightmaps result field!");

   // Load resource
   Stream* pStream = ResourceManager->openStream(mInteriorFileName);
   if (pStream == NULL) {
      Con::errorf(ConsoleLogEntry::General, "Unable to load interior: %s", mInteriorFileName);
      return false;
   }

   InteriorResource* pResource = new InteriorResource;
   bool success = pResource->read(*pStream);
   ResourceManager->closeStream(pStream);

   if (success == false)
   {
      delete pResource;
      return false;
   }
   AssertFatal(pResource->getNumDetailLevels() == mInteriorRes->getNumDetailLevels(),
               "Mismatched detail levels!");
   
   *lightmaps  = new GBitmap**[mInteriorRes->getNumDetailLevels()];

   for (U32 i = 0; i < pResource->getNumDetailLevels(); i++)
   {
      Interior* pInterior = pResource->getDetailLevel(i);
      (*lightmaps)[i] = new GBitmap*[pInterior->mLightmaps.size()];
      for (U32 j = 0; j < pInterior->mLightmaps.size(); j++)
      {
         ((*lightmaps)[i])[j] = pInterior->mLightmaps[j];
         pInterior->mLightmaps[j] = NULL;
      }
      pInterior->mLightmaps.clear();
   }

   delete pResource;
   return true;
}

   
