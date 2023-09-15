//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

// The ShapeMimic tries to hold court in both the App world and
// the Torque three-space world.  It holds a shape tree isomorphic
// to what the shape will look like when exported, but maintains
// links to App objects and delays computing certain things
// until the tsshape is finally created in generateShape().

#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "ShapeMimic.h"
#include "appConfig.h"
#include "translucentSort.h"
#include "nvStripWrap.h"
#include "stripper.h"

// See comment in dtsBitMatrix.h for why we don't use this.
// #define USE_NVIDIA_STRIPPER

namespace DTS {

std::vector<Quaternion*> ShapeMimic::nodeRotCache;
std::vector<Point3D*> ShapeMimic::nodeTransCache;
std::vector<Quaternion*> ShapeMimic::nodeScaleRotCache;
std::vector<Point3D*> ShapeMimic::nodeScaleCache;

std::vector<AppNode*> ShapeMimic::cutNodes;
std::vector<AppNode*> ShapeMimic::cutNodesParents;


//-----------------------------------------------------------
//
//-----------------------------------------------------------

ShapeMimic::ShapeMimic()
{
}

ShapeMimic::~ShapeMimic()
{
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

Shape * ShapeMimic::generateShape()
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   // this may be our second time around, make sure
   // certain variables and lists are initialized:
   nodes.clear();

   // cull as needed
   collapseTransforms();

   // no frills construction
   Shape * shape = new Shape;

   // step one:    generate bounds
   generateBounds(shape);

   // step two:    generate detail levels sort subTrees according to dl
   generateDetails(shape);

   // step three:  generate subTrees (tree structure w/o objects connected)
   generateSubtrees(shape);

   // step four:   generate objects -- hook up to nodes
   generateObjects(shape);
   
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;

   // at this point, we have a shape with all the details,
   // nodes, and objects set up.  We have also added a bunch 
   // of names.  In addition, the meshIndexList is set up and
   // the meshes list is set up.

   // step five:   set default states (including mesh data)
   generateDefaultStates(shape);

   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;
   
   // step six:    generate ifl materials
   generateIflMaterials(shape);

   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;
   
   // step seven:  animation
   if (AppConfig::GetEnableSequences())
       generateSequences(shape);
       
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;
   
   // step eight:  generate material list
   generateMaterialList(shape);

   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;
   
   // step eight:  generate the skins    
   generateSkins(shape);
   
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;
   
   // step nine:   optimize the meshes (but only if exporting them)
   if (AppConfig::GetExportOptimized())
      optimizeMeshes(shape);

   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return shape;
   
   // step ten:    convert sortObjects
   convertSortObjects(shape);

   initShape(shape);

   if (!isError())
      dumpShape(shape);

   return shape;
}

void ShapeMimic::initShape(Shape * shape)
{
   shape->setSmallestSize(0);

   S32 numss = shape->subshapes.size();

   // compute subShape numNodes,
   S32 i,prev = shape->nodes.size();
   for (i=numss-1; i>=0; i--)
   {
      shape->subshapes[i].numNodes = prev - shape->subshapes[i].firstNode;
      prev = shape->subshapes[i].firstNode;
   }

   // compute subShape numObjects
   prev = shape->objects.size();
   for (i=numss-1; i>=0; i--)
   {
      shape->subshapes[i].numObjects = prev - shape->subshapes[i].firstObject;
      prev = shape->subshapes[i].firstObject;
   }
   
   // compute subShape numDecals -- don't do decals...so this should be easy
   for (i=0; i<numss; i++)
      shape->subshapes[i].numDecals = shape->subshapes[i].firstDecal = 0;

   // make sure bounds are built on all the meshes
   for (i=0; i<shape->meshes.size(); i++)
      shape->meshes[i].calculateBounds();

   // ts shape quaternions are transposes of dtsSdk versions
   for (i=0; i<shape->nodeDefRotations.size(); i++)
      shape->nodeDefRotations[i][3] *= -1.0f;
   for (i=0; i<shape->nodeRotations.size(); i++)
      shape->nodeRotations[i][3] *= -1.0f;
   for (i=0; i<shape->nodeScaleRotsArbitrary.size(); i++)
      shape->nodeScaleRotsArbitrary[i][3] *= -1.0f;
   for (i=0; i<shape->groundRotations.size(); i++)
      shape->groundRotations[i][3] *= -1.0f;
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateBounds(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppMesh * boundsMesh = boundsNode->getNumMesh() ? boundsNode->getMesh(0) : NULL;
   if (!boundsMesh)
   {
      setExportError("Bounds node has no mesh.");
      return;
   }

   Matrix<4,4,F32> meshMat = boundsMesh->getMeshTransform(AppTime::DefaultTime());
   Matrix<4,4,F32> nodeMat = boundsNode->getNodeTransform(AppTime::DefaultTime());
   zapScale(nodeMat);
   Matrix<4,4,F32> objectOffset = nodeMat.inverse() * meshMat;

   Box boundsBox = boundsMesh->getBounds(objectOffset);
   F32 radius = boundsMesh->getRadius(objectOffset);
   F32 tubeRadius = boundsMesh->getTubeRadius(objectOffset);

   // now set up shape bounds parameters
   shape->center = (boundsBox.min + boundsBox.max) * 0.5f;
   shape->bounds.min = boundsBox.min;
   shape->bounds.max = boundsBox.max;
   shape->radius = radius;
   shape->tubeRadius = tubeRadius;
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateDetails(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // if nothing to export...
   if (subtrees.empty())
   {
       setExportError("Nothing to export.");
       return;
   }

   // each subTree has a set of detail levels
   // just plug them in and sort the list

   S32 i,j;
   for (i=0; i<subtrees.size(); i++)
   {
      Subtree * subtree = subtrees[i];
      for (j=0; j<subtree->validDetails.size(); j++)
      {
         DetailLevel detail;
         detail.subshape = i;
         detail.objectDetail = j;
         detail.size = (F32) subtree->validDetails[j];
         detail.avgError = -1;
         detail.maxError = -1;
         detail.polyCount = 0; // not currently using this
         detail.name = addName(subtree->detailNames[j],shape);
         if (!strnicmp(subtree->detailNames[j],"BB::",4))
            generateBillboardDetail(subtree->detailNodes[j],detail);
         shape->detailLevels.push_back(detail);
      }
   }

   // sort detail levels based on projection size
   sortTSDetails(shape->detailLevels);
}

void ShapeMimic::generateBillboardDetail(AppNode * detailNode, DetailLevel & detail)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // this is a billboard detail, this works a little differently...
   detail.subshape = -1;
   
   // determine properties...
   S32 numEquatorSteps = 4;
   S32 numPolarSteps = 0;
   F32 polarAngle;
   S32 dl = 0;
   S32 dim = 64;
   bool includePoles = true;

   detailNode->getInt("BB::EQUATOR_STEPS",numEquatorSteps);
   detailNode->getInt("BB::POLAR_STEPS",numPolarSteps);
   polarAngle = M_PI/(F32)(((numPolarSteps>>1)<<1)+5);
   detailNode->getFloat("BB::POLAR_ANGLE",polarAngle);
   detailNode->getInt("BB::DL",dl);
   detailNode->getInt("BB::DIM",dim);
   detailNode->getBool("BB::INCLUDE_POLES",includePoles);

   // set properties...
   U32 props = 0;
   props |= numEquatorSteps;
   props |= (numPolarSteps>>1) << 7;
   props |= ((S32)(64.0f * polarAngle/(M_PI*0.5f))) << 13;
   props |= dl<<19;
   props |= dim<<23;
   props |= includePoles ? 1<<31 : 0;
   detail.objectDetail = props;
}

S32 __cdecl compareTSDetails( void const *e1, void const *e2 )
{
   const DetailLevel * d1 = (const DetailLevel*)e1;
   const DetailLevel * d2 = (const DetailLevel*)e2;

   if (d1->size > d2->size)
      return -1;
   if (d2->size > d1->size)
      return 1;

   return 0;
}

void ShapeMimic::sortTSDetails(std::vector<DetailLevel> & details)
{
   if (details.size())
      qsort(&details[0],details.size(),sizeof(DetailLevel),compareTSDetails);
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

S32 ShapeMimic::addFaceMaterial(AppMesh * mesh,S32 faceNum)
{
   S32 matIdx = mesh->getFaceMaterial(faceNum);
   Material mat;
   if (!mesh->getMaterial(matIdx,mat))
      // code no material as -1...
      return -1;

   S32 retIdx = addMaterial(mat);

   // if we just added an ifl material, check to see if the ifl mimic
   // already exists...if not, create it
   if (mat.flags & Material::IFLMaterial)
   {
      S32 i;
      for (i=0; i<iflList.size(); i++)
      {
         if (iflList[i]->materialSlot == retIdx)
            // already there
            break;
      }
      if (i==iflList.size())
      {
         // no ifl mimic, add it now
         IflMimic * ifl = new IflMimic;
         ifl->appIfl = mesh->getIfl(retIdx);
         ifl->materialSlot = retIdx;
         iflList.push_back(ifl);
      }
   }

   return retIdx;
}

S32 ShapeMimic::addMaterial(Material mat)
{
   // return the index to this material if we already have it saved off
   // otherwise create a new entry...
   // material considered new if anything is different (name, flags, or any of the maps)
   // exception #1 is if the incoming item is an auxiliary item (a refelctionMap, bumpMap,
   //    or detail map) in that case, it will ignore everything but the name...
   // exception #2 is if the incoming item is an ifl (in which case it is added to back)

   bool auxiliary     = mat.flags & Material::AuxiliaryMask ? true : false;
   bool isTranslucent = mat.flags & Material::Translucent ? true : false;
   bool wraps         = mat.flags & (Material::SWrap|Material::TWrap) ? true : false;
   bool isIfl         = mat.flags & Material::IFLMaterial ? true : false;

   if (AppConfig::GetNoMipMap())
      mat.flags |= Material::NoMipMap;
   if (AppConfig::GetNoMipMapTranslucent() && isTranslucent)
      mat.flags |= Material::NoMipMap;
   if (AppConfig::GetZapBorder() && isTranslucent && !wraps)
      // material is translucent and doesn't wrap -- zap border
      mat.flags |= Material::MipMapZeroBorder;

   // get rid of path -- drop everything before last slash or :
   mat.name = std::string(getFileBase(mat.name.c_str()));

   for (S32 i=0; i<materials.size(); i++)
   {
      // first check name
      if (stricmp(mat.name.c_str(),materials[i].name.c_str()))
         continue;

      // good enough for auxiliary
      if (auxiliary)
      {
         if (materials[i].flags & Material::AuxiliaryMask)
            // if we're using an auxiliary map for two purpose, let it be known...
            materials[i].flags |= (mat.flags & Material::AuxiliaryMask);
         return i;
      }

      // a reflection map of -1 gets mapped to i...
      if (mat.reflectance!=-1 && materials[i].reflectance!=mat.reflectance)
         continue;

      // check the rest
      if (materials[i].flags==mat.flags &&
          materials[i].bump == mat.bump &&
          materials[i].detail == mat.detail &&
          isEqual(materials[i].detailScale,mat.detailScale,0.01f) &&
          isEqual(materials[i].reflection,mat.reflection,0.01f))
         return i;
   }

   // always have a reflection map, even if it's ourself
   if (mat.reflectance==-1 && !auxiliary)
      mat.reflectance = materials.size();

   // new one -- save the texture map material and return new index
   materials.push_back(mat);
   return materials.size()-1;
}

void ShapeMimic::addNodeRotation(NodeMimic * curNode, const AppTime & time, Shape * shape, bool blend, Quaternion & rot, bool defaultVal)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDNodeStates,avar("Adding%snode rotation at time %s for node \"%s\".\r\n",
        blend ? " blend " : " ", time.getStr(), curNode->appNode->getName()));

   if (!defaultVal)
      shape->nodeRotations.push_back(rot);
   else
      shape->nodeDefRotations.push_back(rot);

   AppConfig::PrintDump(PDNodeStateDetails,avar("  rotation:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",rot.x(),rot.y(),rot.z(),rot.w()));

   // all added, add separator to dump file...
   AppConfig::PrintDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeTranslation(NodeMimic * curNode, const AppTime & time, Shape * shape, bool blend, Point3D & trans, bool defaultVal)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDNodeStates,avar("Adding%snode translation at time %s for node \"%s\".\r\n",
       blend ? " blend " : " ", time.getStr(), curNode->appNode->getName()));

   if (!defaultVal)
      shape->nodeTranslations.push_back(trans);
   else
      shape->nodeDefTranslations.push_back(trans);

   AppConfig::PrintDump(PDNodeStateDetails,avar("  translation:     x=%3.5f, y=%3.5f, z=%3.5f\r\n",trans.x(),trans.y(),trans.z()));

   // all added, add separator to dump file...
   AppConfig::PrintDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeUniformScale(NodeMimic * curNode, const AppTime & time, Shape * shape, bool blend, F32 scale)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDNodeStates,avar("Adding%snode scale at time %s for node \"%s\".\r\n",
       blend ? " blend " : " ", time.getStr(), curNode->appNode->getName()));

   shape->nodeScalesUniform.push_back(scale);
   AppConfig::PrintDump(PDNodeStateDetails,avar("  uniform scale:     %3.5f\r\n",scale));

   // all added, add separator to dump file...
   AppConfig::PrintDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeAlignedScale(NodeMimic * curNode, const AppTime & time, Shape * shape, bool blend, Point3D & scale)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDNodeStates,avar("Adding%snode scale at time %s for node \"%s\".\r\n",
        blend ? " blend " : " ", time.getStr(), curNode->appNode->getName()));

   shape->nodeScalesAligned.push_back(scale);
   AppConfig::PrintDump(PDNodeStateDetails,avar("  aligned scale:     x=%3.5f, y=%3.5f, z=%3.5f\r\n",scale.x(),scale.y(),scale.z()));

   // all added, add separator to dump file...
   AppConfig::PrintDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::addNodeArbitraryScale(NodeMimic * curNode, const AppTime & time, Shape * shape, bool blend, Quaternion & qrot, Point3D & scale)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDNodeStates,avar("Adding%snode scale at time %s for node \"%s\".\r\n",
        blend ? " blend " : " ", time.getStr(), curNode->appNode->getName()));

   shape->nodeScaleRotsArbitrary.push_back(qrot);
   shape->nodeScalesArbitrary.push_back(scale);
   AppConfig::PrintDump(PDNodeStateDetails,avar("  arbitrary scale rot:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",qrot.x(),qrot.y(),qrot.z(),qrot.w()));
   AppConfig::PrintDump(PDNodeStateDetails,avar("  arbitrary scale factor:  x=%3.5f, y=%3.5f, z=%3.5f\r\n",scale.x(),scale.y(),scale.z()));

   // all added, add separator to dump file...
   AppConfig::PrintDump(PDNodeStates|PDNodeStateDetails,"---------------------------------\r\n");
}

