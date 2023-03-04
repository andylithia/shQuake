#include "quakedef.h"

#include "d_local.h"

extern int bbextents;
extern int bbextentt;



void draw_span_nspire_fw_8( byte *pdest, byte *pbase, fixed16_t f16_rps, fixed16_t f16_sstep, fixed16_t f16_rpt, fixed16_t f16_tstep, int i_cachewidth )
{
	byte ui8_pel;

	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest++ = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
}


void draw_span_nspire_bw_8( byte *pdest, byte *pbase, fixed16_t f16_rps, fixed16_t f16_sstep, fixed16_t f16_rpt, fixed16_t f16_tstep, int i_cachewidth )
{
	byte ui8_pel;

	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
	ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
	*pdest-- = ui8_pel;
	f16_rps += f16_sstep;
	f16_rpt += f16_tstep;
}

static void draw_span_nspire_fw_s( byte *pdest, byte *pbase, fixed16_t f16_rps, fixed16_t f16_sstep, fixed16_t f16_rpt, fixed16_t f16_tstep, int i_cachewidth, int i_count )
{
	byte ui8_pel;

	switch( i_count )
	{
	case 8:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 7:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 6:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 5:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 4:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 3:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 2:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 1:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest++ = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	}
}

static void draw_span_nspire_bw_s( byte *pdest, byte *pbase, fixed16_t f16_rps, fixed16_t f16_sstep, fixed16_t f16_rpt, fixed16_t f16_tstep, int i_cachewidth, int i_count )
{
	byte ui8_pel;

	switch( i_count )
	{
	case 8:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 7:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 6:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 5:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 4:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 3:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 2:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	case 1:
		ui8_pel = *(pbase + (f16_rps >> 16) + (f16_rpt >> 16) * i_cachewidth);
		*pdest-- = ui8_pel;
		f16_rps += f16_sstep;
		f16_rpt += f16_tstep;
	}
}



#define NSPIRE_NOCLIP_FTW 1

void draw_span_nspire_fw_c( draw_span8_nspire_t *ps_dset )
{
	int spancount;
	fixed16_t f16_psnext;
	fixed16_t f16_ptnext;
	register int f16_rps, f16_rpt, f16_sstep, f16_tstep;

	ps_dset->f16_s = 0;
	ps_dset->f16_t = 0;
	do
	{
	// calculate s and t at the far end of the span
		if (ps_dset->count >= 8)
			spancount = 8;
		else
			spancount = ps_dset->count;

		ps_dset->count -= spancount;

		if ( ps_dset->count)
		{
			fixed16_t i_ipersp;
			ps_dset->f16_p = ( ps_dset->f16_p + ( ps_dset->f16_stepp ) );
			i_ipersp = udiv_fast_32_32_incorrect( ps_dset->f16_p, 0xffffffffU );

			ps_dset->f16_s = ( ps_dset->f16_s + ( ps_dset->f16_steps << 3 ) );
			f16_psnext = ps_dset->f16_sstart + llmull_s16( ps_dset->f16_s, i_ipersp );
#if !NSPIRE_NOCLIP_FTW
			if( f16_psnext > ps_dset->i_bbextents )
			{
				f16_psnext = ps_dset->i_bbextents;
			}
			if( f16_psnext < 8 )
			{
				f16_psnext = 8;
			}
#endif
			f16_sstep = ( f16_psnext - ps_dset->f16_ps ) >> 3;
			f16_rps = ps_dset->f16_ps;
			ps_dset->f16_ps = f16_psnext;


			ps_dset->f16_t = ( ps_dset->f16_t + ( ps_dset->f16_stept << 3 ) );
			f16_ptnext = ps_dset->f16_tstart + llmull_s16( ps_dset->f16_t, i_ipersp );
#if !NSPIRE_NOCLIP_FTW
			if( f16_ptnext > ps_dset->i_bbextentt )
			{
				f16_ptnext = ps_dset->i_bbextentt;
			}
			if( f16_ptnext < 8 )
			{
				f16_ptnext = 8;
			}
#endif
			f16_tstep = ( f16_ptnext - ps_dset->f16_pt ) >> 3;
			f16_rpt = ps_dset->f16_pt;
			ps_dset->f16_pt = f16_ptnext;

			draw_span_nspire_fw_8( ps_dset->pdest, ps_dset->pbase, f16_rps, f16_sstep, f16_rpt, f16_tstep, ps_dset->i_cachewidth );
			ps_dset->pdest += 8;
		}
		else
		{
			static const int rgi_mtab[ 8 ] = { 0, 65536, 32768, 21845, 16384, 13107, 10922, 9362 };

			if (spancount > 1 )
			{
				f16_psnext = ps_dset->f16_psend;
				f16_sstep = llmull_s16( f16_psnext - ps_dset->f16_ps, rgi_mtab[ spancount - 1 ] );

				f16_ptnext = ps_dset->f16_ptend;
				f16_tstep = llmull_s16( f16_ptnext - ps_dset->f16_pt, rgi_mtab[ spancount - 1 ] );
			}

			draw_span_nspire_fw_s( ps_dset->pdest, ps_dset->pbase, ps_dset->f16_ps, f16_sstep, ps_dset->f16_pt, f16_tstep, ps_dset->i_cachewidth, spancount );
		}
	} while (ps_dset->count > 0);
}


