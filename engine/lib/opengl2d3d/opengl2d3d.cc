//-----------------------------------------------------------------------------
// Torque Game Engine 
// Quake GL DirectX wrapper
//-----------------------------------------------------------------------------

#include "opengl2d3d/opengl2d3d.h"
#include <stdio.h>

#ifdef USEICECAP		  					
#include <icapexp.h>
#endif

static Globals g;
static void dirtyArray();


GLAPI BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved )
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

static void QuakeUpdateViewport()
{
    D3DVIEWPORT7 vport;
    g.m_d3ddev->GetViewport(&vport);
    if(g.m_scissoring) 
    {
        // Check whether viewport is completely within scissor rect
        RECT scirect, vwprect, xrect;
        scirect.left   = g.m_scix;
        scirect.top    = g.m_sciy;
        scirect.right  = g.m_scix + g.m_sciw;
        scirect.bottom = g.m_sciy + g.m_scih;
        vwprect.left   = g.m_vwx;
        vwprect.top    = g.m_vwy;
        vwprect.right  = g.m_vwx + g.m_vww;
        vwprect.bottom = g.m_vwy + g.m_vwh;
        if(IntersectRect(&xrect, &scirect, &vwprect))
        {
            if(EqualRect(&xrect, &vwprect))
            {
                goto updvwp;
            }
        }
        else
        {
            goto updvwp;
        }
        // BUGBUG: if the viewport overlaps the scissor rect, then none of this works
        vport.dwX = g.m_scix;
        vport.dwY = g.m_winHeight - (g.m_sciy + g.m_scih);
        vport.dwWidth = g.m_sciw;
        vport.dwHeight = g.m_scih;
        D3DVALUE dvClipX = (2.f * g.m_scix) / g.m_vww - 1.0f;
        D3DVALUE dvClipY = (2.f * (g.m_sciy + g.m_scih)) / g.m_vwh - 1.0f;
        D3DVALUE dvClipWidth = (2.f * g.m_sciw) / g.m_vww;
        D3DVALUE dvClipHeight = (2.f * g.m_scih) / g.m_vwh;
        D3DMATRIX c;
        c._11 = 2.f / dvClipWidth;
        c._21 = 0.f;
        c._31 = 0.f;
        c._41 = -1.f - 2.f * (dvClipX / dvClipWidth);
        c._12 = 0.f;
        c._22 = 2.f / dvClipHeight;
        c._32 = 0.f;
        c._42 = -1.f - 2.f * (dvClipY / dvClipHeight);
        c._13 = 0.f;
        c._23 = 0.f;
        c._33 = 1.f;
        c._43 = 0.f;
        c._14 = 0.f;
        c._24 = 0.f;
        c._34 = 0.f;
        c._44 = 1.f;
        g.m_d3ddev->MultiplyTransform(D3DTRANSFORMSTATE_PROJECTION, &c);
    }
    else
    {
updvwp:
        vport.dwX = g.m_vwx;
        vport.dwY = g.m_winHeight - (g.m_vwy + g.m_vwh);
        vport.dwWidth = g.m_vww;
        vport.dwHeight = g.m_vwh;
    }
    g.m_d3ddev->SetViewport(&vport); 
    g.m_updvwp = FALSE;
}

static void QuakeSetTexturingState()
{
	if (g.m_updvwp)
		QuakeUpdateViewport();

	if (g.m_texturing == TRUE) {
		if (g.m_texHandleValid == FALSE) {
			TexInfo &ti = g.m_tex[g.m_curstagebinding[0]];

			if (ti.m_dwStage != 0)
			{
				g.m_d3ddev->DeleteStateBlock(ti.m_block);
				g.m_d3ddev->BeginStateBlock();
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSU,ti.m_addu);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSV,ti.m_addv);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_MAGFILTER,ti.m_magmode);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_MINFILTER,ti.m_minmode);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_MIPFILTER,ti.m_mipmode);
				g.m_d3ddev->SetTexture(0, ti.m_ddsurf);
				g.m_d3ddev->EndStateBlock(&ti.m_block);
				ti.m_dwStage = 0;
				ti.m_capture = FALSE;
			}
			if (ti.m_capture)
			{
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSU,ti.m_addu);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_ADDRESSV,ti.m_addv);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_MAGFILTER,ti.m_magmode);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_MINFILTER,ti.m_minmode);
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_MIPFILTER,ti.m_mipmode);
				g.m_d3ddev->SetTexture(0, ti.m_ddsurf);
				g.m_d3ddev->CaptureStateBlock(ti.m_block);
				ti.m_capture = FALSE;
			}
			else
				g.m_d3ddev->ApplyStateBlock(ti.m_block);

			switch (g.m_blendmode[0]) {
				case GL_REPLACE:
					switch (ti.m_internalformat) {
						case GL_COLOR_INDEX8_EXT:
							if (ti.m_rgbaindexed)
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][1]);
							else
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][0]);
							break;
						case GL_RGB5:
						case GL_RGB5_A1:
						case GL_RGB:
                  case GL_RGB8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][0]);
							break;
						case GL_RGBA4:
						case GL_RGBA:
                  case GL_RGBA8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][1]);
							break;
					}
					break;
				case GL_MODULATE:
					switch (ti.m_internalformat) {
						case GL_ALPHA:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][8]);
							break;
						case GL_COLOR_INDEX8_EXT:
							if (ti.m_rgbaindexed)
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][3]);
							else
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][2]);
							break;	
						case GL_LUMINANCE:
						case GL_RGB5:
						case GL_RGB5_A1:
						case GL_RGB:
                  case GL_RGB8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][2]);
							break;
						case GL_RGBA4:
						case GL_RGBA:
                  case GL_RGBA8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][3]);
							break;
					}
					break;
				case GL_DECAL:
					switch (ti.m_internalformat) {
						case GL_COLOR_INDEX8_EXT:
							if (ti.m_rgbaindexed)
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][5]);
							else
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][4]);
							break;
						case GL_RGB5:
						case GL_RGB5_A1:
						case GL_RGB:
                  case GL_RGB8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][4]);
							break;
						case GL_RGBA4:
						case GL_RGBA:
                  case GL_RGBA8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][5]);
							break;
					}
					break;
				case GL_BLEND:
					switch (ti.m_internalformat) {
						case GL_COLOR_INDEX8_EXT:
							if (ti.m_rgbaindexed)
								 g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][7]);
							else
								 g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][6]);
							break;	
						case GL_RGB5:
						case GL_RGB5_A1:
						case GL_RGB:
                  case GL_RGB8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][6]);
							break;
						case GL_RGBA4:
						case GL_RGBA:
                  case GL_RGBA8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][7]);
							break;
					}
					break;
				case GL_COMBINE_EXT:
					switch (ti.m_internalformat) {
						case GL_COLOR_INDEX8_EXT:
						case GL_RGBA4:
						case GL_RGBA:
                  case GL_RGBA8:
							g.m_d3ddev->ApplyStateBlock(g.m_shaders[0][9]);
							break;
					}
					break;
			}
            
			if (g.m_mtex != FALSE) {
				TexInfo &ti2 = g.m_tex[g.m_curstagebinding[1]];

				if (ti2.m_dwStage != 1)
				{
					g.m_d3ddev->DeleteStateBlock(ti2.m_block);
					g.m_d3ddev->BeginStateBlock();
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSU,ti2.m_addu);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSV,ti2.m_addv);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_MAGFILTER,ti2.m_magmode);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_MINFILTER,ti2.m_minmode);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_MIPFILTER,ti2.m_mipmode);
					g.m_d3ddev->SetTexture(1, ti2.m_ddsurf);
					g.m_d3ddev->EndStateBlock(&ti2.m_block);
					ti2.m_dwStage = 1;
					ti2.m_capture = FALSE;
				}
				if (ti2.m_capture)
				{
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSU,ti2.m_addu);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_ADDRESSV,ti2.m_addv);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_MAGFILTER,ti2.m_magmode);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_MINFILTER,ti2.m_minmode);
					g.m_d3ddev->SetTextureStageState (1, D3DTSS_MIPFILTER,ti2.m_mipmode);
					g.m_d3ddev->SetTexture(1, ti2.m_ddsurf);
					g.m_d3ddev->CaptureStateBlock(ti2.m_block);
					ti2.m_capture = FALSE;
				}
				else
					g.m_d3ddev->ApplyStateBlock(ti2.m_block);

				switch (g.m_blendmode[1]) {
					case GL_REPLACE:
						switch (ti2.m_internalformat) {
							case GL_COLOR_INDEX8_EXT:
								if (ti2.m_rgbaindexed)
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][1]);
								else
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][0]);
								break;
							case GL_RGB5:
							case GL_RGB5_A1:
							case GL_RGB:
                     case GL_RGB8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][0]);
								break;
							case GL_RGBA4:
							case GL_RGBA:
                     case GL_RGBA8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][1]);
								break;
						}
						break;
					case GL_MODULATE:
						switch (ti2.m_internalformat) {
							case GL_COLOR_INDEX8_EXT:
								if (ti2.m_rgbaindexed)
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][3]);
								else
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][2]);
								break;
							case GL_RGB5:
							case GL_RGB5_A1:
							case GL_RGB:
                     case GL_RGB8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][2]);
								break;
							case GL_RGBA4:
							case GL_RGBA:
                     case GL_RGBA8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][3]);
								break;
						}
						break;
					case GL_DECAL:
						switch (ti2.m_internalformat) {
							case GL_COLOR_INDEX8_EXT:
								if (ti2.m_rgbaindexed)
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][5]);
								else
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][4]);
								break;
							case GL_RGB5:
							case GL_RGB5_A1:
							case GL_RGB:
                     case GL_RGB8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][4]);
								break;
							case GL_RGBA4:
							case GL_RGBA:
                     case GL_RGBA8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][5]);
								break;
						}
						break;
					case GL_BLEND:
						switch (ti2.m_internalformat) {
							case GL_COLOR_INDEX8_EXT:
								if (ti2.m_rgbaindexed)
									 g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][7]);
								else
									 g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][6]);
								break;
							case GL_RGB5:
							case GL_RGB5_A1:
							case GL_RGB:
                     case GL_RGB8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][6]);
								break;
							case GL_RGBA4:
							case GL_RGBA:
                     case GL_RGBA8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][7]);
								break;
						}
						break;
					case GL_COMBINE_EXT:
						switch (ti2.m_internalformat) {
							case GL_COLOR_INDEX8_EXT:
								if (ti2.m_rgbaindexed)
									if (g.m_source2rgbext == GL_CONSTANT_EXT)
										g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][8]);
									else
										g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][10]);
								else
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][9]);
								break;
							case GL_RGB5:
							case GL_RGB:
                     case GL_RGB8:
								g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][9]);
								break;
							case GL_RGBA4:
							case GL_RGBA:
                     case GL_RGBA8:
								if (g.m_source2rgbext == GL_CONSTANT_EXT)
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][8]);
								else
									g.m_d3ddev->ApplyStateBlock(g.m_shaders[1][10]);
								break;
						}
						break;
				}
			}
			g.m_texHandleValid = TRUE;
		}

		if (g.m_texgen[0] && g.m_texgenmode[0] == GL_OBJECT_LINEAR)
		{
			if (g.m_inversedirty)
			{
				D3DMATRIX world;
				float det;

				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_WORLD, &world);
				D3DXMatrixInverse((D3DXMATRIX *) &g.m_inverseworld,&det,(const D3DXMATRIX *) &world); 
			}
			if ((g.m_inversedirty || g.m_objectdirty[0]) &&
			 	 g.m_texgen[0] && g.m_texgenmode[0] == GL_OBJECT_LINEAR)
			{
				D3DMATRIX object;
				GLfloat *u = g.m_texgenplane[0][0];
				GLfloat *v = g.m_texgenplane[0][1];

				object._11 = u[0]; object._12 = v[0]; object._13 = 0; object._14 = 0;
				object._21 = u[1]; object._22 = v[1]; object._23 = 0; object._24 = 0;
				object._31 = u[2]; object._32 = v[2]; object._33 = 0; object._34 = 0;
				object._41 = u[3]; object._42 = v[3]; object._43 = 0; object._44 = 0;

				g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0,&object);
				g.m_d3ddev->MultiplyTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_curtexmatrix);
				g.m_d3ddev->MultiplyTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_inverseworld);
				if (g.m_objectdirty[0])
					g.m_objectdirty[0] = FALSE;
			}
			if (g.m_inversedirty)
				g.m_inversedirty = FALSE;
		}
    }
}

static void RawToCanon(DWORD dwFormat, DWORD dwWidth, DWORD dwHeight, const void *lpPixels, DWORD *lpdwCanon)
{
	switch (dwFormat)
	{
		case GL_ALPHA:
		case GL_LUMINANCE:
		case GL_COLOR_INDEX8_EXT:
			{
				for (int i = 0, j = dwWidth*dwHeight; i < j; ++i)
				{
					DWORD t = ((UCHAR *) lpPixels)[i];

					lpdwCanon[i] = (t << 24) | (t << 16) | (t << 8) | t;
				}
			}
			break;
		case GL_RGB5_A1:
			{
				USHORT *rgba = (USHORT *) lpPixels;

				for (int i = 0, j = dwWidth * dwHeight; i < j; ++i)
					lpdwCanon[i] = ((rgba[i] & 0xF800) >> 8) | 0x7 |
										((rgba[i] & 0x07C0) << 5) | 0x700 |
										((rgba[i] & 0x003E) << 18) | 0x70000;
			}
			break;
		case GL_RGB5:
		case GL_RGB:
			{
				UCHAR *rgb = (UCHAR *) lpPixels;

				for (int i = 0, j = dwWidth * dwHeight, k = 0; i < j; ++i, k += 3)
					lpdwCanon[i] = rgb[k] | (rgb[k+1] << 8) | (rgb[k+2] << 16);
			}
			break;
		case GL_RGBA4:
		case GL_RGBA:
			memcpy(lpdwCanon, lpPixels, dwWidth * dwHeight * sizeof(DWORD));
			break;
		default:
		{
			char buf[64];

			sprintf(buf,"Wrapper: Unsupported texture format: %x\n", dwFormat);
			OutputDebugString(buf);
		}
	}
}

static void Resize(DWORD dwWidth, DWORD dwHeight, const DWORD *lpdwCanon,
						 DWORD dwNewWidth, DWORD dwNewHeight, DWORD *lpdwNewCanon)
{
	DWORD i, j;
	double rx = (double) dwWidth / (double) dwNewWidth;
	double ry = (double) dwHeight / (double) dwNewHeight;

	for (i = 0; i < dwNewHeight; ++i)
		for (j = 0; j < dwNewWidth; ++j)
			lpdwNewCanon[i * dwNewWidth + j] = lpdwCanon[((DWORD)(i * ry)) * dwWidth + (DWORD)(j * rx)];
}

static void ALPHATo8888(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	DWORD *lpPixels = (DWORD *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (DWORD *) ((UCHAR *) lpPixels + lpddsd->lPitch)) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = lpuchar[i] << 24;
}

static void ALPHATo4444(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch)) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = (lpuchar[i] & 0xF0) << 8;
}

static void ALPHATo8(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	UCHAR *lpPixels = (UCHAR *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels += lpddsd->lPitch) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = lpuchar[i];
}

static void LUMINANCETo8888(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	DWORD *lpPixels = (DWORD *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (DWORD *) ((UCHAR *) lpPixels + lpddsd->lPitch)) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
		{
			DWORD t = lpuchar[i];
			
			lpPixels[l] = t << 24 | t << 16 | t << 8 | t;
		}
}

static void LUMINANCETo4444(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch)) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
		{
			DWORD t = lpuchar[i] & 0xF0;
			
			lpPixels[l] = t << 8 | t << 4 | t | t >> 4;
		}
}

static void LUMINANCETo8(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	UCHAR *lpPixels = (UCHAR *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels += lpddsd->lPitch) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = lpuchar[i];
}

static void RGBCOLOR_INDEXTo8(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	UCHAR *lpPixels = (UCHAR *) lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = lpPixels + lpddsd->lPitch)
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = lpuchar[i];
}

static void RGBCOLOR_INDEXTo4444(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	WORD *lpPixels = (WORD *) lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (WORD *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = g.m_rgbapalette[lpuchar[i]];
}

static void CanonToRGBCOLOR_INDEXTo4444(LPRECT lprect, const DWORD *lpdwCanon, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	WORD *lpPixels = (WORD *) lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (WORD *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = g.m_rgbapalette[lpdwCanon[i] & 0xFF];
}

static void CanonTo8(LPRECT lprect, const DWORD *lpdwCanon, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	UCHAR *lpPixels = (UCHAR *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (UCHAR *) lpPixels + lpddsd->lPitch)
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = (UCHAR) (lpdwCanon[i] & 0xFF);
}

static void RGBTo565(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, i += 3, ++l)
			lpPixels[l] = ((lpuchar[i] & 0xF8) << 8) | ((lpuchar[i+1] & 0xFC) << 3) | ((lpuchar[i+2] & 0xF8) >> 3);
}

static void RGBTo555(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, i += 3, ++l)
			lpPixels[l] = ((lpuchar[i] & 0xF8) << 7) | ((lpuchar[i+1] & 0xF8) << 2) | ((lpuchar[i+2] & 0xF8) >> 3);
}

static void CanonTo565(LPRECT lprect, const DWORD *lpdwCanon, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT*)lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
        for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
            lpPixels[l] = (USHORT) (((lpdwCanon[i] & 0xF8) << 8) | ((lpdwCanon[i] & 0xFC00) >> 5) | ((lpdwCanon[i] & 0xF80000) >> 19));
}

static void RGB5To565(LPRECT lprect, const USHORT *lpushort, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = (lpushort[i] & 0xFFC0) | 0x20 | ((lpushort[i] & 0x3E) >> 1);
}

static void CanonTo555(LPRECT lprect, const DWORD *lpdwCanon, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = (USHORT) (((lpdwCanon[i] & 0xF8) << 7) | ((lpdwCanon[i] & 0xF800) >> 6) | ((lpdwCanon[i] & 0xF80000) >> 19));
}

static void RGB5To555(LPRECT lprect, const USHORT *lpushort, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT*)lpddsd->lpSurface;
	
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = lpushort[i] >> 1;
}

static void CanonTo4444(LPRECT lprect, const DWORD *lpdwCanon, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	USHORT *lpPixels = (USHORT *) lpddsd->lpSurface;
		 
	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (USHORT *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = (USHORT) (((lpdwCanon[i] & 0xF0) << 4) | ((lpdwCanon[i] & 0xF000) >> 8) | ((lpdwCanon[i] & 0xF00000) >> 20) | ((lpdwCanon[i] & 0xF0000000) >> 16));
}

static void CanonTo8888(LPRECT lprect, const DWORD *lpdwCanon, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	DWORD *lpPixels = (DWORD*)lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (DWORD *) ((UCHAR *) lpPixels + lpddsd->lPitch))
		for (j = lprect->left, l = 0; j < lprect->right; ++j, ++i, ++l)
			lpPixels[l] = ((lpdwCanon[i] & 0xFF00FF00) | ((lpdwCanon[i] & 0xFF) << 16) | ((lpdwCanon[i] & 0xFF0000) >> 16));
}

static void RGBTo8888(LPRECT lprect, const UCHAR *lpuchar, LPDDSURFACEDESC2 lpddsd)
{
	LONG i, j, k, l;
	DWORD *lpPixels = (DWORD *) lpddsd->lpSurface;

	for (k = lprect->top, i = 0; k < lprect->bottom; ++k, lpPixels = (DWORD *) ((UCHAR *) lpPixels + lpddsd->lPitch)) 
		for (j = lprect->left, l = 0; j < lprect->right; ++j, i += 3, ++l)
			lpPixels[l] = (lpuchar[i]) << 16 | (lpuchar[i+1] << 8) | lpuchar[i+2];
}

static void LoadSurface(LPDIRECTDRAWSURFACE7 lpDDS, DWORD dwFormat,
								DWORD dwWidth, DWORD dwHeight, DWORD dwNewWidth, DWORD dwNewHeight,
								const void *pixels)
{
	DDSURFACEDESC2 ddsd;
	HRESULT ddrval;
	DWORD *lpdwCanon, *lpdwNewCanon;	
	RECT rect;
    
	/*
	 * Lock the surface so it can be filled with the texture
	 */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddrval = lpDDS->Lock(NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if (ddrval != DD_OK) {
		lpDDS->Release();
		OutputDebugString("Lock failed while loading surface\n");

		return;
	}
	SetRect(&rect, 0, 0, ddsd.dwWidth, ddsd.dwHeight);

	if (dwWidth == dwNewWidth && dwHeight == dwNewHeight)
	{
		switch (dwFormat)
		{
			case GL_ALPHA:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					ALPHATo8888(&rect,(const UCHAR *) pixels,&ddsd);
				else
					if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF00)
						ALPHATo4444(&rect,(const UCHAR *) pixels,&ddsd);
					else
						ALPHATo8(&rect,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_LUMINANCE:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					LUMINANCETo8888(&rect,(const UCHAR *) pixels,&ddsd);
				else
					if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF00)
						LUMINANCETo4444(&rect,(const UCHAR *) pixels,&ddsd);
					else
						LUMINANCETo8(&rect,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_COLOR_INDEX8_EXT:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF00)
					RGBCOLOR_INDEXTo4444(&rect,(const UCHAR *) pixels, &ddsd);
				else
					RGBCOLOR_INDEXTo8(&rect,(const UCHAR *) pixels, &ddsd);
				break;
			case GL_RGB5:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
					RGBTo565(&rect,(const UCHAR *) pixels,&ddsd);
				else
					RGBTo555(&rect,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_RGB5_A1:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
					RGB5To565(&rect,(const USHORT *) pixels,&ddsd);
				else
					RGB5To555(&rect,(const USHORT *) pixels,&ddsd);
				break;
			case GL_RGBA4:
				CanonTo4444(&rect,(const DWORD *) pixels,&ddsd);
				break;
			case GL_RGB:
      case GL_RGB8:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					RGBTo8888(&rect,(const UCHAR *) pixels,&ddsd);
				else
					if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
						RGBTo565(&rect,(const UCHAR *) pixels,&ddsd);
					else
						RGBTo555(&rect,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_RGBA:
      case GL_RGBA8:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					CanonTo8888(&rect,(const DWORD *) pixels,&ddsd);
				else
					CanonTo4444(&rect,(const DWORD *) pixels,&ddsd);
				break;
		}	
	}
	else
	{ 
		/*
		 * Convert the GL texture into a canonical format (8888),
		 * so that we can cleanly do image ops (such as resize) without 
		 * having to worry about the bit format.
		 */
		lpdwCanon = (DWORD *) malloc(dwWidth * dwHeight * sizeof(DWORD));
		RawToCanon(dwFormat, dwWidth, dwHeight, pixels, lpdwCanon);

		/* Now resize the canon image */
		lpdwNewCanon = (DWORD *) malloc(dwNewWidth * dwNewHeight * sizeof(DWORD));
		Resize(dwWidth, dwHeight, lpdwCanon, dwNewWidth, dwNewHeight, lpdwNewCanon);
		free(lpdwCanon);
    
		/* Copy  the texture into the surface */
		if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
			CanonTo8(&rect, lpdwNewCanon, &ddsd);
		else if (ddsd.ddpfPixelFormat.dwRGBAlphaBitMask == 0xF000)
			if (dwFormat == GL_COLOR_INDEX8_EXT)
				CanonToRGBCOLOR_INDEXTo4444(&rect, lpdwNewCanon, &ddsd);
			else
				CanonTo4444(&rect, lpdwNewCanon, &ddsd);
		else if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
			CanonTo565(&rect, lpdwNewCanon, &ddsd);
		else if (ddsd.ddpfPixelFormat.dwRBitMask == 0x7C00)
			CanonTo555(&rect, lpdwNewCanon, &ddsd);
		else
			CanonTo8888(&rect, lpdwNewCanon, &ddsd);

		free(lpdwNewCanon);
	}

	/*
	 * unlock the surface
	 */
	lpDDS->Unlock(NULL);
}

static HRESULT LoadSubSurface(LPDIRECTDRAWSURFACE7 lpDDS, DWORD dwFormat,
										DWORD dwWidth, DWORD dwHeight, const void *pixels, 
										LPRECT lpsubimage)
{
	DDSURFACEDESC2 ddsd;
	HRESULT ddrval;
	DWORD *lpdwCanon, *lpdwNewCanon;	
	DWORD dwNewWidth=lpsubimage->right-lpsubimage->left;
	DWORD dwNewHeight=lpsubimage->bottom-lpsubimage->top;
	/*
	 * Lock the surface so it can be filled with the texture
	 */
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddrval = lpDDS->Lock(lpsubimage, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, NULL);
	if (ddrval != DD_OK) {
		lpDDS->Release();
		OutputDebugString("Lock failed while loading surface\n");

		return ddrval;
	}

	if (dwWidth == dwNewWidth && dwHeight == dwNewHeight)
	{
		switch (dwFormat)
		{
			case GL_ALPHA:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					ALPHATo8888(lpsubimage,(const UCHAR *) pixels,&ddsd);
				else
					if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF00)
						ALPHATo4444(lpsubimage,(const UCHAR *) pixels,&ddsd);
					else
						ALPHATo8(lpsubimage,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_LUMINANCE:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					LUMINANCETo8888(lpsubimage,(const UCHAR *) pixels,&ddsd);
				else
					if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF00)
						LUMINANCETo4444(lpsubimage,(const UCHAR *) pixels,&ddsd);
					else
						LUMINANCETo8(lpsubimage,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_COLOR_INDEX8_EXT:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF00)
					RGBCOLOR_INDEXTo4444(lpsubimage,(const UCHAR *) pixels, &ddsd);
				else
					RGBCOLOR_INDEXTo8(lpsubimage,(const UCHAR *) pixels, &ddsd);
				break;
			case GL_RGB5:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
					RGBTo565(lpsubimage,(const UCHAR *) pixels,&ddsd);
				else
					RGBTo555(lpsubimage,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_RGB5_A1:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
					RGB5To565(lpsubimage,(const USHORT *) pixels,&ddsd);
				else
					RGB5To555(lpsubimage,(const USHORT *) pixels,&ddsd);
				break;
			case GL_RGBA4:
				CanonTo4444(lpsubimage,(const DWORD *) pixels,&ddsd);
				break;
			case GL_RGB:
      case GL_RGB8:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					RGBTo8888(lpsubimage,(const UCHAR *) pixels,&ddsd);
				else
					if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
						RGBTo565(lpsubimage,(const UCHAR *) pixels,&ddsd);
					else
						RGBTo555(lpsubimage,(const UCHAR *) pixels,&ddsd);
				break;
			case GL_RGBA:
      case GL_RGBA8:
				if (ddsd.ddpfPixelFormat.dwRBitMask == 0xFF0000)
					CanonTo8888(lpsubimage,(const DWORD *) pixels,&ddsd);
				else
					CanonTo4444(lpsubimage,(const DWORD *) pixels,&ddsd);
				break;
		}	
	}
	else
	{
		/*
		 * Convert the GL texture into a canonical format (8888),
		 * so that we can cleanly do image ops (such as resize) without 
		 * having to worry about the bit format.
		 */
		lpdwCanon = (DWORD *) malloc(dwWidth * dwHeight * sizeof(DWORD));
		RawToCanon(dwFormat, dwWidth, dwHeight, pixels, lpdwCanon);

		/* Now resize the canon image */
		lpdwNewCanon = (DWORD *) malloc(dwWidth * dwHeight * sizeof(DWORD));
		Resize(dwWidth, dwHeight, lpdwCanon, dwNewWidth, dwNewHeight, lpdwNewCanon);
		free(lpdwCanon);        

		/* Copy the texture into the surface */
		if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
			CanonTo8(lpsubimage,lpdwNewCanon,&ddsd);
		else if (ddsd.ddpfPixelFormat.dwRGBAlphaBitMask == 0xF000)
			if (dwFormat == GL_COLOR_INDEX8_EXT)
				CanonToRGBCOLOR_INDEXTo4444(lpsubimage,lpdwNewCanon,&ddsd);
			else
				CanonTo4444(lpsubimage,lpdwNewCanon,&ddsd);
		else if (ddsd.ddpfPixelFormat.dwRBitMask == 0xF800)
			CanonTo565(lpsubimage,lpdwNewCanon,&ddsd);
		else if (ddsd.ddpfPixelFormat.dwRBitMask == 0x7C00)
			CanonTo555(lpsubimage, lpdwNewCanon, &ddsd);
		else
			CanonTo8888(lpsubimage, lpdwNewCanon, &ddsd);

		free(lpdwNewCanon);
	}
	/*
	 * unlock the surface
	 */
	lpDDS->Unlock(NULL);

	return DD_OK;
}

///////////////////////////// BEGIN API ENTRIES ///////////////////////////////////////////////////

GLAPI void GLAPIENTRY glActiveTextureARB(GLenum texture)
{
    g.m_curtgt = texture == GL_TEXTURE0_ARB ? 0 : 1;
}

GLAPI void GLAPIENTRY glAlphaFunc (GLenum func, GLclampf ref)
{
    int funcvalue = -1;
    switch(func) {
    case GL_NEVER:
        funcvalue=D3DCMP_NEVER;
        break;
    case GL_LESS: 
        funcvalue=D3DCMP_LESS;
        break;
    case GL_EQUAL: 
        funcvalue=D3DCMP_EQUAL;
        break;
    case GL_LEQUAL: 
        funcvalue=D3DCMP_LESSEQUAL;
        break;
    case GL_GREATER: 
        funcvalue=D3DCMP_GREATER;
        break;
    case GL_NOTEQUAL: 
        funcvalue=D3DCMP_NOTEQUAL;
        break;
    case GL_GEQUAL: 
        funcvalue=D3DCMP_GREATEREQUAL;
        break;
    case GL_ALWAYS: 
        funcvalue=D3DCMP_ALWAYS;
        break;
    }
    if (funcvalue >= 0) {
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, funcvalue);
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ALPHAREF, (D3DFIXED)(ref * 255.f));
    }
}

