//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIORINSTANCE_H_
#define _INTERIORINSTANCE_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _INTERIORRES_H_
#include "interior/interiorRes.h"
#endif
#ifndef _INTERIORLMMANAGER_H_
#include "interior/interiorLMManager.h"
#endif

#ifndef _BITVECTOR_H_
#include "core/bitVector.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class AbstractPolyList;
class LightUpdateGrouper;
class InteriorSubObject;
class InteriorResTrigger;
class MaterialList;
class TextureObject;
class FloorPlan;
class Convex;
class AudioProfile;
class AudioEnvironment;

//--------------------------------------------------------------------------
class InteriorInstance : public SceneObject
{
   typedef SceneObject Parent;
   friend class SceneLighting;
   friend class FloorPlan;

  public:
   InteriorInstance();
   ~InteriorInstance();

   static void init();
   static void destroy();

   // Collision
  public:
   bool buildPolyList(AbstractPolyList *polyList, const Box3F &box, const SphereF &sphere);
   bool castRay(const Point3F &start, const Point3F &end, RayInfo *info);
   virtual void setTransform(const MatrixF &mat);

   void buildConvex(const Box3F& box,Convex* convex);
  private:
   Convex* mConvexList;

  public:
  
   /// @name Lighting control
   /// @{
   
   /// This returns true if the interior is in an alarm state. Alarm state
   /// will put different lighting into the interior and also possibly
   /// have an audio element also.
   bool inAlarmState() {return(mAlarmState);}
   
   /// This sets the alarm mode of the interior.
   /// @param   alarm   If true the interior will be in an alarm state next frame
   void setAlarmMode(const bool alarm);
   
   /// Activates a light with the given name on all detail levels of the interior
   /// @param   pLightName   Name of the light
   void activateLight(const char* pLightName);
   
   /// Deactivates a light with the given name on all detail levels of the interior
   /// @param   pLightName   Name of the light
   void deactivateLight(const char* pLightName);
   
   /// Echos out all the lights in the interior, starting with triggerable then
   /// animated lights
   void echoTriggerableLights();
   
   /// @}

  public:
   /// @name Subobject access interface
   /// @{
      
   /// Returns the number of detail levels for an object
   U32 getNumDetailLevels();
   
   /// Gets the interior associated with a particular detail level
   /// @param   level   Detail level
   Interior* getDetailLevel(const U32 level);
   
   /// Sets the detail level to render manually
   /// @param   level   Detail level to force
   void setDetailLevel(S32 level = -1) { mForcedDetailLevel = level; }
   /// @}

   // Material management for overlays
  public:
  
   /// Reloads material information if the interior skin changes
   void renewOverlays();
   
   /// Sets the interior skin to something different
   /// @param   newBase   New base skin
   void setSkinBase(const char *newBase);

  public:
   static bool smDontRestrictOutside;
   static F32  smDetailModification;

   
   DECLARE_CONOBJECT(InteriorInstance);
   static void initPersistFields();
   static void consoleInit();

   /// Reads the lightmaps of the interior into the provided pointer
   /// @param   lightmaps   Lightmaps in the interior (out)
   bool readLightmaps(GBitmap ****lightmaps);
   
  protected:
   bool onAdd();
   void onRemove();

   void inspectPreApply();
   void inspectPostApply();

   static U32 smLightUpdatePeriod;
   static bool smRenderDynamicLights;

   U32  mLightUpdatedTime;
   void setLightUpdatedTime(const U32);
   U32  getLightUpdatedTime() const;

   bool onSceneAdd(SceneGraph *graph);
   void onSceneRemove();
   U32  getPointZone(const Point3F& p);
   bool getOverlappingZones(SceneObject* obj, U32* zones, U32* numZones);

   U32  calcDetailLevel(SceneState*, const Point3F&);
   bool prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
   void renderObject(SceneState *state, SceneRenderImage *image);
   bool scopeObject(const Point3F&        rootPosition,
                    const F32             rootDistance,
                    bool*                 zoneScopeState);

  public:
   /// Not yet implemented
   void addChildren();
   
   /// Returns true if the interiors are rendering dynamic lighting
   static bool getRenderDynamicLights() { return(smRenderDynamicLights); }
   
   /// Turns on or off dynamic lighting of interiors
   /// @param   val   If true dynamic lighting is enabled
   static void setRenderDynamicLights(bool val) { smRenderDynamicLights = val; }

  private:
   /// @name Light utility methods
   /// These should not be called directly. Use the public activateLight(const char *)
   /// method instead because unless the detail level is rendering and it's much 
   /// easier to not manage the lights on a per-detail level basis.
   /// @{
      
   /// Activates a specific light for a detail level
   /// @param   detail   Detail level
   /// @param   lightIndex   Index of light in light list
   void activateLight(const U32 detail, const U32 lightIndex);
   
