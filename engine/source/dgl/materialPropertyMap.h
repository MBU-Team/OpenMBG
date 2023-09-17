//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MATERIALPROPERTYMAP_H_
#define _MATERIALPROPERTYMAP_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _SIMBASE_H_
#include "console/simBase.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif

class MaterialProperty : public SimObject
{
    typedef SimObject Parent;
    
public:
    StringTableEntry  name;
    StringTableEntry  detailMapName;
    StringTableEntry  environMapName;
    float             environMapFactor;
    F32 friction;
    F32 restitution;
    F32 force;
    S32               sound;
    ColorF            puffColor[2];

public:
    MaterialProperty();
    ~MaterialProperty();
    static void initPersistFields();

    DECLARE_CONOBJECT(MaterialProperty);
};

class MaterialPropertyMap : public SimObject
{
   typedef SimObject Parent;

  public:

   struct MapEntry {
       const char* name;
       MaterialProperty* property;
   };

  public:
   MaterialPropertyMap();
   ~MaterialPropertyMap();
   

   const MaterialProperty* getMapEntry(StringTableEntry) const;
   const MaterialProperty* getMapEntryFromIndex(S32 index) const;
   S32 getIndexFromName(StringTableEntry name) const;

   DECLARE_CONOBJECT(MaterialPropertyMap);

   // Should only be used by console functions
  public:
   bool addMapping(const char* name, MaterialProperty* prop);
   
   //-------------------------------------- Data
  private:
   Vector<MapEntry>  mMapEntries;
};

#endif  // _H_MATERIALPROPERTYMAPPING_