GLAPI void GLAPIENTRY glArrayElement (GLint i)
{ 
	if (g.m_usetexcoordary[0])
	{
		GLfloat *start0 = (GLfloat *)	&((unsigned char *) g.m_texcoordary[0])[i*g.m_texcoordstride[0]];	

		g.m_tu = start0[0];
		g.m_tv = start0[1];
	}
	if (g.m_usetexcoordary[1])
	{
		GLfloat *start1 = (GLfloat *)	&((unsigned char *) g.m_texcoordary[1])[i*g.m_texcoordstride[1]];

		g.m_tu2 = start1[0];
		g.m_tv2 = start1[1];
	}
	if (g.m_usecolorary)
		if (g.m_colortype == GL_UNSIGNED_BYTE)
		{
#ifdef _X86_
			_asm
			{
				mov ebx, g.m_colorary;
				mov eax, i;
				mov edx, g.m_colorstride;
				mul edx;
				cld;
				mov esi, eax;
				mov edx, 0x00FF00FF;
				mov eax, [ebx + esi];
				mov ecx, eax;
				and eax, edx;
				not edx;
				rol eax, 16;
				and ecx, edx;
				or  eax, ecx;
				mov g.m_color, eax;
			}
#else
			GLubyte *start = &((GLubyte *) g.m_colorary)[i*g.m_colorstride];

			g.m_color = RGBA_MAKE(start[0],start[1],start[2],start[3]);
#endif
		}
		else
		{
			GLfloat *start = (GLfloat *) &((unsigned char *) g.m_colorary)[i*g.m_colorstride];
			static float two55 = 255.f;
			unsigned int R, G, B, A;

#ifdef _X86_
			__asm {
				mov ebx, start;
				fld [ebx];
				fld [ebx + 4];
				fld [ebx + 8];
				fld [ebx + 12];
				fld two55;
				fmul st(1), st(0);
				fmul st(2), st(0);
				fmul st(3), st(0);
				fmulp st(4), st(0);
				fistp A;
				fistp B;
				fistp G;
				fistp R;
				mov edx, A;
				cmp edx, 255;
				jle pt1;
				mov edx, 255;
		pt1:	mov eax, B;
				cmp eax, 255;
				jle pt2;
				mov eax, 255;
		pt2:	mov ebx, G;
				cmp ebx, 255;
				jle pt3;
				mov ebx, 255;
		pt3:	mov ecx, R;
				cmp ecx, 255;
				jle pt4;
				mov ecx, 255;
		pt4:	shl ebx, 8;
				shl ecx, 16;
				shl edx, 24;
				or eax, ebx;
				or ecx, edx;
				or eax, ecx;
				mov g.m_color, eax;
			}
#else
			R = (unsigned int)(start[0] * two55);
			G = (unsigned int)(start[1] * two55);
			B = (unsigned int)(start[2] * two55);
			A = (unsigned int)(start[3] * two55);
			if (R > 255)
				R = 255;
			if (G > 255)
				G = 255;
			if (B > 255)
				B = 255;
			if (A > 255)
				A = 255;
			g.m_color = RGBA_MAKE(R, G, B, A);
#endif
		}
	if (g.m_usenormalary)
	{
		unsigned char *start = &((unsigned char *) g.m_normalary)[i*g.m_normalstride];

		memcpy(&g.m_nx,start,sizeof(D3DVALUE)*3);
	}
	if (g.m_nfv[g.m_comp] + g.m_vcnt[g.m_comp] >= (VBUFSIZE - MAXVERTSPERPRIM))
	{
		if (g.m_prim == GL_TRIANGLES)
		{
			if (g.m_vcnt[g.m_comp] % 3 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
		else if (g.m_prim == GL_QUADS)
		{
			if (g.m_vcnt[g.m_comp] % 4 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
		else if(g.m_prim == GL_LINES)
		{
			if (g.m_vcnt[g.m_comp] % 2 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
	}

	if (g.m_vertarysize == 2)
	{
#ifdef _X86_
		_asm
		{   
			mov     eax, i;
			mov	  edx, g.m_vertexstride;
			mul	  edx;
			cld;
			mov     esi, g.m_vertexary;
			mov     ecx, g.m_comp;
			mov     edi, g.m_verts;
			add     esi, eax;
			mov     eax, g.m_vcnt[ecx * 4];
			lea     edx, [eax + 1];
			mov     g.m_vcnt[ecx * 4], edx;
			lea     edx, [ecx * 8 + 28];
			mul     edx;
			cld;
			add     edi, eax;
			movsd;
			movsd;
			mov	  eax, 0;
			movsd;
			lea     esi, g.m_nx;
			lea     ecx, [ecx * 2 + 4];
			rep     movsd;
   	 }
#else
		D3DVALUE *d3dv = &(((D3DVALUE*)g.m_verts)[g.m_vcnt[g.m_comp]++ * (g.m_comp * 2 + 7)]);
		GLfloat *v = (GLfloat *) &((unsigned char *) g.m_vertexary)[i * g.m_vertexstride];
		
		*(d3dv++) = v[0];
		*(d3dv++) = v[1];
		*(d3dv++) = 0.0;
		memcpy(d3dv, &g.m_nx, sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + g.m_comp*sizeof(D3DVALUE)*2);
#endif
	}
	else
	{
#ifdef _X86_
		_asm
		{   
			mov     eax, i;
			mov	  edx, g.m_vertexstride;
			mul	  edx;
			cld;
			mov     esi, g.m_vertexary;
			mov     ecx, g.m_comp;
			mov     edi, g.m_verts;
			add     esi, eax;
			mov     eax, g.m_vcnt[ecx * 4];
			lea     edx, [eax + 1];
			mov     g.m_vcnt[ecx * 4], edx;
			lea     edx, [ecx * 8 + 28];
			mul     edx;
			cld;
			add     edi, eax;
			movsd;
			movsd;
			movsd;
			lea     esi, g.m_nx;
			lea     ecx, [ecx * 2 + 4];
			rep     movsd;
   	 }
#else
		D3DVALUE *d3dv = &(((D3DVALUE*)g.m_verts)[g.m_vcnt[g.m_comp]++ * (g.m_comp * 2 + 7)]);
		GLfloat *v = (GLfloat *) &((unsigned char *) g.m_vertexary)[i * g.m_vertexstride];
		
		*(d3dv++) = v[0];
		*(d3dv++) = v[1];
		*(d3dv++) = v[2];
		memcpy(d3dv, &g.m_nx, sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + g.m_comp*sizeof(D3DVALUE)*2);
#endif
	}
}

GLAPI void GLAPIENTRY glBegin (GLenum mode)
{    
    g.m_prim = mode;
    if(g.m_texturing) 
    {
        if(g.m_mtex)
        {
            g.m_comp = 2;
            g.m_vcnt[2] = 0;
            if(g.m_nfv[2] > (VBUFSIZE - MAXVERTSPERPRIM)) // check if space available
            {
                g.m_mtvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
                g.m_nfv[2] = 0;
            }
            else
            {
                g.m_mtvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
                g.m_verts = &(((QuakeMTVertex*)g.m_verts)[g.m_nfv[2]]);
            }
        }
        else
        {
            g.m_comp = 1;
            g.m_vcnt[1] = 0;
            if(g.m_nfv[1] > (VBUFSIZE - MAXVERTSPERPRIM)) // check if space available
            {
                g.m_tvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
                g.m_nfv[1] = 0;
            }
            else
            {
                g.m_tvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
                g.m_verts = &(((QuakeTVertex*)g.m_verts)[g.m_nfv[1]]);
            }
        }
    }
    else
    {
        g.m_comp = 0;
        g.m_vcnt[0] = 0;
        if(g.m_nfv[0] > (VBUFSIZE - MAXVERTSPERPRIM)) // check if space available
        {
            g.m_vbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
            g.m_nfv[0] = 0;
        }
        else
        {
            g.m_vbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
            g.m_verts = &(((QuakeVertex*)g.m_verts)[g.m_nfv[0]]);
        }
    }    
	QuakeSetTexturingState();
}

GLAPI void GLAPIENTRY glBindTexture (GLenum target, GLuint texture)
{
	target;

	g.m_curstagebinding[g.m_curtgt] = texture;
	g.m_texHandleValid = FALSE;
}

GLAPI void GLAPIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor)
{
    int svalue = -1, dvalue = -1;
    
    switch(sfactor) {
    case GL_ZERO:
        svalue=D3DBLEND_ZERO;
        break;
    case GL_ONE:
        svalue=D3DBLEND_ONE;
        break;
    case GL_DST_COLOR:
        svalue=D3DBLEND_DESTCOLOR;
        break;
    case GL_ONE_MINUS_DST_COLOR:
        svalue=D3DBLEND_INVDESTCOLOR;
        break;
    case GL_SRC_ALPHA:
        svalue=D3DBLEND_SRCALPHA;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        svalue=D3DBLEND_INVSRCALPHA;
        break;
    case GL_DST_ALPHA:
        svalue=D3DBLEND_DESTALPHA;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        svalue=D3DBLEND_INVDESTALPHA;
        break;
    case GL_SRC_ALPHA_SATURATE:
        svalue=D3DBLEND_SRCALPHASAT;
        break;
    }
    switch(dfactor) {
    case GL_ZERO:
        dvalue=D3DBLEND_ZERO;
        break;
    case GL_ONE:
        dvalue=D3DBLEND_ONE;
        break;
    case GL_SRC_COLOR:
        dvalue=D3DBLEND_SRCCOLOR;
        break;
    case GL_ONE_MINUS_SRC_COLOR:
        dvalue=D3DBLEND_INVSRCCOLOR;
        break;
    case GL_SRC_ALPHA:
        dvalue=D3DBLEND_SRCALPHA;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        dvalue=D3DBLEND_INVSRCALPHA;
        break;
    case GL_DST_ALPHA:
        dvalue=D3DBLEND_DESTALPHA;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        dvalue=D3DBLEND_INVDESTALPHA;
        break;
    }

    if (svalue >= 0) g.m_d3ddev->SetRenderState(D3DRENDERSTATE_SRCBLEND, (DWORD)svalue);
    if (dvalue >= 0) g.m_d3ddev->SetRenderState(D3DRENDERSTATE_DESTBLEND, (DWORD)dvalue);
    
}

GLAPI void GLAPIENTRY glClear (GLbitfield mask)
{
	DWORD flags = 0;

	if (mask & GL_COLOR_BUFFER_BIT)
		flags |= D3DCLEAR_TARGET;
	if (mask & GL_DEPTH_BUFFER_BIT)
		flags |= D3DCLEAR_ZBUFFER;
	if (mask & GL_STENCIL_BUFFER_BIT)
		flags |= D3DCLEAR_STENCIL;

	if (g.m_updvwp)
		QuakeUpdateViewport();

	g.m_d3ddev->Clear(0, NULL, flags, g.m_clearColor, (D3DVALUE) g.m_clearDepth, 0);
}

GLAPI void GLAPIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    static float two55 = 255.f;
    unsigned int R, G, B, A;
#ifdef _X86_
    __asm {
        fld red;
        fld green;
        fld blue;
        fld alpha;
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmul st(3), st(0);
        fmulp st(4), st(0);
        fistp A;
        fistp B;
        fistp G;
        fistp R;
        mov edx, A;
        cmp edx, 255;
        jle pt1;
        mov edx, 255;
pt1:    mov eax, B;
        cmp eax, 255;
        jle pt2;
        mov eax, 255;
pt2:    mov ebx, G;
        cmp ebx, 255;
        jle pt3;
        mov ebx, 255;
pt3:    mov ecx, R;
        cmp ecx, 255;
        jle pt4;
        mov ecx, 255;
pt4:    shl ebx, 8;
        shl ecx, 16;
        shl edx, 24;
        or eax, ebx;
        or ecx, edx;
        or eax, ecx;
        mov g.m_clearColor, eax;
    }
#else
    R = (unsigned int)(red * two55);
    G = (unsigned int)(green * two55);
    B = (unsigned int)(blue * two55);
    A = (unsigned int)(alpha * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    if(A > 255)
        A = 255;
    g.m_clearColor = RGBA_MAKE(R, G, B, A);
#endif
}

GLAPI void GLAPIENTRY glClearDepth (GLclampd depth)
{ 
    g.m_clearDepth = depth;
}

GLAPI void GLAPIENTRY glClientActiveTextureARB(GLenum texture)
{
    g.m_client_active_texture_arb = texture == GL_TEXTURE0_ARB ? 0 : 1;
}

GLAPI void GLAPIENTRY glClipPlane (GLenum plane, const GLdouble *equation)
{
	plane;
	equation;
}

GLAPI void GLAPIENTRY glColor3f (GLfloat red, GLfloat green, GLfloat blue)
{
    static float two55 = 255.f;
    unsigned int R, G, B;
#ifdef _X86_
    __asm {
        fld red;
        fld green;
        fld blue;
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmulp st(3), st(0);
        fistp B;
        fistp G;
        fistp R;
        mov eax, B;
        cmp eax, 255;
        jle pt1;
        mov eax, 255;
pt1:    mov ebx, G;
        cmp ebx, 255;
        jle pt2;
        mov ebx, 255;
pt2:    mov ecx, R;
        cmp ecx, 255;
        jle pt3;
        mov ecx, 255;
pt3:    shl ebx, 8;
        shl ecx, 16;
        or eax, ebx;
        or ecx, 0xFF000000;
        or eax, ecx;
        mov g.m_color, eax;
    }
#else
    R = (unsigned int)(red * two55);
    G = (unsigned int)(green * two55);
    B = (unsigned int)(blue * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    g.m_color = RGBA_MAKE(R, G, B, 255);
#endif
	dirtyArray();
}

GLAPI void GLAPIENTRY glColor3ub (GLubyte red, GLubyte green, GLubyte blue)
{
	g.m_color = RGBA_MAKE(red, green, blue, 255);
	dirtyArray();
}

GLAPI void GLAPIENTRY glColor3ubv (const GLubyte *v)
{
	g.m_color = RGBA_MAKE(v[0], v[1], v[2], 255);
	dirtyArray();
}

GLAPI void GLAPIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{ 
	g.m_color = RGBA_MAKE(red, green, blue, alpha);
	dirtyArray();
}

GLAPI void GLAPIENTRY glColor4ubv (const GLubyte *v)
{
#ifdef _X86_
    _asm
    {
        mov ebx, v;
        mov edx, 0x00FF00FF;
        mov eax, [ebx];
        mov ecx, eax;
        and eax, edx;
        not edx;
        rol eax, 16;
        and ecx, edx;
        or  eax, ecx;
        mov g.m_color, eax;
    }
#else
    g.m_color = RGBA_MAKE(v[0], v[1], v[2], v[3]);
#endif
	dirtyArray();
}

GLAPI void GLAPIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    static float two55 = 255.f;
    unsigned int R, G, B, A;
#ifdef _X86_
    __asm {
        fld red;
        fld green;
        fld blue;
        fld alpha;
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmul st(3), st(0);
        fmulp st(4), st(0);
        fistp A;
        fistp B;
        fistp G;
        fistp R;
        mov edx, A;
        cmp edx, 255;
        jle pt1;
        mov edx, 255;
pt1:    mov eax, B;
        cmp eax, 255;
        jle pt2;
        mov eax, 255;
pt2:    mov ebx, G;
        cmp ebx, 255;
        jle pt3;
        mov ebx, 255;
pt3:    mov ecx, R;
        cmp ecx, 255;
        jle pt4;
        mov ecx, 255;
pt4:    shl ebx, 8;
        shl ecx, 16;
        shl edx, 24;
        or eax, ebx;
        or ecx, edx;
        or eax, ecx;
        mov g.m_color, eax;
    }
#else
    R = (unsigned int)(red * two55);
    G = (unsigned int)(green * two55);
    B = (unsigned int)(blue * two55);
    A = (unsigned int)(alpha * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    if(A > 255)
        A = 255;
    g.m_color = RGBA_MAKE(R, G, B, A);
#endif
	dirtyArray();
}

GLAPI void GLAPIENTRY glColor4fv (const GLfloat *v)
{
    static float two55 = 255.f;
    unsigned int R, G, B, A;

#ifdef _X86_
    __asm {
        mov ebx, v;
        fld [ebx];
        fld [ebx + 4];
        fld [ebx + 8];
        fld [ebx + 12];
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmul st(3), st(0);
        fmulp st(4), st(0);
        fistp A;
        fistp B;
        fistp G;
        fistp R;
        mov edx, A;
        cmp edx, 255;
        jle pt1;
        mov edx, 255;
pt1:    mov eax, B;
        cmp eax, 255;
        jle pt2;
        mov eax, 255;
pt2:    mov ebx, G;
        cmp ebx, 255;
        jle pt3;
        mov ebx, 255;
pt3:    mov ecx, R;
        cmp ecx, 255;
        jle pt4;
        mov ecx, 255;
pt4:    shl ebx, 8;
        shl ecx, 16;
        shl edx, 24;
        or eax, ebx;
        or ecx, edx;
        or eax, ecx;
        mov g.m_color, eax;
    }
#else
    R = (unsigned int)(v[0] * two55);
    G = (unsigned int)(v[1] * two55);
    B = (unsigned int)(v[2] * two55);
    A = (unsigned int)(v[3] * two55);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    if(A > 255)
        A = 255;
    g.m_color = RGBA_MAKE(R, G, B, A);
#endif
	dirtyArray();
}

GLAPI void GLAPIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if (size == 4 && (type == GL_UNSIGNED_BYTE || type == GL_FLOAT))
	{
		if (stride)
			g.m_colorstride = stride;
		else
			g.m_colorstride = (type == GL_UNSIGNED_BYTE) ? 4 : 16;
		g.m_colortype = type;
		g.m_colorary = pointer;
	}
	else
		OutputDebugString("Color array not supported\n");
}

GLAPI void GLAPIENTRY glCullFace (GLenum mode)
{
    g.m_cullMode = mode;
    if(g.m_cullEnabled == TRUE){
        DWORD statevalue;
        if ((mode == GL_BACK && g.m_frontFace == GL_CCW) ||
				(mode == GL_FRONT && g.m_frontFace == GL_CW))
            statevalue=D3DCULL_CW;
        else
            statevalue=D3DCULL_CCW;
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_CULLMODE, statevalue);
    }
}

GLAPI void GLAPIENTRY glGenTextures (GLsizei n, GLuint *textures)
{
	GLListManip<GLuint> freeTextures(&g.m_freeTextures);

	for (int i = 0; i < n; ++i)
	{
		textures[i] = freeTextures();
		freeTextures.remove();
	}
}

GLAPI void GLAPIENTRY glDeleteTextures (GLsizei n, const GLuint *textures)
{
	GLListManip<GLuint> freeTextures(&g.m_freeTextures);

	for(int i = 0; i < n; ++i) {
		TexInfo &ti = g.m_tex[textures[i]];

		if(ti.m_ddsurf != 0) {
			ti.m_ddsurf->Release();
			ti.m_ddsurf = 0;
		}
		if(ti.m_block != 0)
		{
			g.m_d3ddev->DeleteStateBlock(ti.m_block);
			ti.m_block = 0;
		}
		ti.m_capture = FALSE;
		ti.m_dwStage = 0;
		ti.m_minmode = D3DTFN_POINT;
		ti.m_magmode = D3DTFG_LINEAR;
		ti.m_mipmode = D3DTFP_LINEAR;
		ti.m_addu = D3DTADDRESS_WRAP;
		ti.m_addv = D3DTADDRESS_WRAP;
		ti.m_rgbaindexed = FALSE;
		if (ti.m_palette)
		{
			ti.m_palette->Release();
			ti.m_palette = 0;
		}

		freeTextures.insert(textures[i]);
    }
}

GLAPI void GLAPIENTRY glDepthFunc (GLenum func)
{
    int state = -1;
    switch(func) {
    case GL_NEVER:
        state=D3DCMP_NEVER;
        break;
    case GL_LESS: 
        state=D3DCMP_LESS;
        break;
    case GL_EQUAL: 
        state=D3DCMP_EQUAL;
        break;
    case GL_LEQUAL: 
        state=D3DCMP_LESSEQUAL;
        break;
    case GL_GREATER: 
        state=D3DCMP_GREATER;
        break;
    case GL_NOTEQUAL: 
        state=D3DCMP_NOTEQUAL;
        break;
    case GL_GEQUAL: 
        state=D3DCMP_GREATEREQUAL;
        break;
    case GL_ALWAYS: 
        state=D3DCMP_ALWAYS;
        break;
    }
    if(state >= 0)
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZFUNC, state);
}

GLAPI void GLAPIENTRY glDepthMask (GLboolean flag)
{
    if(flag == 0)
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
    else
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
}

GLAPI void GLAPIENTRY glDepthRange (GLclampd zNear, GLclampd zFar)
{
    if(zFar == zNear)
    {
        zFar  += .001;
        zNear -= .001;
    }
    if(zNear < 0.)
        zNear = 0.;
    if(zNear > 1.)
        zNear = 1.;
    if(zFar < 0.)
        zFar = 0.;
    if(zFar > 1.)
        zFar = 1.;
    D3DVIEWPORT7 vport;
    g.m_d3ddev->GetViewport(&vport);
    vport.dvMinZ = (D3DVALUE)zNear;
    vport.dvMaxZ = (D3DVALUE)zFar;
    g.m_d3ddev->SetViewport(&vport);
}

GLAPI void GLAPIENTRY glEnd (void)
{
	if (g.m_texturing)
	{
        if (g.m_mtex)
        {
			if (g.m_vcnt[2] && g.m_prim == GL_LINE_LOOP)
			{
				D3DVALUE *d3dv = (D3DVALUE *) g.m_verts;

				memcpy(&g.m_nx,&d3dv[3],sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + sizeof(D3DVALUE)*4);
				glVertex3fv(d3dv);
			}
				g.m_mtvbuf->Unlock();
            if(g.m_vcnt[2] == 0)
                return;
            unsigned i;
            switch(g.m_prim) 
            {
            case GL_LINES:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_mtvbuf, g.m_nfv[2], g.m_vcnt[2], 0);
                g.m_nfv[2] += g.m_vcnt[2];
                break;
				case GL_LINE_LOOP:
				case GL_LINE_STRIP:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINESTRIP, g.m_mtvbuf, g.m_nfv[2], g.m_vcnt[2], 0);
					g.m_nfv[2] += g.m_vcnt[2];
					break;
				case GL_TRIANGLES:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_mtvbuf, g.m_nfv[2], g.m_vcnt[2], 0);
                g.m_nfv[2] += g.m_vcnt[2];
                break;
            case GL_TRIANGLE_STRIP:
				case GL_QUAD_STRIP:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_mtvbuf, g.m_nfv[2], g.m_vcnt[2], 0);
                g.m_nfv[2] += g.m_vcnt[2];
                break;
            case GL_POLYGON:
            case GL_TRIANGLE_FAN:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvbuf, g.m_nfv[2], g.m_vcnt[2], 0);
                g.m_nfv[2] += g.m_vcnt[2];
                break;
            case GL_QUADS:
                for(i = 0; i < g.m_vcnt[2]; i += 4) 
                {
                    g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvbuf, g.m_nfv[2], 4, 0);
                    g.m_nfv[2] += 4;
                }
                break;
            default:
					{
               	char buf[64];
               	
               	sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",g.m_prim);
               	OutputDebugString(buf);
					}
            }
        }
        else
        {
			if (g.m_vcnt[1] && g.m_prim == GL_LINE_LOOP)
			{
				D3DVALUE *d3dv = (D3DVALUE *) g.m_verts;

				memcpy(&g.m_nx,&d3dv[3],sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + sizeof(D3DVALUE)*2);	
				glVertex3fv(d3dv);
			}
				g.m_tvbuf->Unlock();
            if(g.m_vcnt[1] == 0)
                return;
            unsigned i;
            switch(g.m_prim) 
            {
            case GL_LINES:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_tvbuf, g.m_nfv[1], g.m_vcnt[1], 0);
                g.m_nfv[1] += g.m_vcnt[1];
                break;
				case GL_LINE_LOOP:
				case GL_LINE_STRIP:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINESTRIP, g.m_tvbuf, g.m_nfv[1], g.m_vcnt[1], 0);
					g.m_nfv[1] += g.m_vcnt[1];
					break;
            case GL_TRIANGLES:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_tvbuf, g.m_nfv[1], g.m_vcnt[1], 0);
                g.m_nfv[1] += g.m_vcnt[1];
                break;
            case GL_TRIANGLE_STRIP:
            case GL_QUAD_STRIP:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_tvbuf, g.m_nfv[1], g.m_vcnt[1], 0);
                g.m_nfv[1] += g.m_vcnt[1];
                break;
            case GL_POLYGON:
            case GL_TRIANGLE_FAN:
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvbuf, g.m_nfv[1], g.m_vcnt[1], 0);
                g.m_nfv[1] += g.m_vcnt[1];
                break;
            case GL_QUADS:
                for(i = 0; i < g.m_vcnt[1]; i += 4) 
                {
                    g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvbuf, g.m_nfv[1], 4, 0);
                    g.m_nfv[1] += 4;
                }
                break;
				default:
                {
               	char buf[64];
               	
               	sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",g.m_prim);
               	OutputDebugString(buf);
					}
            }
        }
    }
    else
    {
		if (g.m_vcnt[0] && g.m_prim == GL_LINE_LOOP)
		{
			D3DVALUE *d3dv = (D3DVALUE *) g.m_verts;

			memcpy(&g.m_nx,&d3dv[3],sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR));
			glVertex3fv(d3dv);
		}
		  g.m_vbuf->Unlock();
        if(g.m_vcnt[0] == 0)
            return;
        unsigned i;
        switch(g.m_prim) 
        {
        case GL_LINES:
            g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_vbuf, g.m_nfv[0], g.m_vcnt[0], 0);
            g.m_nfv[0] += g.m_vcnt[0];
            break;
		  case GL_LINE_LOOP:
		  case GL_LINE_STRIP:
				g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINESTRIP, g.m_vbuf, g.m_nfv[0], g.m_vcnt[0], 0);
				g.m_nfv[0] += g.m_vcnt[0];
				break;
        case GL_TRIANGLES:
            g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_vbuf, g.m_nfv[0], g.m_vcnt[0], 0);
            g.m_nfv[0] += g.m_vcnt[0];
            break;
        case GL_TRIANGLE_STRIP:
		  case GL_QUAD_STRIP:
            g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_vbuf, g.m_nfv[0], g.m_vcnt[0], 0);
            g.m_nfv[0] += g.m_vcnt[0];
            break;
        case GL_POLYGON:
        case GL_TRIANGLE_FAN:
            g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vbuf, g.m_nfv[0], g.m_vcnt[0], 0);
            g.m_nfv[0] += g.m_vcnt[0];
            break;
        case GL_QUADS:
            for(i = 0; i < g.m_vcnt[0]; i += 4) 
            {
                g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vbuf, g.m_nfv[0], 4, 0);
                g.m_nfv[0] += 4;
            }
            break;
        default:
            {
            	char buf[64];
               	
               sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",g.m_prim);
               OutputDebugString(buf);
				}
        }
    }
}

