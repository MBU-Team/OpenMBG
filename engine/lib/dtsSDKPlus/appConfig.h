//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPCONFIG_H_
#define DTSAPPCONFIG_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "DTSUtil.h"
#include <fstream>

namespace DTS
{
   class AppNode;

   class AppConfig
   {
      static AppConfig * smConfig;

      F32 mAnimationDelta;
      F32 mSameVertTOL;
      F32 mSameTVertTOL;
      F32 mSameNormTOL;
      F32 mWeightThreshhold;
      S32 mWeightsPerVertex;
      F32 mCyclicSequencePadding;
      F32 mAppFramesPerSec;

      bool mEnableSequences;
      bool mExportOptimized;
      bool mAllowUnusedMeshes;
      bool mAllowCollapseTransform;
      bool mNoMipMap;
      bool mNoMipMapTranslucent;
      bool mZapBorder;
      U32  mDumpConfig;
      char * mErrorString;
      std::ofstream mDumpFile;

      std::vector<char *> mBoolParamNames;
      std::vector<U32 *> mBoolParams;
      std::vector<U32> mBoolParamBit;
      std::vector<char *> mFloatParamNames;
      std::vector<F32 *> mFloatParams;
      std::vector<char *> mIntParamNames;
      std::vector<S32 *> mIntParams;
      std::vector<char *> mStringParamNames;
      std::vector<char *> mStringParams;
      std::vector<S32> mStringParamMaxLen;

      std::vector<char *> mAlwaysExport;
      std::vector<char *> mNeverExport;
      std::vector<char *> mNeverAnimate;

      virtual void printDump(U32 mask, const char * str);
      virtual bool setDumpFile(const char * path, const char * name = NULL);
      virtual bool closeDumpFile();

      virtual bool alwaysExport(AppNode *);
      virtual bool neverExport(AppNode *);
      virtual bool neverAnimate(AppNode *);

      bool readConfigFile(const char * filename, const char * ext);
      void writeConfigFile(const char * filename);

      void setInitialDefaults();
      S32 getParamEntry(const char * name, std::vector<char *> & nameTable);
      void setupConfigParams();
      void clearConfigParams();
      void clearConfigLists();

      // error handling
      void setExportError(const char * str) { if (str && !mErrorString) mErrorString=strnew(str); else if (!str) { delete [] mErrorString; mErrorString=NULL; }}
      bool isExportError() { return mErrorString!=NULL; }
      const char * getExportError() { return mErrorString; }

      public:

      AppConfig();
      ~AppConfig();

      void setConfig(AppConfig * config);

      // access configuration parameters statically...

      static bool SetDumpFile(const char * path, const char * name = NULL) { return smConfig->setDumpFile(path,name); }
      static bool CloseDumpFile() { return smConfig->closeDumpFile(); }
      static void PrintDump(U32 mask, const char * str) { smConfig->printDump(mask,str); }
      static U32  GetDumpMask()          { return smConfig->mDumpConfig; }

      static bool AlwaysExport(AppNode * node) { return smConfig->alwaysExport(node); }
      static bool NeverExport(AppNode * node) { return smConfig->neverExport(node); }
      static bool NeverAnimate(AppNode * node) { return smConfig->neverAnimate(node); }

      static void SetExportError(const char * str) { smConfig->setExportError(str); }
      static bool IsExportError() { return smConfig->isExportError(); }
      static const char * GetExportError() { return smConfig->getExportError(); }

      static bool GetEnableSequences()   { return smConfig->mEnableSequences; }
      static bool GetExportOptimized()   { return smConfig->mExportOptimized; }
      static bool GetAllowUnusedMeshes() { return smConfig->mAllowUnusedMeshes; }
      static bool GetAllowCollapse() { return smConfig->mAllowCollapseTransform; }
      static bool GetNoMipMap() { return smConfig->mNoMipMap; }
      static bool GetNoMipMapTranslucent() { return smConfig->mNoMipMapTranslucent; }
      static bool GetZapBorder() { return smConfig->mZapBorder; }
      static F32  AnimationDelta() { return smConfig->mAnimationDelta; }
      static F32  SameVertTOL() { return smConfig->mSameVertTOL; }
      static F32  SameNormTOL() { return smConfig->mSameNormTOL; }
      static F32  SameTVertTOL() { return smConfig->mSameVertTOL; }
      static F32  WeightThreshhold() { return smConfig->mWeightThreshhold; }
      static S32  WeightsPerVertex() { return smConfig->mWeightsPerVertex; }
      static F32  CyclicSequencePadding() { return smConfig->mCyclicSequencePadding; }
      static F32  AppFramesPerSec() { return smConfig->mAppFramesPerSec; }

      static void SetDefaults() { smConfig->setInitialDefaults(); }
      static bool ReadConfigFile(const char * filename, const char * ext) { return smConfig->readConfigFile(filename,ext); }
      static void WriteConfigFile(const char * filename) { smConfig->writeConfigFile(filename); }
   };

   // enum for printDump
   enum
   {
      PDPass1              = 1 << 0, // collect useful nodes
      PDPass2              = 1 << 1, // put together shape structure
      PDPass3              = 1 << 2, // cull un-needed nodes
      PDObjectOffsets      = 1 << 3, // display object offset transform during 2nd pass
      PDNodeStates         = 1 << 4, // display as added
      PDObjectStates       = 1 << 5, // ""
      PDNodeStateDetails   = 1 << 6, // details of above
      PDObjectStateDetails = 1 << 7, // ""
      PDSequences          = 1 << 8,
      PDShapeHierarchy     = 1 << 9,
      PDAlways             = 0xFFFFFFFF
   };

};


#endif // DTSAPPMATERIAL_H_
