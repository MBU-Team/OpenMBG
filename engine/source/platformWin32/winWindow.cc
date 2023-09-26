//-----------------------------------------------------------------------------
// Torque Game Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platformWin32/platformWin32.h"
#include "platform/platform.h"
#include "platformWin32/platformGL.h"
#include "platform/platformVideo.h"
#include "platformWin32/winOGLVideo.h"
#include "platformWin32/winD3DVideo.h"
#include "platformWin32/winV2Video.h"
#include "platform/event.h"
#include "console/console.h"
#include "platformWin32/winConsole.h"
#include "platformWin32/winDirectInput.h"
#include "platform/gameInterface.h"
#include "math/mRandom.h"
#include "core/fileStream.h"
#include "game/resource.h"

//-------------------------------------- Resource Includes
#include "dgl/gBitmap.h"
#include "dgl/gFont.h"

extern void createFontInit();
extern void createFontShutdown();
extern void installRedBookDevices();
extern void handleRedBookCallback(U32, U32);

static const char *windowClassName = "Darkstar Window Class";
static char windowName[256] = "Darkstar Window";
static bool gWindowCreated = false;

static MRandomLCG sgPlatRandom;
static bool sgQueueEvents;

// is keyboard input a standard (non-changing) VK keycode
#define dIsStandardVK(c) (((0x08 <= (c)) && ((c) <= 0x12)) || \
                          ((c) == 0x1b) ||                    \
                          ((0x20 <= (c)) && ((c) <= 0x2e)) || \
                          ((0x30 <= (c)) && ((c) <= 0x39)) || \
                          ((0x41 <= (c)) && ((c) <= 0x5a)) || \
                          ((0x70 <= (c)) && ((c) <= 0x7B)))

extern U16  DIK_to_Key( U8 dikCode );

Win32PlatState winState;

//--------------------------------------
Win32PlatState::Win32PlatState()
{
   log_fp      = NULL;
   hinstOpenGL = NULL;
   hinstGLU    = NULL;
   hinstOpenAL = NULL;
   appWindow   = NULL;
   appDC       = NULL;
   appInstance = NULL;
   currentTime = 0;
   processId   = 0;
}

static bool windowLocked = false;

static BYTE keyboardState[256];
static bool mouseButtonState[3];
static bool capsLockDown = false;
static S32 modifierKeys = 0;
static bool windowActive = true;
static Point2I lastCursorPos(0,0);
static Point2I windowSize;
static HANDLE gMutexHandle = NULL;
static bool sgDoubleByteEnabled = false;

//--------------------------------------
static const char *getMessageName(S32 msg)
{
   switch(msg)
   {
      case WM_KEYDOWN:
         return "WM_KEYDOWN";
      case WM_KEYUP:
         return "WM_KEYUP";
      case WM_SYSKEYUP:
         return "WM_SYSKEYUP";
      case WM_SYSKEYDOWN:
         return "WM_SYSKEYDOWN";
      default:
         return "Unknown!!";
   }
}

//--------------------------------------
bool Platform::excludeOtherInstances(const char *mutexName)
{
   gMutexHandle = CreateMutexA(NULL, true, mutexName);
   if(!gMutexHandle)
      return false;
   if(GetLastError() == ERROR_ALREADY_EXISTS)
   {
      CloseHandle(gMutexHandle);
      gMutexHandle = NULL;
      return false;
   }
   return true;
}

//--------------------------------------
void Platform::AlertOK(const char *windowTitle, const char *message)
{
   ShowCursor(true);
   MessageBoxA(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OK);
}

//--------------------------------------
bool Platform::AlertOKCancel(const char *windowTitle, const char *message)
{
   ShowCursor(true);
   return MessageBoxA(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_OKCANCEL) == IDOK;
}

//--------------------------------------
bool Platform::AlertRetry(const char *windowTitle, const char *message)
{
   ShowCursor(true);
   return (MessageBoxA(NULL, message, windowTitle, MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TASKMODAL | MB_RETRYCANCEL) == IDRETRY);
}

//--------------------------------------
static void InitInput()
{
   dMemset( keyboardState, 0, 256 );
   dMemset( mouseButtonState, 0, sizeof( mouseButtonState ) );
   capsLockDown = (GetKeyState(VK_CAPITAL) & 0x01);
   if (capsLockDown)
   {
      keyboardState[VK_CAPITAL] |= 0x01;
   }
}

//--------------------------------------
static void setMouseClipping()
{
   ClipCursor(NULL);
   if(windowActive)
   {
      ShowCursor(false);

      RECT r;
      GetWindowRect(winState.appWindow, &r);

      if(windowLocked)
      {
         POINT p;
         GetCursorPos(&p);
         lastCursorPos.set(p.x - r.left, p.y - r.top);

         ClipCursor(&r);

         S32 centerX = (r.right + r.left) >> 1;
         S32 centerY = (r.bottom + r.top) >> 1;
         SetCursorPos(centerX, centerY);
      }
      else
         SetCursorPos(lastCursorPos.x + r.left, lastCursorPos.y + r.top);
   }
   else
      ShowCursor(true);
}

