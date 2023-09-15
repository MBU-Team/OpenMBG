//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#if defined(TORQUE_OS_MAC)
#if __MWERKS__ && (__MWERKS__ < 0x2430) // this should be safe for CW Pro 7.0
// this file crashes the CW6.3 compiler if opimizations greater than 1.
#if __option(global_optimizer)
// don't know which sub-area of the compiler it is as yet, so just set to 1.
#pragma optimization_level 1
#endif
#endif
#endif

#include "game/fx/particleEngine.h"

#include "dgl/dgl.h"
#include "sceneGraph/sceneGraph.h"
#include "sceneGraph/sceneState.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "math/mRandom.h"
#include "dgl/gTexManager.h"


//--------------------------------------------------------------------------
//-------------------------------------- Internal classes
//
struct Particle;
class PEngine
{
   // Interface for emitters.
   static const U32  csmBlockSize;
   Vector<Particle*> mAllocatedBlocks;
   Particle*         mFreeList;

  public:
   void updateParticles(Particle* particles, ParticleEmitter &emitter, const U32 ms);
   void updateSingleParticle(Particle* particle, ParticleEmitter &emitter, const U32 ms);

   Particle* allocateParticle(ParticleEmitter*);
   void      releaseParticle(Particle*);

  public:
   PEngine();
   ~PEngine();
};

const U32 PEngine::csmBlockSize = 512;

//--------------------------------------------------------------------------
//-------------------------------------- Internal global data
//
namespace {

	PEngine*   sgParticleEngine = NULL;
	MRandomLCG sgRandom(0x1);

} // namespace {}


#define MaxParticleSize 50.0
//--------------------------------------
class ParticleData : public SimDataBlock
{
   typedef SimDataBlock Parent;

   enum PDConst
   {
      PDC_MAX_TEX = 50,
   };

  public:
   F32   dragCoefficient;
   F32   windCoefficient;
   F32   gravityCoefficient;

   F32   inheritedVelFactor;
   F32   constantAcceleration;

   S32   lifetimeMS;
   S32   lifetimeVarianceMS;

   F32   spinSpeed;        // degrees per second
   F32   spinRandomMin;    
   F32   spinRandomMax;

   bool  useInvAlpha;

   bool  animateTexture;
   U32   numFrames;
   U32   framesPerSec;

   ColorF colors[ParticleEngine::PC_COLOR_KEYS];
   F32    sizes[ParticleEngine::PC_SIZE_KEYS];
   F32    times[4];

   StringTableEntry  textureNameList[PDC_MAX_TEX];
   TextureHandle     textureList[PDC_MAX_TEX];

  public:
   ParticleData();
   ~ParticleData();

   void initializeParticle(Particle*, const Point3F&);

   void packData(BitStream* stream);
   void unpackData(BitStream* stream);
   bool onAdd();
   bool preload(bool server, char errorBuffer[256]);
   DECLARE_CONOBJECT(ParticleData);
   static void  initPersistFields();
};


//--------------------------------------
struct Particle
{
   Point3F  pos;     // current instantaneous position
   Point3F  vel;     //   "         "         velocity
   Point3F  acc;     // Constant acceleration
   Point3F  orientDir;  // direction particle should go if using oriented particles

   U32           totalLifetime;   // Total ms that this instance should be "live"
   ParticleData* dataBlock;       // datablock that contains global parameters for
                                  //  this instance

   Particle* nextInList;   // Managed by the current owning emitter
   U32       currentAge;

   Particle*        nextInEngine; // Managed by the global engine object
   ParticleEmitter* currentOwner;
   ColorF           color;
   F32              size;
   F32              spinSpeed;
};



//--------------------------------------------------------------------------
//-------------------------------------- Datablock implementation
IMPLEMENT_CO_DATABLOCK_V1(ParticleEmitterData);

ParticleEmitterData::ParticleEmitterData()
{
   VECTOR_SET_ASSOCIATION(particleDataBlocks);
   VECTOR_SET_ASSOCIATION(dataBlockIds);

   ejectionPeriodMS = 100;    // 10 Particles Per second
   periodVarianceMS = 0;      // exactly

   ejectionVelocity = 2.0f;   // From 1.0 - 3.0 meters per sec
   velocityVariance = 1.0f;
   ejectionOffset   = 0.0f;   // ejection from the emitter point

   thetaMin         = 0.0f;   // All heights
   thetaMax         = 90.0f;  

   phiReferenceVel  = 0.0f;   // All directions
   phiVariance      = 360.0f;

   lifetimeMS           = 0;
   lifetimeVarianceMS   = 0;

   overrideAdvance  = false;
   orientParticles  = false;
   orientOnVelocity = true;
   useEmitterSizes  = false;
   useEmitterColors = false;
   particleString   = NULL;
}


IMPLEMENT_GETDATATYPE(ParticleEmitterData)
IMPLEMENT_SETDATATYPE(ParticleEmitterData)

void ParticleEmitterData::initPersistFields()
{
   Parent::initPersistFields();

   Con::registerType("ParticleEmitterDataPtr", TypeParticleEmitterDataPtr, sizeof(ParticleEmitterData*),
                     REF_GETDATATYPE(ParticleEmitterData),
                     REF_SETDATATYPE(ParticleEmitterData));

   addField("ejectionPeriodMS",     TypeS32,    Offset(ejectionPeriodMS,   ParticleEmitterData));
   addField("periodVarianceMS",     TypeS32,    Offset(periodVarianceMS,   ParticleEmitterData));
   addField("ejectionVelocity",     TypeF32,    Offset(ejectionVelocity,   ParticleEmitterData));
   addField("velocityVariance",     TypeF32,    Offset(velocityVariance,   ParticleEmitterData));
   addField("ejectionOffset",       TypeF32,    Offset(ejectionOffset,     ParticleEmitterData));
   addField("thetaMin",             TypeF32,    Offset(thetaMin,           ParticleEmitterData));
   addField("thetaMax",             TypeF32,    Offset(thetaMax,           ParticleEmitterData));
   addField("phiReferenceVel",      TypeF32,    Offset(phiReferenceVel,    ParticleEmitterData));
   addField("phiVariance",          TypeF32,    Offset(phiVariance,        ParticleEmitterData));
   addField("overrideAdvance",      TypeBool,   Offset(overrideAdvance,    ParticleEmitterData));
   addField("orientParticles",      TypeBool,   Offset(orientParticles,    ParticleEmitterData));
   addField("orientOnVelocity",     TypeBool,   Offset(orientOnVelocity,   ParticleEmitterData));
   addField("particles",            TypeString, Offset(particleString,     ParticleEmitterData));
   addField("lifetimeMS",           TypeS32,    Offset(lifetimeMS,         ParticleEmitterData));
   addField("lifetimeVarianceMS",   TypeS32,    Offset(lifetimeVarianceMS, ParticleEmitterData));
   addField("useEmitterSizes",      TypeBool,   Offset(useEmitterSizes,    ParticleEmitterData));
   addField("useEmitterColors",     TypeBool,   Offset(useEmitterColors,   ParticleEmitterData));
}

