//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSMAXAPPNODE_H_
#define DTSMAXAPPNODE_H_

#include "appNode.h"

class INode;
class Matrix3;

namespace DTS
{

   class MaxAppNode : public AppNode
   {
      typedef AppNode Parent;

      INode * mMaxNode;

      void buildMeshList();
      void buildChildList();

      public:

      MaxAppNode(INode * maxNode);

      bool isEqual(AppNode *);

      Matrix<4,4,F32> getNodeTransform(const AppTime & time);
      bool animatesTransform(const AppSequenceData & seqData);
      const char * getName();
      const char * getParentName();
      bool isParentRoot();
      bool getFloat(const char * propName, F32  & defaultVal);
      bool   getInt(const char * propName, S32  & defaultVal);
      bool  getBool(const char * propName, bool & defaultVal);
   };

   extern Matrix<4,4,F32> convertFromMaxMatrix(Matrix3 & mat);

};

#endif // DTSMAXAPPNODE_H_

