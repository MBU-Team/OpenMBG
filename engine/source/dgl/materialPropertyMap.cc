//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/materialPropertyMap.h"
#include "console/consoleTypes.h"

ConsoleFunction( addMaterialMapping, bool, 3, 3, "(string matName, matprop) Set up a material mapping. See MaterialPropertyMap for details.")
{
   MaterialPropertyMap* pMap = static_cast<MaterialPropertyMap*>(Sim::findObject("MaterialPropertyMap"));
   if (pMap == NULL) {
      Con::errorf(ConsoleLogEntry::General, "Error, cannot find the global material map object");
      return false;
   }
   
   SimObject* obj = static_cast<SimObject*>(Sim::findObject(argv[2]));
   if (obj == NULL) {
       Con::errorf(0, "addMaterialMapping: Could not find MaterialProperty %s", argv[2]);
       return false;
   }
   MaterialProperty* matprop = dynamic_cast<MaterialProperty*>(obj);
   if (matprop == NULL) {
       Con::errorf(0, "addMaterialMapping: Could not find MaterialProperty %s", argv[2]);
	   return false;
   }
   return pMap->addMapping(argv[1], matprop);


   // return pMap->addMapping(argc - 1, argv + 1);
}

IMPLEMENT_CONOBJECT(MaterialPropertyMap);
MaterialPropertyMap::MaterialPropertyMap()
{
   VECTOR_SET_ASSOCIATION(mMapEntries);
}

MaterialPropertyMap::~MaterialPropertyMap()
{

}

const MaterialProperty* MaterialPropertyMap::getMapEntry(StringTableEntry name) const
{
   // DMMNOTE: Really slow.  Shouldn't be a problem since these are one time scans
   //  for each object, but might want to replace this with a hash table
   //
   const MaterialProperty* ret = NULL;
   for (S32 i = 0; i < mMapEntries.size(); i++) {
      if (dStricmp(mMapEntries[i].name, name) == 0) {
         ret = mMapEntries[i].property;
         break;
      }
   }

   return ret;
}

const MaterialProperty* MaterialPropertyMap::getMapEntryFromIndex(S32 index) const
{
   const MaterialProperty* ret = NULL;
   if(index < mMapEntries.size())
      ret = mMapEntries[index].property;
   return ret;
}

S32 MaterialPropertyMap::getIndexFromName(StringTableEntry name) const
{
   S32 ret = -1;
   for (S32 i = 0; i < mMapEntries.size(); i++) {
      if (dStricmp(mMapEntries[i].name, name) == 0) {
         ret = i;
         break;
      }
   }
   return ret;
}

bool MaterialPropertyMap::addMapping(const char* name, MaterialProperty* prop)
{
   const char* matName = StringTable->insert(name);

   S32 idx = getIndexFromName(matName);
   if (idx != -1) {
       Con::warnf("MaterialPropertyMap::addMapping: Overwriting default material properties");
       mMapEntries[idx].property = prop;
   }
   else 
   {
       MapEntry e;
       e.name = matName;
       e.property = prop;
       mMapEntries.push_back(e);
   }

   return true;
}

IMPLEMENT_CONOBJECT(MaterialProperty);

MaterialProperty::MaterialProperty()
{
    name = "DefaultMaterial";
    detailMapName = 0;
    environMapName = 0;
    environMapFactor = 1;
    sound = -1;
	puffColor[0] = ColorF(0, 0, 0, 1);
	puffColor[1] = ColorF(0, 0, 0, 1);
    friction = 1;
    restitution = 1;
    force = 0;
}

MaterialProperty::~MaterialProperty()
{
}

void MaterialProperty::initPersistFields()
{
    addField("detailMap", TypeString, Offset(detailMapName, MaterialProperty));
    addField("environmentMap", TypeString, Offset(environMapName, MaterialProperty));
    addField("environmentMapFactor", TypeF32, Offset(environMapFactor, MaterialProperty));
    addField("friction", TypeF32, Offset(friction, MaterialProperty));
    addField("restitution", TypeF32, Offset(restitution, MaterialProperty));
    addField("force", TypeF32, Offset(force, MaterialProperty));
    addField("sound", TypeS32, Offset(sound, MaterialProperty));
    addField("puffColor", 13, Offset(puffColor, MaterialProperty), 2);
}
