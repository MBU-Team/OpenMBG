//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "max2mapExporter/exporter.h"
#include "platformWin32/winConsole.h"
#include "dgl/materialList.h"
#include "dgl/gBitmap.h"

// static data
static HINSTANCE           gInstance;
static PluginDescription   gPluginDesc;
static Interface *         gInterface;   
static EditGeometry *      gWorkingGeometry;
static CollisionDetail *   gCollisionDetail;

static MapProperties       gMapProperties;

static const S32           gcProgramVersion = 69;

static const F32           gcCenterOffsetScale = 0.1f;
static const F32           gcMinCenterOffset = 2.f;
static const F32           gcMaxCenterOffset = 200.f;

static const F32           gcPlaneNormThresh = 0.999f;
static const F32           gcPlaneDistThresh = 0.001f;   

// ------------------------------------------------------------------
namespace // helper functions
{
   //
   Point3F toPoint3F(const Point3 & pnt){return(Point3F(pnt.x, pnt.y, pnt.z));}

   // returns (a mod b)
   F32 floatMod(F32 a, F32 b)
   {
      F32 multiple = mFabs(a / b);
      F32 remainder = multiple - mFloor(multiple);
      return(a < 0 ? -(remainder * b) : (remainder * b));
   } 

   //
   void writeString(FileStream & file, const char * str)
   {
      file.write(dStrlen(str), str);
   }

   //------------------------------------------------------------------------------
   // from Numerical Recipes in C
   #define idx(i,j) (i*4 + j)
   void gaussjInverse(MatrixF & mat)
   {
      U32 indxc[4];
      U32 indxr[4];
      U32 ipiv[4];

      F32 * a = (F32*)mat;

      S32 i, icol, irow, j, k, l, ll;
      F32 big, dum, pivinv, temp;

      for(j = 0; j < 4; j++)  ipiv[j] = 0;
      for(i = 0; i < 4; i++)
      {
         big = 0.f;
         for(j = 0; j < 4; j++)
            if(ipiv[j] != 1)
               for(k = 0; k < 4; k++)
               {
                  if(ipiv[k] == 0)
                  {
                     if(mFabs(a[idx(j,k)]) >= big)
                     {
                        big = mFabs(a[idx(j,k)]);
                        irow = j;
                        icol = k;
                     }
                  } 
                  else if(ipiv[k] > 1)
                     AssertFatal(0, "Doh!");
               }
         ++(ipiv[icol]);

         if(irow != icol)
         {
            for(l = 0; l < 4; l++)
            {
               F32 tmp = a[idx(irow, l)];
               a[idx(irow, l)] = a[idx(icol, l)];
               a[idx(icol, l)] = tmp;
            }
         }

         indxr[i] = irow;
         indxc[i] = icol;
         if(a[idx(icol, icol)] == 0.0)
            AssertFatal(0, "Doh!");

         pivinv = 1.0 / a[idx(icol, icol)];
         a[idx(icol, icol)] = 1.f;

         for(l = 0; l < 4; l++) a[idx(icol, l)] *= pivinv;

         for(ll = 0; ll < 4; ll++)
            if(ll != icol)
            {
               dum = a[idx(ll, icol)];
               a[idx(ll, icol)] = 0.f;
               for(l = 0; l < 4; l++) a[idx(ll, l)] -= a[idx(icol, l)] * dum;
            }
      }

      for(l = 3; l >= 0; l--)
      {
         if(indxr[l] != indxc[l])
            for(k = 0; k < 4; k++)
            {
               F32 tmp = a[idx(k, indxr[l])];
               a[idx(k, indxr[l])] = a[idx(k, indxc[l])];
               a[idx(k, indxc[l])] = tmp;
            }
      }
   }

   // size of the material
   Point2I getMaterialSize(Mtl * material)
   {
      Texmap * texMap = material->GetSubTexmap(ID_DI);
      Bitmap * bitmap = ((BitmapTex*)texMap)->GetBitmap(0);
   
      Point2I size(bitmap->Width(), bitmap->Height());
      return(size);
   }