S32  ShapeMimic::addName(const char * name, Shape * shape)
{
   tweakName(&name);
   return shape->addName(name);
}


//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateSubtrees(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // this should already have been caught, but...
   if (subtrees.empty())
   {
      setExportError("Nothing to export.");
      return;
   }

   // generate a set of nodes for each subtree
   for (S32 i=0; i<subtrees.size(); i++)
   {
      Subtree * subtree = subtrees[i];
      subtree->start.number = -1;  // translates to NULL...
                                   // this means branches will have no parent
      NodeMimic * curNode = subtree->start.child;

      // mark the beginning of the subshape
      
      shape->subshapes.push_back(Subshape());
      shape->subshapes.back().firstNode = shape->nodes.size();

      // traverse depth first
      while (curNode)
      {
         curNode->number = shape->nodes.size();

         // add node to shape
         shape->nodes.push_back(Node());
         Node & tsnode = shape->nodes.back();

         tsnode.name = addName(curNode->appNode->getName(),shape);
         tsnode.parent = curNode->parent->number;

         // set up ShapeMimic::Object with the right ts node index
         for (S32 j=0;j<curNode->objects.size(); j++)
            curNode->objects[j]->tsNodeIndex = curNode->number;

         // add NodeMimic to nodes list to keep track of order of nodes in shape
         nodes.push_back(curNode);

         // figure out where to go next
         curNode = findNextNode(curNode);
      }
   }
}

NodeMimic * ShapeMimic::findNextNode(NodeMimic * cur)
{
   if (cur->child)
      return cur->child;
   if (cur->sibling)
      return cur->sibling;
   while (cur->parent)
   {
      if (cur->parent->sibling)
         return cur->parent->sibling;
      cur = cur->parent;
   }
   // done...
   return NULL;
}

void ShapeMimic::collapseTransforms()
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDPass3,"\r\nThird pass:  Collapsing unneeded nodes...\r\n\r\n");

   cutNodes.clear();
   cutNodesParents.clear();

   Subtree * subtree;
   for (S32 i=0; i<subtrees.size(); i++)
   {
      subtree = subtrees[i];
      NodeMimic * mimicNode = subtree->start.child;
      while (mimicNode)
      {
         if (mimicNode==&subtree->start)
         {
            // this should just never happen...
            setExportError("Assertion failed:  Illegal condition.");
            return;
         }

         // figure out where to go next now
         // because we might end up cutting
         // out the current node
         NodeMimic * nextNode = findNextNode(mimicNode);

         // cut?
         if (cut(mimicNode))
         {
            AppConfig::PrintDump(PDPass3,avar("Removing node \"%s\"\r\n",mimicNode->appNode->getName()));
            snip(mimicNode);
         }
              
         mimicNode = nextNode;
      }
   }
}

bool ShapeMimic::cut(NodeMimic * mimicNode)
{
   const char * name = mimicNode->appNode->getName();

   // search always export list
   if (AppConfig::AlwaysExport(mimicNode->appNode))
      return false;

   // search never export list
   if (AppConfig::NeverExport(mimicNode->appNode))
      return true;

   // if transform collapse is false, only collapse explicitly named nodes (in neverExport list)
   if (!AppConfig::GetAllowCollapse())
      return false;
        
   // not in either list -- cut if no object and not dummy
   return (mimicNode->objects.empty() && !mimicNode->appNode->isDummy());
}

void ShapeMimic::snip(NodeMimic * nodeMimic)
{
   // if nodeMimic has a mesh, we want to make sure there is no animation between it
   // and it's parent...can't do that yet (no sequence) so add to a list and check later
   if (!nodeMimic->objects.empty())
   {
      cutNodes.push_back(nodeMimic->appNode);
      cutNodesParents.push_back(nodeMimic->parent->appNode);
   }

   // get rid of nodeMimic, bring children up to nodeMimic level
   // they will sit between nodeMimic's siblings

   // go to parent, find me
   // point parent->child or prev-sibling->sibling pointer to my child
   NodeMimic * parent = nodeMimic->parent;
   NodeMimic * n;
   if (parent->child == nodeMimic)
      parent->child = nodeMimic->child ? nodeMimic->child : nodeMimic->sibling;
   else
   {
      n = parent->child;
      while (n->sibling != nodeMimic)
         n = n->sibling;
      n->sibling = nodeMimic->child ? nodeMimic->child : nodeMimic->sibling;
   }

   // point my children's parent pointer to my parent
   n = nodeMimic->child;
   if (n)
   {
     n->parent = nodeMimic->parent;
     while (n->sibling)
     {
        n = n->sibling;
        n->parent = nodeMimic->parent;
     }
     // point my last child's sibling point to my sibling pointer
     n->sibling = nodeMimic->sibling;
   }

   // attach my objects to my parent
   for (S32 i=0; i<nodeMimic->objects.size(); i++)
   {
      parent->objects.push_back(nodeMimic->objects[i]);
      parent->objects.back()->appTSParent = parent->appNode;
   }

   delete nodeMimic;
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateObjects(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;
   
   S32 i,j;
   
   // index into shapes meshes
   S32 nextMeshIndex = 0;

   // set object priority -- requires running through all the faces of
   // all the meshes and extracting the materials...
   setObjectPriorities(objectList);
   
   // sort the objectList by subTree and priority
   sortObjectList(objectList);

   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;
   
   // initialize array that indexes first object in subshape
   for (i=0; i<subtrees.size(); i++)
      shape->subshapes[i].firstObject = -1;

   // reserve enough memory for all the possible meshes
   // so that we don't shift in memory while building vector
   S32 maxmeshes = shape->detailLevels.size() * objectList.size();
   shape->meshes.reserve(maxmeshes);

   // go through mesh list and add objects as we go
   for (i=0; i<objectList.size(); i++)
   {
      // if already encountered an error, then
      // we'll just go through the motions
      if (isError()) return;
   
      ObjectMimic * object = objectList[i];
      if (!AppConfig::GetAllowUnusedMeshes() && !object->validDetails)
      {
         setExportError(avar("Mesh \"%s\" not hooked up to shape.",object->fullName));
         return;
      }

      // we may have cut out our actual parent...if so, we need to update object offset
      if (object->appParent != object->appTSParent && object->isBone)
      {
         // trying to cut out a bone node ... not allowed
         setExportError(avar("Cannot collapse node \"%s\" because it is a bone.",object->appParent->getName()));
         return;
      }

      // if object not in shape, skip it
      if (!object->validDetails || object->isBone)
      {
         // don't need it, don't want it
         delete object;
         delElement(objectList,i);
         i--;
         continue;
      }

      std::vector<S32> * validDetails = object->validDetails;
      shape->objects.push_back(Object());
      Object & tsobj = shape->objects.back();
      tsobj.name = addName(object->name,shape);
      tsobj.numMeshes = validDetails->size();
      tsobj.firstMesh = shape->meshes.size();
      tsobj.node = object->tsNodeIndex;

      // is this the first object for this subshape...
      if (shape->subshapes[object->subtreeNum].firstObject == -1)
         shape->subshapes[object->subtreeNum].firstObject = shape->objects.size()-1;

      S32 k,prevk = -1;
      for (j=0; j<object->numDetails; j++)
      {
         for (k=0; k<validDetails->size(); k++)
            if ((*validDetails)[k]==object->details[j].size)
               break;
         if (k==validDetails->size() && !AppConfig::GetAllowUnusedMeshes())
         {
            // ooh, this mesh is an invalid detail size
            setExportError(avar("Mesh \"%s\" was found with invalid detail (%i)",object->name,object->details[j].size));
            return;
         }

         // if this is an invalid detail size get rid of it here
         if (k==validDetails->size())
         {
            delete object->details[j].mesh;
            for (k=j;k+1<object->numDetails;k++)
               object->details[k]=object->details[k+1];
            object->numDetails--;
            j--;
            continue;
         }

         // add NULL meshes for all the unused detail levels
         for (S32 l=prevk+1; l<k; l++)
            shape->meshes.push_back(Mesh(Mesh::T_Null));
         prevk=k;

         // fill in some data for later use
         object->details[j].mesh->meshNum = shape->meshes.size();
         if (object->details[j].mesh->skinMimic)
            object->details[j].mesh->skinMimic->meshNum = shape->meshes.size();

         // now hook up this mesh...
         if (object->details[j].mesh->sortedObject)
            shape->meshes.push_back(Mesh(Mesh::T_Sorted));
         else if (object->details[j].mesh->skinMimic)
            shape->meshes.push_back(Mesh(Mesh::T_Skin));
         else
            shape->meshes.push_back(Mesh(Mesh::T_Standard));
         shape->meshes.back().numFrames = 0;
         shape->meshes.back().matFrames = 0;
         if (object->details[j].mesh->billboard)
         {
            shape->meshes.back().setFlag(Mesh::Billboard);
            if (object->details[j].mesh->appMesh->isBillboardZAxis())
               shape->meshes.back().setFlag(Mesh::BillboardZ);
         }
         // get address of tsMesh -- note: important that mesh
         // vector doesn't shift in memory (see above for how we
         // make sure this doesn't happen).
         object->details[j].mesh->tsMesh = &shape->meshes.back();
      }

      // may have rid ourselves of all the meshes above...
      // if so, delete this object and continue
      if (object->numDetails==0)
      {
         delete object;
         delElement(objectList,i);
         shape->objects.pop_back();
         i--;
         continue;
      }
      // for any remaining null meshes, decrement object count
      for (j=prevk+1; j<validDetails->size(); j++)
         tsobj.numMeshes--;
   }

   // some subtrees may not have objects on them...
   // make sure subShapeFirstObject array is valid
   S32 prev = shape->objects.size();
   for (i=subtrees.size()-1; i>=0; i--)
   {
      if (shape->subshapes[i].firstObject == -1)
         shape->subshapes[i].firstObject = prev;
      prev = shape->subshapes[i].firstObject;
   }

   // now that the address of all the tsobjects are
   // set (since no longer incrementing objects vector)
   // set up pointers back to these objects from the
   // ShapeMimic versions...
   S32 tsObjIndex = 0;
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * obj = objectList[i];

      // if object not in shape, skip it as we did above
      if (!obj->validDetails)
         obj->tsObject = NULL;
      else
      {
         obj->tsObjectIndex = tsObjIndex;
         obj->tsObject = &shape->objects[tsObjIndex++];
      }
   }
}

void ShapeMimic::setObjectPriorities(std::vector<ObjectMimic*> & objects)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i,j;
   for (i=0; i<objects.size(); i++)
   {
   
      ObjectMimic * om = objects[i];
      
      // assign priority as follows:
      // 1. if it has translucency, set the highest 2 bits so it will occur after all non-translucent items
      //    but if it is also a sortObject, only set the highest bit so that it will occur before other
      //    translucent objects
      // 2. if it has exactly 1 material set next bit and put material index into next 14 bits
      //    so all objects that use exactly that material occur consecutively
      // 3. put tsnodeIndex into final 16 bits so objects hanging off same node will be consecutive,
      //    all else (1&2) being equal
      om->priority = (U32) om->tsNodeIndex;
      
      // use highest detail mesh we can find...
      for (j=0;j<om->numDetails;j++)
         if (om->details[j].mesh)
            break;
      if (j==om->numDetails)
         // not sure what this would mean, but don't want to crash
         continue;

      // if bone object, set priority to 0
      if (om->isBone)
      {
         om->priority = 0;
         continue;
      }

      AppMesh * appMesh = om->details[j].mesh->appMesh;
      if (!appMesh)
         // not sure what this would mean, but don't want to crash
         continue;

      bool hasTranslucent = false;
      bool hasMultiple = false;
      bool isSortObject = om->details[j].mesh->sortedObject;
      S32 matIndex = -1;
      AppMeshLock lock = appMesh->lockMesh(AppTime::DefaultTime(),Matrix<4,4,F32>::identity());
      for (j=0; j<appMesh->getNumFaces(); j++)
      {
         // add material for face j
         S32 mi = addFaceMaterial(appMesh,j);

         // if already encountered an error, then
         // we'll just go through the motions
         if (isError()) return;
    
         if (mi==-1 || mi==matIndex)
            continue;
         if (matIndex!=-1)
            hasMultiple=true;
         matIndex = mi;
         if (!(matIndex & Primitive::NoMaterial) && (materials[matIndex].flags & Material::Translucent))
            hasTranslucent = true;
      }

      if (hasTranslucent && !isSortObject)
         om->priority |= 3 << 30;
      else if (hasTranslucent && isSortObject)
         om->priority |= 2 << 30;
      if (!hasMultiple)
      {
         om->priority |= 1 << 29;
         om->priority |= (matIndex & 0x0FFF) << 16;
      }
   }
}

S32 __cdecl compareObjectMimics( void const *e1, void const *e2 )
{
   const ObjectMimic * om1 = *(const ObjectMimic**)e1;
   const ObjectMimic * om2 = *(const ObjectMimic**)e2;

   if (om1->subtreeNum < om2->subtreeNum)
      return -1;
   else if (om1->priority < om2->priority)
      return -1;
   else if (om2->subtreeNum < om1->subtreeNum)
      return 1;
   else if (om2->priority < om1->priority)
      return 1;
   else
      return 0;   
}

void ShapeMimic::sortObjectList(std::vector<ObjectMimic*> & olist)
{
   if (olist.size())
      qsort(&olist[0],olist.size(),sizeof(ObjectMimic*),compareObjectMimics);
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateDefaultStates(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   U32 i;

   AppConfig::PrintDump(PDObjectStates,"\r\nAdd default object states...\r\n\r\n");

   // visit all the objects in order
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * obj = objectList[i];

      // if object not in shape, skip it
      if (!obj->validDetails)
         continue;

      generateObjectState(obj,AppTime::DefaultTime(),shape,true,true);
   }

   AppConfig::PrintDump(PDNodeStates,"\r\nAdd default node states...\r\n\r\n");

   // iterate through the nodes
   for (i=0; i<nodes.size(); i++)
   {
      NodeMimic * curNode = nodes[i];
      Quaternion rot;
      Point3D trans;
      Quaternion srot; // won't matter
      Point3D scale; // should be uniform

      // create affine decomposition for node+parent at default time
      decomp_affine(curNode->appNode->getNodeTransform(AppTime::DefaultTime()),&curNode->child0);
      if (curNode->parent && curNode->parent->appNode)
         decomp_affine(curNode->parent->appNode->getNodeTransform(AppTime::DefaultTime()),&curNode->parent0);
      else
      {
         Matrix<4,4,F32> m = Matrix<4,4,F32>::identity();
         decomp_affine(m,&curNode->parent0);
      }

      generateNodeTransform(curNode,AppTime::DefaultTime(),false,AppTime::DefaultTime(),rot,trans,srot,scale);
      addNodeRotation(curNode,AppTime::DefaultTime(),shape,false,rot,true);
      addNodeTranslation(curNode,AppTime::DefaultTime(),shape,false,trans,true);
      if (!isEqual(scale,Point3D(1,1,1),0.01f))
      {
         setExportError("Assertion failed: scale on default transform");
         return;
      }
   }
}

