//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "dgl/gFont.h"
#include "dgl/dgl.h"
#include "gui/guiTypes.h"
#include "dgl/gBitmap.h"
#include "dgl/gTexManager.h"

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //

GuiCursor::GuiCursor()
{
   mHotSpot.set(0,0);
   mExtent.set(1,1);
}

GuiCursor::~GuiCursor()
{
}

void GuiCursor::initPersistFields()
{
   Parent::initPersistFields();
   addField("hotSpot",     TypePoint2I,   Offset(mHotSpot, GuiCursor));
   addField("bitmapName",  TypeFilename,  Offset(mBitmapName, GuiCursor));
}

bool GuiCursor::onAdd()
{
   if(!Parent::onAdd())
      return false;

   Sim::getGuiDataGroup()->addObject(this);
   
   return true;
}

void GuiCursor::onRemove()
{
   Parent::onRemove();
}

void GuiCursor::render(const Point2I &pos)
{
   if (!mTextureHandle && mBitmapName && mBitmapName[0])
   {
      mTextureHandle = TextureHandle(mBitmapName, BitmapTexture);
      if(!mTextureHandle)
         return;
      mExtent.set(mTextureHandle.getWidth(), mTextureHandle.getHeight());
   }

   dglClearBitmapModulation();
   dglDrawBitmap(mTextureHandle, pos);
}   



static EnumTable::Enums alignEnums[] = 
{
   { GuiControlProfile::LeftJustify,          "left"      },
   { GuiControlProfile::CenterJustify,        "center"    },
   { GuiControlProfile::RightJustify,         "right"     }
};
static EnumTable gAlignTable(3, &alignEnums[0]); 

StringTableEntry GuiControlProfile::sFontCacheDirectory = "";

GuiControlProfile::GuiControlProfile(void) :
   mFontColor(mFontColors[BaseColor]),
   mFontColorHL(mFontColors[ColorHL]),
   mFontColorNA(mFontColors[ColorNA]),
   mFontColorSEL(mFontColors[ColorSEL])
{
   mRefCount = 0;
   mBitmapArrayRects.clear();
   mBorderThickness = 1;
   mMouseOverSelected = false;

   GuiControlProfile *def = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiDefaultProfile"));
   if (def)
   {
      mTabable       = def->mTabable;
      mCanKeyFocus   = def->mCanKeyFocus;

      mOpaque        = def->mOpaque;
      mFillColor     = def->mFillColor;
      mFillColorHL   = def->mFillColorHL;
      mFillColorNA   = def->mFillColorNA;
   
      mBorder        = def->mBorder;
      mBorderThickness = def->mBorderThickness;
      mBorderColor   = def->mBorderColor;
      mBorderColorHL = def->mBorderColorHL;
      mBorderColorNA = def->mBorderColorNA;
   
      // default font
      mFontType      = def->mFontType;
      mFontSize      = def->mFontSize;

      for(U32 i = 0; i < 10; i++)
         mFontColors[i] = def->mFontColors[i];

      // default bitmap
      mBitmapName    = def->mBitmapName;
      mTextOffset    = def->mTextOffset;

      // default sound
      mSoundButtonDown = def->mSoundButtonDown;
      mSoundButtonOver = def->mSoundButtonOver;

      //used by GuiTextCtrl
      mModal         = def->mModal;
      mAlignment     = def->mAlignment;
      mAutoSizeWidth = def->mAutoSizeWidth;
      mAutoSizeHeight= def->mAutoSizeHeight;
      mReturnTab     = def->mReturnTab;
      mNumbersOnly   = def->mNumbersOnly;
      mCursorColor   = def->mCursorColor;
   }
}

GuiControlProfile::~GuiControlProfile()
{
}