static void processVB(unsigned vcount)
{
	DWORD vop = D3DVOP_CLIP | D3DVOP_TRANSFORM;

	if (g.m_lighting)
		vop |= D3DVOP_LIGHT;

	if (g.m_useVBuffer != -1)
	{
		BufferInfo &binfo = g.m_vbuffers[g.m_useVBuffer];

		g.m_mtvtgt->ProcessVertices(vop,0,vcount,binfo.buf,binfo.nfv,g.m_d3ddev,0);
	}
	else if (g.m_usefogary)
	{
		g.m_fmtvbuf->Unlock();
		g.m_fmtvtgt->ProcessVertices(vop,0,vcount,g.m_fmtvbuf,g.m_nfv[3],g.m_d3ddev,0);
	}
	else if (g.m_texturing)
	{
		if (g.m_mtex)
		{
			g.m_mtvbuf->Unlock();
			g.m_mtvtgt->ProcessVertices(vop,0,vcount,g.m_mtvbuf,g.m_nfv[2],g.m_d3ddev,0);
		}
		else
		{
			g.m_tvbuf->Unlock();
			g.m_tvtgt->ProcessVertices(vop,0,vcount,g.m_tvbuf,g.m_nfv[1],g.m_d3ddev,0);
		}
	}
	else
	{
		g.m_vbuf->Unlock();
		g.m_vtgt->ProcessVertices(vop,0,vcount,g.m_vbuf,g.m_nfv[0],g.m_d3ddev,0);
	}
}

static void fillVB(BOOL process, unsigned min, unsigned vcount)
{
	GLsizei i;

	if (g.m_usefogary)
	{
		g.m_comp = 2;
		if (g.m_lckcount != 0)
			g.m_incdex = 3;
		if (g.m_nfv[3] > (VBUFSIZE - vcount))	// check if space available
		{
			g.m_fmtvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
			g.m_nfv[3] = 0;
		}
		else
		{
			g.m_fmtvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
			g.m_verts = &(((QuakeFMTVertex*)g.m_verts)[g.m_nfv[3]]);
		}
	}
	else if (g.m_texturing) 
	{
		if (g.m_mtex)
		{
			g.m_comp = 2;
			if (g.m_lckcount != 0)
				g.m_incdex = 2;
			if (g.m_nfv[2] > (VBUFSIZE - vcount))	// check if space available
			{
				g.m_mtvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
				g.m_nfv[2] = 0;
			}
			else
			{
				g.m_mtvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
				g.m_verts = &(((QuakeMTVertex*)g.m_verts)[g.m_nfv[2]]);
			}
		}
		else
		{
			g.m_comp = 1;
			if (g.m_lckcount != 0)
				g.m_incdex = 1;
			if (g.m_nfv[1] > (VBUFSIZE - vcount))	// check if space available
			{
				g.m_tvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
				g.m_nfv[1] = 0;
			}
			else
			{
				g.m_tvbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
				g.m_verts = &(((QuakeTVertex*)g.m_verts)[g.m_nfv[1]]);
			}
		}
	}
	else
	{
		g.m_comp = 0;
		if (g.m_lckcount != 0)
			g.m_incdex = 0;
		if (g.m_nfv[0] > (VBUFSIZE - vcount))	// check if space available
		{
			g.m_vbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &g.m_verts, 0);
			g.m_nfv[0] = 0;
		}
		else
		{
			g.m_vbuf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &g.m_verts, 0);
			g.m_verts = &(((QuakeVertex*)g.m_verts)[g.m_nfv[0]]);
		}
	} 

	unsigned dstride = 7 + g.m_usefogary + g.m_comp*2;
	if (g.m_texturing)
	{
		if (g.m_usetexcoordary[0])
		{
#ifdef false // _X86_
			_asm {
				mov ecx, vcount;
				mov eax, min;
				mov ebx, g.m_texcoordstride;
				mul ebx;
				mov esi, g.m_texcoordary;
				lea esi, [eax + esi];
				mov ebx, g.m_usefogary;
				add ebx, 7;
				shl ebx, 2;
				mov edi, g.m_verts;
				lea edi, [ebx + edi];
				mov ebx, g.m_texcoordstride;
				sub ebx, 8;
				mov edx, dstride;
				shl edx, 2;
				sub edx, 8;
				cld;
lp0:			lodsd;
				stosd;
				lodsd;
				stosd;
				lea esi, [ebx + esi];
				lea edi, [edx + edi];
				loop lp0;
			}
#else
			unsigned char *tex0 = &((unsigned char *) g.m_texcoordary[0])[min*g.m_texcoordstride[0]];	
			D3DVALUE *data = (D3DVALUE *) g.m_verts+7 + g.m_usefogary;

			for (i = 0; i < vcount; ++i)
			{
				memcpy(data,tex0,2*sizeof(GLfloat));
				tex0 += g.m_texcoordstride[0];
				data += dstride;
			}
#endif
		}

		if (g.m_mtex && g.m_usetexcoordary[1])
		{
#ifdef _X86_
			_asm {
				mov ecx, vcount;
				mov eax, min;
				mov ebx, g.m_texcoordstride+4;
				mul ebx;
				mov esi, g.m_texcoordary+4;
				lea esi, [eax + esi];
				mov ebx, g.m_usefogary;
				add ebx, 9;
				shl ebx, 2;
				mov edi, g.m_verts;
				lea edi, [ebx + edi];
				mov ebx, g.m_texcoordstride+4;
				sub ebx, 8;
				mov edx, dstride;
				shl edx, 2;
				sub edx, 8;
				cld;
lp1:			lodsd;
				stosd;
				lodsd;
				stosd;
				lea esi, [ebx + esi];
				lea edi, [edx + edi];
				loop lp1;
			}
#else
			unsigned char *tex1 = &((unsigned char *) g.m_texcoordary[1])[min*g.m_texcoordstride[1]];	
			D3DVALUE *data = (D3DVALUE *) g.m_verts+9 + g.m_usefogary;

			for (i = 0; i < vcount; ++i)
			{
				memcpy(data,tex1,2*sizeof(GLfloat));
				tex1 += g.m_texcoordstride[1];
				data += dstride;
			}
#endif
		}
	}
		
	if (g.m_usenormalary)
	{
#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_normalstride;
			mul ebx;
			mov esi, g.m_normalary;
			lea esi, [eax + esi];
			mov ebx, 12;
			mov edi, g.m_verts;
			lea edi, [ebx + edi];
			mov ebx, g.m_normalstride;
			sub ebx, 12;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 12;
			cld;
lp2:		lodsd;
			stosd;
			lodsd;
			stosd;
			lodsd;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp2;
		}
#else
		unsigned char *nml = &((unsigned char *) g.m_normalary)[min*g.m_normalstride];
		D3DVALUE *data = (D3DVALUE *) g.m_verts+3;

		for (i = 0; i < vcount; ++i)
		{
			memcpy(data,nml,3*sizeof(GLfloat));
			nml += g.m_normalstride;
			data += dstride;
		}
#endif
	}

	if (g.m_usecolorary)
	{		
		if (g.m_colortype == GL_UNSIGNED_BYTE)
		{	
#ifdef _X86_
			unsigned int esstride = g.m_colorstride - 4;
			unsigned int edstride = (dstride-1) * 4;

			_asm
			{
				mov ecx, vcount;
				mov eax, min;
				mov ebx, g.m_colorstride;
				mul ebx;
				mov esi, g.m_colorary;
				lea esi, [eax + esi];
				mov ebx, 24;
				mov edi, g.m_verts;
				lea edi, [ebx + edi];
				mov edx, 0x00FF00FF;
				cld;            
		lp3:	lodsd;
				mov ebx, eax;
				and eax, edx;
				not edx;
				rol eax, 16;
				and ebx, edx;
				not edx;
				or  eax, ebx;
				stosd;
				mov eax, esstride;
				lea esi, [eax + esi];
				mov ebx, edstride;
				lea edi, [ebx + edi];
				loop lp3;
			}
#else
			GLubyte *clr = &((GLubyte *) g.m_colorary)[min*g.m_colorstride];
			D3DCOLOR *data = (D3DCOLOR *) g.m_verts+6;

			for (i = 0; i < vcount; i++)
			{
				data[0] = RGBA_MAKE(clr[0],clr[1],clr[2],clr[3]);
				clr += g.m_colorstride;
				data += dstride;
			}
#endif
		}
		else
		{
			GLubyte *clr = &((GLubyte *) g.m_colorary)[min*g.m_colorstride];
			D3DCOLOR *data = (D3DCOLOR *) g.m_verts+6;

			for (i = 0; i < vcount; ++i)
			{
				static float two55 = 255.f;
				unsigned int R, G, B, A;

#ifdef _X86_
				__asm {
					mov ebx, clr;
					fld [ebx];
					fld [ebx + 4];
					fld [ebx + 8];
					fld [ebx + 12];
					fld two55;
					fmul st(1), st(0);
					fmul st(2), st(0);
					fmul st(3), st(0);
					fmulp st(4), st(0);
					fistp A;
					fistp B;
					fistp G;
					fistp R;
					mov edx, A;
					cmp edx, 255;
					jle pt0;
					mov edx, 255;
			pt0:	mov eax, B;
					cmp eax, 255;
				  	jle pt1;
				  	mov eax, 255;
			pt1:	mov ebx, G;
					cmp ebx, 255;
					jle pt2;
					mov ebx, 255;
			pt2:	mov ecx, R;
					cmp ecx, 255;
					jle pt3;
					mov ecx, 255;
			pt3:	shl ebx, 8;
        			shl ecx, 16;
        			shl edx, 24;
        			or eax, ebx;
        			or ecx, edx;
        			or eax, ecx;
					mov edi, data;
        			stosd;
				}
#else
				GLfloat *fclr = (GLfloat *) clr;

				R = (unsigned int)(fclr[0] * two55);
				G = (unsigned int)(fclr[1] * two55);
				B = (unsigned int)(fclr[2] * two55);
				A = (unsigned int)(fclr[3] * two55);
				if (R > 255)
					R = 255;
				if (G > 255)
					G = 255;
				if (B > 255)
					B = 255;
				if (A > 255)
					A = 255;
				data[0] = RGBA_MAKE(R, G, B, A);
#endif
				clr += g.m_colorstride;
				data += dstride;
			}
		}
	}
	else
	{
#ifdef _X86_
		_asm
		{
			mov ecx, vcount;
			mov ebx, 24;
			mov edi, g.m_verts;
			lea edi, [ebx + edi];
			mov ebx, dstride;
			shl ebx, 2;
			sub ebx, 4;
			mov eax, g.m_color;
			cld;            
	lp4:	stosd;
			lea edi, [ebx + edi];
			loop lp4;
		}
#else
		D3DCOLOR *data = (D3DCOLOR *) g.m_verts+6;
		
		for (i = 0; i < vcount; ++i)
 		{
 			*data = g.m_color;
 			data += dstride;
 		}
#endif
 	}

 	if (g.m_usefogary)
 	{
		static float two55 = 255.f;
		unsigned int A;

#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_fogstride;
			mul ebx;
			mov esi, g.m_fogary;
			lea esi, [eax + esi];
			mov ebx, 28;
			mov edi, g.m_verts;
			lea edi, [ebx + edi];
			mov ebx, g.m_fogstride;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 4;
			cld;
lp5:		fld [esi];
			fld two55;
			fmulp st(1), st(0);
			fistp A;
			mov eax, 255;
			sub eax, A;
			cmp eax, 0;
			jge pt4;
			mov eax, 0;
pt4:		shl eax, 24;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp5;
		}
#else
		unsigned char *fog = &((unsigned char *) g.m_fogary)[min*g.m_fogstride];
 		D3DCOLOR *data = (D3DCOLOR *) g.m_verts+7;

 		for (i = 0; i < vcount; ++i)
 		{
 			A = (unsigned int) (((GLfloat *) fog)[0] * two55);
 			if (A > 255)
 				A = 255;
 			data[0] = (255-A) << 24;

 			fog += g.m_fogstride; 
 			data += dstride;
 		}
#endif
 	}

 	if (g.m_vertarysize == 2)
 	{	
#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_vertexstride;
			mul ebx;
			mov esi, g.m_vertexary;
			lea esi, [eax + esi];
			mov edi, g.m_verts;
			mov ebx, g.m_vertexstride;
			sub ebx, 8;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 12;
			cld;
lp6:		lodsd;
			stosd;
			lodsd;
			stosd;
			mov eax, 0;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp6;
		}
#else
 		unsigned char *vtx = &((unsigned char *) g.m_vertexary)[min*g.m_vertexstride];
 		D3DVALUE *data = (D3DVALUE *) g.m_verts;

 		for (i = 0; i < vcount; ++i)
 		{
 			memcpy(data,vtx,2*sizeof(GLfloat));
			data[2] = 0.0;
 			vtx += g.m_vertexstride;
 			data += dstride;
 		}
#endif
 	}
	else
	{	
#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_vertexstride;
			mul ebx;
			mov esi, g.m_vertexary;
			lea esi, [eax + esi];
			mov edi, g.m_verts;
			mov ebx, g.m_vertexstride;
			sub ebx, 12;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 12;
			cld;
lp7:		lodsd;
			stosd;
			lodsd;
			stosd;
			lodsd;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp7;
		}
#else
		unsigned char *vtx = &((unsigned char *) g.m_vertexary)[min*g.m_vertexstride];
 		D3DVALUE *data = (D3DVALUE *) g.m_verts;

 		for (i = 0; i < vcount; ++i)
 		{
 			memcpy(data,vtx,3*sizeof(GLfloat));
 			vtx += g.m_vertexstride;
 			data += dstride;
 		}
#endif
 	}

	if (process)
		processVB(vcount);

	g.m_tgtdirty = FALSE;
}

static void fillVB_NC(unsigned min, unsigned vcount)
{
	BufferInfo &binfo = g.m_vbuffers[g.m_useVBuffer];
	
	if (binfo.format != GL_V12MTNVFMT_EXT)
	{
		OutputDebugString("Wrapper: We shouldn't be here without a color per vertex\n");

		return;
	}

	g.m_verts = &(((V12MTNVertex *) binfo.verts)[binfo.nfv]); 

	unsigned dstride = 10;
	if (g.m_texturing)
	{
		if (g.m_usetexcoordary[0])
		{
#ifdef _X86_
			_asm {
				mov ecx, vcount;
				mov eax, min;
				mov ebx, g.m_texcoordstride;
				mul ebx;
				mov esi, g.m_texcoordary;
				lea esi, [eax + esi];
				mov ebx, g.m_usefogary;
				add ebx, 6;
				shl ebx, 2;
				mov edi, g.m_verts;
				lea edi, [ebx + edi];
				mov ebx, g.m_texcoordstride;
				sub ebx, 8;
				mov edx, dstride;
				shl edx, 2;
				sub edx, 8;
				cld;
lp0:			lodsd;
				stosd;
				lodsd;
				stosd;
				lea esi, [ebx + esi];
				lea edi, [edx + edi];
				loop lp0;
			}
#else
			unsigned char *tex0 = &((unsigned char *) g.m_texcoordary[0])[min*g.m_texcoordstride[0]];	
			D3DVALUE *data = (D3DVALUE *) g.m_verts+6;

			for (i = 0; i < vcount; ++i)
			{
				memcpy(data,tex0,2*sizeof(GLfloat));
				tex0 += g.m_texcoordstride[0];
				data += dstride;
			}
#endif
		}

		if (g.m_mtex && g.m_usetexcoordary[1])
		{
#ifdef _X86_
			_asm {
				mov ecx, vcount;
				mov eax, min;
				mov ebx, g.m_texcoordstride+4;
				mul ebx;
				mov esi, g.m_texcoordary+4;
				lea esi, [eax + esi];
				mov ebx, g.m_usefogary;
				add ebx, 8;
				shl ebx, 2;
				mov edi, g.m_verts;
				lea edi, [ebx + edi];
				mov ebx, g.m_texcoordstride+4;
				sub ebx, 8;
				mov edx, dstride;
				shl edx, 2;
				sub edx, 8;
				cld;
lp1:			lodsd;
				stosd;
				lodsd;
				stosd;
				lea esi, [ebx + esi];
				lea edi, [edx + edi];
				loop lp1;
			}
#else
			unsigned char *tex1 = &((unsigned char *) g.m_texcoordary[1])[min*g.m_texcoordstride[1]];	
			D3DVALUE *data = (D3DVALUE *) g.m_verts+8;

			for (i = 0; i < vcount; ++i)
			{
				memcpy(data,tex1,2*sizeof(GLfloat));
				tex1 += g.m_texcoordstride[1];
				data += dstride;
			}
#endif
		}
	}
		
	if (g.m_usenormalary)
	{
#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_normalstride;
			mul ebx;
			mov esi, g.m_normalary;
			lea esi, [eax + esi];
			mov ebx, 12;
			mov edi, g.m_verts;
			lea edi, [ebx + edi];
			mov ebx, g.m_normalstride;
			sub ebx, 12;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 12;
			cld;
lp2:		lodsd;
			stosd;
			lodsd;
			stosd;
			lodsd;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp2;
		}
#else
		unsigned char *nml = &((unsigned char *) g.m_normalary)[min*g.m_normalstride];
		D3DVALUE *data = (D3DVALUE *) g.m_verts+3;

		for (i = 0; i < vcount; ++i)
		{
			memcpy(data,nml,3*sizeof(GLfloat));
			nml += g.m_normalstride;
			data += dstride;
		}
#endif
	}

 	if (g.m_vertarysize == 2)
 	{	
#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_vertexstride;
			mul ebx;
			mov esi, g.m_vertexary;
			lea esi, [eax + esi];
			mov edi, g.m_verts;
			mov ebx, g.m_vertexstride;
			sub ebx, 8;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 12;
			cld;
lp6:		lodsd;
			stosd;
			lodsd;
			stosd;
			mov eax, 0;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp6;
		}
#else
 		unsigned char *vtx = &((unsigned char *) g.m_vertexary)[min*g.m_vertexstride];
 		D3DVALUE *data = (D3DVALUE *) g.m_verts;

 		for (i = 0; i < vcount; ++i)
 		{
 			memcpy(data,vtx,2*sizeof(GLfloat));
			data[2] = 0.0;
 			vtx += g.m_vertexstride;
 			data += dstride;
 		}
#endif
 	}
	else
	{	
#ifdef _X86_
		_asm {
			mov ecx, vcount;
			mov eax, min;
			mov ebx, g.m_vertexstride;
			mul ebx;
			mov esi, g.m_vertexary;
			lea esi, [eax + esi];
			mov edi, g.m_verts;
			mov ebx, g.m_vertexstride;
			sub ebx, 12;
			mov edx, dstride;
			shl edx, 2;
			sub edx, 12;
			cld;
lp7:		lodsd;
			stosd;
			lodsd;
			stosd;
			lodsd;
			stosd;
			lea esi, [ebx + esi];
			lea edi, [edx + edi];
			loop lp7;
		}
#else
		unsigned char *vtx = &((unsigned char *) g.m_vertexary)[min*g.m_vertexstride];
 		D3DVALUE *data = (D3DVALUE *) g.m_verts;

 		for (i = 0; i < vcount; ++i)
 		{
 			memcpy(data,vtx,3*sizeof(GLfloat));
 			vtx += g.m_vertexstride;
 			data += dstride;
 		}
#endif
 	}
}

GLAPI void GLAPIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	if (count == 0)
		return;

	if (count > g.m_numIndices)
	{
		g.m_numIndices = count;
		delete [] g.m_wIndices;
		g.m_wIndices = new WORD[g.m_numIndices];
	}

	unsigned min, max, vcount;
	GLsizei i;

	if (g.m_lckcount != 0)
	{
		switch (type)
		{
			case GL_UNSIGNED_BYTE:
				for (i = 0; i < count; ++i)
					g.m_wIndices[i] = (WORD) ((unsigned char*) indices)[i];
				break;
			case GL_UNSIGNED_SHORT:
				for (i = 0; i < count; ++i)
					g.m_wIndices[i] = (WORD) ((unsigned short*) indices)[i];
				break;
			case GL_UNSIGNED_INT:
				for (i = 0; i < count; ++i)
					g.m_wIndices[i] = (WORD) ((unsigned *) indices)[i];
				break;
		}
		min = g.m_lckfirst;
		max = g.m_lckfirst+g.m_lckcount - 1;
		vcount = g.m_lckcount;
	}
	else
	{
		min = 65535;
		max = 0;
		switch (type)
		{
			case GL_UNSIGNED_BYTE:
				for (i = 0; i < count; ++i)
				{
					unsigned t = ((unsigned char *) indices)[i];
					if (t < min)
						min = t;
					if (t > max)
						max = t;
				}
				
				for (i = 0; i < count; ++i)
					g.m_wIndices[i] = (WORD) (((unsigned char*) indices)[i] - min);
            break;
			case GL_UNSIGNED_SHORT:
				for (i = 0; i < count; ++i)
				{
					unsigned t = ((unsigned short *) indices)[i];

					if (t < min)
						min = t;
					if (t > max)
						max = t;
				}

				for (i = 0; i < count; ++i)
					g.m_wIndices[i] = (WORD) (((unsigned short *) indices)[i] - min);
            break;
			case GL_UNSIGNED_INT:
				for (i = 0; i < count; ++i)
				{
					unsigned t = ((unsigned int *) indices)[i];

					if (t < min)
						min = t;
					if (t > max)
						max = t;
				}

				for (i = 0; i < count; ++i)
					g.m_wIndices[i] = (WORD) (((unsigned int *) indices)[i] - min);
				break;
		}
		vcount = max-min + 1;
	}

	BOOL textrans = (g.m_dd.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) || g.m_usefogary ||
						 (g.m_texgen[0] && g.m_texgenmode[0] == GL_OBJECT_LINEAR) ||
						 (g.m_texgen[1] && g.m_texgenmode[1] == GL_SPHERE_MAP);
	BOOL unlock = false;

	QuakeSetTexturingState();

	if (g.m_useVBuffer != -1)
	{
		if (g.m_lckcount && !textrans && g.m_tgtdirty)
		{
			processVB(vcount);
			g.m_tgtdirty = false;
		}
	}
	else if (g.m_lckcount == 0 || (g.m_tgtdirty && textrans))
	{
		fillVB(false,min,vcount);
		unlock = true;
	}
	else if (g.m_tgtdirty)
		fillVB(true,min,vcount);

	if (g.m_useVBuffer != -1)
	{
		BufferInfo &binfo = g.m_vbuffers[g.m_useVBuffer];
		
		if (g.m_lckcount && !textrans)
			switch (mode) 
			{
				case GL_LINES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
        			break;
				case GL_TRIANGLES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
					break;
				case GL_TRIANGLE_STRIP:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
					break;
				case GL_POLYGON:
				case GL_TRIANGLE_FAN:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
					break;
				case GL_QUADS:
					for (i = 0; i < count; i += 4) 
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvtgt, 0, vcount, &g.m_wIndices[i], 4, 0);
        			break;
				default:
					{
        				char buf[64];
              	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    		}
		else
			switch (mode) 
			{
				case GL_LINES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, binfo.buf, binfo.nfv, vcount, g.m_wIndices, count, 0);
					break;
				case GL_TRIANGLES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, binfo.buf, binfo.nfv, vcount, g.m_wIndices, count, 0);
					break;
				case GL_TRIANGLE_STRIP:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, binfo.buf, binfo.nfv, vcount, g.m_wIndices, count, 0);
					break;
				case GL_POLYGON:
				case GL_TRIANGLE_FAN:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, binfo.buf, binfo.nfv, vcount, g.m_wIndices, count, 0);
					break;
				case GL_QUADS:
					for (i = 0; i < count; i += 4) 
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, binfo.buf, binfo.nfv, vcount, &g.m_wIndices[i], 4, 0);
					break;
				default:
					{
      				char buf[64];
             	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    		}
		if (g.m_lckcount == 0)
		{
			if (binfo.preserve)
			{
				g.m_d3ddev->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
				g.m_d3ddev->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
			}

			binfo.nfv += vcount;
			g.m_useVBuffer = -1;
		}
	}
	else if (g.m_lckcount != 0 && !textrans)
	{
		if (g.m_usefogary)
		{
			if (mode == GL_TRIANGLES)
				g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_fmtvtgt, 0, vcount, g.m_wIndices, count, 0);
			else
			{
        		char buf[64];
               	
				sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
				OutputDebugString(buf);
			}
		}
		else if (g.m_texturing)
		{
			if (g.m_mtex)
			{
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
        				break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvtgt, 0, vcount, g.m_wIndices, count, 0);
						break;
					case GL_QUADS:
						for (i = 0; i < count; i += 4) 
							g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvtgt, 0, vcount, &g.m_wIndices[i], 4, 0);
        				break;
					default:
						{
        					char buf[64];
               	
							sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
							OutputDebugString(buf);
						}
    			}
			}
			else
			{
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_tvtgt, 0, vcount, g.m_wIndices, count, 0);
        				break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_tvtgt, 0, vcount, g.m_wIndices, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_tvtgt, 0, vcount, g.m_wIndices, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvtgt, 0, vcount, g.m_wIndices, count, 0);
						break;
					case GL_QUADS:
						for (i = 0; i < count; i += 4) 
							g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvtgt, 0, vcount, &g.m_wIndices[i], 4, 0);
        				break;
					default:
						{
        					char buf[64];
               	
							sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
							OutputDebugString(buf);
						}
    			}
			}
		}
		else
		{
			switch (mode) 
			{
				case GL_LINES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_vtgt, 0, vcount, g.m_wIndices, count, 0);
        			break;
				case GL_TRIANGLES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_vtgt, 0, vcount, g.m_wIndices, count, 0);
					break;
				case GL_TRIANGLE_STRIP:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_vtgt, 0, vcount, g.m_wIndices, count, 0);
					break;
				case GL_POLYGON:
				case GL_TRIANGLE_FAN:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vtgt, 0, vcount, g.m_wIndices, count, 0);
					break;
				case GL_QUADS:
					for (i = 0; i < count; i += 4) 
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vtgt, 0, vcount, &g.m_wIndices[i], 4, 0);
        			break;
				default:
					{
        				char buf[64];
              	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    		}
		}
	}
	else
	{
		if (g.m_usefogary)
		{
			if (unlock)
				g.m_fmtvbuf->Unlock();
			if (mode == GL_TRIANGLES)
				g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_fmtvbuf, g.m_nfv[3], vcount, g.m_wIndices, count, 0);
			else
			{
        		char buf[64];
               	
				sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
				OutputDebugString(buf);
			}
			if (g.m_lckcount == 0)
				g.m_nfv[3] += vcount;
		}
		else if (g.m_texturing)
		{
			if (g.m_mtex)
			{
				if (unlock)
					g.m_mtvbuf->Unlock();
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_mtvbuf, g.m_nfv[2], vcount, g.m_wIndices, count, 0);
						break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_mtvbuf, g.m_nfv[2], vcount, g.m_wIndices, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_mtvbuf, g.m_nfv[2], vcount, g.m_wIndices, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvbuf, g.m_nfv[2], vcount, g.m_wIndices, count, 0);
						break;
					case GL_QUADS:
						for (i = 0; i < count; i += 4) 
							g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvbuf, g.m_nfv[2], vcount, &g.m_wIndices[i], 4, 0);
        				break;
					default:
						{
        					char buf[64];
               	
							sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
							OutputDebugString(buf);
						}
    			}
				if (g.m_lckcount == 0)
					g.m_nfv[2] += vcount;
			}
			else
			{
				if (unlock)
					g.m_tvbuf->Unlock();
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_tvbuf, g.m_nfv[1], vcount, g.m_wIndices, count, 0);
						break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_tvbuf, g.m_nfv[1], vcount, g.m_wIndices, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_tvbuf, g.m_nfv[1], vcount, g.m_wIndices, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvbuf, g.m_nfv[1], vcount, g.m_wIndices, count, 0);
						break;
					case GL_QUADS:
						for (i = 0; i < count; i += 4) 
							g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvbuf, g.m_nfv[1], vcount, &g.m_wIndices[i], 4, 0);
						break;
					default:
						{
        					char buf[64];
               	
							sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
							OutputDebugString(buf);
						}
    			}
				if (g.m_lckcount == 0)
					g.m_nfv[1] += vcount;
        				
			}
		}
		else
		{
			if (unlock)
				g.m_vbuf->Unlock();
			switch (mode) 
			{
				case GL_LINES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_LINELIST, g.m_vbuf, g.m_nfv[0], vcount, g.m_wIndices, count, 0);
					break;
				case GL_TRIANGLES:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLELIST, g.m_vbuf, g.m_nfv[0], vcount, g.m_wIndices, count, 0);
					break;
				case GL_TRIANGLE_STRIP:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_vbuf, g.m_nfv[0], vcount, g.m_wIndices, count, 0);
					break;
				case GL_POLYGON:
				case GL_TRIANGLE_FAN:
					g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vbuf, g.m_nfv[0], vcount, g.m_wIndices, count, 0);
					break;
				case GL_QUADS:
					for (i = 0; i < count; i += 4) 
						g.m_d3ddev->DrawIndexedPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vbuf, g.m_nfv[0], vcount, &g.m_wIndices[i], 4, 0);
					break;
				default:
					{
        				char buf[64];
              	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    		}
			if (g.m_lckcount == 0)
				g.m_nfv[0] += vcount;
        			
		}
	}
}