void ShapeMimic::generateObjectState(ObjectMimic * om, const AppTime & time, Shape * shape, bool addFrame, bool addMatFrame)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDObjectStates,avar("Adding object state to %i detail level(s) of mesh \"%s\".\r\n",om->numDetails,om->name));
   if (addFrame)
      AppConfig::PrintDump(PDObjectStates,"Adding frame.\r\n");

   shape->objectStates.push_back(ObjectState());
   ObjectState & os = shape->objectStates.back();
    
   os.frame = 0;
   os.matFrame = 0;
   os.vis = om->inTreeMesh ? om->inTreeMesh->getVisValue(time) : 1.0f;
   if (os.vis < 0.0f)
      os.vis = 0.0f;
   else if (os.vis > 1.0f)
      os.vis = 1.0f;

   if (os.vis < 0.01f || os.vis > 0.99f)
      AppConfig::PrintDump(PDObjectStateDetails,avar("Object is%svisible.\r\n",os.vis>0.5f ? " " : " not "));
   else
      AppConfig::PrintDump(PDObjectStateDetails,avar("Object visibility = %5.3f out of 1.\r\n",os.vis));

   if (addFrame || addMatFrame)
   {
      generateFrame(om,time,addFrame,addMatFrame);

      // must have highest detail level...
      if (!om->details[0].mesh->tsMesh || om->details[0].mesh->tsMesh->getType() == Mesh::T_Null)
      {
         setExportError(avar("Missing highest detail level on mesh \"%s\".",om->name));
         return;
      }

      // set the frame number for the object state
      os.frame = om->details[0].mesh->tsMesh->numFrames - 1;
      os.matFrame = om->details[0].mesh->tsMesh->matFrames - 1;
      if (os.frame<0)
         os.frame=0;
      if (os.matFrame<0)
         os.matFrame=0;
   }

   // all added, add separator to dump file...
   AppConfig::PrintDump(PDObjectStates|PDObjectStateDetails,"---------------------------------\r\n");
}

void ShapeMimic::generateFrame(ObjectMimic * om, const AppTime & time, bool addFrame, bool addMatFrame)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (om->isBone)
   {
      setExportError("Assertion failed: bone should no longer be on node");
      return;
   }
   if (om->isSkin)
      // don't generate frame
      return;

   S32 i,dl;
   for (dl=0; dl<om->numDetails; dl++)
   {
      Mesh * tsMesh  = om->details[dl].mesh->tsMesh;
      AppMesh * appMesh = om->details[dl].mesh->appMesh;
      Matrix<4,4,F32> & objectOffset = om->details[dl].mesh->objectOffset;

      // if first frame then compute object offset
      if (tsMesh->numFrames==0)
      {
         // compute object offset -- need to compute offset vs. node in shape
         // tree, not node the mesh hangs off of in the app 
         // Also note, node transform will have scale stripped off in shape,
         // so we zapScale before accounting for that transform.

         Matrix<4,4,F32> meshMat = appMesh->getMeshTransform(AppTime::DefaultTime());
         Matrix<4,4,F32> nodeMat = om->inTreeNode->getNodeTransform(AppTime::DefaultTime());
         zapScale(nodeMat);
         objectOffset = nodeMat.inverse() * meshMat;

         // print out object offsets?
         if (AppConfig::GetDumpMask() & PDObjectOffsets)
         {
            AffineParts parts;
            decomp_affine(objectOffset,&parts);
            AppConfig::PrintDump(PDObjectOffsets,avar("Object offset transform for mesh dl=%i:\r\n",dl));
            AppConfig::PrintDump(PDObjectOffsets,avar("    scale:            x=%3.5f, y=%3.5f, z=%3.5f\r\n",parts.scale.x(),parts.scale.y(),parts.scale.z()));
            AppConfig::PrintDump(PDObjectOffsets,avar("    stretch rotation: x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",parts.scaleRot.x(),parts.scaleRot.y(),parts.scaleRot.z(),parts.scaleRot.w()));
            AppConfig::PrintDump(PDObjectOffsets,avar("    translation:      x=%3.5f, y=%3.5f, z=%3.5f\r\n",parts.trans.x(),parts.trans.y(),parts.trans.z()));
            AppConfig::PrintDump(PDObjectOffsets,avar("    actual rotation:  x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",parts.rot.x(),parts.rot.y(),parts.rot.z(),parts.rot.w()));
            if (parts.sign<0)
                AppConfig::PrintDump(PDObjectOffsets,     "    ---determinant negative---\r\n");
         }
      }

      std::vector<Primitive> faces;
      std::vector<Point3D> verts;
      std::vector<Point3D> norms;
      std::vector<Point2D> tverts;
      std::vector<U16> indices;
      std::vector<U32> smooth;
      std::vector<U32> vertId;

      AppMeshLock lock = appMesh->lockMesh(time,objectOffset);

      appMesh->generateFaces(faces,verts,tverts,indices,smooth,norms,&vertId);

      if (tsMesh->numFrames==0)
      {
         // first frame, copy faces into mesh
         tsMesh->primitives     = faces;
         tsMesh->indices        = indices;
         tsMesh->vertsPerFrame  =  verts.size();
         om->details[dl].mesh->numVerts = verts.size();
         om->details[dl].mesh->smoothingGroups = smooth;
         om->details[dl].mesh->vertId = vertId;

         // make sure all the materials are added
         for (S32 j=0; j<appMesh->getNumFaces(); j++)
         {
            // add material for face j
            S32 mi = addFaceMaterial(appMesh,j);

            // replace appmesh material index with ts material index
            tsMesh->primitives[j].type &= ~Primitive::MaterialMask;
            if (mi<0)
               tsMesh->primitives[j].type |= Primitive::NoMaterial;
            else
               tsMesh->primitives[j].type |= mi;

            // if already encountered an error, then
            // we'll just go through the motions
            if (isError())
               return;
         }
      }
      else
      {
         // not first frame, make sure topology of this frame is same as first frame
         // NOTE:  if multi-frame then can't be multi-res...
         bool error = false;
         if (vertId.size()==om->details[dl].mesh->vertId.size())
         {
            for (i=0;i<vertId.size();i++)
               if (vertId[i]!=om->details[dl].mesh->vertId[i])
                  break;
            if (i!=vertId.size())
               error=true;
         }
         else
            error=true;
         for (i=0; i<faces.size(); i++)
         {
            if (tsMesh->primitives.size() != faces.size())
               // only need to check this once really, but code simpler this way
               break;
            if (tsMesh->indices.size() != indices.size())
               // different number of indices/verts -- should hit prior break in that case...
               break;
            // make sure tsMesh->face[i] is same as face[i]
            if (tsMesh->primitives[i].firstElement!=faces[i].firstElement ||
                tsMesh->primitives[i].type!=faces[i].type)
               break;
         }
         if (i!=faces.size() || error)
         {
            setExportError(avar("Mesh topology is animated on mesh \"%s\".",appMesh->getName()));
            return;
         }
      }

      if (addFrame)
      {
         // copy verts...
         for (i=0; i<verts.size(); i++)
            tsMesh->verts.push_back(verts[i]);
         // copy normals...
         for (i=0; i<verts.size(); i++)
         {
            if (norms.size())
               tsMesh->normals.push_back(norms[i]);
            else
               // not important what...gets overwritten later
               tsMesh->normals.push_back(Point3D(0,0,1));
         }

         tsMesh->numFrames++;
      }
      
      if (addMatFrame)
      {
         // copy tverts...
         for (i=0; i<tverts.size(); i++)
            tsMesh->tverts.push_back(tverts[i]);

         tsMesh->matFrames++;
      }
   }
}

