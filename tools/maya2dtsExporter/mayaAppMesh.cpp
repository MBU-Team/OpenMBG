//-----------------------------/------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "mayaAppMesh.h"
#include "mayaAppNode.h"
#include "appConfig.h"
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MDistance.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MDGContext.h>
#include <maya/MAnimUtil.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnReflectShader.h>

namespace DTS
{

   bool getMeshAndSkin(MDagPath & path, MFnSkinCluster & skinCluster)
   {
      MStatus status;

      // The following is the "correct" way to get the
      // mesh and skinCluster in case the mesh is a skin,
      // according to (and adapted from):
      //    http://www.greggman.com/pages/mayastuff.htm
      // But we don't get the MeshFn from the input plug of the 
      // skinCluster as he does.  Two reasons for this:
      // 1) couldn't get it to work properly, not a valid MeshFn.
      // 2) we want the final mesh, not the intermediate product.

      // the deformed mesh comes into the visible mesh
      // through its "inmesh" plug
      MPlug inMeshPlug = MFnDagNode(path).findPlug("inMesh", &status);
      if (status != MS::kSuccess || !inMeshPlug.isConnected())
         return false; // no skin found

      // walk the tree of stuff upstream from this plug
      MItDependencyGraph dgIt(inMeshPlug,MFn::kInvalid,
                              MItDependencyGraph::kUpstream,
                              MItDependencyGraph::kDepthFirst,
                              MItDependencyGraph::kPlugLevel,
                              &status);

      if (MS::kSuccess == status)
      {
         S32 count = 0;
         dgIt.disablePruningOnFilter();

         for ( ; ! dgIt.isDone(); dgIt.next() )
         {
            MObject thisNode = dgIt.thisNode();

            // go until we find a skinCluster

            if (thisNode.apiType() == MFn::kSkinClusterFilter)
            {
               skinCluster.setObject(thisNode);
               return true; // found a skin
            }
         }
      }
      return false; // no skin found
   }

   MayaAppMesh::MayaAppMesh(MDagPath & nodePath, MDagPath & path)
   {
      mPath = path;
      mNodePath = nodePath;
      mName = NULL;
   }

   MayaAppMesh::~MayaAppMesh()
   {
   }

   void MayaAppMesh::getSkinData()
   {
      MFnSkinCluster skinCluster;
      if (!getMeshAndSkin(mPath,skinCluster))
         return; // no skin, no sweat

      MStatus status;
      S32 i;

      // get bones/influence array
		MDagPathArray bones;
		S32 numBones = skinCluster.influenceObjects(bones, &status);
      if (!CheckMayaStatus(status))
         return;

      S32 bone;
      for (bone=0; bone<numBones; bone++)
      {
         AppConfig::PrintDump(PDObjectStateDetails,avar("Adding bone %s\r\n",bones[bone].partialPathName().asChar()));
         mBones.push_back(new MayaAppNode(bones[bone]));
         mWeights.push_back(new std::vector<F32>);
      }

      MItGeometry gitr(mPath,&status);
      if (!CheckMayaStatus(status))
         return;

      AppConfig::PrintDump(PDObjectStateDetails,avar("SkinPath: %s, # verts: %i, # bones: %i",
         mPath.partialPathName().asChar(),gitr.count(),numBones));

      for (; !gitr.isDone(); gitr.next())
      {
         while ( numBones && mWeights[0]->size() <= gitr.index())
            for (i=0; i<numBones; i++)
               mWeights[i]->push_back(0.0f);

         MObject comp = gitr.component(&status);
         if (!CheckMayaStatus(status))
            return;

         MFloatArray wts;
         U32 weightCount;
         status = skinCluster.getWeights(mPath,comp,wts,weightCount);
         if (!CheckMayaStatus(status))
            return;
         if (weightCount != numBones)
         {
            AppConfig::SetExportError("Assertion failed on skinned mesh");
            return;
         }
         for (i=0; i<numBones; i++)
            (*mWeights[i])[gitr.index()] = wts[i];
      }

      // make sure we have weights for all bones and verts
      S32 numVerts = MFnMesh(mPath,&status).numVertices();
      if (!CheckMayaStatus(status))
         return;
      while ( numBones && mWeights[0]->size() < numVerts)
         for (i=0; i<numBones; i++)
            mWeights[i]->push_back(0.0f);
   }

   const char * MayaAppMesh::getName()
   {
      MStatus status;
      if (!mName)
      {
         MFnDagNode fnNode(mNodePath,&status);
         CheckMayaStatus(status);
         const char * newname = fnNode.name(&status).asChar();
         CheckMayaStatus(status);
         if (newname)
         {
            mName = strnew(newname);
            HandleMayaNegative(mName);
         }
      }
      CheckMayaStatus(status);
      return mName;
   }