GLAPI void GLAPIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
    g.m_vwx = x;
    g.m_vwy = y;
    g.m_vww = width;
    g.m_vwh = height;
    g.m_updvwp = TRUE;
}

GLAPI void GLAPIENTRY glLineWidth (GLfloat width)
{ 
	width;
}

GLAPI void GLAPIENTRY glLoadIdentity (void)
{
	D3DMATRIX unity;

	unity._11 = 1.0f; unity._12 = 0.0f; unity._13 = 0.0f; unity._14 = 0.0f;
	unity._21 = 0.0f; unity._22 = 1.0f; unity._23 = 0.0f; unity._24 = 0.0f;
	unity._31 = 0.0f; unity._32 = 0.0f; unity._33 = 1.0f; unity._34 = 0.0f;
	unity._41 = 0.0f; unity._42 = 0.0f; unity._43 = 0.0f; unity._44 = 1.0f;
	g.m_d3ddev->SetTransform(g.m_matrixMode, &unity);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
	else if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
	{
		g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_TEXTURE0, &g.m_curtexmatrix);
		g.m_objectdirty[0] = TRUE;
	}
}

GLAPI void GLAPIENTRY glMatrixMode (GLenum mode)
{
	if (mode == GL_MODELVIEW)
		g.m_matrixMode = D3DTRANSFORMSTATE_WORLD;
	else if (mode == GL_PROJECTION)
		g.m_matrixMode = D3DTRANSFORMSTATE_PROJECTION;
	else
		g.m_matrixMode = D3DTRANSFORMSTATE_TEXTURE0;
}

GLAPI void GLAPIENTRY glDisable (GLenum cap)
{
	switch(cap)
	{
		case GL_DEPTH_TEST:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
			break;
		case GL_CULL_FACE:
			g.m_cullEnabled = FALSE;
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
			break;
		case GL_FOG:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
			break;
		case GL_BLEND:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
			break;
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
			break;
		case GL_POLYGON_OFFSET_FILL:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZBIAS, 0);
			break;
		case GL_STENCIL_TEST:
			break;
		case GL_SCISSOR_TEST:
			g.m_scissoring = FALSE;
			glViewport(g.m_vwx, g.m_vwy, g.m_vww, g.m_vwh);
			break;
		case GL_TEXTURE_2D:
			if (g.m_curtgt == 0)
			{
				g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_DISABLE);
            g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            g.m_texturing = FALSE;
			}
			else
			{
            g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_DISABLE);
            g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
            g.m_mtex = FALSE;
        	}
        	g.m_texHandleValid = FALSE;
        	dirtyArray();
        	break;
		case GL_ALPHA_TEST:
      	g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
        	break;
		case GL_LIGHTING:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
			g.m_lighting = FALSE;
			dirtyArray();
			break;
		case GL_TEXTURE_GEN_S:
		case GL_TEXTURE_GEN_T:
			g.m_texgen[g.m_curtgt] = FALSE;
			g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXCOORDINDEX,
														g.m_curtgt);
			g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
			dirtyArray();
			break;
		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			g.m_d3ddev->LightEnable(cap-GL_LIGHT0, FALSE);
			break;
    	default:
			{
				char buf[64];

				sprintf(buf,"Wrapper: glDisable on this cap not supported: %x\n", cap);
				OutputDebugString(buf);
			}
    }
}

GLAPI void GLAPIENTRY glDisableClientState (GLenum array)
{ 
	switch(array)
	{
		case GL_COLOR_ARRAY:
			g.m_usecolorary = FALSE;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			g.m_usetexcoordary[g.m_client_active_texture_arb] = FALSE;
			break;
		case GL_VERTEX_ARRAY:
			g.m_usevertexary = FALSE;
			break;
		case GL_NORMAL_ARRAY:
			g.m_usenormalary = FALSE;
			break;
		case GL_FOG_COORDINATE_ARRAY_EXT:
			g.m_usefogary = FALSE;
			break;
		default:
			{
				char buf[64];

				sprintf(buf,"Wrapper: Array not supported: %x\n", array);
				OutputDebugString(buf);
			}
	}
	dirtyArray();
}

GLAPI void GLAPIENTRY glDrawBuffer (GLenum mode)
{
	mode;
}

GLAPI void GLAPIENTRY glEnable (GLenum cap)
{
	switch (cap) {
		case GL_DEPTH_TEST:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
			break;
		case GL_CULL_FACE:
			g.m_cullEnabled = TRUE;
			if ((g.m_cullMode == GL_BACK && g.m_frontFace == GL_CCW) ||
			 	 (g.m_cullMode == GL_FRONT && g.m_frontFace == GL_CW))
				g.m_d3ddev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
			else
				g.m_d3ddev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
			break;
		case GL_FOG:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
			break;
		case GL_BLEND:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
			break;
		case GL_CLIP_PLANE0:
		case GL_CLIP_PLANE1:
		case GL_CLIP_PLANE2:
		case GL_CLIP_PLANE3:
		case GL_CLIP_PLANE4:
		case GL_CLIP_PLANE5:
			break;
		case GL_POLYGON_OFFSET_FILL:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ZBIAS, g.m_zbias*2);
			break;
		case GL_SCISSOR_TEST:
			g.m_scissoring = TRUE;
			g.m_updvwp = TRUE;
			break;
		case GL_TEXTURE_2D:
			if (g.m_curtgt == 0)
				g.m_texturing = TRUE;
			else
				g.m_mtex = TRUE;
			g.m_texHandleValid = FALSE;
			dirtyArray();
			break;
		case GL_ALPHA_TEST:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
			break;
		case GL_LIGHTING:
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_LIGHTING, TRUE);
			g.m_lighting = TRUE;
			dirtyArray();
			break;
		case GL_TEXTURE_GEN_S:
		case GL_TEXTURE_GEN_T:
			g.m_texgen[g.m_curtgt] = TRUE;
			if (g.m_texgenmode[g.m_curtgt] == GL_OBJECT_LINEAR)
			{
				g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXCOORDINDEX,
															g.m_curtgt | D3DTSS_TCI_CAMERASPACEPOSITION);
				g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
			}	
			else
			{
				g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXCOORDINDEX,
															g.m_curtgt | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
				g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
			}
			g.m_texcoordary[g.m_curtgt] = NULL;
			g.m_texcoordstride[g.m_curtgt] = 0;
			dirtyArray();
			break;
		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			g.m_d3ddev->LightEnable(cap-GL_LIGHT0, TRUE);
			break;
		default:
			{
				char buf[64];

				sprintf(buf,"Wrapper: glEnable on this cap not supported: %x\n",cap);
				OutputDebugString(buf);
			}
	}
}

GLAPI void GLAPIENTRY glEnableClientState (GLenum array)
{ 
	switch (array)
	{
		case GL_COLOR_ARRAY:
			g.m_usecolorary = TRUE;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			g.m_usetexcoordary[g.m_client_active_texture_arb] = TRUE;
			break;
		case GL_VERTEX_ARRAY:
			g.m_usevertexary = TRUE;
			break;
		case GL_NORMAL_ARRAY:
			g.m_usenormalary = TRUE;
			break;
		case GL_FOG_COORDINATE_ARRAY_EXT:
			g.m_usefogary = TRUE;
			break;
		default:
			OutputDebugString("Wrapper: Array not supported\n");
	}
	dirtyArray();
}

GLAPI void GLAPIENTRY glFogf (GLenum pname, GLfloat param)
{ 
	FLOAT start, end;
	
	switch (pname)
	{
		case GL_FOG_START:
			start = param;
			//g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGTABLESTART, *(DWORD*)(&start));
			break;
		case GL_FOG_END:
			end = param;
			//g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGTABLEEND, *(DWORD*)(&end));
			break;
		default:
			OutputDebugString("Wrapper: Fog pname not supported\n");
	}
}

GLAPI void GLAPIENTRY glFogfv (GLenum pname, const GLfloat *params)
{ 
	if (pname == GL_FOG_COLOR)
	{
		static float two55 = 255.f;
		unsigned int R, G, B, A;

#ifdef _X86_
		__asm {
			mov ebx, params;
			fld [ebx];
			fld [ebx + 4];
			fld [ebx + 8];
			fld [ebx + 12];
			fld two55;
			fmul st(1), st(0);
			fmul st(2), st(0);
			fmul st(3), st(0);
			fmulp st(4), st(0);
			fistp A;
			fistp B;
			fistp G;
			fistp R;
			mov edx, A;
			cmp edx, 255;
			jle pt1;
			mov edx, 255;
	pt1:	mov eax, B;
			cmp eax, 255;
			jle pt2;
			mov eax, 255;
	pt2:	mov ebx, G;
			cmp ebx, 255;
			jle pt3;
			mov ebx, 255;
	pt3:	mov ecx, R;
			cmp ecx, 255;
			jle pt4;
			mov ecx, 255;
	pt4:	shl ebx, 8;
			shl ecx, 16;
			shl edx, 24;
			or eax, ebx;
			or ecx, edx;
			or eax, ecx;
			mov g.m_fogcolor, eax;
		}
#else
		R = (unsigned int) (params[0] * two55);
		G = (unsigned int) (params[1] * two55);
		B = (unsigned int) (params[2] * two55);
		A = (unsigned int) (params[3] * two55);
		if (R > 255)
			R = 255;
		if (G > 255)
			G = 255;
		if (B > 255)
			B = 255;
		if (A > 255)
			A = 255;
		g.m_fogcolor = RGBA_MAKE(R, G, B, A);
#endif
		g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGCOLOR, g.m_fogcolor);
	}
	else
		OutputDebugString("Wrapper: Fog pname not supported\n");
}

GLAPI void GLAPIENTRY glFogi (GLenum pname, GLint param)
{
	switch(pname)
	{
		case GL_FOG_COORDINATE_SOURCE_EXT:
			break;
		case GL_FOG_MODE:
			switch (param)
			{
				case GL_LINEAR:
					//g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
					break;
				case GL_EXP:
					//g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_EXP);
					break;
				case GL_EXP2:
					//g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_EXP2);
					break;
			}
			break;
		default:
			OutputDebugString("Wrapper: Fog pname not supported\n");
	}
}

GLAPI void GLAPIENTRY glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	D3DMATRIX f;

	f._11 = (D3DVALUE)((2.0 * zNear) / (right - left));
	f._21 = 0.f;
	f._31 = (D3DVALUE)((right + left) / (right - left));
	f._41 = 0.f;
	f._12 = 0.f;
	f._22 = (D3DVALUE)((2.0 * zNear) / (top - bottom));
	f._32 = (D3DVALUE)((top + bottom) / (top - bottom));
	f._42 = 0.f;
	f._13 = 0.f;
	f._23 = 0.f;
	f._33 = (D3DVALUE)(-zFar / (zFar - zNear));
	//f._33 = (D3DVALUE)(-(zFar+zNear)/(zFar-zNear));
	f._43 = (D3DVALUE)(-(zFar * zNear) / (zFar - zNear));
	//f._43 = (D3DVALUE)(-(2.0 * zFar * zNear)/(zFar - zNear));
	f._14 = 0.f;
	f._24 = 0.f;
	f._34 = -1.f;
	f._44 = 0.f;
	g.m_d3ddev->MultiplyTransform(g.m_matrixMode, &f);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
}

GLAPI GLenum GLAPIENTRY glGetError (void)
{
    return GL_NO_ERROR;
}

GLAPI void GLAPIENTRY glGetDoublev (GLenum pname, GLdouble *params)
{
	switch (pname) {
		case GL_MODELVIEW_MATRIX:
			{
				D3DMATRIX tmp;

				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_WORLD, &tmp);
				for (unsigned char i = 0; i < 16; ++i)
					params[i] = ((GLfloat *) &tmp)[i];
			}
			break;
		case GL_PROJECTION_MATRIX:
			{
				D3DMATRIX tmp;

				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_PROJECTION, &tmp);
				for (unsigned char i = 0; i < 16; ++i)
					params[i] = ((GLfloat *) &tmp)[i];
			}			
			break;
		default:
			OutputDebugString("Wrapper: Unimplemented GetDoublev query\n");
	}
}

GLAPI void GLAPIENTRY glGetFloatv (GLenum pname, GLfloat *params)
{
    switch (pname) {
		case GL_MODELVIEW_MATRIX:
			{
				D3DMATRIX tmp;

				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_WORLD, &tmp);
				memcpy(params,&tmp,16*sizeof(GLfloat));
			}
			break;
		case GL_PROJECTION_MATRIX:
			{
				D3DMATRIX tmp;

				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_PROJECTION, &tmp);
				memcpy(params,&tmp,16*sizeof(GLfloat));
			}			
			break;
		case GL_TEXTURE_MATRIX:
			memcpy(params,&g.m_curtexmatrix,16*sizeof(GLfloat));
			break;
		case GL_FOG_COLOR:
			params[0] = (GLfloat) ((g.m_fogcolor >> 16) & 255)/255.0;
			params[1] = (GLfloat) ((g.m_fogcolor >> 8) & 255)/255.0;
			params[2] = (GLfloat) (g.m_fogcolor & 255)/255.0;
			params[3] = (GLfloat) ((g.m_fogcolor >> 24) & 255)/255.0;
			break;
		case GL_COLOR_CLEAR_VALUE:
			params[0] = (GLfloat) ((g.m_clearColor >> 16) & 255)/255.0;
			params[1] = (GLfloat) ((g.m_clearColor >> 8) & 255)/255.0;
			params[2] = (GLfloat) (g.m_clearColor & 255)/255.0;
			params[3] = (GLfloat) ((g.m_clearColor >> 24) & 255)/255.0;
			break;
		default:
			OutputDebugString("Wrapper: Unimplemented GetFloatv query\n");
	}
}

GLAPI void GLAPIENTRY glGetIntegerv (GLenum pname, GLint *params)
{ 
	switch(pname)
	{
		case GL_MAX_TEXTURE_SIZE:
			*params = (g.m_dd.dwMaxTextureWidth >= g.m_dd.dwMaxTextureHeight) ? g.m_dd.dwMaxTextureWidth : g.m_dd.dwMaxTextureHeight;
			break;
		case GL_MAX_TEXTURE_UNITS_ARB:
			*params = g.m_usemtex ? 2 : 1;
			break;
		case GL_MODELVIEW_STACK_DEPTH:
			*params = g.m_matrixStack[0].length() + 1;
			break;
		case GL_PROJECTION_STACK_DEPTH:
			*params = g.m_matrixStack[1].length() + 1;
			break;
		case GL_TEXTURE_STACK_DEPTH:
			*params = g.m_matrixStack[2].length() + 1;
			break;
		case GL_MAX_LIGHTS:
			*params = 8;
			break;
		default:
			OutputDebugString("Wrapper: Unimplemented GetIntegerv query\n");
	}
}

GLAPI const GLubyte* GLAPIENTRY glGetString (GLenum name)
{
    switch(name) {
    case GL_VENDOR:
        return (const GLubyte*)"Microsoft Corp.";
    case GL_RENDERER:
        return (const GLubyte*)"Direct3D";
    case GL_VERSION:
        return (const GLubyte*)"1.1";
    case GL_EXTENSIONS:
		if (g.m_colortable != FALSE)
			if (g.m_usemtex != FALSE)
				return (const GLubyte*)"GL_ARB_multitexture GL_EXT_compiled_vertex_array GL_SGIS_multitexture GL_EXT_fog_coord GL_EXT_texture_env_combine GL_EXT_paletted_texture GL_EXT_vertex_buffer";
			else
				return (const GLubyte*)"GL_EXT_compiled_vertex_array GL_EXT_fog_coord GL_EXT_paletted_texture";
		else
			if (g.m_usemtex != FALSE)
				return (const GLubyte*)"GL_ARB_multitexture GL_EXT_compiled_vertex_array GL_SGIS_multitexture GL_EXT_fog_coord GL_EXT_texture_env_combine GL_EXT_vertex_buffer";
			else
				return (const GLubyte*)"GL_EXT_compiled_vertex_array GL_EXT_fog_coord";
    default:
        OutputDebugString("Wrapper: Unimplemented GetString query\n");
    }
    return (const GLubyte*)"";
}

GLAPI void GLAPIENTRY glLoadMatrixf (const GLfloat *m)
{
	if (g.m_matrixMode == D3DTRANSFORMSTATE_PROJECTION)
	{
		D3DMATRIX f = *((LPD3DMATRIX) m);

		f._13 = (((LPD3DMATRIX) m)->_13 + ((LPD3DMATRIX) m)->_14) * 0.5f;
		f._23 = (((LPD3DMATRIX) m)->_23 + ((LPD3DMATRIX) m)->_24) * 0.5f;
		f._33 = (((LPD3DMATRIX) m)->_33 + ((LPD3DMATRIX) m)->_34) * 0.5f;
		f._43 = (((LPD3DMATRIX) m)->_43 + ((LPD3DMATRIX) m)->_44) * 0.5f;
		g.m_d3ddev->SetTransform(g.m_matrixMode, &f);
	}
	else
	{
		g.m_d3ddev->SetTransform(g.m_matrixMode, (LPD3DMATRIX) m);
		if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
			g.m_inversedirty = TRUE;
		else if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
		{
			g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_TEXTURE0, &g.m_curtexmatrix);
			g.m_objectdirty[0] = TRUE;
		}
	}
}

GLAPI void GLAPIENTRY glLockArraysEXT(GLint first, GLsizei count)
{
	g.m_lckfirst = first;
	g.m_lckcount = count;
	g.m_tgtdirty = TRUE;
}

GLAPI void GLAPIENTRY glMTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t)
{
    if(target == GL_TEXTURE0_SGIS) {
        g.m_tu = s;
        g.m_tv = t;
    }
    else {
        g.m_tu2 = s;
        g.m_tv2 = t;
    }
}

GLAPI void GLAPIENTRY glMultiTexCoord2fARB (GLenum texture, GLfloat s, GLfloat t)
{
	if(texture == GL_TEXTURE0_ARB) 
	{
		g.m_tu = s;
      g.m_tv = t;
    }
    else 
    {
      g.m_tu2 = s;
      g.m_tv2 = t; 
    }
}

GLAPI void GLAPIENTRY glMultiTexCoord2fvARB (GLenum texture, GLfloat *v)
{
	texture;
	v;

	#ifdef DODPFS
	DPF("glMultiTexCoord2fvARB");
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	D3DMATRIX f;

	left += 0.5;
	right += 0.5;
	top += 0.5;
	bottom += 0.5;

	f._11 = (D3DVALUE) (2.0 / (right - left));
	f._21 = 0.f;
	f._31 = 0.f;
	f._41 = (D3DVALUE) (-(right + left) / (right - left));
	f._12 = 0.f;
	f._22 = (D3DVALUE) (2.0 / (top - bottom));
	f._32 = 0.f;
	f._42 = (D3DVALUE) (-(top + bottom) / (top - bottom));
	f._13 = 0.f;
	f._23 = 0.f;
	f._33 = (D3DVALUE) (-1.0 / (zFar - zNear));
	f._43 = (D3DVALUE) (-zNear / (zFar - zNear));
	f._14 = 0.f;
	f._24 = 0.f;
	f._34 = 0.f;
	f._44 = 1.f;
	g.m_d3ddev->MultiplyTransform(g.m_matrixMode, &f);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
}

GLAPI void GLAPIENTRY glPolygonMode (GLenum face, GLenum mode)
{
	face;

	int statevalue=-1;
	switch(mode) {
		case GL_POINT:
			statevalue=D3DFILL_POINT;
			break;
		case GL_LINE:
			statevalue=D3DFILL_WIREFRAME; 
			break;
		case GL_FILL:
			statevalue=D3DFILL_SOLID;
			break;
	}
	if(statevalue >= 0) {
		g.m_d3ddev->SetRenderState(D3DRENDERSTATE_FILLMODE, (DWORD)statevalue);
	}
}

GLAPI void GLAPIENTRY glPolygonOffset (GLfloat factor, GLfloat units)
{
	factor;

	g.m_zbias = (DWORD) -units;
}

GLAPI void GLAPIENTRY glPopMatrix (void)
{
	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
	{
		GLListManip<D3DMATRIX> m(&g.m_matrixStack[0]);

		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_WORLD, &(m()));
		m.remove();
		g.m_inversedirty = TRUE;
	}
	else if (g.m_matrixMode == D3DTRANSFORMSTATE_PROJECTION)
	{
		GLListManip<D3DMATRIX> m(&g.m_matrixStack[1]);

		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &(m()));
		m.remove();
	}
	else
	{
		GLListManip<D3DMATRIX> m(&g.m_matrixStack[2]);

		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0, &(m()));
		g.m_curtexmatrix = m();
		g.m_objectdirty[0] = TRUE;
		m.remove();

		//if (g.m_matrixStack[2].length() == 0 && !g.m_texgen[0])
			//g.m_d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	}
}

GLAPI void GLAPIENTRY glPushMatrix (void)
{
	if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
	{
		//if (g.m_matrixStack[2].length() == 0 && !g.m_texgen[0])
			//g.m_d3ddev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		g.m_matrixStack[2].prepend(g.m_curtexmatrix);
	}
	else
	{
		D3DMATRIX m;

		g.m_d3ddev->GetTransform(g.m_matrixMode, &m);
		if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
			g.m_matrixStack[0].prepend(m);
		else
			g.m_matrixStack[1].prepend(m);
	}
}

GLAPI void GLAPIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	static double PI = 4. * atan(1.);
	D3DMATRIX f;
	double ra = angle * PI / 180.f;
	float ca = (float) cos(ra);
	float sa = (float) sin(ra);

	if (x == 1.f)
	{
		f._11 = 1.f; f._21 = 0.f; f._31 = 0.f; f._41 = 0.f;
		f._12 = 0.f; f._22 = ca;  f._32 = -sa; f._42 = 0.f;
		f._13 = 0.f; f._23 = sa;  f._33 = ca;  f._43 = 0.f;
		f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;    
	}
	else if (y == 1.f)
	{
		f._11 = ca;  f._21 = 0.f; f._31 = sa;  f._41 = 0.f;
		f._12 = 0.f; f._22 = 1.f; f._32 = 0.f; f._42 = 0.f;
		f._13 = -sa; f._23 = 0.f; f._33 = ca;  f._43 = 0.f;
		f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;    
	}
	else if (z == 1.f)
	{
		f._11 = ca;  f._21 = -sa; f._31 = 0.f; f._41 = 0.f;
		f._12 = sa;  f._22 = ca;  f._32 = 0.f; f._42 = 0.f;
		f._13 = 0.f; f._23 = 0.f; f._33 = 1.f; f._43 = 0.f;
		f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;    
	}
	else
		return;

	if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_curtexmatrix);

	g.m_d3ddev->MultiplyTransform(g.m_matrixMode, &f);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
	else if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
	{
		g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_TEXTURE0, &g.m_curtexmatrix);
		g.m_objectdirty[0] = TRUE;
	}
}

GLAPI void GLAPIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	D3DMATRIX f;

	f._11 =   x; f._21 = 0.f; f._31 = 0.f; f._41 = 0.f;
	f._12 = 0.f; f._22 =   y; f._32 = 0.f; f._42 = 0.f; 
	f._13 = 0.f; f._23 = 0.f; f._33 =   z; f._43 = 0.f;
	f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;

	if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_curtexmatrix);

	g.m_d3ddev->MultiplyTransform(g.m_matrixMode, &f);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
	else if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
	{
		g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_TEXTURE0, &g.m_curtexmatrix);
		g.m_objectdirty[0] = TRUE;
	}
}

GLAPI void GLAPIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{ 
    g.m_scix = x;
    g.m_sciy = y;
    g.m_sciw = width;
    g.m_scih = height;
    g.m_updvwp = TRUE;
}

GLAPI void GLAPIENTRY glSelectTextureSGIS(GLenum target)
{
    g.m_curtgt = target == GL_TEXTURE0_SGIS ? 0 : 1;
}

GLAPI void GLAPIENTRY glShadeModel (GLenum mode)
{
    if(mode == GL_SMOOTH)
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    else
        g.m_d3ddev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
}

GLAPI void GLAPIENTRY glTexCoord2f (GLfloat s, GLfloat t)
{
	g.m_tu = s;
	g.m_tv = t;
}

GLAPI void GLAPIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	if (size == 2 && type == GL_FLOAT)
	{
		g.m_texcoordary[g.m_client_active_texture_arb] = (GLfloat *) pointer;
		if (stride)
			g.m_texcoordstride[g.m_client_active_texture_arb] = stride;
		else
			g.m_texcoordstride[g.m_client_active_texture_arb] = 8;
	}
	else
	{
		char buf[128];

		sprintf(buf,"Wrapper: TexCoord array not supported (size: %d  type: %x  stride: %d)\n",
					size, type, stride);
		OutputDebugString(buf);
	}
}

GLAPI void GLAPIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
	target;

	switch (pname)
	{
		case GL_TEXTURE_ENV_MODE:
			{
				g.m_blendmode[g.m_curtgt] = (int) param;
				g.m_texHandleValid = FALSE;
			}
			break;
		case GL_COMBINE_RGB_EXT:
		case GL_SOURCE0_RGB_EXT:
		case GL_OPERAND0_RGB_EXT:
		case GL_SOURCE1_RGB_EXT:
		case GL_OPERAND1_RGB_EXT:
			break;
		case GL_SOURCE2_RGB_EXT:
			if ((int) param == GL_CONSTANT_EXT)
				g.m_d3ddev->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, g.m_envcolor);
			g.m_source2rgbext = (int) param;
			break;
		case GL_OPERAND2_RGB_EXT:
		case GL_COMBINE_ALPHA_EXT:
			break;
		case GL_SOURCE0_ALPHA_EXT:
			if ((int) param == GL_CONSTANT_EXT)
				g.m_d3ddev->SetRenderState(D3DRENDERSTATE_TEXTUREFACTOR, g.m_envcolor);
		case GL_OPERAND0_ALPHA_EXT:
			break;
		case GL_TEXTURE_ENV_COLOR:
			OutputDebugString("Wrapper: GL_TEXTURE_ENV_COLOR not implemented\n");
			break;
	}
}

GLAPI void GLAPIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param)
{ 
	glTexEnvf(target,pname,(GLfloat) param);
}

