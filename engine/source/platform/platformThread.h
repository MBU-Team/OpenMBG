//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _PLATFORMTHREAD_H_
#define _PLATFORMTHREAD_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/types.h"
#endif

typedef void (*ThreadRunFunction)(S32);

class Thread
{
   protected:
      void *      mData;

   public:
      Thread(ThreadRunFunction func = 0, S32 arg = 0, bool start_thread = true);
      virtual ~Thread();

      void start();
      bool join();

      virtual void run(S32 arg = 0);

      bool isAlive();
};

#endif
