//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/consoleTypes.h"
#include "console/console.h"
#include "dgl/dgl.h"
#include "gui/guiTextListCtrl.h"
#include "gui/guiScrollCtrl.h"

static int sortColumn;
static bool sIncreasing;

static const char *getColumn(const char *text)
{
   int ct = sortColumn;
   while(ct--)
   {
      text = dStrchr(text, '\t');
      if(!text)
         return "";
      text++;
   }
   return text;
}

static S32 QSORT_CALLBACK textCompare( const void* a, const void* b )
{
   GuiTextListCtrl::Entry *ea = (GuiTextListCtrl::Entry *) (a);
   GuiTextListCtrl::Entry *eb = (GuiTextListCtrl::Entry *) (b);
   S32 result = dStricmp( getColumn( ea->text ), getColumn( eb->text ) );
   return ( sIncreasing ? result : -result );
}

static S32 QSORT_CALLBACK numCompare(const void *a,const void *b)
{
   GuiTextListCtrl::Entry *ea = (GuiTextListCtrl::Entry *) (a);
   GuiTextListCtrl::Entry *eb = (GuiTextListCtrl::Entry *) (b);
   const char* aCol = getColumn( ea->text );
   const char* bCol = getColumn( eb->text );
   F32 result = dAtof(aCol) - dAtof(bCol);
   S32 res = result < 0 ? -1 : (result > 0 ? 1 : 0);

   return ( sIncreasing ? res : -res );
}

GuiTextListCtrl::GuiTextListCtrl()
{
   VECTOR_SET_ASSOCIATION(mList);
   VECTOR_SET_ASSOCIATION(mColumnOffsets);

   mActive = true;
   mEnumerate = false;
   mResizeCell = true;
   mSize.set(1, 0);
   mColumnOffsets.push_back(0);
   mFitParentWidth = true;
   mClipColumnText = false;
}

void GuiTextListCtrl::initPersistFields()
{
   Parent::initPersistFields();
   addField("enumerate",               TypeBool, Offset(mEnumerate, GuiTextListCtrl));
   addField("resizeCell",              TypeBool, Offset(mResizeCell, GuiTextListCtrl));
   addField("columns",                 TypeS32Vector, Offset(mColumnOffsets, GuiTextListCtrl));  
   addField("fitParentWidth",          TypeBool, Offset(mFitParentWidth, GuiTextListCtrl));
   addField("clipColumnText",          TypeBool, Offset(mClipColumnText, GuiTextListCtrl));
}

ConsoleMethod(GuiTextListCtrl, getSelectedId, S32, 2, 2, "Get the ID of the currently selected item.")
{
   return object->getSelectedId();
}

ConsoleMethod( GuiTextListCtrl, setSelectedById, void, 3, 3, "(int id)"
              "Finds the specified entry by id, then marks its row as selected.")
{
   S32 index = object->findEntryById(dAtoi(argv[2]));
   if(index < 0)
      return ; 

   object->setSelectedCell(Point2I(0, index));
}

ConsoleMethod( GuiTextListCtrl, setSelectedRow, void, 3, 3, "(int rowNum)"
              "Selects the specified row.")
{
   object->setSelectedCell( Point2I( 0, dAtoi( argv[2] ) ) );
} 

ConsoleMethod( GuiTextListCtrl, clearSelection, void, 2, 2, "Set the selection to nothing.")
{
   object->setSelectedCell(Point2I(-1, -1));
}

ConsoleMethod(GuiTextListCtrl, addRow, S32, 4, 5, "(int id, string text, int index=0)"
              "Returns row number of the new item.")
{
   S32 ret = object->mList.size();
   if(argc < 5)
      object->addEntry(dAtoi(argv[2]), argv[3]);
   else 
      object->insertEntry(dAtoi(argv[2]), argv[3], dAtoi(argv[4]));
      
   return ret;
}

ConsoleMethod( GuiTextListCtrl, setRowById, void, 4, 4, "(int id, string text)")
{
   object->setEntry(dAtoi(argv[2]), argv[3]);
}

