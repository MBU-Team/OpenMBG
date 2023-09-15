//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GTEXMANAGER_H_
#define _GTEXMANAGER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

// to be removed at some point, hopefully...
#ifndef _PLATFORMGL_H_

#include "platform/platformAssert.h"
#include "platform/platformGL.h"

#endif

//-------------------------------------- Forward Decls.
class GBitmap;

//------------------------------------------------------------------------------
//-------------------------------------- TextureHandle
//

/// Enumerated values for different types of textures
/// Generally speaking, all textures will have mipmaps extruded automatically
/// and use linear magnification and linear_mipmap_nearest minification filtering
/// unless trilinear filtering is enabled.  Then, the minification is linear_maipmap_linear.
/// Most texture types have their bitmap data destroyed after texture creation.
/// Also, detail settings affect any of the textures except the ones noted.
/// Detail settings are based on the mip maps, so if the detail level is turned down,
/// the highest resolution detail map will NOT be the one that is in the actual
/// texture file, but one that has been averaged down appropriately.
///
/// @note "Small textures" are a different set of textures, dynamically created with
/// certain texture types whose first mip level is the same as the 4th mip level
/// of the first texture.  So, the full res small texture will be the same as the
/// 4th averaged down version of the origional texture.
enum TextureHandleType
{
   BitmapTexture = 0,         ///< Regular bitmap - does not to be pow2, but will be converted to pow2, only 1 mip level
   BitmapKeepTexture,         ///< Same as BitmapTexture, but the data will be kept after creation
   BitmapNoDownloadTexture,   ///< Same as BitmapTexture except data will not be loaded to OpenGL and cannot be "bound"
   RegisteredTexture,         ///< INTERNAL USE ONLY - texture that has already been created and is just being reinstated
   MeshTexture,               ///< Has "small textures"
   TerrainTexture,            ///< OFF LIMITS - for terrain engine use only.  You WILL cause problems if you use this type
   SkyTexture,                ///< Hooks into the sky texture detail level
   InteriorTexture,           ///< Has "small textures" and hooks into the interior texture detail level
   BumpTexture,               ///< Same as DetailTexture, except colors are halved
   InvertedBumpTexture,       ///< Same as BumpTexture, except colors are then inverted
   
   DetailTexture,             ///< If not palettized, will extrude mipmaps, only used for terrain detail maps
   ZeroBorderTexture          ///< Clears the border of the texture on all mip levels
};


class TextureObject
{
  public:
   TextureObject *next;
   TextureObject *prev;
   TextureObject *hashNext;

   GLuint texGLName;       ///< GL-Bindable texture index
   GLuint smallTexGLName;  ///< @see TextureHandleType

#ifdef GATHER_METRICS
   U32 textureSpace;
#endif

   StringTableEntry texFileName;
   /// The actual bitmap data of the texture
   /// @note this is usually destroyed once the texture has been loaded,
   /// except in the case of BitmapKeepTexture, so any changes that might
   /// need to be made to it need to be done within the loading code
   GBitmap * bitmap;

   U32 texWidth;
   U32 texHeight;
   
   U32 bitmapWidth;
   U32 bitmapHeight;

   U32 downloadedWidth;
   U32 downloadedHeight;

   TextureHandleType type;
   bool               filterNearest;
   bool              clamp;
   bool              holding;
   S32               refCount;
};

typedef void (*TextureEventCallback)(const U32 eventCode, void *userData);

struct TextureManager
{
   // additional functions for refreshing the textures, reloading larger
   // mip levels, etc, will go in here, as well as delay-load functions.
   friend class TextureHandle;
   friend class InteriorLMManager;
   friend struct TextureDictionary;

  private:
   /// Loads a texture from file, and returns the result of registerTexture
   static TextureObject* loadTexture(const char *textureName, TextureHandleType type, bool clampToEdge);
   /// Inserts a texture into the hash table, and gives the texture an OpenGL name
   static TextureObject* registerTexture(const char *textureName, const GBitmap *data, bool clampToEdge);
   /// Inserts a texture into the hash table, and gives the texture an OpenGL name
   static TextureObject* registerTexture(const char *textureName, GBitmap *data, TextureHandleType type, bool clampToEdge);
   /// Deletes the texture data and removes the texture from the texture dictionary hash table and OpenGL
   static void           freeTexture(TextureObject *to);
   /// Creates the OpenGL texture and sets all related 
   static bool           createGLName(GBitmap *pb, bool clampToEdge, U32 firstMip, TextureHandleType type, TextureObject* obj);
   static void           refresh(TextureObject *to);
   static void           refresh(TextureObject *to, GBitmap*);
   static GBitmap*       createMipBitmap(const GBitmap* pBitmap);
   /// Just in case the texture specified does not have sides of power-of-2,
   /// this function will copy the texture data into a new texture that IS power-of-2
   /// and fill in the empty areas with the adjacent pixel
   static GBitmap*       createPaddedBitmap(GBitmap* pBitmap);


