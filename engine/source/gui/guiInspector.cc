//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "console/consoleTypes.h"
#include "console/console.h"
#include "dgl/dgl.h"
#include "gui/guiTypes.h"
#include "gui/guiTextCtrl.h"
#include "gui/guiTextEditCtrl.h"
#include "gui/guiInspector.h"
#include "gui/guiCheckBoxCtrl.h"
#include "gui/guiPopUpCtrl.h"
#include "platform/event.h"

// Datablocks types... need to remove this game dependency
#include "game/gameBase.h"
#include "game/projectile.h"
#include "game/debris.h"
#include "game/fx/explosion.h"
#include "game/fx/particleEngine.h"
#include "game/fx/splash.h"

#define NULL_STRING     "<NULL>"

const bool UseFieldSeperators = false;


GuiInspector::GuiInspector()
{
   mEditControlOffset = 5;
   mEntryHeight = 16;
   mTextExtent = 80;
   mEntrySpacing = 2;
   mMaxMenuExtent = 80;
   mUseFieldGrouping = false;
}

void GuiInspector::onRemove()
{
   mTarget = 0;
   while(size())
      first()->deleteObject();
   Parent::onRemove();
}

// we have to override GuiInspector's write so that it doesn't write out
// its subobjects (ie, the inspect info controls) - so we just
// pass by SimSet::write and just write out this object's fields.

void GuiInspector::write(Stream &stream, U32 tabStop, U32 flags)
{
   SimObject::write(stream, tabStop, flags);
}

//------------------------------------------------------------------------------
static S32 QSORT_CALLBACK stringCompare(const void *a,const void *b)
{
   StringTableEntry sa = *(StringTableEntry*)a;
   StringTableEntry sb = *(StringTableEntry*)b;
   return(dStricmp(sa, sb));
}