static ParticleEmitterData gDefaultEmitterData;

void ParticleEmitterData::packData(BitStream* stream)
{
   Parent::packData(stream);

   stream->writeInt(ejectionPeriodMS, 10);
   stream->writeInt(periodVarianceMS, 10);
   stream->writeInt((S32)(ejectionVelocity * 100), 16);
   stream->writeInt((S32)(velocityVariance * 100), 14);
   if(stream->writeFlag(ejectionOffset != gDefaultEmitterData.ejectionOffset))
      stream->writeInt((S32)(ejectionOffset * 100), 16);
   stream->writeRangedU32((U32)thetaMin, 0, 180);
   stream->writeRangedU32((U32)thetaMax, 0, 180);
   if(stream->writeFlag(phiReferenceVel != gDefaultEmitterData.phiReferenceVel))
      stream->writeRangedU32((U32)phiReferenceVel, 0, 360);
   if(stream->writeFlag(phiVariance != gDefaultEmitterData.phiVariance))
      stream->writeRangedU32((U32)phiVariance, 0, 360);
   stream->writeFlag(overrideAdvance);
   stream->writeFlag(orientParticles);
   stream->writeFlag(orientOnVelocity);
   stream->writeInt(lifetimeMS >> 5, 10);
   stream->writeInt(lifetimeVarianceMS >> 5, 10);
   stream->writeFlag(useEmitterSizes);
   stream->writeFlag(useEmitterColors);

   stream->write(dataBlockIds.size());
   for (U32 i = 0; i < dataBlockIds.size(); i++)
      stream->write(dataBlockIds[i]);
}

void ParticleEmitterData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);
   
   ejectionPeriodMS = stream->readInt(10);
   periodVarianceMS = stream->readInt(10);
   ejectionVelocity = stream->readInt(16) / 100.0f;
   velocityVariance = stream->readInt(14) / 100.0f;
   if(stream->readFlag())
      ejectionOffset = stream->readInt(16) / 100.0f;
   else
      ejectionOffset = gDefaultEmitterData.ejectionOffset;

   thetaMin = stream->readRangedU32(0, 180);
   thetaMax = stream->readRangedU32(0, 180);
   if(stream->readFlag())
      phiReferenceVel = stream->readRangedU32(0, 360);
   else
      phiReferenceVel = gDefaultEmitterData.phiReferenceVel;

   if(stream->readFlag())
      phiVariance = stream->readRangedU32(0, 360);
   else
      phiVariance = gDefaultEmitterData.phiVariance;

   overrideAdvance = stream->readFlag();
   orientParticles = stream->readFlag();
   orientOnVelocity = stream->readFlag();
   lifetimeMS = stream->readInt(10) << 5;
   lifetimeVarianceMS = stream->readInt(10) << 5;
   useEmitterSizes = stream->readFlag();
   useEmitterColors = stream->readFlag();
   
   U32 size;
   stream->read(&size);
   dataBlockIds.setSize(size);
   for (U32 i = 0; i < dataBlockIds.size(); i++)
      stream->read(&dataBlockIds[i]);
}

bool ParticleEmitterData::onAdd()
{
   if (Parent::onAdd() == false)
      return false;

//   if (overrideAdvance == true) {
//      Con::errorf(ConsoleLogEntry::General, "ParticleEmitterData: Not going to work.  Fix it!");
//      return false;
//   }

   // Validate the parameters...
   //
   if (ejectionPeriodMS < 1) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) period < 1 ms", getName());
      ejectionPeriodMS = 1;
   }
   if (periodVarianceMS >= ejectionPeriodMS) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) periodVariance >= period", getName());
      periodVarianceMS = ejectionPeriodMS - 1;
   }
   if (ejectionVelocity < 0.0f) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) ejectionVelocity < 0.0f", getName());
      ejectionVelocity = 0.0f;
   }
   if (velocityVariance > ejectionVelocity) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) velocityVariance > ejectionVelocity", getName());
      velocityVariance = ejectionVelocity;
   }
   if (ejectionOffset < 0.0f) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) ejectionOffset < 0", getName());
      ejectionOffset = 0.0f;
   }
   if (thetaMin < 0.0f) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) thetaMin < 0.0", getName());
      thetaMin = 0.0f;
   }
   if (thetaMax > 180.0f) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) thetaMax > 180.0", getName());
      thetaMax = 180.0f;
   }
   if (thetaMin > thetaMax) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) thetaMin > thetaMax", getName());
      thetaMin = thetaMax;
   }
   if (phiVariance < 0.0f || phiVariance > 360.0f) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) invalid phiVariance", getName());
      phiVariance = phiVariance < 0.0f ? 0.0f : 360.0f;
   }
   if (particleString == NULL && dataBlockIds.size() == 0) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) no particleString, invalid datablock", getName());
      return false;
   }
   if (particleString && particleString[0] == '\0') {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) no particleString, invalid datablock", getName());
      return false;
   }
   if (particleString && dStrlen(particleString) > 255) {
      Con::errorf(ConsoleLogEntry::General, "ParticleEmitterData(%s) particle string too long [> 255 chars]", getName());
      return false;
   }
   if (lifetimeMS < 0) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) lifetimeMS < 0.0f", getName());
      lifetimeMS = 0;
   }
   if (lifetimeVarianceMS > lifetimeMS ) {
      Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) lifetimeVarianceMS >= lifetimeMS", getName());
      lifetimeVarianceMS = lifetimeMS;
   }


   // Tokenize and load the particle datablocks...
   //
   if (particleString != NULL) {
      Vector<char*> dataBlocks(__FILE__, __LINE__);
      char* tokCopy = new char[dStrlen(particleString) + 1];
      dStrcpy(tokCopy, particleString);

      char* currTok = dStrtok(tokCopy, " \t");
      while (currTok != NULL) {
         dataBlocks.push_back(currTok);
         currTok = dStrtok(NULL, " \t");
      }
      if (dataBlocks.size() == 0) {
         Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) invalid particles string.  No datablocks found", getName());
         delete [] tokCopy;
         return false;
      }
      particleDataBlocks.clear();
      dataBlockIds.clear();

      for (U32 i = 0; i < dataBlocks.size(); i++) {
         ParticleData* pData = NULL;
         if (Sim::findObject(dataBlocks[i], pData) == false) {
            Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) unable to find particle datablock: %s", getName(), dataBlocks[i]);
         } else {
            particleDataBlocks.push_back(pData);
            dataBlockIds.push_back(pData->getId());
         }
      }
      delete [] tokCopy;
      if (particleDataBlocks.size() == 0) {
         Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) unable to find any particle datablocks", getName());
         return false;
      }
   }

   return true;
}

