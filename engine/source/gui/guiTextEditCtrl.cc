//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "dgl/dgl.h"
#include "gui/guiCanvas.h"
#include "gui/guiMLTextCtrl.h"
#include "gui/guiTextEditCtrl.h"
#include "gui/guiDefaultControlRender.h"

U32 GuiTextEditCtrl::mNumAwake = 0;

GuiTextEditCtrl::GuiTextEditCtrl()
{
   mInsertOn = true;
   mBlockStart = 0;
   mBlockEnd = 0;
   mCursorPos = 0;

   mDragHit = false;
   mTabComplete = false;
   mScrollDir = 0;
   
   mUndoText[0] = '\0';
   mUndoBlockStart = 0;
   mUndoBlockEnd = 0;
   mUndoCursorPos = 0;
   mPasswordText = false;

   mSinkAllKeyEvents = false;
   
   mActive = true;
   
   mTextOffset.x = 65535;

   mHistoryDirty = false;
   mHistorySize = 0; 
   mHistoryLast = -1;  
   mHistoryIndex = 0;  
   mHistoryBuf = NULL;
   
   mValidateCommand = StringTable->insert("");
   mEscapeCommand = StringTable->insert( "" );
   mDeniedSound = dynamic_cast<AudioProfile*>( Sim::findObject( "InputDeniedSound" ) );
}

GuiTextEditCtrl::~GuiTextEditCtrl()
{
   //delete the history buffer if it exists
   if (mHistoryBuf)
   {
      for (S32 i = 0; i < mHistorySize; i++)
         delete [] mHistoryBuf[i];
         
      delete [] mHistoryBuf;
   }
}

void GuiTextEditCtrl::initPersistFields()
{
   Parent::initPersistFields();
   
   addField("validate",          TypeString,    Offset(mValidateCommand, GuiTextEditCtrl));
   addField("escapeCommand",     TypeString,    Offset(mEscapeCommand, GuiTextEditCtrl));
   addField("historySize",       TypeS32,       Offset(mHistorySize, GuiTextEditCtrl));
   addField("password",          TypeBool,      Offset(mPasswordText, GuiTextEditCtrl));     
   addField("tabComplete",       TypeBool,      Offset(mTabComplete, GuiTextEditCtrl));     
   addField("deniedSound",       TypeAudioProfilePtr, Offset(mDeniedSound, GuiTextEditCtrl));
   addField("sinkAllKeyEvents",  TypeBool,      Offset(mSinkAllKeyEvents, GuiTextEditCtrl));
}

bool GuiTextEditCtrl::onAdd()
{
   if ( ! Parent::onAdd() )
      return false;
      
   //create the history buffer
   if ( mHistorySize > 0 )
   {
      mHistoryBuf = new char*[mHistorySize];
      for ( S32 i = 0; i < mHistorySize; i++ )
      {
         mHistoryBuf[i] = new char[GuiTextCtrl::MAX_STRING_LENGTH + 1];
         mHistoryBuf[i][0] = '\0';
      }
   }
   return true;
}

bool GuiTextEditCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
 
   // If this is the first awake text edit control, enable keyboard translation
   if (mNumAwake == 0)
      Platform::enableKeyboardTranslation();
   ++mNumAwake;

   return true;
}

void GuiTextEditCtrl::onSleep()
{
   Parent::onSleep();

   // If this is the last awake text edit control, disable keyboard translation
   --mNumAwake;
   if (mNumAwake == 0)
      Platform::disableKeyboardTranslation();
}

void GuiTextEditCtrl::updateHistory( const char *txt, bool moveIndex )
{
   if(!txt)
      txt = "";
   if(!mHistorySize)
      return;
   
   // see if it's already in
   if(mHistoryLast == -1 || dStrcmp(txt, mHistoryBuf[mHistoryLast]))
   {
      if(mHistoryLast == mHistorySize-1) // we're at the history limit... shuffle the pointers around:
      {
         char *first = mHistoryBuf[0];
         for(U32 i = 0; i < mHistorySize - 1; i++)
            mHistoryBuf[i] = mHistoryBuf[i+1];
         mHistoryBuf[mHistorySize-1] = first;
         if(mHistoryIndex > 0)
            mHistoryIndex--;
      }
      else
         mHistoryLast++;
      
      dStrncpy( mHistoryBuf[mHistoryLast], txt, GuiTextCtrl::MAX_STRING_LENGTH );
      mHistoryBuf[mHistoryLast][GuiTextCtrl::MAX_STRING_LENGTH] = '\0';
   }
   if(moveIndex)
      mHistoryIndex = mHistoryLast + 1;
}   

