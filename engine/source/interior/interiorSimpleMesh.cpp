//-----------------------------------------------------------------------------
// Torque Game Engine Advanced 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------
#include "interior/interiorSimpleMesh.h"

#include "interior/interiorLMManager.h"

#include "console/console.h"
#include "sim/sceneObject.h"
#include "math/mathIO.h"
#include "sceneGraph/sceneGraph.h"


// Checks for polygon level collision with given planes
U32 _whichSide(PlaneF pln, Point3F* verts)
{
	Point3F currv, nextv;
	S32 csd, nsd;

	// Find out which side the first vert is on
	U32 side = PlaneF::On;
	currv = verts[0];
	csd = pln.whichSide(currv);
	if(csd != PlaneF::On)
		side = csd;

	for(U32 k = 1; k < 3; k++)
	{
		nextv = verts[k];
		nsd = pln.whichSide(nextv);
		if((csd == PlaneF::Back && nsd == PlaneF::Front) ||
			(csd == PlaneF::Front && nsd == PlaneF::Back))
			return 2;
		else if (nsd != PlaneF::On)
			side = nsd;
		currv = nextv;
		csd = nsd;
	}

	// Loop back to the first vert
	nextv = verts[0];
	nsd = pln.whichSide(nextv);
	if((csd == PlaneF::Back && nsd == PlaneF::Front) ||
		(csd == PlaneF::Front && nsd == PlaneF::Back))
		return 2;
	else if(nsd != PlaneF::On)
		side = nsd;
	return side;

}


//bool InteriorSimpleMesh::castRay(const Point3F &start, const Point3F &end, RayInfo* info)
//{
//	bool found = false;
//	F32 best_t = F32_MAX;
//	Point3F best_normal = Point3F(0, 0, 1);
//	Point3F dir = end - start;
//
//	for(U32 p=0; p<primitives.size(); p++)
//	{
//		primitive &prim = primitives[p];
//		for(U32 t=2; t<prim.count; t++)
//		{
//			Point3F &v1 = verts[prim.start+t-2];
//			Point3F &v2 = verts[prim.start+t-1];
//			Point3F &v3 = verts[prim.start+t];
//
//			F32 cur_t = 0;
//			Point2F b;
//
//			if(castRayTriangle(start, dir, v1, v2, v3, cur_t, b))
//			{
//				if(cur_t < best_t)
//				{
//					best_t = cur_t;
//					best_normal = norms[prim.start+t];
//					found = true;
//				}
//			}
//		}
//	}
//
//	if(found && info)
//	{
//		info->t = best_t;
//		info->normal = best_normal;
//		info->material = 0;
//	}
//
//	return found;
//}

