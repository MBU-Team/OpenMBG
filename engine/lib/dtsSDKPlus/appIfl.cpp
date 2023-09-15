//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "appIfl.h"
#include "DTSUtil.h"
#include <fstream>

namespace DTS
{

   AppIfl::AppIfl(const char * fullPath)
   {
      mIflFile = strnew(fullPath);

      // load in duration and names

      std::ifstream is;
      is.open(fullPath);

      char buffer[256];
      char name[256];
      S32 duration;
      while (is.good() && !is.eof())
      {
         is.getline(buffer,sizeof(buffer));
         S32 num = sscanf(buffer,"%s %i",name,&duration);
         if (num==1)
         {
            mNames.push_back(strnew(name));
            mDurations.push_back(AppTime(1.0f/30.0f,0));
         }
         else if (num==2)
         {
            mNames.push_back(strnew(name));
            mDurations.push_back(AppTime(F32(duration)/30.0f,0));
         }
      }
   }

   AppTime AppIfl::getStartTime() { return AppTime(0,0); }


   AppIfl::~AppIfl()
   {
      delete [] mIflFile;
      for (S32 i=0; i<mNames.size(); i++)
         delete [] mNames[i];
   }

}; // namespace DTS