void GuiTextEditCtrl::getText( char *dest )
{
   if ( dest )
      dStrcpy( dest, mText );   
}  
 
void GuiTextEditCtrl::setText( const char *txt )
{
   Parent::setText( txt );
   mCursorPos = dStrlen( mText );
}

void GuiTextEditCtrl::reallySetCursorPos( const S32 newPos )
{
   S32 charCount = dStrlen( mText );
   S32 realPos = newPos > charCount ? charCount : newPos < 0 ? 0 : newPos;
   if ( realPos != mCursorPos )
   {
      mCursorPos = realPos;
      setUpdate();
   }
}

S32 GuiTextEditCtrl::setCursorPos( const Point2I &offset )
{
   Point2I ctrlOffset = localToGlobalCoord( Point2I( 0, 0 ) );
   S32 charCount = dStrlen( mText );
   S32 charLength = 0;
   S32 curX;

   curX = offset.x - ctrlOffset.x;
   setUpdate();

   //if the cursor is too far to the left
   if ( curX < 0 )
      return -1;

   //if the cursor is too far to the right
   if ( curX >= ctrlOffset.x + mBounds.extent.x )
      return -2;

   curX = offset.x - mTextOffset.x;
   S32 pos = 0;
   while ( pos < charCount )
   { 
      charLength += mFont->getCharXIncrement( mText[pos] );
      if ( charLength > curX )
         break;
      pos++;
   }

   return pos;
}

void GuiTextEditCtrl::onMouseDown( const GuiEvent &event )
{
   mDragHit = false;

   //undo any block function
   mBlockStart = 0;
   mBlockEnd = 0;
   
   //find out where the cursor should be
   S32 pos = setCursorPos( event.mousePoint );

   // if the position is to the left
   if ( pos == -1 ) 
      mCursorPos = 0;   
   else if ( pos == -2 ) //else if the position is to the right
      mCursorPos = dStrlen(mText);
   else //else set the mCursorPos
      mCursorPos = pos;

   //save the mouseDragPos
   mMouseDragStart = mCursorPos;

   // lock the mouse
   mouseLock();

   //set the drag var
   mDragHit = true;
   
   //let the parent get the event
   setFirstResponder();
}

void GuiTextEditCtrl::onMouseDragged( const GuiEvent &event )
{
   S32 pos = setCursorPos( event.mousePoint );
    
   // if the position is to the left
   if ( pos == -1 )
      mScrollDir = -1;    
   else if ( pos == -2 ) // the position is to the right
      mScrollDir = 1;    
   else // set the new cursor position
   {
      mScrollDir = 0;
      mCursorPos = pos;
   }
    
   // update the block:
   mBlockStart = getMin( mCursorPos, mMouseDragStart );
   mBlockEnd = getMax( mCursorPos, mMouseDragStart );
   if ( mBlockStart < 0 ) 
      mBlockStart = 0;
    
   if ( mBlockStart == mBlockEnd )
      mBlockStart = mBlockEnd = 0;
    
   //let the parent get the event
   Parent::onMouseDragged(event);
}

void GuiTextEditCtrl::onMouseUp(const GuiEvent &event)
{
   event;
   mDragHit = false;
   mScrollDir = 0;
   mouseUnlock();
}

void GuiTextEditCtrl::saveUndoState()
{
    //save the current state
    dStrcpy(mUndoText, mText);
    mUndoBlockStart = mBlockStart;
    mUndoBlockEnd = mBlockEnd;
    mUndoCursorPos = mCursorPos;
}