bool ParticleEmitterData::preload(bool server, char errorBuffer[256])
{
   if (Parent::preload(server, errorBuffer) == false)
      return false;

   particleDataBlocks.clear();
   for (U32 i = 0; i < dataBlockIds.size(); i++) {
      ParticleData* pData = NULL;
      if (Sim::findObject(dataBlockIds[i], pData) == false)
         Con::warnf(ConsoleLogEntry::General, "ParticleEmitterData(%s) unable to find particle datablock: %d", getName(), dataBlockIds[i]);
      else
         particleDataBlocks.push_back(pData);
   }

   return true;
}

//--------------------------------------------------------------------------
//--------------------------------------
IMPLEMENT_CO_DATABLOCK_V1(ParticleData);

ParticleData::ParticleData()
{
   dragCoefficient      = 0.0f;
   windCoefficient      = 1.0f;
   gravityCoefficient   = 0.0f;
   inheritedVelFactor   = 0.0f;
   constantAcceleration = 0.0f;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 0;
   spinSpeed            = 0.0;
   spinRandomMin        = 0.0;    
   spinRandomMax        = 0.0;
   useInvAlpha          = false;
   animateTexture       = false;

   numFrames            = 1;
   framesPerSec         = numFrames;

   S32 i;
   for( i=0; i<ParticleEngine::PC_COLOR_KEYS; i++ )
   {
      colors[i].set( 1.0, 1.0, 1.0, 1.0 );
   }

   for( i=0; i<ParticleEngine::PC_SIZE_KEYS; i++ )
   {
      sizes[i] = 1;
   }

   times[0] = 0.0f;
   times[1] = 1.0f;
   times[2] = 2.0f;
   times[3] = 2.0f;

   dMemset( textureNameList, 0, sizeof( textureNameList ) );
   dMemset( textureList, 0, sizeof( textureList ) );

}

static ParticleData gDefaultParticleData;


ParticleData::~ParticleData()
{
}

void ParticleData::initPersistFields()
{
   Parent::initPersistFields();

   addField("dragCoefficient",      TypeF32,    Offset(dragCoefficient,      ParticleData));   
   addField("windCoefficient",      TypeF32,    Offset(windCoefficient,      ParticleData));
   addField("gravityCoefficient",   TypeF32,    Offset(gravityCoefficient,   ParticleData));
   addField("inheritedVelFactor",   TypeF32,    Offset(inheritedVelFactor,   ParticleData));
   addField("constantAcceleration", TypeF32,    Offset(constantAcceleration, ParticleData));
   addField("lifetimeMS",           TypeS32,    Offset(lifetimeMS,           ParticleData));
   addField("lifetimeVarianceMS",   TypeS32,    Offset(lifetimeVarianceMS,   ParticleData));
   addField("spinSpeed",            TypeF32,    Offset(spinSpeed,            ParticleData));
   addField("spinRandomMin",        TypeF32,    Offset(spinRandomMin,        ParticleData));
   addField("spinRandomMax",        TypeF32,    Offset(spinRandomMax,        ParticleData));
   addField("useInvAlpha",          TypeBool,   Offset(useInvAlpha,          ParticleData));
   addField("animateTexture",       TypeBool,   Offset(animateTexture,       ParticleData));
   addField("framesPerSec",         TypeS32,    Offset(framesPerSec,         ParticleData));

   addField("textureName",          TypeFilename, Offset(textureNameList,      ParticleData));
   addField("animTexName",          TypeFilename, Offset(textureNameList,      ParticleData), PDC_MAX_TEX );

   // Interpolation variables
   addField("colors",               TypeColorF, Offset(colors,               ParticleData), ParticleEngine::PC_COLOR_KEYS);
   addField("sizes",                TypeF32,    Offset(sizes,                ParticleData), ParticleEngine::PC_SIZE_KEYS);
   addField("times",                TypeF32,    Offset(times,                ParticleData), 4);
}

