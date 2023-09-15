//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _SCENEGRAPH_H_
#define _SCENEGRAPH_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _POLYLIST_H_
#include "core/polyList.h"
#endif
#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif
#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _GBITMAP_H_
#include "dgl/gBitmap.h"
#endif

class SceneState;
class NetConnection;

class Sky;
class TerrainBlock;
class DecalManager;

struct FogVolume
{
   float    visibleDistance;
   float    minHeight;
   float    maxHeight;
   float    percentage;
   ColorF   color;   
};

enum FogConstants
{
   MaxFogVolumes = 3,
   FogTextureDistSize = 64,
   FogTextureHeightSize = 64
};

//--------------------------------------------------------------------------
//-------------------------------------- SceneGraph
//
class SceneGraph
{
  public:
   SceneGraph(bool isClient);
   ~SceneGraph();

  public:
   /// @name SceneObject Management
   /// @{

   bool addObjectToScene(SceneObject*);
   void removeObjectFromScene(SceneObject*);
   void zoneInsert(SceneObject*);
   void zoneRemove(SceneObject*);
   /// @}

   // 
  public:
   /// @name Zone management
   /// @{

   void registerZones(SceneObject*, U32 numZones);
   void unregisterZones(SceneObject*);
   SceneObject* getZoneOwner(const U32 zone);
   /// @}

  public:
   /// @name Rendering and Scope Management
   /// @{

   void renderScene(const U32 objectMask = 0xffffffff);
   void scopeScene(const Point3F& scopePosition,
                   const F32      scopeDistance,
                   NetConnection* netConnection);
   /// @}

  public:
   /// @name Camera
   /// For objects, valid only during the rendering cycle
   /// @{

   const Point3F& getBaseCameraPosition() const;
   const Point3F& getCurrCameraPosition() const;
   /// @}

   /// @name Fog/Visibility Management
   /// @{

   ColorF getFogColor();
   F32 getFogDistance();
   F32 getVisibleDistance();
   F32 getFogDistanceMod();
   F32 getVisibleDistanceMod();

   void setFogDistance(F32 dist);
   void setVisibleDistance(F32 dist);
   void setFogColor(ColorF fogColor);
   void setFogVolumes(U32 numFogVolumes, FogVolume *fogVolumes);
   void getFogVolumes(U32 &numFogVolumes, FogVolume * &fogVolumes);
   void buildFogTexture( SceneState *pState );
   void buildFogTextureSpecial( SceneState *pState ); 
   
   void getFogCoordPair(F32 dist, F32 z, F32 &x, F32 &y);
   F32  getFogCoord(F32 dist, F32 z);
   /// @}

   U32 getStateKey() { return(smStateKey); }

  private:
   void setBaseCameraPosition(const Point3F&);
   void setCurrCameraPosition(const Point3F&);

   //-------------------------------------- Private interface and data
  protected:
   TextureHandle mFogTexture;
   TextureHandle mFogTextureIntensity;
   static const U32 csmMaxTraversalDepth;
   static U32 smStateKey;

  public:
   static F32 smVisibleDistanceMod;

   
  public:   
   static bool useSpecial;

  protected:   
   bool mIsClient;
   
   bool mHazeArrayDirty;
   static F32 mHazeArray[FogTextureDistSize];
   static U32 mHazeArrayi[FogTextureDistSize];
   static F32 mDistArray[FogTextureDistSize];

   F32 mInvVisibleDistance;
   F32 mHeightRangeBase;
   F32 mHeightOffset;
   F32 mInvHeightRange;

   U32  mCurrZoneEnd;
   U32  mNumActiveZones;

   Point3F  mBaseCameraPosition;
   Point3F  mCurrCameraPosition;

   U32       mNumFogVolumes;   
   FogVolume mFogVolumes[MaxFogVolumes];
   F32       mFogDistance;
   F32       mVisibleDistance;
   ColorF    mFogColor;

   LightManager mLightManager;

   Sky*          mCurrSky;
   TerrainBlock* mCurrTerrain;
   DecalManager* mCurrDecalManager;

   Vector<SceneObject*> mWaterList;
   
   void            addRefPoolBlock();
   SceneObjectRef* allocateObjectRef();
   void            freeObjectRef(SceneObjectRef*);
   SceneObjectRef*         mFreeRefPool;
   Vector<SceneObjectRef*> mRefPoolBlocks;
   static const U32        csmRefPoolBlockSize;
   
  public:
   LightManager * getLightManager();

   Sky*          getCurrentSky()          { return mCurrSky; }
   TerrainBlock* getCurrentTerrain()      { return mCurrTerrain; }
   DecalManager* getCurrentDecalManager() { return mCurrDecalManager; }
   void getWaterObjectList(SimpleQueryList&);

