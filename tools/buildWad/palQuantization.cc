//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "buildWad/palQuantization.h"
#include "math/mPoint.h"

//------------------------------------------------------------------------------
// Class PalQuantizer:
//------------------------------------------------------------------------------

PalQuantizer::PalQuantizer() :
   mLeafLevel(MaxDepth+1),
   mNumLeafs(0),
   mRoot(0),
   mNumColors(0),
   mPalette(0)
{
   for(U32 i = 0; i < (MaxDepth+1); i++)
      mReduceList[i] = 0;
}

PalQuantizer::~PalQuantizer()
{
   deleteNode(mRoot);
   delete [] mPalette;
}

//------------------------------------------------------------------------------

PalNode * PalQuantizer::makeNode(U32 level)
{
   bool leaf = (level >= mLeafLevel) ? true : false;
   if(leaf)
      mNumLeafs++;
   return(new PalNode(level, leaf));
}

//------------------------------------------------------------------------------

void PalQuantizer::addToTree(GBitmap * bitmap, U32 numColors)
{
   AssertFatal(bitmap && numColors, "PalQuantizer::addToTree - invalid params");
   
   U32 width = bitmap->getWidth();
   U32 height = bitmap->getHeight();

   // check for first run - create root and palette   
   if(!mRoot)
   {
      mRoot = makeNode(0);
      mPalette = new ColorI[numColors];
      for(U32 i = 0; i < numColors; i++)
         mPalette[i].set(0,0,0);
   }   
      
   // walk this bitmap
   for(U32 y = 0; y < height; y++)
      for(U32 x = 0; x < width; x++)
      {
         ColorI col;
         bitmap->getColor(x,y,col);
         
         insertNode(mRoot, col);
         while(mNumLeafs > numColors)
            reduceTree();
      }
}

//------------------------------------------------------------------------------

void PalQuantizer::buildTree(GBitmap * bitmap, U32 numColors)
{
   AssertFatal(bitmap && numColors, "PalQuantizer::buildTree - invalid params");
   AssertFatal(!mRoot, "PalQuantizer::buildTree - already built");
   AssertFatal(!mPalette, "PalQuantizer::buildTree - already build");

   mPalette = new ColorI[numColors];
   for(U32 i = 0; i < numColors; i++)
      mPalette[i].set(0,0,0);
      
   U32 width = bitmap->getWidth();
   U32 height = bitmap->getHeight();
   
   mRoot = makeNode(0);
   
   // build it...
   for(U32 y = 0; y < height; y++)
   {
      for(U32 x = 0; x < width; x++)
      {
         ColorI col;
         AssertISV(bitmap->getColor(x,y,col), "PalQuantizer::buildTree: failed to get pixel.");

         insertNode(mRoot, col);

         while(mNumLeafs > numColors)
            reduceTree();
      }
   }
   
   // fill in the color palette...
   fillPalette(mRoot, &mNumColors);
}

//------------------------------------------------------------------------------

void PalQuantizer::fillPalette(PalNode * node, U32 * index)
{
   if(node)
   {
      if(node->mLeaf || (node->mLevel == mLeafLevel))
      {
         mPalette[*index] = node->getColor();
         node->mIndex = *index;
         (*index)++;
      }
      else
      {
         // do children
         for(U32 i = 0; i < 8; i++)
            fillPalette(node->mChild[i], index);
      }
   }
}

//------------------------------------------------------------------------------

void PalQuantizer::insertNode(PalNode * node, const ColorI & col)
{
   AssertFatal(node, "PalQuantizer::insertNode: invalid args");
   
   // add the color to the node
   node->addColor(col);
   
   if(!node->mLeaf && (node->mLevel < mLeafLevel))
   {
      U32 index = node->findChild(col);
      AssertFatal(index < 8, "PalQuantizer::insertNode - bad child index");
      
      if(node->mChild[index])
      {
         if((node->mCount > 1) && !node->mMarked)
            makeReducible(node);
      }
      else
      {
         // create this child
         node->mChild[index] = makeNode(node->mLevel + 1);
         node->mNumChildren++;
      }
      
      // insert  into child's octree
      insertNode(node->mChild[index], col);
   }
}