   // get the base name of the material
   const char * getMaterialName(Mtl * material)
   {
      Texmap * texMap = material->GetSubTexmap(ID_DI);
   
      char buf[1024];
      dSprintf(buf, sizeof(buf), ((BitmapTex*)texMap)->GetMapName());

      // just get the base name...
      char * cur = dStrrchr(buf, '.');
      if(!cur)
         return(0);

      *cur = '\0';
      cur--;

      while(1)
      {
         // end of filebase?
         if(*cur == '\\' || *cur == '/')
         {
            cur++;
            break;
         }

         // bad char?
         if(*cur == ' ')
            return(0);

         // only base?
         if(cur == buf)
            break;

         cur--;
      }
   
      // good length?   
      U32 len = dStrlen(cur);
      if(!len || len > 15)
         return(0);

      return(StringTable->insert(cur));
   }

   //
   bool isValidMaterial(StdMat * material)
   {
      // grab the diffuse map
      Texmap * texMap = material->GetSubTexmap(ID_DI);

      // valid name?
      if(!getMaterialName(material))
         return(false);

      // make sure a bitmap
      if(texMap && texMap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0))
      {
         Bitmap * bitmap = ((BitmapTex*)texMap)->GetBitmap(0);

         // make sure has a name
         if(!((BitmapTex*)texMap)->GetMapName())
            return(false);
               
         // make sure a valid bitmap width/height
         if(bitmap->Width() < 0x10 || bitmap->Height() < 0x10)
            return(false);

         if(bitmap->Width() & 0x07 || bitmap->Height() & 0x07)
            return(false);
      
         return(true);
      }

      return(false);
   }
};

// ------------------------------------------------------------------
// MapProperties
// ------------------------------------------------------------------
MapProperties::MapProperties()
{
   mWadName = 0;
   mGeometryScale = 64.f;
   mLightingScale = 64.f;
   mAmbientColor.set(0,0,0);
   mEmergencyAmbientColor.set(0,0,0);
}

// ------------------------------------------------------------------
// DetailEnumerator
// ------------------------------------------------------------------
DetailEnumerator::DetailEnumerator()
{
}

DetailEnumerator::~DetailEnumerator()
{
   for(U32 i = 0; i < mDetails.size(); i++)
      delete mDetails[i];
}

// ------------------------------------------------------------------
// enumerates the scene creating details and assigning SceneObjects to them
void DetailEnumerator::enumScene(IScene * scene)
{
   scene->EnumTree(this);   
}

// ------------------------------------------------------------------
int DetailEnumerator::callback(INode * node)
{
   const char * name = node->GetName();

   if(node->IsHidden())                     
      return(TREE_CONTINUE);
   
   Object * obj = node->EvalWorldState(gInterface->GetTime()).obj;

   if(!obj->IsRenderable())
      return(TREE_CONTINUE);

   if(!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID,0)))
      return(TREE_CONTINUE);

   // get the detail this belongs in
   EditGeometry * detail = getNodeDetail(node);
   if(!detail)
   {
      // collision hull?
      if(!isCollisionHullObject(node))
         return(TREE_CONTINUE);
   }

   TriObject * triObj = (TriObject*)obj->ConvertToType(gInterface->GetTime(), 
      Class_ID(TRIOBJ_CLASS_ID, 0));

   // ownership of the TriObject will be transfered
   if(detail)
      detail->addSceneObject(new SceneObject(TriObjRef(triObj, triObj != obj), node));
   else
      mCollisionDetail.addConvexHullObject(new ConvexHullObject(TriObjRef(triObj, triObj != obj), node));

   return(TREE_CONTINUE);
}

// ------------------------------------------------------------------
EditGeometry * DetailEnumerator::getNodeDetail(INode * node)
{
   // search for a detail node
   while(!node->IsRootNode())
   {
      char buf[80];
      dSprintf(buf, sizeof(buf), "%s", node->GetName());
      if(!dStrnicmp(buf, "detail_", 7))
         return(createDetail(dAtoi(buf+7)));

      node = node->GetParentNode();
   }

   return(0);
}

