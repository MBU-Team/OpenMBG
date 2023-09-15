//-----------------------------------------------------------------------------
// Torque Game Engine
// Quake GL DirectX wrapper
//-----------------------------------------------------------------------------

#include "opengl2d3d/glu2d3d.h"

GLAPI BOOL GLAPIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	hModule;
	lpReserved;
	
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}


extern "C" {

GLAPI const GLubyte* GLAPIENTRY gluErrorString(GLenum errorCode)
{
   /* GLU Errors */
   if (errorCode == GLU_NO_ERROR) {
      return (GLubyte *) "no error";
   }
   else if (errorCode == GLU_INVALID_ENUM) {
      return (GLubyte *) "invalid enum";
   }
   else if (errorCode == GLU_INVALID_VALUE) {
      return (GLubyte *) "invalid value";
   }
   else if (errorCode == GLU_OUT_OF_MEMORY) {
      return (GLubyte *) "out of memory";
   }
   else {
      return NULL;
   }
}

GLAPI const GLubyte* GLAPIENTRY gluGetString(GLenum name)
{
   static char *extensions = "";
   static char *version = "1.0 GarageGames";

   switch (name) {
   case GLU_EXTENSIONS:
      return (GLubyte *) extensions;
   case GLU_VERSION:
      return (GLubyte *) version;
   default:
      return NULL;
   }
}

GLAPI void GLAPIENTRY gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
{
   glOrtho(left, right, bottom, top, -1.0, 1.0);
}

GLAPI void GLAPIENTRY gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan(fovy * M_PI / 360.0);
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
}
   
GLAPI void GLAPIENTRY gluPickMatrix(GLdouble x, GLdouble y, GLdouble width, GLdouble height, const GLint viewport[4])
{
   GLfloat m[16];
   GLfloat sx, sy;
   GLfloat tx, ty;

   sx = (GLfloat) (viewport[2] / width);
   sy = (GLfloat) (viewport[3] / height);
   tx = (GLfloat) ((viewport[2] + 2.0 * (viewport[0] - x)) / width);
   ty = (GLfloat) ((viewport[3] + 2.0 * (viewport[1] - y)) / height);

#define M(row,col)  m[col*4+row]
   M(0, 0) = sx;
   M(0, 1) = 0.0;
   M(0, 2) = 0.0;
   M(0, 3) = tx;
   M(1, 0) = 0.0;
   M(1, 1) = sy;
   M(1, 2) = 0.0;
   M(1, 3) = ty;
   M(2, 0) = 0.0;
   M(2, 1) = 0.0;
   M(2, 2) = 1.0;
   M(2, 3) = 0.0;
   M(3, 0) = 0.0;
   M(3, 1) = 0.0;
   M(3, 2) = 0.0;
   M(3, 3) = 1.0;
#undef M

   glMultMatrixf(m);
}

