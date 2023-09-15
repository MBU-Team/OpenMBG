//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _DGL_H_
#define _DGL_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMGL_H_
#include "platform/platformGL.h"
#endif

class TextureObject;
class GFont;
class MatrixF;
class RectI;
class ColorI;
class ColorF;
class Point2I;
class Point2F;
class Point3F;

//------------------------------------------------------------------------------
//-------------------------------------- Bitmap Drawing
//
/// Enumerations used for telling the bitmap drawing how/if to flip a bitmap
/// @see dglDrawBitmap
/// @see dglDrawBitmapStretch
/// @see dglDrawBitmapSR
/// @see dglDrawBitmapStretchSR
enum GFlipConstants {
GFlip_None = 0,
GFlip_X    = BIT(0),
GFlip_Y    = BIT(1),
GFlip_XY   = GFlip_X | GFlip_Y
};

/// @defgroup dgl_bitmap_mod Bitmap Modulation
/// These functions control a modulation color that is used to modulate all drawn objects
/// @{

/// Sets the current color to modulate objects with, similar to glColor3fv()
/// @see dglGetBitmapModulation
/// @see dglClearBitmapModulation
void dglSetBitmapModulation(const ColorF& in_rColor);
/// Gets the current color modulation in float format [0,1]
/// @see dglSetBitmapModulation
/// @see dglClearBitmapModulation
void dglGetBitmapModulation(ColorF* color);
/// Gets the current color modulation in integer format [0,256)
/// @see dglSetBitmapModulation
/// @see dglClearBitmapModulation
void dglGetBitmapModulation(ColorI* color);
/// Sets current color modulation to white, so no modulation takes place
/// @see dglSetBitmapModulation
/// @see dglGetBitmapModulation
void dglClearBitmapModulation();
/// @}


// Note that you must call this _after_ SetBitmapModulation if the two are different
//  SetBMod sets the text anchor to the modulation color
/// Sets the anchor color for text coloring, useful when mixing text colors
void dglSetTextAnchorColor(const ColorF&);

/// @defgroup dgl_bitmap_draw Bitmap Drawing Functions
/// These functions allow you to draw a bitmap.
/// Each function will draw the bitmap in a specific way regarding stretching
/// @{

/// Draws a bitmap, starting from the lower left corner, going up and right
/// @param texObject texture object to be drawn
/// @param in_rAt where to draw the texture in 2d coordinates
/// @param in_flip enumerated constant representing any flipping to be done about the x and/or y axis
void dglDrawBitmap(TextureObject* texObject,
                   const Point2I& in_rAt,
                   const U32      in_flip = GFlip_None);
/// Draws a bitmap that is stretched
/// @param texObject texture object to be drawn
/// @param in_rStretch rectangle where the texture will be drawn in 2d coordinates
/// @param in_flip enumerated constant representing any flipping to be done about the x and/or y axis
void dglDrawBitmapStretch(TextureObject* texObject,
                          const RectI&   in_rStretch,
                          const U32      in_flip = GFlip_None);
/// Draws a sub region of a texture
/// @param texObject texture object to be drawn
/// @param in_rAt point where the texture is to be drawn
/// @param in_rSubRegion portion of the texture to be drawn
/// @param in_flip enumerated constant representing any flipping to be done about the x and/or y axis
void dglDrawBitmapSR(TextureObject* texObject,
                     const Point2I& in_rAt,
                     const RectI&   in_rSubRegion,
                     const U32      in_flip = GFlip_None);
/// Draws a stretched sub region of a texture
/// @param texObject texture object to be drawn
/// @param in_rStretch rectangle where the texture object will be drawn
/// @param in_rSubRegion sub region of the texture that will be applied over the stretch region of the screen
/// @param in_flip enumerated constant representing any flipping to be done about the x and/or y axis
void dglDrawBitmapStretchSR(TextureObject* texObject,
                            const RectI&   in_rStretch,
                            const RectI&   in_rSubRegion,
                            const U32      in_flip = GFlip_None);
/// @}
/// @defgroup dgl_text Text Functions
/// These functions draw a string on the string with a given font
/// @{

