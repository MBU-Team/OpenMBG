//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _CONSOLETYPES_H_
#define _CONSOLETYPES_H_

#ifndef Offset
#if defined(TORQUE_COMPILER_GCC) && (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define Offset(m,T) ((int)(&((T *)1)->m) - 1)
#else
#define Offset(x, cls) ((dsize_t)((const char *)&(((cls *)0)->x)-(const char *)0))
#endif
#endif

enum ConsoleDynamicTypes {

   //Registered in ConsoleTypes.cc
   TypeS8 = 0,
   TypeS32,
   TypeS32Vector,
   TypeBool,
   TypeBoolVector,
   TypeF32,
   TypeF32Vector,
   TypeString,
   TypeCaseString,
   TypeFilename,
   TypeEnum,
   TypeFlag,
   TypeColorI,
   TypeColorF,
   TypeSimObjectPtr,

   //Registered in MathTypes.cc
   TypePoint2I,
   TypePoint2F,
   TypePoint3F,
   TypePoint4F,
   TypeRectI,
   TypeRectF,
   TypeMatrixPosition,
   TypeMatrixRotation,
   TypeBox3F,
   
   //Registered in GuiTypes.cc
   TypeGuiProfile,

   // Game types
   TypeGameBaseDataPtr,
   TypeExplosionDataPtr,
   TypeShockwaveDataPtr,
   TypeSplashDataPtr,
   TypeEnergyProjectileDataPtr,
   TypeBombProjectileDataPtr,
   TypeParticleEmitterDataPtr,
   TypeAudioDescriptionPtr,
   TypeAudioProfilePtr,
   TypeTriggerPolyhedron,
   TypeProjectileDataPtr,
   TypeCannedChatItemPtr,
   TypeWayPointTeam,
   TypeDebrisDataPtr,
   TypeCommanderIconDataPtr,
   TypeDecalDataPtr,
   TypeEffectProfilePtr,
   TypeAudioEnvironmentPtr,
   TypeAudioSampleEnvironmentPtr,

   NumConsoleTypes
};

void RegisterCoreTypes(void);

#endif
