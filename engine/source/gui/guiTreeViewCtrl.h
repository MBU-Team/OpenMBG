//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _GUITREEVIEWCTRL_H_
#define _GUITREEVIEWCTRL_H_

#ifndef _MRECT_H_
#include "math/mRect.h"
#endif
#ifndef _GFONT_H_
#include "dgl/gFont.h"
#endif
#ifndef _GUICONTROL_H_
#include "gui/guiControl.h"
#endif
#ifndef _GUIARRAYCTRL_H_
#include "gui/guiArrayCtrl.h"
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class GuiTreeViewCtrl : public GuiArrayCtrl
{
   private:
      typedef GuiArrayCtrl Parent;

      struct ObjNode
      {
         S32 level;
         SimObject *object;
         bool lastInGroup;
         S32 parentIndex;
         ObjNode(S32 in_level, SimObject *in_object, bool in_last, S32 in_pi)
         {
            level = in_level;
            object = in_object;
            lastInGroup = in_last;
            parentIndex = in_pi;
         }
      };

      Vector<ObjNode> mObjectList;
      
      //font
      Resource<GFont> mFont;

      SimSet *mRootObject;

      // persist data
      bool mAllowMultipleSelections;
      bool mRecurseSets;
      
      ObjNode * getHitNode(const GuiEvent & event);   
      
   public:
      DECLARE_CONOBJECT(GuiTreeViewCtrl);
      GuiTreeViewCtrl();
      static void initPersistFields();
      
      bool onWake();
      void onSleep();

      void setTreeRoot(SimSet *srcObj);
      void buildTree(SimSet *srcObj, S32 srcLevel, S32 srcParentIndex);

      void onPreRender();
      void onMouseDown(const GuiEvent &event);
      void onRightMouseDown(const GuiEvent & event);
      void onRightMouseUp(const GuiEvent & event);

      void setInstantGroup(SimObject * obj);
      void inspectObject(SimObject * obj);
      void selectObject(SimObject * obj, bool rightMouse = false);
      void unselectObject(SimObject * obj);
      void toggleSelected(SimObject * obj);
      void clearSelected();
      void setSelected(SimObject *selObj, bool rightMouse = false);

      void onRenderCell(Point2I offset, Point2I cell, bool, bool);
};

#endif
