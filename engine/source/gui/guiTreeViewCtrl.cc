//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "gui/guiTreeViewCtrl.h"
#include "gui/guiScrollCtrl.h"
#include "console/consoleTypes.h"
#include "console/console.h"
#include "dgl/dgl.h"
#include "gui/guiTypes.h"
#include "platform/event.h"

ConsoleMethod( GuiTreeViewCtrl, open, void, 3, 3, "(SimSet obj) Set the root of the tree view to the specified object, or to the root set.")
{
   SimSet *treeRoot = NULL;
	SimObject* target = Sim::findObject(argv[2]);
   if (target)
      treeRoot = dynamic_cast<SimSet*>(target);
   if (! treeRoot)
      Sim::findObject(RootGroupId, treeRoot);
   object->setTreeRoot(treeRoot);
}

void GuiTreeViewCtrl::initPersistFields()
{
	Parent::initPersistFields();
   addField("allowMultipleSelections", TypeBool, Offset(mAllowMultipleSelections, GuiTreeViewCtrl));
   addField("recurseSets", TypeBool, Offset(mRecurseSets, GuiTreeViewCtrl));
}

GuiTreeViewCtrl::GuiTreeViewCtrl()
{
   VECTOR_SET_ASSOCIATION(mObjectList);
   

   mAllowMultipleSelections = false;
   mRecurseSets = false;
   mSize = Point2I(1, 0);
   mRootObject = NULL;
}

bool GuiTreeViewCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
   
   //get the font
   mFont = mProfile->mFont;
   
   //init the size
   mCellSize.set( 640, 11 );
   return(true);
}

void GuiTreeViewCtrl::onSleep()
{
   Parent::onSleep();
   mFont = NULL;
}

void GuiTreeViewCtrl::buildTree(SimSet *srcObj, S32 srcLevel, S32 srcParentIndex)
{
   //loop through the children of the src object
   SimSet::iterator i;
   for(i = srcObj->begin(); i != srcObj->end(); i++)
   {
      //push the child
      mObjectList.push_back(ObjNode(srcLevel, *i, i+1 == srcObj->end(), srcParentIndex));
      
      //if the child is expanded, build the tree under the child
      SimSet *g = dynamic_cast<SimSet *>(*i);
      if(g && g->isExpanded())
      {
         buildTree(g, srcLevel + 1, mObjectList.size() - 1);
      }
   }
}

void GuiTreeViewCtrl::setTreeRoot(SimSet *srcObj)
{
   if (srcObj)
   {
      mObjectList.clear();
      
      //push the root
      mObjectList.push_back(ObjNode(0, srcObj, true, -1));
      
      if(srcObj->isExpanded())
      {
         buildTree(srcObj, 1, 0);
      }
      
      //set the size
      setSize(Point2I(1, mObjectList.size()));
      
      mRootObject = srcObj;
   }
}

void GuiTreeViewCtrl::onMouseDown(const GuiEvent &event)
{
   if(! mActive)
   {
      Parent::onMouseDown(event);
      return;
   }

   Point2I pt = globalToLocalCoord(event.mousePoint);
   
   //find out which cell was hit
   Point2I cell((pt.x < 0 ? -1 : pt.x / mCellSize.x), (pt.y < 0 ? -1 : pt.y / mCellSize.y));
   if (cell.x >= 0 && cell.x < mSize.x && cell.y >= 0 && cell.y < mSize.y)
   {
      //see where in the cell the hit happened
      ObjNode *hit = &mObjectList[cell.y];
      S32 statusWidth = 11;
      
      S32 statusOffset = statusWidth * hit->level;
      
      //if we clicked on the expanded icon
      SimSet *grp = dynamic_cast<SimSet *>(hit->object);
      if (pt.x >= statusOffset && pt.x <= statusOffset + statusWidth && grp && grp->size())
      {
         grp->setExpanded(! grp->isExpanded());
         setTreeRoot(mRootObject);
      }

      //if we clicked on the object's name...
      else if (pt.x >= 2 + ((hit->level + 1) * statusWidth))
      {
      	if(!hit->object)
         	return;

			if(mAllowMultipleSelections)
         {
         	// calls 'onInspect' for the clicked object and 'onSelect/onUnselect' for
            // every object (for set recursion)
      		if(event.modifier & SI_CTRL)
         		toggleSelected(hit->object);
         	else if(event.modifier & SI_SHIFT)
         		selectObject(hit->object);
         	else if(event.modifier & SI_ALT)
               setInstantGroup(hit->object);
            else
         		setSelected(hit->object);

				//
			 	inspectObject(hit->object);
         }
         else     
         	// will not call 'onInspect'
         	setSelected(hit->object);
      }
   }
}