   TextureHandle getFogTexture()          { return mFogTexture; }
   TextureHandle getFogTextureIntensity() { return mFogTextureIntensity; }
   // Object database for zone managers
  protected:
   struct ZoneManager {
      SceneObject* obj;
      U32          zoneRangeStart;
      U32          numZones;
   };
   Vector<ZoneManager>     mZoneManagers;

   /// Zone Lists
   ///
   /// @note The object refs in this are somewhat singular in that the object pointer does not
   ///  point to a referenced object, but the owner of that zone...
   Vector<SceneObjectRef*> mZoneLists;

  protected:
   void buildSceneTree(SceneState*, SceneObject*, const U32, const U32, const U32);
   void traverseSceneTree(SceneState* pState);
   void treeTraverseVisit(SceneObject*, SceneState*, const U32);

   void compactZonesCheck();
   bool alreadyManagingZones(SceneObject*) const;
   void findZone(const Point3F&, SceneObject*&, U32&);
   void rezoneObject(SceneObject*);

   void addToWaterList(SceneObject* obj);
   void removeFromWaterList(SceneObject* obj);
};

extern SceneGraph* gClientSceneGraph;
extern SceneGraph* gServerSceneGraph;

inline LightManager * SceneGraph::getLightManager()
{
   return(&mLightManager);
}

inline const Point3F& SceneGraph::getBaseCameraPosition() const
{
   return mBaseCameraPosition;
}

inline const Point3F& SceneGraph::getCurrCameraPosition() const
{
   return mCurrCameraPosition;
}

inline void SceneGraph::setBaseCameraPosition(const Point3F& p)
{
   mBaseCameraPosition = p;
}

inline void SceneGraph::getFogCoordPair(F32 dist, F32 z, F32 &x, F32 &y)
{
   x = (getVisibleDistanceMod() - dist) * mInvVisibleDistance;
   y = (z - mHeightOffset) * mInvHeightRange;
}

inline F32 SceneGraph::getFogCoord(F32 dist, F32 z)
{
   GBitmap* pBitmap = mFogTexture.getBitmap();
   AssertFatal(pBitmap->getFormat() == GBitmap::RGBA, "Error, wrong format for this query");
   AssertFatal(pBitmap->getWidth() == 64 && pBitmap->getHeight() == 64, "Error, fog texture wrong dimensions");
   
   S32 x = S32(((getVisibleDistanceMod() - dist) * mInvVisibleDistance) * 63.0f);
   S32 y = S32(((z - mHeightOffset) * mInvHeightRange) * 63.0f);
   U32 samplex = mClamp(x, 0, 63);
   U32 sampley = mClamp(y, 0, 63);

   return F32(pBitmap->pBits[(((sampley * 64) + samplex) * 4) + 3]) / 255.0f;
}

inline ColorF SceneGraph::getFogColor()
{
   return(mFogColor);
}

inline F32 SceneGraph::getFogDistance()
{
   return(mFogDistance);
}

inline F32 SceneGraph::getFogDistanceMod()
{
   return(mFogDistance * smVisibleDistanceMod);
}

inline F32 SceneGraph::getVisibleDistance()
{
   return(mVisibleDistance);
}

inline F32 SceneGraph::getVisibleDistanceMod()
{
   return(mVisibleDistance * smVisibleDistanceMod);
}

inline void SceneGraph::setCurrCameraPosition(const Point3F& p)
{
   mCurrCameraPosition = p;
}

inline void SceneGraph::getFogVolumes(U32 &numFogVolumes, FogVolume * &fogVolumes)
{
   numFogVolumes = mNumFogVolumes;
   fogVolumes = mFogVolumes;
}

//--------------------------------------------------------------------------
inline SceneObjectRef* SceneGraph::allocateObjectRef()
{
   if (mFreeRefPool == NULL) {
      addRefPoolBlock();
   }
   AssertFatal(mFreeRefPool!=NULL, "Error, should always have a free reference here!");

   SceneObjectRef* ret = mFreeRefPool;
   mFreeRefPool = mFreeRefPool->nextInObj;

   ret->nextInObj = NULL;
   return ret;
}

inline void SceneGraph::freeObjectRef(SceneObjectRef* trash)
{
   trash->nextInBin = NULL;
   trash->prevInBin = NULL;
   trash->nextInObj = mFreeRefPool;
   mFreeRefPool     = trash;
}

inline SceneObject* SceneGraph::getZoneOwner(const U32 zone)
{
   AssertFatal(zone < mCurrZoneEnd, "Error, out of bounds zone selected!");

   return mZoneLists[zone]->object;
}


#endif //_SCENEGRAPH_H_

