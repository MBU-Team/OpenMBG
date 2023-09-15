//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSMAYANODE_H_
#define DTSMAYANODE_H_

#include "appnode.h"
#include <maya/MDagPath.h>

namespace DTS
{
   class MayaAppNode : public AppNode
   {
      typedef AppNode Parent;

      MDagPath mPath;

      void buildMeshList();
      void buildChildList();

      public:

      MayaAppNode(MDagPath & path);
      ~MayaAppNode();

      Matrix<4,4,F32> getNodeTransform(const AppTime & time);

      bool isEqual(AppNode *);

      bool animatesTransform(const AppSequenceData & seqData);

      const char * getName();
      const char * getParentName();

      bool getFloat(const char * propName, F32  & defaultVal);
      bool   getInt(const char * propName, S32  & defaultVal);
      bool  getBool(const char * propName, bool & defaultVal);

      bool isParentRoot();
   };

}; // namespace DTS

#endif // DTSMAYANODE_H_