void draw_span_nspire_bw_c( draw_span8_nspire_t *ps_dset )
{
	int spancount;
	fixed16_t f16_psnext;
	fixed16_t f16_ptnext;
	register int f16_rps, f16_rpt, f16_sstep, f16_tstep;

	ps_dset->f16_s = 0;
	ps_dset->f16_t = 0;
	do
	{
	// calculate s and t at the far end of the span
		if (ps_dset->count >= 8)
			spancount = 8;
		else
			spancount = ps_dset->count;

		ps_dset->count -= spancount;

		if ( ps_dset->count)
		{
			fixed16_t i_ipersp;
			ps_dset->f16_p = ( ps_dset->f16_p + ( ps_dset->f16_stepp ) );
			i_ipersp = udiv_fast_32_32_incorrect( ps_dset->f16_p, 0xffffffffU );

			ps_dset->f16_s = ( ps_dset->f16_s + ( ps_dset->f16_steps << 3 ) );
			f16_psnext = ps_dset->f16_sstart + llmull_s16( ps_dset->f16_s, i_ipersp );
#if !NSPIRE_NOCLIP_FTW
			if( f16_psnext > ps_dset->i_bbextents )
			{
				f16_psnext = ps_dset->i_bbextents;
			}
			if( f16_psnext < 8 )
			{
				f16_psnext = 8;
			}
#endif
			f16_sstep = ( f16_psnext - ps_dset->f16_ps ) >> 3;
			f16_rps = ps_dset->f16_ps;
			ps_dset->f16_ps = f16_psnext;


			ps_dset->f16_t = ( ps_dset->f16_t + ( ps_dset->f16_stept << 3 ) );
			f16_ptnext = ps_dset->f16_tstart + llmull_s16( ps_dset->f16_t, i_ipersp );
#if !NSPIRE_NOCLIP_FTW
			if( f16_ptnext > ps_dset->i_bbextentt )
			{
				f16_ptnext = ps_dset->i_bbextentt;
			}
			if( f16_ptnext < 8 )
			{
				f16_ptnext = 8;
			}
#endif
			f16_tstep = ( f16_ptnext - ps_dset->f16_pt ) >> 3;
			f16_rpt = ps_dset->f16_pt;
			ps_dset->f16_pt = f16_ptnext;

			draw_span_nspire_bw_8( ps_dset->pdest, ps_dset->pbase, f16_rps, f16_sstep, f16_rpt, f16_tstep, ps_dset->i_cachewidth );
			ps_dset->pdest -= 8;
		}
		else
		{
			static const int rgi_mtab[ 8 ] = { 0, 65536, 32768, 21845, 16384, 13107, 10922, 9362 };

			if (spancount > 1 )
			{
				f16_psnext = ps_dset->f16_psend;
				f16_sstep = llmull_s16( f16_psnext - ps_dset->f16_ps, rgi_mtab[ spancount - 1 ] );

				f16_ptnext = ps_dset->f16_ptend;
				f16_tstep = llmull_s16( f16_ptnext - ps_dset->f16_pt, rgi_mtab[ spancount - 1 ] );
			}

			draw_span_nspire_bw_s( ps_dset->pdest, ps_dset->pbase, ps_dset->f16_ps, f16_sstep, ps_dset->f16_pt, f16_tstep, ps_dset->i_cachewidth, spancount );
		}
	} while (ps_dset->count > 0);
}
