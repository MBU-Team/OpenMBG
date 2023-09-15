//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsIntegerSet.h"
#include "platform/platform.h"
#include "core/stream.h"

#define SETUPTO(upto) ( ((1<<(upto&31))-1)*2+1 ) // careful not to shift more than 31 times

void TSIntegerSet::clearAll(S32 upto)
{
   AssertFatal(upto<=MAX_TS_SET_SIZE,"TSIntegerSet::clearAll: out of range");

   dMemset(bits,0,(upto>>5)*4);
   if (upto&31)
      bits[upto>>5] &= ~SETUPTO(upto);
}

void TSIntegerSet::setAll(S32 upto)
{
   AssertFatal(upto<=MAX_TS_SET_SIZE,"TSIntegerSet::setAll: out of range");

   dMemset(bits,0xFFFFFFFF,(upto>>5)*4);
   if (upto&31)
      bits[upto>>5] |= SETUPTO(upto);
}

bool TSIntegerSet::testAll(S32 upto) const
{
   AssertFatal(upto<=MAX_TS_SET_SIZE,"TSIntegerSet::testAll: out of range");
   S32 i;
   for (i=0; i<(upto>>5); i++)
      if (bits[i])
         return true;
   if (upto&31)
      return (bits[upto>>5] & SETUPTO(upto)) != 0;
   return false;
}

void TSIntegerSet::intersect(const TSIntegerSet & otherSet)
{
   for (S32 i=0; i<MAX_TS_SET_DWORDS; i++)
      bits[i] &= otherSet.bits[i];
}

void TSIntegerSet::overlap(const TSIntegerSet & otherSet)
{
   for (S32 i=0; i<MAX_TS_SET_DWORDS; i++)
      bits[i] |= otherSet.bits[i];
}

void TSIntegerSet::difference(const TSIntegerSet & otherSet)
{
   for (S32 i=0; i<MAX_TS_SET_DWORDS; i++)
      bits[i] = (bits[i] | otherSet.bits[i]) & ~(bits[i] & otherSet.bits[i]);
}

void TSIntegerSet::takeAway(const TSIntegerSet & otherSet)
{
   for (S32 i=0; i<MAX_TS_SET_DWORDS; i++)
      bits[i] &= ~otherSet.bits[i];
}

S32 TSIntegerSet::start() const
{
   for (S32 i=0; i<MAX_TS_SET_DWORDS; i++)
   {
      // search for set bit one dword at a time
      U32 dword = bits[i];
      if (dword!=0)
      {
         // got dword, now search one byte at a time
         S32 j = 0;
         U32 mask = 0xFF;
         do
         {
            if (dword&mask)
            {
               // got byte, now search one bit at a time
               U32 bit = mask & ~(mask<<1); // grabs the smallest bit
               do
               {
                  if (dword&bit)
                     return (i<<5)+j;
                  j++;
                  bit <<= 1;
               } while (1);
            }
            mask <<= 8;
            j += 8;
         } while (1);
      }
   }

   return MAX_TS_SET_SIZE;
}

S32 TSIntegerSet::end() const
{
   for (S32 i=MAX_TS_SET_DWORDS-1; i>=0; i--)
   {
      // search for set bit one dword at a time
      U32 dword = bits[i];
      if (bits[i])
      {
         // got dword, now search one byte at a time
         S32 j = 31;
         U32 mask = 0xFF000000;
         do
         {
            if (dword&mask)
            {
               // got byte, now one bit at a time
               U32 bit = mask & ~(mask>>1); // grabs the highest bit
               do
               {
                  if (dword&bit)
                     return (i<<5)+j+1;
                  j--;
                  bit >>= 1;
               } while (1);
            }
            mask >>= 8;
            j -= 8;
         } while (1);
      }
   }
      
   return 0;
}

void TSIntegerSet::next(S32 & i) const
{
   i++;
   U32 idx = i>>5;
   U32 bit = 1 << (i&31);
   U32 dword = bits[idx] & ~(bit-1);
   while (dword==0)
   {
      i = (i+32) & ~31;
      if (i>=MAX_TS_SET_SIZE)
         return;
      dword=bits[++idx];
      bit = 1;
   }
   dword = bits[idx];
   while ( (bit & dword) == 0)
   {
      bit <<= 1;
      i++;
   }
}

/* Or would one byte at a time be better...
void TSIntegerSet::next(S32 & i)
{
   U32 idx = i>>3;
   U8 bit = 1 << (i&7);
   U8 byte = ((U8*)bits)[idx] & ~(bit*2-1);
   while (byte==0)
   {
      i = (i+8) & ~7;
      if (i>=MAX_TS_SET_SIZE)
         return;
      byte=((U8*)bits)[++idx];
      bit = 1;
   }
   byte = ((U8*)bits)[idx];
   while (bit & byte == 0)
   {
      bit <<= 1;
      i++;
   }
}
*/

void TSIntegerSet::copy(const TSIntegerSet & otherSet)
{
   dMemcpy(bits,otherSet.bits,MAX_TS_SET_DWORDS*4);
}

TSIntegerSet::TSIntegerSet()
{
   clearAll();
}

TSIntegerSet::TSIntegerSet(const TSIntegerSet & otherSet)
{
   copy(otherSet);
}

void TSIntegerSet::read(Stream * s)
{
   clearAll();

   S32 numInts;
   s->read(&numInts); // don't care about this

   S32 sz;
   s->read(&sz);
   AssertFatal(sz<=MAX_TS_SET_DWORDS,"TSIntegerSet::  set too large...increase max set size and re-compile");

   for (S32 i=0; i<sz; i++) // now mirrors the write code...
      s->read(&(bits[i]));
}

void TSIntegerSet::write(Stream * s)
{
   s->write((S32)0); // don't do this anymore, keep in to avoid versioning
   S32 i,sz=0;
   for (i=0; i<MAX_TS_SET_DWORDS; i++)
      if (bits[i]!=0)
         sz=i+1;
   s->write(sz);
   for (i=0; i<sz; i++)
      s->write(bits[i]);
}

