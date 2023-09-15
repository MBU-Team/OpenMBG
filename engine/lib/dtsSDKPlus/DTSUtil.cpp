//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "DTSUtil.h"
#include "appConfig.h"
#include "decomp/Decompose.h"
#include <stdarg.h>

#define TEST_DTS_MATH

namespace DTS
{

   const char* avar(const char *message, ...)
   {
      static char buffer[4096];
      va_list args;
      va_start(args, message);
      vsprintf(buffer, message, args);
      return( buffer );
   }


   bool isEqualQ16(const Quaternion & a, const Quaternion &b)
   {   
      U16 MAX_VAL = 0x7fff;

      // convert components to 16 bit, then test for equality
      S16 x, y, z, w;
      x = ((S16)(a.x() * float(MAX_VAL))) - ((S16)(b.x() * float(MAX_VAL)));
      y = ((S16)(a.y() * float(MAX_VAL))) - ((S16)(b.y() * float(MAX_VAL)));
      z = ((S16)(a.z() * float(MAX_VAL))) - ((S16)(b.z() * float(MAX_VAL)));
      w = ((S16)(a.w() * float(MAX_VAL))) - ((S16)(b.w() * float(MAX_VAL)));
      return (x==0) && (y==0) && (z==0) && (w==0);
   }

   void crossProduct(const Point3D & a, const Point3D & b, Point3D * c)
   {
      c->x(a.y() * b.z() - a.z() * b.y());
      c->y(a.z() * b.x() - a.x() * b.z());
      c->z(a.x() * b.y() - a.y() * b.x());
   }

   void convertToTransform(Matrix<4,4,F32> & mat, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale)
   {
       AffineParts parts;
       decomp_affine(mat,&parts);
       trans = parts.trans;
       rot = parts.rot;
       srot = parts.scaleRot;
       scale = parts.scale;
   }

   void decomp_affine(const Matrix<4,4,F32> & mat, AffineParts * parts)
   {
      GraphicGems::HMatrix ggMat;
      GraphicGems::AffineParts ggParts;
      for (S32 i=0; i<4; i++)
      {
         for (S32 j=0; j<4; j++)
            ggMat[i][j] = mat[i][j];
      }
      GraphicGems::decomp_affine(ggMat,&ggParts);
      parts->rot = Quaternion(ggParts.q.x,ggParts.q.y,ggParts.q.z,ggParts.q.w);
      parts->scale = Point3D(ggParts.k.x,ggParts.k.y,ggParts.k.z);
      parts->scaleRot = Quaternion(ggParts.u.x,ggParts.u.y,ggParts.u.z,ggParts.u.w);
      parts->trans = Point3D(ggParts.t.x,ggParts.t.y,ggParts.t.z);
      parts->sign = ggParts.f;

#ifdef TEST_DTS_MATH
      // Test math (but only in the unscaled case
      if (isEqual(parts->scale.x(),1.0f,0.01f) && isEqual(parts->scale.y(),1.0f,0.01f) && isEqual(parts->scale.z(),1.0f,0.01f))
      {
         Matrix<4,4,F32> mat2 = parts->rot.toMatrix();
         Vector<F32,4> col;
         col[0] = parts->trans.x();
         col[1] = parts->trans.y();
         col[2] = parts->trans.z();
         col[3] = 1;
         mat2.setCol(3,col);
         for (S32 i=0; i<4; i++)
         {
            for (S32 ii=0; ii<4; ii++)
            {
               if (!isEqual(mat[i][ii],mat2[i][ii],0.01f))
                  AppConfig::PrintDump(-1,"Doh!");
            }
         }
      }
#endif
   }

   void zapScale(Matrix<4,4,F32> & mat)
   {
      AffineParts parts;
      decomp_affine(mat,&parts);

      // now put the matrix back together again without the scale:
      // mat = mat.rot * mat.pos
      Vector<F32,4> trans;
      trans[0] = parts.trans.x();
      trans[1] = parts.trans.y();
      trans[2] = parts.trans.z();
      trans[3] = 1;
      mat = parts.rot.toMatrix();
      mat.setCol(3,trans);

#ifdef TEST_DTS_MATH
      {
         // A test...will get rid of once we know it works...
         Matrix<4,4,F32> mat2;
         decomp_affine(mat,&parts);
         trans[0] = parts.trans.x();
         trans[1] = parts.trans.y();
         trans[2] = parts.trans.z();
         trans[3] = 1;
         mat2 = parts.rot.toMatrix();
         mat2.setCol(3,trans);
         for (S32 i=0; i<4; i++)
         {
            for (S32 j=0; j<4; j++)
            {
               assert(isZero(mat[i][j]-mat2[i][j],0.01f));
            }
         }
      }
#endif
   }


