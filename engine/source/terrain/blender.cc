//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/blender.h"

/*************Configuration Switches*******************************/
// Haven't determined whether the alphaTable is a big win.
// Needs more measurement, must check tuning.
// alphaTable decreases integer math but increases RAM cache usage.
#define USE_ALPHA_TABLE_AT_4         1
#define USE_ALPHA_TABLE_AT_3         1
#define USE_ALPHA_TABLE_AT_2         1


#if defined(TORQUE_SUPPORTS_NASM)
#  define BLENDER_USE_ASM
#endif


/*************Explanation*******************************/
// Manifest CONSTANTS mentioned within are defined right below.
// doc block moved to blender.h -- bjg

// Primary Manifest Values
#define TEXELS_PER_SOURCE_EDGE_LOG2       8
#define TEXELS_PER_TARGET_EDGE_LOG2       TerrainTextureMipLevel
#define LUMELS_PER_TILE_EDGE_LOG2         9
#define SQUARES_PER_TILE_EDGE_LOG2        8
#define MAX_MIPMAP_LEVELS                 4
#define MAXIMUM_TEXTURES                  4
#define SQUARES_PER_MIPMAP_EDGE_LOG2      (10 - TEXELS_PER_TARGET_EDGE_LOG2)

// Derived Manifest Values
#define MAX_TEXELS_PER_SQUARE_EDGE_LOG2   (TEXELS_PER_SOURCE_EDGE_LOG2 - SQUARES_PER_MIPMAP_EDGE_LOG2)
#define MIN_TEXELS_PER_SQUARE_EDGE_LOG2   (MAX_TEXELS_PER_SQUARE_EDGE_LOG2 - MAX_MIPMAP_LEVELS + 1)
#define LUMELS_PER_SQUARE_EDGE_LOG2       (LUMELS_PER_TILE_EDGE_LOG2 - SQUARES_PER_TILE_EDGE_LOG2)

#define MAX_TEXELS_PER_SQUARE_EDGE        (1 << MAX_TEXELS_PER_SQUARE_EDGE_LOG2)
#define MAX_TEXELS_PER_SQUARE_LOG2        (MAX_TEXELS_PER_SQUARE_EDGE_LOG2 << 1)
#define MAX_TEXELS_PER_SQUARE             (MAX_TEXELS_PER_SQUARE_EDGE*MAX_TEXELS_PER_SQUARE_EDGE)
#define LUMELS_PER_SQUARE_EDGE            (1 << LUMELS_PER_SQUARE_EDGE_LOG2)
#define LUMELS_PER_SQUARE                 (LUMELS_PER_SQUARE_EDGE*LUMELS_PER_SQUARE_EDGE)
#define LUMELS_PER_TILE_EDGE              (1 << LUMELS_PER_TILE_EDGE_LOG2)
#define LUMELS_PER_TILE_EDGE_MASK         (LUMELS_PER_TILE_EDGE - 1)
#define SQUARES_PER_TILE_EDGE             (1 << SQUARES_PER_TILE_EDGE_LOG2)
#define SQUARES_PER_TILE_EDGE_MASK        (SQUARES_PER_TILE_EDGE - 1)
#define SQUARES_PER_MIPMAP_EDGE           (1 << SQUARES_PER_MIPMAP_EDGE_LOG2)
#define SQUARES_PER_MIPMAP_EDGE_MASK      (SQUARES_PER_MIPMAP_EDGE-1)
#define SQUARES_PER_MIPMAP                (SQUARES_PER_MIPMAP_EDGE*SQUARES_PER_MIPMAP_EDGE)
#define TEXELS_PER_SOURCE_EDGE            (1 << TEXELS_PER_SOURCE_EDGE_LOG2)
#define TEXELS_PER_TARGET_EDGE            (1 << TEXELS_PER_TARGET_EDGE_LOG2)
#define TEXELS_PER_SOURCE_BMP             (TEXELS_PER_SOURCE_EDGE * TEXELS_PER_SOURCE_EDGE)
#define TEXELS_PER_TARGET_BMP             (TEXELS_PER_TARGET_EDGE * TEXELS_PER_TARGET_EDGE)

// Paul's leftovers
#define ALPHA_WID                         (1 << SQUARES_PER_TILE_EDGE_LOG2)

// -------------------------
// The following defines are for indexing into the source image maps.
// based on endianness, RGBA become ABGR, and we need to change indexing.
#if defined(TORQUE_BIG_ENDIAN)
#define SRC_IS_ABGR     1
#define SRC_OFF_RED     0
#define SRC_OFF_GREEN   1
#define SRC_OFF_BLUE    2
#else
#define SRC_IS_ABGR     0
#define SRC_OFF_RED     0
#define SRC_OFF_GREEN   1
#define SRC_OFF_BLUE    2
#endif

extern "C"
{
   U32 lumels[4];
}

#if defined(BLENDER_USE_ASM)

extern "C"
{
   // declarations for the NASM functions
   void doSquare4( U32 *dst, int sq_shift, const int *aoff, 
      const U32 *const *bmp_ptrs, const U8 *const *alpha_ptrs );
   void doSquare3( U32 *dst, int sq_shift, const int *aoff, 
      const U32 *const *bmp_ptrs, const U8 *const *alpha_ptrs );
   void doSquare2( U32 *dst, int sq_shift, const int *aoff, 
      const U32 *const *bmp_ptrs, const U8 *const *alpha_ptrs );
   void doLumelPlus1Mip( U16 *dstmip0, U16 *dstmip1, const U32 *srcptr );
   void do1x1Lumel( U16 *dstptr, const U32 *srcptr );
   void cheatmips( U16 *srcptr, U16 *dstmip0, U16 *dstmip1, int wid );
   void cheatmips4x4( const U16 *srcptr, U16 *dstmip0, U16 *dstmip1 );

   // global vars used for parameter passing
   extern U32 sTargetTexelsPerLumel_log2;
   extern U32 sTargetTexelsPerLumel;
   extern U32 sTargetTexelsPerLumelDiv2;

   extern U32 nextsrcrow;
   extern U32 nextdstrow;
   extern U32 mip0_dstrowadd;
   extern U32 mip1_dstrowadd;
   extern U32 minus1srcrowsPlus8;
   extern U32 srcrows_x2_MinusTPL;

}