// MM:  This routines' job is to dynamically create controls that reflect the
//		selected objects' fields.  We split the processing of this into two
//		stages.
//
//		Stage1 is to iterate through the static field list and taking note of
//		group header/footer fields as we go.  If we encounter a group header,
//		we note whether the group is open/closed and continue adding or omitting
//		controls accordingly.  When we encounter a group footer we signal a
//		default of open so that ungrouped controls get added by default.
//
//		Stage2 is to iterate through the dynamic field list.  This is slightly
//		different as we don't have group header/footer fields here.  We simply
//		add in a special-case command button to toggle the group open/closed.
//		We then proceed to add or omit controls accordingly.
//
void GuiInspector::inspect(SimObject * obj)
{
   mTarget = obj;

   while(size())
      first()->deleteObject();

   S32 curYOffset = mEntrySpacing;

   if(!bool(mTarget))
   {
      resize(mBounds.point, Point2I(mBounds.extent.x, curYOffset));
      return;
   }

   // MM: Moved buffer earlier.
   char textbuf[1024];

   // MM: Group expansion flag.
   bool groupExpand = true;

   // MM: Get Dynamic Group Expand Flag.
   bool& DynamicGroupExpand = mTarget->getDynamicGroupExpand();

   // MM: Moved this earlier.
   Con::setIntVariable("InspectingObject", mTarget->getId());
   Con::setIntVariable("Inspector", getId());

   // Let's prepare a few things we're going to need.
   GuiButtonCtrl * groupButtonCtrl;
   GuiControl * groupSepCtrl;
   S32 xStartPoint;

   // Let's do field grouping if it's on.
   if (mUseFieldGrouping)
   {
		// MM: Add Expand/Collapse button controls.

		// MM: Expand All.
		dSprintf(textbuf, sizeof(textbuf), "%d.setAllGroupStateScript(%d, true);", getId(), mTarget->getId());
		GuiButtonCtrl * groupButtonCtrl = new GuiButtonCtrl();
		groupButtonCtrl->setField("profile", "GuiButtonProfile");
		groupButtonCtrl->setField("text", "Expand All");
		groupButtonCtrl->setField("command", textbuf);
		groupButtonCtrl->registerObject();
		addObject(groupButtonCtrl);
		// Calculate position.
		xStartPoint = mBounds.point.x + mEntrySpacing;
		// Set new position.
		groupButtonCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
		groupButtonCtrl->mBounds.extent = Point2I(mBounds.extent.x/2 - mEntrySpacing*4, mEntryHeight);
		groupButtonCtrl->setSizing(GuiControl::horizResizeRelative, GuiControl::vertResizeBottom);

		// MM: Collapsed All.
		dSprintf(textbuf, sizeof(textbuf), "%d.setAllGroupStateScript(%d, false);", getId(), mTarget->getId());
		groupButtonCtrl = new GuiButtonCtrl();
		groupButtonCtrl->setField("profile", "GuiButtonProfile");
		groupButtonCtrl->setField("text", "Collapse All");
		groupButtonCtrl->setField("command", textbuf);
		groupButtonCtrl->registerObject();
		addObject(groupButtonCtrl);
		// Calculate position.
		xStartPoint = mBounds.point.x + mBounds.extent.x/2 + mEntrySpacing;
		// Set new position.
		groupButtonCtrl->mBounds.point = Point2I(xStartPoint , curYOffset);
		groupButtonCtrl->mBounds.extent = Point2I(mBounds.extent.x/2 - mEntrySpacing*4, mEntryHeight);
		groupButtonCtrl->setSizing(GuiControl::horizResizeRelative, GuiControl::vertResizeBottom);

		// Move to next position.
		curYOffset += mEntryHeight + 2;

		// MM: Add seperator control.
		groupSepCtrl = new GuiControl();
		groupSepCtrl->setField("profile", "GuiWindowProfile");
		groupSepCtrl->registerObject();
		addObject(groupSepCtrl);
		// MM: Move to next position.
		curYOffset += mEntrySpacing;
		// MM: Calculate position.
		xStartPoint = mBounds.point.x + mEntrySpacing;
		// MM: Set new position.
		groupSepCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
		groupSepCtrl->mBounds.extent = Point2I(mBounds.extent.x - mEntrySpacing*4, 2);
		groupSepCtrl->setSizing(GuiControl::horizResizeWidth, GuiControl::vertResizeBottom);

		// MM: Add group space.
		curYOffset += 6;
   }

   char buf[1024];
   
   dSprintf(buf, 1024, "Class: %s", mTarget->getClassName());

   GuiTextCtrl* classCtrl = new GuiTextCtrl();
   classCtrl->setField("profile", "GuiTextProfile");
   classCtrl->setField("text", buf);
   classCtrl->registerObject();
   addObject(classCtrl);

   S32 textWidth = classCtrl->mProfile->mFont->getStrWidth(buf);

   classCtrl->mBounds.point = Point2I(mEntrySpacing, curYOffset);
   classCtrl->mBounds.extent = Point2I(textWidth, mEntryHeight);

   curYOffset += (mEntryHeight + mEntrySpacing);

   dSprintf(buf, 1024, "Id: %d", mTarget->getId());

   GuiTextCtrl* idCtrl = new GuiTextCtrl();
   idCtrl->setField("profile", "GuiTextProfile");
   idCtrl->setField("text", buf);
   idCtrl->registerObject();
   addObject(idCtrl);

   textWidth = idCtrl->mProfile->mFont->getStrWidth(buf);

   idCtrl->mBounds.point = Point2I(mEntrySpacing, curYOffset);
   idCtrl->mBounds.extent = Point2I(textWidth, mEntryHeight);

   curYOffset += (mEntryHeight + mEntrySpacing);

   // add in the static fields
   AbstractClassRep::FieldList fieldList = mTarget->getFieldList();
   AbstractClassRep::FieldList::iterator itr;
   for(itr = fieldList.begin(); itr != fieldList.end(); itr++)
   {
      if(itr->type == AbstractClassRep::DepricatedFieldType)
         continue;

      // Let's do field grouping if it's on.
      if (mUseFieldGrouping)
	  {
		  // MM: Is this a start group field?
		  if (itr->type == AbstractClassRep::StartGroupFieldType)
		  {
			// Setup command.
			dSprintf(textbuf, sizeof(textbuf), "%d.toggleGroupScript(%d,%s);", getId(), mTarget->getId(), itr->pFieldname);

			// Fetch expansion flag.
			groupExpand = itr->groupExpand;

			// Add group button control.
			groupButtonCtrl = new GuiButtonCtrl();
			groupButtonCtrl->setField("profile", "GuiButtonProfile");
			groupButtonCtrl->setField("text", itr->pGroupname);
			groupButtonCtrl->setField("command", textbuf);
			groupButtonCtrl->setField("buttonType", "ToggleButton");
			groupButtonCtrl->setScriptValue(groupExpand?"1":"0");
			groupButtonCtrl->registerObject();
			groupButtonCtrl->setSizing(GuiControl::horizResizeWidth, GuiControl::vertResizeBottom);
			addObject(groupButtonCtrl);

			// Move to next position.
			curYOffset += mEntrySpacing;

			// Calculate position.
			S32 xStartPoint = mBounds.point.x + mEntrySpacing;

			// Set new position.
			groupButtonCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
			groupButtonCtrl->mBounds.extent = Point2I(mBounds.extent.x - mEntrySpacing*4, mEntryHeight);

			// Move to next position.
			curYOffset += (mEntryHeight + mEntrySpacing);

			// Move to next field.
			continue;
		  }

		  // MM: Is this a end group field?
		  if (itr->type == AbstractClassRep::EndGroupFieldType)
		  {
			  // Let's add a seperator if we're using them.
			  if (UseFieldSeperators)
			  {
					// Add Seperator if group was expanded.
					if(groupExpand)
					{
						// Add group seperator control.
						groupSepCtrl = new GuiControl();
						groupSepCtrl->setField("profile", "GuiWindowProfile");
						groupSepCtrl->registerObject();
						addObject(groupSepCtrl);

						// Move to next position.
						curYOffset += mEntrySpacing;

						// Calculate position.
						S32 xStartPoint = mBounds.point.x + mEntrySpacing;

						// Set new position.
						groupSepCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
						groupSepCtrl->mBounds.extent = Point2I(mBounds.extent.x - mEntrySpacing*4, 2);

						// Add group space.
						curYOffset += 4;
					}
			  }
			// Add group space.
			curYOffset += mEntrySpacing * (groupExpand ? 6 : 1);

			// End Group Expansion (default to showing).
			groupExpand = true;

			// Move to next field.
			continue;
		  }
	  }
	  // Ignore group fields if not.
	  else if (itr->type == AbstractClassRep::StartGroupFieldType || itr->type == AbstractClassRep::EndGroupFieldType)
		  continue;

	  // MM: Is the current group expanded?
	  if (groupExpand)
	  {
		  // Yes, so add controls.

         for(U32 i = 0; i < itr->elementCount; i++)
         {
            const char * dstr = Con::getData(itr->type, (void *)(((const char *)obj) + itr->offset), i, itr->table, itr->flag);
		  if(!dstr)
			 dstr = "";
		  expandEscape(textbuf, dstr);

		  GuiTextCtrl * textCtrl = new GuiTextCtrl();
		  textCtrl->setField("profile", "GuiTextProfile");
            char temp[1024];
            if(itr->elementCount > 1)
               dSprintf(temp, sizeof(temp), "%s%d", itr->pFieldname, i);
            else
               dSprintf(temp, sizeof(temp), "%s", itr->pFieldname);
            textCtrl->setField("text", temp);
		  textCtrl->registerObject();
		  addObject(textCtrl);

            S32 textWidth = textCtrl->mProfile->mFont->getStrWidth(temp);
		  S32 xStartPoint = (textWidth < (mTextExtent + mEntrySpacing + mEditControlOffset)) ? 
			 (mEntrySpacing + mTextExtent) : textWidth + mEditControlOffset;

		  textCtrl->mBounds.point = Point2I(mEntrySpacing, curYOffset);
		  textCtrl->mBounds.extent = Point2I(textWidth, mEntryHeight);

		  S32 maxWidth = mBounds.extent.x - xStartPoint - mEntrySpacing;
         
		  //now add the field
		  GuiControl * editControl = NULL;
  
		  switch(itr->type)
		  {
			 // text control
			 default:
			 {
				GuiTextEditCtrl * edit = new GuiTextEditCtrl();
				edit->setField("profile", "GuiInspectorTextEditProfile");
				edit->setField("text", textbuf);
				editControl = edit;

				edit->mBounds.point = Point2I(xStartPoint, curYOffset);
				edit->mBounds.extent = Point2I(maxWidth - 2, mEntryHeight);
				edit->setSizing(GuiControl::horizResizeWidth, GuiControl::vertResizeBottom);
				break;
			 }

			 // checkbox
			 case TypeBool:
			 case TypeFlag:
			 {
				GuiCheckBoxCtrl * checkBox = new GuiCheckBoxCtrl();
				checkBox->setField("profile", "GuiCheckBoxProfile");
				checkBox->mBounds.point = Point2I(xStartPoint, curYOffset);
				checkBox->mBounds.extent = Point2I(mEntryHeight, mEntryHeight);
				checkBox->setScriptValue(textbuf);

				editControl = checkBox;
				break;
			 }

			 // dropdown list
			 case TypeEnum:
			 {
				AssertFatal(itr->table, "TypeEnum declared with NULL table");
				GuiPopUpMenuCtrl * menu = new GuiPopUpMenuCtrl();
				menu->setField("profile", "GuiPopUpMenuProfile");
				  menu->setField("text", textbuf);

				menu->mBounds.point = Point2I(xStartPoint, curYOffset);
				menu->mBounds.extent = Point2I(getMin(maxWidth, mMaxMenuExtent), mEntryHeight);
        
				//now add the entries
				for(S32 i = 0; i < itr->table->size; i++)
				   menu->addEntry(itr->table->table[i].label, itr->table->table[i].index);

				editControl = menu;
				break;
			 }

			 // guiprofiles
			 case TypeGuiProfile:
			 {
				GuiPopUpMenuCtrl * menu = new GuiPopUpMenuCtrl();
				menu->setField("profile", "GuiPopUpMenuProfile");
				  menu->setField("text", *textbuf ? textbuf : NULL_STRING);
				menu->mBounds.point = Point2I(xStartPoint, curYOffset);
				menu->mBounds.extent = Point2I(getMin(maxWidth, mMaxMenuExtent), mEntryHeight);

				// add 'NULL'
				menu->addEntry(NULL_STRING, -1);
  
				// add entries to list so they can be sorted prior to adding to menu (want null on top)
				Vector<StringTableEntry> entries;
        
				SimGroup * grp = Sim::getGuiDataGroup();
				for(SimGroup::iterator i = grp->begin(); i != grp->end(); i++)
				{
				   GuiControlProfile * profile = dynamic_cast<GuiControlProfile *>(*i);
				   if(profile)
					  entries.push_back(profile->getName());
				}
        
				// sort the entries
				dQsort(entries.address(), entries.size(), sizeof(StringTableEntry), stringCompare);
				for(U32 j = 0; j < entries.size(); j++)
				   menu->addEntry(entries[j], 0);

				editControl = menu;
				break;
			 }

			 // datablock types
			 case TypeGameBaseDataPtr:
			 case TypeExplosionDataPtr:
			 case TypeShockwaveDataPtr:
			 case TypeSplashDataPtr:
			 case TypeEnergyProjectileDataPtr:
			 case TypeParticleEmitterDataPtr:
			 case TypeAudioDescriptionPtr:
			 case TypeAudioProfilePtr:
			 case TypeProjectileDataPtr:
			 case TypeCannedChatItemPtr:
			 case TypeDebrisDataPtr:
			 case TypeCommanderIconDataPtr:
			 {
				GuiPopUpMenuCtrl * menu = new GuiPopUpMenuCtrl();
				menu->setField("profile", "GuiPopUpMenuProfile");
				  menu->setField("text", *textbuf ? textbuf : NULL_STRING);
				menu->mBounds.point = Point2I(xStartPoint, curYOffset);
				menu->mBounds.extent = Point2I(getMin(maxWidth, mMaxMenuExtent), mEntryHeight);
        
				// add the 'NULL' entry on top
				menu->addEntry(NULL_STRING, -1);

				// add to a list so they can be sorted
				Vector<StringTableEntry> entries;

				SimGroup * grp = Sim::getDataBlockGroup();
				for(SimGroup::iterator i = grp->begin(); i != grp->end(); i++)
				{
				   SimObject * obj = 0;
				   switch(itr->type)
				   {
					  case TypeGameBaseDataPtr:
						 obj = dynamic_cast<GameBaseData*>(*i); 
						 break;
					  case TypeExplosionDataPtr:
						 obj = dynamic_cast<ExplosionData*>(*i); 
						 break;
					  case TypeSplashDataPtr:
						 obj = dynamic_cast<SplashData*>(*i); 
						 break;
					  case TypeParticleEmitterDataPtr:
						 obj = dynamic_cast<ParticleEmitterData*>(*i); 
						 break;
					  case TypeAudioDescriptionPtr:
						 obj = dynamic_cast<AudioDescription*>(*i); 
						 break;
					  case TypeAudioProfilePtr:
						 obj = dynamic_cast<AudioProfile*>(*i); 
						 break;
					  case TypeProjectileDataPtr:
						 obj = dynamic_cast<ProjectileData*>(*i);
						 break;
					  case TypeDebrisDataPtr:
						 obj = dynamic_cast<DebrisData*>(*i);
						 break;
				   }

				   if(obj)
					  entries.push_back(obj->getName());
				}

				// sort the entries
				dQsort(entries.address(), entries.size(), sizeof(StringTableEntry), stringCompare);
				for(U32 j = 0; j < entries.size(); j++)
				   menu->addEntry(entries[j], 0);

				editControl = menu;
				break;
			 }
		  }

			if(editControl)
		  {
               dSprintf(textbuf, sizeof(textbuf), "InspectStatic%s%d_%d", itr->pFieldname, i,  itr->type);
  			  editControl->registerObject(textbuf);
  			  addObject(editControl);
		  }

		  curYOffset += (mEntryHeight + mEntrySpacing) + 1;
         }
	   } // MM: Group expansion.
   }

    // Let's do field grouping if it's on.
    if (mUseFieldGrouping)
	{
		// MM: Add dynamic fields group button control.
		// NOTE:- We treat the dynamic fields group as a special case.
		dSprintf(textbuf, sizeof(textbuf), "%d.toggleDynamicGroupScript(%d);", getId(), mTarget->getId());
		groupButtonCtrl = new GuiButtonCtrl();
		groupButtonCtrl->setField("profile", "GuiButtonProfile");
		groupButtonCtrl->setField("text", "Dynamic Fields");
		groupButtonCtrl->setField("command", textbuf);
		groupButtonCtrl->setField("buttonType", "ToggleButton");
		groupButtonCtrl->setScriptValue(DynamicGroupExpand?"1":"0");
		groupButtonCtrl->registerObject();
		groupButtonCtrl->setSizing(GuiControl::horizResizeWidth, GuiControl::vertResizeBottom);
		addObject(groupButtonCtrl);
		// Calculate position.
		xStartPoint = mBounds.point.x + mEntrySpacing;
		// Move to next position.
		curYOffset += mEntrySpacing;
		// Set new position.
		groupButtonCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
		groupButtonCtrl->mBounds.extent = Point2I(mBounds.extent.x - mEntrySpacing*4, mEntryHeight);
		// MM: Move to next position.
		curYOffset += (mEntryHeight + mEntrySpacing);
	}

	GuiTextCtrl* dfCtrl = new GuiTextCtrl();
	dfCtrl->setField("profile", "GuiTextProfile");
	dfCtrl->setField("text", " Dynamic Fields");
	dfCtrl->registerObject();
	addObject(dfCtrl);

	textWidth = dfCtrl->mProfile->mFont->getStrWidth(" Dynamic Fields");

	dfCtrl->mBounds.point = Point2I(mEntrySpacing, curYOffset);
	dfCtrl->mBounds.extent = Point2I(textWidth, mEntryHeight);

	// button
	GuiButtonCtrl * button = new GuiButtonCtrl();
	button->setField("profile", "GuiButtonProfile");
	button->setField("text", "Add...");

	// MM: Calculate right-alignment.
	textWidth = button->mProfile->mFont->getStrWidth("Add...") + mEntrySpacing*6;
	xStartPoint = mBounds.point.x + mBounds.extent.x - textWidth - mEntrySpacing*3;

	// MM: Set new position.
	button->mBounds.point = Point2I(xStartPoint, curYOffset);
	button->mBounds.extent = Point2I(textWidth, mEntryHeight);
  	button->registerObject();

   dSprintf(textbuf, sizeof(textbuf), "%d.addDynamicField(%d);", getId(), mTarget->getId());
   button->setField("command", textbuf);
  	button->setSizing(GuiControl::horizResizeLeft, GuiControl::vertResizeBottom);
  	addObject(button);
   
   // MM: dynamic field offset   
   curYOffset += (mEntryHeight + mEntrySpacing);

   // MM: Are we showing the dynamic fields?
   if (DynamicGroupExpand || !mUseFieldGrouping)
   {
	   // MM: Yes, so add the dynamic fields
	   SimFieldDictionary * fieldDictionary = mTarget->getFieldDictionary();
	   for(SimFieldDictionaryIterator ditr(fieldDictionary); *ditr; ++ditr)
	   {
		  SimFieldDictionary::Entry * entry = (*ditr);

		  // create the name ctrl
		  GuiTextCtrl * nameCtrl = new GuiTextCtrl();

		  nameCtrl->setField("profile", "GuiTextProfile");
		  nameCtrl->setField("text", entry->slotName);
		  nameCtrl->registerObject();
		  addObject(nameCtrl);

		  nameCtrl->mBounds.point = Point2I(mEntrySpacing, curYOffset);
		  nameCtrl->mBounds.extent = Point2I(mTextExtent, mEntryHeight);

		  // add a 'remove' button
		  GuiButtonCtrl * button = new GuiButtonCtrl();

		  button->setField("profile", "GuiButtonProfile");
		  button->setField("text", "x");
		  button->registerObject();
		  addObject(button);

		  dSprintf(textbuf, sizeof(textbuf), "%d.%s = \"\";%d.inspect(%d);", mTarget->getId(), entry->slotName, getId(), mTarget->getId());
		  button->setField("command", textbuf);
         
   		S32 textWidth = mProfile->mFont->getStrWidth(entry->slotName);

		  // 10x10 with 2x2 frame   
		  button->mBounds.point.set(textWidth + 4, curYOffset + 2);
		  button->mBounds.extent.set(10, 10);

		  textWidth += 14;
		  S32 xStartPoint = (textWidth < (mTextExtent + mEntrySpacing + mEditControlOffset)) ? 
			 (mEntrySpacing + mTextExtent) : textWidth + mEditControlOffset;
		  S32 maxWidth = mBounds.extent.x - xStartPoint - mEntrySpacing;

		  expandEscape(textbuf, entry->value ? entry->value : "");

		  // create the edit ctrl
		  GuiTextEditCtrl * editCtrl = new GuiTextEditCtrl();
		  editCtrl->setField("profile", "GuiInspectorTextEditProfile");
		  editCtrl->setField("text", textbuf);
		  editCtrl->setSizing(GuiControl::horizResizeWidth, GuiControl::vertResizeBottom);

  		   dSprintf(textbuf, sizeof(textbuf), "InspectDynamic%s", entry->slotName);
  		   editCtrl->registerObject(textbuf);
		  addObject(editCtrl);
      
		  editCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
		  editCtrl->mBounds.extent = Point2I(maxWidth, mEntryHeight);

		  curYOffset += (mEntryHeight + mEntrySpacing);
	   }
   }

   // Let's do field grouping if it's on.
   if (UseFieldSeperators)
   {
	   // MM: Add group seperator control.
	   groupSepCtrl = new GuiControl();
	   groupSepCtrl->setField("profile", "GuiWindowProfile");
	   groupSepCtrl->registerObject();
	   addObject(groupSepCtrl);
	   // MM: Move to next position.
	   curYOffset += mEntrySpacing;
	   // MM: Calculate position.
	   xStartPoint = mBounds.point.x + mEntrySpacing;
	   // MM: Set new position.
	   groupSepCtrl->mBounds.point = Point2I(xStartPoint, curYOffset);
	   groupSepCtrl->mBounds.extent = Point2I(mBounds.extent.x - mEntrySpacing*4, 2);
	   // MM: Add group space.
	   curYOffset += 4;
   }

   resize(mBounds.point, Point2I(mBounds.extent.x, curYOffset));
}