  public:
   static void create();
   static void preDestroy();
   static void destroy();

   /// @name Zombification
   /// This pair of functions is a flush() equivalent.  To flush
   ///  the cache, call:
   ///   makeZombie(); /* blah blah blah */ resurrect();
   ///  Note that NO drawing must take place until resurrect is
   ///  called.  The manager is a stinking corpse at this point.
   ///  The split is necessary to support changing the OpenGL
   ///  device in the "right way".  This way glDeleteTexture is
   ///  called on the original device rather than on the new
   ///  device, as a flush() call would necessitate.
   /// @{

   static void makeZombie();  
   static void resurrect();   
   /// @}

   /// Added for convenience when you don't need to worry about the above problems. (Zombification)
   static void flush();
   static bool smIsZombie; ///< Is the texture manager a skulking undead brain-eating zombie from the great beyond?  I sure hope not...

#ifdef GATHER_METRICS
   static void dumpStats();
#endif
   
   enum EventCodes {
      BeginZombification = 0,
      CacheResurrected   = 1
   };
   static U32  registerEventCallback(TextureEventCallback, void *userData);
   static void unregisterEventCallback(const U32 callbackKey);

  private:
   static void postTextureEvent(const U32);
   static bool smUseSmallTextures;

  public:
   static const char * csmTexturePrefix;

   static void setSmallTexturesActive(const bool t) { smUseSmallTextures = t;    }
   static bool areSmallTexturesActive()             { return smUseSmallTextures; }
   static GBitmap *loadBitmapInstance(const char *textureName);
   
#ifdef GATHER_METRICS
   static U32 smTextureSpaceLoaded;
   static U32 smTextureCacheMisses;

   static F32 getResidentFraction();
#endif
};

/// This is the main texture manager interface.  Texturing can be
/// a bit complicated, but if you follow these easy steps, it is
/// really quite simple!
///
/// In order to use a texture on disk, first you must create a
/// TextureHandle data structure for it.
/// @code
/// TextureHandle handle = TextureHandle("pathToTexture", textureType);
/// @endcode
/// See the documentation on the different enumerated types for more info
/// on texture types.
///
/// Ok, now you have your texture loaded into video memory or ram,
/// whichever is chooses.  In order to tell OpenGL to use your texture,
/// you have to bind it.  GL_TEXTURE_2D is the type of texture you're
/// binding - a 2 dimisional texture.  Also note that you only have
/// to do this if you are using direct OpenGL commands to draw rather
/// than dgl.  Dgl manages the below on it's own so you don't have to worry about it.
/// @code
/// glBindTexture(GL_TEXTURE_2D, handle.getGLName());
/// @endcode
/// Now you can begin to draw you texture.  If you havn't already,
/// make sure you make a call to glEnable(GL_TEXTURE_2D);  before
/// you start drawing and a call to glDisable(GL_TEXTURE_2D); when
/// you're done.  Failure to call glEnable will cause the texture not
/// to draw, and failure to call glDisable will probably case
/// an assert in debug mode and ugly artifacts in release.
///
/// If you are going through dgl, all you need is the TextureHandle and
/// some points.  See the dgl documentation for more info on each
/// individual function in the dgl library.  However, most dgl functions
/// will take a TextureObject data type.  And, it just so happens that
/// a TextureHandle has a TextureObject!  It also has an
/// operator TextureObject*(), which lets you cast a TextureHandle to
/// a TextureObject.  That means that all you have to do is ignore
/// the TextureObject parameter and just give it a TextureHandle.
///
/// Some tips on texture performance:
///
/// Instead of using hard-coded paths, use a hook to a console variable.
/// You will probably change the directory structure for your game,
/// and that means that you will have to go back to all of the hardcoded
/// paths and change them by hand, then rebuild the engine.  It is much
/// better to use script variables since they are all in one place and
/// easier to change.
///
/// Add the path string for your texture to the StringTable.  Doing so
/// helps in string lookups and faster string performance.
///
/// Don't create the texture every frame if at all possible.  Make it
/// a global variable if you have to - just don't load every frame.
/// Loading data off of the disk every frame WILL cause massive
/// performance loss and should be avoided at all costs.  This is
/// not to mention the overhead of generating mip map levels
/// and uploading the texture into memory when the texture is created.
///
/// @note
/// Texture handles can be allocated in 2 ways - by name to be loaded
/// from disk, or by name to a dynamically generated texture
/// 
/// If you create a GBitmap and register it, the Texture manager
/// owns the pointer - so if you re-register a texture with the same
/// name, the texture manager will delete the second copy.
///
/// Also note the operator TextureObject*, as you can actually cast
/// a TextureHandle to a TextureObject* if necessary.
class TextureHandle
{
   TextureObject *object;
   void lock();
   void unlock();