void ShapeMimic::generateNodeTransform(NodeMimic * curNode, const AppTime & time,
                                       bool blend, const AppTime & blendReferenceTime,
                                       Quaternion & rot, Point3D & trans, Quaternion & qrot, Point3D & scale)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (blend)
      getBlendNodeTransform(curNode->appNode,curNode->parent->appNode,curNode->child0,curNode->parent0,time,blendReferenceTime,rot,trans,qrot,scale);
   else
      getLocalNodeTransform(curNode->appNode,curNode->parent->appNode,curNode->child0,curNode->parent0,time,rot,trans,qrot,scale);
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateIflMaterials(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // if none to make...
   if (iflList.empty())
      return;

   AppConfig::PrintDump(PDSequences,avar("\r\nAdding %i ifl materials...\r\n\r\n",iflList.size()));

   for (S32 i=0; i<iflList.size(); i++)
   {
      shape->IFLmaterials.push_back(IFLMaterial());
      IFLMaterial & iflMaterial = shape->IFLmaterials.back();
      
      iflMaterial.name = addName(getFileBase(iflList[i]->appIfl->getFilename()),shape);
      iflMaterial.slot = iflList[i]->materialSlot;

      AppConfig::PrintDump(PDSequences,avar("Adding ifl material \"%s\".\r\n",iflList[i]->appIfl->getFilename()));
   }
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateSequences(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   AppConfig::PrintDump(PDSequences,avar("\r\nAdding %i sequences...\r\n\r\n",sequences.size()));

   for (S32 i=0; i<sequences.size(); i++)
   {
      AppSequence * appSeq = sequences[i];
      if (appSeq==NULL)
      {
         setExportError("Assertion failed.  Null sequence");
         return;
      }

      AppSequenceData seqData;
      appSeq->getSequenceData(&seqData);

      shape->sequences.push_back(Sequence());
      Sequence & seq = shape->sequences.back();
      constructInPlace(&seq);

      const char * name = appSeq->getName();
      seq.nameIndex = addName(name,shape);
      AppConfig::PrintDump(PDSequences,avar("Adding sequence %i named \"%s\"\r\n",i,name));
      appSeq->setTSSequence(&seq);

      // determine which nodes/objects are controlled by this sequence
      S32 rotCount, transCount, uniformScaleCount, alignedScaleCount, arbitraryScaleCount, objectCount, iflCount;
      setNodeMembership(shape,seq,seqData,rotCount,transCount,uniformScaleCount,alignedScaleCount,arbitraryScaleCount);
      setObjectMembership(shape,seq,seqData,objectCount);
      setIflMembership(shape,seq,seqData,iflCount);

      S32 scaleCount = getmax(uniformScaleCount,getmax(alignedScaleCount,arbitraryScaleCount));
      S32 nodeCount = getmax(rotCount,getmax(transCount,scaleCount));
      const char * scaleType;
      if (arbitraryScaleCount)
         scaleType=" (arbitary scale)";
      else if (alignedScaleCount)
         scaleType=" (aligned scale)";
      else if (uniformScaleCount)
         scaleType=" (uniform scale)";
      else
         scaleType="";

      if (isError()) return;

      // supply some dump information
      if (!seqData.cyclic)
         AppConfig::PrintDump(PDSequences,"One-shot sequence.  ");
      if (seqData.blend)
         AppConfig::PrintDump(PDSequences,"Blend sequence.  ");
      AppConfig::PrintDump(PDSequences,avar("Enabled animation: %c%c%c%c%c%c%c%c%c, Forced animation: %c%c%c, Default priority = %i.\r\n",
                seqData.enableMorph      ? 'M' : ' ',
                seqData.enableVis        ? 'V' : ' ',
                seqData.enableTransform  ? 'T' : ' ',
                seqData.enableUniformScale  ? 'U' : ' ',
                seqData.enableArbitraryScale ? 'A' : ' ',
                seqData.enableIFL        ? 'I' : ' ',
                seqData.forceMorph       ? 'M' : ' ',
                seqData.forceVis         ? 'V' : ' ',
                seqData.forceTransform   ? 'T' : ' ',
                seqData.forceScale       ? 'S' : ' ',
                seqData.priority));
      if (seqData.ignoreGround)
         AppConfig::PrintDump(PDSequences,"Ignoring ground transform.\r\n");
      AppConfig::PrintDump(PDSequences,avar("Duration = %3.5f, secPerFrame = %3.5f, # frames = %i\r\n",seqData.duration.getF32(),seqData.delta.getF32(),seqData.numFrames));
      AppConfig::PrintDump(PDSequences,avar("Sequence includes %i nodes, %i objects, and %i ifl materials\r\n",nodeCount,objectCount,iflCount));
      AppConfig::PrintDump(PDSequences,avar("  %i rotations, %i translations, %i scales%s\r\n\r\n",rotCount,transCount,scaleCount,scaleType));

      // these methods generate the animation data...each of
      // them operates on the most recently created sequence
      generateNodeAnimation(shape,seq,seqData);
      generateObjectAnimation(shape,seq,seqData);
      generateGroundAnimation(shape,seq,seqData);
      generateFrameTriggers(shape,seq,seqData,appSeq);

      if (testCutNodes(seqData))
         return;
   }
}

S32 ShapeMimic::setObjectMembership(Shape * shape, Sequence & seq, AppSequenceData & seqData, S32 & objectCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   // clear out all object membership...
   setMembershipArray(seq.matters.vis,seqData.forceVis,0,objectList.size());
   setMembershipArray(seq.matters.frame,seqData.forceMorph,0,objectList.size());
   setMembershipArray(seq.matters.matframe,seqData.forceTVert,0,objectList.size());

   // don't do any work we don't have to...
   bool doVis = seqData.enableVis && !seqData.forceVis;
   bool doMorph = seqData.enableMorph && !seqData.forceMorph;
   bool doTVert = seqData.enableTVert && !seqData.forceTVert;

   // add objects to the subsets...
   for (S32 i=0; i<objectList.size(); i++)
   {
      // the node to perform membership tests on
      AppMesh * testMesh = objectList[i]->inTreeMesh;

      if (objectList[i]->isSkin)
      {
         // in case force was set
         setMembershipArray(seq.matters.frame,false,i);
         setMembershipArray(seq.matters.matframe,false,i);
         testMesh = objectList[i]->getSkin();
         if (!testMesh)
            continue;
      }

      if (doVis && testMesh->animatesVis(seqData))
         setMembershipArray(seq.matters.vis,true,i);

      if (objectList[i]->isSkin)
         continue;

      if (doTVert && testMesh->animatesMatFrame(seqData))
         setMembershipArray(seq.matters.matframe,true,i);

      if (doMorph && testMesh->animatesFrame(seqData))
         setMembershipArray(seq.matters.frame,true,i);
   }

   // how many objects are in the set?
   objectCount=0;
   for (S32 j=0; j<objectList.size(); j++)
      if (seq.matters.frame[j] ||
          seq.matters.matframe[j] ||
          seq.matters.vis[j])
         objectCount++;

   return objectCount;
}

void ShapeMimic::setNodeMembership(Shape * shape, Sequence & seq, AppSequenceData & seqData,
                                   S32 & rotCount, S32 & transCount, 
                                   S32 & uniformScaleCount, S32 & alignedScaleCount, S32 & arbitraryScaleCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   rotCount = transCount = uniformScaleCount = alignedScaleCount = arbitraryScaleCount = 0;
   if (isError()) return;

   // decide node membership
   S32 skipScaleCount = 0;
   S32 skipTransCount = 0;
   setMembershipArray(seq.matters.rotation,seqData.forceTransform,0,nodes.size());
   setMembershipArray(seq.matters.translation,seqData.forceTransform,0,nodes.size());
   setMembershipArray(seq.matters.scale,seqData.forceScale,0,nodes.size());
   if (seqData.forceTransform || seqData.forceScale)
   {
      // clear nodes that are set not to animate
      for (S32 i=0; i<nodes.size(); i++)
      {
         if (AppConfig::NeverAnimate(nodes[i]->appNode))
         {
            if (seqData.forceTransform)
            {
               setMembershipArray(seq.matters.rotation,false,i);
               setMembershipArray(seq.matters.translation,false,i);
               skipTransCount++;
            }
            if (seqData.forceScale)
            {
               setMembershipArray(seq.matters.scale,false,i);
               skipScaleCount++;
            }
         }
      }
      return;
   }

   if (!seqData.enableTransform && !seqData.enableUniformScale && !seqData.enableArbitraryScale)
      // not animating transforms, so no nodes are members
      return;

   // this shouldn't be allowed, but check anyway...
   if (seqData.numFrames<2)
      return;

   // Note: this fills the cache with current sequence data.
   //       Methods that get called later (e.g.,
   //       generateNodeAnimation) use this info (and assume it's set).
   fillNodeTransformCache(nodes,seq,seqData);

   // test to see if the transform changes over the interval
   // in order to decide whether to animate the transform in 3space
   // we don't use app's mechanism for doing this because it functions
   // different in different apps and we do some special stuff with scale.

   setRotationMembership(shape,seq,seqData,rotCount);
   setTranslationMembership(shape,seq,seqData,transCount);
   setScaleMembership(seq,seqData,arbitraryScaleCount,alignedScaleCount,uniformScaleCount);
   
   // adjust counts by non-rotating nodes (in case of force transform)
   // add scale flags to sequence
   rotCount -= skipTransCount;
   transCount -= skipTransCount;
   if (arbitraryScaleCount)
   {
      arbitraryScaleCount -= skipScaleCount;
      seq.flags |= Sequence::ArbitraryScale;
   }
   if (alignedScaleCount)
   {
      alignedScaleCount -= skipScaleCount;
      seq.flags |= Sequence::AlignedScale;
   }
   if (uniformScaleCount)
   {
      uniformScaleCount -= skipScaleCount;
      seq.flags |= Sequence::UniformScale;
   }
}

void ShapeMimic::setRotationMembership(Shape * shape, Sequence & seq, AppSequenceData & seqData, S32 & rotCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   rotCount = 0;
   if (isError()) return;

   if (seqData.forceTransform || !seqData.enableTransform)
   {
      rotCount = !seqData.enableTransform ? 0 : nodes.size();
      return;
   }

   rotCount = 0;
   for (S32 i=0; i<nodes.size(); i++)
   {
      if (AppConfig::NeverAnimate(nodes[i]->appNode))
         continue;

      // first rotation
      Quaternion * firstRot = &nodeRotCache[i][0];
      Quaternion * prevRot = firstRot;
      Quaternion & defaultRot = shape->nodeDefRotations[i];

      if (!(*firstRot==defaultRot))
      {
         setMembershipArray(seq.matters.rotation,true,i);
         rotCount++;
         continue;
      }

      for (S32 frame=1; frame<seqData.numFrames; frame++)
      {
         Quaternion * curRot = &nodeRotCache[i][frame];
         if (!(*curRot==*prevRot) || !(*curRot==*firstRot))
         {
            setMembershipArray(seq.matters.rotation,true,i);
            rotCount++;
            break;
         }
         prevRot = curRot;
      }
   }
}

void ShapeMimic::setTranslationMembership(Shape * shape, Sequence & seq, AppSequenceData & seqData, S32 & transCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   transCount = 0;
   if (isError()) return;

   if (seqData.forceTransform || !seqData.enableTransform)
   {
      transCount = !seqData.enableTransform ? 0 : nodes.size();
      return;
   }

   transCount = 0;
   for (S32 i=0; i<nodes.size(); i++)
   {
      if (AppConfig::NeverAnimate(nodes[i]->appNode))
         continue;

      // first rotation
      Point3D * firstTrans = &nodeTransCache[i][0];
      Point3D * prevTrans = firstTrans;
      Point3D & defaultTrans = shape->nodeDefTranslations[i];

      if (!isEqual(*firstTrans,defaultTrans,0.001f))
      {
         setMembershipArray(seq.matters.translation,true,i);
         transCount++;
         continue;
      }

      for (S32 frame=1; frame<seqData.numFrames; frame++)
      {
         Point3D * curTrans = &nodeTransCache[i][frame];
         if (!isEqual(*curTrans,*prevTrans,AppConfig::AnimationDelta()) || !isEqual(*curTrans,*firstTrans,AppConfig::AnimationDelta()))
         {
            setMembershipArray(seq.matters.translation,true,i);
            transCount++;
            break;
         }
         prevTrans = curTrans;
      }
   }
}

void ShapeMimic::setScaleMembership(Sequence & seq, AppSequenceData & seqData, S32 & arbitraryScaleCount, S32 & alignedScaleCount, S32 & uniformScaleCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   arbitraryScaleCount = alignedScaleCount = uniformScaleCount = 0;
   if (isError()) return;

   S32 scaleCount = 0;
   if (seqData.forceScale)
      scaleCount = nodes.size();
   else if (!seqData.enableScale)
      return;

   if (animatesArbitraryScale(seqData))
      arbitraryScaleCount = scaleCount ? scaleCount : setArbitraryScaleMembership(seq,seqData);
   else if (animatesAlignedScale(seqData))
      alignedScaleCount = scaleCount ? scaleCount : setAlignedScaleMembership(seq,seqData);
   else
      uniformScaleCount = scaleCount ? scaleCount : setUniformScaleMembership(seq,seqData);
}

S32 ShapeMimic::setUniformScaleMembership(Sequence & seq, AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   S32 nodeCount = 0;
   for (S32 i=0; i<nodes.size(); i++)
   {
      if (AppConfig::NeverAnimate(nodes[i]->appNode))
         continue;

      // first rotation
      Point3D a = nodeScaleCache[i][0];
      F32 firstScale = (a.x()+a.y()+a.z())/3.0f;
      F32 prevScale = firstScale;

      if (fabs(firstScale-1.0f)>0.001f)
      {
         setMembershipArray(seq.matters.scale,true,i);
         nodeCount++;
         continue;
      }

      for (S32 frame=1; frame<seqData.numFrames; frame++)
      {
         Point3D a = nodeScaleCache[i][frame];
         F32 curScale = (a.x()+a.y()+a.z())/3.0f;
         if (fabs(curScale-prevScale)>0.001f)
         {
            setMembershipArray(seq.matters.scale,true,i);
            nodeCount++;
            break;
         }
         prevScale = curScale;
      }
   }

   return nodeCount;
}

S32 ShapeMimic::setAlignedScaleMembership(Sequence & seq, AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return 0;

   S32 nodeCount = 0;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (AppConfig::NeverAnimate(nodes[i]->appNode))
         continue;

      // first rotation
      Point3D * firstScale = &nodeScaleCache[i][0];
      Point3D * prevScale = firstScale;

      if (!isEqual(*firstScale,Point3D(1,1,1),AppConfig::AnimationDelta()))
      {
         setMembershipArray(seq.matters.scale,true,i);
         nodeCount++;
         continue;
      }

      for (S32 frame=1; frame<seqData.numFrames; frame++)
      {
         Point3D * curScale = &nodeScaleCache[i][frame];
         if (!isEqual(*curScale,*prevScale,AppConfig::AnimationDelta()) || !isEqual(*curScale,*firstScale,AppConfig::AnimationDelta()))
         {
            setMembershipArray(seq.matters.scale,true,i);
            nodeCount++;
            break;
         }
         prevScale = curScale;
      }
   }

   return nodeCount;
}

S32 ShapeMimic::setArbitraryScaleMembership(Sequence & seq, AppSequenceData & seqData)
{
   // for determining membership, we just care if scale factor is animated...
   return setAlignedScaleMembership(seq,seqData);
}

bool ShapeMimic::animatesAlignedScale(AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   if (!seqData.enableAlignedScale)
      return false;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (AppConfig::NeverAnimate(nodes[i]->appNode))
         continue;

      for (S32 frame=0; frame<seqData.numFrames; frame++)
      {
         Point3D delta = nodeScaleCache[i][frame] - Point3D(1,1,1);
         if (!isZero(delta,AppConfig::AnimationDelta()) && 
                    (fabs(delta.x()-delta.y())>AppConfig::AnimationDelta() ||
                     fabs(delta.y()-delta.z())>AppConfig::AnimationDelta() ||
                     fabs(delta.z()-delta.x())>AppConfig::AnimationDelta()))
            // we not only animate scale, but we do it non-uniformly
            return true;
      }
   }

   return false;
}

bool ShapeMimic::animatesArbitraryScale(AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return false;

   if (!seqData.enableArbitraryScale)
      return false;

   for (S32 i=0; i<nodes.size(); i++)
   {
      if (AppConfig::NeverAnimate(nodes[i]->appNode))
         continue;

      Quaternion idQuat(0,0,0,1);
      for (S32 frame=0; frame<seqData.numFrames; frame++)
      {
         Quaternion curRot = nodeScaleRotCache[i][frame];
         Point3D curScale = nodeScaleCache[i][frame];
         if (isEqualQ16(curRot,idQuat))
            // scale factor is aligned, not arbitrary
            continue;
         Point3D delta = curScale - Point3D(1,1,1);
         if (fabs(delta.x())<0.001f && fabs(delta.y())<0.001f && fabs(delta.z())<0.001f)
            // no scale
            continue;
         // we have scale and it isn't aligned...
         return true;
      }
   }

   return false;
}

void ShapeMimic::setIflMembership(Shape * shape, Sequence & seq, AppSequenceData & seqData, S32 & iflCount)
{
   // if already encountered an error, then
   // we'll just go through the motions
   iflCount = 0;
   if (isError()) return;

   setMembershipArray(seq.matters.ifl,false,0,iflList.size());

   // get start and end frame
   AppTime startTime = seqData.startTime;
   AppTime endTime   = seqData.endTime;

   // decide object membership
   if (seqData.enableIFL)
   {
      for (S32 i=0; i<iflList.size(); i++)
      {
         // does ifl animate any materials during our range?
         AppTime time = iflList[i]->appIfl->getStartTime();
         const std::vector<AppTime> & durations = iflList[i]->appIfl->getDurations();
         const std::vector<char*> & names = iflList[i]->appIfl->getNames();
         S32 len = names.size();
         if (durations.size() != len)
         {
            setExportError("Assertion failed: mismatch between ifl names and ifl durations");
            return;
         }
         if (len==0)
            // degenerate ifl...just leave
            continue;

         S32 idx = 0;
         const char * prev = "";
         while (time<endTime)
         {
            if (time==startTime && !stricmp(names[0],names[idx % len]))
            {
               // changing material during this sequence...
               iflCount++;
               setMembershipArray(seq.matters.ifl,true,i);
               break;
            }
            if (time>startTime && !stricmp(prev,names[idx % len]))
            {
               // changing material during this sequence...
               iflCount++;
               setMembershipArray(seq.matters.ifl,true,i);
               break;
            }
            prev = names[idx % len];
            time += durations[idx % len];
            ++idx;
         }
      }
   }
}

void ShapeMimic::generateGroundAnimation(Shape * shape, Sequence & seq, AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   seq.firstGroundFrame = shape->groundTranslations.size();
   seq.numGroundFrames = 0;

   if (seqData.ignoreGround)
      // nothing more to do
      return;

   // does this sequence animate the bounds node, if not, don't add ground transform
   if (!boundsNode->animatesTransform(seqData))
      // no ground animation
      return;

   // at this point we know that we do animate bounds node,
   // so we do have ground animation...

   S32 groundNumFrames = seqData.groundNumFrames;

   seq.flags |= Sequence::MakePath;
   seq.numGroundFrames = groundNumFrames-1; // we only really add this many frames

   AppConfig::PrintDump(PDSequences,
      avar("\r\nAdding %i ground transform frames at %3.5f sec per frame intervals.\r\n",groundNumFrames,seqData.groundDelta.getStr()));

   // frame at start isn't added since it would just be identity anyway...
   AppTime time = seqData.startTime + seqData.groundDelta;
   for (S32 i=0; i<groundNumFrames-1; i++, time += seqData.groundDelta)
   {
      shape->groundTranslations.push_back(Point3D());
      shape->groundRotations.push_back(Quaternion());
      Quaternion & rot = shape->groundRotations.back();
      Point3D & trans = shape->groundTranslations.back();
      Quaternion srot;   // ignored on ground transform
      Point3D scale; // ignored on ground transform
      getDeltaTransform(boundsNode,seqData.startTime,time,rot,trans,srot,scale);
   }
}

void ShapeMimic::generateNodeAnimation(Shape * shape, Sequence & seq, AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // add the states -- add all the states for each node in a row
   seq.baseRotation = shape->nodeRotations.size();
   seq.baseTranslation = shape->nodeTranslations.size();
   seq.baseScale = (seq.flags & Sequence::ArbitraryScale) ? shape->nodeScalesArbitrary.size() :
      (seq.flags & Sequence::AlignedScale) ? shape->nodeScalesAligned.size() : shape->nodeScalesUniform.size();
   for (S32 i=0; i<nodes.size(); i++)
   {
      AppTime time = seqData.startTime;
      for (S32 frame = 0; frame<seqData.numFrames; frame++, time += seqData.delta)
      {
         // may go just a tad over/under due to round-off...correct that here
         if (seq.matters.rotation[i])
            addNodeRotation(nodes[i],time,shape,seqData.blend,nodeRotCache[i][frame],false);
         if (seq.matters.translation[i])
            addNodeTranslation(nodes[i],time,shape,seqData.blend,nodeTransCache[i][frame],false);
         if (seq.matters.scale[i])
         {
            Quaternion & rot = nodeScaleRotCache[i][frame];
            Point3D scale = nodeScaleCache[i][frame];
            if (animatesArbitraryScale(seqData))
               addNodeArbitraryScale(nodes[i],time,shape,seqData.blend,rot,scale);
            else if (animatesAlignedScale(seqData))
               addNodeAlignedScale(nodes[i],time,shape,seqData.blend,scale);
            else
               addNodeUniformScale(nodes[i],time,shape,seqData.blend,(scale.x()+scale.y()+scale.z())/3.0f);
         }
      }
   }
}

