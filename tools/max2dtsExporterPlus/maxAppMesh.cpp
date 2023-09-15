//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "maxAppMesh.h"
#include "maxAppNode.h"
#include "appConfig.h"
#include "appIfl.h"
#include "skinHelper.h"

#pragma pack(push,8)
#include <max.h>
#include <iparamb2.h>
#include <ISkin.h>
#include <modstack.h>
#include <istdplug.h>
#include <stdmat.h>
#include <maxtypes.h>
#pragma pack(pop)

//----------------------------------------------------------------
// 3dsMax utility function:
// Grab the tri object from the max mesh node
//----------------------------------------------------------------
TriObject * getTriObject( INode *pNode, S32 time, bool & deleteIt)
{
   TriObject * tri = NULL;
   IParamBlock * paramBlock = NULL;

   // if the object can't convert to a tri-mesh, eval world state to
   // get an object that can:
   const ::ObjectState &os = pNode->EvalWorldState(time);

   if ( os.obj->CanConvertToType(triObjectClassID) )
      tri = (TriObject *)os.obj->ConvertToType( time, triObjectClassID );

   deleteIt = (tri && (tri != os.obj));

   return tri;
}

//----------------------------------------------------------------
// 3dsMax utility function:
// Grab the skin data from the max mesh node
//----------------------------------------------------------------
void findSkinData(INode * pNode, ISkin **skin, ISkinContextData ** skinData)
{
   // till proven otherwise...
   *skin = NULL;
   *skinData = NULL;

   // Get object from node. Abort if no object.
   ::Object* obj = pNode->GetObjectRef();
   if (!obj)
      return;
   
   Modifier * mod = NULL;

   // Is derived object ?
   S32 i;
   while (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
   {
      IDerivedObject* dobj = (IDerivedObject*) obj;
      // Iterate over all entries of the modifier stack.
      for (i=0;i<dobj->NumModifiers();i++)
         if (dobj->GetModifier(i)->ClassID() == SKIN_CLASSID)
            break;

      if (i!=dobj->NumModifiers())
      {
         mod = dobj->GetModifier(i);
         break;
      }
      obj = dobj->GetObjRef();
   }
   if (!mod)
      return;
   *skin = (ISkin*) mod->GetInterface(I_SKIN);
   if (!*skin)
      return;
   *skinData = (*skin)->GetContextInterface(pNode);
   if (!*skinData)
      *skin=NULL; // return both or neither
}

namespace DTS
{
   MaxAppMesh::MaxAppMesh(INode * maxNode, AppNode * appNode)
   {
      mMaxNode = maxNode;
      mAppNode = appNode;
   }

   Matrix<4,4,F32> MaxAppMesh::getMeshTransform(const AppTime & time)
   {
      assert(!mLocked && "Mesh is locked");

      Matrix<4,4,F32> ret;
      Matrix3 mat = mMaxNode->GetObjTMAfterWSM(SecToTicks(time.getF32()));
      return convertFromMaxMatrix(mat);
   }

   F32 MaxAppMesh::getVisValue(const AppTime & time)
   {
      assert(!mLocked && "Mesh is locked");

      return mMaxNode->GetVisibility(SecToTicks(time.getF32()));
   }

   bool MaxAppMesh::getMaterial(S32 matIdx, Material & mat)
   {
      mat.reflectance = -1;
      mat.bump = -1;
      mat.detail = -1;
      mat.detailScale = 1.0f;
      mat.reflection = 1.0f;
      mat.flags = 0;

      // do some hocus pocus to get the material....

      Mtl * mtl = mMaxNode->GetMtl();
      if( !mtl )
         return false;

      if( mtl->ClassID() == Class_ID(MULTI_CLASS_ID,0) )
      {
         MultiMtl * multiMtl = (MultiMtl*)mtl;
         if (multiMtl->NumSubMtls()==0)
            return false;
         matIdx %= multiMtl->NumSubMtls();
      
         mtl = multiMtl->GetSubMtl( matIdx );
      }

      if( mtl->ClassID() != Class_ID(DMTL_CLASS_ID,0) )
      {
         AppConfig::SetExportError(avar("Unexpected material type on mesh \"%s\".",mMaxNode->GetName()));
         return false;
      }
   
      StdMat * stdMat = (StdMat*)mtl;

      // we now have a standard material...this guy has a number of maps
      // the diffuse map corresponds to the texture
      // the reflection map is used for environment mapping (normally this will be in the alpha of
      //    the texture, but under some circumstances -- translucency -- you want a separate map for it)
      // the material also has a bump map and a detail map (we look for this in the ambient map...
      //    ambient because detail maps add ambiance... :)

      // get diffuse map...

      if (stdMat->GetSubTexmap(ID_DI) == NULL || !stdMat->MapEnabled(ID_DI))
         // no diffuse...
         return false;

      if (stdMat->GetSubTexmap(ID_DI)->ClassID() != Class_ID(BMTEX_CLASS_ID,0))
      {
         AppConfig::SetExportError(avar("Diffuse channel on mesh \"%s\" has a non-bitmap texture map.",mMaxNode->GetName()));
         return false;
      }

      BitmapTex * diffuse = (BitmapTex*)stdMat->GetSubTexmap(ID_DI);
      mat.name = diffuse->GetMapName();

      // set up texture flags
      if (stdMat->MapEnabled(ID_OP))
      {
         // note:  translucent if opacity channel is enabled
         mat.flags |= Material::Translucent;
         if (stdMat->GetTransparencyType() == TRANSP_ADDITIVE)
            mat.flags |= Material::Additive;
         else if (stdMat->GetTransparencyType() == TRANSP_SUBTRACTIVE)
            mat.flags |= Material::Subtractive;
      }
      if (!stdMat->MapEnabled(ID_RL))
         // only environment map if reflectance check box is set (but no material necessary)
         mat.flags |= Material::NeverEnvMap;
      mat.reflectance = stdMat->GetTexmapAmt(ID_RL,0);

      if (diffuse->GetUVGen()->GetTextureTiling() & U_WRAP)
         mat.flags |= Material::SWrap;
      if (diffuse->GetUVGen()->GetTextureTiling() & V_WRAP)
         mat.flags |= Material::TWrap;
      if ( stdMat->GetSelfIllum(0) > 0.99f)
         mat.flags |= Material::SelfIlluminating;

      //------------
      // If this is an ifl, then create the ifl material if it doesn't exist and mark as ifl
      const char * dot = strchr(mat.name.c_str(),'.');
      if (dot && !stricmp(dot+1,"ifl"))
      {
         mat.flags |= Material::IFLMaterial;
         while (mIfls.size()<=matIdx)
            mIfls.push_back(NULL);
         if (!mIfls[matIdx])
            mIfls[matIdx] = new AppIfl(mat.name.c_str());
      }

      return true;
   }

   bool MaxAppMesh::animatesFrame(const AppSequenceData & seqData)
   {
      assert(!mLocked && "Mesh is locked");

      S32 startTime = SecToTicks(seqData.startTime.getF32());
      S32 endTime   = SecToTicks(seqData.endTime.getF32());

      bool deleteIt;    
      TriObject * tri = getTriObject(mMaxNode,startTime,deleteIt);
      if (!tri)
         return false;

      Interval ivalid;
      ivalid = tri->ChannelValidity(startTime, GEOM_CHAN_NUM);

      if (deleteIt)
         tri->DeleteMe();

      return (ivalid.Start() > startTime || ivalid.End() < endTime);
   }

   bool MaxAppMesh::animatesMatFrame(const AppSequenceData & seqData)
   {
      assert(!mLocked && "Mesh is locked");

      S32 startTime = SecToTicks(seqData.startTime.getF32());
      S32 endTime   = SecToTicks(seqData.endTime.getF32());

      bool deleteIt;    
      TriObject * tri = getTriObject(mMaxNode,startTime,deleteIt);
      if (!tri)
          return false;

      Interval ivalid;
      ivalid = tri->ChannelValidity(startTime, TEXMAP_CHAN_NUM);

      if (deleteIt)
         tri->DeleteMe();

      return (ivalid.Start() > startTime || ivalid.End() < endTime);
   }

   AppMeshLock MaxAppMesh::lockMesh(const AppTime & time, const Matrix<4,4,F32> & objectOffset)
   {
      // are we mirrored?
      AffineParts parts;
      decomp_affine(objectOffset,&parts);
      bool mirror = parts.sign < 0.0f;

      bool delTri;
      TriObject * tri = getTriObject(mMaxNode,SecToTicks(time.getF32()),delTri);
      ::Mesh & maxMesh = tri->mesh;

      S32 i;
      S32 lastMatIdx = -1;
      Matrix3 uvTransform;

      // start lists empty
      mFaces.clear();
      mVerts.clear();
      mTVerts.clear();
      mIndices.clear();
      mSmooth.clear();
      mVertId.clear();

      // start out with faces and crop data allocated
      mFaces.resize(maxMesh.getNumFaces());
   
      // if no faces, detect it (use redundant test just in case
      bool tmap = maxMesh.mapSupport(1) && maxMesh.tVerts; // 1 == default map

      // get faces, points & materials
      for (i=0; i<mFaces.size();i++)
      {
         Face & maxFace = maxMesh.faces[i];
         Primitive & tsFace = mFaces[i];

         // set faces material index
         tsFace.type = maxFace.getMatID(); // use max mat idx (get's converted by shapemimic)
         tsFace.firstElement = mIndices.size();
         tsFace.numElements = 3;
         tsFace.type |= Primitive::Triangles|Primitive::Indexed;

         // set vertex indices
         S32 idx0 = maxFace.v[0];
         S32 idx1 = maxFace.v[2]; // switch the order to be CW
         S32 idx2 = maxFace.v[1]; // switch the order to be CW
         if (mirror)
         {
            S32 tmp = idx1;
            idx1 = idx2;
            idx2 = tmp;
         }
         Point3D v0(maxMesh.verts[idx0].x,maxMesh.verts[idx0].y,maxMesh.verts[idx0].z);
         Point3D v1(maxMesh.verts[idx1].x,maxMesh.verts[idx1].y,maxMesh.verts[idx1].z);
         Point3D v2(maxMesh.verts[idx2].x,maxMesh.verts[idx2].y,maxMesh.verts[idx2].z);

         Point3D vert0 = objectOffset * v0;
         Point3D vert1 = objectOffset * v1;
         Point3D vert2 = objectOffset * v2;

         // set texture vertex indices
         Point2D tvert0(0,0);
         Point2D tvert1(0,0);
         Point2D tvert2(0,0);
         if (tmap)
         {
            TVFace & maxTVFace = maxMesh.mapFaces(1)[i]; // 1 == default map

            S32 tidx0 = maxTVFace.getTVert(0);
            S32 tidx1 = maxTVFace.getTVert(2); // switch the order to be CW
            S32 tidx2 = maxTVFace.getTVert(1); // switch the order to be CW
            if (mirror)
            {
               S32 tmp = tidx1;
               tidx1 = tidx2;
               tidx2 = tmp;
            }

            if (lastMatIdx != maxFace.getMatID())
            {
               // get texture transform for this material
               uvTransform.IdentityMatrix();
               Mtl * mtl = mMaxNode->GetMtl();
               if( mtl )
               {
                  if( mtl->ClassID() == Class_ID(MULTI_CLASS_ID,0) )
                  {
                     MultiMtl * multiMtl = (MultiMtl*)mtl;
                     if (multiMtl->NumSubMtls()>0)
                        mtl = multiMtl->GetSubMtl( maxFace.getMatID() % multiMtl->NumSubMtls());
                  }

                  if( mtl->ClassID() == Class_ID(DMTL_CLASS_ID,0) )
                  {
                     StdMat * stdMat = (StdMat*)mtl;
                     if (stdMat->GetSubTexmap(ID_DI)!=NULL && stdMat->MapEnabled(ID_DI) && stdMat->GetSubTexmap(ID_DI)->ClassID() == Class_ID(BMTEX_CLASS_ID,0))
                     {
                        BitmapTex * diffuse = (BitmapTex*)stdMat->GetSubTexmap(ID_DI);
                        diffuse->GetUVGen()->GetUVTransform(uvTransform);                     
                     }
                  }
               }
               lastMatIdx = maxFace.getMatID();
            }

            Point2 tv0 = maxMesh.mapVerts(1)[tidx0] * uvTransform;
            Point2 tv1 = maxMesh.mapVerts(1)[tidx1] * uvTransform;
            Point2 tv2 = maxMesh.mapVerts(1)[tidx2] * uvTransform;
            tvert0.x(tv0.x);
            tvert0.y(1.0f-tv0.y);
            tvert1.x(tv1.x);
            tvert1.y(1.0f-tv1.y);
            tvert2.x(tv2.x);
            tvert2.y(1.0f-tv2.y);
         }
         tsFace.type |= maxFace.getMatID();  // return max mat id...gets converted to ts mat id by shapemimic

         // now add indices...this is easy right now...later we'll mess this up
         mIndices.push_back(addVertex(vert0,tvert0,idx0,maxFace.smGroup));
         mIndices.push_back(addVertex(vert1,tvert1,idx1,maxFace.smGroup));
         mIndices.push_back(addVertex(vert2,tvert2,idx2,maxFace.smGroup));
      }

      if (delTri)
         delete tri;

      return Parent::lockMesh(time,objectOffset);
   }

   void MaxAppMesh::unlockMesh()
   {
      Parent::unlockMesh();

      // no more cleanup...but if there were some to do, we'd do it here
   }

   void MaxAppMesh::getSkinData()
   {
      if (mSkinDataFetched)
         return;
      mSkinDataFetched = true;

      ISkin * skin;
      ISkinContextData * skinData;
      findSkinData(mMaxNode,&skin,&skinData);
      if (!skin || !skinData || !skin->GetNumBones())
         return;

      // This is a hack to avoid making the skin helper a ws modifier
      // A hack, but needed because ws modifiers screw things up (add
      // offsets for some reason).
      SkinHelper::smTheOnlyOne = mMaxNode;

      // add skin helper modifier...
      ::Object * obj = mMaxNode->GetObjectRef();
      IDerivedObject * dobj = (IDerivedObject*)CreateDerivedObject(mMaxNode->GetObjectRef());
      SkinHelper * skinHelper = (SkinHelper*)CreateInstance(GetSkinHelperDesc()->SuperClassID(),GetSkinHelperDesc()->ClassID());
      dobj->AddModifier(skinHelper);
      mMaxNode->SetObjectRef(dobj);

      // get bones
      S32 i,j, numBones = skin->GetNumBones();
      for (i=0; i<numBones; i++)
      {
         mBones.push_back(new MaxAppNode(skin->GetBone(i)));
         AppConfig::PrintDump(PDPass2,avar("Adding skin object from skin \"%s\" to bone \"%s\" (%i).\r\n",mMaxNode->GetName(),mBones[i]->getName(),i));
      }

      bool delTri;
      TriObject * tri = NULL;

      // get skin mesh
      tri = getTriObject(mMaxNode,0,delTri);

      // get vertex weights from alternate tv channels
      S32 numPoints = tri->mesh.getNumVerts();
      if (tri->mesh.getNumMaps()<2+((1+numBones)>>1))
      {
         AppConfig::SetExportError("Assertion failed on skin object");
         return;
      }
      mWeights.resize(numBones);
      for (i=0; i<mWeights.size(); i++)
      {
         mWeights[i] = new std::vector<F32>;
         mWeights[i]->resize(numPoints);
         for (j=0; j<numPoints; j++)
            (*mWeights[i])[j]=0.0f;
      }

      for (j=0; j<numBones; j++)
      {
         AppConfig::PrintDump(-1,avar("Adding weights for bone %i (\"%s\")\r\n",j,mBones[j]->getName()));
         std::vector<bool> gotWeight;
         gotWeight.resize(tri->mesh.numVerts);
         for (i=0; i<gotWeight.size(); i++)
            gotWeight[i]=false;
         for (i=0; i<tri->mesh.numFaces; i++)
         {
            S32 ch = 2+(j>>1);
            Face & face = tri->mesh.faces[i];
            TVFace & tvFace = tri->mesh.mapFaces(ch)[i];

            for (S32 count=0; count<3; count++)
            {
               S32 idx = face.v[count];
               if (!gotWeight[idx])
               {
                  UVVert tv = tri->mesh.mapVerts(ch)[tvFace.t[count]];
                  F32 w = (j&1) ? tv.y : tv.x;
                  (*mWeights[j])[idx] = w;
                  if (w>0.01f)
                     AppConfig::PrintDump(-1,avar("   Vertex %i, weight %f\r\n",idx,w));
                  gotWeight[idx]=true;
               }
            }
         }
      }

      if (delTri)
         delete tri;

      // done with helper...remove it now...
      dobj->DeleteModifier(); // this'll be our skin helper
      SkinHelper::smTheOnlyOne = NULL;

      // following copied from AVCUtil.cpp...is needed to get rid of bar in modifier list
      if (dobj->NumModifiers() == 0 && !dobj->TestAFlag(A_DERIVEDOBJ_DONTDELETE))
      {
         obj = dobj->GetObjRef();
         obj->TransferReferences(dobj);
         dobj->SetAFlag(A_LOCK_TARGET);
         dobj->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
         obj->NotifyDependents(FOREVER,0,REFMSG_SUBANIM_STRUCTURE_CHANGED);
         dobj->ClearAFlag(A_LOCK_TARGET);
         dobj->MaybeAutoDelete();
      }
   }

};