bool InteriorSimpleMesh::read(Stream& stream)
{
   // Simple serialization
   S32 vectorSize = 0;

   // Primitives
   stream.read(&vectorSize);
   primitives.setSize(vectorSize);
   for (U32 i = 0; i < primitives.size(); i++)
   {
      stream.read(&primitives[i].alpha);
		stream.read(&primitives[i].texS);
		stream.read(&primitives[i].texT);
		stream.read(&primitives[i].diffuseIndex);
		stream.read(&primitives[i].lightMapIndex);
		stream.read(&primitives[i].start);
		stream.read(&primitives[i].count);
		
      mathRead(stream, &primitives[i].lightMapEquationX);
      mathRead(stream, &primitives[i].lightMapEquationY);
      mathRead(stream, &primitives[i].lightMapOffset);
      mathRead(stream, &primitives[i].lightMapSize);
   }

   // Indices
   stream.read(&vectorSize);
   indices.setSize(vectorSize);
   for (U32 i = 0; i < indices.size(); i++)
      stream.read(&indices[i]);

   // Vertices
   stream.read(&vectorSize);
   verts.setSize(vectorSize);
   for (U32 i = 0; i < verts.size(); i++)
      mathRead(stream, &verts[i]);

   // Normals
   stream.read(&vectorSize);
   norms.setSize(vectorSize);
   for (U32 i = 0; i < norms.size(); i++)
      mathRead(stream, &norms[i]);

   // Diffuse UVs
   stream.read(&vectorSize);
   diffuseUVs.setSize(vectorSize);
   for (U32 i = 0; i < diffuseUVs.size(); i++)
      mathRead(stream, &diffuseUVs[i]);

   // Lightmap UVs
   stream.read(&vectorSize);
   lightmapUVs.setSize(vectorSize);
   for (U32 i = 0; i < lightmapUVs.size(); i++)
      mathRead(stream, &lightmapUVs[i]);

   // Material list
   bool hasMaterialList = false;
   stream.read(&hasMaterialList);
   if (hasMaterialList)
   {
      // Since we are doing this externally to a TSShape read we need to
      // make sure that our read version is the same as our write version.
      // It is possible that it was changed along the way by a loaded TSShape.
      TSShape::smReadVersion = 25;

      if (materialList)
         delete materialList;

      materialList = new TSMaterialList;
      materialList->read(stream);
   }
   else
      materialList = NULL;

   // Diffuse bitmaps
   stream.read(&vectorSize);
   for (U32 i = 0; i < vectorSize; i++)
   {
      // need to read these
      bool hasBitmap = false;
      stream.read(&hasBitmap);
      if(hasBitmap)
      {
         GBitmap* bitMap = new GBitmap;
         bitMap->readPNG(stream);
         delete bitMap;
      }
   }

   // Misc data
   stream.read(&hasSolid);
	stream.read(&hasTranslucency);
	mathRead(stream, &bounds);
   mathRead(stream, &transform);
   mathRead(stream, &scale);

   return true;
}

bool InteriorSimpleMesh::write(Stream& stream) const
{
   // Simple serialization
   // Primitives
   stream.write(primitives.size());
   for (U32 i = 0; i < primitives.size(); i++)
   {
      stream.write(primitives[i].alpha);
		stream.write(primitives[i].texS);
		stream.write(primitives[i].texT);
		stream.write(primitives[i].diffuseIndex);
		stream.write(primitives[i].lightMapIndex);
		stream.write(primitives[i].start);
		stream.write(primitives[i].count);

      mathWrite(stream, primitives[i].lightMapEquationX);
      mathWrite(stream, primitives[i].lightMapEquationY);
      mathWrite(stream, primitives[i].lightMapOffset);
      mathWrite(stream, primitives[i].lightMapSize);
   }

   // Indices
   stream.write(indices.size());
   for (U32 i = 0; i < indices.size(); i++)
      stream.write(indices[i]);

   // Vertices
   stream.write(verts.size());
   for (U32 i = 0; i < verts.size(); i++)
      mathWrite(stream, verts[i]);

   // Normals
   stream.write(norms.size());
   for (U32 i = 0; i < norms.size(); i++)
      mathWrite(stream, norms[i]);

   // Diffuse UVs
   stream.write(diffuseUVs.size());
   for (U32 i = 0; i < diffuseUVs.size(); i++)
      mathWrite(stream, diffuseUVs[i]);

   // Lightmap UVs
   stream.write(lightmapUVs.size());
   for (U32 i = 0; i < lightmapUVs.size(); i++)
      mathWrite(stream, lightmapUVs[i]);

   // Material list
   if (materialList)
   {
      stream.write(true);
      materialList->write(stream);
   }
   else
      stream.write(false);

   // Diffuse bitmaps
   if (!materialList)
      stream.write(0);
   else
   {
      stream.write(materialList->getMaterialCount());

      for (U32 i = 0; i < materialList->getMaterialCount(); i++)
      {
         TextureHandle& handle = materialList->getMaterial(i);

         GBitmap* bitMap = handle.getBitmap();

         if (bitMap)
         {
            stream.write(true);
            bitMap->writePNG(stream);
         }
         else
            stream.write(false);
      }
   }

   // Misc data
   stream.write(hasSolid);
	stream.write(hasTranslucency);
	mathWrite(stream, bounds);
   mathWrite(stream, transform);
   mathWrite(stream, scale);

   return true;
}