GLAPI void GLAPIENTRY gluLookAt(GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz)
{
   GLdouble m[16];
   GLdouble x[3], y[3], z[3];
   GLdouble mag;

   /* Make rotation matrix */

   /* Z vector */
   z[0] = eyex - centerx;
   z[1] = eyey - centery;
   z[2] = eyez - centerz;
   mag = sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
   if (mag) {			/* mpichler, 19950515 */
      z[0] /= mag;
      z[1] /= mag;
      z[2] /= mag;
   }

   /* Y vector */
   y[0] = upx;
   y[1] = upy;
   y[2] = upz;

   /* X vector = Y cross Z */
   x[0] = y[1] * z[2] - y[2] * z[1];
   x[1] = -y[0] * z[2] + y[2] * z[0];
   x[2] = y[0] * z[1] - y[1] * z[0];

   /* Recompute Y = Z cross X */
   y[0] = z[1] * x[2] - z[2] * x[1];
   y[1] = -z[0] * x[2] + z[2] * x[0];
   y[2] = z[0] * x[1] - z[1] * x[0];

   /* mpichler, 19950515 */
   /* cross product gives area of parallelogram, which is < 1.0 for
    * non-perpendicular unit-length vectors; so normalize x, y here
    */

   mag = sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
   if (mag) {
      x[0] /= mag;
      x[1] /= mag;
      x[2] /= mag;
   }

   mag = sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
   if (mag) {
      y[0] /= mag;
      y[1] /= mag;
      y[2] /= mag;
   }

#define M(row,col)  m[col*4+row]
   M(0, 0) = x[0];
   M(0, 1) = x[1];
   M(0, 2) = x[2];
   M(0, 3) = 0.0;
   M(1, 0) = y[0];
   M(1, 1) = y[1];
   M(1, 2) = y[2];
   M(1, 3) = 0.0;
   M(2, 0) = z[0];
   M(2, 1) = z[1];
   M(2, 2) = z[2];
   M(2, 3) = 0.0;
   M(3, 0) = 0.0;
   M(3, 1) = 0.0;
   M(3, 2) = 0.0;
   M(3, 3) = 1.0;
#undef M
   glMultMatrixd(m);

   /* Translate Eye to Origin */
   glTranslated(-eyex, -eyey, -eyez);
}

/*
 * Transform a point (column vector) by a 4x4 matrix.  I.e.  out = m * in
 * Input:  m - the 4x4 matrix
 *         in - the 4x1 vector
 * Output:  out - the resulting 4x1 vector.
 */