void ParticleData::packData(BitStream* stream)
{
   Parent::packData(stream);

   stream->writeFloat(dragCoefficient / 5, 10);
   if(stream->writeFlag(windCoefficient != gDefaultParticleData.windCoefficient))
      stream->write(windCoefficient);
   stream->writeSignedFloat(gravityCoefficient / 10, 12);
   stream->writeFloat(inheritedVelFactor, 9);
   if(stream->writeFlag(constantAcceleration != gDefaultParticleData.constantAcceleration))
      stream->write(constantAcceleration);

   stream->writeInt(lifetimeMS >> 5, 10);
   stream->writeInt(lifetimeVarianceMS >> 5,10);
   if(stream->writeFlag(spinSpeed != gDefaultParticleData.spinSpeed))
      stream->write(spinSpeed);
   if(stream->writeFlag(spinRandomMin != gDefaultParticleData.spinRandomMin || spinRandomMax != gDefaultParticleData.spinRandomMax))
   {
      stream->writeInt((S32)(spinRandomMin + 1000), 11);
      stream->writeInt((S32)(spinRandomMax + 1000), 11);
   }
   stream->writeFlag(useInvAlpha);

   S32 i, count;

   // see how many frames there are:
   for(count = 0; count < 3; count++)
      if(times[count] >= 1)
         break;

   count++;

   stream->writeInt(count-1, 2);

   for( i=0; i<count; i++ )
   {
      stream->writeFloat( colors[i].red, 7);
      stream->writeFloat( colors[i].green, 7);
      stream->writeFloat( colors[i].blue, 7);
      stream->writeFloat( colors[i].alpha, 7);
      stream->writeFloat( sizes[i]/MaxParticleSize, 14);
      stream->writeFloat( times[i], 8);
   }

   for( count=0; count<PDC_MAX_TEX; count++ )
      if(!textureNameList[count])
         break;
   stream->writeInt(count, 6);
   for(i = 0; i < count; i++)
      stream->writeString( textureNameList[i] );
}

void ParticleData::unpackData(BitStream* stream)
{
   Parent::unpackData(stream);

   dragCoefficient = stream->readFloat(10) * 5;
   if(stream->readFlag())
      stream->read(&windCoefficient);
   else
      windCoefficient = gDefaultParticleData.windCoefficient;
   gravityCoefficient = stream->readSignedFloat(12) * 10;
   inheritedVelFactor = stream->readFloat(9);
   if(stream->readFlag())
      stream->read(&constantAcceleration);
   else
      constantAcceleration = gDefaultParticleData.constantAcceleration;

   lifetimeMS = stream->readInt(10) << 5;
   lifetimeVarianceMS = stream->readInt(10) << 5;
   if(stream->readFlag())
      stream->read(&spinSpeed);
   else
      spinSpeed = gDefaultParticleData.spinSpeed;

   if(stream->readFlag())
   {
      spinRandomMin = stream->readInt(11) - 1000;
      spinRandomMax = stream->readInt(11) - 1000;
   }
   else
   {
      spinRandomMin = gDefaultParticleData.spinRandomMin;
      spinRandomMax = gDefaultParticleData.spinRandomMax;
   }

   useInvAlpha = stream->readFlag();

   S32 i;
   S32 count = stream->readInt(2) + 1;
   for(i = 0;i < count; i++)
   {
      colors[i].red = stream->readFloat(7);
      colors[i].green = stream->readFloat(7);
      colors[i].blue = stream->readFloat(7);
      colors[i].alpha = stream->readFloat(7);
      sizes[i] = stream->readFloat(14) * MaxParticleSize;
      times[i] = stream->readFloat(8);
   }
   count = stream->readInt(6);
   for(i = 0; i < count;i ++)
      textureNameList[i] = stream->readSTString();
}

bool ParticleData::onAdd()
{
   if (Parent::onAdd() == false)
      return false;

   if (dragCoefficient < 0.0) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) drag coeff less than 0", getName());
      dragCoefficient = 0.0f;
   }
   if (lifetimeMS < 1) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) lifetime < 1 ms", getName());
      lifetimeMS = 1;
   }
   if (lifetimeVarianceMS >= lifetimeMS) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) lifetimeVariance >= lifetime", getName());
      lifetimeVarianceMS = lifetimeMS - 1;
   }
   if (spinSpeed > 10000.0 || spinSpeed < -10000.0) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinSpeed invalid", getName());
      return false;
   }
   if (spinRandomMin > 10000.0 || spinRandomMin < -10000.0) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinRandomMin invalid", getName());
      spinRandomMin = -360.0;
      return false;
   }
   if (spinRandomMin > spinRandomMax) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinRandomMin greater than spinRandomMax", getName());
      spinRandomMin = spinRandomMax - (spinRandomMin - spinRandomMax );
      return false;
   }
   if (spinRandomMax > 10000.0 || spinRandomMax < -10000.0) {
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) spinRandomMax invalid", getName());
      spinRandomMax = 360.0;
      return false;
   }
   if (numFrames > PDC_MAX_TEX){
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) numFrames invalid", getName());
      numFrames = PDC_MAX_TEX;
      return false;
   }
   if (framesPerSec > 200){
      Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) framesPerSec invalid", getName());
      framesPerSec = 20;
      return false;
   }

   times[0] = 0.0f;
   for (U32 i = 1; i < 4; i++) {
      if (times[i] < times[i-1]) {
         Con::warnf(ConsoleLogEntry::General, "ParticleData(%s) times[%d] < times[%d]", getName(), i, i-1);
         times[i] = times[i-1];
      }
   }

   return true;
}

bool ParticleData::preload(bool server, char errorBuffer[256])
{
   if (Parent::preload(server, errorBuffer) == false)
      return false;

   bool error = false;
   if(!server)
   {
      numFrames = 0;
      for( int i=0; i<PDC_MAX_TEX; i++ )
      {
         if( textureNameList[i] && textureNameList[i][0] )
         {
            textureList[i] = TextureHandle( textureNameList[i], MeshTexture );
            if (!textureList[i].getName())
            {
               dSprintf(errorBuffer, 256, "Missing particle texture: %s", textureNameList[i]);
               error = true;
            }
            numFrames++;
         }
      }
   }

   return !error;
}


void ParticleData::initializeParticle(Particle* init, const Point3F& inheritVelocity)
{
   init->dataBlock = this;

   // Calculate the constant accleration...
   init->vel += inheritVelocity * inheritedVelFactor;
   init->acc  = init->vel * constantAcceleration;

   // Calculate this instance's lifetime...
   init->totalLifetime = lifetimeMS;
   if (lifetimeVarianceMS != 0)
      init->totalLifetime += S32(sgRandom.randI() % (2 * lifetimeVarianceMS + 1)) - S32(lifetimeVarianceMS);

   // assign spin amount
   init->spinSpeed = spinSpeed + sgRandom.randF( spinRandomMin, spinRandomMax );
}


