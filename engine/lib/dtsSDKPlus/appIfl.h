//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPIFL_H_
#define DTSAPPIFL_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "appMesh.h"

namespace DTS
{
   class AppIfl
   {
      char * mIflFile;
      std::vector<AppTime> mDurations;
      std::vector<char *> mNames;

      public:

      // Standard AppIfl just needs a path name to the ifl
      // Can derive from this class, though, in order to support
      // Ifls in a more application dependent manner, if desired.
      AppIfl(const char * fullPath);
      virtual ~AppIfl();

      const char * getFilename() { return mIflFile; }
      const std::vector<AppTime> & getDurations() { return mDurations; }
      const std::vector<char*> & getNames() { return mNames; }
      virtual AppTime getStartTime();
   };
};

#endif // DTSAPPIFL_H_

