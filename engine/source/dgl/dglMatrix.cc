//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "math/mMatrix.h"
#include "dgl/dgl.h"
#include "console/console.h"

void dglLoadMatrix(const MatrixF *m)
{
   //F32 mat[16];
   //m->transposeTo(mat);
   const_cast<MatrixF*>(m)->transpose();
   glLoadMatrixf(*m);
   const_cast<MatrixF*>(m)->transpose();
}

void dglMultMatrix(const MatrixF *m)
{
   //F32 mat[16];
   //m->transposeTo(mat);
//   const F32* mp = *m;
//   Con::errorf(ConsoleLogEntry::General, "Mult: %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g",
//      mp[0],
//      mp[1],
//      mp[2],
//      mp[3],
//      mp[4],
//      mp[5],
//      mp[6],
//      mp[7],
//      mp[8],
//      mp[9],
//      mp[10],
//      mp[11],
//      mp[12],
//      mp[13],
//      mp[14],
//      mp[15]);


   const_cast<MatrixF*>(m)->transpose();
   glMultMatrixf(*m);
   const_cast<MatrixF*>(m)->transpose();
}

void dglGetModelview(MatrixF *m)
{
   glGetFloatv(GL_MODELVIEW_MATRIX, *m);
   m->transpose();
}

void dglGetProjection(MatrixF *m)
{
   glGetFloatv(GL_PROJECTION_MATRIX, *m);
   m->transpose();
}

static F64 frustLeft = 0, frustRight = 1, frustBottom, frustTop, frustNear, frustFar;
static RectI viewPort;
static F32 pixelScale;
static F32 worldToScreenScale;
static bool isOrtho;

void dglSetFrustum(F64 left, F64 right, F64 bottom, F64 top, F64 nearPlane, F64 farPlane, bool ortho)
{
   // this converts from a coord system looking down the pos-y axis
   // to ogl's down neg z axis.
   // it's stored in OGL matrix form
   static F32 darkToOGLCoord[16] = { 1, 0,  0, 0,
                                     0, 0, -1, 0,
                                     0, 1,  0, 0,
                                     0, 0,  0, 1 };

   frustLeft = left;
   frustRight = right;
   frustBottom = bottom;
   frustTop = top;
   frustNear = nearPlane;
   frustFar = farPlane;
   isOrtho = ortho;
   if (ortho)
   {
      glOrtho(left, right, bottom, top, nearPlane, farPlane);
      worldToScreenScale = viewPort.extent.x / (frustRight - frustLeft);
   }
   else
   {
      glFrustum(left, right, bottom, top, nearPlane, farPlane);
      worldToScreenScale = (frustNear * viewPort.extent.x) / (frustRight - frustLeft);
   }
   glMultMatrixf(darkToOGLCoord);
}

void dglGetFrustum(F64 *left, F64 *right, F64 *bottom, F64 *top, F64 *nearPlane, F64 *farPlane)
{
   *left = frustLeft;
   *right = frustRight;
   *bottom = frustBottom;
   *top = frustTop;
   *nearPlane = frustNear;
   *farPlane = frustFar;
}

bool dglIsOrtho()
{
   return isOrtho;
}

void dglSetViewport(const RectI &aViewPort)
{
   viewPort = aViewPort;
   U32 screenHeight = Platform::getWindowSize().y;
   //glViewport(viewPort.point.x, viewPort.point.y + viewPort.extent.y,
   //           viewPort.extent.x, -viewPort.extent.y);

   glViewport(viewPort.point.x, screenHeight - (viewPort.point.y + viewPort.extent.y),
              viewPort.extent.x, viewPort.extent.y);
   pixelScale = viewPort.extent.x / 640.0;
   worldToScreenScale = (frustNear * viewPort.extent.x) / (frustRight - frustLeft);
}

void dglGetViewport(RectI* outViewport)
{
   AssertFatal(outViewport != NULL, "Error, bad point in GetViewport");
   *outViewport = viewPort;
}

F32 dglGetPixelScale()
{
   return pixelScale;
}

F32 dglGetWorldToScreenScale()
{
   return worldToScreenScale;
}

F32 dglProjectRadius(F32 dist, F32 radius)
{
   return (radius / dist) * worldToScreenScale;
}

