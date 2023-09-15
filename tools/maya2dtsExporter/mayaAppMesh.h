//-----------------------------/------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef DTSMAYAMESH_H_
#define DTSMAYAMESH_H_

#include "appMesh.h"
#include <maya/MDagPath.h>

namespace DTS
{

   class MayaAppMesh : public AppMesh
   {
      typedef AppMesh Parent;

      MDagPath mPath;
      MDagPath mNodePath;
      char * mName;
      std::vector<Material> mMaterials;

      void getSkinData();
      void getMaterials();
      MObject findShader( MObject& setNode );

    public:

      MayaAppMesh(MDagPath & nodePath, MDagPath & path);
      ~MayaAppMesh();

      const char * getName();

      Matrix<4,4,F32> getMeshTransform(const AppTime & time);
      F32 getVisValue(const AppTime & time);

      bool getFloat(const char * propName, F32  & defaultVal);
      bool   getInt(const char * propName, S32  & defaultVal);
      bool  getBool(const char * propName, bool & defaultVal);

      bool getMaterial(S32 matIdx, Material &);

      bool animatesVis(const AppSequenceData & seqData);

      AppMeshLock lockMesh(const AppTime & time, const Matrix<4,4,F32> & objectOffset);
      void unlockMesh();
   };

   extern void MayaSetTime(const AppTime & time);
   extern Matrix<4,4,F32> MayaToDtsTransform(MMatrix & mayaMat);
   extern bool CheckMayaStatus(MStatus & status);
   extern void HandleMayaNegative(char * name);

}; // namespace DTS


#endif // DTSAPPMESH_H_
