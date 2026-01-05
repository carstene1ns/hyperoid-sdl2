/* 
 * SDLRoids - An Astroids clone.
 * 
 * Copyright (c) 2000 David Hedbor <david@hedbor.org>
 * 	based on xhyperoid by Russel Marks.
 * 	xhyperoid is based on a Win16 game, Hyperoid by Edward Hutchins 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

/*
 * sdl.c - Graphics handling for SDL.
 */

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <SDL.h>
#include <string.h>

#include "misc.h"	/* for POINT */
#include "getargs.h"
#include "graphics.h"
#include "sdlsound.h"
#include "roidsupp.h"

static SDL_Joystick *joystick = NULL;
static int num_buttons = 0;

#define NUM_BITMAPS	17
/* Loaded BMPs... */
SDL_Texture *bitmaps[NUM_BITMAPS];

static char *gfxname[] = {
  "bmp/num0.bmp",
  "bmp/num1.bmp",
  "bmp/num2.bmp",
  "bmp/num3.bmp",
  "bmp/num4.bmp",
  "bmp/num5.bmp",
  "bmp/num6.bmp",
  "bmp/num7.bmp",
  "bmp/num8.bmp",
  "bmp/num9.bmp",  
  "bmp/blank.bmp",
  "bmp/bomb.bmp",
  "bmp/level.bmp",
  "bmp/life.bmp",
  "bmp/plus.bmp",
  "bmp/score.bmp",
  "bmp/shield.bmp"
};

/* indicies in above array */
#define BMP_NUM0	0
#define BMP_NUM1	1
#define BMP_NUM2	2
#define BMP_NUM3	3
#define BMP_NUM4	4
#define BMP_NUM5	5
#define BMP_NUM6	6
#define BMP_NUM7	7
#define BMP_NUM8	8
#define BMP_NUM9	9
#define BMP_BLANK	10
#define BMP_BOMB	11
#define BMP_LEVEL	12
#define BMP_LIFE	13
#define BMP_PLUS	14
#define BMP_SCORE	15
#define BMP_SHIELD	16

#define BMP_ICON "bmp/icon.bmp"

static SDL_Renderer *renderer;
static SDL_Window *window;
static SDL_Point screen_dims = {0, 0};

static SDL_Color colortable[PALETTE_SIZE];
#define R colortable[current_color].r
#define G colortable[current_color].g
#define B colortable[current_color].b

static int offx=0, offy=0;

static Uint32 current_color; /* current color */
/* this leaves 16 lines at the top for score etc. */
static int width = 480, height = 480, mindimhalf = 240;

/* this oddity is needed to simulate the mapping the original used a
 * Windows call for. MAX_COORD is a power of 2, so when optimised
 * this isn't too evil.
 */
#define USE_CONV_TABLE
#ifdef USE_CONV_TABLE
#define MAX_DRAW_COORD (MAX_COORD+1000)

/* Tables to translate virtual coord to actual coord */
static Sint16 coord2x[MAX_DRAW_COORD*2], coord2y[MAX_DRAW_COORD*2];
static Sint16 *conv_coord2x = coord2x + MAX_DRAW_COORD;
static Sint16 *conv_coord2y = coord2y + MAX_DRAW_COORD;

#define calc_convx(i,x)	coord2x[i] = ((mindimhalf+(x)*(mindimhalf)/MAX_COORD))
#define calc_convy(i,y)	coord2y[i] = ((mindimhalf-(y)*(mindimhalf)/MAX_COORD))

#define convx(x) conv_coord2x[x]
#define convy(y) conv_coord2y[y]
#else
#define convx(x) ((mindimhalf+(x)*(mindimhalf)/MAX_COORD))
#define convy(y) ((mindimhalf-(y)*(mindimhalf)/MAX_COORD))
#endif

#define convr(r)        ((width*r)/MAX_COORD)

