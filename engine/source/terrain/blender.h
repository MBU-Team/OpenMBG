//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#ifndef _BLENDER_H_
#define _BLENDER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TERRDATA_H_
#include "terrain/terrData.h"
#endif
#ifndef _TERRRENDER_H_
#include "terrain/terrRender.h"
#endif

#define GRIDFLAGS(X,Y)   (TerrainRender::mCurrentBlock->findSquare( 0, Point2I( X, Y ) )->flags)
#define MATERIALSTART       (GridSquare::MaterialStart)
/**
   This MODULE contains class Blender. Blender has a set of textures and an ALPHA-VALUE MAP, and
   exposes   via the method Blender::blend(X, Y, LEVEL, LIGHT-MAP, DESTINATION-MIP-MAP) the facility
   to "blend" its textures into DESTINATION-MIP-MAP using X & Y as an offset into the ALPHA-VALUE
   MAP and the LIGHT-MAP.

   The LANDSCAPE of the MISSION is a single (mile square?) TILE repeated infinitely.
   The TILE is a SQUARES_PER_TILE_EDGE (currently 256) by SQUARES_PER_TILE_EDGE Row-Major
   array of SQUARES.

   A TEXEL in this context is a single element of a TEXTURE, an RGB triple or RGBA quad--what
   would be thought of as a PIXEL were it mapped one-to-one to the display device.

   A LUMEL is similarly the smallest unit of a LIGHT-MAP.  It consists of an RGB triple (there doesn't
   seem to be any special specular code, etc.) which is applied to the blended texel.

   And an ALPHEL is similarly the smallest unit of a TEXTURE-ALPHA-MAP. Each TEXEL of the blended
   (destination) texture is formed from the corresponding TEXELs of N source TEXTUREs.  The ALPHELs
   specify what part of the destination TEXEL coems from each source TEXEL.

   The Blender is created with a fixed NUMBER OF MIPMAP LEVELS, a fixed NUMBER OF SOURCE TEXTURES,
   and an ALPHA-VALUE MAP.  The ALPHA-VALUE MAP is passed by reference (I don't know if it
   can change interactively).  The maximum usable value for NUMBER OF MIPMAP LEVELS is
   MAX_MIPMAP_LEVELS (currently 4). I don't think anything bad happens if this is exceeded.

   Each SOURCE TEXTURE is then added to the Blender with addSourceTexture, by index number,
   specifying the MIP-MAP for the texture by value.

   A MIP-MAP is an array of NUMBER OF MIPMAP LEVELS square BIT-MAPs in progressively
   lower resolutions. The highest resolution BIT-MAP must be TEXELS_PER_SOURCE_EDGE (currently 256)
   TEXELS on each side. Each successive BIT-MAP in the MIP-MAP must be 1/2 the resolution of the
   preceding, or 1/4 the size.  Each BIT-MAP is in Row-Major 3*8-bit RGB format.

   Blender stores the value of the MIP-MAPs in an internal format, based on SQUARES. Each MIP-MAP
   is stored as NUMBER OF MIPMAP LEVELS consecutive levels of descending resolution. Each level
   of the MIP-MAP is stored as SQUARES_PER_MIPMAP (currently 64) consecutive SQUAREs in Row-Major
   form. Each Square is stored as consecutive 32-bit 0GBR TEXELs in Row-Major form (the 0 in the
   format means that those bits are waste). Please note that on LITTLE_ENDIAN platforms the
   32-bit 0BGR is identical in representation to 4*8-bit RGB0, however on BIG_ENDIAN platforms
   it is 4*8-bit 0BGR.

   The ALPHA-VALUE MAP consists of NUMBER OF SOURCE TEXTURES individual TEXTURE-ALPHA-MAPs.
   Individual TEXTURE-ALPHA-MAPs consist of a SQUARES_PER_TILE_EDGE by SQUARES_PER_TILE_EDGE
   Row-Major array of 8-bit alpha blending values.  The original code only used the high six bits
   of each 8-bit value, but I am using the whole thing.  I don't know if the lower two bits even
   have meaningful values, but since they are the low-order two bits I don't much care; the worst
   it can do is introduce a charming randomness. The ALPHELs are 8-bit fixed point, implicitly
   divided by 256. It is implied by the code that for each offset into the TEXTURE-ALPHA-MAPs,
   the sum of the ALPHELs at that offset across all of the TEXTURE-ALPHA-MAPs is less than 1.
   I don't know where this is enforced.

   Since the resolution of the ALPHA-MAP is one ALPHEL per SQUARE, the ALPHELs are interpolated
   across the SQUARE when the terrains are being blended.  Each ALPHEL is the ALPHA-VALUE of the
   lower-left corner of the corresponding square.  In order to compute the unlit blended texture
   of a SQUARE (each SQUARE is processed separately), for each SOURCE TEXTURE the ALPHELs at the
   four corners of   the SQUARE are taken and interpolated smoothly across the square.

  <b>How Blender::blend() does its thing:</b>

   Blend() blends an array of SQUAREs of the TILE and a LIGHT-MAP into an 8-level MIP-MAP of
   from 128x128 to 1x1 6-bit RGBA-5551 TEXELs per BIT-MAP.  The location in the TILE to blend
   from is specified by the X,Y co-ordinate of the lower-left corner and the length in SQUAREs
   of a side of the square to be blended.  The length of a side of the square to be blended in
   SQUAREs combined with the fixed 128x128 size of the highest resolution destination BIT-MAP
   implies a umber of TEXELs per SQUARE.

   Blend() blends the desired SQUAREs into the highest resolution texture of the MIP-MAP and
   generates the rest of the MIP-MAP from the first one (this seems like it should be an
   independent utility).

   Blend processes each SQUARE in turn, using the Terrain Grid to choose which SOURCE TEXTUREs
   apply to that square, and the ALPHA-MAPs for that SQUARE to determine the contribution 
   of individual SOURCE TEXTURE's TEXELs.  This is a simple multiplication of TEXEL values
   by the interpolated ALPHELs.

   It then applies the light map by sub-dividing the SQUARE into LUMELS_PER_SQUARE sub-squares and
   interpolating LUMEL values across the sub-square in a manner exactly similar to the ALPHA-MAP. Each
   LUMEL is an RGB triple, and each component of the LUMEL is multipled by the corresponding component
   of the unlit texture to create the lit texture.

   @note Warning, this code is ugly, even though it is pretty well documented. Flee!
 */
