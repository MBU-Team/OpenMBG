//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _ITF_H_
#define _ITF_H_

#ifndef _TYPES_H_
#include "platform/types.h"
#endif
#ifndef _COLOR_H
#include "core/color.h"
#endif
#ifndef _MPOINT_H_
#include "math/mPoint.h"
#endif

#ifndef _INTERIOR_H_
// redecl struct here for now... interior.h brings in the whole fricking codebase.
struct ItrPaddedPoint
{
   Point3F point;
   union {
      F32   fogCoord;
      U8    fogColor[4];
   };
};
#endif

struct OutputPoint
{
   Point3F point;
   union {
      F32    fogCoord; // to match input struct cleanly..
      U32    fogColors;
      U8     fogColor[4];
   };
   Point2F texCoord;
   Point2F lmCoord;
};

struct OutputPointFC_VB
{
   Point3F point; //0/4/8
   union {
      U32    currentColors;
      U8     currentColor[4];
   }; //12
   union {
      U32    fogColors;
      U8     fogColor[4];
   }; //16
   Point2F texCoord; //20/24
   Point2F lmCoord;  //28/32
};

struct OutputPointSP_FC_VB
{
   Point3F point;
   union {
      U32    lmColors;
      U8     lmColor[4];
   };
   union {
      U32    fogColors;
      U8     fogColor[4];
   };
   Point2F texCoord;
};

extern "C"
{
   // Process Globals
   extern F32   texGen0[8];
   extern F32   texGen1[8];
   extern Point2F *fogCoordinatePointer;

   // Process Functions
   void processTriFan(OutputPoint*          dst,
                      const ItrPaddedPoint* srcPoints,
                      const U32*            srcIndices,
                      const U32             numIndices);
   void processTriFanSP(OutputPoint*          dst,
                        const ItrPaddedPoint* srcPoints,
                        const U32*            srcIndices,
                        const U32             numIndices,
                        const ColorI*         srcColors);
   void processTriFanVC_TF(OutputPoint*          dst,
                           const ItrPaddedPoint* srcPoints,
                           const U32*            srcIndices,
                           const U32             numIndices,
                           const ColorI*         srcColors);
   void processTriFanSP_FC(OutputPoint*          dst,
                           const ItrPaddedPoint* srcPoints,
                           const U32*            srcIndices,
                           const U32             numIndices,
                           const ColorI*         srcColors);
   void processTriFanFC_VB(OutputPointFC_VB*         dst,
                           const ItrPaddedPoint*   srcPoints,
                           const U32*               srcIndices,
                           const U32               numIndices);
   void processTriFanSP_FC_VB(OutputPointSP_FC_VB*      dst,
                              const ItrPaddedPoint*   srcPoints,
                              const U32*               srcIndices,
                              const U32               numIndices,
                              const ColorI*            srcColors);

}

#endif
