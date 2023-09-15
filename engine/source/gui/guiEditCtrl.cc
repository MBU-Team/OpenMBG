//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "dgl/dgl.h"
#include "console/simBase.h"
#include "gui/guiCanvas.h"
#include "gui/guiEditCtrl.h"
#include "platform/event.h"
#include "core/fileStream.h"
#include "gui/guiScrollCtrl.h"

GuiEditCtrl::GuiEditCtrl()
{
   VECTOR_SET_ASSOCIATION(mSelectedControls);

   mActive = true;
   mCurrentAddSet = NULL;
   mGridSnap.set(0, 0);
}

ConsoleMethod( GuiEditCtrl, setRoot, void, 3, 3, "(GuiControl root)")
{
   GuiControl *ctrl;
   if(!Sim::findObject(argv[2], ctrl))
      return;
   object->setRoot(ctrl);
}


ConsoleMethod( GuiEditCtrl, addNewCtrl, void, 3, 3, "(GuiControl ctrl)")
{
   GuiControl *ctrl;
   if(!Sim::findObject(argv[2], ctrl))
      return;
   object->addNewControl(ctrl);
}

ConsoleMethod( GuiEditCtrl, select, void, 3, 3, "(GuiControl ctrl)")
{
   GuiControl *ctrl;
   
   if(!Sim::findObject(argv[2], ctrl))
      return;

   object->setSelection(ctrl, false);
}

ConsoleMethod( GuiEditCtrl, setCurrentAddSet, void, 3, 3, "(GuiControl ctrl)")
{
   GuiControl *addSet;
   
   if (!Sim::findObject(argv[2], addSet))
   {
      Con::printf("%s(): Invalid control: %s", argv[0], argv[2]);
      return;
   }
   object->setCurrentAddSet(addSet);
}

ConsoleMethod( GuiEditCtrl, toggle, void, 2, 2, "Toggle activation.")
{
   object->setEditMode(! object->mActive);
}

ConsoleMethod( GuiEditCtrl, justify, void, 3, 3, "(int mode)" )
{
   object->justifySelection((GuiEditCtrl::Justification)dAtoi(argv[2]));
}

ConsoleMethod( GuiEditCtrl, bringToFront, void, 2, 2, "")
{
   object->bringToFront();
}

ConsoleMethod( GuiEditCtrl, pushToBack, void, 2, 2, "")
{
   object->pushToBack();
}

ConsoleMethod( GuiEditCtrl, deleteSelection, void, 2, 2, "Delete the selected text.")
{
   object->deleteSelection();
}

ConsoleMethod( GuiEditCtrl, moveSelection, void, 4, 4, "(int deltax, int deltay)")
{
   object->moveSelection(Point2I(dAtoi(argv[2]), dAtoi(argv[3])));
}

ConsoleMethod( GuiEditCtrl, saveSelection, void, 3, 3, "(string fileName)")
{
   object->saveSelection(argv[2]);
}

ConsoleMethod( GuiEditCtrl, loadSelection, void, 3, 3, "(string fileName)")
{
   object->loadSelection(argv[2]);
}

ConsoleMethod( GuiEditCtrl, selectAll, void, 2, 2, "()")
{
   object->selectAll();
}

bool GuiEditCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
   setEditMode(true);   
   return true;
}

void GuiEditCtrl::setRoot(GuiControl *root)
{
   mContentControl = root;
	mCurrentAddSet = mContentControl;
	mSelectedControls.clear();
}

enum GuiEditConstants { 
   GUI_BLACK = 0, 
   GUI_WHITE = 255,
   NUT_SIZE = 3
};

void GuiEditCtrl::setEditMode(bool value)
{
   mActive = value;
   mSelectedControls.clear();
   if (mActive && mAwake)
      mCurrentAddSet = NULL;
}

void GuiEditCtrl::setCurrentAddSet(GuiControl *ctrl)
{
   if (ctrl != mCurrentAddSet)
   {
      mSelectedControls.clear();
      mCurrentAddSet = ctrl;
   }
}