static void transform_point(GLdouble out[4], const GLdouble m[16], const GLdouble in[4])
{
#define M(row,col)  m[col*4+row]
   out[0] =
      M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
   out[1] =
      M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
   out[2] =
      M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
   out[3] =
      M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

/*
 * Perform a 4x4 matrix multiplication  (product = a x b).
 * Input:  a, b - matrices to multiply
 * Output:  product - product of a and b
 */
static void matmul(GLdouble * product, const GLdouble * a, const GLdouble * b)
{
   /* This matmul was contributed by Thomas Malik */
   GLdouble temp[16];
   GLint i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define T(row,col)  temp[(col<<2)+row]

   /* i-te Zeile */
   for (i = 0; i < 4; i++) {
      T(i, 0) =
	 A(i, 0) * B(0, 0) + A(i, 1) * B(1, 0) + A(i, 2) * B(2, 0) + A(i,
								       3) *
	 B(3, 0);
      T(i, 1) =
	 A(i, 0) * B(0, 1) + A(i, 1) * B(1, 1) + A(i, 2) * B(2, 1) + A(i,
								       3) *
	 B(3, 1);
      T(i, 2) =
	 A(i, 0) * B(0, 2) + A(i, 1) * B(1, 2) + A(i, 2) * B(2, 2) + A(i,
								       3) *
	 B(3, 2);
      T(i, 3) =
	 A(i, 0) * B(0, 3) + A(i, 1) * B(1, 3) + A(i, 2) * B(2, 3) + A(i,
								       3) *
	 B(3, 3);
   }

#undef A
#undef B
#undef T
   memcpy(product, temp, 16 * sizeof(GLdouble));
}

/*
 * Compute inverse of 4x4 transformation matrix.
 * Code contributed by Jacques Leroy jle@star.be
 * Return GL_TRUE for success, GL_FALSE for failure (singular matrix)
 */
static GLboolean invert_matrix(const GLdouble * m, GLdouble * out)
{
/* NB. OpenGL Matrices are COLUMN major. */
#define SWAP_ROWS(a, b) { GLdouble *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

   GLdouble wtmp[4][8];
   GLdouble m0, m1, m2, m3, s;
   GLdouble *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
      r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
      r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
      r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
      r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
      r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
      r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
      r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
      r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
      r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
      r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
      r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (fabs(r3[0]) > fabs(r2[0]))
      SWAP_ROWS(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0]))
      SWAP_ROWS(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0]))
      SWAP_ROWS(r1, r0);
   if (0.0 == r0[0])
      return GL_FALSE;

   /* eliminate first variable     */
   m1 = r1[0] / r0[0];
   m2 = r2[0] / r0[0];
   m3 = r3[0] / r0[0];
   s = r0[1];
   r1[1] -= m1 * s;
   r2[1] -= m2 * s;
   r3[1] -= m3 * s;
   s = r0[2];
   r1[2] -= m1 * s;
   r2[2] -= m2 * s;
   r3[2] -= m3 * s;
   s = r0[3];
   r1[3] -= m1 * s;
   r2[3] -= m2 * s;
   r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) {
      r1[4] -= m1 * s;
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r0[5];
   if (s != 0.0) {
      r1[5] -= m1 * s;
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r0[6];
   if (s != 0.0) {
      r1[6] -= m1 * s;
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r0[7];
   if (s != 0.0) {
      r1[7] -= m1 * s;
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[1]) > fabs(r2[1]))
      SWAP_ROWS(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1]))
      SWAP_ROWS(r2, r1);
   if (0.0 == r1[1])
      return GL_FALSE;

   /* eliminate second variable */
   m2 = r2[1] / r1[1];
   m3 = r3[1] / r1[1];
   r2[2] -= m2 * r1[2];
   r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3];
   r3[3] -= m3 * r1[3];
   s = r1[4];
   if (0.0 != s) {
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r1[5];
   if (0.0 != s) {
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r1[6];
   if (0.0 != s) {
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r1[7];
   if (0.0 != s) {
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[2]) > fabs(r2[2]))
      SWAP_ROWS(r3, r2);
   if (0.0 == r2[2])
      return GL_FALSE;

   /* eliminate third variable */
   m3 = r3[2] / r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
      r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3])
      return GL_FALSE;

   s = 1.0 / r3[3];		/* now back substitute row 3 */
   r3[4] *= s;
   r3[5] *= s;
   r3[6] *= s;
   r3[7] *= s;

   m2 = r2[3];			/* now back substitute row 2 */
   s = 1.0 / r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
      r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
      r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
      r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];			/* now back substitute row 1 */
   s = 1.0 / r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
      r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
      r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];			/* now back substitute row 0 */
   s = 1.0 / r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
      r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out, 0, 0) = r0[4];
   MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
   MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
   MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
   MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
   MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
   MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
   MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
   MAT(out, 3, 3) = r3[7];

   return GL_TRUE;

#undef MAT
#undef SWAP_ROWS
}

/* projection du point (objx,objy,obz) sur l'ecran (winx,winy,winz) */
GLAPI GLint GLAPIENTRY gluProject(GLdouble objx, GLdouble objy, GLdouble objz,
	   const GLdouble model[16], const GLdouble proj[16],
	   const GLint viewport[4],
	   GLdouble * winx, GLdouble * winy, GLdouble * winz)
{
   /* matrice de transformation */
   GLdouble in[4], out[4];

   /* initilise la matrice et le vecteur a transformer */
   in[0] = objx;
   in[1] = objy;
   in[2] = objz;
   in[3] = 1.0;
   transform_point(out, model, in);
   transform_point(in, proj, out);

   /* d'ou le resultat normalise entre -1 et 1 */
   if (in[3] == 0.0)
      return GL_FALSE;

   in[0] /= in[3];
   in[1] /= in[3];
   in[2] /= in[3];

   /* en coordonnees ecran */
   *winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;
   *winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;
   /* entre 0 et 1 suivant z */
   *winz = (1 + in[2]) / 2;
   return GL_TRUE;
}

