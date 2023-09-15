//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
// 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// This file is based on QGL_WIN.C written for the Quake engine by id.
//-----------------------------------------------------------------------------

#include "platformWin32/platformGL.h"
#include "platformWin32/platformWin32.h"
#include "dgl/dgl.h"

#include <time.h>
#include "console/console.h"
#include "console/consoleTypes.h"

#if ENABLE_NPATCH
#include "platformWin32/win32NPatch.h" // since it is plat-dependent
PFNNPatchSetInt glNPatchSetInt = NULL;
#endif


GLState gGLState;

bool gOpenGLDisablePT			= false;
bool gOpenGLDisableCVA			= false;
bool gOpenGLDisableTEC			= false;
bool gOpenGLDisableARBMT		= false;
bool gOpenGLDisableFC			= false;
bool gOpenGLDisableTCompress	= false;
bool gOpenGLNoEnvColor			= false;
float gOpenGLGammaCorrection	= 0.5;
bool gOpenGLNoDrawArraysAlpha	= false;

typedef int ( WINAPI * qwglSwapIntervalEXT_t )(int interval );
qwglSwapIntervalEXT_t qwglSwapIntervalEXT;
typedef BOOL  ( WINAPI * qwglGetDeviceGammaRamp3DFX_t )(HDC, LPVOID );
qwglGetDeviceGammaRamp3DFX_t qwglGetDeviceGammaRamp3DFX;
typedef BOOL  ( WINAPI * qwglSetDeviceGammaRamp3DFX_t )(HDC, LPVOID );
qwglSetDeviceGammaRamp3DFX_t qwglSetDeviceGammaRamp3DFX;
typedef int   ( WINAPI * qwglChoosePixelFormat_t )(HDC, CONST PIXELFORMATDESCRIPTOR *);
qwglChoosePixelFormat_t qwglChoosePixelFormat;
typedef int   ( WINAPI * qwglDescribePixelFormat_t) (HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
qwglDescribePixelFormat_t qwglDescribePixelFormat;
typedef int   ( WINAPI * qwglGetPixelFormat_t )(HDC);
qwglGetPixelFormat_t qwglGetPixelFormat;
typedef BOOL  ( WINAPI * qwglSetPixelFormat_t )(HDC, int, CONST PIXELFORMATDESCRIPTOR *);
qwglSetPixelFormat_t qwglSetPixelFormat;
typedef BOOL  ( WINAPI * qwglSwapBuffers_t )(HDC);
qwglSwapBuffers_t qwglSwapBuffers;
qwglSwapBuffers_t dllSwapBuffers;
typedef BOOL  ( WINAPI * qwglCopyContext_t )(HGLRC, HGLRC, UINT);
qwglCopyContext_t qwglCopyContext;
typedef HGLRC ( WINAPI * qwglCreateContext_t )(HDC);
qwglCreateContext_t qwglCreateContext;
typedef HGLRC ( WINAPI * qwglCreateLayerContext_t )(HDC, int);
qwglCreateLayerContext_t qwglCreateLayerContext;
typedef BOOL  ( WINAPI * qwglDeleteContext_t )(HGLRC);
qwglDeleteContext_t qwglDeleteContext;
typedef HGLRC ( WINAPI * qwglGetCurrentContext_t )(VOID);
qwglGetCurrentContext_t qwglGetCurrentContext;
typedef HDC   ( WINAPI * qwglGetCurrentDC_t )(VOID);
qwglGetCurrentDC_t qwglGetCurrentDC;
typedef PROC  ( WINAPI * qwglGetProcAddress_t )(LPCSTR);
qwglGetProcAddress_t qwglGetProcAddress;
typedef BOOL  ( WINAPI * qwglMakeCurrent_t )(HDC, HGLRC);
qwglMakeCurrent_t qwglMakeCurrent;
typedef BOOL  ( WINAPI * qwglShareLists_t )(HGLRC, HGLRC);
qwglShareLists_t qwglShareLists;
typedef BOOL  ( WINAPI * qwglUseFontBitmaps_t )(HDC, DWORD, DWORD, DWORD);
qwglUseFontBitmaps_t qwglUseFontBitmaps;
typedef BOOL  ( WINAPI * qwglUseFontOutlines_t )(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT);
qwglUseFontOutlines_t qwglUseFontOutlines;
typedef BOOL ( WINAPI * qwglDescribeLayerPlane_t )(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR);
qwglDescribeLayerPlane_t qwglDescribeLayerPlane;
typedef int  ( WINAPI * qwglSetLayerPaletteEntries_t )(HDC, int, int, int, CONST COLORREF *);
qwglSetLayerPaletteEntries_t qwglSetLayerPaletteEntries;
typedef int  ( WINAPI * qwglGetLayerPaletteEntries_t )(HDC, int, int, int, COLORREF *);
qwglGetLayerPaletteEntries_t qwglGetLayerPaletteEntries;
typedef BOOL ( WINAPI * qwglRealizeLayerPalette_t )(HDC, int, BOOL);
qwglRealizeLayerPalette_t qwglRealizeLayerPalette;
typedef BOOL ( WINAPI * qwglSwapLayerBuffers_t )(HDC, UINT);
qwglSwapLayerBuffers_t qwglSwapLayerBuffers;


//------------------------------------------------------------------------------
// GLU Functions
typedef const GLubyte* (APIENTRY * gluErrorString_t) (GLenum errCode);
gluErrorString_t gluErrorString;
typedef const GLubyte* (APIENTRY * gluGetString_t) (GLenum name);
gluGetString_t gluGetString;
typedef void (APIENTRY * gluOrtho2D_t) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
gluOrtho2D_t gluOrtho2D;
typedef void (APIENTRY * gluPerspective_t) (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
gluPerspective_t gluPerspective;
typedef void (APIENTRY * gluPickMatrix_t) (GLdouble x, GLdouble y, GLdouble width, GLdouble height, GLint viewport[4]);
gluPickMatrix_t gluPickMatrix;
typedef void (APIENTRY * gluLookAt_t) (GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz);
gluLookAt_t gluLookAt;
typedef int (APIENTRY * gluProject_t) (GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz);
gluProject_t gluProject;
typedef int (APIENTRY * gluUnProject_t) (GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz);
gluUnProject_t gluUnProject;
typedef int (APIENTRY * gluScaleImage_t) (GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain, GLint widthout, GLint heightout, GLenum typeout, void *dataout);
gluScaleImage_t gluScaleImage;
typedef int (APIENTRY * gluBuild1DMipmaps_t) (GLenum target, GLint components, GLint width, GLenum format, GLenum type, const void *data);
gluBuild1DMipmaps_t gluBuild1DMipmaps;
typedef int (APIENTRY * gluBuild2DMipmaps_t) (GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void *data);
gluBuild2DMipmaps_t gluBuild2DMipmaps;

//------------------------------------------------------------------------------
// GL Functions
typedef void ( APIENTRY * glAccum_t )(GLenum op, GLfloat value);
glAccum_t glAccum;
typedef void ( APIENTRY * glAlphaFunc_t )(GLenum func, GLclampf ref);
glAlphaFunc_t glAlphaFunc;
typedef GLboolean ( APIENTRY * glAreTexturesResident_t )(GLsizei n, const GLuint *textures, GLboolean *residences);
glAreTexturesResident_t glAreTexturesResident;
typedef void ( APIENTRY * glArrayElement_t )(GLint i);
glArrayElement_t glArrayElement;
typedef void ( APIENTRY * glBegin_t )(GLenum mode);
glBegin_t glBegin;
typedef void ( APIENTRY * glBindTexture_t )(GLenum target, GLuint texture);
glBindTexture_t glBindTexture;
typedef void ( APIENTRY * glBitmap_t )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
glBitmap_t glBitmap;
typedef void ( APIENTRY * glBlendFunc_t )(GLenum sfactor, GLenum dfactor);
glBlendFunc_t glBlendFunc;
typedef void ( APIENTRY * glCallList_t )(GLuint list);
glCallList_t glCallList;
typedef void ( APIENTRY * glCallLists_t )(GLsizei n, GLenum type, const GLvoid *lists);
glCallLists_t glCallLists;
typedef void ( APIENTRY * glClear_t )(GLbitfield mask);
glClear_t glClear;
typedef void ( APIENTRY * glClearAccum_t )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
glClearAccum_t glClearAccum;
typedef void ( APIENTRY * glClearColor_t )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
glClearColor_t glClearColor;
typedef void ( APIENTRY * glClearDepth_t )(GLclampd depth);
glClearDepth_t glClearDepth;
typedef void ( APIENTRY * glClearIndex_t )(GLfloat c);
glClearIndex_t glClearIndex;
typedef void ( APIENTRY * glClearStencil_t )(GLint s);
glClearStencil_t glClearStencil;
typedef void ( APIENTRY * glClipPlane_t )(GLenum plane, const GLdouble *equation);
glClipPlane_t glClipPlane;
typedef void ( APIENTRY * glColor3b_t )(GLbyte red, GLbyte green, GLbyte blue);
glColor3b_t glColor3b;
typedef void ( APIENTRY * glColor3bv_t )(const GLbyte *v);
glColor3bv_t glColor3bv;
typedef void ( APIENTRY * glColor3d_t )(GLdouble red, GLdouble green, GLdouble blue);
glColor3d_t glColor3d;
typedef void ( APIENTRY * glColor3dv_t )(const GLdouble *v);
glColor3dv_t glColor3dv;
typedef void ( APIENTRY * glColor3f_t )(GLfloat red, GLfloat green, GLfloat blue);
glColor3f_t glColor3f;
typedef void ( APIENTRY * glColor3fv_t )(const GLfloat *v);
glColor3fv_t glColor3fv;
typedef void ( APIENTRY * glColor3i_t )(GLint red, GLint green, GLint blue);
glColor3i_t glColor3i;
typedef void ( APIENTRY * glColor3iv_t )(const GLint *v);
glColor3iv_t glColor3iv;
typedef void ( APIENTRY * glColor3s_t )(GLshort red, GLshort green, GLshort blue);
glColor3s_t glColor3s;
typedef void ( APIENTRY * glColor3sv_t )(const GLshort *v);
glColor3sv_t glColor3sv;
typedef void ( APIENTRY * glColor3ub_t )(GLubyte red, GLubyte green, GLubyte blue);
glColor3ub_t glColor3ub;
typedef void ( APIENTRY * glColor3ubv_t )(const GLubyte *v);
glColor3ubv_t glColor3ubv;
typedef void ( APIENTRY * glColor3ui_t )(GLuint red, GLuint green, GLuint blue);
glColor3ui_t glColor3ui;
typedef void ( APIENTRY * glColor3uiv_t )(const GLuint *v);
glColor3uiv_t glColor3uiv;
typedef void ( APIENTRY * glColor3us_t )(GLushort red, GLushort green, GLushort blue);
glColor3us_t glColor3us;
typedef void ( APIENTRY * glColor3usv_t )(const GLushort *v);
glColor3usv_t glColor3usv;
typedef void ( APIENTRY * glColor4b_t )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
glColor4b_t glColor4b;
typedef void ( APIENTRY * glColor4bv_t )(const GLbyte *v);
glColor4bv_t glColor4bv;
typedef void ( APIENTRY * glColor4d_t )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
glColor4d_t glColor4d;
typedef void ( APIENTRY * glColor4dv_t )(const GLdouble *v);
glColor4dv_t glColor4dv;
typedef void ( APIENTRY * glColor4f_t )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
glColor4f_t glColor4f;
typedef void ( APIENTRY * glColor4fv_t )(const GLfloat *v);
glColor4fv_t glColor4fv;
typedef void ( APIENTRY * glColor4i_t )(GLint red, GLint green, GLint blue, GLint alpha);
glColor4i_t glColor4i;
typedef void ( APIENTRY * glColor4iv_t )(const GLint *v);
glColor4iv_t glColor4iv;
typedef void ( APIENTRY * glColor4s_t )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
glColor4s_t glColor4s;
typedef void ( APIENTRY * glColor4sv_t )(const GLshort *v);
glColor4sv_t glColor4sv;
typedef void ( APIENTRY * glColor4ub_t )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
glColor4ub_t glColor4ub;
typedef void ( APIENTRY * glColor4ubv_t )(const GLubyte *v);
glColor4ubv_t glColor4ubv;
typedef void ( APIENTRY * glColor4ui_t )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
glColor4ui_t glColor4ui;
typedef void ( APIENTRY * glColor4uiv_t )(const GLuint *v);
glColor4uiv_t glColor4uiv;
typedef void ( APIENTRY * glColor4us_t )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
glColor4us_t glColor4us;
typedef void ( APIENTRY * glColor4usv_t )(const GLushort *v);
glColor4usv_t glColor4usv;
typedef void ( APIENTRY * glColorMask_t )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
glColorMask_t glColorMask;
typedef void ( APIENTRY * glColorMaterial_t )(GLenum face, GLenum mode);
glColorMaterial_t glColorMaterial;
typedef void ( APIENTRY * glColorPointer_t )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
glColorPointer_t glColorPointer;
typedef void ( APIENTRY * glCopyPixels_t )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
glCopyPixels_t glCopyPixels;
typedef void ( APIENTRY * glCopyTexImage1D_t )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
glCopyTexImage1D_t glCopyTexImage1D;
typedef void ( APIENTRY * glCopyTexImage2D_t )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
glCopyTexImage2D_t glCopyTexImage2D;
typedef void ( APIENTRY * glCopyTexSubImage1D_t )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
glCopyTexSubImage1D_t glCopyTexSubImage1D;
typedef void ( APIENTRY * glCopyTexSubImage2D_t )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
glCopyTexSubImage2D_t glCopyTexSubImage2D;
typedef void ( APIENTRY * glCullFace_t )(GLenum mode);
glCullFace_t glCullFace;
typedef void ( APIENTRY * glDeleteLists_t )(GLuint list, GLsizei range);
glDeleteLists_t glDeleteLists;
typedef void ( APIENTRY * glDeleteTextures_t )(GLsizei n, const GLuint *textures);
glDeleteTextures_t glDeleteTextures;
typedef void ( APIENTRY * glDepthFunc_t )(GLenum func);
glDepthFunc_t glDepthFunc;
typedef void ( APIENTRY * glDepthMask_t )(GLboolean flag);
glDepthMask_t glDepthMask;
typedef void ( APIENTRY * glDepthRange_t )(GLclampd zNear, GLclampd zFar);
glDepthRange_t glDepthRange;
typedef void ( APIENTRY * glDisable_t )(GLenum cap);
glDisable_t glDisable;
typedef void ( APIENTRY * glDisableClientState_t )(GLenum array);
glDisableClientState_t glDisableClientState;
typedef void ( APIENTRY * glDrawArrays_t )(GLenum mode, GLint first, GLsizei count);
glDrawArrays_t glDrawArrays;
typedef void ( APIENTRY * glDrawBuffer_t )(GLenum mode);
glDrawBuffer_t glDrawBuffer;
typedef void ( APIENTRY * glDrawElements_t )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
glDrawElements_t glDrawElements;
typedef void ( APIENTRY * glDrawPixels_t )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
glDrawPixels_t glDrawPixels;
typedef void ( APIENTRY * glEdgeFlag_t )(GLboolean flag);
glEdgeFlag_t glEdgeFlag;
typedef void ( APIENTRY * glEdgeFlagPointer_t )(GLsizei stride, const GLvoid *pointer);
glEdgeFlagPointer_t glEdgeFlagPointer;
typedef void ( APIENTRY * glEdgeFlagv_t )(const GLboolean *flag);
glEdgeFlagv_t glEdgeFlagv;
typedef void ( APIENTRY * glEnable_t )(GLenum cap);
glEnable_t glEnable;
typedef void ( APIENTRY * glEnableClientState_t )(GLenum array);
glEnableClientState_t glEnableClientState;
typedef void ( APIENTRY * glEnd_t )(void);
glEnd_t glEnd;
typedef void ( APIENTRY * glEndList_t )(void);
glEndList_t glEndList;
typedef void ( APIENTRY * glEvalCoord1d_t )(GLdouble u);
glEvalCoord1d_t glEvalCoord1d;
typedef void ( APIENTRY * glEvalCoord1dv_t )(const GLdouble *u);
glEvalCoord1dv_t glEvalCoord1dv;
typedef void ( APIENTRY * glEvalCoord1f_t )(GLfloat u);
glEvalCoord1f_t glEvalCoord1f;
typedef void ( APIENTRY * glEvalCoord1fv_t )(const GLfloat *u);
glEvalCoord1fv_t glEvalCoord1fv;
typedef void ( APIENTRY * glEvalCoord2d_t )(GLdouble u, GLdouble v);
glEvalCoord2d_t glEvalCoord2d;
typedef void ( APIENTRY * glEvalCoord2dv_t )(const GLdouble *u);
glEvalCoord2dv_t glEvalCoord2dv;
typedef void ( APIENTRY * glEvalCoord2f_t )(GLfloat u, GLfloat v);
glEvalCoord2f_t glEvalCoord2f;
typedef void ( APIENTRY * glEvalCoord2fv_t )(const GLfloat *u);
glEvalCoord2fv_t glEvalCoord2fv;
typedef void ( APIENTRY * glEvalMesh1_t )(GLenum mode, GLint i1, GLint i2);
glEvalMesh1_t glEvalMesh1;
typedef void ( APIENTRY * glEvalMesh2_t )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
glEvalMesh2_t glEvalMesh2;
typedef void ( APIENTRY * glEvalPoint1_t )(GLint i);
glEvalPoint1_t glEvalPoint1;
typedef void ( APIENTRY * glEvalPoint2_t )(GLint i, GLint j);
glEvalPoint2_t glEvalPoint2;
typedef void ( APIENTRY * glFeedbackBuffer_t )(GLsizei size, GLenum type, GLfloat *buffer);
glFeedbackBuffer_t glFeedbackBuffer;
typedef void ( APIENTRY * glFinish_t )(void);
glFinish_t glFinish;
typedef void ( APIENTRY * glFlush_t )(void);
glFlush_t glFlush;
typedef void ( APIENTRY * glFogf_t )(GLenum pname, GLfloat param);
glFogf_t glFogf;
typedef void ( APIENTRY * glFogfv_t )(GLenum pname, const GLfloat *params);
glFogfv_t glFogfv;
typedef void ( APIENTRY * glFogi_t )(GLenum pname, GLint param);
glFogi_t glFogi;
typedef void ( APIENTRY * glFogiv_t )(GLenum pname, const GLint *params);
glFogiv_t glFogiv;
typedef void ( APIENTRY * glFrontFace_t )(GLenum mode);
glFrontFace_t glFrontFace;
typedef void ( APIENTRY * glFrustum_t )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
glFrustum_t glFrustum;
typedef GLuint ( APIENTRY * glGenLists_t )(GLsizei range);
glGenLists_t glGenLists;
typedef void ( APIENTRY * glGenTextures_t )(GLsizei n, GLuint *textures);
glGenTextures_t glGenTextures;
typedef void ( APIENTRY * glGetBooleanv_t )(GLenum pname, GLboolean *params);
glGetBooleanv_t glGetBooleanv;
typedef void ( APIENTRY * glGetClipPlane_t )(GLenum plane, GLdouble *equation);
glGetClipPlane_t glGetClipPlane;
typedef void ( APIENTRY * glGetDoublev_t )(GLenum pname, GLdouble *params);
glGetDoublev_t glGetDoublev;
typedef GLenum ( APIENTRY * glGetError_t )(void);
glGetError_t glGetError;
typedef void ( APIENTRY * glGetFloatv_t )(GLenum pname, GLfloat *params);
glGetFloatv_t glGetFloatv;
typedef void ( APIENTRY * glGetIntegerv_t )(GLenum pname, GLint *params);
glGetIntegerv_t glGetIntegerv;
typedef void ( APIENTRY * glGetLightfv_t )(GLenum light, GLenum pname, GLfloat *params);
glGetLightfv_t glGetLightfv;
typedef void ( APIENTRY * glGetLightiv_t )(GLenum light, GLenum pname, GLint *params);
glGetLightiv_t glGetLightiv;
typedef void ( APIENTRY * glGetMapdv_t )(GLenum target, GLenum query, GLdouble *v);
glGetMapdv_t glGetMapdv;
typedef void ( APIENTRY * glGetMapfv_t )(GLenum target, GLenum query, GLfloat *v);
glGetMapfv_t glGetMapfv;
typedef void ( APIENTRY * glGetMapiv_t )(GLenum target, GLenum query, GLint *v);
glGetMapiv_t glGetMapiv;
typedef void ( APIENTRY * glGetMaterialfv_t )(GLenum face, GLenum pname, GLfloat *params);
glGetMaterialfv_t glGetMaterialfv;
typedef void ( APIENTRY * glGetMaterialiv_t )(GLenum face, GLenum pname, GLint *params);
glGetMaterialiv_t glGetMaterialiv;
typedef void ( APIENTRY * glGetPixelMapfv_t )(GLenum map, GLfloat *values);
glGetPixelMapfv_t glGetPixelMapfv;
typedef void ( APIENTRY * glGetPixelMapuiv_t )(GLenum map, GLuint *values);
glGetPixelMapuiv_t glGetPixelMapuiv;
typedef void ( APIENTRY * glGetPixelMapusv_t )(GLenum map, GLushort *values);
glGetPixelMapusv_t glGetPixelMapusv;
typedef void ( APIENTRY * glGetPointerv_t )(GLenum pname, GLvoid* *params);
glGetPointerv_t glGetPointerv;
typedef void ( APIENTRY * glGetPolygonStipple_t )(GLubyte *mask);
glGetPolygonStipple_t glGetPolygonStipple;
typedef const GLubyte * ( APIENTRY * glGetString_t )(GLenum name);
glGetString_t glGetString;
typedef void ( APIENTRY * glGetTexEnvfv_t )(GLenum target, GLenum pname, GLfloat *params);
glGetTexEnvfv_t glGetTexEnvfv;
typedef void ( APIENTRY * glGetTexEnviv_t )(GLenum target, GLenum pname, GLint *params);
glGetTexEnviv_t glGetTexEnviv;
typedef void ( APIENTRY * glGetTexGendv_t )(GLenum coord, GLenum pname, GLdouble *params);
glGetTexGendv_t glGetTexGendv;
typedef void ( APIENTRY * glGetTexGenfv_t )(GLenum coord, GLenum pname, GLfloat *params);
glGetTexGenfv_t glGetTexGenfv;
typedef void ( APIENTRY * glGetTexGeniv_t )(GLenum coord, GLenum pname, GLint *params);
glGetTexGeniv_t glGetTexGeniv;
typedef void ( APIENTRY * glGetTexImage_t )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
glGetTexImage_t glGetTexImage;
typedef void ( APIENTRY * glGetTexLevelParameterfv_t )(GLenum target, GLint level, GLenum pname, GLfloat *params);
glGetTexLevelParameterfv_t glGetTexLevelParameterfv;
typedef void ( APIENTRY * glGetTexLevelParameteriv_t )(GLenum target, GLint level, GLenum pname, GLint *params);
glGetTexLevelParameteriv_t glGetTexLevelParameteriv;
typedef void ( APIENTRY * glGetTexParameterfv_t )(GLenum target, GLenum pname, GLfloat *params);
glGetTexParameterfv_t glGetTexParameterfv;
typedef void ( APIENTRY * glGetTexParameteriv_t )(GLenum target, GLenum pname, GLint *params);
glGetTexParameteriv_t glGetTexParameteriv;
typedef void ( APIENTRY * glHint_t )(GLenum target, GLenum mode);
glHint_t glHint;
typedef void ( APIENTRY * glIndexMask_t )(GLuint mask);
glIndexMask_t glIndexMask;
typedef void ( APIENTRY * glIndexPointer_t )(GLenum type, GLsizei stride, const GLvoid *pointer);
glIndexPointer_t glIndexPointer;
typedef void ( APIENTRY * glIndexd_t )(GLdouble c);
glIndexd_t glIndexd;
typedef void ( APIENTRY * glIndexdv_t )(const GLdouble *c);
glIndexdv_t glIndexdv;
typedef void ( APIENTRY * glIndexf_t )(GLfloat c);
glIndexf_t glIndexf;
typedef void ( APIENTRY * glIndexfv_t )(const GLfloat *c);
glIndexfv_t glIndexfv;
typedef void ( APIENTRY * glIndexi_t )(GLint c);
glIndexi_t glIndexi;
typedef void ( APIENTRY * glIndexiv_t )(const GLint *c);
glIndexiv_t glIndexiv;
typedef void ( APIENTRY * glIndexs_t )(GLshort c);
glIndexs_t glIndexs;
typedef void ( APIENTRY * glIndexsv_t )(const GLshort *c);
glIndexsv_t glIndexsv;
typedef void ( APIENTRY * glIndexub_t )(GLubyte c);
glIndexub_t glIndexub;
typedef void ( APIENTRY * glIndexubv_t )(const GLubyte *c);
glIndexubv_t glIndexubv;
typedef void ( APIENTRY * glInitNames_t )(void);
glInitNames_t glInitNames;
typedef void ( APIENTRY * glInterleavedArrays_t )(GLenum format, GLsizei stride, const GLvoid *pointer);
glInterleavedArrays_t glInterleavedArrays;
typedef GLboolean ( APIENTRY * glIsEnabled_t )(GLenum cap);
glIsEnabled_t glIsEnabled;
typedef GLboolean ( APIENTRY * glIsList_t )(GLuint list);
glIsList_t glIsList;
typedef GLboolean ( APIENTRY * glIsTexture_t )(GLuint texture);
glIsTexture_t glIsTexture;
typedef void ( APIENTRY * glLightModelf_t )(GLenum pname, GLfloat param);
glLightModelf_t glLightModelf;
typedef void ( APIENTRY * glLightModelfv_t )(GLenum pname, const GLfloat *params);
glLightModelfv_t glLightModelfv;
typedef void ( APIENTRY * glLightModeli_t )(GLenum pname, GLint param);
glLightModeli_t glLightModeli;
typedef void ( APIENTRY * glLightModeliv_t )(GLenum pname, const GLint *params);
glLightModeliv_t glLightModeliv;
typedef void ( APIENTRY * glLightf_t )(GLenum light, GLenum pname, GLfloat param);
glLightf_t glLightf;
typedef void ( APIENTRY * glLightfv_t )(GLenum light, GLenum pname, const GLfloat *params);
glLightfv_t glLightfv;
typedef void ( APIENTRY * glLighti_t )(GLenum light, GLenum pname, GLint param);
glLighti_t glLighti;
typedef void ( APIENTRY * glLightiv_t )(GLenum light, GLenum pname, const GLint *params);
glLightiv_t glLightiv;
typedef void ( APIENTRY * glLineStipple_t )(GLint factor, GLushort pattern);
glLineStipple_t glLineStipple;
typedef void ( APIENTRY * glLineWidth_t )(GLfloat width);
glLineWidth_t glLineWidth;
typedef void ( APIENTRY * glListBase_t )(GLuint base);
glListBase_t glListBase;
typedef void ( APIENTRY * glLoadIdentity_t )(void);
glLoadIdentity_t glLoadIdentity;
typedef void ( APIENTRY * glLoadMatrixd_t )(const GLdouble *m);
glLoadMatrixd_t glLoadMatrixd;
typedef void ( APIENTRY * glLoadMatrixf_t )(const GLfloat *m);
glLoadMatrixf_t glLoadMatrixf;
typedef void ( APIENTRY * glLoadName_t )(GLuint name);
glLoadName_t glLoadName;
typedef void ( APIENTRY * glLogicOp_t )(GLenum opcode);
glLogicOp_t glLogicOp;
typedef void ( APIENTRY * glMap1d_t )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
glMap1d_t glMap1d;
typedef void ( APIENTRY * glMap1f_t )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
glMap1f_t glMap1f;
typedef void ( APIENTRY * glMap2d_t )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
glMap2d_t glMap2d;
typedef void ( APIENTRY * glMap2f_t )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
glMap2f_t glMap2f;
typedef void ( APIENTRY * glMapGrid1d_t )(GLint un, GLdouble u1, GLdouble u2);
glMapGrid1d_t glMapGrid1d;
typedef void ( APIENTRY * glMapGrid1f_t )(GLint un, GLfloat u1, GLfloat u2);
glMapGrid1f_t glMapGrid1f;
typedef void ( APIENTRY * glMapGrid2d_t )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
glMapGrid2d_t glMapGrid2d;
typedef void ( APIENTRY * glMapGrid2f_t )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
glMapGrid2f_t glMapGrid2f;
typedef void ( APIENTRY * glMaterialf_t )(GLenum face, GLenum pname, GLfloat param);
glMaterialf_t glMaterialf;
typedef void ( APIENTRY * glMaterialfv_t )(GLenum face, GLenum pname, const GLfloat *params);
glMaterialfv_t glMaterialfv;
typedef void ( APIENTRY * glMateriali_t )(GLenum face, GLenum pname, GLint param);
glMateriali_t glMateriali;
typedef void ( APIENTRY * glMaterialiv_t )(GLenum face, GLenum pname, const GLint *params);
glMaterialiv_t glMaterialiv;
typedef void ( APIENTRY * glMatrixMode_t )(GLenum mode);
glMatrixMode_t glMatrixMode;
typedef void ( APIENTRY * glMultMatrixd_t )(const GLdouble *m);
glMultMatrixd_t glMultMatrixd;
typedef void ( APIENTRY * glMultMatrixf_t )(const GLfloat *m);
glMultMatrixf_t glMultMatrixf;
typedef void ( APIENTRY * glNewList_t )(GLuint list, GLenum mode);
glNewList_t glNewList;
typedef void ( APIENTRY * glNormal3b_t )(GLbyte nx, GLbyte ny, GLbyte nz);
glNormal3b_t glNormal3b;
typedef void ( APIENTRY * glNormal3bv_t )(const GLbyte *v);
glNormal3bv_t glNormal3bv;
typedef void ( APIENTRY * glNormal3d_t )(GLdouble nx, GLdouble ny, GLdouble nz);
glNormal3d_t glNormal3d;
typedef void ( APIENTRY * glNormal3dv_t )(const GLdouble *v);
glNormal3dv_t glNormal3dv;
typedef void ( APIENTRY * glNormal3f_t )(GLfloat nx, GLfloat ny, GLfloat nz);
glNormal3f_t glNormal3f;
typedef void ( APIENTRY * glNormal3fv_t )(const GLfloat *v);
glNormal3fv_t glNormal3fv;
typedef void ( APIENTRY * glNormal3i_t )(GLint nx, GLint ny, GLint nz);
glNormal3i_t glNormal3i;
typedef void ( APIENTRY * glNormal3iv_t )(const GLint *v);
glNormal3iv_t glNormal3iv;
typedef void ( APIENTRY * glNormal3s_t )(GLshort nx, GLshort ny, GLshort nz);
glNormal3s_t glNormal3s;
typedef void ( APIENTRY * glNormal3sv_t )(const GLshort *v);
glNormal3sv_t glNormal3sv;
typedef void ( APIENTRY * glNormalPointer_t )(GLenum type, GLsizei stride, const GLvoid *pointer);
glNormalPointer_t glNormalPointer;
typedef void ( APIENTRY * glOrtho_t )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
glOrtho_t glOrtho;
typedef void ( APIENTRY * glPassThrough_t )(GLfloat token);
glPassThrough_t glPassThrough;
typedef void ( APIENTRY * glPixelMapfv_t )(GLenum map, GLsizei mapsize, const GLfloat *values);
glPixelMapfv_t glPixelMapfv;
typedef void ( APIENTRY * glPixelMapuiv_t )(GLenum map, GLsizei mapsize, const GLuint *values);
glPixelMapuiv_t glPixelMapuiv;
typedef void ( APIENTRY * glPixelMapusv_t )(GLenum map, GLsizei mapsize, const GLushort *values);
glPixelMapusv_t glPixelMapusv;
typedef void ( APIENTRY * glPixelStoref_t )(GLenum pname, GLfloat param);
glPixelStoref_t glPixelStoref;
typedef void ( APIENTRY * glPixelStorei_t )(GLenum pname, GLint param);
glPixelStorei_t glPixelStorei;
typedef void ( APIENTRY * glPixelTransferf_t )(GLenum pname, GLfloat param);
glPixelTransferf_t glPixelTransferf;
typedef void ( APIENTRY * glPixelTransferi_t )(GLenum pname, GLint param);
glPixelTransferi_t glPixelTransferi;
typedef void ( APIENTRY * glPixelZoom_t )(GLfloat xfactor, GLfloat yfactor);
glPixelZoom_t glPixelZoom;
typedef void ( APIENTRY * glPointSize_t )(GLfloat size);
glPointSize_t glPointSize;
typedef void ( APIENTRY * glPolygonMode_t )(GLenum face, GLenum mode);
glPolygonMode_t glPolygonMode;
typedef void ( APIENTRY * glPolygonOffset_t )(GLfloat factor, GLfloat units);
glPolygonOffset_t glPolygonOffset;
typedef void ( APIENTRY * glPolygonStipple_t )(const GLubyte *mask);
glPolygonStipple_t glPolygonStipple;
typedef void ( APIENTRY * glPopAttrib_t )(void);
glPopAttrib_t glPopAttrib;
typedef void ( APIENTRY * glPopClientAttrib_t )(void);
glPopClientAttrib_t glPopClientAttrib;
typedef void ( APIENTRY * glPopMatrix_t )(void);
glPopMatrix_t glPopMatrix;
typedef void ( APIENTRY * glPopName_t )(void);
glPopName_t glPopName;
typedef void ( APIENTRY * glPrioritizeTextures_t )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
glPrioritizeTextures_t glPrioritizeTextures;
typedef void ( APIENTRY * glPushAttrib_t )(GLbitfield mask);
glPushAttrib_t glPushAttrib;
typedef void ( APIENTRY * glPushClientAttrib_t )(GLbitfield mask);
glPushClientAttrib_t glPushClientAttrib;
typedef void ( APIENTRY * glPushMatrix_t )(void);
glPushMatrix_t glPushMatrix;
typedef void ( APIENTRY * glPushName_t )(GLuint name);
glPushName_t glPushName;
typedef void ( APIENTRY * glRasterPos2d_t )(GLdouble x, GLdouble y);
glRasterPos2d_t glRasterPos2d;
typedef void ( APIENTRY * glRasterPos2dv_t )(const GLdouble *v);
glRasterPos2dv_t glRasterPos2dv;
typedef void ( APIENTRY * glRasterPos2f_t )(GLfloat x, GLfloat y);
glRasterPos2f_t glRasterPos2f;
typedef void ( APIENTRY * glRasterPos2fv_t )(const GLfloat *v);
glRasterPos2fv_t glRasterPos2fv;
typedef void ( APIENTRY * glRasterPos2i_t )(GLint x, GLint y);
glRasterPos2i_t glRasterPos2i;
typedef void ( APIENTRY * glRasterPos2iv_t )(const GLint *v);
glRasterPos2iv_t glRasterPos2iv;
typedef void ( APIENTRY * glRasterPos2s_t )(GLshort x, GLshort y);
glRasterPos2s_t glRasterPos2s;
typedef void ( APIENTRY * glRasterPos2sv_t )(const GLshort *v);
glRasterPos2sv_t glRasterPos2sv;
typedef void ( APIENTRY * glRasterPos3d_t )(GLdouble x, GLdouble y, GLdouble z);
glRasterPos3d_t glRasterPos3d;
typedef void ( APIENTRY * glRasterPos3dv_t )(const GLdouble *v);
glRasterPos3dv_t glRasterPos3dv;
typedef void ( APIENTRY * glRasterPos3f_t )(GLfloat x, GLfloat y, GLfloat z);
glRasterPos3f_t glRasterPos3f;
typedef void ( APIENTRY * glRasterPos3fv_t )(const GLfloat *v);
glRasterPos3fv_t glRasterPos3fv;
typedef void ( APIENTRY * glRasterPos3i_t )(GLint x, GLint y, GLint z);
glRasterPos3i_t glRasterPos3i;
typedef void ( APIENTRY * glRasterPos3iv_t )(const GLint *v);
glRasterPos3iv_t glRasterPos3iv;
typedef void ( APIENTRY * glRasterPos3s_t )(GLshort x, GLshort y, GLshort z);
glRasterPos3s_t glRasterPos3s;
typedef void ( APIENTRY * glRasterPos3sv_t )(const GLshort *v);
glRasterPos3sv_t glRasterPos3sv;
typedef void ( APIENTRY * glRasterPos4d_t )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
glRasterPos4d_t glRasterPos4d;
typedef void ( APIENTRY * glRasterPos4dv_t )(const GLdouble *v);
glRasterPos4dv_t glRasterPos4dv;
typedef void ( APIENTRY * glRasterPos4f_t )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
glRasterPos4f_t glRasterPos4f;
typedef void ( APIENTRY * glRasterPos4fv_t )(const GLfloat *v);
glRasterPos4fv_t glRasterPos4fv;
typedef void ( APIENTRY * glRasterPos4i_t )(GLint x, GLint y, GLint z, GLint w);
glRasterPos4i_t glRasterPos4i;
typedef void ( APIENTRY * glRasterPos4iv_t )(const GLint *v);
glRasterPos4iv_t glRasterPos4iv;
typedef void ( APIENTRY * glRasterPos4s_t )(GLshort x, GLshort y, GLshort z, GLshort w);
glRasterPos4s_t glRasterPos4s;
typedef void ( APIENTRY * glRasterPos4sv_t )(const GLshort *v);
glRasterPos4sv_t glRasterPos4sv;
typedef void ( APIENTRY * glReadBuffer_t )(GLenum mode);
glReadBuffer_t glReadBuffer;
typedef void ( APIENTRY * glReadPixels_t )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
glReadPixels_t glReadPixels;
typedef void ( APIENTRY * glRectd_t )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
glRectd_t glRectd;
typedef void ( APIENTRY * glRectdv_t )(const GLdouble *v1, const GLdouble *v2);
glRectdv_t glRectdv;
typedef void ( APIENTRY * glRectf_t )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
glRectf_t glRectf;
typedef void ( APIENTRY * glRectfv_t )(const GLfloat *v1, const GLfloat *v2);
glRectfv_t glRectfv;
typedef void ( APIENTRY * glRecti_t )(GLint x1, GLint y1, GLint x2, GLint y2);
glRecti_t glRecti;
typedef void ( APIENTRY * glRectiv_t )(const GLint *v1, const GLint *v2);
glRectiv_t glRectiv;
typedef void ( APIENTRY * glRects_t )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
glRects_t glRects;
typedef void ( APIENTRY * glRectsv_t )(const GLshort *v1, const GLshort *v2);
glRectsv_t glRectsv;
typedef GLint ( APIENTRY * glRenderMode_t )(GLenum mode);
glRenderMode_t glRenderMode;
typedef void ( APIENTRY * glRotated_t )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
glRotated_t glRotated;
typedef void ( APIENTRY * glRotatef_t )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
glRotatef_t glRotatef;
typedef void ( APIENTRY * glScaled_t )(GLdouble x, GLdouble y, GLdouble z);
glScaled_t glScaled;
typedef void ( APIENTRY * glScalef_t )(GLfloat x, GLfloat y, GLfloat z);
glScalef_t glScalef;
typedef void ( APIENTRY * glScissor_t )(GLint x, GLint y, GLsizei width, GLsizei height);
glScissor_t glScissor;
typedef void ( APIENTRY * glSelectBuffer_t )(GLsizei size, GLuint *buffer);
glSelectBuffer_t glSelectBuffer;
typedef void ( APIENTRY * glShadeModel_t )(GLenum mode);
glShadeModel_t glShadeModel;
typedef void ( APIENTRY * glStencilFunc_t )(GLenum func, GLint ref, GLuint mask);
glStencilFunc_t glStencilFunc;
typedef void ( APIENTRY * glStencilMask_t )(GLuint mask);
glStencilMask_t glStencilMask;
typedef void ( APIENTRY * glStencilOp_t )(GLenum fail, GLenum zfail, GLenum zpass);
glStencilOp_t glStencilOp;
typedef void ( APIENTRY * glTexCoord1d_t )(GLdouble s);
glTexCoord1d_t glTexCoord1d;
typedef void ( APIENTRY * glTexCoord1dv_t )(const GLdouble *v);
glTexCoord1dv_t glTexCoord1dv;
typedef void ( APIENTRY * glTexCoord1f_t )(GLfloat s);
glTexCoord1f_t glTexCoord1f;
typedef void ( APIENTRY * glTexCoord1fv_t )(const GLfloat *v);
glTexCoord1fv_t glTexCoord1fv;
typedef void ( APIENTRY * glTexCoord1i_t )(GLint s);
glTexCoord1i_t glTexCoord1i;
typedef void ( APIENTRY * glTexCoord1iv_t )(const GLint *v);
glTexCoord1iv_t glTexCoord1iv;
typedef void ( APIENTRY * glTexCoord1s_t )(GLshort s);
glTexCoord1s_t glTexCoord1s;
typedef void ( APIENTRY * glTexCoord1sv_t )(const GLshort *v);
glTexCoord1sv_t glTexCoord1sv;
typedef void ( APIENTRY * glTexCoord2d_t )(GLdouble s, GLdouble t);
glTexCoord2d_t glTexCoord2d;
typedef void ( APIENTRY * glTexCoord2dv_t )(const GLdouble *v);
glTexCoord2dv_t glTexCoord2dv;
typedef void ( APIENTRY * glTexCoord2f_t )(GLfloat s, GLfloat t);
glTexCoord2f_t glTexCoord2f;
typedef void ( APIENTRY * glTexCoord2fv_t )(const GLfloat *v);
glTexCoord2fv_t glTexCoord2fv;
typedef void ( APIENTRY * glTexCoord2i_t )(GLint s, GLint t);
glTexCoord2i_t glTexCoord2i;
typedef void ( APIENTRY * glTexCoord2iv_t )(const GLint *v);
glTexCoord2iv_t glTexCoord2iv;
typedef void ( APIENTRY * glTexCoord2s_t )(GLshort s, GLshort t);
glTexCoord2s_t glTexCoord2s;
typedef void ( APIENTRY * glTexCoord2sv_t )(const GLshort *v);
glTexCoord2sv_t glTexCoord2sv;
typedef void ( APIENTRY * glTexCoord3d_t )(GLdouble s, GLdouble t, GLdouble r);
glTexCoord3d_t glTexCoord3d;
typedef void ( APIENTRY * glTexCoord3dv_t )(const GLdouble *v);
glTexCoord3dv_t glTexCoord3dv;
typedef void ( APIENTRY * glTexCoord3f_t )(GLfloat s, GLfloat t, GLfloat r);
glTexCoord3f_t glTexCoord3f;
typedef void ( APIENTRY * glTexCoord3fv_t )(const GLfloat *v);
glTexCoord3fv_t glTexCoord3fv;
typedef void ( APIENTRY * glTexCoord3i_t )(GLint s, GLint t, GLint r);
glTexCoord3i_t glTexCoord3i;
typedef void ( APIENTRY * glTexCoord3iv_t )(const GLint *v);
glTexCoord3iv_t glTexCoord3iv;
typedef void ( APIENTRY * glTexCoord3s_t )(GLshort s, GLshort t, GLshort r);
glTexCoord3s_t glTexCoord3s;
typedef void ( APIENTRY * glTexCoord3sv_t )(const GLshort *v);
glTexCoord3sv_t glTexCoord3sv;
typedef void ( APIENTRY * glTexCoord4d_t )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
glTexCoord4d_t glTexCoord4d;
typedef void ( APIENTRY * glTexCoord4dv_t )(const GLdouble *v);
glTexCoord4dv_t glTexCoord4dv;
typedef void ( APIENTRY * glTexCoord4f_t )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
glTexCoord4f_t glTexCoord4f;
typedef void ( APIENTRY * glTexCoord4fv_t )(const GLfloat *v);
glTexCoord4fv_t glTexCoord4fv;
typedef void ( APIENTRY * glTexCoord4i_t )(GLint s, GLint t, GLint r, GLint q);
glTexCoord4i_t glTexCoord4i;
typedef void ( APIENTRY * glTexCoord4iv_t )(const GLint *v);
glTexCoord4iv_t glTexCoord4iv;
typedef void ( APIENTRY * glTexCoord4s_t )(GLshort s, GLshort t, GLshort r, GLshort q);
glTexCoord4s_t glTexCoord4s;
typedef void ( APIENTRY * glTexCoord4sv_t )(const GLshort *v);
glTexCoord4sv_t glTexCoord4sv;
typedef void ( APIENTRY * glTexCoordPointer_t )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
glTexCoordPointer_t glTexCoordPointer;
typedef void ( APIENTRY * glTexEnvf_t )(GLenum target, GLenum pname, GLfloat param);
glTexEnvf_t glTexEnvf;
typedef void ( APIENTRY * glTexEnvfv_t )(GLenum target, GLenum pname, const GLfloat *params);
glTexEnvfv_t glTexEnvfv;
typedef void ( APIENTRY * glTexEnvi_t )(GLenum target, GLenum pname, GLint param);
glTexEnvi_t glTexEnvi;
typedef void ( APIENTRY * glTexEnviv_t )(GLenum target, GLenum pname, const GLint *params);
glTexEnviv_t glTexEnviv;
typedef void ( APIENTRY * glTexGend_t )(GLenum coord, GLenum pname, GLdouble param);
glTexGend_t glTexGend;
typedef void ( APIENTRY * glTexGendv_t )(GLenum coord, GLenum pname, const GLdouble *params);
glTexGendv_t glTexGendv;
typedef void ( APIENTRY * glTexGenf_t )(GLenum coord, GLenum pname, GLfloat param);
glTexGenf_t glTexGenf;
typedef void ( APIENTRY * glTexGenfv_t )(GLenum coord, GLenum pname, const GLfloat *params);
glTexGenfv_t glTexGenfv;
typedef void ( APIENTRY * glTexGeni_t )(GLenum coord, GLenum pname, GLint param);
glTexGeni_t glTexGeni;
typedef void ( APIENTRY * glTexGeniv_t )(GLenum coord, GLenum pname, const GLint *params);
glTexGeniv_t glTexGeniv;
typedef void ( APIENTRY * glTexImage1D_t )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
glTexImage1D_t glTexImage1D;
typedef void ( APIENTRY * glTexImage2D_t )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
glTexImage2D_t glTexImage2D;
typedef void ( APIENTRY * glTexParameterf_t )(GLenum target, GLenum pname, GLfloat param);
glTexParameterf_t glTexParameterf;
typedef void ( APIENTRY * glTexParameterfv_t )(GLenum target, GLenum pname, const GLfloat *params);
glTexParameterfv_t glTexParameterfv;
typedef void ( APIENTRY * glTexParameteri_t )(GLenum target, GLenum pname, GLint param);
glTexParameteri_t glTexParameteri;
typedef void ( APIENTRY * glTexParameteriv_t )(GLenum target, GLenum pname, const GLint *params);
glTexParameteriv_t glTexParameteriv;
typedef void ( APIENTRY * glTexSubImage1D_t )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
glTexSubImage1D_t glTexSubImage1D;
typedef void ( APIENTRY * glTexSubImage2D_t )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
glTexSubImage2D_t glTexSubImage2D;
typedef void ( APIENTRY * glTranslated_t )(GLdouble x, GLdouble y, GLdouble z);
glTranslated_t glTranslated;
typedef void ( APIENTRY * glTranslatef_t )(GLfloat x, GLfloat y, GLfloat z);
glTranslatef_t glTranslatef;
typedef void ( APIENTRY * glVertex2d_t )(GLdouble x, GLdouble y);
glVertex2d_t glVertex2d;
typedef void ( APIENTRY * glVertex2dv_t )(const GLdouble *v);
glVertex2dv_t glVertex2dv;
typedef void ( APIENTRY * glVertex2f_t )(GLfloat x, GLfloat y);
glVertex2f_t glVertex2f;
typedef void ( APIENTRY * glVertex2fv_t )(const GLfloat *v);
glVertex2fv_t glVertex2fv;
typedef void ( APIENTRY * glVertex2i_t )(GLint x, GLint y);
glVertex2i_t glVertex2i;
typedef void ( APIENTRY * glVertex2iv_t )(const GLint *v);
glVertex2iv_t glVertex2iv;
typedef void ( APIENTRY * glVertex2s_t )(GLshort x, GLshort y);
glVertex2s_t glVertex2s;
typedef void ( APIENTRY * glVertex2sv_t )(const GLshort *v);
glVertex2sv_t glVertex2sv;
typedef void ( APIENTRY * glVertex3d_t )(GLdouble x, GLdouble y, GLdouble z);
glVertex3d_t glVertex3d;
typedef void ( APIENTRY * glVertex3dv_t )(const GLdouble *v);
glVertex3dv_t glVertex3dv;
typedef void ( APIENTRY * glVertex3f_t )(GLfloat x, GLfloat y, GLfloat z);
glVertex3f_t glVertex3f;
typedef void ( APIENTRY * glVertex3fv_t )(const GLfloat *v);
glVertex3fv_t glVertex3fv;
typedef void ( APIENTRY * glVertex3i_t )(GLint x, GLint y, GLint z);
glVertex3i_t glVertex3i;
typedef void ( APIENTRY * glVertex3iv_t )(const GLint *v);
glVertex3iv_t glVertex3iv;
typedef void ( APIENTRY * glVertex3s_t )(GLshort x, GLshort y, GLshort z);
glVertex3s_t glVertex3s;
typedef void ( APIENTRY * glVertex3sv_t )(const GLshort *v);
glVertex3sv_t glVertex3sv;
typedef void ( APIENTRY * glVertex4d_t )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
glVertex4d_t glVertex4d;
typedef void ( APIENTRY * glVertex4dv_t )(const GLdouble *v);
glVertex4dv_t glVertex4dv;
typedef void ( APIENTRY * glVertex4f_t )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
glVertex4f_t glVertex4f;
typedef void ( APIENTRY * glVertex4fv_t )(const GLfloat *v);
glVertex4fv_t glVertex4fv;
typedef void ( APIENTRY * glVertex4i_t )(GLint x, GLint y, GLint z, GLint w);
glVertex4i_t glVertex4i;
typedef void ( APIENTRY * glVertex4iv_t )(const GLint *v);
glVertex4iv_t glVertex4iv;
typedef void ( APIENTRY * glVertex4s_t )(GLshort x, GLshort y, GLshort z, GLshort w);
glVertex4s_t glVertex4s;
typedef void ( APIENTRY * glVertex4sv_t )(const GLshort *v);
glVertex4sv_t glVertex4sv;
typedef void ( APIENTRY * glVertexPointer_t )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
glVertexPointer_t glVertexPointer;
typedef void ( APIENTRY * glViewport_t )(GLint x, GLint y, GLsizei width, GLsizei height);
glViewport_t glViewport;


/* EXT_paletted_texture */
typedef void ( GLAPIENTRY* glColorTable_t)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void* data);
glColorTable_t glColorTableEXT;

/* EXT_compiled_vertex_array */
typedef void ( GLAPIENTRY * glLockArrays_t)(GLint first, GLsizei count);
typedef void ( GLAPIENTRY * glUnlockArrays_t)(void);
glLockArrays_t   glLockArraysEXT;
glUnlockArrays_t glUnlockArraysEXT;

/* ARB_multitexture */
typedef void ( GLAPIENTRY * glActiveTextureARB_t)(GLenum target);
typedef void ( GLAPIENTRY * glClientActiveTextureARB_t)(GLenum target);
typedef void ( GLAPIENTRY * glMultiTexCoord2fARB_t)(GLenum texture, GLfloat, GLfloat);
typedef void ( GLAPIENTRY * glMultiTexCoord2fvARB_t)(GLenum texture, const GLfloat*);
glActiveTextureARB_t       glActiveTextureARB;
glClientActiveTextureARB_t glClientActiveTextureARB;
glMultiTexCoord2fARB_t     glMultiTexCoord2fARB;
glMultiTexCoord2fvARB_t    glMultiTexCoord2fvARB;

/* NV_vertex_array_range */

typedef void (GLAPIENTRY* glVertexArrayRange_t)(GLsizei length, void* pointer);
typedef void (GLAPIENTRY* glFlushVertexArrayRange_t)(void);
typedef void* (GLAPIENTRY* wglAllocateMemory_t)(GLsizei, GLfloat, GLfloat, GLfloat);
typedef void (GLAPIENTRY*  wglFreeMemory_t)(void*);
glVertexArrayRange_t      glVertexArrayRangeNV;
glFlushVertexArrayRange_t glFlushVertexArrayRangeNV;
wglAllocateMemory_t       wglAllocateMemoryNV;
wglFreeMemory_t           wglFreeMemoryNV;

/* EXT_fog_coord */
typedef void (GLAPIENTRY* glFogCoordf_t)(GLfloat);
typedef void (GLAPIENTRY* glFogCoordPointer_t)(GLenum, GLsizei, void*);
glFogCoordf_t       glFogCoordfEXT;
glFogCoordPointer_t glFogCoordPointerEXT;

/* ARB_texture_compression */
typedef void (GLAPIENTRY* glCompressedTexImage3DARB_t)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void*);
typedef void (GLAPIENTRY* glCompressedTexImage2DARB_t)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void*);
typedef void (GLAPIENTRY* glCompressedTexImage1DARB_t)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const void*);
typedef void (GLAPIENTRY* glCompressedTexSubImage3DARB_t)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void*);
typedef void (GLAPIENTRY* glCompressedTexSubImage2DARB_t)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void*);
typedef void (GLAPIENTRY* glCompressedTexSubImage1DARB_t)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void*);
typedef void (GLAPIENTRY* glGetCompressedTexImageARB_t)(GLenum target, GLint lod, void* img);
glCompressedTexImage3DARB_t     glCompressedTexImage3DARB;
glCompressedTexImage2DARB_t     glCompressedTexImage2DARB;
glCompressedTexImage1DARB_t     glCompressedTexImage1DARB;
glCompressedTexSubImage3DARB_t  glCompressedTexSubImage3DARB;
glCompressedTexSubImage2DARB_t  glCompressedTexSubImage2DARB;
glCompressedTexSubImage1DARB_t  glCompressedTexSubImage1DARB;
glGetCompressedTexImageARB_t glGetCompressedTexImageARB;

