//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUIINSPECTOR_H_
#define _GUIINSPECTOR_H_

#ifndef _GUICONTROL_H_
#include "gui/guiControl.h"
#endif
#ifndef _GUIARRAYCTRL_H_
#include "gui/guiArrayCtrl.h"
#endif

class GuiInspector : public GuiControl
{
   private:
      typedef GuiControl Parent;
      SimObjectPtr<SimObject>    mTarget;

   public:
      DECLARE_CONOBJECT(GuiInspector);

      GuiInspector();

      // field data
      S32                  mEditControlOffset;
      S32                  mEntryHeight;
      S32                  mTextExtent;
      S32                  mEntrySpacing;
      S32                  mMaxMenuExtent;
      bool                 mUseFieldGrouping;

      static void initPersistFields();
      void onRemove();
      void write(Stream &stream, U32 tabStop, U32 flags);

      void inspect(SimObject *);
      void apply(const char *);
      void toggleGroupExpand(SimObject *, const char*);   // MM: toggle Group Expansion.
      void toggleDynamicGroupExpand();               // MM: toggle Dynamic Group Expansion.
      void setAllGroupState(bool GroupState);            // MM: set All Group State.
};

#endif
