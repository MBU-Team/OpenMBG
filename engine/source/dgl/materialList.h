//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _MATERIALLIST_H_
#define _MATERIALLIST_H_

#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif
#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif
#ifndef _CONSOLE_H_
#include "console/console.h"
#endif


//--------------------------------------
class MaterialList : public ResourceInstance
{
private:
   friend class TSMaterialList;

   enum Constants { BINARY_FILE_VERSION = 1 };
   
public:
   VectorPtr<char*> mMaterialNames;
   Vector<TextureHandle> mMaterials;
protected:
   bool mClampToEdge;
   TextureHandleType mTextureType;

public:
   MaterialList();
   MaterialList(U32 materialCount, const char **materialNames);
   ~MaterialList();

   /// Note: this is not to be confused with MaterialList(const MaterialList&).  Copying
   ///  a material list in the middle of it's lifetime is not a good thing, so we force
   ///  it to copy at construction time by retricting the copy syntax to
   ///  ML* pML = new ML(&copy);
   explicit MaterialList(const MaterialList*);

   U32 getMaterialCount() { return (U32)mMaterials.size(); }
   const char * getMaterialName(U32 index) { return mMaterialNames[index]; }
   TextureHandle &getMaterial(U32 index)
   {
      AssertFatal(index < (U32) mMaterials.size(), "MaterialList::getMaterial: index lookup out of range.");
      return mMaterials[index];
   }

   // material properties
   void setTextureType(TextureHandleType type) { mTextureType = type; }
   void setClampToEdge(bool tf) { mClampToEdge = tf; }

   void set(U32 materialCount, const char **materialNames);
   U32  push_back(TextureHandle textureHandle, const char *filename);
   U32  push_back(const char *filename);
   U32  push_back(const char *filename, GBitmap *bmp, TextureHandleType type, bool clampToEdge = false);

   virtual void load(U32 index, const char* path = 0);
   bool load(const char* path = 0);
   bool load(TextureHandleType type, const char* path = 0, bool clampToEdge = false);
   void unload();
   virtual void free();

   typedef Vector<TextureHandle>::iterator iterator;
   typedef Vector<TextureHandle>::value_type value;
   TextureHandle& front() { return mMaterials.front(); }
   TextureHandle& first() { return mMaterials.first(); }
   TextureHandle& last()  { return mMaterials.last(); }
   bool       empty() { return mMaterials.empty();   }
   U32        size()  { return (U32)mMaterials.size(); }
   iterator   begin() { return mMaterials.begin(); }
   iterator   end()   { return mMaterials.end(); }
   value operator[] (S32 index) { return getMaterial(U32(index)); }

   bool read(Stream &stream);
   bool write(Stream &stream);

   bool readText(Stream &stream, U8 firstByte);
   bool readText(Stream &stream);
   bool writeText(Stream &stream);
};


//--------------------------------------
inline bool MaterialList::load(TextureHandleType type, const char* path, bool clampToEdge)
{
   mTextureType = type;
   mClampToEdge = clampToEdge;
   return load(path);
}


extern ResourceInstance* constructMaterialList(Stream &stream);


#endif
