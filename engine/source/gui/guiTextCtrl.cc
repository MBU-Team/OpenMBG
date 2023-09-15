//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "core/color.h"
#include "gui/guiTextCtrl.h"
#include "dgl/dgl.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

GuiTextCtrl::GuiTextCtrl()
{
   //default fonts
   mInitialText = StringTable->insert("");
   mText[0] = '\0';
   mMaxStrLen = GuiTextCtrl::MAX_STRING_LENGTH;
}

ConsoleMethod( GuiTextCtrl, setText, void, 3, 3, "obj.setText( newText )" )
{
   argc;
   object->setText( argv[2] );
}

void GuiTextCtrl::initPersistFields()
{
   Parent::initPersistFields();
   addField( "text",       TypeCaseString,  Offset( mInitialText, GuiTextCtrl ) );
   addField( "maxLength",  TypeS32,         Offset( mMaxStrLen, GuiTextCtrl ) );     
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

bool GuiTextCtrl::onAdd()
{
   if(!Parent::onAdd())
      return false;
   dStrncpy(mText, mInitialText, MAX_STRING_LENGTH);
   mText[MAX_STRING_LENGTH] = '\0';
   return true;
}

void GuiTextCtrl::inspectPostApply()
{
   Parent::inspectPostApply();
   setText(mInitialText);
}

bool GuiTextCtrl::onWake()
{
   if ( !Parent::onWake() )
      return false;
   
   mFont = mProfile->mFont;
   AssertFatal(mFont, "GuiTextCtrl::onWake: invalid font in profile" );

   if ( mConsoleVariable[0] )
   {
      const char *txt = Con::getVariable( mConsoleVariable );
      if ( txt )
      {
         if ( dStrlen( txt ) > mMaxStrLen )
         {
            char* buf = new char[mMaxStrLen + 1];
            dStrncpy( buf, txt, mMaxStrLen );
            buf[mMaxStrLen] = 0;
            setScriptValue( buf );
            delete [] buf;
         }
         else
            setScriptValue( txt );
      }
   }
   
   //resize
   if ( mProfile->mAutoSizeWidth )
   {
      if ( mProfile->mAutoSizeHeight )
         resize( mBounds.point, Point2I( mFont->getStrWidth( mText ), mFont->getHeight() + 4 ) );
      else
         resize( mBounds.point, Point2I( mFont->getStrWidth( mText ), mBounds.extent.y ) );
   }
   else if ( mProfile->mAutoSizeHeight )
      resize( mBounds.point, Point2I( mBounds.extent.x, mFont->getHeight() + 4 ) );

   return true;
}

void GuiTextCtrl::onSleep()
{
   Parent::onSleep();
   mFont = NULL;
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

void GuiTextCtrl::setText(const char *txt)
{
   //make sure we don't call this before onAdd();
   if( !mProfile )
      return;
   
   if (txt)
      dStrncpy(mText, txt, MAX_STRING_LENGTH);
   mText[MAX_STRING_LENGTH] = '\0';
   
   //Make sure we have a font
   mProfile->incRefCount();
   mFont = mProfile->mFont;
   
   //resize
   if (mProfile->mAutoSizeWidth)
   {
      if (mProfile->mAutoSizeHeight)
         resize(mBounds.point, Point2I(mFont->getStrWidth(mText), mFont->getHeight() + 4));
      else
         resize(mBounds.point, Point2I(mFont->getStrWidth(mText), mBounds.extent.y));
   }
   else if (mProfile->mAutoSizeHeight)
   {
      resize(mBounds.point, Point2I(mBounds.extent.x, mFont->getHeight() + 4));
   }
      
   setVariable(mText);
   setUpdate();
   
   //decrement the profile referrence
   mProfile->decRefCount();
} 

//------------------------------------------------------------------------------
void GuiTextCtrl::onPreRender()
{
   const char * var = getVariable();
   if(var && var[0] && dStricmp(mText, var))
      setText(var);
}

//------------------------------------------------------------------------------
void GuiTextCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   dglSetBitmapModulation( mProfile->mFontColor );
   renderJustifiedText(offset, mBounds.extent, mText);
   
   //render the child controls
   renderChildControls(offset, updateRect);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

const char *GuiTextCtrl::getScriptValue()
{
   return getText();
}

void GuiTextCtrl::setScriptValue(const char *val)
{
   setText(val);
}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// EOF //