static float joy_x = 0.0, joy_y = 0.0;
static int *joy_button;
float IsKeyDown(int key)
{
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  switch(key)
  {
   case KEY_F1:		return keystate[SDL_SCANCODE_F1];
   case KEY_TAB:	return keystate[SDL_SCANCODE_TAB]
			  || (ARG_JSHIELD < num_buttons &&
			      joy_button[ARG_JSHIELD]); 
   case KEY_S:		return keystate[SDLK_s]
			  || (ARG_JBOMB < num_buttons
			      && joy_button[ARG_JBOMB]); 
   case KEY_LEFT:	return keystate[SDL_SCANCODE_LEFT]
			  ? 1 : (joy_x < 0 ? -joy_x : 0 ); 
   case KEY_RIGHT:	return keystate[SDL_SCANCODE_RIGHT]
			  ? 1 : (joy_x > 0 ? joy_x :0); 
   case KEY_DOWN:	return keystate[SDL_SCANCODE_DOWN]
			  ? 1 : (joy_y > 0 ? joy_y : 0 ); 
   case KEY_UP:		return keystate[SDL_SCANCODE_UP]
			  ? 1 : (joy_y < 0 ? -joy_y : 0 ); 
   case KEY_SPACE:	return keystate[SDL_SCANCODE_SPACE]
			  || (ARG_JFIRE < num_buttons
			      && joy_button[ARG_JFIRE]);  
   case KEY_ESC:	return keystate[SDL_SCANCODE_ESCAPE]; 
   default:
    return 0;
  }
}

/* Draw a circle in the current color. Based off the version in SGE. */
static inline void drawcircle(Sint16 x, Sint16 y, Sint32 r)
{
  Sint32 cx = 0;
  Sint32 cy = r;
  Sint32 df = 1 - r;
  Sint32 d_e = 3;
  Sint32 d_se = -2 * r + 5;
  SDL_Point points[8];

  do {
    points[0].x=x+cx; points[0].y=y+cy;
    points[1].x=x-cx; points[1].y=y+cy;
    points[2].x=x+cx; points[2].y=y-cy;
    points[3].x=x-cx; points[3].y=y-cy;
    points[4].x=x+cy; points[4].y=y+cx;
    points[5].x=x+cy; points[5].y=y-cx;
    points[6].x=x-cy; points[6].y=y+cx;
    points[7].x=x-cy; points[7].y=y-cx;
    SDL_RenderDrawPoints(renderer, points, 8);
    
    if (df < 0)  {
      df   += d_e;
      d_e  += 2;
      d_se += 2;
    } else {
      df   += d_se;
      d_e  += 2;
      d_se += 4;
      cy--;
    }
    cx++;
  } while(cx <= cy);
}

inline void ResetRefreshCoords()
{
//
}

inline void RedrawObject() {
//
}

inline void Polyline(POINT *pts,int n)
{
  if(n<2) return;

  SDL_Point points[MAX_PTS];
  for(int i = 0; i < n; i++, pts++) {
    points[i].x = convx(pts->x) + offx;
    points[i].y = convy(pts->y) + offy;
  }
  SDL_SetRenderDrawColor(renderer, R, G, B, 0xff);
  SDL_RenderDrawLines(renderer, points, n);
}

inline void Circle(Sint16 x, Sint16 y, Sint32 r)
{
  x = convx(x) + offx;
  y = convy(y) + offy;
  
  SDL_SetRenderDrawColor(renderer, R, G, B, 0xff);
  drawcircle(x, y,convr(r));
}

inline void SetPixel(Sint16 x, Sint16 y,Uint32 c)
{
  current_color = c;
  x = convx(x);
  y = convy(y);

  if(x >= width || x < 0 ||
     y >= height || y < 0) {
    return;
  } 
  x += offx;
  y += offy;

  SDL_SetRenderDrawColor(renderer, R, G, B, 0xff);
  SDL_RenderDrawPoint(renderer, x, y);
}


inline void set_colour(int c)
{
  current_color = c;
}


/* SetIndicator - set a quantity indicator */

int SetIndicator( char * npBuff, char bitmap, int nQuant )
{
  if (nQuant > 5)
  {
    *npBuff++ = bitmap; *npBuff++ = bitmap;
    *npBuff++ = bitmap; *npBuff++ = bitmap;
    *npBuff++ = BMP_PLUS;
  }
  else
  {
    int nBlank = 5 - nQuant;
    while (nQuant--) *npBuff++ = bitmap;
    while (nBlank--) *npBuff++ = BMP_BLANK;
  }
  return( 5 );
}