void GuiEditCtrl::setSelection(GuiControl *ctrl, bool inclusive)
{
   //sanity check
   if (! ctrl)
      return;
   
   if(mContentControl == ctrl)
   {
      mCurrentAddSet = ctrl;
      mSelectedControls.clear();
   }
   else
   {
      // otherwise, we hit a new control...
      GuiControl *newAddSet = ctrl->getParent();
   
      //see if we should clear the old selection set
      if (newAddSet != mCurrentAddSet || (! inclusive))
         mSelectedControls.clear();
   
      //set the selection
      mCurrentAddSet = newAddSet;
      mSelectedControls.push_back(ctrl);
   }
}

void GuiEditCtrl::addNewControl(GuiControl *ctrl)
{
   if (! mCurrentAddSet)
      mCurrentAddSet = mContentControl;

   mCurrentAddSet->addObject(ctrl);
   mSelectedControls.clear();
   mSelectedControls.push_back(ctrl);
}

void GuiEditCtrl::drawNut(const Point2I &nut, ColorI &outlineColor, ColorI &nutColor)
{
   RectI r(nut.x - NUT_SIZE, nut.y - NUT_SIZE, 2 * NUT_SIZE + 1, 2 * NUT_SIZE + 1);
   r.inset(1, 1);
   dglDrawRectFill(r, nutColor);
   r.inset(-1, -1);
   dglDrawRect(r, outlineColor);
}

static inline bool inNut(const Point2I &pt, S32 x, S32 y)
{
   S32 dx = pt.x - x;
   S32 dy = pt.y - y;
   return dx <= NUT_SIZE && dx >= -NUT_SIZE && dy <= NUT_SIZE && dy >= -NUT_SIZE;
}

S32 GuiEditCtrl::getSizingHitKnobs(const Point2I &pt, const RectI &box)
{
   S32 lx = box.point.x, rx = box.point.x + box.extent.x - 1;
   S32 cx = (lx + rx) >> 1;
   S32 ty = box.point.y, by = box.point.y + box.extent.y - 1;
   S32 cy = (ty + by) >> 1;

   if (inNut(pt, lx, ty))
      return sizingLeft | sizingTop;
   if (inNut(pt, cx, ty))
      return sizingTop;
   if (inNut(pt, rx, ty))
      return sizingRight | sizingTop;
   if (inNut(pt, lx, by))
      return sizingLeft | sizingBottom;
   if (inNut(pt, cx, by))
      return sizingBottom;
   if (inNut(pt, rx, by))
      return sizingRight | sizingBottom;
   if (inNut(pt, lx, cy))
      return sizingLeft;
   if (inNut(pt, rx, cy))
      return sizingRight;
   return sizingNone;
}

void GuiEditCtrl::drawNuts(RectI &box, ColorI &outlineColor, ColorI &nutColor)
{
   S32 lx = box.point.x, rx = box.point.x + box.extent.x - 1;
   S32 cx = (lx + rx) >> 1;
   S32 ty = box.point.y, by = box.point.y + box.extent.y - 1;
   S32 cy = (ty + by) >> 1;
   ColorF greenLine(0,1,0,0.6);
   ColorF lightGreenLine(0,1,0,0.3);
   if(lx > 0 && ty > 0)
   {
      dglDrawLine(0, ty, lx, ty, greenLine);
      dglDrawLine(lx, 0, lx, ty, greenLine);
   }
   if(lx > 0 && by > 0)
      dglDrawLine(0, by, lx, by, greenLine);

   if(rx > 0 && ty > 0)
      dglDrawLine(rx, 0, rx, ty, greenLine);

   Point2I extent = localToGlobalCoord(mBounds.extent);
   
   if(lx < extent.x && by < extent.y)
      dglDrawLine(lx, by, lx, extent.y, lightGreenLine);
   if(rx < extent.x && by < extent.y)
   {
      dglDrawLine(rx, by, rx, extent.y, lightGreenLine);
      dglDrawLine(rx, by, extent.x, by, lightGreenLine);
   }
   if(rx < extent.x && ty < extent.y)
      dglDrawLine(rx, ty, extent.x, ty, lightGreenLine);

   drawNut(Point2I(lx, ty), outlineColor, nutColor);
   drawNut(Point2I(lx, cy), outlineColor, nutColor);
   drawNut(Point2I(lx, by), outlineColor, nutColor);
   drawNut(Point2I(rx, ty), outlineColor, nutColor);
   drawNut(Point2I(rx, cy), outlineColor, nutColor);
   drawNut(Point2I(rx, by), outlineColor, nutColor);
   drawNut(Point2I(cx, ty), outlineColor, nutColor);
   drawNut(Point2I(cx, by), outlineColor, nutColor);
}

