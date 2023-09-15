//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DECALMANAGER_H_
#define _DECALMANAGER_H_

#ifndef _SCENEOBJECT_H_
#include "sim/sceneObject.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif

/// DataBlock implementation for decals.
class DecalData : public SimDataBlock
{
   typedef SimDataBlock Parent;

   //-------------------------------------- Console set variables
  public:
   F32               sizeX;
   F32               sizeY;
   StringTableEntry  textureName;

   //-------------------------------------- load set variables
  public:
   TextureHandle textureHandle;

  public:
   DecalData();
   ~DecalData();

   void packData(BitStream*);
   void unpackData(BitStream*);
   bool preload(bool server, char errorBuffer[256]);

   DECLARE_CONOBJECT(DecalData);
   static void initPersistFields();
};

/// Store an instance of a decal.
struct DecalInstance
{
   DecalData* decalData;
   Point3F    point[4];

   U32            allocTime;
   F32            fade;
   DecalInstance* next;
};

/// Manage decals in the world.
class DecalManager : public SceneObject
{
   typedef SceneObject Parent;

   Vector<DecalInstance*> mDecalQueue;
   bool                   mQueueDirty;

   static U32             smMaxNumDecals;
   static U32             smDecalTimeout;
   
   static const U32          csmFreePoolBlockSize;
   Vector<DecalInstance*>    mFreePoolBlocks;
   DecalInstance*            mFreePool;
   
  protected:
   bool prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
   void renderObject(SceneState *state, SceneRenderImage *image);

   DecalInstance* allocateDecalInstance();
   void freeDecalInstance(DecalInstance*);
   
  public:                     
   DecalManager();
   ~DecalManager();

   static void consoleInit();
   
   /// @name Decal Addition
   ///
   /// These functions allow you to add new decals to the world.
   /// @{
   void addDecal(const Point3F& pos,
                 const Point3F& rot,
                 Point3F normal,
                 const Point3F& scale,
                 DecalData*);
   void addDecal(const Point3F& pos,
                 const Point3F& rot,
                 Point3F normal,
                 DecalData*);
   void addDecal(const Point3F& pos,
                 Point3F normal,
                 DecalData*);
   /// @}

   void dataDeleted(DecalData *data);

   void renderDecal();
   DECLARE_CONOBJECT(DecalManager);

   static bool smDecalsOn;
};

extern DecalManager* gDecalManager;

#endif // _H_DecalManager
