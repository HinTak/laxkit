//
//	
//    The Laxkit, a windowing toolkit
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 3 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>.
//
//    Copyright (C) 2004-2013 by Tom Lechner
//


#include <cstring>
#include <png.h>
#include <cstdio>
#include <cmath>
#include <errno.h>

#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/language.h>

#include <lax/displayer-xlib.h>
#include <lax/displayer-cairo.h>

#include <iostream>
using namespace std;
#define DBG 


//include various backends...
#ifdef LAX_USES_CAIRO
#include <lax/displayer-cairo.h>
#endif

#ifdef LAX_USES_XLIB
#include <lax/displayer-xlib.h>
#endif

#ifdef LAX_USES_GL
//#include <lax/displayer-gl.h>
#endif


//------------------------------------------------------------

namespace Laxkit {

/*! \defgroup laxutils Common Laxkit Utilities
 * This is a bunch of commonly used functions for some drawing helpers,
 * mouse position helpers, and other miscellaneous things.
 *
 * @{
 */

/*! \enum DrawThingTypes
 * \brief Various common things that draw_thing() can draw simply.
 */



//--------------------------------------- Default Renderer Stuff ---------------------------

/*! \typedef Displayer *NewDisplayerFunc(aDrawable *w);
 * \brief Function type to create new Displayer objects.
 */

//! The default Displayer "constructor".
NewDisplayerFunc newDisplayer = NULL; //note: this is here, so that main displayer.cc doesn't have to include subclasses

#ifdef LAX_USES_CAIRO
Displayer *newDisplayer_cairo(aDrawable *w)
{ return new DisplayerCairo(dynamic_cast<anXWindow*>(w),NULL); }
#endif 

#ifdef LAX_USES_XLIB
Displayer *newDisplayer_xlib(aDrawable *w)
{ return new DisplayerXlib(w); }
#endif



//! There can be only one (default displayer).
static Displayer *dp = NULL;


/*! Creates if not existing.
 */
Displayer *GetDefaultDisplayer()
{
	SetNewDisplayerFunc(NULL);
	if (!dp && newDisplayer) dp=newDisplayer(NULL);
	return dp;
}

//! Set the default Displayer, incrementing its count.
/*! Passing NULL will decrement the default, and set default to NULL.
 * This can be used in anXApp::close(), for instance.
 */
int SetDefaultDisplayer(Displayer *displayer)
{
	if (dp==displayer) return 0;
	if (dp) dp->dec_count();
	dp=displayer;
	if (dp) dp->inc_count();
	return 0;
}



//! Set the default "constructor" for Displayer objects.
/*! If func==NULL, then use the default, checks for cairo, then for xlib.
 * Return 0 for success, or 1 for unable to set to non-null.
 */
int SetNewDisplayerFunc(const char *backend)
{
	NewDisplayerFunc func = NULL;
	if (!backend && newDisplayer) return 0;

	if (!backend) backend=LAX_DEFAULT_BACKEND;

	if (!func) {
		if (!strcmp(backend,"cairo")) {
#ifdef LAX_USES_CAIRO
            if (!func) func=newDisplayer_cairo;
#endif
            if (!func) {
                cerr <<" Ack! Trying to initialize cairo displayer, but no cairo in Laxkit!!"<<endl;
                return 1;
            }


        } else if (!strcmp(backend,"xlib")) {

#ifdef LAX_USES_XLIB
            if (!func) func=newDisplayer_xlib;
#endif
            if (!func) {
                cerr <<" Ack! Trying to initialize xlib displayer, but no xlib in Laxkit!!"<<endl;
                return 1;
            }
        }
	}


	newDisplayer=func;
	return 0;
}




//-------------------------- Mouse related functions -------------------------

//! Return the screen pixel color underneath the mouse.
/*! If mouse_id==0, then get color under a default mouse.
 */
unsigned long screen_color_at_mouse(int mouse_id)
{
#ifdef _LAX_PLATFORM_XLIB
	int xx,yy;
	mouseposition(mouse_id, NULL,&xx,&yy,NULL,NULL);
	cout <<"x,y:"<<xx<<","<<yy<<endl;
	XImage *img=XGetImage(anXApp::app->dpy, DefaultRootWindow(anXApp::app->dpy), 
						xx,yy, 1,1, ~0, ZPixmap);
	unsigned long pixel=XGetPixel(img,0,0);
	XDestroyImage(img);

	return pixel;
#else
	return 0;
#endif
}

/*! Return 1 if windows are on different screens, or 0 for success.
 * If from or to is NULL (but not both), then assume you want the root window of whichever is not null.
 *
 * If both are NULL, then just pass x,y to xx,yy.
 *
 * If kid!=NULL, then grab the pointer to the child window of to that contains the coordinates. If the app
 * does not know of any such window, then fill with NULL.
 */
int translate_window_coordinates(anXWindow *from, int x, int y, anXWindow *to, int *xx, int *yy, anXWindow **kid)
{
#ifdef _LAX_PLATFORM_XLIB
	if (from && !from->xlib_window) return 2;
	if (to && !to->xlib_window) return 2;

	if (!from && !to) {
		*xx=x;
		*yy=y;
		return 0;
	}

	Window xfrom, xto;
	if (from) xfrom=from->xlib_window;
	else {
		XWindowAttributes xatts;
		XGetWindowAttributes(anXApp::app->dpy, to->xlib_window, &xatts);
		xfrom=RootWindowOfScreen(xatts.screen);
	}
	if (to) xto=to->xlib_window;
	else {
		XWindowAttributes xatts;
		XGetWindowAttributes(anXApp::app->dpy, from->xlib_window, &xatts);
		xto=RootWindowOfScreen(xatts.screen);
	}

//	Bool XTranslateCoordinates(Display *display, Window src_w, dest_w, int src_x, int src_y, int *dest_x_return, int *dest_y_return, Window
//			              *child_return);

	Window child;
	Bool e=XTranslateCoordinates(anXApp::app->dpy, xfrom,xto, x,y, xx,yy, &child);
	if (kid) {
		if (child) *kid=anXApp::app->findwindow_xlib(child);
		else *kid=NULL;
	}

	return e == True ? 0 : 1;

#else
	return 1;

#endif
}

//! Return whether the mouse is within the bounds of win.
/*!  If mouse_id==0, then find for a default mouse.
 */
int mouseisin(int mouse_id, anXWindow *win)
{
	int x,y;
	if (mouseposition(mouse_id, win,&x,&y,NULL,NULL)==0)
		return x>=0 && x<win->win_w && y>=0 && y<win->win_h;
	return 0;
}
	

//! Find the mouse x,y in win coordinates, also return the pointer mask, and what child it is in if any.
/*! If win==NULL, find coords on the whole screen. Passing NULL for x, y, state, or child is ok.
 * 
 * This will return coordinates in win space even if the mouse is not in that window.
 * If child!=NULL, then assign it to the anXWindow that is a direct child of win that contains the
 * pointer *** is that right??.
 *
 * Please note that if the mouse is on a different screen, the results are
 * meaningless, and 1 is returned, and none of the variables are modified.
 *
 * On success, 0 is returned. Nonzero for error.
 *
 * If mouse_id==0, then find for a default mouse.
 */
int mouseposition(int mouse_id, anXWindow *win, int *x, int *y, unsigned int *state,anXWindow **child,int *screen, ScreenInformation **monitor)//child=NULL
{
	if (win && !win->win_on) return 1;
	
	LaxMouse *mouse = anXApp::app->devicemanager->findMouse(mouse_id);
	if (!mouse) return 2;

	double xx,yy;
	unsigned int mods;
	int er = mouse->getInfo(win, screen, child, &xx,&yy, &mods, NULL, NULL, NULL, monitor);
	if (er != 0) return 3;

	if (state) *state = mods;
	if (x) *x = xx;
	if (y) *y = yy;
	
	return 0;
}



//-----------------------Color and other state stuff

/*! Turn str into LaxCompositeOp, or a number if str seems to be a number.
 * If it doesn't seem to be any of those, return LAXOP_Undefined.
 */
int StringToLaxop(const char *str)
{
	if (!strcasecmp(str, "None")) return LAXOP_None;
	else if (!strcasecmp(str, "Clear")) return LAXOP_Clear;
	else if (!strcasecmp(str, "Source")) return LAXOP_Source;
	else if (!strcasecmp(str, "Copy")) return LAXOP_Over;
	else if (!strcasecmp(str, "Over")) return LAXOP_Over;
	else if (!strcasecmp(str, "In")) return LAXOP_In;
	else if (!strcasecmp(str, "Out")) return LAXOP_Out;
	else if (!strcasecmp(str, "Atop")) return LAXOP_Atop;
	else if (!strcasecmp(str, "Dest")) return LAXOP_Dest;
	else if (!strcasecmp(str, "Dest_over")) return LAXOP_Dest_over;
	else if (!strcasecmp(str, "Dest_in")) return LAXOP_Dest_in;
	else if (!strcasecmp(str, "Dest_out")) return LAXOP_Dest_out;
	else if (!strcasecmp(str, "Dest_atop")) return LAXOP_Dest_atop;
	else if (!strcasecmp(str, "Xor")) return LAXOP_Xor;
	else if (!strcasecmp(str, "Add")) return LAXOP_Add;
	else if (!strcasecmp(str, "Saturate")) return LAXOP_Saturate;
	else if (!strcasecmp(str, "Multiply")) return LAXOP_Multiply;
	else if (!strcasecmp(str, "Screen")) return LAXOP_Screen;
	else if (!strcasecmp(str, "Overlay")) return LAXOP_Overlay;
	else if (!strcasecmp(str, "Darken")) return LAXOP_Darken;
	else if (!strcasecmp(str, "Lighten")) return LAXOP_Lighten;
	else if (!strcasecmp(str, "Color_dodge")) return LAXOP_Color_dodge;
	else if (!strcasecmp(str, "Color_burn")) return LAXOP_Color_burn;
	else if (!strcasecmp(str, "Hard_light")) return LAXOP_Hard_light;
	else if (!strcasecmp(str, "Soft_light")) return LAXOP_Soft_light;
	else if (!strcasecmp(str, "Difference")) return LAXOP_Difference;
	else if (!strcasecmp(str, "Exclusion")) return LAXOP_Exclusion;
	else if (!strcasecmp(str, "Hsl_hue")) return LAXOP_Hsl_hue;
	else if (!strcasecmp(str, "Hsl_saturation")) return LAXOP_Hsl_saturation;
	else if (!strcasecmp(str, "Hsl_color")) return LAXOP_Hsl_color;
	else if (!strcasecmp(str, "Hsl_luminosity")) return LAXOP_Hsl_luminosity;

	char *endptr=NULL;
	int d=strtol(str, &endptr, 10);
	if (endptr!=str) return d;

	return LAXOP_Undefined;
}

/*! Turn function into a string, and write into str_ret, which is a buffer of length len.
 * If it can't fit, return how many chars are needed in len_ret, and return NULL.
 * If it does fit, return actual number
 * of chars used in len_ret, including null termination, and return str_ret.
 *
 * If however str_ret==NULL, then create and return a new char[]. just big enough to fit.
 *
 * Converts ONLY functions that are LaxCompositeOp. If function is LAXOP_MAX or greater,
 * then NULL is returned.
 */
char *LaxopToString(int function, char *str_ret, int len, int *len_ret)
{
	const char *str=NULL;

	if (function==LAXOP_None) str="None";
	else if (function==LAXOP_Undefined) str="Undefined";
	else if (function==LAXOP_Clear) str="Clear";
	else if (function==LAXOP_Source) str="Source";
	else if (function==LAXOP_Over) str="Over";
	else if (function==LAXOP_In) str="In";
	else if (function==LAXOP_Out) str="Out";
	else if (function==LAXOP_Atop) str="Atop";
	else if (function==LAXOP_Dest) str="Dest";
	else if (function==LAXOP_Dest_over) str="Dest_over";
	else if (function==LAXOP_Dest_in) str="Dest_in";
	else if (function==LAXOP_Dest_out) str="Dest_out";
	else if (function==LAXOP_Dest_atop) str="Dest_atop";
	else if (function==LAXOP_Xor) str="Xor";
	else if (function==LAXOP_Add) str="Add";
	else if (function==LAXOP_Saturate) str="Saturate";
	else if (function==LAXOP_Multiply) str="Multiply";
	else if (function==LAXOP_Screen) str="Screen";
	else if (function==LAXOP_Overlay) str="Overlay";
	else if (function==LAXOP_Darken) str="Darken";
	else if (function==LAXOP_Lighten) str="Lighten";
	else if (function==LAXOP_Color_dodge) str="Color_dodge";
	else if (function==LAXOP_Color_burn) str="Color_burn";
	else if (function==LAXOP_Hard_light) str="Hard_light";
	else if (function==LAXOP_Soft_light) str="Soft_light";
	else if (function==LAXOP_Difference) str="Difference";
	else if (function==LAXOP_Exclusion) str="Exclusion";
	else if (function==LAXOP_Hsl_hue) str="Hsl_hue";
	else if (function==LAXOP_Hsl_saturation) str="Hsl_saturation";
	else if (function==LAXOP_Hsl_color) str="Hsl_color";
	else if (function==LAXOP_Hsl_luminosity) str="Hsl_luminosity";

	if (str==NULL) {
		 //op is past max, so bail
		if (len_ret) *len_ret = -1;
		return NULL;
	}

	int l=strlen(str); 
	if (len_ret) *len_ret = l+1;

	if (str_ret==NULL) return newstr(str);
	if (l>=len) return NULL;
	
	sprintf(str_ret, "%s", str);
	return str_ret;
}

//! Just pass mode onto dp->BlendMode(mode).
LaxCompositeOp drawing_function(LaxCompositeOp mode)
{
	return dp->BlendMode(mode);
}

//! Set characteristics of drawn lines.
void drawing_line_attributes(double width, int type, int cap, int join)
{
	dp->LineAttributes(width, type, cap, join);
}


static unsigned long default_bg_color=0;
static unsigned long default_fg_color=0;

//! Set the new default background color, and return the old one.
unsigned long background_color(unsigned long newcolor)
{
	unsigned long l=dp->NewBG(newcolor);
	default_bg_color=newcolor;
	return l;
}

void foreground_color(double r,double g,double b,double a)
{ dp->NewFG(r,g,b,a); }

void background_color(double r,double g,double b)
{ dp->NewBG(r,g,b); }


//! Set the new default foreground color.
/*! Returns the old foreground color.
 */
unsigned long foreground_color(unsigned long newcolor)
{
	unsigned long l=dp->NewFG(newcolor);
	default_fg_color=newcolor;

	int r,g,b;
	colorrgb(newcolor, &r, &g, &b);

	return l;
}



/*! Return a color that should stand out against the given color.
 * If bw then this will always return black or white. Else each color channel
 * is the furthest from the channel value.
 */
unsigned long standoutcolor(const Laxkit::ScreenColor &color, bool bw)
{
    ScreenColor col(0,0,0,65535);

	if (bw) {
		 //convert to grayscale, then pick opposite black or white
		int g=simple_rgb_to_gray(color.red,color.green,color.blue,65535);
		if (g<32768) col.red=col.green=col.blue=65535;
		else col.red=col.green=col.blue=0;

	} else {
		 //for each channel, choose furthest max or min
		int nummax=0;
		if (color.red<32768) { col.red=65535; nummax++; }
		if (color.green<32768) { col.green=65535; nummax++; }
		if (color.blue<32768) { col.blue=65535; nummax++; }
		if (bw) {
			if (nummax<2) col.rgbf(0.,0.,0.,1.); //black
			else col.rgbf(1.,1.,1.,1.);
		}
	}
    return col.Pixel();
}


//! result=a*(1-r)+b*r. Returns result.
ScreenColor *coloravg(ScreenColor *result, ScreenColor *a, ScreenColor *b,float r) //r==.5
{
	 // please note that this assumes color components are 0..0xffff
	result->red  =(unsigned int)(a->red  *(1-r)+b->red  *r);
	result->green=(unsigned int)(a->green*(1-r)+b->green*r);
	result->blue =(unsigned int)(a->blue *(1-r)+b->blue *r);
	result->alpha=(unsigned int)(a->alpha*(1-r)+b->alpha*r);
	return result;
}

//! Return conversion of col values to 8 bit color, as for Xlib's XSetForeground(). Ignores alpha.
unsigned long pixelfromcolor(ScreenColor *col)
{
	 // assumes app does 8 bit rgb
	return rgbcolor(col->red>>8, col->green>>8, col->blue>>8);
}

//defaults are 8 bits per channel, coded as argb.
static unsigned int   red_shift=0;
static unsigned int green_shift=8;
static unsigned int  blue_shift=16;
static unsigned int alpha_shift=24;
static unsigned int   red_mask =0xff;
static unsigned int green_mask =0xff00;
static unsigned int  blue_mask =0xff0000;
static unsigned int alpha_mask =0xff000000;
static unsigned int   red_size =256;
static unsigned int green_size =256;
static unsigned int  blue_size =256;
static unsigned int alpha_size =256;

static unsigned int color_size =256;//currently always 256

//! Set up how to pack colors within a 32 bit location.
/*! This exists to set the proper info to make rgbcolor() and colorrgb() work
 * as expected. This is necessary on very limited displays where Xlib core colors
 * are defined by different levels of masking and shifting. for instance, sometimes
 * pixel colors are defined by rgb in 5-6-5 bits, and not 8 bits per channel.
 *
 * If am==0, then use the remaining unused bits after the rgb for the alpha bits.
 */
void set_color_shift_info(unsigned int rm, unsigned int gm, unsigned int bm, unsigned int am)
{
	red_mask=rm;
	green_mask=gm;
	blue_mask=bm;
	alpha_mask=am;

	unsigned long mask;
	red_shift=green_shift=blue_shift=alpha_shift=0;
	red_size=green_size=blue_size=alpha_size=1;

	mask=red_mask;
	while ((mask&1)==0) { mask>>=1; red_shift++; }
	while (mask && mask&1) { mask>>=1; red_size*=2; }
	
	mask=green_mask;
	while ((mask&1)==0) { mask>>=1; green_shift++; }
	while (mask && mask&1) { mask>>=1; green_size*=2; }
	
	mask=blue_mask;
	while ((mask&1)==0) { mask>>=1; blue_shift++; }
	while (mask && mask&1) { mask>>=1; blue_size*=2; }

	if (!alpha_mask) alpha_mask=~(red_mask|green_mask|blue_mask);
	mask=alpha_mask;
	while ((mask&1)==0) { mask>>=1; alpha_shift++; }
	while (mask && mask&1) { mask>>=1; alpha_size*=2; }


	DBG cerr <<"----color shift setup----"<<endl;
	DBG cerr.setf(ios_base::hex,ios_base::basefield);
	DBG cerr <<"rgba:"<<red_mask<<","<<green_mask<<","<<blue_mask<<","<<alpha_mask<<endl;
	DBG cerr.setf(ios_base::dec,ios_base::basefield);
	DBG cerr <<"red_size:"<<red_size<<" shift:"<<red_shift<<endl;
	DBG cerr <<"green_size:"<<green_size<<" shift:"<<green_shift<<endl;
	DBG cerr <<"blue_size:"<<blue_size<<" shift:"<<blue_shift<<endl;
	DBG cerr <<"alpha_size:"<<alpha_size<<" shift:"<<alpha_shift<<endl;
}

unsigned long coloravg(const ScreenColor &a, const ScreenColor &b,float r)
{
	return coloravg(a.Pixel(), b.Pixel(), r);
}

//! Return a scaled average of 2 colors. Returns equivalent of a*(1-r) + b*r
/*! default is straight average (r=1/2).
 */
unsigned long coloravg(unsigned long a,unsigned long b,float r) //r==.5
{
	int r1,g1,b1,r2,g2,b2;
	colorrgb(a,&r1,&g1,&b1);
	colorrgb(b,&r2,&g2,&b2);
	return rgbcolor((int)(r1*(1-r)+r2*r),(int)(g1*(1-r)+g2*r),(int)(b1*(1-r)+b2*r));
}

//! Decompose a pixel color into separate red, green, and blue components.
/*! Does not check for NULL r, g, or b.
 */
void colorrgb(unsigned long col,int *r,int *g,int *b,int *a)
{
	*r=((col&red_mask)   >> red_shift  ) * color_size / red_size;
	*g=((col&green_mask) >> green_shift) * color_size / green_size;
	*b=((col&blue_mask)  >> blue_shift ) * color_size / blue_size;
	if (a) *a=((col&alpha_mask) >> alpha_shift) * color_size / alpha_size;
}

//! Get a pixel value suitable for X graphics functions from the r,g,b components.
unsigned long rgbcolor(int r,int g,int b)
{
	unsigned long c= (( r * red_size  /color_size) << red_shift )
			+ ((g * green_size/color_size) << green_shift )
			+ ((b * blue_size /color_size) << blue_shift)
			+ ((0xff * alpha_size /color_size) << alpha_shift);

	return c;
}

//! Get pixel value from floating point channel values in range [0..1].
unsigned long rgbcolorf(double r,double g,double b)
{
	unsigned long c= (int(r * (red_size-1)  ) << red_shift )
				   + (int(g * (green_size-1)) << green_shift )
 				   + (int(b * (blue_size-1) ) << blue_shift)
 				   + (int(1.0 * (alpha_size-1) ) << alpha_shift);

	return c;
}

//----------------------------- Various drawing utilities -------------------------


//! Get coordinates for various graphical things. Coordinates are in a square bound by x=[0..scale], y=[0..scale].
/*! This returns points describing one or more closed paths for things like arrows, double arrows, squares, etc.
 *
 * If buffer==NULL and buffer_size<0, then a new flatpoint[] is returned with the coordinates,
 * and n_ret gets how many points there are. 
 * 
 * If buffer==NULL and buffer_size>=0, then ONLY return in n_ret how many points need to be in a buffer.
 * NULL will be the function return value.
 *
 * If buffer!=NULL, then it is assumed that there are buffer_size elements in buffer.
 * If the coordinates will take more than buffer_size, then NULL is returned, and n_ret is set
 * to how many elements the buffer must have. Otherwise, the points are
 * put in buffer (which is also the return value), and the actual number of points is returned in n_ret.
 *
 * Points returned may be composed of either straight or bezier segments. Each path may be either
 * totally bezier based, with c-v-c - c-v-c..., or merely a list of vertex points.
 *
 * You can draw the returned points with, for instance Displayer::drawbez() or Displayer::drawlines().
 *
 * For points returned, their info values have special meanings.
 *   - info&LINE_Vertex means the point is part of a polyline segment
 *   - info&LINE_Bez    means the point is part of a bezier segment
 *   - info&LINE_Closed means this point is the final point in a closed path
 *   - info&LINE_Open    means this point is the final point in an open path
 *
 * Paths are defined clockwise, except that holes in paths are defined counter clockwise.
 *
 * \todo bounds is currently ignored
 */
flatpoint *draw_thing_coordinates(DrawThingTypes thing, flatpoint *buffer, int buffer_size, int *n_ret,double scale,DoubleBBox *bounds)
{
	if (thing==THING_None) {
		*n_ret=0;
		return NULL;
	}

	if (thing==THING_Circle || thing==THING_Circle_X || thing==THING_Circle_Plus) {
		int n=12;
		if (thing==THING_Circle_X) n+=6;
		else if (thing==THING_Circle_Plus) n+=6;
		if ((buffer && buffer_size<n) || (!buffer && buffer_size>=0)) { *n_ret=n; return NULL; }

		*n_ret=n;
		if (!buffer)  buffer=new flatpoint[n];
		bez_circle(buffer,4, .5,.5,.5);
		buffer[11].info|=LINE_Closed|LINE_End;
		
		if (thing==THING_Circle_X) {
			double s=sqrt(2)/4;
			buffer[12].x=.5-s;  buffer[12].y=.5+s;
			buffer[13].x=.5+s;  buffer[13].y=.5-s;  buffer[13].info=LINE_Open;

			buffer[14].x=.5-s;  buffer[14].y=.5-s;
			buffer[15].x=.5+s;  buffer[15].y=.5+s;  buffer[15].info=LINE_Open;
		} else if (thing==THING_Circle_Plus) {
			buffer[12].x=.5;  buffer[12].y=1;
			buffer[13].x=.5;  buffer[13].y=0;  buffer[13].info=LINE_Open;

			buffer[14].x=0;  buffer[14].y=.5;
			buffer[15].x=1;  buffer[15].y=.5;  buffer[15].info=LINE_Open;
		}
		
	} else if (thing==THING_Square) {
		if ((buffer && buffer_size<4) || (!buffer && buffer_size>=0)) { *n_ret=4; return NULL; }
		*n_ret=4;
		if (!buffer) buffer=new flatpoint[4];
		buffer[0]=flatpoint(0,0);
		buffer[1]=flatpoint(0,1);
		buffer[2]=flatpoint(1,1);
		buffer[3]=flatpoint(1,0); buffer[3].info=LINE_Closed;

	} else if (thing==THING_Wrench) {
		 //grabbed from icons/things.svg:
		const char *d="m 280.86887,787.70308 17.16582,-16.37373 c 0,0 5.64467,17.15995 -5.9772,28.77992 -11.62187,11.61714 -27.42471,3.52989 -27.42471,3.52989 l -44.52409,44.52349 c -4.24347,4.26459 -10.51393,5.06684 -15.63423,0 -5.1203,-5.11752 -4.29751,-11.38351 -0.0563,-15.63122 l 44.5241,-44.52629 c 0,0 -8.07651,-15.80293 3.54537,-27.4257 11.62187,-11.61715 29.17446,-5.57916 29.17446,-5.57916 l -16.76917,16.77039 2.70337,13.59007 z";
		int n=0;
		flatpoint *pts = SvgToFlatpoints(d, NULL, 0, NULL,0, &n, true);

		if ((buffer && buffer_size<n) || (!buffer && buffer_size>=0)) {
			delete[] pts;
			*n_ret=n;
			return NULL;
		}

		if (!buffer) buffer=pts;
		else {
			memcpy(buffer, pts, n*sizeof(flatpoint));
			delete[] pts;
		}
		*n_ret=n;

		return buffer;

	} else if (thing==THING_Magnifying_Glass) {
		 //grabbed from icons/things.svg:
		const char *d="m 262.44573,867.24638 c -12.24836,0 -22.17762,9.92926 -22.17762,22.17762 0,12.24837 9.92925,22.17763 22.17762,22.17763 12.24837,0 22.17763,-9.92926 22.17763,-22.17763 0,-12.24837 -9.92926,-22.17762 -22.17763,-22.17762 z m -43.39948,79.87358 c -3.78198,3.8556 -9.71159,5.86615 -14.83189,0.79931 -5.1203,-5.11752 -3.10091,-11.05847 0.74604,-14.83191 l 25.7593,-25.26698 c -3.02489,-5.62842 -4.77034,-11.80857 -4.77034,-18.39638 0,-20.15639 16.33998,-36.49637 36.49637,-36.49637 20.15639,0 36.49638,16.33998 36.49638,36.49637 0,20.15639 -16.33999,36.49638 -36.49638,36.49638 -5.913,0 -11.49756,-1.40619 -16.67463,-4.06909 z";
		int n=0;
		flatpoint *pts = SvgToFlatpoints(d, NULL, 0, NULL,0, &n, true);

		if ((buffer && buffer_size<n) || (!buffer && buffer_size>=0)) {
			delete[] pts;
			*n_ret=n;
			return NULL;
		}

		if (!buffer) buffer=pts;
		else {
			memcpy(buffer, pts, n*sizeof(flatpoint));
			delete[] pts;
		}
		*n_ret=n;

		return buffer;

	} else if (thing==THING_Cancel) {
		 //circle with a line through it. Solid version.
		 //grabbed from icons/things.svg:
		int n=0; 
		const char *d="m 223.60428,976.09028 c -11.76205,11.79065 -14.12366,30.61262 -5.75491,45.01152 l 50.18801,-50.18802 c -14.40647,-8.37312 -32.6438,-6.59923 -44.4331,5.1765 z m 3.86147,55.86812 c 14.6173,11.0438 35.33478,10.0302 48.30368,-2.9096 12.93862,-12.9691 14.14996,-33.88398 3.10657,-48.50069 z m 57.08056,-64.24809 c 19.30709,19.30715 19.30708,50.61019 -3e-5,69.91729 -19.30712,19.3072 -51.20516,19.5055 -70.51233,0.1984 -19.30715,-19.3071 -18.71215,-50.80854 0.59501,-70.11568 19.30714,-19.30714 50.6102,-19.30715 69.91735,-10e-6 z";
		flatpoint *pts = SvgToFlatpoints(d, NULL, 0, NULL,0, &n, true);

		if ((buffer && buffer_size<n) || (!buffer && buffer_size>=0)) {
			delete[] pts;
			*n_ret=n;
			return NULL;
		}

		if (!buffer) buffer=pts;
		else {
			memcpy(buffer, pts, n*sizeof(flatpoint));
			delete[] pts;
		}
		*n_ret=n;

		return buffer;


	} else if (thing==THING_Octagon) {
		if ((buffer && buffer_size<8) || (!buffer && buffer_size>=0)) { *n_ret=8; return NULL; }
		*n_ret=8;
		if (!buffer) buffer=new flatpoint[8];
		double a=1/(1+sqrt(2))/sqrt(2);
		buffer[0]=flatpoint(0,a);
		buffer[1]=flatpoint(0,1-a);
		buffer[2]=flatpoint(a,1);
		buffer[3]=flatpoint(1-a,1);
		buffer[4]=flatpoint(1,1-a);
		buffer[5]=flatpoint(1,a);
		buffer[6]=flatpoint(1-a,0);
		buffer[7]=flatpoint(a,0); buffer[7].info=LINE_Closed;

	} else if (thing==THING_Diamond) {
		if ((buffer && buffer_size<4) || (!buffer && buffer_size>=0)) { *n_ret=4; return NULL; }
		*n_ret=4;
		if (!buffer) buffer=new flatpoint[4];
		buffer[0]=flatpoint(.5,0);
		buffer[1]=flatpoint(0,.5);
		buffer[2]=flatpoint(.5,1);
		buffer[3]=flatpoint(1,.5); buffer[3].info=LINE_Closed;

	} else if (thing==THING_To_Bottom
			|| thing==THING_To_Top
			|| thing==THING_To_Left
			|| thing==THING_To_Right) {
		if ((buffer && buffer_size<7) || (!buffer && buffer_size>=0)) { *n_ret=7; return NULL; }
		*n_ret=7;
		if (!buffer) buffer=new flatpoint[7];
		buffer[0]=flatpoint(0,0);
		buffer[1]=flatpoint(.5,.8);
		buffer[2]=flatpoint(1,0); buffer[2].info=LINE_Closed;
		buffer[3]=flatpoint(0,.8);
		buffer[4]=flatpoint(1,.8);
		buffer[5]=flatpoint(1,1);
		buffer[6]=flatpoint(0,1); buffer[6].info=LINE_Closed;

		if (thing==THING_To_Top || thing==THING_To_Left) {
			for (int c=0; c<7; c++) buffer[c].y=1-buffer[c].y;
		}
		if (thing==THING_To_Left || thing==THING_To_Right) {
			for (int c=0; c<7; c++) { double t=buffer[c].x; buffer[c].x=buffer[c].y; buffer[c].y=t; }
		}

	} else if (thing==THING_Triangle_Down) {
		if ((buffer && buffer_size<3) || (!buffer && buffer_size>=0)) { *n_ret=3; return NULL; }
		*n_ret=3;
		if (!buffer) buffer=new flatpoint[3];
		buffer[0]=flatpoint(0,0);
		buffer[1]=flatpoint(.5,1);
		buffer[2]=flatpoint(1,0); buffer[2].info=LINE_Closed;

	} else if (thing==THING_Triangle_Up) {
		if ((buffer && buffer_size<3) || (!buffer && buffer_size>=0)) { *n_ret=3; return NULL; }
		*n_ret=3;
		if (!buffer) buffer=new flatpoint[3];
		buffer[0]=flatpoint(0,1);
		buffer[1]=flatpoint(1,1);
		buffer[2]=flatpoint(.5,0); buffer[2].info=LINE_Closed;

	} else if (thing==THING_Triangle_Left) {
		if ((buffer && buffer_size<3) || (!buffer && buffer_size>=0)) { *n_ret=3; return NULL; }
		*n_ret=3;
		if (!buffer) buffer=new flatpoint[3];
		buffer[0]=flatpoint(1,1);
		buffer[1]=flatpoint(1,0);
		buffer[2]=flatpoint(0,.5); buffer[2].info=LINE_Closed;

	} else if (thing==THING_Triangle_Right) {
		if ((buffer && buffer_size<3) || (!buffer && buffer_size>=0)) { *n_ret=3; return NULL; }
		*n_ret=3;
		if (!buffer) buffer=new flatpoint[3];
		buffer[0]=flatpoint(0,1);
		buffer[1]=flatpoint(1,.5);
		buffer[2]=flatpoint(0,0); buffer[2].info=LINE_Closed;

	} else if (thing==THING_Plus) {
		if ((buffer && buffer_size<4) || (!buffer && buffer_size>=0)) { *n_ret=4; return NULL; }
		*n_ret=4;
		if (!buffer) buffer=new flatpoint[4];

		buffer[0].x=.5;  buffer[0].y=1;
		buffer[1].x=.5;  buffer[1].y=0;  buffer[1].info=LINE_Open;

		buffer[2].x=0;  buffer[2].y=.5;
		buffer[3].x=1;  buffer[3].y=.5;  buffer[3].info=LINE_Open;

	} else if (thing==THING_X) {
		if ((buffer && buffer_size<4) || (!buffer && buffer_size>=0)) { *n_ret=4; return NULL; }
		*n_ret=4;
		if (!buffer) buffer=new flatpoint[4];

		double s=sqrt(2)/4;
		buffer[0].x=.5-s;  buffer[0].y=.5+s;
		buffer[1].x=.5+s;  buffer[1].y=.5-s;  buffer[1].info=LINE_Open;

		buffer[2].x=.5-s;  buffer[2].y=.5-s;
		buffer[3].x=.5+s;  buffer[3].y=.5+s;  buffer[3].info=LINE_Open;

	} else if (thing==THING_Asterix) {
		if ((buffer && buffer_size<6) || (!buffer && buffer_size>=0)) { *n_ret=6; return NULL; }
		*n_ret=6;
		if (!buffer) buffer=new flatpoint[6];

		double s=sqrt(3)/4;
		buffer[0].x=.25;  buffer[0].y=.5+s;
		buffer[1].x=.75;  buffer[1].y=.5-s;  buffer[1].info=LINE_Open;

		buffer[2].x=.25;  buffer[2].y=.5-s;
		buffer[3].x=.75;  buffer[3].y=.5+s;  buffer[3].info=LINE_Open;

		buffer[4].x=0;  buffer[4].y=.5;
		buffer[5].x=1;  buffer[5].y=.5;  buffer[5].info=LINE_Open;

	} else if (thing==THING_Folder) {
		if ((buffer && buffer_size<7) || (!buffer && buffer_size>=0)) { *n_ret=7; return NULL; }
		*n_ret=7;
		if (!buffer) buffer=new flatpoint[7];

		buffer[0]=flatpoint(0,    .75 );
        buffer[1]=flatpoint(1  ,  .75 );
        buffer[2]=flatpoint(1  ,  .333);
        buffer[3]=flatpoint(.6  , .333);
        buffer[4]=flatpoint(.4  , .15 );
        buffer[5]=flatpoint(.1  , .15 );
        buffer[6]=flatpoint(0,    .333); buffer[6].info|=LINE_Closed;

	} else if (thing==THING_Pause) {
		if ((buffer && buffer_size<8) || (!buffer && buffer_size>=0)) { *n_ret=8; return NULL; }
		*n_ret=8;
		if (!buffer) buffer=new flatpoint[8];

		buffer[0].x=.125; buffer[0].y=0;  
		buffer[1].x=.4;   buffer[1].y=0;  
		buffer[2].x=.4;   buffer[2].y=1;  
		buffer[3].x=.125; buffer[3].y=1;    buffer[3].info=LINE_Closed;

		buffer[4].x=.6;   buffer[4].y=0;  
		buffer[5].x=.875; buffer[5].y=0;  
		buffer[6].x=.875; buffer[6].y=1;  
		buffer[7].x=.6;   buffer[7].y=1;    buffer[7].info=LINE_Closed;

	} else if (thing==THING_Eject) {
		if ((buffer && buffer_size<7) || (!buffer && buffer_size>=0)) { *n_ret=7; return NULL; }
		*n_ret=7;
		if (!buffer) buffer=new flatpoint[7];

		buffer[0].x=0;  buffer[0].y=0;
		buffer[1].x=0;  buffer[1].y=.25;
		buffer[2].x=1;  buffer[2].y=.25;
		buffer[3].x=1;  buffer[3].y=0;   buffer[3].info=LINE_Closed;
		buffer[4].x=0;  buffer[4].y=.33;
		buffer[5].x=.5; buffer[5].y=1;
		buffer[6].x=1;  buffer[6].y=.33; buffer[6].info=LINE_Closed;

	} else if (thing==THING_Double_Triangle_Up) {
		if ((buffer && buffer_size<6) || (!buffer && buffer_size>=0)) { *n_ret=6; return NULL; }
		*n_ret=6;
		if (!buffer) buffer=new flatpoint[6];

		buffer[0].x=0;  buffer[0].y=0;
		buffer[1].x=.5; buffer[1].y=.5;
		buffer[2].x=1;  buffer[2].y=0;   buffer[2].info=LINE_Closed;
		buffer[3].x=0;  buffer[3].y=.5;
		buffer[4].x=.5; buffer[4].y=1;
		buffer[5].x=1;  buffer[5].y=.5; buffer[5].info=LINE_Closed;

	} else if (thing==THING_Double_Triangle_Down) {
		if ((buffer && buffer_size<6) || (!buffer && buffer_size>=0)) { *n_ret=6; return NULL; }
		*n_ret=6;
		if (!buffer) buffer=new flatpoint[6];

		buffer[0].x=0;  buffer[0].y=1;
		buffer[1].x=1;  buffer[1].y=1;
		buffer[2].x=.5; buffer[2].y=.5;  buffer[2].info=LINE_Closed;
		buffer[3].x=0;  buffer[3].y=.5;
		buffer[4].x=1;  buffer[4].y=.5;
		buffer[5].x=.5; buffer[5].y=0; buffer[5].info=LINE_Closed;

	} else if (thing==THING_Double_Triangle_Left) {
		if ((buffer && buffer_size<6) || (!buffer && buffer_size>=0)) { *n_ret=6; return NULL; }
		*n_ret=6;
		if (!buffer) buffer=new flatpoint[6];

		buffer[0].x=.5; buffer[0].y=1;
		buffer[1].x=.5; buffer[1].y=0;
		buffer[2].x=0;  buffer[2].y=.5;  buffer[2].info=LINE_Closed;
		buffer[3].x=1;  buffer[3].y=1;
		buffer[4].x=1;  buffer[4].y=0;
		buffer[5].x=.5; buffer[5].y=.5;  buffer[5].info=LINE_Closed;

	} else if (thing==THING_Double_Triangle_Right) {
		if ((buffer && buffer_size<6) || (!buffer && buffer_size>=0)) { *n_ret=6; return NULL; }
		*n_ret=6;
		if (!buffer) buffer=new flatpoint[6];

		buffer[0].x=0;  buffer[0].y=1;
		buffer[1].x=.5; buffer[1].y=.5;
		buffer[2].x=0;  buffer[2].y=0;  buffer[2].info=LINE_Closed;
		buffer[3].x=.5; buffer[3].y=1;
		buffer[4].x=1;  buffer[4].y=.5;
		buffer[5].x=.5; buffer[5].y=0;  buffer[5].info=LINE_Closed;

	} else if (thing==THING_Pan_Arrows) {
		if ((buffer && buffer_size<25) || (!buffer && buffer_size>=0)) { *n_ret=25; return NULL; }
		*n_ret=25;
		if (!buffer) buffer=new flatpoint[25];

		flatpoint p[25]={flatpoint(6,1),
						flatpoint(8,3),
						flatpoint(7,3),
						flatpoint(7,5),
						flatpoint(9,5),
						flatpoint(9,4),
						flatpoint(11,6),
						flatpoint(9,8),
						flatpoint(9,7),
						flatpoint(7,7),
						flatpoint(7,9),
						flatpoint(8,9),
						flatpoint(6,11),
						flatpoint(4,9),
						flatpoint(5,9),
						flatpoint(5,7),
						flatpoint(3,7),
						flatpoint(3,8),
						flatpoint(1,6),
						flatpoint(3,4),
						flatpoint(3,5),
						flatpoint(5,5),
						flatpoint(5,3),
						flatpoint(4,3),
						flatpoint(6,1)
					};
		for (int c=0; c<25; c++) {
			p[c].x=p[c].x/12;
			p[c].y=p[c].y/12;
		}
		p[24].info=LINE_Closed;
		memcpy(buffer,p,25*sizeof(flatpoint));

	} else if (thing==THING_Check) {
		if ((buffer && buffer_size<4) || (!buffer && buffer_size>=0)) { *n_ret=4; return NULL; }
		*n_ret=4;
		if (!buffer) buffer=new flatpoint[4];

		buffer[0]=flatpoint(.1,.6);
		buffer[1]=flatpoint(.33,.267);
		buffer[2]=flatpoint(.9,.8);
		buffer[3]=flatpoint(.33,.444, LINE_Closed|LINE_End);

	} else if (thing==THING_Locked) {
		if ((buffer && buffer_size<16) || (!buffer && buffer_size>=0)) { *n_ret=16; return NULL; }
		*n_ret=16;
		if (!buffer) buffer=new flatpoint[16];

		buffer[ 0]=flatpoint(.166,.4);
		buffer[ 1]=flatpoint(.166,.1, LINE_Bez);
		buffer[ 2]=flatpoint(.266,0, LINE_Bez);
		buffer[ 3]=flatpoint(.33,0);
		buffer[ 4]=flatpoint(.66,0);
		buffer[ 5]=flatpoint(.734,0, LINE_Bez);
		buffer[ 6]=flatpoint(.834,.1, LINE_Bez);
		buffer[ 7]=flatpoint(.834,.4, LINE_Closed|LINE_End);
		buffer[ 8]=flatpoint(.25,.4);
		buffer[ 9]=flatpoint(.25,.8, LINE_Bez);
		buffer[10]=flatpoint(.75,.8, LINE_Bez);
		buffer[11]=flatpoint(.75,.4);
		buffer[12]=flatpoint(.65,.4);
		buffer[13]=flatpoint(.65,.7, LINE_Bez);
		buffer[14]=flatpoint(.35,.7, LINE_Bez);
		buffer[15]=flatpoint(.35,.4, LINE_Closed|LINE_End);


	} else if (thing==THING_Unlocked) {
		if ((buffer && buffer_size<16) || (!buffer && buffer_size>=0)) { *n_ret=16; return NULL; }
		*n_ret=16;
		if (!buffer) buffer=new flatpoint[16];

		 //pad
		buffer[ 0]=flatpoint(0,.4);
		buffer[ 1]=flatpoint(0,.1, LINE_Bez);
		buffer[ 2]=flatpoint(.1,0, LINE_Bez);
		buffer[ 3]=flatpoint(.164,0);
		buffer[ 4]=flatpoint(.494,0);
		buffer[ 5]=flatpoint(.568,0, LINE_Bez);
		buffer[ 6]=flatpoint(.668,.1, LINE_Bez);
		buffer[ 7]=flatpoint(.668,.4, LINE_Closed|LINE_End);
		 //loop
		buffer[ 8]=flatpoint(.35,.4);
		buffer[ 9]=flatpoint(.35,.9, LINE_Bez);
		buffer[10]=flatpoint(.85,.9, LINE_Bez);
		buffer[11]=flatpoint(.85,.5);
		buffer[12]=flatpoint(.75,.5);
		buffer[13]=flatpoint(.75,.8, LINE_Bez);
		buffer[14]=flatpoint(.45,.8, LINE_Bez);
		buffer[15]=flatpoint(.45,.4, LINE_Closed|LINE_End);

	} else if (thing==THING_Open_Eye) {
		int numlashes=4;
		int n=numlashes*2+6+4;

		if ((buffer && buffer_size<n) || (!buffer && buffer_size>=0)) { *n_ret=n; return NULL; }
		*n_ret=n;
		if (!buffer) buffer=new flatpoint[n];

		double vv=.276; //one side of vector length for bez circle here
		double rr=1/sqrt(2);
		buffer[0]=flatpoint(vv,.5-vv, LINE_Bez);
		buffer[1]=flatpoint(0,.5, LINE_Bez|LINE_Vertex);
		buffer[2]=flatpoint(vv,.5+vv, LINE_Bez);

		buffer[3]=flatpoint(1-vv,.5+vv, LINE_Bez);
		buffer[4]=flatpoint(1,.5, LINE_Bez|LINE_Vertex);
		buffer[5]=flatpoint(1-vv,.5-vv, LINE_Bez|LINE_Closed);

		 //lashes:
		flatvector v;
		int i=6;
		for (double ang=-M_PI/2/3, l=0; l<numlashes; l++, ang+=M_PI/3/(numlashes-1)) {
			v.x=rr*sin(ang); v.y=rr*cos(ang);
			buffer[i++]=flatpoint(.5+  v.x,  v.y);
			buffer[i++]=flatpoint(.5+1.4*v.x,1.4*v.y, LINE_Open);
		}

		 //pupil
		double pr=(rr-.5)/2;
		buffer[i++]=flatpoint(.5,    .5-pr);
		buffer[i++]=flatpoint(.5+pr, .5);
		buffer[i++]=flatpoint(.5, .5+pr);
		buffer[i++]=flatpoint(.5-pr, .5, LINE_Closed);


	} else if (thing==THING_Closed_Eye) {
		int numlashes=4;
		int n=numlashes*2+6;

		if ((buffer && buffer_size<n) || (!buffer && buffer_size>=0)) { *n_ret=n; return NULL; }
		*n_ret=n;
		if (!buffer) buffer=new flatpoint[n];

		double vv=.276; //one side of vector length for bez circle here
		double rr=1/sqrt(2);
		buffer[0]=flatpoint(0,.5, LINE_Bez);
		buffer[1]=flatpoint(0,.5, LINE_Bez|LINE_Vertex);
		buffer[2]=flatpoint(vv,.5-vv, LINE_Bez);
		buffer[3]=flatpoint(1-vv,.5-vv, LINE_Bez);
		buffer[4]=flatpoint(1,.5, LINE_Bez|LINE_Vertex);
		buffer[5]=flatpoint(1,.5, LINE_Bez|LINE_Open);

		 //lashes:
		flatvector v;
		int i=6;
		for (double ang=-M_PI/2/3, l=0; l<numlashes; l++, ang+=M_PI/3/(numlashes-1)) {
			v.x=rr*sin(ang); v.y=rr*cos(ang);
			buffer[i++]=flatpoint(.5+  v.x, 1-v.y);
			buffer[i++]=flatpoint(.5+1.4*v.x,1-1.4*v.y, LINE_Open);
		}




	} else if (thing==THING_Arrow_Left || thing==THING_Arrow_Right || thing==THING_Arrow_Up || thing==THING_Arrow_Down) {
		if ((buffer && buffer_size<7) || (!buffer && buffer_size>=0)) { *n_ret=7; return NULL; }
		*n_ret=7;
		if (!buffer) buffer=new flatpoint[7];

		 //coords for arrow right
		double t=1./3;
		buffer[0]=flatpoint(0,t);
		buffer[1]=flatpoint(0,2*t);
		buffer[2]=flatpoint(2*t,2*t);
		buffer[3]=flatpoint(2*t,1);
		buffer[4]=flatpoint(1,.5);
		buffer[5]=flatpoint(2*t,0);
		buffer[6]=flatpoint(2*t,t); buffer[6].info=LINE_Closed;

		if (thing==THING_Arrow_Left) for (int c=0; c<7; c++) { buffer[c].x=1-buffer[c].x; }
		else if (thing==THING_Arrow_Up) {
			for (int c=0; c<7; c++) { t=buffer[c].y; buffer[c].y=buffer[c].x; buffer[c].x=t; }
		} else if (thing==THING_Arrow_Down) {
			for (int c=0; c<7; c++) { t=buffer[c].y; buffer[c].y=1-buffer[c].x; buffer[c].x=t; }
		}

	} else if (thing==THING_Double_Arrow_Horizontal || thing==THING_Double_Arrow_Vertical) {
		if ((buffer && buffer_size<10) || (!buffer && buffer_size>=0)) { *n_ret=10; return NULL; }
		*n_ret=10;
		if (!buffer) buffer=new flatpoint[10];

		 //coords for arrow left/right
		buffer[0]=flatpoint(0,.5);
		buffer[1]=flatpoint(.25,.75);
		buffer[2]=flatpoint(.25,.625);
		buffer[3]=flatpoint(.75,.625);
		buffer[4]=flatpoint(.75,.75);
		buffer[5]=flatpoint(1,.5);
		buffer[6]=flatpoint(.75,.25);
		buffer[7]=flatpoint(.75,.375);
		buffer[8]=flatpoint(.25,.375);
		buffer[9]=flatpoint(.25,.25); buffer[9].info=LINE_Closed;

		double t;
		if (thing==THING_Double_Arrow_Vertical) { //flip to up and down
			for (int c=0; c<10; c++) { t=buffer[c].y; buffer[c].y=buffer[c].x; buffer[c].x=t; }
		}

	} else {
		DBG cerr <<" *** must fully implement draw_thing_coordinates()"<<endl;
		if (n_ret) *n_ret=0;
		return NULL;
	}

	if (scale<0) {
		 //flip y
		for (int c=0; c<*n_ret; c++) buffer[c].y=1-buffer[c].y;
		if (scale==-1) scale=1; else scale=-scale; //this -1 check to avoid rounding errors, not sure if really necessary
	}
	if (scale!=1) {
		for (int c=0; c<*n_ret; c++) buffer[c]*=scale;
	}


	return buffer;
}

//! Draw a little graphic with current foreground and line width.
/*! Return 1 for drawn, 0 for unknown thing. 
 *
 * bounding box is x:[x-rx,x+rx], y:[y-ry,y+ry].
 *
 * If fill==0, then only the outline is drawn.
 * If fill==1, then the outline is with the current foreground, and the thing
 * is filled also with the current foreground.
 * If fill==2, then the outline is with the current foreground, 
 * and the thing is filled with a background bg.
 * \todo *** implement the different fill options!!
 *
 * thing is any of the DrawThingTypes enum, particularly: \n
 *  circle\n
 *  square\n
 *  diamond\n
 *  triangle pointing up\n
 *  triangle pointing down\n
 *  triangle pointing right\n
 *  triangle pointing left\n
 *  +\n
 *  x\n
 *  *\n
 *  circle with + in it\n
 *  circle with x in it\n
 *
 *  \todo double triangles, triangle pointing to line, eject.  arrows, double sided arrows, pan arrow
 */
int draw_thing(aDrawable *win,double x, double y, double rx, double ry,int fill, DrawThingTypes thing)
{
	dp->MakeCurrent(win);
	dp->drawthing(x,y, rx,ry, fill,thing);
	return 0;
}

//! Draw a thing with outline color fg, and insides color bg.
/*! See the other draw_thing() for the meaning of thing.
 * The default foreground and background colors will be fg and bg after this function is called.
 * This essentially just sets new line width, foreground and background colors, and calls the other draw_thing()
 * with fill=2.
 *
 * Return 1 for thing drawn, or 0 for unknown thing.
 */
int draw_thing(aDrawable *win,double x, double y, double rx, double ry, DrawThingTypes thing,unsigned long fg,unsigned long bg,int lwidth)
{
	dp->MakeCurrent(win);
	dp->drawthing(x,y, rx,ry, thing, fg,bg,lwidth);
	return 0;
}

/*! Fill region x,y,w,h with checkerboard pattern square number of pixels per smallest square.
 */
void fill_faux_transparent(aDrawable *win, ScreenColor &color, int x, int y, int w, int h, int square)
{
	cerr << " *** fill_faux_transparent() deprecated! Fix your code or it will break soon!!"<<endl;
    unsigned int bg1=coloravg(rgbcolorf(.3,.3,.3),color.Pixel(), color.alpha/65535.);
    unsigned int bg2=coloravg(rgbcolorf(.6,.6,.6),color.Pixel(), color.alpha/65535.);
    int ww=square,hh;
    int a=0;

    for (int xx=x; xx<x+w; xx+=square) {
        a=(xx/square)%2;
        hh=square;
        if (xx+ww>x+w) ww=x+w-xx;
        for (int yy=y; yy<y+h; yy+=square) {
            if (yy+hh>y+h) hh=y+h-yy;
            foreground_color(a ? bg1 : bg2);
            fill_rectangle(win, xx,yy,ww,hh);
            a=!a;
        }
        ww=square;
    }
}

void fill_with_transparency(aDrawable *win, ScreenColor &color, double square, double x,double y,double w,double h)
{
	cerr << " *** fill_with_transparency() deprecated! Fix your code or it will break soon!!"<<endl;

    unsigned int bg1=coloravg(rgbcolorf(.3,.3,.3),color.Pixel(), color.alpha/65535.);
    unsigned int bg2=coloravg(rgbcolorf(.6,.6,.6),color.Pixel(), color.alpha/65535.);
    int ww=square,hh;
    int a=0;

    for (double xx=x; xx<x+w; xx+=square) {
        a=int(xx/square)%2;
        hh=square;
        if (xx+ww>x+w) ww=x+w-xx;
        for (int yy=y; yy<y+h; yy+=square) {
            if (yy+hh>y+h) hh=y+h-yy;
            foreground_color(a ? bg1 : bg2);
            fill_rectangle(win, xx,yy,ww,hh);
            a=!a;
        }
        ww=square;
    }
}


/*! which must be COLOR_None, COLOR_Knockout, or COLOR_Registration.
 */
void draw_special_color(aDrawable *win, int which, double square, double x, double y, double w, double h)
{
    Displayer *dp=GetDefaultDisplayer();
    dp->MakeCurrent(win);//should have been done already

    if (which==COLOR_None) {
		 //none
        dp->NewFG(~0);
        dp->drawrectangle(x,y,w,h, 1);

        int ll=3;
        x+=ll/2; y+=ll/2; w-=ll; h-=ll;
        dp->LineAttributes(ll,LineSolid,LAXCAP_Round,LAXJOIN_Round);
        dp->NewFG(0,0,0);
        dp->drawline(x+ll/2,y, x+ll/2+w,y+h);
        dp->drawline(x+ll/2,y+h, x+ll/2+w,y);

        dp->NewFG(1.,0.,0.);
        dp->drawline(x,y, x+w,y+h);
        dp->drawline(x,y+h, x+w,y);

    } else if (which==COLOR_Registration) {
		 //registration
        dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
        dp->NewFG(~0);
        dp->drawrectangle(x,y,w,h, 1);
        int ww=w;
        if (h<ww) ww=h;
        dp->NewFG(0,0,0);
        dp->drawline(x+w/2,      y+h/2-ww/2,  x+w/2     , y+h/2+ww/2);
        dp->drawline(x+w/2-ww/2, y+h/2     ,  x+w/2+ww/2, y+h/2     );
        dp->drawpoint(x+w/2,y+h/2, ww/4, 0);

    } else if (which==COLOR_Knockout) {
		 //knockout
        dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
        ScreenColor color(0,0,0,0);
        fill_with_transparency(win,color,square, x,y,w,h);
        //dp->drawrectangle(x,y,w,h, 1);

        dp->NewFG(1.,1.,1.);
        dp->moveto(x,y+h*.5);
        dp->lineto(x,y);
        dp->lineto(x+w,y);
        dp->lineto(x+w,y+h*.25);
        dp->curveto(flatpoint(x+w*.6,y+h*.75), flatpoint(x+w*.6,y), flatpoint(x,y+h*.5));
        dp->closed();
        dp->fill(0);

        dp->moveto(x,y+h);
        dp->lineto(x+w,y+h);
        dp->lineto(x+w,y+h*.5);
        dp->curveto(flatpoint(x+w*.6,y+h), flatpoint(x+w*.6,y+h*.333), flatpoint(x,y+h));
        dp->closed();
        dp->fill(0);

    }
	
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
}

//! Clear the window area.
/*! Just call dp->ClearWindow() after dp->MakeCurrent(win).
 */
void clear_window(anXWindow *win)
{
	dp->MakeCurrent(win);
	dp->ClearWindow();
}

//! Draw a rectangle on win with the current foreground color.
void draw_rectangle(aDrawable *win, double x, double y, double w, double h)
{ 
	dp->MakeCurrent(win);
	dp->drawrectangle(x,y,w,h, 0);
}

//! Fill a rectangle on win with the current foreground color.
void fill_rectangle(aDrawable *win, double x, double y, double w, double h)
{ 
	dp->MakeCurrent(win);
	dp->drawrectangle(x,y,w,h, 1);
}

//! Draw a line in screen coordinates.
void draw_line(aDrawable *win, double x1,double y1, double x2,double y2)
{
	dp->MakeCurrent(win);
	dp->drawline(x1,y1, x2,y2);
}

//! Draw an optionally filled arc. Angles are in radians.
void draw_arc(aDrawable *win, double x,double y, double xradius, double yradius, double start_radians, double end_radians)
{
	dp->MakeCurrent(win);
	dp->drawarc(flatpoint(x,y), xradius,yradius, start_radians,end_radians);
}

void fill_arc(aDrawable *win, double x,double y, double xradius, double yradius, double start_radians, double end_radians)
{
	dp->MakeCurrent(win);
	dp->drawellipse(x,y, xradius,yradius, start_radians,end_radians, 1);
}

void draw_arc_wh(aDrawable *win, double x,double y, double width, double height, double start_radians, double end_radians)
{ draw_arc(win, x+width/2,y+height/2,width/2,height/2,start_radians,end_radians); }

void fill_arc_wh(aDrawable *win, double x,double y, double width, double height, double start_radians, double end_radians)
{ fill_arc(win, x+width/2,y+height/2,width/2,height/2,start_radians,end_radians); }

//! Draw a line connecting the dots in p.
/*! If isclosed, then make sure that the final point connects to the initial point.
 */
void draw_lines(aDrawable *win, flatpoint *p, int n, int isclosed)
{
	dp->MakeCurrent(win);
	dp->drawlines(p,n, isclosed,0);
}

//! Fill a polygon line connecting the dots in p with the current foreground color.
void fill_polygon(aDrawable *win, flatpoint *p, int n)
{
	dp->MakeCurrent(win);
	dp->drawlines(p,n, 1,1);
}

//-------------------------- Text drawing utilities -----------------------------

LaxFont *get_default_font()
{
	//return dp->???
	return anXApp::app->defaultlaxfont;
}

//! Find the text extent with the default font for the given utf8 text.
/*! If len<0, then use strlen(str). len is number of bytes, not number of utf8 characters.
 *
 * fasc,fdes are for font, not for actual visual text bounds.
 *  r=1 means use visual (r for real) ascent/descent rather than that for font.
 *
 *  Returns the x advance. The actual width is put in ex.
 *
 *  \todo r doesn't work any more. returns height, but not sure how to get actual visual ascent and descent!
 *  \todo this is a little messy with mixup between advance and extent.
 */
double getextent(const char *str,int len,double *ex,double *ey,double *fasc,double *fdes,char r)
{
	return getextent(NULL,str,len,ex,ey,fasc,fdes,r);
}

//! Find the text extent with the given font for the given utf8 text.
/*! If font==NULL, then use the default font.
 *
 * If len<0, then use strlen(str). len is number of bytes, not number of utf8 characters.
 *
 * fasc,fdes are for font, not for actual visual text bounds.
 *  r=1 means use visual (r for real) ascent/descent rather than that for font.
 *
 *  Returns the x advance. The actual width is put in ex.
 *
 *  \todo r doesn't work any more. returns height, but not sure how to get actual visual ascent and descent!
 *  \todo this is a little messy with mixup between advance and extent.
 */
double getextent(LaxFont *font, const char *str,int len,double *ex,double *ey,double *fasc,double *fdes,char r)
{
	return dp->textextent(font, str,len, ex,ey,fasc,fdes,r);
}

double text_height()
{ return dp->textheight(); }


//! Write one line of utf8 text out with the default LaxFont. Uses foreground color only.
/*! Defaults to center alignment. align can be set to some or'd combination of LAX_LEFT, LAX_HCENTER,
 *  LAX_RIGHT, LAX_TOP, LAX_VCENTER, LAX_BASELINE, LAX_BOTTOM. Note that LAX_CENTER is the same
 *  as LAX_HCENTER|LAX_VCENTER. These center, right, baseline, etc. are
 *  relative to the given x and y coordinates. 
 *
 *  Writes the first len bytes of thetext. If len is equal to zero, nothing is drawn. 
 *  If len is less than 0, then strlen(thetext) is used.
 *
 *  Returns the pixel length of the string.
 */
double textout(aDrawable *win,const char *thetext,int len,double x,double y,unsigned long align)
{
	return textout(win,NULL,thetext,len,x,y,align);
}

double textout(aDrawable *win,LaxFont *font, const char *thetext,int len,double x,double y,unsigned long align)
{
	dp->MakeCurrent(win);
	if (font) dp->font(font);
	double ret=dp->textout(x,y, thetext,len, align);
	if (font) dp->font(anXApp::app->defaultlaxfont,-1);
	return ret;
}

double textout_matrix(aDrawable *win, double *m,const char *thetext,int len,double x,double y,unsigned long align)
{
	dp->MakeCurrent(win);
	return dp->textout(m, x,y, thetext,len, align);
}

double textout_rotated(aDrawable *win, double radians,const char *thetext,int len,double x,double y,unsigned long align)
{
	dp->MakeCurrent(win);
	return dp->textout(radians, x,y, thetext,len, align);
}

double textout_rotated(aDrawable *win, LaxFont *font, double radians,const char *thetext,int len,double x,double y,unsigned long align)
{
	dp->MakeCurrent(win);
	dp->font(font);
	double ret=dp->textout(radians, x,y, thetext,len, align);
	if (font) dp->font(anXApp::app->defaultlaxfont,-1);
	return ret;
}

//! Write out possibly many lines of text. Each line delimited with a '\n'.
double textout_multiline(aDrawable *win,const char *thetext,int len,double x,double y,unsigned long align)
{
	dp->MakeCurrent(win);
	return dp->textout(x,y, thetext,len, align);
}

//! Figure out the extent and placement of an image and a label.
/*! This lays out a text label (IBUT_TEXT_ONLY), an icon (IBUT_ICON_ONLY), 
 * an icon then horizontally a label (IBUT_ICON_TEXT), or label then icon (IBUT_TEXT_ICON).
 * gap goes between the icon and label if both are used.
 *
 * If IBUT_ICON_ONLY is requested, but the image is not available, then the text is used
 * instead. If neither is available, or if IBUT_TEXT_ONLY and the text is not available, then
 * use neither.
 *
 * If either the image or the text are not used, then LAX_WAY_OFF (==-10000) is returned for their x values.
 *
 * This function assumes a utf8 label, using app->defaultfont->font
 *
 * \todo implement all of:
 *    LAX_ICON_ONLY        (0)
 *    LAX_TEXT_ONLY        (1)
 *    LAX_TEXT_ICON        (2)
 *    LAX_ICON_TEXT        (3)
 *    LAX_ICON_OVER_TEXT   (4)
 *    LAX_TEXT_OVER_ICON   (5)
 *    LAX_ICON_STYLE_MASK  (7)
 */
void get_placement(LaxImage *image,const char *label,int gap,unsigned int how,
					int *w,int *h,int *tx,int *ty,int *ix,int *iy)
{
	LaxImage *i=NULL;
	const char *l=NULL;
	if (!image || how==LAX_TEXT_ONLY || how==LAX_TEXT_ICON || how==LAX_ICON_TEXT) l=label;
	if (image && (how==LAX_ICON_ONLY || how==LAX_TEXT_ICON || how==LAX_ICON_TEXT)) i=image;
	double th=0,tw=0,iw=0,ih=0,hh;
	if (l) getextent(l,-1,&tw,&th,NULL,NULL,0);
	if (i) { iw=image->w(); ih=image->h(); }
	hh=(ih>th?ih:th);
	if (h) *h=hh;
	if (ty) *ty=(hh-th)/2;
	if (iy) *iy=(hh-ih)/2;
	if (l && i) {
		if (w) *w=iw+tw+gap;
		if (how==LAX_TEXT_ICON) {
			if (tx) *tx=0;
			if (ix) *ix=tw+gap;
		} else {
			if (ix) *ix=0;
			if (tx) *tx=iw+gap;
		}
	} else if (l) { 
		if (w) *w=tw;
		if (tx) *tx=0; 
		if (ix) *ix=LAX_WAY_OFF;
	} else if (i) {
		if (w) *w=iw;
		if (ix) *ix=0;
		if (tx) *tx=LAX_WAY_OFF;
	} else {
		if (tx) *tx=LAX_WAY_OFF;
		if (ix) *ix=LAX_WAY_OFF;
	}
}

//! Figure out how to place a box with dimensions thingw,thingh next to a text label.
/*! See the other get_placement() for info about how.
 */
void get_placement(int thingw,int thingh,const char *label,int gap,unsigned int how,
					int *w,int *h,int *tx,int *ty,int *ix,int *iy)
{
	const char *l=NULL;
	int usei=0;
	if (how==LAX_TEXT_ONLY || how==LAX_TEXT_ICON || how==LAX_ICON_TEXT) l=label;
	if (how==LAX_ICON_ONLY || how==LAX_TEXT_ICON || how==LAX_ICON_TEXT) usei=1;
	double th=0,tw=0,iw=0,ih=0,hh=0;

	if (l) getextent(l,-1,&tw,&th,NULL,NULL,0);

	iw=thingw; ih=thingh;
	hh=(ih>th?ih:th);
	if (h) *h=hh;
	if (ty) *ty=(hh-th)/2;
	if (iy) *iy=(hh-ih)/2;

	if (l && usei) {
		if (w) *w=iw+tw+gap;
		if (how==LAX_TEXT_ICON) {
			if (tx) *tx=0;
			if (ix) *ix=tw+gap;
		} else {
			if (ix) *ix=0;
			if (tx) *tx=iw+gap;
		}
	} else if (l) { 
		if (w) *w=tw;
		if (tx) *tx=0; 
		if (ix) *ix=LAX_WAY_OFF;
	} else if (usei) {
		if (w) *w=iw;
		if (ix) *ix=0;
		if (tx) *tx=LAX_WAY_OFF;
	}
}

/*! From things like LAX_LRTB, return something like "lrtb".
 * If dir is not one of the 8 flow directions, NULL is returned.
 */
const char *flow_name(int direction)
{
	if (direction==LAX_LRTB) return "lrtb";
	else if (direction==LAX_LRBT) return "lrbt";
	else if (direction==LAX_RLTB) return "rltb";
	else if (direction==LAX_RLBT) return "rlbt";
	else if (direction==LAX_TBLR) return "tblr";
	else if (direction==LAX_TBRL) return "tbrl";
	else if (direction==LAX_BTLR) return "btlr";
	else if (direction==LAX_BTRL) return "btrl";
	return NULL;
}

/*! Return a human readable string for dir.
 */
const char *flow_name_translated(int direction)
{
	if (direction==LAX_LRTB) return _("Left to right, top to bottom");
	else if (direction==LAX_LRBT) return _("Left to right, bottom to top");
	else if (direction==LAX_RLTB) return _("Right to left, top to bottom");
	else if (direction==LAX_RLBT) return _("Right to left, bottom to top");
	else if (direction==LAX_TBLR) return _("Top to bottom, left to right");
	else if (direction==LAX_TBRL) return _("Top to bottom, right to left");
	else if (direction==LAX_BTLR) return _("Bottom to top, left to right");
	else if (direction==LAX_BTRL) return _("Bottom to top, right to left");
	return NULL;
}

/*! From "lrtb" return LAX_LRTB, for instance.
 * The 4 character codes are caselessly checked.
 *
 * Unknown dir returns -1.
 */
int flow_id(const char *dir)
{
	if (!dir) return -1;
	if      (!strcasecmp(dir, "lrtb")) return LAX_LRTB;
	else if (!strcasecmp(dir, "lrbt")) return LAX_LRBT;
	else if (!strcasecmp(dir, "rltb")) return LAX_RLTB;
	else if (!strcasecmp(dir, "rlbt")) return LAX_RLBT;
	else if (!strcasecmp(dir, "tblr")) return LAX_TBLR;
	else if (!strcasecmp(dir, "tbrl")) return LAX_TBRL;
	else if (!strcasecmp(dir, "btlr")) return LAX_BTLR;
	else if (!strcasecmp(dir, "btrl")) return LAX_BTRL;
	return -1; 
}



/*! @} */

} // namespace Laxkit

