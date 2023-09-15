//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMMUTEX_H_
#define _PLATFORMMUTEX_H_

struct Mutex
{
   static void* createMutex( void );
   static void destroyMutex( void* );
   static void lockMutex( void* );
   static void unlockMutex( void* );
};

#endif
