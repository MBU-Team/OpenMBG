//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/simBase.h"
#include "core/stringTable.h"
#include "console/console.h"
#include "core/fileStream.h"
#include "core/resManager.h"
#include "core/fileObject.h"
#include "console/consoleInternal.h"
#include "core/idGenerator.h"

//---------------------------------------------------------------------------
//--------------------------------------------------------------------------- 

// We comment out the implementation of the Con namespace when doxygenizing because
// otherwise Doxygen decides to ignore our docs in console.h
#ifndef DOXYGENIZING

namespace Sim
{


//--------------------------------------------------------------------------- 

//--------------------------------------------------------------------------- 
// event queue variables:

SimTime gCurrentTime;
SimTime gTargetTime;

SimEvent *gEventQueue;
U32 gEventSequence;

//--------------------------------------------------------------------------- 
// event queue init/shutdown

static void initEventQueue()
{
   gCurrentTime = 0;
   gTargetTime = 0;
   gEventSequence = 1;
   gEventQueue = NULL;
}

static void shutdownEventQueue()
{
	// Delete all pending events
   SimEvent *walk = gEventQueue;
   while(walk)
   {
      SimEvent *temp = walk->nextEvent;
      delete walk;
      walk = temp;
   }
}

//--------------------------------------------------------------------------- 
// event post

U32 postEvent(SimObject *destObject, SimEvent* event,U32 time)
{
	AssertFatal(time >= gCurrentTime,
		"Sim::postEvent: Cannot go back in time. (flux capacitor unavailable -- BJG)");
   AssertFatal(destObject, "Destination object for event doesn't exist.");

   event->time = time;
   event->destObject = destObject;

   if(!destObject)
   {
      delete event;
      return InvalidEventId;
   }
   event->sequenceCount = gEventSequence++;
   SimEvent **walk = &gEventQueue;
   SimEvent *current;
   
   while((current = *walk) != NULL && (current->time < event->time))
      walk = &(current->nextEvent);   
   event->nextEvent = current;
   *walk = event;
   return event->sequenceCount;
}

//--------------------------------------------------------------------------- 
// event cancellation

void cancelEvent(U32 eventSequence)
{
   SimEvent **walk = &gEventQueue;
   SimEvent *current;
   
   while((current = *walk) != NULL)
   {
      if(current->sequenceCount == eventSequence)
      {
         *walk = current->nextEvent;
         delete current;
         return;
      }
      else
         walk = &(current->nextEvent);
   }
}

void cancelPendingEvents(SimObject *obj)
{
   SimEvent **walk = &gEventQueue;
   SimEvent *current;
   
   while((current = *walk) != NULL)
   {
      if(current->destObject == obj)
      {
         *walk = current->nextEvent;
         delete current;
      }
      else
         walk = &(current->nextEvent);
   }
}

//--------------------------------------------------------------------------- 
// event pending test

bool isEventPending(U32 eventSequence)
{
   for(SimEvent *walk = gEventQueue; walk; walk = walk->nextEvent)
      if(walk->sequenceCount == eventSequence)
         return true;
   return false;
}


//--------------------------------------------------------------------------- 
// event timing

void advanceToTime(SimTime targetTime)
{
   AssertFatal(targetTime >= gCurrentTime, "EventQueue::process: cannot advance to time in the past.");

   gTargetTime = targetTime;
   while(gEventQueue && gEventQueue->time <= targetTime)
   {
      SimEvent *event = gEventQueue;
      gEventQueue = gEventQueue->nextEvent;
      AssertFatal(event->time >= gCurrentTime, 
			"SimEventQueue::pop: Cannot go back in time (flux capacitor not installed - BJG).");
      gCurrentTime = event->time;
      SimObject *obj = event->destObject;
      
      if(!obj->isDeleted())
         event->process(obj);
      delete event;
   }
	gCurrentTime = targetTime;
}

void advanceTime(SimTime delta)
{
   advanceToTime(gCurrentTime + delta);
}

U32 getCurrentTime()
{
   return gCurrentTime;
}

U32 getTargetTime()
{
   return gTargetTime;
}

//--------------------------------------------------------------------------- 
//--------------------------------------------------------------------------- 

SimGroup *gRootGroup = NULL;
SimManagerNameDictionary *gNameDictionary;
SimIdDictionary *gIdDictionary;
U32 gNextObjectId;

static void initRoot()
{
   gIdDictionary = new SimIdDictionary;
   gNameDictionary = new SimManagerNameDictionary;
   
   gRootGroup = new SimGroup();
   gRootGroup->setId(RootGroupId);
   gRootGroup->assignName("RootGroup");
   gRootGroup->registerObject();

   gNextObjectId = DynamicObjectIdFirst;
}

static void shutdownRoot() 
{
   gRootGroup->deleteObject();

   delete gNameDictionary;
   delete gIdDictionary;
}

//--------------------------------------------------------------------------- 

SimObject* findObject(const char* name)
{
   SimObject *obj;
   char c = *name;
   if(c == '/')
      return gRootGroup->findObject(name + 1 );
   if(c >= '0' && c <= '9')
   {
      // it's an id group
      const char* temp = name + 1;
      for(;;)
      {
         c = *temp++;
         if(!c)
            return findObject(dAtoi(name));
         else if(c == '/')
         {
            obj = findObject(dAtoi(name));
            if(!obj)
               return NULL;
            return obj->findObject(temp);
         }
      }
   }
   S32 len;
   
   for(len = 0; name[len] != 0 && name[len] != '/'; len++)
      ;
   StringTableEntry stName = StringTable->lookupn(name, len);
   if(!stName)
      return NULL;
   obj = gNameDictionary->find(stName);
   if(!name[len])
      return obj;
   if(!obj)
      return NULL;
   return obj->findObject(name + len + 1);
}   

SimObject* findObject(SimObjectId id)
{
	return gIdDictionary->find(id);
}

SimGroup *getRootGroup()
{
   return gRootGroup;
}

//--------------------------------------------------------------------------- 
//--------------------------------------------------------------------------- 

#define InstantiateNamedSet(set) g##set = new SimSet; g##set->registerObject(#set); gRootGroup->addObject(g##set)
#define InstantiateNamedGroup(set) g##set = new SimGroup; g##set->registerObject(#set); gRootGroup->addObject(g##set)

SimDataBlockGroup *gDataBlockGroup;
SimDataBlockGroup *getDataBlockGroup()
{
   return gDataBlockGroup;
}


void init()
{
   initEventQueue();
   initRoot();

   InstantiateNamedSet(ActiveActionMapSet);
   InstantiateNamedSet(GhostAlwaysSet);
   InstantiateNamedSet(LightSet);
   InstantiateNamedSet(WayPointSet);
   InstantiateNamedSet(fxReplicatorSet);
   InstantiateNamedSet(fxFoliageSet);
   InstantiateNamedGroup(ActionMapGroup);
   InstantiateNamedGroup(ClientGroup);
   InstantiateNamedGroup(GuiGroup);
   InstantiateNamedGroup(GuiDataGroup);
   InstantiateNamedGroup(TCPGroup);
   InstantiateNamedGroup(ClientConnectionGroup);
   gDataBlockGroup = new SimDataBlockGroup();
   gDataBlockGroup->registerObject("DataBlockGroup");
   gRootGroup->addObject(gDataBlockGroup);
}

void shutdown()
{
   shutdownRoot();
   shutdownEventQueue();
}

}