GuiTreeViewCtrl::ObjNode * GuiTreeViewCtrl::getHitNode(const GuiEvent & event)
{
   Point2I pt = globalToLocalCoord(event.mousePoint);
   
   //find out which cell was hit
   Point2I cell((pt.x < 0 ? -1 : pt.x / mCellSize.x), (pt.y < 0 ? -1 : pt.y / mCellSize.y));
   if (cell.x >= 0 && cell.x < mSize.x && cell.y >= 0 && cell.y < mSize.y)
   {
      //see where in the cell the hit happened
      ObjNode *hit = &mObjectList[cell.y];
      if(!hit->object)
         return(0);

      S32 statusWidth = 11;

      //if we clicked on the object's name...
      if (pt.x >= 2 + ((hit->level + 1) * statusWidth))
         return(hit);
   }
   return(0);
}

void GuiTreeViewCtrl::onRightMouseDown(const GuiEvent & event)
{
   if(!mActive)
   {
      Parent::onRightMouseDown(event);
      return;
   }

   ObjNode * hit;
   if(!(hit = getHitNode(event)))
      return;

   if(!hit->object)
      return;

   setSelected(hit->object, true);
}

void GuiTreeViewCtrl::onRightMouseUp(const GuiEvent & event)
{
   if(!mActive)
   {
      Parent::onMouseDown(event);
      return;
   }

   ObjNode * hit;
   if(!(hit = getHitNode(event)))
      return;

   if(!hit->object)
      return;

   //
   char buf1[32];
   dSprintf(buf1, sizeof(buf1), "%d %d", event.mousePoint.x, event.mousePoint.y);

   char buf2[16];
   dSprintf(buf2, sizeof(buf2), "%d", hit->object->getId());

   Con::executef(this, 3, "onContextMenu", buf1, buf2);
}

void GuiTreeViewCtrl::setInstantGroup(SimObject * obj)
{
   // make sure a group
   SimGroup * grp = dynamic_cast<SimGroup*>(obj);
   if(!grp)
      return;
      
   char buf[16];
   dSprintf(buf, sizeof(buf), "%d", grp->getId());
   Con::setVariable("instantGroup", buf);
}

void GuiTreeViewCtrl::inspectObject(SimObject * obj)
{
	char buf[16];
   dSprintf(buf, sizeof(buf), "%d", obj->getId());
   Con::executef(this, 2, "onInspect", buf);
}

void GuiTreeViewCtrl::selectObject(SimObject * obj, bool rightMouse)
{
	if(mRecurseSets)
   {
   	SimSet * set = dynamic_cast<SimSet*>(obj);
      if(set)
      	for(SimSet::iterator itr = set->begin(); itr != set->end(); itr++)
         	selectObject(*itr, rightMouse);
   }
   
   //
   obj->setSelected(true);
   
   //
   Con::executef(this, 3, "onSelect", Con::getIntArg(obj->getId()), Con::getIntArg(rightMouse));
}

void GuiTreeViewCtrl::unselectObject(SimObject * obj)
{
	if(mRecurseSets)
   {
   	SimSet * set = dynamic_cast<SimSet*>(obj);
      if(set)
      	for(SimSet::iterator itr = set->begin(); itr != set->end(); itr++)
         	unselectObject(*itr);
   }

	//	
   obj->setSelected(false);
   
   //
   char buf[16];
   dSprintf(buf, sizeof(buf), "%d", obj->getId());
   Con::executef(this, 2, "onUnselect", buf);
}

void GuiTreeViewCtrl::clearSelected()
{
	for(U32 i = 0; i < mObjectList.size(); i++)
   	if(mObjectList[i].object->isSelected())
         unselectObject(mObjectList[i].object);
}

void GuiTreeViewCtrl::toggleSelected(SimObject * obj)
{
	if(obj->isSelected())
      unselectObject(obj);
   else
      selectObject(obj);
}

void GuiTreeViewCtrl::setSelected(SimObject *selObj, bool rightMouse)
{
   clearSelected();
   selectObject(selObj, rightMouse);
}

void GuiTreeViewCtrl::onPreRender()
{
   setTreeRoot(mRootObject);
}