bool GuiTextEditCtrl::onKeyDown(const GuiEvent &event)
{
   S32 stringLen = dStrlen(mText);
   setUpdate();
   
    if (event.modifier & SI_SHIFT)
    {
        switch (event.keyCode)
        {
            case KEY_TAB:
               if ( mTabComplete )
               {
                  Con::executef( this, 2, "onTabComplete", "1" );
                  return( true );
               }

            case KEY_HOME:
               mBlockStart = 0;
               mBlockEnd = mCursorPos;
               mCursorPos = 0;
               return true;
                
            case KEY_END:
                mBlockStart = mCursorPos;
                mBlockEnd = stringLen;
                mCursorPos = stringLen;
                return true;
            
            case KEY_LEFT:
                if ((mCursorPos > 0) & (stringLen > 0))
                {
                    //if we already have a selected block
                    if (mCursorPos == mBlockEnd)
                    {
                        mCursorPos--;
                        mBlockEnd--;
                        if (mBlockEnd == mBlockStart)
                        {
                            mBlockStart = 0;
                            mBlockEnd = 0;
                        }
                    }
                    else {
                        mCursorPos--;
                        mBlockStart = mCursorPos;
                        
                        if (mBlockEnd == 0)
                        {
                            mBlockEnd = mCursorPos + 1;
                        }
                    }
                }
                return true;
                
            case KEY_RIGHT:
                if (mCursorPos < stringLen)
                {
                    if ((mCursorPos == mBlockStart) && (mBlockEnd > 0))
                    {
                        mCursorPos++;
                        mBlockStart++;
                        if (mBlockStart == mBlockEnd)
                        {
                            mBlockStart = 0;
                            mBlockEnd = 0;
                        }
                    }
                    else
                    {
                        if (mBlockEnd == 0)
                        {
                            mBlockStart = mCursorPos;
                            mBlockEnd = mCursorPos;
                        }
                        mCursorPos++;
                        mBlockEnd++;
                    }
                }
                return true;
        }
    }
   else if (event.modifier & SI_CTRL)
   {
      switch(event.keyCode)
      {
         // Added UNIX emacs key bindings - just a little hack here...

         // Ctrl-B - move one character back
         case KEY_B:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_LEFT;
            return(onKeyDown(new_event));
         }

         // Ctrl-F - move one character forward
         case KEY_F:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_RIGHT;
            return(onKeyDown(new_event));
         }

         // Ctrl-A - move to the beginning of the line
         case KEY_A:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_HOME;
            return(onKeyDown(new_event));
         }

         // Ctrl-E - move to the end of the line
         case KEY_E:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_END;
            return(onKeyDown(new_event));
         }

         // Ctrl-P - move backward in history
         case KEY_P:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_UP;
            return(onKeyDown(new_event));
         }

         // Ctrl-N - move forward in history
         case KEY_N:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_DOWN;
            return(onKeyDown(new_event));
         }

         // Ctrl-D - delete under cursor
         case KEY_D:
         { GuiEvent new_event;
            new_event.modifier = 0;
            new_event.keyCode = KEY_DELETE;
            return(onKeyDown(new_event));
         }

         case KEY_U:
         { GuiEvent new_event;
            new_event.modifier = SI_CTRL;
            new_event.keyCode = KEY_DELETE;
            return(onKeyDown(new_event));
         }

         // End added UNIX emacs key bindings

         case KEY_C:
         case KEY_X:
            if(mPasswordText)
               return true;
            char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
            if (mBlockEnd > 0)
            {
               //save the current state
               saveUndoState();

               //copy the text to the clipboard
               char temp = mText[mBlockEnd];
               mText[mBlockEnd] = '\0';
               Platform::setClipboard(&mText[mBlockStart]);
               mText[mBlockEnd] = temp;

               //if we pressed ctrl-x, we need to cut the selected text from the control...
               if (event.keyCode == KEY_X)
               {
                  dStrcpy(buf, &mText[mBlockEnd]);
                  mCursorPos = mBlockStart;
                  dStrcpy(&mText[mBlockStart], buf);
               }

               mBlockStart = 0;
               mBlockEnd = 0;
            }
            
            return true;
            
         case KEY_V:
         {
            char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];

            //first, make sure there's something in the clipboard to copy...
            const char *temp = Platform::getClipboard();
            const char *clipBuf = GuiMLTextCtrl::stripControlChars(temp);
            if (dStrlen(clipBuf) <= 0)
               return true;
            
            //save the current state
            saveUndoState();

            //delete anything hilited
            if (mBlockEnd > 0)
            {
                dStrcpy(buf, &mText[mBlockEnd]);
                mCursorPos = mBlockStart;
                dStrcpy(&mText[mBlockStart], buf);
                mBlockStart = 0;
                mBlockEnd = 0;
            }
      
            S32 pasteLen = dStrlen(clipBuf);
            if ((stringLen + pasteLen) > mMaxStrLen)
               pasteLen = mMaxStrLen - stringLen;
                                    
            if (mCursorPos == stringLen)
            {
               dStrncpy(&mText[mCursorPos], clipBuf, pasteLen);
               mText[mCursorPos + pasteLen] = '\0';
            }
            else
            {
               dStrcpy(buf, &mText[mCursorPos]);
               dStrncpy(&mText[mCursorPos], clipBuf, pasteLen);
               dStrcpy(&mText[mCursorPos + pasteLen], buf);
            }
            mCursorPos += pasteLen;

            return true;
         }

         case KEY_Z:
            if (! mDragHit)
            {
                char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
                S32 tempBlockStart;
                S32 tempBlockEnd;
                S32 tempCursorPos;
                
                //save the current
                dStrcpy(buf, mText);
                tempBlockStart = mBlockStart;
                tempBlockEnd = mBlockEnd;
                tempCursorPos = mCursorPos;
                
                //restore the prev
                dStrcpy(mText, mUndoText);
                mBlockStart = mUndoBlockStart;
                mBlockEnd = mUndoBlockEnd;
                mCursorPos = mUndoCursorPos;
                
                //update the undo
                dStrcpy(mUndoText, buf);
                mUndoBlockStart = tempBlockStart;
                mUndoBlockEnd = tempBlockEnd;
                mUndoCursorPos = tempCursorPos;
                
                return true;
             }
                
         case KEY_DELETE:
         case KEY_BACKSPACE:
            //save the current state
            saveUndoState();
            
            //delete everything in the field
            mText[0] = '\0';
            mCursorPos = 0;
            mBlockStart = 0;
            mBlockEnd = 0;

            // Execute the console command!
            if ( mConsoleCommand[0] )
            {
               char buf[16];
               dSprintf( buf, sizeof( buf ), "%d", getId() );
               Con::setVariable( "$ThisControl", buf );
               Con::evaluate( mConsoleCommand, false );
            }

            // Update the console variable:
            if ( mConsoleVariable[0] )
               Con::setVariable( mConsoleVariable, mText );

            return true;
      } //switch
   }
   else
   {
      switch(event.keyCode)
      {
         case KEY_ESCAPE:
            if ( mEscapeCommand[0] )
            {
               Con::evaluate( mEscapeCommand );
               return( true );
            }
            return( Parent::onKeyDown( event ) );

         case KEY_RETURN:
            //first validate
            onLoseFirstResponder();
            mHistoryDirty = false;
            
            //next exec the alt console command
            if ( mAltConsoleCommand[0] )
            {
               char buf[16];
               dSprintf( buf, sizeof( buf ), "%d", getId() );
               Con::setVariable( "$ThisControl", buf );
               Con::evaluate( mAltConsoleCommand, false );
            }
            
            if (mProfile->mReturnTab)
            {
               GuiCanvas *root = getRoot();
               if (root)
               {
                  root->tabNext();
                  return true;
               }
            }
            return true;
         
         case KEY_UP:
            if(mHistoryDirty)
            {
               updateHistory(mText, false);
               mHistoryDirty = false;
            }
            mHistoryIndex--;
            if(mHistoryIndex >= 0 && mHistoryIndex <= mHistoryLast)
               setText(mHistoryBuf[mHistoryIndex]);
            else if(mHistoryIndex < 0)
               mHistoryIndex = 0;
            return true;
         case KEY_DOWN:
            if(mHistoryDirty)
            {
               updateHistory(mText, false);
               mHistoryDirty = false;
            }
            mHistoryIndex++;
            if(mHistoryIndex > mHistoryLast)
            {
               mHistoryIndex = mHistoryLast + 1;
               setText("");
            }
            else
               setText(mHistoryBuf[mHistoryIndex]);
            return true;
         case KEY_LEFT:
            mBlockStart = 0;
            mBlockEnd = 0;
            if (mCursorPos > 0)
            {
                mCursorPos--;
            }
            return true;
            
         case KEY_RIGHT:
            mBlockStart = 0;
            mBlockEnd = 0;
            if (mCursorPos < stringLen)
            {
                mCursorPos++;
            }
            return true;
                     
         case KEY_BACKSPACE:
            //save the current state
            saveUndoState();

            if (mBlockEnd > 0)
            {
               char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
               dStrcpy(buf, &mText[mBlockEnd]);
               mCursorPos = mBlockStart;
               dStrcpy(&mText[mBlockStart], buf);
               mBlockStart = 0;
               mBlockEnd = 0;
               mHistoryDirty = true;

               // Execute the console command!
               if ( mConsoleCommand[0] )
               {
                  char buf[16];
                  dSprintf( buf, sizeof( buf ), "%d", getId() );
                  Con::setVariable( "$ThisControl", buf );
                  Con::evaluate( mConsoleCommand, false );
               }

               // Update the console variable!
               if (mConsoleVariable[0])
                  Con::setVariable(mConsoleVariable, mText);
            }
            else if (mCursorPos > 0)
            {
               char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
               dStrcpy(buf, &mText[mCursorPos]);
               mCursorPos--;
               dStrcpy(&mText[mCursorPos], buf);
               mHistoryDirty = true;

               // Execute the console command!
               if ( mConsoleCommand[0] )
               {
                  char buf[16];
                  dSprintf( buf, sizeof( buf ), "%d", getId() );
                  Con::setVariable( "$ThisControl", buf );
                  Con::evaluate( mConsoleCommand, false );
               }

               // Update the console variable!
               if (mConsoleVariable[0])
                  Con::setVariable(mConsoleVariable, mText);
            }
            return true;
            
         case KEY_DELETE:
            //save the current state
            saveUndoState();
                
            if (mBlockEnd > 0)
            {
               mHistoryDirty = true;
               char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
               dStrcpy(buf, &mText[mBlockEnd]);
               mCursorPos = mBlockStart;
               dStrcpy(&mText[mBlockStart], buf);
               mBlockStart = 0;
               mBlockEnd = 0;

               // Execute the console command!
               if ( mConsoleCommand[0] )
               {
                  char buf[16];
                  dSprintf( buf, sizeof( buf ), "%d", getId() );
                  Con::setVariable( "$ThisControl", buf );
                  Con::evaluate( mConsoleCommand, false );
               }

               // Update the console variable!
               if (mConsoleVariable[0])
                  Con::setVariable(mConsoleVariable, mText);
            }
            else if (mCursorPos < stringLen)
            {
               mHistoryDirty = true;
               char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
               dStrcpy(buf, &mText[mCursorPos + 1]);
               dStrcpy(&mText[mCursorPos], buf);

               // Execute the console command!
               if ( mConsoleCommand[0] )
               {
                  char buf[16];
                  dSprintf( buf, sizeof( buf ), "%d", getId() );
                  Con::setVariable( "$ThisControl", buf );
                  Con::evaluate( mConsoleCommand, false );
               }

               // Update the console variable!
               if (mConsoleVariable[0])
                  Con::setVariable(mConsoleVariable, mText);
            }
            return true;
         
         case KEY_INSERT:
            mInsertOn = !mInsertOn;
            return true;
         
         case KEY_HOME:
            mBlockStart = 0;
            mBlockEnd = 0;
            mCursorPos = 0;
            return true;
            
         case KEY_END:
            mBlockStart = 0;
            mBlockEnd = 0;
            mCursorPos = stringLen;
            return true;
            
         }
   }
   
   switch ( event.keyCode )
   {
      case KEY_TAB:
         if ( mTabComplete )
         {
            Con::executef( this, 2, "onTabComplete", "0" );
            return( true );
         }
      case KEY_UP:
      case KEY_DOWN:
      case KEY_ESCAPE:
         return Parent::onKeyDown( event );
   }
   
   if ( mFont->isValidChar( event.ascii ) )
   {
      //see if it's a number field
      if ( mProfile->mNumbersOnly )
      {
         if (event.ascii == '-')
         {
            //a minus sign only exists at the beginning, and only a single minus sign
            if ( mCursorPos != 0 )
            { 
               playDeniedSound();
               return true;
            }

            if ( mInsertOn && ( mText[0] == '-' ) ) 
            {
               playDeniedSound();
               return true;
            }
         }
         else if ( event.ascii < '0' || event.ascii > '9' )
         {
            playDeniedSound();
            return true;
         }
      }
      
      //save the current state
      saveUndoState();
                
      //delete anything hilited
      if ( mBlockEnd > 0 )
      {
          char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
          dStrcpy( buf, &mText[mBlockEnd] );
          mCursorPos = mBlockStart;
          dStrcpy( &mText[mBlockStart], buf );
          mBlockStart = 0;
          mBlockEnd = 0;
      }
      
      if ( ( mInsertOn && ( stringLen < mMaxStrLen ) ) ||
          ( !mInsertOn && ( mCursorPos < mMaxStrLen ) ) )
      {
         if ( mCursorPos == stringLen )
         {
            mText[mCursorPos++] = event.ascii;
            mText[mCursorPos] = '\0';
         }
         else
         {
            if ( mInsertOn )
            {
                char buf[GuiTextCtrl::MAX_STRING_LENGTH + 1];
                dStrcpy( buf, &mText[mCursorPos] );
                mText[mCursorPos] = event.ascii;
                mCursorPos++;
                dStrcpy( &mText[mCursorPos], buf );
            }
            else
            {
                mText[mCursorPos++] = event.ascii;
                if ( mCursorPos > stringLen )
                   mText[mCursorPos] = '\0';
            }
         }
      }
      else
         playDeniedSound();
      
      //reset the history index
      mHistoryDirty = true;
                           
      //execute the console command if it exists
      if ( mConsoleCommand[0] )
      {
         char buf[16];
         dSprintf( buf, sizeof( buf ), "%d", getId() );
         Con::setVariable( "$ThisControl", buf );
         Con::evaluate( mConsoleCommand, false );
      }
      
      //now set the console var if exists
      if ( mConsoleVariable[0] )
         Con::setVariable( mConsoleVariable, mText );
      
      return true;
   }
   
   //not handled - pass the event to it's parent

   if (mSinkAllKeyEvents) {
      return true;
   }
   else {
      return Parent::onKeyDown( event );

   }
}

