//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _TSSHAPE_H_
#include "ts/tsShape.h"
#endif
#ifndef _LIGHTMANAGER_H_
#include "sceneGraph/lightManager.h"
#endif
#ifndef _PLATFORMAUDIO_H_
#include "platform/platformAudio.h"
#endif

#include "game/fx/particleEmitter.h"

class ExplosionData;
class SplashData;
class DecalData;
class ShapeBase;
class TSShapeInstance;
class TSThread;

//--------------------------------------------------------------------------
/// Datablock for projectiles.  This class is the base class for all other projectiles.
class ProjectileData : public GameBaseData
{
   typedef GameBaseData Parent;

protected:
   bool onAdd();

public:
   // variables set in datablock definition:
   // Shape related
   const char* projectileShapeName;

   bool hasLight;
   F32 lightRadius;
   ColorF lightColor;

   bool hasWaterLight;
   ColorF waterLightColor;

   /// Set to true if it is a billboard and want it to always face the viewer, false otherwise
   bool faceViewer;
   Point3F scale;

   /// [0,1] scale of how much velocity should be inherited from the parent object
   F32 velInheritFactor;
   /// Speed of the projectile when fired
   F32 muzzleVelocity;

   bool doDynamicClientHits;
   /// Should it arc?
   bool isBallistic;

   /// How HIGH should it bounce (parallel to normal), [0,1]
   F32 bounceElasticity;
   /// How much momentum should be lost when it bounces (perpendicular to normal), [0,1]
   F32 bounceFriction;

   /// Should this projectile fall/rise different than a default object?
   F32 gravityMod;

   /// How long the projectile should exist before deleting itself
   U32 lifetime;     // all times are internally represented as ticks
   /// How long it should not detonate on impact
   S32 armingDelay;  // the values are converted on initialization with
   S32 fadeDelay;    // the IRangeValidatorScaled field validator

   ExplosionData* explosion;           // Explosion Datablock
   S32 explosionId;                    // Explosion ID
   ExplosionData* waterExplosion;      // Water Explosion Datablock
   S32 waterExplosionId;               // Water Explosion ID

   SplashData* splash;                 // Water Splash Datablock
   S32 splashId;                       // Water splash ID

   AudioProfile* sound;                // Projectile Sound
   S32 soundId;                        // Projectile Sound ID

   enum DecalConstants {               // Number of decals constant
      NumDecals = 6,
   };
   DecalData* decals[NumDecals];       // Decal Datablocks
   S32 decalId[NumDecals];             // Decal IDs
   U32 decalCount;                     // # of loaded Decal Datablocks

   // variables set on preload:
   Resource<TSShape> projectileShape;
   S32 activateSeq;
   S32 maintainSeq;

   ParticleEmitterData* particleEmitter;
   S32 particleEmitterId;
   ParticleEmitterData* particleWaterEmitter;
   S32 particleWaterEmitterId;

   ProjectileData();

   void packData(BitStream*);
   void unpackData(BitStream*);
   bool preload(bool server, char errorBuffer[256]);

   static void initPersistFields();
   DECLARE_CONOBJECT(ProjectileData);
};


//--------------------------------------------------------------------------
/// Base class for all projectiles.
class Projectile : public GameBase
{
   typedef GameBase Parent;
   ProjectileData* mDataBlock;

public:
   // Initial conditions
   enum ProjectileConstants {
      SourceIdTimeoutTicks = 7,   // = 231 ms
      DeleteWaitTime       = 500, ///< 500 ms delete timeout (for network transmission delays)
      ExcessVelDirBits     = 7,
      MaxLivingTicks       = 4095,
   };
   enum UpdateMasks {
      BounceMask    = Parent::NextFreeMask,
      ExplosionMask = Parent::NextFreeMask << 1,
      NextFreeMask  = Parent::NextFreeMask << 2
   };
protected:

   ParticleEmitter* mParticleEmitter;
   ParticleEmitter* mParticleWaterEmitter;

   AUDIOHANDLE mSoundHandle;

   Point3F  mCurrPosition;
   Point3F  mCurrVelocity;
   S32      mSourceObjectId;
   S32      mSourceObjectSlot;

   // Time related variables common to all projectiles, managed by processTick

   U32 mCurrTick;                         ///< Current time in ticks
   SimObjectPtr<ShapeBase> mSourceObject; ///< Actual pointer to the source object, times out after SourceIdTimeoutTicks

   // Rendering related variables
   TSShapeInstance* mProjectileShape;
   TSThread*        mActivateThread;
   TSThread*        mMaintainThread;

   void registerLights(LightManager * lm, bool lightingScene);
   LightInfo mLight;

   bool             mHidden;        ///< set by the derived class, if true, projectile doesn't render
   F32              mFadeValue;     ///< set in processTick, interpolation between fadeDelay and lifetime
                                    ///< in data block

   // Warping and back delta variables.  Only valid on the client
   //
   Point3F mWarpStart;
   Point3F mWarpEnd;
   U32     mWarpTicksRemaining;

   Point3F mCurrDeltaBase;
   Point3F mCurrBackDelta;

   Point3F mExplosionPosition;
   Point3F mExplosionNormal;

   bool onAdd();
   void onRemove();
   bool onNewDataBlock(GameBaseData *dptr);

   void processTick(const Move *move);
   void advanceTime(F32 dt);
   void interpolateTick(F32 delta);

   /// What to do once this projectile collides with something
   virtual void onCollision(const Point3F& p, const Point3F& n, SceneObject*);

   /// What to do when this projectile explodes
   virtual void explode(const Point3F& p, const Point3F& n, const U32 collideType );

   void emitParticles(const Point3F&, const Point3F&, const Point3F&, const U32);
   void updateSound();

   // Rendering
   void prepModelView    ( SceneState *state);
   bool prepRenderImage  ( SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState=false);
   void renderObject     ( SceneState *state, SceneRenderImage *image);

   // These are stolen from the player class ..
   bool pointInWater( const Point3F &point );        ///< Tests to see if a point is in water

   U32  packUpdate  (NetConnection *conn, U32 mask, BitStream *stream);
   void unpackUpdate(NetConnection *conn,           BitStream *stream);

public:
   F32 getUpdatePriority(CameraScopeQuery *focusObject, U32 updateMask, S32 updateSkips);

   Projectile();
   ~Projectile();

   DECLARE_CONOBJECT(Projectile);
   static void initPersistFields();

   virtual bool calculateImpact(float    simTime,
                                Point3F& pointOfImpact,
                                float&   impactTime);

public:
   static U32 smProjectileWarpTicks;

protected:
   static const U32 csmStaticCollisionMask;
   static const U32 csmDynamicCollisionMask;
   static const U32 csmDamageableMask;
};

#endif // _H_PROJECTILE