/* EXT_vertex_buffer */
typedef GLboolean (GLAPIENTRY* glAvailableVertexBufferEXT_t)(void);
typedef GLint (GLAPIENTRY* glAllocateVertexBufferEXT_t)(GLsizei size, GLint format, GLboolean preserve);
typedef void* (GLAPIENTRY* glLockVertexBufferEXT_t)(GLint handle, GLsizei size);
typedef void (GLAPIENTRY* glUnlockVertexBufferEXT_t)(GLint handle);
typedef void (GLAPIENTRY* glSetVertexBufferEXT_t)(GLint handle);
typedef void (GLAPIENTRY* glOffsetVertexBufferEXT_t)(GLint handle, GLuint offset);
typedef void (GLAPIENTRY* glFillVertexBufferEXT_t)(GLint handle, GLint first, GLsizei count);
typedef void (GLAPIENTRY* glFreeVertexBufferEXT_t)(GLint handle);
glAvailableVertexBufferEXT_t	glAvailableVertexBufferEXT;
glAllocateVertexBufferEXT_t	glAllocateVertexBufferEXT;
glLockVertexBufferEXT_t			glLockVertexBufferEXT;
glUnlockVertexBufferEXT_t		glUnlockVertexBufferEXT;
glSetVertexBufferEXT_t			glSetVertexBufferEXT;
glOffsetVertexBufferEXT_t		glOffsetVertexBufferEXT;
glFillVertexBufferEXT_t			glFillVertexBufferEXT;
glFreeVertexBufferEXT_t			glFreeVertexBufferEXT;

