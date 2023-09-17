//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/console.h"
#include "console/consoleTypes.h"
#include "dgl/dgl.h"

#include "gui/guiBitmapCtrl.h"



GuiBitmapCtrl::GuiBitmapCtrl(void)
{
   mBitmapName = StringTable->insert("");
	startPoint.set(0, 0);
	mWrap = false;
}


void GuiBitmapCtrl::initPersistFields()
{
   Parent::initPersistFields();
   addGroup("Misc");		// MM: Added Group Header.
   addField("bitmap", TypeFilename, Offset(mBitmapName, GuiBitmapCtrl));
   addField("wrap",   TypeBool,     Offset(mWrap,       GuiBitmapCtrl));
   endGroup("Misc");		// MM: Added Group Footer.
}

ConsoleMethod( GuiBitmapCtrl, setValue, void, 4, 4, "(int xAxis, int yAxis)"
              "Set the offset of the bitmap.")
{
   object->setValue(dAtoi(argv[2]), dAtoi(argv[3]));
}

ConsoleMethod( GuiBitmapCtrl, setBitmap, void, 3, 4, "(string filename, bool resize)"
              "Set the bitmap displayed in the control. Note that it is limited in size, to 256x256.")
{
	if (argc == 4)
		object->setBitmap(argv[2], dAtob(argv[3]));
	else 
        object->setBitmap(argv[2]);
}

bool GuiBitmapCtrl::onWake()
{
   if (! Parent::onWake())
      return false;
   setActive(true);
   setBitmap(mBitmapName);
   return true;
}

void GuiBitmapCtrl::onSleep()
{
   mTextureHandle = NULL;
   Parent::onSleep();
}

//-------------------------------------
void GuiBitmapCtrl::inspectPostApply()
{
   // if the extent is set to (0,0) in the gui editor and appy hit, this control will
   // set it's extent to be exactly the size of the bitmap (if present)
   Parent::inspectPostApply();

   if (!mWrap && (mBounds.extent.x == 0) && (mBounds.extent.y == 0) && mTextureHandle)
   {
      TextureObject *texture = (TextureObject *) mTextureHandle;
      mBounds.extent.x = texture->bitmapWidth;
      mBounds.extent.y = texture->bitmapHeight;
   }
}

void GuiBitmapCtrl::setBitmap(const char *name, bool resize)
{
   mBitmapName = StringTable->insert(name);
   if (*mBitmapName) {
      mTextureHandle = TextureHandle(mBitmapName, BitmapTexture, true);

      // Resize the control to fit the bitmap
      if (resize) {
         TextureObject* texture = (TextureObject *) mTextureHandle;
         mBounds.extent.x = texture->bitmapWidth;
         mBounds.extent.y = texture->bitmapHeight;
         Point2I extent = getParent()->getExtent();
         parentResized(extent,extent);
      }
   }
   else
      mTextureHandle = NULL;
   setUpdate();
}   


void GuiBitmapCtrl::setBitmap(const TextureHandle &handle, bool resize)
{
   mTextureHandle = handle;   

   // Resize the control to fit the bitmap
   if (resize) {
      TextureObject* texture = (TextureObject *) mTextureHandle;
      mBounds.extent.x = texture->bitmapWidth;
      mBounds.extent.y = texture->bitmapHeight;
      Point2I extent = getParent()->getExtent();
      parentResized(extent,extent);
   }
}   


void GuiBitmapCtrl::onRender(Point2I offset, const RectI &updateRect)
{
   if (mTextureHandle)
   {
      dglClearBitmapModulation();
		if(mWrap)
		{
 			TextureObject* texture = (TextureObject *) mTextureHandle;
			RectI srcRegion;
			RectI dstRegion;
			float xdone = ((float)mBounds.extent.x/(float)texture->bitmapWidth)+1;
			float ydone = ((float)mBounds.extent.y/(float)texture->bitmapHeight)+1;

			int xshift = startPoint.x%texture->bitmapWidth;
			int yshift = startPoint.y%texture->bitmapHeight;
			for(int y = 0; y < ydone; ++y)
				for(int x = 0; x < xdone; ++x)
				{
		 			srcRegion.set(0,0,texture->bitmapWidth,texture->bitmapHeight);
  					dstRegion.set( ((texture->bitmapWidth*x)+offset.x)-xshift,
								      ((texture->bitmapHeight*y)+offset.y)-yshift,
								      texture->bitmapWidth,	
								      texture->bitmapHeight);
   				dglDrawBitmapStretchSR(texture,dstRegion, srcRegion, false);
				}
		}
		else
      {
         RectI rect(offset, mBounds.extent);
	      dglDrawBitmapStretch(mTextureHandle, rect);
      }
   }

   if (mProfile->mBorder || !mTextureHandle)
   {
      RectI rect(offset.x, offset.y, mBounds.extent.x, mBounds.extent.y);
      dglDrawRect(rect, mProfile->mBorderColor);
   }

   renderChildControls(offset, updateRect);
}

void GuiBitmapCtrl::setValue(S32 x, S32 y)
{
   if (mTextureHandle)
   {
		TextureObject* texture = (TextureObject *) mTextureHandle;
		x+=texture->bitmapWidth/2;
		y+=texture->bitmapHeight/2;
  	}
  	while (x < 0)
  		x += 256;
  	startPoint.x = x % 256;
  				   
  	while (y < 0)
  		y += 256;
  	startPoint.y = y % 256;
}
