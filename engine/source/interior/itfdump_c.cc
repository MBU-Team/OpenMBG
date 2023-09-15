//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "interior/itf.h"

//two55	   dd 0x437F0000
//alpha	   dd 0

//============================================================
void FN_CDECL processTriFan(OutputPoint*          dst,
                   const ItrPaddedPoint* srcPoints,
                   const U32*            srcIndices,
                   const U32             numIndices)
{
	U32 i, j;
	F32 x,y,z;
	for (i=0; i<numIndices; i++)
	{
		j = srcIndices[i];
		x = srcPoints[j].point.x;
		y = srcPoints[j].point.y;
		z = srcPoints[j].point.z;

		dst->point.x = x;
		dst->point.y = y;
		dst->point.z = z;
		dst->fogCoord = srcPoints[j].fogCoord;
		
		dst->texCoord.x = (texGen0[0]*x)
								+ (texGen0[1]*y)
								+ (texGen0[2]*z)
								+ (texGen0[3]);

		dst->texCoord.y = (texGen0[4]*x)
								+ (texGen0[5]*y)
								+ (texGen0[6]*z)
								+ (texGen0[7]);

		dst->lmCoord.x = (texGen1[0]*x)
								+ (texGen1[1]*y)
								+ (texGen1[2]*z)
								+ (texGen1[3]);

		dst->lmCoord.y = (texGen1[4]*x)
								+ (texGen1[5]*y)
								+ (texGen1[6]*z)
								+ (texGen1[7]);

// move to next ptr.
		dst++;
	}
}


//============================================================
void FN_CDECL processTriFanSP(OutputPoint*          dst,
                     const ItrPaddedPoint* srcPoints,
                     const U32*            srcIndices,
                     const U32             numIndices,
                     const ColorI*         srcColors)
{
	U32 i, j;
	float x,y,z;
	for (i=0; i<numIndices; i++)
	{
		j = srcIndices[i];
		x = srcPoints[j].point.x;
		y = srcPoints[j].point.y;
		z = srcPoints[j].point.z;

		dst->point.x = x;
		dst->point.y = y;
		dst->point.z = z;
		dst->fogColors = srcColors[j].getARGBEndian();

		dst->texCoord.x = (texGen0[0]*x)
								+ (texGen0[1]*y)
								+ (texGen0[2]*z)
								+ (texGen0[3]);

		dst->texCoord.y = (texGen0[4]*x)
								+ (texGen0[5]*y)
								+ (texGen0[6]*z)
								+ (texGen0[7]);
// move to next ptr.
		dst++;
	}
}


//============================================================
void FN_CDECL processTriFanVC_TF(OutputPoint*          dst,
                        const ItrPaddedPoint* srcPoints,
                        const U32*            srcIndices,
                        const U32             numIndices,
                        const ColorI*         srcColors)
{
	U32 i, j;
	float x,y,z;
	for (i=0; i<numIndices; i++)
	{
		j = srcIndices[i];
		x = srcPoints[j].point.x;
		y = srcPoints[j].point.y;
		z = srcPoints[j].point.z;

		dst->point.x = x;
		dst->point.y = y;
		dst->point.z = z;
		dst->fogColors = srcColors[j].getARGBEndian();

		// dc - I >think< I got this right...
		dst->texCoord.x = fogCoordinatePointer[j].x;
		dst->texCoord.y = fogCoordinatePointer[j].y;

		dst->lmCoord.x = (texGen0[0]*x)
							+ (texGen0[1]*y)
							+ (texGen0[2]*z)
							+ (texGen0[3]);

		dst->lmCoord.y = (texGen0[4]*x)
							+ (texGen0[5]*y)
							+ (texGen0[6]*z)
							+ (texGen0[7]);
// move to next ptr.
		dst++;
	}
}


//============================================================
void FN_CDECL processTriFanSP_FC(OutputPoint*          dst,
                        const ItrPaddedPoint* srcPoints,
                        const U32*            srcIndices,
                        const U32             numIndices,
                        const ColorI*         srcColors)
{
	U32 i, j;
	float x,y,z;
	for (i=0; i<numIndices; i++)
	{
		j = srcIndices[i];
		x = srcPoints[j].point.x;
		y = srcPoints[j].point.y;
		z = srcPoints[j].point.z;

		dst->point.x = x;
		dst->point.y = y;
		dst->point.z = z;
		dst->fogColors = srcColors[j].getARGBEndian();
		dst->lmCoord.x = srcPoints[j].fogCoord;

		dst->texCoord.x = (texGen0[0]*x)
								+ (texGen0[1]*y)
								+ (texGen0[2]*z)
								+ (texGen0[3]);

		dst->texCoord.y = (texGen0[4]*x)
								+ (texGen0[5]*y)
								+ (texGen0[6]*z)
								+ (texGen0[7]);
// move to next ptr.
		dst++;
	}
}


