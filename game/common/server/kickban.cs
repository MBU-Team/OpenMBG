//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

function kick(%client)
{
   messageAll( 'MsgAdminForce', '\c2The Admin has kicked %1.', %client.name);

   if (!%client.isAIControlled())
      BanList::add(%client.guid, %client.getAddress(), $Pref::Server::KickBanTime);
   %client.delete("You have been banned from this server");
}

function ban(%client)
{
   messageAll('MsgAdminForce', '\c2The Admin has banned %1.', %client.name);

   if (!%client.isAIControlled())
      BanList::add(%client.guid, %client.getAddress(), $Pref::Server::BanTime);
   %client.delete("You have been kicked from this server");
}