/* transformation du point ecran (winx,winy,winz) en point objet */
GLAPI GLint GLAPIENTRY gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
	     const GLdouble model[16], const GLdouble proj[16],
	     const GLint viewport[4],
	     GLdouble * objx, GLdouble * objy, GLdouble * objz)
{
   /* matrice de transformation */
   GLdouble m[16], A[16];
   GLdouble in[4], out[4];

   /* transformation coordonnees normalisees entre -1 et 1 */
   in[0] = (winx - viewport[0]) * 2 / viewport[2] - 1.0;
   in[1] = (winy - viewport[1]) * 2 / viewport[3] - 1.0;
   in[2] = 2 * winz - 1.0;
   in[3] = 1.0;

   /* calcul transformation inverse */
   matmul(A, proj, model);
   invert_matrix(A, m);

   /* d'ou les coordonnees objets */
   transform_point(out, m, in);
   if (out[3] == 0.0)
      return GL_FALSE;
   *objx = out[0] / out[3];
   *objy = out[1] / out[3];
   *objz = out[2] / out[3];
   return GL_TRUE;
}

GLAPI GLint GLAPIENTRY gluScaleImage(GLenum format,
	      GLsizei widthin, GLsizei heightin,
	      GLenum typein, const void *datain,
	      GLsizei widthout, GLsizei heightout,
	      GLenum typeout, void *dataout)
{
   GLint components, i, j, k;
   GLfloat *tempin, *tempout;
   GLfloat sx, sy;
   GLint unpackrowlength, unpackalignment, unpackskiprows, unpackskippixels;
   GLint packrowlength, packalignment, packskiprows, packskippixels;
   GLint sizein, sizeout;
   GLint rowstride, rowlen;


   /* Determine number of components per pixel */
   switch (format) {
   case GL_COLOR_INDEX:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
      components = 1;
      break;
   case GL_LUMINANCE_ALPHA:
      components = 2;
      break;
   case GL_RGB:
   case GL_BGR:
      components = 3;
      break;
   case GL_RGBA:
   case GL_BGRA:
#ifdef GL_EXT_abgr
   case GL_ABGR_EXT:
#endif
      components = 4;
      break;
   default:
      return GLU_INVALID_ENUM;
   }

   /* Determine bytes per input datum */
   switch (typein) {
   case GL_UNSIGNED_BYTE:
      sizein = sizeof(GLubyte);
      break;
   case GL_BYTE:
      sizein = sizeof(GLbyte);
      break;
   case GL_UNSIGNED_SHORT:
      sizein = sizeof(GLushort);
      break;
   case GL_SHORT:
      sizein = sizeof(GLshort);
      break;
   case GL_UNSIGNED_INT:
      sizein = sizeof(GLuint);
      break;
   case GL_INT:
      sizein = sizeof(GLint);
      break;
   case GL_FLOAT:
      sizein = sizeof(GLfloat);
      break;
   case GL_BITMAP:
      /* not implemented yet */
   default:
      return GL_INVALID_ENUM;
   }

   /* Determine bytes per output datum */
   switch (typeout) {
   case GL_UNSIGNED_BYTE:
      sizeout = sizeof(GLubyte);
      break;
   case GL_BYTE:
      sizeout = sizeof(GLbyte);
      break;
   case GL_UNSIGNED_SHORT:
      sizeout = sizeof(GLushort);
      break;
   case GL_SHORT:
      sizeout = sizeof(GLshort);
      break;
   case GL_UNSIGNED_INT:
      sizeout = sizeof(GLuint);
      break;
   case GL_INT:
      sizeout = sizeof(GLint);
      break;
   case GL_FLOAT:
      sizeout = sizeof(GLfloat);
      break;
   case GL_BITMAP:
      /* not implemented yet */
   default:
      return GL_INVALID_ENUM;
   }

   /* Get glPixelStore state */
   glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackrowlength);
   glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackalignment);
   glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackskiprows);
   glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackskippixels);
   glGetIntegerv(GL_PACK_ROW_LENGTH, &packrowlength);
   glGetIntegerv(GL_PACK_ALIGNMENT, &packalignment);
   glGetIntegerv(GL_PACK_SKIP_ROWS, &packskiprows);
   glGetIntegerv(GL_PACK_SKIP_PIXELS, &packskippixels);

   /* Allocate storage for intermediate images */
   tempin = (GLfloat *) malloc(widthin * heightin
			       * components * sizeof(GLfloat));
   if (!tempin) {
      return GLU_OUT_OF_MEMORY;
   }
   tempout = (GLfloat *) malloc(widthout * heightout
				* components * sizeof(GLfloat));
   if (!tempout) {
      free(tempin);
      return GLU_OUT_OF_MEMORY;
   }


   /*
    * Unpack the pixel data and convert to floating point
    */

   if (unpackrowlength > 0) {
      rowlen = unpackrowlength;
   }
   else {
      rowlen = widthin;
   }
   if (sizein >= unpackalignment) {
      rowstride = components * rowlen;
   }
   else {
      rowstride = unpackalignment / sizein
	 * CEILING(components * rowlen * sizein, unpackalignment);
   }

   switch (typein) {
   case GL_UNSIGNED_BYTE:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLubyte *ubptr = (GLubyte *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * ubptr++;
	 }
      }
      break;
   case GL_BYTE:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLbyte *bptr = (GLbyte *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * bptr++;
	 }
      }
      break;
   case GL_UNSIGNED_SHORT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLushort *usptr = (GLushort *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * usptr++;
	 }
      }
      break;
   case GL_SHORT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLshort *sptr = (GLshort *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * sptr++;
	 }
      }
      break;
   case GL_UNSIGNED_INT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLuint *uiptr = (GLuint *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * uiptr++;
	 }
      }
      break;
   case GL_INT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLint *iptr = (GLint *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = (GLfloat) * iptr++;
	 }
      }
      break;
   case GL_FLOAT:
      k = 0;
      for (i = 0; i < heightin; i++) {
	 GLfloat *fptr = (GLfloat *) datain
	    + i * rowstride
	    + unpackskiprows * rowstride + unpackskippixels * components;
	 for (j = 0; j < widthin * components; j++) {
	    dummy(j, k);
	    tempin[k++] = *fptr++;
	 }
      }
      break;
   default:
      return GLU_INVALID_ENUM;
   }


   /*
    * Scale the image!
    */

   if (widthout > 1)
      sx = (GLfloat) (widthin - 1) / (GLfloat) (widthout - 1);
   else
      sx = (GLfloat) (widthin - 1);
   if (heightout > 1)
      sy = (GLfloat) (heightin - 1) / (GLfloat) (heightout - 1);
   else
      sy = (GLfloat) (heightin - 1);

