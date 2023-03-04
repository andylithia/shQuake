/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// d_polyset.c: routines for drawing sets of polygons sharing the same
// texture (used for Alias models)

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

#define NSPIRE_POLYSET_DRAWSPANS 0

// TODO: put in span spilling to shrink list size
// !!! if this is changed, it must be changed in d_polysa.s too !!!
#define DPS_MAXSPANS			MAXHEIGHT+1	
									// 1 extra for spanpackage that marks end

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
	void			*pdest;
	short			*pz;
	int				count;
	byte			*ptex;
	int				sfrac, tfrac, light, zi;
} spanpackage_t;

typedef struct {
	int		isflattop;
	int		numleftedges;
	int		*pleftedgevert0;
	int		*pleftedgevert1;
	int		*pleftedgevert2;
	int		numrightedges;
	int		*prightedgevert0;
	int		*prightedgevert1;
	int		*prightedgevert2;
} edgetable;

int	r_p0[6], r_p1[6], r_p2[6];

byte		*d_pcolormap;

int			d_aflatcolor;
int			d_xdenom;

edgetable	*pedgetable;

edgetable	edgetables[12] = {
	{0, 1, r_p0, r_p2, NULL, 2, r_p0, r_p1, r_p2 },
	{0, 2, r_p1, r_p0, r_p2,   1, r_p1, r_p2, NULL},
	{1, 1, r_p0, r_p2, NULL, 1, r_p1, r_p2, NULL},
	{0, 1, r_p1, r_p0, NULL, 2, r_p1, r_p2, r_p0 },
	{0, 2, r_p0, r_p2, r_p1,   1, r_p0, r_p1, NULL},
	{0, 1, r_p2, r_p1, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p2, r_p1, NULL, 2, r_p2, r_p0, r_p1 },
	{0, 2, r_p2, r_p1, r_p0,   1, r_p2, r_p0, NULL},
	{0, 1, r_p1, r_p0, NULL, 1, r_p1, r_p2, NULL},
	{1, 1, r_p2, r_p1, NULL, 1, r_p0, r_p1, NULL},
	{1, 1, r_p1, r_p0, NULL, 1, r_p2, r_p0, NULL},
	{0, 1, r_p0, r_p2, NULL, 1, r_p0, r_p1, NULL},
};

// FIXME: some of these can become statics
int				a_sstepxfrac, a_tstepxfrac, r_lstepx, a_ststepxwhole;
int				r_sstepx, r_tstepx, r_lstepy, r_sstepy, r_tstepy;
int				r_zistepx, r_zistepy;
int				d_aspancount, d_countextrastep;

spanpackage_t			*a_spans;
spanpackage_t			*d_pedgespanpackage;
static int				ystart;
byte					*d_pdest, *d_ptex;
short					*d_pz;
int						d_sfrac, d_tfrac, d_light, d_zi;
int						d_ptexextrastep, d_sfracextrastep;
int						d_tfracextrastep, d_lightextrastep, d_pdestextrastep;
int						d_lightbasestep, d_pdestbasestep, d_ptexbasestep;
int						d_sfracbasestep, d_tfracbasestep;
int						d_ziextrastep, d_zibasestep;
int						d_pzextrastep, d_pzbasestep;

typedef struct {
	int		quotient;
	int		remainder;
} adivtab_t;

static adivtab_t	adivtab[32*32] = {
#include "adivtab.h"
};

byte	*skintable[MAX_LBM_HEIGHT];
int		skinwidth;
byte	*skinstart;

void D_PolysetDrawSpans8 (spanpackage_t *pspanpackage);
void D_PolysetCalcGradients (int skinwidth);
void D_DrawSubdiv (void);
void D_DrawNonSubdiv (void);
void D_PolysetRecursiveTriangle (int *p1, int *p2, int *p3);
void D_PolysetSetEdgeTable (void);
void D_RasterizeAliasPolySmooth (void);
void D_PolysetScanLeftEdge (int height);

#if	!id386