bool DetailEnumerator::isCollisionHullObject(INode * node)
{
   while(!node->IsRootNode())
   {
      if(!dStrnicmp(node->GetName() ? node->GetName() : "", "collision", 9))
         return(true);
      node = node->GetParentNode();
   }
   return(false);
}

// ------------------------------------------------------------------
EditGeometry * DetailEnumerator::createDetail(U32 minPixels)
{
   // search for it
   for(U32 i = 0; i < mDetails.size(); i++)
      if(mDetails[i]->mMinPixels == minPixels)
         return(mDetails[i]);
   
   // create a new one
   EditGeometry * detail = new EditGeometry;
   detail->mMinPixels = minPixels;

   mDetails.push_back(detail);

   return(detail);
}

// ------------------------------------------------------------------
static S32 QSORT_CALLBACK detailCompare(const void * a, const void * b)
{
   return(((S32)(*(EditGeometry**)b)->mMinPixels) -
          ((S32)(*(EditGeometry**)a)->mMinPixels));
}

void DetailEnumerator::sortDetails()
{
   dQsort((void*)&mDetails[0], mDetails.size(), sizeof(EditGeometry*), detailCompare);
}

bool DetailEnumerator::sequentialDetails()
{
   for(U32 i = 0; i < (mDetails.size() - 1); i++)
      if(mDetails[i]->mMinPixels <= mDetails[i+1]->mMinPixels)
         return(false);
   return(true);
}

// ------------------------------------------------------------------
// Class TriObjRef
// ------------------------------------------------------------------
TriObjRef::TriObjRef(TriObject * obj, bool ownObject) :
   mRef(obj),
   mOwnObject(ownObject)
{
}

TriObjRef::TriObjRef(TriObjRef & rhs)
{
   mOwnObject = rhs.mOwnObject;
   mRef = rhs.mRef;
   rhs.mOwnObject = false;
}

TriObjRef::~TriObjRef()
{
   if(mOwnObject)
      delete mRef;
}

// ------------------------------------------------------------------
// Class ObjectBase
// ------------------------------------------------------------------
ObjectBase::ObjectBase(TriObjRef objRef, INode * node) :
   mTriObjRef(objRef),
   mNode(node)
{
}

// ------------------------------------------------------------------
// Class SceneObject
// ------------------------------------------------------------------
SceneObject::SceneObject(TriObjRef objRef, INode * node) :
   ObjectBase(objRef, node)
{
}

