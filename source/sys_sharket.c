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

#define LCD_WIDTH 800
#define LCD_HEIGHT 600
#define LCD_SIZE (LCD_WIDTH*LCD_HEIGHT)


// sys_null.h -- null system driver to aid porting efforts
#include "SDL.h"
#include "quakedef.h"
#include "errno.h"
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

void Sys_SharketInput( void );

qboolean			isDedicated = false;

SDL_Surface* semu;
SDL_Surface* sscr;
uint8_t SDL_done;

SDL_Rect rsrc = {
    .x=0,
    .y=0,
    .w=BASEWIDTH,
    .h=BASEHEIGHT
};        
SDL_Rect rdst = {
    .x=(LCD_WIDTH-BASEWIDTH*2)/2,
    .y=(LCD_HEIGHT-BASEHEIGHT*2)/2,
    .w=BASEWIDTH*2,
    .h=BASEHEIGHT*2
};


void *p_sharket_stack_redirect = NULL;
void *p_sharket_membase = NULL;
#define MEMSIZE_MB     10
#define STACKSIZE_KB   256

#define CONSOLE_TO_STDOUT

/*
===============================================================================

FILE IO

===============================================================================
*/

#define	MAX_HANDLES		10
FILE	*sys_handles[MAX_HANDLES];

int		findhandle (void)
{
	int		i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ("out of handles");
	return -1;
}

/*
================
Qfilelength
================
*/
static int Qfilelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;
	
	return Qfilelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE	*f;
	int		i;
	
	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	if ( handle >= 0 ) {
		fclose (sys_handles[handle]);
		sys_handles[handle] = NULL;
	}
}

void Sys_FileSeek (int handle, int position)
{
	if ( handle >= 0 ) {
		fseek (sys_handles[handle], position, SEEK_SET);
	}
}

int Sys_FileRead (int handle, void *dst, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = dst;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle]);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
		
}

int Sys_FileWrite (int handle, void *src, int count)
{
	char *data;
	int size, done;

	size = 0;
	if ( handle >= 0 ) {
		data = src;
		while ( count > 0 ) {
			done = fread (data, 1, count, sys_handles[handle]);
			if ( done == 0 ) {
				break;
			}
			data += done;
			count -= done;
			size += done;
		}
	}
	return size;
}

