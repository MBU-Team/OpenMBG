function optionsDlg::setPane(%this, %pane)
{
   OptAudioPane.setVisible(false);
   OptGraphicsPaneMac.setVisible(false);
   OptGraphicsPane.setVisible(false);
   OptNetworkPane.setVisible(false);
   OptControlsPane.setVisible(false);
   if(%pane $= "Graphics")
   {
      if($platform $= "windows")
         OptGraphicsPane.setVisible(true);
      else
      {
         OptGraphicsPaneMac.setVisible(true);
         if ($platform $= "x86UNIX")
            StencilShadowsButtonMac.setVisible(true);
      }
   }
   else
      ("Opt" @ %pane @ "Pane").setVisible(true);
   OptControlsPane.showMappings();
   %par = OptBoxFrame.getGroup();

   RootGroup.add(OptBoxFrame);
   RootGroup.add("Opt" @ %pane @ "Tab");
   %par.add(OptBoxFrame);
   %par.add("Opt" @ %pane @ "Tab");
}

function OptGraphicsPane::setResolution(%this, %res)
{
   $pref::Video::resolution = %res SPC getWord($pref::Video::resolution, 2);
}

function OptGraphicsPane::setMode(%this, %mode)
{
   $pref::Video::fullScreen = (%mode $= "Full");
}

function OptGraphicsPane::setDriver(%this, %driver)
{
   $pref::Video::displayDevice = %driver;
}

function OptGraphicsPane::setDepth(%this, %depth)
{
   $pref::Video::resolution = getWords($pref::Video::resolution, 0, 1) SPC %depth;
}

function OptionsDlg::onWake(%this)
{
   if ($platform $= "x86UNIX")
      // enable stencil shadows button text on unix
      OptGfxText.setText("<font:DomCasualD:32><just:right>Screen Resolution:\n\nScreen Style:\n\nColor Depth:\n\nStencil Shadows:\n\n");
   else
      OptGfxText.setText("<font:DomCasualD:32><just:right>Screen Resolution:\n\nScreen Style:\n\nColor Depth:\n\n");
      
   %this.setPane("Graphics");
   %buffer = getDisplayDeviceList();
   %count = getFieldCount( %buffer );

   OptGfx640480.setValue(false);
   OptGfx800600.setValue(false);
   OptGfx1024768.setValue(false);
   OptGfx640480Mac.setValue(false);
   OptGfx800600Mac.setValue(false);
   OptGfx1024768Mac.setValue(false);

   %res = getWords($pref::Video::resolution, 0, 1);
   switch$(%res)
   {
      case "640 480":
         OptGfx640480.setValue(true);
         OptGfx640480Mac.setValue(true);
      case "800 600":
         OptGfx800600.setValue(true);
         OptGfx800600Mac.setValue(true);
      case "1024 768":
         OptGfx1024768.setValue(true);
         OptGfx1024768Mac.setValue(true);
   }
   %isOgl = $pref::Video::displayDevice $= "OpenGL";
   OptGfxOpenGL.setValue(%isOgl);
   OptGfxD3D.setValue(!%isOgl);

   OptGfxFull.setValue($pref::Video::fullScreen);
   OptGfxWindow.setValue(!$pref::Video::fullScreen);
   OptGfxFullMac.setValue($pref::Video::fullScreen);
   OptGfxWindowMac.setValue(!$pref::Video::fullScreen);
   
   %is32 = getWord($pref::Video::resolution, 2) == 32;
   OptGfx16.setValue(!%is32);
   OptGfx32.setValue(%is32);
   OptGfx16Mac.setValue(!%is32);
   OptGfx32Mac.setValue(%is32);

   OptGraphicsDriverMenu.clear();
   for(%i = 0; %i < %count; %i++)
      OptGraphicsDriverMenu.add(getField(%buffer, %i), %i);
   %selId = OptGraphicsDriverMenu.findText( $pref::Video::displayDevice );
	if ( %selId == -1 )
		%selId = 0; // How did THAT happen?
	OptGraphicsDriverMenu.setSelected( %selId );
	OptGraphicsDriverMenu.onSelect( %selId, "" );

   // Audio 
   OptAudioUpdate();
   OptAudioVolumeMaster.setValue( $pref::Audio::masterVolume);
   OptAudioVolumeShell.setValue(  $pref::Audio::channelVolume[$GuiAudioType]);
   OptAudioVolumeSim.setValue(    $pref::Audio::channelVolume[$SimAudioType]);
   OptAudioDriverList.clear();
   OptAudioDriverList.add("OpenAL", 1);
   OptAudioDriverList.add("none", 2);
   %selId = OptAudioDriverList.findText($pref::Audio::driver);
	if ( %selId == -1 )
		%selId = 0; // How did THAT happen?
	OptAudioDriverList.setSelected( %selId );
	OptAudioDriverList.onSelect( %selId, "" );
}

