//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsShape.h"


TSMaterialList::TSMaterialList(U32 materialCount,
                               const char **materialNames,
                               const U32 * materialFlags,
                               const U32 * reflectanceMaps,
                               const U32 * bumpMaps,
                               const U32 * detailMaps,
                               const F32 * detailScales,
                               const F32 * reflectionAmounts)
 : MaterialList(materialCount,materialNames),
   mNamesTransformed(false)
{
   VECTOR_SET_ASSOCIATION(mFlags);
   VECTOR_SET_ASSOCIATION(mReflectanceMaps);
   VECTOR_SET_ASSOCIATION(mBumpMaps);
   VECTOR_SET_ASSOCIATION(mDetailMaps);
   VECTOR_SET_ASSOCIATION(mDetailScales);
   VECTOR_SET_ASSOCIATION(mReflectionAmounts);

   allocate(getMaterialCount());

   dMemcpy(mFlags.address(),materialFlags,getMaterialCount()*sizeof(U32));
   dMemcpy(mReflectanceMaps.address(),reflectanceMaps,getMaterialCount()*sizeof(U32));
   dMemcpy(mBumpMaps.address(),bumpMaps,getMaterialCount()*sizeof(U32));
   dMemcpy(mDetailMaps.address(),detailMaps,getMaterialCount()*sizeof(U32));
   dMemcpy(mDetailScales.address(),detailScales,getMaterialCount()*sizeof(F32));
   dMemcpy(mReflectionAmounts.address(),reflectionAmounts,getMaterialCount()*sizeof(F32));
}

TSMaterialList::TSMaterialList()
   : mNamesTransformed(false)
{
   VECTOR_SET_ASSOCIATION(mFlags);
   VECTOR_SET_ASSOCIATION(mReflectanceMaps);
   VECTOR_SET_ASSOCIATION(mBumpMaps);
   VECTOR_SET_ASSOCIATION(mDetailMaps);
   VECTOR_SET_ASSOCIATION(mDetailScales);
   VECTOR_SET_ASSOCIATION(mReflectionAmounts);
}

TSMaterialList::TSMaterialList(const TSMaterialList* pCopy)
   : MaterialList(pCopy)
{
   VECTOR_SET_ASSOCIATION(mFlags);
   VECTOR_SET_ASSOCIATION(mReflectanceMaps);
   VECTOR_SET_ASSOCIATION(mBumpMaps);
   VECTOR_SET_ASSOCIATION(mDetailMaps);
   VECTOR_SET_ASSOCIATION(mDetailScales);
   VECTOR_SET_ASSOCIATION(mReflectionAmounts);

   mFlags             = pCopy->mFlags;
   mReflectanceMaps   = pCopy->mReflectanceMaps;
   mBumpMaps          = pCopy->mBumpMaps;
   mDetailMaps        = pCopy->mDetailMaps;
   mDetailScales      = pCopy->mDetailScales;
   mReflectionAmounts = pCopy->mReflectionAmounts;
   mNamesTransformed  = pCopy->mNamesTransformed;
}

TSMaterialList::~TSMaterialList()
{
   free();
}

void TSMaterialList::free()
{
   // IflMaterials will duplicate names and textures found in other material slots
   // (In particular, IflFrame material slots).
   // Set the names to NULL now so our parent doesn't delete them twice.
   // Texture handles should stay as is...
   for (U32 i=0; i<getMaterialCount(); i++)
      if (mFlags[i] & TSMaterialList::IflMaterial)
         mMaterialNames[i] = NULL;

   // these aren't found on our parent, clear them out here to keep in synch
   mFlags.clear();
   mReflectanceMaps.clear();
   mBumpMaps.clear();
   mDetailMaps.clear();
   mDetailScales.clear();
   mReflectionAmounts.clear();

   Parent::free();
}

void TSMaterialList::remap(U32 toIndex, U32 fromIndex)
{
   AssertFatal(toIndex < size() && fromIndex < size(),"TSMaterial::remap");

   // only remap texture handle...flags and maps should stay the same...

   mMaterials[toIndex] = mMaterials[fromIndex];
   mMaterialNames[toIndex] = mMaterialNames[fromIndex];
}

void TSMaterialList::push_back(const char * name, U32 flags, U32 rMap, U32 bMap, U32 dMap, F32 dScale, F32 emapAmount)
{
   Parent::push_back(name);
   mFlags.push_back(flags);
   if (rMap==0xFFFFFFFF)
      mReflectanceMaps.push_back(getMaterialCount()-1);
   else
      mReflectanceMaps.push_back(rMap);
   mBumpMaps.push_back(bMap);
   mDetailMaps.push_back(dMap);
   mDetailScales.push_back(dScale);
   mReflectionAmounts.push_back(emapAmount);
}

