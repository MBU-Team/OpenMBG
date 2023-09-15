
//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

echo ("Converting PNG files into split JPG");

function onExit() {}
enableWinConsole(true);
setModPaths("demo");

$File = findFirstFile("*.png");
while ($File !$= "") {
   png2jpg($File);
   $File = findNextFile("*.png");
}

quit();