// ------------------------------------------------------------------
bool SceneObject::process()
{
   // do materials first
   Mtl * material = mNode->GetMtl();
   if(!material)
      return(false);

   Vector<StdMat *> materials;
   Vector<U32> materialIDs;

   // single material
   if(material->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
   {
      if(!isValidMaterial((StdMat*)material))
         return(false);

      materials.push_back((StdMat*)material);
      materialIDs.push_back(gWorkingGeometry->insertMaterial(material));
   }

   // multiple materials
   if(material->ClassID() == Class_ID(MULTI_CLASS_ID, 0))
   {
      U32 numMat = material->NumSubMtls();
      for(U32 i = 0; i < numMat; i++)
      {
         Mtl * subMat = material->GetSubMtl(i);

         // check it
         if(subMat->ClassID() != Class_ID(DMTL_CLASS_ID, 0))
            return(false);

         if(!isValidMaterial((StdMat*)subMat))
            return(false);

         materials.push_back((StdMat*)subMat);
         materialIDs.push_back(gWorkingGeometry->insertMaterial(subMat));
      }
   }

   if(!materials.size())
      return(false);
   
   // get the mesh and walk the faces
   Mesh & mesh = mTriObjRef->GetMesh();
   Matrix3 nodeTM = mNode->GetNodeTM(gInterface->GetTime());

   Point3 offset = mNode->GetObjOffsetPos();
   
   // create all the surfaces from the mesh
   for(U32 i = 0; i < mesh.numFaces; i++)
   {
      TVFace & texFace = mesh.tvFace[i];
      Face & face = mesh.faces[i];

      U32 matID = face.getMatID() % materials.size();
   
      // create the surface and add to the current detail   
      Surface surface;
      dMemset(&surface, 0, sizeof(Surface));
      surface.materialIndex = materialIDs[matID];
   
      // 'Face' is a triangular surface...
      surface.winding.numIndices = 3;

      Vector<Point3F> textureCoords;

      // insert the vertices
      for(U32 j = 0; j < surface.winding.numIndices; j++)
      {
         Point3 maxPnt = nodeTM.PointTransform(mesh.getVert(face.v[j]));
         maxPnt += offset;
         
         // scale it
         maxPnt *= gMapProperties.mGeometryScale;

         surface.winding.indices[j] = gWorkingGeometry->insertPoint(toPoint3F(maxPnt));

         // get the texture coordinate
         const UVVert & texVert = mesh.getTVert(texFace.t[j]);
         textureCoords.push_back(toPoint3F(texVert));
         textureCoords[j].z = 0;
      }

      // insert the plane - normal to empty
      PlaneF plane(gWorkingGeometry->getPoint(surface.winding.indices[2]),
                   gWorkingGeometry->getPoint(surface.winding.indices[1]),
                   gWorkingGeometry->getPoint(surface.winding.indices[0]));
      surface.planeIndex = gWorkingGeometry->insertPlane(plane);
      
      // now do the texture stuff...
      //---------------------------------------

      // get some info
      BitmapTex * bitmapTex = (BitmapTex*)materials[matID]->GetSubTexmap(ID_DI);
      Bitmap * bitmap = bitmapTex->GetBitmap(gInterface->GetTime());

      // grab the points for easy access
      Vector<Point3F> mappedCoords;
      mappedCoords.setSize(surface.winding.numIndices);

      for(U32 l = 0; l < surface.winding.numIndices; l++)
      {
         const Point3F & pnt = gWorkingGeometry->getPoint(surface.winding.indices[l]);
         mappedCoords[l] = pnt;   
      }

      MatrixF mat;

      Point4F sb, tb;
      sb.w = tb.w = 0;

      for(U32 row = 0; row < 3; row++)
      {
         mat.setRow(row, mappedCoords[row]);
         ((F32*)sb)[row] = textureCoords[row].x;
         ((F32*)tb)[row] = textureCoords[row].y;
      }

      Point3F norm;
      Point3F na = mappedCoords[1] - mappedCoords[0];
      Point3F nb = mappedCoords[2] - mappedCoords[1];
      na.normalize();
      nb.normalize();
      mCross(na, nb, &norm);

      mat.setRow(3, norm);
      mat.setColumn(3, Point4F(1,1,1,0));

      // inverse
      gaussjInverse(mat);

      mat.mul(sb);
      mat.mul(tb);

      // set the gen's
      surface.texGenX.x = sb.x;
      surface.texGenX.y = sb.y;
      surface.texGenX.z = sb.z;
      surface.texGenX.d = sb.w;

      surface.texGenY.x = -tb.x;
      surface.texGenY.y = -tb.y;
      surface.texGenY.z = -tb.z;
      surface.texGenY.d = -tb.w;

      gWorkingGeometry->mSurfaces.push_back(surface);
   }

   return(true);
}

// ------------------------------------------------------------------
// Class ConvexHullObject
// ------------------------------------------------------------------
ConvexHullObject::ConvexHullObject(TriObjRef objRef, INode * node) :
   ObjectBase(objRef, node)
{
   mIsConvex = false;
}

void ConvexHullObject::process()
{
   AssertFatal(!mPlanes.size(), "ConvexHullObject::process: already processed");

   // grab all the planes
   Mesh & mesh = mTriObjRef->GetMesh();

   Matrix3 nodeTM = mNode->GetNodeTM(gInterface->GetTime());

   Point3 offset = mNode->GetObjOffsetPos();
   
   // create all the surfaces from the mesh
   for(U32 i = 0; i < mesh.numFaces; i++)
   {
      Face & face = mesh.faces[i];
      
      Point3F pnts[3];
      
      for(U32 j = 0; j < 3; j++)
      {
         Point3 maxPnt = nodeTM.PointTransform(mesh.getVert(face.v[j]));
         maxPnt += offset;
         maxPnt *= gMapProperties.mGeometryScale;
         pnts[j] = toPoint3F(maxPnt);
      }   

      PlaneF plane(pnts[0], pnts[1], pnts[2]);
      
      // dont insert duplicate planes...   
      bool found = false;
      for(S32 k = mPlanes.size() - 1; (k >= 0) && !found; k--)
      {
         if((mDot(plane, mPlanes[k]) > gcPlaneNormThresh) && 
            (mFabs(plane.d - mPlanes[k].d) < gcPlaneDistThresh))
            found = true;
      }
      
      if(!found)
      {
         mPlanes.push_back(plane);
         mPoints.push_back(pnts[0]);
         mPoints.push_back(pnts[1]);
         mPoints.push_back(pnts[2]);
      }
   }

   // converting into a MNMesh does not correctly solve convex problem.. so we
   // are to assume that this is convex
   if(mPlanes.size() >= 4)
      mIsConvex = true;
}

// ------------------------------------------------------------------
// Class CollisionDetail
// ------------------------------------------------------------------
CollisionDetail::~CollisionDetail()
{
   for(S32 i = mConvexHulls.size() - 1; i >= 0; i--)
      delete mConvexHulls[i];
}

void CollisionDetail::processConvexHullObjects()
{
   for(S32 i = mConvexHulls.size() - 1; i >= 0; i--)
      mConvexHulls[i]->process();
}

void CollisionDetail::export(FileStream & file)
{
   if(!mConvexHulls.size())
      return;

   writeString(file, "{\n");
   writeString(file, "\"classname\" \"collision\"\n");

   char buf[1024];

   for(U32 i = 0; i < mConvexHulls.size(); i++)
   {
      ConvexHullObject * hull = mConvexHulls[i];
      
      if(!hull->mIsConvex)
         continue;

      writeString(file, "{\n");         

      for(U32 j = 0; j < hull->mPlanes.size(); j++)
      {
         U32 k = (j * 3);
         dSprintf(buf, sizeof(buf), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) NULL [ 1 0 0 0 ] [ 0 1 0 0 ] 0 1 1\n",
            (S32)hull->mPoints[k+2].x, (S32)hull->mPoints[k+2].y, (S32)hull->mPoints[k+2].z,
            (S32)hull->mPoints[k+1].x, (S32)hull->mPoints[k+1].y, (S32)hull->mPoints[k+1].z,
            (S32)hull->mPoints[k].x,   (S32)hull->mPoints[k].y,   (S32)hull->mPoints[k].z);
         writeString(file, buf);
      }

      writeString(file, "}\n");         
   }

   writeString(file, "}\n");
}