#else /* BLENDER_USE_ASM */

static U8 alphaTable[64 * 256];
class InitAlphaTable
{
public:
   // build alpha blending table for C versions...
   // This is just a cached pre-calc of some math...
   InitAlphaTable()
   {
      for (U32 alpha = 0; alpha < 64; ++alpha)
         for (U32 pix = 0; pix < 256; ++pix)
         {
            alphaTable[(alpha << 8) | pix] = (pix * alpha + 32) / 63;
         }
   }
};
static InitAlphaTable initAlphaTable; // Okay, cheesy


static void doSquare4( U32 *bmp_dest, int sq_shift, const int *alphaOffsets, const U32 *const *bmp_ptrs, 
                      const U8 *const *alpha_ptrs )
{
   int squareSize = 1 << sq_shift;
   int squareSizeColors = squareSize;

   int left_scan_edge_alpha_a;
   int left_scan_edge_alpha_b;
   int left_scan_edge_alpha_c;
   int left_scan_edge_alpha_d;

   int delta_left_alpha_a;
   int delta_left_alpha_b;
   int delta_left_alpha_c;
   int delta_left_alpha_d;

   int right_scan_edge_alpha_a;
   int right_scan_edge_alpha_b;
   int right_scan_edge_alpha_c;
   int right_scan_edge_alpha_d;

   int delta_right_alpha_a;
   int delta_right_alpha_b;
   int delta_right_alpha_c;
   int delta_right_alpha_d;

   {
      int top_left_alpha = alpha_ptrs[0][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int top_right_alpha = alpha_ptrs[0][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int bot_left_alpha = alpha_ptrs[0][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int bot_right_alpha = alpha_ptrs[0][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_a = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_a = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_a = top_left_alpha;
      right_scan_edge_alpha_a = top_right_alpha;

      top_left_alpha = alpha_ptrs[1][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      top_right_alpha = alpha_ptrs[1][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_left_alpha = alpha_ptrs[1][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_right_alpha = alpha_ptrs[1][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_b = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_b = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_b = top_left_alpha;
      right_scan_edge_alpha_b = top_right_alpha;

      top_left_alpha = alpha_ptrs[2][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      top_right_alpha = alpha_ptrs[2][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_left_alpha = alpha_ptrs[2][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_right_alpha = alpha_ptrs[2][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_c = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_c = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_c = top_left_alpha;
      right_scan_edge_alpha_c = top_right_alpha;

      top_left_alpha = alpha_ptrs[3][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      top_right_alpha = alpha_ptrs[3][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_left_alpha = alpha_ptrs[3][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_right_alpha = alpha_ptrs[3][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_d = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_d = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_d = top_left_alpha;
      right_scan_edge_alpha_d = top_right_alpha;
   }

   U8* sourcePtr_a = (U8*)bmp_ptrs[0];
   U8* sourcePtr_b = (U8*)bmp_ptrs[1];
   U8* sourcePtr_c = (U8*)bmp_ptrs[2];
   U8* sourcePtr_d = (U8*)bmp_ptrs[3];
   U8* destPtr = (U8*)bmp_dest;

   for(S32 iy = squareSize; --iy >= 0;)
   {
      int scan_alpha_a = left_scan_edge_alpha_a;
      left_scan_edge_alpha_a += delta_left_alpha_a;
      int delta_scan_alpha_a = (right_scan_edge_alpha_a - scan_alpha_a) / squareSize;
      right_scan_edge_alpha_a += delta_right_alpha_a;

      int scan_alpha_b = left_scan_edge_alpha_b;
      left_scan_edge_alpha_b += delta_left_alpha_b;
      int delta_scan_alpha_b = (right_scan_edge_alpha_b - scan_alpha_b) / squareSize;
      right_scan_edge_alpha_b += delta_right_alpha_b;

      int scan_alpha_c = left_scan_edge_alpha_c;
      left_scan_edge_alpha_c += delta_left_alpha_c;
      int delta_scan_alpha_c = (right_scan_edge_alpha_c - scan_alpha_c) / squareSize;
      right_scan_edge_alpha_c += delta_right_alpha_c;

      int scan_alpha_d = left_scan_edge_alpha_d;
      left_scan_edge_alpha_d += delta_left_alpha_d;
      int delta_scan_alpha_d = (right_scan_edge_alpha_d - scan_alpha_d) / squareSize;
      right_scan_edge_alpha_d += delta_right_alpha_d;

      for(S32 ix = squareSizeColors; --ix >= 0;)
      {
#if USE_ALPHA_TABLE_AT_4
         int six_bit_alpha_a = (scan_alpha_a >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         int six_bit_alpha_b = (scan_alpha_b >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         int six_bit_alpha_c = (scan_alpha_c >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         int six_bit_alpha_d = (scan_alpha_d >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         destPtr[0] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_RED]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_RED]] +
            alphaTable[six_bit_alpha_c | sourcePtr_c[SRC_OFF_RED]] +
            alphaTable[six_bit_alpha_d | sourcePtr_d[SRC_OFF_RED]];
         destPtr[1] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_GREEN]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_GREEN]] +
            alphaTable[six_bit_alpha_c | sourcePtr_c[SRC_OFF_GREEN]] +
            alphaTable[six_bit_alpha_d | sourcePtr_d[SRC_OFF_GREEN]];
         destPtr[2] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_BLUE]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_BLUE]] +
            alphaTable[six_bit_alpha_c | sourcePtr_c[SRC_OFF_BLUE]] +
            alphaTable[six_bit_alpha_d | sourcePtr_d[SRC_OFF_BLUE]];
#else
#define EIGHT_BITS_SQ_SHIFT_2 (0xff * MAX_TEXELS_PER_SQUARE)
         destPtr[0] = (scan_alpha_a * sourcePtr_a[SRC_OFF_RED] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_RED] +
            scan_alpha_c * sourcePtr_c[SRC_OFF_RED] +
            scan_alpha_d * sourcePtr_d[SRC_OFF_RED]) / EIGHT_BITS_SQ_SHIFT_2;
         destPtr[1] = (scan_alpha_a * sourcePtr_a[SRC_OFF_GREEN] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_GREEN] +
            scan_alpha_c * sourcePtr_c[SRC_OFF_GREEN] +
            scan_alpha_d * sourcePtr_d[SRC_OFF_GREEN]) / EIGHT_BITS_SQ_SHIFT_2;
         destPtr[2] = (scan_alpha_a * sourcePtr_a[SRC_OFF_BLUE] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_BLUE] +
            scan_alpha_c * sourcePtr_c[SRC_OFF_BLUE] +
            scan_alpha_d * sourcePtr_d[SRC_OFF_BLUE]) / EIGHT_BITS_SQ_SHIFT_2;
#endif
         scan_alpha_a += delta_scan_alpha_a;
         scan_alpha_b += delta_scan_alpha_b;
         scan_alpha_c += delta_scan_alpha_c;
         scan_alpha_d += delta_scan_alpha_d;
         sourcePtr_a += 4;
         sourcePtr_b += 4;
         sourcePtr_c += 4;
         sourcePtr_d += 4;
         destPtr += 4;
      }
   }
}

static void doSquare3( U32 *bmp_dest, int sq_shift, const int *alphaOffsets, const U32 *const *bmp_ptrs, 
                      const U8 *const *alpha_ptrs )
{
   int squareSize = 1 << sq_shift;
   int squareSizeColors = squareSize;

   int left_scan_edge_alpha_a;
   int left_scan_edge_alpha_b;
   int left_scan_edge_alpha_c;

   int delta_left_alpha_a;
   int delta_left_alpha_b;
   int delta_left_alpha_c;

   int right_scan_edge_alpha_a;
   int right_scan_edge_alpha_b;
   int right_scan_edge_alpha_c;

   int delta_right_alpha_a;
   int delta_right_alpha_b;
   int delta_right_alpha_c;

   {
      int top_left_alpha = alpha_ptrs[0][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int top_right_alpha = alpha_ptrs[0][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int bot_left_alpha = alpha_ptrs[0][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int bot_right_alpha = alpha_ptrs[0][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_a = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_a = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_a = top_left_alpha;
      right_scan_edge_alpha_a = top_right_alpha;

      top_left_alpha = alpha_ptrs[1][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      top_right_alpha = alpha_ptrs[1][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_left_alpha = alpha_ptrs[1][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_right_alpha = alpha_ptrs[1][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_b = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_b = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_b = top_left_alpha;
      right_scan_edge_alpha_b = top_right_alpha;

      top_left_alpha = alpha_ptrs[2][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      top_right_alpha = alpha_ptrs[2][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_left_alpha = alpha_ptrs[2][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_right_alpha = alpha_ptrs[2][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_c = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_c = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_c = top_left_alpha;
      right_scan_edge_alpha_c = top_right_alpha;
   }

   U8* sourcePtr_a = (U8*)bmp_ptrs[0];
   U8* sourcePtr_b = (U8*)bmp_ptrs[1];
   U8* sourcePtr_c = (U8*)bmp_ptrs[2];
   U8* destPtr = (U8*)bmp_dest;

   for(S32 iy = squareSize; --iy >= 0;)
   {
      int scan_alpha_a = left_scan_edge_alpha_a;
      left_scan_edge_alpha_a += delta_left_alpha_a;
      int delta_scan_alpha_a = (right_scan_edge_alpha_a - scan_alpha_a) / squareSize;
      right_scan_edge_alpha_a += delta_right_alpha_a;

      int scan_alpha_b = left_scan_edge_alpha_b;
      left_scan_edge_alpha_b += delta_left_alpha_b;
      int delta_scan_alpha_b = (right_scan_edge_alpha_b - scan_alpha_b) / squareSize;
      right_scan_edge_alpha_b += delta_right_alpha_b;

      int scan_alpha_c = left_scan_edge_alpha_c;
      left_scan_edge_alpha_c += delta_left_alpha_c;
      int delta_scan_alpha_c = (right_scan_edge_alpha_c - scan_alpha_c) / squareSize;
      right_scan_edge_alpha_c += delta_right_alpha_c;

      for(S32 ix = squareSizeColors; --ix >= 0;)
      {
#if USE_ALPHA_TABLE_AT_3
         int six_bit_alpha_a = (scan_alpha_a >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         int six_bit_alpha_b = (scan_alpha_b >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         int six_bit_alpha_c = (scan_alpha_c >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         destPtr[0] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_RED]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_RED]] +
            alphaTable[six_bit_alpha_c | sourcePtr_c[SRC_OFF_RED]];
         destPtr[1] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_GREEN]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_GREEN]] +
            alphaTable[six_bit_alpha_c | sourcePtr_c[SRC_OFF_GREEN]];
         destPtr[2] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_BLUE]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_BLUE]] +
            alphaTable[six_bit_alpha_c | sourcePtr_c[SRC_OFF_BLUE]];
#else
#define EIGHT_BITS_SQ_SHIFT_2 (0xff * MAX_TEXELS_PER_SQUARE)
         destPtr[0] = (scan_alpha_a * sourcePtr_a[SRC_OFF_RED] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_RED] +
            scan_alpha_c * sourcePtr_c[SRC_OFF_RED]) / EIGHT_BITS_SQ_SHIFT_2;
         destPtr[1] = (scan_alpha_a * sourcePtr_a[SRC_OFF_GREEN] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_GREEN] +
            scan_alpha_c * sourcePtr_c[SRC_OFF_GREEN]) / EIGHT_BITS_SQ_SHIFT_2;
         destPtr[2] = (scan_alpha_a * sourcePtr_a[SRC_OFF_BLUE] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_BLUE] +
            scan_alpha_c * sourcePtr_c[SRC_OFF_BLUE]) / EIGHT_BITS_SQ_SHIFT_2;
#endif
         scan_alpha_a += delta_scan_alpha_a;
         scan_alpha_b += delta_scan_alpha_b;
         scan_alpha_c += delta_scan_alpha_c;
         sourcePtr_a += 4;
         sourcePtr_b += 4;
         sourcePtr_c += 4;
         destPtr += 4;
      }
   }
}

static void doSquare2( U32 *bmp_dest, int sq_shift, int *alphaOffsets, const U32 *const *bmp_ptrs, 
                      const U8 *const *alpha_ptrs )
{
   int squareSize = 1 << sq_shift;
   int squareSizeColors = squareSize;

   int left_scan_edge_alpha_a;
   int left_scan_edge_alpha_b;

   int delta_left_alpha_a;
   int delta_left_alpha_b;

   int right_scan_edge_alpha_a;
   int right_scan_edge_alpha_b;

   int delta_right_alpha_a;
   int delta_right_alpha_b;

   {
      int top_left_alpha = alpha_ptrs[0][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int top_right_alpha = alpha_ptrs[0][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int bot_left_alpha = alpha_ptrs[0][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      int bot_right_alpha = alpha_ptrs[0][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_a = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_a = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_a = top_left_alpha;
      right_scan_edge_alpha_a = top_right_alpha;

      top_left_alpha = alpha_ptrs[1][ alphaOffsets[0] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      top_right_alpha = alpha_ptrs[1][ alphaOffsets[1] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_left_alpha = alpha_ptrs[1][ alphaOffsets[2] ] << MAX_TEXELS_PER_SQUARE_LOG2;
      bot_right_alpha = alpha_ptrs[1][ alphaOffsets[3] ] << MAX_TEXELS_PER_SQUARE_LOG2;

      delta_left_alpha_b = (bot_left_alpha - top_left_alpha) / squareSize;
      delta_right_alpha_b = (bot_right_alpha - top_right_alpha) / squareSize;

      left_scan_edge_alpha_b = top_left_alpha;
      right_scan_edge_alpha_b = top_right_alpha;
   }

   U8* sourcePtr_a = (U8*)bmp_ptrs[0];
   U8* sourcePtr_b = (U8*)bmp_ptrs[1];
   U8* destPtr = (U8*)bmp_dest;

   for(S32 iy = squareSize; --iy >= 0;)
   {
      int scan_alpha_a = left_scan_edge_alpha_a;
      left_scan_edge_alpha_a += delta_left_alpha_a;
      int delta_scan_alpha_a = (right_scan_edge_alpha_a - scan_alpha_a) / squareSize;
      right_scan_edge_alpha_a += delta_right_alpha_a;

      int scan_alpha_b = left_scan_edge_alpha_b;
      left_scan_edge_alpha_b += delta_left_alpha_b;
      int delta_scan_alpha_b = (right_scan_edge_alpha_b - scan_alpha_b) / squareSize;
      right_scan_edge_alpha_b += delta_right_alpha_b;

      for(S32 ix = squareSizeColors; --ix >= 0;)
      {
#if USE_ALPHA_TABLE_AT_2
         int six_bit_alpha_a = (scan_alpha_a >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         int six_bit_alpha_b = (scan_alpha_b >> (MAX_TEXELS_PER_SQUARE_LOG2 - 6)) & 0x3F00;
         destPtr[0] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_RED]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_RED]];
         destPtr[1] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_GREEN]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_GREEN]];
         destPtr[2] = alphaTable[six_bit_alpha_a | sourcePtr_a[SRC_OFF_BLUE]] +
            alphaTable[six_bit_alpha_b | sourcePtr_b[SRC_OFF_BLUE]];
#else
#define EIGHT_BITS_SQ_SHIFT_2 (0xff * MAX_TEXELS_PER_SQUARE)
         destPtr[0] = (scan_alpha_a * sourcePtr_a[SRC_OFF_RED] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_RED] +
            EIGHT_BITS_SQ_SHIFT_2 / 2) / EIGHT_BITS_SQ_SHIFT_2;
         destPtr[1] = (scan_alpha_a * sourcePtr_a[SRC_OFF_GREEN] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_GREEN] +
            EIGHT_BITS_SQ_SHIFT_2 / 2) / EIGHT_BITS_SQ_SHIFT_2;
         destPtr[2] = (scan_alpha_a * sourcePtr_a[SRC_OFF_BLUE] +
            scan_alpha_b * sourcePtr_b[SRC_OFF_BLUE] +
            EIGHT_BITS_SQ_SHIFT_2 / 2) / EIGHT_BITS_SQ_SHIFT_2;
#endif
         scan_alpha_a += delta_scan_alpha_a;
         scan_alpha_b += delta_scan_alpha_b;
         sourcePtr_a += 4;
         sourcePtr_b += 4;
         destPtr += 4;
      }
   }
}

#endif /* BLENDER_USE_ASM */

// old C extruder
static void extrude5551( const U16 *srcMip, U16 *mip, U32 height, U32 width )
{
   const U16 *src = srcMip;
   U16 *dst = mip;
   U32 stride = width << 1;

   for(U32 y = 0; y < height; y++)
   {
      for(U32 x = 0; x < width; x++)
      {
         U32 a = src[0];
         U32 b = src[1];
         U32 c = src[stride];
         U32 d = src[stride+1];
#if SRC_IS_ABGR
         dst[x] = ((( ((a >> 10) & 0x1f) + ((b >> 10) & 0x1f) + ((c >> 10) & 0x1f) + ((d >> 10) & 0x1F) ) >> 2) << 10) |
            ((( ((a >> 5) & 0x1f) + ((b >> 5) & 0x1f) + ((c >> 5) & 0x1f) + ((d >> 5) & 0x1F) ) >> 2) << 5) |
            ((( ((a >> 0) & 0x1F) + ((b >> 0) & 0x1F) + ((c >> 0) & 0x1f) + ((d >> 0) & 0x1f)) >> 2) << 0);
#else
         dst[x] = ((((a >> 11) + (b >> 11) + (c >> 11) + (d >> 11)) >> 2) << 11) |
            (((  ((a >> 6) & 0x1f) + ((b >> 6) & 0x1f) + ((c >> 6) & 0x1f) + ((d >> 6) & 0x1F) ) >> 2) << 6) |
            ((( ((a >> 1) & 0x1F) + ((b >> 1) & 0x1F) + ((c >> 1) & 0x1f) + ((d >> 1) & 0x1f)) >> 2) << 1);
#endif
         src += 2;
      }
      src += stride;
      dst += width;
   }
}


/// Take first mip in array, and extrude rest into other entries of array
///  i.e. power is 7 for 128x128, but there should be power+1 entries in the
///  array.
static void extrude( U16 **mips, U32 power )
{
   U32 width = 1 << (power - 1);

   for ( U32 i = 0; i < power; i++ )
   {
      extrude5551( mips[i], mips[i+1], width, width );
      width >>= 1;
   }
}


/// Blend textures!
///
///   - level is between 2 (high detail) and 5 (low detail) inclusive;
///   - x and y are in alpha sized squares (not tex squares)
///   - lmap is light map data, format is 5551 (1555 ABGR)
///   - destmips is a list of 16-bit 5551 RGBA (1555 ABGR) mip-map buffers for the result.
void Blender::blend( int x, int y, int squaresPerTargetEdge_log2, const U16 *lmap, U16 **destmips )
{
   const int squaresPerTargetEdge(1 << squaresPerTargetEdge_log2); // 32 (low detail) to 4 (high detail).
   const int texelsPerSquareEdge_log2(TEXELS_PER_TARGET_EDGE_LOG2 - squaresPerTargetEdge_log2);  // 5 (high detail) to 2 (low detail)
   const int texelsPerSquareEdge(1 << texelsPerSquareEdge_log2); // == TEXELS_PER_TARGET_EDGE / squaresPerTargetEdge); 4 (low) to 32 (high) detail.
   const int texelsPerSquare_log2(texelsPerSquareEdge_log2 << 1);  // 10 (high detail) to 4 (low detail)
   const int sourceMipMapIndex(MAX_TEXELS_PER_SQUARE_EDGE_LOG2 - texelsPerSquareEdge_log2);
   const int targetTexelsPerLumel_log2(texelsPerSquareEdge_log2 - LUMELS_PER_SQUARE_EDGE_LOG2);
   const int targetTexelsPerLumel(1 << targetTexelsPerLumel_log2);
   const U32 yStrideThroughTarget(TEXELS_PER_TARGET_EDGE);
   const U32 yStrideThroughSquare(texelsPerSquareEdge);
   const U32 xStrideAcrossLumels(targetTexelsPerLumel);
   const U32 yStrideThroughTargetAcrossLumels(yStrideThroughTarget << targetTexelsPerLumel_log2);
   const U32 yStrideThroughSquareAcrossLumels(yStrideThroughSquare << targetTexelsPerLumel_log2);

#if defined(BLENDER_USE_ASM)
   // These are all secret parameters passed to the assembly language code through statics.
   sTargetTexelsPerLumel_log2 = targetTexelsPerLumel_log2;
   sTargetTexelsPerLumel = targetTexelsPerLumel;

   sTargetTexelsPerLumelDiv2 = targetTexelsPerLumel >> 1;
   nextsrcrow = ((yStrideThroughSquare) << 2);
   nextdstrow = ((yStrideThroughTarget) << 1);

   mip0_dstrowadd = (nextdstrow << 1) - (targetTexelsPerLumel << 1);
   mip1_dstrowadd = (nextdstrow >> 1) - (targetTexelsPerLumel);
   minus1srcrowsPlus8 = 8 - nextsrcrow;
   srcrows_x2_MinusTPL = (nextsrcrow << 1) - (targetTexelsPerLumel << 2);
#endif

   const U32 *const*const allSourceBitMaps = &bmpdata[sourceMipMapIndex * num_src_bmps];

   // sy & sx index through the SQUAREs of the DESTINATION MIP-MAP
   // All Source MIP-MAPs are 2D arrays of squares: 
   //      SQUARE source_mip_map_2D[SQUARES_PER_MIPMAP_EDGE][SQUARES_PER_MIPMAP_EDGE];
   // But they are stored as 1D arrays:
   //      SQUARE source_mip_map_1D[SQUARES_PER_MIPMAP_EDGE*SQUARES_PER_MIPMAP_EDGE];
   // therefore the following are equivalent:
   //               source_mip_map_2D[Y][X]
   //                  source_mip_map_1D[(Y * SQUARES_PER_MIPMAP_EDGE) + X]
   //                  source_mip_map_1D[(Y << SQUARES_PER_MIPMAP_EDGE_LOG2) + X]
   //                  source_mip_map_1D[(Y << SQUARES_PER_MIPMAP_EDGE_LOG2) | X]
   // This loop is from [0] through [squaresPerTargetEdge - 1] of the destination
   // and from [y] through [y + squaresPerTargetEdge - 1] of the source.
   // A single terrain TILE is equivalently:
   //      SQUARE terrain_tile_2D[SQUARES_PER_TILE_EDGE][SQUARES_PER_TILE_EDGE];
   // or
   //      SQUARE terrain_tile_1D[SQUARES_PER_TILE_EDGE*SQUARES_PER_TILE_EDGE];
   // therefore the following are equivalent:
   //               terrain_tile_2D[Y][X]
   //                  terrain_tile_1D[(Y << SQUARES_PER_MIPMAP_EDGE_LOG2) | X]
   // Neither source_mip_map_1D nor terrain_tile_1D appear explicitly.
   for ( int yInTarget = 0; yInTarget < squaresPerTargetEdge; yInTarget++ )
   {
      // This whole section is called "doing 2-dimensional array indexing the hard way"
      // yInTile & after_yInTile are the bottom and top of the source square we are actually processing,
      // masked to tile size which is what causes the "repeating" effect
      const int yInTile((y + yInTarget) & SQUARES_PER_TILE_EDGE_MASK);            
      const int after_yInTile((yInTile + 1) & SQUARES_PER_TILE_EDGE_MASK);

      // yInTile_offset and after_yInTile_offset are the offsets into the terrain_tile_1D format arrays
      const int yInTile_offset(yInTile << SQUARES_PER_TILE_EDGE_LOG2);
      const int after_yInTile_offset(after_yInTile << SQUARES_PER_TILE_EDGE_LOG2);

      // py is the row index in squares into the source_mip_map_2D
      const int yInSource(yInTile & SQUARES_PER_MIPMAP_EDGE_MASK);
      // yInSource_offset is the offset in squares into the source_mip_map_1D, times the size
      // of the squares.
      const int yInSource_offset(yInSource << (texelsPerSquare_log2 + SQUARES_PER_MIPMAP_EDGE_LOG2));

      // This loop is from [yInTarget][0] through [yInTarget][squaresPerTargetEdge - 1] of the destination
      // and from [yInTile][x] through [yInTile][x + squaresPerTargetEdge - 1] of the source.
      for ( int xInTarget = 0; xInTarget < squaresPerTargetEdge; xInTarget++ )
      {
         // xInTile & after_xInTile are the left and right side of the source square we are actually processing,
         // masked to tile size which is what causes the "repeating" effect
         const int xInTile((x + xInTarget) & SQUARES_PER_TILE_EDGE_MASK);
         const int after_xInTile((xInTile + 1) & SQUARES_PER_TILE_EDGE_MASK);
         // xInSource is the column index in squares into the source_mip_map_2D
         const int xInSource(xInTile & SQUARES_PER_MIPMAP_EDGE_MASK);
         // As you can see the GRID is accessed in TILE co-ordinates
         const U32 gridflags(GRIDFLAGS( xInTile, yInTile ));

         int numTexturesToBlend = 0;
         int alphaOffsets[4];
         alphaOffsets[0] = yInTile_offset | xInTile;            // precalculate offsets for tile-coords[yInTile][xInTile]
         alphaOffsets[1] = yInTile_offset | after_xInTile;      // and so on for the square bounded by
         alphaOffsets[2] = after_yInTile_offset | xInTile;      //   [yInTile][xInTile]         [yInTile][after_xInTile]
         alphaOffsets[3] = after_yInTile_offset | after_xInTile;//   [after_yInTile][xInTile]   [after_yInTile][after_xInTile]

         // Cache the source textures at our mip-map level as specified by the GRID-FLAGS
         const U32 *sourceSquareBitMaps[MAXIMUM_TEXTURES];
         // Cache the Alpha-Maps as specified by the GRID-FLAGS
         const U8 *alphaMaps[MAXIMUM_TEXTURES];

         // Pre-calculate (U8*) &source_mip_map_2D[yInSource][xInSource] --
         // ( (yInSource * SQUARES_PER_MIPMAP_EDGE) + xInSource ) * sizeof(SQUARE)
         const int bitmapOffset(yInSource_offset | (xInSource << texelsPerSquare_log2));

         for ( int i = 0; i < num_src_bmps; i++ )
            if ( gridflags & (MATERIALSTART << i) ) // Gridflags tell us which materials are used for this square
            {// Cache Alpha maps and bitmaps.
               alphaMaps[ numTexturesToBlend ] = alpha_data[ i ];
               sourceSquareBitMaps[ numTexturesToBlend++ ] = &allSourceBitMaps[ i ][bitmapOffset];

               if ( numTexturesToBlend == MAXIMUM_TEXTURES )
                  break; // Why? What happens if more than (4) textures should be blended?
            }

            const U32 *bufferToLightFrom = blendbuffer;
            switch( numTexturesToBlend ) // Blend 1 square of the numTexturesToBlend bit-maps into the blend buffer
            {
            case 0:// paranoia
            case 1:
               // don't copy the square over...just leave it and tell
               //  lighting code to use src bmp as the source instead of
               //  the blend_buffer;
               bufferToLightFrom = sourceSquareBitMaps[ 0 ];
               break;
            case 2:
               doSquare2( blendbuffer, texelsPerSquareEdge_log2, alphaOffsets, sourceSquareBitMaps, alphaMaps  );
               break;
            case 3:
               doSquare3( blendbuffer, texelsPerSquareEdge_log2, alphaOffsets, sourceSquareBitMaps, alphaMaps  );
               break;
            default: // more subtle paranoia
               doSquare4( blendbuffer, texelsPerSquareEdge_log2, alphaOffsets, sourceSquareBitMaps, alphaMaps  );
               break;
            }

            // [these comments are making me paranoid -- Ed.]

            // copy in the lighting info

            // Once again we make with the linear 2D array
            const U32 xTexelInTarget(xInTarget << texelsPerSquareEdge_log2);
            const U32 yTexelInTarget(yInTarget << texelsPerSquareEdge_log2);
            const U32 yTexelInTarget_offset((yTexelInTarget << TEXELS_PER_TARGET_EDGE_LOG2));
            U16 *const bits0 = &destmips[0][ yTexelInTarget_offset + xTexelInTarget ];
            U16 *const bits1 = &destmips[1][ (yTexelInTarget_offset >> 2) + (xTexelInTarget >> 1) ];
            U16 *const bits2 = &destmips[2][ (yTexelInTarget_offset >> 4) + (xTexelInTarget >> 2) ];

            const U32 base_xInLightmap(xInTile << LUMELS_PER_SQUARE_EDGE_LOG2);
            const U32 base_yInLightmap(yInTile << LUMELS_PER_SQUARE_EDGE_LOG2);

            U32 yInLightmap_offset(base_yInLightmap << LUMELS_PER_TILE_EDGE_LOG2);
            U32 next_yInLightmap(base_yInLightmap);
            U32 yTexelInTargetSquare_offset(0);
            U32 yTexelInSquare_offset(0);
            for(U32 yLumelInSquare(0); yLumelInSquare < LUMELS_PER_SQUARE_EDGE; ++yLumelInSquare)
            {
               next_yInLightmap = (next_yInLightmap + 1) & LUMELS_PER_TILE_EDGE_MASK;
               U32 next_yInLightmap_offset(next_yInLightmap << LUMELS_PER_TILE_EDGE_LOG2);

               U32 xInLightmap(base_xInLightmap);
               U32 xTexelInSquare_offset = 0;

               for(U32 xLumelInSquare(0); xLumelInSquare < LUMELS_PER_SQUARE_EDGE; ++xLumelInSquare)
               {
                  U32 next_xInLightmap((xInLightmap + 1) & LUMELS_PER_TILE_EDGE_MASK);
                  U32 texelInTargetSquare_offset = yTexelInTargetSquare_offset + xTexelInSquare_offset;
                  U32 texelInSquare_offset = yTexelInSquare_offset + xTexelInSquare_offset;

                  // lumels are secret parameters to subroutines
                  lumels[0] = U32(lmap[xInLightmap | yInLightmap_offset]);
                  lumels[1] = U32(lmap[next_xInLightmap | yInLightmap_offset]);
                  lumels[2] = U32(lmap[xInLightmap | next_yInLightmap_offset]);
                  lumels[3] = U32(lmap[next_xInLightmap | next_yInLightmap_offset]);

#if defined(BLENDER_USE_ASM)
                  if ( targetTexelsPerLumel > 1 )
                  {
                     doLumelPlus1Mip( &bits0[ texelInTargetSquare_offset ], 
                        &bits1[ (yTexelInTargetSquare_offset >> 2) + (xTexelInSquare_offset >> 1) ], 
                        &bufferToLightFrom[ texelInSquare_offset ] );
                  }
                  else
                     do1x1Lumel( &bits0[ texelInTargetSquare_offset ], &bufferToLightFrom[ texelInSquare_offset ] );
#else
                  // Split the LUMELs into colors
                  U32 col[3][4];

                  U32 i;
                  for(i = 0; i < 4; i++)
                  {
                     // dc - leave this alone on mac, as seem to be have lightmaps as 5551 RGBA (pc format)
                     col[0][i] = (lumels[i])      & (0x1f << 11);
                     col[1][i] = (lumels[i] << 5)   & (0x1f << 11);
                     col[2][i] = (lumels[i] << 10)   & (0x1f << 11);
                  }

                  // One for each color component
                  U32 left_component_delta[3];
                  U32 right_component_delta[3];
                  U32 vscan_left_component[3];
                  U32 vscan_right_component[3];

                  for(i = 0; i < 3; i++)
                  {
                     left_component_delta[i] = (col[i][2] - col[i][0]) >> targetTexelsPerLumel_log2;
                     right_component_delta[i] = (col[i][3] - col[i][1]) >> targetTexelsPerLumel_log2;

                     vscan_left_component[i] = col[i][0];
                     vscan_right_component[i] = col[i][1];
                  }

                  // Now we interpolate the color shifts across the square
                  for(U32 yTexelInLumel = 0; yTexelInLumel < targetTexelsPerLumel; yTexelInLumel++)
                  {
                     U32 across_component_delta[3];
                     U32 hscan_component[3];

                     for(i = 0; i < 3; i++)
                     {
                        across_component_delta[i] = (vscan_right_component[i] - vscan_left_component[i]) >> targetTexelsPerLumel_log2;
                        hscan_component[i] = vscan_left_component[i];
                        vscan_left_component[i] += left_component_delta[i];
                        vscan_right_component[i] += right_component_delta[i];
                     }                     

                     U16 *dstbits = &bits0[ texelInTargetSquare_offset ];
                     const U8 *srcbits = (U8 *)&bufferToLightFrom[ texelInSquare_offset ];

                     for(U32 xTexelInLumel = 0; xTexelInLumel < targetTexelsPerLumel; xTexelInLumel++)
                     {
                        U16 dstcol[3];

                        for(i = 0; i < 3; i++) // Unroll this dumb loop?
                        {
                           U32 index = (hscan_component[i] >> 2) & 0x3F00;
                           dstcol[i] = alphaTable[index | srcbits[i]];
                           hscan_component[i] += across_component_delta[i];
                        }
#if SRC_IS_ABGR
                        // NOTE that on Mac, color order is flipped (ABGR1555 instead of RGBA5551), so:
                        // 1. we already reversed color order via BIG_ENDIAN indexing above, but
                        // 2. we need to change the shifts for alpha being the high bit instead of the low.
                        *dstbits++ = ((dstcol[0] & 0xf8) << 7) | ((dstcol[1] & 0xf8) << 2) | ((dstcol[2] & 0xf8) >> 3);
#else
                        *dstbits++ = ((dstcol[0] & 0xf8) << 8) | ((dstcol[1] & 0xf8) << 3) | ((dstcol[2] & 0xf8) >> 2);
#endif
                        srcbits += 4;
                     }

                     texelInTargetSquare_offset += yStrideThroughTarget;
                     texelInSquare_offset += yStrideThroughSquare;
                  }
#endif
                  xTexelInSquare_offset += xStrideAcrossLumels;
                  xInLightmap = next_xInLightmap;
               }
               yInLightmap_offset = next_yInLightmap_offset;
               yTexelInTargetSquare_offset += yStrideThroughTargetAcrossLumels;
               yTexelInSquare_offset += yStrideThroughSquareAcrossLumels;
            }
            // end of lighting.
      }
   }

#if defined(BLENDER_USE_ASM)
   if ( targetTexelsPerLumel > 1)
   {
      cheatmips( destmips[1], destmips[2], destmips[3], 64 );
      cheatmips( destmips[3], destmips[4], destmips[5], 16 );
      cheatmips4x4( destmips[5], destmips[6], destmips[7] );
   }
   else
#endif
      extrude( destmips, TEXELS_PER_TARGET_EDGE_LOG2 );
}

/// dc - note that this takes an 24bit texture and places it in 32bit space,
/// prob for better alignment/access.
void Blender::addSourceTexture( int textureIndex, const U8 **bmps )
{
   int texelsPerSquareEdge = MAX_TEXELS_PER_SQUARE_EDGE;

   for ( int mipLevel = 0; mipLevel < num_mip_levels; mipLevel++, texelsPerSquareEdge >>= 1)
   {
      U32 *dst = bmpdata[ mipLevel * num_src_bmps + textureIndex ];
      const U8 *rowPtr = bmps[ mipLevel ];
      const U32 texelStride(3);
      const U32 colStride(texelsPerSquareEdge * texelStride);
      const U32 blkStride(colStride * SQUARES_PER_MIPMAP_EDGE);
      const U32 rowStride(texelsPerSquareEdge * blkStride);

      // copy the bmp data over, changing the format so each block
      //  is contiguous. 
      for ( int row = 0; row < SQUARES_PER_MIPMAP_EDGE; row++, rowPtr += rowStride)
      {
         const U8* colPtr = rowPtr;

         for ( int col = 0; col < SQUARES_PER_MIPMAP_EDGE; col++, colPtr += colStride)
         {
            const U8 *blkPtr = colPtr;

            for ( int py = 0; py < texelsPerSquareEdge; py++, blkPtr += blkStride)
            {
               const U8* texelPtr = blkPtr;
               for ( int px = 0; px < texelsPerSquareEdge; px++, texelPtr += texelStride )
#if SRC_IS_ABGR
                  *dst++ = (U32(texelPtr[0]) << 24) | (U32(texelPtr[1]) << 16) | (U32(texelPtr[2]) << 8);
#else
                  *dst++ = texelPtr[0] | (U32(texelPtr[1]) << 8) | (U32(texelPtr[2]) << 16);
#endif
            }
         }
      }
   }
}

#define CACHE_ROUND_SHIFT   12
#define CACHE_ROUND_ADJUST  ((1 << CACHE_ROUND_SHIFT) - 1)
#define CACHE_ROUND_MASK    (~CACHE_ROUND_ADJUST)
#define DWORD_STAGGER       0

static U32 *round_to_cache_start( U32 *ptr )
{
   return ( (U32 *) ((dsize_t(ptr) + CACHE_ROUND_ADJUST) & CACHE_ROUND_MASK) );
}

Blender::Blender( int num_src, int num_mips, U8 **alphas )
{
   int bmps_size = MAX_TEXELS_PER_SQUARE;        // blending buffer (1 square)
   int mip_size = TEXELS_PER_SOURCE_BMP;
   int i, j;

   alpha_data = new U8*[num_src];
   for (i = 0; i < num_src; i++)
      alpha_data[i] = alphas[i];

   num_src_bmps = num_src;
   num_mip_levels = num_mips;

   bmpdata = new U32*[ num_src * num_mips ];

   for ( i = 0; i < num_mips; i++ )
   {
      bmps_size += (mip_size + DWORD_STAGGER) * num_src;
      mip_size >>= 2;
   }

   bmp_alloc_ptr = new U32[ bmps_size + CACHE_ROUND_ADJUST ];
   U32 *bmps = round_to_cache_start( bmp_alloc_ptr );

   // buffer that we'll be blending into, and lighting out of.
   blendbuffer = bmps;

   U32 *curbmp = blendbuffer + MAX_TEXELS_PER_SQUARE;
   int bmp_size = TEXELS_PER_SOURCE_BMP;
   int bmpnum = 0;

   // initialize pointers into buffer for source textures.
   for ( j = 0; j < num_mips; j++ )
   {
      for ( i = 0; i < num_src; i++ )
      {
         bmpdata[ bmpnum ] = curbmp;
         //      U32 *bptr = curbmp;

         curbmp += (bmp_size + DWORD_STAGGER);
         bmpnum++;
      }

      bmp_size >>= 2;
   }
}

Blender::~Blender()
{
   if ( bmp_alloc_ptr )
      delete [] bmp_alloc_ptr;

   if ( bmpdata )
      delete [] bmpdata;

   if ( alpha_data )
      delete [] alpha_data;
}