/*#define POINT_SAMPLE*/
#ifdef POINT_SAMPLE
   for (i = 0; i < heightout; i++) {
      GLint ii = i * sy;
      for (j = 0; j < widthout; j++) {
	 GLint jj = j * sx;

	 GLfloat *src = tempin + (ii * widthin + jj) * components;
	 GLfloat *dst = tempout + (i * widthout + j) * components;

	 for (k = 0; k < components; k++) {
	    *dst++ = *src++;
	 }
      }
   }
#else
   if (sx < 1.0 && sy < 1.0) {
      /* magnify both width and height:  use weighted sample of 4 pixels */
      GLint i0, i1, j0, j1;
      GLfloat alpha, beta;
      GLfloat *src00, *src01, *src10, *src11;
      GLfloat s1, s2;
      GLfloat *dst;

      for (i = 0; i < heightout; i++) {
	 i0 = (GLint) (i * sy);
	 i1 = i0 + 1;
	 if (i1 >= heightin)
	    i1 = heightin - 1;
/*	 i1 = (i+1) * sy - EPSILON;*/
	 alpha = i * sy - i0;
	 for (j = 0; j < widthout; j++) {
	    j0 = (GLint) (j * sx);
	    j1 = j0 + 1;
	    if (j1 >= widthin)
	       j1 = widthin - 1;
/*	    j1 = (j+1) * sx - EPSILON; */
	    beta = j * sx - j0;

	    /* compute weighted average of pixels in rect (i0,j0)-(i1,j1) */
	    src00 = tempin + (i0 * widthin + j0) * components;
	    src01 = tempin + (i0 * widthin + j1) * components;
	    src10 = tempin + (i1 * widthin + j0) * components;
	    src11 = tempin + (i1 * widthin + j1) * components;

	    dst = tempout + (i * widthout + j) * components;

	    for (k = 0; k < components; k++) {
	       s1 = (GLfloat) (*src00++ * (1.0 - beta) + *src01++ * beta);
	       s2 = (GLfloat) (*src10++ * (1.0 - beta) + *src11++ * beta);
	       *dst++ = (GLfloat) (s1 * (1.0 - alpha) + s2 * alpha);
	    }
	 }
      }
   }
   else {
      /* shrink width and/or height:  use an unweighted box filter */
      GLint i0, i1;
      GLint j0, j1;
      GLint ii, jj;
      GLfloat sum, *dst;

      for (i = 0; i < heightout; i++) {
	 i0 = (GLint) (i * sy);
	 i1 = i0 + 1;
	 if (i1 >= heightin)
	    i1 = heightin - 1;
/*	 i1 = (i+1) * sy - EPSILON; */
	 for (j = 0; j < widthout; j++) {
	    j0 = (GLint) (j * sx);
	    j1 = j0 + 1;
	    if (j1 >= widthin)
	       j1 = widthin - 1;
/*	    j1 = (j+1) * sx - EPSILON; */

	    dst = tempout + (i * widthout + j) * components;

	    /* compute average of pixels in the rectangle (i0,j0)-(i1,j1) */
	    for (k = 0; k < components; k++) {
	       sum = 0.0;
	       for (ii = i0; ii <= i1; ii++) {
		  for (jj = j0; jj <= j1; jj++) {
		     sum += *(tempin + (ii * widthin + jj) * components + k);
		  }
	       }
	       sum /= (j1 - j0 + 1) * (i1 - i0 + 1);
	       *dst++ = sum;
	    }
	 }
      }
   }