/* score_graphics - draw score and rest of status display */

void score_graphics(int level,int score,int lives,int shield,int bomb)
{
  SDL_Rect src, dest;
  static char szScore[40];
  static char ozScore[40];
  char szBuff[sizeof(szScore)];
  char *npBuff = szBuff;
  int nLen, x, myoffx=0, mywidth, xysize;
  float scale;
  memset(ozScore, NUM_BITMAPS, 40);

  *npBuff++ = BMP_LEVEL;
  sprintf( npBuff, "%2.2d", level );
  while (isdigit( *npBuff ))
    *npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
  *npBuff++ = BMP_BLANK;
  *npBuff++ = BMP_SCORE;
  sprintf( npBuff, "%7.7d", score );
  while (isdigit( *npBuff ))
    *npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
  *npBuff++ = BMP_BLANK;
  *npBuff++ = BMP_SHIELD;
  sprintf( npBuff, "%3.3d", shield );
  while (isdigit( *npBuff ))
    *npBuff = (char)(*npBuff + BMP_NUM0 - '0'), ++npBuff;
  *npBuff++ = BMP_BLANK;
  npBuff += SetIndicator( npBuff, BMP_LIFE, lives );
  npBuff += SetIndicator( npBuff, BMP_BOMB, bomb );
  nLen = npBuff - szBuff;
  
  mywidth = nLen * 16;
  if(mywidth > screen_dims.x) {
    scale = (float)screen_dims.x / (float)mywidth;
    xysize = (int)floor(16.0 * scale);
  } else {
    myoffx = (screen_dims.x - mywidth)/2;
    if(myoffx > offx) myoffx = offx;
    xysize = 16;
  }
  src.w = src.h = 16;
  src.x = src.y = 0;
  dest.w = dest.h = xysize;
  dest.y = offy - 2 - xysize;
  if(dest.y < 0) dest.y = 0;

  for(x = 0;x < nLen; x++) {
    if(ozScore[x] != szBuff[x]) {
      ozScore[x] = szBuff[x];
      dest.x = myoffx + x*xysize;
      SDL_RenderCopy(renderer, bitmaps[(int)szBuff[x]], &src, &dest);
    }
  }
}

int bmp2texture(int num)
{
  SDL_Surface *loaded;
  if((loaded = SDL_LoadBMP(gfxname[num])) ||
     (loaded = SDL_LoadBMP(datafilename(NULL, gfxname[num]))) ||
     (loaded = SDL_LoadBMP(datafilename(DATADIR, gfxname[num]))) ||
     (loaded = SDL_LoadBMP(datafilename(bindir, gfxname[num])))) {

    /* Enable the "transparent" color... */
    SDL_SetColorKey(loaded, SDL_TRUE, SDL_MapRGB(loaded->format, 255, 0, 255));
    SDL_Texture *converted = SDL_CreateTextureFromSurface(renderer, loaded);
    SDL_FreeSurface(loaded);
    if(converted) {
      bitmaps[num] = converted;
      return 1;
    }
  }

  fprintf(stderr, "Error loading image %s!\n", gfxname[num]);
  return 0;
}

void load_images()
{
  int f;
  for(f = 0; f < NUM_BITMAPS; f++) {
    if(!bmp2texture(f)) { 
      exit(1);
    }
  }
}

void init_colours(int *palrgb)
{
  for(current_color = 0; current_color < PALETTE_SIZE; current_color++)
  {
    R = ((palrgb[current_color*3  ]<<8)|palrgb[current_color*3  ]);
    G = ((palrgb[current_color*3+1]<<8)|palrgb[current_color*3+1]);
    B = ((palrgb[current_color*3+2]<<8)|palrgb[current_color*3+2]);
  }
}

