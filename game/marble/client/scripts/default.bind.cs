//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

if ( isObject( moveMap ) )
   moveMap.delete();
new ActionMap(moveMap);

if ( isObject( demoMap) )
   demoMap.delete();
new ActionMap(demoMap);

if ( isObject( gamepadMap ) )
   gamepadMap.delete();

new ActionMap(gamepadMap);

demoMap.bindCmd(keyboard, "escape", "", "onDemoPlayDone(true);");

//------------------------------------------------------------------------------
// Non-remapable binds
//------------------------------------------------------------------------------

function escapeFromGame()
{
   if ( $Game::State $= "End")
      return;
   if ( $Server::ServerType $= "SinglePlayer" )  {
      // In single player, we'll pause the game while the dialog box is up.
      pauseGame();
      ExitGameText.setText("<just:center><font:DomCasualD:32>Exit from this Level?");
      Canvas.pushDialog(ExitGameDlg);
   }
   else
      MessageBoxYesNo( "Disconnect", "Disconnect from the server?",
         "disconnect();", "");
}

moveMap.bindCmd(keyboard, "escape", "", "escapeFromGame();");


//------------------------------------------------------------------------------
// Movement Keys
//------------------------------------------------------------------------------

$movementSpeed = 1; // m/s

function setSpeed(%speed)
{
   if(%speed)
      $movementSpeed = %speed;
}

function moveleft(%val)
{
   $mvLeftAction = %val;
}

function moveright(%val)
{
   $mvRightAction = %val;
}

function moveforward(%val)
{
   $mvForwardAction = %val;
}

function movebackward(%val)
{
   $mvBackwardAction = %val;
}

function moveXAxis(%val)
{
   if(mAbs(%val) < 0.1)
      %val = 0;
   if(%val < 0)
   {
      $mvLeftAction = -%val;
      $mvRightAction = 0;
   }
   else
   {
      $mvLeftAction = 0;
      $mvRightAction  = %val;
   }
}

function moveYAxis(%val)
{
   if(mAbs(%val) < 0.05)
      %val = 0;
   if(%val < 0)
   {
      $mvForwardAction = -%val;
      $mvBackwardAction = 0;
   }
   else
   {
      $mvForwardAction = 0;
      $mvBackwardAction  = %val;
   }
}

function moveYawAxis(%val)
{
   if(mAbs(%val) < 0.05)
      %val = 0;
   if(%val < 0)
   {
      $mvYawRightSpeed = -%val * $Pref::Input::KeyboardTurnSpeed;
      $mvYawLeftSpeed = 0;
   }
   else
   {
      $mvYawRightSpeed = 0;
      $mvYawLeftSpeed = %val * $Pref::Input::KeyboardTurnSpeed;
   }
}

function movePitchAxis(%val)
{
   //if(mAbs(%val) < 0.3)
   //{
   //   %val = 0;
      //$mvPitchUpSpeed = $mvPitchDownSpeed = 0;
      //return;
   //}
   %ival = %val;

   if(%val < 0)
   {
      //$mvPitchUpSpeed = -%val * $Pref::Input::KeyboardTurnSpeed;
      if(%val < -0.3)
      {
         %val = (%val + 0.3) * (1.0 / 0.7);
         //%val = %val 
         %destPitch = (-%val * 1.05) + 0.45;
      }
      else
         %destPitch = 0.45;
      //$mvPitchDownSpeed = 0;
   }
   else
   {
      if(%val > 0.3)
      {
         %val = (%val - 0.3) * (1.0 / 0.7);
         //$mvPitchUpSpeed = 0;
         //$mvPitchDownSpeed = %val * $Pref::Input::KeyboardTurnSpeed;
         %destPitch = (-1.3 * %val) + 0.45;
      }
      else
         %destPitch = 0.45;
   }
   $mvPitch = %destPitch - $marblePitch;
   //echo("IVAL= " @ %ival @ " MVMP = " @ $marblePitch @ " DestPitch = " @ %destPitch);
//   if(%destPitch > $marblePitch)
   //{
      //$mvPitchDownSpeed = 0;
   //}
   //else
   //{
      //$mvPitchDownSpeed = $marblePitch - %destPitch;
      //$mvPitchUpSpeed = 0;
   //   $mvPitch = 
   //}
}

gamepadMap.bind(joystick, xaxis, moveXAxis);
gamepadMap.bind(joystick, yaxis, moveYAxis);
gamepadMap.bind(joystick, rzaxis, moveYawAxis);
gamepadMap.bind(joystick, slider, movePitchAxis);
gamepadMap.bind(joystick, button6, jump);
gamepadMap.bind(joystick, button7, mouseFire);