void GuiEditCtrl::getDragRect(RectI &box)
{
   box.point.x = getMin(mLastMousePos.x, mSelectionAnchor.x);
   box.extent.x = getMax(mLastMousePos.x, mSelectionAnchor.x) - box.point.x + 1;
   box.point.y = getMin(mLastMousePos.y, mSelectionAnchor.y);
   box.extent.y = getMax(mLastMousePos.y, mSelectionAnchor.y) - box.point.y + 1;
}

void GuiEditCtrl::onPreRender()
{
   setUpdate();
}

void GuiEditCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   Point2I ctOffset;
   Point2I cext;
   bool keyFocused = isFirstResponder();

   if (mActive)
   {
      if (mCurrentAddSet)
      {
         // draw a white frame inset around the current add set.
         cext = mCurrentAddSet->getExtent();
         ctOffset = mCurrentAddSet->localToGlobalCoord(Point2I(0,0));
         RectI box(ctOffset.x, ctOffset.y, cext.x, cext.y);

			box.inset(-2, -2);
			dglDrawRect(box, ColorI(0,0,128,140));
			box.inset(1,1);
			dglDrawRect(box, ColorI(0,0,128,140));
			box.inset(1,1);
			dglDrawRect(box, ColorI(0,255,0,140));
			box.inset(1,1);
			dglDrawRect(box, ColorI(255,255,0,140));
			box.inset(1,1);
			dglDrawRect(box, ColorI(255,255,0,140));
      }
      Vector<GuiControl *>::iterator i;
      bool multisel = mSelectedControls.size() > 1;
      for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
      {
         GuiControl *ctrl = (*i);
         cext = ctrl->getExtent();
         ctOffset = ctrl->localToGlobalCoord(Point2I(0,0));
         RectI box(ctOffset.x,ctOffset.y, cext.x, cext.y);
         ColorI nutColor = multisel ? ColorI(255,255,255) : ColorI(0,0,0);
         ColorI outlineColor = multisel ? ColorI(0,0,0) : ColorI(255,255,255);
         if(!keyFocused)
            nutColor.set(128,128,128);

         drawNuts(box, outlineColor, nutColor);
      }
      if (mMouseDownMode == DragSelecting)
      {
         RectI b;
         getDragRect(b);
         b.point += offset;
         dglDrawRect(b, ColorI(255, 255, 255));
      }
   }
   
   renderChildControls(offset, updateRect);
}

