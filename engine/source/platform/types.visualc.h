
#ifndef INCLUDED_TYPES_VISUALC_H
#define INCLUDED_TYPES_VISUALC_H


// For more information on VisualC++ predefined macros
// http://support.microsoft.com/default.aspx?scid=kb;EN-US;q65472

//--------------------------------------
// Types
typedef signed _int64   S64;
typedef unsigned _int64 U64;


//--------------------------------------
// Compiler Version
#define TORQUE_COMPILER_VISUALC _MSC_VER

//--------------------------------------
// Identify the compiler string
#if _MSC_VER < 1200
   // No support for old compilers
#  error "VC: Minimum VisualC++ 6.0 or newer required"
#else _MSC_VER >= 1200
#  define TORQUE_COMPILER_STRING "VisualC++"
#endif


//--------------------------------------
// Identify the Operating System
#if defined(_WIN32)
#  define TORQUE_OS_STRING "Win32"
#  define TORQUE_OS_WIN32
#  include "platform/types.win32.h"
#else 
#  error "VC: Unsupported Operating System"
#endif


//--------------------------------------
// Identify the CPU
#if defined( _M_X64 )
#  define TORQUE_CPU_STRING "x64"
#  define TORQUE_CPU_X64
#  define TORQUE_LITTLE_ENDIAN
#elif defined(_M_IX86)
#  define TORQUE_CPU_STRING "x86"
#  define TORQUE_CPU_X86
#  define TORQUE_LITTLE_ENDIAN
#ifndef __clang__ // asm not yet supported with clang
#  define TORQUE_SUPPORTS_NASM
#  define TORQUE_SUPPORTS_VC_INLINE_X86_ASM
#endif
#else
#  error "VC: Unsupported Target CPU"
#endif


#define FN_CDECL __cdecl            ///< Calling convention

// disable warning caused by memory layer
// see msdn.microsoft.com "Compiler Warning (level 1) C4291" for more details
#pragma warning(disable: 4291) 


#endif // INCLUDED_TYPES_VISUALC_H