//--------------------------------------
static bool sgTaskbarHidden = false;
static HWND sgTaskbar = NULL;

static void hideTheTaskbar()
{
//    if ( !sgTaskbarHidden )
//    {
//       sgTaskbar = FindWindow( "Shell_TrayWnd", NULL );
//       if ( sgTaskbar )
//       {
//          APPBARDATA abData;
//          dMemset( &abData, 0, sizeof( abData ) );
//          abData.cbSize = sizeof( abData );
//          abData.hWnd = sgTaskbar;
//          SHAppBarMessage( ABM_REMOVE, &abData );
//          //ShowWindow( sgTaskbar, SW_HIDE );
//          sgTaskbarHidden = true;
//       }
//    }
}

static void restoreTheTaskbar()
{
//    if ( sgTaskbarHidden )
//    {
//       APPBARDATA abData;
//       dMemset( &abData, 0, sizeof( abData ) );
//       abData.cbSize = sizeof( abData );
//       abData.hWnd = sgTaskbar;
//       SHAppBarMessage( ABM_ACTIVATE, &abData );
//       //ShowWindow( sgTaskbar, SW_SHOW );
//       sgTaskbarHidden = false;
//    }
}

//--------------------------------------
void Platform::enableKeyboardTranslation(void)
{
}

//--------------------------------------
void Platform::disableKeyboardTranslation(void)
{
}

//--------------------------------------
void Platform::setWindowLocked(bool locked)
{
   windowLocked = locked;
   setMouseClipping();
}

//--------------------------------------
void Platform::minimizeWindow()
{
   ShowWindow(winState.appWindow, SW_MINIMIZE);
   restoreTheTaskbar();
}

//--------------------------------------
static void processKeyMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
   S32 repeatCount = lParam & 0xffff;
   S32 scanCode  = (lParam >> 16) & 0xff;
   S32 nVirtkey  =  dIsStandardVK(wParam) ? TranslateOSKeyCode(wParam) : DIK_to_Key(scanCode);
   S32 keyCode;
   if ( wParam == VK_PROCESSKEY && sgDoubleByteEnabled )
      keyCode = MapVirtualKey( scanCode, 1 );   // This is the REAL virtual key...
   else
      keyCode = wParam;

   bool extended = (lParam >> 24) & 1;
   bool repeat   = (lParam >> 30) & 1;
   bool make     = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);

   S32 newVirtKey = nVirtkey;
   switch(nVirtkey)
   {
      case KEY_ALT:
         newVirtKey = extended ? KEY_RALT : KEY_LALT;
         break;
      case KEY_CONTROL:
         newVirtKey = extended ? KEY_RCONTROL : KEY_LCONTROL;
         break;
      case KEY_SHIFT:
         newVirtKey = ( scanCode == 54 ) ? KEY_RSHIFT : KEY_LSHIFT;
         break;
      case KEY_RETURN:
         if ( extended )
            newVirtKey = KEY_NUMPADENTER;
         break;
   }

   S32 modKey = modifierKeys;

   if(make)
   {
      switch (newVirtKey)
      {
         case KEY_LSHIFT:     modifierKeys |= SI_LSHIFT; modKey = 0; break;
         case KEY_RSHIFT:     modifierKeys |= SI_RSHIFT; modKey = 0; break;
         case KEY_LCONTROL:   modifierKeys |= SI_LCTRL; modKey = 0; break;
         case KEY_RCONTROL:   modifierKeys |= SI_RCTRL; modKey = 0; break;
         case KEY_LALT:       modifierKeys |= SI_LALT; modKey = 0; break;
         case KEY_RALT:       modifierKeys |= SI_RALT; modKey = 0; break;
      }
      if(nVirtkey == KEY_CAPSLOCK)
      {
         capsLockDown = !capsLockDown;
         if(capsLockDown)
            keyboardState[keyCode] |= 0x01;
         else
            keyboardState[keyCode] &= 0xFE;
      }
      keyboardState[keyCode] |= 0x80;
   }
   else
   {
      switch (newVirtKey)
      {
         case KEY_LSHIFT:     modifierKeys &= ~SI_LSHIFT; modKey = 0; break;
         case KEY_RSHIFT:     modifierKeys &= ~SI_RSHIFT; modKey = 0; break;
         case KEY_LCONTROL:   modifierKeys &= ~SI_LCTRL; modKey = 0; break;
         case KEY_RCONTROL:   modifierKeys &= ~SI_RCTRL; modKey = 0; break;
         case KEY_LALT:       modifierKeys &= ~SI_LALT; modKey = 0; break;
         case KEY_RALT:       modifierKeys &= ~SI_RALT; modKey = 0; break;
      }
      keyboardState[keyCode] &= 0x7f;
   }

   U16  ascii[3];
   WORD asciiCode = 0;
   dMemset( &ascii, 0, sizeof( ascii ) );

   S32 res = ToAscii( keyCode, scanCode, keyboardState, ascii, 0 );
   if (res == 2)
   {
      asciiCode = ascii[1];
   }
   else if ((res == 1) || (res < 0))
   {
      asciiCode = ascii[0];
   }

   InputEvent event;

   event.deviceInst = 0;
   event.deviceType = KeyboardDeviceType;
   event.objType = SI_KEY;
   event.objInst = newVirtKey;
   event.action = make ? (repeat ? SI_REPEAT : SI_MAKE ) : SI_BREAK;
   event.modifier = modKey;
   event.ascii = asciiCode;
   event.fValue = make ? 1.0 : 0.0;

