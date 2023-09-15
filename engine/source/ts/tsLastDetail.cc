//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "ts/tsLastDetail.h"

#include "dgl/dgl.h"
#include "ts/tsShape.h"
#include "ts/tsShapeInstance.h"

Point2F TSLastDetail::smTVerts[4] = { Point2F(0,1), Point2F(1,1), Point2F(1,0), Point2F(0,0) };
Point3F TSLastDetail::smNorms[4] = { Point3F(0,-1,0), Point3F(0,-1,0), Point3F(0,-1,0), Point3F(0,-1,0) };


TSLastDetail::TSLastDetail(TSShape * shape,
                           U32 numEquatorSteps,
                           U32 numPolarSteps,
                           F32 polarAngle,
                           bool includePoles,
                           S32 dl, S32 dim)
{
   VECTOR_SET_ASSOCIATION(mBitmaps);
   VECTOR_SET_ASSOCIATION(mTextures);

   mNumEquatorSteps = numEquatorSteps;
   mNumPolarSteps = numPolarSteps;
   mPolarAngle = polarAngle;
   mIncludePoles = includePoles;

   F32 equatorStepSize = M_2PI_F / (F32) numEquatorSteps;
   F32 polarStepSize = numPolarSteps>0 ? (0.5f * M_PI_F - polarAngle) / (F32)numPolarSteps : 0.0f;

   U32 i;
   F32 rotZ = 0;
   for (i=0; i<numEquatorSteps; i++)
   {
      F32 rotX = numPolarSteps>0 ? polarAngle - 0.5f * M_PI_F : 0.0f;
      for (U32 j=0; j<2*numPolarSteps+1; j++)
      {
         MatrixF angMat;
         angMat.mul(MatrixF(EulerF(0,0,-M_PI_F+rotZ)),MatrixF(EulerF(rotX,0,0)));
         mBitmaps.push_back(TSShapeInstance::snapshot(shape,dim,dim,true,angMat,dl,1.0f,true));
         rotX += polarStepSize;
      }
      rotZ += equatorStepSize;
   }
   if (includePoles)
   {
      MatrixF m1( EulerF( M_PI_F / 2.0f, 0, 0 ) );
      MatrixF m2( EulerF( -M_PI_F / 2.0f, 0, 0 ) );
      mBitmaps.push_back(TSShapeInstance::snapshot(shape,dim,dim,true,m1,dl,1.0f,true));
      mBitmaps.push_back(TSShapeInstance::snapshot(shape,dim,dim,true,m2,dl,1.0f,true));
   }

   mTextures.setSize(mBitmaps.size());   
   for (i=0; i<(U32)mBitmaps.size(); i++)
   {
      if (mBitmaps[i]) // snapshot routine may refuse to give us a bitmap sometimes...
         mTextures[i] = new TextureHandle(NULL,mBitmaps[i],true);
      else
         mTextures[i] = NULL;
   }

   mPoints[0].set(-shape->radius,0, shape->radius);
   mPoints[1].set( shape->radius,0, shape->radius);
   mPoints[2].set( shape->radius,0,-shape->radius);
   mPoints[3].set(-shape->radius,0,-shape->radius);

   mCenter = shape->center;
}

TSLastDetail::~TSLastDetail()
{
   for (S32 i=0; i<mTextures.size(); i++) {
      delete mTextures[i];
      mTextures[i] = NULL;
   }

   // mBitmaps...owned by mTextures
}

