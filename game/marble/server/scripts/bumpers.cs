//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

datablock AudioProfile(BumperDing)
{
   filename    = "~/data/sound/bumperDing1.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(BumperFlat)
{
   filename    = "~/data/sound/bumper1.wav";
   description = AudioDefault3d;
   preload = true;
};

function Bumper::onClientCollision(%this,%obj,%col,%vec, %vecLen, %material)
{
   // Currently activates when any object hits it.
   if (%material $= "BumperMaterial") {
      %obj.stopThread(0);
      %obj.playThread(0,"push");
      %obj.playAudio(0,%this.sound);
   }
}

//-----------------------------------------------------------------------------

datablock StaticShapeData(AngleBumper)
{
   category = "Bumpers";
   className = "Bumper";
   shapeFile = "~/data/shapes/bumpers/angleBumper.dts";
   scopeAlways = true;
   sound = BumperFlat;
};

datablock StaticShapeData(TriangleBumper)
{
   category = "Bumpers";
   className = "Bumper";
   shapeFile = "~/data/shapes/bumpers/pball_tri.dts";
   scopeAlways = true;
   sound = BumperFlat;
};

datablock StaticShapeData(RoundBumper)
{
   category = "Bumpers";
   className = "Bumper";
   shapeFile = "~/data/shapes/bumpers/pball_round.dts";
   scopeAlways = true;
   sound = BumperDing;
};


