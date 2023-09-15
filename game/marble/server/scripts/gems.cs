//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Gem base class
//-----------------------------------------------------------------------------

datablock AudioProfile(GotGemSfx)
{
   filename    = "~/data/sound/gotGem.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(GotAllGemsSfx)
{
   filename    = "~/data/sound/gotAllGems.wav";
   description = AudioDefault3d;
   preload = true;
};


//-----------------------------------------------------------------------------

$GemSkinColors[0] = "base";
$GemSkinColors[1] = "base";
$GemSkinColors[2] = "blue";
$GemSkinColors[3] = "red";
$GemSkinColors[4] = "yellow";
$GemSkinColors[5] = "purple";
$GemSkinColors[6] = "orange";
$GemSkinColors[7] = "green";
$GemSkinColors[8] = "turquoise";
$GemSkinColors[9] = "black";

function Gem::onAdd(%this,%obj)
{
   if (%this.skin !$= "")
      %obj.setSkinName(%this.skin);
   else {
      // Random skin if none assigned
      %obj.setSkinName($GemSkinColors[getRandom(9)]);
   }
}

function Gem::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this,%obj,%user,%amount);
   %user.client.onFoundGem(%amount);
   return true;
}

function Gem::saveState(%this,%obj,%state)
{
   %state.object[%obj.getId()] = %obj.isHidden();
}

function Gem::restoreState(%this,%obj,%state)
{
   %obj.hide(%state.object[%obj.getId()]);
}

//-----------------------------------------------------------------------------

datablock ItemData(GemItem)
{
   // Mission editor category
   category = "Gems";
   className = "Gem";

   // Basic Item properties
   shapeFile = "~/data/shapes/items/gem.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupName = "a gem!";
   maxInventory = 1;
   noRespawn = true;
   gemType = 1;
   noPickupMessage = true;
};

datablock ItemData(GemItemBlue: GemItem) 
{
   skin = "blue";
};

datablock ItemData(GemItemRed: GemItem)
{
   skin = "red";
};

datablock ItemData(GemItemYellow: GemItem)
{
   skin = "yellow";
};

datablock ItemData(GemItemPurple: GemItem)
{
   skin = "purple";
};

datablock ItemData(GemItemGreen: GemItem)
{
   skin = "Green";
};

datablock ItemData(GemItemTurquoise: GemItem)
{
   skin = "Turquoise";
};

datablock ItemData(GemItemOrange: GemItem)
{
   skin = "orange";
};

datablock ItemData(GemItemBlack: GemItem)
{
   skin = "black";
};