//------------------------------------------------------------------------------
// GLU DLL Functions
static const GLubyte* (APIENTRY * dllgluErrorString) (GLenum errCode);
static const GLubyte* (APIENTRY * dllgluGetString) (GLenum name);
static void (APIENTRY * dllgluOrtho2D) (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
static void (APIENTRY * dllgluPerspective) (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
static void (APIENTRY * dllgluPickMatrix) (GLdouble x, GLdouble y, GLdouble width, GLdouble height, GLint viewport[4]);
static void (APIENTRY * dllgluLookAt) (GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz);
static int (APIENTRY * dllgluProject) (GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz);
static int (APIENTRY * dllgluUnProject) (GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz);
static int (APIENTRY * dllgluScaleImage) (GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain, GLint widthout, GLint heightout, GLenum typeout, void *dataout);
static int (APIENTRY * dllgluBuild1DMipmaps) (GLenum target, GLint components, GLint width, GLenum format, GLenum type, const void *data);
static int (APIENTRY * dllgluBuild2DMipmaps) (GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void *data);

//------------------------------------------------------------------------------
// GL DLL Functions
static void ( APIENTRY * dllAccum )(GLenum op, GLfloat value);
static void ( APIENTRY * dllAlphaFunc )(GLenum func, GLclampf ref);
GLboolean ( APIENTRY * dllAreTexturesResident )(GLsizei n, const GLuint *textures, GLboolean *residences);
static void ( APIENTRY * dllArrayElement )(GLint i);
static void ( APIENTRY * dllBegin )(GLenum mode);
static void ( APIENTRY * dllBindTexture )(GLenum target, GLuint texture);
static void ( APIENTRY * dllBitmap )(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
static void ( APIENTRY * dllBlendFunc )(GLenum sfactor, GLenum dfactor);
static void ( APIENTRY * dllCallList )(GLuint list);
static void ( APIENTRY * dllCallLists )(GLsizei n, GLenum type, const GLvoid *lists);
static void ( APIENTRY * dllClear )(GLbitfield mask);
static void ( APIENTRY * dllClearAccum )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void ( APIENTRY * dllClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
static void ( APIENTRY * dllClearDepth )(GLclampd depth);
static void ( APIENTRY * dllClearIndex )(GLfloat c);
static void ( APIENTRY * dllClearStencil )(GLint s);
static void ( APIENTRY * dllClipPlane )(GLenum plane, const GLdouble *equation);
static void ( APIENTRY * dllColor3b )(GLbyte red, GLbyte green, GLbyte blue);
static void ( APIENTRY * dllColor3bv )(const GLbyte *v);
static void ( APIENTRY * dllColor3d )(GLdouble red, GLdouble green, GLdouble blue);
static void ( APIENTRY * dllColor3dv )(const GLdouble *v);
static void ( APIENTRY * dllColor3f )(GLfloat red, GLfloat green, GLfloat blue);
static void ( APIENTRY * dllColor3fv )(const GLfloat *v);
static void ( APIENTRY * dllColor3i )(GLint red, GLint green, GLint blue);
static void ( APIENTRY * dllColor3iv )(const GLint *v);
static void ( APIENTRY * dllColor3s )(GLshort red, GLshort green, GLshort blue);
static void ( APIENTRY * dllColor3sv )(const GLshort *v);
static void ( APIENTRY * dllColor3ub )(GLubyte red, GLubyte green, GLubyte blue);
static void ( APIENTRY * dllColor3ubv )(const GLubyte *v);
static void ( APIENTRY * dllColor3ui )(GLuint red, GLuint green, GLuint blue);
static void ( APIENTRY * dllColor3uiv )(const GLuint *v);
static void ( APIENTRY * dllColor3us )(GLushort red, GLushort green, GLushort blue);
static void ( APIENTRY * dllColor3usv )(const GLushort *v);
static void ( APIENTRY * dllColor4b )(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
static void ( APIENTRY * dllColor4bv )(const GLbyte *v);
static void ( APIENTRY * dllColor4d )(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
static void ( APIENTRY * dllColor4dv )(const GLdouble *v);
static void ( APIENTRY * dllColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
static void ( APIENTRY * dllColor4fv )(const GLfloat *v);
static void ( APIENTRY * dllColor4i )(GLint red, GLint green, GLint blue, GLint alpha);
static void ( APIENTRY * dllColor4iv )(const GLint *v);
static void ( APIENTRY * dllColor4s )(GLshort red, GLshort green, GLshort blue, GLshort alpha);
static void ( APIENTRY * dllColor4sv )(const GLshort *v);
static void ( APIENTRY * dllColor4ub )(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
static void ( APIENTRY * dllColor4ubv )(const GLubyte *v);
static void ( APIENTRY * dllColor4ui )(GLuint red, GLuint green, GLuint blue, GLuint alpha);
static void ( APIENTRY * dllColor4uiv )(const GLuint *v);
static void ( APIENTRY * dllColor4us )(GLushort red, GLushort green, GLushort blue, GLushort alpha);
static void ( APIENTRY * dllColor4usv )(const GLushort *v);
static void ( APIENTRY * dllColorMask )(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
static void ( APIENTRY * dllColorMaterial )(GLenum face, GLenum mode);
static void ( APIENTRY * dllColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllCopyPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
static void ( APIENTRY * dllCopyTexImage1D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
static void ( APIENTRY * dllCopyTexImage2D )(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
static void ( APIENTRY * dllCopyTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
static void ( APIENTRY * dllCopyTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllCullFace )(GLenum mode);
static void ( APIENTRY * dllDeleteLists )(GLuint list, GLsizei range);
static void ( APIENTRY * dllDeleteTextures )(GLsizei n, const GLuint *textures);
static void ( APIENTRY * dllDepthFunc )(GLenum func);
static void ( APIENTRY * dllDepthMask )(GLboolean flag);
static void ( APIENTRY * dllDepthRange )(GLclampd zNear, GLclampd zFar);
static void ( APIENTRY * dllDisable )(GLenum cap);
static void ( APIENTRY * dllDisableClientState )(GLenum array);
static void ( APIENTRY * dllDrawArrays )(GLenum mode, GLint first, GLsizei count);
static void ( APIENTRY * dllDrawBuffer )(GLenum mode);
static void ( APIENTRY * dllDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
static void ( APIENTRY * dllDrawPixels )(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllEdgeFlag )(GLboolean flag);
static void ( APIENTRY * dllEdgeFlagPointer )(GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllEdgeFlagv )(const GLboolean *flag);
static void ( APIENTRY * dllEnable )(GLenum cap);
static void ( APIENTRY * dllEnableClientState )(GLenum array);
static void ( APIENTRY * dllEnd )(void);
static void ( APIENTRY * dllEndList )(void);
static void ( APIENTRY * dllEvalCoord1d )(GLdouble u);
static void ( APIENTRY * dllEvalCoord1dv )(const GLdouble *u);
static void ( APIENTRY * dllEvalCoord1f )(GLfloat u);
static void ( APIENTRY * dllEvalCoord1fv )(const GLfloat *u);
static void ( APIENTRY * dllEvalCoord2d )(GLdouble u, GLdouble v);
static void ( APIENTRY * dllEvalCoord2dv )(const GLdouble *u);
static void ( APIENTRY * dllEvalCoord2f )(GLfloat u, GLfloat v);
static void ( APIENTRY * dllEvalCoord2fv )(const GLfloat *u);
static void ( APIENTRY * dllEvalMesh1 )(GLenum mode, GLint i1, GLint i2);
static void ( APIENTRY * dllEvalMesh2 )(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
static void ( APIENTRY * dllEvalPoint1 )(GLint i);
static void ( APIENTRY * dllEvalPoint2 )(GLint i, GLint j);
static void ( APIENTRY * dllFeedbackBuffer )(GLsizei size, GLenum type, GLfloat *buffer);
static void ( APIENTRY * dllFinish )(void);
static void ( APIENTRY * dllFlush )(void);
static void ( APIENTRY * dllFogf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllFogfv )(GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllFogi )(GLenum pname, GLint param);
static void ( APIENTRY * dllFogiv )(GLenum pname, const GLint *params);
static void ( APIENTRY * dllFrontFace )(GLenum mode);
static void ( APIENTRY * dllFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint ( APIENTRY * dllGenLists )(GLsizei range);
static void ( APIENTRY * dllGenTextures )(GLsizei n, GLuint *textures);
static void ( APIENTRY * dllGetBooleanv )(GLenum pname, GLboolean *params);
static void ( APIENTRY * dllGetClipPlane )(GLenum plane, GLdouble *equation);
static void ( APIENTRY * dllGetDoublev )(GLenum pname, GLdouble *params);
GLenum ( APIENTRY * dllGetError )(void);
static void ( APIENTRY * dllGetFloatv )(GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetIntegerv )(GLenum pname, GLint *params);
static void ( APIENTRY * dllGetLightfv )(GLenum light, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetLightiv )(GLenum light, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetMapdv )(GLenum target, GLenum query, GLdouble *v);
static void ( APIENTRY * dllGetMapfv )(GLenum target, GLenum query, GLfloat *v);
static void ( APIENTRY * dllGetMapiv )(GLenum target, GLenum query, GLint *v);
static void ( APIENTRY * dllGetMaterialfv )(GLenum face, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetMaterialiv )(GLenum face, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetPixelMapfv )(GLenum map, GLfloat *values);
static void ( APIENTRY * dllGetPixelMapuiv )(GLenum map, GLuint *values);
static void ( APIENTRY * dllGetPixelMapusv )(GLenum map, GLushort *values);
static void ( APIENTRY * dllGetPointerv )(GLenum pname, GLvoid* *params);
static void ( APIENTRY * dllGetPolygonStipple )(GLubyte *mask);
const GLubyte * ( APIENTRY * dllGetString )(GLenum name);
static void ( APIENTRY * dllGetTexEnvfv )(GLenum target, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexEnviv )(GLenum target, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexGendv )(GLenum coord, GLenum pname, GLdouble *params);
static void ( APIENTRY * dllGetTexGenfv )(GLenum coord, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexGeniv )(GLenum coord, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexImage )(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllGetTexLevelParameterfv )(GLenum target, GLint level, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexLevelParameteriv )(GLenum target, GLint level, GLenum pname, GLint *params);
static void ( APIENTRY * dllGetTexParameterfv )(GLenum target, GLenum pname, GLfloat *params);
static void ( APIENTRY * dllGetTexParameteriv )(GLenum target, GLenum pname, GLint *params);
static void ( APIENTRY * dllHint )(GLenum target, GLenum mode);
static void ( APIENTRY * dllIndexMask )(GLuint mask);
static void ( APIENTRY * dllIndexPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllIndexd )(GLdouble c);
static void ( APIENTRY * dllIndexdv )(const GLdouble *c);
static void ( APIENTRY * dllIndexf )(GLfloat c);
static void ( APIENTRY * dllIndexfv )(const GLfloat *c);
static void ( APIENTRY * dllIndexi )(GLint c);
static void ( APIENTRY * dllIndexiv )(const GLint *c);
static void ( APIENTRY * dllIndexs )(GLshort c);
static void ( APIENTRY * dllIndexsv )(const GLshort *c);
static void ( APIENTRY * dllIndexub )(GLubyte c);
static void ( APIENTRY * dllIndexubv )(const GLubyte *c);
static void ( APIENTRY * dllInitNames )(void);
static void ( APIENTRY * dllInterleavedArrays )(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean ( APIENTRY * dllIsEnabled )(GLenum cap);
GLboolean ( APIENTRY * dllIsList )(GLuint list);
GLboolean ( APIENTRY * dllIsTexture )(GLuint texture);
static void ( APIENTRY * dllLightModelf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllLightModelfv )(GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllLightModeli )(GLenum pname, GLint param);
static void ( APIENTRY * dllLightModeliv )(GLenum pname, const GLint *params);
static void ( APIENTRY * dllLightf )(GLenum light, GLenum pname, GLfloat param);
static void ( APIENTRY * dllLightfv )(GLenum light, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllLighti )(GLenum light, GLenum pname, GLint param);
static void ( APIENTRY * dllLightiv )(GLenum light, GLenum pname, const GLint *params);
static void ( APIENTRY * dllLineStipple )(GLint factor, GLushort pattern);
static void ( APIENTRY * dllLineWidth )(GLfloat width);
static void ( APIENTRY * dllListBase )(GLuint base);
static void ( APIENTRY * dllLoadIdentity )(void);
static void ( APIENTRY * dllLoadMatrixd )(const GLdouble *m);
static void ( APIENTRY * dllLoadMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllLoadName )(GLuint name);
static void ( APIENTRY * dllLogicOp )(GLenum opcode);
static void ( APIENTRY * dllMap1d )(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
static void ( APIENTRY * dllMap1f )(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
static void ( APIENTRY * dllMap2d )(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
static void ( APIENTRY * dllMap2f )(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
static void ( APIENTRY * dllMapGrid1d )(GLint un, GLdouble u1, GLdouble u2);
static void ( APIENTRY * dllMapGrid1f )(GLint un, GLfloat u1, GLfloat u2);
static void ( APIENTRY * dllMapGrid2d )(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
static void ( APIENTRY * dllMapGrid2f )(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
static void ( APIENTRY * dllMaterialf )(GLenum face, GLenum pname, GLfloat param);
static void ( APIENTRY * dllMaterialfv )(GLenum face, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllMateriali )(GLenum face, GLenum pname, GLint param);
static void ( APIENTRY * dllMaterialiv )(GLenum face, GLenum pname, const GLint *params);
static void ( APIENTRY * dllMatrixMode )(GLenum mode);
static void ( APIENTRY * dllMultMatrixd )(const GLdouble *m);
static void ( APIENTRY * dllMultMatrixf )(const GLfloat *m);
static void ( APIENTRY * dllNewList )(GLuint list, GLenum mode);
static void ( APIENTRY * dllNormal3b )(GLbyte nx, GLbyte ny, GLbyte nz);
static void ( APIENTRY * dllNormal3bv )(const GLbyte *v);
static void ( APIENTRY * dllNormal3d )(GLdouble nx, GLdouble ny, GLdouble nz);
static void ( APIENTRY * dllNormal3dv )(const GLdouble *v);
static void ( APIENTRY * dllNormal3f )(GLfloat nx, GLfloat ny, GLfloat nz);
static void ( APIENTRY * dllNormal3fv )(const GLfloat *v);
static void ( APIENTRY * dllNormal3i )(GLint nx, GLint ny, GLint nz);
static void ( APIENTRY * dllNormal3iv )(const GLint *v);
static void ( APIENTRY * dllNormal3s )(GLshort nx, GLshort ny, GLshort nz);
static void ( APIENTRY * dllNormal3sv )(const GLshort *v);
static void ( APIENTRY * dllNormalPointer )(GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
static void ( APIENTRY * dllPassThrough )(GLfloat token);
static void ( APIENTRY * dllPixelMapfv )(GLenum map, GLsizei mapsize, const GLfloat *values);
static void ( APIENTRY * dllPixelMapuiv )(GLenum map, GLsizei mapsize, const GLuint *values);
static void ( APIENTRY * dllPixelMapusv )(GLenum map, GLsizei mapsize, const GLushort *values);
static void ( APIENTRY * dllPixelStoref )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllPixelStorei )(GLenum pname, GLint param);
static void ( APIENTRY * dllPixelTransferf )(GLenum pname, GLfloat param);
static void ( APIENTRY * dllPixelTransferi )(GLenum pname, GLint param);
static void ( APIENTRY * dllPixelZoom )(GLfloat xfactor, GLfloat yfactor);
static void ( APIENTRY * dllPointSize )(GLfloat size);
static void ( APIENTRY * dllPolygonMode )(GLenum face, GLenum mode);
static void ( APIENTRY * dllPolygonOffset )(GLfloat factor, GLfloat units);
static void ( APIENTRY * dllPolygonStipple )(const GLubyte *mask);
static void ( APIENTRY * dllPopAttrib )(void);
static void ( APIENTRY * dllPopClientAttrib )(void);
static void ( APIENTRY * dllPopMatrix )(void);
static void ( APIENTRY * dllPopName )(void);
static void ( APIENTRY * dllPrioritizeTextures )(GLsizei n, const GLuint *textures, const GLclampf *priorities);
static void ( APIENTRY * dllPushAttrib )(GLbitfield mask);
static void ( APIENTRY * dllPushClientAttrib )(GLbitfield mask);
static void ( APIENTRY * dllPushMatrix )(void);
static void ( APIENTRY * dllPushName )(GLuint name);
static void ( APIENTRY * dllRasterPos2d )(GLdouble x, GLdouble y);
static void ( APIENTRY * dllRasterPos2dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllRasterPos2fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos2i )(GLint x, GLint y);
static void ( APIENTRY * dllRasterPos2iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos2s )(GLshort x, GLshort y);
static void ( APIENTRY * dllRasterPos2sv )(const GLshort *v);
static void ( APIENTRY * dllRasterPos3d )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRasterPos3dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllRasterPos3fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos3i )(GLint x, GLint y, GLint z);
static void ( APIENTRY * dllRasterPos3iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos3s )(GLshort x, GLshort y, GLshort z);
static void ( APIENTRY * dllRasterPos3sv )(const GLshort *v);
static void ( APIENTRY * dllRasterPos4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void ( APIENTRY * dllRasterPos4dv )(const GLdouble *v);
static void ( APIENTRY * dllRasterPos4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void ( APIENTRY * dllRasterPos4fv )(const GLfloat *v);
static void ( APIENTRY * dllRasterPos4i )(GLint x, GLint y, GLint z, GLint w);
static void ( APIENTRY * dllRasterPos4iv )(const GLint *v);
static void ( APIENTRY * dllRasterPos4s )(GLshort x, GLshort y, GLshort z, GLshort w);
static void ( APIENTRY * dllRasterPos4sv )(const GLshort *v);
static void ( APIENTRY * dllReadBuffer )(GLenum mode);
static void ( APIENTRY * dllReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
static void ( APIENTRY * dllRectd )(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
static void ( APIENTRY * dllRectdv )(const GLdouble *v1, const GLdouble *v2);
static void ( APIENTRY * dllRectf )(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
static void ( APIENTRY * dllRectfv )(const GLfloat *v1, const GLfloat *v2);
static void ( APIENTRY * dllRecti )(GLint x1, GLint y1, GLint x2, GLint y2);
static void ( APIENTRY * dllRectiv )(const GLint *v1, const GLint *v2);
static void ( APIENTRY * dllRects )(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
static void ( APIENTRY * dllRectsv )(const GLshort *v1, const GLshort *v2);
GLint ( APIENTRY * dllRenderMode )(GLenum mode);
static void ( APIENTRY * dllRotated )(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScaled )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllScalef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
static void ( APIENTRY * dllSelectBuffer )(GLsizei size, GLuint *buffer);
static void ( APIENTRY * dllShadeModel )(GLenum mode);
static void ( APIENTRY * dllStencilFunc )(GLenum func, GLint ref, GLuint mask);
static void ( APIENTRY * dllStencilMask )(GLuint mask);
static void ( APIENTRY * dllStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
static void ( APIENTRY * dllTexCoord1d )(GLdouble s);
static void ( APIENTRY * dllTexCoord1dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord1f )(GLfloat s);
static void ( APIENTRY * dllTexCoord1fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord1i )(GLint s);
static void ( APIENTRY * dllTexCoord1iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord1s )(GLshort s);
static void ( APIENTRY * dllTexCoord1sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord2d )(GLdouble s, GLdouble t);
static void ( APIENTRY * dllTexCoord2dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord2f )(GLfloat s, GLfloat t);
static void ( APIENTRY * dllTexCoord2fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord2i )(GLint s, GLint t);
static void ( APIENTRY * dllTexCoord2iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord2s )(GLshort s, GLshort t);
static void ( APIENTRY * dllTexCoord2sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord3d )(GLdouble s, GLdouble t, GLdouble r);
static void ( APIENTRY * dllTexCoord3dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord3f )(GLfloat s, GLfloat t, GLfloat r);
static void ( APIENTRY * dllTexCoord3fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord3i )(GLint s, GLint t, GLint r);
static void ( APIENTRY * dllTexCoord3iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord3s )(GLshort s, GLshort t, GLshort r);
static void ( APIENTRY * dllTexCoord3sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoord4d )(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
static void ( APIENTRY * dllTexCoord4dv )(const GLdouble *v);
static void ( APIENTRY * dllTexCoord4f )(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
static void ( APIENTRY * dllTexCoord4fv )(const GLfloat *v);
static void ( APIENTRY * dllTexCoord4i )(GLint s, GLint t, GLint r, GLint q);
static void ( APIENTRY * dllTexCoord4iv )(const GLint *v);
static void ( APIENTRY * dllTexCoord4s )(GLshort s, GLshort t, GLshort r, GLshort q);
static void ( APIENTRY * dllTexCoord4sv )(const GLshort *v);
static void ( APIENTRY * dllTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllTexEnvf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexEnvfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexEnvi )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexEnviv )(GLenum target, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexGend )(GLenum coord, GLenum pname, GLdouble param);
static void ( APIENTRY * dllTexGendv )(GLenum coord, GLenum pname, const GLdouble *params);
static void ( APIENTRY * dllTexGenf )(GLenum coord, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexGenfv )(GLenum coord, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexGeni )(GLenum coord, GLenum pname, GLint param);
static void ( APIENTRY * dllTexGeniv )(GLenum coord, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexImage1D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexParameterf )(GLenum target, GLenum pname, GLfloat param);
static void ( APIENTRY * dllTexParameterfv )(GLenum target, GLenum pname, const GLfloat *params);
static void ( APIENTRY * dllTexParameteri )(GLenum target, GLenum pname, GLint param);
static void ( APIENTRY * dllTexParameteriv )(GLenum target, GLenum pname, const GLint *params);
static void ( APIENTRY * dllTexSubImage1D )(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
static void ( APIENTRY * dllTranslated )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllTranslatef )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex2d )(GLdouble x, GLdouble y);
static void ( APIENTRY * dllVertex2dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex2f )(GLfloat x, GLfloat y);
static void ( APIENTRY * dllVertex2fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex2i )(GLint x, GLint y);
static void ( APIENTRY * dllVertex2iv )(const GLint *v);
static void ( APIENTRY * dllVertex2s )(GLshort x, GLshort y);
static void ( APIENTRY * dllVertex2sv )(const GLshort *v);
static void ( APIENTRY * dllVertex3d )(GLdouble x, GLdouble y, GLdouble z);
static void ( APIENTRY * dllVertex3dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex3f )(GLfloat x, GLfloat y, GLfloat z);
static void ( APIENTRY * dllVertex3fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex3i )(GLint x, GLint y, GLint z);
static void ( APIENTRY * dllVertex3iv )(const GLint *v);
static void ( APIENTRY * dllVertex3s )(GLshort x, GLshort y, GLshort z);
static void ( APIENTRY * dllVertex3sv )(const GLshort *v);
static void ( APIENTRY * dllVertex4d )(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
static void ( APIENTRY * dllVertex4dv )(const GLdouble *v);
static void ( APIENTRY * dllVertex4f )(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void ( APIENTRY * dllVertex4fv )(const GLfloat *v);
static void ( APIENTRY * dllVertex4i )(GLint x, GLint y, GLint z, GLint w);
static void ( APIENTRY * dllVertex4iv )(const GLint *v);
static void ( APIENTRY * dllVertex4s )(GLshort x, GLshort y, GLshort z, GLshort w);
static void ( APIENTRY * dllVertex4sv )(const GLshort *v);
static void ( APIENTRY * dllVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
static void ( APIENTRY * dllViewport )(GLint x, GLint y, GLsizei width, GLsizei height);

/* EXT_paletted_texture */
static void ( APIENTRY * dllColorTableEXT)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void* data);

/* EXT_compiled_vertex_array */
static void ( APIENTRY * dllLockArraysEXT)(GLint, GLsizei);
static void ( APIENTRY * dllUnlockArraysEXT)();

/* ARB_multitexture */
static void ( APIENTRY * dllActiveTextureARB)(GLenum target);
static void ( APIENTRY * dllClientActiveTextureARB)(GLenum target);
static void ( APIENTRY * dllMultiTexCoord2fARB)(GLenum texture, GLfloat, GLfloat);
static void ( APIENTRY * dllMultiTexCoord2fvARB)(GLenum texture, const GLfloat*);

/* NV_vertex_array_range */
static void (APIENTRY* dllVertexArrayRangeNV)(GLsizei length, void* pointer);
static void (APIENTRY* dllFlushVertexArrayRangeNV)();
static void* (APIENTRY* dllAllocateMemoryNV)(GLsizei length, GLfloat, GLfloat, GLfloat);
static void  (APIENTRY* dllFreeMemoryNV)(void*);

/* EXT_fog_coord */
static void (APIENTRY* dllFogCoordfEXT)(GLfloat coord);
static void (APIENTRY* dllFogCoordPointerEXT)(GLenum, GLsizei, void*);

/* ARB_texture_compression */
static void (APIENTRY* dllCompressedTexImage3DARB)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void*);
static void (APIENTRY* dllCompressedTexImage2DARB)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void*);
static void (APIENTRY* dllCompressedTexImage1DARB)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const void*);
static void (APIENTRY* dllCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void*);
static void (APIENTRY* dllCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void*);
static void (APIENTRY* dllCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void*);
static void (APIENTRY* dllGetCompressedTexImageARB)(GLenum target, GLint lod, void* img);

/* EXT_vertex_buffer */
static GLboolean (GLAPIENTRY* dllAvailableVertexBufferEXT)();
static GLint (GLAPIENTRY* dllAllocateVertexBufferEXT)(GLsizei size, GLint format, GLboolean preserve);
static void* (GLAPIENTRY* dllLockVertexBufferEXT)(GLint handle, GLsizei size);
static void (GLAPIENTRY* dllUnlockVertexBufferEXT)(GLint handle);
static void (GLAPIENTRY* dllSetVertexBufferEXT)(GLint handle);
static void (GLAPIENTRY* dllOffsetVertexBufferEXT)(GLint handle, GLuint offset);
static void (GLAPIENTRY* dllFillVertexBufferEXT)(GLint handle, GLint first, GLsizei count);
static void (GLAPIENTRY* dllFreeVertexBufferEXT)(GLint handle);

static void printLastError(const char* msg)
{
   LPVOID lpMsgBuf;
   FormatMessage( 
       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
       FORMAT_MESSAGE_FROM_SYSTEM | 
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       GetLastError(),
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
       (LPTSTR) &lpMsgBuf,
       0,
       NULL 
   );
   Con::printf("%s: %s",msg,(const char*)lpMsgBuf);
   LocalFree( lpMsgBuf );
}   

static const char * BooleanToString( GLboolean b )
{
   if ( b == GL_FALSE )
      return "GL_FALSE";
   else if ( b == GL_TRUE )
      return "GL_TRUE";
   else
      return "OUT OF RANGE FOR BOOLEAN";
}

static const char * FuncToString( GLenum f )
{
   switch ( f )
   {
   case GL_ALWAYS:
      return "GL_ALWAYS";
   case GL_NEVER:
      return "GL_NEVER";
   case GL_LEQUAL:
      return "GL_LEQUAL";
   case GL_LESS:
      return "GL_LESS";
   case GL_EQUAL:
      return "GL_EQUAL";
   case GL_GREATER:
      return "GL_GREATER";
   case GL_GEQUAL:
      return "GL_GEQUAL";
   case GL_NOTEQUAL:
      return "GL_NOTEQUAL";
   default:
      return "!!! UNKNOWN !!!";
   }
}

static const char* CoordToString( GLenum coord)
{
   if (coord == GL_S)
      return "GL_S";
   else if (coord == GL_T)
      return "GL_T";
   else
      return "INVALID_COORD";
}

static const char * PrimToString( GLenum mode )
{
   static char prim[1024];

   if ( mode == GL_TRIANGLES )
      strcpy( prim, "GL_TRIANGLES" );
   else if ( mode == GL_TRIANGLE_STRIP )
      strcpy( prim, "GL_TRIANGLE_STRIP" );
   else if ( mode == GL_TRIANGLE_FAN )
      strcpy( prim, "GL_TRIANGLE_FAN" );
   else if ( mode == GL_QUADS )
      strcpy( prim, "GL_QUADS" );
   else if ( mode == GL_QUAD_STRIP )
      strcpy( prim, "GL_QUAD_STRIP" );
   else if ( mode == GL_POLYGON )
      strcpy( prim, "GL_POLYGON" );
   else if ( mode == GL_POINTS )
      strcpy( prim, "GL_POINTS" );
   else if ( mode == GL_LINES )
      strcpy( prim, "GL_LINES" );
   else if ( mode == GL_LINE_STRIP )
      strcpy( prim, "GL_LINE_STRIP" );
   else if ( mode == GL_LINE_LOOP )
      strcpy( prim, "GL_LINE_LOOP" );
   else
      sprintf( prim, "0x%x", mode );

   return prim;
}

static const char * CapToString( GLenum cap )
{
   static char buffer[1024];

   switch ( cap )
   {
   case GL_TEXTURE_2D:
      return "GL_TEXTURE_2D";
   case GL_BLEND:
      return "GL_BLEND";
   case GL_DEPTH_TEST:
      return "GL_DEPTH_TEST";
   case GL_CULL_FACE:
      return "GL_CULL_FACE";
   case GL_CLIP_PLANE0:
      return "GL_CLIP_PLANE0";
   case GL_COLOR_ARRAY:
      return "GL_COLOR_ARRAY";
   case GL_TEXTURE_COORD_ARRAY:
      return "GL_TEXTURE_COORD_ARRAY";
   case GL_VERTEX_ARRAY:
      return "GL_VERTEX_ARRAY";
   case GL_ALPHA_TEST:
      return "GL_ALPHA_TEST";
   case GL_STENCIL_TEST:
      return "GL_STENCIL_TEST";
   case GL_TEXTURE_GEN_S:
      return "GL_TEXTURE_GEN_S";
   case GL_TEXTURE_GEN_T:
      return "GL_TEXTURE_GEN_T";
   default:
      sprintf( buffer, "0x%x", cap );
   }

   return buffer;
}

static const char * TypeToString( GLenum t )
{
   switch ( t )
   {
   case GL_BYTE:
      return "GL_BYTE";
   case GL_UNSIGNED_BYTE:
      return "GL_UNSIGNED_BYTE";
   case GL_SHORT:
      return "GL_SHORT";
   case GL_UNSIGNED_SHORT:
      return "GL_UNSIGNED_SHORT";
   case GL_INT:
      return "GL_INT";
   case GL_UNSIGNED_INT:
      return "GL_UNSIGNED_INT";
   case GL_FLOAT:
      return "GL_FLOAT";
   case GL_DOUBLE:
      return "GL_DOUBLE";
   default:
      return "!!! UNKNOWN !!!";
   }
}

//------------------------------------------------------------------------------
// GLU Log Functions
static void APIENTRY loggluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
   fprintf(winState.log_fp, "gluOrtho2D( %d, %d, %d, %d )\n", left, right, bottom, top);
   fflush(winState.log_fp);
   dllgluOrtho2D(left, right, bottom, top);
}  
 
static void APIENTRY loggluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
   fprintf(winState.log_fp, "gluPerspective( %d, %d, %d, %d )\n", fovy, aspect, zNear, zFar);
   fflush(winState.log_fp);
   dllgluPerspective(fovy, aspect, zNear, zFar);
}  
 
static void APIENTRY loggluPickMatrix(GLdouble x, GLdouble y, GLdouble width, GLdouble height, GLint viewport[4])
{
   fprintf(winState.log_fp, "gluPickMatrix(%d, %d, %d, %d, VIEW)\n", x, y, width, height);
   fflush(winState.log_fp);
   dllgluPickMatrix(x, y, width, height, viewport);   
}  
 
static void APIENTRY loggluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz)
{
   fprintf(winState.log_fp, "gluLookAt(%d, %d, %d, %d, %d, %d, %d, %d, %d)\n",eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
   fflush(winState.log_fp);
   dllgluLookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
}  
 
static int APIENTRY loggluProject(GLdouble objx, GLdouble objy, GLdouble objz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz)
{
   fprintf(winState.log_fp, "gluProject\n");
   fflush(winState.log_fp);
   return dllgluProject(objx, objy, objz, modelMatrix, projMatrix, viewport, winx, winy, winz);   
}  
 
static int APIENTRY loggluUnProject(GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz)
{
   fprintf(winState.log_fp, "gluUnProject\n");
   fflush(winState.log_fp);
   return dllgluUnProject(winx, winy, winz, modelMatrix, projMatrix, viewport, objx, objy, objz);
}  
 
static int APIENTRY loggluScaleImage(GLenum format, GLint widthin, GLint heightin, GLenum typein, const void *datain, GLint widthout, GLint heightout, GLenum typeout, void *dataout)
{
   fprintf(winState.log_fp, "gluScaleImage\n");
   fflush(winState.log_fp);
   return dllgluScaleImage(format, widthin, heightin, typein, datain, widthout, heightout, typeout, dataout);
}  
 
static int APIENTRY loggluBuild1DMipmaps(GLenum target, GLint components, GLint width, GLenum format, GLenum type, const void *data)
{
   fprintf(winState.log_fp, "gluBuild1DMipmaps\n");
   fflush(winState.log_fp);
   return dllgluBuild1DMipmaps(target, components, width, format, type, data);   
}  
 
static int APIENTRY loggluBuild2DMipmaps(GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void *data)
{
   fprintf(winState.log_fp, "gluBuild2DMipmaps\n");
   fflush(winState.log_fp);
   return dllgluBuild2DMipmaps(target, components, width, height, format, type, data);   
}  
 

//------------------------------------------------------------------------------
// GL LOG Functions
static void APIENTRY logAccum(GLenum op, GLfloat value)
{
   fprintf( winState.log_fp, "glAccum\n" );
   fflush(winState.log_fp);
   dllAccum( op, value );
}

static void APIENTRY logAlphaFunc(GLenum func, GLclampf ref)
{
   fprintf( winState.log_fp, "glAlphaFunc( 0x%x, %f )\n", func, ref );
   fflush(winState.log_fp);
   dllAlphaFunc( func, ref );
}

static GLboolean APIENTRY logAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   fprintf( winState.log_fp, "glAreTexturesResident\n" );
   fflush(winState.log_fp);
   return dllAreTexturesResident( n, textures, residences );
}

static void APIENTRY logArrayElement(GLint i)
{
   fprintf( winState.log_fp, "glArrayElement\n" );
   fflush(winState.log_fp);
   dllArrayElement( i );
}

static void APIENTRY logBegin(GLenum mode)
{
   fprintf( winState.log_fp, "glBegin( %s )\n", PrimToString( mode ));
   fflush(winState.log_fp);
   dllBegin( mode );
}

static void APIENTRY logBindTexture(GLenum target, GLuint texture)
{
   fprintf( winState.log_fp, "glBindTexture( 0x%x, %u )\n", target, texture );
   fflush(winState.log_fp);
   dllBindTexture( target, texture );
}

static void APIENTRY logBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
   fprintf( winState.log_fp, "glBitmap\n" );
   fflush(winState.log_fp);
   dllBitmap( width, height, xorig, yorig, xmove, ymove, bitmap );
}

static void BlendToName( char *n, GLenum f )
{
   switch ( f )
   {
   case GL_ONE:
      strcpy( n, "GL_ONE" );
      break;
   case GL_ZERO:
      strcpy( n, "GL_ZERO" );
      break;
   case GL_SRC_ALPHA:
      strcpy( n, "GL_SRC_ALPHA" );
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      strcpy( n, "GL_ONE_MINUS_SRC_ALPHA" );
      break;
   case GL_SRC_COLOR:
      strcpy( n, "GL_SRC_COLOR" );
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      strcpy( n, "GL_ONE_MINUS_SRC_COLOR" );
      break;
   case GL_DST_COLOR:
      strcpy( n, "GL_DST_COLOR" );
      break;
   case GL_ONE_MINUS_DST_COLOR:
      strcpy( n, "GL_ONE_MINUS_DST_COLOR" );
      break;
   case GL_DST_ALPHA:
      strcpy( n, "GL_DST_ALPHA" );
      break;
   default:
      sprintf( n, "0x%x", f );
   }
}
static void APIENTRY logBlendFunc(GLenum sfactor, GLenum dfactor)
{
   char sf[128], df[128];

   BlendToName( sf, sfactor );
   BlendToName( df, dfactor );

   fprintf( winState.log_fp, "glBlendFunc( %s, %s )\n", sf, df );
   fflush(winState.log_fp);
   dllBlendFunc( sfactor, dfactor );
}

static void APIENTRY logCallList(GLuint list)
{
   fprintf( winState.log_fp, "glCallList( %u )\n", list );
   fflush(winState.log_fp);
   dllCallList( list );
}

static void APIENTRY logCallLists(GLsizei n, GLenum type, const void *lists)
{
   fprintf( winState.log_fp, "glCallLists\n" );
   fflush(winState.log_fp);
   dllCallLists( n, type, lists );
}

static void APIENTRY logClear(GLbitfield mask)
{
   fprintf( winState.log_fp, "glClear( 0x%x = ", mask );

   if ( mask & GL_COLOR_BUFFER_BIT )
      fprintf( winState.log_fp, "GL_COLOR_BUFFER_BIT " );
   if ( mask & GL_DEPTH_BUFFER_BIT )
      fprintf( winState.log_fp, "GL_DEPTH_BUFFER_BIT " );
   if ( mask & GL_STENCIL_BUFFER_BIT )
      fprintf( winState.log_fp, "GL_STENCIL_BUFFER_BIT " );
   if ( mask & GL_ACCUM_BUFFER_BIT )
      fprintf( winState.log_fp, "GL_ACCUM_BUFFER_BIT " );

   fprintf( winState.log_fp, ")\n" );
   fflush(winState.log_fp);
   dllClear( mask );
}

static void APIENTRY logClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   fprintf( winState.log_fp, "glClearAccum\n" );
   fflush(winState.log_fp);
   dllClearAccum( red, green, blue, alpha );
}

static void APIENTRY logClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   fprintf( winState.log_fp, "glClearColor\n" );
   fflush(winState.log_fp);
   dllClearColor( red, green, blue, alpha );
}

static void APIENTRY logClearDepth(GLclampd depth)
{
   fprintf( winState.log_fp, "glClearDepth( %f )\n", ( F32 ) depth );
   fflush(winState.log_fp);
   dllClearDepth( depth );
}

static void APIENTRY logClearIndex(GLfloat c)
{
   fprintf( winState.log_fp, "glClearIndex\n" );
   fflush(winState.log_fp);
   dllClearIndex( c );
}

static void APIENTRY logClearStencil(GLint s)
{
   fprintf( winState.log_fp, "glClearStencil( %d )\n", s );
   fflush(winState.log_fp);
   dllClearStencil( s );
}

static void APIENTRY logClipPlane(GLenum plane, const GLdouble *equation)
{
   fprintf( winState.log_fp, "glClipPlane\n" );
   fflush(winState.log_fp);
   dllClipPlane( plane, equation );
}

static void APIENTRY logColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
   fprintf( winState.log_fp, "glColor3b\n" );
   fflush(winState.log_fp);
   dllColor3b( red, green, blue );
}

static void APIENTRY logColor3bv(const GLbyte *v)
{
   fprintf( winState.log_fp, "glColor3bv\n" );
   fflush(winState.log_fp);
   dllColor3bv( v );
}

static void APIENTRY logColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
   fprintf( winState.log_fp, "glColor3d\n" );
   fflush(winState.log_fp);
   dllColor3d( red, green, blue );
}

static void APIENTRY logColor3dv(const GLdouble *v)
{
   fprintf( winState.log_fp, "glColor3dv\n" );
   fflush(winState.log_fp);
   dllColor3dv( v );
}

static void APIENTRY logColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
   fprintf( winState.log_fp, "glColor3f\n" );
   fflush(winState.log_fp);
   dllColor3f( red, green, blue );
}

static void APIENTRY logColor3fv(const GLfloat *v)
{
   fprintf( winState.log_fp, "glColor3fv\n" );
   fflush(winState.log_fp);
   dllColor3fv( v );
}

static void APIENTRY logColor3i(GLint red, GLint green, GLint blue)
{
   fprintf( winState.log_fp, "glColor3i\n" );
   fflush(winState.log_fp);
   dllColor3i( red, green, blue );
}

static void APIENTRY logColor3iv(const GLint *v)
{
   fprintf( winState.log_fp, "glColor3iv\n" );
   fflush(winState.log_fp);
   dllColor3iv( v );
}

static void APIENTRY logColor3s(GLshort red, GLshort green, GLshort blue)
{
   fprintf( winState.log_fp, "glColor3s\n" );
   fflush(winState.log_fp);
   dllColor3s( red, green, blue );
}

static void APIENTRY logColor3sv(const GLshort *v)
{
   fprintf( winState.log_fp, "glColor3sv\n" );
   fflush(winState.log_fp);
   dllColor3sv( v );
}

static void APIENTRY logColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
   fprintf( winState.log_fp, "glColor3ub\n" );
   fflush(winState.log_fp);
   dllColor3ub( red, green, blue );
}

static void APIENTRY logColor3ubv(const GLubyte *v)
{
   fprintf( winState.log_fp, "glColor3ubv\n" );
   fflush(winState.log_fp);
   dllColor3ubv( v );
}

#define SIG( x ) fprintf( winState.log_fp, x "\n" ); fflush(winState.log_fp)

static void APIENTRY logColor3ui(GLuint red, GLuint green, GLuint blue)
{
   SIG( "glColor3ui" );
   dllColor3ui( red, green, blue );
}

static void APIENTRY logColor3uiv(const GLuint *v)
{
   SIG( "glColor3uiv" );
   dllColor3uiv( v );
}

static void APIENTRY logColor3us(GLushort red, GLushort green, GLushort blue)
{
   SIG( "glColor3us" );
   dllColor3us( red, green, blue );
}

static void APIENTRY logColor3usv(const GLushort *v)
{
   SIG( "glColor3usv" );
   dllColor3usv( v );
}

static void APIENTRY logColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
   SIG( "glColor4b" );
   dllColor4b( red, green, blue, alpha );
}

static void APIENTRY logColor4bv(const GLbyte *v)
{
   SIG( "glColor4bv" );
   dllColor4bv( v );
}

static void APIENTRY logColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
   SIG( "glColor4d" );
   dllColor4d( red, green, blue, alpha );
}
static void APIENTRY logColor4dv(const GLdouble *v)
{
   SIG( "glColor4dv" );
   dllColor4dv( v );
}
static void APIENTRY logColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   fprintf( winState.log_fp, "glColor4f( %f,%f,%f,%f )\n", red, green, blue, alpha );
   fflush(winState.log_fp);
   dllColor4f( red, green, blue, alpha );
}
static void APIENTRY logColor4fv(const GLfloat *v)
{
   fprintf( winState.log_fp, "glColor4fv( %f,%f,%f,%f )\n", v[0], v[1], v[2], v[3] );
   fflush(winState.log_fp);
   dllColor4fv( v );
}
static void APIENTRY logColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
   SIG( "glColor4i" );
   dllColor4i( red, green, blue, alpha );
}
static void APIENTRY logColor4iv(const GLint *v)
{
   SIG( "glColor4iv" );
   dllColor4iv( v );
}
static void APIENTRY logColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
   SIG( "glColor4s" );
   dllColor4s( red, green, blue, alpha );
}
static void APIENTRY logColor4sv(const GLshort *v)
{
   SIG( "glColor4sv" );
   dllColor4sv( v );
}
static void APIENTRY logColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
   SIG( "glColor4ub" );
   dllColor4b( red, green, blue, alpha );
}
static void APIENTRY logColor4ubv(const GLubyte *v)
{
   SIG( "glColor4ubv" );
   dllColor4ubv( v );
}
static void APIENTRY logColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   SIG( "glColor4ui" );
   dllColor4ui( red, green, blue, alpha );
}
static void APIENTRY logColor4uiv(const GLuint *v)
{
   SIG( "glColor4uiv" );
   dllColor4uiv( v );
}
static void APIENTRY logColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
   SIG( "glColor4us" );
   dllColor4us( red, green, blue, alpha );
}
static void APIENTRY logColor4usv(const GLushort *v)
{
   SIG( "glColor4usv" );
   dllColor4usv( v );
}
static void APIENTRY logColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   SIG( "glColorMask" );
   dllColorMask( red, green, blue, alpha );
}
static void APIENTRY logColorMaterial(GLenum face, GLenum mode)
{
   SIG( "glColorMaterial" );
   dllColorMaterial( face, mode );
}

static void APIENTRY logColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
   fprintf( winState.log_fp, "glColorPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
   fflush(winState.log_fp);
   dllColorPointer( size, type, stride, pointer );
}

static void APIENTRY logCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   SIG( "glCopyPixels" );
   dllCopyPixels( x, y, width, height, type );
}

static void APIENTRY logCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
   SIG( "glCopyTexImage1D" );
   dllCopyTexImage1D( target, level, internalFormat, x, y, width, border );
}

static void APIENTRY logCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   SIG( "glCopyTexImage2D" );
   dllCopyTexImage2D( target, level, internalFormat, x, y, width, height, border );
}

static void APIENTRY logCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   SIG( "glCopyTexSubImage1D" );
   dllCopyTexSubImage1D( target, level, xoffset, x, y, width );
}

static void APIENTRY logCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   SIG( "glCopyTexSubImage2D" );
   dllCopyTexSubImage2D( target, level, xoffset, yoffset, x, y, width, height );
}

static void APIENTRY logCullFace(GLenum mode)
{
   fprintf( winState.log_fp, "glCullFace( %s )\n", ( mode == GL_FRONT ) ? "GL_FRONT" : "GL_BACK" );
   fflush(winState.log_fp);
   dllCullFace( mode );
}

static void APIENTRY logDeleteLists(GLuint list, GLsizei range)
{
   SIG( "glDeleteLists" );
   dllDeleteLists( list, range );
}

static void APIENTRY logDeleteTextures(GLsizei n, const GLuint *textures)
{
   SIG( "glDeleteTextures" );
   dllDeleteTextures( n, textures );
}

static void APIENTRY logDepthFunc(GLenum func)
{
   fprintf( winState.log_fp, "glDepthFunc( %s )\n", FuncToString( func ) );
   fflush(winState.log_fp);
   dllDepthFunc( func );
}

static void APIENTRY logDepthMask(GLboolean flag)
{
   fprintf( winState.log_fp, "glDepthMask( %s )\n", BooleanToString( flag ) );
   fflush(winState.log_fp);
   dllDepthMask( flag );
}

static void APIENTRY logDepthRange(GLclampd zNear, GLclampd zFar)
{
   fprintf( winState.log_fp, "glDepthRange( %f, %f )\n", ( F32 ) zNear, ( F32 ) zFar );
   fflush(winState.log_fp);
   dllDepthRange( zNear, zFar );
}

static void APIENTRY logDisable(GLenum cap)
{
   fprintf( winState.log_fp, "glDisable( %s )\n", CapToString( cap ) );
   fflush(winState.log_fp);
   dllDisable( cap );
}

static void APIENTRY logDisableClientState(GLenum array)
{
   fprintf( winState.log_fp, "glDisableClientState( %s )\n", CapToString( array ) );
   fflush(winState.log_fp);
   dllDisableClientState( array );
}

static void APIENTRY logDrawArrays(GLenum mode, GLint first, GLsizei count)
{
   SIG( "glDrawArrays" );
   dllDrawArrays( mode, first, count );
}

static void APIENTRY logDrawBuffer(GLenum mode)
{
   SIG( "glDrawBuffer" );
   dllDrawBuffer( mode );
}

static void APIENTRY logDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
   fprintf( winState.log_fp, "glDrawElements( %s, %d, %s, MEM )\n", PrimToString( mode ), count, TypeToString( type ) );
   fflush(winState.log_fp);
   dllDrawElements( mode, count, type, indices );
}

static void APIENTRY logDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
   SIG( "glDrawPixels" );
   dllDrawPixels( width, height, format, type, pixels );
}

static void APIENTRY logEdgeFlag(GLboolean flag)
{
   SIG( "glEdgeFlag" );
   dllEdgeFlag( flag );
}

static void APIENTRY logEdgeFlagPointer(GLsizei stride, const void *pointer)
{
   SIG( "glEdgeFlagPointer" );
   dllEdgeFlagPointer( stride, pointer );
}

static void APIENTRY logEdgeFlagv(const GLboolean *flag)
{
   SIG( "glEdgeFlagv" );
   dllEdgeFlagv( flag );
}

static void APIENTRY logEnable(GLenum cap)
{
   fprintf( winState.log_fp, "glEnable( %s )\n", CapToString( cap ) );
   fflush(winState.log_fp);
   dllEnable( cap );
}

static void APIENTRY logEnableClientState(GLenum array)
{
   fprintf( winState.log_fp, "glEnableClientState( %s )\n", CapToString( array ) );
   fflush(winState.log_fp);
   dllEnableClientState( array );
}

static void APIENTRY logEnd(void)
{
   SIG( "glEnd" );
   dllEnd();
}

static void APIENTRY logEndList(void)
{
   SIG( "glEndList" );
   dllEndList();
}

static void APIENTRY logEvalCoord1d(GLdouble u)
{
   SIG( "glEvalCoord1d" );
   dllEvalCoord1d( u );
}

static void APIENTRY logEvalCoord1dv(const GLdouble *u)
{
   SIG( "glEvalCoord1dv" );
   dllEvalCoord1dv( u );
}

static void APIENTRY logEvalCoord1f(GLfloat u)
{
   SIG( "glEvalCoord1f" );
   dllEvalCoord1f( u );
}

static void APIENTRY logEvalCoord1fv(const GLfloat *u)
{
   SIG( "glEvalCoord1fv" );
   dllEvalCoord1fv( u );
}
static void APIENTRY logEvalCoord2d(GLdouble u, GLdouble v)
{
   SIG( "glEvalCoord2d" );
   dllEvalCoord2d( u, v );
}
static void APIENTRY logEvalCoord2dv(const GLdouble *u)
{
   SIG( "glEvalCoord2dv" );
   dllEvalCoord2dv( u );
}
static void APIENTRY logEvalCoord2f(GLfloat u, GLfloat v)
{
   SIG( "glEvalCoord2f" );
   dllEvalCoord2f( u, v );
}
static void APIENTRY logEvalCoord2fv(const GLfloat *u)
{
   SIG( "glEvalCoord2fv" );
   dllEvalCoord2fv( u );
}

static void APIENTRY logEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
   SIG( "glEvalMesh1" );
   dllEvalMesh1( mode, i1, i2 );
}
static void APIENTRY logEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   SIG( "glEvalMesh2" );
   dllEvalMesh2( mode, i1, i2, j1, j2 );
}
static void APIENTRY logEvalPoint1(GLint i)
{
   SIG( "glEvalPoint1" );
   dllEvalPoint1( i );
}
static void APIENTRY logEvalPoint2(GLint i, GLint j)
{
   SIG( "glEvalPoint2" );
   dllEvalPoint2( i, j );
}

