//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSAPPTIME_H_
#define DTSAPPTIME_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"

namespace DTS
{
   class AppTime
   {
      static AppTime smDefaultTime;
      static F64 smTOL;
      static bool smPrintInt;
      static bool smPrintFloat;

      F64 f64;
      U32 u32;
      char * mBuffer;

     public:

      void set(F64 f, U32 i) { u32=i;f64=f; }

      // app can use however desired
      F32 getF32() const { return (F32)f64; }
      F64 getF64() const { return f64; }
      U32 getU32() const { return u32; }
      S32 getS32() const { return S32(u32); }

      const char * getStr() const;

      static const AppTime & DefaultTime() { return smDefaultTime; }
      static void SetDefaultTime(const AppTime & def) { smDefaultTime = def; }
      static F64 TOL() { return smTOL; }
      static void SetTOL(F64 tol) { smTOL = tol; }
      static void SetPrintInt(bool i) { smPrintInt=i; }
      static void SetPrintFloat(bool f) { smPrintFloat=f; }

      // operators...
      AppTime & operator+=(const AppTime & r)
      {
         f64 += r.f64;
         u32 += r.u32;
         return *this;
      }
      AppTime & operator-=(const AppTime & r)
      {
         f64 -= r.f64;
         u32 -= r.u32;
         return *this;
      }
      friend AppTime operator+(const AppTime & a, const AppTime & b)
      {
         return AppTime(a.f64+b.f64,a.u32+b.u32);
      }
      friend bool operator<(const AppTime & a, const AppTime & b)
      {
         return (a.f64 < b.f64 && a.u32 <= b.u32);
      }
      friend bool operator>(const AppTime & a, const AppTime & b)
      {
         return b<a;
      }
      friend bool operator<=(const AppTime & a, const AppTime & b)
      {
         return (a.f64 <= b.f64 && a.u32 <= b.u32);
      }
      friend bool operator>=(const AppTime & a, const AppTime & b)
      {
         return b<=a;
      }
      friend bool operator==(const AppTime & a, const AppTime & b)
      {
         return ((fabs(a.f64 - b.f64) < AppTime::TOL()) && (a.u32 == b.u32));
      }
      friend bool operator!=(const AppTime & a, const AppTime & b)
      {
         return !(a==b);
      }

      AppTime(F64 f, U32 i) { set(f,i); mBuffer=NULL; }
      AppTime() { set(0,0); mBuffer=NULL; }
      ~AppTime() { delete [] mBuffer; }
   };
};

#endif // DTSAPPTIME_H_
