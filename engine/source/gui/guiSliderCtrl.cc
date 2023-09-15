//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "dgl/dgl.h"
#include "dgl/gTexManager.h"
#include "gui/guiSliderCtrl.h"
#include "platform/event.h"
#include "gui/guiDefaultControlRender.h"

//----------------------------------------------------------------------------
GuiSliderCtrl::GuiSliderCtrl(void)
{
	mActive = true;
   mRange.set( 0.0f, 1.0f );
   mTicks = 10;
   mValue = 0.5f;
   mThumbSize.set(8,20);
   mShiftPoint = 5;
   mShiftExtent = 10;
   mDisplayValue = false;
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::initPersistFields()
{
   Parent::initPersistFields();

   addGroup( "Slider" );
   addField("range", TypePoint2F,   Offset(mRange, GuiSliderCtrl));
   addField("ticks", TypeS32,       Offset(mTicks, GuiSliderCtrl));
   addField("value", TypeF32,       Offset(mValue, GuiSliderCtrl));
   endGroup( "Slider" );
}

//----------------------------------------------------------------------------
ConsoleMethod( GuiSliderCtrl, getValue, F32, 2, 2, "Get the position of the slider.")
{
   return object->getValue();
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::setScriptValue(const char *val)
{
	mValue = dAtof(val);
   updateThumb(mValue);
}

//----------------------------------------------------------------------------
bool GuiSliderCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
  
   if(mThumbSize.y + mProfile->mFont->getHeight()-4 <= mBounds.extent.y)
      mDisplayValue = true;
   else 
      mDisplayValue = false;

   updateThumb( mValue, true );
      
   return true;
}

//----------------------------------------------------------------------------
void GuiSliderCtrl::onMouseDown(const GuiEvent &event)
{
   if ( !mActive || !mAwake || !mVisible )
      return;

   mouseLock();
   setFirstResponder();
      
   Point2I curMousePos = globalToLocalCoord(event.mousePoint);
   F32 value;
   if (mBounds.extent.x >= mBounds.extent.y)
      value = F32(curMousePos.x-mShiftPoint) / F32(mBounds.extent.x-mShiftExtent)*(mRange.y-mRange.x) + mRange.x;
   else
      value = F32(curMousePos.y) / F32(mBounds.extent.y)*(mRange.y-mRange.x) + mRange.x;
   updateThumb(value);   
}   

//----------------------------------------------------------------------------
void GuiSliderCtrl::onMouseDragged(const GuiEvent &event)
{
   if ( !mActive || !mAwake || !mVisible )
      return;

   Point2I curMousePos = globalToLocalCoord(event.mousePoint);
   F32 value;
   if (mBounds.extent.x >= mBounds.extent.y)
      value = F32(curMousePos.x-mShiftPoint) / F32(mBounds.extent.x-mShiftExtent)*(mRange.y-mRange.x) + mRange.x;
   else
      value = F32(curMousePos.y) / F32(mBounds.extent.y)*(mRange.y-mRange.x) + mRange.x;

   if (value > mRange.y)
      value = mRange.y;
   else if (value < mRange.x)
      value = mRange.x;

   if ((event.modifier & SI_SHIFT) && mTicks > 2) {
      // If the shift key is held, snap to the nearest tick, if any are being drawn

      F32 tickStep = (mRange.y - mRange.x) / F32(mTicks + 1);

      F32 tickSteps = (value - mRange.x) / tickStep;
      S32 actualTick = S32(tickSteps + 0.5);

      value = actualTick * tickStep + mRange.x;
      AssertFatal(value <= mRange.y && value >= mRange.x, "Error, out of bounds value generated from shift-snap of slider");
   }

   updateThumb(value);   
}   

//----------------------------------------------------------------------------
void GuiSliderCtrl::onMouseUp(const GuiEvent &)
{
   if ( !mActive || !mAwake || !mVisible )
      return;

   mouseUnlock();
   if (mConsoleCommand[0])
   {
   	char buf[16];
      dSprintf(buf, sizeof(buf), "%d", getId());
      Con::setVariable("$ThisControl", buf);
      Con::evaluate(mConsoleCommand, false);
   }
}   

//----------------------------------------------------------------------------
void GuiSliderCtrl::updateThumb( F32 value, bool onWake )
{
   mValue = value;
   // clamp the thumb to legal values
   if (mValue < mRange.x)  mValue = mRange.x;
   if (mValue > mRange.y)  mValue = mRange.y;

   Point2I ext = mBounds.extent;
	ext.x -= ( mShiftExtent + mThumbSize.x ) / 2;
   // update the bounding thumb rect
   if (mBounds.extent.x >= mBounds.extent.y)
   {  // HORZ thumb
      S32 mx = (S32)((F32(ext.x) * (mValue-mRange.x) / (mRange.y-mRange.x)));
      S32 my = ext.y/2;
      if(mDisplayValue)
         my = mThumbSize.y/2;

      mThumb.point.x  = mx - (mThumbSize.x/2);
      mThumb.point.y  = my - (mThumbSize.y/2);
      mThumb.extent   = mThumbSize;
   }
   else
   {  // VERT thumb
      S32 mx = ext.x/2;
      S32 my = (S32)((F32(ext.y) * (mValue-mRange.x) / (mRange.y-mRange.x)));
      mThumb.point.x  = mx - (mThumbSize.y/2);
      mThumb.point.y  = my - (mThumbSize.x/2);
      mThumb.extent.x = mThumbSize.y;
      mThumb.extent.y = mThumbSize.x;
   }
   setFloatVariable(mValue);
   setUpdate();

   // Use the alt console command if you want to continually update:
   if ( !onWake && mAltConsoleCommand[0] )
      Con::evaluate( mAltConsoleCommand, false );
}   

//----------------------------------------------------------------------------
void GuiSliderCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   Point2I pos(offset.x+mShiftPoint, offset.y); 
   Point2I ext(mBounds.extent.x - mShiftExtent, mBounds.extent.y);

   if (mBounds.extent.x >= mBounds.extent.y)
   {
      Point2I mid(ext.x, ext.y/2);
      if(mDisplayValue)
         mid.set(ext.x, mThumbSize.y/2);
   
      glColor4f(0, 0, 0, 1);
      glBegin(GL_LINES);
         // horz rule
         glVertex2i(pos.x,       pos.y+mid.y);
         glVertex2i(pos.x+mid.x, pos.y+mid.y);
      
         // tick marks
         for (U32 t = 0; t <= (mTicks+1); t++)
         {
            S32 x = (S32)(F32(mid.x-1)/F32(mTicks+1)*F32(t));
            glVertex2i(pos.x+x, pos.y+mid.y-mShiftPoint);
            glVertex2i(pos.x+x, pos.y+mid.y+mShiftPoint);
         }
      glEnd();
   }
   else
   {
      Point2I mid(ext.x/2, ext.y);

      glColor4f(0, 0, 0, 1);
      glBegin(GL_LINES);
         // horz rule
         glVertex2i(pos.x+mid.x, pos.y);
         glVertex2i(pos.x+mid.x, pos.y+mid.y);
   
         // tick marks
         for (U32 t = 0; t <= (mTicks+1); t++)
         {
            S32 y = (S32)(F32(mid.y-1)/F32(mTicks+1)*F32(t));
            glVertex2i(pos.x+mid.x-mShiftPoint, pos.y+y);
            glVertex2i(pos.x+mid.x+mShiftPoint, pos.y+y);
         }
      glEnd();
      mDisplayValue = false;
   }
   // draw the thumb
   RectI thumb = mThumb;
   thumb.point += pos;
   renderRaisedBox(thumb, mProfile);

   if(mDisplayValue)
   {
   	char buf[20];
  		dSprintf(buf,sizeof(buf),"%0.3f",mValue);

   	Point2I textStart = thumb.point;

      S32 txt_w = mProfile->mFont->getStrWidth(buf);

   	textStart.x += (S32)((thumb.extent.x/2.0f));
   	textStart.y += thumb.extent.y - 2; //19
   	textStart.x -=	(txt_w/2);
   	if(textStart.x	< offset.x)
   		textStart.x = offset.x;
   	else if(textStart.x + txt_w > offset.x+mBounds.extent.x)
   		textStart.x -=((textStart.x + txt_w) - (offset.x+mBounds.extent.x));

    	dglSetBitmapModulation(mProfile->mFontColor);
    	dglDrawText(mProfile->mFont, textStart, buf, mProfile->mFontColors);
   }
   renderChildControls(offset, updateRect);
}

