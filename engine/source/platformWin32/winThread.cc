//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platformThread.h"
#include "platformWin32/platformWin32.h"
#include "platform/platformSemaphore.h"

//--------------------------------------------------------------------------
struct WinThreadData
{
   ThreadRunFunction       mRunFunc;
   S32                     mRunArg;
   Thread *                mThread;
   void *                  mSemaphore;

   WinThreadData()
   {
      mRunFunc    = 0;
      mRunArg     = 0;
      mThread     = 0;
      mSemaphore  = 0;
   };
};

//--------------------------------------------------------------------------
Thread::Thread(ThreadRunFunction func, S32 arg, bool start_thread)
{
   WinThreadData * threadData = new WinThreadData();
   threadData->mRunFunc = func;
   threadData->mRunArg = arg;
   threadData->mThread = this;
   threadData->mSemaphore = Semaphore::createSemaphore();

   mData = reinterpret_cast<void*>(threadData);
   if (start_thread)
      start();
}

Thread::~Thread()
{
   join();

   WinThreadData * threadData = reinterpret_cast<WinThreadData*>(mData);
   Semaphore::destroySemaphore(threadData->mSemaphore);
   delete threadData;
}

static DWORD WINAPI ThreadRunHandler(void * arg)
{
   WinThreadData * threadData = reinterpret_cast<WinThreadData*>(arg);

   threadData->mThread->run(threadData->mRunArg);
   Semaphore::releaseSemaphore(threadData->mSemaphore);

   return(0);
}

void Thread::start()
{
   if(isAlive())
      return;

   WinThreadData * threadData = reinterpret_cast<WinThreadData*>(mData);
   Semaphore::acquireSemaphore(threadData->mSemaphore);

   DWORD threadID;
   CreateThread(0, 0, ThreadRunHandler, mData, 0, &threadID);
}

bool Thread::join()
{
   if(!isAlive())
      return(false);

   WinThreadData * threadData = reinterpret_cast<WinThreadData*>(mData);
   return(Semaphore::acquireSemaphore(threadData->mSemaphore));
}

void Thread::run(S32 arg)
{
   WinThreadData * threadData = reinterpret_cast<WinThreadData*>(mData);
   if(threadData->mRunFunc)
      threadData->mRunFunc(arg);
}

bool Thread::isAlive()
{
   WinThreadData * threadData = reinterpret_cast<WinThreadData*>(mData);
   
   bool signal = Semaphore::acquireSemaphore(threadData->mSemaphore, false);
   if(signal)
      Semaphore::releaseSemaphore(threadData->mSemaphore);
   return(!signal);
}
