//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "maxAppNode.h"
#include "maxAppMesh.h"
#include "DTSUtil.h"

#pragma pack(push,8)
#include <max.h>
#pragma pack(pop)

namespace DTS
{

   MaxAppNode::MaxAppNode(INode * maxNode)
   {
      mMaxNode = maxNode;
   }

   void MaxAppNode::buildMeshList()
   {
      ::ObjectState os = mMaxNode->EvalWorldState(0);
      if (os.obj->CanConvertToType(triObjectClassID))
         mMeshes.push_back(new MaxAppMesh(mMaxNode,this));
   }

   void MaxAppNode::buildChildList()
   {
      for (S32 i=0; i<mMaxNode->NumberOfChildren(); i++)
      {
         INode * child = mMaxNode->GetChildNode(i);
         mChildNodes.push_back(new MaxAppNode(child));
      }
   }

   bool MaxAppNode::isEqual(AppNode * node)
   {
      MaxAppNode * maxAppNode = dynamic_cast<MaxAppNode*>(node);
      return maxAppNode && (maxAppNode->mMaxNode == mMaxNode);
   }

   Matrix<4,4,F32> MaxAppNode::getNodeTransform(const AppTime & time)
   {
      // Get the node marix then copy from max format into our matrix struct
      // Note that this results in a transpose of the matrix...
      Matrix3 nodeMat = mMaxNode->GetNodeTM( SecToTicks(time.getF32()) );
      return convertFromMaxMatrix(nodeMat);
   }

   bool MaxAppNode::animatesTransform(const AppSequenceData & seqData)
   {
      Interval range(SecToTicks(seqData.startTime.getF32()),SecToTicks(seqData.endTime.getF32()));
      Interval test = range;

      // does this sequence animate the bounds node, if so, add ground transform
      S32 midpoint = (range.Start() + range.End()) / 2;
      mMaxNode->GetNodeTM(midpoint,&test);
      return ( test.Start()!=range.Start() || test.End()!=range.End() );
   }

   const char * MaxAppNode::getName()
   {
      if (!mName)
         mName = strnew(mMaxNode->GetName());
      return mName;
   }

   const char * MaxAppNode::getParentName()
   {
      if (!mParentName)
         mParentName = mMaxNode->GetParentNode() ? strnew(mMaxNode->GetParentNode()->GetName()) : strnew("ROOT");
      return mParentName;
   }

   bool MaxAppNode::isParentRoot()
   {
      return (mMaxNode->GetParentNode()==NULL) || mMaxNode->GetParentNode()->IsRootNode();
   }

   bool MaxAppNode::getFloat(const char * propName, F32  & defaultVal)
   {
      // don't trust max not to touch value...
      F32 val;
      if (mMaxNode->GetUserPropFloat(propName,val))
      {
         defaultVal = val;
         return true;
      }
      return false;
   }

   bool MaxAppNode::getInt(const char * propName, S32  & defaultVal)
   {
      // don't trust max not to touch value...
      S32 val;
      if (mMaxNode->GetUserPropInt(propName,val))
      {
         defaultVal = val;
         return true;
      }
      return false;
   }

   bool  MaxAppNode::getBool(const char * propName, bool & defaultVal)
   {
      // don't trust max not to touch value...
      BOOL val;
      if (mMaxNode->GetUserPropBool(propName,val))
      {
         defaultVal = val ? true : false;
         return true;
      }
      return false;
   }

   Matrix<4,4,F32> convertFromMaxMatrix(Matrix3 & mat)
   {
      Matrix<4,4,F32> ret;
      for (S32 i=0; i<4; i++)
      {
         Point3 row = mat.GetRow(i);
         Vector<F32,4> col;
         col.set(0,row.x);
         col.set(1,row.y);
         col.set(2,row.z);
         col.set(3,i==3 ? 1 : 0);
         ret.setCol(i,col);
      }
      return ret;
   }

};

