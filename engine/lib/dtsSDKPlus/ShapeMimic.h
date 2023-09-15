//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// The ShapeMimic tries to hold court in both the App world and
// the Torque three-space world.  It holds a shape tree isomorphic
// to what the shape will look like when exported, but maintains
// links to App objects and delays computing certain things
// until the tsshape is finally created in generateShape().

#ifndef SHAPEMIMIC_H_
#define SHAPEMIMIC_H_

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSPlusTypes.h"
#include "DTSUtil.h"
#include "appNode.h"
#include "appMesh.h"
#include "appSequence.h"
#include "appIfl.h"
#include "appConfig.h"

namespace DTS
{
   struct IflMimic
   {
      AppIfl * appIfl;
      S32 materialSlot;
   };

   struct SkinMimic
   {
      typedef std::vector<F32> WeightList;
   
      AppMesh * appMesh;
      Mesh * skinMesh;
      S32 detailSize;
      S32 skinNum;
      S32 meshNum;
   
      std::vector<Primitive> faces;
      std::vector<Point3D> verts;
      std::vector<Point3D> normals;
      std::vector<Point2D> tverts;
      std::vector<U16> indices;
      std::vector<U32> smoothingGroups;
      std::vector<U32> vertId;
   
      std::vector<AppNode *> bones;
      std::vector<WeightList*> weights;
      
      ~SkinMimic() { for (S32 i=0;i<weights.size(); i++) delete weights[i]; }
   };

   struct MeshMimic
   {
      AppMesh * appMesh;
      Mesh * tsMesh;
      SkinMimic * skinMimic;
      bool billboard; // i.e., face camera
      bool sortedObject;
      S32 numVerts; // number of unique vertices
      S32 meshNum;
      std::vector<U32> smoothingGroups;
      std::vector<U32> remap;
      std::vector<U32> vertId;
   
      Matrix<4,4,F32> objectOffset; // NOTE: not valid till late in the game
   
      MeshMimic(AppMesh * mesh) { appMesh = mesh; skinMimic = NULL; tsMesh = NULL; }
   };

   struct ObjectMimic
   {
      enum { MaxDetails=20 };
      struct Detail
      {
         S32 size;
         MeshMimic * mesh;
      };
   
      // object name
      char * name;
      char * fullName; // name of object in tree
   
      // each object has several meshes attached
      S32 numDetails;
      Detail details[MaxDetails];
   
      // we'll check the object against this list in the end        
      std::vector<S32> * validDetails;
      AppNode * inTreeNode; // this is the node that sits in the shape's node hierrarchy
      AppMesh * inTreeMesh; // this is the mesh that sits in the shape's node hierrarchy
   
      // The next two items are used for sorting objects
      // objects are sorted by subTreeNum first, and then
      // priority (smallest to highest in both cases).
      S32 subtreeNum;
      U32 priority;
   
      // The node in the 3D app this object hangs on (i.e., the one in the shape not
      // the loose objects that make up the detail levels).
      AppNode * appParent;

      // Similar to above: the app node that corresponds to tsNode that will
      // be our parent.  Starts out the same as appParent, but is revised
      // as we prune unwanted nodes from the tree structure.
      AppNode * appTSParent;
   
      // ts node index we hang off
      S32 tsNodeIndex;
      S32 tsObjectIndex;
   
      // This is the eventual payoff
      DTS::Object * tsObject;
       
      //
      bool isBone;
      bool isSkin;

      AppMesh * getSkin()
      {
         for (S32 dl=0; dl<numDetails; dl++)
            if (details[dl].mesh)
               return details[dl].mesh->skinMimic ? details[dl].mesh->skinMimic->appMesh : NULL;
         return NULL;
      }
   
      ~ObjectMimic()
      {
         delete [] name;
         delete [] fullName;
         for (S32 i=0;i<numDetails;i++)
            delete details[i].mesh;
      }
   };

