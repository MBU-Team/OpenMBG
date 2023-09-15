//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSMesh.h"
#include "DTSPlusTypes.h"
#include "DTSUtil.h"

// This is rather painful.  BitVector and BitMatrix copied from engine repository
// in order to work with triangle stripper.  Would rather use NVidia stripper, but
// there is a bug in it whereby we sometimes get flipped faces.  So we are stuck
// with the old quick/dirty/ugly (but supremely reliable) original implementation.

namespace DTS
{

   #define AssertFatal(a,b) assert(a && b)
   class BitVector
   {
      U8*   mBits;
      U32   mByteSize;

      U32   mSize;

      static U32 calcByteSize(const U32 numBits);

     public:
      BitVector();
      BitVector(const U32 _size);
      ~BitVector();

      /// @name Size Management
      /// @{
      void setSize(const U32 _size);
      U32  getSize() const;
      U32  getByteSize() const;
      U32  getAllocatedByteSize() const { return mByteSize; }

      const U8* getBits() const { return mBits; }
      U8*       getNCBits()     { return mBits; }

      /// @}

      bool copy(const BitVector& from);

      /// @name Mutators
      /// Note that bits are specified by index, unlike BitSet32.
      /// @{

      /// Clear all the bits.
      void clear();

      /// Set all the bits.
      void set();

      /// Set the specified bit.
      void set(U32 bit);
      /// Clear the specified bit.
      void clear(U32 bit);
      /// Test that the specified bit is set.
      bool test(U32 bit) const;

      /// @}
   };

   inline BitVector::BitVector()
   {
      mBits     = NULL;
      mByteSize = 0;

      mSize = 0;
   }


   inline BitVector::BitVector(const U32 _size)
   {
      mBits     = NULL;
      mByteSize = 0;

      mSize = 0;

      setSize(_size);
   }

   inline BitVector::~BitVector()
   {
      delete [] mBits;
      mBits = NULL;
      mByteSize = 0;

      mSize = 0;
   }

   inline U32 BitVector::calcByteSize(const U32 numBits)
   {
      // Make sure that we are 32 bit aligned
      //
      return (((numBits + 0x7) >> 3) + 0x3) & ~0x3;
   }

   inline void BitVector::setSize(const U32 _size)
   {
      if (_size != 0) {
         U32 newSize = calcByteSize(_size);
         if (mByteSize < newSize) {
            delete [] mBits;
            mBits     = new U8[newSize];
            mByteSize = newSize;
         }
      } else {
         delete [] mBits;
         mBits     = NULL;
         mByteSize = 0;
      }

      mSize = _size;
   }

   inline U32 BitVector::getSize() const
   {
      return mSize;
   }

   inline U32 BitVector::getByteSize() const
   {
      return calcByteSize(mSize);
   }

   inline void BitVector::clear()
   {
      if (mSize != 0)
         memset(mBits, 0x00, calcByteSize(mSize));
   }

   inline bool BitVector::copy(const BitVector& from)
   {
      U32   sourceSize = from.getSize();
      if (sourceSize) {
         setSize(sourceSize);
         memcpy(mBits, from.getBits(), getByteSize());
         return true;
      }
      return false;
   }

   inline void BitVector::set()
   {
      if (mSize != 0)
         memset(mBits, 0xFF, calcByteSize(mSize));
   }

   inline void BitVector::set(U32 bit)
   {
      AssertFatal(bit < mSize, "Error, out of range bit");

      mBits[bit >> 3] |= U8(1 << (bit & 0x7));
   }

   inline void BitVector::clear(U32 bit)
   {
      AssertFatal(bit < mSize, "Error, out of range bit");

      mBits[bit >> 3] &= U8(~(1 << (bit & 0x7)));
   }

   inline bool BitVector::test(U32 bit) const
   {
      AssertFatal(bit < mSize, "Error, out of range bit");

      return (mBits[bit >> 3] & U8(1 << (bit & 0x7))) != 0;
   }