  public:
   TextureHandle() { object = NULL; }

   TextureHandle(TextureObject *to)
   {
      object = to;
      lock();
   }

   TextureHandle(const TextureHandle &th) {
      object = th.object;
      lock();
   }

   TextureHandle(const char*       textureName,
                 TextureHandleType type,                 // was =BitmapTexture - dc removed to eliminate overload confusion.
                 bool              clampToEdge = false)
   {
      object = TextureManager::loadTexture(textureName, type, clampToEdge);
      lock();
   }

   TextureHandle(const char*    textureName,
                 const GBitmap* bmp,
                 bool           clampToEdge = false) {
      object = TextureManager::registerTexture(textureName, bmp, clampToEdge);
      lock();
   }

   TextureHandle(const char*       textureName,
                 GBitmap*          bmp,
                 TextureHandleType type,
                 bool              clampToEdge = false) {
      object = TextureManager::registerTexture(textureName, bmp, type, clampToEdge);
      lock();
   }

   ~TextureHandle() { unlock(); }

   TextureHandle& operator=(const TextureHandle &t) {
      unlock();
      object = t.object;
      lock();
      return *this;
   }

   bool set(const char *textureName,
            TextureHandleType type=BitmapTexture,
            bool clampToEdge = false) {
      TextureObject* newObject = TextureManager::loadTexture(textureName, type, clampToEdge);;
      if (newObject != object)
      {
         unlock();
         object = newObject;
         lock();
      }
      return (object != NULL);
   }

   bool set(const char *textureName,
            const GBitmap *data,
            bool clampToEdge = false) {
      TextureObject* newObject = TextureManager::registerTexture(textureName, data, clampToEdge);
      if (newObject != object)
      {
         unlock();
         object = newObject;
         lock();
      }
      return (object != NULL);
   }

   bool set(const char *textureName,
            GBitmap *bmp,
            TextureHandleType type,
            bool clampToEdge = false) {
      TextureObject* newObject = TextureManager::registerTexture(textureName, bmp, type, clampToEdge);
      if (newObject != object)
      {
         unlock();
         object = newObject;
         lock();
      }
      return (object != NULL);
   }

   bool operator==(const TextureHandle &t) const { return t.object == object; }
   bool operator!=(const TextureHandle &t) const { return t.object != object; }

   void setClamp(const bool);
   void setFilterNearest();
   
   void refresh();
   void refresh(GBitmap*);
   operator TextureObject*()        { return object; }
   /// Returns the texture's filename if it exists
   const char* getName() const      { return (object ? object->texFileName      : NULL); }
   U32 getWidth() const             { return (object ? object->bitmapWidth      : 0UL);    }
   U32 getHeight() const            { return (object ? object->bitmapHeight     : 0UL);    }
   U32 getDownloadedWidth() const   { return (object ? object->downloadedWidth  : 0UL);    }
   U32 getDownloadedHeight() const  { return (object ? object->downloadedHeight : 0UL);    }
   GBitmap* getBitmap()             { return (object ? object->bitmap           : NULL); }
   /// Gets the OpenGL index of the texture for use in glBindTexture().
   U32 getGLName() const;
};

#if defined(GATHER_METRICS) && GATHER_METRICS > 1
#ifndef _PLATFORMGL_H_
#if defined(TORQUE_OS_MAC)
#include "PlatformMacCarb/platformGL.h"
#elif defined(TORQUE_OS_WIN32)
#include "PlatformWin32/platformGL.h"
#endif
#endif

inline U32 TextureHandle::getGLName() const
{
   if (!object)
      return 0;

   U32 useName = object->texGLName;
   if (TextureManager::areSmallTexturesActive() && object->smallTexGLName != 0)
      useName = object->smallTexGLName;

   if (useName != 0) {
      GLboolean res;
      glAreTexturesResident(1, &useName, &res);
      if (res == GL_FALSE)
         TextureManager::smTextureCacheMisses++;
   }

   return useName;
}

#else

inline U32 TextureHandle::getGLName() const
{
   if (!object)
      return 0;

   U32 useName = object->texGLName;
   if (TextureManager::areSmallTexturesActive() && object->smallTexGLName != 0)
      useName = object->smallTexGLName;

   return useName;
}

#endif

#endif // _GTEXMANAGER_H_