// ------------------------------------------------------------------
// DLL Entry point
// ------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hInst, ULONG dwReason, LPVOID pReserved)
{
   static bool initialized = false;
   if(!initialized)
   {
      initialized = true;
      _StringTable::create();
   }
   
   gInstance = hInst;
   return(TRUE);
}

DLLEXPORT int LibNumberClasses()
{
   return(1);
}

DLLEXPORT ClassDesc * LibClassDesc(int index)
{
   if(index != 0)
      return(0);
   return(&gPluginDesc);
}

DLLEXPORT const TCHAR * LibDescription()
{
   return(_T("WorldCraft 3.3 Map Exporter"));
}

DLLEXPORT ULONG LibVersion()
{
   return(VERSION_3DSMAX);
}

//------------------------------------------------------------------------------
// Class Detail
//------------------------------------------------------------------------------
EditGeometry::EditGeometry()
{
}

EditGeometry::~EditGeometry()
{
   // remove the scene info
   for(U32 i = 0; i < mSceneObjects.size(); i++)
      delete mSceneObjects[i];
}

void EditGeometry::processSceneObjects()
{
   // have each object create it's surfaces...
   for(U32 i = 0; i < mSceneObjects.size(); i++)
      mSceneObjects[i]->process();
}

//------------------------------------------------------------------------------
U32 EditGeometry::insertMaterial(Mtl * material)
{
   const char * matName = getMaterialName(material);
   for(U32 i = 0; i < mMaterialNames.size(); i++)
      if(!dStricmp(matName, mMaterialNames[i]))
         return(i);

   // insert the name and size
   mMaterialNames.push_back(matName);
   mMaterialSizes.push_back(getMaterialSize(material));
   
   return(mMaterialNames.size() - 1);
}