void draw_boundary_box()
{
  set_colour(RED);
  SDL_SetRenderDrawColor(renderer, R, G, B, 0xff);
  SDL_RenderDrawLine(renderer, 0, 16, width+17, 16);
  SDL_RenderDrawLine(renderer, 0, width+17, width+17, width+17);
  SDL_RenderDrawLine(renderer, 0, 16, 0, width+17);
  SDL_RenderDrawLine(renderer, width+17, 16, width+17, width+17);
}

void calc_boundary_box()
{
#ifdef USE_CONV_TABLE
  int i;
#endif
  int myoffy, myoffx;

  offx=offy=0;
  if(width < (height - 16)) {
    myoffy = (height-width)/2;
    myoffx = 0;
    height = width;    
  }
  else {
    myoffx = (width-height)/2 + 8;
    myoffy = 0;
    height -= 16;
    width = height;
  }
  offx = myoffx+1; offy = myoffy+17;
  height = width -= 2;
  mindimhalf = height/2;

  printf("offx %d, offy %d, width %d, height %d\n", offx, offy, width, height);

#ifdef USE_CONV_TABLE
  for(i = 0; i < (MAX_DRAW_COORD*2); i++)
  {
    int mycoord = i - MAX_DRAW_COORD;
    calc_convx(i, mycoord);
    calc_convy(i, mycoord);
  }
#endif
}

void clear_graphics() {
  SDL_SetRenderDrawColor(renderer, 0,0,0,0);
  SDL_RenderClear(renderer);
}

void init_graphics(int *palrgb)
{
  int window_flags = SDL_WINDOW_RESIZABLE;
  if(ARG_FSCRN) 
    window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  
  if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0 ) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);
  if(ARG_FSCRN) {
    SDL_ShowCursor(0);
  }
  /* Print information about the joysticks */
  if(SDL_NumJoysticks()) {
    printf("There are %d joysticks attached\n", SDL_NumJoysticks());
    for (int i=0; i<SDL_NumJoysticks(); ++i ) {
      const char *name = SDL_JoystickNameForIndex(i);
      printf("Joystick %d = %s",i,name ? name : "Unknown Joystick");
      if(ARG_JLIST == 0 && i == ARG_JOYNUM) {
        joystick = SDL_JoystickOpen(i );
        if(joystick == NULL) {
          printf(" (failed to open)\n");
        } else{ 
          printf(" (opened)\n");
        }
      } else {
        printf("\n");
      }
    }
    if(joystick != NULL) {
      joy_button = calloc(num_buttons = SDL_JoystickNumButtons(joystick), sizeof(int));
      if(ARG_JFIRE >= num_buttons ||
         ARG_JSHIELD >= num_buttons ||
         ARG_JBOMB >= num_buttons) {
        fprintf(stderr, "Selected joystick button out of range (0-%d)\n", num_buttons-1);
        exit(1);
      }
    }
    if(ARG_JLIST) {
      exit(0);
    }
  }

  if(ARG_WIDTH) {
    width  = ARG_WIDTH;
    height = ARG_HEIGHT;
  }
  /* Initialize the display */
  window = SDL_CreateWindow("SDL2Roids " VERSION,
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    width, height, window_flags);
  if (window) {
    renderer = SDL_CreateRenderer(window, -1, 0);
  }
  if(renderer) {
    SDL_RenderSetLogicalSize(renderer, width, height);
  }

  if ( !window || !renderer ) {
    fprintf(stderr, "Couldn't set %dx%d video mode: %s\n",width, height, 
      SDL_GetError());
    exit(1);
  }

  if (window) {
    SDL_Surface *icon;
    if((icon = SDL_LoadBMP(BMP_ICON)) ||
      (icon = SDL_LoadBMP(datafilename(NULL, BMP_ICON))) ||
      (icon = SDL_LoadBMP(datafilename(DATADIR, BMP_ICON))) ||
      (icon = SDL_LoadBMP(datafilename(bindir, BMP_ICON)))) {
      SDL_SetWindowIcon(window, icon);
      SDL_FreeSurface(icon);
    }
  }

  screen_dims.x = width;
  screen_dims.y = height;

  init_colours(palrgb);
  load_images();
  calc_boundary_box();
}