//----------------------------------------------------------------------------
//-------------------------------------- Emitter implementation
//
ParticleEmitter::ParticleEmitter()
{
   mDeleteWhenEmpty  = false;
   mDeleteOnTick     = false;

   mParticleListHead = NULL;

   mInternalClock    = 0;
   mNextParticleTime = 0;

   mLastPosition.set(0, 0, 0);
   mHasLastPosition = false;

   mLifetimeMS = 0;
   mElapsedTimeMS = 0;
}

ParticleEmitter::~ParticleEmitter()
{
   AssertFatal(mParticleListHead == NULL, "Error, particles remain in emitter after remove?");
}

//--------------------------------------------------------------------------
bool ParticleEmitter::onAdd()
{
   if(!Parent::onAdd())
      return false;
      
   removeFromProcessList();

   mLifetimeMS = mDataBlock->lifetimeMS;
   if( mDataBlock->lifetimeVarianceMS )
   {
      mLifetimeMS += S32( sgRandom.randI() % (2 * mDataBlock->lifetimeVarianceMS + 1)) - S32(mDataBlock->lifetimeVarianceMS );
   }

   return true;
}


//--------------------------------------------------------------------------
void ParticleEmitter::onRemove()
{
   Particle* pProbe = mParticleListHead;
   while (pProbe != NULL) {
      Particle* pRemove = pProbe;
      pProbe = pProbe->nextInList;

      pRemove->nextInList = NULL;
      sgParticleEngine->releaseParticle(pRemove);
   }
   mParticleListHead = NULL;

   if (mSceneManager != NULL) {
      gClientContainer.removeObject(this);
      gClientSceneGraph->removeObjectFromScene(this);
   }

   Parent::onRemove();
}


bool ParticleEmitter::onNewDataBlock(GameBaseData* dptr)
{
   mDataBlock = dynamic_cast<ParticleEmitterData*>(dptr);
   if (!mDataBlock || !Parent::onNewDataBlock(dptr))
      return false;

   scriptOnNewDataBlock();
   return true;
}


//--------------------------------------------------------------------------
bool ParticleEmitter::prepRenderImage(SceneState* state, const U32 stateKey,
                                       const U32 /*startZone*/, const bool /*modifyBaseState*/)
{
   if (isLastState(state, stateKey))
      return false;
   setLastState(state, stateKey);

   // This should be sufficient for most objects that don't manage zones, and
   //  don't need to return a specialized RenderImage...
   if (state->isObjectRendered(this)) {
      SceneRenderImage* image = new SceneRenderImage;
      image->obj = this;
      image->isTranslucent = true;
      image->sortType = SceneRenderImage::Point;
      state->setImageRefPoint(this, image);

      state->insertRenderImage(image);
   }

   return false;
}


struct SortParticle
{
   Particle* p;
   F32       k;
};

int QSORT_CALLBACK cmpSortParticles(const void* p1, const void* p2)
{
   const SortParticle* sp1 = (const SortParticle*)p1;
   const SortParticle* sp2 = (const SortParticle*)p2;

   if (sp2->k > sp1->k)
      return 1;
   else if (sp2->k == sp1->k)
      return 0;
   else
      return -1;
}

void ParticleEmitter::renderObject(SceneState* state, SceneRenderImage*)
{
   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on entry");

   RectI viewport;
   dglGetViewport(&viewport);

   // Uncomment this if this is a "simple" (non-zone managing) object
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   state->setupObjectProjection(this);

   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   dglMultMatrix(&mObjToWorld);

   MatrixF modelview;
   dglGetModelview(&modelview);
   Point3F x;
   Point3F y;
   modelview.getRow(0, &x);
   modelview.getRow(2, &y);

   Point3F viewvec;
   modelview.getRow(1, &viewvec);

   MatrixF camView;
   modelview.transposeTo( (F32*) &camView );

   // DMMFIX: slow!
   //
   Vector<SortParticle> orderedVector(__FILE__, __LINE__);
   Particle* pProbe = mParticleListHead;
   while (pProbe) {
      orderedVector.increment();
      orderedVector.last().p = pProbe;
      orderedVector.last().k = mDot(pProbe->pos, viewvec);
      pProbe = pProbe->nextInList;
   }
//   if (orderedVector.size() != 0)
//      dQsort(orderedVector.address(), orderedVector.size(), sizeof(SortParticle), cmpSortParticles);

   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   glDepthMask(GL_FALSE);


   if( orderedVector.size() > 0 && orderedVector[0].p->dataBlock->useInvAlpha )
   {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   }


   Point3F basePoints[4];
   basePoints[0] = Point3F(-1.0, 0.0, -1.0);
   basePoints[1] = Point3F( 1.0, 0.0, -1.0);
   basePoints[2] = Point3F( 1.0, 0.0,  1.0);
   basePoints[3] = Point3F(-1.0, 0.0,  1.0);

   F32 spinFactor = (1.0/1000.0) * (1.0/360.0) * M_PI * 2.0;


   for (U32 i = 0; i < orderedVector.size(); i++) {
      Particle* particle = orderedVector[i].p;

      if( particle->dataBlock->animateTexture )
      {
         U32 texNum = (U32)(particle->currentAge * (1.0/1000.0) * particle->dataBlock->framesPerSec);
         texNum %= particle->dataBlock->numFrames;
         glBindTexture(GL_TEXTURE_2D, particle->dataBlock->textureList[texNum].getGLName());
      }
      else
      {
         glBindTexture(GL_TEXTURE_2D, particle->dataBlock->textureList[0].getGLName());
      }

      if( mDataBlock->orientParticles )
      {
         renderOrientedParticle( *particle, state->getCameraPosition() );
      }
      else
      {
         renderBillboardParticle( *particle, basePoints, camView, spinFactor );
      }

   }
   
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glDepthMask(GL_TRUE);

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   dglSetViewport(viewport);

   AssertFatal(dglIsInCanonicalState(), "Error, GL not in canonical state on exit");
}