   Matrix<4,4,F32> MayaAppMesh::getMeshTransform(const AppTime & time)
   {
      MStatus status;
      MayaSetTime(time);
      MMatrix mayaMat = mPath.inclusiveMatrix(&status);
      CheckMayaStatus(status);
      return MayaToDtsTransform(mayaMat);
   }

   F32 MayaAppMesh::getVisValue(const AppTime & time)
   {
      MStatus status;

      MFnDagNode fnNode(mPath,&status);

      MPlug plug = fnNode.findPlug(MString("visibility"),&status);
      if (MStatus::kSuccess != status)
         return 1.0f;

      F32 val;
      status = plug.getValue(val,MDGContext());
      if (status != MStatus::kSuccess)
         val = 1.0f;
      return val;
   }

   bool MayaAppMesh::getFloat(const char * propName, F32 & defaultVal)
   {
      MStatus status;

      MFnDagNode fnNode(mNodePath,&status);
      CheckMayaStatus(status);

      MPlug plug = fnNode.findPlug(MString(propName),&status);
      if (status!=MStatus::kSuccess)
         return false;

      F32 val;
      status = plug.getValue(val,MDGContext());
      if (status == MStatus::kSuccess)
         defaultVal = val;
      return status==MStatus::kSuccess;
   }

   bool MayaAppMesh::getInt(const char * propName, S32 & defaultVal)
   {
      MStatus status;

      MFnDagNode fnNode(mNodePath,&status);
      CheckMayaStatus(status);

      MPlug plug = fnNode.findPlug(MString(propName),&status);
      if (status!=MStatus::kSuccess)
         return false;

      S32 val;
      status = plug.getValue(val,MDGContext());
      if (status == MStatus::kSuccess)
         defaultVal = val;
      return status==MStatus::kSuccess;
   }

   bool MayaAppMesh::getBool(const char * propName, bool & defaultVal)
   {
      MStatus status;

      MFnDagNode fnNode(mNodePath,&status);
      CheckMayaStatus(status);

      MPlug plug = fnNode.findPlug(MString(propName),&status);
      if (status!=MStatus::kSuccess)
         return false;

      bool val;
      status = plug.getValue(val,MDGContext());
      if (status == MStatus::kSuccess)
         defaultVal = val;
      return status==MStatus::kSuccess;
   }

   bool MayaAppMesh::getMaterial(S32 matIdx, Material & mat)
   {
      if (matIdx>=0 && matIdx<mMaterials.size())
      {
         mat = mMaterials[matIdx];
         return true;
      }
      return false;
   }

   bool MayaAppMesh::animatesVis(const AppSequenceData & seqData)
   {
      MStatus status;

      MFnDagNode fnNode(mPath,&status);
      CheckMayaStatus(status);

      MPlug plug = fnNode.findPlug(MString("visibility"),&status);
      CheckMayaStatus(status);
      if (status!=MStatus::kSuccess)
         return false;

      if (MAnimUtil::isAnimated(plug,false,&status))
         return Parent::animatesVis(seqData);
      CheckMayaStatus(status);
      return false;
   }

