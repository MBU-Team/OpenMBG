//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PowerUp base class
//-----------------------------------------------------------------------------

function PowerUp::onPickup(%this,%obj,%user,%amount)
{
   // Dont' pickup the power up if it's the same
   // one we already have.
   if (%user.powerUpData == %this)
      return false;

   // Grab it...
   %user.client.play2d(%this.pickupAudio);
   if (%this.powerUpId)
   {
      if(%obj.showHelpOnPickup)
         addHelpLine("Press <func:bind mouseFire> to use the " @ %this.useName @ "!", false);
   
      %user.setPowerUp(%this);
   }
   Parent::onPickup(%this, %obj, %user, %amount);
   return true;
}


//-----------------------------------------------------------------------------

datablock AudioProfile(doSuperJumpSfx)
{
   filename    = "~/data/sound/doSuperJump.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(PuSuperJumpVoiceSfx)
{
   filename    = "~/data/sound/puSuperJumpVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(SuperJumpItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 1;

   activeAudio = DoSuperJumpSfx;
   pickupAudio = PuSuperJumpVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/superjump.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   pickupName = "a Super Jump PowerUp!";
   useName = "Super Jump PowerUp";
   maxInventory = 1;
};


//-----------------------------------------------------------------------------

datablock AudioProfile(doSuperBounceSfx)
{
   filename    = "~/data/sound/doSuperBounce.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(PuSuperBounceVoiceSfx)
{
   filename    = "~/data/sound/puSuperBounceVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(SuperBounceItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 3;

   activeAudio = DoSuperBounceSfx;
   pickupAudio = PuSuperBounceVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/superbounce.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupName = "a Super Bounce PowerUp!";
   useName = "Super Bounce PowerUp";
   maxInventory = 1;
};

datablock AudioProfile(SuperBounceLoopSfx)
{
   filename    = "~/data/sound/forcefield.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock ShapeBaseImageData(SuperBounceImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/images/glow_bounce.dts";
   emap = true;

   // Specify mount point & offset for 3rd person, and eye offset
   // for first person rendering.
   mountPoint = 0;
   offset = "0 0 0";
   stateName[0] = "Blah";
   stateSound[0] = SuperBounceLoopSfx;
};


//-----------------------------------------------------------------------------

datablock AudioProfile(doSuperSpeedSfx)
{
   filename    = "~/data/sound/doSuperSpeed.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(PuSuperSpeedVoiceSfx)
{
   filename    = "~/data/sound/puSuperSpeedVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(SuperSpeedItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 2;

   activeAudio = DoSuperSpeedSfx;
   pickupAudio = PuSuperSpeedVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/superspeed.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   pickupName = "a Super Speed PowerUp!";
   useName = "Super Speed PowerUp";
   maxInventory = 1;
};


//-----------------------------------------------------------------------------

datablock AudioProfile(doShockAbsorberSfx)
{
   filename    = "~/data/sound/doShockAbsorber.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(PuShockAbsorberVoiceSfx)
{
   filename    = "~/data/sound/puShockAbsorberVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(ShockAbsorberItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 4;

   activeAudio = DoShockAbsorberSfx;
   pickupAudio = PuShockAbsorberVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/items/shockabsorber.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupName = "a Shock Absorber PowerUp!";
   useName = "Shock Absorber PowerUp";
   maxInventory = 1;
   emap = false;
};

datablock AudioProfile(ShockLoopSfx)
{
   filename    = "~/data/sound/superbounceactive.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock ShapeBaseImageData(ShockAbsorberImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/images/glow_bounce.dts";
   emap = true;

   // Specify mount point & offset for 3rd person, and eye offset
   // for first person rendering.
   mountPoint = 0;
   offset = "0 0 0";
   stateName[0] = "Blah";
   stateSound[0] = ShockLoopSfx;
};


//-----------------------------------------------------------------------------

datablock AudioProfile(doHelicopterSfx)
{
   filename    = "~/data/sound/doHelicopter.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(PuGyrocopterVoiceSfx)
{
   filename    = "~/data/sound/puGyrocopterVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(HelicopterItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";
   powerUpId = 5;

   activeAudio = DoHelicopterSfx;
   pickupAudio = PuGyrocopterVoiceSfx;

   // Basic Item properties
   shapeFile = "~/data/shapes/images/helicopter.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;

   // Dynamic properties defined by the scripts
   pickupName = "a Gyrocopter PowerUp!";
   useName = "Gyrocopter PowerUp";
   maxInventory = 1;
};

datablock AudioProfile(HelicopterLoopSfx)
{
   filename    = "~/data/sound/Use_Gyrocopter.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock ShapeBaseImageData(HelicopterImage)
{
   // Basic Item properties
   shapeFile = "~/data/shapes/images/helicopter.dts";
   emap = true;
   mountPoint = 0;
   offset = "0 0 0";
   stateName[0]                     = "Rotate";
   stateSequence[0]                 = "rotate";
   stateSound[0] = HelicopterLoopSfx;
   ignoreMountRotation = true;
};


//-----------------------------------------------------------------------------
// Special non-inventory power ups
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

datablock AudioProfile(PuTimeTravelVoiceSfx)
{
   filename    = "~/data/sound/puTimeTravelVoice.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(TimeTravelItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";

   // Basic Item properties
   pickupAudio = PuTimeTravelVoiceSfx;
   shapeFile = "~/data/shapes/items/timetravel.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   noRespawn = true;
   maxInventory = 1;
};

function TimeTravelItem::getPickupName(%this, %obj)
{
   if(%obj.timeBonus !$= "")
      %time = %obj.timeBonus / 1000;
   else
      %time = $Game::TimeTravelBonus / 1000;

   return "a " @ %time @ " second Time Travel Bonus!";
}

function TimeTravelItem::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this, %obj, %user, %amount);
   if(%obj.timeBonus !$= "")
      %user.client.incBonusTime(%obj.timeBonus);
   else
      %user.client.incBonusTime($Game::TimeTravelBonus);
}


//-----------------------------------------------------------------------------

datablock AudioProfile(PuAntiGravityVoiceSfx)
{
   filename    = "~/data/sound/gravitychange.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock ItemData(AntiGravityItem)
{
   // Mission editor category
   category = "Powerups";
   className = "PowerUp";

   pickupAudio = PuAntiGravityVoiceSfx;
   pickupName = "a Gravity Modifier!";

   // Basic Item properties
   shapeFile = "~/data/shapes/items/antiGravity.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;

   // Dynamic properties defined by the scripts
   maxInventory = 1;
};

function AntiGravityItem::onAdd(%this, %obj)
{
   %obj.playThread(0,"Ambient");
}   

function AntiGravityItem::onPickup(%this,%obj,%user,%amount)
{
   %rotation = getWords(%obj.getTransform(),3);
   %ortho = vectorOrthoBasis(%rotation);
   %up = getWords(%ortho,6);
   if (getGravityDir() !$= %up) {
      Parent::onPickup(%this, %obj, %user, %amount);
      setGravityDir(%ortho);
   }
}