   Matrix<4,4,F32> & getLocalNodeMatrix(AppNode * node, AppNode  * parent, const AppTime & time, Matrix<4,4,F32> & matrix, AffineParts & a10, AffineParts & a20)
   {
      // Here's the story:  the default transforms have no scale.  In order to account for scale, the
      // scale at the default time is folded into the object offset (which is multiplied into the points
      // before exporting).  Because of this, the local transform at a given time must take into account
      // the scale of the parent and child node at time 0 in addition to the current time.  In particular,
      // the world transform at a given time is WT(time) = T(time) * inverse(Tscale(0))

      // in order to avoid recomputing matrix at default time over and over, we assume that the first request
      // for the matrix will be at the default time, and thereafter, we will pass that matrix in and reuse it...
      Matrix<4,4,F32> m1 = node->getNodeTransform(time);
      Matrix<4,4,F32> m2;
      if (parent)
         m2 = parent->getNodeTransform(time);
      else
         m2 = Matrix<4,4,F32>::identity();
      if (time == AppTime::DefaultTime())
      {
         decomp_affine(m1,&a10);
         decomp_affine(m2,&a20);
      }

      // build the inverse scale matrices
      Matrix<4,4,F32> stretchRot10,stretchRot20;
      Matrix<4,4,F32> scaleMat10,scaleMat20;
      Matrix<4,4,F32> invScale10, invScale20;
      Point3D sfactor10, sfactor20;
      stretchRot10 = a10.scaleRot.toMatrix();
      stretchRot20 = a20.scaleRot.toMatrix();
      sfactor10 = Point3D(a10.sign/a10.scale.x(),a10.sign/a10.scale.y(),a10.sign/a10.scale.z());
      sfactor20 = Point3D(a20.sign/a20.scale.x(),a20.sign/a20.scale.y(),a20.sign/a20.scale.z());
      scaleMat10 = Matrix<4,4,F32>::identity();
      scaleMat10[0][0] = sfactor10.x();
      scaleMat10[1][1] = sfactor10.y();
      scaleMat10[2][2] = sfactor10.z();
      scaleMat20 = Matrix<4,4,F32>::identity();
      scaleMat20[0][0] = sfactor20.x();
      scaleMat20[1][1] = sfactor20.y();
      scaleMat20[2][2] = sfactor20.z();


      invScale10 = stretchRot10 * scaleMat10 * stretchRot10.inverse();
      invScale20 = stretchRot20 * scaleMat20 * stretchRot20.inverse();

      // build world transforms
      m1 = m1 * invScale10;
      m2 = m2 * invScale20;

      // build local transform
      matrix = m2.inverse() * m1;

#ifdef TEST_DTS_MATH
      {
         Matrix<4,4,F32> testMat;
         Matrix<4,4,F32> m2inv = m2.inverse();
         testMat = m2inv * m2;
         {
            for (S32 i=0; i<4; i++)
               for (S32 j=0; j<4; j++)
               {
                  F32 val = i==j ? 1.0f : 0.0f;
                  assert(isEqual(testMat[i][j],val,0.01f) && "assertion failed");
               }
         }
         testMat = m2 * m2inv;
         {
            for (S32 i=0; i<4; i++)
               for (S32 j=0; j<4; j++)
               {
                  F32 val = i==j ? 1.0f : 0.0f;
                  assert(isEqual(testMat[i][j],val,0.01f) && "assertion failed");
               }
         }
      }
#endif

      return matrix;
   }

   void getLocalNodeTransform(AppNode * node, AppNode * parent, AffineParts & child0, AffineParts & parent0, const AppTime & time, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale)
   {
      Matrix<4,4,F32> localMat;
      getLocalNodeMatrix(node,parent,time,localMat,child0,parent0);
      convertToTransform(localMat,rot,trans,srot,scale);
   }

