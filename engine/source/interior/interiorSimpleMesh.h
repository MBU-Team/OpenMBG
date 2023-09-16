//-----------------------------------------------------------------------------
// Torque Game Engine Advanced 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INTERIORSIMPLEMESH_H_
#define _INTERIORSIMPLEMESH_H_

#include "core/tVector.h"
//#include "dgl/dgl.h"
#include "math/mBox.h"
#include "core/fileStream.h"
#include "ts/tsShapeInstance.h"


class InteriorSimpleMesh
{
public:
	class primitive
	{
	public:
		bool alpha;
		U32 texS;
		U32 texT;
		S32 diffuseIndex;
		S32 lightMapIndex;
		U32 start;
		U32 count;

      // used to relight the surface in-engine...
      PlaneF lightMapEquationX;
      PlaneF lightMapEquationY;
      Point2I lightMapOffset;
      Point2I lightMapSize;

		primitive()
		{
			alpha = false;
			texS = 2;
			texT = 2;
			diffuseIndex = 0;
			lightMapIndex = 0;
			start = 0;
			count = 0;

         lightMapEquationX = PlaneF(0, 0, 0, 0);
         lightMapEquationY = PlaneF(0, 0, 0, 0);
         lightMapOffset = Point2I(0, 0);
         lightMapSize = Point2I(0, 0);
		}
	};

	InteriorSimpleMesh()
	{
	}
	~InteriorSimpleMesh(){}

   Vector<U16> packedIndices;
   Vector<primitive> packedPrimitives;/// tri-list instead of strips

	bool hasSolid;
	bool hasTranslucency;
	Box3F bounds;
    MatrixF transform;
    Point3F scale;

	Vector<primitive> primitives;

	// same index relationship...
	Vector<U16> indices;
	Vector<Point3F> verts;
	Vector<Point3F> norms;
	Vector<Point2F> diffuseUVs;
	Vector<Point2F> lightmapUVs;

	TSMaterialList *materialList;

   bool read(Stream& stream);
   bool write(Stream& stream) const;
};

#endif //_INTERIORSIMPLEMESH_H_