/*
================
D_PolysetDraw
================
*/
void D_PolysetDraw (void)
{
	spanpackage_t	spans[DPS_MAXSPANS + 1 +
			((CACHE_SIZE - 1) / sizeof(spanpackage_t)) + 1];
						// one extra because of cache line pretouching

	a_spans = (spanpackage_t *)
			(((uintptr_t)&spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_affinetridesc.drawtype)
	{
		D_DrawSubdiv ();
	}
	else
	{
		D_DrawNonSubdiv ();
	}
}


/*
================
D_PolysetDrawFinalVerts
================
*/
void D_PolysetDrawFinalVerts (finalvert_t *fv, int numverts)
{
	int		i, z;
	short	*zbuf;

	for (i=0 ; i<numverts ; i++, fv++)
	{
	// valid triangle coordinates for filling can include the bottom and
	// right clip edges, due to the fill rule; these shouldn't be drawn
		if ((fv->v[0] < r_refdef.vrectright) &&
			(fv->v[1] < r_refdef.vrectbottom))
		{
			z = fv->v[5]>>16;
			zbuf = zspantable[fv->v[1]] + fv->v[0];
			if (z >= *zbuf)
			{
				int		pix;
				
				*zbuf = z;
				pix = skintable[fv->v[3]>>16][fv->v[2]>>16];
				pix = ((byte *)acolormap)[pix + (fv->v[4] & 0xFF00) ];
				d_viewbuffer[d_scantable[fv->v[1]] + fv->v[0]] = pix;
			}
		}
	}
}


/*
================
D_DrawSubdiv
================
*/
void D_DrawSubdiv (void)
{
	mtriangle_t		*ptri;
	finalvert_t		*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++)
	{
		index0 = pfv + ptri[i].vertindex[0];
		index1 = pfv + ptri[i].vertindex[1];
		index2 = pfv + ptri[i].vertindex[2];

		if (((index0->v[1]-index1->v[1]) *
			 (index0->v[0]-index2->v[0]) -
			 (index0->v[0]-index1->v[0]) * 
			 (index0->v[1]-index2->v[1])) >= 0)
		{
			continue;
		}

		d_pcolormap = &((byte *)acolormap)[index0->v[4] & 0xFF00];

		if (ptri[i].facesfront)
		{
			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);
		}
		else
		{
			int		s0, s1, s2;

			s0 = index0->v[2];
			s1 = index1->v[2];
			s2 = index2->v[2];

			if (index0->flags & ALIAS_ONSEAM)
				index0->v[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				index1->v[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				index2->v[2] += r_affinetridesc.seamfixupX16;

			D_PolysetRecursiveTriangle(index0->v, index1->v, index2->v);

			index0->v[2] = s0;
			index1->v[2] = s1;
			index2->v[2] = s2;
		}
	}
}


/*
================
D_DrawNonSubdiv
================
*/
void D_DrawNonSubdiv (void)
{
	mtriangle_t		*ptri;
	finalvert_t		*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++, ptri++)
	{
		index0 = pfv + ptri->vertindex[0];
		index1 = pfv + ptri->vertindex[1];
		index2 = pfv + ptri->vertindex[2];

		d_xdenom = (index0->v[1]-index1->v[1]) *
				(index0->v[0]-index2->v[0]) -
				(index0->v[0]-index1->v[0])*(index0->v[1]-index2->v[1]);

		if (d_xdenom >= 0)
		{
			continue;
		}

		r_p0[0] = index0->v[0];		// u
		r_p0[1] = index0->v[1];		// v
		r_p0[2] = index0->v[2];		// s
		r_p0[3] = index0->v[3];		// t
		r_p0[4] = index0->v[4];		// light
		r_p0[5] = index0->v[5];		// iz

		r_p1[0] = index1->v[0];
		r_p1[1] = index1->v[1];
		r_p1[2] = index1->v[2];
		r_p1[3] = index1->v[3];
		r_p1[4] = index1->v[4];
		r_p1[5] = index1->v[5];

		r_p2[0] = index2->v[0];
		r_p2[1] = index2->v[1];
		r_p2[2] = index2->v[2];
		r_p2[3] = index2->v[3];
		r_p2[4] = index2->v[4];
		r_p2[5] = index2->v[5];

		if (!ptri->facesfront)
		{
			if (index0->flags & ALIAS_ONSEAM)
				r_p0[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				r_p1[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				r_p2[2] += r_affinetridesc.seamfixupX16;
		}

		D_PolysetSetEdgeTable ();
		D_RasterizeAliasPolySmooth ();
	}
}


/*
================
D_PolysetRecursiveTriangle
================
*/
void D_PolysetRecursiveTriangle (int *lp1, int *lp2, int *lp3)
{
	int		*temp;
	int		d;
	int		new[6];
	int		z;
	short	*zbuf;

	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	d = lp3[0] - lp2[0];
	if (d < -1 || d > 1)
		goto split2;
	d = lp3[1] - lp2[1];
	if (d < -1 || d > 1)
		goto split2;

	d = lp1[0] - lp3[0];
	if (d < -1 || d > 1)
		goto split3;
	d = lp1[1] - lp3[1];
	if (d < -1 || d > 1)
	{
split3:
		temp = lp1;
		lp1 = lp3;
		lp3 = lp2;
		lp2 = temp;

		goto split;
	}

	return;			// entire tri is filled

split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

split:
// split this edge
	new[0] = (lp1[0] + lp2[0]) >> 1;
	new[1] = (lp1[1] + lp2[1]) >> 1;
	new[2] = (lp1[2] + lp2[2]) >> 1;
	new[3] = (lp1[3] + lp2[3]) >> 1;
	new[5] = (lp1[5] + lp2[5]) >> 1;

// draw the point if splitting a leading edge
	if (lp2[1] > lp1[1])
		goto nodraw;
	if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
		goto nodraw;


	z = new[5]>>16;
	zbuf = zspantable[new[1]] + new[0];
	if (z >= *zbuf)
	{
		int		pix;
		
		*zbuf = z;
		pix = d_pcolormap[skintable[new[3]>>16][new[2]>>16]];
		d_viewbuffer[d_scantable[new[1]] + new[0]] = pix;
	}

nodraw:
// recursively continue
	D_PolysetRecursiveTriangle (lp3, lp1, new);
	D_PolysetRecursiveTriangle (lp3, new, lp2);
}

#endif	// !id386


/*
================
D_PolysetUpdateTables
================
*/
void D_PolysetUpdateTables (void)
{
	int		i;
	byte	*s;
	
	if (r_affinetridesc.skinwidth != skinwidth ||
		r_affinetridesc.pskin != skinstart)
	{
		skinwidth = r_affinetridesc.skinwidth;
		skinstart = r_affinetridesc.pskin;
		s = skinstart;
		for (i=0 ; i<MAX_LBM_HEIGHT ; i++, s+=skinwidth)
			skintable[i] = s;
	}
}


#if	!id386

/*
===================
D_PolysetScanLeftEdge
====================
*/
void D_PolysetScanLeftEdge (int height)
{

	do
	{
		d_pedgespanpackage->pdest = d_pdest;
		d_pedgespanpackage->pz = d_pz;
		d_pedgespanpackage->count = d_aspancount;
		d_pedgespanpackage->ptex = d_ptex;

		d_pedgespanpackage->sfrac = d_sfrac;
		d_pedgespanpackage->tfrac = d_tfrac;

	// FIXME: need to clamp l, s, t, at both ends?
		d_pedgespanpackage->light = d_light;
		d_pedgespanpackage->zi = d_zi;

		d_pedgespanpackage++;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_pdest += d_pdestextrastep;
			d_pz += d_pzextrastep;
			d_aspancount += d_countextrastep;
			d_ptex += d_ptexextrastep;
			d_sfrac += d_sfracextrastep;
			d_ptex += d_sfrac >> 16;

			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracextrastep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_light += d_lightextrastep;
			d_zi += d_ziextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_pdest += d_pdestbasestep;
			d_pz += d_pzbasestep;
			d_aspancount += ubasestep;
			d_ptex += d_ptexbasestep;
			d_sfrac += d_sfracbasestep;
			d_ptex += d_sfrac >> 16;
			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracbasestep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_light += d_lightbasestep;
			d_zi += d_zibasestep;
		}
	} while (--height);
}

#endif	// !id386


/*
===================
D_PolysetSetUpForLineScan
====================
*/
void D_PolysetSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
		fixed8_t endvertu, fixed8_t endvertv)
{
#if !NSPIRE_SMALL_OPTS
	double		dm, dn;
#endif
	int			tm, tn;
	adivtab_t	*ptemp;

// TODO: implement x86 version

	errorterm = -1;

	tm = endvertu - startvertu;
	tn = endvertv - startvertv;

	if (((tm <= 16) && (tm >= -15)) &&
		((tn <= 16) && (tn >= -15)))
	{
		ptemp = &adivtab[((tm+15) << 5) + (tn+15)];
		ubasestep = ptemp->quotient;
		erroradjustup = ptemp->remainder;
		erroradjustdown = tn;
	}
	else
	{
#if NSPIRE_SMALL_OPTS
		FloorDivModFixed( tm, tn, &ubasestep, &erroradjustup );
		erroradjustdown = tn;
#else
		dm = (double)tm;
		dn = (double)tn;

		FloorDivMod (dm, dn, &ubasestep, &erroradjustup);
		erroradjustdown = dn;
#endif
	}
}


