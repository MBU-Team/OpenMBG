//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "dgl/dgl.h"
#include "gui/guiCanvas.h"
#include "gui/guiCheckBoxCtrl.h"
#include "console/consoleTypes.h"

//--------------------------------------------------------------------------- 
GuiCheckBoxCtrl::GuiCheckBoxCtrl()
{
   mBounds.extent.set(140, 30);
	mStateOn = false;
   mButtonType = ButtonTypeCheck;
}

//--------------------------------------------------------------------------- 

bool GuiCheckBoxCtrl::onWake()
{
   if(!Parent::onWake())
      return false;
   
   // make sure there is a bitmap array for this control type
   // if it is declared as such in the control
   mProfile->constructBitmapArray(); 

   return true;
}

//--------------------------------------------------------------------------- 
void GuiCheckBoxCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   ColorI backColor = mActive ? mProfile->mFillColor : mProfile->mFillColorNA; 
   ColorI fontColor = mMouseOver ? mProfile->mFontColorHL : mProfile->mFontColor;
	ColorI insideBorderColor = isFirstResponder() ? mProfile->mBorderColorHL : mProfile->mBorderColor; 

   // just draw the check box and the text:
   S32 xOffset = 0;
	dglClearBitmapModulation();
   if(mProfile->mBitmapArrayRects.size() >= 4)
   {
      S32 index = mStateOn;
      if(!mActive)
         index = 2;
      else if(mDepressed)
         index += 2;
      xOffset = mProfile->mBitmapArrayRects[0].extent.x + 2;
      S32 y = (mBounds.extent.y - mProfile->mBitmapArrayRects[0].extent.y) / 2;
      dglDrawBitmapSR(mProfile->mTextureHandle, offset + Point2I(0, y), mProfile->mBitmapArrayRects[index]);
   }

	if(mButtonText[0] != NULL)
	{
	   dglSetBitmapModulation( fontColor );
      renderJustifiedText(Point2I(offset.x + xOffset, offset.y), 
                          Point2I(mBounds.extent.x - mBounds.extent.y, mBounds.extent.y),
                          mButtonText);
  	}
   //render the children
   renderChildControls(offset, updateRect);
}

//--------------------------------------------------------------------------- 
// EOF //


