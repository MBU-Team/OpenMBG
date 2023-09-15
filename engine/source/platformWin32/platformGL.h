//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

#define ENABLE_NPATCH      1

#ifndef _PLATFORMGL_H_
#define _PLATFORMGL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "platformWin32/gl_types.h"

#define GLAPI extern
#define GLAPIENTRY __stdcall

#include "platformWin32/gl_types.h"
#define GL_FUNCTION(fn_return,fn_name,fn_args,fn_value) extern fn_return (__stdcall *fn_name)fn_args; 
#include "platformX86UNIX/gl_func.h"
#include "platformX86UNIX/glu_func.h"
#undef GL_FUNCTION

/* EXT_vertex_buffer */
#define GL_V12MTVFMT_EXT                     0x8702
#define GL_V12MTNVFMT_EXT                     0x8703
#define GL_V12FTVFMT_EXT                     0x8704
#define GL_V12FMTVFMT_EXT                     0x8705

struct GLState
{
   bool suppARBMultitexture;
   bool suppPackedPixels;
   bool suppTexEnvAdd;
   bool suppLockedArrays;
   bool suppTextureEnvCombine;
   bool suppVertexArrayRange;
   bool suppFogCoord;
   bool suppEdgeClamp;
   bool suppTextureCompression;
   bool suppS3TC;
   bool suppFXT1;
   bool suppTexAnisotropic;
   bool suppPalettedTexture;
   bool suppVertexBuffer;
   bool suppSwapInterval;

#if ENABLE_NPATCH
   bool suppNPatch;
   GLint maxNPatchLevel;
#endif

   unsigned int triCount[4];
   unsigned int primCount[4];
   unsigned int primMode; // 0-3

   GLfloat maxAnisotropy;
   GLint   maxTextureUnits;
};

extern GLState gGLState;
#define UNSIGNED_SHORT_5_6_5 0x8363
#define UNSIGNED_SHORT_5_6_5_REV 0x8364

extern bool gOpenGLDisablePT;
extern bool gOpenGLDisableCVA;
extern bool gOpenGLDisableTEC;
extern bool gOpenGLDisableARBMT;
extern bool gOpenGLDisableFC;
extern bool gOpenGLDisableTCompress;
extern bool gOpenGLNoEnvColor;
extern float gOpenGLGammaCorrection;
extern bool gOpenGLNoDrawArraysAlpha;

inline void dglSetRenderPrimType(unsigned int type)
{
   gGLState.primMode = type;
}

inline void dglClearPrimMetrics()
{
   for(int i = 0; i < 4; i++)
      gGLState.triCount[i] = gGLState.primCount[i] = 0;
}

inline bool dglDoesSupportPalettedTexture()
{
   return gGLState.suppPalettedTexture && (gOpenGLDisablePT == false);
}

inline bool dglDoesSupportCompiledVertexArray() 
{
   return gGLState.suppLockedArrays && (gOpenGLDisableCVA == false);
}

inline bool dglDoesSupportTextureEnvCombine()
{
   return gGLState.suppTextureEnvCombine && (gOpenGLDisableTEC == false);
}

inline bool dglDoesSupportARBMultitexture()
{
   return gGLState.suppARBMultitexture && (gOpenGLDisableARBMT == false);
}

inline bool dglDoesSupportVertexArrayRange()
{
   return gGLState.suppVertexArrayRange;
}

inline bool dglDoesSupportFogCoord()
{
   return gGLState.suppFogCoord && (gOpenGLDisableFC == false);
}

inline bool dglDoesSupportEdgeClamp()
{
   return gGLState.suppEdgeClamp;
}

inline bool dglDoesSupportTextureCompression()
{
   return gGLState.suppTextureCompression && (gOpenGLDisableTCompress == false);
}

inline bool dglDoesSupportS3TC()
{
   return gGLState.suppS3TC;
}

inline bool dglDoesSupportFXT1()
{
   return gGLState.suppFXT1;
}

inline bool dglDoesSupportTexEnvAdd()
{
   return gGLState.suppTexEnvAdd;
}

inline bool dglDoesSupportTexAnisotropy()
{
   return gGLState.suppTexAnisotropic;
}

inline bool dglDoesSupportVertexBuffer()
{
   return gGLState.suppVertexBuffer;
}

inline GLfloat dglGetMaxAnisotropy()
{
   return gGLState.maxAnisotropy;
}

inline GLint dglGetMaxTextureUnits()
{
   if (dglDoesSupportARBMultitexture())
      return gGLState.maxTextureUnits;
   else
      return 1; 
}

#ifdef ENABLE_NPATCH

inline bool dglDoesSupportNPatch()
{
   return gGLState.suppNPatch;
}

inline GLint dglGetMaxNPatchLevel()
{
   return gGLState.maxNPatchLevel;
}

// platform-specific functions.
void dglSetNPatch(GLint npenable);
void dglSetNPatchLevel(GLint nplevel);
void dglSetNPatchInterp(GLint nppoint, GLint npnormal);

#endif // ENABLE_NPATCH


#ifdef __cplusplus
}
#endif

#endif 