   AppMeshLock MayaAppMesh::lockMesh(const AppTime & time, const Matrix<4,4,F32> & objectOffset)
   {
      MayaSetTime(time);

      MStatus status;
      MObject component = MObject::kNullObj; // not really sure what this is for...

      MFnMesh fnMesh(mPath,&status);
      if (!CheckMayaStatus(status))
         return Parent::lockMesh(time,objectOffset);

      MItMeshPolygon polyIter(mPath,component,&status);
      if (!CheckMayaStatus(status))
         return Parent::lockMesh(time,objectOffset);
      S32 polyVertexCount = polyIter.polygonVertexCount();
      if (polyVertexCount != 3)
      {
         AppConfig::SetExportError("Non-triangle mesh not supported");
         return Parent::lockMesh(time,objectOffset);
      }

      MItMeshVertex vtxIter(mPath,component,&status);
      if (!CheckMayaStatus(status))
         return Parent::lockMesh(time,objectOffset);

      // Write out the vertex table
      //

      std::vector<Point3D> verts;
      std::vector<Point2D> tverts;
      for ( ; !vtxIter.isDone(); vtxIter.next() )
      {
         MPoint p = vtxIter.position( MSpace::kObject );
         // convert from maya point to dts point...
         // ...also, convert from internal units 
         // to the current ui units
         Point3D pos;
         MDistance dist; // starts as internal units

         dist.setValue(p.x);
         pos[0] = dist.asMeters();
         dist.setValue(p.y);
         pos[1] = dist.asMeters();
         dist.setValue(p.z);
         pos[2] = dist.asMeters();
         verts.push_back(objectOffset * pos);
      }

      S32 i;
      MFloatArray uArray, vArray;
      fnMesh.getUVs( uArray, vArray );
      S32 uvLength = uArray.length();
      for (i=0; i<uvLength; i++ )
      {
         Point2D tvert(uArray[i],1.0f-vArray[i]);
         tverts.push_back(tvert);
      }

      for ( ; !polyIter.isDone(); polyIter.next() )
      {
         Primitive face;
         face.firstElement = mIndices.size();
         face.numElements = 3;
         face.type = Primitive::Triangles|Primitive::Indexed|Primitive::NoMaterial;

         for (S32 vtx=polyVertexCount-1; vtx>=0; vtx-- )
         {
            U16 vertId = polyIter.vertexIndex(vtx);
            Point3D vert = verts[vertId];

            // compute normal (TODO: handle non-uniform scale properly)
            MVector mayaNorm;
            status = polyIter.getNormal(vtx,mayaNorm,MSpace::kObject);
            if (!CheckMayaStatus(status))
               return Parent::lockMesh(time,objectOffset);
            Point3D norm(mayaNorm[0],mayaNorm[1],mayaNorm[2]);
            norm = (objectOffset * norm) - (objectOffset * Point3D(0,0,0));
            if (dotProduct(norm,norm)>0.01f)
               norm.normalize();
            else
               norm = Point3D(0,0,1);

            // grab tvert
            Point2D tvert(0,0);
            if (fnMesh.numUVs() > 0)
            {
               if (polyIter.hasUVs())
               {
                  S32 uvIndex;
                  if (polyIter.getUVIndex(vtx,uvIndex))
                     tvert = tverts[uvIndex];
               }
            }

            mIndices.push_back(addVertex(vert,norm,tvert,vertId));
         }
         mFaces.push_back(face);
      }

      getMaterials();

      return Parent::lockMesh(time,objectOffset);
   }