function OptionsDlg::onSleep(%this)
{
   // write out the control config into the fps/config.cs file
   moveMap.save( "~/client/config.cs" );

}

function OptGraphicsDriverMenu::onSelect( %this, %id, %text )
{
	// Attempt to keep the same res and bpp settings:
	if ( OptGraphicsResolutionMenu.size() > 0 )
		%prevRes = OptGraphicsResolutionMenu.getText();
	else
		%prevRes = getWords( $pref::Video::resolution, 0, 1 );

	// Check if this device is full-screen only:
	if ( isDeviceFullScreenOnly( %this.getText() ) )
	{
		OptGraphicsFullscreenToggle.setValue( true );
		OptGraphicsFullscreenToggle.setActive( false );
		OptGraphicsFullscreenToggle.onAction();
	}
	else
		OptGraphicsFullscreenToggle.setActive( true );

	if ( OptGraphicsFullscreenToggle.getValue() )
	{
		if ( OptGraphicsBPPMenu.size() > 0 )
			%prevBPP = OptGraphicsBPPMenu.getText();
		else
			%prevBPP = getWord( $pref::Video::resolution, 2 );
	}

	// Fill the resolution and bit depth lists:
	OptGraphicsResolutionMenu.init( %this.getText(), OptGraphicsFullscreenToggle.getValue() );
	OptGraphicsBPPMenu.init( %this.getText() );

	// Try to select the previous settings:
	%selId = OptGraphicsResolutionMenu.findText( %prevRes );
	if ( %selId == -1 )
		%selId = 0;
	OptGraphicsResolutionMenu.setSelected( %selId );

	if ( OptGraphicsFullscreenToggle.getValue() )
	{
		%selId = OptGraphicsBPPMenu.findText( %prevBPP );
		if ( %selId == -1 )
			%selId = 0;
		OptGraphicsBPPMenu.setSelected( %selId );
		OptGraphicsBPPMenu.setText( OptGraphicsBPPMenu.getTextById( %selId ) );
	}
	else
		OptGraphicsBPPMenu.setText( "Default" );

}

function OptGraphicsResolutionMenu::init( %this, %device, %fullScreen )
{
	%this.clear();
	%resList = getResolutionList( %device );
	%resCount = getFieldCount( %resList );
	%deskRes = getDesktopResolution();

   %count = 0;
	for ( %i = 0; %i < %resCount; %i++ )
	{
		%res = getWords( getField( %resList, %i ), 0, 1 );

		if ( !%fullScreen )
		{
			if ( firstWord( %res ) >= firstWord( %deskRes ) )
				continue;
			if ( getWord( %res, 1 ) >= getWord( %deskRes, 1 ) )
				continue;
		}

		// Only add to list if it isn't there already:
		if ( %this.findText( %res ) == -1 )
      {
			%this.add( %res, %count );
         %count++;
      }
	}
}

function OptGraphicsFullscreenToggle::onAction(%this)
{
   Parent::onAction();
   %prevRes = OptGraphicsResolutionMenu.getText();

   // Update the resolution menu with the new options
   OptGraphicsResolutionMenu.init( OptGraphicsDriverMenu.getText(), %this.getValue() );

   // Set it back to the previous resolution if the new mode supports it.
   %selId = OptGraphicsResolutionMenu.findText( %prevRes );
   if ( %selId == -1 )
   	%selId = 0;
 	OptGraphicsResolutionMenu.setSelected( %selId );
}