#endif


   /*
    * Return output image
    */

   if (packrowlength > 0) {
      rowlen = packrowlength;
   }
   else {
      rowlen = widthout;
   }
   if (sizeout >= packalignment) {
      rowstride = components * rowlen;
   }
   else {
      rowstride = packalignment / sizeout
	 * CEILING(components * rowlen * sizeout, packalignment);
   }

   switch (typeout) {
   case GL_UNSIGNED_BYTE:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLubyte *ubptr = (GLubyte *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *ubptr++ = (GLubyte) tempout[k++];
	 }
      }
      break;
   case GL_BYTE:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLbyte *bptr = (GLbyte *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *bptr++ = (GLbyte) tempout[k++];
	 }
      }
      break;
   case GL_UNSIGNED_SHORT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLushort *usptr = (GLushort *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *usptr++ = (GLushort) tempout[k++];
	 }
      }
      break;
   case GL_SHORT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLshort *sptr = (GLshort *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *sptr++ = (GLshort) tempout[k++];
	 }
      }
      break;
   case GL_UNSIGNED_INT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLuint *uiptr = (GLuint *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *uiptr++ = (GLuint) tempout[k++];
	 }
      }
      break;
   case GL_INT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLint *iptr = (GLint *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *iptr++ = (GLint) tempout[k++];
	 }
      }
      break;
   case GL_FLOAT:
      k = 0;
      for (i = 0; i < heightout; i++) {
	 GLfloat *fptr = (GLfloat *) dataout
	    + i * rowstride
	    + packskiprows * rowstride + packskippixels * components;
	 for (j = 0; j < widthout * components; j++) {
	    dummy(j, k + i);
	    *fptr++ = tempout[k++];
	 }
      }
      break;
   default:
      return GLU_INVALID_ENUM;
   }


   /* free temporary image storage */
   free(tempin);
   free(tempout);

   return 0;
}

