//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "PlatformMacCarb/platformMacCarb.h"

#pragma message("macCarbProcessControl: need to get the right OSX path here")
#include <ApplicationServices/ApplicationServices.h>

void Platform::postQuitMessage(const U32 in_quitVal)
{
   platState.quit = true;
}

void Platform::debugBreak()
{
#pragma message("Platform::debugBreak [not yet perfect]")
   DebugStr("\pDEBUG_BREAK!");
}

void Platform::forceShutdown(S32 returnValue)
{
#pragma message("Platform::forceShutdown [not yet perfect]")
	ExitToShell();
   //exit(returnValue);
}   