#ifdef LOG_INPUT
   char keyName[25];
   GetKeyNameText( lParam, keyName, 24 );
   if ( event.action == SI_MAKE )
      Input::log( "EVENT (Win): %s key pressed (Repeat count: %d). MODS:%c%c%c\n", keyName, repeatCount,
            ( modKey & SI_SHIFT ? 'S' : '.' ),
            ( modKey & SI_CTRL ? 'C' : '.' ),
            ( modKey & SI_ALT ? 'A' : '.' ) );
   else
      Input::log( "EVENT (Win): %s key released.\n", keyName );
#endif

   Game->postEvent(event);
}

// static void processCharMessage( WPARAM wParam, LPARAM lParam )
// {
//    TCHAR charCode = wParam;
//    if ( charCode )
//    {
//       S32 repeatCount = lParam & 0xFFFF;
//       InputEvent event;
//       event.deviceInst = 0;
//       event.deviceType = KeyboardDeviceType;
//       event.action = SI_CHAR;
//       event.modifier = modifierKeys;
//       event.ascii = charCode;
//
//       for ( U32 i = 0; i < repeatCount; i++ )
//       {
//          Game->postEvent( event );
//       }
//    }
// }

static S32 mouseX = 0xFFFFFFFF;
static S32 mouseY = 0xFFFFFFFF;

//--------------------------------------
static void CheckCursorPos()
{
   if(windowLocked && windowActive)
   {
      POINT mousePos;
      GetCursorPos(&mousePos);
      RECT r;

      GetWindowRect(winState.appWindow, &r);

      S32 centerX = (r.right + r.left) >> 1;
      S32 centerY = (r.bottom + r.top) >> 1;

      if(mousePos.x != centerX)
      {
         InputEvent event;

         event.deviceInst = 0;
         event.deviceType = MouseDeviceType;
         event.objType = SI_XAXIS;
         event.objInst = 0;
         event.action = SI_MOVE;
         event.modifier = modifierKeys;
         event.ascii = 0;
         event.fValue = (mousePos.x - centerX);
         Game->postEvent(event);
      }
      if(mousePos.y != centerY)
      {
         InputEvent event;

         event.deviceInst = 0;
         event.deviceType = MouseDeviceType;
         event.objType = SI_YAXIS;
         event.objInst = 0;
         event.action = SI_MOVE;
         event.modifier = modifierKeys;
         event.ascii = 0;
         event.fValue = (mousePos.y - centerY);
         Game->postEvent(event);
      }
      SetCursorPos(centerX, centerY);
   }
}

//--------------------------------------
static void mouseButtonEvent(S32 action, S32 objInst)
{
   CheckCursorPos();
   if(!windowLocked)
   {
      if(action == SI_MAKE)
         SetCapture(winState.appWindow);
      else
         ReleaseCapture();
   }

   U32 buttonId = objInst - KEY_BUTTON0;
   if ( buttonId < 3 )
      mouseButtonState[buttonId] = ( action == SI_MAKE ) ? true : false;

   InputEvent event;

   event.deviceInst = 0;
   event.deviceType = MouseDeviceType;
   event.objType = SI_BUTTON;
   event.objInst = objInst;
   event.action = action;
   event.modifier = modifierKeys;
   event.ascii = 0;
   event.fValue = action == SI_MAKE ? 1.0 : 0.0;

#ifdef LOG_INPUT
   if ( action == SI_MAKE )
      Input::log( "EVENT (Win): mouse button%d pressed. MODS:%c%c%c\n", buttonId, ( modifierKeys & SI_SHIFT ? 'S' : '.' ), ( modifierKeys & SI_CTRL ? 'C' : '.' ), ( modifierKeys & SI_ALT ? 'A' : '.' ) );
   else
      Input::log( "EVENT (Win): mouse button%d released.\n", buttonId );
#endif
   Game->postEvent(event);
}

//--------------------------------------
static void mouseWheelEvent( S32 delta )
{
   static S32 _delta = 0;

   _delta += delta;
   if ( abs( delta ) >= WHEEL_DELTA )
   {
      _delta = 0;
      InputEvent event;

      event.deviceInst = 0;
      event.deviceType = MouseDeviceType;
      event.objType = SI_ZAXIS;
      event.objInst = 0;
      event.action = SI_MOVE;
      event.modifier = modifierKeys;
      event.ascii = 0;
      event.fValue = delta;

#ifdef LOG_INPUT
      Input::log( "EVENT (Win): mouse wheel moved. delta = %d\n", delta );
#endif

      Game->postEvent( event );
   }
}