/*
 * Return the largest k such that 2^k <= n.
 */
static GLint ilog2(GLint n)
{
   GLint k;

   if (n <= 0)
      return 0;
   for (k = 0; n >>= 1; k++) {}
   return k;
}

/*
 * Find the value nearest to n which is also a power of two.
 */
static GLint round2(GLint n)
{
   GLint m;

   for (m = 1; m < n; m *= 2) {}

   /* m>=n */
   if (m - n <= n - m / 2) {
      return m;
   }
   else {
      return m / 2;
   }
}

/*
 * WARNING: This function isn't finished and has never been tested!!!!
 */
GLAPI GLint GLAPIENTRY gluBuild1DMipmaps(GLenum target, GLint components,
		  GLsizei width, GLenum format, GLenum type, const void *data)
{
	target;

   GLubyte *texture;
   GLint levels, max_levels;
   GLint new_width, max_width;
   GLint i, j, k, l;

   if (width < 1)
      return GLU_INVALID_VALUE;

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_width);
   max_levels = ilog2(max_width) + 1;

   /* Compute how many mipmap images to make */
   levels = ilog2(width) + 1;
   if (levels > max_levels) {
      levels = max_levels;
   }

   new_width = 1 << (levels - 1);

   texture = (GLubyte *) malloc(new_width * components);
   if (!texture) {
      return GLU_OUT_OF_MEMORY;
   }

   if (width != new_width) {
      /* initial rescaling */
      switch (type) {
      case GL_UNSIGNED_BYTE:
	 {
	    GLubyte *ub_data = (GLubyte *) data;
	    for (i = 0; i < new_width; i++) {
	       j = i * width / new_width;
	       for (k = 0; k < components; k++) {
		  texture[i * components + k] = ub_data[j * components + k];
	       }
	    }
	 }
	 break;
      default:
	 /* Not implemented */
	 return GLU_ERROR;
      }
   }

   /* generate and load mipmap images */
   for (l = 0; l < levels; l++) {
      glTexImage1D(GL_TEXTURE_1D, l, components, new_width, 0,
		   format, GL_UNSIGNED_BYTE, texture);

      /* Scale image down to 1/2 size */
      new_width = new_width / 2;
      for (i = 0; i < new_width; i++) {
	 for (k = 0; k < components; k++) {
	    GLint sample1, sample2;
	    sample1 = (GLint) texture[i * 2 * components + k];
	    sample2 = (GLint) texture[(i * 2 + 1) * components + k];
	    texture[i * components + k] = (GLubyte) ((sample1 + sample2) / 2);
	 }
      }
   }

   free(texture);

   return 0;
}

/*
 * Given an pixel format and datatype, return the number of bytes to
 * store one pixel.
 */
static GLint bytes_per_pixel(GLenum format, GLenum type)
{
   GLint n, m;

   switch (format) {
   case GL_COLOR_INDEX:
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
      n = 1;
      break;
   case GL_LUMINANCE_ALPHA:
      n = 2;
      break;
   case GL_RGB:
   case GL_BGR:
      n = 3;
      break;
   case GL_RGBA:
   case GL_BGRA:
#ifdef GL_EXT_abgr
   case GL_ABGR_EXT:
#endif
      n = 4;
      break;
   default:
      n = 0;
   }

   switch (type) {
   case GL_UNSIGNED_BYTE:
      m = sizeof(GLubyte);
      break;
   case GL_BYTE:
      m = sizeof(GLbyte);
      break;
   case GL_BITMAP:
      m = 1;
      break;
   case GL_UNSIGNED_SHORT:
      m = sizeof(GLushort);
      break;
   case GL_SHORT:
      m = sizeof(GLshort);
      break;
   case GL_UNSIGNED_INT:
      m = sizeof(GLuint);
      break;
   case GL_INT:
      m = sizeof(GLint);
      break;
   case GL_FLOAT:
      m = sizeof(GLfloat);
      break;
   default:
      m = 0;
   }

   return n * m;
}