static void APIENTRY logFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
   SIG( "glFeedbackBuffer" );
   dllFeedbackBuffer( size, type, buffer );
}

static void APIENTRY logFinish(void)
{
   SIG( "glFinish" );
   dllFinish();
}

static void APIENTRY logFlush(void)
{
   SIG( "glFlush" );
   dllFlush();
}

static void APIENTRY logFogf(GLenum pname, GLfloat param)
{
   SIG( "glFogf" );
   dllFogf( pname, param );
}

static void APIENTRY logFogfv(GLenum pname, const GLfloat *params)
{
   SIG( "glFogfv" );
   dllFogfv( pname, params );
}

static void APIENTRY logFogi(GLenum pname, GLint param)
{
   SIG( "glFogi" );
   dllFogi( pname, param );
}

static void APIENTRY logFogiv(GLenum pname, const GLint *params)
{
   SIG( "glFogiv" );
   dllFogiv( pname, params );
}

static void APIENTRY logFrontFace(GLenum mode)
{
   SIG( "glFrontFace" );
   dllFrontFace( mode );
}

static void APIENTRY logFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
   SIG( "glFrustum" );
   dllFrustum( left, right, bottom, top, zNear, zFar );
}

static GLuint APIENTRY logGenLists(GLsizei range)
{
   SIG( "glGenLists" );
   return dllGenLists( range );
}

static void APIENTRY logGenTextures(GLsizei n, GLuint *textures)
{
   SIG( "glGenTextures" );
   dllGenTextures( n, textures );
}

static void APIENTRY logGetBooleanv(GLenum pname, GLboolean *params)
{
   SIG( "glGetBooleanv" );
   dllGetBooleanv( pname, params );
}

static void APIENTRY logGetClipPlane(GLenum plane, GLdouble *equation)
{
   SIG( "glGetClipPlane" );
   dllGetClipPlane( plane, equation );
}

static void APIENTRY logGetDoublev(GLenum pname, GLdouble *params)
{
   SIG( "glGetDoublev" );
   dllGetDoublev( pname, params );
}

static GLenum APIENTRY logGetError(void)
{
   SIG( "glGetError" );
   return dllGetError();
}

static void APIENTRY logGetFloatv(GLenum pname, GLfloat *params)
{
   SIG( "glGetFloatv" );
   dllGetFloatv( pname, params );
}

static void APIENTRY logGetIntegerv(GLenum pname, GLint *params)
{
   SIG( "glGetIntegerv" );
   dllGetIntegerv( pname, params );
}

static void APIENTRY logGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
   SIG( "glGetLightfv" );
   dllGetLightfv( light, pname, params );
}

static void APIENTRY logGetLightiv(GLenum light, GLenum pname, GLint *params)
{
   SIG( "glGetLightiv" );
   dllGetLightiv( light, pname, params );
}

static void APIENTRY logGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
   SIG( "glGetMapdv" );
   dllGetMapdv( target, query, v );
}

static void APIENTRY logGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
   SIG( "glGetMapfv" );
   dllGetMapfv( target, query, v );
}

static void APIENTRY logGetMapiv(GLenum target, GLenum query, GLint *v)
{
   SIG( "glGetMapiv" );
   dllGetMapiv( target, query, v );
}

static void APIENTRY logGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
   SIG( "glGetMaterialfv" );
   dllGetMaterialfv( face, pname, params );
}

static void APIENTRY logGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
   SIG( "glGetMaterialiv" );
   dllGetMaterialiv( face, pname, params );
}

static void APIENTRY logGetPixelMapfv(GLenum map, GLfloat *values)
{
   SIG( "glGetPixelMapfv" );
   dllGetPixelMapfv( map, values );
}

static void APIENTRY logGetPixelMapuiv(GLenum map, GLuint *values)
{
   SIG( "glGetPixelMapuiv" );
   dllGetPixelMapuiv( map, values );
}

static void APIENTRY logGetPixelMapusv(GLenum map, GLushort *values)
{
   SIG( "glGetPixelMapusv" );
   dllGetPixelMapusv( map, values );
}

static void APIENTRY logGetPointerv(GLenum pname, GLvoid* *params)
{
   SIG( "glGetPointerv" );
   dllGetPointerv( pname, params );
}

static void APIENTRY logGetPolygonStipple(GLubyte *mask)
{
   SIG( "glGetPolygonStipple" );
   dllGetPolygonStipple( mask );
}

static const GLubyte * APIENTRY logGetString(GLenum name)
{
   SIG( "glGetString" );
   return dllGetString( name );
}

static void APIENTRY logGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
   SIG( "glGetTexEnvfv" );
   dllGetTexEnvfv( target, pname, params );
}

static void APIENTRY logGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
   SIG( "glGetTexEnviv" );
   dllGetTexEnviv( target, pname, params );
}

static void APIENTRY logGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
   SIG( "glGetTexGendv" );
   dllGetTexGendv( coord, pname, params );
}

static void APIENTRY logGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
   SIG( "glGetTexGenfv" );
   dllGetTexGenfv( coord, pname, params );
}

static void APIENTRY logGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
   SIG( "glGetTexGeniv" );
   dllGetTexGeniv( coord, pname, params );
}

static void APIENTRY logGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void *pixels)
{
   SIG( "glGetTexImage" );
   dllGetTexImage( target, level, format, type, pixels );
}
static void APIENTRY logGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params )
{
   SIG( "glGetTexLevelParameterfv" );
   dllGetTexLevelParameterfv( target, level, pname, params );
}

static void APIENTRY logGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
   SIG( "glGetTexLevelParameteriv" );
   dllGetTexLevelParameteriv( target, level, pname, params );
}

static void APIENTRY logGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   SIG( "glGetTexParameterfv" );
   dllGetTexParameterfv( target, pname, params );
}

static void APIENTRY logGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
   SIG( "glGetTexParameteriv" );
   dllGetTexParameteriv( target, pname, params );
}

static void APIENTRY logHint(GLenum target, GLenum mode)
{
   fprintf( winState.log_fp, "glHint( 0x%x, 0x%x )\n", target, mode );
   fflush(winState.log_fp);
   dllHint( target, mode );
}

static void APIENTRY logIndexMask(GLuint mask)
{
   SIG( "glIndexMask" );
   dllIndexMask( mask );
}

static void APIENTRY logIndexPointer(GLenum type, GLsizei stride, const void *pointer)
{
   SIG( "glIndexPointer" );
   dllIndexPointer( type, stride, pointer );
}

static void APIENTRY logIndexd(GLdouble c)
{
   SIG( "glIndexd" );
   dllIndexd( c );
}

static void APIENTRY logIndexdv(const GLdouble *c)
{
   SIG( "glIndexdv" );
   dllIndexdv( c );
}

static void APIENTRY logIndexf(GLfloat c)
{
   SIG( "glIndexf" );
   dllIndexf( c );
}

static void APIENTRY logIndexfv(const GLfloat *c)
{
   SIG( "glIndexfv" );
   dllIndexfv( c );
}

static void APIENTRY logIndexi(GLint c)
{
   SIG( "glIndexi" );
   dllIndexi( c );
}

static void APIENTRY logIndexiv(const GLint *c)
{
   SIG( "glIndexiv" );
   dllIndexiv( c );
}

static void APIENTRY logIndexs(GLshort c)
{
   SIG( "glIndexs" );
   dllIndexs( c );
}

static void APIENTRY logIndexsv(const GLshort *c)
{
   SIG( "glIndexsv" );
   dllIndexsv( c );
}

static void APIENTRY logIndexub(GLubyte c)
{
   SIG( "glIndexub" );
   dllIndexub( c );
}

static void APIENTRY logIndexubv(const GLubyte *c)
{
   SIG( "glIndexubv" );
   dllIndexubv( c );
}

static void APIENTRY logInitNames(void)
{
   SIG( "glInitNames" );
   dllInitNames();
}

static void APIENTRY logInterleavedArrays(GLenum format, GLsizei stride, const void *pointer)
{
   SIG( "glInterleavedArrays" );
   dllInterleavedArrays( format, stride, pointer );
}

static GLboolean APIENTRY logIsEnabled(GLenum cap)
{
   SIG( "glIsEnabled" );
   return dllIsEnabled( cap );
}
static GLboolean APIENTRY logIsList(GLuint list)
{
   SIG( "glIsList" );
   return dllIsList( list );
}
static GLboolean APIENTRY logIsTexture(GLuint texture)
{
   SIG( "glIsTexture" );
   return dllIsTexture( texture );
}

static void APIENTRY logLightModelf(GLenum pname, GLfloat param)
{
   SIG( "glLightModelf" );
   dllLightModelf( pname, param );
}

static void APIENTRY logLightModelfv(GLenum pname, const GLfloat *params)
{
   SIG( "glLightModelfv" );
   dllLightModelfv( pname, params );
}

static void APIENTRY logLightModeli(GLenum pname, GLint param)
{
   SIG( "glLightModeli" );
   dllLightModeli( pname, param );

}

static void APIENTRY logLightModeliv(GLenum pname, const GLint *params)
{
   SIG( "glLightModeliv" );
   dllLightModeliv( pname, params );
}

static void APIENTRY logLightf(GLenum light, GLenum pname, GLfloat param)
{
   SIG( "glLightf" );
   dllLightf( light, pname, param );
}

static void APIENTRY logLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
   SIG( "glLightfv" );
   dllLightfv( light, pname, params );
}

static void APIENTRY logLighti(GLenum light, GLenum pname, GLint param)
{
   SIG( "glLighti" );
   dllLighti( light, pname, param );
}

static void APIENTRY logLightiv(GLenum light, GLenum pname, const GLint *params)
{
   SIG( "glLightiv" );
   dllLightiv( light, pname, params );
}

static void APIENTRY logLineStipple(GLint factor, GLushort pattern)
{
   SIG( "glLineStipple" );
   dllLineStipple( factor, pattern );
}

static void APIENTRY logLineWidth(GLfloat width)
{
   SIG( "glLineWidth" );
   dllLineWidth( width );
}

static void APIENTRY logListBase(GLuint base)
{
   SIG( "glListBase" );
   dllListBase( base );
}

static void APIENTRY logLoadIdentity(void)
{
   SIG( "glLoadIdentity" );
   dllLoadIdentity();
}

static void APIENTRY logLoadMatrixd(const GLdouble *m)
{
   SIG( "glLoadMatrixd" );
   dllLoadMatrixd( m );
}

static void APIENTRY logLoadMatrixf(const GLfloat *m)
{
   SIG( "glLoadMatrixf" );
   dllLoadMatrixf( m );
}

static void APIENTRY logLoadName(GLuint name)
{
   SIG( "glLoadName" );
   dllLoadName( name );
}

static void APIENTRY logLogicOp(GLenum opcode)
{
   SIG( "glLogicOp" );
   dllLogicOp( opcode );
}

static void APIENTRY logMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
   SIG( "glMap1d" );
   dllMap1d( target, u1, u2, stride, order, points );
}

static void APIENTRY logMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
   SIG( "glMap1f" );
   dllMap1f( target, u1, u2, stride, order, points );
}

static void APIENTRY logMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
   SIG( "glMap2d" );
   dllMap2d( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void APIENTRY logMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
   SIG( "glMap2f" );
   dllMap2f( target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points );
}

static void APIENTRY logMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
   SIG( "glMapGrid1d" );
   dllMapGrid1d( un, u1, u2 );
}

static void APIENTRY logMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
   SIG( "glMapGrid1f" );
   dllMapGrid1f( un, u1, u2 );
}

static void APIENTRY logMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
   SIG( "glMapGrid2d" );
   dllMapGrid2d( un, u1, u2, vn, v1, v2 );
}
static void APIENTRY logMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
   SIG( "glMapGrid2f" );
   dllMapGrid2f( un, u1, u2, vn, v1, v2 );
}
static void APIENTRY logMaterialf(GLenum face, GLenum pname, GLfloat param)
{
   SIG( "glMaterialf" );
   dllMaterialf( face, pname, param );
}
static void APIENTRY logMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
   SIG( "glMaterialfv" );
   dllMaterialfv( face, pname, params );
}

static void APIENTRY logMateriali(GLenum face, GLenum pname, GLint param)
{
   SIG( "glMateriali" );
   dllMateriali( face, pname, param );
}

static void APIENTRY logMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
   SIG( "glMaterialiv" );
   dllMaterialiv( face, pname, params );
}

static void APIENTRY logMatrixMode(GLenum mode)
{
   SIG( "glMatrixMode" );
   dllMatrixMode( mode );
}

static void APIENTRY logMultMatrixd(const GLdouble *m)
{
   SIG( "glMultMatrixd" );
   dllMultMatrixd( m );
}

static void APIENTRY logMultMatrixf(const GLfloat *m)
{
   SIG( "glMultMatrixf" );
   dllMultMatrixf( m );
}

static void APIENTRY logNewList(GLuint list, GLenum mode)
{
   SIG( "glNewList" );
   dllNewList( list, mode );
}

static void APIENTRY logNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
   SIG ("glNormal3b" );
   dllNormal3b( nx, ny, nz );
}

static void APIENTRY logNormal3bv(const GLbyte *v)
{
   SIG( "glNormal3bv" );
   dllNormal3bv( v );
}

static void APIENTRY logNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
   SIG( "glNormal3d" );
   dllNormal3d( nx, ny, nz );
}

static void APIENTRY logNormal3dv(const GLdouble *v)
{
   SIG( "glNormal3dv" );
   dllNormal3dv( v );
}

static void APIENTRY logNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
   SIG( "glNormal3f" );
   dllNormal3f( nx, ny, nz );
}

static void APIENTRY logNormal3fv(const GLfloat *v)
{
   SIG( "glNormal3fv" );
   dllNormal3fv( v );
}
static void APIENTRY logNormal3i(GLint nx, GLint ny, GLint nz)
{
   SIG( "glNormal3i" );
   dllNormal3i( nx, ny, nz );
}
static void APIENTRY logNormal3iv(const GLint *v)
{
   SIG( "glNormal3iv" );
   dllNormal3iv( v );
}
static void APIENTRY logNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
   SIG( "glNormal3s" );
   dllNormal3s( nx, ny, nz );
}
static void APIENTRY logNormal3sv(const GLshort *v)
{
   SIG( "glNormal3sv" );
   dllNormal3sv( v );
}
static void APIENTRY logNormalPointer(GLenum type, GLsizei stride, const void *pointer)
{
   SIG( "glNormalPointer" );
   dllNormalPointer( type, stride, pointer );
}
static void APIENTRY logOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
   SIG( "glOrtho" );
   dllOrtho( left, right, bottom, top, zNear, zFar );
}

static void APIENTRY logPassThrough(GLfloat token)
{
   SIG( "glPassThrough" );
   dllPassThrough( token );
}

static void APIENTRY logPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values)
{
   SIG( "glPixelMapfv" );
   dllPixelMapfv( map, mapsize, values );
}

static void APIENTRY logPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values)
{
   SIG( "glPixelMapuiv" );
   dllPixelMapuiv( map, mapsize, values );
}

static void APIENTRY logPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values)
{
   SIG( "glPixelMapusv" );
   dllPixelMapusv( map, mapsize, values );
}
static void APIENTRY logPixelStoref(GLenum pname, GLfloat param)
{
   SIG( "glPixelStoref" );
   dllPixelStoref( pname, param );
}
static void APIENTRY logPixelStorei(GLenum pname, GLint param)
{
   SIG( "glPixelStorei" );
   dllPixelStorei( pname, param );
}
static void APIENTRY logPixelTransferf(GLenum pname, GLfloat param)
{
   SIG( "glPixelTransferf" );
   dllPixelTransferf( pname, param );
}

static void APIENTRY logPixelTransferi(GLenum pname, GLint param)
{
   SIG( "glPixelTransferi" );
   dllPixelTransferi( pname, param );
}

static void APIENTRY logPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
   SIG( "glPixelZoom" );
   dllPixelZoom( xfactor, yfactor );
}

static void APIENTRY logPointSize(GLfloat size)
{
   SIG( "glPointSize" );
   dllPointSize( size );
}

static void APIENTRY logPolygonMode(GLenum face, GLenum mode)
{
   fprintf( winState.log_fp, "glPolygonMode( 0x%x, 0x%x )\n", face, mode );
   fflush(winState.log_fp);
   dllPolygonMode( face, mode );
}

static void APIENTRY logPolygonOffset(GLfloat factor, GLfloat units)
{
   SIG( "glPolygonOffset" );
   dllPolygonOffset( factor, units );
}
static void APIENTRY logPolygonStipple(const GLubyte *mask )
{
   SIG( "glPolygonStipple" );
   dllPolygonStipple( mask );
}
static void APIENTRY logPopAttrib(void)
{
   SIG( "glPopAttrib" );
   dllPopAttrib();
}

static void APIENTRY logPopClientAttrib(void)
{
   SIG( "glPopClientAttrib" );
   dllPopClientAttrib();
}

static void APIENTRY logPopMatrix(void)
{
   SIG( "glPopMatrix" );
   dllPopMatrix();
}

static void APIENTRY logPopName(void)
{
   SIG( "glPopName" );
   dllPopName();
}

static void APIENTRY logPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   SIG( "glPrioritizeTextures" );
   dllPrioritizeTextures( n, textures, priorities );
}

static void APIENTRY logPushAttrib(GLbitfield mask)
{
   SIG( "glPushAttrib" );
   dllPushAttrib( mask );
}

static void APIENTRY logPushClientAttrib(GLbitfield mask)
{
   SIG( "glPushClientAttrib" );
   dllPushClientAttrib( mask );
}

static void APIENTRY logPushMatrix(void)
{
   SIG( "glPushMatrix" );
   dllPushMatrix();
}

static void APIENTRY logPushName(GLuint name)
{
   SIG( "glPushName" );
   dllPushName( name );
}

static void APIENTRY logRasterPos2d(GLdouble x, GLdouble y)
{
   SIG ("glRasterPot2d" );
   dllRasterPos2d( x, y );
}

static void APIENTRY logRasterPos2dv(const GLdouble *v)
{
   SIG( "glRasterPos2dv" );
   dllRasterPos2dv( v );
}

static void APIENTRY logRasterPos2f(GLfloat x, GLfloat y)
{
   SIG( "glRasterPos2f" );
   dllRasterPos2f( x, y );
}
static void APIENTRY logRasterPos2fv(const GLfloat *v)
{
   SIG( "glRasterPos2dv" );
   dllRasterPos2fv( v );
}
static void APIENTRY logRasterPos2i(GLint x, GLint y)
{
   SIG( "glRasterPos2if" );
   dllRasterPos2i( x, y );
}
static void APIENTRY logRasterPos2iv(const GLint *v)
{
   SIG( "glRasterPos2iv" );
   dllRasterPos2iv( v );
}
static void APIENTRY logRasterPos2s(GLshort x, GLshort y)
{
   SIG( "glRasterPos2s" );
   dllRasterPos2s( x, y );
}
static void APIENTRY logRasterPos2sv(const GLshort *v)
{
   SIG( "glRasterPos2sv" );
   dllRasterPos2sv( v );
}
static void APIENTRY logRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   SIG( "glRasterPos3d" );
   dllRasterPos3d( x, y, z );
}
static void APIENTRY logRasterPos3dv(const GLdouble *v)
{
   SIG( "glRasterPos3dv" );
   dllRasterPos3dv( v );
}
static void APIENTRY logRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   SIG( "glRasterPos3f" );
   dllRasterPos3f( x, y, z );
}
static void APIENTRY logRasterPos3fv(const GLfloat *v)
{
   SIG( "glRasterPos3fv" );
   dllRasterPos3fv( v );
}
static void APIENTRY logRasterPos3i(GLint x, GLint y, GLint z)
{
   SIG( "glRasterPos3i" );
   dllRasterPos3i( x, y, z );
}
static void APIENTRY logRasterPos3iv(const GLint *v)
{
   SIG( "glRasterPos3iv" );
   dllRasterPos3iv( v );
}
static void APIENTRY logRasterPos3s(GLshort x, GLshort y, GLshort z)
{
   SIG( "glRasterPos3s" );
   dllRasterPos3s( x, y, z );
}
static void APIENTRY logRasterPos3sv(const GLshort *v)
{
   SIG( "glRasterPos3sv" );
   dllRasterPos3sv( v );
}
static void APIENTRY logRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   SIG( "glRasterPos4d" );
   dllRasterPos4d( x, y, z, w );
}
static void APIENTRY logRasterPos4dv(const GLdouble *v)
{
   SIG( "glRasterPos4dv" );
   dllRasterPos4dv( v );
}
static void APIENTRY logRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   SIG( "glRasterPos4f" );
   dllRasterPos4f( x, y, z, w );
}
static void APIENTRY logRasterPos4fv(const GLfloat *v)
{
   SIG( "glRasterPos4fv" );
   dllRasterPos4fv( v );
}
static void APIENTRY logRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   SIG( "glRasterPos4i" );
   dllRasterPos4i( x, y, z, w );
}
static void APIENTRY logRasterPos4iv(const GLint *v)
{
   SIG( "glRasterPos4iv" );
   dllRasterPos4iv( v );
}
static void APIENTRY logRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   SIG( "glRasterPos4s" );
   dllRasterPos4s( x, y, z, w );
}
static void APIENTRY logRasterPos4sv(const GLshort *v)
{
   SIG( "glRasterPos4sv" );
   dllRasterPos4sv( v );
}
static void APIENTRY logReadBuffer(GLenum mode)
{
   SIG( "glReadBuffer" );
   dllReadBuffer( mode );
}
static void APIENTRY logReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels)
{
   SIG( "glReadPixels" );
   dllReadPixels( x, y, width, height, format, type, pixels );
}

static void APIENTRY logRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   SIG( "glRectd" );
   dllRectd( x1, y1, x2, y2 );
}

static void APIENTRY logRectdv(const GLdouble *v1, const GLdouble *v2)
{
   SIG( "glRectdv" );
   dllRectdv( v1, v2 );
}

static void APIENTRY logRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   SIG( "glRectf" );
   dllRectf( x1, y1, x2, y2 );
}

static void APIENTRY logRectfv(const GLfloat *v1, const GLfloat *v2)
{
   SIG( "glRectfv" );
   dllRectfv( v1, v2 );
}
static void APIENTRY logRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   SIG( "glRecti" );
   dllRecti( x1, y1, x2, y2 );
}
static void APIENTRY logRectiv(const GLint *v1, const GLint *v2)
{
   SIG( "glRectiv" );
   dllRectiv( v1, v2 );
}
static void APIENTRY logRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   SIG( "glRects" );
   dllRects( x1, y1, x2, y2 );
}
static void APIENTRY logRectsv(const GLshort *v1, const GLshort *v2)
{
   SIG( "glRectsv" );
   dllRectsv( v1, v2 );
}
static GLint APIENTRY logRenderMode(GLenum mode)
{
   SIG( "glRenderMode" );
   return dllRenderMode( mode );
}
static void APIENTRY logRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   SIG( "glRotated" );
   dllRotated( angle, x, y, z );
}

static void APIENTRY logRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   SIG( "glRotatef" );
   dllRotatef( angle, x, y, z );
}

static void APIENTRY logScaled(GLdouble x, GLdouble y, GLdouble z)
{
   SIG( "glScaled" );
   dllScaled( x, y, z );
}

static void APIENTRY logScalef(GLfloat x, GLfloat y, GLfloat z)
{
   SIG( "glScalef" );
   dllScalef( x, y, z );
}

static void APIENTRY logScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
   fprintf( winState.log_fp, "glScissor( %d, %d, %d, %d )\n", x, y, width, height );
   fflush(winState.log_fp);
   dllScissor( x, y, width, height );
}

static void APIENTRY logSelectBuffer(GLsizei size, GLuint *buffer)
{
   SIG( "glSelectBuffer" );
   dllSelectBuffer( size, buffer );
}

static void APIENTRY logShadeModel(GLenum mode)
{
   SIG( "glShadeModel" );
   dllShadeModel( mode );
}

static void APIENTRY logStencilFunc(GLenum func, GLint ref, GLuint mask)
{
   SIG( "glStencilFunc" );
   dllStencilFunc( func, ref, mask );
}

static void APIENTRY logStencilMask(GLuint mask)
{
   SIG( "glStencilMask" );
   dllStencilMask( mask );
}

static void APIENTRY logStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   SIG( "glStencilOp" );
   dllStencilOp( fail, zfail, zpass );
}

static void APIENTRY logTexCoord1d(GLdouble s)
{
   SIG( "glTexCoord1d" );
   dllTexCoord1d( s );
}

static void APIENTRY logTexCoord1dv(const GLdouble *v)
{
   SIG( "glTexCoord1dv" );
   dllTexCoord1dv( v );
}

static void APIENTRY logTexCoord1f(GLfloat s)
{
   SIG( "glTexCoord1f" );
   dllTexCoord1f( s );
}
static void APIENTRY logTexCoord1fv(const GLfloat *v)
{
   SIG( "glTexCoord1fv" );
   dllTexCoord1fv( v );
}
static void APIENTRY logTexCoord1i(GLint s)
{
   SIG( "glTexCoord1i" );
   dllTexCoord1i( s );
}
static void APIENTRY logTexCoord1iv(const GLint *v)
{
   SIG( "glTexCoord1iv" );
   dllTexCoord1iv( v );
}
static void APIENTRY logTexCoord1s(GLshort s)
{
   SIG( "glTexCoord1s" );
   dllTexCoord1s( s );
}
static void APIENTRY logTexCoord1sv(const GLshort *v)
{
   SIG( "glTexCoord1sv" );
   dllTexCoord1sv( v );
}
static void APIENTRY logTexCoord2d(GLdouble s, GLdouble t)
{
   SIG( "glTexCoord2d" );
   dllTexCoord2d( s, t );
}

static void APIENTRY logTexCoord2dv(const GLdouble *v)
{
   SIG( "glTexCoord2dv" );
   dllTexCoord2dv( v );
}
static void APIENTRY logTexCoord2f(GLfloat s, GLfloat t)
{
   SIG( "glTexCoord2f" );
   dllTexCoord2f( s, t );
}
static void APIENTRY logTexCoord2fv(const GLfloat *v)
{
   SIG( "glTexCoord2fv" );
   dllTexCoord2fv( v );
}
static void APIENTRY logTexCoord2i(GLint s, GLint t)
{
   SIG( "glTexCoord2i" );
   dllTexCoord2i( s, t );
}
static void APIENTRY logTexCoord2iv(const GLint *v)
{
   SIG( "glTexCoord2iv" );
   dllTexCoord2iv( v );
}
static void APIENTRY logTexCoord2s(GLshort s, GLshort t)
{
   SIG( "glTexCoord2s" );
   dllTexCoord2s( s, t );
}
static void APIENTRY logTexCoord2sv(const GLshort *v)
{
   SIG( "glTexCoord2sv" );
   dllTexCoord2sv( v );
}
static void APIENTRY logTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
   SIG( "glTexCoord3d" );
   dllTexCoord3d( s, t, r );
}
static void APIENTRY logTexCoord3dv(const GLdouble *v)
{
   SIG( "glTexCoord3dv" );
   dllTexCoord3dv( v );
}
static void APIENTRY logTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
   SIG( "glTexCoord3f" );
   dllTexCoord3f( s, t, r );
}
static void APIENTRY logTexCoord3fv(const GLfloat *v)
{
   SIG( "glTexCoord3fv" );
   dllTexCoord3fv( v );
}
static void APIENTRY logTexCoord3i(GLint s, GLint t, GLint r)
{
   SIG( "glTexCoord3i" );
   dllTexCoord3i( s, t, r );
}
static void APIENTRY logTexCoord3iv(const GLint *v)
{
   SIG( "glTexCoord3iv" );
   dllTexCoord3iv( v );
}
static void APIENTRY logTexCoord3s(GLshort s, GLshort t, GLshort r)
{
   SIG( "glTexCoord3s" );
   dllTexCoord3s( s, t, r );
}
static void APIENTRY logTexCoord3sv(const GLshort *v)
{
   SIG( "glTexCoord3sv" );
   dllTexCoord3sv( v );
}
static void APIENTRY logTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   SIG( "glTexCoord4d" );
   dllTexCoord4d( s, t, r, q );
}
static void APIENTRY logTexCoord4dv(const GLdouble *v)
{
   SIG( "glTexCoord4dv" );
   dllTexCoord4dv( v );
}
static void APIENTRY logTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   SIG( "glTexCoord4f" );
   dllTexCoord4f( s, t, r, q );
}
static void APIENTRY logTexCoord4fv(const GLfloat *v)
{
   SIG( "glTexCoord4fv" );
   dllTexCoord4fv( v );
}
static void APIENTRY logTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
   SIG( "glTexCoord4i" );
   dllTexCoord4i( s, t, r, q );
}
static void APIENTRY logTexCoord4iv(const GLint *v)
{
   SIG( "glTexCoord4iv" );
   dllTexCoord4iv( v );
}
static void APIENTRY logTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
   SIG( "glTexCoord4s" );
   dllTexCoord4s( s, t, r, q );
}
static void APIENTRY logTexCoord4sv(const GLshort *v)
{
   SIG( "glTexCoord4sv" );
   dllTexCoord4sv( v );
}
static void APIENTRY logTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
   fprintf( winState.log_fp, "glTexCoordPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
   fflush(winState.log_fp);
   dllTexCoordPointer( size, type, stride, pointer );
}

static void APIENTRY logTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
   fprintf( winState.log_fp, "glTexEnvf( 0x%x, 0x%x, %f )\n", target, pname, param );
   fflush(winState.log_fp);
   dllTexEnvf( target, pname, param );
}

static void APIENTRY logTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
   SIG( "glTexEnvfv" );
   dllTexEnvfv( target, pname, params );
}

static void APIENTRY logTexEnvi(GLenum target, GLenum pname, GLint param)
{
   fprintf( winState.log_fp, "glTexEnvi( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
   fflush(winState.log_fp);
   dllTexEnvi( target, pname, param );
}
static void APIENTRY logTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
   SIG( "glTexEnviv" );
   dllTexEnviv( target, pname, params );
}

static void APIENTRY logTexGend(GLenum coord, GLenum pname, GLdouble param)
{
   SIG( "glTexGend" );
   dllTexGend( coord, pname, param );
}

static void APIENTRY logTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
   SIG( "glTexGendv" );
   dllTexGendv( coord, pname, params );
}

static void APIENTRY logTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
   SIG( "glTexGenf" );
   dllTexGenf( coord, pname, param );
}
static void APIENTRY logTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
//   fprintf( winState.log_fp, "glTexGenfv( %s, (%f, %f, %f, %f) )\n", CoordToString( coord ), params[0], params[1], params[2], params[3]);
   fflush(winState.log_fp);
   fprintf( winState.log_fp, "glTexGenfv( %s, MEM )\n", CoordToString( coord ));
   fflush(winState.log_fp);
   dllTexGenfv( coord, pname, params );
}
static void APIENTRY logTexGeni(GLenum coord, GLenum pname, GLint param)
{
   SIG( "glTexGeni" );
   dllTexGeni( coord, pname, param );
}
static void APIENTRY logTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
   SIG( "glTexGeniv" );
   dllTexGeniv( coord, pname, params );
}
static void APIENTRY logTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels)
{
   SIG( "glTexImage1D" );
   dllTexImage1D( target, level, internalformat, width, border, format, type, pixels );
}
static void APIENTRY logTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
   SIG( "glTexImage2D" );
   dllTexImage2D( target, level, internalformat, width, height, border, format, type, pixels );
}

static void APIENTRY logTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   fprintf( winState.log_fp, "glTexParameterf( 0x%x, 0x%x, %f )\n", target, pname, param );
   fflush(winState.log_fp);
   dllTexParameterf( target, pname, param );
}

static void APIENTRY logTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   SIG( "glTexParameterfv" );
   dllTexParameterfv( target, pname, params );
}
static void APIENTRY logTexParameteri(GLenum target, GLenum pname, GLint param)
{
   fprintf( winState.log_fp, "glTexParameteri( 0x%x, 0x%x, 0x%x )\n", target, pname, param );
   fflush(winState.log_fp);
   dllTexParameteri( target, pname, param );
}
static void APIENTRY logTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   SIG( "glTexParameteriv" );
   dllTexParameteriv( target, pname, params );
}
static void APIENTRY logTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels)
{
   SIG( "glTexSubImage1D" );
   dllTexSubImage1D( target, level, xoffset, width, format, type, pixels );
}
static void APIENTRY logTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
   SIG( "glTexSubImage2D" );
   dllTexSubImage2D( target, level, xoffset, yoffset, width, height, format, type, pixels );
}
static void APIENTRY logTranslated(GLdouble x, GLdouble y, GLdouble z)
{
   SIG( "glTranslated" );
   dllTranslated( x, y, z );
}

static void APIENTRY logTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
   SIG( "glTranslatef" );
   dllTranslatef( x, y, z );
}

static void APIENTRY logVertex2d(GLdouble x, GLdouble y)
{
   SIG( "glVertex2d" );
   dllVertex2d( x, y );
}