#if	!id386

/*
================
D_PolysetCalcGradients
================
*/

#define NSPIRE_POLYSET_GRAD_FIXED 1

void D_PolysetCalcGradients (int skinwidth)
{
#if !NSPIRE_POLYSET_GRAD_FIXED
	float	xstepdenominv, ystepdenominv, t0, t1;
	float	p01_minus_p21, p11_minus_p21, p00_minus_p20, p10_minus_p20;

	p00_minus_p20 = r_p0[0] - r_p2[0];
	p01_minus_p21 = r_p0[1] - r_p2[1];
	p10_minus_p20 = r_p1[0] - r_p2[0];
	p11_minus_p21 = r_p1[1] - r_p2[1];

	xstepdenominv = 1.0f / (float)d_xdenom;

	ystepdenominv = -xstepdenominv;

// ceil () for light so positive steps are exaggerated, negative steps
// diminished,  pushing us away from underflow toward overflow. Underflow is
// very visible, overflow is very unlikely, because of ambient lighting
	t0 = r_p0[4] - r_p2[4];
	t1 = r_p1[4] - r_p2[4];
	r_lstepx = (int)
			ceilf((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
	r_lstepy = (int)
			ceilf((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	t0 = r_p0[2] - r_p2[2];
	t1 = r_p1[2] - r_p2[2];
	r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			xstepdenominv);
	r_sstepy = (int)((t1 * p00_minus_p20 - t0* p10_minus_p20) *
			ystepdenominv);

	t0 = r_p0[3] - r_p2[3];
	t1 = r_p1[3] - r_p2[3];
	r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			xstepdenominv);
	r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			ystepdenominv);

	t0 = r_p0[5] - r_p2[5];
	t1 = r_p1[5] - r_p2[5];
	r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			xstepdenominv);
	r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			ystepdenominv);

