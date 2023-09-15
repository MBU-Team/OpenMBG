//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/simDictionary.h"
#include "console/simBase.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
extern S32 HashPointer(StringTableEntry e);

SimNameDictionary::SimNameDictionary()
{
   hashTable = NULL;
}

SimNameDictionary::~SimNameDictionary()
{
   delete[] hashTable;
}

void SimNameDictionary::insert(SimObject* obj)
{
   if(!obj->objectName)
      return;

   if(!hashTable)
   {
      hashTable = new SimObject *[DefaultTableSize];
      hashTableSize = DefaultTableSize;
      hashEntryCount = 0;
      S32 i;
      for(i = 0; i < hashTableSize; i++)
         hashTable[i] = NULL;
   }
   S32 idx = HashPointer(obj->objectName) % hashTableSize;
   obj->nextNameObject = hashTable[idx];
   hashTable[idx] = obj;
   hashEntryCount++;
   if(hashEntryCount > hashTableSize)
   {
      // resize the hash table
      S32 i;
      SimObject *head = NULL, *walk, *temp;
   	for(i = 0; i < hashTableSize; i++) {
   		walk = hashTable[i];
         while(walk)
         {
            temp = walk->nextNameObject;
            walk->nextNameObject = head;
            head = walk;
            walk = temp;
         }
   	}
      delete[] hashTable;
      hashTableSize = hashTableSize * 2 + 1;
      hashTable = new SimObject *[hashTableSize];
      
      for(i = 0; i < hashTableSize;i++)
         hashTable[i] = NULL;
      while(head)
      {
         temp = head->nextNameObject;
         idx = HashPointer(head->objectName) % hashTableSize;
         head->nextNameObject = hashTable[idx];
         hashTable[idx] = head;
         head = temp;
      }
   }
}

SimObject* SimNameDictionary::find(StringTableEntry name)
{
   // NULL is a valid lookup - it will always return NULL
   if(!hashTable)
      return NULL;
      
   S32 idx = HashPointer(name) % hashTableSize;
   SimObject *walk = hashTable[idx];
   while(walk)
   {
      if(walk->objectName == name)
         return walk;
      walk = walk->nextNameObject;
   }
   return NULL;
}

void SimNameDictionary::remove(SimObject* obj)
{
   if(!obj->objectName)
      return;

   SimObject **walk = &hashTable[HashPointer(obj->objectName) % hashTableSize];
   while(*walk)
   {
      if(*walk == obj)
      {
         *walk = obj->nextNameObject;
			obj->nextNameObject = (SimObject*)-1;
         hashEntryCount--;
         return;
      }
      walk = &((*walk)->nextNameObject);
   }
}	

//----------------------------------------------------------------------------

SimManagerNameDictionary::SimManagerNameDictionary()
{
   hashTable = new SimObject *[DefaultTableSize];
   hashTableSize = DefaultTableSize;
   hashEntryCount = 0;
   S32 i;
   for(i = 0; i < hashTableSize; i++)
      hashTable[i] = NULL;
}

SimManagerNameDictionary::~SimManagerNameDictionary()
{
   delete[] hashTable;
}

void SimManagerNameDictionary::insert(SimObject* obj)
{
   if(!obj->objectName)
      return;

   S32 idx = HashPointer(obj->objectName) % hashTableSize;
   obj->nextManagerNameObject = hashTable[idx];
   hashTable[idx] = obj;
   hashEntryCount++;
   if(hashEntryCount > hashTableSize)
   {
      // resize the hash table
      S32 i;
      SimObject *head = NULL, *walk, *temp;
   	for(i = 0; i < hashTableSize; i++) {
   		walk = hashTable[i];
         while(walk)
         {
            temp = walk->nextManagerNameObject;
            walk->nextManagerNameObject = head;
            head = walk;
            walk = temp;
         }
   	}
      delete[] hashTable;
      hashTableSize = hashTableSize * 2 + 1;
      hashTable = new SimObject *[hashTableSize];
      
      for(i = 0; i < hashTableSize;i++)
         hashTable[i] = NULL;
      while(head)
      {
         temp = head->nextManagerNameObject;
         idx = HashPointer(head->objectName) % hashTableSize;
         head->nextManagerNameObject = hashTable[idx];
         hashTable[idx] = head;
         head = temp;
      }
   }
}

SimObject* SimManagerNameDictionary::find(StringTableEntry name)
{
   // NULL is a valid lookup - it will always return NULL

   S32 idx = HashPointer(name) % hashTableSize;
   SimObject *walk = hashTable[idx];
   while(walk)
   {
      if(walk->objectName == name)
         return walk;
      walk = walk->nextManagerNameObject;
   }
   return NULL;
}

void SimManagerNameDictionary::remove(SimObject* obj)
{
   if(!obj->objectName)
      return;

   SimObject **walk = &hashTable[HashPointer(obj->objectName) % hashTableSize];
   while(*walk)
   {
      if(*walk == obj)
      {
         *walk = obj->nextManagerNameObject;
			obj->nextManagerNameObject = (SimObject*)-1;
         hashEntryCount--;
         return;
      }
      walk = &((*walk)->nextManagerNameObject);
   }
}	

//--------------------------------------------------------------------------- 
//--------------------------------------------------------------------------- 

SimIdDictionary::SimIdDictionary()
{
   for(S32 i = 0; i < DefaultTableSize; i++)
      table[i] = NULL;
}

SimIdDictionary::~SimIdDictionary()
{
}

void SimIdDictionary::insert(SimObject* obj)
{
   S32 idx = obj->getId() & TableBitMask;
   obj->nextIdObject = table[idx];
   table[idx] = obj;
}

SimObject* SimIdDictionary::find(S32 id)
{
   S32 idx = id & TableBitMask;
   SimObject *walk = table[idx];
   while(walk)
   {
      if(walk->getId() == U32(id))
         return walk;
      walk = walk->nextIdObject;
   }
   return NULL;
}

void SimIdDictionary::remove(SimObject* obj)
{
   SimObject **walk = &table[obj->getId() & TableBitMask];
   while(*walk && *walk != obj)
      walk = &((*walk)->nextIdObject);
   if(*walk)
      *walk = obj->nextIdObject;
}

//--------------------------------------------------------------------------- 
//--------------------------------------------------------------------------- 

