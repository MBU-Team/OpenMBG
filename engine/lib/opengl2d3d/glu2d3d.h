//-----------------------------------------------------------------------------
// Torque Game Engine 
// Quake GL DirectX wrapper
//-----------------------------------------------------------------------------

#ifndef _OPENGL2D3D_H_

#include "opengl2d3d/opengl2d3d.h"
#endif

#define CEILING( A, B )  ( (A) % (B) == 0 ? (A)/(B) : (A)/(B)+1 )

/* To work around optimizer bug in MSVC4.1 */
#if defined(__WIN32__) && !defined(OPENSTEP)
void
dummy(GLuint j, GLuint k)
{
}
#else
#define dummy(J, K)
#endif

/* EXT_bgra */
#define GL_BGR					0x80E0
#define GL_BGRA				0x80E1

/* Errors */
#define GLU_NO_ERROR			0
#define GLU_ERROR				100103
#define GLU_INVALID_ENUM	100900
#define GLU_INVALID_VALUE	100901
#define GLU_OUT_OF_MEMORY	100902

/* GLU 1.1 and later */
#define GLU_VERSION			100800
#define GLU_EXTENSIONS		100801