   void MayaAppMesh::getMaterials()
   {
      // Maya material extraction code pulled from
      // findTexturesPerPolyCmd.cpp from Maya sample plugins.

      MStatus status;

      S32 instanceNum = 0;
      if (mPath.isInstanced())
         instanceNum = mPath.instanceNumber();

       // Get a list of all sets pertaining to the selected shape and the
       // members of those sets.
      MFnMesh fnMesh(mPath);
      MObjectArray sets;
      MObjectArray comps;
      if (!fnMesh.getConnectedSetsAndMembers(instanceNum, sets, comps, true))
      {
         AppConfig::SetExportError("ERROR: getMaterials: MFnMesh::getConnectedSetsAndMembers\r\n");
         return;
      }

      // Loop through all the sets.  If the set is a polygonal set, find the
      // shader attached to the and print out the texture file name for the
      // set along with the polygons in the set.
      //
      for ( S32 i=0; i<sets.length(); i++ )
      {
         MObject set = sets[i];
         MObject comp = comps[i];

         MFnSet fnSet( set, &status );
         if (MS::kFailure == status)
         {
            AppConfig::SetExportError("ERROR: getMaterials: MFnSet::MFnSet\r\n");
            return;
         }

         // Make sure the set is a polygonal set.  If not, continue.
         MItMeshPolygon piter(mPath, comp, &status);
         if (MS::kFailure == status)
            continue;

         // Find the texture that is applied to this set.  First, get the
         // shading node connected to the set.  Then, if there is an input
         // attribute called "color", search upstream from it for a texture
         // file node.
         MObject shaderNode = findShader(set);
         if (shaderNode == MObject::kNullObj)
            continue;

         MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", &status);
         if (status == MS::kFailure)
            continue;

         MItDependencyGraph dgIt(colorPlug, MFn::kFileTexture,
                        MItDependencyGraph::kUpstream, 
                        MItDependencyGraph::kBreadthFirst,
                        MItDependencyGraph::kNodeLevel, 
                        &status);

         if (status == MS::kFailure)
            continue;
         
         dgIt.disablePruningOnFilter();

         // If no texture file node was found, just continue.
         if (dgIt.isDone())
            continue;
           
         // Print out the texture node name and texture file that it references.
         //
         MObject textureNode = dgIt.thisNode();
         MPlug filenamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
         MString textureName;
         filenamePlug.getValue(textureName);

         for (; !piter.isDone(); piter.next())
         {
            S32 faceIdx = piter.index();
            if (mFaces[faceIdx].type & Primitive::NoMaterial)
            {
               mFaces[faceIdx].type ^= Primitive::NoMaterial;
               mFaces[faceIdx].type |= mMaterials.size();
            }
            else
               AppConfig::SetExportError("Mesh::getMaterials: face with multiple materials found");
         }

         Material mat;
         mat.name = textureName.asChar();
         mat.reflectance = -1;
         mat.bump = -1;
         mat.detail = -1;
         mat.detailScale = 1.0f;
         mat.reflection = 0.0f;
         mat.flags = Material::SWrap|Material::TWrap|Material::NeverEnvMap;

         MFnLambertShader lshader(shaderNode,&status);
         if (status == MStatus::kSuccess)
         {
            F32 translucence = lshader.translucenceCoeff();
            F32 selfIlluminating = lshader.glowIntensity();
            if (translucence>0.1f)
               mat.flags |= Material::Translucent;
            if (selfIlluminating>0.1f)
               mat.flags |= Material::SelfIlluminating;
         }
         MFnReflectShader rshader(shaderNode,&status);
         if (status == MStatus::kSuccess)
         {
            F32 reflectivity = rshader.reflectivity();
            if (reflectivity>0.01f)
            {
               mat.reflection = reflectivity;
               mat.flags ^= Material::NeverEnvMap;
            }

         }

         // now get the texture placement node
         status = dgIt.resetTo(colorPlug, MFn::kPlace2dTexture,
                               MItDependencyGraph::kUpstream, 
                               MItDependencyGraph::kBreadthFirst,
                               MItDependencyGraph::kNodeLevel);

         dgIt.disablePruningOnFilter();
         if (status != MS::kFailure)
               if (!dgIt.isDone())
         {
            MObject placeNode = dgIt.thisNode();

            MStatus stat;

            MFnDependencyNode fnNode(placeNode,&stat);

            bool swrap = true;
            bool twrap = true;
            MPlug plug = fnNode.findPlug(MString("wrapU"),&stat);
            plug.getValue(swrap,MDGContext());
            plug = fnNode.findPlug(MString("wrapV"),&stat);
            plug.getValue(swrap,MDGContext());

            if (!swrap)
               mat.flags ^= Material::SWrap;
            if (!twrap)
               mat.flags ^= Material::TWrap;
          }
           
         /*
         Need to deal with these flags:
         Additive          = 0x00000008,
         Subtractive       = 0x00000010,
         IFLMaterial       = 0x08000000,
         */

         mMaterials.push_back(mat);
      }
   }


   MObject MayaAppMesh::findShader( MObject& setNode )
   {
      //  Description: Find the shading node for the given shading group set node.
      MFnDependencyNode fnNode(setNode);
      MPlug shaderPlug = fnNode.findPlug("surfaceShader");
            
      if (!shaderPlug.isNull())
      {
         MPlugArray connectedPlugs;
         bool asSrc = false;
         bool asDst = true;
         shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );

         if (connectedPlugs.length() != 1)
            AppConfig::SetExportError("Error getting shader\r\n");
         else 
            return connectedPlugs[0].node();
      }         
      
      return MObject::kNullObj;
   }

   void MayaAppMesh::unlockMesh()
   {
      Parent::unlockMesh();

      // no more cleanup...but if there were some to do, we'd do it here
   }

   void MayaSetTime(const AppTime & time)
   {
      MTime mayaTime(time.getF64(),MTime::kSeconds);
      MGlobal::viewFrame(mayaTime);
   }

   Matrix<4,4,F32> MayaToDtsTransform(MMatrix & mayaMat)
   {
      Matrix<4,4,F32> mat;
      for (S32 i=0; i<4; i++)
         for (S32 j=0; j<4; j++)
            mat[j][i] = mayaMat[i][j];

      F64 scale;
      MDistance dist; // starts as internal units
      dist.setValue(1.0f);
      scale = dist.asMeters();

      mat[0][3] *= scale;
      mat[1][3] *= scale;
      mat[2][3] *= scale;

      return mat;
   }

   bool CheckMayaStatus(MStatus & status)
   {
      if (!bool(status))
      {
         AppConfig::SetExportError(avar("Maya error: %s",status.errorString().asChar()));
         status = MStatus::kSuccess;
         return false;
      }
      return true;
   }

   void HandleMayaNegative(char * name)
   {
      S32 pos = strlen(name);
      while (pos)
      {
         --pos;
         if (isdigit(name[pos]))
            continue;
         if (name[pos]=='_')
            name[pos]='-';
         break;
      }
   }

}; // namespace DTS