void TSLastDetail::chooseView(const MatrixF & mat, const Point3F & scale)
{
   scale; // ignore for now

// Following is called often enough that it's worth trying to optimize
/* 
   Point3F x,y,z,pos;
   mat.getColumn(0,&x);
   mat.getColumn(1,&y);
   mat.getColumn(2,&z);
   mat.getColumn(3,&pos);

   F32 dotX = mDot(x,pos);
   F32 dotY = mDot(y,pos);
   F32 dotZ = mDot(z,pos);
*/

   const F32 * m = (const F32*)&mat;
   F32 dotX = m[3] * m[0];
   F32 dotY = m[3] * m[1];
   F32 dotZ = m[3] * m[2];
   dotX    += m[7] * m[4];
   dotY    += m[7] * m[5];
   dotZ    += m[7] * m[6];
   dotX    += m[11] * m[8];
   dotY    += m[11] * m[9];
   dotZ    += m[11] * m[10];

   // which bmp to choose?
   F32 rotX = mIncludePoles || mNumPolarSteps ? mAcos(dotZ/mSqrt(dotX*dotX+dotY*dotY+dotZ*dotZ)) : 0.0f;
   F32 rotZ = 0.0f;
   AssertFatal(rotX>=0 && rotX<M_2PI,"TSLastDetail::chooseView: rotX out of range");
   if (mIncludePoles && (rotX<mPolarAngle || rotX>M_PI-mPolarAngle))
   {
      mBitmapIndex = mNumEquatorSteps * (2*mNumPolarSteps+1);
      if (rotX>mPolarAngle)
         mBitmapIndex++;
//      mRotY = mAtan(y.y,x.y);
      mRotY = mAtan(m[5],m[4]);
   }
   else
   {
      F32 equatorStepSize = M_2PI_F / (F32) mNumEquatorSteps;
      F32 polarStepSize = mNumPolarSteps>0 ? (0.5f * M_PI_F - mPolarAngle) / (F32) mNumPolarSteps : 0.0f;
      rotZ = 0.999f * (mAtan(dotX,dotY) + M_PI_F); // the 0.99f makes sure we are in range
      AssertFatal(rotZ>=0 && rotZ<M_2PI_F,"TSLastDetail::chooseView: rotZ out of range");
      mBitmapIndex = ((S32)(rotZ/equatorStepSize)) * (S32)(2*mNumPolarSteps+1);
      if (mNumPolarSteps>0)
         mBitmapIndex = (U32)(mBitmapIndex + ((rotX-mPolarAngle) / polarStepSize));
//      mRotY = mAtan(z.x,z.z);
      mRotY = mAtan(m[2],m[10]);
   }
   
   // make sure we don´t get invalid bitmap index!
   mBitmapIndex = mClamp(mBitmapIndex, 0, mTextures.size()-1);
}

void TSLastDetail::render(F32 alpha, bool drawFog)
{
   glPushMatrix();

   // get camera matrix, adjust for shape center
   MatrixF mat;
   Point3F p,center;
   dglGetModelview(&mat);
   mat.getColumn(3,&p);
   mat.mulV(mCenter,&center);
   p += center;
   mat.setColumn(3,p);

   Point3F ones( 1, 1, 1 );
   chooseView(mat,ones);

   // following is a quicker version of mat.set(EulerF(0,rotY,0));
   // note:  we assume mat[12]=1 and mat[3]=mat[7]=mat[11]=0 to start with
   F32 * m = (F32*)mat;  // because [] operator isn't implemented on MatrixF, so it finds mat[0] ambiguous (const)
   AssertFatal(mFabs(m[15]-1.0f)<0.01f && mFabs(m[14])<0.01f && mFabs(m[13])<0.01f && mFabs(m[12])<0.01f,"TSLastDetail::render");
   if (mRotY*mRotY>0.0001f)
   {
      m[0] =  m[10] = mCos(mRotY);
      m[2] = mSin(mRotY); m[8] = -m[2];
      m[1] =  m[4] = m[6] = m[9] = 0.0f;
      m[5] =  1.0f;
   }
   else
   {
      m[0] = m[5] = m[10] = 1.0f;
      m[1] = m[2] = m[4] = m[6] = m[8] = m[9] = 0.0f;
   }
   
   dglLoadMatrix(&mat);
   if (TSShapeInstance::smRenderData.objectScale)
      glScalef(TSShapeInstance::smRenderData.objectScale->x,TSShapeInstance::smRenderData.objectScale->y,TSShapeInstance::smRenderData.objectScale->z);

   if (!drawFog)
      renderNoFog(alpha);
   else
   {
      if (dglDoesSupportTextureEnvCombine() && dglDoesSupportARBMultitexture())
         renderFog_MultiCombine(alpha);
//      else if (dglDoesSupportTextureEnvCombine())
//         renderFog_Combine(alpha);
      else
         renderNoFog(alpha * TSShapeInstance::smRenderData.fogColor.w);
   }

   glPopMatrix();
}