   struct NodeMimic
   {
      // our twin in the max world:
      AppNode * appNode;
       
      // our neighbors in the mimic world:
      NodeMimic * parent;
      NodeMimic * child;
      NodeMimic * sibling;
   
      // transforms at default time
      AffineParts child0;
      AffineParts parent0;

      // index of our ts version
      S32 number;
       
      // our mimic object
      std::vector<ObjectMimic*> objects;
   };

   class ShapeMimic
   {
      struct Subtree
      {
         std::vector<S32> validDetails;
         std::vector<const char*> detailNames;
         std::vector<AppNode*> detailNodes;
         NodeMimic start;
      };

      std::vector<Subtree*> subtrees;
      std::vector<ObjectMimic*> objectList;
      std::vector<SkinMimic*> skins;
      std::vector<IflMimic*> iflList;
      std::vector<AppSequence*> sequences;
      std::vector<Material> materials;
      AppNode * boundsNode;
   
      // this gets filled in late in the game
      // it holds the nodes that actually go into the shape
      // in the order they appear in the shape
      std::vector<NodeMimic*> nodes;

      static std::vector<Quaternion*> nodeRotCache;
      static std::vector<Point3D*> nodeTransCache;
      static std::vector<Quaternion*> nodeScaleRotCache;
      static std::vector<Point3D*> nodeScaleCache;

      static std::vector<AppNode*> cutNodes;
      static std::vector<AppNode*> cutNodesParents;
       
      // error control
      void setExportError(const char * errStr) { AppConfig::SetExportError(errStr); }
      const char * getError() { return AppConfig::GetExportError(); }
      bool isError() { return AppConfig::IsExportError(); }
   
      // called by generateShape
      void generateBounds(Shape * shape);
      void generateDetails(Shape * shape);
      void generateSubtrees(Shape * shape);
      void generateObjects(Shape * shape);
      void generateDefaultStates(Shape * shape);
      void generateIflMaterials(Shape * shape);
      void generateSequences(Shape * shape);
      void generateMaterialList(Shape * shape);
      void generateSkins(Shape * shape);
      void optimizeMeshes(Shape * shape);
      void convertSortObjects(Shape * shape);
      void initShape(Shape*);

      // deeper generate methods...
      void generateBillboardDetail(AppNode * detailNode, DetailLevel & detail);
      void sortTSDetails(std::vector<DTS::DetailLevel> & details);
      void setObjectPriorities(std::vector<ObjectMimic*> & objects);
      void sortObjectList(std::vector<ObjectMimic*> & objects);
      void generateNodeTransform(NodeMimic *, const AppTime & time, bool blend, const AppTime & blendReferenceTime, Quaternion & rot, Point3D & trans, Quaternion & qrot, Point3D & scale);
      void generateObjectState(ObjectMimic *, const AppTime & time, Shape *, bool addFrame, bool addMatFrame);

      // generate animation data
      void generateFrame(ObjectMimic *, const AppTime & time, bool addFrame, bool addMatFrame);
      void generateGroundAnimation(Shape *, Sequence &, AppSequenceData &);
      void generateObjectAnimation(Shape *, Sequence &, AppSequenceData &);
      void generateFrameTriggers(Shape *, Sequence &, AppSequenceData &, AppSequence *);
      void generateNodeAnimation(Shape *, Sequence &, AppSequenceData &);

      // add transform data
      void addNodeRotation(NodeMimic *, const AppTime & time, Shape *, bool blend, Quaternion & rot, bool defaultVal);
      void addNodeTranslation(NodeMimic *, const AppTime & time, Shape *, bool blend, Point3D & trans, bool defaultVal);
      void addNodeUniformScale(NodeMimic *, const AppTime & time, Shape *, bool blend, F32 scale);
      void addNodeAlignedScale(NodeMimic *, const AppTime & time, Shape *, bool blend, Point3D & scale);
      void addNodeArbitraryScale(NodeMimic *, const AppTime & time, Shape *, bool blend, Quaternion & qrot, Point3D & scale);