void ShapeMimic::generateObjectAnimation(Shape * shape, Sequence & seq, AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // add the states -- add all the states for each object in a row
   seq.baseObjectState = shape->objectStates.size();
   for (S32 i=0; i<objectList.size(); i++)
   {
      if (seq.matters.frame[i] || seq.matters.matframe[i] || seq.matters.vis[i])
      {
         AppTime time = seqData.startTime;
         for (S32 frame = 0; frame<seqData.numFrames; frame++, time += seqData.delta)
            generateObjectState(objectList[i],time,shape,seq.matters.frame[i],seq.matters.matframe[i]);
      }
   }
}

void ShapeMimic::generateFrameTriggers(Shape * shape, Sequence & seq, AppSequenceData & seqData, AppSequence * appSeq)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // initialize triggers...
   seq.firstTrigger = shape->triggers.size();
   seq.numTriggers  = appSeq->getNumTriggers();

   if (!seq.numTriggers)
      // no triggers
      return;

   S32 i;
   for (i=0; i<seq.numTriggers; i++)
      shape->triggers.push_back(appSeq->getTrigger(i));

   // track the triggers that get turned off by this shape...normally, triggers
   // aren't turned on/off, just on...if we are a trigger that does both then we
   // need to mark ourselves as such so that on/off can become off/on when sequence
   // is played in reverse...
   U32 offTriggers = 0;
   for (i=0; i<seq.numTriggers; i++)
   {
      U32 state = shape->triggers[seq.firstTrigger+i].state;
      if ((state & TriggerState::StateOn) == 0)
         offTriggers |= state & (~TriggerState::StateMask);
   }

   // we now know which states are turned off, set invert on all those (including when turned on)
   for (i=0; i<seq.numTriggers; i++)
   {
      if (shape->triggers[seq.firstTrigger + i].state & offTriggers)
         shape->triggers[seq.firstTrigger + i].state |= TriggerState::InvertOnReverse;
   }

   // do a quick bubble sort so that we don't have to mess with trigger compare operators
   for (i=0; i<seq.numTriggers-1; i++)
   {
      for (S32 j=i+1; j<seq.numTriggers; j++)
      {
         if (shape->triggers[seq.firstTrigger + j].pos < shape->triggers[seq.firstTrigger + i].pos)
         {
            // swap
            Trigger tmp = shape->triggers[seq.firstTrigger + j];
            shape->triggers[seq.firstTrigger + j] = shape->triggers[seq.firstTrigger + i];
            shape->triggers[seq.firstTrigger + i] = tmp;
         }
      }
   }
}

bool ShapeMimic::testCutNodes(AppSequenceData & seqData)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return true;

   // this shouldn't be allowed, but check anyway...
   S32 numFrames = seqData.numFrames;
   if (numFrames<2)
       return false;

   S32 i, frame;

   std::vector<Quaternion> rotTrans(numFrames);
   std::vector<Point3D> transTrans(numFrames);
   std::vector<Point3D> scaleTrans(numFrames);
   std::vector<Quaternion> scaleRotTrans(numFrames);
   std::vector<AffineParts> child0(cutNodes.size());
   std::vector<AffineParts> parent0(cutNodes.size());

   Quaternion tmpRot;
   Point3D tmpTrans;
   Quaternion tmpScaleRot;
   Point3D tmpScaleTrans;
   for (i=0;i<cutNodes.size();i++)
      getLocalNodeTransform(cutNodes[i],cutNodesParents[i],child0[i],parent0[i],AppTime::DefaultTime(),tmpRot,tmpTrans,tmpScaleRot,tmpScaleTrans);

   // get the transform for each frame
   for (i=0;i<cutNodes.size();i++)
   {
      // get all the node transforms for every frame
      AppTime time = seqData.startTime;
      for (frame = 0; frame<numFrames; frame++, time += seqData.delta)
         getLocalNodeTransform(cutNodes[i],cutNodesParents[i],child0[i],parent0[i],time,rotTrans[frame],transTrans[frame],scaleRotTrans[frame],scaleTrans[frame]);

      // we now have all numFrames transforms...check to see if they change
      Quaternion * firstRot = &rotTrans[0];
      Quaternion * prevRot = firstRot;

      Point3D * firstTrans = &transTrans[0];
      Point3D * prevTrans = firstTrans;

      Quaternion * firstScaleRot = &scaleRotTrans[0];
      Quaternion * prevScaleRot = firstScaleRot;

      Point3D * firstScale = &scaleTrans[0];
      Point3D * prevScale = firstScale;

      for (frame=0; frame<numFrames; frame++)
      {
         Quaternion * curRot = &rotTrans[frame];
         Point3D * curTrans = &transTrans[frame];
         Quaternion * curScaleRot = &scaleRotTrans[frame];
         Point3D * curScale = &scaleTrans[frame];
         Point3D delta = *curTrans-*prevTrans;
         Point3D deltaScale = *curScale-*prevScale;
         bool idScale1 = isEqual(*curScale,Point3D(1,1,1),0.01f);
         bool idScale2 = isEqual(*prevScale,Point3D(1,1,1),0.01f);
         bool pureScaleDiff = isZero(deltaScale,AppConfig::AnimationDelta());
         bool isScaled = pureScaleDiff || ((!idScale1 || !idScale2));
         bool isTrans = isZero(delta,AppConfig::AnimationDelta());
         bool isRot = !isEqualQ16(*curRot,*prevRot);
         if (isRot || isTrans || isScaled)
         {
            // going to report error -- add extra information to the dump file:
            AppConfig::PrintDump(PDAlways,"\r\n----------------------------------------------\r\n");
            AppConfig::PrintDump(PDAlways,avar("\r\nIllegal transform animiation detected between collapsed node \"%s\" and \"%s\".\r\n",
                cutNodes[i]->getName(),cutNodesParents[i]->getName()));
            AppConfig::PrintDump(PDAlways,"Transform dump:\r\n\r\n");
            Point3D maxT(0,0,0);
            Quaternion maxQ(0,0,0,0);
            Point3D startT = *firstTrans;
            Quaternion startQ = *firstRot;
           
            for (S32 f=0; f<numFrames; f++)
            {
               Point3D t = transTrans[f];
               Quaternion q = rotTrans[f];
               AppConfig::PrintDump(PDAlways,avar("  translation:  x=%3.5f, y=%3.5f, z=%3.5f\r\n",t.x(),t.y(),t.z()));
               AppConfig::PrintDump(PDAlways,avar("  rotation:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",q.x(),q.y(),q.z(),q.w()));
               AppConfig::PrintDump(PDAlways,"---------------------------------\r\n");

               F32 diff;

               // get maximum difference for the dump file
               diff = fabs(t.x()-startT.x());
               if (diff > maxT.x())
                  maxT.x(diff);

               diff = fabs(t.y()-startT.y());
               if (diff > maxT.y())
                  maxT.y(diff);

               diff = fabs(t.z()-startT.z());
               if (diff > maxT.z())
                  maxT.z(diff);

               diff = fabs(q.x()-startQ.x());
               if (diff > maxQ.x())
                  maxQ.x(diff);

               diff = fabs(q.y()-startQ.y());
               if (diff > maxQ.y())
                  maxQ.y(diff);

               diff = fabs(q.z()-startQ.z());
               if (diff > maxQ.z())
                  maxQ.z(diff);

               diff = fabs(q.w()-startQ.w());
               if (diff > maxQ.w())
                  maxQ.w(diff);
            }
            AppConfig::PrintDump(PDAlways,"Maximum deviation:\r\n");
            AppConfig::PrintDump(PDAlways,avar("  translation:  x=%3.5f, y=%3.5f, z=%3.5f\r\n",maxT.x(),maxT.y(),maxT.z()));
            AppConfig::PrintDump(PDAlways,avar("  rotation:     x=%3.5f, y=%3.5f, z=%3.5f, w=%3.5f\r\n",maxQ.x(),maxQ.y(),maxQ.z(),maxQ.w()));
            AppConfig::PrintDump(PDAlways,"        Scale may have animated too.\r\n");
            AppConfig::PrintDump(PDAlways,"---------------------------------\r\n");

            setExportError(avar("Illegal transform animation detected between collapsed node \"%s\" and \"%s\".",
                           cutNodes[i]->getName(),cutNodesParents[i]->getName()));
            return true;
         }
         prevTrans = curTrans;
         prevScaleRot = curScaleRot;
         prevScale = curScale;
      }
   }
   return false;
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateMaterialList(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   shape->materials = materials;
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::generateSkins(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // put skins in the right order...we're basically sorting by detailSize member of skinMimic
   // but we'll do it a slow way to make sure in synch with detail ordering
   {
      S32 i,j,k;
      j=0;
      for (i=0; i<shape->detailLevels.size(); i++)
      {
         // search for skins of size details[i].size
         // place any found at j and advance j
         for (k=j; k<skins.size(); k++)
            if (skins[k]->detailSize == (S32) shape->detailLevels[i].size)
            {
               // swap skin j and k, increment j
               SkinMimic * tmp = skins[j];
               skins[j] = skins[k];
               skins[k] = tmp;
               j++;
            }
      }
      if (j!=skins.size() && !AppConfig::GetAllowUnusedMeshes())
      {
         setExportError("Unused skins were found.");
         return;
      }
      for (;j<skins.size();j++)
      {
         delete skins[j];
         delElement(skins,j);
         j--;
      }
   }
   // skins are now in order of detail size and unused ones have been deleted

   for (S32 i=0; i<objectList.size(); i++)
   {
      if (!objectList[i]->isSkin)
         continue;
      // all meshes on this object should be skins...
      for (S32 meshNum=0; meshNum<objectList[i]->numDetails; meshNum++)
      {
         if (!objectList[i]->details[meshNum].mesh)
            continue;
         if (!objectList[i]->details[meshNum].mesh->skinMimic)
         {
            setExportError("Assertion failed generating skins");
            return;
         }

         // this mesh is a skin...set it up (it's already been created)
         Mesh * skinMesh = objectList[i]->details[meshNum].mesh->tsMesh;

         // now find the corresponding skin
         SkinMimic * skin = objectList[i]->details[meshNum].mesh->skinMimic;
         skin->skinMesh = skinMesh;
         skinMesh->numFrames = 1;
         skinMesh->matFrames = 1;
         skinMesh->vertsPerFrame = skin->verts.size();

         skinMesh->primitives = skin->faces;
         skinMesh->indices = skin->indices;
         skinMesh->normals = skin->normals;
         skinMesh->verts = skin->verts;
         skinMesh->tverts = skin->tverts;

         S32 j,k;

         // up till now weights have been stored in terms of vertId...convert to vertex number now
         // copyWeightsToVerts(objectList[i]->details[meshNum].mesh->skinMimic);

         // for each bone, indicate node index in shape and inverseDefaultTransform
         assert(skinMesh->nodeTransform.size()==0 && "Assertion failed");
         assert(skinMesh->nodeIndex.size()==0 && "Assertion failed");
         Matrix<4,4,F32> boundsTransform = boundsNode->getNodeTransform( AppTime::DefaultTime() );
         zapScale(boundsTransform);
         for (j=0; j<skin->bones.size(); j++)
         {
            // find node index
            for (k=0; k<nodes.size(); k++)
               if (nodes[k]->appNode->isEqual(skin->bones[j]))
                  break;
            if (k==nodes.size())
            {
               setExportError("Error: bone missing from shape");
               return;
            }
            skinMesh->nodeIndex.push_back(k);

            Matrix<4,4,F32> boneTransform = nodes[k]->appNode->getNodeTransform( AppTime::DefaultTime() );

            zapScale(boneTransform);
      
            Matrix<4,4,F32> initTransform = boneTransform.inverse() * boundsTransform;
            
            skinMesh->nodeTransform.push_back(initTransform);
         }
   
         AppConfig::PrintDump(PDObjectStateDetails|PDPass2,avar("\r\nGenerating skin \"%s\".\r\n",skin->appMesh->getName()));
      
         // push all vertex, bone, weight triples
         AppConfig::PrintDump(PDObjectStateDetails,"\r\nVertex, bone, & weight data:\r\n\r\n");
         for (j=0; j<skinMesh->verts.size(); j++)
         {
            AppConfig::PrintDump(PDObjectStateDetails,avar("Vertex %i\r\n",j));   
            for (k=0; k<skin->bones.size(); k++)
            {
               if ((*skin->weights[k])[j]>=AppConfig::WeightThreshhold())
               {
                  skinMesh->vindex.push_back(j);
                  skinMesh->vbone.push_back(k);
                  skinMesh->vweight.push_back((*skin->weights[k])[j]);

                  AppConfig::PrintDump(PDObjectStateDetails,avar("  Bone %i, weight = %5.3f, name =  \"%s\"\r\n",
                     skinMesh->vbone.back(),skinMesh->vweight.back(),skin->bones[k]->getName()));
               }
            }
         }
      }
   }
}

void ShapeMimic::copyWeightsToVerts(SkinMimic * skinMimic)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // on input, weights are stored in a bone x vertId matrix
   // on output, weights will be stored in a bone x vert index matrix
   // difference between vertId and vert Index is that the former
   // signifies the order of the vert in the 3d app while the latter
   // corresponds to the order in our vert list

   S32 i,j;
   std::vector<SkinMimic::WeightList*> oldWeights = skinMimic->weights;
   for (i=0; i<oldWeights.size(); i++)
      skinMimic->weights[i] = new SkinMimic::WeightList;

   for (i=0; i<oldWeights.size(); i++)
      for (j=0; j<skinMimic->vertId.size(); j++)
         skinMimic->weights[i]->push_back((*(oldWeights[i]))[skinMimic->vertId[j]]);

   for (i=0; i<oldWeights.size(); i++)
         delete oldWeights[i];
}



