//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _H_PARTICLEEMITTER
#define _H_PARTICLEEMITTER

#ifndef _GAMEBASE_H_
#include "game/gameBase.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

//-------------------------------------- Engine initialization...
//
namespace ParticleEngine {

   enum ParticleConsts
   {
      PC_COLOR_KEYS = 4,
      PC_SIZE_KEYS = 4,
   };

   /// Initalize the particle engine
   void init();
   
   /// Destroy the particle engine
   void destroy();
   
   extern Point3F windVelocity; ///< Global wind velocity for all particles
   
   /// Sets the wind velocity for all particles
   /// @param   vel   Velocity
   inline void setWindVelocity(const Point3F & vel) { windVelocity = vel; }
   
   /// Returns the wind velocity
   inline Point3F getWindVelocity() { return windVelocity; }
}


//--------------------------------------------------------------------------
//-------------------------------------- The data and the Emitter class
//                                        are all that the game should deal
//                                        with (other than initializing the
//                                        global engine pointer of course)
//
struct Particle;
class  ParticleData;

//--------------------------------------
class ParticleEmitterData : public GameBaseData {
   typedef GameBaseData Parent;

  public:
   ParticleEmitterData();
   DECLARE_CONOBJECT(ParticleEmitterData);
   static void initPersistFields();
   void packData(BitStream* stream);
   void unpackData(BitStream* stream);
   bool preload(bool server, char errorBuffer[256]);

   bool onAdd();

  public:
   S32   ejectionPeriodMS;                   ///< Time, in Miliseconds, between particle ejection
   S32   periodVarianceMS;                   ///< Varience in ejection peroid between 0 and n

   F32   ejectionVelocity;                   ///< Ejection velocity
   F32   velocityVariance;                   ///< Variance for velocity between 0 and n
   F32   ejectionOffset;                     ///< Z offset from emitter point to eject from

   F32   thetaMin;                           ///< Minimum angle, from the horizontal plane, to eject from
   F32   thetaMax;                           ///< Maximum angle, from the horizontal plane, to eject from

   F32   phiReferenceVel;                    ///< Reference angle, from the verticle plane, to eject from
   F32   phiVariance;                        ///< Varience from the reference angle, from 0 to n

   U32   lifetimeMS;                         ///< Lifetime of particles
   U32   lifetimeVarianceMS;                 ///< Varience in lifetime from 0 to n

   bool  overrideAdvance;                    ///< 
   bool  orientParticles;                    ///< Particles always face the screen
   bool  orientOnVelocity;                   ///< Particles face the screen at the start
   bool  useEmitterSizes;                    ///< Use emitter specified sizes instead of datablock sizes
   bool  useEmitterColors;                   ///< Use emitter specified colors instead of datablock colors

   StringTableEntry      particleString;     ///< Used to load particle data directly from a string
   Vector<ParticleData*> particleDataBlocks; ///< Datablocks for particle emissions
   Vector<U32>           dataBlockIds;       ///< Datablock IDs which corospond to the particleDataBlocks
};


//--------------------------------------
class ParticleEmitter : public GameBase
{
   typedef GameBase Parent;
   friend class PEngine;

  public:
   ParticleEmitter();
   ~ParticleEmitter();

   /// Sets sizes of particles based on sizelist provided
   /// @param   sizeList   List of sizes
   void setSizes( F32 *sizeList );
   
   /// Sets colors for particles based on color list provided
   /// @param   colorList   List of colors
   void setColors( ColorF *colorList );
   
   ParticleEmitterData *getDataBlock(){ return mDataBlock; }
   bool onNewDataBlock(GameBaseData* dptr);

   /// By default, a particle renderer will wait for it's owner to delete it.  When this
   /// is turned on, it will delete itself as soon as it's particle count drops to zero.
   void deleteWhenEmpty(); 

   /// @name Particle Emission
   /// Main interface for creating particles.  The emitter does _not_ track changes
   ///  in axis or velocity over the course of a single update, so this should be called
   ///  at a fairly fine grain.  The emitter will potentially track the last particle
   ///  to be created into the next call to this function in order to create a uniformly
   ///  random time distribution of the particles.  If the object to which the emitter is
   ///  attached is in motion, it should try to ensure that for call (n+1) to this
   ///  function, start is equal to the end from call (n).  This will ensure a uniform
   ///  spatial distribution.
   /// @{

   void emitParticles(const Point3F& start,
                      const Point3F& end,
                      const Point3F& axis,
                      const Point3F& velocity,
                      const U32      numMilliseconds);
   void emitParticles(const Point3F& point,
                      const bool     useLastPosition,
                      const Point3F& axis,
                      const Point3F& velocity,
                      const U32      numMilliseconds);
   void emitParticles(const Point3F& rCenter,
                      const Point3F& rNormal,
                      const F32      radius,
                      const Point3F& velocity,
                      S32 count);
   /// @}

  protected:
   /// @name Internal interface
   /// @{

   /// Adds a particle
   /// @param   pos   Initial position of particle
   /// @param   axis   
   /// @param   vel   Initial velocity
   /// @param   axisx   
   void addParticle(const Point3F &pos, const Point3F &axis, const Point3F &vel, const Point3F &axisx);
   
   /// Renders a particle facing the camera with a spin factor
   /// @param   part   Particle
   /// @param   basePnts   Base points for the quad the particle is rendered on
   /// @param   camView   Camera view matrix
   /// @param   spinFactor   0.0-1.0 modifyer for 
   void renderBillboardParticle( Particle &part, Point3F *basePnts, MatrixF &camView, F32 spinFactor );
   
   /// Renders a particle which will face the camera but spin itself to look
   /// like it is facing a particular velocity.
   /// @param   part   Particle
   /// @param   camPos   Camera position
   void renderOrientedParticle( Particle &part, const Point3F &camPos );
   
   /// Updates the bounding box for the particle system
   void updateBBox();
   
   /// @}
  protected:
   bool onAdd();
   void onRemove();

   void processTick(const Move *move);
   void advanceTime(F32 dt);

   // Rendering
  protected:
   bool prepRenderImage(SceneState *state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState);
   void renderObject(SceneState *state, SceneRenderImage *image);

   // PEngine interface
  private:
   /// Removes the provided particle from the system and lets the caller of the
   /// function assume control of it
   /// @param   part   Particle
   void stealParticle(Particle *part);

  private:
   ParticleEmitterData* mDataBlock;

   Particle* mParticleListHead;

   U32       mInternalClock;

   U32       mNextParticleTime;

   Point3F   mLastPosition;
   bool      mHasLastPosition;

   bool      mDeleteWhenEmpty;
   bool      mDeleteOnTick;

   S32       mLifetimeMS;
   S32       mElapsedTimeMS;

   F32       sizes[ParticleEngine::PC_SIZE_KEYS];
   ColorF    colors[ParticleEngine::PC_COLOR_KEYS];
};

#endif // _H_PARTICLEEMITTER