      // membership tests for animation
      void setIflMembership(Shape *, Sequence &, AppSequenceData &, S32 & iflCount);
      S32 setObjectMembership(Shape *, Sequence &, AppSequenceData &, S32 & objectCount);
      void setNodeMembership(Shape *, Sequence &, AppSequenceData &,
                             S32 & rotCount, S32 & transCount, 
                             S32 & uniformScaleCount, S32 & alignedScaleCount, S32 & arbitraryScaleCount);
      void setRotationMembership(Shape *, Sequence &, AppSequenceData &, S32 & rotCount);
      void setTranslationMembership(Shape *, Sequence &, AppSequenceData &, S32 & transCount);
      void setScaleMembership(Sequence &, AppSequenceData &, S32 & arbitraryScale, S32 & alignedScale, S32 & uniformScale);
      bool animatesAlignedScale(AppSequenceData &);
      bool animatesArbitraryScale(AppSequenceData &);
      S32 setUniformScaleMembership(Sequence &, AppSequenceData &);
      S32 setAlignedScaleMembership(Sequence &, AppSequenceData &);
      S32 setArbitraryScaleMembership(Sequence &, AppSequenceData &);

      // add material from mesh
      S32 addFaceMaterial(AppMesh *,S32 faceNum);
      S32 addMaterial(Material mat);

      // utility methods
      void fillNodeTransformCache(std::vector<NodeMimic*> &, Sequence &, AppSequenceData &);
      ObjectMimic * addObject(AppNode *, AppMesh *, std::vector<S32> * validDetails);
      ObjectMimic * getObject(AppNode *, AppMesh *, char * name, S32 size, S32 * detailNum, bool matchFullName = true, bool isBone=false, bool isSkin=false);
      ObjectMimic * addBoneObject(AppNode * node, S32 subtreeNum);
      MeshMimic * addSkinObject(SkinMimic * skinMimic);
      S32  addName(const char *, Shape * shape);
      void computeNormals(std::vector<Primitive> &, std::vector<U16> & indices, std::vector<Point3D> & verts, std::vector<Point3D> & norms, std::vector<U32> & smooth, S32 vertsPerFrame, S32 numFrames);
      void copyWeightsToVerts(SkinMimic * skinMimic);
      void collapseVertices(Mesh *, std::vector<U32> & smooth, std::vector<U32> & remap, std::vector<U32> * vertId);
      bool vertexSame(Point3D & v1, Point3D & v2, Point2D & tv1, Point2D & tv2, U32 smooth1, U32 smooth2, Point3D & norm1, Point3D & norm2, U32 idx1, U32 idx2, std::vector<U32> * vertId);
      void stripify(std::vector<Primitive> &, std::vector<U16> & indices);
      void collapseTransforms();
      bool cut(NodeMimic * mimicNode);
      void snip(NodeMimic * nodeMimic);
      bool testCutNodes(AppSequenceData & seqData);
      NodeMimic * findNextNode(NodeMimic *);

      void dumpShapeNode(Shape * shape, S32 level, S32 nodeIndex, std::vector<S32> & detailSizes);
      void dumpShape(Shape * shape);

   public:
      ShapeMimic();
      ~ShapeMimic();
   
      // add shape items as we walk the scene
      void addBounds(AppNode * appNode) { boundsNode=appNode; }
      void addSubtree(AppNode * appNode);
      void addNode(NodeMimic *,AppNode *, std::vector<S32> &,bool);
      void addMesh(AppNode * node, AppMesh * mesh) { addObject(node,mesh,NULL); }
      void addSkin(AppMesh * mesh);
      void addSequence(AppSequence * sequence) { sequences.push_back(sequence); }

      // the payoff...call after adding all of the above
      Shape * generateShape();
   };
}; // namespace DTS


#endif