void GuiInspector::apply(const char * newName)
{
   if(!bool(mTarget))
   {
      while(size())
         first()->deleteObject();
      return;
   }

   mTarget->assignName(newName);
   mTarget->inspectPreApply();

   SimObject * obj = static_cast<SimObject*>(mTarget);

   //now add in the fields
   AbstractClassRep::FieldList fieldList = mTarget->getFieldList();
   AbstractClassRep::FieldList::iterator itr;
   for(itr = fieldList.begin(); itr != fieldList.end(); itr++)
   {
      if(itr->type == AbstractClassRep::DepricatedFieldType || 
         itr->type == AbstractClassRep::StartGroupFieldType ||
         itr->type == AbstractClassRep::EndGroupFieldType)
         continue;

      for(U32 i = 0; i < itr->elementCount; i++)
      {
      char textbuf[1024];
         dSprintf(textbuf, sizeof(textbuf), "InspectStatic%s%d_%d", itr->pFieldname, i, itr->type);
      GuiControl * editCtrl = NULL;
      SimObject * inspectObj = Sim::findObject(textbuf);
      if(inspectObj)
         editCtrl = dynamic_cast<GuiControl*>(inspectObj);
      if(!editCtrl)
         continue;

      const char * newValue = 0;
      
      // check for null on profiles (-1 popup id)
      GuiPopUpMenuCtrl * menu = dynamic_cast<GuiPopUpMenuCtrl*>(editCtrl);
      if(!(menu && (menu->getSelected() == -1)))
         newValue = editCtrl->getScriptValue();
         
      if(!newValue)
         newValue = "";

      dStrcpy(textbuf, newValue);
      collapseEscape(textbuf);
      
      //now set the field
      const char *argv[1];
      argv[0] = &textbuf[0];
         Con::setData(itr->type, (void *)(((const char *)obj) + itr->offset), i, 1, argv, itr->table, itr->flag);
      }
   }

   // get the dynamic field data
   SimFieldDictionary * fieldDictionary = mTarget->getFieldDictionary();
   for(SimFieldDictionaryIterator ditr(fieldDictionary); *ditr; ++ditr)
   {
      SimFieldDictionary::Entry * entry = (*ditr);

      char buf[1024];
      dSprintf(buf, sizeof(buf), "InspectDynamic%s", entry->slotName);

      GuiControl * editCtrl = static_cast<GuiControl*>(Sim::findObject(buf));
      if(!editCtrl)
         continue;

      const char * newValue = editCtrl->getScriptValue();
      dStrcpy(buf, newValue ? newValue : "");
      collapseEscape(buf);

      fieldDictionary->setFieldValue(entry->slotName, buf);
   }

   mTarget->inspectPostApply();

   //now re-inspect the object
   inspect(mTarget);
}