   /// Deactivates a specific light for a detail level
   /// @param   detail   Detail level
   /// @param   lightIndex   Index of light in the light list
   void deactivateLight(const U32 detail, const U32 lightIndex);
   
   /// @}

   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);


   enum UpdateMaskBits {
      InitMask       = BIT(0),
      TransformMask  = BIT(1),
      AlarmMask      = BIT(2),

      // Reserved for light updates (8 bits for now)
      _lightupdate0  = BIT(3),
      _lightupdate1  = BIT(4),
      _lightupdate2  = BIT(5),
      _lightupdate3  = BIT(6),
      _lightupdate4  = BIT(7),
      _lightupdate5  = BIT(8),
      _lightupdate6  = BIT(9),
      _lightupdate7  = BIT(10),

      SkinBaseMask   = BIT(11),
      AudioMask      = BIT(12),
      NextFreeMask   = BIT(13)
   };
   enum Constants {
      LightUpdateBitStart = 3,
      LightUpdateBitEnd   = 10
   };

  private:
   StringTableEntry                     mInteriorFileName;     ///< File name of the interior this instance encapuslates
   U32                                  mInteriorFileHash;     ///< Hash for interior file name, used for sorting
   Resource<InteriorResource>           mInteriorRes;          ///< Interior managed by resource manager
   Vector<MaterialList*>                mMaterialMaps;         ///< Materials for this interior
   StringTableEntry                     mSkinBase;             ///< Skin for this interior

   Vector< Vector<InteriorSubObject*> > mInteriorSubObjects;   ///< Sub objects of this interior
   bool                                 mShowTerrainInside;    ///< Enables or disables terrain showing through the interior
   LM_HANDLE                            mLMHandle;             ///< Handle to the light manager
   AudioProfile *                       mAudioProfile;         ///< Audio profile 
   AudioEnvironment *                   mAudioEnvironment;     ///< Audio environment 
   S32                                  mForcedDetailLevel;    ///< Forced LOD, if -1 auto LOD
   U32                                  mCRC;                  ///< CRC for the interior
   
  public:
   
   /// Returns the Light Manager handle
   LM_HANDLE getLMHandle() { return(mLMHandle); }
   
   /// Returns the audio profile
   AudioProfile * getAudioProfile() { return(mAudioProfile); }
   
   /// Returns the audio environment
   AudioEnvironment * getAudioEnvironment() { return(mAudioEnvironment); }
   
   /// This is used to determine just how 'inside' a point is in an interior.
   /// This is used by the environmental audio code for audio properties and the
   /// function always returns true.
   /// @param   pos   Point to test
   /// @param   pScale   How inside is the point 0 = totally outside, 1 = totally inside (out)
   bool getPointInsideScale(const Point3F & pos, F32 * pScale);   // ~0: outside -> 1: inside

   /// Returns the interior resource
   Resource<InteriorResource> & getResource() {return(mInteriorRes);} // SceneLighting::InteriorProxy interface
   
   /// Returns the CRC for validation
   U32 getCRC() { return(mCRC); }

   /// @name Vertex Lighting 
   /// Vertex lighting is the alternative to lightmapped interiors
   /// @{
      
   Vector<Vector<ColorI>*> mVertexColorsNormal;       ///< Vertex colors under normal lighting per detail level
   Vector<Vector<ColorI>*> mVertexColorsAlarm;       ///< Vertex colors under alarm lighting per detail level
   
   /// Rebuilds the vertex colors for alarm and normal states for all detail levels
   void rebuildVertexColors();
   
   /// Returns the normal vertex lighting colors for a detail level
   /// @param   detail   Detail level
   Vector<ColorI>* getVertexColorsNormal(U32 detail);
   
   /// Returns the alarm vertex lighting colors for a detail level
   /// @param   detail   Detail level
   Vector<ColorI>* getVertexColorsAlarm(U32 detail);
   
   /// @}
   
   // Alarm state information
  private:
   enum AlarmState {
      Normal          = 0,
      Alarm           = 1
   };

   bool mAlarmState;                   ///< Alarm state of the interior

   // LightingAnimation information
  private:
   struct LightInfo {
      struct Light {
         U32    curState;
         U32    curTime;
         ColorI curColor;

         bool   active;
         bool   alarm;
      };
      struct StateDataInfo {
         ColorI curColor;
         U8*    curMap;
         bool   alarm;
      };

      Vector<Light> mLights;
      BitVector             mSurfaceInvalid;
      Vector<StateDataInfo> mStateDataInfo;
   };
   Vector<LightInfo>   mLightInfo;           ///< Light info, one per detail level
   LightUpdateGrouper* mUpdateGrouper;       ///< Designed to group net updates for lights to reduce traffic

   /// @name Light Grouper
   /// This is for managing light updates across the network
   /// @{
      
   /// Creates an update key for the LightGrouper
   /// @param   detail   Detail level
   /// @param   lightIndex   Index of light in the interior
   static U32 makeUpdateKey(const U32 detail, const U32 lightIndex);
   
   /// Takes an update key and returns the detail level part of it
   /// @param   key   Update key
   static U32 detailFromUpdateKey(const U32 key);
   
   /// Takes an update key and returns the light index part of it
   /// 2param   key   Update key
   static U32 indexFromUpdateKey(const U32 key);
   /// @}

   /// @name Animated light functions
   /// @{
      
   /// Steps the animated light simulation by a delta
   /// @param   detail   Detail level of interior
   /// @param   lightIndex   Index of light to work on
   /// @param   ms   Time delta from last update in miliseconds
   void updateLightTime(const U32 detail, const U32 lightIndex, const U32 ms);
   
   /// This loops through all the surfaces in an interior and calls updateLightMap on them
   /// @param   state   SceneState - Not used
   /// @param   pInterior   Interior to operate on
   /// @param   rLightInfo   Light to use
   void downloadLightmaps(SceneState *state, Interior *pInterior, LightInfo &rLightInfo);
   
   /// This will set up a particular light in a particular detail level
   /// @param   detail   Detail level
   /// @param   lightIndex   Light to install
   void installLight(const U32 detail, const U32 lightIndex);
   
   /// Called by updateLightTime to update a light with a looping animation
   /// @param   interior   Interior to work on
   /// @param   light   Light to update
   /// @param   lightIndex   Index of animated light
   /// @param   ms   Time delta from last update in miliseconds
   void updateLoopingLight(Interior *interior, LightInfo::Light &light, const U32 lightIndex, const U32 ms);
   
   /// Called by updateLightTime to update a light with a flicker animation
   /// @param   interior   Interior to work on
   /// @param   light   Light to update
   /// @param   lightIndex   Index of animated light
   /// @param   ms   Time delta from last update in miliseconds
   void updateFlickerLight(Interior *interior, LightInfo::Light &light, const U32 lightIndex, const U32 ms);
   
   /// Called by updateLightTime to update a light with a fade-up (ramp) animation light
   /// @param   interior   Interior to work on
   /// @param   light   Light to update
   /// @param   lightIndex   Index of animated light
   /// @param   ms   Time delta from last update in miliseconds
   void updateRampLight(Interior *interior, LightInfo::Light &light, const U32 lightIndex, const U32 ms);
   
   /// Updates the animation for all lights
   /// @param   ms   Time delta since last call in ms
   void updateAllLights(const U32 ms);
   
   /// Takes the original lightmap and adds the animated lights to it and then
   /// binds the texture to it
   /// @param   pInterior   Interior object to map
   /// @param   rLightInfo  Light info to use to update the light map
   /// @param   surfaceIndex   The surface to operate on inside the interior
   void updateLightMap(Interior *pInterior, LightInfo &rLightInfo, const U32 surfaceIndex);
   
   /// lightMap is a 24-bit RGB texture, intensityMap is an 8 bit intensity map.  
   /// This generates lightmap = [lightmap + (intensityMap * color)]
   /// @param   lightMap   Lightmap to operate on (in/out)
   /// @param   width   width of the ligth map
   /// @param   height   hight of the light map
   /// @param   intensityMap   Intensity map
   /// @param   color   Color
   void intensityMapMerge(U8* lightMap,
                          const U32 width, const U32 height,
                          const U8* intensityMap, const ColorI& color);
   /// @}

  private:
  
   /// Creates a transform based on an trigger area
   /// @param   trigger   Trigger to create a transform for
   /// @param   transform Transform generated (out)
   void createTriggerTransform(const InteriorResTrigger *trigger, MatrixF *transform);
};