static void APIENTRY logVertex2dv(const GLdouble *v)
{
   SIG( "glVertex2dv" );
   dllVertex2dv( v );
}
static void APIENTRY logVertex2f(GLfloat x, GLfloat y)
{
   SIG( "glVertex2f" );
   dllVertex2f( x, y );
}
static void APIENTRY logVertex2fv(const GLfloat *v)
{
   SIG( "glVertex2fv" );
   dllVertex2fv( v );
}
static void APIENTRY logVertex2i(GLint x, GLint y)
{
   SIG( "glVertex2i" );
   dllVertex2i( x, y );
}
static void APIENTRY logVertex2iv(const GLint *v)
{
   SIG( "glVertex2iv" );
   dllVertex2iv( v );
}
static void APIENTRY logVertex2s(GLshort x, GLshort y)
{
   SIG( "glVertex2s" );
   dllVertex2s( x, y );
}
static void APIENTRY logVertex2sv(const GLshort *v)
{
   SIG( "glVertex2sv" );
   dllVertex2sv( v );
}
static void APIENTRY logVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
   SIG( "glVertex3d" );
   dllVertex3d( x, y, z );
}
static void APIENTRY logVertex3dv(const GLdouble *v)
{
   SIG( "glVertex3dv" );
   dllVertex3dv( v );
}
static void APIENTRY logVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
   SIG( "glVertex3f" );
   dllVertex3f( x, y, z );
}
static void APIENTRY logVertex3fv(const GLfloat *v)
{
   SIG( "glVertex3fv" );
   dllVertex3fv( v );
}
static void APIENTRY logVertex3i(GLint x, GLint y, GLint z)
{
   SIG( "glVertex3i" );
   dllVertex3i( x, y, z );
}
static void APIENTRY logVertex3iv(const GLint *v)
{
   SIG( "glVertex3iv" );
   dllVertex3iv( v );
}
static void APIENTRY logVertex3s(GLshort x, GLshort y, GLshort z)
{
   SIG( "glVertex3s" );
   dllVertex3s( x, y, z );
}
static void APIENTRY logVertex3sv(const GLshort *v)
{
   SIG( "glVertex3sv" );
   dllVertex3sv( v );
}
static void APIENTRY logVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   SIG( "glVertex4d" );
   dllVertex4d( x, y, z, w );
}
static void APIENTRY logVertex4dv(const GLdouble *v)
{
   SIG( "glVertex4dv" );
   dllVertex4dv( v );
}
static void APIENTRY logVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   SIG( "glVertex4f" );
   dllVertex4f( x, y, z, w );
}
static void APIENTRY logVertex4fv(const GLfloat *v)
{
   SIG( "glVertex4fv" );
   dllVertex4fv( v );
}
static void APIENTRY logVertex4i(GLint x, GLint y, GLint z, GLint w)
{
   SIG( "glVertex4i" );
   dllVertex4i( x, y, z, w );
}
static void APIENTRY logVertex4iv(const GLint *v)
{
   SIG( "glVertex4iv" );
   dllVertex4iv( v );
}
static void APIENTRY logVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   SIG( "glVertex4s" );
   dllVertex4s( x, y, z, w );
}
static void APIENTRY logVertex4sv(const GLshort *v)
{
   SIG( "glVertex4sv" );
   dllVertex4sv( v );
}
static void APIENTRY logVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
   fprintf( winState.log_fp, "glVertexPointer( %d, %s, %d, MEM )\n", size, TypeToString( type ), stride );
   fflush(winState.log_fp);
   dllVertexPointer( size, type, stride, pointer );
}
static void APIENTRY logViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   fprintf( winState.log_fp, "glViewport( %d, %d, %d, %d )\n", x, y, width, height );
   fflush(winState.log_fp);
   dllViewport( x, y, width, height );
}

static void APIENTRY logColorTableEXT(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void* data)
{
   AssertFatal(dllColorTableEXT != NULL, "Error, shouldn't have called unsupported paletted_texture extension");

   fprintf(winState.log_fp, "glColorTableEXT(%d, %d, %d, %d, %d, <data>)\n",
           target,
           internalFormat,
           width,
           format,
           type);
   fflush(winState.log_fp);
   dllColorTableEXT(target, internalFormat, width, format, type, data);
}

static void APIENTRY logLockArraysEXT(GLint first, GLsizei count)
{
   AssertFatal(dllLockArraysEXT != NULL, "Error, shouldn't have called unsupported compiled_vertex_array extension");

   fprintf( winState.log_fp, "glLockArraysEXT( %d, %d )\n", first, count);
   fflush(winState.log_fp);
   dllLockArraysEXT(first, count);
}

static void APIENTRY logUnlockArraysEXT()
{
   AssertFatal(dllLockArraysEXT != NULL, "Error, shouldn't have called unsupported compiled_vertex_array extension");

   SIG("glUnlockArraysEXT");
   dllUnlockArraysEXT();
}

/* ARB_multitexture */

static const char* gARBMTenums[] = {
   "GL_TEXTURE0_ARB",  "GL_TEXTURE1_ARB",  "GL_TEXTURE2_ARB",
   "GL_TEXTURE3_ARB",  "GL_TEXTURE4_ARB",  "GL_TEXTURE5_ARB",
   "GL_TEXTURE6_ARB",  "GL_TEXTURE7_ARB",  "GL_TEXTURE8_ARB",
   "GL_TEXTURE9_ARB",  "GL_TEXTURE10_ARB", "GL_TEXTURE11_ARB",
   "GL_TEXTURE12_ARB", "GL_TEXTURE13_ARB", "GL_TEXTURE14_ARB",
   "GL_TEXTURE15_ARB", "GL_TEXTURE16_ARB", "GL_TEXTURE17_ARB",
   "GL_TEXTURE18_ARB", "GL_TEXTURE19_ARB", "GL_TEXTURE20_ARB",
   "GL_TEXTURE21_ARB", "GL_TEXTURE22_ARB", "GL_TEXTURE23_ARB",
   "GL_TEXTURE24_ARB", "GL_TEXTURE25_ARB", "GL_TEXTURE26_ARB",
   "GL_TEXTURE27_ARB", "GL_TEXTURE28_ARB", "GL_TEXTURE29_ARB",
   "GL_TEXTURE30_ARB", "GL_TEXTURE31_ARB"
};

static void APIENTRY logActiveTextureARB(GLenum target)
{
   U32 index = target - GL_TEXTURE0_ARB;

   fprintf( winState.log_fp, "glActiveTexturesARB( %s )\n", gARBMTenums[index]);
   fflush(winState.log_fp);
   dllActiveTextureARB(target);
}

static void APIENTRY logClientActiveTextureARB(GLenum target)
{
   U32 index = target - GL_TEXTURE0_ARB;

   fprintf( winState.log_fp, "glClientActiveTexturesARB( %s )\n", gARBMTenums[index]);
   fflush(winState.log_fp);
   dllClientActiveTextureARB(target);
}

static void APIENTRY logMultiTexCoord2fARB(GLenum texture, GLfloat x, GLfloat y)
{
   U32 index = texture - GL_TEXTURE0_ARB;

   fprintf( winState.log_fp, "glMultiTexCoord2fARB( %s, %f, %f )\n", gARBMTenums[index], x, y);
   fflush(winState.log_fp);
   dllMultiTexCoord2fARB(texture, x, y);
}

static void APIENTRY logMultiTexCoord2fvARB(GLenum texture, const GLfloat* p)
{
   U32 index = texture - GL_TEXTURE0_ARB;

   fprintf( winState.log_fp, "glMultiTexCoord2fARB( %s, [%f, %f] )\n", gARBMTenums[index], p[0], p[1]);
   fflush(winState.log_fp);
   dllMultiTexCoord2fvARB(texture, p);
}

/* NV_vertex_array_range */

static void APIENTRY logVertexArrayRangeNV(GLsizei length, void* pointer)
{
   fprintf(winState.log_fp, "glVertexArrayRangeNV( %d, MEMORY )", length);
   fflush(winState.log_fp);
   dllVertexArrayRangeNV(length, pointer);
}

static void APIENTRY logFlushVertexArrayRangeNV()
{
   SIG("glFlushVertexArrayRangeNV");
   dllFlushVertexArrayRangeNV();
}

static void* APIENTRY logAllocateMemoryNV(GLsizei length, GLfloat read, GLfloat write, GLfloat priority)
{
   fprintf(winState.log_fp, "wglAllocateMemoryNV( %d, %g, %g, %g)", length, read, write, priority);
   fflush(winState.log_fp);
   return dllAllocateMemoryNV(length, read, write, priority);
}

static void APIENTRY logFreeMemoryNV(void* pointer)
{
   SIG("glFreeMemoryNV(MEM)");
   dllFreeMemoryNV(pointer);
}

/* EXT_fog_coord */

static void APIENTRY logFogCoordfEXT(GLfloat coord)
{
   fprintf( winState.log_fp, "glFogCoordEXT(%f)\n", coord);
   fflush(winState.log_fp);
   dllFogCoordfEXT(coord);
}

static void APIENTRY logFogCoordPointerEXT(GLenum type, GLsizei stride, void *pointer)
{
   fprintf( winState.log_fp, "glFogCoordPointerEXT(%s, %d, MEMORY)\n", TypeToString(type), stride);
   fflush(winState.log_fp);
   dllFogCoordPointerEXT(type, stride, pointer);
}

/* ARB_texture_compression */

static void APIENTRY logCompressedTexImage3DARB(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void* data)
{
   fprintf( winState.log_fp, "glCompressedTexImage3DARB(...)\n");
   fflush(winState.log_fp);
   dllCompressedTexImage3DARB(target, level, internalformat, width, height, depth, border, imageSize, data);
}

static void APIENTRY logCompressedTexImage2DARB(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
   fprintf( winState.log_fp, "glCompressedTexImage3DARB(...)\n");
   fflush(winState.log_fp);
   dllCompressedTexImage2DARB(target, level, internalformat, width, height, border, imageSize, data);
}

static void APIENTRY logCompressedTexImage1DARB(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const void* data)
{
   fprintf( winState.log_fp, "glCompressedTexImage3DARB(...)\n");
   fflush(winState.log_fp);
   dllCompressedTexImage1DARB(target, level, internalformat, width, border, imageSize, data);
}