GLAPI void GLAPIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei glwidth, GLsizei glheight, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	target;
	border;
	format;
	type;

	DWORD width, height;
	TexInfo &ti = g.m_tex[g.m_curstagebinding[g.m_curtgt]];
    
	/* See if texture needs to be subsampled */
	if (g.m_subsample)
		if (level == 0)
			if (glwidth > 256 || glheight > 256)
				if (glwidth > glheight) {
					width = 256;
					height = (glheight * 256) / glwidth;
				}
				else {
					height = 256;
					width = (glwidth * 256) / glheight;
				}
			else {
				width = glwidth;
				height = glheight;
			}
		else	// mip-map levels need to be resampled too!
			if (ti.m_width != ti.m_oldwidth ||
				 ti.m_height != ti.m_oldheight)
			{
				width = (float) glwidth * (float) ti.m_width/(float) ti.m_oldwidth;
				height = (float) glheight * (float) ti.m_height/(float) ti.m_oldheight;
				if (!width || !height)
					return;
			}
			else
			{
				width = glwidth;
				height = glheight;
			}
	else {
		width = glwidth;
		height = glheight;
	}
    
	/* See if texture needs to be square */
	if (g.m_makeSquare)
		if (height > width)
			width = height;
		else
			height = width;
    
	if (level == 0) {
		LPDIRECTDRAWSURFACE7 ddsurf;
		D3DX_SURFACEFORMAT fmt;

		switch(internalformat) {
			case GL_COLOR_INDEX8_EXT:
				if (ti.m_rgbaindexed)
					fmt = g.m_ddFourBitAlphaSurfFormat;
				else
					fmt = D3DX_SF_PALETTE8;
				break;
			case GL_LUMINANCE:
				fmt = g.m_ddLuminanceSurfFormat;
				break;
			case GL_RGB5:
			case GL_RGB5_A1:
				fmt = g.m_ddFiveBitSurfFormat;
				break;
			case GL_RGBA4:
				fmt = g.m_ddFourBitAlphaSurfFormat;
				break;
			case GL_RGB:
         case GL_RGB8:
				fmt = g.m_ddEightBitSurfFormat;
				break;
			case GL_RGBA:
         case GL_RGBA8:
				fmt = g.m_ddEightBitAlphaSurfFormat;
				break;
			case GL_ALPHA:
				fmt = g.m_ddAlphaSurfFormat;
				break;
			default:
				OutputDebugString("Wrapper: Unimplemented internalformat\n");
				break;
		}

		if (fmt == D3DX_SF_PALETTE8)
		{
			DDSURFACEDESC2 ddsd;

			ZeroMemory(&ddsd, sizeof(ddsd));
			ddsd.dwSize  = sizeof(ddsd);
			ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
			ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
			ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
			ddsd.dwHeight = height;
			ddsd.dwWidth  = width;
			ddsd.ddpfPixelFormat.dwSize  = sizeof(DDPIXELFORMAT);
			ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
 
			// Set the bit depth for an 8-bit surface, but DO NOT 
			// specify any RGB mask values. The masks must be zero
			// for a palettized surface.
			ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

			HRESULT ddrval = g.m_dddev->CreateSurface(&ddsd,&ddsurf,NULL);

			if (ddrval != DD_OK) 
			{
				OutputDebugString("Wrapper: CreateSurface failed\n");
			
				return;
			}

			ddrval = ddsurf->SetPalette(ti.m_palette);
			if (ddrval != DD_OK)
			{
				OutputDebugString("Wrapper: SetPalette failed\n");

				return;
			}
		}
		else
		{
			DWORD flags = D3DX_TEXTURE_NOMIPMAP;
			HRESULT ddrval = D3DXCreateTexture(g.m_d3ddev,
														  &flags,
														  (DWORD*)&width, (DWORD*)&height, &fmt,
														  NULL, // Palette
														  &ddsurf,
														  NULL);
													  
			if (ddrval != DD_OK) 
			{
				OutputDebugString("Wrapper: CreateTexture failed\n");
			
				return;
			}
		}

		LoadSurface(ddsurf, internalformat, glwidth, glheight, width, height, (const DWORD*)pixels);
		if (ti.m_ddsurf != 0)
			ti.m_ddsurf->Release();
		ti.m_dwStage = g.m_curtgt;
		ti.m_fmt = fmt;
		ti.m_internalformat = internalformat;
		ti.m_width = width;
		ti.m_height = height;
		ti.m_ddsurf = ddsurf;
		ti.m_oldwidth = glwidth;
		ti.m_oldheight = glheight;

		if (ti.m_block == 0)
		{
			g.m_d3ddev->BeginStateBlock();
			g.m_d3ddev->SetTextureStageState (g.m_curtgt, D3DTSS_ADDRESSU,ti.m_addu);
			g.m_d3ddev->SetTextureStageState (g.m_curtgt, D3DTSS_ADDRESSV,ti.m_addv);
			g.m_d3ddev->SetTextureStageState (g.m_curtgt, D3DTSS_MAGFILTER,ti.m_magmode);
			g.m_d3ddev->SetTextureStageState (g.m_curtgt, D3DTSS_MINFILTER,ti.m_minmode);
			g.m_d3ddev->SetTextureStageState (g.m_curtgt, D3DTSS_MIPFILTER,ti.m_mipmode);
			g.m_d3ddev->SetTexture(g.m_curtgt, ti.m_ddsurf);
			g.m_d3ddev->EndStateBlock(&ti.m_block);
			ti.m_capture = FALSE;
		}
		else
			ti.m_capture = TRUE;
	}
	else if (level == 1 && g.m_usemipmap) { // oops, a mipmap
		LPDIRECTDRAWSURFACE7 ddsurf;
		
		if (ti.m_fmt == D3DX_SF_PALETTE8)
		{
			DDSURFACEDESC2 ddsd;

			ZeroMemory(&ddsd, sizeof(ddsd));
			ddsd.dwSize  = sizeof(ddsd);
			ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
			ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
			ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
			ddsd.dwHeight = ti.m_height;
			ddsd.dwWidth  = ti.m_width;
			ddsd.ddpfPixelFormat.dwSize  = sizeof(DDPIXELFORMAT);
			ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
 
			// Set the bit depth for an 8-bit surface, but DO NOT 
			// specify any RGB mask values. The masks must be zero
			// for a palettized surface.
			ddsd.ddpfPixelFormat.dwRGBBitCount = 8;

			HRESULT ddrval = g.m_dddev->CreateSurface(&ddsd,&ddsurf,NULL);

			if (ddrval != DD_OK) 
			{
				OutputDebugString("Wrapper: CreateSurface failed\n");
			
				return;
			}

			ddrval = ddsurf->SetPalette(ti.m_palette);
			if (ddrval != DD_OK)
			{
				OutputDebugString("Wrapper: SetPalette failed\n");

				return;
			}
		}
		else
		{
			DWORD flags = 0;
			HRESULT ddrval = D3DXCreateTexture(g.m_d3ddev,
														  &flags,
														  (DWORD *) &ti.m_width, (DWORD *) &ti.m_height, &ti.m_fmt,
														  NULL, // Palette
														  &ddsurf,
														  NULL);

			if (ddrval != DD_OK) {
				OutputDebugString("Wrapper: CreateSurface for texture failed\n");

				return;
			}
		}

		ddsurf->Blt(NULL, ti.m_ddsurf, NULL, DDBLT_WAIT, NULL);
		ti.m_ddsurf->Release();
		ti.m_ddsurf = ddsurf;

		DDSURFACEDESC2 ddsd;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;

		LPDIRECTDRAWSURFACE7 lpDDSTmp;

		ti.m_ddsurf->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSTmp);
		LoadSurface(lpDDSTmp, internalformat, glwidth, glheight, width, height, (const DWORD*)pixels);
		lpDDSTmp->Release();
		ti.m_capture = TRUE;
	}
	else if (g.m_usemipmap) {
		LPDIRECTDRAWSURFACE7 ddsurf = ti.m_ddsurf;

		ddsurf->AddRef();
		for (int i = 0; i < level; ++i) {
			DDSURFACEDESC2 ddsd;

			memset(&ddsd, 0, sizeof(ddsd));
			ddsd.dwSize = sizeof(ddsd);
			ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;

			LPDIRECTDRAWSURFACE7 lpDDSTmp;
			HRESULT ddrval = ddsurf->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSTmp);

			ddsurf->Release();
			if (ddrval == DDERR_NOTFOUND)
				return;

			ddsurf = lpDDSTmp;
		}

		LoadSurface(ddsurf, internalformat, glwidth, glheight, width, height, (const DWORD*)pixels);
		ddsurf->Release();
	}
	g.m_texHandleValid = FALSE;
}

GLAPI void GLAPIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	target;
	format;
	type;
	
	TexInfo &ti = g.m_tex[g.m_curstagebinding[g.m_curtgt]];
	LPDIRECTDRAWSURFACE7 ddsurf = ti.m_ddsurf;
	RECT subimage;

	if (!ddsurf)
	{
		// We haven't got a surface -- so just do a glTexImage2D
		// NOTE:  this won't work for a mip-mapped texture
		glTexImage2D(target,level,format,width,height,0,format,type,pixels);

		return;
	}

	for(int i = 0; i < level; ++i) {
		DDSURFACEDESC2 ddsd;
		HRESULT ddrval;

		memset(&ddsd.ddsCaps, 0, sizeof(ddsd.ddsCaps));
		ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
		
		LPDIRECTDRAWSURFACE7 lpDDSTmp;
		
		ddrval = ddsurf->GetAttachedSurface(&ddsd.ddsCaps, &lpDDSTmp);
		if (ddrval == DDERR_NOTFOUND)
			return;
		ddsurf = lpDDSTmp;
		ddsurf->Release();
	}

	GLsizei newWidth = width;
	GLsizei newHeight = height;

	if (ti.m_width != ti.m_oldwidth || ti.m_height != ti.m_oldheight)
	{
		newWidth = (float) width * (float) ti.m_width/(float) ti.m_oldwidth;
		newHeight = (float) height * (float) ti.m_height/(float) ti.m_oldheight;
		if (!newWidth || !newHeight)
			return;
	
		xoffset = (float) xoffset * (float) ti.m_width/(float) ti.m_oldwidth;
	  	yoffset = (float) yoffset * (float) ti.m_height/(float) ti.m_oldheight;
	}

	SetRect(&subimage, xoffset, yoffset, newWidth + xoffset, newHeight + yoffset);
	if (DD_OK != LoadSubSurface(ddsurf, ti.m_internalformat, width, height, (const DWORD*)pixels, &subimage)) {
		OutputDebugString("Wrapper: LoadSubSurface Failure.\n");
		
		return;
	}
}		

GLAPI void GLAPIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	target;

    switch(pname) {
    case GL_TEXTURE_MIN_FILTER:
        switch((int)param) {
        case GL_NEAREST:
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_minmode = D3DTFN_POINT;
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_mipmode = D3DTFP_NONE;
            break;
        case GL_LINEAR:
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_minmode = D3DTFN_LINEAR;
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_mipmode = D3DTFP_NONE;
            break;
        case GL_NEAREST_MIPMAP_NEAREST:
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_minmode = D3DTFN_POINT;
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_mipmode = D3DTFP_POINT;
            break;
        case GL_NEAREST_MIPMAP_LINEAR:
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_minmode = D3DTFN_POINT;
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_mipmode = D3DTFP_LINEAR;
            break;
        case GL_LINEAR_MIPMAP_NEAREST:
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_minmode = D3DTFN_LINEAR;
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_mipmode = D3DTFP_POINT;
            break;
        case GL_LINEAR_MIPMAP_LINEAR:
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_minmode = D3DTFN_LINEAR;
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_mipmode = D3DTFP_LINEAR;
            break;
        }
        break;
    case GL_TEXTURE_MAG_FILTER:
        if((int)param == GL_NEAREST)
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_magmode = D3DTFG_POINT;
        else
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_magmode = D3DTFG_LINEAR;
        break;
    case GL_TEXTURE_WRAP_S:
        if((int)param == GL_CLAMP)
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_addu = D3DTADDRESS_CLAMP;
        else
            //GL_REPEAT falls here
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_addu = D3DTADDRESS_WRAP;
        break;
    case GL_TEXTURE_WRAP_T:
        if((int)param == GL_CLAMP)
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_addv = D3DTADDRESS_CLAMP;
        else
            //GL_REPEAT falls here
            g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_addv = D3DTADDRESS_WRAP;
        break;
    }
    g.m_tex[g.m_curstagebinding[g.m_curtgt]].m_capture = TRUE;
    g.m_texHandleValid = FALSE;
}

GLAPI void GLAPIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	glTexParameterf(target,pname,(GLfloat) param);
}

GLAPI void GLAPIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	D3DMATRIX f;

	f._11 = 1.f; f._21 = 0.f; f._31 = 0.f; f._41 = x;
	f._12 = 0.f; f._22 = 1.f; f._32 = 0.f; f._42 = y;
	f._13 = 0.f; f._23 = 0.f; f._33 = 1.f; f._43 = z;
	f._14 = 0.f; f._24 = 0.f; f._34 = 0.f; f._44 = 1.f;

	if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_curtexmatrix);

	g.m_d3ddev->MultiplyTransform(g.m_matrixMode, &f);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
	else if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
	{
		g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_TEXTURE0, &g.m_curtexmatrix);
		g.m_objectdirty[0] = TRUE;
	}
}

static void dirtyArray()
{
	if (g.m_incdex > -1)
	{
		g.m_nfv[g.m_incdex] += g.m_lckcount;
		g.m_incdex = -1;
	}
	g.m_tgtdirty = TRUE;
}

GLAPI void GLAPIENTRY glUnlockArraysEXT()
{
	if (g.m_useVBuffer != -1)
	{
		BufferInfo &binfo = g.m_vbuffers[g.m_useVBuffer];

		if (binfo.preserve)
		{
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
			g.m_d3ddev->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
		}
		
		binfo.nfv += g.m_lckcount;
		g.m_useVBuffer = -1;
	}
	else if (g.m_incdex > -1)
	{
		g.m_nfv[g.m_incdex] += g.m_lckcount;
		g.m_incdex = -1;
	} 
	g.m_lckfirst = 0;
	g.m_lckcount = 0;
}

GLAPI void GLAPIENTRY glVertex2f (GLfloat x, GLfloat y)
{
	y;

	if (g.m_nfv[g.m_comp] + g.m_vcnt[g.m_comp] >= (VBUFSIZE - MAXVERTSPERPRIM))
	{
		if (g.m_prim == GL_TRIANGLES)
		{
			if (g.m_vcnt[g.m_comp] % 3 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
		else if (g.m_prim == GL_QUADS)
		{
			if (g.m_vcnt[g.m_comp] % 4 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
		else if (g.m_prim == GL_LINES)
		{
			if (g.m_vcnt[g.m_comp] % 2 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}	
	}
#ifdef _X86_
	__asm
	{    
		mov     ecx, g.m_comp;
		mov     edi, g.m_verts;
		lea     esi, x;
		mov     eax, g.m_vcnt[ecx * 4];
		lea     edx, [eax + 1];
		mov     g.m_vcnt[ecx * 4], edx;
		lea     edx, [ecx * 8 + 28];
		mul     edx;
		cld;
		add     edi, eax;
		xor     eax, eax;
		movsd;
		movsd;
		stosd;
		lea     esi, g.m_nx;
		lea     ecx, [ecx * 2 + 4];
		rep     movsd;
	}
#else
	D3DVALUE *d3dv = &(((D3DVALUE*)g.m_verts)[g.m_vcnt[g.m_comp]++ * (g.m_comp * 2 + 7)]);

	*(d3dv++) = x;
	*(d3dv++) = y;
	*(d3dv++) = 0.f;
	memcpy(d3dv, &g.m_nx, (sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + g.m_comp*sizeof(D3DVALUE)*2));
#endif
}

GLAPI void GLAPIENTRY glVertex2i (GLint x, GLint y)
{
	glVertex2f((GLfloat) x, (GLfloat) y);
}

GLAPI void GLAPIENTRY glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
	y;
	z;

	if (g.m_nfv[g.m_comp] + g.m_vcnt[g.m_comp] >= (VBUFSIZE - MAXVERTSPERPRIM))
	{
		if (g.m_prim == GL_TRIANGLES)
		{
			if (g.m_vcnt[g.m_comp] % 3 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
		else if(g.m_prim == GL_QUADS)
		{
			if (g.m_vcnt[g.m_comp] % 4 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
		else if (g.m_prim == GL_LINES)
		{
			if (g.m_vcnt[g.m_comp] % 2 == 0)
			{
				glEnd();
				glBegin(g.m_prim);
			}
		}
	}
#ifdef _X86_
	__asm
	{    
		mov     ecx, g.m_comp;
		mov     edi, g.m_verts;
		lea     esi, x;
		mov     eax, g.m_vcnt[ecx * 4];
		lea     edx, [eax + 1];
		mov     g.m_vcnt[ecx * 4], edx;
		lea     edx, [ecx * 8 + 28];
		mul     edx;
		cld;
		add     edi, eax;
		movsd;
		movsd;
		movsd;
		lea     esi, g.m_nx;
		lea     ecx, [ecx * 2 + 4];
		rep     movsd;
	}
#else
	D3DVALUE *d3dv = &(((D3DVALUE*)g.m_verts)[g.m_vcnt[g.m_comp]++ * (g.m_comp * 2 + 7)]);

	*(d3dv++) = x;
	*(d3dv++) = y;
	*(d3dv++) = z;
	memcpy(d3dv, &g.m_nx, (sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + g.m_comp*sizeof(D3DVALUE)*2));
#endif
}

GLAPI void GLAPIENTRY glVertex2fv (const GLfloat *v)
{
    if(g.m_nfv[g.m_comp] + g.m_vcnt[g.m_comp] >= (VBUFSIZE - MAXVERTSPERPRIM))
    {
        if(g.m_prim == GL_TRIANGLES)
        {
            if(g.m_vcnt[g.m_comp] % 3 == 0)
            {
                glEnd();
                glBegin(g.m_prim);
            }
        }
        else if(g.m_prim == GL_QUADS)
        {
            if(g.m_vcnt[g.m_comp] % 4 == 0)
            {
                glEnd();
                glBegin(g.m_prim);
            }
        }
        else if(g.m_prim == GL_LINES)
        {
            if(g.m_vcnt[g.m_comp] % 2 == 0)
            {
                glEnd();
                glBegin(g.m_prim);
            }
        }
    }
#ifdef _X86_
    __asm
    {    
        mov     ecx, g.m_comp;
        mov     edi, g.m_verts;
        mov     esi, v;
        mov     eax, g.m_vcnt[ecx * 4];
        lea     edx, [eax + 1];
        mov     g.m_vcnt[ecx * 4], edx;
        lea     edx, [ecx * 8 + 28];
        mul     edx;
        cld;
        add     edi, eax;
		  xor     eax, eax;
        movsd;
        movsd;
        stosd;
        lea     esi, g.m_nx;
        lea     ecx, [ecx * 2 + 4];
        rep     movsd;
    }
#else
    D3DVALUE *d3dv = &(((D3DVALUE*)g.m_verts)[g.m_vcnt[g.m_comp]++ * (g.m_comp * 2 + 7)]);
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    *(d3dv++) = 0.f;
    memcpy(d3dv, &g.m_nx, (sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + g.m_comp*sizeof(D3DVALUE)*2));
#endif
}

GLAPI void GLAPIENTRY glVertex3fv (const GLfloat *v)
{
    if(g.m_nfv[g.m_comp] + g.m_vcnt[g.m_comp] >= (VBUFSIZE - MAXVERTSPERPRIM))
    {
        if(g.m_prim == GL_TRIANGLES)
        {
            if(g.m_vcnt[g.m_comp] % 3 == 0)
            {
                glEnd();
                glBegin(g.m_prim);
            }
        }
        else if(g.m_prim == GL_QUADS)
        {
            if(g.m_vcnt[g.m_comp] % 4 == 0)
            {
                glEnd();
                glBegin(g.m_prim);
            }
        }
        else if(g.m_prim == GL_LINES)
        {
            if(g.m_vcnt[g.m_comp] % 2 == 0)
            {
                glEnd();
                glBegin(g.m_prim);
            }
        }
    }
#ifdef _X86_
    __asm
    {    
        mov     ecx, g.m_comp;
        mov     edi, g.m_verts;
        mov     esi, v;
        mov     eax, g.m_vcnt[ecx * 4];
        lea     edx, [eax + 1];
        mov     g.m_vcnt[ecx * 4], edx;
        lea     edx, [ecx * 8 + 28];
        mul     edx;
        cld;
        add     edi, eax;
        movsd;
        movsd;
        movsd;
        lea     esi, g.m_nx;
        lea     ecx, [ecx * 2 + 4];
        rep     movsd;
    }
#else
    D3DVALUE *d3dv = &(((D3DVALUE*)g.m_verts)[g.m_vcnt[g.m_comp]++ * (g.m_comp * 2 + 7)]);
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    *(d3dv++) = *(v++);
    memcpy(d3dv, &g.m_nx, (sizeof(D3DVALUE)*3 + sizeof(D3DCOLOR) + g.m_comp*sizeof(D3DVALUE)*2));
#endif
}

GLAPI void GLAPIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{ 
	if (size == 3 && type == GL_FLOAT)
	{
		g.m_vertexary = (GLfloat *) pointer;
		g.m_vertarysize = 3;
		if (stride)
			g.m_vertexstride = stride;
		else
			g.m_vertexstride = 12;
	}
	else if (size == 2 && type == GL_FLOAT)
	{
		g.m_vertexary = (GLfloat *) pointer;
		g.m_vertarysize = 2;
		if (stride)
			g.m_vertexstride = stride;
		else
			g.m_vertexstride = 8;
	}
	else
	{
		char buf[128];

		sprintf(buf,"Wrapper: Vertex array not supported (size: %d  type: %x  stride: %d)\n",
					size, type, stride);
		OutputDebugString(buf);
	}
}

GLAPI void GLAPIENTRY glRecti (GLint x1, GLint y1, GLint x2, GLint y2)
{ 
	glBegin(GL_POLYGON);

	glVertex2f((GLfloat) x1, (GLfloat) y1);
	glVertex2f((GLfloat) x2, (GLfloat) y1);
	glVertex2f((GLfloat) x2, (GLfloat) y2);
	glVertex2f((GLfloat) x1, (GLfloat) y2);

	glEnd();
}

GLAPI void GLAPIENTRY glMultMatrixf (const GLfloat *m)
{ 
	if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
		g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_curtexmatrix);

	g.m_d3ddev->MultiplyTransform(g.m_matrixMode, (LPD3DMATRIX) m);

	if (g.m_matrixMode == D3DTRANSFORMSTATE_WORLD)
		g.m_inversedirty = TRUE;
	else if (g.m_matrixMode == D3DTRANSFORMSTATE_TEXTURE0)
	{
		g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_TEXTURE0, &g.m_curtexmatrix);
		g.m_objectdirty[0] = TRUE;
	}
}

GLAPI GLboolean GLAPIENTRY glIsEnabled (GLenum cap)
{ 
	DWORD enabled;

	switch (cap)
	{
		case GL_BLEND:
			g.m_d3ddev->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &enabled);
			break;
		case GL_TEXTURE_2D:
			if (g.m_curtgt == 0)
            enabled = g.m_texturing;
			else
            enabled = g.m_mtex;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			enabled = g.m_usetexcoordary[g.m_client_active_texture_arb];
			break;
		case GL_COLOR_ARRAY:
			enabled = g.m_usecolorary;
			break;
		case GL_VERTEX_ARRAY:
			enabled = g.m_usevertexary;
			break;
		case GL_NORMAL_ARRAY:
			enabled = g.m_usenormalary;
			break;
		case GL_FOG_COORDINATE_ARRAY_EXT:
			enabled = g.m_usefogary;
			break;
		case GL_LIGHTING:
			enabled = g.m_lighting;
			break;
      case GL_CULL_FACE:
         enabled = g.m_cullEnabled;
         break;   
		case GL_LIGHT0:
		case GL_LIGHT1:
		case GL_LIGHT2:
		case GL_LIGHT3:
		case GL_LIGHT4:
		case GL_LIGHT5:
		case GL_LIGHT6:
		case GL_LIGHT7:
			{
				BOOL lightOn;

				g.m_d3ddev->GetLightEnable(cap-GL_LIGHT0,&lightOn);
				enabled = lightOn;
			}
			break;
		default:
			{		
				char buf[64];

				sprintf(buf,"Wrapper: unsupported glIsEnabled query: %x", cap);	
				OutputDebugString(buf);
		
				enabled = false;
			}
			break;
	}

	return enabled;
}

GLAPI void GLAPIENTRY glGetTexEnviv (GLenum target, GLenum pname, GLint *params)
{ 
	target;

	if (pname == GL_TEXTURE_ENV_MODE)
		*params = g.m_blendmode[g.m_curtgt];
	else
		OutputDebugString("Wrapper: GL_TEXTURE_ENV_COLOR not implemented\n");
}

GLAPI void GLAPIENTRY glFrontFace (GLenum mode)
{ 
	g.m_frontFace = mode;
	if (g.m_cullEnabled == TRUE)
	{
		DWORD statevalue;

		if ((g.m_cullMode == GL_BACK && mode == GL_CCW) ||
			 (g.m_cullMode == GL_FRONT && mode == GL_CW))
			statevalue=D3DCULL_CW;
		else
			statevalue=D3DCULL_CCW;
		g.m_d3ddev->SetRenderState(D3DRENDERSTATE_CULLMODE, statevalue);
	}
}

GLAPI void GLAPIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param)
{
	coord;
	pname;

	g.m_texgenmode[g.m_curtgt] = param;
	if (g.m_texgen[g.m_curtgt] && param == GL_OBJECT_LINEAR)
	{
		g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXCOORDINDEX,
													g.m_curtgt | D3DTSS_TCI_CAMERASPACEPOSITION);
		g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	}
	else
	{
		g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXCOORDINDEX,
													g.m_curtgt | D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	  	g.m_d3ddev->SetTextureStageState(g.m_curtgt, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	}
}

GLAPI void GLAPIENTRY glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params)
{
	pname;

	switch (coord)
	{
		case GL_S:
			memcpy(g.m_texgenplane[g.m_curtgt][0],params,sizeof(GLfloat)*4);
			g.m_objectdirty[g.m_curtgt] = TRUE;
			break;
		case GL_T:
			memcpy(g.m_texgenplane[g.m_curtgt][1],params,sizeof(GLfloat)*4);
			g.m_objectdirty[g.m_curtgt] = TRUE;
			break;
		default:
			{
				char buf[64];

				sprintf(buf,"Wrapper: TexGen coordinate unsupported: %x\n", coord);
				OutputDebugString(buf);
			}
			break;
	}
}

GLAPI void GLAPIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	if (count == 0)
		return;

	unsigned i;
	BOOL textrans = (g.m_dd.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ||
						 (g.m_texgen[0] && g.m_texgenmode[0] == GL_OBJECT_LINEAR) ||
		 				 (g.m_texgen[1] && g.m_texgenmode[1] == GL_SPHERE_MAP);
	BOOL unlock = g.m_lckcount == 0 || g.m_tgtdirty;

	QuakeSetTexturingState();

	if (g.m_lckcount == 0)
		fillVB(false,first,count);
	else
		if (g.m_tgtdirty)
			if (textrans)
				fillVB(false,g.m_lckfirst,g.m_lckcount);
			else
				fillVB(true,g.m_lckfirst,g.m_lckcount);

	if (g.m_lckcount != 0 && !textrans)
	{
		if (g.m_usefogary)
		{
			if (mode == GL_TRIANGLES)
				g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_fmtvtgt, first, count, 0);
			else
			{
  	      	char buf[64];
               	
				sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
				OutputDebugString(buf);
			}
		}
		else if (g.m_texturing)
		{
			if (g.m_mtex)
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_mtvtgt, first, count, 0);
  	      			break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_mtvtgt, first, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_mtvtgt, first, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvtgt, first, count, 0);
						break;
					case GL_QUADS:
						for (i = 0; i < count; i += 4)
							g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvtgt, first+i, 4, 0);	
        				break;
					default:
					{
        				char buf[64];
        	       	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    			}
			else
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_tvtgt, first, count, 0);
        				break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_tvtgt, first, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_tvtgt, first, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvtgt, first, count, 0);
						break;
					case GL_QUADS:
						for (i = 0; i < count; i += 4) 
							g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvtgt, first+i, 4, 0);
     	   			break;
					default:
					{
     	   			char buf[64];
     	          	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    			}
		}
		else
			switch (mode) 
			{
				case GL_LINES:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_vtgt, first, count, 0);
  	     			break;
				case GL_TRIANGLES:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_vtgt, first, count, 0);
					break;
				case GL_TRIANGLE_STRIP:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_vtgt, first, count, 0);
					break;
				case GL_POLYGON:
				case GL_TRIANGLE_FAN:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vtgt, first, count, 0);
					break;
				case GL_QUADS:
					for (i = 0; i < count; i += 4) 
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vtgt, first+i, 4, 0);
					break;
				default:
				{
  	   			char buf[64];
              	
					sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
					OutputDebugString(buf);
				}
  			}
	}
	else
	{
		GLint foffset;

		if (g.m_lckcount == 0)
			foffset = 0;
		else
			foffset = first;

		if (g.m_usefogary)
		{
			if (unlock)
				g.m_fmtvbuf->Unlock();
			if (mode == GL_TRIANGLES)
				g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_fmtvbuf, g.m_nfv[3]+foffset, count, 0);
			else
			{
  	      	char buf[64];
               	
				sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
				OutputDebugString(buf);
			}
			if (g.m_lckcount == 0)
				g.m_nfv[3] += count;
		}
		else if (g.m_texturing)
		{
			if (g.m_mtex)
			{
				if (unlock)
					g.m_mtvbuf->Unlock();
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_mtvbuf, g.m_nfv[2]+foffset, count, 0);
  	      			break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_mtvbuf, g.m_nfv[2]+foffset, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_mtvbuf, g.m_nfv[2]+foffset, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvbuf, g.m_nfv[2]+foffset, count, 0);
						break;
					case GL_QUADS:
						{
							unsigned vindex = g.m_nfv[2];
		
							for (i = 0; i < count; i += 4) 
							{
								g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_mtvbuf, vindex+foffset, 4, 0);
								vindex += 4;
							}
						}
        				break;
					default:
					{
        				char buf[64];
        	       	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    			}
				if (g.m_lckcount == 0)
					g.m_nfv[2] += count;
			}
			else
			{
				if (unlock)
					g.m_tvbuf->Unlock();
				switch (mode) 
				{
					case GL_LINES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_tvbuf, g.m_nfv[1]+foffset, count, 0);
        				break;
					case GL_TRIANGLES:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_tvbuf, g.m_nfv[1]+foffset, count, 0);
						break;
					case GL_TRIANGLE_STRIP:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_tvbuf, g.m_nfv[1]+foffset, count, 0);
						break;
					case GL_POLYGON:
					case GL_TRIANGLE_FAN:
						g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvbuf, g.m_nfv[1]+foffset, count, 0);
						break;
					case GL_QUADS:
						{
							unsigned vindex = g.m_nfv[1];
			
							for (i = 0; i < count; i += 4) 
							{
								g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_tvbuf, vindex+foffset, 4, 0);
								vindex += 4;
							}
						}
     	   			break;
					default:
					{
     	   			char buf[64];
     	          	
						sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
						OutputDebugString(buf);
					}
    			}
				if (g.m_lckcount == 0)
					g.m_nfv[1] += count;
			}
		}
		else
		{
			if (unlock)
				g.m_vbuf->Unlock();
			switch (mode) 
			{
				case GL_LINES:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_LINELIST, g.m_vbuf, g.m_nfv[0]+foffset, count, 0);
  	     			break;
				case GL_TRIANGLES:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLELIST, g.m_vbuf, g.m_nfv[0]+foffset, count, 0);
					break;
				case GL_TRIANGLE_STRIP:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLESTRIP, g.m_vbuf, g.m_nfv[0]+foffset, count, 0);
					break;
				case GL_POLYGON:
				case GL_TRIANGLE_FAN:
					g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vbuf, g.m_nfv[0]+foffset, count, 0);
					break;
				case GL_QUADS:
					{
						unsigned vindex = g.m_nfv[0];

						for (i = 0; i < count; i += 4) 
						{
							g.m_d3ddev->DrawPrimitiveVB(D3DPT_TRIANGLEFAN, g.m_vbuf, vindex+foffset, 4, 0);
							vindex += 4;
						}
					}
  	   			break;
				default:
				{
  	   			char buf[64];
              	
					sprintf(buf,"Wrapper: Unimplemented primitive type: %x\n",mode);
					OutputDebugString(buf);
				}
  			}
			if (g.m_lckcount == 0)
				g.m_nfv[0] += count;
		}
	}
}