void GuiControlProfile::initPersistFields()
{
   Parent::initPersistFields();
   
   addField("tab",           TypeBool,       Offset(mTabable, GuiControlProfile));
   addField("canKeyFocus",   TypeBool,       Offset(mCanKeyFocus, GuiControlProfile));
   addField("mouseOverSelected", TypeBool,   Offset(mMouseOverSelected, GuiControlProfile));
   
   addField("modal",         TypeBool,       Offset(mModal, GuiControlProfile));
   addField("opaque",        TypeBool,       Offset(mOpaque, GuiControlProfile));
   addField("fillColor",     TypeColorI,     Offset(mFillColor, GuiControlProfile));
   addField("fillColorHL",   TypeColorI,     Offset(mFillColorHL, GuiControlProfile));
   addField("fillColorNA",   TypeColorI,     Offset(mFillColorNA, GuiControlProfile));
   addField("border",        TypeS32,        Offset(mBorder, GuiControlProfile));
   addField("borderThickness",TypeS32,       Offset(mBorderThickness, GuiControlProfile));
   addField("borderColor",   TypeColorI,     Offset(mBorderColor, GuiControlProfile));
   addField("borderColorHL", TypeColorI,     Offset(mBorderColorHL, GuiControlProfile));
   addField("borderColorNA", TypeColorI,     Offset(mBorderColorNA, GuiControlProfile));
   
   addField("fontType",      TypeString,     Offset(mFontType, GuiControlProfile));
   addField("fontSize",      TypeS32,        Offset(mFontSize, GuiControlProfile));
   addField("fontColors",    TypeColorI,     Offset(mFontColors, GuiControlProfile), 10);
   addField("fontColor",     TypeColorI,     Offset(mFontColors[BaseColor], GuiControlProfile));
   addField("fontColorHL",   TypeColorI,     Offset(mFontColors[ColorHL], GuiControlProfile));
   addField("fontColorNA",   TypeColorI,     Offset(mFontColors[ColorNA], GuiControlProfile));
   addField("fontColorSEL",  TypeColorI,     Offset(mFontColors[ColorSEL], GuiControlProfile));
   addField("fontColorLink", TypeColorI,     Offset(mFontColors[ColorUser0], GuiControlProfile));
   addField("fontColorLinkHL", TypeColorI,     Offset(mFontColors[ColorUser1], GuiControlProfile));
   
   addField("justify",       TypeEnum,       Offset(mAlignment, GuiControlProfile), 1, &gAlignTable);
   addField("textOffset",    TypePoint2I,    Offset(mTextOffset, GuiControlProfile));
   addField("autoSizeWidth", TypeBool,       Offset(mAutoSizeWidth, GuiControlProfile));
   addField("autoSizeHeight",TypeBool,       Offset(mAutoSizeHeight, GuiControlProfile));
   addField("returnTab",     TypeBool,       Offset(mReturnTab, GuiControlProfile));
   addField("numbersOnly",   TypeBool,       Offset(mNumbersOnly, GuiControlProfile));
   addField("cursorColor",   TypeColorI,     Offset(mCursorColor, GuiControlProfile));
   
   addField("bitmap",        TypeFilename,   Offset(mBitmapName, GuiControlProfile));

   addField("soundButtonDown", TypeAudioProfilePtr,  Offset(mSoundButtonDown, GuiControlProfile));
   addField("soundButtonOver", TypeAudioProfilePtr,  Offset(mSoundButtonOver, GuiControlProfile));
}

bool GuiControlProfile::onAdd()
{
   if(!Parent::onAdd())
      return false;

   Sim::getGuiDataGroup()->addObject(this);
      
   return true;
}