function OptGraphicsBPPMenu::init( %this, %device )
{
	%this.clear();

	if ( %device $= "Voodoo2" )
		%this.add( "16", 0 );
	else
	{
		%resList = getResolutionList( %device );
		%resCount = getFieldCount( %resList );
      %count = 0;
		for ( %i = 0; %i < %resCount; %i++ )
		{
			%bpp = getWord( getField( %resList, %i ), 2 );

			// Only add to list if it isn't there already:
			if ( %this.findText( %bpp ) == -1 )
         {
				%this.add( %bpp, %count );
            %count++;
         }
		}
	}
}

function optionsDlg::applyGraphics( %this )
{
//	%newDriver = OptGraphicsDriverMenu.getText();
//	%newRes = OptGraphicsResolutionMenu.getText();
//	%newBpp = OptGraphicsBPPMenu.getText();
//	%newFullScreen = OptGraphicsFullscreenToggle.getValue();
   pauseMusic();
	if ($pref::Video::displayDevice !$= getDisplayDeviceName())
	{
		setDisplayDevice( $pref::Video::displayDevice, 
		            firstWord( $pref::Video::resolution ), 
		            getWord( $pref::Video::resolution, 1 ), 
		            getWord( $pref::Video::resolution, 2), 
		            $pref::Video::fullScreen );
		//OptionsDlg::deviceDependent( %this );
	}
	else if($pref::Video::resolution !$= getResolution())
   {
		setScreenMode( firstWord( $pref::Video::resolution ), 
		            getWord( $pref::Video::resolution, 1 ), 
		            getWord( $pref::Video::resolution, 2), 
		            $pref::Video::fullScreen );
   }
   else if($pref::Video::fullScreen != isFullScreen())
      toggleFullScreen();
   resumeMusic();
}



$RemapCount = 0;
$RemapName[$RemapCount] = "Move Forward";
$RemapCmd[$RemapCount] = "moveforward";
$RemapCount++;
$RemapName[$RemapCount] = "Move Backward";
$RemapCmd[$RemapCount] = "movebackward";
$RemapCount++;
$RemapName[$RemapCount] = "Move Left";
$RemapCmd[$RemapCount] = "moveleft";
$RemapCount++;
$RemapName[$RemapCount] = "Move Right";
$RemapCmd[$RemapCount] = "moveright";
$RemapCount++;
$RemapName[$RemapCount] = "Jump";
$RemapCmd[$RemapCount] = "jump";
$RemapCount++;
$RemapName[$RemapCount] = "Use PowerUp";
$RemapCmd[$RemapCount] = "mouseFire";
$RemapCount++;
$RemapName[$RemapCount] = "Rotate Camera Left";
$RemapCmd[$RemapCount] = "turnLeft";
$RemapCount++;
$RemapName[$RemapCount] = "Rotate Camera Right";
$RemapCmd[$RemapCount] = "turnRight";
$RemapCount++;
$RemapName[$RemapCount] = "Rotate Camera Up";
$RemapCmd[$RemapCount] = "panUp";
$RemapCount++;
$RemapName[$RemapCount] = "Rotate Camera Down";
$RemapCmd[$RemapCount] = "panDown";
$RemapCount++;
$RemapName[$RemapCount] = "Free Look";
$RemapCmd[$RemapCount] = "freelook";
$RemapCount++;


function restoreDefaultMappings()
{
   moveMap.delete();
   exec( "~/client/scripts/default.bind.cs" );
   OptRemapList.fillList();
}