int	Sys_FileTime (char *path)
{
	FILE	*f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
#ifdef __WIN32__
    mkdir (path);
#else
    mkdir (path, 0777);
#endif
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr; 
    static char data[1024];
    FILE *fp;
    
    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
    fp = fopen(file, "a");
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

int b_valid_timer = 0;

void Sys_Init( void )
{
	
#if FORNSPIRE
	if( has_colors )
	{
		ui32_nspire_cx_timer_load_value = NSPIRE_CX_TIMER_LOAD_VALUE;
		b_valid_timer = 1;

		NSPIRE_CX_TIMER_LOAD_VALUE = 0xffffffff;
		ui32_last_nspire_cx_timer_value = NSPIRE_CX_TIMER_VALUE;
		ui64_nspire_cx_timer_start_value = ui64_nspire_cx_timer_current_value = ui32_last_nspire_cx_timer_value;
	}
#endif
}


void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (char *error, ...)
{
#ifndef WIN32FORNSPIRE
//	nio_console csl;
#endif
	va_list         argptr;
	char rgc_errorstr[ 256 ];
	va_start (argptr,error);
	vsprintf (rgc_errorstr, error,argptr);
	va_end (argptr);

	printf("Sys Error:\n");
	printf( rgc_errorstr );
	printf("\n");

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
#ifdef CONSOLE_TO_STDOUT
    va_list         argptr;
	
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
#endif
}

void Sys_Quit (void)
{
	int i;

	Host_Shutdown();

	for( i = 0; i < MAX_HANDLES; i++ )
	{
		if( sys_handles[ i ] )
		{
			fclose( sys_handles[ i ] );
			sys_handles[ i ] = NULL;
		}
	}
	

	// Free memory and stack
	if( p_sharket_membase )
		free( p_sharket_membase );
	if( p_sharket_stack_redirect )
		free( p_sharket_stack_redirect );
	exit (0);

}

double Sys_FloatTime (void)
{
	struct timeval  tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
	SDL_Delay(1);
}

void Sys_SendKeyEvents (void)
{

    SDL_Event event;
    int sym, state;
     int modstate;

    while (SDL_PollEvent(&event))
    {
        switch (event.type) {

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                sym = event.key.keysym.sym;
                state = event.key.state;
                modstate = SDL_GetModState();
                switch(sym)
                {
                   case SDLK_DELETE: sym = K_DEL; break;
                   case SDLK_BACKSPACE: sym = K_BACKSPACE; break;
                   case SDLK_F1: sym = K_F1; break;
                   case SDLK_F2: sym = K_F2; break;
                   case SDLK_F3: sym = K_F3; break;
                   case SDLK_F4: sym = K_F4; break;
                   case SDLK_F5: sym = K_F5; break;
                   case SDLK_F6: sym = K_F6; break;
                   case SDLK_F7: sym = K_F7; break;
                   case SDLK_F8: sym = K_F8; break;
                   case SDLK_F9: sym = K_F9; break;
                   case SDLK_F10: sym = K_F10; break;
                   case SDLK_F11: sym = K_F11; break;
                   case SDLK_F12: sym = K_F12; break;
                   case SDLK_BREAK:
                   case SDLK_PAUSE: sym = K_PAUSE; break;
                   case SDLK_UP: sym = K_UPARROW; break;
                   case SDLK_DOWN: sym = K_DOWNARROW; break;
                   case SDLK_RIGHT: sym = K_RIGHTARROW; break;
                   case SDLK_LEFT: sym = K_LEFTARROW; break;
                   case SDLK_INSERT: sym = K_INS; break;
                   case SDLK_HOME: sym = K_HOME; break;
                   case SDLK_END: sym = K_END; break;
                   case SDLK_PAGEUP: sym = K_PGUP; break;
                   case SDLK_PAGEDOWN: sym = K_PGDN; break;
                   case SDLK_RSHIFT:
                   case SDLK_LSHIFT: sym = K_SHIFT; break;
                   case SDLK_RCTRL:
                   case SDLK_LCTRL: sym = K_CTRL; break;
                   case SDLK_RALT:
                   case SDLK_LALT: sym = K_ALT; break;
                   case SDLK_KP0: 
                       if(modstate & KMOD_NUM) sym = K_INS; 
                       else sym = SDLK_0;
                       break;
                   case SDLK_KP1:
                       if(modstate & KMOD_NUM) sym = K_END;
                       else sym = SDLK_1;
                       break;
                   case SDLK_KP2:
                       if(modstate & KMOD_NUM) sym = K_DOWNARROW;
                       else sym = SDLK_2;
                       break;
                   case SDLK_KP3:
                       if(modstate & KMOD_NUM) sym = K_PGDN;
                       else sym = SDLK_3;
                       break;
                   case SDLK_KP4:
                       if(modstate & KMOD_NUM) sym = K_LEFTARROW;
                       else sym = SDLK_4;
                       break;
                   case SDLK_KP5: sym = SDLK_5; break;
                   case SDLK_KP6:
                       if(modstate & KMOD_NUM) sym = K_RIGHTARROW;
                       else sym = SDLK_6;
                       break;
                   case SDLK_KP7:
                       if(modstate & KMOD_NUM) sym = K_HOME;
                       else sym = SDLK_7;
                       break;
                   case SDLK_KP8:
                       if(modstate & KMOD_NUM) sym = K_UPARROW;
                       else sym = SDLK_8;
                       break;
                   case SDLK_KP9:
                       if(modstate & KMOD_NUM) sym = K_PGUP;
                       else sym = SDLK_9;
                       break;
                   case SDLK_KP_PERIOD:
                       if(modstate & KMOD_NUM) sym = K_DEL;
                       else sym = SDLK_PERIOD;
                       break;
                   case SDLK_KP_DIVIDE: sym = SDLK_SLASH; break;
                   case SDLK_KP_MULTIPLY: sym = SDLK_ASTERISK; break;
                   case SDLK_KP_MINUS: sym = SDLK_MINUS; break;
                   case SDLK_KP_PLUS: sym = SDLK_PLUS; break;
                   case SDLK_KP_ENTER: sym = SDLK_RETURN; break;
                   case SDLK_KP_EQUALS: sym = SDLK_EQUALS; break;
                }
                // If we're not directly handled and still above 255
                // just force it to 0
                if(sym > 255) sym = 0;
                Key_Event(sym, state);
                break;

            case SDL_QUIT:
                CL_Disconnect ();
                Host_ShutdownServer(false);        
                Sys_Quit ();
                break;
            default:
                break;
        }
    }
}


void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}


