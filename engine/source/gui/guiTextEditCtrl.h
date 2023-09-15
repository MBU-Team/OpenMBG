//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITEXTEDITCTRL_H_
#define _GUITEXTEDITCTRL_H_

#ifndef _GUITYPES_H_
#include "gui/guiTypes.h"
#endif
#ifndef _GUITEXTCTRL_H_
#include "gui/guiTextCtrl.h"
#endif

class GuiTextEditCtrl : public GuiTextCtrl
{
private:
   typedef GuiTextCtrl Parent;

   static U32 mNumAwake;

protected:
   StringTableEntry mValidateCommand;
   StringTableEntry mEscapeCommand;
   AudioProfile*  mDeniedSound;

   // for animating the cursor
   S32      mNumFramesElapsed;
   U32   mTimeLastCursorFlipped;
   ColorI   mCursorColor;
   bool      mCursorOn;

   bool       mInsertOn;
   S32      mMouseDragStart;
   Point2I   mTextOffset;
   bool      mDragHit;
   bool     mTabComplete;
   S32      mScrollDir;

   //undo members
   char      mUndoText[GuiTextCtrl::MAX_STRING_LENGTH + 1];
   S32      mUndoBlockStart;
   S32      mUndoBlockEnd;
   S32      mUndoCursorPos;
   void saveUndoState();

   S32      mBlockStart;
   S32      mBlockEnd;
   S32      mCursorPos;
   S32 setCursorPos(const Point2I &offset);

   bool     mHistoryDirty;
   S32      mHistoryLast;
   S32      mHistoryIndex;
   S32      mHistorySize;
   bool     mPasswordText;

   bool    mSinkAllKeyEvents;   // any non-ESC key is handled here or not at all
   char      **mHistoryBuf;
   void updateHistory(const char *txt, bool moveIndex);

   void playDeniedSound();

public:   
   GuiTextEditCtrl();
   ~GuiTextEditCtrl();
   DECLARE_CONOBJECT(GuiTextEditCtrl);
   static void initPersistFields();

   bool onAdd();
   bool onWake();
   void onSleep();

   void getText(char *dest);  // dest must be of size
                              // StructDes::MAX_STRING_LEN + 1

   void setText(S32 tag);
   virtual void setText(const char *txt);
   S32   getCursorPos()   { return( mCursorPos ); }
   void  reallySetCursorPos( const S32 newPos );

   bool onKeyDown(const GuiEvent &event);
   void onMouseDown(const GuiEvent &event);
   void onMouseDragged(const GuiEvent &event);
   void onMouseUp(const GuiEvent &event);

   void onLoseFirstResponder();

   void parentResized(const Point2I &oldParentExtent, const Point2I &newParentExtent);
   bool hasText();

   void onPreRender();
   void onRender(Point2I offset, const RectI &updateRect);
   virtual void DrawText( const RectI &drawRect, bool isFocused );
};

#endif //_GUI_TEXTEDIT_CTRL_H