function getMapDisplayName( %device, %action, %fullText )
{
	if ( %device $= "keyboard" )
   {
	   if(%action $= "space")
         return "Space Bar";
		return( upperFirst(%action) );
   }
	else if ( strstr( %device, "mouse" ) != -1 )
	{
		// Substitute "mouse" for "button" in the action string:
		%pos = strstr( %action, "button" );
		if ( %pos != -1 )
		{
			%mods = getSubStr( %action, 0, %pos );
			%object = getSubStr( %action, %pos, 1000 );
			%instance = getSubStr( %object, strlen( "button" ), 1000 );
			if(%fullText)
         {
            if(%instance < 2)
            {
   		      if(%mods $= "")
                  %mods = "the ";
   		      if($platform $= "macos" && %instance == 0)
                  return %mods @ "Mouse Button";
               if(%instance == 0)
                  return %mods @ "Left Mouse Button";
               return %mods @ "Right Mouse Button";
            }
            else
      			return( %mods @ "Mouse Button " @ ( %instance + 1 ) );
	      
	      }
	      else
         {
            if(%instance < 2)
            {
               if($platform $= "macos" && %instance == 0)
                  return %mods @ "Mouse Button";
               if(%instance == 0)
                  return %mods @ "Left Mouse";
               return %mods @ "Right Mouse";
            }
   	   	else
   	   	   return( %mods @ "Mouse Btn. " @ ( %instance + 1 ) );
         }
		}
		else
			error( "Mouse input object other than button passed to getDisplayMapName!" );
	}
	else if ( strstr( %device, "joystick" ) != -1 )
	{
		// Substitute "joystick" for "button" in the action string:
		%pos = strstr( %action, "button" );
		if ( %pos != -1 )
		{
			%mods = getSubStr( %action, 0, %pos );
			%object = getSubStr( %action, %pos, 1000 );
			%instance = getSubStr( %object, strlen( "button" ), 1000 );
			return( %mods @ "Joystick" @ ( %instance + 1 ) );
		}
		else
	   { 
	      %pos = strstr( %action, "pov" );
         if ( %pos != -1 )
         {
            %wordCount = getWordCount( %action );
            %mods = %wordCount > 1 ? getWords( %action, 0, %wordCount - 2 ) @ " " : "";
            %object = getWord( %action, %wordCount - 1 );
            switch$ ( %object )
            {
               case "upov":   %object = "POV1 up";
               case "dpov":   %object = "POV1 down";
               case "lpov":   %object = "POV1 left";
               case "rpov":   %object = "POV1 right";
               case "upov2":  %object = "POV2 up";
               case "dpov2":  %object = "POV2 down";
               case "lpov2":  %object = "POV2 left";
               case "rpov2":  %object = "POV2 right";
               default:       %object = "??";
            }
            return( %mods @ %object );
         }
         else
            error( "Unsupported Joystick input object passed to getDisplayMapName!" );
      }
	}
		
	return( "??" );		
}

function buildFullMapString( %index )
{
   %name       = $RemapName[%index];
   %cmd        = $RemapCmd[%index];

	%temp = moveMap.getBinding( %cmd );
   %device = getField( %temp, 0 );
   %object = getField( %temp, 1 );
   if ( %device !$= "" && %object !$= "" )
	   %mapString = getMapDisplayName( %device, %object );
   else
      %mapString = "";

	return( %cmd TAB %mapString );
}

function OptRemapList::fillList( %this )
{
}

//------------------------------------------------------------------------------

function OptControlsPane::showMappings()
{
   for ( %i = 0; %i < $RemapCount; %i++ )
   {
      %str = buildFullMapString(%i);
      %ctrl = nameToId("remap_" @ getField(%str, 0));
      %ctrl.setText(getField(%str, 1));
   }
}

function OptControlsPane::remap(%this, %ctrl, %name )
{
   OptRemapText.setText( "<just:center><font:DomCasualD:24>Press a new key or button for \"" @ %name @ "\"" );
   OptRemapInputCtrl.ctrl = %ctrl;
   OptRemapInputCtrl.nameText = %name;
	Canvas.pushDialog( RemapDlg );
}

//------------------------------------------------------------------------------
function redoMapping( %device, %action, %cmd)
{
	//%actionMap.bind( %device, %action, $RemapCmd[%newIndex] );
	moveMap.bind( %device, %action, %cmd );
   OptControlsPane.showMappings();
}

//------------------------------------------------------------------------------
function findRemapCmdIndex( %command )
{
	for ( %i = 0; %i < $RemapCount; %i++ )
	{
		if ( %command $= $RemapCmd[%i] )
			return( %i );			
	}
	return( -1 );	
}