GLAPI void GLAPIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{ 
	target;
	pname;
	params;

	static float two55 = 255.f;
	unsigned int R, G, B, A;
#ifdef _X86_
    __asm {
        mov ebx, params;
        fld [ebx];
        fld [ebx + 4];
        fld [ebx + 8];
        fld [ebx + 12];
        fld two55;
        fmul st(1), st(0);
        fmul st(2), st(0);
        fmul st(3), st(0);
        fmulp st(4), st(0);
        fistp A;
        fistp B;
        fistp G;
        fistp R;
        mov edx, A;
        cmp edx, 255;
        jle pt1;
        mov edx, 255;
pt1:    mov eax, B;
        cmp eax, 255;
        jle pt2;
        mov eax, 255;
pt2:    mov ebx, G;
        cmp ebx, 255;
        jle pt3;
        mov ebx, 255;
pt3:    mov ecx, R;
        cmp ecx, 255;
        jle pt4;
        mov ecx, 255;
pt4:    shl ebx, 8;
        shl ecx, 16;
        shl edx, 24;
        or eax, ebx;
        or ecx, edx;
        or eax, ecx;
        mov g.m_envcolor, eax;
    }
#else
	R = (unsigned int) (params[0] * two55);
	G = (unsigned int) (params[1] * two55);
	B = (unsigned int) (params[2] * two55);
	A = (unsigned int) (params[3] * two55);
	if (R > 255)
		R = 255;
	if (G > 255)
		G = 255;
	if (B > 255)
		B = 255;
	if (A > 255)
		A = 255;
	g.m_envcolor = RGBA_MAKE(R, G, B, A);
#endif
}

GLAPI void GLAPIENTRY glTexCoord2fv (const GLfloat *v)
{
	g.m_tu = v[0];
	g.m_tv = v[1];
}

GLAPI void GLAPIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{ 
	type;

	if (stride)
		g.m_normalstride = stride;
	else
		g.m_normalstride = 12;
	g.m_normalary = (const GLfloat *) pointer;
}

GLAPI void GLAPIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{ 
	face;
	pname;
	params;

	switch (pname)
	{
		case GL_SPECULAR:
		case GL_EMISSION:
			// TSShow tool uses these
			break;
		case GL_AMBIENT:
			memcpy(&g.m_material.ambient,params,sizeof(GLfloat)*4);
			break;
		case GL_DIFFUSE:
			memcpy(&g.m_material.diffuse,params,sizeof(GLfloat)*4);
			break;
		case GL_AMBIENT_AND_DIFFUSE:
			memcpy(&g.m_material.ambient,params,sizeof(GLfloat)*4);
			memcpy(&g.m_material.diffuse,params,sizeof(GLfloat)*4);
			break;
		default:
			{
				char buf[64];

				sprintf(buf,"Wrapper: Materialfv pname not supported: %x\n", pname);
				OutputDebugString(buf);
			}
			break;
	}
	g.m_d3ddev->SetMaterial(&g.m_material);
}

GLAPI void GLAPIENTRY glLightf (GLenum light, GLenum pname, GLfloat param)
{ 
	GLenum i = light - GL_LIGHT0;

	switch (pname)
	{
		case GL_SPOT_CUTOFF:
			if (param >= 180.0)
			{
				g.m_lights[i].dltType = (g.m_lights[i].dltType == D3DLIGHT_DIRECTIONAL) ? D3DLIGHT_DIRECTIONAL : D3DLIGHT_POINT;
				g.m_lights[i].dvTheta = g.m_lights[i].dvPhi = M_PI/2.0;
			}
			else
			{
				g.m_lights[i].dltType = (g.m_lights[i].dltType == D3DLIGHT_DIRECTIONAL) ? D3DLIGHT_DIRECTIONAL : D3DLIGHT_SPOT;
				g.m_lights[i].dvTheta = g.m_lights[i].dvPhi = mDegToRad(param*2.0);
			}
			break;
		case GL_CONSTANT_ATTENUATION:
			g.m_lights[i].dvAttenuation0 = param;
			break;
		case GL_LINEAR_ATTENUATION:
			g.m_lights[i].dvAttenuation1 = param;
			break;
		case GL_QUADRATIC_ATTENUATION:
			g.m_lights[i].dvAttenuation2 = param;
			break;
	}

	g.m_d3ddev->SetLight(i,&g.m_lights[i]);
}

GLAPI void GLAPIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params)
{ 
	GLenum i = light - GL_LIGHT0;

	switch (pname)
	{
		case GL_POSITION:
			{
				D3DXMATRIX world;

				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_WORLD, (D3DMATRIX *) &world);

				if (params[3] == 0.0)
				{
					D3DXVECTOR4 mdir(params), wdir;

					g.m_lights[i].dltType = D3DLIGHT_DIRECTIONAL;
					
					// HACK: why this works I'll never understand
					// I thought you would transform the light's direction
					// by the inverse of the current modelview...but that
					// doesn't work.  What the hell?!?!?
					
					D3DXVec4Transform(&wdir,&mdir,&world);
					g.m_lights[i].dvDirection.x = -wdir.x;
					g.m_lights[i].dvDirection.y = -wdir.y;
					g.m_lights[i].dvDirection.z = -wdir.z;
				}
				else
				{
					D3DXVECTOR4 mpos(params), wpos;

					g.m_lights[i].dltType = (g.m_lights[i].dvPhi == M_2PI) ? D3DLIGHT_POINT : D3DLIGHT_SPOT;
					D3DXVec4Transform(&wpos,&mpos,&world);
					memcpy(&g.m_lights[i].dvPosition,wpos,sizeof(GLfloat)*3);
				}
			}
			break;
		case GL_SPOT_DIRECTION:
			{
				D3DXMATRIX world;
				D3DXVECTOR4 mdir(params), wdir;

				// HACK: why this works I'll never understand
				// I thought you would transform the light's direction
				// by the inverse of the current modelview...but that
				// doesn't work.  What the hell?!?!?
				g.m_d3ddev->GetTransform(D3DTRANSFORMSTATE_WORLD, (D3DMATRIX *) &world);
				D3DXVec4Transform(&wdir,&mdir,&world);
				g.m_lights[i].dvDirection.x = -wdir.x;
				g.m_lights[i].dvDirection.y = -wdir.y;
				g.m_lights[i].dvDirection.z = -wdir.z;
			}
			break;
		case GL_DIFFUSE:
			memcpy(&g.m_lights[i].dcvDiffuse,params,sizeof(GLfloat)*3);
			break;
	  	case GL_AMBIENT:
			memcpy(&g.m_lights[i].dcvAmbient,params,sizeof(GLfloat)*3);
			break;
		case GL_SPECULAR:
			memcpy(&g.m_lights[i].dcvSpecular,params,sizeof(GLfloat)*3);
			break;
	}

	g.m_d3ddev->SetLight(i,&g.m_lights[i]);
}

GLAPI void GLAPIENTRY glLightModelfv (GLenum pname, const GLfloat *params)
{ 
	if (pname == GL_LIGHT_MODEL_AMBIENT)
	{
		DWORD ambient;

		static float two55 = 255.f;
		unsigned int R, G, B, A;

#ifdef _X86_
		__asm {
			mov ebx, params;
			fld [ebx];
			fld [ebx + 4];
			fld [ebx + 8];
			fld [ebx + 12];
			fld two55;
			fmul st(1), st(0);
			fmul st(2), st(0);
			fmul st(3), st(0);
			fmulp st(4), st(0);
			fistp A;
			fistp B;
			fistp G;
			fistp R;
			mov edx, A;
			cmp edx, 255;
			jle pt1;
			mov edx, 255;
	pt1:	mov eax, B;
			cmp eax, 255;
			jle pt2;
			mov eax, 255;
	pt2:	mov ebx, G;
			cmp ebx, 255;
			jle pt3;
			mov ebx, 255;
	pt3:	mov ecx, R;
			cmp ecx, 255;
			jle pt4;
			mov ecx, 255;
	pt4:	shl ebx, 8;
			shl ecx, 16;
			shl edx, 24;
			or eax, ebx;
			or ecx, edx;
			or eax, ecx;
			mov ambient, eax;
		}
#else
		R = (unsigned int) (params[0] * two55);
		G = (unsigned int) (params[1] * two55);
		B = (unsigned int) (params[2] * two55);
		A = (unsigned int) (params[3] * two55);
		if (R > 255)
			R = 255;
		if (G > 255)
			G = 255;
		if (B > 255)
			B = 255;
		if (A > 255)
			A = 255;
		ambient = RGBA_MAKE(R, G, B, A);
#endif
		g.m_d3ddev->SetRenderState(D3DRENDERSTATE_AMBIENT, ambient);
	}
	else
	{
		char buf[64];

		sprintf(buf,"Wrapper:  Unsupported LightModelfv pname: %x\n", pname);
		OutputDebugString(buf);
	}
}

GLAPI void GLAPIENTRY glFogCoordPointerEXT(GLenum type, GLsizei stride, void *pointer)
{
	if (type == GL_FLOAT)
	{
		if (stride)
			g.m_fogstride = stride;
		else
			g.m_fogstride = 4;
		g.m_fogary = (GLfloat *) pointer;
	}
	else
		OutputDebugString("Wrapper: Unsupported FogCoordPointerEXT array type\n");
}

GLAPI void GLAPIENTRY glPointSize (GLfloat size)
{ 
	size;
}

GLAPI void GLAPIENTRY glColorTableEXT(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void* data)
{
	target;
	width;
	format;
	type;

	TexInfo &ti = g.m_tex[g.m_curstagebinding[0]];

   if (internalFormat == GL_RGB || internalFormat == GL_RGB8)
	{
		UCHAR *rgb = (UCHAR *) data;

		for (int i = 0; i < 256; ++i)
		{
			g.m_paletteentries[i].peRed = *(rgb++);
			g.m_paletteentries[i].peGreen = *(rgb++);		
			g.m_paletteentries[i].peBlue = *(rgb++);
			rgb++;
		}

		if (!ti.m_palette)
		{
			HRESULT hr = g.m_dddev->CreatePalette( DDPCAPS_8BIT|DDPCAPS_ALLOW256, g.m_paletteentries, &ti.m_palette, NULL );
			
			if ( FAILED(hr) )
			{
				OutputDebugString("Wrapper: CreatePalette failed\n");

				return;
			}
		}	

		ti.m_palette->SetEntries(0,0,256,g.m_paletteentries);

		ti.m_rgbaindexed = false;
	}
	else
	{
		UCHAR *rgba = (UCHAR *) data;
		WORD *rgba4 = g.m_rgbapalette;

		for (int i = 0; i < 256; ++i, rgba += 4)
			*(rgba4++) = ((rgba[3] & 0xF0) << 8) | ((rgba[0] & 0xF0) << 4) | (rgba[1] & 0xF0) | ((rgba[2] & 0xF0) >> 4);
		
		ti.m_rgbaindexed = true;
	}
}

GLAPI void GLAPIENTRY glReadBuffer (GLenum mode)
{
	if (mode != GL_FRONT)
		OutputDebugString("Wrapper: Unsupported glReadBuffer mode\n");
}

GLAPI void GLAPIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	x;
	y;
	width;
	height;
	format;
	type;
	pixels;

	HRESULT hr;
	DDSURFACEDESC2 mddsd2, vddsd2;
	LPDIRECTDRAWSURFACE7 lpDDS;

	vddsd2.dwSize = sizeof(DDSURFACEDESC2);
	g.m_pD3DX->GetPrimary()->GetSurfaceDesc(&vddsd2);

	int bpp = vddsd2.ddpfPixelFormat.dwRGBBitCount/8;
	char *temp = new char[width*bpp*height];
 
	// Initialize the surface description.
	ZeroMemory(&mddsd2, sizeof(DDSURFACEDESC2));
	mddsd2.dwSize = sizeof(DDSURFACEDESC2);
	mddsd2.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_LPSURFACE |
						  DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
	
	mddsd2.dwWidth = width;
	mddsd2.dwHeight = height;
	mddsd2.lPitch = (LONG) width*bpp;
	mddsd2.lpSurface = temp;
	mddsd2.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

	memcpy(&mddsd2.ddpfPixelFormat,&vddsd2.ddpfPixelFormat,sizeof(DDPIXELFORMAT));

	RECT src;

	GetWindowRect(g.m_hwnd,&src);

	int xadj = ((src.right-src.left)-width)/2;
	int yadj = (src.bottom-src.top)-height;

	src.left += xadj;
	src.right -= xadj;
	src.top += (yadj-xadj);
	src.bottom -= xadj;
 
	// Create the surface
	hr = g.m_dddev->CreateSurface(&mddsd2, &lpDDS, NULL);
	if (FAILED(hr))
	{
		OutputDebugString("Wrapper: Couldn't create surface for glReadPixels blit\n");
		delete [] temp;

		return;
	}
	hr = lpDDS->BltFast(0,0,g.m_pD3DX->GetPrimary(),&src,DDBLTFAST_WAIT | DDBLTFAST_NOCOLORKEY);
	lpDDS->Release();

	switch (bpp)
	{
		case 2:
			{
				WORD *spixels = (WORD *) temp;

				for (int y = 1; y <= height; ++y)
				{
					char *dpixels = (char *) pixels + ((height-y)*width*3);

					for (int x = 0; x < width; ++x)
					{
						*(dpixels++) = (*spixels & 0xF800) >> 8;
						*(dpixels++) = (*spixels & 0x7E0) >> 3;
						*(dpixels++) = (*spixels & 0x1F) << 3;
						spixels++;
					}
				}
			}
			break;
		case 3:
			{
				char *spixels = temp;

				// I'm guessing on this one that little/big endian shouldn't make no difference
				for (int y = 1; y <= height; ++y)
				{
					char *dpixels = (char *) pixels + ((height-y)*width*3);
		
					memcpy(dpixels,spixels,width*3);
					spixels += width*3;
				}
			}
			break;
		case 4:
			{
				DWORD *spixels = (DWORD *) temp;

				for (int y = 1; y <= height; ++y)
				{
					char *dpixels = (char *) pixels + ((height-y)*width*3);
		
					for (int x = 0; x < width; ++x)
					{
						*(dpixels++) = (*spixels & 0xFF0000) >> 16;
						*(dpixels++) = (*spixels & 0xFF00) >> 8;
						*(dpixels++) = *spixels & 0xFF;
						spixels++;
					}
				}
			}
			break;
	}
	delete [] temp;		
}

GLAPI void GLAPIENTRY glPixelStorei (GLenum pname, GLint param)
{
	pname;
	param;
}

GLAPI GLboolean GLAPIENTRY glAvailableVertexBufferEXT()
{
	return (!g.m_lowVidMemory && g.m_freeVBuffers.length());
}

GLAPI GLint GLAPIENTRY glAllocateVertexBufferEXT(GLsizei size, GLint format, GLboolean preserve)
{
	if (g.m_lowVidMemory || !g.m_freeVBuffers.length())
	{
		OutputDebugString("Wrapper: Out of vertex buffers\n");
		
		return -1;
	}

	GLListManip<GLuint> freeVBuffers(&g.m_freeVBuffers);
	GLint handle = freeVBuffers();
	BufferInfo &binfo = g.m_vbuffers[handle];
	D3DVERTEXBUFFERDESC vbufdesc;

	vbufdesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
	vbufdesc.dwNumVertices = size;
	if (g.m_dd.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		vbufdesc.dwCaps = D3DVBCAPS_WRITEONLY;
	else
		vbufdesc.dwCaps = D3DVBCAPS_SYSTEMMEMORY;
	switch (format)
	{
		case GL_V12MTVFMT_EXT:
			vbufdesc.dwFVF = V12MTVFMT;
			break;
		case GL_V12MTNVFMT_EXT:
			vbufdesc.dwFVF = V12MTNVFMT;
			break;
		case GL_V12FTVFMT_EXT:
			vbufdesc.dwFVF = V12FTVFMT;
			break;
		case GL_V12FMTVFMT_EXT:
			vbufdesc.dwFVF = V12FMTVFMT;
			break;
	}

	if (size == 0)
		binfo.buf = 0;
	else
	{
		HRESULT hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &binfo.buf, 0);

		if (FAILED(hr))
		{
			OutputDebugString("Wrapper: Low video memory\n");
			g.m_lowVidMemory = TRUE;

			return -1;
		}
	}

	binfo.size = size;
	binfo.format = format;
	binfo.preserve = preserve;
	freeVBuffers.remove();

	return handle;
}

GLAPI void* GLAPIENTRY glLockVertexBufferEXT(GLint handle, GLsizei size)
{
	BufferInfo &binfo = g.m_vbuffers[handle];

	if (!binfo.buf)
		return NULL;

	if (binfo.preserve)
	{
		binfo.buf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY, &binfo.verts, 0);
		binfo.nfv = 0;
	}
	else
		if (binfo.nfv > (binfo.size - size))	// check if space available
		{
			binfo.buf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_OKTOSWAP, &binfo.verts, 0);
			binfo.nfv = 0;
		}
		else
		{
			binfo.buf->Lock(DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOOVERWRITE, &binfo.verts, 0);
			switch (binfo.format)
			{
				case GL_V12MTVFMT_EXT:
					binfo.verts = &(((V12MTVertex *) binfo.verts)[binfo.nfv]);
					break;
				case GL_V12MTNVFMT_EXT:
					binfo.verts = &(((V12MTNVertex *) binfo.verts)[binfo.nfv]);
					break;
				case GL_V12FTVFMT_EXT:
					binfo.verts = &(((V12FTVertex *) binfo.verts)[binfo.nfv]);
					break;
				case GL_V12FMTVFMT_EXT:
					binfo.verts = &(((V12FMTVertex *) binfo.verts)[binfo.nfv]);
					break;
			}
		}

	return binfo.verts;
}

GLAPI void GLAPIENTRY glUnlockVertexBufferEXT(GLint handle)
{
	BufferInfo &binfo = g.m_vbuffers[handle];

	if (binfo.buf)
	{
		binfo.buf->Unlock();
		binfo.verts = 0;	
	}
}

GLAPI void GLAPIENTRY glSetVertexBufferEXT(GLint handle)
{
	BufferInfo &binfo = g.m_vbuffers[handle];

	if (binfo.preserve)
	{
		g.m_d3ddev->SetRenderState(D3DRENDERSTATE_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
		g.m_d3ddev->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
	}

	g.m_useVBuffer = handle;
}

GLAPI void GLAPIENTRY glOffsetVertexBufferEXT(GLint handle, GLuint offset)
{
	BufferInfo &binfo = g.m_vbuffers[handle];

	binfo.nfv = offset;
}

GLAPI void GLAPIENTRY glFillVertexBufferEXT(GLint handle, GLint first, GLsizei count)
{
	BufferInfo &binfo = g.m_vbuffers[handle];
	
	g.m_useVBuffer = handle;
	fillVB_NC(first, count);
	g.m_useVBuffer = -1;
	binfo.nfv += count;			
}

GLAPI void GLAPIENTRY glFreeVertexBufferEXT(GLint handle)
{
	GLListManip<GLuint> freeVBuffers(&g.m_freeVBuffers);
	BufferInfo &binfo = g.m_vbuffers[handle];

	binfo.size = 0;
	binfo.format = 0;
	binfo.preserve = FALSE;
	binfo.nfv = 0;
	if (binfo.buf)
	{
		binfo.buf->Release();
		binfo.buf = 0;
	}
	if (binfo.verts)
		OutputDebugString("Wrapper: Locked vertex buffer being freed\n");

	freeVBuffers.insert(handle);
	if (g.m_lowVidMemory)
	{
		OutputDebugString("Wrapper: Video memory made available\n");
		g.m_lowVidMemory = FALSE;	
	}
}

GLAPI void GLAPIENTRY glFinish (void)
{
	g.m_d3ddev->BeginScene();
}

GLAPI int GLAPIENTRY wglChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
	hdc;
	ppfd;

    return 1;
}

static LRESULT CALLBACK MyMsgHandler(
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
	if (uMsg == WM_SIZE)
	{
		g.m_winWidth = LOWORD(lParam);
		g.m_winHeight = HIWORD(lParam);
	}

	return CallWindowProc(g.m_wndproc, hwnd, uMsg, wParam, lParam); 
}