// MM: Lets toggle the group expansion.
void GuiInspector::toggleGroupExpand(SimObject * obj, const char* in_pFieldname)
{
   // Fetch the static fields
   AbstractClassRep::FieldList& fieldList = mTarget->getModifiableFieldList();

   // Find the selected group field.
   for (U32 i = 0; i < fieldList.size(); i++)
   {
      if (dStricmp(in_pFieldname, fieldList[i].pFieldname) == 0)
      {
         // Check that it's a start group.
         if (fieldList[i].type != AbstractClassRep::StartGroupFieldType)
         {
            // It's not so send warning.
            Con::warnf("GuiInspector: Attempt to toggle non-group field expansion!");
            return;
         }

         // Flip the state.
         fieldList[i].groupExpand = !fieldList[i].groupExpand;

         // Finish here.
         return;
      }
   }
}

// MM: Lets toggle the Dynamic group expansion.
void GuiInspector::toggleDynamicGroupExpand()
{
   // Toggle the Dynamic Group Expand Flag.
   mTarget->getDynamicGroupExpand() = !mTarget->getDynamicGroupExpand();
}

// MM: Set all groups state.
void GuiInspector::setAllGroupState(bool GroupState)
{
   // Fetch the static fields
   AbstractClassRep::FieldList& fieldList = mTarget->getModifiableFieldList();

   // Find the selected group field.
   for (U32 i = 0; i < fieldList.size(); i++)
   {
      // Check that it's a start group.
      if (fieldList[i].type == AbstractClassRep::StartGroupFieldType)
      {
         // Set the Group State.
         fieldList[i].groupExpand = GroupState;
      }
   }

   // Set the Dynamic Group State.
   mTarget->getDynamicGroupExpand() = GroupState;
}