U32 EditGeometry::insertPoint(const Point3F & pnt)
{
   for(U32 i = 0; i < mPoints.size(); i++)
      if(mPoints[i] == pnt)
         return(i);
   
   mPoints.push_back(pnt);
   return(mPoints.size() - 1);
}

U32 EditGeometry::insertPlane(const PlaneF & plane)
{
   for(U32 i = 0; i < mPlanes.size(); i++)
      if(mPlanes[i] == plane)
         return(i);

   mPlanes.push_back(plane);

   return(mPlanes.size() - 1);
}

//------------------------------------------------------------------------------
Point3F EditGeometry::getSurfaceCenter(Surface & surface)
{
   AssertFatal(surface.winding.numIndices, "EditGeometry::getSurfaceCenter: invalid surface");

   Point3F center(0,0,0);
   for(U32 i = 0; i < surface.winding.numIndices; i++)
      center += getPoint(surface.winding.indices[i]);

   center /= surface.winding.numIndices;
   return(center);
}

//------------------------------------------------------------------------------
void EditGeometry::exportToMap(const char * fileName, U32 detailNum)
{
   FileStream file;
   file.open(fileName, FileStream::Write);

   char buf[1024];
   writeString(file, "{\n");
   writeString(file, "\"classname\" \"worldspawn\"\n");
   writeString(file, "\"mapversion\" \"220\"\n");

   // wadname
   dSprintf(buf, sizeof(buf), "\"wad\" \"%s\"\n", gMapProperties.mWadName);
   writeString(file, buf);

   // geometry scale
   dSprintf(buf, sizeof(buf), "\"geometry_scale\" \"%f\"\n", gMapProperties.mGeometryScale);
   writeString(file, buf);

   // lighting scale
   dSprintf(buf, sizeof(buf), "\"light_geometry_scale\" \"%f\"\n", gMapProperties.mLightingScale);
   writeString(file, buf);

   // ambient color
   dSprintf(buf, sizeof(buf), "\"ambient_color\", \"%d %d %d\"\n", 
      gMapProperties.mAmbientColor.red, gMapProperties.mAmbientColor.green, gMapProperties.mAmbientColor.blue);
   writeString(file, buf);

   // emergency lighting color
   dSprintf(buf, sizeof(buf), "\"emergency_ambient_color\", \"%d %d %d\"\n",
      gMapProperties.mEmergencyAmbientColor.red, gMapProperties.mEmergencyAmbientColor.green, gMapProperties.mEmergencyAmbientColor.blue);
   writeString(file, buf);

   // minpixels
   dSprintf(buf, sizeof(buf), "\"min_pixels\" \"%d\"\n", mMinPixels);
   writeString(file, buf);

   // detailNum
   dSprintf(buf, sizeof(buf), "\"detail_number\" \"%d\"\n", detailNum);
   writeString(file, buf);

   // create a brush per face..
   for(U32 i = 0; i < mSurfaces.size(); i++)
   {
      Surface & surface = mSurfaces[i];

      writeString(file, "{\n");

      char buf[1024];
      
      // surface plane..
      Point3F pnts[3];
      for(U32 j = 0; j < 3; j++)
         pnts[j] = getPoint(surface.winding.indices[j]);

      // unscale texgens
      surface.texGenX.x *= mMaterialSizes[surface.materialIndex].x;
      surface.texGenX.y *= mMaterialSizes[surface.materialIndex].x;
      surface.texGenX.z *= mMaterialSizes[surface.materialIndex].x;
      surface.texGenX.d *= mMaterialSizes[surface.materialIndex].x;

      surface.texGenY.x *= mMaterialSizes[surface.materialIndex].y;
      surface.texGenY.y *= mMaterialSizes[surface.materialIndex].y;
      surface.texGenY.z *= mMaterialSizes[surface.materialIndex].y;
      surface.texGenY.d *= mMaterialSizes[surface.materialIndex].y;

      // write out this plane
      dSprintf(buf, sizeof(buf), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s [ %f %f %f %f ] [ %f %f %f %f ] 0 1 1\n",
         (S32)pnts[2].x, (S32)pnts[2].y, (S32)pnts[2].z,
         (S32)pnts[1].x, (S32)pnts[1].y, (S32)pnts[1].z,
         (S32)pnts[0].x, (S32)pnts[0].y, (S32)pnts[0].z,
         getMaterial(surface.materialIndex),
         surface.texGenX.x, surface.texGenX.y, surface.texGenX.z, surface.texGenX.d,
         surface.texGenY.x, surface.texGenY.y, surface.texGenY.z, surface.texGenY.d);
      writeString(file, buf);

      PlaneF plane = getPlane(surface.planeIndex);
      
      // get the surface area
      Point3F areaNorm(0.f,0.f,0.f);
      Point3F tmp;

      mCross(pnts[0], pnts[1], &tmp);  areaNorm += tmp;
      mCross(pnts[1], pnts[2], &tmp);  areaNorm += tmp;
      mCross(pnts[2], pnts[0], &tmp);  areaNorm += tmp;

      F32 len = mSqrt(mFabs(mDot(plane, areaNorm) * 0.5f));
      
      // null bounding planes
      Point3F center = getSurfaceCenter(surface);
      center -= (plane * mClampF(len * gcCenterOffsetScale, gcMinCenterOffset, gcMaxCenterOffset));

      // write out the null planes
      for(U32 k = 0; k < surface.winding.numIndices; k++)
      {
         U32 l = (k+1) % surface.winding.numIndices;

         pnts[0] = getPoint(surface.winding.indices[l]);
         pnts[1] = getPoint(surface.winding.indices[k]);
         pnts[2] = center;

         // bunk texture info (null surfaces)
         dSprintf(buf, sizeof(buf), "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) NULL [ 1 0 0 0 ] [ 0 1 0 0 ] 0 1 1\n",
            (S32)pnts[2].x, (S32)pnts[2].y, (S32)pnts[2].z,
            (S32)pnts[1].x, (S32)pnts[1].y, (S32)pnts[1].z,
            (S32)pnts[0].x, (S32)pnts[0].y, (S32)pnts[0].z);
         writeString(file, buf);
      }

      writeString(file, "}\n");
   }

   // wad file
   writeString(file, "}\n");

   // the collision info is dumped on highest detail
   if(!detailNum)
   {
      gCollisionDetail->processConvexHullObjects();
      gCollisionDetail->export(file);
   }
      
   file.close();
}