S32 GuiControlProfile::constructBitmapArray()
{
   if(mBitmapArrayRects.size())
      return mBitmapArrayRects.size();

   GBitmap *bmp = mTextureHandle.getBitmap();

   //get the separator color
   ColorI sepColor;
   if ( !bmp || !bmp->getColor( 0, 0, sepColor ) )
	{
		AssertFatal( false, "createBitmapArray - failed to get sepearator color!" );
      return 0;
	}
   
   //now loop through all the scroll pieces, and find the bounding rectangle for each piece in each state
   S32 curY = 0;

   // ascertain the height of this row...
   ColorI color;
   mBitmapArrayRects.clear();
   while(curY < bmp->getHeight())
   {
      // skip any sep colors
      bmp->getColor( 0, curY, color);
      if(color == sepColor)
      {
         curY++;
         continue;
      }
      // ok, process left to right, grabbing bitmaps as we go...
      S32 curX = 0;
      while(curX < bmp->getWidth())
      {
         bmp->getColor(curX, curY, color);
         if(color == sepColor)
         {
            curX++;
            continue;
         }
         S32 startX = curX;
         while(curX < bmp->getWidth())
         {
            bmp->getColor(curX, curY, color);
            if(color == sepColor)
               break;
            curX++;
         }
         S32 stepY = curY;
         while(stepY < bmp->getHeight())
         {
            bmp->getColor(startX, stepY, color);
            if(color == sepColor)
               break;
            stepY++;
         }
         mBitmapArrayRects.push_back(RectI(startX, curY, curX - startX, stepY - curY));
      }
      // ok, now skip to the next seperation color on column 0
      while(curY < bmp->getHeight())
      {
         bmp->getColor(0, curY, color);
         if(color == sepColor)
            break;
         curY++;
      }
   }
   return mBitmapArrayRects.size();
}

void GuiControlProfile::incRefCount()
{
   if(!mRefCount++)
   {
      sFontCacheDirectory = Con::getVariable("$GUI::fontCacheDirectory");

      //verify the font
      mFont = GFont::create(mFontType, mFontSize, sFontCacheDirectory);
      if (mFont.isNull())
         Con::errorf("Failed to load/create profile font (%s/%d)", mFontType, mFontSize);
      
      //verify the bitmap
      mTextureHandle = TextureHandle(mBitmapName, BitmapKeepTexture);
      if (!(bool)mTextureHandle)
         Con::errorf("Failed to load profile bitmap (%s)",mBitmapName);
   }
}

void GuiControlProfile::decRefCount()
{
   AssertFatal(mRefCount, "GuiControlProfile::decRefCount: zero ref count");
   if(!mRefCount)
      return;

   if(!--mRefCount)
   {
      mFont = NULL;
      mTextureHandle = NULL;
   }
}

static void setDataTypeGuiProfile(void *dptr, S32 argc, const char **argv, EnumTable *, BitSet32)
{
   GuiControlProfile *profile = NULL;
   if(argc == 1)
      Sim::findObject(argv[0], profile);
   
   AssertWarn(profile != NULL, avar("GuiControlProfile: requested gui profile (%s) does not exist.", argv[0]));
   if(!profile)
      profile = dynamic_cast<GuiControlProfile*>(Sim::findObject("GuiDefaultProfile"));

   AssertFatal(profile != NULL, avar("GuiControlProfile: unable to find specified profile (%s) and GuiDefaultProfile does not exist!", argv[0]));
   
   GuiControlProfile **obj = (GuiControlProfile **)dptr;
   if((*obj) == profile)
      return;
      
   if(*obj)
      (*obj)->decRefCount();

   *obj = profile;
   (*obj)->incRefCount();
}

static const char *getDataTypeGuiProfile(void *dptr, EnumTable *, BitSet32)
{
   static char returnBuffer[256];
   
   GuiControlProfile **obj = (GuiControlProfile**)dptr;
   dSprintf(returnBuffer, sizeof(returnBuffer), "%s", *obj ? (*obj)->getName() : "");
   return returnBuffer;
}

void RegisterGuiTypes(void)
{
   Con::registerType("GuiProfile", TypeGuiProfile, sizeof(GuiControlProfile *), getDataTypeGuiProfile,  setDataTypeGuiProfile);
}