void GuiTextEditCtrl::onLoseFirstResponder()
{
   //first, update the history
   updateHistory( mText, true );
      
   //execute the validate command
   if ( mValidateCommand[0] )
      Con::evaluate( mValidateCommand, false );

   // Redraw the control:
   setUpdate();
}
               


void GuiTextEditCtrl::parentResized(const Point2I &oldParentExtent, const Point2I &newParentExtent)
{
   Parent::parentResized( oldParentExtent, newParentExtent );
   mTextOffset.x = 65535;
}

void GuiTextEditCtrl::onRender(Point2I offset, const RectI & /*updateRect*/)
{
   RectI ctrlRect( offset, mBounds.extent );
   
   //if opaque, fill the update rect with the fill color
   if ( mProfile->mOpaque )
      dglDrawRectFill( ctrlRect, mProfile->mFillColor );
   
   DrawText( ctrlRect, isFirstResponder() );
   
   //if there's a border, draw the border
   if ( mProfile->mBorder )
      renderBorder( ctrlRect, mProfile );
}

void GuiTextEditCtrl::onPreRender()
{
   if ( isFirstResponder() )
   {
      U32 timeElapsed = Platform::getVirtualMilliseconds() - mTimeLastCursorFlipped;
      mNumFramesElapsed++;
      if ( ( timeElapsed > 500 ) && ( mNumFramesElapsed > 3 ) )
      {
         mCursorOn = 1 - mCursorOn;
         mTimeLastCursorFlipped = Sim::getCurrentTime();   
         mNumFramesElapsed = 0;
         setUpdate();
      }
      
      //update the cursor if the text is scrolling
      if ( mDragHit )
      {
         if ( ( mScrollDir < 0 ) && ( mCursorPos > 0 ) )
            mCursorPos--;
         else if ( ( mScrollDir > 0 ) && ( mCursorPos < (S32) dStrlen( mText ) ) )
            mCursorPos++;
      }
   }
}    