//------------------------------------------------------------------------------
// Class Exporter
//------------------------------------------------------------------------------
const TCHAR * Exporter::Ext(int index)
{
   if(index != 0)
      return(_T(""));
   return(_T("map"));
}

//------------------------------------------------------------------------------
const char * Exporter::generateMapName(const char * baseName, U32 detail, U32 numDetails)
{
   if(numDetails == 1)
      return(baseName);

   // work on a copy
   char buf[1024];
   dSprintf(buf, sizeof(buf), baseName);

   char * cur = dStrrchr(buf, '.');
   if(!cur)
      return(0);

   char ext[16];
   dSprintf(ext, sizeof(ext), "_%d.map", detail);
   *cur = '\0';

   // fixup the name
   dStrcat(buf, ext);
   return(StringTable->insert(buf));
}

bool Exporter::getMapProperties()
{
   bool gotLightingScale = false;

   //  walk and grab anything we like
   for(U32 i = 0; i < gInterface->GetNumProperties(PROPSET_USERDEFINED); i++)
   {
      const PROPSPEC * propSpec = gInterface->GetPropertySpec(PROPSET_USERDEFINED, i);
		const PROPVARIANT * propVar = gInterface->GetPropertyVariant(PROPSET_USERDEFINED, i);

      if(propVar->vt != VT_LPWSTR && propVar->vt != VT_LPSTR)
         continue;

      char key[1024];
      char value[1024];

      _tcscpy(key, TSTR(propSpec->lpwstr));
      _tcscpy(value, propVar->vt == VT_LPWSTR ? TSTR(propVar->pwszVal) : TSTR(propVar->pszVal));

      //
      if(!dStricmp(key, "wad"))
      {
         char buf[256];
         dSprintf(buf, sizeof(buf), "\\%s.wad", value);
         gMapProperties.mWadName = StringTable->insert(buf);
      }
      else if(!dStricmp(key, "geometry_scale"))
      {
         dSscanf(value, "%f", &gMapProperties.mGeometryScale);
      }
      else if(!dStricmp(key, "lighting_scale"))
      {
         gotLightingScale = true;
         dSscanf(value, "%f", &gMapProperties.mLightingScale);
      }
      else if(!dStricmp(key, "ambient_color"))
      {
         S32 r, g, b;
         dSscanf(value, "%d %d %d", &r, &g, &b);
         gMapProperties.mAmbientColor.red = U8(r);
         gMapProperties.mAmbientColor.green = U8(g);
         gMapProperties.mAmbientColor.blue = U8(b);
      }
      else if(!dStricmp(key, "emergency_ambient_color"))
      {
         S32 r, g, b;
         dSscanf(value, "%d %d %d", &r, &g, &b);
         gMapProperties.mEmergencyAmbientColor.red = U8(r);
         gMapProperties.mEmergencyAmbientColor.green = U8(g);
         gMapProperties.mEmergencyAmbientColor.blue = U8(b);
      }
   }

   // if there was no lighting scale, then set to the geometry scale
   if(!gotLightingScale)
      gMapProperties.mLightingScale = gMapProperties.mLightingScale;
      
   return(gMapProperties.mWadName ? true : false);
}