struct WinMessage
{
   UINT message;
   WPARAM wParam;
   LPARAM lParam;

   WinMessage(UINT m, WPARAM w, LPARAM l) : message(m), wParam(w), lParam(l) {}
};

Vector<WinMessage> sgWinMessages;


#ifndef PBT_APMQUERYSUSPEND
#define PBT_APMQUERYSUSPEND             0x0000
#endif

//--------------------------------------
static LRESULT PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch ( message )
   {
      case WM_POWERBROADCAST:
      {
         if(wParam == PBT_APMQUERYSUSPEND)
            return BROADCAST_QUERY_DENY;
         return true;
      }
      case WM_ACTIVATEAPP:
         if ((bool) wParam)
         {
            Video::reactivate();
            ShowCursor(false);
            if ( Video::isFullScreen() )
               hideTheTaskbar();
            // HACK:  Windows 98 (after switching from fullscreen to windowed mode)
            SetForegroundWindow( winState.appWindow );
         }
         else
         {
            Video::deactivate();
            restoreTheTaskbar();
         }
         break;
      case WM_SYSCOMMAND:
         switch(wParam)
         {
            case SC_SCREENSAVE:
            case SC_MONITORPOWER:
               if(GetForegroundWindow() == winState.appWindow)
               {
                  return 0;
               }
               break;
         }
         break;
      case WM_ACTIVATE:
         windowActive = LOWORD(wParam) != WA_INACTIVE;
         if ( windowActive )
         {
            Game->refreshWindow();
            Input::activate();
         }
         else
         {
            DInputManager* mgr = dynamic_cast<DInputManager*>( Input::getManager() );
            if ( !mgr || !mgr->isMouseActive() )
            {
               // Deactivate all the mouse triggers:
               for ( U32 i = 0; i < 3; i++ )
               {
                  if ( mouseButtonState[i] )
                     mouseButtonEvent( SI_BREAK, KEY_BUTTON0 + i );
               }
            }
            Input::deactivate();
         }
         setMouseClipping();
         break;

      case WM_MOVE:
         Game->refreshWindow();
         break;

      case MM_MCINOTIFY:
         handleRedBookCallback(wParam, lParam);
         break;

      //case WM_DESTROY:
      case WM_CLOSE:
         PostQuitMessage(0);
         break;
      default:
      {
         if (sgQueueEvents)
         {
         	WinMessage msg(message,wParam,lParam);

         	sgWinMessages.push_front(msg);
			}
      }
   }

   return DefWindowProcA(hWnd, message, wParam, lParam);
}

//--------------------------------------
static void OurDispatchMessages()
{
   WinMessage msg(0,0,0);
   UINT message;
   WPARAM wParam;
   LPARAM lParam;

   DInputManager* mgr = dynamic_cast<DInputManager*>( Input::getManager() );

   while (sgWinMessages.size())
   {
      msg = sgWinMessages.front();
      sgWinMessages.pop_front();
      message = msg.message;
      wParam = msg.wParam;
      lParam = msg.lParam;

      if ( !mgr || !mgr->isMouseActive() )
      {
         switch ( message )
         {
            case WM_MOUSEMOVE:
               if ( !windowLocked )
               {
                  MouseMoveEvent event;
						S16 xPos = LOWORD(lParam);
						S16 yPos = HIWORD(lParam);

						event.xPos = xPos;  // horizontal position of cursor
                  event.yPos = yPos;  // vertical position of cursor
                  event.modifier = modifierKeys;

#ifdef LOG_INPUT
#ifdef LOG_MOUSEMOVE
                  Input::log( "EVENT (Win): mouse move to (%d, %d).\n", event.xPos, event.yPos );
#endif
#endif
                  Game->postEvent(event);
               }
               break;
            case WM_LBUTTONDOWN:
               mouseButtonEvent(SI_MAKE, KEY_BUTTON0);
               break;
            case WM_MBUTTONDOWN:
               mouseButtonEvent(SI_MAKE, KEY_BUTTON2);
               break;
            case WM_RBUTTONDOWN:
               mouseButtonEvent(SI_MAKE, KEY_BUTTON1);
               break;
            case WM_LBUTTONUP:
               mouseButtonEvent(SI_BREAK, KEY_BUTTON0);
               break;
            case WM_MBUTTONUP:
               mouseButtonEvent(SI_BREAK, KEY_BUTTON2);
               break;
            case WM_RBUTTONUP:
               mouseButtonEvent(SI_BREAK, KEY_BUTTON1);
               break;
            case WM_MOUSEWHEEL:
               mouseWheelEvent( (S16) HIWORD( wParam ) );
               break;

//             case WM_CHAR:
//                processCharMessage( wParam, lParam );
//                break;
         }
      }

      if ( !mgr || !mgr->isKeyboardActive() )
      {
         switch ( message )
         {
            case WM_KEYUP:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
               processKeyMessage(message, wParam, lParam);
               break;
         }
      }
   }
}

//--------------------------------------
static bool ProcessMessages()
{
   MSG msg;

   while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
   {
      if(msg.message == WM_QUIT)
         return false;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
      OurDispatchMessages();
   }

   return true;
}