void GuiTextEditCtrl::DrawText( const RectI &drawRect, bool isFocused )
{
   Point2I drawPoint = drawRect.point;

   // Center vertically:
   drawPoint.y += ( ( drawRect.extent.y - mFont->getHeight() ) / 2 ) + mProfile->mTextOffset.y;
   char textBuffer[GuiTextCtrl::MAX_STRING_LENGTH + 1];
   
   dStrcpy(textBuffer, mText);
   if(mPasswordText)
   for(U32 i = 0; textBuffer[i]; i++)
      textBuffer[i] = '*';

   // Align horizontally:
   S32 txt_w = mFont->getStrWidth( textBuffer );
   if ( drawRect.extent.x > txt_w )
   {
      switch( mProfile->mAlignment )
      {
         case GuiControlProfile::RightJustify:
            //BH localStart.set( mBounds.extent.x - txt_w, 0 );
            drawPoint.x += ( drawRect.extent.x - txt_w );  
            break;
         case GuiControlProfile::CenterJustify:
            //BH localStart.set( ( mBounds.extent.x - txt_w ) / 2, 0 );
            drawPoint.x += ( ( drawRect.extent.x - txt_w ) / 2 );
            break;
         //BH default:
            // GuiControlProfile::LeftJustify
            //BH localStart.set( ( mProfile->mTextOffset.x != 0 ? mProfile->mTextOffset.x : 4 ), 0 );
            //BH break;
      }
   }
   //BH else
      //BH localStart.set( ( mProfile->mTextOffset.x != 0 ? mProfile->mTextOffset.x : 4 ), 0 );

   //BH Point2I ctrlOffset = localToGlobalCoord( localStart );
                  
   ColorI fontColor = mActive ? mProfile->mFontColor : mProfile->mFontColorNA;
   
   // now draw the text
   //BH S32 txt_h = mFont->getHeight();
   Point2I cursorStart, cursorEnd;
   //BH mTextOffset.y = ctrlOffset.y + ( ( mBounds.extent.y - txt_h ) >> 1 );
   mTextOffset.y = drawPoint.y;
   if ( ( mTextOffset.x == 65535 ) || ( txt_w < drawRect.extent.x ) )
      mTextOffset.x = drawRect.point.x + 3;
   
   // calculate the cursor
   if ( isFocused )
   {
      char temp = textBuffer[mCursorPos];
      textBuffer[mCursorPos] = '\0';
      S32 tempWidth = mFont->getStrWidth( textBuffer );
      
      if( tempWidth >= drawRect.extent.x)
      {
         cursorStart.x = drawRect.extent.x + drawRect.point.x + 4;
      }
      else
      {
         cursorStart.x = mTextOffset.x + tempWidth;
      }
      
      textBuffer[mCursorPos] = temp;
      cursorEnd.x = cursorStart.x;
      
      S32 cursorHeight = mFont->getHeight();
      if ( cursorHeight < drawRect.extent.y )
      {
         cursorStart.y = drawRect.point.y + ( ( drawRect.extent.y - cursorHeight ) / 2 ) - 2;
         cursorEnd.y = cursorStart.y + cursorHeight;
      }
      else
      {
         cursorStart.y = drawRect.point.y;
         cursorEnd.y = cursorStart.y + drawRect.extent.y - 1;
      }
      
      if ( cursorStart.x < ( drawRect.point.x ) )
      {
         //mTextOffset.x += (3 + mTextClipRect.point.x - cursorStart.x);
         cursorStart.x = 3 + drawRect.point.x;
         mTextOffset.x = cursorStart.x - tempWidth;
         cursorEnd.x = cursorStart.x;
      }
//      else if ( cursorStart.x >= ( drawRect.point.x + drawRect.extent.x ) )
      else if ( cursorStart.x >= ( drawRect.point.x + drawRect.extent.x ) )
      {
         cursorStart.x = drawRect.point.x + mBounds.extent.x - 4;
//         mTextOffset.x = cursorStart.x - tempWidth;
       mTextOffset.x = drawRect.point.x - (tempWidth-drawRect.extent.x)  ;
         cursorEnd.x = cursorStart.x;
      }
   }
   else
   {
      if(mTextOffset.x < drawRect.point.x)
      {
         mTextOffset.x = drawRect.point.x + 3;
      }
   }
   
   //draw the text
   if ( !isFocused )
      mBlockStart = mBlockEnd = 0;
   
   //also verify the block start/end
   if ((mBlockStart > S32(dStrlen(textBuffer))) || (mBlockEnd > S32(dStrlen(textBuffer))) || (mBlockStart > mBlockEnd))
      mBlockStart = mBlockEnd = 0;
   
   char temp;
   
   Point2I tempOffset = mTextOffset;
   tempOffset.y -= 2;
   
   //draw the portion before the highlite
   if ( mBlockStart > 0 )
   {
      temp = textBuffer[mBlockStart];
      textBuffer[mBlockStart] = '\0';

      dglSetBitmapModulation( fontColor );
      dglDrawText( mFont, tempOffset, textBuffer, mProfile->mFontColors );
      tempOffset.x += mFont->getStrWidth( textBuffer );
      textBuffer[mBlockStart] = temp;
   }
   
   //draw the hilited portion
   if ( mBlockEnd > 0 )
   {
      temp = textBuffer[mBlockEnd];
      textBuffer[mBlockEnd] = '\0';
      S32 highlightWidth = mFont->getStrWidth( &textBuffer[mBlockStart] );
      
      dglDrawRectFill( Point2I( tempOffset.x, drawRect.point.y ),
                     Point2I( tempOffset.x + highlightWidth, drawRect.point.y + drawRect.extent.y - 1),
                     mProfile->mFillColorHL );
      
      dglSetBitmapModulation( mProfile->mFontColorHL );
      dglDrawText( mFont, tempOffset, &textBuffer[mBlockStart], mProfile->mFontColors );
      tempOffset.x += highlightWidth;
      textBuffer[mBlockEnd] = temp;
   }
   
   //draw the portion after the highlite
   dglSetBitmapModulation( fontColor );
   dglDrawText( mFont, tempOffset, &textBuffer[mBlockEnd], mProfile->mFontColors );
   
   //draw the cursor      
   if ( isFocused && mCursorOn )
      dglDrawLine( cursorStart, cursorEnd, mProfile->mCursorColor );
}

bool GuiTextEditCtrl::hasText()
{
   return (mText[0]);
}

void GuiTextEditCtrl::playDeniedSound()
{
   if ( mDeniedSound )
   {
      AUDIOHANDLE handle = alxCreateSource( mDeniedSound );
      alxPlay( handle );
   }
}

ConsoleMethod( GuiTextEditCtrl, getCursorPos, S32, 2, 2, "textEditCtrl.getCursorPos()" )
{
   argc; argv;
   return( object->getCursorPos() );
}

ConsoleMethod( GuiTextEditCtrl, setCursorPos, void, 3, 3, "textEditCtrl.setCursorPos( newPos )" )
{
   argc;
   object->reallySetCursorPos( dAtoi( argv[2] ) );
}
