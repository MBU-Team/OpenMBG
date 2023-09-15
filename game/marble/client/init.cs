//-----------------------------------------------------------------------------

// Variables used by client scripts & code.  The ones marked with (c)
// are accessed from code.  Variables preceeded by Pref:: are client
// preferences and stored automatically in the ~/client/prefs.cs file
// in between sessions.
//
//    (c) Client::MissionFile             Mission file name
//    ( ) Client::Password                Password for server join

//    (?) Pref::Player::CurrentFOV
//    (?) Pref::Player::DefaultFov
//    ( ) Pref::Input::KeyboardTurnSpeed

//    (c) pref::Master[n]                 List of master servers
//    (c) pref::Net::RegionMask     
//    (c) pref::Client::ServerFavoriteCount
//    (c) pref::Client::ServerFavorite[FavoriteCount]
//    .. Many more prefs... need to finish this off

// Moves, not finished with this either...
//    (c) firstPerson
//    $mv*Action...

//-----------------------------------------------------------------------------
// These are variables used to control the shell scripts and
// can be overriden by mods:

//-----------------------------------------------------------------------------

function initClient()
{
   echo("\n--------- Initializing FPS: Client ---------");

   // Make sure this variable reflects the correct state.
   $Server::Dedicated = false;

   // Game information used to query the master server
   $Client::GameTypeQuery = "Test App";
   $Client::MissionTypeQuery = "Any";

   // Default level qualification
   if (!$pref::QualifiedLevel["Beginner"])
      $pref::QualifiedLevel["Beginner"] = 1;
   if (!$pref::QualifiedLevel["Intermediate"])
      $pref::QualifiedLevel["Intermediate"] = 1;
   if (!$pref::QualifiedLevel["Advanced"])
      $pref::QualifiedLevel["Advanced"] = 1;

   // The common module provides basic client functionality
   initBaseClient();

   // InitCanvas starts up the graphics system.
   // The canvas needs to be constructed before the gui scripts are
   // run because many of the controls assume the canvas exists at
   // load time.
   initCanvas(" Marble Blast");

   /// Load client-side Audio Profiles/Descriptions
   exec("./scripts/audioProfiles.cs");

   // Load up the Game GUIs
   exec("./ui/defaultGameProfiles.cs");
   exec("./ui/PlayGui.gui");

   // Load up the shell GUIs
   exec("./ui/ignitionGui.gui");
   exec("./ui/ignitionStatusGui.gui");
   exec("./ui/presentsGui.gui");
   exec("./ui/productionGui.gui");
   exec("./ui/titleGui.gui");
   exec("./ui/playMissionGui.gui");
   exec("./ui/mainMenuGui.gui");
   exec("./ui/aboutDlg.gui");
      exec("./ui/startMissionGui.gui");      // to be deleted
   exec("./ui/chooseGui.gui");
   exec("./ui/joinServerGui.gui");
   exec("./ui/endGameGui.gui");
   exec("./ui/upsell/UpsellGui.gui");
   EndGameGui.preload();
   exec("./ui/loadingGui.gui");
   exec("./ui/optionsDlg.gui");
   exec("./ui/remapDlg.gui");
   exec("./ui/MOTDGui.gui");
   exec("./ui/EnterNameDlg.gui");
   EnterNameDlg.preload();
   exec("./ui/HelpCreditsGui.gui");
   exec("./ui/ExitGameDlg.gui");
   exec("./ui/MiniShotGui.gui");

   // Client scripts
   exec("./scripts/client.cs");
   exec("./scripts/helpCredits.cs");
   exec("./scripts/missionDownload.cs");
   exec("./scripts/serverConnection.cs");
   exec("./scripts/playerList.cs");
   exec("./scripts/loadingGui.cs");
   exec("./scripts/optionsDlg.cs");
   exec("./scripts/chatHud.cs");
   exec("./scripts/messageHud.cs");
   exec("./scripts/playGui.cs");
   exec("./scripts/centerPrint.cs");
   exec("./scripts/game.cs");
   exec("./scripts/version.cs");

   // Default player key bindings
   exec("./scripts/default.bind.cs");
   exec("./config.cs");

   // Really shouldn't be starting the networking unless we are
   // going to connect to a remote server, or host a multi-player
   // game.
   // setNetPort(0);

   // Copy saved script prefs into C++ code.
   setShadowDetailLevel( $pref::shadows );
   setDefaultFov( $pref::Player::defaultFov );
   setZoomSpeed( $pref::Player::zoomSpeed );

   // Start up the main menu... this is separated out into a 
   // method for easier mod override.
   loadMainMenu();

   // Connect to server if requested.
   if ($JoinGameAddress !$= "") {
      connect($JoinGameAddress, "", $Pref::Player::Name);
   }
   else if($missionArg !$= "") {
      %file = findNamedFile($missionArg, ".mis");
      if(%file !$= "")
      {
         %this.io = new IgnitionObject();
         %status = %this.io.validate();
         echo("Ignition: " @ %status);

         doCreateGame(%file);
      }
   }
   else if($interiorArg !$= "") {
      %file = findNamedFile($interiorArg, ".dif");
      if(%file !$= "")
      {
         onServerCreated(); // gotta hack here to get the datablocks loaded...

         %this.io = new IgnitionObject();
         %status = %this.io.validate();
         echo("Ignition: " @ %status);

         %missionGroup = createEmptyMission($interiorArg);
         %interior = new InteriorInstance() {
                        position = "0 0 0";
                        rotation = "1 0 0 0";
                        scale = "1 1 1";
                        interiorFile = %file;
                     };
         %missionGroup.add(%interior);
         %interior.magicButton();

         if(!isObject(StartPoint))
         {
            %pt = new StaticShape(StartPoint) {
               position = "0 -5 100";
               rotation = "1 0 0 0";
               scale = "1 1 1";
               dataBlock = "StartPad";
            };
            MissionGroup.add(%pt);
         }

         if(!isObject(EndPoint))
         {
            %pt = new StaticShape(EndPoint) {
               position = "0 5 100";
               rotation = "1 0 0 0";
               scale = "1 1 1";
               dataBlock = "EndPad";
            };
            MissionGroup.add(%pt);
         }
         %box = %interior.getWorldBox();
         %mx = getWord(%box, 0);
         %my = getWord(%box, 1);
         %mz = getWord(%box, 2);
         %MMx = getWord(%box, 3);
         %MMy = getWord(%box, 4);
         %MMz = getWord(%box, 5);
         %pos = (%mx - 3) SPC (%MMy + 3) SPC (%mz - 3);
         %scale = (%MMx - %mx + 6) SPC (%MMy - %my + 6) SPC (%MMz - %mz + 20);
         echo(%box);
         echo(%pos);
         echo(%scale);

         new Trigger(Bounds) {
            position = %pos;
            scale = %scale;
            rotation = "1 0 0 0";
            dataBlock = "InBoundsTrigger";
            polyhedron = "0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 -1.0000000 0.0000000 0.0000000 0.0000000 1.0000000";
         };
         MissionGroup.add(Bounds);


         %missionGroup.save("marble/data/missions/testMission.mis");
         %missionGroup.delete();
         doCreateGame("marble/data/missions/testMission.mis");
      }
   }
}

