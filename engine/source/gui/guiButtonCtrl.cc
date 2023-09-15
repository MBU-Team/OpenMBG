//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "dgl/dgl.h"
#include "console/consoleTypes.h"
#include "platform/platformAudio.h"
#include "gui/guiCanvas.h"
#include "gui/guiButtonCtrl.h"
#include "gui/guiDefaultControlRender.h"

GuiButtonCtrl::GuiButtonCtrl()
{
   mBounds.extent.set(140, 30);
   mButtonText = StringTable->insert("Button");
}

//--------------------------------------------------------------------------

void GuiButtonCtrl::onRender(Point2I      offset,
                             const RectI& updateRect)
{
   bool highlight = mMouseOver;
   bool depressed = mDepressed;

   ColorI fontColor   = mActive ? (highlight ? mProfile->mFontColorHL : mProfile->mFontColor) : mProfile->mFontColorNA;
   ColorI backColor   = mActive ? mProfile->mFillColor : mProfile->mFillColorNA; 
   ColorI borderColor = mActive ? mProfile->mBorderColor : mProfile->mBorderColorNA;

   RectI boundsRect(offset, mBounds.extent);

   if (mDepressed || mStateOn)
      renderLoweredBox(boundsRect, mProfile);
   else
      renderRaisedBox(boundsRect, mProfile);

   Point2I textPos = offset;
   if(depressed)
      textPos += Point2I(1,1);

   dglSetBitmapModulation( fontColor );
   renderJustifiedText(textPos, mBounds.extent, mButtonText);
   
   //render the children
   renderChildControls( offset, updateRect);
}

