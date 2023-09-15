//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GCHUNKEDTEXMANAGER_H_
#define _GCHUNKEDTEXMANAGER_H_

#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif

class ChunkedTextureObject
{
public:
   ChunkedTextureObject *next;
   StringTableEntry texFileName;
   U32 texWidthCount;
   U32 texHeightCount;
   U32 width;
   U32 height;
   TextureHandle *textureHandles;
   S32 refCount;
   GBitmap *bitmap;
};

class ChunkedTextureManager
{
   friend class ChunkedTextureHandle;
   static ChunkedTextureObject* loadTexture(const char *textureName);
   static ChunkedTextureObject* registerTexture(const char *textureName, GBitmap *data, bool keep);
   static void freeTexture(ChunkedTextureObject *to);
   static void refresh(ChunkedTextureObject *to);
public:
   static void makeZombie();
   static void resurrect();
};

class ChunkedTextureHandle
{
   ChunkedTextureObject *object;
   void lock();
   void unlock();
public:
   ChunkedTextureHandle() { object = NULL; }
   ChunkedTextureHandle(const ChunkedTextureHandle &th) {
      object = th.object;
      lock();
   }
   ChunkedTextureHandle(const char *textureName)
   {
      object = ChunkedTextureManager::loadTexture(textureName);
      lock();
   }
   ChunkedTextureHandle(const char *textureName, GBitmap *bmp)
   {
      object = ChunkedTextureManager::registerTexture(textureName, bmp, true);
      lock();
   }
   ~ChunkedTextureHandle() { unlock(); }
   
   ChunkedTextureHandle& operator=(const ChunkedTextureHandle &t) { unlock(); object = t.object; lock(); return *this; }
   void refresh() { if(object) ChunkedTextureManager::refresh(object); }
   operator ChunkedTextureObject*()        { return object; }
   const char* getName() const      { return (object ? object->texFileName      : NULL); }
   U32 getWidth() const             { return (object ? object->width            : 0UL);    }
   U32 getHeight() const            { return (object ? object->height           : 0UL);    }
   GBitmap* getBitmap()             { return (object ? object->bitmap           : NULL); }
   TextureHandle getSubTexture(U32 x, U32 y);
   U32 getTextureCountWidth() { return (object ? object->texWidthCount : 0UL);    };
   U32 getTextureCountHeight(){ return (object ? object->texHeightCount : 0UL);    };
};

#endif