GLAPI HGLRC GLAPIENTRY wd3dCreateContext(HDC hdc)
{
    g.m_hdc = hdc;
    g.m_hwnd = WindowFromDC(g.m_hdc);

    RECT rect;
    BOOLEAN found = FALSE;
    DWORD texfmts = 0;

    GetClientRect(g.m_hwnd, &rect);
    g.m_winWidth = (USHORT)rect.right;
    g.m_winHeight = (USHORT)rect.bottom;
    g.m_vwx = rect.left;
    g.m_vwy = rect.top;
    g.m_vww = rect.right - rect.left;
    g.m_vwh = rect.bottom - rect.top;
    
    HRESULT hr = D3DXInitialize();

    if (FAILED(hr))
    {
        return 0;
    }
    
    DWORD DeviceType = D3DX_DEFAULT;
    // Check registry key to see if we need to do software emulation	
    HKEY hKey;

    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_QUAKE, &hKey)) {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, "Emulation", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0) {
            DeviceType = D3DX_HWLEVEL_2D;
        }
        RegCloseKey( hKey );
    }
    
    D3DX_VIDMODEDESC VidMode;

	hr = D3DXGetCurrentVideoMode(DeviceType, &VidMode);
	if (FAILED(hr))
    	goto fail1d;

	// See if the window is full screen
	if ((DWORD) rect.right == VidMode.width && (DWORD) rect.bottom == VidMode.height)
		// We are full screen
		hr = D3DXCreateContextEx(DeviceType, D3DX_CONTEXT_FULLSCREEN, g.m_hwnd, g.m_hwnd,
										 VidMode.bpp, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
										 VidMode.width, VidMode.height, VidMode.refreshRate, &g.m_pD3DX);
	else
		hr = D3DXCreateContext(DeviceType, 0, g.m_hwnd, D3DX_DEFAULT, D3DX_DEFAULT, &g.m_pD3DX);
	if (FAILED(hr))
		goto fail1d;
    
	// Keep a copy of the D3D device
	g.m_d3ddev = g.m_pD3DX->GetD3DDevice();
	if (g.m_d3ddev == NULL)
		goto fail1c;
	// Keep a copy of the DD device
	g.m_dddev = g.m_pD3DX->GetDD();
	if (g.m_dddev == NULL)
		goto fail1b;
	// Get a D3D ptr to create the vertex buffer.
	g.m_pD3D = g.m_pD3DX->GetD3D();
	if (g.m_pD3D == NULL)
		goto fail1a;

	hr = g.m_d3ddev->GetCaps(&g.m_dd);
	if (FAILED(hr))
		goto fail1;

    // Check registry key to see if we need to turn off mipmapping
    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_QUAKE, &hKey)) {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, "DisableMipMap", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0) {
            g.m_usemipmap = FALSE;
            OutputDebugString("Wrapper: Mipmapping disabled\n");
        }
        else {
            g.m_usemipmap = TRUE;
        }
        RegCloseKey( hKey );
    }
    else {
        g.m_usemipmap = TRUE;
    }

    // Enumerate texture formats and find the right ones to use
    texfmts = D3DXGetMaxSurfaceFormats(DeviceType, NULL, D3DX_SC_COLORTEXTURE);
    // Look for a four bit alpha surface
    found = FALSE;
    DWORD i;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &g.m_ddFourBitAlphaSurfFormat);
        if( FAILED(hr) )
        		goto fail1;
        if(g.m_ddFourBitAlphaSurfFormat == D3DX_SF_A4R4G4B4) 
        {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE )
    {
        OutputDebugString("Wrapper: Unable to find 4444 texture.\n");
        goto fail1;
    }
    // Look for a eight bit alpha surface
    found = FALSE;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &g.m_ddEightBitAlphaSurfFormat);
        if( FAILED(hr) )
        		goto fail1;
        if(g.m_ddEightBitAlphaSurfFormat == D3DX_SF_A8R8G8B8) 
        {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE )
    {
        OutputDebugString("Wrapper: Not using 8888 texture\n");
        g.m_ddEightBitAlphaSurfFormat = g.m_ddFourBitAlphaSurfFormat;
    }
    // Look for a surface
    found = FALSE;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &g.m_ddFiveBitSurfFormat);
        if( FAILED(hr) )
        		goto fail1;
        if(g.m_ddFiveBitSurfFormat == D3DX_SF_R5G5B5 || g.m_ddFiveBitSurfFormat == D3DX_SF_R5G6B5) 
        {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE )
    {
        OutputDebugString("Wrapper: Unable to find 555 or 565 texture.\n");
        goto fail1;
    }
    // Look for a 8-bit surface
    found = FALSE;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &g.m_ddEightBitSurfFormat);
        if( FAILED(hr) )
        		goto fail1;
        if(g.m_ddEightBitSurfFormat == D3DX_SF_X8R8G8B8) 
        {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE )
    {
		OutputDebugString("Wrapper: Not using 888 texture\n");
		if (g.m_ddEightBitAlphaSurfFormat == D3DX_SF_A8R8G8B8)
			g.m_ddEightBitSurfFormat = g.m_ddEightBitAlphaSurfFormat;
		else
			g.m_ddEightBitSurfFormat = g.m_ddFiveBitSurfFormat;
    }
    // Look for a luminance surface
    found = FALSE;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &g.m_ddLuminanceSurfFormat);
        if( FAILED(hr) )
        		goto fail1;
        if(g.m_ddLuminanceSurfFormat == D3DX_SF_L8) 
        {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE )
    {
        OutputDebugString("Wrapper: Not using luminance texture\n");
        g.m_ddLuminanceSurfFormat = g.m_ddEightBitAlphaSurfFormat;
    }
	 // Look for an alpha surface
    found = FALSE;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &g.m_ddAlphaSurfFormat);
        if( FAILED(hr) )
        		goto fail1;
        if(g.m_ddAlphaSurfFormat == D3DX_SF_A8) 
        {
            found = TRUE;
            break;
        }
    }
    if ( found == FALSE )
    {
        OutputDebugString("Wrapper: Not using alpha texture\n");
        g.m_ddAlphaSurfFormat = g.m_ddEightBitAlphaSurfFormat;
    }
	 // Look for a palettized surface
	 D3DX_SURFACEFORMAT tmpformat;

    found = FALSE;
    for(i = 0; i < texfmts; ++i)
    {
        hr = D3DXGetSurfaceFormat(DeviceType, NULL, D3DX_SC_COLORTEXTURE, i, &tmpformat);
        if( FAILED(hr) )
        		goto fail1;
        if(tmpformat == D3DX_SF_PALETTE8) 
        {
            found = TRUE;
				g.m_colortable = TRUE;

            break;
        }
    }
    if ( found == FALSE )
    {
        OutputDebugString("Wrapper: No palette textures -- no GL_EXT_paletted_texture\n");
        g.m_colortable = FALSE;
    }                   

    // Do misc init stuff
    if(g.m_dd.dwMaxTextureWidth < 512 || g.m_dd.dwMaxTextureHeight < 512) {
        g.m_subsample = TRUE;
        OutputDebugString("Wrapper: Subsampling textures to 256 x 256\n");
    }
    else
        g.m_subsample = FALSE;
    if(g.m_dd.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) {
        g.m_makeSquare = TRUE;
        OutputDebugString("Wrapper: Forcing all textures to be square\n");
    }
    else
        g.m_makeSquare = FALSE;
    if(g.m_dd.wMaxSimultaneousTextures > 1) {
        g.m_usemtex = TRUE;
        OutputDebugString("Wrapper: Multitexturing enabled\n");
    }
    else {
        g.m_usemtex = FALSE;
        OutputDebugString("Wrapper: Multitexturing not available with this driver\n");
    }
    if(!(g.m_dd.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR) &&
        !(g.m_dd.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPNEAREST) &&
        !(g.m_dd.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR) &&
        !(g.m_dd.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPNEAREST)) {
        g.m_usemipmap = FALSE;
        OutputDebugString("Wrapper: Mipmapping disabled\n");
    }
	if (!(g.m_dd.dwVertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS))
	{
		g.m_usedirectional = FALSE;
		OutputDebugString("Wrapper: Directional lights disabled\n");
	}
	else
		g.m_usedirectional = TRUE;
    if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_QUAKE, &hKey)) {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = 4;
        if (ERROR_SUCCESS == RegQueryValueEx( hKey, "DoFlip", NULL, &dwType, (LPBYTE) &dwValue, &dwSize) &&
            dwType == REG_DWORD &&
            dwValue != 0) {
            g.m_doFlip = TRUE;
        }
        else {
            g.m_doFlip = FALSE;
        }
        RegCloseKey( hKey );
    }
    else {
        g.m_doFlip = FALSE;
    }
    
	// Create vertex buffers
    	
	D3DVERTEXBUFFERDESC vbufdesc;

	vbufdesc.dwSize = sizeof(D3DVERTEXBUFFERDESC);
	vbufdesc.dwNumVertices = VBUFSIZE;
	vbufdesc.dwCaps = D3DVBCAPS_WRITEONLY;

	if (!(g.m_dd.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
		vbufdesc.dwCaps = D3DVBCAPS_SYSTEMMEMORY;

	vbufdesc.dwFVF = QUAKEFMTVFMT;
	hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_fmtvbuf, 0);
	if( FAILED(hr) )
		goto fail1;
	vbufdesc.dwFVF = QUAKEMTVFMT;
	hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_mtvbuf, 0);
	if ( FAILED(hr) )
		goto fail2;
	vbufdesc.dwFVF = QUAKETVFMT;
	hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_tvbuf, 0);
	if ( FAILED(hr) )
    	goto fail3;
	vbufdesc.dwFVF = QUAKEVFMT;
	hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_vbuf, 0);
	if ( FAILED(hr) )
		goto fail4;

	if (!(g.m_dd.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
	{
		vbufdesc.dwFVF = QUAKETRFMTVFMT;
		hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_fmtvtgt, 0);
		if( FAILED(hr) )
			goto fail5;
		vbufdesc.dwFVF = QUAKETRMTVFMT;
		hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_mtvtgt, 0);
		if ( FAILED(hr) )
			goto fail6;
		vbufdesc.dwFVF = QUAKETRTVFMT;
		hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_tvtgt, 0);
		if ( FAILED(hr) )
			goto fail7;
		vbufdesc.dwFVF = QUAKETRVFMT;
		hr = g.m_pD3D->CreateVertexBuffer(&vbufdesc, &g.m_vtgt, 0);
		if ( FAILED(hr) )
			goto fail8;
	}
	
	goto success;

fail8:
	g.m_tvtgt->Release();
fail7:
	g.m_mtvtgt->Release();
fail6:
	g.m_fmtvtgt->Release();
fail5:
	g.m_vbuf->Release();
fail4:
	g.m_tvbuf->Release();
fail3:
	g.m_mtvbuf->Release();
fail2:
	g.m_fmtvbuf->Release();
fail1:
	g.m_pD3D->Release();
fail1a:
	g.m_dddev->Release();
fail1b:
	g.m_d3ddev->Release();
fail1c:
	g.m_pD3DX->Release();
fail1d:
	D3DXUninitialize();

	return 0;

success:    
	// Some more init stuff
	g.m_cullMode = GL_BACK;
	g.m_cullEnabled = FALSE;
	 
	g.m_texHandleValid = FALSE;
	g.m_texturing = FALSE;
	g.m_updvwp = TRUE;
	g.m_blendmode[0] = GL_MODULATE;
	g.m_blendmode[1] = GL_MODULATE;
	g.m_nfv[0] = g.m_nfv[1] = g.m_nfv[2] = g.m_nfv[3] = 0;
	g.m_curtgt = 0;
	g.m_client_active_texture_arb = 0;
	g.m_nx = 0.0;
	g.m_ny = 0.0;
	g.m_nz = 1.0;
	g.m_color = 0xFFFFFFFF;
	g.m_tu = g.m_tv = g.m_tu2 = g.m_tv2 = 0.f;
	g.m_clearColor = 0;
	g.m_clearDepth = 1.f;
	g.m_usecolorary = FALSE;
	g.m_usetexcoordary[0] = FALSE;
	g.m_usetexcoordary[1] = FALSE;
	g.m_usevertexary = FALSE;
	g.m_scix = 0;
	g.m_sciy = 0;
	g.m_sciw = g.m_winWidth;
	g.m_scih = g.m_winHeight;
	g.m_lckfirst = 0;
	g.m_lckcount = 0;
	g.m_numIndices = 1024;
	g.m_wIndices = new WORD[g.m_numIndices];
	g.m_frontFace = GL_CCW;
	g.m_usenormalary = FALSE;
	g.m_normalstride = 0;
	g.m_texcoordstride[0] = 0;
	g.m_texcoordstride[1] = 0;
	g.m_texgen[0] = FALSE;
	g.m_texgen[1] = FALSE;
	g.m_texgenmode[0] = GL_EYE_LINEAR;
	g.m_texgenmode[0] = GL_EYE_LINEAR;
	g.m_texgenplane[0][0][0] = 1.0;
	g.m_texgenplane[0][0][1] = 0.0;
	g.m_texgenplane[0][0][2] = 0.0;
	g.m_texgenplane[0][0][3] = 0.0;
	g.m_texgenplane[0][1][0] = 0.0;
	g.m_texgenplane[0][1][1] = 1.0;
	g.m_texgenplane[0][1][2] = 0.0;
	g.m_texgenplane[0][1][3] = 0.0;
	g.m_texgenplane[1][0][0] = 1.0;
	g.m_texgenplane[1][0][1] = 0.0;
	g.m_texgenplane[1][0][2] = 0.0;
	g.m_texgenplane[1][0][3] = 0.0;
	g.m_texgenplane[1][1][0] = 0.0;
	g.m_texgenplane[1][1][1] = 1.0;
	g.m_texgenplane[1][1][2] = 0.0;
	g.m_texgenplane[1][1][3] = 0.0;
	g.m_inversedirty = TRUE;
	g.m_objectdirty[0] = TRUE;
	g.m_objectdirty[1] = TRUE;
	g.m_envcolor = 0;
	g.m_colorstride = 0;
	g.m_colortype = GL_FLOAT;
	g.m_zbias = 0;
	g.m_fogcolor = 0;
	g.m_usefogary = FALSE;
	g.m_fogstride = 0;
	g.m_tgtdirty = TRUE;
	g.m_lighting = FALSE;
	g.m_incdex = -1;
	g.m_source2rgbext = GL_CONSTANT_EXT;
	g.m_vertarysize = 0;
	g.m_useVBuffer = -1;
	g.m_lowVidMemory = FALSE;

	// Initialize the structure.
	ZeroMemory(g.m_lights, sizeof(D3DLIGHT7));
 
	// Set up for a white point light.
	g.m_lights[0].dltType = D3DLIGHT_DIRECTIONAL;
	g.m_lights[0].dcvDiffuse.r = 1.0f;
	g.m_lights[0].dcvDiffuse.g = 1.0f;
	g.m_lights[0].dcvDiffuse.b = 1.0f;
	g.m_lights[0].dcvAmbient.r = 0.0f;
	g.m_lights[0].dcvAmbient.g = 0.0f;
	g.m_lights[0].dcvAmbient.b = 0.0f;
	g.m_lights[0].dcvSpecular.r = 1.0f;
	g.m_lights[0].dcvSpecular.g = 1.0f;
	g.m_lights[0].dcvSpecular.b = 1.0f;
 
    // Position it high in the scene, and behind the viewer.
    // (Remember, these coordinates are in world space, so
    //  the "viewer" could be anywhere in world space, too. 
    //  For the purposes of this example, assume the viewer
    //  is at the origin of world space.)
	g.m_lights[0].dvPosition.x = 0.0f;
	g.m_lights[0].dvPosition.y = 0.0f;
	g.m_lights[0].dvPosition.z = 1.0f;
	g.m_lights[0].dvDirection.x = 0.0f;
	g.m_lights[0].dvDirection.y = 0.0f;
	g.m_lights[0].dvDirection.z = -1.0f;
 
	// Don't attenuate.
	g.m_lights[0].dvAttenuation0 = 1.0f; 
	g.m_lights[0].dvRange = D3DLIGHT_RANGE_MAX;

	g.m_lights[0].dvTheta = g.m_lights[0].dvPhi = M_PI/4.0;
	g.m_lights[0].dvFalloff = 1.0;
	g.m_lights[0].dvRange = D3DLIGHT_RANGE_MAX;

	for (i = 1; i < 8; ++i)
	{
		D3DLIGHT7 &d3dLight = g.m_lights[i];

		// Initialize the structure.
		ZeroMemory(&d3dLight, sizeof(D3DLIGHT7));
 
		// Set up for a white point light.
		d3dLight.dltType = D3DLIGHT_DIRECTIONAL;
		d3dLight.dcvDiffuse.r = 0.0f;
		d3dLight.dcvDiffuse.g = 0.0f;
		d3dLight.dcvDiffuse.b = 0.0f;
		d3dLight.dcvAmbient.r = 0.0f;
		d3dLight.dcvAmbient.g = 0.0f;
		d3dLight.dcvAmbient.b = 0.0f;
		d3dLight.dcvSpecular.r = 0.0f;
		d3dLight.dcvSpecular.g = 0.0f;
		d3dLight.dcvSpecular.b = 0.0f;
 
		// Position it high in the scene, and behind the viewer.
		// (Remember, these coordinates are in world space, so
		//  the "viewer" could be anywhere in world space, too. 
		//  For the purposes of this example, assume the viewer
		//  is at the origin of world space.)
		d3dLight.dvPosition.x = 0.0f;
		d3dLight.dvPosition.y = 0.0f;
		d3dLight.dvPosition.z = 1.0f;
		d3dLight.dvDirection.x = 0.0f;
		d3dLight.dvDirection.y = 0.0f;
		d3dLight.dvDirection.z = -1.0f;
 
		// Don't attenuate.
		d3dLight.dvAttenuation0 = 1.0f; 
		d3dLight.dvRange = D3DLIGHT_RANGE_MAX;

		d3dLight.dvTheta = d3dLight.dvPhi = M_PI/2.0;
		d3dLight.dvFalloff = 1.0;
		d3dLight.dvRange = D3DLIGHT_RANGE_MAX;
	}

	ZeroMemory(g.m_paletteentries,256*sizeof(PALETTEENTRY));
	ZeroMemory(g.m_rgbapalette,256*sizeof(WORD));

	g.m_curtexmatrix._11 = 1.0f; g.m_curtexmatrix._12 = 0.0f; g.m_curtexmatrix._13 = 0.0f; g.m_curtexmatrix._14 = 0.0f;
	g.m_curtexmatrix._21 = 0.0f; g.m_curtexmatrix._22 = 1.0f; g.m_curtexmatrix._23 = 0.0f; g.m_curtexmatrix._24 = 0.0f;
	g.m_curtexmatrix._31 = 0.0f; g.m_curtexmatrix._32 = 0.0f; g.m_curtexmatrix._33 = 1.0f; g.m_curtexmatrix._34 = 0.0f;
	g.m_curtexmatrix._41 = 0.0f; g.m_curtexmatrix._42 = 0.0f; g.m_curtexmatrix._43 = 0.0f; g.m_curtexmatrix._44 = 1.0f;

	g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE0,&g.m_curtexmatrix);

	D3DMATRIX sphere;

	sphere._11 = 0.5; sphere._12 = 0.0; sphere._13 = 0.0; sphere._14 = 0.0;
	sphere._21 = 0.0; sphere._22 = 0.5; sphere._23 = 0.0; sphere._24 = 0.0;
	sphere._31 = 0.0; sphere._32 = 0.0; sphere._33 = 0.0; sphere._34 = 0.0;
	sphere._41 = 0.5; sphere._42 = 0.5; sphere._43 = 0.0; sphere._44 = 0.0;
				
	g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_TEXTURE1,&sphere);

	g.m_material.ambient.r = 0.2;
	g.m_material.ambient.g = 0.2;
	g.m_material.ambient.b = 0.2;
	g.m_material.ambient.a = 1.0;
	g.m_material.diffuse.r = 0.8;
	g.m_material.diffuse.g = 0.8;
	g.m_material.diffuse.b = 0.8;
	g.m_material.diffuse.a = 1.0;
	g.m_material.specular.r = 0.0;
	g.m_material.specular.g = 0.0;
	g.m_material.specular.b = 0.0;
	g.m_material.specular.a = 1.0;
	g.m_material.emissive.r = 0.0;
	g.m_material.emissive.g = 0.0;
	g.m_material.emissive.b = 0.0;
	g.m_material.emissive.a = 1.0;
	g.m_material.power = 0.0;

	g.m_d3ddev->SetMaterial(&g.m_material);

	 GLListManip<GLuint> freeTextures(&g.m_freeTextures);

	for (i = 0; i < MAXGLTEXHANDLES; ++i) {
		g.m_tex[i].m_ddsurf = 0;
		g.m_tex[i].m_block = 0;
		g.m_tex[i].m_capture = FALSE;
		g.m_tex[i].m_dwStage = 0;
		g.m_tex[i].m_minmode = D3DTFN_POINT;
		g.m_tex[i].m_magmode = D3DTFG_LINEAR;
		g.m_tex[i].m_mipmode = D3DTFP_LINEAR;
		g.m_tex[i].m_addu = D3DTADDRESS_WRAP;
		g.m_tex[i].m_addv = D3DTADDRESS_WRAP;
		g.m_tex[i].m_rgbaindexed = FALSE; 
    	g.m_tex[i].m_palette = 0;

		if (i)
			freeTextures.insert(i);
    }

	GLListManip<GLuint> freeVBuffers(&g.m_freeVBuffers);

	for (i = 0; i < MAX_VBUFFERS; ++i)
	{
		g.m_vbuffers[i].size = 0;
		g.m_vbuffers[i].format = 0;
		g.m_vbuffers[i].preserve = FALSE;
		g.m_vbuffers[i].nfv = 0;
		g.m_vbuffers[i].buf = 0;
		g.m_vbuffers[i].verts = 0;

		freeVBuffers.insert(i);
	}

    D3DMATRIX unity;
    unity._11 = 1.0f; unity._12 = 0.0f; unity._13 = 0.0f; unity._14 = 0.0f;
    unity._21 = 0.0f; unity._22 = 1.0f; unity._23 = 0.0f; unity._24 = 0.0f;
    unity._31 = 0.0f; unity._32 = 0.0f; unity._33 = 1.0f; unity._34 = 0.0f;
    unity._41 = 0.0f; unity._42 = 0.0f; unity._43 = 0.0f; unity._44 = 1.0f;
    g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_VIEW, &unity); 
    g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_WORLD, &unity); 
    g.m_d3ddev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &unity); 
    g.m_curtex[0] = NULL;
    g.m_curtex[1] = NULL;
    g.m_d3ddev->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
    g.m_d3ddev->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
    g.m_d3ddev->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE);
    g.m_d3ddev->SetRenderState(D3DRENDERSTATE_CLIPPING, TRUE);
    g.m_d3ddev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
	 g.m_d3ddev->SetRenderState(D3DRENDERSTATE_AMBIENT, 0x32323219);
	 g.m_d3ddev->SetRenderState(D3DRENDERSTATE_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
    g.m_d3ddev->SetRenderState(D3DRENDERSTATE_EXTENTS, FALSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_TEXCOORDINDEX,0);
    if(g.m_usemtex == TRUE)
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_TEXCOORDINDEX,1);
    
    // Create shaders
    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][0]);                

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][1]);

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][2]);

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][3]);

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][4]);

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][5]);

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][6]);

    g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][7]);

	 g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][8]);

	 g.m_d3ddev->BeginStateBlock();
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    // following stage state to speedup software rasterizer
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    g.m_d3ddev->SetTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g.m_d3ddev->EndStateBlock(&g.m_shaders[0][9]);

    if(g.m_usemtex)
    {
        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][0]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][1]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][2]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][3]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][4]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][5]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        // following stage state to speedup software rasterizer
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][6]);

        g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE | D3DTA_COMPLEMENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][7]);

		  g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_BLENDFACTORALPHA);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][8]);

		  g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_BLENDCURRENTALPHA);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][9]);

		  g.m_d3ddev->BeginStateBlock();
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLORARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        g.m_d3ddev->SetTextureStageState (1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
        g.m_d3ddev->EndStateBlock(&g.m_shaders[1][10]);
    }

    // Hook into message loop
	 //
	 // Disabled because we're not using it and alt-tabbing causes a crash (still hooked
	 // up when the DLL unloads
	 //
    //g.m_wndproc = (WNDPROC)SetWindowLong(g.m_hwnd, GWL_WNDPROC, (LONG)MyMsgHandler);
    
    // Start a scene
    g.m_d3ddev->BeginScene();
    
    return (HGLRC)1;
}

GLAPI BOOL GLAPIENTRY wd3dDeleteContext(HGLRC hglrc)
{
	hglrc;

    g.m_d3ddev->EndScene();
    //SetWindowLong(g.m_hwnd, GWL_WNDPROC, (LONG)g.m_wndproc);
    int i;
    for(i = 0; i < MAXGLTEXHANDLES; ++i){
        if(g.m_tex[i].m_ddsurf != 0) {
            g.m_tex[i].m_ddsurf->Release();
            g.m_tex[i].m_ddsurf = 0;
            if(g.m_tex[i].m_block != 0)
            {
                g.m_d3ddev->DeleteStateBlock(g.m_tex[i].m_block);
                g.m_tex[i].m_block = 0;
            }
            g.m_tex[i].m_capture = FALSE;
				g.m_tex[i].m_rgbaindexed = FALSE;
				if (g.m_tex[i].m_palette)
				{
					g.m_tex[i].m_palette->Release();
					g.m_tex[i].m_palette = 0;
				}
        }
    }

	GLListManip<GLuint> freeTextures(&g.m_freeTextures);

	while (g.m_freeTextures.length())
		freeTextures.remove();

	GLListManip<GLuint> freeVBuffers(&g.m_freeVBuffers);

	while (g.m_freeVBuffers.length())
		freeVBuffers.remove();

	for (i = 0; i < 10; ++i)
		g.m_d3ddev->DeleteStateBlock(g.m_shaders[0][i]);
	if (g.m_usemtex)
		for (i = 0; i < 11; ++i)
			g.m_d3ddev->DeleteStateBlock(g.m_shaders[1][i]);

	if (!(g.m_dd.dwDevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
	{
		g.m_vtgt->Release();
		g.m_vtgt = 0;
		g.m_tvtgt->Release();
		g.m_tvtgt = 0;
		g.m_mtvtgt->Release();
		g.m_mtvtgt = 0;
		g.m_fmtvtgt->Release();
		g.m_fmtvtgt = 0;
	}
	g.m_vbuf->Release();
	g.m_vbuf = 0;
	g.m_tvbuf->Release();
	g.m_tvbuf = 0;
	g.m_mtvbuf->Release();
	g.m_mtvbuf = 0;
	g.m_fmtvbuf->Release();
	g.m_fmtvbuf = 0;
	
	g.m_pD3D->Release();
	g.m_pD3D = 0;
	g.m_dddev->Release();
	g.m_dddev = 0;
	g.m_d3ddev->Release();
	g.m_d3ddev = 0;
	g.m_pD3DX->Release();
 	g.m_pD3DX = 0;

	D3DXUninitialize();
	delete [] g.m_wIndices;
	g.m_wIndices = 0;

	return TRUE;
}

GLAPI int GLAPIENTRY wglDescribePixelFormat(HDC hdc, INT iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd)
{
	hdc;
	iPixelFormat;
	nBytes;

    HRESULT hr = D3DXInitialize();
    if (FAILED(hr))
        return 0;

    D3DX_VIDMODEDESC VidMode;
    hr = D3DXGetCurrentVideoMode(D3DX_DEFAULT, &VidMode);
    if (FAILED(hr))
    {
        D3DXUninitialize();
        return 0;
    }

    D3DXUninitialize();

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_GENERIC_ACCELERATED;
    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = (unsigned char)VidMode.bpp;    
    ppfd->cAccumBits = 0;
    ppfd->cAccumRedBits = 0;
    ppfd->cAccumGreenBits = 0;
    ppfd->cAccumBlueBits = 0;
    ppfd->cAccumAlphaBits = 0;
    ppfd->cStencilBits = 0;
    ppfd->cAuxBuffers = 0;
    ppfd->iLayerType = 0;
    ppfd->bReserved = 0;
    ppfd->dwLayerMask = 0;
    ppfd->dwVisibleMask = 0;
    ppfd->dwDamageMask = 0;
    if(VidMode.bpp == 16)
    {
        ppfd->cRedBits = 5;
        ppfd->cRedShift = 11;
        ppfd->cGreenBits = 6;
        ppfd->cGreenShift = 5;
        ppfd->cBlueBits = 5;
        ppfd->cBlueShift = 0;
        ppfd->cAlphaBits = 0;
        ppfd->cAlphaShift = 0;
        ppfd->cDepthBits = 16;
    }
    else if(VidMode.bpp == 24 || VidMode.bpp == 32)
    {
        ppfd->cRedBits = 8;
        ppfd->cRedShift = 16;
        ppfd->cGreenBits = 8;
        ppfd->cGreenShift = 8;
        ppfd->cBlueBits = 8;
        ppfd->cBlueShift = 0;
        ppfd->cAlphaBits = 0;
        ppfd->cAlphaShift = 0;
        ppfd->cDepthBits = 32;
    }
    else
    {
        return 0;
    }

    return 1;
} 

GLAPI HGLRC GLAPIENTRY wd3dGetCurrentContext(VOID)
{
    return (HGLRC)1;
}

GLAPI HDC GLAPIENTRY wd3dGetCurrentDC(VOID)
{
    return g.m_hdc;
}

GLAPI int GLAPIENTRY wglGetPixelFormat(HDC hdc)
{
	hdc;

	return 1;
}

GLAPI PROC GLAPIENTRY wd3dGetProcAddress(LPCSTR str)
{
	if(strcmp(str, "glMTexCoord2fSGIS") == 0)
		return (PROC) glMTexCoord2fSGIS;
	else if(strcmp(str, "glSelectTextureSGIS") == 0)
		return (PROC) glSelectTextureSGIS;
	else if(strcmp(str, "glActiveTextureARB") == 0)
		return (PROC) glActiveTextureARB;
	else if(strcmp(str, "glClientActiveTextureARB") == 0)
		return (PROC) glClientActiveTextureARB;
	else if(strcmp(str, "glMultiTexCoord2fARB") == 0)
		return (PROC) glMultiTexCoord2fARB;
	else if(strcmp(str, "glMultiTexCoord2fvARB") == 0)
		return (PROC) glMultiTexCoord2fvARB;
	else if(strcmp(str, "glLockArraysEXT") == 0)
		return (PROC) glLockArraysEXT;
	else if(strcmp(str, "glUnlockArraysEXT") == 0)
		return (PROC) glUnlockArraysEXT;
	else if(strcmp(str, "glFogCoordfEXT") == 0)
		return (PROC) glFogCoordfEXT;
	else if(strcmp(str, "glFogCoordPointerEXT") == 0)
		return (PROC) glFogCoordPointerEXT;	
	else if(strcmp(str, "glColorTableEXT") == 0)
		return (PROC) glColorTableEXT;
	else if(strcmp(str, "glAvailableVertexBufferEXT") == 0)
		return (PROC) glAvailableVertexBufferEXT;
	else if(strcmp(str, "glAllocateVertexBufferEXT") == 0)
		return (PROC) glAllocateVertexBufferEXT;
	else if(strcmp(str, "glLockVertexBufferEXT") == 0)
		return (PROC) glLockVertexBufferEXT;
	else if(strcmp(str, "glUnlockVertexBufferEXT") == 0)
		return (PROC) glUnlockVertexBufferEXT;
	else if(strcmp(str, "glSetVertexBufferEXT") == 0)
		return (PROC) glSetVertexBufferEXT;	
	else if(strcmp(str, "glOffsetVertexBufferEXT") == 0)
		return (PROC) glOffsetVertexBufferEXT;
	else if(strcmp(str, "glFillVertexBufferEXT") == 0)
		return (PROC) glFillVertexBufferEXT;
	else if(strcmp(str, "glFreeVertexBufferEXT") == 0)
		return (PROC) glFreeVertexBufferEXT;
	else
	{
		OutputDebugString("Wrapper: Unimplemented function ");
		OutputDebugString(str);
		OutputDebugString("\n");
	}

	return NULL;
}

GLAPI BOOL GLAPIENTRY wd3dMakeCurrent(HDC hdc, HGLRC hglrc)
{
	hdc;
	hglrc;

	return TRUE;
}

GLAPI BOOL GLAPIENTRY wglSetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
	hdc;
	iPixelFormat;
	ppfd;

	return TRUE;
}

GLAPI BOOL GLAPIENTRY wglSwapBuffers(HDC hdc)
{
	hdc;

#ifdef USEICECAP
	static unsigned nframes = 0;
	++nframes;
	if(nframes == 1000)
	{
		OutputDebugString("Started profiling\n");
		StartCAP();
	}
	if(nframes == 1500)
	{
		StopCAP();
		OutputDebugString("stopped profiling\n");
	}
#endif

	g.m_d3ddev->EndScene();
	while (g.m_pD3DX->UpdateFrame(g.m_doFlip ? 0 : D3DX_UPDATE_NOVSYNC) == DDERR_SURFACEBUSY) {}

	return TRUE;
}


//////////////////////////////////////////// NOT USED by QuakeGL ///////////////////////////////////////////////////////

#define DODPFS

static void DPF(char *str)
{
	OutputDebugString(str);
	OutputDebugString("\n");
}

GLAPI void GLAPIENTRY glAccum (GLenum op, GLfloat value)
{ 
	op;
	value;

	#ifdef DODPFS 
	DPF("glAccum"); 
	#endif //DODPFS
}

GLAPI GLboolean GLAPIENTRY glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences)
{ 
	n;
	textures;
	residences;

	GLboolean dummy = FALSE;

	#ifdef DODPFS 
	DPF("glAreTexturesResident"); 
	#endif //DODPFS

	return dummy;
}

