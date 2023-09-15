//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIBITMAPCTRL_H_
#define _GUIBITMAPCTRL_H_

#ifndef _GUICONTROL_H_
#include "gui/guiControl.h"
#endif
#ifndef _GTEXMANAGER_H_
#include "dgl/gTexManager.h"
#endif

/// Renders a bitmap.
class GuiBitmapCtrl : public GuiControl
{
private:
   typedef GuiControl Parent;

protected:
   StringTableEntry mBitmapName;
   TextureHandle mTextureHandle;
   Point2I startPoint;
   bool mWrap;
   
public:
   //creation methods
   DECLARE_CONOBJECT(GuiBitmapCtrl);
   GuiBitmapCtrl();
   static void initPersistFields();

   //Parental methods
   bool onWake();
   void onSleep();
   void inspectPostApply();

   void setBitmap(const char *name,bool resize = false);
   void setBitmap(const TextureHandle &handle,bool resize = false);

   S32 getWidth() const       { return(mTextureHandle.getWidth()); }
   S32 getHeight() const      { return(mTextureHandle.getHeight()); }

   void onRender(Point2I offset, const RectI &updateRect);
   void setValue(S32 x, S32 y);
};

#endif