//------------------------------------------------------------------------------
ConsoleMethod( GuiInspector, inspect, void, 3, 3, "(SimObject obj)"
              "Set this GuiInspector to show information about the specified object.")
{
   SimObject * target = Sim::findObject(argv[2]);
   if(!target)
   {
      Con::printf("%s(): invalid object: %s", argv[0], argv[2]);
      return;
   }
   object->inspect(target);
}

ConsoleMethod( GuiInspector, apply, void, 3, 3, "(string newName)"
              "Apply any changed values to the current object.")
{
   object->apply(argv[2]);
}

ConsoleMethod( GuiInspector, toggleGroupExpand, void, 4, 4, "(SimObject obj, string groupName)"
              "Toggle the expandedness of a given group on the specified object.")
{
   SimObject * target = Sim::findObject(argv[2]);
   object->toggleGroupExpand(target, argv[3]);
}

ConsoleMethod( GuiInspector, toggleDynamicGroupExpand, void, 2, 2, "Toggle the dynamic group expandedness.")
{
   object->toggleDynamicGroupExpand();
}

ConsoleMethod( GuiInspector, setAllGroupState, void, 3, 3, "(bool groupState)"
              "Set all groups on the current object to be expanded or not.")
{
   object->setAllGroupState(dAtob(argv[2]));
}

void GuiInspector::initPersistFields()
{
   Parent::initPersistFields();
   addField("editControlOffset", TypeS32, Offset(mEditControlOffset, GuiInspector));
   addField("entryHeight", TypeS32, Offset(mEntryHeight, GuiInspector));
   addField("textExtent", TypeS32, Offset(mTextExtent, GuiInspector));
   addField("entrySpacing", TypeS32, Offset(mEntrySpacing, GuiInspector));
   addField("maxMenuExtent", TypeS32, Offset(mMaxMenuExtent, GuiInspector));
   addField("useFieldGrouping", TypeBool, Offset(mUseFieldGrouping, GuiInspector));
}
