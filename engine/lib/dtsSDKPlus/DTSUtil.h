//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSUTIL_H_
#define DTSUTIL_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "appNode.h"

namespace DTS
{
#ifndef M_PI
   const F64 M_PI = 3.14159265358979323846;
#endif
   const char PATH_DELIM_CHAR = '\\';
   const char PATH_DELIM_STR[] = "\\";

   template <class T> inline const T & getmax(const T & t1, const T & t2) {  return t1>t2 ? t1 : t2; }

   // don't like std:vector insert/erase...

   template <class T>
   inline void delElement(std::vector<T> & vec, S32 idx) { vec.erase(&vec[idx]); }
   template <class T>
   inline void insElement(std::vector<T> & vec, S32 idx, const T & el) { vec.insert(&vec[idx],el); }

   // add a method that Torque vectors has but std::vectors don't
   template <class T>
   inline void appendVector(std::vector<T> & m1, const std::vector<T> & m2) { m1.insert(m1.end(),m2.begin(),m2.end()); }

   inline bool isZero(F32 f, F32 tol) { return fabs(f)<tol; }
   inline bool isZero(const Point2D & a, F32 tol) { return isZero(a.x(),tol) && isZero(a.y(),tol); }
   inline bool isZero(const Point3D & a, F32 tol) { return isZero(a.x(),tol) && isZero(a.y(),tol) && isZero(a.z(),tol); }
   inline bool isZero(const Quaternion & a, F32 tol) { isZero(a.x(),tol) && isZero(a.y(),tol) && isZero(a.z(),tol) && isZero(a.w(),tol); }
   inline bool isEqual(F32 a, F32 b, F32 tol) { return isZero(a-b,tol); }
   inline bool isEqual(const Point2D & a, const Point2D & b, F32 tol) { return isZero(a-b,tol); }
   inline bool isEqual(const Point3D & a, const Point3D & b, F32 tol) { return isZero(a-b,tol); }
   inline bool isEqual(const Quaternion & a, const Quaternion & b, F32 tol) { return isZero(a-b,tol); }
   extern bool isEqualQ16(const Quaternion & a, const Quaternion &b);

   inline F32 dotProduct(const Point3D & a, const Point3D & b) { return a.x() * b.x() + a.y() * b.y() + a.z() * b.z(); }
   extern void crossProduct(const Point3D & a, const Point3D & b, Point3D * c);

   template <class T>
   inline T* constructInPlace(T* p)
   {
      return new(p) T;
   }

   template <class T>
   inline void destructInPlace(T* p)
   {
      p->~T();
   }

	struct AffineParts
	{
      Point3D trans;       // Translation components
      Quaternion rot;      // Essential rotation
      Quaternion scaleRot; // Stretch rotation
      Point3D scale;       // Stretch factors
      F32 sign;            // Sign of determinant
	};

   extern void decomp_affine(const Matrix<4,4,F32> &, AffineParts *);
   extern void zapScale(Matrix<4,4,F32> &);

   extern void getLocalNodeTransform(AppNode * node, AppNode * parent, AffineParts & child0, AffineParts & parent0, const AppTime & time, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale);
   extern void getBlendNodeTransform(AppNode * node, AppNode * parent, AffineParts & child0, AffineParts & parent0, const AppTime & time, const AppTime & referenceTime, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale);
   extern void getDeltaTransform(AppNode * node, const AppTime & time1, const AppTime & time2, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale);

   inline void setMembershipArray(std::vector<bool> & m, bool setTo, S32 a, S32 b)
   {
      if (m.size()<b)
         m.resize(b);
      for (S32 i=a; i<b; i++)
         m[i]=setTo;
   }

   inline void setMembershipArray(std::vector<bool> & m, bool setTo, S32 a) 
   {
      m[a]=setTo;
   }

   inline bool allSet(std::vector<bool> & m)
   {
      for (S32 i=0; i<m.size(); i++)
      {
         if (!m[i]) return false;
      }
      return true;
   }

   inline void overlapSet(std::vector<bool> & m1, const std::vector<bool> & m2)
   {
      assert(m1.size()==m2.size());
      for (S32 i=0; i<m2.size(); i++)
      {
         if (m2[i])
            m1[i]=true;
      }
   }

   inline void subtractSet(std::vector<bool> & m1, const std::vector<bool> & m2)
   {
      assert(m1.size()==m2.size());
      for (S32 i=0; i<m2.size(); i++)
      {
         if (m2[i])
            m1[i]=false;
      }
   }

   extern S32 getTrailingNumber(const char * fullName);
   extern char * chopTrailingNumber(const char * fullName, S32 & size);
   extern void tweakName(const char ** name);
   extern const char* avar(const char *in_msg, ...);
   extern bool stringEqual(const char * s1, const char * s2);
   inline char * strnew(const char * str) { char * ret = new char[strlen(str)+1]; strcpy(ret,str); return ret; }
   extern char * getFileBase(char * str);
   extern const char * getFileBase(const char * str);
   extern char * getFilePath(const char * str, S32 pad=0);

   inline char * strnew(std::string str) { return strnew(str.c_str()); }
};

#endif // DTSUTIL_H_

