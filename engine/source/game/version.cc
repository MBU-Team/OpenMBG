//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "game/version.h"

static const U32 csgVersionNumber = 1200;

U32 getVersionNumber()
{
   return csgVersionNumber;
}

const char* getVersionString()
{
   static char buffer[10];
   dSprintf(buffer, sizeof(buffer), "%5.3f", F32(csgVersionNumber)/1000.0f);
   return buffer;
}   

const char* getCompileTimeString()
{
   return __DATE__ " at " __TIME__;
}   