void Sys_NSpireInput( void )
{
	int i_idx;
}

//=============================================================================
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

void main_s( int argc, char **argv )
{
	// init SDL
	SDL_done = 0;
	printf("SDL Init\n");
	SDL_Event event;
	uint8_t done = false;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "could not initialize sdl: %s\n", SDL_GetError());
        return;
    }
    sscr = SDL_SetVideoMode(LCD_WIDTH, LCD_HEIGHT,8,false);
    semu = SDL_CreateRGBSurface(SDL_SWSURFACE,BASEWIDTH, BASEHEIGHT,8,0,0,0,0);
	memset(semu->pixels,0xFF,sizeof(semu->pixels));
	memset(sscr->pixels,0xFF,sizeof(sscr->pixels));
    if ((sscr == NULL)|| (semu == NULL)) {
        fprintf(stderr, "could not create surface: %s\n", SDL_GetError());
        return;
    }
	SDL_Flip(sscr);

	char *pc_basedir_term;
	static quakeparms_t    parms;
	unsigned int i = 0;
	double f64_start, f64_end, f64_delta;

	// char rgc_test[ 128 ];

	printf("main_s\n" );

	Sys_Init();
	// lcd_incolor();

	printf("main_s alloc\n" );

	parms.memsize = MEMSIZE_MB*1024*1024;
	parms.membase = p_sharket_membase = calloc (1, parms.memsize);
	parms.basedir = ".";

	if( !p_sharket_membase ) {
		Sys_Error( "could not alloc %.2f kb of memory\n", ( ( double )parms.memsize ) / 1024.0 );
	}

	pc_basedir_term = argv[ 0 ] + Q_strlen( argv[ 0 ] );
	while( pc_basedir_term >= argv[ 0 ] )
	{
		if( *pc_basedir_term == '/' || *pc_basedir_term == '\\' )
		{
			*pc_basedir_term = 0;
			break;
		}
		pc_basedir_term--;
	}
	parms.basedir = argv[ 0 ];

	/*
	for( i = 0; i < parms.memsize; i++ )
	{
		( ( unsigned char * )parms.membase )[ i ] = 0;
	}
	*/

	printf("membase: %p\n", parms.membase );
	printf("argc: %d\n", argc );

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	printf ("Host_Init\n");
	/* printf ("Chained..\n");*/

	Host_Init (&parms);
	f64_start = f64_end = Sys_FloatTime();
	printf ("Frame Starts\n");
	while (1)
	{
		f64_delta = f64_end - f64_start;
		SDL_SoftStretch(semu, &rsrc, sscr, &rdst);
		SDL_Flip(sscr);
		Host_Frame ( f64_delta );
		f64_start = f64_end;
		f64_end = Sys_FloatTime();
	}
}

int main (int argc, char **argv) {
	p_sharket_stack_redirect = calloc( 1, STACKSIZE_KB*1024 );
	main_s( argc, argv );
	// free( p_sharket_stack_redirect );
	return 0;
}


