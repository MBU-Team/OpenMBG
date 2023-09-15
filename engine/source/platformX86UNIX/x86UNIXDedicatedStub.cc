//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------



// GL stubs
#ifndef NULL
#define NULL 0
#endif

#include "platformX86UNIX/platformGL.h"
#include "platform/platformAL.h"
#include "platform/platformInput.h"

// declare stub functions
#define GL_FUNCTION(fn_return, fn_name, fn_args, fn_value) fn_return stub_##fn_name fn_args{ fn_value }
#include "platformX86UNIX/gl_func.h"
#include "platformX86UNIX/glu_func.h"
#undef GL_FUNCTION

// point gl function pointers at stub functions
#define GL_FUNCTION(fn_return,fn_name,fn_args, fn_value) fn_return (*fn_name)fn_args = stub_##fn_name;
#include "platformX86UNIX/gl_func.h"
#include "platformX86UNIX/glu_func.h"
#undef GL_FUNCTION

GLState gGLState;
bool  gOpenGLDisablePT                   = false;
bool  gOpenGLDisableCVA                  = false;
bool  gOpenGLDisableTEC                  = false;
bool  gOpenGLDisableARBMT                = false;
bool  gOpenGLDisableFC                   = false;
bool  gOpenGLDisableTCompress            = false;
bool  gOpenGLNoEnvColor                  = false;
float gOpenGLGammaCorrection             = 0.5;
bool  gOpenGLNoDrawArraysAlpha           = false;

// AL stubs
//#include <al/altypes.h>
///#include <al/alctypes.h>
//#define INITGUID
//#include <al/eaxtypes.h>

// Define the OpenAL and Extension Stub functions
#define AL_FUNCTION(fn_return, fn_name, fn_args, fn_value) fn_return stub_##fn_name fn_args{ fn_value }
#include <al/al_func.h>
#include <al/alc_func.h>
#include <al/eax_func.h>
#undef AL_FUNCTION


// Declare the OpenAL and Extension Function pointers
// And initialize them to the stub functions
#define AL_FUNCTION(fn_return,fn_name,fn_args, fn_value) fn_return (*fn_name)fn_args = stub_##fn_name;
#include <al/al_func.h>
#include <al/alc_func.h>
#include <al/eax_func.h>
#undef AL_FUNCTION

namespace Audio
{
bool OpenALDLLInit() { return false; }
void OpenALDLLShutdown() {}
}

// Platform Stubs
const char* Platform::getClipboard() { return ""; }
bool Platform::setClipboard(const char *text) { return false; }
GFont *createFont(const char *name, dsize_t size) { return NULL; }
void PlatformBlitInit( void ) {}

// #include <X11/Xlib.h>

// void NotifySelectionEvent(XEvent& event)
// {
// }

// Input stubs
void Input::init() {}
void Input::destroy() {}
bool Input::enable() { return false; }
void Input::disable() {}
void Input::activate() {}
void Input::deactivate() {}
void Input::reactivate() {} 
U16 Input::getAscii( U16 keyCode, KEY_STATE keyState ) { return 0; }
U16 Input::getKeyCode( U16 asciiCode ) { return 0; }
bool Input::isEnabled() { return false; }
bool Input::isActive() { return false; }
void Input::process() {}
InputManager* Input::getManager() { return NULL; }
