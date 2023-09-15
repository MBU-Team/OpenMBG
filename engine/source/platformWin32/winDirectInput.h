//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _WINDIRECTINPUT_H_
#define _WINDIRECTINPUT_H_

#ifndef _PLATFORMWIN32_H_
#include "platformWin32/platformWin32.h"
#endif
#ifndef _PLATFORMINPUT_H_
#include "platform/platformInput.h"
#endif
#ifndef _WINDINPUTDEVICE_H_
#include "platformWin32/winDInputDevice.h"
#endif

#include <dinput.h>

struct InputEvent;

//------------------------------------------------------------------------------
class DInputManager : public InputManager
{
   private:
      typedef SimGroup Parent;

      HMODULE        mDInputLib;
      LPDIRECTINPUT8A mDInputInterface;

      static bool smKeyboardEnabled;
      static bool smMouseEnabled;
      static bool smJoystickEnabled;

      bool mKeyboardActive;
      bool mMouseActive;
      bool mJoystickActive;

      void  enumerateDevices();

      static BOOL CALLBACK EnumDevicesProc( const DIDEVICEINSTANCEA *pddi, LPVOID pvRef ); 

      bool acquire( U8 deviceType, U8 deviceID );
      void unacquire( U8 deviceType, U8 deviceID );

   public:
      DInputManager();

      bool enable();
      void disable();

      void onDeleteNotify( SimObject* object );
      bool onAdd();
      void onRemove();

      void process();

      // DirectInput functions:
      static void init();

      static bool enableKeyboard();
      static void disableKeyboard();
      static bool isKeyboardEnabled();
      bool activateKeyboard();
      void deactivateKeyboard();
      bool isKeyboardActive()       { return( mKeyboardActive ); }

      static bool enableMouse();
      static void disableMouse();
      static bool isMouseEnabled();
      bool activateMouse();
      void deactivateMouse();
      bool isMouseActive()          { return( mMouseActive ); }          

      static bool enableJoystick();
      static void disableJoystick();
      static bool isJoystickEnabled();
      bool activateJoystick();
      void deactivateJoystick();
      bool isJoystickActive()       { return( mJoystickActive ); }          

      // Console interface:
      const char* getJoystickAxesString( U32 deviceID );
};

#endif  // _H_WINDIRECTINPUT_