#endif // DOXYGENIZING.

SimDataBlockGroup::SimDataBlockGroup()
{
   mLastModifiedKey = 0;
}

S32 QSORT_CALLBACK SimDataBlockGroup::compareModifiedKey(const void* a,const void* b)
{
	return (reinterpret_cast<const SimDataBlock* >(a))->getModifiedKey() -
		(reinterpret_cast<const SimDataBlock*>(b))->getModifiedKey();
}


void SimDataBlockGroup::sort()
{
   if(mLastModifiedKey != SimDataBlock::getNextModifiedKey())
   {
      mLastModifiedKey = SimDataBlock::getNextModifiedKey();
    	dQsort(objectList.address(),objectList.size(),sizeof(SimObject *),compareModifiedKey);
   }
}

//--------------------------------------------------------------------------- 
//--------------------------------------------------------------------------- 

bool SimObject::registerObject()
{
	mFlags.clear(Deleted | Removed);

   if(!mId)
      mId = Sim::gNextObjectId++;

   Sim::gIdDictionary->insert(this);	

   Sim::gNameDictionary->insert(this);

	// Notify object
   bool ret = onAdd();

   if(!ret)
      unregisterObject();

   AssertFatal(!ret || isProperlyAdded(), "Object did not call SimObject::onAdd()");
   return ret;
}

//--------------------------------------------------------------------------- 

void SimObject::unregisterObject()
{
   mFlags.set(Removed);
   
    // Notify object first
    onRemove();

    // Clear out any pending notifications before
    // we call our own, just in case they delete
    // something that we have referenced.
    clearAllNotifications();

    // Notify all objects that are waiting for delete
    // messages
    if (getGroup())
        getGroup()->removeObject(this);

    processDeleteNotifies();

    // Do removals from the Sim.
    Sim::gNameDictionary->remove(this);
    Sim::gIdDictionary->remove(this);
    Sim::cancelPendingEvents(this);
}

//--------------------------------------------------------------------------- 

void SimObject::deleteObject()
{
	AssertFatal(mFlags.test(Added),
		"SimObject::deleteObject: Object not registered.");
	AssertFatal(!isDeleted(),"SimManager::deleteObject: "
		"Object has already been deleted");
	AssertFatal(!isRemoved(),"SimManager::deleteObject: "
		"Object in the process of being removed");
	mFlags.set(Deleted);

   unregisterObject();
   delete this;
}

//--------------------------------------------------------------------------- 


void SimObject::setId(SimObjectId newId)
{
   if(!mFlags.test(Added))
   {
      mId = newId;
      return;
   }
   
   // get this object out of the id dictionary if it's in it
   Sim::gIdDictionary->remove(this);

	// Free current Id.
	// Assign new one.
   mId = newId ? newId : Sim::gNextObjectId++;
	Sim::gIdDictionary->insert(this);
}   

void SimObject::assignName(const char *name)
{
   StringTableEntry newName = NULL;
   if(name[0])
      newName = StringTable->insert(name);
      
   if(mGroup)
      mGroup->nameDictionary.remove(this);
   if(mFlags.test(Added))
      Sim::gNameDictionary->remove(this);
      
   objectName = newName;
   
   if(mGroup)
      mGroup->nameDictionary.insert(this);
   if(mFlags.test(Added))
      Sim::gNameDictionary->insert(this);
}

//--------------------------------------------------------------------------- 

bool SimObject::registerObject(U32 id)
{
   setId(id);
   return registerObject();
}

bool SimObject::registerObject(const char *name)
{
   assignName(name);
   return registerObject();
}

bool SimObject::registerObject(const char *name, U32 id)
{
   setId(id);
   assignName(name);
   return registerObject();
}
