//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// These scripts make use of dynamic attribute values on Item datablocks,
// these are as follows:
//
//    maxInventory      Max inventory per object (100 bullets per box, etc.)
//    pickupName        Name to display when client pickups item
//
// Item objects can have:
//
//    count             The # of inventory items in the object.  This
//                      defaults to maxInventory if not set.

//-----------------------------------------------------------------------------
// ItemData base class methods used by all items
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function Item::respawn(%this)
{
   // This method is used to respawn static ammo and weapon items
   // and is usually called when the item is picked up.
   // Instant fade...
   %this.startFade(0, 0, true);
   %this.hide(true);

   // Shedule a reapearance
   %this.schedule($Item::RespawnTime, "hide", false);
   %this.schedule($Item::RespawnTime + 100, "startFade", 1000, 0, false);
}   

function Item::onMissionReset(%this)
{
   cancelAll(%this);
   %this.hide(false);
   %this.startFade(0, 0, false);
}

function ItemData::getPickupName(%this, %obj)
{
   return %this.pickupName;
}

function ItemData::onPickup(%this,%obj,%user,%amount)
{
   // Inform the client what they got.
   if (%user.client && !%this.noPickupMessage)
      messageClient(%user.client, 'MsgItemPickup', '\c0You picked up %1', %this.getPickupName(%obj));

   // If the item is a static respawn item, then go ahead and
   // respawn it, otherwise remove it from the world.
   // Anything not taken up by inventory is lost.
   if (%this.permanent)
      %obj.setCollisionTimeout(%user);
   else
      if (%obj.isStatic()) {
         if (%this.noRespawn)
            %obj.hide(true);
         else
            %obj.respawn();
      }
      else
         %obj.delete();
   return true;
}


//-----------------------------------------------------------------------------
// Hook into the mission editor.

function ItemData::create(%data)
{
   // The mission editor invokes this method when it wants to create
   // an object of the given datablock type.  For the mission editor
   // we always create "static" re-spawnable rotating objects.
   %obj = new Item() {
      dataBlock = %data;
      static = true;
      rotate = true;
   };
   return %obj;
}

