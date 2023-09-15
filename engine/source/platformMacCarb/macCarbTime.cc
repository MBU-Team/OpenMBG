//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "PlatformMacCarb/platformMacCarb.h"
#include <time.h>
#include <timer.h>

#pragma message("time code needs eval -- may not be accurate in all cases, and might cause performance hit.")

//--------------------------------------
void Platform::getLocalTime(LocalTime &lt)
{
   struct tm *systime;
   time_t long_time;

   time( &long_time );                // Get time as long integer.
   systime = localtime( &long_time ); // Convert to local time.
   
   lt.sec      = systime->tm_sec;      
   lt.min      = systime->tm_min;      
   lt.hour     = systime->tm_hour;     
   lt.month    = systime->tm_mon;    
   lt.monthday = systime->tm_mday; 
   lt.weekday  = systime->tm_wday;  
   lt.year     = systime->tm_year;     
   lt.yearday  = systime->tm_yday;  
   lt.isdst    = systime->tm_isdst;    
}   

U32 Platform::getTime()
{
   time_t long_time;
   time( &long_time );
   return long_time;
}   

U32 GetMilliseconds()
{
   UnsignedWide time;
   Microseconds(&time);
   return (time.hi*5000000) + (time.lo/1000);
}

U32 Platform::getRealMilliseconds()
{
   UnsignedWide time;
   Microseconds(&time);
   return (time.hi*5000000) + (time.lo/1000);
}   

U32 Platform::getVirtualMilliseconds()
{
   return platState.currentTime;   
}   

void Platform::advanceTime(U32 delta)
{
   platState.currentTime += delta;
}   