#if	id386
	a_sstepxfrac = r_sstepx << 16;
	a_tstepxfrac = r_tstepx << 16;
#else
#if !NSPIRE_POLYSET_DRAWSPANS
	a_sstepxfrac = r_sstepx & 0xFFFF;
	a_tstepxfrac = r_tstepx & 0xFFFF;
#else
	a_sstepxfrac = r_sstepx;
	a_tstepxfrac = r_tstepx;
#endif
#endif

	a_ststepxwhole = skinwidth * (r_tstepx >> 16) + (r_sstepx >> 16);
#else
	fixed16_t xstepdenominv, ystepdenominv, t0, t1;
	fixed16_t p01_minus_p21, p11_minus_p21, p00_minus_p20, p10_minus_p20;

	p00_minus_p20 = r_p0[0] - r_p2[0];
	p01_minus_p21 = r_p0[1] - r_p2[1];
	p10_minus_p20 = r_p1[0] - r_p2[0];
	p11_minus_p21 = r_p1[1] - r_p2[1];

	if( d_xdenom < 0 )
	{
		xstepdenominv = -udiv_fast_32_32_incorrect( -d_xdenom, 0x1000000 );
	}
	else
	{
		xstepdenominv = udiv_fast_32_32_incorrect( d_xdenom, 0x1000000 );
	}
	ystepdenominv = -xstepdenominv;

