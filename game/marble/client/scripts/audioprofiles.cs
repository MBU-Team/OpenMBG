//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// Channel assignments (channel 0 is unused in-game).

$GuiAudioType     = 1;
$SimAudioType     = 1;
$MessageAudioType = 1;
$EffectAudioType = 1;
$MusicAudioType = 2;

new AudioDescription(AudioGui)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = $GuiAudioType;
};

new AudioDescription(AudioMessage)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = $MessageAudioType;
};

new AudioDescription(ClientAudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   type = $EffectAudioType;
};

new AudioProfile(TimeTravelLoopSfx)
{
   filename    = "~/data/sound/TimeTravelActive.wav";
   description = ClientAudioLooping2d;
   preload = true;
};

new AudioProfile(AudioButtonOver)
{
   filename = "~/data/sound/buttonOver.wav";
   description = "AudioGui";
	preload = true;
};

new AudioProfile(AudioButtonDown)
{
   filename = "~/data/sound/ButtonPress.wav";
   description = "AudioGui";
	preload = true;
};

new AudioDescription(AudioMusic)
{
   volume   = 1.0;
   isLooping = true;
   isStreaming = true;
   is3D     = false;
   type     = $MusicAudioType;
};

function playMusic(%musicFileBase)
{
   alxStop($currentMusicHandle);
   if(isObject(MusicProfile))
      MusicProfile.delete();

   %file = "~/data/sound/" @ %musicFileBase;
   new AudioProfile(MusicProfile) {
      fileName = %file;
      description = "AudioMusic";
      preload = false;
   };
   $currentMusicBase = %musicFileBase;
   $currentMusicHandle = alxPlay(MusicProfile);  //add this line
}

function playShellMusic()
{
   playMusic("Shell.ogg");
}

function playGameMusic()
{
   if(!$musicFound)
   {
      $NumMusicFiles = 0;
      for(%file = findFirstFile("*.ogg"); %file !$= ""; %file = findNextFile("*.ogg"))
      {
         if(fileBase(%file) !$= "Shell")
         {
            $Music[$NumMusicFiles] = fileBase(%file) @ ".ogg";
            $NumMusicFiles++;
         }
      }
      $musicFound = true;
   }   
   if($NumMusicFiles)
      playMusic($Music[MissionInfo.level % $NumMusicFiles]);
   else
      playMusic("Shell.ogg");
}

function pauseMusic()
{
   alxStop($currentMusicHandle);
}

function resumeMusic()
{
   playMusic($currentMusicBase);
}

