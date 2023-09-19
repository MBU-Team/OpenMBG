//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/guiTSControl.h"
#include "console/consoleTypes.h"
#include "sceneGraph/sceneLighting.h"

IMPLEMENT_CONOBJECT(GuiTSCtrl);

U32 GuiTSCtrl::smFrameCount = 0;

GuiTSCtrl::GuiTSCtrl()
{
   mCameraZRot = 0;
   mForceFOV = 0;

   for(U32 i = 0; i < 16; i++)
   {
      mSaveModelview[i] = 0;
      mSaveProjection[i] = 0;
   }
   mSaveModelview[0] = 1;
   mSaveModelview[5] = 1;
   mSaveModelview[10] = 1;
   mSaveModelview[15] = 1;
   mSaveProjection[0] = 1;
   mSaveProjection[5] = 1;
   mSaveProjection[10] = 1;
   mSaveProjection[15] = 1;
   
   mSaveViewport[0] = 0;
   mSaveViewport[1] = 0;
   mSaveViewport[2] = 2;
   mSaveViewport[3] = 2;
}

void GuiTSCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addField("cameraZRot", TypeF32, Offset(mCameraZRot, GuiTSCtrl));
   addField("forceFOV",   TypeF32, Offset(mForceFOV,   GuiTSCtrl));
}

void GuiTSCtrl::consoleInit()
{
   Con::addVariable("$TSControl::frameCount", TypeS32, &smFrameCount);
}

void GuiTSCtrl::onPreRender()
{
   setUpdate();
}

bool GuiTSCtrl::processCameraQuery(CameraQuery *)
{
   return false;
}

void GuiTSCtrl::renderWorld(const RectI& /*updateRect*/)
{
}

bool GuiTSCtrl::project(const Point3F &pt, Point3F *dest)
{
   GLdouble winx, winy, winz;
   GLint result = gluProject(pt.x, pt.y, pt.z, 
                     mSaveModelview, mSaveProjection, mSaveViewport,
                     &winx, &winy, &winz);
   if(result == GL_FALSE || winz < 0 || winz > 1)
      return false;
   dest->set(winx, winy, winz);
   return true;
}

bool GuiTSCtrl::unproject(const Point3F &pt, Point3F *dest)
{
   GLdouble objx, objy, objz;
   GLint result = gluUnProject(pt.x, pt.y, pt.z, 
                     mSaveModelview, mSaveProjection, mSaveViewport,
                     &objx, &objy, &objz);
   if(result == GL_FALSE)
      return false;
   dest->set(objx, objy, objz);
   return true;
}

void GuiTSCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   //if(SceneLighting::isLighting())
   //   return;

   if(!processCameraQuery(&mLastCameraQuery))
      return;

   if(mForceFOV != 0)
      mLastCameraQuery.fov = mDegToRad(mForceFOV);
   
   if(mCameraZRot)
   {
      MatrixF rotMat(EulerF(0, 0, mDegToRad(mCameraZRot)));
      mLastCameraQuery.cameraMatrix.mul(rotMat);
   }

   // set up the camera and viewport stuff:
   F32 wwidth = mLastCameraQuery.nearPlane * mTan(mLastCameraQuery.fov / 2);
   F32 wheight = F32(mBounds.extent.y) / F32(mBounds.extent.x) * wwidth;

   F32 hscale = wwidth * 2 / F32(mBounds.extent.x);
   F32 vscale = wheight * 2 / F32(mBounds.extent.y);
   
   F32 left = (updateRect.point.x - offset.x) * hscale - wwidth;
   F32 right = (updateRect.point.x + updateRect.extent.x - offset.x) * hscale - wwidth;
   F32 top = wheight - vscale * (updateRect.point.y - offset.y);
   F32 bottom = wheight - vscale * (updateRect.point.y + updateRect.extent.y - offset.y);

   dglSetViewport(updateRect);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   dglSetFrustum(left, right, bottom, top, mLastCameraQuery.nearPlane, mLastCameraQuery.farPlane);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   //Point3F refPt = query.position + query.viewVector;

   mLastCameraQuery.cameraMatrix.inverse();
   dglMultMatrix(&mLastCameraQuery.cameraMatrix);
   
   glGetDoublev(GL_PROJECTION_MATRIX, mSaveProjection);
   glGetDoublev(GL_MODELVIEW_MATRIX, mSaveModelview);
   //glGetIntegerv(GL_VIEWPORT, mSaveViewport);
   
   //U32 screenHeight = Platform::getWindowSize().y;
   mSaveViewport[0] = updateRect.point.x;
   mSaveViewport[1] = updateRect.point.y + updateRect.extent.y;
   mSaveViewport[2] = updateRect.extent.x;
   mSaveViewport[3] = -updateRect.extent.y;
   

   //gluLookAt(query.position.x, query.position.y, query.position.z,
   //          refPt.x, refPt.y, refPt.z,
   //          query.upVector.x, query.upVector.y, query.upVector.z);

   renderWorld(updateRect);
   renderChildControls(offset, updateRect);
   smFrameCount++;
}