GLAPI GLint GLAPIENTRY gluBuild2DMipmaps(GLenum target, GLint components,
		  GLsizei width, GLsizei height, GLenum format,
		  GLenum type, const void *data)
{
   GLint w, h, maxsize;
   void *image, *newimage;
   GLint neww, newh, level, bpp;
   int error;
   GLboolean done;
   GLint retval = 0;
   GLint unpackrowlength, unpackalignment, unpackskiprows, unpackskippixels;
   GLint packrowlength, packalignment, packskiprows, packskippixels;

   if (width < 1 || height < 1)
      return GLU_INVALID_VALUE;

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);

   w = round2(width);
   if (w > maxsize) {
      w = maxsize;
   }
   h = round2(height);
   if (h > maxsize) {
      h = maxsize;
   }

   bpp = bytes_per_pixel(format, type);
   if (bpp == 0) {
      /* probably a bad format or type enum */
      return GLU_INVALID_ENUM;
   }

   /* Get current glPixelStore values */
   glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackrowlength);
   glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackalignment);
   glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackskiprows);
   glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackskippixels);
   glGetIntegerv(GL_PACK_ROW_LENGTH, &packrowlength);
   glGetIntegerv(GL_PACK_ALIGNMENT, &packalignment);
   glGetIntegerv(GL_PACK_SKIP_ROWS, &packskiprows);
   glGetIntegerv(GL_PACK_SKIP_PIXELS, &packskippixels);

   /* set pixel packing */
   glPixelStorei(GL_PACK_ROW_LENGTH, 0);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_SKIP_ROWS, 0);
   glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

   done = GL_FALSE;

   if (w != width || h != height) {
      /* must rescale image to get "top" mipmap texture image */
      image = malloc((w + 4) * h * bpp);
      if (!image) {
	 return GLU_OUT_OF_MEMORY;
      }
      error = gluScaleImage(format, width, height, type, data,
			    w, h, type, image);
      if (error) {
	 retval = error;
	 done = GL_TRUE;
      }
   }
   else {
      image = (void *) data;
   }

   level = 0;
   while (!done) {
      if (image != data) {
	 /* set pixel unpacking */
	 glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	 glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	 glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	 glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
      }

      glTexImage2D(target, level, components, w, h, 0, format, type, image);

      if (w == 1 && h == 1)
	 break;

      neww = (w < 2) ? 1 : w / 2;
      newh = (h < 2) ? 1 : h / 2;
      newimage = malloc((neww + 4) * newh * bpp);
      if (!newimage) {
	 return GLU_OUT_OF_MEMORY;
      }

      error = gluScaleImage(format, w, h, type, image,
			    neww, newh, type, newimage);
      if (error) {
	 retval = error;
	 done = GL_TRUE;
      }

      if (image != data) {
	 free(image);
      }
      image = newimage;

      w = neww;
      h = newh;
      level++;
   }

   if (image != data) {
      free(image);
   }

   /* Restore original glPixelStore state */
   glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackrowlength);
   glPixelStorei(GL_UNPACK_ALIGNMENT, unpackalignment);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackskiprows);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackskippixels);
   glPixelStorei(GL_PACK_ROW_LENGTH, packrowlength);
   glPixelStorei(GL_PACK_ALIGNMENT, packalignment);
   glPixelStorei(GL_PACK_SKIP_ROWS, packskiprows);
   glPixelStorei(GL_PACK_SKIP_PIXELS, packskippixels);

   return retval;
}

}
