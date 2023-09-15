//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "dgl/dgl.h"
#include "gui/guiControl.h"
#include "console/consoleTypes.h"
#include "game/gameConnection.h"
#include "game/shapeBase.h"

//-----------------------------------------------------------------------------

/// Vary basic HUD clock.
/// Displays the current simulation time offset from some base. The base time
/// is usually synchronized with the server as mission start time.  This hud
/// currently only displays minutes:seconds.
class GuiClockHud : public GuiControl
{
   typedef GuiControl Parent;

   bool     mShowFrame;
   bool     mShowFill;

   ColorF   mFillColor;
   ColorF   mFrameColor;
   ColorF   mTextColor;

   S32      mTimeOffset;

public:
   GuiClockHud();

   void setTime(F32 newTime);
   F32  getTime();

   void onRender( Point2I, const RectI &);
   static void initPersistFields();
   DECLARE_CONOBJECT( GuiClockHud );
};


//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT( GuiClockHud );

GuiClockHud::GuiClockHud()
{
   mShowFrame = mShowFill = true;
   mFillColor.set(0, 0, 0, 0.5);
   mFrameColor.set(0, 1, 0, 1);
   mTextColor.set( 0, 1, 0, 1 );

   mTimeOffset = 0;
}

void GuiClockHud::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Misc");		// MM: Added Group Header.
   addField( "showFill", TypeBool, Offset( mShowFill, GuiClockHud ) );
   addField( "showFrame", TypeBool, Offset( mShowFrame, GuiClockHud ) );
   addField( "fillColor", TypeColorF, Offset( mFillColor, GuiClockHud ) );
   addField( "frameColor", TypeColorF, Offset( mFrameColor, GuiClockHud ) );
   addField( "textColor", TypeColorF, Offset( mTextColor, GuiClockHud ) );
   endGroup("Misc");		// MM: Added Group Footer.
}


//-----------------------------------------------------------------------------

void GuiClockHud::onRender(Point2I offset, const RectI &updateRect)
{
   // Background first
   if (mShowFill)
      dglDrawRectFill(updateRect, mFillColor);
   
   // Convert ms time into hours, minutes and seconds.
   S32 time = S32(getTime());
   S32 secs = time % 60;
   S32 mins = (time % 3600) / 60;
   S32 hours = time / 3600;

   // Currently only displays min/sec
   char buf[256];
   dSprintf(buf,sizeof(buf), "%02d:%02d",mins,secs);

   // Center the text
   offset.x += (mBounds.extent.x - mProfile->mFont->getStrWidth(buf)) / 2;
   offset.y += (mBounds.extent.y - mProfile->mFont->getHeight()) / 2;
   dglSetBitmapModulation(mTextColor);
   dglDrawText(mProfile->mFont, offset, buf);
   dglClearBitmapModulation();

   // Border last
   if (mShowFrame)
      dglDrawRect(updateRect, mFrameColor);
}


//-----------------------------------------------------------------------------

void GuiClockHud::setTime(F32 time)
{
   // Set the current time in seconds.
   mTimeOffset = S32(time * 1000) - Platform::getVirtualMilliseconds();
}

F32 GuiClockHud::getTime()
{
   // Return elapsed time in seconds.
   return F32(mTimeOffset + Platform::getVirtualMilliseconds()) / 1000;
}

ConsoleMethod(GuiClockHud,setTime,void,3, 3,"(time in sec)Sets the current base time for the clock")
{
   object->setTime(dAtof(argv[2]));
}

ConsoleMethod(GuiClockHud,getTime, F32, 2, 2,"()Returns current time in secs.")
{
   return object->getTime();
}