extern int bPaused;

static void Pause(int stop) {
  //POINT Pt = {0, 0};
  if(stop) {
    //    PrintLetters( "PAUSED", Pt, Pt, RED, 600 );
    bPaused = 1;
  } else {
    bPaused = 0;
    //    PrintLetters( "PAUSED", Pt, Pt, BLACK, 600 );
  }
  SDL_PauseAudio(bPaused);
}

void update_graphics(void)
{
  SDL_Event event;
  while ( SDL_PollEvent(&event) ) {
    switch (event.type) {
      case SDL_QUIT:
        SDL_Quit();
        exit(0);
        break;

      case SDL_WINDOWEVENT:
        if(event.window.event == SDL_WINDOWEVENT_SHOWN)
          Pause(0);
        if(event.window.event == SDL_WINDOWEVENT_HIDDEN)
          Pause(1);
        break;
#if 0
     case SDL_VIDEORESIZE: 
      screen = SDL_SetVideoMode(event.resize.w, 
				event.resize.h,
				screen->format->BitsPerPixel,
				screen->flags);
      width = event.resize.w;
      height = event.resize.h;
      /* calculate offsets for boundary box, etc */
      calc_boundary_box();
      break;    
#endif
      /* Keyboard */
      case SDL_KEYDOWN:
        if((event.key.keysym.sym == SDLK_RETURN &&
           event.key.keysym.mod & (KMOD_ALT | KMOD_GUI)) ||
           event.key.keysym.sym == SDLK_BACKSPACE) {
          /* toggle fullscreen*/
          if(!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
            /* currently windowed. */
            SDL_ShowCursor(0);
            SDL_SetWindowGrab(window, SDL_TRUE);
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
          } else {
            SDL_ShowCursor(1);
            SDL_SetWindowGrab(window, SDL_FALSE);
            SDL_SetWindowFullscreen(window, 0);
          }
        } else if(event.key.keysym.sym == SDLK_g &&
                  event.key.keysym.mod & KMOD_CTRL) {
          if(!(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
            if(!SDL_GetWindowGrab(window)) {
              SDL_SetWindowGrab(window, SDL_TRUE);
              SDL_ShowCursor(0);
            } else {
              SDL_SetWindowGrab(window, SDL_FALSE);
              SDL_ShowCursor(1);
            }
          }
        } else if(event.key.keysym.sym == SDLK_z &&
                  event.key.keysym.mod & KMOD_CTRL) {
          Pause(1);
          SDL_MinimizeWindow(window);
        } else if(event.key.keysym.sym == SDLK_PAUSE) {
          Pause(!bPaused);
        } else if(event.key.keysym.sym == SDLK_DOWN ||
                  event.key.keysym.sym == SDLK_UP) {
          loopsam(PTHRUST_CHANNEL,PTHRUST_SAMPLE);
        }
        break;
     case SDL_KEYUP:
      if(event.key.keysym.sym == SDLK_DOWN ||
         event.key.keysym.sym == SDLK_UP) {
        loopsam(PTHRUST_CHANNEL,-1);
      } 
      break;

     /* Joysticks */
     case SDL_JOYAXISMOTION:
      float value = 0;
      if(event.jaxis.value > 5000 || event.jaxis.value < -5000)
        value = event.jaxis.value / 32767.0;
      switch(event.jaxis.axis)
      {
        case 0: joy_x = value; break;
        case 1: joy_y = value; break;
      }
      break;
     case SDL_JOYBUTTONDOWN:
      if(joystick != NULL)
        joy_button[event.jbutton.button] = 1;
      break;
     case SDL_JOYBUTTONUP:
      if(joystick != NULL)
        joy_button[event.jbutton.button] = 0;
      break;
    }
  }

  draw_boundary_box();
  SDL_RenderPresent(renderer);
}

void exit_graphics(void)
{
  for(int i = 0; i < NUM_BITMAPS; i++)
    SDL_DestroyTexture(bitmaps[i]);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  if(joystick != NULL) {
    SDL_JoystickClose(joystick);
    free(joy_button);
  }
}
