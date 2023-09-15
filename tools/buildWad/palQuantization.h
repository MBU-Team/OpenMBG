//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INC_PALQUANTIZATION
#define _INC_PALQUANTIZATION

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _GBITMAP_H_
#include "dgl/gBitmap.h"
#endif

static const U32 MaxDepth = 7;
static const U32 NumColors = 256;

// Octree Color Quantization
class PalNode
{
   public:
      U32         mLevel;
      bool        mLeaf;
      bool        mMarked;          // has been added to reduce list
      U32         mCount;
      ColorF      mColSum;
      U32         mIndex;
      U32         mNumChildren;
      PalNode *   mChild[8];        // children
      PalNode *   mNext;            // next reducible node
      PalNode *   mPrev;            // previous reducible node
      
      inline PalNode(U32 level, bool leaf);
      inline ColorI getColor();
      inline void addColor(const ColorI & col);
      inline bool testBit(U8 val, U32 index);
      inline U32 findChild(const ColorI & col);
};

//------------------------------------------------------------------------------

class PalQuantizer
{
   public:
      U32               mLeafLevel;
      U32               mNumLeafs;
      PalNode *         mReduceList[MaxDepth+1];    // reducible node list
      PalNode *         mRoot;
      ColorI *          mPalette;
      U32               mNumColors;
      
      PalQuantizer();
      ~PalQuantizer();
      
      PalNode * makeNode(U32 level);
      void reduceTree();
      void makeReducible(PalNode * node);
      PalNode * getReducibleNode();
      void deleteNode(PalNode * node);
      void fillPalette(PalNode * node, U32 * index);
      void insertNode(PalNode * node, const ColorI & col);
      void buildTree(GBitmap * bitmap, U32 numColors = NumColors);
      U32 getColorIndex(const ColorI & col){return(quantizeColor(mRoot, col));}
      U32 quantizeColor(PalNode * node, const ColorI & col);
      
      void addToTree(GBitmap * bitmap, U32 numColors);
};

//------------------------------------------------------------------------------
// Class PalNode:
//------------------------------------------------------------------------------

PalNode::PalNode(U32 level, bool leaf) :
   mLevel(level),
   mLeaf(leaf),
   mMarked(false),
   mCount(0),
   mColSum(0,0,0),
   mIndex(0),
   mNumChildren(0),
   mNext(0),
   mPrev(0)
{
   for(U32 i = 0; i < 8; i++)
      mChild[i] = 0;
}

//------------------------------------------------------------------------------

ColorI PalNode::getColor()
{
   ColorI col(0,0,0);
   col.red = (U8)(mColSum.red / mCount);
   col.green = (U8)(mColSum.green / mCount);
   col.blue = (U8)(mColSum.blue / mCount);
   return(col);
}

//------------------------------------------------------------------------------

void PalNode::addColor(const ColorI & col)
{
   mColSum.red += col.red;
   mColSum.green += col.green;
   mColSum.blue += col.blue;
   mCount++;
}

//------------------------------------------------------------------------------

bool PalNode::testBit(U8 val, U32 index)
{
   return((val & (1<<index)) ? true : false);
}

//------------------------------------------------------------------------------

U32 PalNode::findChild(const ColorI & col)
{
#if defined(TORQUE_SUPPORTS_VC_INLINE_X86_ASM)
   U32 bit = MaxDepth-mLevel;
   U32 index;
   __asm {
      push eax
      push ebx
      push ecx
      push edx
      push esi

      //
      mov ecx, bit
   
      //
      mov ebx, 0
      mov edx, 0
      mov esi, [col]
      
      // red
      mov bl, BYTE PTR [esi]
      bt ebx, ecx
      setc dl
      shl dl, 2
      mov eax, edx
      
      // green
      mov bl, BYTE PTR [esi+1]
      bt ebx, ecx
      setc dl
      shl dl, 1
      or eax, edx

      // blue
      mov bl, BYTE PTR [esi+2]
      bt ebx, ecx
      setc dl
      or eax, edx
      
      //
      mov index, eax
      
      pop esi
      pop edx
      pop ecx
      pop ebx
      pop eax
   }
#else
   U32 index = testBit(col.red, (MaxDepth - mLevel)) << 2 |
               testBit(col.green, (MaxDepth - mLevel)) << 1 |
               testBit(col.blue, (MaxDepth - mLevel));
#endif
   return(index);   
}

#endif