   void getBlendNodeTransform(AppNode * node, AppNode * parent, AffineParts & child0, AffineParts & parent0, const AppTime & time, const AppTime & referenceTime, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale)
   {
      Matrix<4,4,F32> m1, invm1, m2, retMat;

      getLocalNodeMatrix(node, parent, referenceTime, m1, child0, parent0);
      getLocalNodeMatrix(node, parent,          time, m2, child0, parent0);
      invm1 = m1.inverse();
      retMat = invm1 * m2;
      convertToTransform(retMat, rot,trans,srot,scale);
   }

   void getDeltaTransform(AppNode * node, const AppTime & time1, const AppTime & time2, Quaternion & rot, Point3D & trans, Quaternion & srot, Point3D & scale)
   {
      Matrix<4,4,F32> m1 = node->getNodeTransform(time1);
      Matrix<4,4,F32> m2 = node->getNodeTransform(time2);
      convertToTransform(m1.inverse() * m2,rot,trans,srot,scale);
   }

   bool neverAnimateNode(AppNode*)
   {
      return false;
   }

   char * chopNum(char * s)
   {
      if (s==NULL)
          return NULL;
       
      char * p = s + strlen(s);

      if (p==s)
          return s;
      p--;

      // trim spaces from the end
      while (p!=s && *p==' ')
         p--;

      // back up until we reach a non-digit
      // gotta be better way than this...
      if (isdigit(*p))
         do
         {
            if (p--==s)
               return p+1;
         } while (isdigit(*p));
      
      // allow negative numbers, treat _ as - for Maya
      if (*p=='-' || *p=='_')
         p--;

      // trim spaces separating name and number
      while (*p==' ')
      {
         p--;
         if (p==s)
            return p;
      }

      // return first space if there was one,
      // o.w. return first digit
      return p+1;
   }

   char * chopTrailingNumber(const char * fullName, S32 & size)
   {
      if (!fullName)
      {
         size = -1;
         return NULL;
      }

      char * buffer = strnew(fullName);
      char * p = chopNum(buffer);
      if (*p=='\0')
      {
         size = -1;
         return buffer;
      }
    
      size = 1;
      if (*p=='_')
         size = -atoi(p+1);
      else
         size = atoi(p);
      *p='\0'; // terminate string
      return buffer;
   }

   S32 getTrailingNumber(const char * fullName)
   {
      S32 size;
      delete [] chopTrailingNumber(fullName,size);
      return size;
   }

   void tweakName(const char ** name)
   {
      char * pre[] = { "BB::", "BBZ::", "SORT::", "SEQUENCE::",
                       "BB_" , "BBZ_" , "SORT_" , "SEQUENCE_" ,  "" };
      for (S32 i=0; strlen(pre[i]) != 0; i++)
      {
         if (!strnicmp(*name,pre[i],strlen(pre[i])))
         {
            // found prefix...now skip the prefix and return
            *name += strlen(pre[i]);
            break;
         }
      }
   }

   const char * getFileBase(const char * str)
   {
      const char * ret = strrchr(str,'/');
      if (!ret)
         ret = strrchr(str,'\\');
      if (!ret)
         ret = strrchr(str,':');
      if (!ret)
         ret = str;
      else
         ++ret;
      return ret;
   }

   char * getFileBase(char * str)
   {
      return (char*)getFileBase((const char*)str);
   }

   char * getFilePath(const char * str, S32 pad)
   {
      const char * slash = getFileBase(str);
      S32 len = slash-str;
      char * ret = new char[len+1+pad];
      strncpy(ret,str,len);
      ret[len]='\0';
      return ret;
   }

   // perform string compare with wildcards (in s2 only) and case insensitivity
   bool stringEqual(const char * s1, const char * s2)
   {
       if (*s1=='\0' && *s2=='\0')
           return true;

       if (*s2=='*')
       {
           if (stringEqual(s1,s2+1))
               return true;
           if (*s1=='\0')
               return false;
           return stringEqual(s1+1,s2);
       }
       if (toupper(*s1)==toupper(*s2))
           return stringEqual(s1+1,s2+1);
       return false;
   }

};

