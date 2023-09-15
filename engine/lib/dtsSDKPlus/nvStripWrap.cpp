//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------


#ifdef _MSC_VER
#pragma warning(disable : 4786)
#endif

#include "nvStripWrap.h"
#include "nvtristrip/NvTriStrip.h"
#include "nvtristrip/NvTriStripObjects.h"

namespace DTS
{

   void nvMakeStrips(std::vector<Primitive> & primitives, std::vector<U16> & indices, S32 cacheSize, U32 matIndex)
   {
      // incoming indices may have some "holes" in them...e.g., may have index 1,2,3,6,7 only
      // map to 0..N-1
      std::vector<S16> map;
      std::vector<S16> remap;
      U16 next=0;
      S32 i;
      for (i=0; i<indices.size(); i++)
      {
         while (indices[i]>=map.size())
            map.push_back(-1);
         if (map[indices[i]]<0)
         {
            map[indices[i]]=next++;
            remap.push_back(indices[i]);
         }
         indices[i] = map[indices[i]];
      }

      PrimitiveGroup * primGroups;
      U16 numGroups;
      GenerateStrips(&indices[0],indices.size(),&primGroups,&numGroups);

      for (i=0; i<numGroups; i++)
      {
         assert(primGroups[i].type == PT_STRIP);
         Primitive strip;
         strip.type = matIndex;
         strip.firstElement = indices.size();
         strip.numElements  = primGroups[i].numIndices;
         primitives.push_back(strip);
         for (S32 j=0; j<primGroups[i].numIndices; j++)
            indices.push_back(primGroups[i].indices[j]);
      }
      delete [] primGroups;

      // remap indices...
      for (i=0; i<indices.size(); i++)
         indices[i] = remap[indices[i]];
   }

   void nvStripWrap(std::vector<Primitive> & faces, std::vector<U16> & indices, S32 cacheSize)
   {
      // set stripper parameters...we'll just set some default ones
      SetCacheSize(cacheSize);
      SetStitchStrips(true);              // engine can handle this, so let it
      SetListsOnly(false);                 // engine can handle this, so let it
      SetMinStripSize(0);                  // engine can handle this, so let it

      // main strip loop...
      U32 start, end, i;
      std::vector<Primitive> someStrips;
      std::vector<Primitive> retStrips;
      std::vector<U16> someIndices;
      std::vector<U16> retIndices;
      for (start = 0; start<faces.size(); start=end)
      {
         for (end=start; end<faces.size() && faces[start].type==faces[end].type; end++)
            ;

         // copy start to end faces into new list -- this is so we end up doing less copying
         // down the road (when we are doing the look ahead simulation)
         someStrips.clear();
         someIndices.clear();
         for (i=start;i<end;i++)
         {
            someIndices.push_back(indices[faces[i].firstElement + 0]);
            someIndices.push_back(indices[faces[i].firstElement + 1]);
            someIndices.push_back(indices[faces[i].firstElement + 2]);
         }

         U32 matIndex = faces[start].type ^ (Primitive::Triangles|Primitive::Strip);
         nvMakeStrips(someStrips,someIndices,cacheSize,matIndex);

         // now move strips and indices into larger list
         S32 startStrips = retStrips.size();
         retStrips.resize(startStrips+someStrips.size());
         S32 startIndices = retIndices.size();
         retIndices.resize(startIndices+someIndices.size());
         memcpy(&retStrips[startStrips],&someStrips[0],someStrips.size()*sizeof(Primitive));
         memcpy(&retIndices[startIndices],&someIndices[0],someIndices.size()*sizeof(U16));
         // now adjust start of new strips
         for (i=startStrips; i<retStrips.size(); i++)
            retStrips[i].firstElement += startIndices;
      }
      indices = retIndices;
      faces = retStrips;
   }

}; // namespace DTS