//--------------------------------------
void Platform::process()
{
   DInputManager* mgr = dynamic_cast<DInputManager*>( Input::getManager() );
   if ( !mgr || !mgr->isMouseActive() )
      CheckCursorPos();
   WindowsConsole->process();

   if(!ProcessMessages())
   {
      // generate a quit event
      Event quitEvent;
      quitEvent.type = QuitEventType;

      Game->postEvent(quitEvent);
   }
   // if there's no window, we sleep 1, otherwise we sleep 0
   if(!Game->isJournalReading())
      Sleep(gWindowCreated ? 0 : 1); // give others some process time if necessary...
   //HWND window = GetForegroundWindow();
   //if (window && gWindowCreated)
   //{
   //   // check to see if we are in the foreground or not
   //   // if not Sleep for 100ms or until a Win32 message/input is recieved
   //   DWORD foregroundProcessId;
   //   GetWindowThreadProcessId(window, &foregroundProcessId);
   //   if (foregroundProcessId != winState.processId)
   //      MsgWaitForMultipleObjects(0, NULL, false, 100, QS_ALLINPUT);
   //}

   Input::process();
}

extern U32 calculateCRC(void * buffer, S32 len, U32 crcVal );

#if defined(TORQUE_DEBUG) || defined(INTERNAL_RELEASE)
static U32 stubCRC = 0;
#else
static U32 stubCRC = 0xEA63F56C;
#endif

//--------------------------------------
static void InitWindowClass()
{
   WNDCLASSA wc;
   dMemset(&wc, 0, sizeof(wc));

   wc.style         = CS_OWNDC;
   wc.lpfnWndProc   = WindowProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = winState.appInstance;
   wc.hIcon         = LoadIcon(winState.appInstance, MAKEINTRESOURCE(IDI_ICON2));
   wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
   wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
   wc.lpszMenuName  = 0;
   wc.lpszClassName = windowClassName;
   RegisterClassA( &wc );

   // Curtain window class:
   wc.lpfnWndProc   = DefWindowProcA;
   wc.hCursor       = NULL;
   wc.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
   wc.lpszClassName = "Curtain";
   RegisterClassA( &wc );
}

//--------------------------------------
static void GetDesktopState()
{
   HWND hDesktop = GetDesktopWindow();
   HDC hDeskDC = GetDC( hDesktop );
   winState.desktopBitsPixel = GetDeviceCaps( hDeskDC, BITSPIXEL );
   winState.desktopWidth = GetDeviceCaps( hDeskDC, HORZRES );
   winState.desktopHeight = GetDeviceCaps( hDeskDC, VERTRES );
   ReleaseDC( hDesktop, hDeskDC );
}

//--------------------------------------
HWND CreateOpenGLWindow( U32 width, U32 height, bool fullScreen )
{
   S32 windowStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
   S32 exWindowStyle = 0;

   if ( fullScreen )
      windowStyle |= ( WS_POPUP | WS_MAXIMIZE );
   else
      windowStyle |= ( WS_OVERLAPPED | WS_CAPTION );

   return CreateWindowExA(
      exWindowStyle,
      windowClassName,
      windowName,
      windowStyle,
      0, 0, width, height,
      NULL, NULL,
      winState.appInstance,
      NULL);
}

//--------------------------------------
HWND CreateCurtain( U32 width, U32 height )
{
   return CreateWindowA(
      "Curtain",
      "",
      ( WS_POPUP | WS_MAXIMIZE | WS_VISIBLE ),
      0, 0,
      width, height,
      NULL, NULL,
      winState.appInstance,
      NULL );
}

//--------------------------------------
void CreatePixelFormat( PIXELFORMATDESCRIPTOR *pPFD, S32 colorBits, S32 depthBits, S32 stencilBits, bool stereo )
{
   PIXELFORMATDESCRIPTOR src =
   {
      sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd
      1,                      // version number
      PFD_DRAW_TO_WINDOW |    // support window
      PFD_SUPPORT_OPENGL |    // support OpenGL
      PFD_DOUBLEBUFFER,       // double buffered
      PFD_TYPE_RGBA,          // RGBA type
      colorBits,              // color depth
      0, 0, 0, 0, 0, 0,       // color bits ignored
      0,                      // no alpha buffer
      0,                      // shift bit ignored
      0,                      // no accumulation buffer
      0, 0, 0, 0,             // accum bits ignored
      depthBits,              // z-buffer
      stencilBits,            // stencil buffer
      0,                      // no auxiliary buffer
      PFD_MAIN_PLANE,         // main layer
      0,                      // reserved
      0, 0, 0                 // layer masks ignored
    };

   if ( stereo )
   {
      //ri.Printf( PRINT_ALL, "...attempting to use stereo\n" );
      src.dwFlags |= PFD_STEREO;
      //glConfig.stereoEnabled = true;
   }
   else
   {
      //glConfig.stereoEnabled = qfalse;
   }
   *pPFD = src;
}

//--------------------------------------
enum WinConstants { MAX_PFDS = 256 };