   class BitMatrix
   {
      U32 mWidth;
      U32 mHeight;
      U32 mRowByteWidth;

      U8* mBits;
      U32 mSize;

      BitVector mColFlags;
      BitVector mRowFlags;

     public:

      /// Create a new bit matrix.
      ///
      /// @param  width    Width of matrix in bits.
      /// @param  height   Height of matrix in bits.
      BitMatrix(const U32 width, const U32 height);
      ~BitMatrix();

      /// @name Setters
      /// @{

      /// Set all the bits in the matrix to false.
      void clearAllBits();

      /// Set all the bits in the matrix to true.
      void setAllBits();

      /// Set a bit at a given location in the matrix.
      void setBit(const U32 x, const U32 y);

      /// Clear a bit at a given location in the matrix.
      void clearBit(const U32 x, const U32 y);

      /// @}

      /// @name Queries
      /// @{

      /// Is the specified bit set?
      bool isSet(const U32 x, const U32 y) const;

      /// Is any bit in the given column set?
      bool isAnySetCol(const U32 x);

      /// Is any bit in the given row set?
      bool isAnySetRow(const U32 y);

      /// @}
   };

   inline BitMatrix::BitMatrix(const U32 width, const U32 height)
    : mColFlags(width),
      mRowFlags(height)
   {
      AssertFatal(width != 0 && height != 0, "Error, w/h must be non-zero");

      mWidth        = width;
      mHeight       = height;
      mRowByteWidth = (width + 7) >> 3;

      mSize         = mRowByteWidth * mHeight;
      mBits         = new U8[mSize];
   }

   inline BitMatrix::~BitMatrix()
   {
      mWidth        = 0;
      mHeight       = 0;
      mRowByteWidth = 0;
      mSize         = 0;

      delete [] mBits;
      mBits = NULL;
   }

   inline void BitMatrix::clearAllBits()
   {
      AssertFatal(mBits != NULL, "Error, clearing after deletion");

      memset(mBits, 0x00, mSize);
      mColFlags.clear();
      mRowFlags.clear();
   }

   inline void BitMatrix::setAllBits()
   {
      AssertFatal(mBits != NULL, "Error, setting after deletion");

      memset(mBits, 0xFF, mSize);
      mColFlags.set();
      mRowFlags.set();
   }

   inline void BitMatrix::setBit(const U32 x, const U32 y)
   {
      AssertFatal(x < mWidth && y < mHeight, "Error, out of bounds bit!");

      U8* pRow = &mBits[y * mRowByteWidth];

      U8* pByte = &pRow[x >> 3];
      *pByte   |= 1 << (x & 0x7);

      mColFlags.set(x);
      mRowFlags.set(y);
   }

   inline void BitMatrix::clearBit(const U32 x, const U32 y)
   {
      AssertFatal(x < mWidth && y < mHeight, "Error, out of bounds bit!");

      U8* pRow = &mBits[y * mRowByteWidth];

      U8* pByte = &pRow[x >> 3];
      *pByte   &= ~(1 << (x & 0x7));
   }

   inline bool BitMatrix::isSet(const U32 x, const U32 y) const
   {
      AssertFatal(x < mWidth && y < mHeight, "Error, out of bounds bit!");

      U8* pRow = &mBits[y * mRowByteWidth];

      U8* pByte = &pRow[x >> 3];
      return (*pByte & (1 << (x & 0x7))) != 0;
   }

   inline bool BitMatrix::isAnySetCol(const U32 x)
   {
      AssertFatal(x < mWidth, "Error, out of bounds column!");

      return mColFlags.test(x);
   }

   inline bool BitMatrix::isAnySetRow(const U32 y)
   {
      AssertFatal(y < mHeight, "Error, out of bounds row!");

      return mRowFlags.test(y);
   }
} // namespace DTS