void GuiTreeViewCtrl::onRenderCell(Point2I offset, Point2I cell, bool, bool)
{
   Point2I cellOffset = offset;
   
   ObjNode *obj = &(mObjectList[cell.y]);
   ObjNode *prev = cell.y ? &(mObjectList[cell.y - 1]) : NULL;
   
   S32 statusWidth = 11;
   bool sel = obj->object->isSelected();
   
   //draw the background behind the selected cell
   if( sel ) {
      RectI selRect( Point2I( cellOffset.x + 2, cellOffset.y ), Point2I( mBounds.extent.x - 2, mBounds.extent.y ) );
      dglDrawRectFill( selRect, ColorI( 255, 255, 255 ) );
   }
   
   // check if instantGroup - should draw a bitmap or something else...
   const char * instantGroupName = Con::getVariable("instantGroup");
   SimGroup * instantGroup = dynamic_cast<SimGroup*>(Sim::findObject(instantGroupName));
   if(instantGroup && instantGroup->getId() == obj->object->getId())
   {
      RectI selRect(Point2I(cellOffset.x + 2, cellOffset.y), Point2I(mBounds.extent.x - 2, mBounds.extent.y));
      dglDrawRectFill(selRect, ColorI(200, 200, 200));
   }
   
   // Do Status
   SimSet *grp = dynamic_cast<SimSet *>(obj->object);
   
   S32 render = 1;
   
   if( !( grp && grp->size() ) )
      if( !prev || obj->level == 0 )
         if( obj->lastInGroup )
            render = 0;
            
   if( render ){
      // Get our points
      Point2I boxStart( cellOffset.x + ( obj->level * statusWidth ), cellOffset.y );
         
      boxStart.x += 2;
      boxStart.y += 1;
         
      Point2I boxEnd = Point2I( boxStart );
         
      boxEnd.x += 8;
      boxEnd.y += 8;
      
      // Start drawing stuff
      if( grp && grp->size() ) { // If we need a box...
         dglDrawRectFill( boxStart, boxEnd, mProfile->mFillColor ); // Box background
         dglDrawRect( boxStart, boxEnd, mProfile->mFontColor );     // Border

         // Cross line
         dglDrawLine( boxStart.x + 2, boxStart.y + 4, boxStart.x + 7, boxStart.y + 4, mProfile->mFontColor );

         if( !grp->isExpanded() ) // If it's a [+] draw down line
            dglDrawLine( boxStart.x + 4, boxStart.y + 2, boxStart.x + 4, boxStart.y + 7, mProfile->mFontColor );
      }
      else {
         // Draw horizontal line
         dglDrawLine( boxStart.x + 4, boxStart.y + 4, boxStart.x + 9, boxStart.y + 4, mProfile->mFontColor );
         
         if( !obj->lastInGroup ) // If it's a continuing one, draw a long down line
            dglDrawLine( boxStart.x + 4, boxStart.y - 6, boxStart.x + 4, boxStart.y + 10, mProfile->mFontColor );
         else  // Otherwise, just a small one
            dglDrawLine( boxStart.x + 4, boxStart.y - 2, boxStart.x + 4, boxStart.y + 4, mProfile->mFontColor );  
      }
   }
   
   //draw in all the required continuation lines
   S32 parent = obj->parentIndex;
   
   while( parent != -1 ) {
      ObjNode *p = &(mObjectList[parent]);
      
      if( !p->lastInGroup ) {
         dglDrawLine( cellOffset.x + ( p->level * statusWidth ) + 6, 
                      cellOffset.y - 2, 
                      cellOffset.x + ( p->level * statusWidth ) + 6, 
                      cellOffset.y + 11, 
                      mProfile->mFontColor );
      }
      parent = p->parentIndex;
   }
   

   // Determine the prefix
   char prefix[16]; // Max is 3 + 1 + 3 + 1 = 8, so 16 for safety

   bool isH = obj->object->isHidden();
   bool isL = obj->object->isLocked();

   dSprintf(prefix, sizeof(prefix), "%s%s%s%s", (isH ? "(H)" : ""), (isH && isL ? " " : ""), (isL ? "(L)" : ""), (isL || isH ? " " : ""));

   // Draw the name...
   char buf[256];
   dSprintf(buf, sizeof(buf), "%s%d: %s - %s",  prefix, 
            obj->object->getId(), 
            obj->object->getName() ? obj->object->getName() : "", 
            obj->object->getClassName()
           );
   
   dglSetBitmapModulation( sel ? mProfile->mFontColorHL : mProfile->mFontColor );
   dglDrawText( mFont, Point2I( cellOffset.x + 2 + (obj->level + 1) * statusWidth, cellOffset.y ), 
                buf, mProfile->mFontColors );
}
