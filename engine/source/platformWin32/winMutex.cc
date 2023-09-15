//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "platform/platformMutex.h"

void * Mutex::createMutex()
{
   CRITICAL_SECTION * mutex = new CRITICAL_SECTION;
   InitializeCriticalSection(mutex);
   return((void*)mutex);
}

void Mutex::destroyMutex(void * mutex)
{
   AssertFatal(mutex, "Mutex::destroyMutex: invalid mutex");
   DeleteCriticalSection((CRITICAL_SECTION*)mutex);
   delete mutex;
}

void Mutex::lockMutex(void * mutex)
{
   AssertFatal(mutex, "Mutex::lockMutex: invalid mutex");
   EnterCriticalSection((CRITICAL_SECTION*)mutex);
}

void Mutex::unlockMutex(void * mutex)
{
   AssertFatal(mutex, "Mutex::unlockMutex: invalid mutex");
   LeaveCriticalSection((CRITICAL_SECTION*)mutex);
}