/// Draws text at a location in 2d gui coordinates
/// Also supports color tags to modulate the given text
/// @returns the number of x-pixels traversed
/// @param font font to draw with, usually found in the profile
/// @param ptDraw point where to start drawing text
/// @param in_string string to be drawn
/// @param colorTable lookups for the color tags
/// @param maxColorIndex size of the colorTable
U32 dglDrawText(GFont *font, const Point2I &ptDraw, const void *in_string, const ColorI *colorTable = NULL, const U32 maxColorIndex = 9);
/// Draws "n" number of characters from the string, in_string
/// @returns the number of x-pixels traversed
/// @see dglDrawText
U32 dglDrawTextN(GFont *font, const Point2I &ptDraw, const void *in_string, U32 n, const ColorI *colorTable = NULL, const U32 maxColorIndex = 9);
/// @}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Drawing primitives

/// @defgroup dgl_primitive Primitive Drawing
/// Easy functions for drawing lines and (un)textured rectangles in 2d or 3d space
/// @{

/// draws a line from x1,y1 to x2,y2 in the specified color
void dglDrawLine(S32 x1, S32 y1, S32 x2, S32 y2, const ColorI &color);
/// draws a line from startPt to endPt in specified color
void dglDrawLine(const Point2I &startPt, const Point2I &endPt, const ColorI &color);
/// draws a wireframe rectangle from upperL to lowerR in specified color
void dglDrawRect(const Point2I &upperL, const Point2I &lowerR, const ColorI &color);
/// draws a wireframe rectangle in "rect" in specified color
void dglDrawRect(const RectI &rect, const ColorI &color);
/// draws an UNTEXTURED filled rectangle from upperL to lowerR in specified color
void dglDrawRectFill(const Point2I &upperL, const Point2I &lowerR, const ColorI &color);
/// draws an UNTEXTURED filled rectangle in "rect" in specified color
void dglDrawRectFill(const RectI &rect, const ColorI &color);
/// draws a square, with center point "screenPoint", width of "width" on an angle of "spinAngle" in 2d
void dglDraw2DSquare( const Point2F &screenPoint, F32 width, F32 spinAngle );
/// draws a square, with center point "position", width of "width" on an angle of "spinAngle" in 3d
void dglDrawBillboard( const Point3F &position, F32 width, F32 spinAngle );
/// @}
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Matrix functions

/// @defgroup dgl_matrix Matrix Functions
/// These functions manipulate the current matrix.  The current matrix could be modelivew, projection, or texture
/// @note Make sure you specify which matrix you want to manipulate with a call to glMatrixMode(enum matrix); before calling dglLoadMatrix() or dglMultMatrix()
/// @{

/// loads matrix "m" into the current matrix mode
void dglLoadMatrix(const MatrixF *m);
/// multiplies the current transformation matrix by matrix "m"
void dglMultMatrix(const MatrixF *m);
/// returns the current modelview matrix
void dglGetModelview(MatrixF *m);
/// returns the current projection matrix
void dglGetProjection(MatrixF *m);
/// @}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Camera functions
/// @defgroup dgl_camera_func Camera functions
/// @{

/// Returns the pixel scale, namely:
///
/// viewPort.extent.x / 640.0
F32 dglGetPixelScale();
/// The scale factor of a world unit to a screen unit
F32 dglGetWorldToScreenScale();
/// Returns the screen length of a line of distance "radius" that is "dist" units away from the camera that is perpendicular to the line of sight, namely:
///
/// (radius / dist) * worldToScreenScale
F32 dglProjectRadius(F32 dist, F32 radius);
/// @}

/// @defgroup dgl_view Viewing Volume Functions
/// These functions set up the view cube of the window.
/// @{