// ceil () for light so positive steps are exaggerated, negative steps
// diminished,  pushing us away from underflow toward overflow. Underflow is
// very visible, overflow is very unlikely, because of ambient lighting
	t0 = r_p0[4] - r_p2[4];
	t1 = r_p1[4] - r_p2[4];
	r_lstepx = (int) ((((long long)(t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv) + 0xffffff) >> 24 );
	r_lstepy = (int) ((((long long)(t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv) + 0xffffff) >> 24 );

	t0 = r_p0[2] - r_p2[2];
	t1 = r_p1[2] - r_p2[2];
	r_sstepx = (int)(((long long)(t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv) >> 24 );
	r_sstepy = (int)(((long long)(t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv) >> 24 );

	t0 = r_p0[3] - r_p2[3];
	t1 = r_p1[3] - r_p2[3];
	r_tstepx = (int)(((long long)(t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv) >> 24 );
	r_tstepy = (int)(((long long)(t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv) >> 24 );

	t0 = (r_p0[5] - r_p2[5])>>16;
	t1 = (r_p1[5] - r_p2[5])>>16;
	r_zistepx = (int)(((long long)(t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv) >> 8);
	r_zistepy = (int)(((long long)(t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv) >> 8);

#if	id386
	a_sstepxfrac = r_sstepx << 16;
	a_tstepxfrac = r_tstepx << 16;
#else
#if !NSPIRE_POLYSET_DRAWSPANS
	a_sstepxfrac = r_sstepx & 0xFFFF;
	a_tstepxfrac = r_tstepx & 0xFFFF;
#else
	a_sstepxfrac = r_sstepx;
	a_tstepxfrac = r_tstepx;
#endif
#endif

	a_ststepxwhole = skinwidth * (r_tstepx >> 16) + (r_sstepx >> 16);
#endif
}

#endif	// !id386


#if 0
byte gelmap[256];
void InitGel (byte *palette)
{
	int		i;
	int		r;

	for (i=0 ; i<256 ; i++)
	{
//		r = (palette[i*3]>>4);
		r = (palette[i*3] + palette[i*3+1] + palette[i*3+2])/(16*3);
		gelmap[i] = /* 64 */ 0 + r;
	}
}
#endif


#if	!id386

/*
================
D_PolysetDrawSpans8
================
*/
typedef struct {
	int		lcount;
	byte	*lpdest;
	byte	*lptex;
	short	*lpz;

	byte    *acolormap;
	int     skinwidth;

	fixed16_t lsfrac;
	fixed16_t ltfrac;
	fixed16_t llight;
	fixed16_t lzi;

	fixed16_t a_sstepxfrac;
	fixed16_t a_tstepxfrac;
	fixed16_t r_zistepx;
	fixed16_t r_lstepx;
} d_polyset_draw_spans_8_nspire_t;

#if NSPIRE_POLYSET_DRAWSPANS
#ifndef FORNSPIRE
void D_NSpirePolysetDrawSpan ( d_polyset_draw_spans_8_nspire_t *ps_dset )
{
	short *lpz;
	int lcount = ps_dset->lcount;
	int i_zpass, local_skinwidth;
	fixed16_t dzi, lzi, slzi, local_r_zistepx;
	byte *local_acolormap, *lptex, *lpdest;
	fixed16_t lsfrac, ltfrac, llight, local_r_lstepx, local_a_sstepxfrac, local_a_tstepxfrac;

	do
	{
		i_zpass = 0;

		/* z */
		lpz = ps_dset->lpz;
		lzi = ps_dset->lzi;
		local_r_zistepx = ps_dset->r_zistepx;

#define NSPIRE_POLYSET_ZITER( x )	\
do {							\
	slzi = lzi >> 16;			\
	dzi = *lpz;					\
	lzi += local_r_zistepx;		\
	if( slzi >= dzi )			\
	{							\
		*lpz = slzi;			\
		i_zpass |= x;			\
	}							\
	lpz++;						\
} while( 0 )

		switch( lcount )
		{
		default:
		case 8:
			NSPIRE_POLYSET_ZITER( 1 );
		case 7:
			NSPIRE_POLYSET_ZITER( 2 );
		case 6:
			NSPIRE_POLYSET_ZITER( 4 );
		case 5:
			NSPIRE_POLYSET_ZITER( 8 );
		case 4:
			NSPIRE_POLYSET_ZITER( 16 );
		case 3:
			NSPIRE_POLYSET_ZITER( 32 );
		case 2:
			NSPIRE_POLYSET_ZITER( 64 );
		case 1:
			NSPIRE_POLYSET_ZITER( 128 );
		case 0:
			;
		}

		ps_dset->lpz = lpz;
		ps_dset->lzi = lzi;

		/* tex + l */

		lpdest = ps_dset->lpdest;
		lptex = ps_dset->lptex;
		local_skinwidth = ps_dset->skinwidth;
		local_acolormap = ps_dset->acolormap;
		local_r_lstepx = ps_dset->r_lstepx;
		local_a_sstepxfrac = ps_dset->a_sstepxfrac;
		local_a_tstepxfrac = ps_dset->a_tstepxfrac;
		llight = ps_dset->llight;
		lsfrac = ps_dset->lsfrac;
		ltfrac = ps_dset->ltfrac;

#define NSPIRE_POLYSET_TITER( x )			\
do {										\
	if( i_zpass & x )						\
	{										\
		*lpdest = (local_acolormap)[ lptex[ ( lsfrac >> 16 ) + ( ( ltfrac >> 16 ) * local_skinwidth ) ] + ( llight & 0xff00 ) ];	\
	}										\
	lpdest++;								\
	llight += local_r_lstepx;				\
	lsfrac += local_a_sstepxfrac;			\
	ltfrac += local_a_tstepxfrac;			\
} while( 0 )

		switch( lcount )
		{
		default:
		case 8:
			NSPIRE_POLYSET_TITER( 1 );
		case 7:
			NSPIRE_POLYSET_TITER( 2 );
		case 6:
			NSPIRE_POLYSET_TITER( 4 );
		case 5:
			NSPIRE_POLYSET_TITER( 8 );
		case 4:
			NSPIRE_POLYSET_TITER( 16 );
		case 3:
			NSPIRE_POLYSET_TITER( 32 );
		case 2:
			NSPIRE_POLYSET_TITER( 64 );
		case 1:
			NSPIRE_POLYSET_TITER( 128 );
		case 0:
			;
		}

		ps_dset->lpdest = lpdest;
		ps_dset->llight = llight;
		ps_dset->lsfrac = lsfrac;
		ps_dset->ltfrac = ltfrac;

		lcount -= 8;
	} while( lcount > 0 );
}
#endif
#endif

void D_PolysetDrawSpans8 (spanpackage_t *pspanpackage)
{
	int		lcount;
	byte	*lpdest;
	byte	*lptex;
	int		lsfrac, ltfrac;
	int		llight;
	int		lzi;
	short	*lpz;
	int i_tmp, i_tmp2, i_zpass;


#if !NSPIRE_POLYSET_DRAWSPANS
	do
	{
		lcount = d_aspancount - pspanpackage->count;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_aspancount += d_countextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_aspancount += ubasestep;
		}

		if (lcount)
		{
			lpdest = pspanpackage->pdest;
			lptex = pspanpackage->ptex;
			lpz = pspanpackage->pz;
			lsfrac = pspanpackage->sfrac;
			ltfrac = pspanpackage->tfrac;
			llight = pspanpackage->light;
			lzi = pspanpackage->zi;

			do
			{
				if ((lzi >> 16) >= *lpz)
				{
					*lpdest = ((byte *)acolormap)[*lptex + (llight & 0xFF00)];
// gel mapping					*lpdest = gelmap[*lpdest];
					*lpz = lzi >> 16;
				}
				lpdest++;
				lzi += r_zistepx;
				lpz++;
				llight += r_lstepx;
				lptex += a_ststepxwhole;
				lsfrac += a_sstepxfrac;
				lptex += lsfrac >> 16;
				lsfrac &= 0xFFFF;
				ltfrac += a_tstepxfrac;
				if (ltfrac & 0x10000)
				{
					lptex += r_affinetridesc.skinwidth;
					ltfrac &= 0xFFFF;
				}
			} while (--lcount);
		}

		pspanpackage++;
	} while (pspanpackage->count != -999999);
#else
	d_polyset_draw_spans_8_nspire_t s_dset;
	s_dset.acolormap = ( byte *)acolormap;
	s_dset.skinwidth = r_affinetridesc.skinwidth;
	s_dset.a_sstepxfrac = a_sstepxfrac;
	s_dset.a_tstepxfrac = a_tstepxfrac;
	s_dset.r_lstepx = r_lstepx;
	s_dset.r_zistepx = r_zistepx;

	do
	{
		lcount = d_aspancount - pspanpackage->count;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_aspancount += d_countextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_aspancount += ubasestep;
		}

		if (lcount)
		{

			s_dset.lcount = lcount;
			s_dset.lpdest = ( byte *)pspanpackage->pdest;
			s_dset.lptex = pspanpackage->ptex;
			s_dset.lpz = pspanpackage->pz;
			s_dset.lsfrac = pspanpackage->sfrac;
			s_dset.ltfrac = pspanpackage->tfrac;
			s_dset.llight = pspanpackage->light;
			s_dset.lzi = pspanpackage->zi;

			D_NSpirePolysetDrawSpan( &s_dset );

#if 0
						__asm volatile(														\
						"mov		%[_tmp], %[_tex_s], asr #16								\n\t"	\
						"smlabt		%[_tmp], %[_skinwidth], %[_tex_t], %[_tmp]				\n\t"	\
						"ldrb		%[_tmp2], [ %[_tex], %[_tmp] ]							\n\t"	\
						"and		%[_tmp], %[_light], #65280								\n\t"	\
						"add		%[_tmp2], %[_tmp], %[_tmp2]								\n\t"	\
						"ldrb		%[_tmp], [ %[_colormap], %[_tmp2] ]						\n\t"	\
						"strb		%[_tmp], [ %[_pdest] ]									\n\t"	\
						:  [_tmp] "=&r" (i_tmp), [_tmp2] "=&r" (i_tmp2) : [_pdest] "r" (lpdest), [_tex] "r" (lptex), [_colormap] "r" (local_acolormap), [_tex_s] "r" (lsfrac), [_tex_t] "r" (ltfrac), [_skinwidth] "r" (local_skinwidth), [_light] "r" (llight) : );
#endif
		}
		pspanpackage++;
	} while (pspanpackage->count != -999999);
#endif
}
#endif	// !id386


/*
================
D_PolysetFillSpans8
================
*/
void D_PolysetFillSpans8 (spanpackage_t *pspanpackage)
{
	int				color;

// FIXME: do z buffering

	color = d_aflatcolor++;

	while (1)
	{
		int		lcount;
		byte	*lpdest;

		lcount = pspanpackage->count;

		if (lcount == -1)
			return;

		if (lcount)
		{
			lpdest = pspanpackage->pdest;

			do
			{
				*lpdest++ = color;
			} while (--lcount);
		}

		pspanpackage++;
	}
}

/*
================
D_RasterizeAliasPolySmooth
================
*/
void D_RasterizeAliasPolySmooth (void)
{
	int				initialleftheight, initialrightheight;
	int				*plefttop, *prighttop, *pleftbottom, *prightbottom;
	int				working_lstepx, originalcount;

	plefttop = pedgetable->pleftedgevert0;
	prighttop = pedgetable->prightedgevert0;

	pleftbottom = pedgetable->pleftedgevert1;
	prightbottom = pedgetable->prightedgevert1;

	initialleftheight = pleftbottom[1] - plefttop[1];
	initialrightheight = prightbottom[1] - prighttop[1];

//
// set the s, t, and light gradients, which are consistent across the triangle
// because being a triangle, things are affine
//
	D_PolysetCalcGradients (r_affinetridesc.skinwidth);

//
// rasterize the polygon
//

//
// scan out the top (and possibly only) part of the left edge
//
	d_pedgespanpackage = a_spans;

	ystart = plefttop[1];
	d_aspancount = plefttop[0] - prighttop[0];

	d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
			(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
#if	id386
	d_sfrac = (plefttop[2] & 0xFFFF) << 16;
	d_tfrac = (plefttop[3] & 0xFFFF) << 16;
#else
	d_sfrac = plefttop[2] & 0xFFFF;
	d_tfrac = plefttop[3] & 0xFFFF;
#endif
	d_light = plefttop[4];
	d_zi = plefttop[5];

	d_pdest = (byte *)d_viewbuffer +
			ystart * screenwidth + plefttop[0];
	d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

	if (initialleftheight == 1)
	{
		d_pedgespanpackage->pdest = d_pdest;
		d_pedgespanpackage->pz = d_pz;
		d_pedgespanpackage->count = d_aspancount;
		d_pedgespanpackage->ptex = d_ptex;

		d_pedgespanpackage->sfrac = d_sfrac;
		d_pedgespanpackage->tfrac = d_tfrac;

	// FIXME: need to clamp l, s, t, at both ends?
		d_pedgespanpackage->light = d_light;
		d_pedgespanpackage->zi = d_zi;

		d_pedgespanpackage++;
	}
	else
	{
		D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
							  pleftbottom[0], pleftbottom[1]);

	#if	id386
		d_pzbasestep = (d_zwidth + ubasestep) << 1;
		d_pzextrastep = d_pzbasestep + 2;
	#else
		d_pzbasestep = d_zwidth + ubasestep;
		d_pzextrastep = d_pzbasestep + 1;
	#endif

		d_pdestbasestep = screenwidth + ubasestep;
		d_pdestextrastep = d_pdestbasestep + 1;

	// TODO: can reuse partial expressions here

	// for negative steps in x along left edge, bias toward overflow rather than
	// underflow (sort of turning the floor () we did in the gradient calcs into
	// ceil (), but plus a little bit)
		if (ubasestep < 0)
			working_lstepx = r_lstepx - 1;
		else
			working_lstepx = r_lstepx;

		d_countextrastep = ubasestep + 1;
		d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
				((r_tstepy + r_tstepx * ubasestep) >> 16) *
				r_affinetridesc.skinwidth;
	#if	id386
		d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
		d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
	#else
		d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
		d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
	#endif
		d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
		d_zibasestep = r_zistepy + r_zistepx * ubasestep;

		d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
				((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
				r_affinetridesc.skinwidth;
	#if	id386
		d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) << 16;
		d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) << 16;
	#else
		d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) & 0xFFFF;
		d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) & 0xFFFF;
	#endif
		d_lightextrastep = d_lightbasestep + working_lstepx;
		d_ziextrastep = d_zibasestep + r_zistepx;

		D_PolysetScanLeftEdge (initialleftheight);
	}

//
// scan out the bottom part of the left edge, if it exists
//
	if (pedgetable->numleftedges == 2)
	{
		int		height;

		plefttop = pleftbottom;
		pleftbottom = pedgetable->pleftedgevert2;

		height = pleftbottom[1] - plefttop[1];

// TODO: make this a function; modularize this function in general

		ystart = plefttop[1];
		d_aspancount = plefttop[0] - prighttop[0];
		d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
				(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
		d_sfrac = 0;
		d_tfrac = 0;
		d_light = plefttop[4];
		d_zi = plefttop[5];

		d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
		d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

		if (height == 1)
		{
			d_pedgespanpackage->pdest = d_pdest;
			d_pedgespanpackage->pz = d_pz;
			d_pedgespanpackage->count = d_aspancount;
			d_pedgespanpackage->ptex = d_ptex;

			d_pedgespanpackage->sfrac = d_sfrac;
			d_pedgespanpackage->tfrac = d_tfrac;

		// FIXME: need to clamp l, s, t, at both ends?
			d_pedgespanpackage->light = d_light;
			d_pedgespanpackage->zi = d_zi;

			d_pedgespanpackage++;
		}
		else
		{
			D_PolysetSetUpForLineScan(plefttop[0], plefttop[1],
								  pleftbottom[0], pleftbottom[1]);

			d_pdestbasestep = screenwidth + ubasestep;
			d_pdestextrastep = d_pdestbasestep + 1;

	#if	id386
			d_pzbasestep = (d_zwidth + ubasestep) << 1;
			d_pzextrastep = d_pzbasestep + 2;
	#else
			d_pzbasestep = d_zwidth + ubasestep;
			d_pzextrastep = d_pzbasestep + 1;
	#endif

			if (ubasestep < 0)
				working_lstepx = r_lstepx - 1;
			else
				working_lstepx = r_lstepx;

			d_countextrastep = ubasestep + 1;
			d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
					((r_tstepy + r_tstepx * ubasestep) >> 16) *
					r_affinetridesc.skinwidth;
	#if	id386
			d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
			d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
	#else
			d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
			d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
	#endif
			d_lightbasestep = r_lstepy + working_lstepx * ubasestep;
			d_zibasestep = r_zistepy + r_zistepx * ubasestep;

			d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
					((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
					r_affinetridesc.skinwidth;
	#if	id386
			d_sfracextrastep = ((r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF)<<16;
			d_tfracextrastep = ((r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF)<<16;
	#else
			d_sfracextrastep = (r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF;
			d_tfracextrastep = (r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF;
	#endif
			d_lightextrastep = d_lightbasestep + working_lstepx;
			d_ziextrastep = d_zibasestep + r_zistepx;

			D_PolysetScanLeftEdge (height);
		}
	}

// scan out the top (and possibly only) part of the right edge, updating the
// count field
	d_pedgespanpackage = a_spans;

	D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
						  prightbottom[0], prightbottom[1]);
	d_aspancount = 0;
	d_countextrastep = ubasestep + 1;
	originalcount = a_spans[initialrightheight].count;
	a_spans[initialrightheight].count = -999999; // mark end of the spanpackages
	D_PolysetDrawSpans8 (a_spans);

// scan out the bottom part of the right edge, if it exists
	if (pedgetable->numrightedges == 2)
	{
		int				height;
		spanpackage_t	*pstart;

		pstart = a_spans + initialrightheight;
		pstart->count = originalcount;

		d_aspancount = prightbottom[0] - prighttop[0];

		prighttop = prightbottom;
		prightbottom = pedgetable->prightedgevert2;

		height = prightbottom[1] - prighttop[1];

		D_PolysetSetUpForLineScan(prighttop[0], prighttop[1],
							  prightbottom[0], prightbottom[1]);

		d_countextrastep = ubasestep + 1;
		a_spans[initialrightheight + height].count = -999999;
											// mark end of the spanpackages
		D_PolysetDrawSpans8 (pstart);
	}
}


/*
================
D_PolysetSetEdgeTable
================
*/
void D_PolysetSetEdgeTable (void)
{
	int			edgetableindex;

	edgetableindex = 0;	// assume the vertices are already in
						//  top to bottom order

//
// determine which edges are right & left, and the order in which
// to rasterize them
//
	if (r_p0[1] >= r_p1[1])
	{
		if (r_p0[1] == r_p1[1])
		{
			if (r_p0[1] < r_p2[1])
				pedgetable = &edgetables[2];
			else
				pedgetable = &edgetables[5];

			return;
		}
		else
		{
			edgetableindex = 1;
		}
	}

	if (r_p0[1] == r_p2[1])
	{
		if (edgetableindex)
			pedgetable = &edgetables[8];
		else
			pedgetable = &edgetables[9];

		return;
	}
	else if (r_p1[1] == r_p2[1])
	{
		if (edgetableindex)
			pedgetable = &edgetables[10];
		else
			pedgetable = &edgetables[11];

		return;
	}

	if (r_p0[1] > r_p2[1])
		edgetableindex += 2;

	if (r_p1[1] > r_p2[1])
		edgetableindex += 4;

	pedgetable = &edgetables[edgetableindex];
}


#if 0

void D_PolysetRecursiveDrawLine (int *lp1, int *lp2)
{
	int		d;
	int		new[6];
	int 	ofs;
	
	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	return;	// line is completed

split:
// split this edge
	new[0] = (lp1[0] + lp2[0]) >> 1;
	new[1] = (lp1[1] + lp2[1]) >> 1;
	new[5] = (lp1[5] + lp2[5]) >> 1;
	new[2] = (lp1[2] + lp2[2]) >> 1;
	new[3] = (lp1[3] + lp2[3]) >> 1;
	new[4] = (lp1[4] + lp2[4]) >> 1;

// draw the point
	ofs = d_scantable[new[1]] + new[0];
	if (new[5] > d_pzbuffer[ofs])
	{
		int		pix;
		
		d_pzbuffer[ofs] = new[5];
		pix = skintable[new[3]>>16][new[2]>>16];
//		pix = ((byte *)acolormap)[pix + (new[4] & 0xFF00)];
		d_viewbuffer[ofs] = pix;
	}

// recursively continue
	D_PolysetRecursiveDrawLine (lp1, new);
	D_PolysetRecursiveDrawLine (new, lp2);
}

void D_PolysetRecursiveTriangle2 (int *lp1, int *lp2, int *lp3)
{
	int		d;
	int		new[4];
	
	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;
	return;

split:
// split this edge
	new[0] = (lp1[0] + lp2[0]) >> 1;
	new[1] = (lp1[1] + lp2[1]) >> 1;
	new[5] = (lp1[5] + lp2[5]) >> 1;
	new[2] = (lp1[2] + lp2[2]) >> 1;
	new[3] = (lp1[3] + lp2[3]) >> 1;
	new[4] = (lp1[4] + lp2[4]) >> 1;

	D_PolysetRecursiveDrawLine (new, lp3);

// recursively continue
	D_PolysetRecursiveTriangle (lp1, new, lp3);
	D_PolysetRecursiveTriangle (new, lp2, lp3);
}

#endif