//------------------------------------------------------------------------------
int Exporter::DoExport(const TCHAR * name, ExpInterface * expIFace, Interface * iFace, BOOL suppressPrompts, DWORD options)
{
   suppressPrompts;options;
   
   // setup globals   
   gInterface = iFace;

   mDetailEnum.enumScene(expIFace->theScene);
   gCollisionDetail = mDetailEnum.getCollisionDetail();

   if(!getMapProperties())
      return(-1);

   // has detail levels?
   if(!mDetailEnum.numDetails())
   {
      MessageBox(NULL, "No details found to export", "Export failed!", MB_OK);
      return(-1);
   }

   // sort them
   mDetailEnum.sortDetails();
   if(!mDetailEnum.sequentialDetails())
   {
      MessageBox(NULL, "Details' minPixels are not sequential or are not unique!", "Export failed!", MB_OK);
      return(-1);
   }
      
   // work with each of the details
   U32 i;
   for(i = 0; i < mDetailEnum.numDetails(); i++)
   {
      gWorkingGeometry = mDetailEnum.getDetail(i);
      gWorkingGeometry->processSceneObjects();

      const char * mapName = generateMapName(name, i, mDetailEnum.numDetails());
      if(!mapName)
      {
         MessageBox(NULL, "Failed to generate mapname", "Export failed!", MB_OK);
         return(-1);
      }

      // multiple detail levels?
      gWorkingGeometry->exportToMap(mapName, i);
   }

   return(-1);
}

//------------------------------------------------------------------------------
// Resolve external's
//------------------------------------------------------------------------------
U32 GameAddTaggedString(const char *)  {return(0);}
void GameReactivate()                  {}
void GameDeactivate(bool)              {}
