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
// vid_null.c -- null video driver to aid porting efforts
#include "quakedef.h"
#include "d_local.h"
#include "SDL.h"
// viddef_t	vid;				// global video state


// byte	vid_buffer[BASEWIDTH*BASEHEIGHT];
extern uint8_t SDL_done;
extern SDL_Surface* semu;
extern SDL_Surface* sscr;

short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte	surfcache[SURFCACHE_SIZE_AT_320X200];

unsigned short	d_8to16table[256];
unsigned	    d_8to24table[256];


unsigned char rgui8_palette[ 256 ][ 3 ];
unsigned short rgui16_palette[ 256 ];

byte align256_colormap[ 0x4000 + 255 ];


void VID_UpdatePalette( )
{
	int i;
	for( i = 0; i < 256; i++ )
	{
		rgui16_palette[ i ] = ( rgui8_palette[ i ][ 2 ] >> 3 ) | ( ( rgui8_palette[ i ][ 1 ] >> 2 ) << 5 ) | ( ( rgui8_palette[ i ][ 0 ] >> 3 ) << 11 );
	}
}

void	VID_SetPalette (unsigned char *palette)
{
	int i;
    SDL_Color colors[256];

    for ( i=0; i<256; ++i )
    {
        colors[i].r = *palette++;
        colors[i].g = *palette++;
        colors[i].b = *palette++;
    }
    SDL_SetColors(semu, colors, 0, 256);
    SDL_SetColors(sscr, colors, 0, 256);
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{
	byte *aligned_colormap;
	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;

	/* man the colormap has just been loaded and then we copy it over and let it sit there unused while quake runs.. wasted 16k, sad.. */
	aligned_colormap = ( byte * )( ( ( ( size_t )&align256_colormap[ 0 ] ) + 255 ) & ~0xff );
	memcpy( aligned_colormap, host_colormap, 0x4000 );
	vid.colormap = aligned_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = semu->pixels;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;
	
	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));

	VID_UpdatePalette();
    SDL_ShowCursor(0);
}

void	VID_Shutdown (void)
{
	printf("VID_shutdown\n");
	SDL_Quit();
}

void	VID_Update (vrect_t *rects)
{
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}