function doCreateGame(%file)
{
   echo("creating game!");
   createServer("SinglePlayer", %file);
   %conn = new GameConnection(ServerConnection);
   RootGroup.add(ServerConnection);
   %conn.setConnectArgs($pref::Player::Name);
   %conn.setJoinPassword($Client::Password);
   %conn.connectLocal();
}

function findNamedFile(%file, %ext)
{
   if(fileExt(%file) !$= %ext)
      %file = %file @ %ext;

   %found = findFirstFile(%file);
   if(%found $= "")
      %found = findFirstFile("*/" @ %file);
   return %found;
}

function setPlayMissionGui()
{
   Canvas.setContent(playMissionGui);
}

//-----------------------------------------------------------------------------

function loadMainMenu()
{
   // Startup the client with the Main menu...
   runPresentation();
   buildMissionList();
   Canvas.setCursor("DefaultCursor");
   playShellMusic();
}

function createEmptyMission(%interiorArg)
{
   return new SimGroup(MissionGroup) {
     new ScriptObject(MissionInfo) {
           level = "001";
           desc = "A test mission for an interior";
           type = "Template";
           name = "Interior Test: " @ %interiorArg;
           time = 0;
     };
     new MissionArea(MissionArea) {
        area = "-360 -648 720 1296";
        flightCeiling = "300";
        flightCeilingRange = "20";
        locked = "true";
     };
     new Sky(Sky) {
        position = "336 136 0";
        rotation = "1 0 0 0";
        scale = "1 1 1";
        visibleDistance = "500";
        useSkyTextures = "1";
        renderBottomTexture = "1";
        SkySolidColor = "0.600000 0.600000 0.600000 1.000000";
        fogDistance = "300";
        fogColor = "0.600000 0.600000 0.600000 1.000000";
        materialList = "~/data/skies/sky_day.dml";
        noRenderBans = "1";
     };
     new Sun() {
        direction = "0.57735 0.57735 -0.57735";
        color = "0.600000 0.600000 0.600000 1.000000";
        ambient = "0.400000 0.400000 0.400000 1.000000";
     };
   };
}