ConsoleMethod( GuiTextListCtrl, sort, void, 3, 4, "(int columnID, bool increasing=false)"
              "Performs a standard (alphabetical) sort on the values in the specified column.")
{
   if ( argc == 3 )
      object->sort(dAtoi(argv[2]));
   else
      object->sort( dAtoi( argv[2] ), dAtob( argv[3] ) );
}

ConsoleMethod(GuiTextListCtrl, sortNumerical, void, 3, 4, "(int columnID, bool increasing=false)"
              "Perform a numerical sort on the values in the specified column.")
{
   if ( argc == 3 )
      object->sortNumerical( dAtoi( argv[2] ) );
   else
      object->sortNumerical( dAtoi( argv[2] ), dAtob( argv[3] ) );
}

ConsoleMethod( GuiTextListCtrl, clear, void, 2, 2, "Clear the list.")
{
   object->clear();
}

ConsoleMethod( GuiTextListCtrl, rowCount, S32, 2, 2, "Get the number of rows.")
{
   return object->getNumEntries();
}

ConsoleMethod( GuiTextListCtrl, getRowId, S32, 3, 3, "(int index)"
              "Get the row ID for an index.")
{
   U32 index = dAtoi(argv[2]);
   if(index >= object->getNumEntries())
      return -1;
      
   return object->mList[index].id;
}

ConsoleMethod( GuiTextListCtrl, getRowTextById, const char*, 3, 3, "(int id)"
              "Get the text of a row with the specified id.")
{
   S32 index = object->findEntryById(dAtoi(argv[2]));
   if(index < 0)
      return "";
   return object->mList[index].text;
}

ConsoleMethod( GuiTextListCtrl, getRowNumById, S32, 3, 3, "(int id)"
              "Get the row number for a specified id.")
{
   S32 index = object->findEntryById(dAtoi(argv[2]));
   if(index < 0)
      return -1;
   return index;
}

ConsoleMethod( GuiTextListCtrl, getRowText, const char*, 3, 3, "(int index)"
              "Get the text of the row with the specified index.")
{
   U32 index = dAtoi(argv[2]);
   if(index < 0 || index >= object->mList.size())
      return "";
   return object->mList[index].text;
}

ConsoleMethod( GuiTextListCtrl, removeRowById, void, 3, 3,"(int id)"
              "Remove row with the specified id.")
{
   object->removeEntry(dAtoi(argv[2]));
}

ConsoleMethod( GuiTextListCtrl, removeRow, void, 3, 3, "(int index)"
              "Remove a row from the table, based on its index.")
{
   U32 index = dAtoi(argv[2]);
   object->removeEntryByIndex(index);
}

ConsoleMethod( GuiTextListCtrl, scrollVisible, void, 3, 3, "(int rowNum)"
              "Scroll so the specified row is visible.")
{
   object->scrollCellVisible(Point2I(0, dAtoi(argv[2])));
}

ConsoleMethod( GuiTextListCtrl, findTextIndex, S32, 3, 3, "(string needle)"
              "Find needle in the list, and return the row number it was found in.")
{
   return( object->findEntryByText( argv[2] ) );
}

ConsoleMethod( GuiTextListCtrl, setRowActive, void, 4, 4, "(int rowNum, bool active)"
              "Mark a specified row as active/not.")
{
   object->setEntryActive( U32( dAtoi( argv[2] ) ), dAtob( argv[3] ) );
}

ConsoleMethod( GuiTextListCtrl, isRowActive, bool, 3, 3, "(int rowNum)"
              "Is the specified row currently active?")
{
   return( object->isEntryActive( U32( dAtoi( argv[2] ) ) ) );
}

bool GuiTextListCtrl::onWake()
{
   if(!Parent::onWake())
      return false;
   
   setSize(mSize);
   return true;
}

U32 GuiTextListCtrl::getSelectedId()
{
   if (mSelectedCell.y == -1)
      return InvalidId;

   return mList[mSelectedCell.y].id;
}

void GuiTextListCtrl::onCellSelected(Point2I cell)
{
   Con::executef(this, 3, "onSelect", Con::getIntArg(mList[cell.y].id), mList[cell.y].text);
   
   if (mConsoleCommand[0])
      Con::evaluate(mConsoleCommand, false);
}

