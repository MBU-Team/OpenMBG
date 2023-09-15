//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _LIGHTUPDATEGROUPER_H_
#define _LIGHTUPDATEGROUPER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif

class LightUpdateGrouper
{
   Vector<U32> mKeys;
   U32         mBitStart;
   U32         mBitEnd;

  public:
   class BitIterator {
      friend class LightUpdateGrouper;

     private:
      Vector<U32>         mKeyArray;
      U32                 mCurrBit;
      LightUpdateGrouper* mGrouper;

      void resetKeyArray();

     public:
      typedef U32 const* iterator;

      bool valid();
      U32  getNumKeys();
      U32  getMask();

      BitIterator& operator++(int);
      BitIterator& operator++();

      iterator begin();
      iterator end();
   };
   friend class BitIterator;

  public:
   LightUpdateGrouper(const U32 bitStart, const U32 bitEnd);
   ~LightUpdateGrouper();

   void addKey(const U32 key);
   U32  getKeyMask(const U32 key) const;

   BitIterator begin();
};


//--------------------------------------------------------------------------
inline LightUpdateGrouper::BitIterator& LightUpdateGrouper::BitIterator::operator++()
{
   mCurrBit++;
   resetKeyArray();

   return *this;
}

inline LightUpdateGrouper::BitIterator&  LightUpdateGrouper::BitIterator::operator++(int)
{
   return operator++();
}

inline LightUpdateGrouper::BitIterator::iterator LightUpdateGrouper::BitIterator::begin()
{
   if (valid() == false)
      return NULL;

   return mKeyArray.begin();
}

inline LightUpdateGrouper::BitIterator::iterator LightUpdateGrouper::BitIterator::end()
{
   if (valid() == false)
      return NULL;
   
   return mKeyArray.end();
}

inline bool LightUpdateGrouper::BitIterator::valid()
{
   return mCurrBit <= mGrouper->mBitEnd;
}

inline U32 LightUpdateGrouper::BitIterator::getNumKeys()
{
   return mKeyArray.size();
}

inline U32 LightUpdateGrouper::BitIterator::getMask()
{
   return (1 << mCurrBit);
}



#endif  // _H_LIGHTUPDATEGROUPER_
