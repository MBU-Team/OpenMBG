//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _INC_MAPEXPORTER
#define _INC_MAPEXPORTER

// MAX files
#pragma pack(push,4)
#include <max.h>
#include <stdmat.h>
#include <bmmlib.h>
#include <bitmap.h>
#include <iparamb2.h>
#pragma pack(pop)

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _TVECTOR_H_
#include "core/tVector.h"
#endif
#ifndef _BITVECTOR_H_
#include "core/bitVector.h"
#endif
#ifndef _COLOR_H_
#include "core/color.h"
#endif
#ifndef _FILESTREAM_H_
#include "core/fileStream.h"
#endif

#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif

extern const S32 gcProgramVersion;

// Forward declarations
class MaterialList;
class Interior;

//
class MapProperties
{
   public:
      MapProperties();

      const char *   mWadName;
      F32            mGeometryScale;
      F32            mLightingScale;
      ColorI         mAmbientColor;
      ColorI         mEmergencyAmbientColor;
};

//------------------------------------------------------------------------------
// Object::ConvertToType can create a new object which we are responsible for deleting
class TriObjRef 
{
   private:
      TriObject *    mRef;
      bool           mOwnObject;
      
      TriObject & operator =(const TriObjRef& rhs);   // disable

   public:
      TriObjRef(TriObject * obj, bool ownObject);
      TriObjRef(TriObjRef & rhs);
      ~TriObjRef();
      
      // --- inlines
      operator TriObject *(void){ return(mRef); }
      TriObject & operator*(void){ return(*mRef); }
      TriObject * operator->(void){ return(mRef); }
};

//------------------------------------------------------------------------------

static const U32 MaxWindingPoints       = 32;

struct Winding
{
   U32   numIndices;
   U32   indices[MaxWindingPoints];
};

// this is a WorldCraft 3.3 compatable surface description
struct Surface
{
   S32      planeIndex;
   Winding  winding;
   PlaneF   texGenX;
   PlaneF   texGenY;
   U16      materialIndex;
};

//------------------------------------------------------------------------------
class ObjectBase
{
   protected:
      TriObjRef         mTriObjRef;
      INode *           mNode;

   public:
      ObjectBase(TriObjRef objRef, INode * node);
};

class SceneObject : public ObjectBase
{
   public:
      SceneObject(TriObjRef objRef, INode * node);
      bool process();
};

//--------------------------------------------------------------------------
class ConvexHullObject : public ObjectBase
{
   public:
      Vector<PlaneF>    mPlanes;
      Vector<Point3F>   mPoints;
      bool              mIsConvex;
      
      ConvexHullObject(TriObjRef objRef, INode * node);
      void process();
};

//
class CollisionDetail
{
   public:
      Vector<ConvexHullObject*>     mConvexHulls;

      ~CollisionDetail();

      void addConvexHullObject(ConvexHullObject * obj) { mConvexHulls.push_back(obj); }
      void processConvexHullObjects();
      void export(FileStream & file);
};

//
class EditGeometry
{
   public:

      EditGeometry();
      ~EditGeometry();

      Vector<SceneObject*> mSceneObjects;
      Vector<Surface>      mSurfaces;
      U32                  mMinPixels;

   private:

      Vector<const char *> mMaterialNames;
      Vector<Point2I>      mMaterialSizes;
      Vector<PlaneF>       mPlanes;
      Vector<Point3F>      mPoints;

   public:

      U32 numPlanes(){return(mPlanes.size());}
      U32 insertPlane(const PlaneF & plane);
      const PlaneF & getPlane(U32 index){return(mPlanes[index]);}

      U32 numPoints(){return(mPoints.size());}
      U32 insertPoint(const Point3F & pnt);
      const Point3F & getPoint(U32 index){return(mPoints[index]);}

      U32 numMaterials(){return(mMaterialNames.size());}
      U32 insertMaterial(Mtl * material);
      const char * getMaterial(U32 index){return(mMaterialNames[index]);}

      void addSceneObject(SceneObject * obj){mSceneObjects.push_back(obj);}
      void processSceneObjects();
      Point3F getSurfaceCenter(Surface & surface);

      const char * getWadName();
      void exportToMap(const char * fileName, U32 detailNum);
};

extern EditGeometry * gWorkingGeometry;

// ------------------------------------------------------------------
class DetailEnumerator : public ITreeEnumProc
{
   private:
      Vector<EditGeometry *>     mDetails;
      CollisionDetail            mCollisionDetail;

   public:
   
      DetailEnumerator();
      ~DetailEnumerator();

      // ITreeEnumProc
      int callback(INode * node);

      void enumScene(IScene * scene);
      EditGeometry * createDetail(U32 minPixels);
      EditGeometry * getNodeDetail(INode * node);

      bool isCollisionHullObject(INode * node);

      void sortDetails();
      bool sequentialDetails();

      U32 numDetails()                       { return(mDetails.size()); }
      EditGeometry * getDetail(U32 index)    { return(mDetails[index]); }
      CollisionDetail * getCollisionDetail() { return(&mCollisionDetail); }
};

//------------------------------------------------------------------------------
// main plugin class
class Exporter : public SceneExport
{
   public:

      DetailEnumerator     mDetailEnum;
      MapProperties        mMapProperties;

      bool getMapProperties();
      const char * generateMapName(const char * baseName, U32 detail, U32 numDetails);

      // SceneExport
      const TCHAR * Ext(int index);
      int DoExport(const TCHAR * name, ExpInterface * expIFace, Interface * iFace, BOOL suppressPrompts=FALSE, DWORD options=0);

      // -- inlines
      int ExtCount(){return(1);}
      const TCHAR * LongDesc(){return(_T("WorldCraft 3.3 Map Exporter"));}

#if defined(TORQUE_DEBUG)
      const TCHAR * ShortDesc(){return(_T("WorldCraft 3.3 Map File(DBG)"));}
#else
      const TCHAR * ShortDesc(){return(_T("WorldCraft 3.3 Map File"));}
#endif
      const TCHAR * AuthorName(){return(_T("JohnF"));}
      const TCHAR * CopyrightMessage(){return(_T("Copyright GarageGames.com, 2001"));}
      const TCHAR * OtherMessage1(){return(_T(""));}
      const TCHAR * OtherMessage2(){return(_T(""));}
      unsigned int Version(){return(gcProgramVersion);}
      void ShowAbout(HWND hWnd){};
};

//------------------------------------------------------------------------------
// This class gives MAX a description of our plugin
class PluginDescription : public ClassDesc
{
   public:

      // ClassDesc
      int IsPublic(){return(true);}
      void * Create(BOOL loading = false){return(new Exporter);}
      const TCHAR * ClassName(){return _T("Torque WorldCraft 3.3 Map Exporter");}
      SClass_ID SuperClassID(){return(SCENE_EXPORT_CLASS_ID);}
      Class_ID ClassID(){return(Class_ID(0x4a23445d, 0x577b4b6b));}
      const TCHAR * Category() {return _T("");}
      void DeleteThis(){}
};

#endif