void TSLastDetail::renderNoFog(F32 alpha)
{
   // set up arrays
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(3,GL_FLOAT,0,mPoints);
   glTexCoordPointer(2,GL_FLOAT,0,smTVerts);
   glNormalPointer(GL_FLOAT,0,smNorms);

   glDepthMask(GL_FALSE);

   // light the material
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,Point4F(1,1,1,alpha));
   
   // additive transparency
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   // texture
   glEnable(GL_TEXTURE_2D);

   if (TSShapeInstance::smRenderData.useOverride == false)
      glBindTexture(GL_TEXTURE_2D, mTextures[mBitmapIndex]->getGLName());
   else
      glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.override.getGLName());

   // set color state
   glColor4f(1,1,1,1); 

   glDrawArrays(GL_TRIANGLE_FAN,0,4);
   
   glDisableClientState(GL_NORMAL_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);

   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
}

void TSLastDetail::renderFog_Combine(F32 alpha)
{
   // set up arrays
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(3,GL_FLOAT,0,mPoints);
   glTexCoordPointer(2,GL_FLOAT,0,smTVerts);
   glNormalPointer(GL_FLOAT,0,smNorms);

   glDepthMask(GL_FALSE);

   // light the material
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,Point4F(1,1,1,alpha));
   glColor4f(1,1,1,1); 
   
   // additive transparency
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

   // texture
   glEnable(GL_TEXTURE_2D);
   if (TSShapeInstance::smRenderData.useOverride == false)
      glBindTexture(GL_TEXTURE_2D, mTextures[mBitmapIndex]->getGLName());
   else
      glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.override.getGLName());

   glDrawArrays(GL_TRIANGLE_FAN,0,4);
   
   // now render the fog

   glDisableClientState(GL_NORMAL_ARRAY);

   GLboolean wasLit = glIsEnabled(GL_LIGHTING);
   glDisable(GL_LIGHTING);
   glDepthMask(GL_TRUE);

   glColor4fv(TSShapeInstance::smRenderData.fogColor);

   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
   // color comes from constant color...
   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_REPLACE);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_PRIMARY_COLOR_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
   // alpha is product of texture and constant alpha...
   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_MODULATE);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PRIMARY_COLOR_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_EXT,GL_TEXTURE);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_EXT,GL_SRC_ALPHA);

   glDrawArrays(GL_TRIANGLE_FAN,0,4);

   // restore state...
   if (wasLit)
      glEnable(GL_LIGHTING);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ZERO);
}

void TSLastDetail::renderFog_MultiCombine(F32 alpha)
{
   // set up materials
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glVertexPointer(3,GL_FLOAT,0,mPoints);
   glTexCoordPointer(2,GL_FLOAT,0,smTVerts);
   glNormalPointer(GL_FLOAT,0,smNorms);
   
   // set color state
   glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,Point4F(1,1,1,alpha));
   glColor4f(1,1,1,1); 

   // first TE applies lighting to billboard texture
   glEnable(GL_TEXTURE_2D);
   if (TSShapeInstance::smRenderData.useOverride == false)
      glBindTexture(GL_TEXTURE_2D, mTextures[mBitmapIndex]->getGLName());
   else
      glBindTexture(GL_TEXTURE_2D, TSShapeInstance::smRenderData.override.getGLName());
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
   
   // second TE applies fog to the result
   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable(GL_TEXTURE_2D);
   glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,TSShapeInstance::smRenderData.fogColor);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_EXT,GL_INTERPOLATE_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_EXT,GL_CONSTANT_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_EXT,GL_SRC_COLOR);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_EXT,GL_PREVIOUS_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_EXT,GL_SRC_COLOR);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_EXT,GL_CONSTANT_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_EXT,GL_SRC_ALPHA);
   glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_EXT,GL_REPLACE);
   glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_EXT,GL_PREVIOUS_EXT);
   glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_EXT,GL_SRC_ALPHA);

   // additive transparency
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_FALSE);

   // render billboard+fog
   glDrawArrays(GL_TRIANGLE_FAN,0,4);
   
   // restore state...
   glDisable(GL_TEXTURE_2D);
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glDisableClientState(GL_NORMAL_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);
   glDisable(GL_TEXTURE_2D);
}