inline void InteriorInstance::setLightUpdatedTime(const U32 now)
{
   mLightUpdatedTime = now;
}

inline U32 InteriorInstance::getLightUpdatedTime() const
{
   return mLightUpdatedTime;
}

inline U32 InteriorInstance::makeUpdateKey(const U32 detail, const U32 lightIndex)
{
   AssertFatal(detail < (1 << 16) && lightIndex < (1 << 16), "Error, out of bounds key params");

   return (detail << 16) | (lightIndex & 0x0000FFFF);
}

inline U32 InteriorInstance::detailFromUpdateKey(const U32 key)
{
   return (key >> 16) & 0xFFFF;
}

inline U32 InteriorInstance::indexFromUpdateKey(const U32 key)
{
   return (key >> 0) & 0xFFFF;
}

inline Vector<ColorI>* InteriorInstance::getVertexColorsNormal(U32 detail)
{
   if (bool(mInteriorRes) == false || detail > mInteriorRes->getNumDetailLevels())
      return NULL;

   return mVertexColorsNormal[detail];
}

inline Vector<ColorI>* InteriorInstance::getVertexColorsAlarm(U32 detail)
{
   if (bool(mInteriorRes) == false || detail > mInteriorRes->getNumDetailLevels())
      return NULL;

   return mVertexColorsAlarm[detail];
}

#endif //_INTERIORBLOCK_H_