function OptRemapInputCtrl::onInputEvent( %this, %device, %action )
{
	//error( "** onInputEvent called - device = " @ %device @ ", action = " @ %action @ " **" );
	Canvas.popDialog( RemapDlg );

	// Test for the reserved keystrokes:
	if ( %device $= "keyboard" )
	{
      // Cancel...
      if ( %action $= "escape" )
      {
         // Do nothing...
		   return;
      }
	}
	if(%action $= "")
      return;

   %cmd  = %this.ctrl;
   %name = %this.nameText;

	// First check to see if the given action is already mapped:
	%prevMap = moveMap.getCommand( %device, %action );
   if ( %prevMap !$= %cmd )
   {
	   if ( %prevMap $= "" )
	   {
         moveMap.bind( %device, %action, %cmd );
		   OptControlsPane.showMappings();
	   }
	   else
	   {
         %mapName = getMapDisplayName( %device, %action );
		   %prevMapIndex = findRemapCmdIndex( %prevMap );
		   if ( %prevMapIndex == -1 )
			   MessageBoxOK( "REMAP FAILED", "\"" @ %mapName @ "\" is already bound to a non-remappable command!" );
		   else
         {
            %prevCmdName = $RemapName[%prevMapIndex];
			   MessageBoxYesNo( "WARNING", 
				   "\"" @ %mapName @ "\" is already bound to \"" 
					   @ %prevCmdName @ "\"!\nDo you want to undo this mapping?", 
				   "redoMapping(" @ %device @ ", \"" @ %action @ "\", \"" @ %cmd @ "\");", "" );
         }
		   return;
	   }
   }
}

// Audio 
function OptAudioUpdate()
{
   // set the driver text
   %text =   "Vendor: " @ alGetString("AL_VENDOR") @
           "\nVersion: " @ alGetString("AL_VERSION") @
           "\nRenderer: " @ alGetString("AL_RENDERER");

   // don't display the extensions on linux.  there's too many of them and 
   // they mess up the control
   if ($platform $= "x86UNIX")
      %text = %text @ "\nExtensions: (See Console) ";
   else
      %text = %text @ "\nExtensions: " @ alGetString("AL_EXTENSIONS");
   OptAudioInfo.setText(%text);
}


// Channel 0 is unused in-game, but is used here to test master volume.

new AudioDescription(AudioChannel0)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 0;
};

new AudioDescription(AudioChannel1)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 1;
};

new AudioDescription(AudioChannel2)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 2;
};

new AudioDescription(AudioChannel3)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 3;
};

new AudioDescription(AudioChannel4)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 4;
};

new AudioDescription(AudioChannel5)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 5;
};

new AudioDescription(AudioChannel6)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 6;
};

new AudioDescription(AudioChannel7)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 7;
};

new AudioDescription(AudioChannel8)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = 8;
};

$AudioTestHandle = 0;

function OptAudioUpdateMasterVolume(%volume)
{
   if (%volume == $pref::Audio::masterVolume)
      return;
   alxListenerf(AL_GAIN_LINEAR, %volume);
   $pref::Audio::masterVolume = %volume;
   if (!alxIsPlaying($AudioTestHandle))
   {
      $AudioTestHandle = alxCreateSource("AudioChannel0", expandFilename("~/data/sound/testing.wav"));
      alxPlay($AudioTestHandle);
   }
}

function OptAudioUpdateChannelVolume(%channel)
{
   if (%channel < 1 || %channel > 8)
      return;
         
   alxSetChannelVolume(%channel, $pref::Audio::channelVolume[%channel]);
   if (!alxIsPlaying($AudioTestHandle) && %channel == 1)
   {
      $AudioTestHandle = alxCreateSource("AudioChannel"@%channel, expandFilename("~/data/sound/testing.wav"));
      alxPlay($AudioTestHandle);
   }
}


function OptAudioDriverList::onSelect( %this, %id, %text )
{
   if (%text $= "")
      return;
   
   if ($pref::Audio::driver $= %text)
      return;

   $pref::Audio::driver = %text;
   OpenALInit();
}

