$NumHelpPages = 12;

function HelpCreditsGui::onWake(%this)
{
   $CurHelpPage = 1;
   %this.setPage($CurHelpPage);
}

function HelpCreditsGui::setPage(%this, %page)
{
   HC_StartPad.setVisible(false);
   HC_EndPad.setVisible(false);
   HC_Gem1.setVisible(false);
   HC_Gem2.setVisible(false);
   HC_Gem3.setVisible(false);
   HC_SuperSpeed.setVisible(false);
   HC_SuperJump.setVisible(false);
   HC_ShockAbsorber.setVisible(false);
   HC_AntiGravity.setVisible(false);
   HC_Helicopter.setVisible(false);
   HC_TimeTravel.setVisible(false);
   HC_DuctFan.setVisible(false);
   HC_Tornado.setVisible(false);
   HC_Trapdoor.setVisible(false);
   HC_Oilslick.setVisible(false);
   HC_Landmine.setVisible(false);
   HC_Bumper.setVisible(false);
   HC_SuperBounce.setVisible(false);

   switch(%page)
   {
      case 1:
         %text = "<font:Expo:50><color:000000><just:center>Overview<font:DomCasualD:32><just:left>\n\n" @
                 "Roll your marble through a rich cartoon landscape of moving platforms and dangerous hazards.  Along the way find power ups to increase your speed, jumping ability or flight power, and use them to collect the hidden gems and race to the finish for the fastest time.";
      %text = "<font:Expo:50><color:000000><just:center>Overview<font:DomCasualD:32><just:left>\n\n" @
              "Roll your marble through a rich cartoon landscape of moving platforms and dangerous hazards.  Along the way find power ups to increase your speed, jumping ability or flight power, and use them to collect the hidden gems and race to the finish for the fastest time.";
      case 2:
         %text = "<font:Expo:50><color:000000><just:center>Basic Controls<font:DomCasualD:32><just:left>\n\n" @
                 "The marble can be moved forward, back, left and right by pressing <func:bind moveforward>, <func:bind movebackward>, <func:bind moveleft> and <func:bind moveright>, respectively.  Pressing <func:bind jump> causes the marble to jump, and pressing <func:bind mouseFire> uses whatever powerup you currently have available.  All movement is relative to the view direction.";
      case 3:
         %text = "<font:Expo:50><color:000000><just:center>Camera Controls<font:DomCasualD:32><just:left>\n\n" @
                 "The camera direction can be changed by moving the mouse or by pressing <func:bind panUp>, <func:bind panDown>, <func:bind turnLeft> or <func:bind turnRight>.  In order to look up and down freely with the mouse, hold down <func:bind freelook>.  You can turn free look on always from the Mouse pane of the Control Options screen.";
      case 4:
         %text = "<font:Expo:50><color:000000><just:center>Goals<font:DomCasualD:32><just:left>\n\n" @
                 "<lmargin:70>Start Pad - this is where you start the level.\n\nEnd Pad - roll your marble here to end the level.\n\nGems - if a level has gems, you must pick them all up before you can exit.";
         HC_StartPad.setVisible(true);
         HC_EndPad.setVisible(true);
         HC_Gem1.setVisible(true);
         HC_Gem2.setVisible(true);
         HC_Gem3.setVisible(true);

      case 5:
         %text = "<font:Expo:50><color:000000><just:center>Bonus Items (1/2)<font:Arial:16>\n\n<font:DomCasualD:32><just:left>" @
                 "<lmargin:70>Super Speed PowerUp - gives you a burst of speed.\n\nSuper Jump PowerUp - gives you a big jump up.\n\nShock Absorber PowerUp - absorbs bounce impacts.\n\nSuper Bounce PowerUp - makes you bounce higher.";
         HC_SuperSpeed.setVisible(true);
         HC_SuperJump.setVisible(true);
         HC_ShockAbsorber.setVisible(true);
         HC_SuperBounce.setVisible(true);
      case 6:
         %text = "<font:Expo:50><color:000000><just:center>Bonus Items (2/2)<font:DomCasualD:32><just:left>\n\n" @
                 "<lmargin:70>Gyrocopter PowerUp - slows your fall in the air.\n\nTime Travel - takes some time off the clock.\n\nGravity Modifier - Changes the direction of \"down\" - the new down is in the direction of the arrow.";
         HC_AntiGravity.setVisible(true);
         HC_Helicopter.setVisible(true);
         HC_TimeTravel.setVisible(true);
      case 7:
         %text = "<font:Expo:50><color:000000><just:center>Hazards (1/2)<font:DomCasualD:32><just:left>\n\n" @
                 "<lmargin:70>Duct Fan - be careful this doesn't blow you away!\n\nTornado - it'll pull you in and spit you out.\n\nTrap Door - keep moving when you're rolling over one of these.";
         HC_DuctFan.setVisible(true);
         HC_Tornado.setVisible(true);
         HC_Trapdoor.setVisible(true);
      case 8:
         %text = "<font:Expo:50><color:000000><just:center>Hazards (2/2)<font:DomCasualD:32><just:left>\n\n" @
                 "<lmargin:70>Bumper - this'll bounce you if you touch it.\n\nLand Mine - Warning!  Explodes on contact!\n\nOil Slick - you won't have much traction on these surfaces.";
         HC_Bumper.setVisible(true);
         HC_Landmine.setVisible(true);
         HC_Oilslick.setVisible(true);
      case 9:
         %text = "<font:Expo:50><color:000000><just:center>About GarageGames<font:DomCasualD:32><just:left>\n\n" @
                 "GarageGames is a unique Internet publishing label for independent games and gamemakers.  Our mission is to provide the independent developer with tools, knowledge, co-conspirators - whatever is needed to unleash the creative spirit and get great innovative independent games to market.";
      case 10:
        %text = "<font:Expo:50><color:000000><just:center>About the Torque<font:DomCasualD:32><just:left>\n\n" @
                "The Torque Game Engine (TGE) is a full featured AAA title engine with the latest in scripting, geometry, particle effects, animation and texturing, as well as award winning multi-player networking code.  For $100 per programmer, you get the source to the engine!  Not possible? Check the FAQ for the details.";

      case 11:
         %text = "<font:Expo:50><color:000000><just:center>The Marble Blast Team<font:DomCasualD:32><just:left>\n\n" @
            "<lmargin%:15><rmargin%:85>" @
            "<just:left>Alex Swanson" @ 
            "<just:right>Mark Frohnmayer" @
            "\n<just:left>Jeff Tunnell" @
            "<just:right>Brian Hahn" @
            "\n<just:left>Liam Ryan" @
            "<just:right>Tim Gift" @
            "\n<just:left>Rick Overman" @
            "<just:right>Kevin Ryan" @
            "\n<just:left>Timothy Clarke" @
            "<just:right>Jay Moore" @
            "\n<just:left>Pat Wilson" @
            "<just:right>John Quigley";

      case 12:
         %text = "<font:Expo:50><color:000000><just:center>Special Thanks<font:DomCasualD:32><just:left>\n\n" @
            "We'd like to thank Nullsoft, for the SuperPiMP Install System, " @
            "and Markus F.X.J. Oberhumer, Laszlo Molnar and the rest of the UPX team for the UPX executable packer."  @
            "  Thanks also to Kurtis Seebaldt for his work on integrating Ogg/Vorbis streaming into the Torque engine, and to the Ogg/Vorbis team.";

   }
   HC_Text.setText(%text);
}

function HelpNext()
{
   $CurHelpPage++;
   if($CurHelpPage > $NumHelpPages)
      $CurHelpPage = 1;
   HelpCreditsGui.setPage($CurHelpPage);
}

function HelpPrev()
{
   $CurHelpPage--;
   if($CurHelpPage < 1)
      $CurHelpPage = $NumHelpPages;
   HelpCreditsGui.setPage($CurHelpPage);
}