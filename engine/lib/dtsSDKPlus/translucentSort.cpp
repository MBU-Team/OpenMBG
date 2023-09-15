//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "translucentSort.h"

namespace DTS
{

F32 epsilon = 0.0001f;

// hold indices and sizes of biggest faces...these are marked as higher priority for splitting with
std::vector<S32> bigFaces;
std::vector<F32> bigFaceSizes;

// planes in this list are hidden because we are on the other side of them at the time...
std::vector<Point3D> noAddNormals;

TranslucentSort::TranslucentSort(std::vector<Primitive> & faces,
                                 std::vector<U16> & indices,
                                 std::vector<Point3D> & verts,
                                 std::vector<Point3D> & norms,
                                 std::vector<Point2D> & tverts,
                                 S32 numBigFaces, S32 maxDepth, bool zLayerUp, bool zLayerDown) :
                  mFaces(faces), mIndices(indices), mVerts(verts), mNorms(norms), mTVerts(tverts)
{
   mNumBigFaces = numBigFaces;
   mMaxDepth = maxDepth;
   mZLayerUp = zLayerUp;
   mZLayerDown = zLayerDown;
   bigFaces.clear();
   bigFaceSizes.clear();

   initFaces();
   frontSort = backSort = NULL;
   currentDepth = 0;
}

TranslucentSort::TranslucentSort(TranslucentSort * from) :
   mFaces(from->mFaces), mIndices(from->mIndices), mVerts(from->mVerts), mNorms(from->mNorms), mTVerts(from->mTVerts)
{
   faceInfoList.resize(mFaces.size());
   for (S32 i=0; i<mFaces.size(); i++)
   {
      faceInfoList[i] = new FaceInfo;
      *faceInfoList[i] = *from->faceInfoList[i];
   }

   currentDepth = from->currentDepth + 1;
   mMaxDepth = from->mMaxDepth;
   mZLayerUp = from->mZLayerUp;
   mZLayerDown = from->mZLayerDown;
   mNumBigFaces = 0; // never used...
   frontSort = backSort = NULL;
}

TranslucentSort::~TranslucentSort()
{
   delete frontSort;
   delete backSort;
   S32 i;
   for (i=0; i<faceInfoList.size(); i++)
      delete faceInfoList[i];
   for (i=0; i<saveFaceInfoList.size(); i++)
      delete saveFaceInfoList[i];
   for (i=0; i<frontClusters.size(); i++)
      delete frontClusters[i];
   for (i=0; i<backClusters.size(); i++)
      delete backClusters[i];
}

void TranslucentSort::initFaces()
{
   faceInfoList.resize(mFaces.size());

   S32 i;
   for (i=0; i<mFaces.size(); i++)
   {
      faceInfoList[i] = new FaceInfo;
      faceInfoList[i]->used = false;
   }

   for (i=0; i<mFaces.size(); i++)
   {
      initFaceInfo(mFaces[i],*faceInfoList[i]);
      setFaceInfo(mFaces[i],*faceInfoList[i]);
   }
}

void TranslucentSort::sort()
{
   S32 i;
   for (i=0; i<faceInfoList.size(); i++)
      if (!faceInfoList[i]->used)
         break;
   if (i==faceInfoList.size())
      return; // no unused faces...

   while (1)
   {
      // 1. select faces with no one behind them -- these guys get drawn first
      backClusters.push_back(new IntegerSet);
      backClusters.back()->resize(mFaces.size());
      for (i=0; i<faceInfoList.size(); i++)
         if (!faceInfoList[i]->used && !allSet(faceInfoList[i]->isBehindMe) && !allSet(faceInfoList[i]->isCutByMe))
         {
            setMembershipArray(*backClusters.back(),true,i);
            faceInfoList[i]->used = true; // select as used so we don't grab it below
         }
      
      // 2. select faces with no one in front of them -- these guys get drawn last
      insElement(frontClusters,0,new IntegerSet);
      frontClusters.front()->resize(mFaces.size());
      for (i=0; i<faceInfoList.size(); i++)
         if (!faceInfoList[i]->used && !allSet(faceInfoList[i]->isInFrontOfMe) && !allSet(faceInfoList[i]->isCutByMe))
         {
            setMembershipArray(*frontClusters.front(),true,i);
            faceInfoList[i]->used = true; // this won't have any effect, but it's here to parallel above
         }

      // 3. clear above faces and repeat 1&2 until no more faces found in either step
      IntegerSet removeThese = *backClusters.back();
      overlapSet(removeThese,*frontClusters.front());

      if (!allSet(removeThese))
         // didn't remove anything...
         break;

      clearFaces(removeThese);
   }

   // 4. pick face cutting fewest other faces and resulting in most balanced split, call this cutFace
   S32 fewestCuts;
   S32 balance;
   S32 priority = 0;
   S32 cutFace = -1;
   for (i=0; i<mFaces.size(); i++)
   {
      if (faceInfoList[i]->used)
         continue;
      S32 cut = 0, front = 0, back = 0;
      for (S32 j=0; j<mFaces.size(); j++)
      {
         if (faceInfoList[j]->used)
            continue;
         if (faceInfoList[i]->isCutByMe[j])
            cut++;
         if (faceInfoList[i]->isInFrontOfMe[j])
            front++;
         if (faceInfoList[i]->isBehindMe[j])
            back++;
      }
      if (cutFace>=0)
      {
         if (faceInfoList[i]->priority < priority)
            continue;
         if (faceInfoList[i]->priority == priority)
         {
            if ( (cut>fewestCuts) || (cut==fewestCuts && fabs(front-back)>=balance) )
               continue;
         }
      }
      // if we get this far, this is our new cutFace
      cutFace = i;
      fewestCuts = cut;
      priority = faceInfoList[i]->priority;
      balance = fabs(front-back);
   }

   if (cutFace>=0 && currentDepth<mMaxDepth)
   {
      // 5. cut all faces cut by cutFace
      if (allSet(faceInfoList[cutFace]->isCutByMe))
      {
         S32 startSize = mFaces.size(); // won't need to split beyond here, even though more faces added
         for (i=0; i<startSize; i++)
            if (!faceInfoList[i]->used && faceInfoList[cutFace]->isCutByMe[i])
               splitFace(i,faceInfoList[cutFace]->normal,faceInfoList[cutFace]->k);

         // may be new faces and some old faces may have been disabled, recompute face info
         for (i=0; i<mFaces.size(); i++)
            if (!faceInfoList[i]->used)
               setFaceInfo(mFaces[i],*faceInfoList[i]);
      }

      S32 startNumFaces = mFaces.size();
      IntegerSet disableSet;
      
      // 6. branch into two orders depending on which side of cutFace camera is, perform translucent sort on each

      // back
      backSort = new TranslucentSort(this);
      disableSet.resize(mFaces.size());
      setMembershipArray(disableSet,false,0,disableSet.size());
      for (i=0; i<mFaces.size(); i++)
      {
         if (backSort->faceInfoList[i]->used || backSort->faceInfoList[cutFace]->isBehindMe[i])
            continue;
         if (backSort->faceInfoList[cutFace]->isCutByMe[i])
            assert(0); // doh, perform hard assert :(...

         if (backSort->faceInfoList[cutFace]->isCoplanarWithMe[i] || cutFace==i)
         {
            if (dotProduct(backSort->faceInfoList[cutFace]->normal,backSort->faceInfoList[i]->normal)<0.0f)
               continue;
         }
         else if (!backSort->faceInfoList[cutFace]->isInFrontOfMe[i] && cutFace!=i)
            assert(0);

         setMembershipArray(disableSet,true,i);
      }

      if (!allSet(disableSet))
         assert(0);

      backSort->clearFaces(disableSet);

      backSort->sort();

      if (backSort->backSort==NULL && backSort->frontSort==NULL && backSort->frontClusters.empty() && backSort->backClusters.empty())
      {
         // empty, no reason to keep backSort
         delete backSort;
         backSort = NULL;
      }

      // create faceInfo entry for any faces that got added (set used=true)
      faceInfoList.resize(mFaces.size());
      for (i=startNumFaces; i<faceInfoList.size(); i++)
      {
         faceInfoList[i] = new FaceInfo;
         faceInfoList[i]->used = true;
      }

      // front
      frontSort = new TranslucentSort(this);
      disableSet.resize(mFaces.size());
      setMembershipArray(disableSet,false,0,disableSet.size());
      for (i=0; i<mFaces.size(); i++)
      {
         if (frontSort->faceInfoList[i]->used || frontSort->faceInfoList[cutFace]->isInFrontOfMe[i])
            continue;
         if (frontSort->faceInfoList[cutFace]->isCutByMe[i])
            assert(0); // doh, perform hard assert...

         if (frontSort->faceInfoList[cutFace]->isCoplanarWithMe[i] || cutFace==i)
         {
            if (dotProduct(frontSort->faceInfoList[cutFace]->normal,frontSort->faceInfoList[i]->normal)>0.0f)
               continue;
         }
         else if (!frontSort->faceInfoList[cutFace]->isBehindMe[i] && i!=cutFace)
            assert(0);

         setMembershipArray(disableSet,true,i);
      }

      if (!allSet(disableSet))
         assert(0);

      frontSort->clearFaces(disableSet);

      frontSort->sort();

      if (frontSort->backSort==NULL && frontSort->frontSort==NULL && frontSort->frontClusters.empty() && frontSort->backClusters.empty())
      {
         // empty, no reason to keep backSort
         delete backSort;
         backSort = NULL;
      }
      
      // setup cut plane
      splitNormal = faceInfoList[cutFace]->normal;
      splitK = faceInfoList[cutFace]->k;
   }
   else if (cutFace>=0)
   {
      // we've gotten too deep, just dump the remaing faces -- but dump in best order we can
      if (mZLayerUp)
         layerSort(middleCluster,true);
      else if (mZLayerDown)
         layerSort(middleCluster,false);
      else
         copeSort(middleCluster);
   }
}

// called copeSort because we can't really get a perfect order, we'll do the best we can
void TranslucentSort::copeSort(std::vector<S32> & cluster)
{
   std::vector<S32> frontOrderedCluster, backOrderedCluster;

   // restore after following loop
   saveFaceInfo();

   while (1)
   {
      S32 bestFace = -1;
      S32 bestCount = 0x7FFFFFFF;
      bool front;

      // we need to find face with fewest polys behind or in front (cut implies both)
      for (S32 i=0; i<faceInfoList.size(); i++)
      {
         if (faceInfoList[i]->used)
            continue;
         S32 frontCount = 0;
         S32 backCount = 0;
         for (S32 j=0; j<faceInfoList.size(); j++)
         {
            if (faceInfoList[j]->used)
               continue;
            if (faceInfoList[i]->isInFrontOfMe[j])
               frontCount++;
            else if (faceInfoList[i]->isBehindMe[j])
               backCount++;
            else if (faceInfoList[i]->isCutByMe[j])
            {
               frontCount++;
               backCount++;
            }
         }
         if (backCount < bestCount || bestFace<0)
         {
            bestCount = backCount;
            bestFace = i;
            front = false;
         }
         if (frontCount==0 && frontCount < bestCount)
         {
            bestCount = frontCount;
            bestFace = i;
            front = true;
         }
      }
      if (bestFace>=0)
      {
         if (front)
            insElement(frontOrderedCluster,0,bestFace);
         else
            backOrderedCluster.push_back(bestFace);
         IntegerSet disableSet;
         disableSet.resize(mFaces.size());
         setMembershipArray(disableSet,true,bestFace);
         clearFaces(disableSet);
      }
      else
         break;
   }
   
   cluster = backOrderedCluster;
   appendVector(cluster,frontOrderedCluster);

   // we need face info back...
   restoreFaceInfo();

   // we have a good ordering...but see if we can make some local optimizations
   for (S32 i=0; i<cluster.size(); i++)
   {
      S32 face1 = cluster[i];
      FaceInfo & faceInfo1 = *faceInfoList[face1];
      for (S32 j=i+1; j<cluster.size(); j++)
      {
         S32 face2 = cluster[j];
         FaceInfo & faceInfo2 = *faceInfoList[face2];
         if ((faceInfo1.isBehindMe[face2] && faceInfo2.isInFrontOfMe[face1]) ||
             (faceInfo1.isCutByMe[face2] && faceInfo2.isInFrontOfMe[face1])  ||
             (faceInfo1.isBehindMe[face2] && faceInfo2.isCutByMe[face1]))
         {
            // these two guys should be switched...now check to see if we can do it
            S32 k;
            for (k=i+1; k<j; k++)
            {
               S32 face12 = cluster[k];
               FaceInfo & faceInfo12 = *faceInfoList[face12];
               
               // Currently, face1 precedes face12 in the list...under what conditions is it ok
               // to have face1 follow face12?  Answer:  face12 behind face1, or face1 in front of face12.
               // Similarly, face12 precedes face2...
               if ((faceInfo1.isBehindMe[face12] || faceInfo12.isInFrontOfMe[face1]) &&
                   (faceInfo12.isBehindMe[face2] || faceInfo2.isInFrontOfMe[face12]))
                  continue;
               break;
            }
            if (k==j)
            {
               // switch has been approved...
               cluster[i] = face2;
               cluster[j] = face1;
               i--;
               break; // TODO:  do we need to make sure no infinite loop occurs?
            }
         }
         
      }
   }
}

void TranslucentSort::layerSort(std::vector<S32> & cluster, bool pointUp)
{
   // sort up-pointing faces from bottom to top and down-pointing faces from top to bottom
   std::vector<S32> upCluster, downCluster;
   std::vector<F32> upZ, downZ;

   // go through each face, decide which list to add it to and where
   for (S32 i=0; i<faceInfoList.size(); i++)
   {
      if (faceInfoList[i]->used)
         continue;
      Primitive & face = mFaces[i];
      S32 idx0 = mIndices[face.firstElement + 0];
      S32 idx1 = mIndices[face.firstElement + 1];
      S32 idx2 = mIndices[face.firstElement + 2];
      Point3D & v0 = mVerts[idx0];
      Point3D & v1 = mVerts[idx1];
      Point3D & v2 = mVerts[idx2];
      
      // find smallest z
      F32 smallZ = v0.z() < v1.z() ? v0.z() : v1.z();
      smallZ = smallZ < v2.z() ? smallZ : v2.z();
      // find largets Z
      F32 bigZ = v0.z() > v1.z() ? v0.z() : v1.z();
      bigZ = bigZ > v2.z() ? bigZ : v2.z();

      F32 sortBy = pointUp ? smallZ : bigZ;
      S32 j;

      if (faceInfoList[i]->normal.z()>0.0f)
      {
         // we face up

         if (upCluster.empty())
         {
            upCluster.push_back(i);
            upZ.push_back(sortBy);
         }
         else
         {
            // keep sorted in order of increasing z (so bottom faces are first)
            for (j=0; j<upZ.size(); j++)
               if (sortBy<upZ[j])
                  break;
            insElement(upZ,j,sortBy);
            insElement(upCluster,j,i);
         }
      }
      else
      {
         // we face down
         
         if (downCluster.empty())
         {
            downCluster.push_back(i);
            downZ.push_back(sortBy);
         }
         else
         {
            // keep sorted in order of decreasing z (so top faces are first)
            for (j=0; j<downZ.size(); j++)
               if (sortBy>downZ[j])
                  break;
            insElement(downZ,j,sortBy);
            insElement(downCluster,j,i);
         }
      }
   }

   if (pointUp)
   {
      cluster = upCluster;
      appendVector(cluster,downCluster);

   }
   else
   {
      cluster = downCluster;
      appendVector(cluster,upCluster);
   }
}

void TranslucentSort::saveFaceInfo()
{
   if (saveFaceInfoList.size())
   {
      for (S32 i=0; i<saveFaceInfoList.size(); i++)
         delete saveFaceInfoList[i];
   }
   saveFaceInfoList.resize(faceInfoList.size());
   
   for (S32 i=0; i<faceInfoList.size(); i++)
   {
      saveFaceInfoList[i] = new FaceInfo;
      *saveFaceInfoList[i] = *faceInfoList[i];
   }
}

void TranslucentSort::restoreFaceInfo()
{
   for (S32 i=0; i<saveFaceInfoList.size(); i++)
      *faceInfoList[i] = *saveFaceInfoList[i];
}

void TranslucentSort::clearFaces(IntegerSet & removeThese)
{
   for (S32 i=0; i<faceInfoList.size(); i++)
   {
      FaceInfo & faceInfo = *faceInfoList[i];
      subtractSet(faceInfo.isInFrontOfMe,removeThese);
      subtractSet(faceInfo.isBehindMe,removeThese);
      subtractSet(faceInfo.isCutByMe,removeThese);
      subtractSet(faceInfo.isCoplanarWithMe,removeThese);
      if (removeThese[i])
         faceInfo.used = true;
   }
}

void TranslucentSort::initFaceInfo(Primitive & face, FaceInfo & faceInfo, bool setPriority)
{
   faceInfo.used = false;
   faceInfo.parentFace = -1;
   faceInfo.childFace1 = -1;
   faceInfo.childFace2 = -1;
   faceInfo.childFace3 = -1;

   // get normal and plane constant
   S32 idx0 = mIndices[face.firstElement + 0];
   S32 idx1 = mIndices[face.firstElement + 1];
   S32 idx2 = mIndices[face.firstElement + 2];
   Point3D & vert0 = mVerts[idx0];
   Point3D & vert1 = mVerts[idx1];
   Point3D & vert2 = mVerts[idx2];
   Point3D & normal = faceInfo.normal;
   // compute normal using largest gap...
   Point3D edge01 = vert1-vert0;
   Point3D edge12 = vert2-vert1;
   Point3D edge20 = vert0-vert2;
   if (dotProduct(edge01,edge01)>=dotProduct(edge12,edge12) && dotProduct(edge01,edge01)>=dotProduct(edge20,edge20))
   {
      // edge01 biggest gap
      crossProduct(edge12,edge20,&normal);
      normal *= -1.0f;
   }
   else if (dotProduct(edge12,edge12)>=dotProduct(edge20,edge20) && dotProduct(edge12,edge12)>=dotProduct(edge01,edge01))
   {
      // edge12 biggest gap
      crossProduct(edge20,edge01,&normal);
      normal *= -1.0f;
   }
   else
   {
      // edge20 biggest gap
      crossProduct(edge01,edge12,&normal);
      normal *= -1.0f;
   }
   normal.normalize();

   faceInfo.k = dotProduct(normal,vert0);

   if (setPriority)
   {
      faceInfo.priority = 0;
      F32 maxExtent = dotProduct(edge01,edge01);
      if (maxExtent<dotProduct(edge12,edge12))
         maxExtent = dotProduct(edge12,edge12);
      if (maxExtent<dotProduct(edge20,edge20))
         maxExtent = dotProduct(edge20,edge20);
         
      for (S32 i=0; i<mNumBigFaces; i++)
      {
         if (i==bigFaceSizes.size() || maxExtent>bigFaceSizes[i])
         {
            insElement(bigFaceSizes,i,maxExtent);
            insElement(bigFaces,i,(S32)(&face-&mFaces[0]));

            for (; i<bigFaceSizes.size(); i++)
               faceInfoList[bigFaces[i]]->priority = i<mNumBigFaces ? mNumBigFaces-i : 0;
            while (bigFaceSizes.size()>mNumBigFaces)
            {
               bigFaceSizes.pop_back();
               bigFaces.pop_back();
            }
            break;
         }
      }
   }
}

void TranslucentSort::setFaceInfo(Primitive & face, FaceInfo & faceInfo)
{
   faceInfo.isInFrontOfMe.resize(mFaces.size());
   faceInfo.isBehindMe.resize(mFaces.size());
   faceInfo.isCutByMe.resize(mFaces.size());
   faceInfo.isCoplanarWithMe.resize(mFaces.size());

   setMembershipArray(faceInfo.isInFrontOfMe,false,0,faceInfo.isInFrontOfMe.size());
   setMembershipArray(faceInfo.isBehindMe,false,0,faceInfo.isBehindMe.size());
   setMembershipArray(faceInfo.isCutByMe,false,0,faceInfo.isCutByMe.size());
   setMembershipArray(faceInfo.isCoplanarWithMe,false,0,faceInfo.isCoplanarWithMe.size());

   Point3D & normal = faceInfo.normal;
   F32 & k = faceInfo.k;

   S32 myIndex = (S32)(&face-&mFaces[0]);

   for (S32 i=0; i<mFaces.size(); i++)
   {
      if (i==myIndex || faceInfoList[i]->used)
         continue;
         
      Primitive & otherFace = mFaces[i];
      
      S32 idx0 = mIndices[otherFace.firstElement + 0];
      S32 idx1 = mIndices[otherFace.firstElement + 1];
      S32 idx2 = mIndices[otherFace.firstElement + 2];
      Point3D & v0 = mVerts[idx0];
      Point3D & v1 = mVerts[idx1];
      Point3D & v2 = mVerts[idx2];
      bool hasFrontVert = false, hasBackVert = false;
      if (dotProduct(normal,v0) > k + epsilon)
         hasFrontVert = true;
      else if (dotProduct(normal,v0) < k - epsilon)
         hasBackVert = true;
      if (dotProduct(normal,v1) > k + epsilon)
         hasFrontVert = true;
      else if (dotProduct(normal,v1) < k - epsilon)
         hasBackVert = true;
      if (dotProduct(normal,v2) > k + epsilon)
         hasFrontVert = true;
      else if (dotProduct(normal,v2) < k - epsilon)
         hasBackVert = true;
         
      if (hasFrontVert && !hasBackVert)
         setMembershipArray(faceInfo.isInFrontOfMe,true,i);
      else if (!hasFrontVert && hasBackVert)
         setMembershipArray(faceInfo.isBehindMe,true,i);
      else if (hasFrontVert && hasBackVert)
         setMembershipArray(faceInfo.isCutByMe,true,i);
      else if (!hasFrontVert && !hasBackVert)
         setMembershipArray(faceInfo.isCoplanarWithMe,true,i);
   }
}

bool TranslucentSort::anyInFrontOfPlane(Point3D normal, F32 k)
{
   // make sure no face in use is in front of plane
   for (S32 i=0; i<mFaces.size(); i++)
   {
      if (faceInfoList[i]->used)
         continue;
      S32 idx0 = mIndices[mFaces[i].firstElement + 0];
      S32 idx1 = mIndices[mFaces[i].firstElement + 1];
      S32 idx2 = mIndices[mFaces[i].firstElement + 2];
      if (dotProduct(normal,mVerts[idx0]) > k + epsilon)
         return true;
      if (dotProduct(normal,mVerts[idx1]) > k + epsilon)
         return true;
      if (dotProduct(normal,mVerts[idx2]) > k + epsilon)
         return true;
   }
   return false;
}

bool TranslucentSort::anyBehindPlane(Point3D normal, F32 k)
{
   // make sure no face in use is behind plane
   for (S32 i=0; i<mFaces.size(); i++)
   {
      if (faceInfoList[i]->used)
         continue;
      S32 idx0 = mIndices[mFaces[i].firstElement + 0];
      S32 idx1 = mIndices[mFaces[i].firstElement + 1];
      S32 idx2 = mIndices[mFaces[i].firstElement + 2];
      if (dotProduct(normal,mVerts[idx0]) < k - epsilon)
         return true;
      if (dotProduct(normal,mVerts[idx1]) < k - epsilon)
         return true;
      if (dotProduct(normal,mVerts[idx2]) < k - epsilon)
         return true;
   }
   return false;
}

void getMinMaxExtents(Point3D & x, Point3D & v0, Point3D & v1, Point3D & v2, F32 & xmin, F32 & xmax)
{
   xmin = xmax = dotProduct(x,v0);
   F32 dot = dotProduct(x,v1);
   if (xmin>dot)
      xmin=dot;
   else if (xmax<dot)
      xmax=dot;
   dot = dotProduct(x,v2);
   if (xmin>dot)
      xmin=dot;
   else if (xmax<dot)
      xmax=dot;
}

void TranslucentSort::splitFace(S32 faceIndex, Point3D normal, F32 k)
{
   S32 idx0 = mIndices[mFaces[faceIndex].firstElement + 0];
   S32 idx1 = mIndices[mFaces[faceIndex].firstElement + 1];
   S32 idx2 = mIndices[mFaces[faceIndex].firstElement + 2];

   Point3D v0 = mVerts[idx0];
   Point3D v1 = mVerts[idx1];
   Point3D v2 = mVerts[idx2];

   F32 k0 = dotProduct(normal,v0);
   F32 k1 = dotProduct(normal,v1);
   F32 k2 = dotProduct(normal,v2);
   
   // if v0, v1, or v2 is on the plane defined by normal and k, call special case routine
   if (fabs(k0-k) < epsilon || fabs(k1-k) < epsilon || fabs(k2-k) < epsilon)
   {
      splitFace2(faceIndex,normal,k);
      return;
   }

   // find the odd man out (the vertex alone on his side of the plane)
   S32 code = 0, rogue;
   if (k0 < k)
      code |= 1;
   if (k1 < k)
      code |= 2;
   if (k2 < k)
      code |= 4;
   switch (code)
   {
      case 1:
      case 6: rogue = 0; break;
      case 2:
      case 5: rogue = 1; break;
      case 4:
      case 3: rogue = 2; break;
      case 0:
      case 7:
         return; // shouldn't happen...
   }

   // re-order verts so that rogue vert is first vert
   idx0 = mIndices[mFaces[faceIndex].firstElement + ((rogue+0)%3)];
   idx1 = mIndices[mFaces[faceIndex].firstElement + ((rogue+1)%3)];
   idx2 = mIndices[mFaces[faceIndex].firstElement + ((rogue+2)%3)];
   v0 = mVerts[idx0];
   v1 = mVerts[idx1];
   v2 = mVerts[idx2];
   k0 = dotProduct(normal,v0);
   k1 = dotProduct(normal,v1);
   k2 = dotProduct(normal,v2);
   Point2D tv0 = mTVerts[idx0];
   Point2D tv1 = mTVerts[idx1];
   Point2D tv2 = mTVerts[idx2];
   Point3D n0 = mNorms[idx0];
   Point3D n1 = mNorms[idx1];
   Point3D n2 = mNorms[idx2];

   // find intersection of edges and plane
   F32 a01 = (k-k0)/(k1-k0);
   F32 a02 = (k-k0)/(k2-k0);
   Point3D v01 = v1-v0;
   v01 *= a01;
   v01 += v0;
   Point2D tv01 = tv1-tv0;
   tv01 *= a01;
   tv01 += tv0;
   Point3D v02 = v2-v0;
   v02 *= a02;
   v02 += v0;
   Point2D tv02 = tv2-tv0;
   tv02 *= a02;
   tv02 += tv0;

   // interpolate the normals too (we'll just linearly interpolate...perhaps slerp if later)
   Point3D n01 = n1-n0;
   n01 *= a01;
   n01 += n0;
   n01.normalize();
   Point3D n02 = n2-n0;
   n02 *= a02;
   n02 += n0;
   n02.normalize();

   // add two new verst
   S32 idx01 = mVerts.size();
   mVerts.push_back(v01);
   mNorms.push_back(n01);
   mTVerts.push_back(tv01);
   S32 idx02 = mVerts.size();
   mVerts.push_back(v02);
   mNorms.push_back(n02);
   mTVerts.push_back(tv02);

   // add three faces
   mFaces.push_back(Primitive());
   mFaces.push_back(Primitive());
   mFaces.push_back(Primitive());
   Primitive & face0 = mFaces[mFaces.size()-3];
   Primitive & face1 = mFaces[mFaces.size()-2];
   Primitive & face2 = mFaces[mFaces.size()-1];

   // add "rogue" face
   face0.firstElement = mIndices.size();
   face0.numElements = 3;
   face0.type = mFaces[faceIndex].type;
   mIndices.push_back(idx0);
   mIndices.push_back(idx01);
   mIndices.push_back(idx02);
   
   // add idx01, idx1, idx02
   face1.firstElement = mIndices.size();
   face1.numElements = 3;
   face1.type = mFaces[faceIndex].type;
   mIndices.push_back(idx01);
   mIndices.push_back(idx1);
   mIndices.push_back(idx02);
   
   // add idx2, idx02, idx01
   face2.firstElement = mIndices.size();
   face2.numElements = 3;
   face2.type = mFaces[faceIndex].type;
   mIndices.push_back(idx2);
   mIndices.push_back(idx02);
   mIndices.push_back(idx1);

   // finally, set faceInfo
   S32 numFaces = mFaces.size();
   faceInfoList.push_back(new FaceInfo);
   faceInfoList.push_back(new FaceInfo);
   faceInfoList.push_back(new FaceInfo);

   faceInfoList[faceIndex]->used = true;
   faceInfoList[faceIndex]->childFace1 = numFaces-3;
   faceInfoList[faceIndex]->childFace2 = numFaces-2;
   faceInfoList[faceIndex]->childFace3 = numFaces-1;

   initFaceInfo(mFaces[numFaces-3],*faceInfoList[numFaces-3],false);
   initFaceInfo(mFaces[numFaces-2],*faceInfoList[numFaces-2],false);
   initFaceInfo(mFaces[numFaces-1],*faceInfoList[numFaces-1],false);

   faceInfoList[numFaces-3]->priority = faceInfoList[faceIndex]->priority;
   faceInfoList[numFaces-2]->priority = faceInfoList[faceIndex]->priority;
   faceInfoList[numFaces-1]->priority = faceInfoList[faceIndex]->priority;
   faceInfoList[numFaces-3]->parentFace = faceIndex;
   faceInfoList[numFaces-2]->parentFace = faceIndex;
   faceInfoList[numFaces-1]->parentFace = faceIndex;
}

void TranslucentSort::splitFace2(S32 faceIndex, Point3D normal, F32 k)
{
   S32 idx0 = mIndices[mFaces[faceIndex].firstElement + 0];
   S32 idx1 = mIndices[mFaces[faceIndex].firstElement + 1];
   S32 idx2 = mIndices[mFaces[faceIndex].firstElement + 2];

   Point3D v0 = mVerts[idx0];
   Point3D v1 = mVerts[idx1];
   Point3D v2 = mVerts[idx2];

   F32 k0 = dotProduct(normal,v0);
   F32 k1 = dotProduct(normal,v1);
   F32 k2 = dotProduct(normal,v2);

   // make sure we got here legitimately
   if (fabs(k0-k) >= epsilon && fabs(k1-k) >= epsilon && fabs(k2-k) >= epsilon)
      assert(0);

   // find the odd man out (the vertex that is on the plane)
   S32 rogue;
   if (fabs(k0-k) < epsilon)
      rogue = 0;
   else if (fabs(k1-k) < epsilon)
      rogue = 1;
   else if (fabs(k2-k) < epsilon)
      rogue = 2;
   else
      assert(0);

   // re-order verts so that rogue vert is first vert
   idx0 = mIndices[mFaces[faceIndex].firstElement + ((rogue+0)%3)];
   idx1 = mIndices[mFaces[faceIndex].firstElement + ((rogue+1)%3)];
   idx2 = mIndices[mFaces[faceIndex].firstElement + ((rogue+2)%3)];
   v0 = mVerts[idx0];
   v1 = mVerts[idx1];
   v2 = mVerts[idx2];
   k0 = dotProduct(normal,v0);
   k1 = dotProduct(normal,v1);
   k2 = dotProduct(normal,v2);
   Point2D tv0 = mTVerts[idx0];
   Point2D tv1 = mTVerts[idx1];
   Point2D tv2 = mTVerts[idx2];
   Point3D n0 = mNorms[idx0];
   Point3D n1 = mNorms[idx1];
   Point3D n2 = mNorms[idx2];

   // find intersection of edges and plane
   F32 a12 = (k-k1)/(k2-k1);
   Point3D v12 = v2-v1;
   v12 *= a12;
   v12 += v1;
   Point2D tv12 = tv2-tv1;
   tv12 *= a12;
   tv12 += tv1;

   // interpolate the normals too (we'll just linearly interpolate...perhaps slerp if later)
   Point3D n12 = n2-n1;
   n12 *= a12;
   n12 += n1;
   n12.normalize();

   // add new vert
   S32 idx12 = mVerts.size();
   mVerts.push_back(v12);
   mNorms.push_back(n12);
   mTVerts.push_back(tv12);

   // add two faces
   mFaces.push_back(Primitive());
   mFaces.push_back(Primitive());
   Primitive & face0 = mFaces[mFaces.size()-2];
   Primitive & face1 = mFaces[mFaces.size()-1];

   // add idx0, idx2, idx12
   face0.firstElement = mIndices.size();
   face0.numElements = 3;
   face0.type = mFaces[faceIndex].type;
   mIndices.push_back(idx0);
   mIndices.push_back(idx2);
   mIndices.push_back(idx12);
   
   // add idx0, idx12, idx1
   face1.firstElement = mIndices.size();
   face1.numElements = 3;
   face1.type = mFaces[faceIndex].type;
   mIndices.push_back(idx0);
   mIndices.push_back(idx12);
   mIndices.push_back(idx1);
   
   // finally, set faceInfo
   S32 numFaces = mFaces.size();
   faceInfoList.push_back(new FaceInfo);
   faceInfoList.push_back(new FaceInfo);

   faceInfoList[faceIndex]->used = true;
   faceInfoList[faceIndex]->childFace1 = numFaces-2;
   faceInfoList[faceIndex]->childFace2 = numFaces-1;
   faceInfoList[faceIndex]->childFace3 = -1;

   initFaceInfo(mFaces[numFaces-2],*faceInfoList[numFaces-2],false);
   initFaceInfo(mFaces[numFaces-1],*faceInfoList[numFaces-1],false);

   faceInfoList[numFaces-2]->priority = faceInfoList[faceIndex]->priority;
   faceInfoList[numFaces-1]->priority = faceInfoList[faceIndex]->priority;
   faceInfoList[numFaces-2]->parentFace = faceIndex;
   faceInfoList[numFaces-1]->parentFace = faceIndex;
}

void TranslucentSort::addFaces(std::vector<IntegerSet *> & addClusters, std::vector<Primitive> & faces, std::vector<U16> & indices, bool continueLast)
{
   S32 startFaces = faces.size();
   for (S32 i=0; i<addClusters.size(); i++)
      addFaces(addClusters[i],faces,indices, (faces.size()==startFaces) ? continueLast : true);
}

void TranslucentSort::addFaces(IntegerSet * addCluster, std::vector<Primitive> & faces, std::vector<U16> & indices, bool continueLast)
{
   IntegerSet toAdd = *addCluster;

   bool startNewFace = !continueLast || faces.empty();
   while (allSet(toAdd))
   {
//      for (S32 i=0; i<toAdd.getSize(); i++)
      for (S32 i=0; i<mFaces.size(); i++)
      {
         if (!startNewFace && faces.back().type!=mFaces[i].type)
            continue;
         if (!toAdd[i])
            continue;
         for (S32 k=0; k<noAddNormals.size(); k++)
         {
            if (dotProduct(noAddNormals[k],faceInfoList[i]->normal) > 0.99f)
               setMembershipArray(toAdd,false,i);
         }
         if (!toAdd[i])
            continue;
         // add this face...
         if (startNewFace)
         {
            faces.push_back(Primitive());
            faces.back().firstElement = indices.size();
            faces.back().numElements = 0;
            faces.back().type = mFaces[i].type;
            startNewFace = false;
         }
         faces.back().numElements += 3;
         indices.push_back(mIndices[mFaces[i].firstElement+0]);
         indices.push_back(mIndices[mFaces[i].firstElement+1]);
         indices.push_back(mIndices[mFaces[i].firstElement+2]);
         setMembershipArray(toAdd,false,i);
      }
      startNewFace = true;
   }
}

void TranslucentSort::addOrderedFaces(std::vector<S32> & orderedCluster, std::vector<Primitive> & faces, std::vector<U16> & indices, bool continueLast)
{
   std::vector<S32> toAdd = orderedCluster;

   bool startNewFace = !continueLast || faces.empty();
   while (!toAdd.empty())
   {
      for (S32 i=0; i<toAdd.size(); i++)
      {
         S32 k;      
         for (k=0; k<noAddNormals.size(); k++)
         {
            if (dotProduct(noAddNormals[k],faceInfoList[toAdd[i]]->normal) > 0.99f)
            {
               delElement(toAdd,i);
               i--;
               break;
            }
         }
         if (k!=noAddNormals.size())
            continue;
         if (!startNewFace && mFaces[toAdd[i]].type != faces.back().type)
            continue;
         if (startNewFace)
         {
            faces.push_back(Primitive());
            faces.back().firstElement = indices.size();
            faces.back().numElements = 0;
            faces.back().type = mFaces[toAdd[i]].type;
            startNewFace = false;
         }
         faces.back().numElements += 3;
         indices.push_back(mIndices[mFaces[toAdd[i]].firstElement+0]);
         indices.push_back(mIndices[mFaces[toAdd[i]].firstElement+1]);
         indices.push_back(mIndices[mFaces[toAdd[i]].firstElement+2]);
         delElement(toAdd,i);
         i--;
      }
      startNewFace = true;
   }
}

void TranslucentSort::generateClusters(std::vector<Cluster> & clusters, std::vector<Primitive> & faces, std::vector<U16> & indices, S32 retIndex)
{
   S32 idx = clusters.size();

   clusters.push_back(Cluster());
   clusters.push_back(Cluster());

   // add back faces
   clusters[idx].startPrimitive = faces.size();
   addFaces(backClusters,faces,indices);
   clusters[idx].endPrimitive = faces.size();

   clusters[idx].normal = splitNormal;
   clusters[idx].k = splitK;

   if (frontSort && backSort)
   {
      // Note: below there are some lines dealing with the variable "noAddNormal" scattered in.  Kind of a hack.
      // Here is what it does:  it is an optimization.  Any face with a normal matching an entry in that list will
      // not be added to the mesh.  This is desired if we know we are on one side of a plane (then we don't want
      // to bother adding faces that face the opposite direction).

      // back then front -- but add in opp. order because we know where to return from frontSort but not backSort
      S32 frontSide = clusters.size();
      frontSort->generateClusters(clusters,faces,indices,idx+1);
      clusters[idx].frontCluster = clusters.size();
      noAddNormals.push_back(-splitNormal);
      backSort->generateClusters(clusters,faces,indices,frontSide);
      noAddNormals.pop_back();
      clusters[idx].backCluster = clusters[idx].frontCluster;

      // front then back -- but add in opp. order because we know where to return from backSort but not frontSort
      S32 backSide = clusters.size();
      backSort->generateClusters(clusters,faces,indices,idx+1);
      clusters[idx].backCluster = clusters.size();
      noAddNormals.push_back(splitNormal);
      frontSort->generateClusters(clusters,faces,indices,backSide);
      noAddNormals.pop_back();
   }
   else if (frontSort)
   {
      clusters[idx].frontCluster = clusters[idx].backCluster = clusters.size();
      frontSort->generateClusters(clusters,faces,indices,idx+1);
   }
   else if (backSort)
   {
      clusters[idx].frontCluster = clusters[idx].backCluster = clusters.size();
      backSort->generateClusters(clusters,faces,indices,idx+1);
   }
   else
   {
      addOrderedFaces(middleCluster,faces,indices,clusters[idx].startPrimitive!=faces.size());
      addFaces(frontClusters,faces,indices,clusters[idx].startPrimitive!=faces.size());
      clusters[idx].endPrimitive = faces.size();
      clusters[idx].frontCluster = clusters[idx].backCluster = retIndex;
   }

   if (frontSort || backSort)
   {
      clusters[idx+1].normal = Point3D(0.0f,0.0f,0.0f);
      clusters[idx+1].k = 0.0f;

      clusters[idx+1].startPrimitive = faces.size();
      addFaces(frontClusters,faces,indices);
      clusters[idx+1].endPrimitive = faces.size();
      
      clusters[idx+1].frontCluster = clusters[idx+1].backCluster = retIndex;
   }
   else
      clusters.pop_back();
}

void TranslucentSort::generateSortedMesh(Mesh * mesh, S32 numBigFaces, S32 maxDepth, bool zLayerUp, bool zLayerDown)
{
   // on entry, mesh is organized like a standard mesh...
   // numFrames, numMatFrames, & vertsPerFrame describe what
   // is held in verts, norms, and tverts arrays
   // primitives and indices vectors describe the faces (same
   // faces on each frame/matFrame)
   
   // we want to convert this over to the structure that will be
   // used by TSSortedMesh...we also want to sort the faces, of course...

   std::vector<Primitive> meshFaces;
   std::vector<U16> meshIndices;
   std::vector<Point3D> meshVerts;
   std::vector<Point3D> meshNorms;
   std::vector<Point2D> meshTVerts;
   std::vector<Cluster> meshClusters;

   S32 i,j;
   for (i=0; i<mesh->numFrames; i++)
   {
      for (j=0; j<mesh->matFrames; j++)
      {
         std::vector<Primitive> faces = mesh->primitives;
         std::vector<U16> indices = mesh->indices;
         std::vector<Point3D> verts;
         std::vector<Point3D> norms;
         std::vector<Point2D> tverts;
         std::vector<Cluster> clusters;
         verts.resize(mesh->vertsPerFrame);
         memcpy(&verts[0],&mesh->verts[i*mesh->vertsPerFrame],sizeof(Point3D)*mesh->vertsPerFrame);
         norms.resize(mesh->vertsPerFrame);
         memcpy(&norms[0],&mesh->normals[i*mesh->vertsPerFrame],sizeof(Point3D)*mesh->vertsPerFrame);
         tverts.resize(mesh->vertsPerFrame);
         memcpy(&tverts[0],&mesh->tverts[i*mesh->vertsPerFrame],sizeof(Point2D)*mesh->vertsPerFrame);

         TranslucentSort sort(faces,indices,verts,norms,tverts,numBigFaces,maxDepth,zLayerUp,zLayerDown);
         sort.sort();
         std::vector<Primitive> newFaces;
         std::vector<U16> newIndices;
         sort.generateClusters(clusters,newFaces,newIndices);
         S32 k;
         for (k=0; k<clusters.size(); k++)
         {
            if (clusters[k].startPrimitive==clusters[k].endPrimitive && clusters[k].frontCluster==clusters[k].backCluster)
            {
               // this cluster servers no purpose...get rid of it
               for (S32 l=0; l<clusters.size(); l++)
               {
                  if (l==k)
                     continue;
                  if (clusters[l].frontCluster==k)
                     clusters[l].frontCluster = clusters[k].frontCluster;
                  if (clusters[l].frontCluster>k)
                     clusters[l].frontCluster--;
                  if (clusters[l].backCluster==k)
                     clusters[l].backCluster = clusters[k].backCluster;
                  if (clusters[l].backCluster>k)
                     clusters[l].backCluster--;
               }
               delElement(clusters,k);
               k = -1; // start over, our parent may now be useless...
            }
         }
         if (j==0)
         {
            mesh->startCluster.push_back(meshClusters.size());
            mesh->firstVerts.push_back(meshVerts.size());
            mesh->numVerts.push_back(verts.size());
         }
         // TODO: if tverts same as some previous frame, use that frame number
         // o.w.
         mesh->firstTVerts.push_back(meshTVerts.size());

         // adjust startPrimitive, endPrimitive, frontCluster, & backCluster on list of clusters just generated
         for (k=0; k<clusters.size(); k++)
         {
            Cluster & cluster = clusters[k];
            cluster.startPrimitive += meshFaces.size();
            cluster.endPrimitive += meshFaces.size();
            cluster.frontCluster += meshClusters.size();
            cluster.backCluster += meshClusters.size();
         }
         // now merge in just computed verts, tverts, indices, primitives, and clusters...
         appendVector(meshVerts,verts);
         if (mesh->firstTVerts.back()==meshTVerts.size())
            appendVector(meshTVerts,tverts);
         appendVector(meshNorms,norms);
         appendVector(meshIndices,newIndices);
         appendVector(meshFaces,newFaces);
         appendVector(meshClusters,clusters);
      }
   }
   mesh->clusters = meshClusters;
   mesh->primitives = meshFaces;
   mesh->indices = meshIndices;
   mesh->verts = meshVerts;
   mesh->normals = meshNorms;
   mesh->tverts = meshTVerts;
}

}; // namespace DTS