#ifndef PFD_GENERIC_ACCELERATED
#define PFD_GENERIC_ACCELERATED     0x00001000
#endif

S32 ChooseBestPixelFormat(HDC hDC, PIXELFORMATDESCRIPTOR *pPFD)
{
   PIXELFORMATDESCRIPTOR pfds[MAX_PFDS+1];
   S32 i;
   S32 bestMatch = 0;

   S32 maxPFD = qwglDescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), &pfds[0]);
   if(maxPFD > MAX_PFDS)
      maxPFD = MAX_PFDS;

   bool accelerated = false;

   for(i = 1; i <= maxPFD; i++)
   {
      qwglDescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfds[i]);

      // make sure this has hardware acceleration:
      if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 )
         continue;

      // verify pixel type
      if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
         continue;

      // verify proper flags
      if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags )
         continue;

      accelerated = !(pfds[i].dwFlags & PFD_GENERIC_FORMAT);

      //
      // selection criteria (in order of priority):
      //
      //  PFD_STEREO
      //  colorBits
      //  depthBits
      //  stencilBits
      //
      if ( bestMatch )
      {
         // check stereo
         if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) )
         {
            bestMatch = i;
            continue;
         }

         if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) )
         {
            bestMatch = i;
            continue;
         }

         // check color
         if ( pfds[bestMatch].cColorBits != pPFD->cColorBits )
         {
            // prefer perfect match
            if ( pfds[i].cColorBits == pPFD->cColorBits )
            {
               bestMatch = i;
               continue;
            }
            // otherwise if this PFD has more bits than our best, use it
            else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits )
            {
               bestMatch = i;
               continue;
            }
         }

         // check depth
         if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits )
         {
            // prefer perfect match
            if ( pfds[i].cDepthBits == pPFD->cDepthBits )
            {
               bestMatch = i;
               continue;
            }
            // otherwise if this PFD has more bits than our best, use it
            else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits )
            {
               bestMatch = i;
               continue;
            }
         }

         // check stencil
         if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits )
         {
            // prefer perfect match
            if ( pfds[i].cStencilBits == pPFD->cStencilBits )
            {
               bestMatch = i;
               continue;
            }
            // otherwise if this PFD has more bits than our best, use it
            else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) &&
                ( pPFD->cStencilBits > 0 ) )
            {
               bestMatch = i;
               continue;
            }
         }
      }
      else
      {
         bestMatch = i;
      }
   }

   if ( !bestMatch )
      return 0;

   else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
   {
      // MCD
   }
   else
   {
      // ICD
   }

   *pPFD = pfds[bestMatch];

   return bestMatch;
}

//--------------------------------------
//
// This function exists so DirectInput can communicate with the Windows mouse
// in windowed mode.
//
//--------------------------------------
void setModifierKeys( S32 modKeys )
{
   modifierKeys = modKeys;
}

//--------------------------------------
const Point2I &Platform::getWindowSize()
{
   return windowSize;
}

//--------------------------------------
void Platform::setWindowSize( U32 newWidth, U32 newHeight )
{
   windowSize.set( newWidth, newHeight );
}

//--------------------------------------
static void InitWindow(const Point2I &initialSize)
{
   windowSize = initialSize;

   // The window is created when the display device is activated. BH

   winState.processId = GetCurrentProcessId();
}

//--------------------------------------
static void InitOpenGL()
{
   // The OpenGL initialization stuff has been mostly moved to the display
   // devices' activate functions. BH

   DisplayDevice::init();

   // Get the video settings from the prefs:
   const char* resString = Con::getVariable( "$pref::Video::resolution" );
   char* tempBuf = new char[dStrlen( resString ) + 1];
   dStrcpy( tempBuf, resString );
   char* temp = dStrtok( tempBuf, " x\0" );
   U32 width = ( temp ? dAtoi( temp ) : 800 );
   temp = dStrtok( NULL, " x\0" );
   U32 height = ( temp ? dAtoi( temp ) : 600 );
   temp = dStrtok( NULL, "\0" );
   U32 bpp = ( temp ? dAtoi( temp ) : 16 );
   delete [] tempBuf;

   bool fullScreen = Con::getBoolVariable( "$pref::Video::fullScreen" );

   // If no device is specified, see which ones we have...
   if ( !Video::setDevice( Con::getVariable( "$pref::Video::displayDevice" ), width, height, bpp, fullScreen ) )
   {
      // First, try the default OpenGL device:
      if ( !Video::setDevice( "OpenGL", width, height, bpp, fullScreen ) )
      {
         // Next, try the D3D device:
         if ( !Video::setDevice( "D3D", width, height, bpp, fullScreen ) )
         {
            // Finally, try the Voodoo2 device:
            if ( !Video::setDevice( "Voodoo2", width, height, bpp, fullScreen ) )
            {
              	AssertFatal( false, "Could not find a compatible display device!" );
              	return;
				}
         }
      }
   }
}