void GuiTextListCtrl::onRenderCell(Point2I offset, Point2I cell, bool selected, bool mouseOver)
{
   if ( mList[cell.y].active )
   {
      if (selected || (mProfile->mMouseOverSelected && mouseOver))
      {
         dglDrawRectFill(RectI(offset.x, offset.y, mCellSize.x, mCellSize.y), mProfile->mFillColorHL);
         dglSetBitmapModulation(mProfile->mFontColorHL);
      }
      else
         dglSetBitmapModulation(mouseOver ? mProfile->mFontColorHL : mProfile->mFontColor);
   }
   else
      dglSetBitmapModulation( mProfile->mFontColorNA );
   
   const char *text = mList[cell.y].text;
   for(U32 index = 0; index < mColumnOffsets.size(); index++)
   {
      const char *nextCol = dStrchr(text, '\t');
      if(mColumnOffsets[index] >= 0)
      {
         dsize_t slen;
         if(nextCol)
            slen = nextCol - text;
         else
            slen = dStrlen(text);

         Point2I pos(offset.x + 4 + mColumnOffsets[index], offset.y);

         RectI saveClipRect;
         bool clipped = false;

         if(mClipColumnText && (index != (mColumnOffsets.size() - 1)))
         {
            saveClipRect = dglGetClipRect();
            
            RectI clipRect(pos, Point2I(mColumnOffsets[index+1] - mColumnOffsets[index] - 4, mCellSize.y));
            if(clipRect.intersect(saveClipRect))
            {
               clipped = true;
               dglSetClipRect(clipRect);
            }
         }

         dglDrawTextN(mFont, pos, text, slen, mProfile->mFontColors);

         if(clipped)
            dglSetClipRect(saveClipRect);
      }
      if(!nextCol)
         break;
      text = nextCol+1;
   }
}   

U32 GuiTextListCtrl::getRowWidth(Entry *row)
{
   U32 width = 1;
   const char *text = row->text;
   for(U32 index = 0; index < mColumnOffsets.size(); index++)
   {
      const char *nextCol = dStrchr(text, '\t');
      U32 textWidth;
      if(nextCol)
         textWidth = mFont->getStrNWidth(text, nextCol - text);
      else
         textWidth = mFont->getStrWidth(text);
      if(mColumnOffsets[index] >= 0)
         width = getMax(width, mColumnOffsets[index] + textWidth);
      if(!nextCol)
         break;
      text = nextCol+1;
   }
   return width;
}

void GuiTextListCtrl::insertEntry(U32 id, const char *text, S32 index)
{
   Entry e;
   e.text = dStrdup(text);
   e.id = id;
   e.active = true;
   if(!mList.size())
      mList.push_back(e);
   else
   {
      if(index > mList.size())
         index = mList.size();
      mList.insert(&mList[index],e);
   }
   setSize(Point2I(1, mList.size()));
}

void GuiTextListCtrl::addEntry(U32 id, const char *text)
{
   Entry e;
   e.text = dStrdup(text);
   e.id = id;
   e.active = true;
   mList.push_back(e);
   setSize(Point2I(1, mList.size()));
}

void GuiTextListCtrl::setEntry(U32 id, const char *text)
{
   S32 e = findEntryById(id);
   if(e == -1)
      addEntry(id, text);
   else
   {
      dFree(mList[e].text);
      mList[e].text = dStrdup(text);

      // Still have to call this to make sure cells are wide enough for new values:
      setSize( Point2I( 1, mList.size() ) );
   }
   setUpdate();
}

void GuiTextListCtrl::setEntryActive(U32 id, bool active)
{
   S32 index = findEntryById( id );
   if ( index == -1 )
      return;

   if ( mList[index].active != active )
   {
      mList[index].active = active;

      // You can't have an inactive entry selected...
      if ( !active && mSelectedCell.y >= 0 && mSelectedCell.y < mList.size() 
           && mList[mSelectedCell.y].id == id )
         setSelectedCell( Point2I( -1, -1 ) );
   
      setUpdate();
   }
}

S32 GuiTextListCtrl::findEntryById(U32 id)
{
   for(U32 i = 0; i < mList.size(); i++)
      if(mList[i].id == id)
         return i;
   return -1;
}