//--------------------------------------------------------------------------
inline void ParticleEmitter::renderBillboardParticle( Particle &part, Point3F *basePnts, 
                                                      MatrixF &camView, F32 spinFactor )
{

   glBegin(GL_QUADS);

      glColor4f(part.color.red,
                part.color.green,
                part.color.blue,
                part.color.alpha);


      F32 width     = part.size * 0.5;
      F32 spinAngle = part.spinSpeed * part.currentAge * spinFactor;

      F32 sy, cy;
      mSinCos(spinAngle, sy, cy);
      Point3F points[4];

      for( int i=0; i<4; i++ )
      {
         points[i].x = cy * basePnts[i].x - sy * basePnts[i].z;
         points[i].y = basePnts[i].y;
         points[i].z = sy * basePnts[i].x + cy * basePnts[i].z;
         camView.mulP( points[i] );
         points[i] *= width;
         points[i] += part.pos;
      }

      glTexCoord2f(0, 1);
      glVertex3fv(points[0]);
      glTexCoord2f(1, 1);
      glVertex3fv(points[1]);
      glTexCoord2f(1, 0);
      glVertex3fv(points[2]);
      glTexCoord2f(0, 0);
      glVertex3fv(points[3]);

   glEnd();
}

//--------------------------------------------------------------------------
inline void ParticleEmitter::renderOrientedParticle( Particle &part, const Point3F &camPos )
{
   Point3F dir;

   if( mDataBlock->orientOnVelocity )
   {
      // don't render oriented particle if it has no velocity
      if( part.vel.magnitudeSafe() == 0.0 ) return;
      dir = part.vel;
   }
   else
   {
      dir = part.orientDir;
   }

   Point3F dirFromCam = part.pos - camPos;
   Point3F crossDir;
   mCross( dirFromCam, dir, &crossDir );
   crossDir.normalize();
   dir.normalize();

   glBegin(GL_QUADS);

      glColor4f(part.color.red,
                part.color.green,
                part.color.blue,
                part.color.alpha);


      F32 width = part.size * 0.5;

      dir *= width;
      crossDir *= width;
      Point3F start = part.pos - dir;
      Point3F end = part.pos + dir;


      glTexCoord2f(0, 0);
      glVertex3fv( start + crossDir );

      glTexCoord2f(0, 1);
      glVertex3fv( start - crossDir );

      glTexCoord2f(1, 1);
      glVertex3fv( end - crossDir );

      glTexCoord2f(1, 0);
      glVertex3fv( end + crossDir );

   glEnd();
}


//--------------------------------------------------------------------------
void ParticleEmitter::stealParticle(Particle* steal)
{
   Particle** ppParticle = &mParticleListHead;
   while (*ppParticle) {
      if (*ppParticle == steal) {
         *ppParticle = (*ppParticle)->nextInList;
         steal->nextInList = NULL;
         return;
      }

      ppParticle = &((*ppParticle)->nextInList);
   }
   AssertFatal(false, "Trying to steal a particle that doesn't belong to this emitter!");
}

//--------------------------------------------------------------------------
void ParticleEmitter::setSizes( F32 *sizeList )
{
   for( int i=0; i<ParticleEngine::PC_SIZE_KEYS; i++ )
   {
      sizes[i] = sizeList[i];
   }
}

//--------------------------------------------------------------------------
void ParticleEmitter::setColors( ColorF *colorList )
{
   for( int i=0; i<ParticleEngine::PC_COLOR_KEYS; i++ )
   {
      colors[i] = colorList[i];
   }
}

//--------------------------------------------------------------------------
void ParticleEmitter::deleteWhenEmpty()
{
   mDeleteWhenEmpty = true;
}

void ParticleEmitter::emitParticles(const Point3F& point,
                                    const bool     useLastPosition,
                                    const Point3F& axis,
                                    const Point3F& velocity,
                                    const U32      numMilliseconds)
{
   // lifetime over - no more particles
   if( mLifetimeMS > 0 && mElapsedTimeMS > mLifetimeMS )
   {
      return;
   }

   Point3F realStart;
   if (useLastPosition && mHasLastPosition)
      realStart = mLastPosition;
   else
      realStart = point;

   emitParticles(realStart, point,
                 axis,
                 velocity,
                 numMilliseconds);
}

void ParticleEmitter::emitParticles(const Point3F& start,
                                    const Point3F& end,
                                    const Point3F& axis,
                                    const Point3F& velocity,
                                    const U32      numMilliseconds)
{
   // lifetime over - no more particles
   if( mLifetimeMS > 0 && mElapsedTimeMS > mLifetimeMS )
   {
      return;
   }

   U32 currTime = 0;
   bool particlesAdded = false;

   Point3F axisx;
   if (mFabs(axis.z) < 0.9f)
      mCross(axis, Point3F(0, 0, 1), &axisx);
   else
      mCross(axis, Point3F(0, 1, 0), &axisx);
   axisx.normalize();

   if (mNextParticleTime != 0) {
      // Need to handle next particle
      //
      if (mNextParticleTime > numMilliseconds) {
         // Defer to next update
         //  (Note that this introduces a potential spatial irregularity if the owning
         //   object is accelerating, and updating at a low frequency)
         //
         mNextParticleTime -= numMilliseconds;
         mInternalClock += numMilliseconds;
         mLastPosition = end;
         mHasLastPosition = true;
         return;
      } else {
         currTime       += mNextParticleTime;
         mInternalClock += mNextParticleTime;
         // Emit particle at curr time
         
         // Create particle at the correct position
         Point3F pos;
         pos.interpolate(start, end, F32(currTime) / F32(numMilliseconds));
         addParticle(pos, axis, velocity, axisx);
         particlesAdded = true;

         U32 advanceMS = numMilliseconds - currTime;
         if (advanceMS > mParticleListHead->totalLifetime) {
            // Well, shoot, why did we create this in the first place?
            Particle* old     = mParticleListHead;
            mParticleListHead = old->nextInList;
            old->nextInList   = NULL;
            sgParticleEngine->releaseParticle(old);
         } else {
            if (advanceMS != 0)
               sgParticleEngine->updateSingleParticle(mParticleListHead, *this, advanceMS);
         }
         mNextParticleTime = 0;
      }
   }

   while (currTime < numMilliseconds) {
      S32 nextTime = mDataBlock->ejectionPeriodMS;
      if (mDataBlock->periodVarianceMS != 0) {
         nextTime += S32(sgRandom.randI() % (2 * mDataBlock->periodVarianceMS + 1)) -
                     S32(mDataBlock->periodVarianceMS);
      }
      AssertFatal(nextTime > 0, "Error, next particle ejection time must always be greater than 0");

      if (currTime + nextTime > numMilliseconds) {
         mNextParticleTime = (currTime + nextTime) - numMilliseconds;
         mInternalClock   += numMilliseconds - currTime;
         AssertFatal(mNextParticleTime > 0, "Error, should not have deferred this particle!");
         break;
      }

      currTime       += nextTime;
      mInternalClock += nextTime;

      // Create particle at the correct position
      Point3F pos;
      pos.interpolate(start, end, F32(currTime) / F32(numMilliseconds));
      addParticle(pos, axis, velocity, axisx);
      particlesAdded = true;

      // NOTE: We are assuming that the just added particle is at the head of our
      //  list.  If that changes, so must this...
      U32 advanceMS = numMilliseconds - currTime;
      if (mDataBlock->overrideAdvance == false && advanceMS != 0) {
         if (advanceMS > mParticleListHead->totalLifetime) {
            // Well, shoot, why did we create this in the first place?
            Particle* old     = mParticleListHead;
            mParticleListHead = old->nextInList;
            old->nextInList   = NULL;
            sgParticleEngine->releaseParticle(old);
         } else {
            if (advanceMS != 0)
               sgParticleEngine->updateSingleParticle(mParticleListHead, *this, advanceMS);
         }
      }
   }

   // DMMFIX: Lame and slow...
   if (particlesAdded == true)
      updateBBox();
   if (mParticleListHead != NULL && mSceneManager == NULL) {
      gClientSceneGraph->addObjectToScene(this);
      gClientContainer.addObject(this);
      gClientProcessList.addObject(this);
   }

   mLastPosition = end;
   mHasLastPosition = true;
}