//--------------------------------------
ConsoleFunction( getDesktopResolution, const char*, 1, 1, "getDesktopResolution()" )
{
   argc; argv;
   char buffer[256];
   dSprintf( buffer, sizeof( buffer ), "%d %d %d", winState.desktopWidth, winState.desktopHeight, winState.desktopBitsPixel );
   char* returnString = Con::getReturnBuffer( dStrlen( buffer ) + 1 );
   dStrcpy( returnString, buffer );
   return( returnString );
}

//--------------------------------------
void Platform::init()
{
   // Set the platform variable for the scripts
   Con::setVariable( "$platform", "windows" );

   WinConsole::create();
   if ( !WinConsole::isEnabled() )
      Input::init();
   InitInput();   // in case DirectInput falls through
   InitWindowClass();
   GetDesktopState();
   installRedBookDevices();

   sgDoubleByteEnabled = GetSystemMetrics( SM_DBCSENABLED );
	sgQueueEvents = true;
}

//--------------------------------------
void Platform::shutdown()
{
	sgQueueEvents = false;

   if(gMutexHandle)
      CloseHandle(gMutexHandle);
   setWindowLocked( false );
   Video::destroy();
   Input::destroy();
   WinConsole::destroy();
}


class WinTimer
{
   private:
      U32 mTickCountCurrent;
      U32 mTickCountNext;
      S64 mPerfCountCurrent;
      S64 mPerfCountNext;
      S64 mFrequency;
      F64 mPerfCountRemainderCurrent;
      F64 mPerfCountRemainderNext;
      bool mUsingPerfCounter;
   public:
      WinTimer()
      {
         mPerfCountRemainderCurrent = 0.0f;
         mUsingPerfCounter = QueryPerformanceFrequency((LARGE_INTEGER *) &mFrequency);
         if(mUsingPerfCounter)
            mUsingPerfCounter = QueryPerformanceCounter((LARGE_INTEGER *) &mPerfCountCurrent);
         if(!mUsingPerfCounter)
            mTickCountCurrent = GetTickCount();
      }
      U32 getElapsedMS()
      {
         if(mUsingPerfCounter)
         {
            QueryPerformanceCounter( (LARGE_INTEGER *) &mPerfCountNext);
            F64 elapsedF64 = (1000.0 * F64(mPerfCountNext - mPerfCountCurrent) / F64(mFrequency));
            elapsedF64 += mPerfCountRemainderCurrent;
            U32 elapsed = mFloor(elapsedF64);
            mPerfCountRemainderNext = elapsedF64 - F64(elapsed);
            return elapsed;
         }
         else
         {
            mTickCountNext = GetTickCount();
            return mTickCountNext - mTickCountCurrent;
         }
      }
      void advance()
      {
         mTickCountCurrent = mTickCountNext;
         mPerfCountCurrent = mPerfCountNext;
         mPerfCountRemainderCurrent = mPerfCountRemainderNext;
      }
};

static WinTimer gTimer;

extern bool LinkConsoleFunctions;

//--------------------------------------
static S32 run(S32 argc, const char **argv)
{

   // Console hack to ensure consolefunctions get linked in
   LinkConsoleFunctions=true;

   createFontInit();
   windowSize.set(0,0);

   S32 ret = Game->main(argc, argv);
   createFontShutdown();
   return ret;
}

//--------------------------------------
void Platform::initWindow(const Point2I &initialSize, const char *name)
{
   Con::printf( "Video Init:" );
   Video::init();
   if ( Video::installDevice( OpenGLDevice::create() ) )
      Con::printf( "   Accelerated OpenGL display device detected." );
   else
      Con::printf( "   Accelerated OpenGL display device not detected." );

   if ( Video::installDevice( D3DDevice::create() ) )
      Con::printf( "   Accelerated D3D device detected." );
   else
      Con::printf( "   Accelerated D3D device not detected." );

   if ( Video::installDevice( Voodoo2Device::create() ) )
      Con::printf( "   Voodoo 2 display device detected." );
   else
      Con::printf( "   Voodoo 2 display device not detected." );
   Con::printf( "" );

   gWindowCreated = true;
   dStrcpy(windowName, name);
   InitWindow(initialSize);
   InitOpenGL();
}

//--------------------------------------
S32 main(S32 argc, const char **argv)
{
   winState.appInstance = GetModuleHandleA(argv[0]);
   return run(argc, argv);
}

//--------------------------------------
S32 PASCAL WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, S32)
{
   Vector<char *> argv;
   char moduleName[256];
   GetModuleFileNameA(NULL, moduleName, sizeof(moduleName));
   argv.push_back(moduleName);

   for (const char* word,*ptr = lpszCmdLine; *ptr; )  {
      // Eat white space
      for (; dIsspace(*ptr) && *ptr; ptr++)
         ;
      // Pick out the next word
      for (word = ptr; !dIsspace(*ptr) && *ptr; ptr++)
         ;
      // Add the word to the argument list.
      if (*word) {
         int len = ptr - word;
         char *arg = (char *) dMalloc(len + 1);
         dStrncpy(arg, word, len);
         arg[len] = 0;
         argv.push_back(arg);
      }
   }

   winState.appInstance = hInstance;

   S32 retVal = run(argv.size(), (const char **) argv.address());

   for(U32 j = 1; j < argv.size(); j++)
      dFree(argv[j]);

   return retVal;
}