S32 GuiTextListCtrl::findEntryByText(const char *text)
{
   for(U32 i = 0; i < mList.size(); i++)
      if(!dStricmp(mList[i].text, text))
         return i;
   return -1;
}

bool GuiTextListCtrl::isEntryActive(U32 id)
{
   S32 index = findEntryById( id );
   if ( index == -1 )
      return( false );

   return( mList[index].active );
}

void GuiTextListCtrl::setSize(Point2I newSize)
{
   mSize = newSize;

   if ( bool( mFont ) )
   {
      if ( mSize.x == 1 && mFitParentWidth )
      {
         GuiScrollCtrl* parent = dynamic_cast<GuiScrollCtrl *>(getParent());
         if ( parent )
            mCellSize.x = parent->getContentExtent().x;
      }
      else
      {
         // Find the maximum width cell:
         S32 maxWidth = 1;
         for ( U32 i = 0; i < mList.size(); i++ )
         {
            U32 rWidth = getRowWidth( &mList[i] );
            if ( rWidth > maxWidth )
               maxWidth = rWidth;
         }
         
         mCellSize.x = maxWidth + 8;
      }

      mCellSize.y = mFont->getHeight() + 2;
   }
   
   Point2I newExtent( newSize.x * mCellSize.x + mHeaderDim.x, newSize.y * mCellSize.y + mHeaderDim.y );
   resize( mBounds.point, newExtent );
}

void GuiTextListCtrl::clear()
{
   while (mList.size())
      removeEntry(mList[0].id);
      
   mMouseOverCell.set( -1, -1 );
   setSelectedCell(Point2I(-1, -1));
} 

void GuiTextListCtrl::sort(U32 column, bool increasing)
{
   if (getNumEntries() < 2)
      return;
   sortColumn = column;
   sIncreasing = increasing;
   dQsort((void *)&(mList[0]), mList.size(), sizeof(Entry), textCompare);
}  

void GuiTextListCtrl::sortNumerical( U32 column, bool increasing )
{
   if ( getNumEntries() < 2 )
      return;

   sortColumn = column;
   sIncreasing = increasing;
   dQsort( (void*) &( mList[0] ), mList.size(), sizeof( Entry ), numCompare );
}  

void GuiTextListCtrl::onRemove()
{
   clear();
   Parent::onRemove();
}   

U32 GuiTextListCtrl::getNumEntries()
{
   return mList.size();
}

void GuiTextListCtrl::removeEntryByIndex(S32 index)
{
   if(index < 0 || index >= mList.size())
      return;
   dFree(mList[index].text);
   mList.erase(index);

   setSize(Point2I( 1, mList.size()));
   setSelectedCell(Point2I(-1, -1));
}

void GuiTextListCtrl::removeEntry(U32 id)
{
   S32 index = findEntryById(id);
   removeEntryByIndex(index);
}                                

const char *GuiTextListCtrl::getSelectedText()
{
   if (mSelectedCell.y == -1)
      return NULL;

   return mList[mSelectedCell.y].text;
}

const char *GuiTextListCtrl::getScriptValue()
{
   return getSelectedText();
}

void GuiTextListCtrl::setScriptValue(const char *val)
{
   S32 e = findEntryByText(val);
   if(e == -1)
      setSelectedCell(Point2I(-1, -1));
   else
      setSelectedCell(Point2I(0, e));
}

bool GuiTextListCtrl::onKeyDown( const GuiEvent &event )
{
   //if this control is a dead end, make sure the event stops here
   if ( !mVisible || !mActive || !mAwake )
      return true;

   if ( event.keyCode == KEY_RETURN )
   { 
      if ( mAltConsoleCommand[0] )
         Con::evaluate( mAltConsoleCommand, false );
      return( true );
   }

   if ( event.keyCode == KEY_DELETE && ( mSelectedCell.y >= 0 && mSelectedCell.y < mList.size() ) )
   {
      Con::executef( this, 2, "onDeleteKey", Con::getIntArg( mList[mSelectedCell.y].id ) );
      return( true );
   }

   return( Parent::onKeyDown( event ) );  
}

