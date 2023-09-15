//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------


//-------------------------------------
//
// Bitmap Button Contrl
// Set 'bitmap' comsole field to base name of bitmaps to use.  This control will 
// append '_n' for normal
// append '_h' for hilighted
// append '_d' for depressed
//
// if bitmap cannot be found it will use the default bitmap to render.
//
// if the extent is set to (0,0) in the gui editor and appy hit, this control will
// set it's extent to be exactly the size of the normal bitmap (if present)
//


#include "console/console.h"
#include "dgl/dgl.h"
#include "console/consoleTypes.h"
#include "platform/platformAudio.h"
#include "gui/guiCanvas.h"
#include "gui/guiBitmapButtonCtrl.h"
#include "gui/guiDefaultControlRender.h"


//-------------------------------------
GuiBitmapButtonCtrl::GuiBitmapButtonCtrl()
{
   mBitmapName = StringTable->insert("");
   mBounds.extent.set(140, 30);
}


//-------------------------------------
void GuiBitmapButtonCtrl::initPersistFields()
{
   Parent::initPersistFields();
   addField("bitmap", TypeFilename, Offset(mBitmapName, GuiBitmapButtonCtrl));
}


//-------------------------------------
bool GuiBitmapButtonCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
   setActive(true);
   setBitmap(mBitmapName);
   return true;
}


//-------------------------------------
void GuiBitmapButtonCtrl::onSleep()
{
   mTextureNormal = NULL;
   mTextureHilight = NULL;
   mTextureDepressed = NULL;
   Parent::onSleep();
}


//-------------------------------------

ConsoleMethod( GuiBitmapButtonCtrl, setBitmap, void, 3, 3, "(filepath name)")
{
   object->setBitmap(argv[2]);
}

//-------------------------------------
void GuiBitmapButtonCtrl::inspectPostApply()
{
   // if the extent is set to (0,0) in the gui editor and appy hit, this control will
   // set it's extent to be exactly the size of the normal bitmap (if present)
   Parent::inspectPostApply();

   if ((mBounds.extent.x == 0) && (mBounds.extent.y == 0) && mTextureNormal)
   {
      TextureObject *texture = (TextureObject *) mTextureNormal;
      mBounds.extent.x = texture->bitmapWidth;
      mBounds.extent.y = texture->bitmapHeight;
   }
}


//-------------------------------------
void GuiBitmapButtonCtrl::setBitmap(const char *name)
{
   mBitmapName = StringTable->insert(name);
   if(!isAwake())
      return;

   if (*mBitmapName)
   {
      char buffer[1024];
      char *p;
      dStrcpy(buffer, name);
      p = buffer + dStrlen(buffer);
   
      dStrcpy(p, "_n");
      mTextureNormal = TextureHandle(buffer, BitmapTexture, true);
      dStrcpy(p, "_h");
      mTextureHilight = TextureHandle(buffer, BitmapTexture, true);
      if (!mTextureHilight)
         mTextureHilight = mTextureNormal;
      dStrcpy(p, "_d");
      mTextureDepressed = TextureHandle(buffer, BitmapTexture, true);
      if (!mTextureDepressed)
         mTextureDepressed = mTextureHilight;
      dStrcpy(p, "_i");
      mTextureInactive = TextureHandle(buffer, BitmapTexture, true);
      if (!mTextureInactive)
         mTextureInactive = mTextureNormal;
   }
   else
   {
      mTextureNormal = NULL;
      mTextureHilight = NULL;
      mTextureDepressed = NULL;
      mTextureInactive = NULL;
   }
   setUpdate();
}   


//-------------------------------------
void GuiBitmapButtonCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   enum { 
      NORMAL, 
      HILIGHT, 
      DEPRESSED,
      INACTIVE
   } state = NORMAL;

   if (mActive)
   {
      if (mMouseOver) state = HILIGHT;
      if (mDepressed || mStateOn) state = DEPRESSED;
   }
   else
      state = INACTIVE;

   switch (state)
   {
      case NORMAL:      renderButton(mTextureNormal, offset, updateRect); break;
      case HILIGHT:     renderButton(mTextureHilight ? mTextureHilight : mTextureNormal, offset, updateRect); break;
      case DEPRESSED:   renderButton(mTextureDepressed, offset, updateRect); break;
      case INACTIVE:    renderButton(mTextureInactive ? mTextureInactive : mTextureNormal, offset, updateRect); break;
   }
}


//-------------------------------------
void GuiBitmapButtonCtrl::renderButton(TextureHandle &texture, Point2I &offset, const RectI& updateRect)
{
   if (texture)
   {
      RectI rect(offset, mBounds.extent);
      dglClearBitmapModulation();
      dglDrawBitmapStretch(texture, rect);
      renderChildControls( offset, updateRect);
   }
   else
      Parent::onRender(offset, updateRect);
}

void GuiBitmapButtonTextCtrl::onRender(Point2I offset, const RectI& updateRect)
{
   enum { 
      NORMAL, 
      HILIGHT, 
      DEPRESSED,
      INACTIVE
   } state = NORMAL;

   if (mActive)
   {
      if (mMouseOver) state = HILIGHT;
      if (mDepressed || mStateOn) state = DEPRESSED;
   }
   else
      state = INACTIVE;
   TextureHandle texture;

   switch (state)
   {
      case NORMAL: 
         texture = mTextureNormal;
         break;
      case HILIGHT: 
         texture = mTextureHilight;
         break;
      case DEPRESSED: 
         texture = mTextureDepressed;
         break;
      case INACTIVE:
         texture = mTextureInactive;
         if(!texture)
            texture = mTextureNormal;
         break;
   }
   if (texture)
   {
      RectI rect(offset, mBounds.extent);
      dglClearBitmapModulation();
      dglDrawBitmapStretch(texture, rect);

      Point2I textPos = offset;
      if(mDepressed)
         textPos += Point2I(1,1);
      dglSetBitmapModulation( mProfile->mFontColor );
      renderJustifiedText(textPos, mBounds.extent, mButtonText);

      renderChildControls( offset, updateRect);
   }
   else
      Parent::onRender(offset, updateRect);
}