extern U32 gTickInterval;

//--------------------------------------
void TimeManager::process()
{
   TimeEvent event;
   event.elapsedTime = gTimer.getElapsedMS();

   if(event.elapsedTime > gTickInterval)
   {
      gTimer.advance();
      Game->postEvent(event);
   }
}

/*
GLimp_Init
   GLW_LoadOpenGL
      QGL_Init(driver);
      GLW_StartDriverAndSetMode
         GLW_SetMode
            ChangeDisplaySettings
            GLW_CreateWindow
               GLW_InitDriver
                  GLW_CreatePFD
                  GLW_MakeContext
                     GLW_ChoosePFD
                     DescribePixelFormat
                     SetPixelFormat

   GLW_InitExtensions
   WG_CheckHardwareGamma
*/

F32 Platform::getRandom()
{
   return sgPlatRandom.randF();
}

////--------------------------------------
/// Spawn the default Operating System web browser with a URL
/// @param webAddress URL to pass to browser
/// @return true if browser successfully spawned
bool Platform::openWebBrowser( const char* webAddress )
{
   static bool sHaveKey = false;
   static char sWebKey[512];

   if ( !sHaveKey )
   {
      HKEY regKey;
      DWORD size = sizeof( sWebKey );

      if ( RegOpenKeyExA( HKEY_CLASSES_ROOT, "\\http\\shell\\open\\command", 0, KEY_QUERY_VALUE, &regKey ) != ERROR_SUCCESS )
      {
         Con::errorf( ConsoleLogEntry::General, "Failed to open the HKCR\\http registry key!!!");
         return( false );
      }

      size = sizeof( sWebKey );
      if ( RegQueryValueExA( regKey, "", NULL, NULL, (unsigned char*)sWebKey, &size ) != ERROR_SUCCESS )
      {
         Con::errorf( ConsoleLogEntry::General, "Failed to query the open command registry key!!!" );
         return( false );
      }

      RegCloseKey( regKey );
      sHaveKey = true;

      char *p = dStrstr(sWebKey, "\"%1\"");
      if (p) *p = 0;
   }

   STARTUPINFOA si;
   dMemset( &si, 0, sizeof( si ) );
   si.cb = sizeof( si );

    char buf[1024];
    dSprintf( buf, sizeof( buf ), "%s %s", (const char*) sWebKey, webAddress );

   //Con::errorf( ConsoleLogEntry::General, "** Web browser command = %s **", buf );

   PROCESS_INFORMATION pi;
   dMemset( &pi, 0, sizeof( pi ) );
   CreateProcessA( NULL,
         buf,
         NULL,
         NULL,
         false,
         CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
         NULL,
         NULL,
         &si,
         &pi );

   return( true );
}

//--------------------------------------
// Login password routines:
//--------------------------------------
static const char* TorqueRegKey = "SOFTWARE\\GarageGames\\Torque";

const char* Platform::getLoginPassword()
{
   HKEY regKey;
   char* returnString = NULL;
   if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, TorqueRegKey, 0, KEY_QUERY_VALUE, &regKey ) == ERROR_SUCCESS )
   {
      U8 buf[32];
      DWORD size = sizeof( buf );
      if ( RegQueryValueExA( regKey, "LoginPassword", NULL, NULL, buf, &size ) == ERROR_SUCCESS )
      {
         returnString = Con::getReturnBuffer( size + 1 );
         dStrcpy( returnString, (const char*) buf );
      }

      RegCloseKey( regKey );
   }

   if ( returnString )
      return( returnString );
   else
      return( "" );
}

//--------------------------------------
bool Platform::setLoginPassword( const char* password )
{
   HKEY regKey;
   if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, TorqueRegKey, 0, KEY_WRITE, &regKey ) == ERROR_SUCCESS )
   {
      if ( RegSetValueExA( regKey, "LoginPassword", 0, REG_SZ, (const U8*) password, dStrlen( password ) + 1 ) != ERROR_SUCCESS )
         Con::errorf( ConsoleLogEntry::General, "setLoginPassword - Failed to set the subkey value!" );

      RegCloseKey( regKey );
      return( true );
   }
   else
      Con::errorf( ConsoleLogEntry::General, "setLoginPassword - Failed to open the Torque registry key!" );

   return( false );
}

//--------------------------------------
// Silly Korean registry key checker:
//--------------------------------------
ConsoleFunction( isKoreanBuild, bool, 1, 1, "isKoreanBuild()" )
{
   argc; argv;
   HKEY regKey;
   bool result = false;
   if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE, TorqueRegKey, 0, KEY_QUERY_VALUE, &regKey ) == ERROR_SUCCESS )
   {
      DWORD val;
      DWORD size = sizeof( val );
      if ( RegQueryValueExA( regKey, "Korean", NULL, NULL, (U8*) &val, &size ) == ERROR_SUCCESS )
         result = ( val > 0 );

      RegCloseKey( regKey );
   }

   return( result );
}