void ParticleEmitter::updateBBox()
{
   Point3F min(1e10,   1e10,  1e10);
   Point3F max(-1e10, -1e10, -1e10);

   Particle* pProbe = mParticleListHead;
   while (pProbe != NULL) {
      min.setMin(pProbe->pos);
      max.setMax(pProbe->pos);

      pProbe = pProbe->nextInList;
   }

   mObjBox = Box3F(min, max);
   MatrixF temp = getTransform();
   setTransform(temp);
}


//--------------------------------------------------------------------------

void ParticleEmitter::emitParticles(const Point3F& rCenter,
                                    const Point3F& rNormal,
                                    const F32      radius,
                                    const Point3F& velocity,
                                    S32 count)
{
   // lifetime over - no more particles
   if( mLifetimeMS > 0 && mElapsedTimeMS > mLifetimeMS )
   {
      return;
   }


   Point3F axisx, axisy;
   Point3F axisz = rNormal;

   if( axisz.isZero() )
   {
      axisz.set( 0.0, 0.0, 1.0 );
   }

   if (mFabs(axisz.z) < 0.98) {
      mCross(axisz, Point3F(0, 0, 1), &axisy);
      axisy.normalize();
   } else {
      mCross(axisz, Point3F(0, 1, 0), &axisy);
      axisy.normalize();
   }
   mCross(axisz, axisy, &axisx);
   axisx.normalize();

   // Should think of a better way to distribute the
   // particles within the hemisphere.
   for (S32 i = 0; i < count; i++) {
      Point3F pos = axisx * (radius * (1 - (2 * sgRandom.randF())));
      pos        += axisy * (radius * (1 - (2 * sgRandom.randF())));
      pos        += axisz * (radius * sgRandom.randF());

      Point3F axis = pos;
      axis.normalize();
      pos += rCenter;

      addParticle(pos, axis, velocity, axisz);
   }

   // Set world bounding box
   mObjBox.min = rCenter - Point3F(radius, radius, radius);
   mObjBox.max = rCenter + Point3F(radius, radius, radius);
   resetWorldBox();

   // Make sure we're part of the world
   if (mParticleListHead != NULL && mSceneManager == NULL) {
      gClientSceneGraph->addObjectToScene(this);
      gClientContainer.addObject(this);
      gClientProcessList.addObject(this);
   }
   mHasLastPosition = false;
}   


//--------------------------------------------------------------------------
void ParticleEmitter::addParticle(const Point3F& pos,
                                  const Point3F& axis,
                                  const Point3F& vel,
                                  const Point3F& axisx)
{
   Particle* pNew    = sgParticleEngine->allocateParticle(this);
   pNew->nextInList  = mParticleListHead;
   mParticleListHead = pNew;

   Point3F ejectionAxis = axis;
   F32 theta = (mDataBlock->thetaMax - mDataBlock->thetaMin) * sgRandom.randF() +
               mDataBlock->thetaMin;

   F32 ref  = (F32(mInternalClock) / 1000.0) * mDataBlock->phiReferenceVel;
   F32 phi  = ref + sgRandom.randF() * mDataBlock->phiVariance;

   // Both phi and theta are in degs.  Create axis angles out of them, and create the
   //  appropriate rotation matrix...
   AngAxisF thetaRot(axisx, theta * (M_PI / 180.0));
   AngAxisF phiRot(axis,    phi   * (M_PI / 180.0));

   MatrixF temp(true);
   thetaRot.setMatrix(&temp);
   temp.mulP(ejectionAxis);
   phiRot.setMatrix(&temp);
   temp.mulP(ejectionAxis);

   F32 initialVel = mDataBlock->ejectionVelocity;
   initialVel    += (mDataBlock->velocityVariance * 2.0f * sgRandom.randF()) - mDataBlock->velocityVariance;

   pNew->pos = pos + (ejectionAxis * mDataBlock->ejectionOffset);
   pNew->vel = ejectionAxis * initialVel;
   pNew->orientDir = ejectionAxis;
   pNew->acc.set(0, 0, 0);
   pNew->currentAge = 0;

   // Select a datablock for this particle
   U32 dBlockIndex = (U32)(mCeil(sgRandom.randF() * F32(mDataBlock->particleDataBlocks.size())) - 1);
   mDataBlock->particleDataBlocks[dBlockIndex]->initializeParticle(pNew, vel);
}