void TSMaterialList::allocate(U32 sz)
{
   mFlags.setSize(sz);
   mReflectanceMaps.setSize(sz);
   mBumpMaps.setSize(sz);
   mDetailMaps.setSize(sz);
   mDetailScales.setSize(sz);
   mReflectionAmounts.setSize(sz);
}

U32 TSMaterialList::getFlags(U32 index)
{
   AssertFatal(index < getMaterialCount(),"TSMaterialList::getFlags: index out of range");
   return mFlags[index];
}

void TSMaterialList::setFlags(U32 index, U32 value)
{
   AssertFatal(index < getMaterialCount(),"TSMaterialList::getFlags: index out of range");
   mFlags[index] = value;
}

void TSMaterialList::load(U32 index,const char* path)
{
   AssertFatal(index < getMaterialCount(),"TSMaterialList::getFlags: index out of range");

   if (mFlags[index] & TSMaterialList::NoMipMap)
      mTextureType = BitmapTexture;
   else if (mFlags[index] & TSMaterialList::MipMap_ZeroBorder)
      mTextureType = ZeroBorderTexture;
   else
      mTextureType = MeshTexture;

   Parent::load(index,path);
}

bool TSMaterialList::write(Stream & s)
{
   if (!Parent::write(s))
      return false;

   U32 i;
   for (i=0; i<getMaterialCount(); i++)
      s.write(mFlags[i]);

   for (i=0; i<getMaterialCount(); i++)
      s.write(mReflectanceMaps[i]);

   for (i=0; i<getMaterialCount(); i++)
      s.write(mBumpMaps[i]);

   for (i=0; i<getMaterialCount(); i++)
      s.write(mDetailMaps[i]);
      
   for (i=0; i<getMaterialCount(); i++)
      s.write(mDetailScales[i]);

   for (i=0; i<getMaterialCount(); i++)
      s.write(mReflectionAmounts[i]);
      
   return (s.getStatus() == Stream::Ok);
}

bool TSMaterialList::read(Stream & s)
{
   if (!Parent::read(s))
      return false;

   allocate(getMaterialCount());

   U32 i;
   if (TSShape::smReadVersion<2)
   {
      for (i=0; i<getMaterialCount(); i++)
         setFlags(i,S_Wrap|T_Wrap);
   }
   else
   {
      for (i=0; i<getMaterialCount(); i++)
         s.read(&mFlags[i]);
   }

   if (TSShape::smReadVersion<5)
   {
      for (i=0; i<getMaterialCount(); i++)
      {
         mReflectanceMaps[i] = i;
         mBumpMaps[i] = 0xFFFFFFFF;
         mDetailMaps[i] = 0xFFFFFFFF;
      }
   }
   else
   {
      for (i=0; i<getMaterialCount(); i++)
         s.read(&mReflectanceMaps[i]);
      for (i=0; i<getMaterialCount(); i++)
         s.read(&mBumpMaps[i]);
      for (i=0; i<getMaterialCount(); i++)
         s.read(&mDetailMaps[i]);
   }

   if (TSShape::smReadVersion>11)
   {
      for (i=0; i<getMaterialCount(); i++)
         s.read(&mDetailScales[i]);
   }
   else
   {
      for (i=0; i<getMaterialCount(); i++)
         mDetailScales[i] = 1.0f;
   }

   if (TSShape::smReadVersion>20)
   {
      for (i=0; i<getMaterialCount(); i++)
         s.read(&mReflectionAmounts[i]);
   }
   else
   {
      for (i=0; i<getMaterialCount(); i++)
         mReflectionAmounts[i] = 1.0f;
   }

   if (TSShape::smReadVersion<16)
   {
      // make sure emapping is off for translucent materials on old shapes
      for (i=0; i<getMaterialCount(); i++)
         if (mFlags[i] & TSMaterialList::Translucent)
            mFlags[i] |= TSMaterialList::NeverEnvMap;
   }

   // get rid of name of any ifl material names
   for (i=0; i<getMaterialCount(); i++)
   {
      const char * str = dStrrchr(mMaterialNames[i],'.');
      if (mFlags[i] & TSMaterialList::IflMaterial ||
          (TSShape::smReadVersion<6 && str && dStricmp(str,".ifl")==0))
      {
         delete [] mMaterialNames[i];
         mMaterialNames[i] = NULL;
      }
   }

   return (s.getStatus() == Stream::Ok);
}

