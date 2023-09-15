//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _OBJECTTYPES_H_
#define _OBJECTTYPES_H_

// Types used for SimObject type masks (SimObject::mTypeMask)
//

/* NB!  If a new object type is added, don't forget to add it to the
 *      consoleInit function in simBase.cc
 */

enum SimObjectTypes
{
   #define bit(x) (1 << (x))

   /// @name Types used by the SceneObject class
   /// @{
   DefaultObjectType =           0,
   StaticObjectType =            bit(0),
   /// @}

   /// @name Basic Engine Types
   /// @{
   EnvironmentObjectType =       bit(1),
   TerrainObjectType =           bit(2),
   InteriorObjectType =          bit(3),
   WaterObjectType =             bit(4),
   TriggerObjectType =           bit(5),
   MarkerObjectType =            bit(6),
   UNUSED_AVAILABLE =            bit(7),
   UNUSED_AVAILABLE2 =           bit(8),
   DecalManagerObjectType =      bit(9),
   /// @}

   /// @name Game Types
   /// @{
   GameBaseObjectType =          bit(10),
   ShapeBaseObjectType =         bit(11),
   CameraObjectType =            bit(12),
   StaticShapeObjectType =       bit(13),
   PlayerObjectType =            bit(14),
   ItemObjectType =              bit(15),
   VehicleObjectType =           bit(16),
   VehicleBlockerObjectType =    bit(17),
   ProjectileObjectType =        bit(18),
   ExplosionObjectType  =        bit(19),
   CorpseObjectType =            bit(20),
   DebrisObjectType =            bit(22),
   PhysicalZoneObjectType =      bit(23),
   StaticTSObjectType =          bit(24),
   UNUSED_AVAILABLE3 =           bit(25),
   StaticRenderedObjectType =    bit(26),
   /// @}

   /// @name Other
   /// The following are allowed types that can be set on datablocks for static shapes
   /// @{
   DamagableItemObjectType =     bit(27),
   /// @}
};

#define STATIC_COLLISION_MASK   (   TerrainObjectType    |  \
                                    InteriorObjectType   |  \
                                    StaticObjectType )      \

#define DAMAGEABLE_MASK  ( PlayerObjectType        |  \
                           VehicleObjectType       |  \
                           DamagableItemObjectType )  \

#endif