//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::optimizeMeshes(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;
   
   S32 i,j,k;
   
   AppConfig::PrintDump(PDObjectStateDetails,"\r\nOptimizing meshes...\r\n");

   // go through meshes and optimize each one...
   for (i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      if (om->isSkin)
         // save skins for later...
         continue;
      for (j=om->numDetails-1; j>=0; j--)
      {
         if (!om->details[j].mesh)
            continue;

         AppConfig::PrintDump(PDObjectStateDetails,avar("\r\nOptimizing mesh \"%s\" detail level %i.\r\n",om->name,om->details[j].size));

         Mesh * mesh = om->details[j].mesh->tsMesh;
         std::vector<U32> & smooth = om->details[j].mesh->smoothingGroups;
         std::vector<U32> & remap = om->details[j].mesh->remap;

         // collapse vertices
         collapseVertices(mesh,smooth,remap,NULL);

         // need to sprinkle these here and there to avoid crashes...
         if (isError()) return;

         // now that verts are collapsed, delete any trivial facees
         for (S32 k=0; k<mesh->primitives.size(); k++)
         {
            Primitive & face = mesh->primitives[k];
            U32 start = face.firstElement;
            U32 idx0 = mesh->indices[start + 0];
            U32 idx1 = mesh->indices[start + 1];
            U32 idx2 = mesh->indices[start + 2];
            if (idx0==idx1 || idx1==idx2 || idx0==idx2)
            {
               delElement(mesh->indices,start);
               delElement(mesh->indices,start);
               delElement(mesh->indices,start);

               for (S32 l=0; l<mesh->primitives.size(); l++)
                  if (U16(mesh->primitives[l].firstElement) >= start)
                     mesh->primitives[l].firstElement -= 3;
               delElement(mesh->primitives,k);
               k--;
            }
         }
         
         //
         if (om->details[j].mesh->sortedObject)
            continue;

         // strip
         stripify(mesh->primitives,mesh->indices);

         // need to sprinkle these here and there to avoid crashes...
         if (isError()) return;
      }
   }
   
   // need to sprinkle these here and there to avoid crashes...
   if (isError()) return;

   // optimize skins...
   std::vector<U32> remap;
   for (i=skins.size()-1; i>=0; i--)
   {
      SkinMimic * skin = skins[i];
      Mesh * skinMesh = skin->skinMesh;
      std::vector<U32> & smooth = skin->smoothingGroups;
      std::vector<U32> * vertId = &skin->vertId;
      
      // first make sure we have no missing verts...
      for (j=1; j<skinMesh->vindex.size(); j++)
      {
         if (skinMesh->vindex[j]-skinMesh->vindex[j-1]>1)
         {
            setExportError(avar("Vertex %i missing weight on skin \"%s\"",skinMesh->vindex[j]+1,skin->appMesh->getName()));
            return;
         }
      }

      // start optimizing this skin...
      AppConfig::PrintDump(PDObjectStateDetails,avar("\r\nOptimizing skin mesh \"%s\" detail level %i.\r\n",skin->appMesh->getName(),skin->detailSize));

      // we don't respect smoothing groups on skins
      for (j=0;j<smooth.size();j++)
         smooth[j]=0;

      // collapse vertices      
      collapseVertices(skinMesh,smooth,remap,vertId);

      // remap some information
      for (j=0; j<skinMesh->vindex.size(); j++)
         skinMesh->vindex[j] = remap[skinMesh->vindex[j]];
      for (j=(S32)skinMesh->vindex.size()-1; j>0; j--)
      {
         for (k=0; k<j; k++)
         {
            if (skinMesh->vindex[k]==skinMesh->vindex[j] && skinMesh->vbone[k]==skinMesh->vbone[j])
            {
               if (fabs(skinMesh->vweight[j]-skinMesh->vweight[k])>0.01f)
               {
                  setExportError("Assertion failed when collapsing vertices on skin (1)");
                  return;
               }

               // vertex and bone index for kth and jth tuple match...merge them
               delElement(skinMesh->vweight,j);
               delElement(skinMesh->vindex,j);
               delElement(skinMesh->vbone,j);
               break; // out of k loop
            }
         }
      }

      // re-sort the vertexIndex, boneIndex, weight lists by vertex and bone, respectively...
      for (j=0; j<(S32)skinMesh->vindex.size()-1; j++)
      {
         for (k=j+1; k<skinMesh->vindex.size(); k++)
         {
            if ((skinMesh->vindex[k]<skinMesh->vindex[j]) || (skinMesh->vindex[k]==skinMesh->vindex[j] && skinMesh->vbone[k]<skinMesh->vbone[j]))
            {
               // swap
               S32 tmp = skinMesh->vindex[k];
               skinMesh->vindex[k] = skinMesh->vindex[j];
               skinMesh->vindex[j] = tmp;
               tmp = skinMesh->vbone[k];
               skinMesh->vbone[k] = skinMesh->vbone[j];
               skinMesh->vbone[j] = tmp;
               F32 tmp2 = skinMesh->vweight[k];
               skinMesh->vweight[k] = skinMesh->vweight[j];
               skinMesh->vweight[j] = tmp2;
            }
         }
      }

      // strip
      stripify(skinMesh->primitives,skinMesh->indices);
   }
}

void ShapeMimic::collapseVertices(Mesh * mesh, std::vector<U32> & smooth, std::vector<U32> & remap, std::vector<U32> * vertId)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;
   
   if (mesh->verts.size() != mesh->normals.size())
   {
      setExportError("Assertion failed when collapsing vertices (2)");
      return;
   }

   AppConfig::PrintDump(PDObjectStateDetails,avar("%i verts before joining verts\r\n",mesh->verts.size()));

   S32 i,j;

   // set up remap
   remap.resize(mesh->vertsPerFrame);
   for (i=0; i<remap.size(); i++)
      remap[i]=i;

   for (i=(S32)mesh->vertsPerFrame-1; i>0; i--)
   {
      // try to find a vertex earlier in the list that matches this one
      for (j=i-1; j>=0; j--)
      {
         //------------------------------------------
         // same location, tvert, smoothing group, vert id (if passed)?
         U32 s1 = smooth.size()!=0 ? smooth[i] : 1;
         U32 s2 = smooth.size()!=0 ? smooth[j] : 1;
         if (!vertexSame(mesh->verts[i],mesh->verts[j],mesh->tverts[i],mesh->tverts[j],s1,s2,mesh->normals[i],mesh->normals[j],i,j,vertId))
            continue;

         //------------------------------------------
         // ok, but are we the same for all frames and matFrames too?
         S32 k,l;
         for (k=1; k<mesh->numFrames; k++)
         {
            S32 startVert = k * mesh->vertsPerFrame;
            for (l=1; l<mesh->matFrames; l++)
            {
               S32 startTVert = l * mesh->vertsPerFrame;
               if (!vertexSame(mesh->verts[i+startVert],mesh->verts[j+startVert],mesh->tverts[i+startTVert],mesh->tverts[j+startTVert],s1,s2,mesh->normals[i+startVert],mesh->normals[j+startVert],i,j,vertId))
                  break;
            }
            if (l!=mesh->matFrames)
               break;
         }
         if (k!=mesh->numFrames)
            // not same throughout
            continue;

         //------------------------------------------
         // alright, vertex i and j are the same...get rid of vertex i (i>j)
         if (i<=j)
         {
            setExportError("Assertion failed when collapsing vertex (3)");
            return;
         }
         for (k=0; k<mesh->indices.size(); k++)
         {
            if (mesh->indices[k] == i)
               mesh->indices[k] = j;
            else if (mesh->indices[k]>i)
               mesh->indices[k]--;
         }

         for (k=mesh->numFrames-1; k>=0; k--)
         {
            S32 startVert = mesh->vertsPerFrame * k;
            delElement(mesh->verts,i+startVert);
            delElement(mesh->normals,i+startVert);
         }
         for (k=mesh->matFrames-1; k>=0; k--)
         {
            S32 startTVert = mesh->vertsPerFrame * k;
            delElement(mesh->tverts,i+startTVert);
         }
         if (vertId)
            delElement(*vertId,i);
         if (smooth.size() != 0)
            delElement(smooth,i);
         mesh->vertsPerFrame--;

         // update remap -- we're getting rid of vertex i and replacing it with vertex j
         // any vertex currently mapped to i should be replaced by j, but we know all verts
         // before i are there original selves (since i-loop is going backwards) so we can start
         // at i...also shift indices greater than i
         for (k=i;k<remap.size();k++)
            if (remap[k]==i)
               remap[k]=j;
            else if (remap[k]>i)
               remap[k]--;

         break; // out of j loop
      }
   }

   if (smooth.size() != 0)
      // generate normals using smoothing groups...
      computeNormals(mesh->primitives,mesh->indices,mesh->verts,mesh->normals,smooth,mesh->vertsPerFrame,mesh->numFrames);

   // have normals...now encode them
   mesh->enormals.clear();
   for (i=0; i<mesh->normals.size(); i++)
      mesh->enormals.push_back(Mesh::encodeNormal(mesh->normals[i]));

   AppConfig::PrintDump(PDObjectStateDetails,avar("%i verts after joining verts\r\n",mesh->verts.size()));

   if (mesh->verts.size() * mesh->matFrames != mesh->tverts.size() * mesh->numFrames)
      setExportError("ShapeMimic::collapseVertices (3)");
   else if (mesh->verts.size() != mesh->normals.size())
      setExportError("ShapeMimic::collapseVertices (4)");
}

void ShapeMimic::computeNormals(std::vector<Primitive> & faces, std::vector<U16> & indices, std::vector<Point3D> & verts, std::vector<Point3D> & norms, std::vector<U32> & smooth, S32 vertsPerFrame, S32 numFrames)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (vertsPerFrame * numFrames != verts.size() || vertsPerFrame!=smooth.size())
   {
      setExportError("Assertion failed:  vertex number mismatch");
      return;
   }
      
   S32 i,j;
   std::vector<S32> counts(verts.size());
   norms.resize(verts.size());
   for (i=0; i<verts.size(); i++)
   {
      counts[i]=0;
      norms[i] = Point3D(0.0f,0.0f,0.0f);
   }
   for (S32 frameNum = 0; frameNum<numFrames; frameNum++)
   {
      S32 startVert = frameNum * vertsPerFrame;
      for (i=0; i<faces.size(); i++)
      {
         Primitive & tsFace = faces[i];
         if ((tsFace.type & Primitive::TypeMask) != Primitive::Triangles)
         {
            setExportError("Assertion error while computing normals");
            return;
         }
         // find the normal to this face
         S32 idx0 = indices[tsFace.firstElement+0];
         S32 idx1 = indices[tsFace.firstElement+1];
         S32 idx2 = indices[tsFace.firstElement+2];
         Point3D v0 = verts[startVert+idx0];
         Point3D v1 = verts[startVert+idx1];
         Point3D v2 = verts[startVert+idx2];

         Point3D n,v20,v10;
         v20 = v2-v0;
         if (dotProduct(v20,v20)>0.0000001f)
            v20.normalize();
         v10 = v1-v0;
         if (dotProduct(v10,v10)>0.0000001f)
            v10.normalize();
         crossProduct(v20,v10,&n);
         if (dotProduct(n,n) > 0.0000001f)
         {
            n.normalize();
            for (S32 j=0; j<vertsPerFrame; j++)
            {
               Point3D vj = verts[startVert+j];
               if (isEqual(v0,vj,AppConfig::SameVertTOL()) && (smooth[idx0]&smooth[j] || smooth[idx0]==smooth[j]))
               {
                  norms[startVert+j] += n;
                  counts[startVert+j]++;
               }
               if (isEqual(v1,vj,AppConfig::SameVertTOL()) && (smooth[idx1]&smooth[j] || smooth[idx1]==smooth[j]))
               {
                  norms[startVert+j] += n;
                  counts[startVert+j]++;
               }
               if (isEqual(v2,vj,AppConfig::SameVertTOL()) && (smooth[idx2]&smooth[j] || smooth[idx2]==smooth[j]))
               {
                  norms[startVert+j] += n;
                  counts[startVert+j]++;
               }
            }
         }
      }
   }
   // now average normals...
   for (i=0; i<norms.size(); i++)
   {
      if (counts[i] && dotProduct(norms[i],norms[i])>0.0000001f)
         norms[i].normalize();
   }
   // for verts w/o a normal, search for someone with same vert location and smoothing group
   for (i=0; i<counts.size(); i++)
   {
      if (!counts[i])
      {
         for (j=0; j<verts.size(); j++)
         {
            if (!counts[j])
               continue;
            if (!isEqual(verts[i],verts[j],AppConfig::SameVertTOL()))
               continue;
            if (smooth[i]==smooth[j])
            {
               norms[i]=norms[j];
               counts[i]++;
               break;
            }
         }
      }
   }
   // if above didn't work, try anyone in same location
   for (i=0; i<counts.size(); i++)
   {
      if (!counts[i])
      {
         // search for someone with same vert location sans smoothing group
         for (j=0; j<verts.size(); j++)
         {
            if (!counts[j])
               continue;
            if (!isEqual(verts[i],verts[j],AppConfig::SameVertTOL()))
               continue;
            norms[i]=norms[j];
            counts[i]++;
            break;
         }
      }
   }
   // just in case, set any normal still without a value to 0,0,1
   for (i=0; i<counts.size(); i++)
      if (!counts[i])
         norms[i] = Point3D(0,0,1);
}

void ShapeMimic::stripify(std::vector<Primitive> & primitives, std::vector<U16> & indices)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   if (primitives.empty() || indices.empty())
      // shouldn't really have empty meshes...but no harm, no foul (we would, however, cause
      // problems in the stripper with empty meshes).
      return;

   S32 i;

   S32 startFaces = primitives.size();

   // in:  primitives better just be faces and better use indexes
   for (i=0; i<primitives.size(); i++)
   {
      if (primitives[i].type == -1)
      {
         setExportError("Assertion failed when stripping -- negative material index");
         return;
      }
      if ( (primitives[i].type & ~(Primitive::NoMaterial^Primitive::MaterialMask)) != (Primitive::Triangles|Primitive::Indexed) || primitives[i].numElements!=3)
      {
         setExportError("Assertion failed when stripifying (1)");
         return;
      }
   }

   AppConfig::PrintDump(PDObjectStateDetails,avar("%i faces before stripping\r\n",startFaces));

#ifdef USE_NVIDIA_STRIPPER
   nvStripWrap(primitives,indices,16);
   const char * method = "NVidia";
#else
   Stripper stripper(primitives,indices);
   stripper.setLimitStripLength(false);
   stripper.makeStrips();
   stripper.getStrips(primitives,indices);
   const char * method = "Quick, dirty, & ugly";
#endif

   if (AppConfig::GetDumpMask() & PDObjectStateDetails)
   {
      AppConfig::PrintDump(PDObjectStateDetails,avar("Using %s stripping method.\r\n",method));
      float len = 0.0f;
      S32 hi = -1;
      S32 lo = -1;
      for (i=0; i<primitives.size(); i++)
      {
         len += primitives[i].numElements;
         if (primitives[i].numElements > hi)
            hi = primitives[i].numElements;
         if (lo==-1 || primitives[i].numElements < lo)
            lo = primitives[i].numElements;
      }
      S32 reversals = len - (startFaces + primitives.size() * 2); // no. of times we needed to reverse order of face by sending extra vert
      if (!primitives.empty())
         len *= 1.0f / (F32)primitives.size();
      AppConfig::PrintDump(PDObjectStateDetails,avar("%i strips with average length %3.2f (range %i to %i) and %i reversals\r\n",primitives.size(),len,lo,hi,reversals));
   }
}