bool GuiEditCtrl::selectionContains(GuiControl *ctrl)
{
   Vector<GuiControl *>::iterator i;
   for (i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
      if (ctrl == *i) return true;
   return false;
}

void GuiEditCtrl::onRightMouseDown(const GuiEvent &event)
{
   if (! mActive)
   {
      Parent::onRightMouseDown(event);
      return;
   }
   setFirstResponder();
      
   //search for the control hit in any layer below the edit layer
   GuiControl *hitCtrl = mContentControl->findHitControl(globalToLocalCoord(event.mousePoint), mLayer - 1);
   if (hitCtrl != mCurrentAddSet)
   {
      mSelectedControls.clear();
      mCurrentAddSet = hitCtrl;
   }
}
void GuiEditCtrl::select(GuiControl *ctrl)
{
   mSelectedControls.clear();
   if(ctrl != mContentControl)
      mSelectedControls.push_back(ctrl);
   else
      mCurrentAddSet = mContentControl;
}
   
void GuiEditCtrl::onMouseDown(const GuiEvent &event)
{
   if (! mActive)
   {
      Parent::onMouseDown(event);
      return;
   }
   if(!mContentControl)
      return;
      
   setFirstResponder();
   //lock the mouse   
   mouseLock();
   
   Point2I ctOffset;
   Point2I cext;
   GuiControl *ctrl;

   mLastMousePos = globalToLocalCoord(event.mousePoint);
   
   // first see if we hit a sizing knob on the currently selected control...
   if (mSelectedControls.size() == 1)
   {
      ctrl = mSelectedControls.first();
      cext = ctrl->getExtent();
      ctOffset = globalToLocalCoord(ctrl->localToGlobalCoord(Point2I(0,0)));
      RectI box(ctOffset.x,ctOffset.y,cext.x, cext.y);

      if ((mSizingMode = (GuiEditCtrl::sizingModes)getSizingHitKnobs(mLastMousePos, box)) != 0)
      {
         mMouseDownMode = SizingSelection;
         return;
      }
   }

   if(!mCurrentAddSet)
      mCurrentAddSet = mContentControl;
   
   //find the control we clicked
   ctrl = mContentControl->findHitControl(mLastMousePos, mCurrentAddSet->mLayer);
   
   if (selectionContains(ctrl))
   {
      //if we're holding shift, de-select the clicked ctrl
      if (event.modifier & SI_SHIFT)
      {
         Vector<GuiControl *>::iterator i;
         for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
         {
            if (*i == ctrl)
            {
               mSelectedControls.erase(i);
               break;
            }
         }
         
         //set the mode
         mMouseDownMode = Selecting;
      }
      
      //else we hit a ctrl we've already selected, so set the mode to moving
      else
         mMouseDownMode = MovingSelection;
   }
   
   //else we clicked on an unselected control
   else
   {
      //if we clicked in the current add set
      if (ctrl == mCurrentAddSet)
      {
         // start dragging a rectangle
         // if the shift is not down, nuke prior selection
         if (!(event.modifier & SI_SHIFT))
            mSelectedControls.clear();
         mSelectionAnchor = mLastMousePos;
         mMouseDownMode = DragSelecting;
      }
      else
      {
         //find the new add set
         GuiControl *newAddSet = ctrl->getParent();
         
         //if we're holding shift and the ctrl is in the same add set
         if (event.modifier & SI_SHIFT && newAddSet == mCurrentAddSet)
         {
            mSelectedControls.push_back(ctrl);
            mMouseDownMode = Selecting;
         }
         else if (ctrl != mContentControl)
         {
            //find and set the new add set
            mCurrentAddSet = ctrl->getParent();
            
            //clear and set the selected controls
            mSelectedControls.clear();
            mSelectedControls.push_back(ctrl);
            mMouseDownMode = Selecting;
         }
         else
            mMouseDownMode = Selecting;
      }
   }
}

void GuiEditCtrl::onMouseUp(const GuiEvent &event)
{
   if (! mActive)
   {
      Parent::onMouseUp(event);
      return;
   }
   
   //unlock the mouse
   mouseUnlock();
      
   mLastMousePos = globalToLocalCoord(event.mousePoint);
   if (mMouseDownMode == DragSelecting)
   {
      RectI b;
      getDragRect(b);
      GuiControl::iterator i;
      for(i = mCurrentAddSet->begin(); i != mCurrentAddSet->end(); i++)
      {
         GuiControl *ctrl = dynamic_cast<GuiControl *>(*i);
         Point2I upperL = globalToLocalCoord(ctrl->localToGlobalCoord(Point2I(0,0)));
         Point2I lowerR = upperL + ctrl->mBounds.extent - Point2I(1, 1);

         if (b.pointInRect(upperL) && b.pointInRect(lowerR) && !selectionContains(ctrl))
            mSelectedControls.push_back(ctrl);
      }
   }
   if (mSelectedControls.size() == 1)
      Con::executef(this, 2, "onSelect", avar("%d", mSelectedControls[0]->getId()));
      
   setFirstResponder();
   //reset the mouse mode
   mMouseDownMode = Selecting;
}

void GuiEditCtrl::onMouseDragged(const GuiEvent &event)
{
   if (! mActive)
   {
      Parent::onMouseDragged(event);
      return;
   }
      
   if(!mCurrentAddSet)
      mCurrentAddSet = mContentControl;

   Point2I mousePoint = globalToLocalCoord(event.mousePoint);

   if (mMouseDownMode == SizingSelection)
   {
      if (mGridSnap.x)
         mousePoint.x -= mousePoint.x % mGridSnap.x;
      if (mGridSnap.y)
         mousePoint.y -= mousePoint.y % mGridSnap.y;

      GuiControl *ctrl = mSelectedControls.first();
      Point2I ctrlPoint = mCurrentAddSet->globalToLocalCoord(event.mousePoint);
      Point2I newPosition = ctrl->getPosition();
      Point2I newExtent = ctrl->getExtent();
      Point2I minExtent = ctrl->getMinExtent();
      
      if (mSizingMode & sizingLeft)
      {
         newPosition.x = ctrlPoint.x;
         newExtent.x = ctrl->mBounds.extent.x + ctrl->mBounds.point.x - ctrlPoint.x;
         if(newExtent.x < minExtent.x)
         {
            newPosition.x -= minExtent.x - newExtent.x;
            newExtent.x = minExtent.x;
         }
      }
      else if (mSizingMode & sizingRight)
      {
         newExtent.x = ctrlPoint.x - ctrl->mBounds.point.x;
         if(newExtent.x < minExtent.x)
            newExtent.x = minExtent.x;
      }   
      
      if (mSizingMode & sizingTop)
      {
         newPosition.y = ctrlPoint.y;
         newExtent.y = ctrl->mBounds.extent.y + ctrl->mBounds.point.y - ctrlPoint.y;
         if(newExtent.y < minExtent.y)
         {
            newPosition.y -= minExtent.y - newExtent.y;
            newExtent.y = minExtent.y;
         }
      }
      else if (mSizingMode & sizingBottom)
      {
         newExtent.y = ctrlPoint.y - ctrl->mBounds.point.y;
         if(newExtent.y < minExtent.y)
            newExtent.y = minExtent.y;
      }
      
      ctrl->resize(newPosition, newExtent);
      mCurrentAddSet->childResized(ctrl);
      Con::executef(this, 2, "onSelect", avar("%d", mSelectedControls[0]->getId()));
   } 
   else if (mMouseDownMode == MovingSelection && mSelectedControls.size())
   {
      Vector<GuiControl *>::iterator i = mSelectedControls.begin();
      Point2I minPos = (*i)->mBounds.point;
      for(; i != mSelectedControls.end(); i++)
      {
         if ((*i)->mBounds.point.x < minPos.x)
            minPos.x = (*i)->mBounds.point.x;
         if ((*i)->mBounds.point.y < minPos.y)
            minPos.y = (*i)->mBounds.point.y;
      }
      Point2I delta = mousePoint - mLastMousePos;
      delta += minPos; // find new minPos;

      if (mGridSnap.x)
         delta.x -= delta.x % mGridSnap.x;
      if (mGridSnap.y)
         delta.y -= delta.y % mGridSnap.y;

      delta -= minPos;
      moveSelection(delta);
      mLastMousePos += delta;
   }
   else
      mLastMousePos = mousePoint;
}

void GuiEditCtrl::moveSelection(const Point2I &delta)
{
   Vector<GuiControl *>::iterator i;

   for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
      (*i)->resize((*i)->mBounds.point + delta, (*i)->mBounds.extent);
   if (mSelectedControls.size() == 1)
      Con::executef(this, 2, "onSelect", avar("%d", mSelectedControls[0]->getId()));
}

void GuiEditCtrl::justifySelection(Justification j)
{
   S32 minX, maxX;
   S32 minY, maxY;
   S32 extentX, extentY;

   if (mSelectedControls.size() < 2)
      return;

   Vector<GuiControl *>::iterator i = mSelectedControls.begin();
   minX = (*i)->mBounds.point.x;
   maxX = minX + (*i)->mBounds.extent.x;
   minY = (*i)->mBounds.point.y;
   maxY = minY + (*i)->mBounds.extent.y;
   extentX = (*i)->mBounds.extent.x;
   extentY = (*i)->mBounds.extent.y;
   i++;
   for(;i != mSelectedControls.end(); i++)
   {
      minX = getMin(minX, (*i)->mBounds.point.x);
      maxX = getMax(maxX, (*i)->mBounds.point.x + (*i)->mBounds.extent.x);
      minY = getMin(minY, (*i)->mBounds.point.y);
      maxY = getMax(maxY, (*i)->mBounds.point.y + (*i)->mBounds.extent.y);
      extentX += (*i)->mBounds.extent.x;
      extentY += (*i)->mBounds.extent.y;
   }
   S32 deltaX = maxX - minX;
   S32 deltaY = maxY - minY;
   switch(j)
   {
      case JUSTIFY_LEFT:
         for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            (*i)->resize(Point2I(minX, (*i)->mBounds.point.y), (*i)->mBounds.extent);
         break;
      case JUSTIFY_TOP:
         for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            (*i)->resize(Point2I((*i)->mBounds.point.x, minY), (*i)->mBounds.extent);
         break;
      case JUSTIFY_RIGHT:
         for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            (*i)->resize(Point2I(maxX - (*i)->mBounds.extent.x + 1, (*i)->mBounds.point.y), (*i)->mBounds.extent);
         break;
      case JUSTIFY_BOTTOM:
         for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            (*i)->resize(Point2I((*i)->mBounds.point.x, maxY - (*i)->mBounds.extent.y + 1), (*i)->mBounds.extent);
         break;
      case JUSTIFY_CENTER:
         for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            (*i)->resize(Point2I(minX + ((deltaX - (*i)->mBounds.extent.x) >> 1), (*i)->mBounds.point.y),
                                                                                 (*i)->mBounds.extent);
         break;
      case SPACING_VERTICAL:
         {
            Vector<GuiControl *> sortedList;
            Vector<GuiControl *>::iterator k;
            for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            {
               for(k = sortedList.begin(); k != sortedList.end(); k++)
               {
                  if ((*i)->mBounds.point.y < (*k)->mBounds.point.y)
                     break;
               }
               sortedList.insert(k, *i);
            }
            S32 space = (deltaY - extentY) / (mSelectedControls.size() - 1);
            S32 curY = minY;
            for(k = sortedList.begin(); k != sortedList.end(); k++)
            {
               (*k)->resize(Point2I((*k)->mBounds.point.x, curY), (*k)->mBounds.extent); 
               curY += (*k)->mBounds.extent.y + space;
            }
         }
         break;
      case SPACING_HORIZONTAL:
         {
            Vector<GuiControl *> sortedList;
            Vector<GuiControl *>::iterator k;
            for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
            {
               for(k = sortedList.begin(); k != sortedList.end(); k++)
               {
                  if ((*i)->mBounds.point.x < (*k)->mBounds.point.x)
                     break;
               }
               sortedList.insert(k, *i);
            }
            S32 space = (deltaX - extentX) / (mSelectedControls.size() - 1);
            S32 curX = minX;
            for(k = sortedList.begin(); k != sortedList.end(); k++)
            {
               (*k)->resize(Point2I(curX, (*k)->mBounds.point.y), (*k)->mBounds.extent); 
               curX += (*k)->mBounds.extent.x + space;
            }
         }
         break;
   }
}

void GuiEditCtrl::deleteSelection(void)
{

   Vector<GuiControl *>::iterator i;
   for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
      (*i)->deleteObject();
   mSelectedControls.clear();
}

void GuiEditCtrl::loadSelection(const char* filename)
{
   if (! mCurrentAddSet)
      mCurrentAddSet = mContentControl;

   Con::executef(2, "exec", filename);
   SimSet *set;
   if(!Sim::findObject("guiClipboard", set))
      return;
      
   if(set->size())
   {
      mSelectedControls.clear();
      for(U32 i = 0; i < set->size(); i++)
      {
         GuiControl *ctrl = dynamic_cast<GuiControl *>((*set)[i]);
         if(ctrl)
         {
            mCurrentAddSet->addObject(ctrl);
            mSelectedControls.push_back(ctrl);
         }
      }
   }
   set->deleteObject();
}

void GuiEditCtrl::saveSelection(const char* filename)
{
   // if there are no selected objects, then don't save
   if (mSelectedControls.size() == 0)
      return;

   FileStream stream;
   if(!ResourceManager->openFileForWrite(stream, filename)) 
      return;
   SimSet *clipboardSet = new SimSet;
   clipboardSet->registerObject();
   Sim::getRootGroup()->addObject(clipboardSet, "guiClipboard");
   
   Vector<GuiControl *>::iterator i;
   for(i = mSelectedControls.begin(); i != mSelectedControls.end(); i++)
      clipboardSet->addObject(*i);
   
   clipboardSet->write(stream, 0);
   clipboardSet->deleteObject();
}

void GuiEditCtrl::selectAll()
{
   GuiControl::iterator i;
   if (!mCurrentAddSet)
      return;
   mSelectedControls.clear();
   for(i = mCurrentAddSet->begin(); i != mCurrentAddSet->end(); i++)
   {
      GuiControl *ctrl = dynamic_cast<GuiControl *>(*i);
      mSelectedControls.push_back(ctrl);
   }
}

void GuiEditCtrl::bringToFront()
{
   if (mSelectedControls.size() != 1)
      return;
      
   GuiControl *ctrl = *(mSelectedControls.begin());
   mCurrentAddSet->pushObjectToBack(ctrl);
}

void GuiEditCtrl::pushToBack()
{
   if (mSelectedControls.size() != 1)
      return;
      
   GuiControl *ctrl = *(mSelectedControls.begin());
   mCurrentAddSet->bringObjectToFront(ctrl);
}

bool GuiEditCtrl::onKeyDown(const GuiEvent &event)
{
   if (! mActive)
      return Parent::onKeyDown(event);

   if (!(event.modifier & SI_CTRL))
   {
      switch(event.keyCode)
      {
         case KEY_BACKSPACE:
         case KEY_DELETE:
            deleteSelection();
            return true;
      }
   }
   return false;
}

class GuiEditorRuler : public GuiControl {
   StringTableEntry refCtrl;
   typedef GuiControl Parent;
public:

   void onPreRender()
   {
      setUpdate();
   }
   void onRender(Point2I offset, const RectI &updateRect)
   {
      dglDrawRectFill(updateRect, ColorF(1,1,1,1));
      GuiScrollCtrl *ref;
      SimObject *o = Sim::findObject(refCtrl);

      //Sim::findObject(refCtrl, &ref);
      ref = dynamic_cast<GuiScrollCtrl *>(o);
      Point2I choffset(0,0);
      if(ref)
         choffset = ref->getChildPos();
      if(mBounds.extent.x > mBounds.extent.y)
      {
         // it's horizontal.
         for(U32 i = 0; i < mBounds.extent.x; i++)
         {
            S32 x = offset.x + i;
            S32 pos = i - choffset.x;
            if(!(pos % 10))
            {
               S32 start = 6;
               if(!(pos %20))
                  start = 4;
               if(!(pos % 100))
                  start = 1;
               dglDrawLine(x, offset.y + start, x, offset.y + 10, ColorF(0,0,0,1));
            }
         }
      }
      else
      {
         // it's vertical.
         for(U32 i = 0; i < mBounds.extent.y; i++)
         {
            S32 y = offset.y + i;
            S32 pos = i - choffset.y;
            if(!(pos % 10))
            {
               S32 start = 6;
               if(!(pos %20))
                  start = 4;
               if(!(pos % 100))
                  start = 1;
               dglDrawLine(offset.x + start, y, offset.x + 10, y, ColorF(0,0,0,1));
            }
         }
      }
   }
   static void initPersistFields()
   {
      Parent::initPersistFields();
      addField("refCtrl", TypeString, Offset(refCtrl, GuiEditorRuler));
   }
   DECLARE_CONOBJECT(GuiEditorRuler);
};

IMPLEMENT_CONOBJECT(GuiEditorRuler);