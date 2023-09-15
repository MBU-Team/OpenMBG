//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/lightUpdateGrouper.h"

LightUpdateGrouper::LightUpdateGrouper(const U32 bitStart, const U32 bitEnd)
{
   AssertFatal(bitEnd >= bitStart, "Error, bitend must be greater than bit start");
   AssertFatal(bitEnd < 32, "Error, bitend too large.  must be in the range 0..31");

   mBitStart = bitStart;
   mBitEnd   = bitEnd;
}


LightUpdateGrouper::~LightUpdateGrouper()
{
   mBitStart = 0xFFFFFFFF;
   mBitEnd   = 0xFFFFFFFF;
}


void LightUpdateGrouper::addKey(const U32 key)
{
#ifdef TORQUE_DEBUG
   for (U32 i = 0; i < mKeys.size(); i++)
      AssertFatal(mKeys[i] != key, "Error, key already in the array!");
#endif

   mKeys.push_back(key);
}

U32 LightUpdateGrouper::getKeyMask(const U32 key) const
{
   U32 numBits = mBitEnd - mBitStart + 1;
   for (U32 i = 0; i < mKeys.size(); i++) {
      if (mKeys[i] == key) {
         U32 idx = i % numBits;
         return (1 << (idx + mBitStart));
      }
   }

   AssertFatal(false, "Error, key not in the array!");
   return 0;
}

LightUpdateGrouper::BitIterator LightUpdateGrouper::begin()
{
   BitIterator itr;
   itr.mGrouper = this;
   itr.mCurrBit = mBitStart;
   itr.resetKeyArray();

   return itr;
}

void LightUpdateGrouper::BitIterator::resetKeyArray()
{
   mKeyArray.clear();
   if (valid() == false)
      return;

   // Ok, we need to select out every (mBitEnd - mBitStart - 1)th key,
   //  starting at mCurrBit.

   U32 numBits = mGrouper->mBitEnd - mGrouper->mBitStart + 1;
   U32 numKeys = mGrouper->mKeys.size();

   for (U32 i = mCurrBit - mGrouper->mBitStart; i < numKeys; i += numBits) {
      mKeyArray.push_back(mGrouper->mKeys[i]);
   }
}