class Blender
{
    /// Pointer to big buffer of source textures and mipmaps.
    U32 *bmp_alloc_ptr;

    /// One square buffer used for blending...
    U32 *blendbuffer;

    /// List of pointers into bmp buffer.  Grouped by mip level, 
    ///  so first X pointers are textures 0-X, mip 0.
    U32 **bmpdata;

    /// List of pointers to alpha data for the bmp types.
    U8 **alpha_data;

    /// Number of bmp types
    int num_src_bmps;

    /// Mip levels (including top detail) for each bmp type
    int num_mip_levels;

public:
    /// Constructor
    ///
    /// @param  num_bmp_types   Number of textures we are blending.
    /// @param  mips_per_bmp    This should include top level (always >= 1).
    /// @param  alphadata       This is an 8 bit 256x256 bitmap.
    Blender( int num_bmp_types, int mips_per_bmp, U8 **alphadata );
    ~Blender();

    /// Blends into 5551 format.
    ///
    /// X and Y are in blocks (same resolution as alpha table, i.e. at 
    /// high detail, there are 4x4 blocks covered by the 128x128 destination bmp.)
    ///
    /// @param  x           Position in blocks.
    /// @param  y           Position in blocks.
    /// @param  lightmap    A 16 bit 512x512 bitmap.
    /// @param  level       What mipmapping level to blend to?
    /// @param  destmips    This is an array of pointers to the bitmap and it's 
    ///                     mips to be filled in by this function
    void blend( int x, int y, int level, const U16 *lightmap, U16 **destmips );

    /// Add a texture to use in blending.
    ///
    /// Call this once per bmp type.  It copies the bmp into it's own format,
    ///  so you can then delete your versions of the bmp and mips.
    ///
    /// @param  bmp_type    Type of the bitmap.
    /// @param  bmps        This is an array of pointers to the bitmap and its mipmaps,
    ///                     highest detail first.  Should be in 24bit format.
    void addSourceTexture( int bmp_type, const U8 **bmps );
};

#endif
