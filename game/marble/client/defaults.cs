//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// The master server is declared with the server defaults, which is
// loaded on both clients & dedicated servers.  If the server mod
// is not loaded on a client, then the master must be defined. 
//$pref::Master[0] = "2:v12master.dyndns.org:28002";

$pref::Player::Name = "Test Guy";
$pref::Player::defaultFov = 90;
$pref::Player::zoomSpeed = 0;

$pref::QualifiedLevel["Beginner"] = 1;
$pref::QualifiedLevel["Intermediate"] = 1;
$pref::QualifiedLevel["Advanced"] = 1;
$pref::QualifiedLevel["Custom"] = 1000;

$pref::checkMOTDAndVersion = "1"; // check the version by default

$pref::Net::LagThreshold = 400;

$pref::shadows = "2";
$pref::HudMessageLogSize = 40;
$pref::ChatHudLength = 1;
$pref::useStencilShadows = false;

$pref::Input::LinkMouseSensitivity = 1;
// DInput keyboard, mouse, and joystick prefs
$pref::Input::KeyboardEnabled = 1;
$pref::Input::MouseEnabled = 1;
$pref::Input::JoystickEnabled = 0;
$pref::Input::KeyboardTurnSpeed = 0.025;

$pref::Input::MouseSensitivity = 0.75;
$pref::Input::InvertYAxis = false;
$pref::Input::AlwaysFreeLook = false;

$pref::sceneLighting::cacheSize = 20000;
$pref::sceneLighting::purgeMethod = "lastCreated";
$pref::sceneLighting::cacheLighting = 1;
$pref::sceneLighting::terrainGenerateLevel = 1;

$pref::Terrain::DynamicLights = 1;
$pref::Interior::TexturedFog = 0;
$pref::Video::displayDevice = "OpenGL";
$pref::Video::allowOpenGL = 1;
$pref::Video::allowD3D = 1;
$pref::Video::preferOpenGL = 1;
$pref::Video::appliedPref = 0;
$pref::Video::disableVerticalSync = 1;
$pref::Video::monitorNum = 0;
$pref::Video::resolution = "800 600 32";
$pref::Video::windowedRes = "800 600";
$pref::Video::fullScreen = "1";

$pref::OpenGL::force16BitTexture = "0";
$pref::OpenGL::forcePalettedTexture = "0";
$pref::OpenGL::maxHardwareLights = 3;
$pref::VisibleDistanceMod = 1.0;

$pref::Audio::driver = "OpenAL";
$pref::Audio::forceMaxDistanceUpdate = 0;
$pref::Audio::environmentEnabled = 0;
$pref::Audio::masterVolume   = 1.0;
$pref::Audio::channelVolume1 = 1.0;
$pref::Audio::channelVolume2 = 0.7;
$pref::Audio::channelVolume3 = 0.8;
$pref::Audio::channelVolume4 = 0.8;
$pref::Audio::channelVolume5 = 0.8;
$pref::Audio::channelVolume6 = 0.8;
$pref::Audio::channelVolume7 = 0.8;
$pref::Audio::channelVolume8 = 0.8;

$pref::LastReadMOTD = "Welcome to MarbleBlast!";
$pref::CurrentMOTD = "Welcome to MarbleBlast!";

$Pref::Unix::OpenALFrequency = 44100;