gamepadMap.push();

function moveup(%val)
{
   $mvUpAction = %val;
}

function movedown(%val)
{
   $mvDownAction = %val;
}

function turnLeft( %val )
{
   $mvYawRightSpeed = %val ? $Pref::Input::KeyboardTurnSpeed : 0;
}

function turnRight( %val )
{
   $mvYawLeftSpeed = %val ? $Pref::Input::KeyboardTurnSpeed : 0;
}

function panUp( %val )
{
   $mvPitchDownSpeed = %val ? $Pref::Input::KeyboardTurnSpeed : 0;
}

function panDown( %val )
{
   $mvPitchUpSpeed = %val ? $Pref::Input::KeyboardTurnSpeed : 0;
}

function getMouseAdjustAmount(%val)
{
   // based on a default camera fov of 90'
   return($pref::Input::MouseSensitivity * %val * ($cameraFov / 90) * 0.01);
}

function yaw(%val)
{
   $mvYaw += getMouseAdjustAmount(%val);
}

function pitch(%val)
{
   if($freelooking || $pref::Input::AlwaysFreeLook)
   {
      if($pref::input::InvertYAxis)
         $mvPitch -= getMouseAdjustAmount(%val);
      else
         $mvPitch += getMouseAdjustAmount(%val);
   }
}

function jump(%val)
{
   $mvTriggerCount2++;
}

function freelook(%val)
{
   $freeLooking = %val;
}

moveMap.bind( keyboard, a, moveleft );
moveMap.bind( keyboard, d, moveright );
moveMap.bind( keyboard, w, moveforward );
moveMap.bind( keyboard, s, movebackward );
moveMap.bind( keyboard, space, jump );
moveMap.bind( mouse, xaxis, yaw );
moveMap.bind( mouse, yaxis, pitch );
moveMap.bind(keyboard, up, panUp);
moveMap.bind(keyboard, down, panDown);
moveMap.bind(keyboard, left, turnLeft);
moveMap.bind(keyboard, right, turnRight);


//------------------------------------------------------------------------------
// Mouse Trigger
//------------------------------------------------------------------------------

function mouseFire(%val)
{
   $mvTriggerCount0++;
}

function altTrigger(%val)
{
   $mvTriggerCount1++;
}

moveMap.bind( mouse, button0, mouseFire );
if($platform $= "macos")
   moveMap.bind( keyboard, e, freelook );
else
   moveMap.bind( mouse, button1, freelook );


//------------------------------------------------------------------------------
// Camera & View functions
//------------------------------------------------------------------------------

function toggleCamera(%val)
{
   if (%val && $testCheats)
      commandToServer('ToggleCamera');
}

moveMap.bind(keyboard, "alt c", toggleCamera);

//------------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------------

function dropCameraAtPlayer(%val)
{
   if (%val && $testCheats)
      commandToServer('dropCameraAtPlayer');
}

function dropPlayerAtCamera(%val)
{
   if (%val && $testCheats)
      commandToServer('DropPlayerAtCamera');
}

moveMap.bind(keyboard, "F8", dropCameraAtPlayer);
moveMap.bind(keyboard, "F7", dropPlayerAtCamera);


//------------------------------------------------------------------------------
// Dubuging Functions
//------------------------------------------------------------------------------

$MFDebugRenderMode = 0;
function cycleDebugRenderMode(%val)
{
   if (!%val)
      return;

   if (getBuildString() $= "Debug")
   {
      if($MFDebugRenderMode == 0)
      {
         // Outline mode, including fonts so no stats
         $MFDebugRenderMode = 1;
         GLEnableOutline(true);
      }
      else if ($MFDebugRenderMode == 1)
      {
         // Interior debug mode
         $MFDebugRenderMode = 2;
         GLEnableOutline(false);
         setInteriorRenderMode(7);
         showInterior();
      }
      else if ($MFDebugRenderMode == 2)
      {
         // Back to normal
         $MFDebugRenderMode = 0;
         setInteriorRenderMode(0);
         GLEnableOutline(false);
         show();
      }
   }
   else
   {
      echo("Debug render modes only available when running a Debug build.");
   }
}

GlobalActionMap.bind(keyboard, "F9", cycleDebugRenderMode);


//------------------------------------------------------------------------------
// Misc.
//------------------------------------------------------------------------------

GlobalActionMap.bind(keyboard, "tilde", toggleConsole);
GlobalActionMap.bindCmd(keyboard, "alt enter", "", "pauseMusic();toggleFullScreen();resumeMusic();");