static void APIENTRY logCompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void* data)
{
   fprintf( winState.log_fp, "glCompressedTexSubImage3DARB(...)\n");
   fflush(winState.log_fp);
   dllCompressedTexSubImage3DARB(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
}

static void APIENTRY logCompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
   fprintf( winState.log_fp, "glCompressedTexSubImage2DARB(...)\n");
   fflush(winState.log_fp);
   dllCompressedTexSubImage2DARB(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

static void APIENTRY logCompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data)
{
   fprintf( winState.log_fp, "glCompressedTexSubImage1DARB(...)\n");
   fflush(winState.log_fp);
   dllCompressedTexSubImage1DARB(target, level, xoffset, width, format, imageSize, data);
}

static void APIENTRY logGetCompressedTexImageARB(GLenum target, GLint lod, void* img)
{
   fprintf( winState.log_fp, "glGetCompressedTexImage3DARB(...)\n");
   fflush(winState.log_fp);
   dllGetCompressedTexImageARB(target, lod, img);
}

/* EXT_vertex_buffer */

static GLboolean APIENTRY logAvailableVertexBufferEXT()
{
	fprintf( winState.log_fp, "glAvailableVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	return dllAvailableVertexBufferEXT();
}

static GLint APIENTRY logAllocateVertexBufferEXT(GLsizei size, GLint format, GLboolean preserve)
{
	fprintf( winState.log_fp, "glAllocateVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	return dllAllocateVertexBufferEXT(size, format, preserve);
}

static void * APIENTRY logLockVertexBufferEXT(GLint handle, GLsizei size)
{
	fprintf( winState.log_fp, "glLockVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	return dllLockVertexBufferEXT(handle, size);
}

static void APIENTRY logUnlockVertexBufferEXT(GLint handle)
{
	fprintf( winState.log_fp, "glUnlockVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	dllUnlockVertexBufferEXT(handle);
}

static void APIENTRY logSetVertexBufferEXT(GLint handle)
{
	fprintf( winState.log_fp, "glSetVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	dllSetVertexBufferEXT(handle);
}

static void APIENTRY logOffsetVertexBufferEXT(GLint handle, GLuint offset)
{
	fprintf( winState.log_fp, "glOffsetVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	dllOffsetVertexBufferEXT(handle, offset);
}

static void APIENTRY logFillVertexBufferEXT(GLint handle, GLint first, GLsizei count)
{
	fprintf( winState.log_fp, "glFillVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	dllFillVertexBufferEXT(handle, first, count);
}

static void APIENTRY logFreeVertexBufferEXT(GLint handle)
{
	fprintf( winState.log_fp, "glFreeVertexBufferEXT(...)\n");
	fflush(winState.log_fp);
	dllFreeVertexBufferEXT(handle);
}


/*
** QGL_Shutdown
**
** Unloads the specified DLL then nulls out all the proc pointers.  This
** is only called during a hard shutdown of the OGL subsystem (e.g. vid_restart).
*/
void QGL_Shutdown( void )
{
   if ( winState.hinstOpenGL )
      FreeLibrary( winState.hinstOpenGL );
   winState.hinstOpenGL = NULL;

   if ( winState.hinstGLU )
      FreeLibrary( winState.hinstGLU );
   winState.hinstGLU = NULL;

   gGLState.suppSwapInterval = false;

   // GLU Functions
   gluErrorString              = NULL;
   gluGetString                = NULL;
   gluOrtho2D                  = NULL;
   gluPerspective              = NULL;
   gluPickMatrix               = NULL;
   gluLookAt                   = NULL;
   gluProject                  = NULL;
   gluUnProject                = NULL;
   gluScaleImage               = NULL;
   gluBuild1DMipmaps           = NULL;
   gluBuild2DMipmaps           = NULL;

   // GL Functions
   glAccum                     = NULL;
   glAlphaFunc                 = NULL;
   glAreTexturesResident       = NULL;
   glArrayElement              = NULL;
   glBegin                     = NULL;
   glBindTexture               = NULL;
   glBitmap                    = NULL;
   glBlendFunc                 = NULL;
   glCallList                  = NULL;
   glCallLists                 = NULL;
   glClear                     = NULL;
   glClearAccum                = NULL;
   glClearColor                = NULL;
   glClearDepth                = NULL;
   glClearIndex                = NULL;
   glClearStencil              = NULL;
   glClipPlane                 = NULL;
   glColor3b                   = NULL;
   glColor3bv                  = NULL;
   glColor3d                   = NULL;
   glColor3dv                  = NULL;
   glColor3f                   = NULL;
   glColor3fv                  = NULL;
   glColor3i                   = NULL;
   glColor3iv                  = NULL;
   glColor3s                   = NULL;
   glColor3sv                  = NULL;
   glColor3ub                  = NULL;
   glColor3ubv                 = NULL;
   glColor3ui                  = NULL;
   glColor3uiv                 = NULL;
   glColor3us                  = NULL;
   glColor3usv                 = NULL;
   glColor4b                   = NULL;
   glColor4bv                  = NULL;
   glColor4d                   = NULL;
   glColor4dv                  = NULL;
   glColor4f                   = NULL;
   glColor4fv                  = NULL;
   glColor4i                   = NULL;
   glColor4iv                  = NULL;
   glColor4s                   = NULL;
   glColor4sv                  = NULL;
   glColor4ub                  = NULL;
   glColor4ubv                 = NULL;
   glColor4ui                  = NULL;
   glColor4uiv                 = NULL;
   glColor4us                  = NULL;
   glColor4usv                 = NULL;
   glColorMask                 = NULL;
   glColorMaterial             = NULL;
   glColorPointer              = NULL;
   glCopyPixels                = NULL;
   glCopyTexImage1D            = NULL;
   glCopyTexImage2D            = NULL;
   glCopyTexSubImage1D         = NULL;
   glCopyTexSubImage2D         = NULL;
   glCullFace                  = NULL;
   glDeleteLists               = NULL;
   glDeleteTextures            = NULL;
   glDepthFunc                 = NULL;
   glDepthMask                 = NULL;
   glDepthRange                = NULL;
   glDisable                   = NULL;
   glDisableClientState        = NULL;
   glDrawArrays                = NULL;
   glDrawBuffer                = NULL;
   glDrawElements              = NULL;
   glDrawPixels                = NULL;
   glEdgeFlag                  = NULL;
   glEdgeFlagPointer           = NULL;
   glEdgeFlagv                 = NULL;
   glEnable                    = NULL;
   glEnableClientState         = NULL;
   glEnd                       = NULL;
   glEndList                   = NULL;
   glEvalCoord1d               = NULL;
   glEvalCoord1dv              = NULL;
   glEvalCoord1f               = NULL;
   glEvalCoord1fv              = NULL;
   glEvalCoord2d               = NULL;
   glEvalCoord2dv              = NULL;
   glEvalCoord2f               = NULL;
   glEvalCoord2fv              = NULL;
   glEvalMesh1                 = NULL;
   glEvalMesh2                 = NULL;
   glEvalPoint1                = NULL;
   glEvalPoint2                = NULL;
   glFeedbackBuffer            = NULL;
   glFinish                    = NULL;
   glFlush                     = NULL;
   glFogf                      = NULL;
   glFogfv                     = NULL;
   glFogi                      = NULL;
   glFogiv                     = NULL;
   glFrontFace                 = NULL;
   glFrustum                   = NULL;
   glGenLists                  = NULL;
   glGenTextures               = NULL;
   glGetBooleanv               = NULL;
   glGetClipPlane              = NULL;
   glGetDoublev                = NULL;
   glGetError                  = NULL;
   glGetFloatv                 = NULL;
   glGetIntegerv               = NULL;
   glGetLightfv                = NULL;
   glGetLightiv                = NULL;
   glGetMapdv                  = NULL;
   glGetMapfv                  = NULL;
   glGetMapiv                  = NULL;
   glGetMaterialfv             = NULL;
   glGetMaterialiv             = NULL;
   glGetPixelMapfv             = NULL;
   glGetPixelMapuiv            = NULL;
   glGetPixelMapusv            = NULL;
   glGetPointerv               = NULL;
   glGetPolygonStipple         = NULL;
   glGetString                 = NULL;
   glGetTexEnvfv               = NULL;
   glGetTexEnviv               = NULL;
   glGetTexGendv               = NULL;
   glGetTexGenfv               = NULL;
   glGetTexGeniv               = NULL;
   glGetTexImage               = NULL;
   glGetTexLevelParameterfv    = NULL;
   glGetTexLevelParameteriv    = NULL;
   glGetTexParameterfv         = NULL;
   glGetTexParameteriv         = NULL;
   glHint                      = NULL;
   glIndexMask                 = NULL;
   glIndexPointer              = NULL;
   glIndexd                    = NULL;
   glIndexdv                   = NULL;
   glIndexf                    = NULL;
   glIndexfv                   = NULL;
   glIndexi                    = NULL;
   glIndexiv                   = NULL;
   glIndexs                    = NULL;
   glIndexsv                   = NULL;
   glIndexub                   = NULL;
   glIndexubv                  = NULL;
   glInitNames                 = NULL;
   glInterleavedArrays         = NULL;
   glIsEnabled                 = NULL;
   glIsList                    = NULL;
   glIsTexture                 = NULL;
   glLightModelf               = NULL;
   glLightModelfv              = NULL;
   glLightModeli               = NULL;
   glLightModeliv              = NULL;
   glLightf                    = NULL;
   glLightfv                   = NULL;
   glLighti                    = NULL;
   glLightiv                   = NULL;
   glLineStipple               = NULL;
   glLineWidth                 = NULL;
   glListBase                  = NULL;
   glLoadIdentity              = NULL;
   glLoadMatrixd               = NULL;
   glLoadMatrixf               = NULL;
   glLoadName                  = NULL;
   glLogicOp                   = NULL;
   glMap1d                     = NULL;
   glMap1f                     = NULL;
   glMap2d                     = NULL;
   glMap2f                     = NULL;
   glMapGrid1d                 = NULL;
   glMapGrid1f                 = NULL;
   glMapGrid2d                 = NULL;
   glMapGrid2f                 = NULL;
   glMaterialf                 = NULL;
   glMaterialfv                = NULL;
   glMateriali                 = NULL;
   glMaterialiv                = NULL;
   glMatrixMode                = NULL;
   glMultMatrixd               = NULL;
   glMultMatrixf               = NULL;
   glNewList                   = NULL;
   glNormal3b                  = NULL;
   glNormal3bv                 = NULL;
   glNormal3d                  = NULL;
   glNormal3dv                 = NULL;
   glNormal3f                  = NULL;
   glNormal3fv                 = NULL;
   glNormal3i                  = NULL;
   glNormal3iv                 = NULL;
   glNormal3s                  = NULL;
   glNormal3sv                 = NULL;
   glNormalPointer             = NULL;
   glOrtho                     = NULL;
   glPassThrough               = NULL;
   glPixelMapfv                = NULL;
   glPixelMapuiv               = NULL;
   glPixelMapusv               = NULL;
   glPixelStoref               = NULL;
   glPixelStorei               = NULL;
   glPixelTransferf            = NULL;
   glPixelTransferi            = NULL;
   glPixelZoom                 = NULL;
   glPointSize                 = NULL;
   glPolygonMode               = NULL;
   glPolygonOffset             = NULL;
   glPolygonStipple            = NULL;
   glPopAttrib                 = NULL;
   glPopClientAttrib           = NULL;
   glPopMatrix                 = NULL;
   glPopName                   = NULL;
   glPrioritizeTextures        = NULL;
   glPushAttrib                = NULL;
   glPushClientAttrib          = NULL;
   glPushMatrix                = NULL;
   glPushName                  = NULL;
   glRasterPos2d               = NULL;
   glRasterPos2dv              = NULL;
   glRasterPos2f               = NULL;
   glRasterPos2fv              = NULL;
   glRasterPos2i               = NULL;
   glRasterPos2iv              = NULL;
   glRasterPos2s               = NULL;
   glRasterPos2sv              = NULL;
   glRasterPos3d               = NULL;
   glRasterPos3dv              = NULL;
   glRasterPos3f               = NULL;
   glRasterPos3fv              = NULL;
   glRasterPos3i               = NULL;
   glRasterPos3iv              = NULL;
   glRasterPos3s               = NULL;
   glRasterPos3sv              = NULL;
   glRasterPos4d               = NULL;
   glRasterPos4dv              = NULL;
   glRasterPos4f               = NULL;
   glRasterPos4fv              = NULL;
   glRasterPos4i               = NULL;
   glRasterPos4iv              = NULL;
   glRasterPos4s               = NULL;
   glRasterPos4sv              = NULL;
   glReadBuffer                = NULL;
   glReadPixels                = NULL;
   glRectd                     = NULL;
   glRectdv                    = NULL;
   glRectf                     = NULL;
   glRectfv                    = NULL;
   glRecti                     = NULL;
   glRectiv                    = NULL;
   glRects                     = NULL;
   glRectsv                    = NULL;
   glRenderMode                = NULL;
   glRotated                   = NULL;
   glRotatef                   = NULL;
   glScaled                    = NULL;
   glScalef                    = NULL;
   glScissor                   = NULL;
   glSelectBuffer              = NULL;
   glShadeModel                = NULL;
   glStencilFunc               = NULL;
   glStencilMask               = NULL;
   glStencilOp                 = NULL;
   glTexCoord1d                = NULL;
   glTexCoord1dv               = NULL;
   glTexCoord1f                = NULL;
   glTexCoord1fv               = NULL;
   glTexCoord1i                = NULL;
   glTexCoord1iv               = NULL;
   glTexCoord1s                = NULL;
   glTexCoord1sv               = NULL;
   glTexCoord2d                = NULL;
   glTexCoord2dv               = NULL;
   glTexCoord2f                = NULL;
   glTexCoord2fv               = NULL;
   glTexCoord2i                = NULL;
   glTexCoord2iv               = NULL;
   glTexCoord2s                = NULL;
   glTexCoord2sv               = NULL;
   glTexCoord3d                = NULL;
   glTexCoord3dv               = NULL;
   glTexCoord3f                = NULL;
   glTexCoord3fv               = NULL;
   glTexCoord3i                = NULL;
   glTexCoord3iv               = NULL;
   glTexCoord3s                = NULL;
   glTexCoord3sv               = NULL;
   glTexCoord4d                = NULL;
   glTexCoord4dv               = NULL;
   glTexCoord4f                = NULL;
   glTexCoord4fv               = NULL;
   glTexCoord4i                = NULL;
   glTexCoord4iv               = NULL;
   glTexCoord4s                = NULL;
   glTexCoord4sv               = NULL;
   glTexCoordPointer           = NULL;
   glTexEnvf                   = NULL;
   glTexEnvfv                  = NULL;
   glTexEnvi                   = NULL;
   glTexEnviv                  = NULL;
   glTexGend                   = NULL;
   glTexGendv                  = NULL;
   glTexGenf                   = NULL;
   glTexGenfv                  = NULL;
   glTexGeni                   = NULL;
   glTexGeniv                  = NULL;
   glTexImage1D                = NULL;
   glTexImage2D                = NULL;
   glTexParameterf             = NULL;
   glTexParameterfv            = NULL;
   glTexParameteri             = NULL;
   glTexParameteriv            = NULL;
   glTexSubImage1D             = NULL;
   glTexSubImage2D             = NULL;
   glTranslated                = NULL;
   glTranslatef                = NULL;
   glVertex2d                  = NULL;
   glVertex2dv                 = NULL;
   glVertex2f                  = NULL;
   glVertex2fv                 = NULL;
   glVertex2i                  = NULL;
   glVertex2iv                 = NULL;
   glVertex2s                  = NULL;
   glVertex2sv                 = NULL;
   glVertex3d                  = NULL;
   glVertex3dv                 = NULL;
   glVertex3f                  = NULL;
   glVertex3fv                 = NULL;
   glVertex3i                  = NULL;
   glVertex3iv                 = NULL;
   glVertex3s                  = NULL;
   glVertex3sv                 = NULL;
   glVertex4d                  = NULL;
   glVertex4dv                 = NULL;
   glVertex4f                  = NULL;
   glVertex4fv                 = NULL;
   glVertex4i                  = NULL;
   glVertex4iv                 = NULL;
   glVertex4s                  = NULL;
   glVertex4sv                 = NULL;
   glVertexPointer             = NULL;
   glViewport                  = NULL;

   // EXT_compiled_vertex_array
   glLockArraysEXT             = NULL;
   glUnlockArraysEXT           = NULL;

   // ARB_multitexture
   glActiveTextureARB          = NULL;
   glClientActiveTextureARB    = NULL;
   glMultiTexCoord2fARB        = NULL;
   glMultiTexCoord2fvARB       = NULL;

   // NV_vertex_array_range
   glVertexArrayRangeNV        = NULL;
   glFlushVertexArrayRangeNV   = NULL;
   wglAllocateMemoryNV         = NULL;
   wglFreeMemoryNV             = NULL;

   // EXT_fog_coord
   glFogCoordfEXT              = NULL;
   glFogCoordPointerEXT        = NULL;

   /* ARB_texture_compression */
   glCompressedTexImage3DARB    = NULL;
   glCompressedTexImage2DARB    = NULL;
   glCompressedTexImage1DARB    = NULL;
   glCompressedTexSubImage3DARB = NULL;
   glCompressedTexSubImage2DARB = NULL;
   glCompressedTexSubImage1DARB = NULL;
   glGetCompressedTexImageARB   = NULL;

	glAvailableVertexBufferEXT	= NULL;
	glAllocateVertexBufferEXT	= NULL;
	glLockVertexBufferEXT		= NULL;
	glUnlockVertexBufferEXT		= NULL;
	glSetVertexBufferEXT			= NULL;
	glOffsetVertexBufferEXT		= NULL;
	glFillVertexBufferEXT		= NULL;
	glFreeVertexBufferEXT		= NULL;

   qwglCopyContext             = NULL;
   qwglCreateContext           = NULL;
   qwglCreateLayerContext      = NULL;
   qwglDeleteContext           = NULL;
   qwglDescribeLayerPlane      = NULL;
   qwglGetCurrentContext       = NULL;
   qwglGetCurrentDC            = NULL;
   qwglGetLayerPaletteEntries  = NULL;
   qwglGetProcAddress          = NULL;
   qwglMakeCurrent             = NULL;
   qwglRealizeLayerPalette     = NULL;
   qwglSetLayerPaletteEntries  = NULL;
   qwglShareLists              = NULL;
   qwglSwapLayerBuffers        = NULL;
   qwglUseFontBitmaps          = NULL;
   qwglUseFontOutlines         = NULL;

   qwglChoosePixelFormat       = NULL;
   qwglDescribePixelFormat     = NULL;
   qwglGetPixelFormat          = NULL;
   qwglSetPixelFormat          = NULL;
   qwglSwapBuffers             = NULL;
}


#define GR_NUM_BOARDS 0x0f

static bool GlideIsValid( void )
{
//   int numBoards;
//   void (__stdcall *grGet)(unsigned int, unsigned int, int*);

    if ( LoadLibraryA("Glide3X") != 0 ) 
   {
      // FIXME: 3Dfx needs to fix this shit
      return true;

#if 0
        grGet = (void *)GetProcAddress( hGlide, "_grGet@12");

      if ( grGet )
      {
           grGet( GR_NUM_BOARDS, sizeof(int), &numBoards);
      }
      else
      {
         // if we've reached this point, something is seriously wrong
         ri.Printf( PRINT_WARNING, "WARNING: could not find grGet in GLIDE3X.DLL\n" );
         numBoards = 0;
      }

      FreeLibrary( hGlide );
      hGlide = NULL;

      if ( numBoards > 0 )
      {
         return true;
      }

      ri.Printf( PRINT_WARNING, "WARNING: invalid Glide installation!\n" );
#endif
    }

   return false;
} 

#   define GPA_GL( a ) GetProcAddress( winState.hinstOpenGL, a )
#   define GPA_GLU( a ) GetProcAddress( winState.hinstGLU, a )

/*
** QGL_Init
**
** This is responsible for binding our gl function pointers to 
** the appropriate GL stuff.  In Windows this means doing a 
** LoadLibrary and a bunch of calls to GetProcAddress.  On other
** operating systems we need to do the right thing, whatever that
** might be.
** 
*/

//--------------------------------------
bool QGL_Init( const char *dllname_gl, const char *dllname_glu )
{
   if ( winState.hinstOpenGL && winState.hinstGLU)
      return true;

   // Load OpenGL DLL
   if (!winState.hinstOpenGL)
   {
      // NOTE: this assumes that 'dllname' is lower case (and it should be)!
      if ( strstr( dllname_gl, "voodoo" ) )
      {
         if ( !GlideIsValid() )
         {
            return false;
         }
      }
      if ( ( winState.hinstOpenGL = LoadLibraryA( dllname_gl ) ) == 0 )
         return false;
   }

   // Load OpenGL GLU DLL
   if ( !winState.hinstGLU )
   {
      if ( ( winState.hinstGLU = LoadLibraryA( dllname_glu ) ) == 0 )
         return false;
   }

   // GLU Functions
   gluErrorString = dllgluErrorString = (gluErrorString_t) GPA_GLU( "gluErrorString" );
   gluGetString = dllgluGetString = (gluGetString_t) GPA_GLU( "gluGetString" );
   gluOrtho2D = dllgluOrtho2D = (gluOrtho2D_t) GPA_GLU( "gluOrtho2D" );
   gluPerspective = dllgluPerspective = (gluPerspective_t) GPA_GLU( "gluPerspective" );
   gluPickMatrix = dllgluPickMatrix = (gluPickMatrix_t) GPA_GLU( "gluPickMatrix" );
   gluLookAt = dllgluLookAt = (gluLookAt_t) GPA_GLU( "gluLookAt" );
   gluProject = dllgluProject = (gluProject_t) GPA_GLU( "gluProject" );
   gluUnProject = dllgluUnProject = (gluUnProject_t) GPA_GLU( "gluUnProject" );
   gluScaleImage = dllgluScaleImage = (gluScaleImage_t) GPA_GLU( "gluScaleImage" );
   gluBuild1DMipmaps = dllgluBuild1DMipmaps  = (gluBuild1DMipmaps_t) GPA_GLU( "gluBuild1DMipmaps" );
   gluBuild2DMipmaps = dllgluBuild2DMipmaps  = (gluBuild2DMipmaps_t) GPA_GLU( "gluBuild2DMipmaps" );

   // GL Functions
   glAccum = dllAccum = (glAccum_t) GPA_GL( "glAccum" );
   glAlphaFunc = dllAlphaFunc = (glAlphaFunc_t) GPA_GL( "glAlphaFunc" );
   glAreTexturesResident = dllAreTexturesResident = (glAreTexturesResident_t) GPA_GL( "glAreTexturesResident" );
   glArrayElement = dllArrayElement = (glArrayElement_t) GPA_GL( "glArrayElement" );
   glBegin = dllBegin = (glBegin_t) GPA_GL( "glBegin" );
   glBindTexture = dllBindTexture = (glBindTexture_t) GPA_GL( "glBindTexture" );
   glBitmap = dllBitmap = (glBitmap_t) GPA_GL( "glBitmap" );
   glBlendFunc = dllBlendFunc = (glBlendFunc_t) GPA_GL( "glBlendFunc" );
   glCallList = dllCallList = (glCallList_t) GPA_GL( "glCallList" );
   glCallLists = dllCallLists = (glCallLists_t) GPA_GL( "glCallLists" );
   glClear = dllClear = (glClear_t) GPA_GL( "glClear" );
   glClearAccum = dllClearAccum = (glClearAccum_t) GPA_GL( "glClearAccum" );
   glClearColor = dllClearColor = (glClearColor_t) GPA_GL( "glClearColor" );
   glClearDepth = dllClearDepth = (glClearDepth_t) GPA_GL( "glClearDepth" );
   glClearIndex = dllClearIndex = (glClearIndex_t) GPA_GL( "glClearIndex" );
   glClearStencil = dllClearStencil = (glClearStencil_t) GPA_GL( "glClearStencil" );
   glClipPlane = dllClipPlane = (glClipPlane_t) GPA_GL( "glClipPlane" );
   glColor3b = dllColor3b = (glColor3b_t) GPA_GL( "glColor3b" );
   glColor3bv = dllColor3bv = (glColor3bv_t) GPA_GL( "glColor3bv" );
   glColor3d = dllColor3d = (glColor3d_t) GPA_GL( "glColor3d" );
   glColor3dv = dllColor3dv = (glColor3dv_t) GPA_GL( "glColor3dv" );
   glColor3f = dllColor3f = (glColor3f_t) GPA_GL( "glColor3f" );
   glColor3fv = dllColor3fv = (glColor3fv_t) GPA_GL( "glColor3fv" );
   glColor3i = dllColor3i = (glColor3i_t) GPA_GL( "glColor3i" );
   glColor3iv = dllColor3iv = (glColor3iv_t) GPA_GL( "glColor3iv" );
   glColor3s = dllColor3s = (glColor3s_t) GPA_GL( "glColor3s" );
   glColor3sv = dllColor3sv = (glColor3sv_t) GPA_GL( "glColor3sv" );
   glColor3ub = dllColor3ub = (glColor3ub_t) GPA_GL( "glColor3ub" );
   glColor3ubv = dllColor3ubv = (glColor3ubv_t) GPA_GL( "glColor3ubv" );
   glColor3ui = dllColor3ui = (glColor3ui_t) GPA_GL( "glColor3ui" );
   glColor3uiv = dllColor3uiv = (glColor3uiv_t) GPA_GL( "glColor3uiv" );
   glColor3us = dllColor3us = (glColor3us_t) GPA_GL( "glColor3us" );
   glColor3usv = dllColor3usv = (glColor3usv_t) GPA_GL( "glColor3usv" );
   glColor4b = dllColor4b = (glColor4b_t) GPA_GL( "glColor4b" );
   glColor4bv = dllColor4bv = (glColor4bv_t) GPA_GL( "glColor4bv" );
   glColor4d = dllColor4d = (glColor4d_t) GPA_GL( "glColor4d" );
   glColor4dv = dllColor4dv = (glColor4dv_t) GPA_GL( "glColor4dv" );
   glColor4f = dllColor4f = (glColor4f_t) GPA_GL( "glColor4f" );
   glColor4fv = dllColor4fv = (glColor4fv_t) GPA_GL( "glColor4fv" );
   glColor4i = dllColor4i = (glColor4i_t) GPA_GL( "glColor4i" );
   glColor4iv = dllColor4iv = (glColor4iv_t) GPA_GL( "glColor4iv" );
   glColor4s = dllColor4s = (glColor4s_t) GPA_GL( "glColor4s" );
   glColor4sv = dllColor4sv = (glColor4sv_t) GPA_GL( "glColor4sv" );
   glColor4ub = dllColor4ub = (glColor4ub_t) GPA_GL( "glColor4ub" );
   glColor4ubv = dllColor4ubv = (glColor4ubv_t) GPA_GL( "glColor4ubv" );
   glColor4ui = dllColor4ui = (glColor4ui_t) GPA_GL( "glColor4ui" );
   glColor4uiv = dllColor4uiv = (glColor4uiv_t) GPA_GL( "glColor4uiv" );
   glColor4us = dllColor4us = (glColor4us_t) GPA_GL( "glColor4us" );
   glColor4usv = dllColor4usv = (glColor4usv_t) GPA_GL( "glColor4usv" );
   glColorMask = dllColorMask = (glColorMask_t) GPA_GL( "glColorMask" );
   glColorMaterial = dllColorMaterial = (glColorMaterial_t) GPA_GL( "glColorMaterial" );
   glColorPointer = dllColorPointer = (glColorPointer_t) GPA_GL( "glColorPointer" );
   glCopyPixels = dllCopyPixels = (glCopyPixels_t) GPA_GL( "glCopyPixels" );
   glCopyTexImage1D = dllCopyTexImage1D = (glCopyTexImage1D_t) GPA_GL( "glCopyTexImage1D" );
   glCopyTexImage2D = dllCopyTexImage2D = (glCopyTexImage2D_t) GPA_GL( "glCopyTexImage2D" );
   glCopyTexSubImage1D = dllCopyTexSubImage1D = (glCopyTexSubImage1D_t) GPA_GL( "glCopyTexSubImage1D" );
   glCopyTexSubImage2D = dllCopyTexSubImage2D = (glCopyTexSubImage2D_t) GPA_GL( "glCopyTexSubImage2D" );
   glCullFace = dllCullFace = (glCullFace_t) GPA_GL( "glCullFace" );
   glDeleteLists = dllDeleteLists = (glDeleteLists_t) GPA_GL( "glDeleteLists" );
   glDeleteTextures = dllDeleteTextures = (glDeleteTextures_t) GPA_GL( "glDeleteTextures" );
   glDepthFunc = dllDepthFunc = (glDepthFunc_t) GPA_GL( "glDepthFunc" );
   glDepthMask = dllDepthMask = (glDepthMask_t) GPA_GL( "glDepthMask" );
   glDepthRange = dllDepthRange = (glDepthRange_t) GPA_GL( "glDepthRange" );
   glDisable = dllDisable = (glDisable_t) GPA_GL( "glDisable" );
   glDisableClientState = dllDisableClientState = (glDisableClientState_t) GPA_GL( "glDisableClientState" );
   glDrawArrays = dllDrawArrays = (glDrawArrays_t) GPA_GL( "glDrawArrays" );
   glDrawBuffer = dllDrawBuffer = (glDrawBuffer_t) GPA_GL( "glDrawBuffer" );
   glDrawElements = dllDrawElements = (glDrawElements_t) GPA_GL( "glDrawElements" );
   glDrawPixels = dllDrawPixels = (glDrawPixels_t) GPA_GL( "glDrawPixels" );
   glEdgeFlag = dllEdgeFlag = (glEdgeFlag_t) GPA_GL( "glEdgeFlag" );
   glEdgeFlagPointer = dllEdgeFlagPointer = (glEdgeFlagPointer_t) GPA_GL( "glEdgeFlagPointer" );
   glEdgeFlagv = dllEdgeFlagv = (glEdgeFlagv_t) GPA_GL( "glEdgeFlagv" );
   glEnable = dllEnable = (glEnable_t) GPA_GL( "glEnable" );
   glEnableClientState = dllEnableClientState = (glEnableClientState_t) GPA_GL( "glEnableClientState" );
   glEnd = dllEnd = (glEnd_t) GPA_GL( "glEnd" );
   glEndList = dllEndList = (glEndList_t) GPA_GL( "glEndList" );
   glEvalCoord1d = dllEvalCoord1d    = (glEvalCoord1d_t) GPA_GL( "glEvalCoord1d" );
   glEvalCoord1dv = dllEvalCoord1dv = (glEvalCoord1dv_t) GPA_GL( "glEvalCoord1dv" );
   glEvalCoord1f = dllEvalCoord1f  = (glEvalCoord1f_t) GPA_GL( "glEvalCoord1f" );
   glEvalCoord1fv = dllEvalCoord1fv = (glEvalCoord1fv_t) GPA_GL( "glEvalCoord1fv" );
   glEvalCoord2d = dllEvalCoord2d  = (glEvalCoord2d_t) GPA_GL( "glEvalCoord2d" );
   glEvalCoord2dv = dllEvalCoord2dv = (glEvalCoord2dv_t) GPA_GL( "glEvalCoord2dv" );
   glEvalCoord2f = dllEvalCoord2f  = (glEvalCoord2f_t) GPA_GL( "glEvalCoord2f" );
   glEvalCoord2fv = dllEvalCoord2fv = (glEvalCoord2fv_t) GPA_GL( "glEvalCoord2fv" );
   glEvalMesh1 = dllEvalMesh1 = (glEvalMesh1_t) GPA_GL( "glEvalMesh1" );
   glEvalMesh2 = dllEvalMesh2 = (glEvalMesh2_t) GPA_GL( "glEvalMesh2" );
   glEvalPoint1 = dllEvalPoint1 = (glEvalPoint1_t) GPA_GL( "glEvalPoint1" );
   glEvalPoint2 = dllEvalPoint2 = (glEvalPoint2_t) GPA_GL( "glEvalPoint2" );
   glFeedbackBuffer = dllFeedbackBuffer = (glFeedbackBuffer_t) GPA_GL( "glFeedbackBuffer" );
   glFinish = dllFinish = (glFinish_t) GPA_GL( "glFinish" );
   glFlush = dllFlush = (glFlush_t) GPA_GL( "glFlush" );
   glFogf = dllFogf = (glFogf_t) GPA_GL( "glFogf" );
   glFogfv = dllFogfv = (glFogfv_t) GPA_GL( "glFogfv" );
   glFogi = dllFogi = (glFogi_t) GPA_GL( "glFogi" );
   glFogiv = dllFogiv = (glFogiv_t) GPA_GL( "glFogiv" );
   glFrontFace = dllFrontFace = (glFrontFace_t) GPA_GL( "glFrontFace" );
   glFrustum = dllFrustum = (glFrustum_t) GPA_GL( "glFrustum" );
   glGenLists = dllGenLists       = (glGenLists_t) GPA_GL( "glGenLists" );
   glGenTextures = dllGenTextures = (glGenTextures_t) GPA_GL( "glGenTextures" );
   glGetBooleanv = dllGetBooleanv = (glGetBooleanv_t) GPA_GL( "glGetBooleanv" );
   glGetClipPlane = dllGetClipPlane = (glGetClipPlane_t) GPA_GL( "glGetClipPlane" );
   glGetDoublev = dllGetDoublev = (glGetDoublev_t) GPA_GL( "glGetDoublev" );
   glGetError = dllGetError = (glGetError_t) GPA_GL( "glGetError" );
   glGetFloatv = dllGetFloatv = (glGetFloatv_t) GPA_GL( "glGetFloatv" );
   glGetIntegerv = dllGetIntegerv = (glGetIntegerv_t) GPA_GL( "glGetIntegerv" );
   glGetLightfv = dllGetLightfv = (glGetLightfv_t) GPA_GL( "glGetLightfv" );
   glGetLightiv = dllGetLightiv = (glGetLightiv_t) GPA_GL( "glGetLightiv" );
   glGetMapdv = dllGetMapdv = (glGetMapdv_t) GPA_GL( "glGetMapdv" );
   glGetMapfv = dllGetMapfv = (glGetMapfv_t) GPA_GL( "glGetMapfv" );
   glGetMapiv = dllGetMapiv = (glGetMapiv_t) GPA_GL( "glGetMapiv" );
   glGetMaterialfv  = dllGetMaterialfv = (glGetMaterialfv_t) GPA_GL( "glGetMaterialfv" );
   glGetMaterialiv  = dllGetMaterialiv = (glGetMaterialiv_t) GPA_GL( "glGetMaterialiv" );
   glGetPixelMapfv  = dllGetPixelMapfv = (glGetPixelMapfv_t) GPA_GL( "glGetPixelMapfv" );
   glGetPixelMapuiv = dllGetPixelMapuiv  = (glGetPixelMapuiv_t) GPA_GL( "glGetPixelMapuiv" );
   glGetPixelMapusv = dllGetPixelMapusv  = (glGetPixelMapusv_t) GPA_GL( "glGetPixelMapusv" );
   glGetPointerv = dllGetPointerv = (glGetPointerv_t) GPA_GL( "glGetPointerv" );
   glGetPolygonStipple = dllGetPolygonStipple = (glGetPolygonStipple_t) GPA_GL( "glGetPolygonStipple" );
   glGetString = dllGetString = (glGetString_t) GPA_GL( "glGetString" );
   glGetTexEnvfv = dllGetTexEnvfv = (glGetTexEnvfv_t) GPA_GL( "glGetTexEnvfv" );
   glGetTexEnviv = dllGetTexEnviv = (glGetTexEnviv_t) GPA_GL( "glGetTexEnviv" );
   glGetTexGendv = dllGetTexGendv = (glGetTexGendv_t) GPA_GL( "glGetTexGendv" );
   glGetTexGenfv = dllGetTexGenfv = (glGetTexGenfv_t) GPA_GL( "glGetTexGenfv" );
   glGetTexGeniv = dllGetTexGeniv = (glGetTexGeniv_t) GPA_GL( "glGetTexGeniv" );
   glGetTexImage = dllGetTexImage = (glGetTexImage_t) GPA_GL( "glGetTexImage" );
   glGetTexLevelParameterfv = dllGetTexLevelParameterfv = (glGetTexLevelParameterfv_t) GPA_GL( "glGetLevelParameterfv" );
   glGetTexLevelParameteriv = dllGetTexLevelParameteriv = (glGetTexLevelParameteriv_t) GPA_GL( "glGetLevelParameteriv" );
   glGetTexParameterfv = dllGetTexParameterfv = (glGetTexParameterfv_t) GPA_GL( "glGetTexParameterfv" );
   glGetTexParameteriv = dllGetTexParameteriv = (glGetTexParameteriv_t) GPA_GL( "glGetTexParameteriv" );
   glHint = dllHint = (glHint_t) GPA_GL( "glHint" );
   glIndexMask = dllIndexMask = (glIndexMask_t) GPA_GL( "glIndexMask" );
   glIndexPointer = dllIndexPointer = (glIndexPointer_t) GPA_GL( "glIndexPointer" );
   glIndexd = dllIndexd = (glIndexd_t) GPA_GL( "glIndexd" );
   glIndexdv = dllIndexdv = (glIndexdv_t) GPA_GL( "glIndexdv" );
   glIndexf = dllIndexf = (glIndexf_t) GPA_GL( "glIndexf" );
   glIndexfv = dllIndexfv = (glIndexfv_t) GPA_GL( "glIndexfv" );
   glIndexi = dllIndexi = (glIndexi_t) GPA_GL( "glIndexi" );
   glIndexiv = dllIndexiv = (glIndexiv_t) GPA_GL( "glIndexiv" );
   glIndexs = dllIndexs = (glIndexs_t) GPA_GL( "glIndexs" );
   glIndexsv = dllIndexsv = (glIndexsv_t) GPA_GL( "glIndexsv" );
   glIndexub = dllIndexub = (glIndexub_t) GPA_GL( "glIndexub" );
   glIndexubv = dllIndexubv = (glIndexubv_t) GPA_GL( "glIndexubv" );
   glInitNames = dllInitNames = (glInitNames_t) GPA_GL( "glInitNames" );
   glInterleavedArrays = dllInterleavedArrays = (glInterleavedArrays_t) GPA_GL( "glInterleavedArrays" );
   glIsEnabled = dllIsEnabled = (glIsEnabled_t) GPA_GL( "glIsEnabled" );
   glIsList = dllIsList = (glIsList_t) GPA_GL( "glIsList" );
   glIsTexture = dllIsTexture = (glIsTexture_t) GPA_GL( "glIsTexture" );
   glLightModelf = dllLightModelf = (glLightModelf_t) GPA_GL( "glLightModelf" );
   glLightModelfv = dllLightModelfv = (glLightModelfv_t) GPA_GL( "glLightModelfv" );
   glLightModeli = dllLightModeli = (glLightModeli_t) GPA_GL( "glLightModeli" );
   glLightModeliv = dllLightModeliv = (glLightModeliv_t) GPA_GL( "glLightModeliv" );
   glLightf = dllLightf = (glLightf_t) GPA_GL( "glLightf" );
   glLightfv = dllLightfv = (glLightfv_t) GPA_GL( "glLightfv" );
   glLighti = dllLighti = (glLighti_t) GPA_GL( "glLighti" );
   glLightiv = dllLightiv = (glLightiv_t) GPA_GL( "glLightiv" );
   glLineStipple  = dllLineStipple = (glLineStipple_t) GPA_GL( "glLineStipple" );
   glLineWidth = dllLineWidth = (glLineWidth_t) GPA_GL( "glLineWidth" );
   glListBase = dllListBase = (glListBase_t) GPA_GL( "glListBase" );
   glLoadIdentity = dllLoadIdentity = (glLoadIdentity_t) GPA_GL( "glLoadIdentity" );
   glLoadMatrixd  = dllLoadMatrixd = (glLoadMatrixd_t) GPA_GL( "glLoadMatrixd" );
   glLoadMatrixf  = dllLoadMatrixf = (glLoadMatrixf_t) GPA_GL( "glLoadMatrixf" );
   glLoadName = dllLoadName = (glLoadName_t) GPA_GL( "glLoadName" );
   glLogicOp = dllLogicOp = (glLogicOp_t) GPA_GL( "glLogicOp" );
   glMap1d = dllMap1d = (glMap1d_t) GPA_GL( "glMap1d" );
   glMap1f = dllMap1f = (glMap1f_t) GPA_GL( "glMap1f" );
   glMap2d = dllMap2d = (glMap2d_t) GPA_GL( "glMap2d" );
   glMap2f = dllMap2f = (glMap2f_t) GPA_GL( "glMap2f" );
   glMapGrid1d = dllMapGrid1d = (glMapGrid1d_t) GPA_GL( "glMapGrid1d" );
   glMapGrid1f = dllMapGrid1f = (glMapGrid1f_t) GPA_GL( "glMapGrid1f" );
   glMapGrid2d = dllMapGrid2d = (glMapGrid2d_t) GPA_GL( "glMapGrid2d" );
   glMapGrid2f = dllMapGrid2f = (glMapGrid2f_t) GPA_GL( "glMapGrid2f" );
   glMaterialf = dllMaterialf = (glMaterialf_t) GPA_GL( "glMaterialf" );
   glMaterialfv = dllMaterialfv = (glMaterialfv_t) GPA_GL( "glMaterialfv" );
   glMateriali = dllMateriali = (glMateriali_t) GPA_GL( "glMateriali" );
   glMaterialiv = dllMaterialiv = (glMaterialiv_t) GPA_GL( "glMaterialiv" );
   glMatrixMode = dllMatrixMode = (glMatrixMode_t) GPA_GL( "glMatrixMode" );
   glMultMatrixd  = dllMultMatrixd = (glMultMatrixd_t) GPA_GL( "glMultMatrixd" );
   glMultMatrixf  = dllMultMatrixf = (glMultMatrixf_t) GPA_GL( "glMultMatrixf" );
   glNewList = dllNewList = (glNewList_t) GPA_GL( "glNewList" );
   glNormal3b = dllNormal3b = (glNormal3b_t) GPA_GL( "glNormal3b" );
   glNormal3bv = dllNormal3bv = (glNormal3bv_t) GPA_GL( "glNormal3bv" );
   glNormal3d = dllNormal3d = (glNormal3d_t) GPA_GL( "glNormal3d" );
   glNormal3dv = dllNormal3dv = (glNormal3dv_t) GPA_GL( "glNormal3dv" );
   glNormal3f = dllNormal3f = (glNormal3f_t) GPA_GL( "glNormal3f" );
   glNormal3fv = dllNormal3fv = (glNormal3fv_t) GPA_GL( "glNormal3fv" );
   glNormal3i = dllNormal3i = (glNormal3i_t) GPA_GL( "glNormal3i" );
   glNormal3iv = dllNormal3iv = (glNormal3iv_t) GPA_GL( "glNormal3iv" );
   glNormal3s = dllNormal3s = (glNormal3s_t) GPA_GL( "glNormal3s" );
   glNormal3sv = dllNormal3sv = (glNormal3sv_t) GPA_GL( "glNormal3sv" );
   glNormalPointer = dllNormalPointer = (glNormalPointer_t) GPA_GL( "glNormalPointer" );
   glOrtho = dllOrtho = (glOrtho_t) GPA_GL( "glOrtho" );
   glPassThrough = dllPassThrough = (glPassThrough_t) GPA_GL( "glPassThrough" );
   glPixelMapfv = dllPixelMapfv = (glPixelMapfv_t) GPA_GL( "glPixelMapfv" );
   glPixelMapuiv = dllPixelMapuiv = (glPixelMapuiv_t) GPA_GL( "glPixelMapuiv" );
   glPixelMapusv = dllPixelMapusv = (glPixelMapusv_t) GPA_GL( "glPixelMapusv" );
   glPixelStoref = dllPixelStoref = (glPixelStoref_t) GPA_GL( "glPixelStoref" );
   glPixelStorei = dllPixelStorei = (glPixelStorei_t) GPA_GL( "glPixelStorei" );
   glPixelTransferf = dllPixelTransferf = (glPixelTransferf_t) GPA_GL( "glPixelTransferf" );
   glPixelTransferi = dllPixelTransferi = (glPixelTransferi_t) GPA_GL( "glPixelTransferi" );
   glPixelZoom = dllPixelZoom = (glPixelZoom_t) GPA_GL( "glPixelZoom" );
   glPointSize = dllPointSize = (glPointSize_t) GPA_GL( "glPointSize" );
   glPolygonMode = dllPolygonMode = (glPolygonMode_t) GPA_GL( "glPolygonMode" );
   glPolygonOffset = dllPolygonOffset = (glPolygonOffset_t) GPA_GL( "glPolygonOffset" );
   glPolygonStipple = dllPolygonStipple = (glPolygonStipple_t) GPA_GL( "glPolygonStipple" );
   glPopAttrib = dllPopAttrib = (glPopAttrib_t) GPA_GL( "glPopAttrib" );
   glPopClientAttrib = dllPopClientAttrib = (glPopClientAttrib_t) GPA_GL( "glPopClientAttrib" );
   glPopMatrix = dllPopMatrix = (glPopMatrix_t) GPA_GL( "glPopMatrix" );
   glPopName = dllPopName = (glPopName_t) GPA_GL( "glPopName" );
   glPrioritizeTextures = dllPrioritizeTextures = (glPrioritizeTextures_t) GPA_GL( "glPrioritizeTextures" );
   glPushAttrib = dllPushAttrib = (glPushAttrib_t) GPA_GL( "glPushAttrib" );
   glPushClientAttrib = dllPushClientAttrib = (glPushClientAttrib_t) GPA_GL( "glPushClientAttrib" );
   glPushMatrix = dllPushMatrix = (glPushMatrix_t) GPA_GL( "glPushMatrix" );
   glPushName = dllPushName = (glPushName_t) GPA_GL( "glPushName" );
   glRasterPos2d = dllRasterPos2d = (glRasterPos2d_t) GPA_GL( "glRasterPos2d" );
   glRasterPos2dv = dllRasterPos2dv  = (glRasterPos2dv_t) GPA_GL( "glRasterPos2dv" );
   glRasterPos2f = dllRasterPos2f = (glRasterPos2f_t) GPA_GL( "glRasterPos2f" );
   glRasterPos2fv = dllRasterPos2fv  = (glRasterPos2fv_t) GPA_GL( "glRasterPos2fv" );
   glRasterPos2i = dllRasterPos2i = (glRasterPos2i_t) GPA_GL( "glRasterPos2i" );
   glRasterPos2iv = dllRasterPos2iv  = (glRasterPos2iv_t) GPA_GL( "glRasterPos2iv" );
   glRasterPos2s = dllRasterPos2s = (glRasterPos2s_t) GPA_GL( "glRasterPos2s" );
   glRasterPos2sv = dllRasterPos2sv  = (glRasterPos2sv_t) GPA_GL( "glRasterPos2sv" );
   glRasterPos3d = dllRasterPos3d = (glRasterPos3d_t) GPA_GL( "glRasterPos3d" );
   glRasterPos3dv = dllRasterPos3dv  = (glRasterPos3dv_t) GPA_GL( "glRasterPos3dv" );
   glRasterPos3f = dllRasterPos3f = (glRasterPos3f_t) GPA_GL( "glRasterPos3f" );
   glRasterPos3fv = dllRasterPos3fv  = (glRasterPos3fv_t) GPA_GL( "glRasterPos3fv" );
   glRasterPos3i = dllRasterPos3i = (glRasterPos3i_t) GPA_GL( "glRasterPos3i" );
   glRasterPos3iv = dllRasterPos3iv  = (glRasterPos3iv_t) GPA_GL( "glRasterPos3iv" );
   glRasterPos3s = dllRasterPos3s = (glRasterPos3s_t) GPA_GL( "glRasterPos3s" );
   glRasterPos3sv = dllRasterPos3sv  = (glRasterPos3sv_t) GPA_GL( "glRasterPos3sv" );
   glRasterPos4d = dllRasterPos4d = (glRasterPos4d_t) GPA_GL( "glRasterPos4d" );
   glRasterPos4dv = dllRasterPos4dv  = (glRasterPos4dv_t) GPA_GL( "glRasterPos4dv" );
   glRasterPos4f = dllRasterPos4f = (glRasterPos4f_t) GPA_GL( "glRasterPos4f" );
   glRasterPos4fv = dllRasterPos4fv  = (glRasterPos4fv_t) GPA_GL( "glRasterPos4fv" );
   glRasterPos4i = dllRasterPos4i = (glRasterPos4i_t) GPA_GL( "glRasterPos4i" );
   glRasterPos4iv = dllRasterPos4iv  = (glRasterPos4iv_t) GPA_GL( "glRasterPos4iv" );
   glRasterPos4s = dllRasterPos4s = (glRasterPos4s_t) GPA_GL( "glRasterPos4s" );
   glRasterPos4sv = dllRasterPos4sv  = (glRasterPos4sv_t) GPA_GL( "glRasterPos4sv" );
   glReadBuffer = dllReadBuffer = (glReadBuffer_t) GPA_GL( "glReadBuffer" );
   glReadPixels = dllReadPixels = (glReadPixels_t) GPA_GL( "glReadPixels" );
   glRectd = dllRectd = (glRectd_t) GPA_GL( "glRectd" );
   glRectdv = dllRectdv = (glRectdv_t) GPA_GL( "glRectdv" );
   glRectf = dllRectf = (glRectf_t) GPA_GL( "glRectf" );
   glRectfv = dllRectfv = (glRectfv_t) GPA_GL( "glRectfv" );
   glRecti = dllRecti = (glRecti_t) GPA_GL( "glRecti" );
   glRectiv = dllRectiv = (glRectiv_t) GPA_GL( "glRectiv" );
   glRects = dllRects = (glRects_t) GPA_GL( "glRects" );
   glRectsv = dllRectsv = (glRectsv_t) GPA_GL( "glRectsv" );
   glRenderMode = dllRenderMode = (glRenderMode_t) GPA_GL( "glRenderMode" );
   glRotated = dllRotated = (glRotated_t) GPA_GL( "glRotated" );
   glRotatef = dllRotatef = (glRotatef_t) GPA_GL( "glRotatef" );
   glScaled = dllScaled = (glScaled_t) GPA_GL( "glScaled" );
   glScalef = dllScalef = (glScalef_t) GPA_GL( "glScalef" );
   glScissor = dllScissor = (glScissor_t) GPA_GL( "glScissor" );
   glSelectBuffer = dllSelectBuffer  = (glSelectBuffer_t) GPA_GL( "glSelectBuffer" );
   glShadeModel = dllShadeModel = (glShadeModel_t) GPA_GL( "glShadeModel" );
   glStencilFunc = dllStencilFunc = (glStencilFunc_t) GPA_GL( "glStencilFunc" );
   glStencilMask = dllStencilMask = (glStencilMask_t) GPA_GL( "glStencilMask" );
   glStencilOp = dllStencilOp = (glStencilOp_t) GPA_GL( "glStencilOp" );
   glTexCoord1d = dllTexCoord1d = (glTexCoord1d_t) GPA_GL( "glTexCoord1d" );
   glTexCoord1dv = dllTexCoord1dv = (glTexCoord1dv_t) GPA_GL( "glTexCoord1dv" );
   glTexCoord1f = dllTexCoord1f = (glTexCoord1f_t) GPA_GL( "glTexCoord1f" );
   glTexCoord1fv = dllTexCoord1fv = (glTexCoord1fv_t) GPA_GL( "glTexCoord1fv" );
   glTexCoord1i = dllTexCoord1i = (glTexCoord1i_t) GPA_GL( "glTexCoord1i" );
   glTexCoord1iv = dllTexCoord1iv = (glTexCoord1iv_t) GPA_GL( "glTexCoord1iv" );
   glTexCoord1s = dllTexCoord1s = (glTexCoord1s_t) GPA_GL( "glTexCoord1s" );
   glTexCoord1sv = dllTexCoord1sv = (glTexCoord1sv_t) GPA_GL( "glTexCoord1sv" );
   glTexCoord2d = dllTexCoord2d = (glTexCoord2d_t) GPA_GL( "glTexCoord2d" );
   glTexCoord2dv = dllTexCoord2dv = (glTexCoord2dv_t) GPA_GL( "glTexCoord2dv" );
   glTexCoord2f = dllTexCoord2f = (glTexCoord2f_t) GPA_GL( "glTexCoord2f" );
   glTexCoord2fv = dllTexCoord2fv = (glTexCoord2fv_t) GPA_GL( "glTexCoord2fv" );
   glTexCoord2i = dllTexCoord2i = (glTexCoord2i_t) GPA_GL( "glTexCoord2i" );
   glTexCoord2iv = dllTexCoord2iv = (glTexCoord2iv_t) GPA_GL( "glTexCoord2iv" );
   glTexCoord2s = dllTexCoord2s = (glTexCoord2s_t) GPA_GL( "glTexCoord2s" );
   glTexCoord2sv = dllTexCoord2sv = (glTexCoord2sv_t) GPA_GL( "glTexCoord2sv" );
   glTexCoord3d = dllTexCoord3d = (glTexCoord3d_t) GPA_GL( "glTexCoord3d" );
   glTexCoord3dv = dllTexCoord3dv = (glTexCoord3dv_t) GPA_GL( "glTexCoord3dv" );
   glTexCoord3f = dllTexCoord3f = (glTexCoord3f_t) GPA_GL( "glTexCoord3f" );
   glTexCoord3fv = dllTexCoord3fv = (glTexCoord3fv_t) GPA_GL( "glTexCoord3fv" );
   glTexCoord3i = dllTexCoord3i = (glTexCoord3i_t) GPA_GL( "glTexCoord3i" );
   glTexCoord3iv = dllTexCoord3iv = (glTexCoord3iv_t) GPA_GL( "glTexCoord3iv" );
   glTexCoord3s = dllTexCoord3s = (glTexCoord3s_t) GPA_GL( "glTexCoord3s" );
   glTexCoord3sv = dllTexCoord3sv = (glTexCoord3sv_t) GPA_GL( "glTexCoord3sv" );
   glTexCoord4d = dllTexCoord4d = (glTexCoord4d_t) GPA_GL( "glTexCoord4d" );
   glTexCoord4dv = dllTexCoord4dv = (glTexCoord4dv_t) GPA_GL( "glTexCoord4dv" );
   glTexCoord4f = dllTexCoord4f = (glTexCoord4f_t) GPA_GL( "glTexCoord4f" );
   glTexCoord4fv = dllTexCoord4fv = (glTexCoord4fv_t) GPA_GL( "glTexCoord4fv" );
   glTexCoord4i = dllTexCoord4i = (glTexCoord4i_t) GPA_GL( "glTexCoord4i" );
   glTexCoord4iv = dllTexCoord4iv = (glTexCoord4iv_t) GPA_GL( "glTexCoord4iv" );
   glTexCoord4s = dllTexCoord4s = (glTexCoord4s_t) GPA_GL( "glTexCoord4s" );
   glTexCoord4sv = dllTexCoord4sv = (glTexCoord4sv_t) GPA_GL( "glTexCoord4sv" );
   glTexCoordPointer = dllTexCoordPointer = (glTexCoordPointer_t) GPA_GL( "glTexCoordPointer" );
   glTexEnvf = dllTexEnvf = (glTexEnvf_t) GPA_GL( "glTexEnvf" );
   glTexEnvfv = dllTexEnvfv  = (glTexEnvfv_t) GPA_GL( "glTexEnvfv" );
   glTexEnvi = dllTexEnvi = (glTexEnvi_t) GPA_GL( "glTexEnvi" );
   glTexEnviv = dllTexEnviv  = (glTexEnviv_t) GPA_GL( "glTexEnviv" );
   glTexGend = dllTexGend = (glTexGend_t) GPA_GL( "glTexGend" );
   glTexGendv = dllTexGendv  = (glTexGendv_t) GPA_GL( "glTexGendv" );
   glTexGenf = dllTexGenf = (glTexGenf_t) GPA_GL( "glTexGenf" );
   glTexGenfv = dllTexGenfv  = (glTexGenfv_t) GPA_GL( "glTexGenfv" );
   glTexGeni = dllTexGeni = (glTexGeni_t) GPA_GL( "glTexGeni" );
   glTexGeniv = dllTexGeniv  = (glTexGeniv_t) GPA_GL( "glTexGeniv" );
   glTexImage1D = dllTexImage1D = (glTexImage1D_t) GPA_GL( "glTexImage1D" );
   glTexImage2D = dllTexImage2D = (glTexImage2D_t) GPA_GL( "glTexImage2D" );
   glTexParameterf  = dllTexParameterf  = (glTexParameterf_t) GPA_GL( "glTexParameterf" );
   glTexParameterfv = dllTexParameterfv = (glTexParameterfv_t) GPA_GL( "glTexParameterfv" );
   glTexParameteri  = dllTexParameteri  = (glTexParameteri_t) GPA_GL( "glTexParameteri" );
   glTexParameteriv = dllTexParameteriv = (glTexParameteriv_t) GPA_GL( "glTexParameteriv" );
   glTexSubImage1D  = dllTexSubImage1D  = (glTexSubImage1D_t) GPA_GL( "glTexSubImage1D" );
   glTexSubImage2D  = dllTexSubImage2D  = (glTexSubImage2D_t) GPA_GL( "glTexSubImage2D" );
   glTranslated = dllTranslated = (glTranslated_t) GPA_GL( "glTranslated" );
   glTranslatef = dllTranslatef = (glTranslatef_t) GPA_GL( "glTranslatef" );
   glVertex2d = dllVertex2d = (glVertex2d_t) GPA_GL( "glVertex2d" );
   glVertex2dv = dllVertex2dv = (glVertex2dv_t) GPA_GL( "glVertex2dv" );
   glVertex2f = dllVertex2f = (glVertex2f_t) GPA_GL( "glVertex2f" );
   glVertex2fv = dllVertex2fv = (glVertex2fv_t) GPA_GL( "glVertex2fv" );
   glVertex2i = dllVertex2i = (glVertex2i_t) GPA_GL( "glVertex2i" );
   glVertex2iv = dllVertex2iv = (glVertex2iv_t) GPA_GL( "glVertex2iv" );

   glVertex2s = dllVertex2s = (glVertex2s_t) GPA_GL( "glVertex2s" );
   glVertex2sv = dllVertex2sv = (glVertex2sv_t) GPA_GL( "glVertex2sv" );
   glVertex3d = dllVertex3d = (glVertex3d_t) GPA_GL( "glVertex3d" );
   glVertex3dv = dllVertex3dv = (glVertex3dv_t) GPA_GL( "glVertex3dv" );
   glVertex3f = dllVertex3f = (glVertex3f_t) GPA_GL( "glVertex3f" );
   glVertex3fv = dllVertex3fv = (glVertex3fv_t) GPA_GL( "glVertex3fv" );
   glVertex3i = dllVertex3i = (glVertex3i_t) GPA_GL( "glVertex3i" );
   glVertex3iv = dllVertex3iv = (glVertex3iv_t) GPA_GL( "glVertex3iv" );
   glVertex3s = dllVertex3s = (glVertex3s_t) GPA_GL( "glVertex3s" );
   glVertex3sv = dllVertex3sv = (glVertex3sv_t) GPA_GL( "glVertex3sv" );
   glVertex4d = dllVertex4d = (glVertex4d_t) GPA_GL( "glVertex4d" );
   glVertex4dv = dllVertex4dv = (glVertex4dv_t) GPA_GL( "glVertex4dv" );
   glVertex4f = dllVertex4f = (glVertex4f_t) GPA_GL( "glVertex4f" );
   glVertex4fv = dllVertex4fv = (glVertex4fv_t) GPA_GL( "glVertex4fv" );
   glVertex4i = dllVertex4i = (glVertex4i_t) GPA_GL( "glVertex4i" );
   glVertex4iv = dllVertex4iv = (glVertex4iv_t) GPA_GL( "glVertex4iv" );
   glVertex4s = dllVertex4s = (glVertex4s_t) GPA_GL( "glVertex4s" );
   glVertex4sv = dllVertex4sv = (glVertex4sv_t) GPA_GL( "glVertex4sv" );
   glVertexPointer = dllVertexPointer  = (glVertexPointer_t) GPA_GL( "glVertexPointer" );
   glViewport = dllViewport = (glViewport_t) GPA_GL( "glViewport" );
   qwglChoosePixelFormat = (qwglChoosePixelFormat_t) GPA_GL( "wglChoosePixelFormat" );
   qwglDescribePixelFormat = (qwglDescribePixelFormat_t) GPA_GL( "wglDescribePixelFormat" );
   qwglGetPixelFormat = (qwglGetPixelFormat_t) GPA_GL( "wglGetPixelFormat" );
   qwglSetPixelFormat = (qwglSetPixelFormat_t) GPA_GL( "wglSetPixelFormat" );
   dllSwapBuffers = qwglSwapBuffers = (qwglSwapBuffers_t) GPA_GL( "wglSwapBuffers" );

	if (dStrstr(dllname_gl,"d3d") != NULL)
	{
		qwglUseFontBitmaps = (qwglUseFontBitmaps_t) GPA_GL( "wd3dUseFontBitmapsA" );
   	qwglUseFontOutlines = (qwglUseFontOutlines_t) GPA_GL( "wd3dUseFontOutlinesA" );
		qwglCreateContext = (qwglCreateContext_t) GPA_GL( "wd3dCreateContext" );
		qwglDeleteContext = (qwglDeleteContext_t) GPA_GL( "wd3dDeleteContext" );
		qwglGetCurrentContext = (qwglGetCurrentContext_t) GPA_GL( "wd3dGetCurrentContext" );
		qwglGetCurrentDC = (qwglGetCurrentDC_t) GPA_GL( "wd3dGetCurrentDC" );
		qwglGetProcAddress = (qwglGetProcAddress_t) GPA_GL( "wd3dGetProcAddress" );
		qwglMakeCurrent = (qwglMakeCurrent_t) GPA_GL( "wd3dMakeCurrent" );
		qwglCopyContext = (qwglCopyContext_t) GPA_GL( "wd3dCopyContext" );
		qwglCreateLayerContext = (qwglCreateLayerContext_t) GPA_GL( "wd3dCreateLayerContext" );
		qwglDescribeLayerPlane = (qwglDescribeLayerPlane_t) GPA_GL( "wd3dDescribeLayerPlane" );
		qwglGetLayerPaletteEntries = (qwglGetLayerPaletteEntries_t) GPA_GL( "wd3dGetLayerPaletteEntries" );
		qwglRealizeLayerPalette = (qwglRealizeLayerPalette_t) GPA_GL( "wd3dRealizeLayerPalette" );
		qwglSetLayerPaletteEntries = (qwglSetLayerPaletteEntries_t) GPA_GL( "wd3dSetLayerPaletteEntries" );
		qwglShareLists = (qwglShareLists_t) GPA_GL( "wd3dShareLists" );
		qwglSwapLayerBuffers = (qwglSwapLayerBuffers_t) GPA_GL( "wd3dSwapLayerBuffers" );
	}
	else
	{
		qwglUseFontBitmaps = (qwglUseFontBitmaps_t) GPA_GL( "wglUseFontBitmapsA" );
   	qwglUseFontOutlines = (qwglUseFontOutlines_t) GPA_GL( "wglUseFontOutlinesA" );
		qwglCreateContext = (qwglCreateContext_t) GPA_GL( "wglCreateContext" );
		qwglDeleteContext = (qwglDeleteContext_t) GPA_GL( "wglDeleteContext" );
		qwglGetCurrentContext = (qwglGetCurrentContext_t) GPA_GL( "wglGetCurrentContext" );
		qwglGetCurrentDC = (qwglGetCurrentDC_t) GPA_GL( "wglGetCurrentDC" );
		qwglGetProcAddress = (qwglGetProcAddress_t) GPA_GL( "wglGetProcAddress" );
		qwglMakeCurrent = (qwglMakeCurrent_t) GPA_GL( "wglMakeCurrent" );
		qwglCopyContext = (qwglCopyContext_t) GPA_GL( "wglCopyContext" );
		qwglCreateLayerContext = (qwglCreateLayerContext_t) GPA_GL( "wglCreateLayerContext" );
		qwglDescribeLayerPlane = (qwglDescribeLayerPlane_t) GPA_GL( "wglDescribeLayerPlane" );
		qwglGetLayerPaletteEntries = (qwglGetLayerPaletteEntries_t) GPA_GL( "wglGetLayerPaletteEntries" );
		qwglRealizeLayerPalette = (qwglRealizeLayerPalette_t) GPA_GL( "wglRealizeLayerPalette" );
		qwglSetLayerPaletteEntries = (qwglSetLayerPaletteEntries_t) GPA_GL( "wglSetLayerPaletteEntries" );
   	qwglShareLists = (qwglShareLists_t) GPA_GL( "wglShareLists" );
		qwglSwapLayerBuffers = (qwglSwapLayerBuffers_t) GPA_GL( "wglSwapLayerBuffers" );
	}

   qwglSwapIntervalEXT = 0;
   
   return true;
}


bool QGL_EXT_Init( )
{
   // Load extensions...
   //
   const char* pExtString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
   gGLState.primMode = 0;

// extern void ( GLAPIENTRY* glColorTableEXT)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void* data);
   // EXT_paletted_texture
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_paletted_texture") != NULL)
   {
      glColorTableEXT = dllColorTableEXT = (glColorTable_t) qwglGetProcAddress("glColorTableEXT");
      gGLState.suppPalettedTexture = true;
   }
   else
   {
      gGLState.suppPalettedTexture = false;
   }
   
   // EXT_compiled_vertex_array
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_compiled_vertex_array") != NULL)
   {
      glLockArraysEXT   = dllLockArraysEXT   = (glLockArrays_t)   qwglGetProcAddress("glLockArraysEXT");
      glUnlockArraysEXT = dllUnlockArraysEXT = (glUnlockArrays_t) qwglGetProcAddress("glUnlockArraysEXT");
      gGLState.suppLockedArrays = true;
   }
   else
   {
      glLockArraysEXT   = dllLockArraysEXT   = NULL;
      glUnlockArraysEXT = dllUnlockArraysEXT = NULL;
      gGLState.suppLockedArrays = false;
   }

   // ARB_multitexture
   if (pExtString && dStrstr(pExtString, (const char*)"GL_ARB_multitexture") != NULL) {
      glActiveTextureARB       = dllActiveTextureARB       = (glActiveTextureARB_t)       qwglGetProcAddress("glActiveTextureARB");
      glClientActiveTextureARB = dllClientActiveTextureARB = (glClientActiveTextureARB_t) qwglGetProcAddress("glClientActiveTextureARB");
      glMultiTexCoord2fARB     = dllMultiTexCoord2fARB     = (glMultiTexCoord2fARB_t)     qwglGetProcAddress("glMultiTexCoord2fARB");
      glMultiTexCoord2fvARB    = dllMultiTexCoord2fvARB    = (glMultiTexCoord2fvARB_t)    qwglGetProcAddress("glMultiTexCoord2fvARB");
      gGLState.suppARBMultitexture = true;
   } else {
      glActiveTextureARB       = dllActiveTextureARB       = NULL;
      glClientActiveTextureARB = dllClientActiveTextureARB = NULL;
      glMultiTexCoord2fARB     = dllMultiTexCoord2fARB     = NULL;
      glMultiTexCoord2fvARB    = dllMultiTexCoord2fvARB    = NULL;
      gGLState.suppARBMultitexture = false;
   }

   // NV_vertex_array_range
   if (pExtString && dStrstr(pExtString, (const char*)"GL_NV_vertex_array_range") != NULL) {
      glVertexArrayRangeNV      = dllVertexArrayRangeNV      = (glVertexArrayRange_t)      qwglGetProcAddress("glVertexArrayRangeNV");
      glFlushVertexArrayRangeNV = dllFlushVertexArrayRangeNV = (glFlushVertexArrayRange_t) qwglGetProcAddress("glFlushVertexArrayRangeNV");
      wglAllocateMemoryNV       = dllAllocateMemoryNV        = (wglAllocateMemory_t)       qwglGetProcAddress("wglAllocateMemoryNV");
      wglFreeMemoryNV           = dllFreeMemoryNV            = (wglFreeMemory_t)           qwglGetProcAddress("wglFreeMemoryNV");

      gGLState.suppVertexArrayRange = true;
   } else {
      glVertexArrayRangeNV      = dllVertexArrayRangeNV      = NULL;
      glFlushVertexArrayRangeNV = dllFlushVertexArrayRangeNV = NULL;
      wglAllocateMemoryNV       = dllAllocateMemoryNV        = NULL;
      wglFreeMemoryNV           = dllFreeMemoryNV            = NULL;
      gGLState.suppVertexArrayRange = false;
   }

   // EXT_fog_coord
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_fog_coord") != NULL) {
      glFogCoordfEXT       = dllFogCoordfEXT       = (glFogCoordf_t)       qwglGetProcAddress("glFogCoordfEXT");
      glFogCoordPointerEXT = dllFogCoordPointerEXT = (glFogCoordPointer_t) qwglGetProcAddress("glFogCoordPointerEXT");
      gGLState.suppFogCoord = true;
   } else {
      glFogCoordfEXT       = dllFogCoordfEXT       = NULL;
      glFogCoordPointerEXT = dllFogCoordPointerEXT = NULL;
      gGLState.suppFogCoord = false;
   }

   // ARB_texture_compression
   if (pExtString && dStrstr(pExtString, (const char*)"GL_ARB_texture_compression") != NULL) {
      glCompressedTexImage3DARB    = dllCompressedTexImage3DARB    = (glCompressedTexImage3DARB_t)    qwglGetProcAddress("glCompressedTexImage3DARB");
      glCompressedTexImage2DARB    = dllCompressedTexImage2DARB    = (glCompressedTexImage2DARB_t)    qwglGetProcAddress("glCompressedTexImage2DARB");
      glCompressedTexImage1DARB    = dllCompressedTexImage1DARB    = (glCompressedTexImage1DARB_t)    qwglGetProcAddress("glCompressedTexImage1DARB");
      glCompressedTexSubImage3DARB = dllCompressedTexSubImage3DARB = (glCompressedTexSubImage3DARB_t) qwglGetProcAddress("glCompressedTexSubImage3DARB");
      glCompressedTexSubImage2DARB = dllCompressedTexSubImage2DARB = (glCompressedTexSubImage2DARB_t) qwglGetProcAddress("glCompressedTexSubImage2DARB");
      glCompressedTexSubImage1DARB = dllCompressedTexSubImage1DARB = (glCompressedTexSubImage1DARB_t) qwglGetProcAddress("glCompressedTexSubImage1DARB");
      glGetCompressedTexImageARB   = dllGetCompressedTexImageARB   = (glGetCompressedTexImageARB_t)   qwglGetProcAddress("glGetCompressedTexImageARB");

      gGLState.suppTextureCompression = true;
   } else {
      glCompressedTexImage3DARB    = dllCompressedTexImage3DARB    = NULL;
      glCompressedTexImage2DARB    = dllCompressedTexImage2DARB    = NULL;
      glCompressedTexImage1DARB    = dllCompressedTexImage1DARB    = NULL;
      glCompressedTexSubImage3DARB = dllCompressedTexSubImage3DARB = NULL;
      glCompressedTexSubImage2DARB = dllCompressedTexSubImage2DARB = NULL;
      glCompressedTexSubImage1DARB = dllCompressedTexSubImage1DARB = NULL;
      glGetCompressedTexImageARB   = dllGetCompressedTexImageARB   = NULL;

      gGLState.suppTextureCompression = false;
   }

   // 3DFX_texture_compression_FXT1
   if (pExtString && dStrstr(pExtString, (const char*)"GL_3DFX_texture_compression_FXT1") != NULL)
      gGLState.suppFXT1 = true;
   else
      gGLState.suppFXT1 = false;

   // EXT_texture_compression_S3TC
   if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_texture_compression_s3tc") != NULL)
      gGLState.suppS3TC = true;
   else
      gGLState.suppS3TC = false;

   // WGL_3DFX_gamma_control
   if (pExtString && dStrstr(pExtString, (const char*)"WGL_3DFX_gamma_control" ) != NULL)
   { 
      qwglGetDeviceGammaRamp3DFX = (qwglGetDeviceGammaRamp3DFX_t) qwglGetProcAddress( "wglGetDeviceGammaRamp3DFX" ); 
      qwglSetDeviceGammaRamp3DFX = (qwglSetDeviceGammaRamp3DFX_t) qwglGetProcAddress( "wglSetDeviceGammaRamp3DFX" );
   }
   else
   {
      qwglGetDeviceGammaRamp3DFX = NULL;
      qwglSetDeviceGammaRamp3DFX = NULL;
   }

	if (pExtString && dStrstr(pExtString, (const char*)"GL_EXT_vertex_buffer") != NULL)
	{
		glAvailableVertexBufferEXT	= dllAvailableVertexBufferEXT	= (glAvailableVertexBufferEXT_t)	qwglGetProcAddress("glAvailableVertexBufferEXT");
		glAllocateVertexBufferEXT	= dllAllocateVertexBufferEXT	= (glAllocateVertexBufferEXT_t)	qwglGetProcAddress("glAllocateVertexBufferEXT");
		glLockVertexBufferEXT		= dllLockVertexBufferEXT		= (glLockVertexBufferEXT_t)		qwglGetProcAddress("glLockVertexBufferEXT");
		glUnlockVertexBufferEXT		= dllUnlockVertexBufferEXT		= (glUnlockVertexBufferEXT_t)		qwglGetProcAddress("glUnlockVertexBufferEXT");
		glSetVertexBufferEXT			= dllSetVertexBufferEXT			= (glSetVertexBufferEXT_t)			qwglGetProcAddress("glSetVertexBufferEXT");
		glOffsetVertexBufferEXT		= dllOffsetVertexBufferEXT		= (glOffsetVertexBufferEXT_t)		qwglGetProcAddress("glOffsetVertexBufferEXT");
		glFillVertexBufferEXT		= dllFillVertexBufferEXT		= (glFillVertexBufferEXT_t)		qwglGetProcAddress("glFillVertexBufferEXT");
		glFreeVertexBufferEXT		= dllFreeVertexBufferEXT		= (glFreeVertexBufferEXT_t)		qwglGetProcAddress("glFreeVertexBufferEXT");

		gGLState.suppVertexBuffer = true;
	}
	else
	{
		glAvailableVertexBufferEXT	= dllAvailableVertexBufferEXT	= NULL;
		glAllocateVertexBufferEXT	= dllAllocateVertexBufferEXT	= NULL;
		glLockVertexBufferEXT		= dllLockVertexBufferEXT		= NULL;
		glUnlockVertexBufferEXT		= dllUnlockVertexBufferEXT		= NULL;
		glSetVertexBufferEXT			= dllSetVertexBufferEXT			= NULL;
		glOffsetVertexBufferEXT		= dllOffsetVertexBufferEXT		= NULL;
		glFillVertexBufferEXT		= dllFillVertexBufferEXT		= NULL;
		glFreeVertexBufferEXT		= dllFreeVertexBufferEXT		= NULL;

		gGLState.suppVertexBuffer = false;
	}

   // Binary states, i.e., no supporting functions
   // EXT_packed_pixels
   // EXT_texture_env_combine
   //
   // dhc note: a number of these can have multiple matching 'versions', private, ext, and arb.
   gGLState.suppPackedPixels      = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_packed_pixels") != NULL) : false;
   gGLState.suppTextureEnvCombine = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_env_combine") != NULL) : false;
   gGLState.suppEdgeClamp         = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_edge_clamp") != NULL) : false;
   gGLState.suppEdgeClamp        |= pExtString? (dStrstr(pExtString, (const char*)"GL_SGIS_texture_edge_clamp") != NULL) : false;
   gGLState.suppTexEnvAdd         = pExtString? (dStrstr(pExtString, (const char*)"GL_ARB_texture_env_add") != NULL) : false;
   gGLState.suppTexEnvAdd        |= pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_env_add") != NULL) : false;

   // Anisotropic filtering
   gGLState.suppTexAnisotropic    = pExtString? (dStrstr(pExtString, (const char*)"GL_EXT_texture_filter_anisotropic") != NULL) : false;
   if (gGLState.suppTexAnisotropic)
      glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gGLState.maxAnisotropy);
   if (gGLState.suppARBMultitexture)
      glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gGLState.maxTextureUnits);
   else
      gGLState.maxTextureUnits = 1;

   // Swap interval
   if (pExtString && dStrstr(pExtString, (const char*)"WGL_EXT_swap_control") != NULL)
   {
      qwglSwapIntervalEXT = (qwglSwapIntervalEXT_t) qwglGetProcAddress( "wglSwapIntervalEXT" );
      gGLState.suppSwapInterval = ( qwglSwapIntervalEXT != NULL );
   }
   else
   {
      qwglSwapIntervalEXT = NULL;
      gGLState.suppSwapInterval = false;
   }
   
   // NPatch/Truform support ========================================
#if ENABLE_NPATCH
   gGLState.suppNPatch = false;
   if (pExtString && dStrstr(pExtString, (const char*)GL_NPATCH_EXT_STRING) != NULL)
   {
      glGetIntegerv(GETINT_NPATCH_MAX_LEVEL, &gGLState.maxNPatchLevel);
      glNPatchSetInt = (PFNNPatchSetInt)qwglGetProcAddress(GL_NPATCH_SETINT_STRING);
      if (glNPatchSetInt && gGLState.maxNPatchLevel)
         gGLState.suppNPatch = true;
      if (gGLState.suppNPatch)
      {
         gNPatch = Con::getBoolVariable("$pref::OpenGL::nPatch", false);
         gNPatchLevel = Con::getIntVariable("$pref::OpenGL::nPatchLevel", 2);
      }
   }
#endif
   
   Con::printf("OpenGL Init: Enabled Extensions");
   if (gGLState.suppARBMultitexture)    Con::printf("  ARB_multitexture (Max Texture Units: %d)", gGLState.maxTextureUnits);
   if (gGLState.suppPalettedTexture)    Con::printf("  EXT_paletted_texture");
   if (gGLState.suppLockedArrays)       Con::printf("  EXT_compiled_vertex_array");
   if (gGLState.suppVertexArrayRange)   Con::printf("  NV_vertex_array_range");
   if (gGLState.suppTextureEnvCombine)  Con::printf("  EXT_texture_env_combine");
   if (gGLState.suppPackedPixels)       Con::printf("  EXT_packed_pixels");
   if (gGLState.suppFogCoord)           Con::printf("  EXT_fog_coord");
   if (gGLState.suppTextureCompression) Con::printf("  ARB_texture_compression");
   if (gGLState.suppS3TC)               Con::printf("  EXT_texture_compression_s3tc");
   if (gGLState.suppFXT1)               Con::printf("  3DFX_texture_compression_FXT1");
   if (gGLState.suppTexEnvAdd)          Con::printf("  (ARB|EXT)_texture_env_add");
   if (gGLState.suppTexAnisotropic)     Con::printf("  EXT_texture_filter_anisotropic (Max anisotropy: %f)", gGLState.maxAnisotropy);
   if (gGLState.suppSwapInterval)       Con::printf("  WGL_EXT_swap_control");
#if ENABLE_NPATCH
   if (gGLState.suppNPatch)             Con::printf("  NPatch tessellation");
#endif

   Con::warnf("OpenGL Init: Disabled Extensions");
   if (!gGLState.suppARBMultitexture)    Con::warnf("  ARB_multitexture");
   if (!gGLState.suppPalettedTexture)    Con::warnf("  EXT_paletted_texture");
   if (!gGLState.suppLockedArrays)       Con::warnf("  EXT_compiled_vertex_array");
   if (!gGLState.suppVertexArrayRange)   Con::warnf("  NV_vertex_array_range");
   if (!gGLState.suppTextureEnvCombine)  Con::warnf("  EXT_texture_env_combine");
   if (!gGLState.suppPackedPixels)       Con::warnf("  EXT_packed_pixels");
   if (!gGLState.suppFogCoord)           Con::warnf("  EXT_fog_coord");
   if (!gGLState.suppTextureCompression) Con::warnf("  ARB_texture_compression");
   if (!gGLState.suppS3TC)               Con::warnf("  EXT_texture_compression_s3tc");
   if (!gGLState.suppFXT1)               Con::warnf("  3DFX_texture_compression_FXT1");
   if (!gGLState.suppTexEnvAdd)          Con::warnf("  (ARB|EXT)_texture_env_add");
   if (!gGLState.suppTexAnisotropic)     Con::warnf("  EXT_texture_filter_anisotropic");
   if (!gGLState.suppSwapInterval)       Con::warnf("  WGL_EXT_swap_control");
#if ENABLE_NPATCH
   if (!gGLState.suppNPatch)             Con::warnf("  NPatch tessellation");
#endif
   Con::printf("");

   // Set some console variables:
   Con::setBoolVariable( "$FogCoordSupported", gGLState.suppFogCoord );
   Con::setBoolVariable( "$TextureCompressionSupported", gGLState.suppTextureCompression );
   Con::setBoolVariable( "$AnisotropySupported", gGLState.suppTexAnisotropic );
	Con::setBoolVariable( "$PalettedTextureSupported", gGLState.suppPalettedTexture );
   Con::setBoolVariable( "$SwapIntervalSupported", gGLState.suppSwapInterval );

	if (!gGLState.suppPalettedTexture && Con::getBoolVariable("$pref::OpenGL::forcePalettedTexture",false))
	{
		Con::setBoolVariable("$pref::OpenGL::forcePalettedTexture", false);
		Con::setBoolVariable("$pref::OpenGL::force16BitTexture", true);
	}

   return true;
}

//--------------------------------------
#if ENABLE_NPATCH

void dglSetNPatch(GLint npenable)
{
   if (!gGLState.suppNPatch) return;

   if (npenable)
      glEnable(GL_NPATCH_FLAG);
   else
      glDisable(GL_NPATCH_FLAG);

   glGetError(); // clear any error flags.  !!!TBD - check error flags, in case we need to do something!
}

void dglSetNPatchLevel(GLint nplevel)
{
   if (!gGLState.suppNPatch) return;
   
   if (nplevel < 0)
      nplevel = 0;
   else
   if (nplevel > gGLState.maxNPatchLevel)
      nplevel = gGLState.maxNPatchLevel;
   
   glNPatchSetInt(GL_NPATCH_LOD, nplevel);

   glGetError(); // clear any error flags.  !!!TBD - check error flags, in case we need to do something!
}

void dglSetNPatchInterp(GLint nppoint, GLint npnormal)
{
   if (!gGLState.suppNPatch) return;

   glNPatchSetInt(SETINT_NPATCH_POINTINTERP, nppoint?NPATCH_POINTINTERP_MAX:NPATCH_POINTINTERP_MIN);
   glNPatchSetInt(SETINT_NPATCH_NORMALINTERP, npnormal?NPATCH_NORMALINTERP_MAX:NPATCH_NORMALINTERP_MIN);

   glGetError(); // clear any error flags.  !!!TBD - check error flags, in case we need to do something!
}

#endif // ENABLE_NPATCH
//--------------------------------------

static bool loggingEnabled = false;
static bool outlineEnabled = false;
static bool perfEnabled = false;

#if defined (TORQUE_DEBUG) || defined(INTERNAL_RELEASE)
ConsoleFunction(GLEnableLogging, void, 2, 2, "GLEnableLogging(bool);")
{
   argc;
   bool enable = dAtob(argv[1]);

   if(loggingEnabled == enable)
      return;
   
   if(enable && (outlineEnabled || perfEnabled))
      return;
   
   loggingEnabled = enable;

   if ( enable )
   {
      if ( !winState.log_fp )
      {
         struct tm *newtime;
         time_t aclock;

         time( &aclock );
         newtime = localtime( &aclock );

         asctime( newtime );

         winState.log_fp = fopen( "gl_log.txt", "wt" );

         fprintf( winState.log_fp, "%s\n", asctime( newtime ) );
         fflush(winState.log_fp);
      }


      // GLU Functions
      gluOrtho2D                  = loggluOrtho2D;
      gluPerspective              = loggluPerspective;
      gluPickMatrix               = loggluPickMatrix;
      gluLookAt                   = loggluLookAt;
      gluProject                  = loggluProject;
      gluUnProject                = loggluUnProject;
      gluScaleImage               = loggluScaleImage;
      gluBuild1DMipmaps           = loggluBuild1DMipmaps;
      gluBuild2DMipmaps           = loggluBuild2DMipmaps;

      // GL Functions
      glAccum                     = logAccum;
      glAlphaFunc                 = logAlphaFunc;
      glAreTexturesResident       = logAreTexturesResident;
      glArrayElement              = logArrayElement;
      glBegin                     = logBegin;
      glBindTexture               = logBindTexture;
      glBitmap                    = logBitmap;
      glBlendFunc                 = logBlendFunc;
      glCallList                  = logCallList;
      glCallLists                 = logCallLists;
      glClear                     = logClear;
      glClearAccum                = logClearAccum;
      glClearColor                = logClearColor;
      glClearDepth                = logClearDepth;
      glClearIndex                = logClearIndex;
      glClearStencil              = logClearStencil;
      glClipPlane                 = logClipPlane;
      glColor3b                   = logColor3b;
      glColor3bv                  = logColor3bv;
      glColor3d                   = logColor3d;
      glColor3dv                  = logColor3dv;
      glColor3f                   = logColor3f;
      glColor3fv                  = logColor3fv;
      glColor3i                   = logColor3i;
      glColor3iv                  = logColor3iv;
      glColor3s                   = logColor3s;
      glColor3sv                  = logColor3sv;
      glColor3ub                  = logColor3ub;
      glColor3ubv                 = logColor3ubv;
      glColor3ui                  = logColor3ui;
      glColor3uiv                 = logColor3uiv;
      glColor3us                  = logColor3us;
      glColor3usv                 = logColor3usv;
      glColor4b                   = logColor4b;
      glColor4bv                  = logColor4bv;
      glColor4d                   = logColor4d;
      glColor4dv                  = logColor4dv;
      glColor4f                   = logColor4f;
      glColor4fv                  = logColor4fv;
      glColor4i                   = logColor4i;
      glColor4iv                  = logColor4iv;
      glColor4s                   = logColor4s;
      glColor4sv                  = logColor4sv;
      glColor4ub                  = logColor4ub;
      glColor4ubv                 = logColor4ubv;
      glColor4ui                  = logColor4ui;
      glColor4uiv                 = logColor4uiv;
      glColor4us                  = logColor4us;
      glColor4usv                 = logColor4usv;
      glColorMask                 = logColorMask;
      glColorMaterial             = logColorMaterial;
      glColorPointer              = logColorPointer;
      glCopyPixels                = logCopyPixels;
      glCopyTexImage1D            = logCopyTexImage1D;
      glCopyTexImage2D            = logCopyTexImage2D;
      glCopyTexSubImage1D         = logCopyTexSubImage1D;
      glCopyTexSubImage2D         = logCopyTexSubImage2D;
      glCullFace                  = logCullFace;
      glDeleteLists               = logDeleteLists ;
      glDeleteTextures            = logDeleteTextures;
      glDepthFunc                 = logDepthFunc;
      glDepthMask                 = logDepthMask;
      glDepthRange                = logDepthRange;
      glDisable                   = logDisable;
      glDisableClientState        = logDisableClientState;
      glDrawArrays                = logDrawArrays;
      glDrawBuffer                = logDrawBuffer;
      glDrawElements              = logDrawElements;
      glDrawPixels                = logDrawPixels;
      glEdgeFlag                  = logEdgeFlag;
      glEdgeFlagPointer           = logEdgeFlagPointer;
      glEdgeFlagv                 = logEdgeFlagv;
      glEnable                    = logEnable;
      glEnableClientState         = logEnableClientState;
      glEnd                       = logEnd;
      glEndList                   = logEndList;
      glEvalCoord1d               = logEvalCoord1d;
      glEvalCoord1dv              = logEvalCoord1dv;
      glEvalCoord1f               = logEvalCoord1f;
      glEvalCoord1fv              = logEvalCoord1fv;
      glEvalCoord2d               = logEvalCoord2d;
      glEvalCoord2dv              = logEvalCoord2dv;
      glEvalCoord2f               = logEvalCoord2f;
      glEvalCoord2fv              = logEvalCoord2fv;
      glEvalMesh1                 = logEvalMesh1;
      glEvalMesh2                 = logEvalMesh2;
      glEvalPoint1                = logEvalPoint1;
      glEvalPoint2                = logEvalPoint2;
      glFeedbackBuffer            = logFeedbackBuffer;
      glFinish                    = logFinish;
      glFlush                     = logFlush;
      glFogf                      = logFogf;
      glFogfv                     = logFogfv;
      glFogi                      = logFogi;
      glFogiv                     = logFogiv;
      glFrontFace                 = logFrontFace;
      glFrustum                   = logFrustum;
      glGenLists                  = logGenLists;
      glGenTextures               = logGenTextures;
      glGetBooleanv               = logGetBooleanv;
      glGetClipPlane              = logGetClipPlane;
      glGetDoublev                = logGetDoublev;
      glGetError                  = logGetError;
      glGetFloatv                 = logGetFloatv;
      glGetIntegerv               = logGetIntegerv;
      glGetLightfv                = logGetLightfv;
      glGetLightiv                = logGetLightiv;
      glGetMapdv                  = logGetMapdv;
      glGetMapfv                  = logGetMapfv;
      glGetMapiv                  = logGetMapiv;
      glGetMaterialfv             = logGetMaterialfv;
      glGetMaterialiv             = logGetMaterialiv;
      glGetPixelMapfv             = logGetPixelMapfv;
      glGetPixelMapuiv            = logGetPixelMapuiv;
      glGetPixelMapusv            = logGetPixelMapusv;
      glGetPointerv               = logGetPointerv;
      glGetPolygonStipple         = logGetPolygonStipple;
      // JMQ: MinGW gcc 3.2 needs the following cast
      glGetString                 = (glGetString_t)logGetString;
      glGetTexEnvfv               = logGetTexEnvfv;
      glGetTexEnviv               = logGetTexEnviv;
      glGetTexGendv               = logGetTexGendv;
      glGetTexGenfv               = logGetTexGenfv;
      glGetTexGeniv               = logGetTexGeniv;
      glGetTexImage               = logGetTexImage;
      glGetTexLevelParameterfv    = logGetTexLevelParameterfv;
      glGetTexLevelParameteriv    = logGetTexLevelParameteriv;
      glGetTexParameterfv         = logGetTexParameterfv;
      glGetTexParameteriv         = logGetTexParameteriv;
      glHint                      = logHint;
      glIndexMask                 = logIndexMask;
      glIndexPointer              = logIndexPointer;
      glIndexd                    = logIndexd;
      glIndexdv                   = logIndexdv;
      glIndexf                    = logIndexf;
      glIndexfv                   = logIndexfv;
      glIndexi                    = logIndexi;
      glIndexiv                   = logIndexiv;
      glIndexs                    = logIndexs;
      glIndexsv                   = logIndexsv;
      glIndexub                   = logIndexub;
      glIndexubv                  = logIndexubv;
      glInitNames                 = logInitNames;
      glInterleavedArrays         = logInterleavedArrays;
      glIsEnabled                 = logIsEnabled;
      glIsList                    = logIsList;
      glIsTexture                 = logIsTexture;
      glLightModelf               = logLightModelf;
      glLightModelfv              = logLightModelfv;
      glLightModeli               = logLightModeli;
      glLightModeliv              = logLightModeliv;
      glLightf                    = logLightf;
      glLightfv                   = logLightfv;
      glLighti                    = logLighti;
      glLightiv                   = logLightiv;
      glLineStipple               = logLineStipple;
      glLineWidth                 = logLineWidth;
      glListBase                  = logListBase;
      glLoadIdentity              = logLoadIdentity;
      glLoadMatrixd               = logLoadMatrixd;
      glLoadMatrixf               = logLoadMatrixf;
      glLoadName                  = logLoadName;
      glLogicOp                   = logLogicOp;
      glMap1d                     = logMap1d;
      glMap1f                     = logMap1f;
      glMap2d                     = logMap2d;
      glMap2f                     = logMap2f;
      glMapGrid1d                 = logMapGrid1d;
      glMapGrid1f                 = logMapGrid1f;
      glMapGrid2d                 = logMapGrid2d;
      glMapGrid2f                 = logMapGrid2f;
      glMaterialf                 = logMaterialf;
      glMaterialfv                = logMaterialfv;
      glMateriali                 = logMateriali;
      glMaterialiv                = logMaterialiv;
      glMatrixMode                = logMatrixMode;
      glMultMatrixd               = logMultMatrixd;
      glMultMatrixf               = logMultMatrixf;
      glNewList                   = logNewList;
      glNormal3b                  = logNormal3b;
      glNormal3bv                 = logNormal3bv;
      glNormal3d                  = logNormal3d;
      glNormal3dv                 = logNormal3dv;
      glNormal3f                  = logNormal3f;
      glNormal3fv                 = logNormal3fv;
      glNormal3i                  = logNormal3i;
      glNormal3iv                 = logNormal3iv;
      glNormal3s                  = logNormal3s;
      glNormal3sv                 = logNormal3sv;
      glNormalPointer             = logNormalPointer;
      glOrtho                     = logOrtho;
      glPassThrough               = logPassThrough;
      glPixelMapfv                = logPixelMapfv;
      glPixelMapuiv               = logPixelMapuiv;
      glPixelMapusv               = logPixelMapusv;
      glPixelStoref               = logPixelStoref;
      glPixelStorei               = logPixelStorei;
      glPixelTransferf            = logPixelTransferf;
      glPixelTransferi            = logPixelTransferi;
      glPixelZoom                 = logPixelZoom;
      glPointSize                 = logPointSize;
      glPolygonMode               = logPolygonMode;
      glPolygonOffset             = logPolygonOffset;
      glPolygonStipple            = logPolygonStipple;
      glPopAttrib                 = logPopAttrib;
      glPopClientAttrib           = logPopClientAttrib;
      glPopMatrix                 = logPopMatrix;
      glPopName                   = logPopName;
      glPrioritizeTextures        = logPrioritizeTextures;
      glPushAttrib                = logPushAttrib;
      glPushClientAttrib          = logPushClientAttrib;
      glPushMatrix                = logPushMatrix;
      glPushName                  = logPushName;
      glRasterPos2d               = logRasterPos2d;
      glRasterPos2dv              = logRasterPos2dv;
      glRasterPos2f               = logRasterPos2f;
      glRasterPos2fv              = logRasterPos2fv;
      glRasterPos2i               = logRasterPos2i;
      glRasterPos2iv              = logRasterPos2iv;
      glRasterPos2s               = logRasterPos2s;
      glRasterPos2sv              = logRasterPos2sv;
      glRasterPos3d               = logRasterPos3d;
      glRasterPos3dv              = logRasterPos3dv;
      glRasterPos3f               = logRasterPos3f;
      glRasterPos3fv              = logRasterPos3fv;
      glRasterPos3i               = logRasterPos3i;
      glRasterPos3iv              = logRasterPos3iv;
      glRasterPos3s               = logRasterPos3s;
      glRasterPos3sv              = logRasterPos3sv;
      glRasterPos4d               = logRasterPos4d;
      glRasterPos4dv              = logRasterPos4dv;
      glRasterPos4f               = logRasterPos4f;
      glRasterPos4fv              = logRasterPos4fv;
      glRasterPos4i               = logRasterPos4i;
      glRasterPos4iv              = logRasterPos4iv;
      glRasterPos4s               = logRasterPos4s;
      glRasterPos4sv              = logRasterPos4sv;
      glReadBuffer                = logReadBuffer;
      glReadPixels                = logReadPixels;
      glRectd                     = logRectd;
      glRectdv                    = logRectdv;
      glRectf                     = logRectf;
      glRectfv                    = logRectfv;
      glRecti                     = logRecti;
      glRectiv                    = logRectiv;
      glRects                     = logRects;
      glRectsv                    = logRectsv;
      glRenderMode                = logRenderMode;
      glRotated                   = logRotated;
      glRotatef                   = logRotatef;
      glScaled                    = logScaled;
      glScalef                    = logScalef;
      glScissor                   = logScissor;
      glSelectBuffer              = logSelectBuffer;
      glShadeModel                = logShadeModel;
      glStencilFunc               = logStencilFunc;
      glStencilMask               = logStencilMask;
      glStencilOp                 = logStencilOp;
      glTexCoord1d                = logTexCoord1d;
      glTexCoord1dv               = logTexCoord1dv;
      glTexCoord1f                = logTexCoord1f;
      glTexCoord1fv               = logTexCoord1fv;
      glTexCoord1i                = logTexCoord1i;
      glTexCoord1iv               = logTexCoord1iv;
      glTexCoord1s                = logTexCoord1s;
      glTexCoord1sv               = logTexCoord1sv;
      glTexCoord2d                = logTexCoord2d;
      glTexCoord2dv               = logTexCoord2dv;
      glTexCoord2f                = logTexCoord2f;
      glTexCoord2fv               = logTexCoord2fv;
      glTexCoord2i                = logTexCoord2i;
      glTexCoord2iv               = logTexCoord2iv;
      glTexCoord2s                = logTexCoord2s;
      glTexCoord2sv               = logTexCoord2sv;
      glTexCoord3d                = logTexCoord3d;
      glTexCoord3dv               = logTexCoord3dv;
      glTexCoord3f                = logTexCoord3f;
      glTexCoord3fv               = logTexCoord3fv;
      glTexCoord3i                = logTexCoord3i;
      glTexCoord3iv               = logTexCoord3iv;
      glTexCoord3s                = logTexCoord3s;
      glTexCoord3sv               = logTexCoord3sv;
      glTexCoord4d                = logTexCoord4d;
      glTexCoord4dv               = logTexCoord4dv;
      glTexCoord4f                = logTexCoord4f;
      glTexCoord4fv               = logTexCoord4fv;
      glTexCoord4i                = logTexCoord4i;
      glTexCoord4iv               = logTexCoord4iv;
      glTexCoord4s                = logTexCoord4s;
      glTexCoord4sv               = logTexCoord4sv;
      glTexCoordPointer           = logTexCoordPointer;
      glTexEnvf                   = logTexEnvf;
      glTexEnvfv                  = logTexEnvfv;
      glTexEnvi                   = logTexEnvi;
      glTexEnviv                  = logTexEnviv;
      glTexGend                   = logTexGend;
      glTexGendv                  = logTexGendv;
      glTexGenf                   = logTexGenf;
      glTexGenfv                  = logTexGenfv;
      glTexGeni                   = logTexGeni;
      glTexGeniv                  = logTexGeniv;
      glTexImage1D                = logTexImage1D;
      glTexImage2D                = logTexImage2D;
      glTexParameterf             = logTexParameterf;
      glTexParameterfv            = logTexParameterfv;
      glTexParameteri             = logTexParameteri;
      glTexParameteriv            = logTexParameteriv;
      glTexSubImage1D             = logTexSubImage1D;
      glTexSubImage2D             = logTexSubImage2D;
      glTranslated                = logTranslated;
      glTranslatef                = logTranslatef;
      glVertex2d                  = logVertex2d;
      glVertex2dv                 = logVertex2dv;
      glVertex2f                  = logVertex2f;
      glVertex2fv                 = logVertex2fv;
      glVertex2i                  = logVertex2i;
      glVertex2iv                 = logVertex2iv;
      glVertex2s                  = logVertex2s;
      glVertex2sv                 = logVertex2sv;
      glVertex3d                  = logVertex3d;
      glVertex3dv                 = logVertex3dv;
      glVertex3f                  = logVertex3f;
      glVertex3fv                 = logVertex3fv;
      glVertex3i                  = logVertex3i;
      glVertex3iv                 = logVertex3iv;
      glVertex3s                  = logVertex3s;
      glVertex3sv                 = logVertex3sv;
      glVertex4d                  = logVertex4d;
      glVertex4dv                 = logVertex4dv;
      glVertex4f                  = logVertex4f;
      glVertex4fv                 = logVertex4fv;
      glVertex4i                  = logVertex4i;
      glVertex4iv                 = logVertex4iv;
      glVertex4s                  = logVertex4s;
      glVertex4sv                 = logVertex4sv;
      glVertexPointer             = logVertexPointer;
      glViewport                  = logViewport;

      if (dglDoesSupportPalettedTexture())
      {
         glColorTableEXT = logColorTableEXT;
      }

      if (dglDoesSupportCompiledVertexArray()) {
         glLockArraysEXT             = logLockArraysEXT;
         glUnlockArraysEXT           = logUnlockArraysEXT;
      }

      if (dglDoesSupportARBMultitexture()) {
         glActiveTextureARB       = logActiveTextureARB;
         glClientActiveTextureARB = logClientActiveTextureARB;
         glMultiTexCoord2fARB     = logMultiTexCoord2fARB;
         glMultiTexCoord2fvARB    = logMultiTexCoord2fvARB;
      }

      if (dglDoesSupportVertexArrayRange()) {
         glVertexArrayRangeNV      = logVertexArrayRangeNV;
         glFlushVertexArrayRangeNV = logFlushVertexArrayRangeNV;
         // JMQ: MinGW gcc 3.2 needs the following cast
         wglAllocateMemoryNV       = (wglAllocateMemory_t)logAllocateMemoryNV;
         wglFreeMemoryNV           = logFreeMemoryNV;
      }

      if (dglDoesSupportFogCoord()) {
         glFogCoordfEXT            = logFogCoordfEXT;
         glFogCoordPointerEXT      = logFogCoordPointerEXT;
      }

      if (dglDoesSupportTextureCompression()) {
         glCompressedTexImage3DARB    = logCompressedTexImage3DARB;
         glCompressedTexImage2DARB    = logCompressedTexImage2DARB;
         glCompressedTexImage1DARB    = logCompressedTexImage1DARB;
         glCompressedTexSubImage3DARB = logCompressedTexSubImage3DARB;
         glCompressedTexSubImage2DARB = logCompressedTexSubImage2DARB;
         glCompressedTexSubImage1DARB = logCompressedTexSubImage1DARB;
         glGetCompressedTexImageARB   = logGetCompressedTexImageARB;
      }

		if (dglDoesSupportVertexBuffer()) {
			glAvailableVertexBufferEXT	= logAvailableVertexBufferEXT;
			glAllocateVertexBufferEXT	= logAllocateVertexBufferEXT;
			// JMQ: MinGW gcc 3.2 needs the following cast
			glLockVertexBufferEXT		= (glLockVertexBufferEXT_t)logLockVertexBufferEXT;
			glUnlockVertexBufferEXT		= logUnlockVertexBufferEXT;
			glSetVertexBufferEXT			= logSetVertexBufferEXT;
			glOffsetVertexBufferEXT		= logOffsetVertexBufferEXT;
			glFillVertexBufferEXT		= logFillVertexBufferEXT;
			glFreeVertexBufferEXT		= logFreeVertexBufferEXT;
		}
   }
   else
   {
      if ( winState.log_fp )   {
         fprintf( winState.log_fp, "*** CLOSING LOG ***\n" );
         fflush(winState.log_fp);
         fclose( winState.log_fp );
         winState.log_fp = NULL;
      }

      // GLU Functions
      gluOrtho2D                  = dllgluOrtho2D;
      gluPerspective              = dllgluPerspective;
      gluPickMatrix               = dllgluPickMatrix;
      gluLookAt                   = dllgluLookAt;
      gluProject                  = dllgluProject;
      gluUnProject                = dllgluUnProject;
      gluScaleImage               = dllgluScaleImage;
      gluBuild1DMipmaps           = dllgluBuild1DMipmaps;
      gluBuild2DMipmaps           = dllgluBuild2DMipmaps;

      // GL Functions
      glAccum                     = dllAccum;
      glAlphaFunc                 = dllAlphaFunc;
      glAreTexturesResident       = dllAreTexturesResident;
      glArrayElement              = dllArrayElement;
      glBegin                     = dllBegin;
      glBindTexture               = dllBindTexture;
      glBitmap                    = dllBitmap;
      glBlendFunc                 = dllBlendFunc;
      glCallList                  = dllCallList;
      glCallLists                 = dllCallLists;
      glClear                     = dllClear;
      glClearAccum                = dllClearAccum;
      glClearColor                = dllClearColor;
      glClearDepth                = dllClearDepth;
      glClearIndex                = dllClearIndex;
      glClearStencil              = dllClearStencil;
      glClipPlane                 = dllClipPlane;
      glColor3b                   = dllColor3b;
      glColor3bv                  = dllColor3bv;
      glColor3d                   = dllColor3d;
      glColor3dv                  = dllColor3dv;
      glColor3f                   = dllColor3f;
      glColor3fv                  = dllColor3fv;
      glColor3i                   = dllColor3i;
      glColor3iv                  = dllColor3iv;
      glColor3s                   = dllColor3s;
      glColor3sv                  = dllColor3sv;
      glColor3ub                  = dllColor3ub;
      glColor3ubv                 = dllColor3ubv;
      glColor3ui                  = dllColor3ui;
      glColor3uiv                 = dllColor3uiv;
      glColor3us                  = dllColor3us;
      glColor3usv                 = dllColor3usv;
      glColor4b                   = dllColor4b;
      glColor4bv                  = dllColor4bv;
      glColor4d                   = dllColor4d;
      glColor4dv                  = dllColor4dv;
      glColor4f                   = dllColor4f;
      glColor4fv                  = dllColor4fv;
      glColor4i                   = dllColor4i;
      glColor4iv                  = dllColor4iv;
      glColor4s                   = dllColor4s;
      glColor4sv                  = dllColor4sv;
      glColor4ub                  = dllColor4ub;
      glColor4ubv                 = dllColor4ubv;
      glColor4ui                  = dllColor4ui;
      glColor4uiv                 = dllColor4uiv;
      glColor4us                  = dllColor4us;
      glColor4usv                 = dllColor4usv;
      glColorMask                 = dllColorMask;
      glColorMaterial             = dllColorMaterial;
      glColorPointer              = dllColorPointer;
      glCopyPixels                = dllCopyPixels;
      glCopyTexImage1D            = dllCopyTexImage1D;
      glCopyTexImage2D            = dllCopyTexImage2D;
      glCopyTexSubImage1D         = dllCopyTexSubImage1D;
      glCopyTexSubImage2D         = dllCopyTexSubImage2D;
      glCullFace                  = dllCullFace;
      glDeleteLists               = dllDeleteLists;
      glDeleteTextures            = dllDeleteTextures;
      glDepthFunc                 = dllDepthFunc;
      glDepthMask                 = dllDepthMask;
      glDepthRange                = dllDepthRange;
      glDisable                   = dllDisable;
      glDisableClientState        = dllDisableClientState;
      glDrawArrays                = dllDrawArrays;
      glDrawBuffer                = dllDrawBuffer;
      glDrawElements              = dllDrawElements;
      glDrawPixels                = dllDrawPixels;
      glEdgeFlag                  = dllEdgeFlag;
      glEdgeFlagPointer           = dllEdgeFlagPointer;
      glEdgeFlagv                 = dllEdgeFlagv;
      glEnable                    = dllEnable;
      glEnableClientState         = dllEnableClientState;
      glEnd                       = dllEnd;
      glEndList                   = dllEndList;
      glEvalCoord1d               = dllEvalCoord1d;
      glEvalCoord1dv              = dllEvalCoord1dv;
      glEvalCoord1f               = dllEvalCoord1f;
      glEvalCoord1fv              = dllEvalCoord1fv;
      glEvalCoord2d               = dllEvalCoord2d;
      glEvalCoord2dv              = dllEvalCoord2dv;
      glEvalCoord2f               = dllEvalCoord2f;
      glEvalCoord2fv              = dllEvalCoord2fv;
      glEvalMesh1                 = dllEvalMesh1;
      glEvalMesh2                 = dllEvalMesh2;
      glEvalPoint1                = dllEvalPoint1;
      glEvalPoint2                = dllEvalPoint2;
      glFeedbackBuffer            = dllFeedbackBuffer;
      glFinish                    = dllFinish;
      glFlush                     = dllFlush;
      glFogf                      = dllFogf;
      glFogfv                     = dllFogfv;
      glFogi                      = dllFogi;
      glFogiv                     = dllFogiv;
      glFrontFace                 = dllFrontFace;
      glFrustum                   = dllFrustum;
      glGenLists                  = dllGenLists;
      glGenTextures               = dllGenTextures;
      glGetBooleanv               = dllGetBooleanv;
      glGetClipPlane              = dllGetClipPlane;
      glGetDoublev                = dllGetDoublev;
      glGetError                  = dllGetError;
      glGetFloatv                 = dllGetFloatv;
      glGetIntegerv               = dllGetIntegerv;
      glGetLightfv                = dllGetLightfv;
      glGetLightiv                = dllGetLightiv;
      glGetMapdv                  = dllGetMapdv;
      glGetMapfv                  = dllGetMapfv;
      glGetMapiv                  = dllGetMapiv;
      glGetMaterialfv             = dllGetMaterialfv;
      glGetMaterialiv             = dllGetMaterialiv;
      glGetPixelMapfv             = dllGetPixelMapfv;
      glGetPixelMapuiv            = dllGetPixelMapuiv;
      glGetPixelMapusv            = dllGetPixelMapusv;
      glGetPointerv               = dllGetPointerv;
      glGetPolygonStipple         = dllGetPolygonStipple;
      glGetString                 = dllGetString;
      glGetTexEnvfv               = dllGetTexEnvfv;
      glGetTexEnviv               = dllGetTexEnviv;
      glGetTexGendv               = dllGetTexGendv;
      glGetTexGenfv               = dllGetTexGenfv;
      glGetTexGeniv               = dllGetTexGeniv;
      glGetTexImage               = dllGetTexImage;
      glGetTexLevelParameterfv    = dllGetTexLevelParameterfv;
      glGetTexLevelParameteriv    = dllGetTexLevelParameteriv;
      glGetTexParameterfv         = dllGetTexParameterfv;
      glGetTexParameteriv         = dllGetTexParameteriv;
      glHint                      = dllHint;
      glIndexMask                 = dllIndexMask;
      glIndexPointer              = dllIndexPointer;
      glIndexd                    = dllIndexd;
      glIndexdv                   = dllIndexdv;
      glIndexf                    = dllIndexf;
      glIndexfv                   = dllIndexfv;
      glIndexi                    = dllIndexi;
      glIndexiv                   = dllIndexiv;
      glIndexs                    = dllIndexs;
      glIndexsv                   = dllIndexsv;
      glIndexub                   = dllIndexub;
      glIndexubv                  = dllIndexubv;
      glInitNames                 = dllInitNames;
      glInterleavedArrays         = dllInterleavedArrays;
      glIsEnabled                 = dllIsEnabled;
      glIsList                    = dllIsList;
      glIsTexture                 = dllIsTexture;
      glLightModelf               = dllLightModelf;
      glLightModelfv              = dllLightModelfv;
      glLightModeli               = dllLightModeli;
      glLightModeliv              = dllLightModeliv;
      glLightf                    = dllLightf;
      glLightfv                   = dllLightfv;
      glLighti                    = dllLighti;
      glLightiv                   = dllLightiv;
      glLineStipple               = dllLineStipple;
      glLineWidth                 = dllLineWidth;
      glListBase                  = dllListBase;
      glLoadIdentity              = dllLoadIdentity;
      glLoadMatrixd               = dllLoadMatrixd;
      glLoadMatrixf               = dllLoadMatrixf;
      glLoadName                  = dllLoadName;
      glLogicOp                   = dllLogicOp;
      glMap1d                     = dllMap1d;
      glMap1f                     = dllMap1f;
      glMap2d                     = dllMap2d;
      glMap2f                     = dllMap2f;
      glMapGrid1d                 = dllMapGrid1d;
      glMapGrid1f                 = dllMapGrid1f;
      glMapGrid2d                 = dllMapGrid2d;
      glMapGrid2f                 = dllMapGrid2f;
      glMaterialf                 = dllMaterialf;
      glMaterialfv                = dllMaterialfv;
      glMateriali                 = dllMateriali;
      glMaterialiv                = dllMaterialiv;
      glMatrixMode                = dllMatrixMode;
      glMultMatrixd               = dllMultMatrixd;
      glMultMatrixf               = dllMultMatrixf;
      glNewList                   = dllNewList;
      glNormal3b                  = dllNormal3b;
      glNormal3bv                 = dllNormal3bv;
      glNormal3d                  = dllNormal3d;
      glNormal3dv                 = dllNormal3dv;
      glNormal3f                  = dllNormal3f;
      glNormal3fv                 = dllNormal3fv;
      glNormal3i                  = dllNormal3i;
      glNormal3iv                 = dllNormal3iv;
      glNormal3s                  = dllNormal3s;
      glNormal3sv                 = dllNormal3sv;
      glNormalPointer             = dllNormalPointer;
      glOrtho                     = dllOrtho;
      glPassThrough               = dllPassThrough;
      glPixelMapfv                = dllPixelMapfv;
      glPixelMapuiv               = dllPixelMapuiv;
      glPixelMapusv               = dllPixelMapusv;
      glPixelStoref               = dllPixelStoref;
      glPixelStorei               = dllPixelStorei;
      glPixelTransferf            = dllPixelTransferf;
      glPixelTransferi            = dllPixelTransferi;
      glPixelZoom                 = dllPixelZoom;
      glPointSize                 = dllPointSize;
      glPolygonMode               = dllPolygonMode;
      glPolygonOffset             = dllPolygonOffset;
      glPolygonStipple            = dllPolygonStipple;
      glPopAttrib                 = dllPopAttrib;
      glPopClientAttrib           = dllPopClientAttrib;
      glPopMatrix                 = dllPopMatrix;
      glPopName                   = dllPopName;
      glPrioritizeTextures        = dllPrioritizeTextures;
      glPushAttrib                = dllPushAttrib;
      glPushClientAttrib          = dllPushClientAttrib;
      glPushMatrix                = dllPushMatrix;
      glPushName                  = dllPushName;
      glRasterPos2d               = dllRasterPos2d;
      glRasterPos2dv              = dllRasterPos2dv;
      glRasterPos2f               = dllRasterPos2f;
      glRasterPos2fv              = dllRasterPos2fv;
      glRasterPos2i               = dllRasterPos2i;
      glRasterPos2iv              = dllRasterPos2iv;
      glRasterPos2s               = dllRasterPos2s;
      glRasterPos2sv              = dllRasterPos2sv;
      glRasterPos3d               = dllRasterPos3d;
      glRasterPos3dv              = dllRasterPos3dv;
      glRasterPos3f               = dllRasterPos3f;
      glRasterPos3fv              = dllRasterPos3fv;
      glRasterPos3i               = dllRasterPos3i;
      glRasterPos3iv              = dllRasterPos3iv;
      glRasterPos3s               = dllRasterPos3s;
      glRasterPos3sv              = dllRasterPos3sv;
      glRasterPos4d               = dllRasterPos4d;
      glRasterPos4dv              = dllRasterPos4dv;
      glRasterPos4f               = dllRasterPos4f;
      glRasterPos4fv              = dllRasterPos4fv;
      glRasterPos4i               = dllRasterPos4i;
      glRasterPos4iv              = dllRasterPos4iv;
      glRasterPos4s               = dllRasterPos4s;
      glRasterPos4sv              = dllRasterPos4sv;
      glReadBuffer                = dllReadBuffer;
      glReadPixels                = dllReadPixels;
      glRectd                     = dllRectd;
      glRectdv                    = dllRectdv;
      glRectf                     = dllRectf;
      glRectfv                    = dllRectfv;
      glRecti                     = dllRecti;
      glRectiv                    = dllRectiv;
      glRects                     = dllRects;
      glRectsv                    = dllRectsv;
      glRenderMode                = dllRenderMode;
      glRotated                   = dllRotated;
      glRotatef                   = dllRotatef;
      glScaled                    = dllScaled;
      glScalef                    = dllScalef;
      glScissor                   = dllScissor;
      glSelectBuffer              = dllSelectBuffer;
      glShadeModel                = dllShadeModel;
      glStencilFunc               = dllStencilFunc;
      glStencilMask               = dllStencilMask;
      glStencilOp                 = dllStencilOp;
      glTexCoord1d                = dllTexCoord1d;
      glTexCoord1dv               = dllTexCoord1dv;
      glTexCoord1f                = dllTexCoord1f;
      glTexCoord1fv               = dllTexCoord1fv;
      glTexCoord1i                = dllTexCoord1i;
      glTexCoord1iv               = dllTexCoord1iv;
      glTexCoord1s                = dllTexCoord1s;
      glTexCoord1sv               = dllTexCoord1sv;
      glTexCoord2d                = dllTexCoord2d;
      glTexCoord2dv               = dllTexCoord2dv;
      glTexCoord2f                = dllTexCoord2f;
      glTexCoord2fv               = dllTexCoord2fv;
      glTexCoord2i                = dllTexCoord2i;
      glTexCoord2iv               = dllTexCoord2iv;
      glTexCoord2s                = dllTexCoord2s;
      glTexCoord2sv               = dllTexCoord2sv;
      glTexCoord3d                = dllTexCoord3d;
      glTexCoord3dv               = dllTexCoord3dv;
      glTexCoord3f                = dllTexCoord3f;
      glTexCoord3fv               = dllTexCoord3fv;
      glTexCoord3i                = dllTexCoord3i;
      glTexCoord3iv               = dllTexCoord3iv;
      glTexCoord3s                = dllTexCoord3s;
      glTexCoord3sv               = dllTexCoord3sv;
      glTexCoord4d                = dllTexCoord4d;
      glTexCoord4dv               = dllTexCoord4dv;
      glTexCoord4f                = dllTexCoord4f;
      glTexCoord4fv               = dllTexCoord4fv;
      glTexCoord4i                = dllTexCoord4i;
      glTexCoord4iv               = dllTexCoord4iv;
      glTexCoord4s                = dllTexCoord4s;
      glTexCoord4sv               = dllTexCoord4sv;
      glTexCoordPointer           = dllTexCoordPointer;
      glTexEnvf                   = dllTexEnvf;
      glTexEnvfv                  = dllTexEnvfv;
      glTexEnvi                   = dllTexEnvi;
      glTexEnviv                  = dllTexEnviv;
      glTexGend                   = dllTexGend;
      glTexGendv                  = dllTexGendv;
      glTexGenf                   = dllTexGenf;
      glTexGenfv                  = dllTexGenfv;
      glTexGeni                   = dllTexGeni;
      glTexGeniv                  = dllTexGeniv;
      glTexImage1D                = dllTexImage1D;
      glTexImage2D                = dllTexImage2D;
      glTexParameterf             = dllTexParameterf;
      glTexParameterfv            = dllTexParameterfv;
      glTexParameteri             = dllTexParameteri;
      glTexParameteriv            = dllTexParameteriv;
      glTexSubImage1D             = dllTexSubImage1D;
      glTexSubImage2D             = dllTexSubImage2D;
      glTranslated                = dllTranslated;
      glTranslatef                = dllTranslatef;
      glVertex2d                  = dllVertex2d;
      glVertex2dv                 = dllVertex2dv;
      glVertex2f                  = dllVertex2f;
      glVertex2fv                 = dllVertex2fv;
      glVertex2i                  = dllVertex2i;
      glVertex2iv                 = dllVertex2iv;
      glVertex2s                  = dllVertex2s;
      glVertex2sv                 = dllVertex2sv;
      glVertex3d                  = dllVertex3d;
      glVertex3dv                 = dllVertex3dv;
      glVertex3f                  = dllVertex3f;
      glVertex3fv                 = dllVertex3fv;
      glVertex3i                  = dllVertex3i;
      glVertex3iv                 = dllVertex3iv;
      glVertex3s                  = dllVertex3s;
      glVertex3sv                 = dllVertex3sv;
      glVertex4d                  = dllVertex4d;
      glVertex4dv                 = dllVertex4dv;
      glVertex4f                  = dllVertex4f;
      glVertex4fv                 = dllVertex4fv;
      glVertex4i                  = dllVertex4i;
      glVertex4iv                 = dllVertex4iv;
      glVertex4s                  = dllVertex4s;
      glVertex4sv                 = dllVertex4sv;
      glVertexPointer             = dllVertexPointer;
      glViewport                  = dllViewport;

      glColorTableEXT             = dllColorTableEXT;
      
      glLockArraysEXT             = dllLockArraysEXT;
      glUnlockArraysEXT           = dllUnlockArraysEXT;

      glActiveTextureARB          = dllActiveTextureARB;
      glClientActiveTextureARB    = dllClientActiveTextureARB;
      glMultiTexCoord2fARB        = dllMultiTexCoord2fARB;
      glMultiTexCoord2fvARB       = dllMultiTexCoord2fvARB;

      glVertexArrayRangeNV        = dllVertexArrayRangeNV;
      glFlushVertexArrayRangeNV   = dllFlushVertexArrayRangeNV;
      wglAllocateMemoryNV         = dllAllocateMemoryNV;
      wglFreeMemoryNV             = dllFreeMemoryNV;

      glFogCoordfEXT              = dllFogCoordfEXT;
      glFogCoordPointerEXT        = dllFogCoordPointerEXT;

      glCompressedTexImage3DARB    = dllCompressedTexImage3DARB;
      glCompressedTexImage2DARB    = dllCompressedTexImage2DARB;
      glCompressedTexImage1DARB    = dllCompressedTexImage1DARB;
      glCompressedTexSubImage3DARB = dllCompressedTexSubImage3DARB;
      glCompressedTexSubImage2DARB = dllCompressedTexSubImage2DARB;
      glCompressedTexSubImage1DARB = dllCompressedTexSubImage1DARB;
      glGetCompressedTexImageARB   = dllGetCompressedTexImageARB;

		glAvailableVertexBufferEXT	= dllAvailableVertexBufferEXT;
		glAllocateVertexBufferEXT	= dllAllocateVertexBufferEXT;
		glLockVertexBufferEXT		= dllLockVertexBufferEXT;
		glUnlockVertexBufferEXT		= dllUnlockVertexBufferEXT;
		glSetVertexBufferEXT			= dllSetVertexBufferEXT;
		glOffsetVertexBufferEXT		= dllOffsetVertexBufferEXT;
		glFillVertexBufferEXT		= dllFillVertexBufferEXT;
		glFreeVertexBufferEXT		= dllFreeVertexBufferEXT;
   }
}

static void APIENTRY outlineDrawArrays(GLenum mode, GLint first, GLsizei count)
{
   if(mode == GL_POLYGON)
      mode = GL_LINE_LOOP;

   if(mode == GL_POINTS || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP || mode == GL_LINES)   
      dllDrawArrays( mode, first, count );
   else
   {
      dllBegin(GL_LINES);
      if(mode == GL_TRIANGLE_STRIP)
      {
         U32 i;
         for(i = 0; i < count - 1; i++) 
         {
            dllArrayElement(first + i);
            dllArrayElement(first + i + 1);
            if(i + 2 != count)
            {
               dllArrayElement(first + i);
               dllArrayElement(first + i + 2);
            }
         }
      }
      else if(mode == GL_TRIANGLE_FAN)
      {
         for(U32 i = 1; i < count; i ++)
         {
            dllArrayElement(first);
            dllArrayElement(first + i);
            if(i != count - 1)
            {
               dllArrayElement(first + i);
               dllArrayElement(first + i + 1);
            }
         }
      }
      else if(mode == GL_TRIANGLES)
      {
         for(U32 i = 3; i <= count; i += 3)
         {
            dllArrayElement(first + i - 3);
            dllArrayElement(first + i - 2);
            dllArrayElement(first + i - 2);
            dllArrayElement(first + i - 1);
            dllArrayElement(first + i - 3);
            dllArrayElement(first + i - 1);
         }
      }
      else if(mode == GL_QUADS)
      {
         for(U32 i = 4; i <= count; i += 4)
         {
            dllArrayElement(first + i - 4);
            dllArrayElement(first + i - 3);
            dllArrayElement(first + i - 3);
            dllArrayElement(first + i - 2);
            dllArrayElement(first + i - 2);
            dllArrayElement(first + i - 1);
            dllArrayElement(first + i - 4);
            dllArrayElement(first + i - 1);
         }
      }
      else if(mode == GL_QUAD_STRIP)
      {
         if(count < 4)
            return;
         dllArrayElement(first + 0);
         dllArrayElement(first + 1);
         for(U32 i = 4; i <= count; i += 2)
         {
            dllArrayElement(first + i - 4);
            dllArrayElement(first + i - 2);

            dllArrayElement(first + i - 3);
            dllArrayElement(first + i - 1);

            dllArrayElement(first + i - 2);
            dllArrayElement(first + i - 1);
         }
      }
      dllEnd();
   }
}

static U32 getIndex(GLenum type, const void *indices, U32 i)
{
   if(type == GL_UNSIGNED_BYTE)
      return ((U8 *) indices)[i];
   else if(type == GL_UNSIGNED_SHORT)
      return ((U16 *) indices)[i];
   else
      return ((U32 *) indices)[i];
}

static BOOL WINAPI outlineSwapBuffers(HDC dc)
{
   bool ret = dllSwapBuffers(dc);
   dllClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   return ret;
}

static void APIENTRY outlineDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
   if(mode == GL_POLYGON)
      mode = GL_LINE_LOOP;

   if(mode == GL_POINTS || mode == GL_LINE_STRIP || mode == GL_LINE_LOOP || mode == GL_LINES)   
      dllDrawElements( mode, count, type, indices );
   else
   {
      dllBegin(GL_LINES);
      if(mode == GL_TRIANGLE_STRIP)
      {
         U32 i;
         for(i = 0; i < count - 1; i++) 
         {
            dllArrayElement(getIndex(type, indices, i));
            dllArrayElement(getIndex(type, indices, i + 1));
            if(i + 2 != count)
            {
               dllArrayElement(getIndex(type, indices, i));
               dllArrayElement(getIndex(type, indices, i + 2));
            }
         }
      }
      else if(mode == GL_TRIANGLE_FAN)
      {
         for(U32 i = 1; i < count; i ++)
         {
            dllArrayElement(getIndex(type, indices, 0));
            dllArrayElement(getIndex(type, indices, i));
            if(i != count - 1)
            {
               dllArrayElement(getIndex(type, indices, i));
               dllArrayElement(getIndex(type, indices, i + 1));
            }
         }
      }
      else if(mode == GL_TRIANGLES)
      {
         for(U32 i = 3; i <= count; i += 3)
         {
            dllArrayElement(getIndex(type, indices, i - 3));
            dllArrayElement(getIndex(type, indices, i - 2));
            dllArrayElement(getIndex(type, indices, i - 2));
            dllArrayElement(getIndex(type, indices, i - 1));
            dllArrayElement(getIndex(type, indices, i - 3));
            dllArrayElement(getIndex(type, indices, i - 1));
         }
      }
      else if(mode == GL_QUADS)
      {
         for(U32 i = 4; i <= count; i += 4)
         {
            dllArrayElement(getIndex(type, indices, i - 4));
            dllArrayElement(getIndex(type, indices, i - 3));
            dllArrayElement(getIndex(type, indices, i - 3));
            dllArrayElement(getIndex(type, indices, i - 2));
            dllArrayElement(getIndex(type, indices, i - 2));
            dllArrayElement(getIndex(type, indices, i - 1));
            dllArrayElement(getIndex(type, indices, i - 4));
            dllArrayElement(getIndex(type, indices, i - 1));
         }
      }
      else if(mode == GL_QUAD_STRIP)
      {
         if(count < 4)
            return;
         dllArrayElement(getIndex(type, indices, 0));
         dllArrayElement(getIndex(type, indices, 1));
         for(U32 i = 4; i <= count; i += 2)
         {
            dllArrayElement(getIndex(type, indices, i - 4));
            dllArrayElement(getIndex(type, indices, i - 2));

            dllArrayElement(getIndex(type, indices, i - 3));
            dllArrayElement(getIndex(type, indices, i - 1));

            dllArrayElement(getIndex(type, indices, i - 2));
            dllArrayElement(getIndex(type, indices, i - 1));
         }
      }
      dllEnd();
   }
}

ConsoleFunction(GLEnableOutline, void, 2, 2, "GLEnableOutline(bool);")
{
   argc;
   bool enable = dAtob(argv[1]);
   if(outlineEnabled == enable)
      return;
   
   if(enable && (loggingEnabled || perfEnabled))
      return;
   
   outlineEnabled = enable;

   if ( enable )
   {
      glDrawElements = outlineDrawElements;
      glDrawArrays = outlineDrawArrays;
      qwglSwapBuffers = outlineSwapBuffers;
   }
   else
   {
      glDrawElements = dllDrawElements;
      glDrawArrays = dllDrawArrays;
      qwglSwapBuffers = dllSwapBuffers;

   }
}

static void APIENTRY perfDrawArrays(GLenum mode, GLint first, GLsizei count)
{
   gGLState.primCount[gGLState.primMode]++;
   U32 tc = 0;

   if(mode == GL_TRIANGLES)
      tc = count / 3;
   else if(mode == GL_TRIANGLE_FAN || mode == GL_TRIANGLE_STRIP)
      tc = count - 2;

   gGLState.triCount[gGLState.primMode] += tc;
   dllDrawArrays( mode, first, count );
}

static void APIENTRY perfDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
   gGLState.primCount[gGLState.primMode]++;
   U32 tc = 0;

   if(mode == GL_TRIANGLES)
      tc = count / 3;
   else if(mode == GL_TRIANGLE_FAN || mode == GL_TRIANGLE_STRIP)
      tc = count - 2;

   gGLState.triCount[gGLState.primMode] += tc;
   dllDrawElements( mode, count, type, indices );
}

ConsoleFunction(GLEnableMetrics, void, 2, 2, "GLEnableMetrics(bool);")
{
   argc;
   static bool varsAdded = false;
   
   if(!varsAdded)
   {
      Con::addVariable("OpenGL::triCount0", TypeS32, &gGLState.triCount[0]);
      Con::addVariable("OpenGL::triCount1", TypeS32, &gGLState.triCount[1]);
      Con::addVariable("OpenGL::triCount2", TypeS32, &gGLState.triCount[2]);
      Con::addVariable("OpenGL::triCount3", TypeS32, &gGLState.triCount[3]);

      Con::addVariable("OpenGL::primCount0", TypeS32, &gGLState.primCount[0]);
      Con::addVariable("OpenGL::primCount1", TypeS32, &gGLState.primCount[1]);
      Con::addVariable("OpenGL::primCount2", TypeS32, &gGLState.primCount[2]);
      Con::addVariable("OpenGL::primCount3", TypeS32, &gGLState.primCount[3]);
      varsAdded = true;
   }
   
   bool enable = dAtob(argv[1]);
   if(perfEnabled == enable)
      return;
   
   if(enable && (loggingEnabled || outlineEnabled))
      return;
   
   perfEnabled = enable;

   if ( enable )
   {
      glDrawElements = perfDrawElements;
      glDrawArrays = perfDrawArrays;
   }
   else
   {
      glDrawElements = dllDrawElements;
      glDrawArrays = dllDrawArrays;
   }
}

#endif