bool ShapeMimic::vertexSame(Point3D & v1, Point3D & v2, Point2D & tv1, Point2D & tv2, U32 smooth1, U32 smooth2, Point3D & norm1, Point3D & norm2, U32 idx1, U32 idx2, std::vector<U32> * vertId)
{
   if (!isEqual(norm1,norm2,AppConfig::SameNormTOL()))
      return false;
   if (!isEqual(v1,v2,AppConfig::SameVertTOL()) || !isEqual(tv1,tv2,AppConfig::SameTVertTOL()) || smooth1 != smooth2)
      return false;

   if (!vertId || (*vertId)[idx1]==(*vertId)[idx2])
      return true;

   // At this point, we know that the vert has the same tvert and vert coords, but that the 3d app thinks of
   // it as a different vertex...for non-skin meshes we don't care, but we won't get _this_ far in that case
   // (we'd have returned true because vertId==NULL for non-skin meshes).  

   // At some point, we may want some way to determine whether we can delete this vertex if we get this far...
   // e.g., if both verts have same set of weights...

   return false;
}

//-----------------------------------------------------------
//
//-----------------------------------------------------------

void ShapeMimic::convertSortObjects(Shape * shape)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // go through meshes and convert sortObjects when we find them
   for (S32 i=0; i<objectList.size(); i++)
   {
      ObjectMimic * om = objectList[i];
      for (S32 j=0; j<om->numDetails; j++)
      {
         if (!om->details[j].mesh || !om->details[j].mesh->sortedObject)
            continue;

         Mesh * sortMesh = om->details[j].mesh->tsMesh;

         // get sort data from user properties...
         AppMesh * appMesh = om->details[j].mesh->appMesh;
         S32 numBigFaces = 0;
         S32 maxDepth = 2;
         bool zLayerUp = false;
         bool zLayerDown = false;
         appMesh->getInt("SORT::NUM_BIG_FACES",numBigFaces);
         appMesh->getInt("SORT::MAX_DEPTH",maxDepth);
         appMesh->getBool("SORT::Z_LAYER_UP",zLayerUp);
         appMesh->getBool("SORT::Z_LAYER_DOWN",zLayerDown);

         if (zLayerUp && zLayerDown)
         {
            setExportError("Cannot use both Z_LAYER_UP and Z_LAYER_DOWN.");
            return;
         }

         TranslucentSort::generateSortedMesh(sortMesh,numBigFaces,maxDepth,zLayerUp,zLayerDown);
         S32 saveNumFrames = sortMesh->numFrames;
         sortMesh->vertsPerFrame = sortMesh->verts.size();
         std::vector<U32> remap;
         std::vector<U32> smooth(sortMesh->verts.size());
         for (S32 k=0; k<smooth.size(); k++)
            smooth[k]=1;
         collapseVertices(sortMesh,smooth,remap,NULL);
         sortMesh->numFrames = saveNumFrames;
         sortMesh->vertsPerFrame = 0; // not used
      }
   }
}

void ShapeMimic::fillNodeTransformCache(std::vector<NodeMimic*> & nodes, Sequence & seq, AppSequenceData & seqData)
{
   S32 i;

   // clear out the transform caches and set it up for this sequence
   for (i=0; i<nodeRotCache.size(); i++)
      delete [] nodeRotCache[i];
   nodeRotCache.clear();
   for (i=0; i<nodeTransCache.size(); i++)
      delete [] nodeTransCache[i];
   nodeTransCache.clear();
   for (i=0; i<nodeScaleRotCache.size(); i++)
      delete [] nodeScaleRotCache[i];
   nodeScaleRotCache.clear();
   for (i=0; i<nodeScaleCache.size(); i++)
      delete [] nodeScaleCache[i];
   nodeScaleCache.clear();

   nodeRotCache.resize(nodes.size());
   for (i=0; i<nodeRotCache.size(); i++)
      nodeRotCache[i] = new Quaternion[seqData.numFrames];
   nodeTransCache.resize(nodes.size());
   for (i=0; i<nodeTransCache.size(); i++)
      nodeTransCache[i] = new Point3D[seqData.numFrames];
   nodeScaleRotCache.resize(nodes.size());
   for (i=0; i<nodeScaleRotCache.size(); i++)
      nodeScaleRotCache[i] = new Quaternion[seqData.numFrames];
   nodeScaleCache.resize(nodes.size());
   for (i=0; i<nodeScaleCache.size(); i++)
      nodeScaleCache[i] = new Point3D[seqData.numFrames];

   // get all the node transforms for every frame
   AppTime time = seqData.startTime;
   for (S32 frame = 0; frame<seqData.numFrames; frame++, time += seqData.delta)
      for (i=0;i<nodes.size();i++)
         generateNodeTransform(nodes[i],time,seqData.blend,seqData.blendReferenceTime,
                               nodeRotCache[i][frame],nodeTransCache[i][frame],
                               nodeScaleRotCache[i][frame],nodeScaleCache[i][frame]);
}


//-----------------------------------------------------------
//
//-----------------------------------------------------------
ObjectMimic * ShapeMimic::addObject(AppNode * node, AppMesh * mesh, std::vector<S32> * validDetails)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   ObjectMimic * om;
   const char * name = mesh->getName();
   tweakName(&name);
    
   // separate object name from detail size for current mesh
   S32 size;
   char * objectName = chopTrailingNumber(name,size);

   // artist can set detail level in the user properties if they want...
   mesh->getInt("Detail",size);

   S32 detailPos;
   om = getObject(node,mesh,objectName,size,&detailPos);
   if (!om)
      return NULL;

   // set up the table of allowed detail levels
   // this will be checked when we start adding meshes
   // to the shape...start by making sure it hasn't
   // been set up already or if it has that at least
   // it points to the same place...
   if (om->validDetails && validDetails && validDetails!=om->validDetails)
   {
      setExportError(avar("Mesh \"%s\" occurs in two different places on the shape.",om->name));
      return NULL;
   }
   if (validDetails)
   {
      // set valid detail levels...
      om->validDetails = validDetails;
      om->inTreeNode = node;
      om->inTreeMesh = mesh;
      // we now know what subtree we belong in -- unless error
      if (om->subtreeNum>=0 && om->subtreeNum != subtrees.size()-1)
      {
         setExportError(avar("Mesh \"%s\" occurs in two different subtrees on the shape.",om->name));
         return NULL;
      }
      om->subtreeNum = subtrees.size() - 1;
   }

   // if we were passed validDetails then we are on the shape (a hack for a check?)
   // in this case, fill in the parent of the object
   if (validDetails)
   {
      om->appParent = node;
      om->appTSParent = node; // this may change later...
   }

   return om;
}

ObjectMimic * ShapeMimic::getObject(AppNode * node, AppMesh * mesh, char * name, S32 size, S32 * detailNum, bool matchFull, bool isBone, bool isSkin)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   S32 i;

   // if this is a billboard object, detect that now and adjust name accordingly
   // Note: billboard objects are named BB::* (the BB:: is dropped from here on).
   bool billboard = mesh ? mesh->isBillboard() : false;
   bool sortedObject = mesh ? mesh->isSorted() : false;

   // if the name ends in : then ignore the : (and when we search this object, we compare to full name instead of name)
   char * colon = NULL;
   if (strlen(name)>1 && name[strlen(name)-1]==':')
   {
      colon  = name+strlen(name)-1;
      *colon = '\0';
   }
      
   // if we're in the shape tree, find out here (alternative is that we are unlinked and a detail level of a mesh)
   bool inTree = node && !node->isParentRoot();

   // if we're in the tree, we may need the full name...
   const char * fullName = NULL;
   if (inTree)
   {
      fullName = mesh? mesh->getName() : node->getName();
      tweakName(&fullName);  // get's rid of "BB::" prefix, for example
   }

   // look for name in list of objects
   for (i=0; i<objectList.size(); i++)
   {
      if (isBone!=objectList[i]->isBone || isSkin!=objectList[i]->isSkin)
         continue;
      if (matchFull && inTree && objectList[i]->fullName && !stricmp(fullName,objectList[i]->fullName))
         break;
      if ( colon && objectList[i]->fullName && !stricmp(name,objectList[i]->fullName))
         break;
      if (!colon && !stricmp(name,objectList[i]->name))
         break;
   }

   // add an entry if needed
   if (i==objectList.size())
   {
      objectList.push_back(new ObjectMimic);
      objectList.back()->name = name;
      // note:  not straight forward ... full name is the full name of the object
      // on the tree.  If inTree, add that now...if "colon" is not null, use "name" for
      // full name.
      if (inTree)
         objectList.back()->fullName = strnew(fullName);
      else if (colon)
         objectList.back()->fullName = strnew(name);
      else
         objectList.back()->fullName = NULL;
      objectList.back()->numDetails = 0;
      objectList.back()->validDetails = NULL;
      objectList.back()->subtreeNum = -1;
      objectList.back()->appParent = NULL;
      objectList.back()->appTSParent = NULL;
      objectList.back()->tsObject = NULL;
      objectList.back()->tsNodeIndex = -1;
      objectList.back()->isBone = isBone;
      objectList.back()->isSkin = isSkin;
      AppConfig::PrintDump(PDPass2,avar("Adding object named \"%s\".\r\n",name));
   }
   else
      delete [] name; // don't need duplicate name

   ObjectMimic * om = objectList[i];
   if (om->isBone)
   {
      AppConfig::PrintDump(PDPass2,"Object is bone\r\n");
      om->appParent = node;
      om->appTSParent = node;
      return om;
   }

   // enter data
   S32 dl = om->numDetails++;
   if (om->numDetails>ObjectMimic::MaxDetails)
   {                               
      setExportError(avar("Assertion failed:  too many details for mesh %s.",name));
      return NULL;
   }

   AppConfig::PrintDump(PDPass2,avar("Adding mesh of size %i to object \"%s\".\r\n", size,om->name));

   // keep meshes sorted by size
   S32 j;
   for (j=dl; j>=0; j--)
   {
      if (j==0 || size<om->details[j-1].size)
      {
         if (j<dl && om->details[j].size==size)
         {
            setExportError(avar("Found two meshes named \"%s\" of size %i.\r\n",om->name,size));
            // avoid crash...
            *detailNum = j;
            om->details[j].size = size;
            om->details[j].mesh = NULL;
            return NULL;
         }

         // this is where we go:
         // larger than all that follow us
         // smaller than all that precede us
         om->details[j].size = size;
         om->details[j].mesh = new MeshMimic(mesh);
         om->details[j].mesh->billboard = billboard;
         om->details[j].mesh->sortedObject = sortedObject;
         *detailNum = j;
         break; // all in order
      }
      else
         // not here, move j-1 up to make room
         om->details[j] = om->details[j-1];
   }

   return om;
}

void ShapeMimic::addSubtree(AppNode * node)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   subtrees.push_back(new Subtree);
   Subtree * subtree = subtrees.back();
   std::vector<S32> & validDetails = subtree->validDetails;
   std::vector<const char*> & detailNames = subtree->detailNames;
   std::vector<AppNode*> & detailNodes = subtree->detailNodes;
   const char * name = node->getName();

   // we need to create a dummy node for branches to hang off of
   // it will correspond to pNode...but won't be exported
   subtree->start.appNode = node;
   subtree->start.parent  = NULL;
   subtree->start.child   = NULL;
   subtree->start.sibling = NULL;

   // first go through the top level and parse
   // into detail markers and shape branches
   std::vector<AppNode*> branches;
   S32 i;
   for (i=0; i<node->getNumChildNodes(); i++)
   {
      AppNode * child = node->getChildNode(i);

      // we'll deal with these separately...
      if (child->isBounds())
         continue;

      if (child->getNumChildNodes()==0)
      {
         S32 size;
         char * dname = chopTrailingNumber(child->getName(),size);
         if (strcmp(dname,child->getName()))
         {
            delete [] dname;
            dname = strnew(child->getName()); // use full name, with size
            validDetails.push_back(size);
            detailNames.push_back(dname);
            detailNodes.push_back(child);
            AppConfig::PrintDump(PDPass2,avar("Adding detail named \"%s\" of size %i to subtree \"%s\".\r\n", dname, size, name));
         }
         else
         {
            AppConfig::PrintDump(PDPass2,avar("Ignoring node named \"%s\" off subtree \"%s\" because no trailing number.\r\n", dname,name));
            delete [] dname;
         }
      }
      else
         branches.push_back(child);
   }

   // the detail list
   // need to keep names and sizes in synch...we don't already
   // have a struct set up to do this the easy way, so do a
   // bubble sort
   S32 j;
   for (i=0; i<(S32)validDetails.size()-1; i++)
   {
      for (j=i+1; j<validDetails.size(); j++)
      {
         if (validDetails[j]>validDetails[i])
         {
            S32 tmpInt;
            const char * tmpCh;
            tmpInt = validDetails[i];
            tmpCh = detailNames[i];
            validDetails[i] = validDetails[j];
            detailNames[i] = detailNames[j];
            validDetails[j] = tmpInt;
            detailNames[j] = tmpCh;
         }
      }
   }

   if (validDetails.empty() || branches.empty())
   {
      // nothing here, but if we made it this far it isn't an error
      delete subtree;
      subtrees.pop_back();
      return;
   }

   addNode(&subtree->start,node,validDetails,false);
   for (i=0; i<branches.size(); i++)
      addNode(subtree->start.child,branches[i],validDetails,true);

   // everything needs to be rooted to the bounds node...
   subtree->start.appNode = boundsNode;
}

void ShapeMimic::addNode(NodeMimic * mimicParent,
                         AppNode * appChild,
                         std::vector<S32> & validDetails,
                         bool recurseChildren)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   // if it's the bounds node or a camera, don't do anything
   if (appChild->isBounds())
      return;

   AppConfig::PrintDump(PDPass2,avar("Adding node \"%s\" with parent \"%s\" to subtree rooted on node \"%s\".\r\n", appChild->getName(), mimicParent->appNode->getName(),subtrees.back()->start.appNode->getName()));

   NodeMimic * mimicChild = new NodeMimic;
   mimicChild->appNode = appChild;
   mimicChild->parent  = mimicParent;
   mimicChild->child   = NULL;
   mimicChild->sibling = NULL;
   if (mimicParent->child)
   {
      // put us at the end of parent's child list
      NodeMimic * sib = mimicParent->child;
      while (sib->sibling)
         sib = sib->sibling;
      sib->sibling = mimicChild;
   }
   else
      // so far, an only child
      mimicParent->child = mimicChild;

   S32 i;
   for (i=0; i<appChild->getNumMesh(); i++)
   {
      AppMesh * appMesh = appChild->getMesh(i);
      if (!appMesh->isDummy())
      {
         AppConfig::PrintDump(PDPass2,"Attaching object to node.\r\n");
         mimicChild->objects.push_back(addObject(appChild,appMesh,&validDetails));
      }
   }

   // now mimic the children of maxChild...
   if (recurseChildren)
   {
      S32 i;
      for (i=0; i<appChild->getNumChildNodes(); i++)
         addNode(mimicChild,appChild->getChildNode(i),validDetails,true);
   }
}