//------------------------------------------------------------------------------

void PalQuantizer::makeReducible(PalNode * node)
{
   AssertFatal(node, "PalQuantizer::makeReducible: invalid args");

   // add to the reduce list
   PalNode * head = mReduceList[node->mLevel];
   node->mNext = head;
   if(head)
      head->mPrev = node;
   mReduceList[node->mLevel] = node;
   node->mMarked = true;
}

//------------------------------------------------------------------------------

PalNode * PalQuantizer::getReducibleNode()
{
   U32 newLevel = mLeafLevel - 1;
   
   while(!mReduceList[newLevel])
      newLevel--;
      
   // get the node with the largest pixel count..
   PalNode * node = mReduceList[newLevel];
   PalNode * current = 0;
   while(node)
   {
      if(!current)
         current = node;
      else if(node->mCount < current->mCount)
         current = node;
      node = node->mNext;
   }
   
   return(current);
}

//------------------------------------------------------------------------------

void PalQuantizer::deleteNode(PalNode * node)
{
   if(!node)
      return;
   
   if(!node->mLeaf)
   {
      for(U32 i = 0; i < 8; i++)
      {
         if(node->mChild[i])
         {
            deleteNode(node->mChild[i]);
            node->mChild[i] = 0;
            node->mNumChildren--;
         }
      }
   }
   else
      mNumLeafs--;
   
   delete node;
}

//------------------------------------------------------------------------------

void PalQuantizer::reduceTree()
{
   PalNode * node = getReducibleNode();
   AssertFatal(node, "PalQuantizer::reduceTree - failed to get reducible node");
   
   // remove lowest child
   U32 lowest = -1;
   for(U32 i = 0; i < 8; i++)
      if(node->mChild[i])
      {
         if(lowest == -1)
            lowest = i;
         else if(node->mChild[i]->mCount < node->mChild[lowest]->mCount)
            lowest = i;
      }
   AssertFatal(lowest != -1, "PalQuantizer::reduceTree - bad node");

   deleteNode(node->mChild[lowest]);
   node->mChild[lowest] = 0;
   node->mNumChildren--;

   if(!node->mNumChildren)
   {
      node->mLeaf = true;
      mNumLeafs++;

      // remove the node from the reduce list
      PalNode * next = node->mNext;
      PalNode * prev = node->mPrev;
      if(!prev)
      {
         mReduceList[node->mLevel] = next;
         if(next)
            next->mPrev = 0;
      }
      else
      {
         prev->mNext = next;
         if(next)
            next->mPrev = prev;
      }
   
      node->mNext = node->mPrev = 0;
   }   
}

//------------------------------------------------------------------------------

U32 PalQuantizer::quantizeColor(PalNode * node, const ColorI & col)
{
   if(node->mLeaf || (node->mLevel == mLeafLevel))
      return(node->mIndex);

   U32 index = node->findChild(col);
   if(!node->mChild[index])
   {
      // get the child that is closest..
      S32 closest = -1;
      F64 dist = 0.f;
      for(U32 i = 0; i < 8; i++)
      {
         if(node->mChild[i])
         {
            ColorI childCol = node->mChild[i]->getColor();
            Point3D pnt(col.red - childCol.red, col.green - childCol.green, col.blue - childCol.blue);
            F64 len = pnt.len();
            
            if((closest == -1) || (len < dist))
            {
               closest = i;
               dist = len;
            }
            break;
         }
      }
      AssertFatal(closest != -1, "PalQuantizer::quantizeColor - failed to get child node");
      index = closest;      
   }
   return(quantizeColor(node->mChild[index], col));
}

