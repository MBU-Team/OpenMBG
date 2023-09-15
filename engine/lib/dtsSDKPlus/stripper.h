//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "DTSTypes.h"
#include "DTSShape.h"
#include "DTSMesh.h"
#include "DTSPlusTypes.h"
#include "DTSUtil.h"
#include "dtsBitMatrix.h" // see comment in dtsMatrix.h

namespace DTS
{
   class Stripper
   {
      std::vector<S32> numAdjacent;
      std::vector<bool> used;
      BitMatrix adjacent;
      std::vector<S32> vertexCache;
      std::vector<S32> recentFaces;
      S32 currentFace;
      bool limitStripLength;
      S32 bestLength;
      U32 cacheMisses;

      std::vector<Primitive> strips;
      std::vector<U16> stripIndices;
   
      std::vector<Primitive> & faces;
      std::vector<U16> & faceIndices;

      void clearCache();
      void addToCache(S32 vertexIndex);
      void addToCache(S32 vertexIndex, U32 posFromBack);

      void getVerts(S32 face, S32 & oldVert0, S32 & oldVert1, S32 & addVert);
      void rotateFace(S32 start, std::vector<U16> & indices);
      bool swapNeeded(S32 oldVert0, S32 oldVert1);
      F32 getScore(S32 face, bool ignoreOrder);
      bool faceHasEdge(S32 face, U32 idx0, U32 idx1);
      void getAdjacentFaces(S32 startFace, S32 endFace, S32 face, S32 & face0, S32 & face1, S32 & face2);

      void setAdjacency(S32 startFace, S32 endFace);
      bool startStrip(Primitive & strip, S32 startFace, S32 endFace);
      bool addStrip(Primitive & strip, S32 startFace, S32 endFace);
      bool stripLongEnough(S32 startFace, S32 endFace);

      void testCache(S32 addedFace);
      bool canGo(S32 face);

      void makeStripsB(); // makeStrips() from faces...assumes all faces have same material index
      void copyParams(Stripper *from);

      public:

      Stripper(std::vector<Primitive> & faces, std::vector<U16> & indices);
      Stripper(Stripper &);
      ~Stripper();
   
      void makeStrips();
      S32 continueStrip(S32 startFace, S32 endFace, S32 len, S32 restart); // used for simulation...
      void getStrips(std::vector<Primitive> & s, std::vector<U16> & si) { s=strips; si=stripIndices; }

      void setLimitStripLength(bool lim) { limitStripLength = lim; }
      void resetCacheMisses() { cacheMisses = 0; }
      U32 getCacheMisses() { return cacheMisses; }
   
      // adjust strip building strategy
      static F32 adjacencyWeight;
      static F32 noswapWeight;
      static F32 alreadyCachedWeight;
      static U32 cacheSize;
      static U32 simK;
   };

}