void ShapeMimic::addSkin(AppMesh * mesh)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return;

   S32 i,j,k;

   skins.push_back(new SkinMimic);
   SkinMimic * skinMimic = skins.back();
   skinMimic->appMesh = mesh;
   skinMimic->detailSize = getTrailingNumber(mesh->getName());

   // get offset matrix
   Matrix<4,4,F32> meshTransform = mesh->getMeshTransform(AppTime::DefaultTime());
   Matrix<4,4,F32> boundsTransform = boundsNode->getNodeTransform(AppTime::DefaultTime());
   zapScale(boundsTransform);
   Matrix<4,4,F32> objectOffset = boundsTransform.inverse() * meshTransform;

   // lock the mesh
   AppMeshLock lock = mesh->lockMesh(AppTime::DefaultTime(),objectOffset);

   // get bones
   S32 numBones = mesh->getNumBones();
   skinMimic->bones.resize(numBones);
   for (i=0; i<numBones; i++)
   {
      skinMimic->bones[i] = mesh->getBone(i);
      AppConfig::PrintDump(PDPass2,avar("Adding skin object from skin \"%s\" to bone \"%s\" (%i).\r\n",mesh->getName(),skinMimic->bones[i]->getName(),i));
   }
   
   // if no bones...don't add anything
   if (skinMimic->bones.empty())
   {
      delete skins.back();
      skins.pop_back();
      return;
   }

   S32 numVerts = mesh->getNumVerts();
   skinMimic->weights.resize(numBones);
   for (i=0; i<skinMimic->weights.size(); i++)
   {
      skinMimic->weights[i] = new SkinMimic::WeightList;
      skinMimic->weights[i]->resize(numVerts);
   }

   for (i=0; i<numBones; i++)
      for (j=0; j<numVerts; j++)
         (*skinMimic->weights[i])[j] = mesh->getWeight(i,j);

   // limit number of bones per vertex and apply weight threshhold
   for (i=0;i<skinMimic->weights[0]->size();i++)
   {
      F32 ** hi  = new F32 * [AppConfig::WeightsPerVertex()];
      for (k=0; k<AppConfig::WeightsPerVertex(); k++)
         hi[k]  = NULL;

      for (j=0; j<skinMimic->bones.size(); j++)
      {
         F32 & w = (*skinMimic->weights[j])[i];
         for (k=0; k<AppConfig::WeightsPerVertex() && (!hi[k] || *hi[k]<w); k++);
         k--;

         if (k<0 || w<AppConfig::WeightThreshhold())
         {
            w=0.0f;
            continue;
         }
         
         S32 pos = k;

         // zero out least significant saved weight
         if (hi[0])
            *hi[0] = 0.0f;
         // shift all the weights below new one down a notch
         for (k=0;k<pos;k++)
            hi[k]  = hi[k+1];
         // add our new weight
         hi[pos] = &w;
      }
      F32 sum = 0;
      for (k=0;k<AppConfig::WeightsPerVertex();k++)
         if (hi[k])
            sum += *hi[k];
      if (sum>AppConfig::WeightThreshhold())
         for (k=0;k<AppConfig::WeightsPerVertex();k++)
            if (hi[k])
               *hi[k] /= sum;

      delete [] hi;
   }

   // some bones may no longer have any weight...if so, throw them out
   for (i=0;i<skinMimic->bones.size();i++)
   {
      F32 sum = 0.0f;
      for (j=0;j<skinMimic->weights[i]->size();j++)
         sum += (*skinMimic->weights[i])[j];
      if (sum<AppConfig::WeightThreshhold())
      {
         // delete weight data for this bone
         AppConfig::PrintDump(PDPass2,avar("Deleting skin object  \"%s\" with no weight.\r\n",skinMimic->bones[i]->getName()));
         delete skinMimic->weights[i];
         delElement(skinMimic->weights,i);
         delElement(skinMimic->bones,i);
         i--;
      }
   }

   MeshMimic * meshMimic = addSkinObject(skinMimic); // goes into object list without node...

   // generate the faces of the mesh -- will be transfered to objects on subtrees later (as ts objects are generated)
   AppConfig::PrintDump(PDPass2,avar("Generating faces for skin \"%s\".\r\n",mesh->getName()));

   mesh->generateFaces(skinMimic->faces,
                       skinMimic->verts,
                       skinMimic->tverts,
                       skinMimic->indices,
                       skinMimic->smoothingGroups,
                       skinMimic->normals,
                       &skinMimic->vertId);
   meshMimic->numVerts = mesh->getNumVerts();
   if (skinMimic->normals.size() == 0)
   {
      skinMimic->normals.resize(meshMimic->numVerts);
      for (i=0; i<skinMimic->normals.size(); i++)
         // normals get reset when collapsing verts...make sure sensible value is in here for now
         skinMimic->normals[i] = Point3D(0,0,1);
   }

   // make sure all the materials are added
   for (j=0; j<mesh->getNumFaces(); j++)
   {
      // add material for face j
      S32 mi = addFaceMaterial(mesh,j);

      // replace appmesh material index with ts material index
      skinMimic->faces[j].type &= ~Primitive::MaterialMask;
      if (mi<0)
         skinMimic->faces[j].type |= Primitive::NoMaterial;
      else
         skinMimic->faces[j].type |= mi;
   }

   // iterate through the subtrees looking for bones...when we find them, add a skin object
   for (i=0; i<subtrees.size(); i++)
   {
      Subtree * subtree = subtrees[i];
      NodeMimic * mimicNode = subtree->start.child;
      while (mimicNode)
      {
         if (mimicNode==&subtree->start)
         {
            // this should just never happen...
            setExportError("Assertion failed:  Illegal condition.");
            return;
         }

         // a bone?
         for (j=0; j<skinMimic->bones.size(); j++)
         {
            if (skinMimic->bones[j]->isEqual(mimicNode->appNode))
            {
               ObjectMimic * obj = addBoneObject(skinMimic->bones[j],i);
               if (!obj)
                  return;
               for (k=0;k<mimicNode->objects.size();k++)
                  if (mimicNode->objects[k]==obj)
                     break;
               if (k==mimicNode->objects.size())
                  mimicNode->objects.push_back(obj);
            }
         }

         // figure out where to go next
         mimicNode = findNextNode(mimicNode);
      }
   }
}

ObjectMimic * ShapeMimic::addBoneObject(AppNode * node, S32 subtreeNum)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   const char * name = node->getName();
   char * boneName = new char[strlen(name)+20];
   sprintf(boneName,"Bone::%s:",name);

   S32 detailPos;
   ObjectMimic * om = getObject(node,NULL,boneName,0,&detailPos,false,true,false);

   return om;
}

MeshMimic * ShapeMimic::addSkinObject(SkinMimic * skinMimic)
{
   // if already encountered an error, then
   // we'll just go through the motions
   if (isError()) return NULL;

   S32 size;

   // first, separate object name from detail size for current mesh
   const char * name = skinMimic->appMesh->getName();
   char * objectName = chopTrailingNumber(name,size);

   S32 detailPos;

   ObjectMimic * om = getObject(NULL,skinMimic->appMesh,objectName,skinMimic->detailSize,&detailPos,true,false,true);
   if (isError() || om==NULL) return NULL; // detailPos might not be valid...
   om->details[detailPos].mesh->skinMimic = skinMimic;

   om->subtreeNum = 0; // we'll assume skins are on the first detail...at least for now
   om->validDetails = &subtrees[om->subtreeNum]->validDetails;
   om->appParent = om->appTSParent = NULL;
   om->inTreeMesh = NULL;
   om->inTreeNode = NULL;

   return om->details[detailPos].mesh;
}

void ShapeMimic::dumpShapeNode(Shape * shape, S32 level, S32 nodeIndex, std::vector<S32> & detailSizes)
{
   if (nodeIndex<0)
      return;

   S32 i;
   char space[256];
   for (i = 0; i < level*3; i++)
      space[i] = ' ';
   space[level*3] = '\0';

   const char *nodeName = "";
   const Node & node = shape->nodes[nodeIndex];
   if (node.name != -1)
     nodeName = shape->names[node.name].c_str();
   AppConfig::PrintDump(PDShapeHierarchy,avar("%s%s", space, nodeName));

   // find all the objects that hang off this node...
   std::vector<Object*> objectList;
   for (i=0; i<shape->objects.size(); i++)
      if (shape->objects[i].node == nodeIndex)
         objectList.push_back(&shape->objects[i]);

   if (objectList.size() == 0)
      AppConfig::PrintDump(PDShapeHierarchy,"\r\n");

   S32 spaceCount = -1;
   for (S32 j=0;j<objectList.size(); j++)
   {
      Object * obj = objectList[j];
      if (!obj)
         continue;

      // object name
      const char *objectName = "";
      if (obj->name!=-1)
         objectName = shape->names[obj->name].c_str();

      // more spaces if this is the second object on this node
      if (spaceCount>0)
      {
         char buf[1024];
         memset(buf,' ',spaceCount);
         buf[spaceCount] = '\0';
         AppConfig::PrintDump(PDShapeHierarchy,buf);
      }

      // dump object name
      AppConfig::PrintDump(PDShapeHierarchy,avar(" --> Object %s with following details: ",objectName));

      // dump object detail levels
      for (S32 k=0; k<obj->numMeshes; k++)
      {
         S32 f = obj->firstMesh;
         if (shape->meshes[f+k].getType() != Mesh::T_Null)
            AppConfig::PrintDump(PDShapeHierarchy,avar(" %i",detailSizes[k]));
      }

      AppConfig::PrintDump(PDShapeHierarchy,"\r\n");

      // how many spaces should we prepend if we have another object on this node
      if (spaceCount<0)
         spaceCount = (S32)(strlen(space) + strlen(nodeName));           
   }

   // search for children
   for (S32 k=nodeIndex+1; k<shape->nodes.size(); k++)
   {
      if (shape->nodes[k].parent == nodeIndex)
         // this is our child
         dumpShapeNode(shape, level+1, k, detailSizes);
   }
}

void ShapeMimic::dumpShape(Shape * shape)
{
   S32 i,j,ss,od,sz;
   const char * name;

   AppConfig::PrintDump(PDShapeHierarchy,"\r\nShape Hierarchy:\r\n");

   AppConfig::PrintDump(PDShapeHierarchy,"\r\n   Details:\r\n");

   for (i=0; i<shape->detailLevels.size(); i++)
   {
      const DetailLevel & detail = shape->detailLevels[i];
      name = detail.name<0 ? NULL : shape->names[detail.name].c_str();
      ss = detail.subshape;
      od = detail.objectDetail;
      sz = (S32)detail.size;
      AppConfig::PrintDump(PDShapeHierarchy,avar("      %s, Subtree %i, objectDetail %i, size %i\r\n",name,ss,od,sz));
   }

   AppConfig::PrintDump(PDShapeHierarchy,"\r\n   Subtrees:\r\n");

   for (i=0; i<shape->subshapes.size(); i++)
   {
      S32 a = shape->subshapes[i].firstNode;
      S32 b = a + shape->subshapes[i].numNodes;
      AppConfig::PrintDump(PDShapeHierarchy,avar("      Subtree %i\r\n",i));

      // compute detail sizes for each subshape
      std::vector<S32> detailSizes;
      for (S32 l=0;l<shape->detailLevels.size(); l++)
      {
          if (shape->detailLevels[l].subshape==i)
              detailSizes.push_back((S32)shape->detailLevels[l].size);
      }

      for (j=a; j<b; j++)
      {
          const Node & node = shape->nodes[j];
          // if the node has a parent, it'll get dumped via the parent
          if (node.parent<0)
              dumpShapeNode(shape,3,j,detailSizes);
      }
   }

   bool foundSkin = false;
   for (i=0; i<shape->objects.size(); i++)
   {
      if (shape->objects[i].node<0) // must be a skin
      {
         if (!foundSkin)
            AppConfig::PrintDump(PDShapeHierarchy,"\r\n   Skins:\r\n");
         foundSkin=true;
         const char * skinName = "";
         S32 nameIndex = shape->objects[i].name;
         if (nameIndex>=0)
            skinName = nameIndex<0 ? NULL : shape->names[nameIndex].c_str();
         AppConfig::PrintDump(PDShapeHierarchy,avar("      Skin %s with following details: ",skinName));
         for (S32 num=0; num<shape->objects[i].numMeshes; num++)
         {
            if (shape->meshes[num].getType() != Mesh::T_Null)
               AppConfig::PrintDump(PDShapeHierarchy,avar(" %i",(S32)shape->detailLevels[num].size));
         }
         AppConfig::PrintDump(PDShapeHierarchy,"\r\n");
      }
   }
   if (foundSkin)
      AppConfig::PrintDump(PDShapeHierarchy,"\r\n");

   AppConfig::PrintDump(PDShapeHierarchy,"\r\n   Sequences:\r\n");
   for (i = 0; i < shape->sequences.size(); i++)
   {
      const char *name = "(none)";
      S32 nameIndex = shape->sequences[i].nameIndex;
      if (nameIndex != -1)
         name = shape->names[nameIndex].c_str();
      AppConfig::PrintDump(PDShapeHierarchy,avar("      %3d: %s\r\n",i,name));
   }

   std::vector<Material> & ml = shape->materials;
   AppConfig::PrintDump(PDShapeHierarchy,"\r\n   Material list:\r\n");
   for (i=0; i<ml.size(); i++)
   {
      U32 flags = ml[i].flags;
      const char * name = ml[i].name.c_str();
      AppConfig::PrintDump(PDShapeHierarchy,avar("   material #%i: \"%s\"%s.",i,name ? name : "",flags&(Material::SWrap|Material::TWrap) ? "" : " not tiled"));
      if (flags & Material::IFLMaterial)
         AppConfig::PrintDump(PDShapeHierarchy,"  Place holder for ifl.");
      if (flags & Material::IFLFrame)
         AppConfig::PrintDump(PDShapeHierarchy,"  Ifl frame.");
      if (flags & Material::DetailMap)
         AppConfig::PrintDump(PDShapeHierarchy,"  Used as a detail map.");
      if (flags & Material::BumpMap)
         AppConfig::PrintDump(PDShapeHierarchy,"  Used as a bump map.");
      if (flags & Material::ReflectanceMap)
         AppConfig::PrintDump(PDShapeHierarchy,"  Used as a reflectance map.");
      if (flags & Material::Translucent)
      {
         if (flags & Material::Additive)
            AppConfig::PrintDump(PDShapeHierarchy,"  Additive-translucent.");
         else if (flags & Material::Subtractive)
            AppConfig::PrintDump(PDShapeHierarchy,"  Subtractive-translucent.");
         else
            AppConfig::PrintDump(PDShapeHierarchy,"  Translucent.");
      }
      AppConfig::PrintDump(PDShapeHierarchy,"\r\n");
   }
}


} // namespace DTS

