//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/materialPropertyMap.h"

ConsoleFunction( addMaterialMapping, bool, 2, 99, "(string matName, ...) Set up a material mapping. See MaterialPropertyMap for details.")
{
   MaterialPropertyMap* pMap = static_cast<MaterialPropertyMap*>(Sim::findObject("MaterialPropertyMap"));
   if (pMap == NULL) {
      Con::errorf(ConsoleLogEntry::General, "Error, cannot find the global material map object");
      return false;
   }

   return pMap->addMapping(argc - 1, argv + 1);
}

IMPLEMENT_CONOBJECT(MaterialPropertyMap);
MaterialPropertyMap::MaterialPropertyMap()
{
   VECTOR_SET_ASSOCIATION(mMapEntries);
}

MaterialPropertyMap::~MaterialPropertyMap()
{

}

const MaterialPropertyMap::MapEntry* MaterialPropertyMap::getMapEntry(StringTableEntry name) const
{
   // DMMNOTE: Really slow.  Shouldn't be a problem since these are one time scans
   //  for each object, but might want to replace this with a hash table
   //
   const MapEntry* ret = NULL;
   for (S32 i = 0; i < mMapEntries.size(); i++) {
      if (dStricmp(mMapEntries[i].name, name) == 0) {
         ret = &mMapEntries[i];
         break;
      }
   }

   return ret;
}

const MaterialPropertyMap::MapEntry* MaterialPropertyMap::getMapEntryFromIndex(S32 index) const
{
   const MapEntry* ret = NULL;
   if(index < mMapEntries.size())
      ret = &mMapEntries[index];
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

MaterialPropertyMap::MapEntry* MaterialPropertyMap::getNCMapEntry(StringTableEntry name)
{
   return const_cast<MapEntry*>(getMapEntry(name));
}

bool MaterialPropertyMap::addMapping(const S32 argc, const char** argv)
{
   const char* matName = StringTable->insert(argv[0]);

   MapEntry* pEntry = getNCMapEntry(matName);
   if (pEntry != NULL) {
      Con::warnf(ConsoleLogEntry::General, "Warning, overwriting material properties for: %s", matName);
   } else {
      mMapEntries.increment();
      pEntry = &mMapEntries.last();
      pEntry->sound          = -1;
      pEntry->puffColor[0].set(0.0f, 0.0f, 0.0f);
      pEntry->puffColor[1].set(0.0f, 0.0f, 0.0f);
   }

   pEntry->name           = matName;
   pEntry->detailMapName  = NULL;
   pEntry->environMapName = NULL;
   pEntry->matType        = Default;
   pEntry->matFlags       = 0;

   for (U32 i = 1; S32(i) < argc; i++) {
      const char* param = argv[i];

      if (dStrnicmp(param, "detail:", dStrlen("detail:")) == 0) {
         // Set the detail map
         const char* pColon = dStrchr(param, ':');
         pColon++;
         while (*pColon == ' ' || *pColon == '\t')
            pColon++;

         pEntry->detailMapName = StringTable->insert(pColon);
      }
      else if (dStrnicmp(param, "environment:", dStrlen("environment:")) == 0) {
         // Set the detail map
         const char* pColon = dStrchr(param, ':');
         pColon++;
         while (*pColon == ' ' || *pColon == '\t')
            pColon++;

         const char* start = pColon;
         while (*pColon != ' ')
            pColon++;
         const char* end = pColon;
         pColon++;

         char buffer[256];
         dStrncpy(buffer, start, end - start);
         buffer[end - start] = '\0';

         pEntry->environMapName   = StringTable->insert(buffer);
         pEntry->environMapFactor = dAtof(pColon);
      }
      else if (dStrnicmp(param, "color:", dStrlen("color:")) == 0) {
         const char* curChar = dStrchr(param, ':');
         curChar++;
         while (*curChar == ' ' || *curChar == '\t')
            curChar++;

         char buffer[5][256];
         S32 index = 0;
         for(S32 x = 0; x < 5; ++x, index = 0)
         {
            while(*curChar != ' ' && *curChar != '\0')
               buffer[x][index++] = *curChar++;
            buffer[x][index++] = '\0';
            while(*curChar == ' ')
               ++curChar;
         }
         pEntry->puffColor[0].set(dAtof(buffer[0]), dAtof(buffer[1]), dAtof(buffer[2]), dAtof(buffer[3]));                        
         pEntry->puffColor[1].set(dAtof(buffer[0]), dAtof(buffer[1]), dAtof(buffer[2]), dAtof(buffer[4]));                        
      }
      else if (dStrnicmp(param, "sound:", dStrlen("sound:")) == 0) {
         // Set the detail map
         const char* pColon = dStrchr(param, ':');
         pColon++;
         while (*pColon == ' ' || *pColon == '\t')
            pColon++;

         const char* start = pColon;
         while(*pColon != ' ' && *pColon != '\0')
            pColon++;
         const char* end = pColon;
         pColon++;

         char buffer[256];
         dStrncpy(buffer, start, end - start);
         buffer[end - start] = '\0';

         pEntry->sound = dAtoi(buffer);
      }
      else if (param[0] == '\0') {
         // Empty statement allowed, does nothing
      }
      else {
         Con::warnf(ConsoleLogEntry::General, "Warning, misunderstood material parameter: %s in materialEntry %s", param, matName);
      }
   }

   return true;
}