/// sets the viewport for the window
void dglSetViewport(const RectI &aViewPort);
/// gets the current viewport of the window
void dglGetViewport(RectI* outViewport);
/// Sets the viewing frustrum.  This effectively creates the view volume and sets up the 6 clipping planes (near, far, left, right, top, bottom)
/// @param left This is the position of the left vertical clipping plane
/// @param right This is the position of the right vertical clipping plane
/// @param top This is the position of the top horizontal clipping plane
/// @param bottom This is the position of the bottom horizontal clipping plane
/// @param nearDist This is the distance between the eye and the near clipping plane
/// @param farDist This is the distance between the eye and the far clipping plane
/// @param ortho (optional, default is false) If left false, calling this function will create a projection viewing volume.  If true, it will be orthographic
void dglSetFrustum(F64 left, F64 right, F64 bottom, F64 top, F64 nearDist, F64 farDist, bool ortho = false);
/// Returns the parameters for the current viewing frustrum
/// @see dglSetFrustrum
void dglGetFrustum(F64 *left, F64 *right, F64 *bottom, F64 *top, F64 *nearDist, F64 *farDist);
/// returns whether or not the coordinate system is orthographic (if it is not projected)
bool dglIsOrtho();
/// Sets up an orthographical viewport and clipping region.  This is best used for guis
/// @param clipRect The bounds of the coordinate system
void dglSetClipRect(const RectI &clipRect);
/// Gets the last clip rect specified by a call to dglSetClipRect
/// @see dglSetClipRect
const RectI& dglGetClipRect();
/// @}

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Misc
/// Projects a point on the screen in 3d space into a point on the screen
/// @returns true if the point is on the screen, false if not and screenPoint will be (0,0,0)
bool  dglPointToScreen( Point3F &point3D, Point3F &screenPoint );

//--------------------------------------------------------------------------
// Debug functions
/// Checks to see if all states are "canonical"
/// @see dglSetCanonicalState
bool dglIsInCanonicalState();

/// Sets states to a "canonical" state
/// @note a "canonical" state is described as:
///
///  BLEND disabled
///
///  TEXTURE_2D disabled on both texture units.
///
///  ActiveTexture set to 0
///
///  LIGHTING off
///
///  winding : clockwise ?
///
///  cullface : disabled
void dglSetCanonicalState();

/// Gets the current state of all transformation matrices
/// @param mvDepth Number of "pushes" made to the modelview matrix without balancing "pops"
/// @param pDepth  Number of "pushes" made to the projection matrix without balancing "pops"
/// @param t0Depth  Number of "pushes" made to the texture 0 matrix without balancing "pops"
/// @param t0Matrix The current texture 0 matrix, should be a 4-element array
/// @param t1Depth  Number of "pushes" made to the texture 1 matrix without balancing "pops"
/// @param t1Matrix The current texture 1 matrix, should be a 4-element array
/// @param vp The current viewport, should be a 4-element array
/// @see dglCheckState
void dglGetTransformState(S32* mvDepth,
                          S32* pDepth,
                          S32* t0Depth,
                          F32* t0Matrix,
                          S32* t1Depth,
                          F32* t1Matrix,
                          S32* vp);

/// Checks to see that the given information matches the current transform state
/// @see dglGetTransformState
bool dglCheckState(const S32 mvDepth, const S32 pDepth,
                   const S32 t0Depth, const F32* t0Matrix,
                   const S32 t1Depth, const F32* t1Matrix,
                   const S32* vp);


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- //
// Advanced hardware functionality.

/// Toggle handle for n-patches
extern bool gNPatch;
/// LOD for n-patches
extern signed int gNPatchLevel;
/// Number of full screen anti-aliasting samples
extern signed int gFSAASamples;

#if ENABLE_NPATCH

/// Starts an n-patch
/// @note Do not forget to call dglNPatchEnd when you are done your N-Patch
/// @see dglNPatchEnd
inline void dglNPatchBegin()
{
   if (gNPatch && dglDoesSupportNPatch())
   {
      dglSetNPatch(gNPatch);
      dglSetNPatchLevel(gNPatchLevel); // could optimize this per frame, I'm guessing. !!!!!TBD
   }
}

/// Ends an n-patch
/// @note Make sure you begin an N-Patch before ending it
/// @see dglNPatchBegin
inline void dglNPatchEnd()
{
   if (gNPatch && dglDoesSupportNPatch())
      dglSetNPatch(false);
}

#else

// no-op the calls.
/// Dummy function when n-patches are not compiled into the engine
#define dglNPatchBegin()
/// Dummy function when n-patches are not compiled into the engine
#define dglNPatchEnd()

#endif

#endif // _H_DGL