//--------------------------------------------------------------------------
void ParticleEmitter::processTick(const Move*)
{
   if (mDeleteOnTick == true)
      deleteObject();
}


void ParticleEmitter::advanceTime(F32 dt)
{
   Parent::advanceTime(dt);

   mElapsedTimeMS += (S32)(dt * 1000.0f);

   U32 numMSToUpdate = (U32)(dt * 1000.0f);
   if (numMSToUpdate == 0)
      return;

   Particle** ppProbe = &mParticleListHead;
   while (*ppProbe != NULL) {
      (*ppProbe)->currentAge += numMSToUpdate;
      if ((*ppProbe)->currentAge >= (*ppProbe)->totalLifetime) {
         // Remove this particle
         Particle* remove   = *ppProbe;
         *ppProbe           = remove->nextInList;
         remove->nextInList = NULL;
         sgParticleEngine->releaseParticle(remove);
      } else {
         ppProbe = &((*ppProbe)->nextInList);
      }
   }

   if (mParticleListHead == NULL && mDeleteWhenEmpty) {
      mDeleteOnTick = true;
   } else {
      if (numMSToUpdate != 0 && mParticleListHead)
         sgParticleEngine->updateParticles(mParticleListHead, *this, numMSToUpdate);
   }
}

//--------------------------------------------------------------------------
//--------------------------------------
//
namespace ParticleEngine {

Point3F windVelocity(0.f, 0.f, 0.f);

void init()
{
   AssertFatal(sgParticleEngine == NULL, "ParticleEngine::init: engine already initialized");

   sgParticleEngine = new PEngine;
}

void destroy()
{
   AssertFatal(sgParticleEngine != NULL, "ParticleEngine::destroy: engine not initialized");

   delete sgParticleEngine;
   sgParticleEngine = NULL;
}

} // namespace ParticleEngine

//--------------------------------------------------------------------------
PEngine::PEngine()
{
   mFreeList = NULL;
}


PEngine::~PEngine()
{
   mFreeList = NULL;
   for (U32 i = 0; i < mAllocatedBlocks.size(); i++)
   {
      delete [] mAllocatedBlocks[i];
      mAllocatedBlocks[i] = NULL;
   }
}

//--------------------------------------------------------------------------
Particle* PEngine::allocateParticle(ParticleEmitter* emitter)
{
   if (mFreeList == NULL)
   {
      // Add a new block to the free list...
      mAllocatedBlocks.push_back(new Particle[csmBlockSize]);
      Particle* pArray = mAllocatedBlocks.last();
      for (U32 i = 0; i < csmBlockSize - 1; i++)
         pArray[i].nextInEngine = &pArray[i + 1];
      pArray[csmBlockSize - 1].nextInEngine = NULL;
      mFreeList = &pArray[0];
   }
   AssertFatal(mFreeList != NULL, "Error, must have a free list here!");

   Particle* pParticle = mFreeList;
   mFreeList = pParticle->nextInEngine;

   dMemset(pParticle, 0, sizeof(Particle));
   pParticle->nextInEngine = NULL;
   pParticle->currentOwner = emitter;

   return pParticle;
}

void PEngine::releaseParticle(Particle* release)
{
   release->nextInEngine = mFreeList;
   mFreeList = release;
}


//--------------------------------------------------------------------------
void PEngine::updateParticles(Particle* particles, ParticleEmitter &emitter, const U32 ms)
{
   AssertFatal(particles != NULL, "PEngine::updateParticles: Error, must have particles to process in this function");
   AssertFatal(ms != 0, "PEngine::updateParticles: error, no time to update?");

   Particle* pProbe = particles;
   while (pProbe != NULL) {
      updateSingleParticle(pProbe, emitter, ms);
      pProbe = pProbe->nextInList;
   }
}

void PEngine::updateSingleParticle(Particle* particle, ParticleEmitter &emitter, const U32 ms)
{
   AssertFatal(particle != NULL, "PEngine::updateSingleParticle: Error, must have a particle to process in this function");
   AssertFatal(ms != 0, "PEngine::updateSingleParticle: error, no time to update?");

   F32 t = F32(ms) / 1000.0;

   Point3F a = particle->acc;
   a -= particle->vel        * particle->dataBlock->dragCoefficient;
   a -= ParticleEngine::windVelocity * particle->dataBlock->windCoefficient;
   a += Point3F(0, 0, -9.81) * particle->dataBlock->gravityCoefficient;

   particle->vel += a * t;
   particle->pos += particle->vel * t;

   // Now update the particle's color
   t = F32(particle->currentAge) / F32(particle->totalLifetime);
   AssertFatal(t <= 1.0f, "Out out bounds filter function for particle.");

   for (U32 i = 1; i < 4; i++) {
      if (particle->dataBlock->times[i] >= t) {
         F32 firstPart = t - particle->dataBlock->times[i-1];
         F32 total     = particle->dataBlock->times[i] -
                         particle->dataBlock->times[i-1];

         firstPart /= total;

         if( emitter.getDataBlock()->useEmitterColors )
         {
            particle->color.interpolate(emitter.colors[i-1], emitter.colors[i], firstPart);
         }
         else
         {
            particle->color.interpolate(particle->dataBlock->colors[i-1],
                                        particle->dataBlock->colors[i],
                                        firstPart);
         }

         if( emitter.getDataBlock()->useEmitterSizes )
         {
            particle->size = (emitter.sizes[i-1] * (1.0 - firstPart)) +
                             (emitter.sizes[i]   * firstPart);
         }
         else
         {
            particle->size = (particle->dataBlock->sizes[i-1] * (1.0 - firstPart)) +
                             (particle->dataBlock->sizes[i]   * firstPart);
         }

         return;
      }
   }
   particle->color = particle->dataBlock->colors[ParticleEngine::PC_COLOR_KEYS - 1];
   particle->size  = particle->dataBlock->sizes[ParticleEngine::PC_COLOR_KEYS - 1];
}


