//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "math/mPoint.h"
#include "math/mMatrix.h"
#include "math/mRect.h"
#include "math/mBox.h"
#include "math/mathTypes.h"
#include "math/mRandom.h"

//------------------------------------------------------------------------------

static const char* getDataTypePoint2I(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   Point2I *pt = (Point2I *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d %d", pt->x, pt->y);
   return returnBuffer;
}

static void setDataTypePoint2I(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   if(argc == 1)
      dSscanf(argv[0], "%d %d", &((Point2I *) dptr)->x, &((Point2I *) dptr)->y);
   else if(argc == 2)
      *((Point2I *) dptr) = Point2I(dAtoi(argv[0]), dAtoi(argv[1]));
   else
      Con::printf("Point2I must be set as { x, y } or \"x y\"");
}

static const char* getDataTypePoint2F(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   Point2F *pt = (Point2F *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%f %f", pt->x, pt->y);
   return returnBuffer;
}

static void setDataTypePoint2F(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   if(argc == 1)
      dSscanf(argv[0], "%f %f", &((Point2F *) dptr)->x, &((Point2F *) dptr)->y);
   else if(argc == 2)
      *((Point2F *) dptr) = Point2F(dAtof(argv[0]), dAtof(argv[1]));
   else
      Con::printf("Point2F must be set as { x, y } or \"x y\"");
}

static const char *getDataTypePoint3F(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   Point3F *pt = (Point3F *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%g %g %g", pt->x, pt->y, pt->z);
   return returnBuffer;
}

static void setDataTypePoint3F(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   if(argc == 1)
      dSscanf(argv[0], "%f %f %f", &((Point3F *) dptr)->x, &((Point3F *) dptr)->y, &((Point3F *) dptr)->z);
   else if(argc == 3)
      *((Point3F *) dptr) = Point3F(dAtof(argv[0]), dAtof(argv[1]), dAtof(argv[2]));
   else
      Con::printf("Point3F must be set as { x, y, z } or \"x y z\"");
}

static const char *getDataTypePoint4F(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   Point4F *pt = (Point4F *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%g %g %g %g", pt->x, pt->y, pt->z, pt->w);
   return returnBuffer;
}

static void setDataTypePoint4F(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   if(argc == 1)
      dSscanf(argv[0], "%f %f %f %f", &((Point4F *) dptr)->x, &((Point4F *) dptr)->y, &((Point4F *) dptr)->z, &((Point4F *) dptr)->w);
   else if(argc == 4)
      *((Point4F *) dptr) = Point4F(dAtof(argv[0]), dAtof(argv[1]), dAtof(argv[2]), dAtof(argv[3]));
   else
      Con::printf("Point3F must be set as { x, y, z, w } or \"x y z w\"");
}

static const char *getDataTypeRectI(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   RectI *rect = (RectI *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%d %d %d %d", rect->point.x, rect->point.y,
            rect->extent.x, rect->extent.y);
   return returnBuffer;
}

static void setDataTypeRectI(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   if(argc == 1)
      dSscanf(argv[0], "%d %d %d %d", &((RectI *) dptr)->point.x, &((RectI *) dptr)->point.y,
              &((RectI *) dptr)->extent.x, &((RectI *) dptr)->extent.y);
   else if(argc == 4)
      *((RectI *) dptr) = RectI(dAtoi(argv[0]), dAtoi(argv[1]), dAtoi(argv[2]), dAtoi(argv[3]));
   else
      Con::printf("RectI must be set as { x, y, w, h } or \"x y w h\"");
}

static const char *getDataTypeRectF(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   RectF *rect = (RectF *) dptr;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%f %f %f %f", rect->point.x, rect->point.y,
            rect->extent.x, rect->extent.y);
   return returnBuffer;
}

static void setDataTypeRectF(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   if(argc == 1)
      dSscanf(argv[0], "%f %f %f %f", &((RectF *) dptr)->point.x, &((RectF *) dptr)->point.y,
              &((RectF *) dptr)->extent.x, &((RectF *) dptr)->extent.y);
   else if(argc == 4)
      *((RectF *) dptr) = RectF(dAtof(argv[0]), dAtof(argv[1]), dAtof(argv[2]), dAtof(argv[3]));
   else
      Con::printf("RectF must be set as { x, y, w, h } or \"x y w h\"");
}

static const char *getDataTypeMatrixPosition(void *dptr, EnumTable *, BitSet32 /*flag*/)
{
   F32 *col = (F32 *) dptr + 3;
   char* returnBuffer = Con::getReturnBuffer(256);
   if(col[12] == 1.f)
      dSprintf(returnBuffer, 256, "%g %g %g", col[0], col[4], col[8]);
   else
      dSprintf(returnBuffer, 256, "%g %g %g %g", col[0], col[4], col[8], col[12]);
   return returnBuffer;
}

static void setDataTypeMatrixPosition(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   F32 *col = ((F32 *) dptr) + 3;
   if (argc == 1)
   {
      col[0] = col[4] = col[8] = 0.f;
      col[12] = 1.f;
      dSscanf(argv[0], "%g %g %g %g", &col[0], &col[4], &col[8], &col[12]);
   }
   else
      if (argc <= 4) {
         for (S32 i = 0; i < argc; i++)
            col[i << 2] = dAtoi(argv[i]);
      }
      else
         Con::printf("Matrix position must be set as { x, y, z, w } or \"x y z w\"");
}

static const char *getDataTypeMatrixRotation(void * dptr, EnumTable *, BitSet32 /*flag*/)
{
   AngAxisF aa(*(MatrixF *) dptr);
   aa.axis.normalize();
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g %g",aa.axis.x,aa.axis.y,aa.axis.z,mRadToDeg(aa.angle));
   return returnBuffer;
}

static void setDataTypeMatrixRotation(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   // DMM: Note that this will ONLY SET the ULeft 3x3 submatrix.
   //
   AngAxisF aa(Point3F(0,0,0),0);
   if (argc == 1)
   {
      dSscanf(argv[0], "%g %g %g %g", &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
      aa.angle = mDegToRad(aa.angle);
   }
   else
      if (argc == 4) {
         for (S32 i = 0; i < argc; i++)
            ((F32*)&aa)[i] = dAtof(argv[i]);
         aa.angle = mDegToRad(aa.angle);
      }
      else
         Con::printf("Matrix rotation must be set as { x, y, z, angle } or \"x y z angle\"");

   //
   MatrixF temp;
   aa.setMatrix(&temp);

   F32* pDst = *(MatrixF *)dptr;
   const F32* pSrc = temp;
   for (U32 i = 0; i < 3; i++)
      for (U32 j = 0; j < 3; j++)
         pDst[i*4 + j] = pSrc[i*4 + j];
}

static const char *getDataTypeBox3F(void * dptr, EnumTable *, BitSet32 /*flag*/)
{
   const Box3F* pBox = (const Box3F*)dptr;

   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 256, "%g %g %g %g %g %g",
            pBox->min.x, pBox->min.y, pBox->min.z,
            pBox->max.x, pBox->max.y, pBox->max.z);

   return returnBuffer;
}

static void setDataTypeBox3F(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32 /*flag*/)
{
   Box3F* pDst = (Box3F*)dptr;

   if (argc == 1) {
      U32 args = dSscanf(argv[0], "%f %f %f %f %f %f",
                         &pDst->min.x, &pDst->min.y, &pDst->min.z,
                         &pDst->max.x, &pDst->max.y, &pDst->max.z);
      AssertWarn(args == 6, "Warning, box probably not read properly");
   } else {
      Con::printf("Box3F must be set as \"xMin yMin zMin xMax yMax zMax\"");
   }
}


//----------------------------------------------------------------------------

ConsoleFunctionGroupBegin( VectorMath, "Vector manipulation functions.");

ConsoleFunction( VectorAdd, const char*, 3, 3, "(Vector3F a, Vector3F b) Returns a+b.")
{
   VectorF v1(0,0,0),v2(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v1.x,&v1.y,&v1.z);
   dSscanf(argv[2],"%f %f %f",&v2.x,&v2.y,&v2.z);
   VectorF v;
   v = v1 + v2;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g",v.x,v.y,v.z);
   return returnBuffer;
}

ConsoleFunction( VectorSub, const char*, 3, 3, "(Vector3F a, Vector3F b) Returns a-b.")
{
   VectorF v1(0,0,0),v2(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v1.x,&v1.y,&v1.z);
   dSscanf(argv[2],"%f %f %f",&v2.x,&v2.y,&v2.z);
   VectorF v;
   v = v1 - v2;
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g",v.x,v.y,v.z);
   return returnBuffer;
}

ConsoleFunction( VectorScale, const char*, 3, 3, "(Vector3F a, float scalar) Returns a scaled by scalar (ie, a*scalar).")
{
   VectorF v(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v.x,&v.y,&v.z);
   v *= dAtof(argv[2]);
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g",v.x,v.y,v.z);
   return returnBuffer;
}

ConsoleFunction( VectorNormalize, const char*, 2, 2, "(Vector3F a) Returns a scaled such that length(a) = 1.")
{
   VectorF v(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v.x,&v.y,&v.z);
   if (v.len() != 0)
      v.normalize();
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g",v.x,v.y,v.z);
   return returnBuffer;
}

ConsoleFunction( VectorDot, F32, 3, 3, "(Vector3F a, Vector3F b) Calculate the dot product of a and b.")
{
   VectorF v1(0,0,0),v2(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v1.x,&v1.y,&v1.z);
   dSscanf(argv[2],"%f %f %f",&v2.x,&v2.y,&v2.z);
   return mDot(v1,v2);
}

ConsoleFunction(VectorCross, const char*, 3, 3, "(Vector3F a, Vector3F b) Calculate the cross product of a and b.")
{
   VectorF v1(0,0,0),v2(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v1.x,&v1.y,&v1.z);
   dSscanf(argv[2],"%f %f %f",&v2.x,&v2.y,&v2.z);
   VectorF v;
   mCross(v1,v2,&v);
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g",v.x,v.y,v.z);
   return returnBuffer;
}

ConsoleFunction(VectorDist, F32, 3, 3, "(Vector3F a, Vector3F b) Calculate the distance between a and b.")
{
   VectorF v1(0,0,0),v2(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v1.x,&v1.y,&v1.z);
   dSscanf(argv[2],"%f %f %f",&v2.x,&v2.y,&v2.z);
   VectorF v = v2 - v1;
   return mSqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

ConsoleFunction(VectorLen, F32, 2, 2, "(Vector3F v) Calculate the length of a vector.")
{
   VectorF v(0,0,0);
   dSscanf(argv[1],"%f %f %f",&v.x,&v.y,&v.z);
   return mSqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}

ConsoleFunction( VectorOrthoBasis, const char*, 2, 2, "(AngAxisF aaf) Create an orthogonal basis from the given vector. Return a matrix.")
{
   AngAxisF aa;
   dSscanf(argv[1],"%f %f %f %f", &aa.axis.x,&aa.axis.y,&aa.axis.z,&aa.angle);
   MatrixF mat;
   aa.setMatrix(&mat);
   Point3F col0, col1, col2;
   mat.getColumn(0, &col0);
   mat.getColumn(1, &col1);
   mat.getColumn(2, &col2);
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g %g %g %g %g %g %g",
            col0.x, col0.y, col0.z, col1.x, col1.y, col1.z, col2.x, col2.y, col2.z);
   return returnBuffer;
}

ConsoleFunctionGroupEnd(VectorMath);

ConsoleFunctionGroupBegin(MatrixMath, "Matrix manipulation functions.");

ConsoleFunction( MatrixCreate, const char*, 3, 3, "(Vector3F pos, Vector3F rot) Create a matrix representing the given translation and rotation.")
{
   Point3F pos;
   dSscanf(argv[1], "%g %g %g", &pos.x, &pos.y, &pos.z);

   AngAxisF aa(Point3F(0,0,0),0);
   dSscanf(argv[2], "%g %g %g %g", &aa.axis.x, &aa.axis.y, &aa.axis.z, &aa.angle);
   aa.angle = mDegToRad(aa.angle);
   
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer, 255, "%g %g %g %g %g %g %g", 
            pos.x, pos.y, pos.z,
            aa.axis.x, aa.axis.y, aa.axis.z,
            aa.angle);
   return returnBuffer;
}

ConsoleFunction( MatrixCreateFromEuler, const char*, 2, 2, "(Vector3F e) Create a matrix from the given rotations.")
{
   EulerF rot;
   dSscanf(argv[1], "%g %g %g", &rot.x, &rot.y, &rot.z);

   QuatF rotQ(rot);
   AngAxisF aa;
   aa.set(rotQ);
   
   char* ret = Con::getReturnBuffer(256);
   dSprintf(ret, 255, "0 0 0 %g %g %g %g",aa.axis.x,aa.axis.y,aa.axis.z,aa.angle);
   return ret;
}


ConsoleFunction( MatrixMultiply, const char*, 3, 3, "(Matrix4F left, Matrix4F right) Multiply the two matrices.")
{
   Point3F pos1;
   AngAxisF aa1(Point3F(0,0,0),0);
   dSscanf(argv[1], "%g %g %g %g %g %g %g", &pos1.x, &pos1.y, &pos1.z, &aa1.axis.x, &aa1.axis.y, &aa1.axis.z, &aa1.angle);

   MatrixF temp1(true);
   aa1.setMatrix(&temp1);
   temp1.setColumn(3, pos1);

   Point3F pos2;
   AngAxisF aa2(Point3F(0,0,0),0);
   dSscanf(argv[2], "%g %g %g %g %g %g %g", &pos2.x, &pos2.y, &pos2.z, &aa2.axis.x, &aa2.axis.y, &aa2.axis.z, &aa2.angle);

   MatrixF temp2(true);
   aa2.setMatrix(&temp2);
   temp2.setColumn(3, pos2);

   temp1.mul(temp2);


   Point3F pos;
   AngAxisF aa(temp1);

   aa.axis.normalize();
   temp1.getColumn(3, &pos);

   char* ret = Con::getReturnBuffer(256);
   dSprintf(ret, 255, "%g %g %g %g %g %g %g", 
            pos.x, pos.y, pos.z,
            aa.axis.x, aa.axis.y, aa.axis.z,
            aa.angle);
   return ret;
}


ConsoleFunction( MatrixMulVector, const char*, 3, 3, "(MatrixF xfrm, Point3F vector) Multiply the vector by the transform.")
{
   Point3F pos1;
   AngAxisF aa1(Point3F(0,0,0),0);
   dSscanf(argv[1], "%g %g %g %g %g %g %g", &pos1.x, &pos1.y, &pos1.z, &aa1.axis.x, &aa1.axis.y, &aa1.axis.z, &aa1.angle);

   MatrixF temp1(true);
   aa1.setMatrix(&temp1);
   temp1.setColumn(3, pos1);

   Point3F vec1;
   dSscanf(argv[2], "%g %g %g", &vec1.x, &vec1.y, &vec1.z);

   Point3F result;
   temp1.mulV(vec1, &result);

   char* ret = Con::getReturnBuffer(256);
   dSprintf(ret, 255, "%g %g %g", result.x, result.y, result.z);
   return ret;
}

ConsoleFunction( MatrixMulPoint, const char*, 3, 3, "(MatrixF xfrm, Point3F pnt) Multiply pnt by xfrm.")
{
   Point3F pos1;
   AngAxisF aa1(Point3F(0,0,0),0);
   dSscanf(argv[1], "%g %g %g %g %g %g %g", &pos1.x, &pos1.y, &pos1.z, &aa1.axis.x, &aa1.axis.y, &aa1.axis.z, &aa1.angle);

   MatrixF temp1(true);
   aa1.setMatrix(&temp1);
   temp1.setColumn(3, pos1);

   Point3F vec1;
   dSscanf(argv[2], "%g %g %g", &vec1.x, &vec1.y, &vec1.z);

   Point3F result;
   temp1.mulP(vec1, &result);

   char* ret = Con::getReturnBuffer(256);
   dSprintf(ret, 255, "%g %g %g", result.x, result.y, result.z);
   return ret;
}

ConsoleFunctionGroupEnd(MatrixMath);

//------------------------------------------------------------------------------

ConsoleFunction( getBoxCenter, const char*, 2, 2, "(Box b) Get the center point of a box.")
{
   Box3F box;
   box.min.set(0,0,0);
   box.max.set(0,0,0);
   dSscanf(argv[1],"%f %f %f %f %f %f",
           &box.min.x,&box.min.y,&box.min.z,
           &box.max.x,&box.max.y,&box.max.z);
   Point3F p;
   box.getCenter(&p);
   char* returnBuffer = Con::getReturnBuffer(256);
   dSprintf(returnBuffer,256,"%g %g %g",p.x,p.y,p.z);
   return returnBuffer;
}


//------------------------------------------------------------------------------
ConsoleFunctionGroupBegin(RandomNumbers, "Functions relating to the generation of random numbers.");

ConsoleFunction(setRandomSeed, void, 1, 2, "(int seed=-1) Set the current random seed. If no seed is provided, then the current time in ms is used.")
{
	U32 seed = Platform::getRealMilliseconds();
	if (argc == 2)
		seed = dAtoi(argv[1]);
	MRandomLCG::setGlobalRandSeed(seed);
}

ConsoleFunction(getRandomSeed, S32, 1, 1, "Return the current random seed.")
{
   return gRandGen.getSeed();
}

S32 mRandI(S32 i1, S32 i2)
{
   return gRandGen.randI(i1, i2);
}

F32 mRandF(F32 f1, F32 f2)
{
   return gRandGen.randF(f1, f2);
}

ConsoleFunction(getRandom, F32, 1, 3, "(int a=1, int b=0)"
                "Get a random number between a and b.")
{
   if (argc == 2)
      return F32(gRandGen.randI(0,dAtoi(argv[1])));
   else
   {
      if (argc == 3) {
         S32 min = dAtoi(argv[1]);
         S32 max = dAtoi(argv[2]);
         if (min > max) {
            S32 t = min;
            min = max;
            max = t;
         }
         return F32(gRandGen.randI(min,max));
      }
   }
   return gRandGen.randF();
}

ConsoleFunctionGroupEnd(RandomNumbers);
//------------------------------------------------------------------------------

void RegisterMathTypes(void)
{
   //register types
   Con::registerType("Point2I",           TypePoint2I,         sizeof(Point2I),        getDataTypePoint2I,        setDataTypePoint2I);
   Con::registerType("Point2F",           TypePoint2F,         sizeof(Point2F),        getDataTypePoint2F,        setDataTypePoint2F);
   Con::registerType("Point3F",           TypePoint3F,         sizeof(Point3F),        getDataTypePoint3F,        setDataTypePoint3F);
   Con::registerType("Point4F",           TypePoint4F,         sizeof(Point4F),        getDataTypePoint4F,        setDataTypePoint4F);
   Con::registerType("RectI",             TypeRectI,           sizeof(RectI),          getDataTypeRectI,          setDataTypeRectI);
   Con::registerType("RectF",             TypeRectF,           sizeof(RectF),          getDataTypeRectF,          setDataTypeRectF);
   Con::registerType("MatrixPosition",    TypeMatrixPosition,  sizeof(4*sizeof(F32)),  getDataTypeMatrixPosition, setDataTypeMatrixPosition);
   Con::registerType("MatrixRotation",    TypeMatrixRotation,  sizeof(MatrixF),        getDataTypeMatrixRotation, setDataTypeMatrixRotation);
   Con::registerType("Box3F",             TypeBox3F,           sizeof(Box3F),          getDataTypeBox3F,          setDataTypeBox3F);
}