//============================================================
// helpers:
const float two55 = (F32)0x473F0000; // !!!!!!TBD -- not sure this is right...

#define ALPHA_CALC(a, c)											\
		a = c * two55;													\
		a = 255 - a; /* flip direction of value. */			\
		if (a < 0) a = 0;

/* ASM for previous calculation:
   fld		dword [esi + 12]
	fld		dword [two55]
	fmulp		st1, st0
	fistp		dword [alpha]
	mov		eax, 255
	sub		eax, [alpha]
	cmp		eax, 0
	jge		near procPointLp1a_fc_vb
	mov		eax, 0
procPointLp1a_fc_vb:
	shl		eax, 24 // left this in the function instead of the macro.
    mov     [edi + 16], eax             ; <- f
*/


//============================================================
void FN_CDECL processTriFanFC_VB(OutputPointFC_VB*			dst,
                        const ItrPaddedPoint*	srcPoints,
                        const U32*					srcIndices,
                        const U32					numIndices)
{
	S32 alpha;
	U32 i, j;
	float x,y,z;
	for (i=0; i<numIndices; i++)
	{
		j = srcIndices[i];
		x = srcPoints[j].point.x;
		y = srcPoints[j].point.y;
		z = srcPoints[j].point.z;

		dst->point.x = x;
		dst->point.y = y;
		dst->point.z = z;
		dst->currentColors = 0xFFFFFFFF;

		ALPHA_CALC(alpha, srcPoints[j].fogCoord);
		dst->fogColors = ((U32)alpha)<<24; // move into alpha position.

		// dc - note the texGens are used in reverse order.  that's what the ASM did...
		dst->texCoord.x = (texGen1[0]*x)
								+ (texGen1[1]*y)
								+ (texGen1[2]*z)
								+ (texGen1[3]);

		dst->texCoord.y = (texGen1[4]*x)
								+ (texGen1[5]*y)
								+ (texGen1[6]*z)
								+ (texGen1[7]);

		dst->lmCoord.x = (texGen0[0]*x)
								+ (texGen0[1]*y)
								+ (texGen0[2]*z)
								+ (texGen0[3]);

		dst->lmCoord.y = (texGen0[4]*x)
								+ (texGen0[5]*y)
								+ (texGen0[6]*z)
								+ (texGen0[7]);

// move to next ptr.
		dst++;
	}
}


//!!!!!!TBD -- is there a rotate intrinsic?????
#define ROL16(x)		(x) = ((((x)<<16)&0xFFFF0000) | (((x)>>16)&0x0000FFFF))

//============================================================
void FN_CDECL processTriFanSP_FC_VB(OutputPointSP_FC_VB*		dst,
                           const ItrPaddedPoint*	srcPoints,
                           const U32*					srcIndices,
                           const U32					numIndices,
                           const ColorI*				srcColors)
{
	S32 alpha;
	U32 i, j, tmp, tmp2;
	float x,y,z;
	for (i=0; i<numIndices; i++)
	{
		j = srcIndices[i];
		x = srcPoints[j].point.x;
		y = srcPoints[j].point.y;
		z = srcPoints[j].point.z;

		dst->point.x = x;
		dst->point.y = y;
		dst->point.z = z;

/*
   mov      edx, [srcColors]            ; color
   lea      edx, [edx + ebp*4]          ; color
   mov      edx, [edx]                  ; color
	mov		eax, edx
	mov		ebx, 0x00FF00FF
	and		edx, ebx
	not		ebx
	and		eax, ebx
	rol		edx, 16
	or			edx, eax
	mov     [edi + 12], edx             ; color
*/

		tmp = srcColors[j].getARGBEndian();
		tmp2 = tmp;
		
		tmp = (tmp & 0x00FF00FF);
		tmp2 = (tmp2 & 0xFF00FF00);
		
		ROL16(tmp);
		
		dst->lmColors = (tmp | tmp2);

		ALPHA_CALC(alpha, srcPoints[j].fogCoord);
		dst->fogColors = ((U32)alpha)<<24; // move into alpha position.

		dst->texCoord.x = (texGen0[0]*x)
								+ (texGen0[1]*y)
								+ (texGen0[2]*z)
								+ (texGen0[3]);

		dst->texCoord.y = (texGen0[4]*x)
								+ (texGen0[5]*y)
								+ (texGen0[6]*z)
								+ (texGen0[7]);
// move to next ptr.
		dst++;
	}
}