GLAPI void GLAPIENTRY glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{ 
	width;
	height;
	xorig;
	yorig;
	xmove;
	ymove;
	bitmap;

	#ifdef DODPFS 
	DPF("glBitmap"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glCallList (GLuint list)
{ 
	list;

	#ifdef DODPFS 
	DPF("glCallList"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glCallLists (GLsizei n, GLenum type, const GLvoid *lists)
{ 
	n;
	type;
	lists;

	#ifdef DODPFS 
	DPF("glCallLists"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glClearAccum"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glClearIndex (GLfloat c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glClearIndex"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glClearStencil (GLint s)
{ 
	s;

	#ifdef DODPFS 
	DPF("glClearStencil"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3b (GLbyte red, GLbyte green, GLbyte blue)
{ 
	red;
	green;
	blue;

	#ifdef DODPFS 
	DPF("glColor3b"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3bv (const GLbyte *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3bv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3d (GLdouble red, GLdouble green, GLdouble blue)
{ 
	red;
	green;
	blue;

	#ifdef DODPFS 
	DPF("glColor3d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3i (GLint red, GLint green, GLint blue)
{ 
    unsigned int R, G, B;
    R = (unsigned int)(red);
    G = (unsigned int)(green);
    B = (unsigned int)(blue);
    if(R > 255)
        R = 255;
    if(G > 255)
        G = 255;
    if(B > 255)
        B = 255;
    g.m_color = RGBA_MAKE(R, G, B, 255);
}

GLAPI void GLAPIENTRY glColor3iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3s (GLshort red, GLshort green, GLshort blue)
{ 
	red;
	green;
	blue;

	#ifdef DODPFS 
	DPF("glColor3s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3ui (GLuint red, GLuint green, GLuint blue)
{ 
	red;
	green;
	blue;

	#ifdef DODPFS 
	DPF("glColor3ui"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3uiv (const GLuint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3uiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3us (GLushort red, GLushort green, GLushort blue)
{ 
	red;
	green;
	blue;

	#ifdef DODPFS 
	DPF("glColor3us"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor3usv (const GLushort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor3usv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColor4b"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4bv (const GLbyte *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor4bv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColor4d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor4dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4i (GLint red, GLint green, GLint blue, GLint alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColor4i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor4iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColor4s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor4sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColor4ui"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4uiv (const GLuint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor4uiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColor4us"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColor4usv (const GLushort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glColor4usv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{ 
	red;
	green;
	blue;
	alpha;

	#ifdef DODPFS 
	DPF("glColorMask"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glColorMaterial (GLenum face, GLenum mode)
{ 
	face;
	mode;

	#ifdef DODPFS 
	DPF("glColorMaterial"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{ 
	x;
	y;
	width;
	height;
	type;

	#ifdef DODPFS 
	DPF("glCopyPixels"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{ 
	target;
	level;
	internalFormat;
	x;
	y;
	width;
	border;

	#ifdef DODPFS 
	DPF("glCopyTexImage1D"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{ 
	target;
	level;
	internalFormat;
	x;
	y;
	width;
	height;
	border;

	#ifdef DODPFS 
	DPF("glCopyTexImage2D"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{ 
	target;
	level;
	xoffset;
	x;
	y;
	width;

	#ifdef DODPFS 
	DPF("glCopyTexSubImage1D"); 
	#endif	//DODPFS
}

GLAPI void GLAPIENTRY glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{ 
	target;
	level;
	xoffset;
	yoffset;
	x;
	y;
	width;
	height;

	#ifdef DODPFS 
	DPF("glCopyTexSubImage2D"); 
	#endif	//DODPFS
}

GLAPI void GLAPIENTRY glDeleteLists (GLuint list, GLsizei range)
{ 
	list;
	range;

	#ifdef DODPFS 
	DPF("glDeleteLists"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{ 
	width;
	height;
	format;
	type;
	pixels;

	#ifdef DODPFS 
	DPF("glDrawPixels"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEdgeFlag (GLboolean flag)
{ 
	flag;

	#ifdef DODPFS 
	DPF("glEdgeFlag"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer)
{ 
	stride;
	pointer;

	#ifdef DODPFS 
	DPF("glEdgeFlagPointer"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEdgeFlagv (const GLboolean *flag)
{ 
	flag;

	#ifdef DODPFS 
	DPF("glEdgeFlagv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEndList (void)
{ 
	#ifdef DODPFS 
	DPF("glEndList"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord1d (GLdouble u)
{ 
	u;

	#ifdef DODPFS 
	DPF("glEvalCoord1d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord1dv (const GLdouble *u)
{ 
	u;

	#ifdef DODPFS 
	DPF("glEvalCoord1dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord1f (GLfloat u)
{ 
	u;

	#ifdef DODPFS 
	DPF("glEvalCoord1f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord1fv (const GLfloat *u)
{ 
	u;

	#ifdef DODPFS 
	DPF("glEvalCoord1fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord2d (GLdouble u, GLdouble v)
{ 
	u;
	v;

	#ifdef DODPFS 
	DPF("glEvalCoord2d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord2dv (const GLdouble *u)
{ 
	u;

	#ifdef DODPFS 
	DPF("glEvalCoord2dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord2f (GLfloat u, GLfloat v)
{ 
	u;
	v;

	#ifdef DODPFS 
	DPF("glEvalCoord2f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalCoord2fv (const GLfloat *u)
{ 
	u;

	#ifdef DODPFS 
	DPF("glEvalCoord2fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalMesh1 (GLenum mode, GLint i1, GLint i2)
{ 
	mode;
	i1;
	i2;

	#ifdef DODPFS 
	DPF("glEvalMesh1"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{ 
	mode;
	i1;
	i2;
	j1;
	j2;

	#ifdef DODPFS 
	DPF("glEvalMesh2"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalPoint1 (GLint i)
{ 
	i;

	#ifdef DODPFS 
	DPF("glEvalPoint1"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glEvalPoint2 (GLint i, GLint j)
{ 
	i;
	j;

	#ifdef DODPFS 
	DPF("glEvalPoint2"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer)
{ 
	size;
	type;
	buffer;

	#ifdef DODPFS 
	DPF("glFeedbackBuffer"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glFlush (void)
{ 
	#ifdef DODPFS 
	DPF("glFlush"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glFogiv (GLenum pname, const GLint *params)
{ 
	pname;
	params;

	#ifdef DODPFS 
	DPF("glFogiv"); 
	#endif //DODPFS
}

GLAPI GLuint GLAPIENTRY glGenLists (GLsizei range)
{ 
	range;

	GLuint dummy = 0;
	#ifdef DODPFS 
	DPF("glGenLists"); 
	#endif //DODPFS
	return dummy;
}

GLAPI void GLAPIENTRY glGetBooleanv (GLenum pname, GLboolean *params)
{ 
	pname;
	params;
	
	#ifdef DODPFS 
	DPF("glGetBooleanv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetClipPlane (GLenum plane, GLdouble *equation)
{ 
	plane;
	equation;

	#ifdef DODPFS 
	DPF("glGetClipPlane"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{ 
	light;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetLightfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetLightiv (GLenum light, GLenum pname, GLint *params)
{ 
	light;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetLightiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetMapdv (GLenum target, GLenum query, GLdouble *v)
{ 
	target;
	query;
	v;

	#ifdef DODPFS 
	DPF("glGetMapdv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetMapfv (GLenum target, GLenum query, GLfloat *v)
{ 
	target;
	query;
	v;

	#ifdef DODPFS 
	DPF("glGetMapfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetMapiv (GLenum target, GLenum query, GLint *v)
{ 
	target;
	query;
	v;

	#ifdef DODPFS 
	DPF("glGetMapiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{ 
	face;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetMaterialfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetMaterialiv (GLenum face, GLenum pname, GLint *params)
{ 
	face;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetMaterialiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetPixelMapfv (GLenum map, GLfloat *values)
{ 
	map;
	values;

	#ifdef DODPFS 
	DPF("glGetPixelMapfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetPixelMapuiv (GLenum map, GLuint *values)
{ 
	map;
	values;

	#ifdef DODPFS 
	DPF("glGetPixelMapuiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetPixelMapusv (GLenum map, GLushort *values)
{ 
	map;
	values;

	#ifdef DODPFS 
	DPF("glGetPixelMapusv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetPointerv (GLenum pname, GLvoid* *params)
{ 
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetPointerv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetPolygonStipple (GLubyte *mask)
{ 
	mask;

	#ifdef DODPFS 
	DPF("glGetPolygonStipple"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params)
{ 
	target;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexEnvfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params)
{ 
	coord;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexGendv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params)
{
	coord;
	pname;
	params;

	#ifdef DODPFS
	DPF("glGetTexGenfv");
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexGeniv (GLenum coord, GLenum pname, GLint *params)
{ 
	coord;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexGeniv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{ 
	target;
	level;
	format;
	type;
	pixels;

	#ifdef DODPFS 
	DPF("glGetTexImage"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params)
{ 
	target;
	level;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexLevelParameterfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params)
{ 
	target;
	level;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexLevelParameteriv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params)
{ 
	target;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexParameterfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glGetTexParameteriv (GLenum target, GLenum pname, GLint *params)
{ 
	target;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glGetTexParameteriv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glHint (GLenum target, GLenum mode)
{
	target;
	mode;

	#ifdef DODPFS 
	//DPF("glHint(%X, %X)",target,mode); 
	DPF("glHint");
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexMask (GLuint mask)
{ 
	mask;

	#ifdef DODPFS 
	DPF("glIndexMask"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{ 
	type;
	stride;
	pointer;

	#ifdef DODPFS 
	DPF("glIndexPointer"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexd (GLdouble c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexd"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexdv (const GLdouble *c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexdv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexf (GLfloat c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexf"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexfv (const GLfloat *c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexi (GLint c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexi"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexiv (const GLint *c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexs (GLshort c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexs"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexsv (const GLshort *c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexsv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexub (GLubyte c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexub"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glIndexubv (const GLubyte *c)
{ 
	c;

	#ifdef DODPFS 
	DPF("glIndexubv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glInitNames (void)
{ 
	#ifdef DODPFS 
	DPF("glInitNames"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{ 
	format;
	stride;
	pointer;

	#ifdef DODPFS 
	DPF("glInterleavedArrays"); 
	#endif //DODPFS
}

GLAPI GLboolean GLAPIENTRY glIsList (GLuint list)
{ 
	list;

	GLboolean dummy = FALSE;
	#ifdef DODPFS 
	DPF("glIsList"); 
	#endif //DODPFS
	return dummy;
}

GLAPI GLboolean GLAPIENTRY glIsTexture (GLuint texture)
{ 
	texture;

	GLboolean dummy = FALSE;
	#ifdef DODPFS 
	DPF("glIsTexture"); 
	#endif //DODPFS
	return dummy;
}

GLAPI void GLAPIENTRY glLightModelf (GLenum pname, GLfloat param)
{ 
	pname;
	param;

	#ifdef DODPFS 
	DPF("glLightModelf"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLightModeli (GLenum pname, GLint param)
{ 
	pname;
	param;

	#ifdef DODPFS 
	DPF("glLightModeli"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLightModeliv (GLenum pname, const GLint *params)
{ 
	pname;
	params;

	#ifdef DODPFS 
	DPF("glLightModeliv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLighti (GLenum light, GLenum pname, GLint param)
{ 
	light;
	pname;
	param;

	#ifdef DODPFS 
	DPF("glLighti"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLightiv (GLenum light, GLenum pname, const GLint *params)
{ 
	light;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glLightiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLineStipple (GLint factor, GLushort pattern)
{ 
	factor;
	pattern;

	#ifdef DODPFS 
	DPF("glLineStipple"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glListBase (GLuint base)
{ 
	base;

	#ifdef DODPFS 
	DPF("glListBase"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLoadMatrixd (const GLdouble *m)
{ 
	m;

	#ifdef DODPFS 
	DPF("glLoadMatrixd"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLoadName (GLuint name)
{ 
	name;

	#ifdef DODPFS 
	DPF("glLoadName"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glLogicOp (GLenum opcode)
{ 
	opcode;

	#ifdef DODPFS 
	DPF("glLogicOp"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{ 
	target;
	u1;
	u2;
	stride;
	order;
	points;

	#ifdef DODPFS 
	DPF("glMap1d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{ 
	target;
	u1;
	u2;
	stride;
	order;
	points;

	#ifdef DODPFS 
	DPF("glMap1f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{ 
	target;
	u1;
	u2;
	ustride;
	uorder;
	v1;
	v2;
	vstride;
	vorder;
	points;

	#ifdef DODPFS 
	DPF("glMap2d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{ 
	target;
	u1;
	u2;
	ustride;
	uorder;
	v1;
	v2;
	vstride;
	vorder;
	points;

	#ifdef DODPFS 
	DPF("glMap2f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMapGrid1d (GLint un, GLdouble u1, GLdouble u2)
{ 
	un;
	u1;
	u2;

	#ifdef DODPFS 
	DPF("glMapGrid1d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMapGrid1f (GLint un, GLfloat u1, GLfloat u2)
{ 
	un;
	u1;
	u2;

	#ifdef DODPFS 
	DPF("glMapGrid1f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{ 
	un;
	u1;
	u2;
	vn;
	v1;
	v2;

	#ifdef DODPFS 
	DPF("glMapGrid2d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{ 
	un;
	u1;
	u2;
	vn;
	v1;
	v2;

	#ifdef DODPFS 
	DPF("glMapGrid2f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param)
{ 
	face;
	pname;
	param;

	#ifdef DODPFS 
	DPF("glMaterialf"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMateriali (GLenum face, GLenum pname, GLint param)
{ 
	face;
	pname;
	param;

	#ifdef DODPFS 
	DPF("glMateriali"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMaterialiv (GLenum face, GLenum pname, const GLint *params)
{ 
	face;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glMaterialiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glMultMatrixd (const GLdouble *m)
{ 
	m;

	#ifdef DODPFS 
	DPF("glMultMatrixd"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNewList (GLuint list, GLenum mode)
{ 
	list;
	mode;

	#ifdef DODPFS 
	DPF("glNewList"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz)
{ 
	nx;
	ny;
	nz;

	#ifdef DODPFS 
	DPF("glNormal3b"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3bv (const GLbyte *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glNormal3bv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz)
{ 
	nx;
	ny;
	nz;

	#ifdef DODPFS 
	DPF("glNormal3d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glNormal3dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{ 
	nx;
	ny;
	nz;

	#ifdef DODPFS 
	DPF("glNormal3f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glNormal3fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3i (GLint nx, GLint ny, GLint nz)
{ 
	nx;
	ny;
	nz;

	#ifdef DODPFS 
	DPF("glNormal3i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glNormal3iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3s (GLshort nx, GLshort ny, GLshort nz)
{ 
	nx;
	ny;
	nz;

	#ifdef DODPFS 
	DPF("glNormal3s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glNormal3sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glNormal3sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPassThrough (GLfloat token)
{ 
	token;

	#ifdef DODPFS 
	DPF("glPassThrough"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat *values)
{ 
	map;
	mapsize;
	values;

	#ifdef DODPFS 
	DPF("glPixelMapfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values)
{ 
	map;
	mapsize;
	values;

	#ifdef DODPFS 
	DPF("glPixelMapuiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values)
{ 
	map;
	mapsize;
	values;

	#ifdef DODPFS 
	DPF("glPixelMapusv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelStoref (GLenum pname, GLfloat param)
{ 
	pname;
	param;

	#ifdef DODPFS 
	DPF("glPixelStoref"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelTransferf (GLenum pname, GLfloat param)
{ 
	pname;
	param;

	#ifdef DODPFS 
	DPF("glPixelTransferf"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelTransferi (GLenum pname, GLint param)
{ 
	pname;
	param;

	#ifdef DODPFS 
	DPF("glPixelTransferi"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPixelZoom (GLfloat xfactor, GLfloat yfactor)
{ 
	xfactor;
	yfactor;

	#ifdef DODPFS 
	DPF("glPixelZoom"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPolygonStipple (const GLubyte *mask)
{ 
	mask;

	#ifdef DODPFS 
	DPF("glPolygonStipple"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPopAttrib (void)
{ 
	#ifdef DODPFS 
	DPF("glPopAttrib"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPopClientAttrib (void)
{ 
	#ifdef DODPFS 
	DPF("glPopClientAttrib"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPopName (void)
{ 
	#ifdef DODPFS 
	DPF("glPopName"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities)
{ 
	n;
	textures;
	priorities;

	#ifdef DODPFS 
	DPF("glPrioritizeTextures"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPushAttrib (GLbitfield mask)
{ 
	mask;

	#ifdef DODPFS 
	DPF("glPushAttrib"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPushClientAttrib (GLbitfield mask)
{ 
	mask;

	#ifdef DODPFS 
	DPF("glPushClientAttrib"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glPushName (GLuint name)
{ 
	name;

	#ifdef DODPFS 
	DPF("glPushName"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2d (GLdouble x, GLdouble y)
{ 
	x;
	y;

	#ifdef DODPFS 
	DPF("glRasterPos2d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos2dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2f (GLfloat x, GLfloat y)
{ 
	x;
	y;

	#ifdef DODPFS 
	DPF("glRasterPos2f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos2fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2i (GLint x, GLint y)
{ 
	x;
	y;

	#ifdef DODPFS 
	DPF("glRasterPos2i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos2iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2s (GLshort x, GLshort y)
{ 
	x;
	y;

	#ifdef DODPFS 
	DPF("glRasterPos2s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos2sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos2sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3d (GLdouble x, GLdouble y, GLdouble z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glRasterPos3d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos3dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3f (GLfloat x, GLfloat y, GLfloat z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glRasterPos3f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos3fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3i (GLint x, GLint y, GLint z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glRasterPos3i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos3iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3s (GLshort x, GLshort y, GLshort z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glRasterPos3s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos3sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos3sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glRasterPos4d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos4dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glRasterPos4f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos4fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4i (GLint x, GLint y, GLint z, GLint w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glRasterPos4i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos4iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glRasterPos4s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRasterPos4sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glRasterPos4sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{ 
	x1;
	y1;
	x2;
	y2;

	#ifdef DODPFS 
	DPF("glRectd"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRectdv (const GLdouble *v1, const GLdouble *v2)
{
	v1; 
	v2;

	#ifdef DODPFS 
	DPF("glRectdv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{ 
	x1;
	y1;
	x2;
	y2;

	#ifdef DODPFS 
	DPF("glRectf"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRectfv (const GLfloat *v1, const GLfloat *v2)
{ 
	v1;
	v2;

	#ifdef DODPFS 
	DPF("glRectfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRectiv (const GLint *v1, const GLint *v2)
{ 
	v1;
	v2;

	#ifdef DODPFS 
	DPF("glRectiv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{ 
	x1;
	y1;
	x2;
	y2;

	#ifdef DODPFS 
	DPF("glRects"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glRectsv (const GLshort *v1, const GLshort *v2)
{ 
	v1;
	v2;

	#ifdef DODPFS 
	DPF("glRectsv"); 
	#endif //DODPFS
}

GLAPI GLint GLAPIENTRY glRenderMode (GLenum mode)
{ 
	mode;

	GLint dummy = 0;

	#ifdef DODPFS 
	DPF("glRenderMode"); 
	#endif //DODPFS

	return dummy;
}

GLAPI void GLAPIENTRY glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{ 
	angle;
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glRotated"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glScaled (GLdouble x, GLdouble y, GLdouble z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glScaled"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glSelectBuffer (GLsizei size, GLuint *buffer)
{ 
	size;
	buffer;

	#ifdef DODPFS 
	DPF("glSelectBuffer"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask)
{ 
	func;
	ref;
	mask;

	#ifdef DODPFS 
	DPF("glStencilFunc"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glStencilMask (GLuint mask)
{ 
	mask;

	#ifdef DODPFS 
	DPF("glStencilMask"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{ 
	fail;
	zfail;
	zpass;

	#ifdef DODPFS 
	DPF("glStencilOp"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1d (GLdouble s)
{ 
	s;

	#ifdef DODPFS 
	DPF("glTexCoord1d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord1dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1f (GLfloat s)
{ 
	s;

	#ifdef DODPFS 
	DPF("glTexCoord1f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord1fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1i (GLint s)
{ 
	s;

	#ifdef DODPFS 
	DPF("glTexCoord1i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord1iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1s (GLshort s)
{ 
	s;

	#ifdef DODPFS 
	DPF("glTexCoord1s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord1sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord1sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord2d (GLdouble s, GLdouble t)
{ 
	s;
	t;

	#ifdef DODPFS 
	DPF("glTexCoord2d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord2dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord2dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord2i (GLint s, GLint t)
{ 
	s;
	t;

	#ifdef DODPFS 
	DPF("glTexCoord2i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord2iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord2iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord2s (GLshort s, GLshort t)
{ 
	s;
	t;

	#ifdef DODPFS 
	DPF("glTexCoord2s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord2sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord2sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3d (GLdouble s, GLdouble t, GLdouble r)
{ 
	s;
	t;
	r;

	#ifdef DODPFS 
	DPF("glTexCoord3d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord3dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3f (GLfloat s, GLfloat t, GLfloat r)
{ 
	s;
	t;
	r;

	#ifdef DODPFS 
	DPF("glTexCoord3f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord3fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3i (GLint s, GLint t, GLint r)
{ 
	s;
	t;
	r;

	#ifdef DODPFS 
	DPF("glTexCoord3i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord3iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3s (GLshort s, GLshort t, GLshort r)
{ 
	s;
	t;
	r;

	#ifdef DODPFS 
	DPF("glTexCoord3s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord3sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord3sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{ 
	s;
	t;
	r;
	q;

	#ifdef DODPFS 
	DPF("glTexCoord4d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord4dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{ 
	s;
	t;
	r;
	q;

	#ifdef DODPFS 
	DPF("glTexCoord4f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord4fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4i (GLint s, GLint t, GLint r, GLint q)
{ 
	s;
	t;
	r;
	q;

	#ifdef DODPFS 
	DPF("glTexCoord4i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord4iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q)
{ 
	s;
	t;
	r;
	q;

	#ifdef DODPFS 
	DPF("glTexCoord4s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexCoord4sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glTexCoord4sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params)
{ 
	target;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glTexEnviv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexGend (GLenum coord, GLenum pname, GLdouble param)
{ 
	coord;
	pname;
	param;

	#ifdef DODPFS 
	DPF("glTexGend"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexGendv (GLenum coord, GLenum pname, const GLdouble *params)
{ 
	coord;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glTexGendv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexGenf (GLenum coord, GLenum pname, GLfloat param)
{ 
	coord;
	pname;
	param;

	#ifdef DODPFS 
	DPF("glTexGenf"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexGeniv (GLenum coord, GLenum pname, const GLint *params)
{ 
	coord;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glTexGeniv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{ 
	target;
	level;
	internalformat;
	width;
	border;
	format;
	type;
	pixels;

	#ifdef DODPFS 
	DPF("glTexImage1D"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{ 
	target;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glTexParameterfv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{ 
	target;
	pname;
	params;

	#ifdef DODPFS 
	DPF("glTexParameteriv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{ 
	target;
	level;
	xoffset;
	width;
	format;
	type;
	pixels;

	#ifdef DODPFS 
	DPF("glTexSubImage1D"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glTranslated (GLdouble x, GLdouble y, GLdouble z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glTranslated"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex2d (GLdouble x, GLdouble y)
{ 
	x;
	y;

	#ifdef DODPFS 
	DPF("glVertex2d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex2dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex2dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex2iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex2iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex2s (GLshort x, GLshort y)
{ 
	x;
	y;

	#ifdef DODPFS 
	DPF("glVertex2s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex2sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex2sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex3d (GLdouble x, GLdouble y, GLdouble z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glVertex3d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex3dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex3dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex3i (GLint x, GLint y, GLint z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glVertex3i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex3iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex3iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex3s (GLshort x, GLshort y, GLshort z)
{ 
	x;
	y;
	z;

	#ifdef DODPFS 
	DPF("glVertex3s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex3sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex3sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glVertex4d"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4dv (const GLdouble *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex4dv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glVertex4f"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4fv (const GLfloat *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex4fv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4i (GLint x, GLint y, GLint z, GLint w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glVertex4i"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4iv (const GLint *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex4iv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w)
{ 
	x;
	y;
	z;
	w;

	#ifdef DODPFS 
	DPF("glVertex4s"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glVertex4sv (const GLshort *v)
{ 
	v;

	#ifdef DODPFS 
	DPF("glVertex4sv"); 
	#endif //DODPFS
}

GLAPI void GLAPIENTRY glFogCoordfEXT (GLfloat f)
{
	f;

	#ifdef DODPFS
	DPF("glFogCoordfEXT");
	#endif //DODPFS
}

GLAPI BOOL GLAPIENTRY wd3dCopyContext(HGLRC src, HGLRC dst, UINT mask)
{
	src;
	dst;
	mask;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglCopyContext"); 
	#endif //DODPFS
	return dummy;
}
GLAPI HGLRC GLAPIENTRY wd3dCreateLayerContext(HDC hdc, int iLayerPlane)
{
	hdc;
	iLayerPlane;

	HGLRC dummy = NULL;
	#ifdef DODPFS 
	DPF("wglCreateLayerContext"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dDescribeLayerPlane(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR plpd)
{
	hdc;
	iPixelFormat;
	iLayerPlane;
	nBytes;
	plpd;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglDescribeLayerPlane"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wglGetDefaultProcAddress()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglGetDefaultProcAddress"); 
	#endif //DODPFS
	return dummy;
}
GLAPI int GLAPIENTRY wd3dGetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, COLORREF *pcr)
{
	hdc;
	iLayerPlane;
	iStart;
	cEntries;
	pcr;

	int dummy = 0;
	#ifdef DODPFS 
	DPF("wglGetLayerPaletteEntries"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize)
{
	hdc;
	iLayerPlane;
	bRealize;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglRealizeLayerPalette"); 
	#endif //DODPFS
	return dummy;
}
GLAPI int GLAPIENTRY wd3dSetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, CONST COLORREF *pcr)
{
	hdc;
	iLayerPlane;
	iStart;
	cEntries;
	pcr;

	int dummy = 0;
	#ifdef DODPFS 
	DPF("wglSetLayerPaletteEntries"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dShareLists(HGLRC hglrc1, HGLRC hglrc2)
{
	hglrc1;
	hglrc2;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglShareLists"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
	hdc;
	fuPlanes;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglSwapLayerBuffers"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listbase)
{
	hdc;
	first;
	count;
	listbase;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglUseFontBitmapsA"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dUseFontBitmapsW()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglUseFontBitmapsW"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dUseFontOutlinesA(HDC hdc, DWORD first, DWORD count, DWORD listBase, float deviation, float extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf)
{
	hdc;
	first;
	count;
	listBase;
	deviation;
	extrusion;
	format;
	lpgmf;

	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglUseFontOutlinesA"); 
	#endif //DODPFS
	return dummy;
}
GLAPI BOOL GLAPIENTRY wd3dUseFontOutlinesW()
{
	BOOL dummy = FALSE;
	#ifdef DODPFS 
	DPF("wglUseFontOutlinesW"); 
	#endif //DODPFS
	return dummy;
}
