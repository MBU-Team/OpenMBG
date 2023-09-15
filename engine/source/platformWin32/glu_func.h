//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (c) 2003 GarageGames.Com
//-----------------------------------------------------------------------------

// $Id: glu_func.h,v 2.1 2004/04/21 04:27:32 timg Exp $

//------------------------------------------------------------------------------
// GLU functions
//------------------------------------------------------------------------------

GL_FUNCTION(int, gluProject, (GLdouble objx, GLdouble objy, GLdouble objz,  const GLdouble modelMatrix[16], const GLdouble projMatrix[16],  const GLint viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz), return 0; )
GL_FUNCTION(int, gluUnProject, (GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16],  const GLint viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz), return 0; )

