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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//


#include <lax/colorbox.h>
#include <lax/language.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/misc.h>
#include <lax/colorsliders.h>
#include <lax/colorevents.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {



//------------------------------- ColorBox ------------------------------
/*! \class ColorBox
 * \ingroup colors
 * \brief A control to select an RGBA color, with each mouse button corresponding to r, g, and b.
 *
 * By default, pressing and dragging the left mouse button changes the red value, 
 * the middle button does the green, and the right button the blue. The message is sent
 * when a button is released.
 *
 * While pressing a mouse button down and hitting r,g,b,c,m,y, or k, will cause that button
 * to shift the corresponding color. Internally, the colors are all rgb, and the cmyk transformations
 * are through the simple non-icc rgb<->cmyk conversion.
 *
 * This window sends SimpleColorEventData.
 *
 * The box can be one of LAX_COLOR_CMYK, LAX_COLOR_RGB, LAX_COLOR_GRAY.
 *
 *
 * \todo would be neat to maintain a stack of previous colors?
 * \todo might be nice to display number in window while dragging..
 * \todo someday do a color managed version of this window
 * \todo incorporate wheel movements to scroll the last accessed color
 */



#define RED     0
#define GREEN   1
#define BLUE    2
#define CYAN    3
#define MAGENTA 4
#define YELLOW  5
#define BLACK   6
#define ALPHA   7
#define MENU    8

#define SHIFT   (1<<9)
#define CONTROL (1<<10)
#define META    (1<<11)
#define ALT     (1<<12)

/*! Will delete newcolorselector in destructor.
 */
ColorBox::ColorBox(anXWindow *parnt,const char *nname,const char *ntitle, unsigned long nstyle,
				   int nx,int ny,int nw,int nh,int brder,
				   anXWindow *prev,unsigned long nowner,const char *mes,
			 	   int ctype, double nstep,
				   double c0,double c1,double c2,double c3,double c4,
			 	   NewWindowObject *newcolorselector)
  : ColorBase(ctype, c0,c1,c2,c3,c4),
	anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,nx,ny,nw,nh,brder,prev,nowner,mes)
{
	//color1 = color2 = nullptr;
	strokew = .4;
	strokeh = .1;

	sendtype=ctype;
	colorselector=newcolorselector;

	topcolor=colors;
	step=nstep;
	currentid=0;

	 //set up which buttons control which color
	if (colortype==LAX_COLOR_CMYK) {
		colormap[RED    ]=1|CONTROL;
		colormap[GREEN  ]=2|CONTROL;
		colormap[BLUE   ]=3|CONTROL;
		colormap[CYAN   ]=1;
		colormap[MAGENTA]=2;
		colormap[YELLOW ]=3;
		colormap[BLACK  ]=1|CONTROL|SHIFT;
		colormap[ALPHA  ]=1|SHIFT;
		colormap[MENU   ]=3|CONTROL|SHIFT;
	} else if (colortype==LAX_COLOR_GRAY) {
		colormap[RED    ]=-1;
		colormap[GREEN  ]=-1;
		colormap[BLUE   ]=-1;
		colormap[CYAN   ]=-1;
		colormap[MAGENTA]=-1;
		colormap[YELLOW ]=-1;
		colormap[BLACK  ]=1;
		colormap[ALPHA  ]=3;
		colormap[MENU   ]=3|CONTROL;
	} else { //default to rgb
		colortype=LAX_COLOR_RGB;
		colormap[RED    ]=1;
		colormap[GREEN  ]=2;
		colormap[BLUE   ]=3;
		colormap[CYAN   ]=-1;
		colormap[MAGENTA]=-1;
		colormap[YELLOW ]=-1;
		colormap[BLACK  ]=-1;
		colormap[ALPHA  ]=1|SHIFT;
		colormap[MENU   ]=3|CONTROL;
	}

	
	needtodraw=1;

	 //create our own copy of default colors..
	win_themestyle = new WindowStyle;
	*win_themestyle = *app->theme->GetStyle(THEME_Panel);
	win_themestyle->bg.rgbf(Red(), Green(), Blue());

	sc=NULL;
}

ColorBox::~ColorBox()
{
	//if (color1) color1->dec_count();
	//if (color2) color2->dec_count();

	if (sc) sc->dec_count();
	if (colorselector) delete colorselector;
}

/*! \todo be ablte use control, meta, and alt?
 */
void mapname(char *buf,int m)
{
	buf[0]=0;
	if (m&SHIFT) sprintf(buf,_("Shift-"));
	if (m&CONTROL) sprintf(buf+strlen(buf),_("Control-"));
	if (m&META) sprintf(buf+strlen(buf),_("Meta-"));
	if (m&ALT) sprintf(buf+strlen(buf),_("Alt-"));
	if ((m&3)==1) sprintf(buf+strlen(buf),_("Left"));
	else if ((m&3)==2) sprintf(buf+strlen(buf),_("Middle"));
	else if ((m&3)==3) sprintf(buf+strlen(buf),_("Right"));
}

//! Return a tip saying what the current mouse maps are.
const char *ColorBox::tooltip(int mouseid)
{
	if (win_tooltip) { delete[] win_tooltip; win_tooltip=NULL; }

	char buf[100];
	if (colormap[RED]>0) {
		appendstr(win_tooltip,_("For red, drag "));
		mapname(buf,colormap[RED]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[GREEN]>0) {
		appendstr(win_tooltip,_("For green, drag "));
		mapname(buf,colormap[GREEN]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[BLUE]>0) {
		appendstr(win_tooltip,_("For blue, drag "));
		mapname(buf,colormap[BLUE]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[CYAN]>0) {
		appendstr(win_tooltip,_("For cyan, drag "));
		mapname(buf,colormap[CYAN]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[MAGENTA]>0) {
		appendstr(win_tooltip,_("For magenta, drag "));
		mapname(buf,colormap[MAGENTA]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[YELLOW]>0) {
		appendstr(win_tooltip,_("For yellow, drag "));
		mapname(buf,colormap[YELLOW]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[BLACK]>0) {
		appendstr(win_tooltip,_("For black, drag "));
		mapname(buf,colormap[BLACK]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[ALPHA]>0) {
		appendstr(win_tooltip,_("For alpha, drag "));
		mapname(buf,colormap[ALPHA]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}

	return win_tooltip;
}

int ColorBox::init()
{
	return 0;
}

void ColorBox::Updated()
{
	win_themestyle->bg=rgbcolor(Red()*255, Green()*255, Blue()*255);
	needtodraw=1;
}

int ColorBox::SetIndex(int index)
{
	if (index == 0) {
		colorspecial = special1;
		colors = color1;
	} else {
		colorspecial = special2;
		colors = color2;
	}
	needtodraw = 1;
	return index ? 1 : 0;
}

/*! Return 1 for set, else 0. */
int ColorBox::SetMode(int mode)
{
	if      (mode == COLORBOX_FG)         win_style = (win_style & (~(COLORBOX_FGBG|COLORBOX_STROKEFILL|COLORBOX_FG))) | COLORBOX_FG;
	else if (mode == COLORBOX_FGBG)       win_style = (win_style & (~(COLORBOX_FGBG|COLORBOX_STROKEFILL|COLORBOX_FG))) | COLORBOX_FGBG;
	else if (mode == COLORBOX_STROKEFILL) win_style = (win_style & (~(COLORBOX_FGBG|COLORBOX_STROKEFILL|COLORBOX_FG))) | COLORBOX_STROKEFILL;
	else return 0;

	if (mode == COLORBOX_FG) {
		colorspecial = special1;
		colors = color1;
	}
	needtodraw = 1;
	return 1;
}

/*! Should be one of SimpleColorId, but only COLOR_Normal, COLOR_None, COLOR_Knockout, COLOR_Registration
 * are supported by default.
 */
int ColorBox::SetSpecial(int newspecial)
{
	if (newspecial==COLOR_None         && !(win_style&COLORBOX_ALLOW_NONE))         return colorspecial;
	if (newspecial==COLOR_Knockout     && !(win_style&COLORBOX_ALLOW_KNOCKOUT))     return colorspecial;
	if (newspecial==COLOR_Registration && !(win_style&COLORBOX_ALLOW_REGISTRATION)) return colorspecial;

	int old = ColorBase::SetSpecial(newspecial);
	needtodraw=1;
	return old;
}


/*! Normalizes all channels to be in range [0..max];
 */
int ColorBox::send(int which)
{
	DBG cerr << "send color "<<(which == 1 ? "fill" : (which == 0 ? "stroke" : (colors == color2 ? "fill" : "stroke")))<<endl;
	if (!win_owner || !win_sendthis) return 0;

    SimpleColorEventData *cevent=NULL;

	double *orig_colors = colors;
	int orig_special = colorspecial;

	bool isfill = false;
	if (which == -1) isfill = (colors == color2);
	else if (which) {
		colors = color2;
		colorspecial = special2;
	} else {
		colors = color1;
		colorspecial = special1;
	}

    if (sendtype==LAX_COLOR_RGB)
        cevent=new SimpleColorEventData(max,max*Red(),max*Green(),max*Blue(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_GRAY)
        cevent=new SimpleColorEventData(max,max*Gray(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_CMYK)
        cevent=new SimpleColorEventData(max,max*Cyan(),max*Magenta(),max*Yellow(),max*Black(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_HSV)
        cevent=new SimpleColorEventData(max,max*Hue()/360,max*HSV_Saturation(),max*Value(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_HSL)
        cevent=new SimpleColorEventData(max,max*Hue()/360,max*HSL_Saturation(),max*Lightness(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_CieLAB)
        cevent=new SimpleColorEventData(max,max*Cie_L()/100,max*(Cie_a()+108)/216,max*(Cie_b()+108)/216,max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_XYZ)
        cevent=new SimpleColorEventData(max,max*X(),max*Y(),max*Z(),max*Alpha(),currentid);


    if (cevent==NULL) {
        DBG cerr <<" WARNING! Unknown color type: "<<sendtype<<endl;

    } else {
		cevent->colorspecial = colorspecial;
        cevent->colorsystem = sendtype;
		cevent->colorindex = isfill ? 1 : 0;
        app->SendMessage(cevent, win_owner,win_sendthis, object_id);
	}

	colors = orig_colors;
	colorspecial = orig_special;
	return 1;
}


//! Change blue by default.
int ColorBox::RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	SetCurrentColor(x,y);

	if (!buttondown.any()) {
		memcpy(oldcolor,colors,5*sizeof(double));
		oldcolortype=colortype;
	}
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

int ColorBox::RBUp(int x,int y, unsigned int state, const LaxMouse *d)
{
	if (!buttondown.any(d->id, RIGHTBUTTON)) return 0;
	buttondown.up(d->id, RIGHTBUTTON);
	if (!buttondown.any(d->id) && ColorChanged()) send();
	return 0;
}

//! Change green by default.
int ColorBox::MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	SetCurrentColor(x,y);

	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	if (!buttondown.any(d->id)) {
		memcpy(oldcolor,colors,5*sizeof(double));
		oldcolortype=colortype;
	}
	return 0;
}

int ColorBox::MBUp(int x,int y, unsigned int state, const LaxMouse *d)
{
	if (!buttondown.any(d->id, MIDDLEBUTTON)) return 0;

	buttondown.up(d->id, MIDDLEBUTTON);
	if (!buttondown.any(d->id) && ColorChanged()) send();
	return 0;
}

void ColorBox::SetCurrentColor(int x,int y)
{
	if (win_style&(COLORBOX_FGBG|COLORBOX_STROKEFILL)) {
		if (x < win_w * strokew) colors = color1;
		else if (y < win_h * strokeh) colors = color1;
		else colors = color2;

	} else {
		colors = color1;
	}
}

//! Change red by default.
/*! \todo Double click to raise a Color Selection Dialog.
 */
int ColorBox::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	SetCurrentColor(x,y);

	if (!buttondown.any()) {
		memcpy(oldcolor,colors,5*sizeof(double));
		oldcolortype=colortype;
	}
	buttondown.down(d->id, LEFTBUTTON, x,y);

	return 0;
}

/*! Return 1 for unable. 0 for success.
 *
 * x,y is position on screen.
 */
int ColorBox::PopupColorSelector(int x,int y)
{
	anXWindow *w=NULL;

	if (!colorselector) {
		
		double cc[5];
		if (Get(sendtype, &cc[0], &cc[1], &cc[2], &cc[3], &cc[4])) {
			unsigned long extra=0;
			if (win_style&COLORBOX_ALLOW_NONE)         extra|=COLORSLIDERS_Allow_None;
			if (win_style&COLORBOX_ALLOW_KNOCKOUT    ) extra|=COLORSLIDERS_Allow_Knockout; 
			if (win_style&COLORBOX_ALLOW_REGISTRATION) extra|=COLORSLIDERS_Allow_Registration;

			w=new ColorSliders(NULL,"New Color","New Color",ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS|extra,
							0,0,200,400,0,
						   NULL,object_id,"newcolor",
						   sendtype,1./255,
						   cc[0],cc[1],cc[2],cc[3],cc[4],
						   x,y);
		}

	} else {
		w=colorselector->function(NULL,"New Color",colorselector->style,this);
	}

	if (!w) return 1;

	app->rundialog(w);
	return 0;
}


int ColorBox::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes,"newcolor")) {
		 // apply message as new current color, pass on to viewport
		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e);
		if (!ce) {
			const ColorEventData *ced = dynamic_cast<const ColorEventData*>(e);
			if (!ced || !ced->color) return 0;

			Set(ced->color);
		} else {
			double cc[5];
		
			 //we maybe need to unnormalize if hsv, hsl, or cielab
			double mx=ce->max;
			for (int c=0; c<5; c++) cc[c]=ce->channels[c]/mx;
			if (ce->colorsystem==LAX_COLOR_HSV || ce->colorsystem==LAX_COLOR_HSL) {
				cc[0]*=360;
			} else if (ce->colorsystem==LAX_COLOR_CieLAB) {
				cc[0]*=100;
				cc[1]=cc[1]*216-108;
				cc[2]=cc[1]*216-108;
			}
			Set(ce->colorsystem, cc[0],cc[1],cc[2],cc[3],cc[4]);
		}

		win_themestyle->bg = rgbcolor(Red()*255, Green()*255, Blue()*255);
		send();
		needtodraw=1;
		return 0;
	}
	return anXWindow::Event(e,mes);
}

int ColorBox::LBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	if (!buttondown.any(d->id, LEFTBUTTON)) return 0;

	int dragged=buttondown.up(d->id, LEFTBUTTON);
	if (dragged<3) { 
		mouseposition(d->id, NULL,&x,&y,NULL,NULL);
		PopupColorSelector(x,y);
		return 0;
	}
	if (!buttondown.any(d->id) && ColorChanged()) send();
	return 0;
}

/*! Drag left, middle, right click for red, green, blue. 
 */
int ColorBox::MouseMove(int x,int y,unsigned int state, const LaxMouse *d)
{
	if (!buttondown.any(d->id)) return 1;

	int mods=0;
	if (state&ShiftMask)   mods|=SHIFT;
	if (state&ControlMask) mods|=CONTROL;
	if (state&MetaMask)    mods|=META;
	if (state&AltMask)     mods|=ALT;

	int b[3];
	b[0]=b[1]=b[2]=-1;
	if (buttondown.isdown(d->id,LEFTBUTTON))   b[0]=1|mods;
	if (buttondown.isdown(d->id,MIDDLEBUTTON)) b[1]=2|mods;
	if (buttondown.isdown(d->id,RIGHTBUTTON))  b[2]=3|mods;

	buttondown.move(d->id,x,y);
	int mx,my;

	if (buttondown.getinfo(d->id,LEFTBUTTON, NULL,NULL,&mx,&my,NULL,NULL) 
		 && buttondown.getinfo(d->id,RIGHTBUTTON, NULL,NULL,&mx,&my,NULL,NULL)
		 && buttondown.getinfo(d->id,MIDDLEBUTTON, NULL,NULL,&mx,&my,NULL,NULL)) {
		return 0; //couldn't find down info!
	}

	for (int c=0; c<3; c++) {
		if (b[c]<0) continue;
		int col;
		for (col=0; col<8; col++) if (colormap[col]==b[c]) break;
		if (col==8) continue; //mapping not found

		if (col==ALPHA) {
			double alpha=Alpha();
			alpha+=(x-mx)*((state&ControlMask)?.01:step);
			if (alpha<0) alpha=0;
			if (alpha>1) alpha=1;
			Alpha(alpha);
			needtodraw=1;

		} else if (col==RED) {
			double red=Red();
			red+=(x-mx)*((state&ControlMask)?.01:step);
			if (red<0) red=0;
			if (red>1) red=1;
			Red(red);
			needtodraw=1;

		} else if (col==GREEN) {
			double green=Green();
			green+=(x-mx)*((state&ControlMask)?.01:step);
			if (green<0) green=0;
			if (green>1) green=1;
			Green(green);
			needtodraw=1;

		} else if (col==BLUE) {
			double blue=Blue();
			blue+=(x-mx)*((state&ControlMask)?.01:step);
			if (blue<0) blue=0;
			if (blue>1) blue=1;
			Blue(blue);
			needtodraw=1;

		} else if (col==CYAN) {
			double cyan=Cyan();
			c+=(x-mx)*((state&ControlMask)?.01:step);
			if (c<0) c=0;
			if (c>1) c=1;
			Cyan(cyan);
			needtodraw=1;

		} else if (col==MAGENTA) {
			double m=Magenta();
			m+=(x-mx)*((state&ControlMask)?.01:step);
			if (m<0) m=0;
			if (m>1) m=1;
			Magenta(m);
			needtodraw=1;

		} else if (col==YELLOW) {
			double y=Yellow();
			y+=(x-mx)*((state&ControlMask)?.01:step);
			if (y<0) y=0;
			if (y>1) y=1;
			Yellow(y);
			needtodraw=1;

		} else if (col==BLACK) {
			double k=Black();
			k+=(x-mx)*((state&ControlMask)?.01:step);
			if (k<0) k=0;
			if (k>1) k=1;
			Black(k);
			needtodraw=1;
		}
	}

	win_themestyle->bg=rgbcolor(Red()*255, Green()*255, Blue()*255);
	needtodraw=1;
	
	char blah[100];
	if (colortype==LAX_COLOR_RGB) sprintf(blah,"%f,%f,%f,%f",Red(),Green(),Blue(),Alpha());
	else if (colortype==LAX_COLOR_CMYK) sprintf(blah,"%f,%f,%f,%f,%f",Cyan(),Magenta(),Yellow(),Black(),Alpha());
	else sprintf(blah,"%f,%f",Gray(),Alpha());

	if (win_style&COLORBOX_SEND_ALL) send();
	return 0;
}

void ColorBox::Refresh()
{
	if (!win_on || !needtodraw) return;

	Displayer *dp=MakeCurrent();
	dp->ClearWindow();

	DBG cerr << "ColorBox::Refresh special1: "<<special1<<", special2: "<<special2<<", special: "<<colorspecial<<", current: "<<(colors == color1 ? "stroke" : "fill")<<endl;

	double *orig_colors = colors;
	int orig_special = colorspecial;

	colors = color1;
	bool alpha1 = (special1 == COLOR_Normal && Alpha() < 1);
	colors = color2;
	bool alpha2 = (special2 == COLOR_Normal && Alpha() < 1);
	colors = orig_colors;

	if (alpha1 || alpha2) {
		//win_themestyle->bg=rgbcolor(Red()*255, Green()*255, Blue()*255);
		//dp->NewFG(coloravg(0,win_themestyle->bg, Alpha()));
		//dp->drawthing(win_w/2,win_h/2,win_w/2,win_h/2,1,THING_Diamond);
		dp->NewFG(.7,.7,.7);
		dp->NewBG(.3,.3,.3);
		dp->drawCheckerboard(0,0,win_w,win_h, win_h/4, 0,0);
	}

	if (win_style&(COLORBOX_FGBG|COLORBOX_STROKEFILL)) {
		 //two color mode, draw one color over another
		int offx,offy;

		bool box_on_box = ((win_style & COLORBOX_FGBG) != 0);
		if (box_on_box) { // boxes of equal size offset in window
			if (topcolor == color1) {
				colors = color2;
				colorspecial = special2;
				offx=win_w*.2; offy=win_h*.2;
			} else {
				colors = color1;
				colorspecial = special1;
				offx=0; offy=0;
			}
			if (colorspecial != COLOR_Normal) {
				draw_special_color(dp, colorspecial, 20, offx,offy, win_w*.8,win_h*.8);
			} else {
				// dp->NewFG(rgbcolor(Red()*255, Green()*255, Blue()*255));
				dp->NewFG(Red(), Green(), Blue(), Alpha());
				dp->drawrectangle(offx,offy, win_w*.8,win_h*.8, 1);
			}

			if (topcolor == color1) {
				colors = color1;
				colorspecial = special1;
				offx=0; offy=0;
			} else {
				colors = color2;
				colorspecial = special2;
				offx=win_w*.2; offy=win_h*.2;
			}
			if (colorspecial != COLOR_Normal) {
				draw_special_color(dp, colorspecial, 20, offx,offy, win_w*.8,win_h*.8);
			} else {
				// dp->NewFG(rgbcolor(Red()*255, Green()*255, Blue()*255));
				dp->NewFG(Red(), Green(), Blue(), Alpha());
				dp->drawrectangle(offx,offy, win_w*.8,win_h*.8, 1);
			}

		} else { //fill on stroke
			//stroke color:
			colors = color1;
			if (special1 != COLOR_Normal) {
				draw_special_color(dp, special1, 20, win_w - 2*(1-.5*strokew)*win_w,0, 2*(1-.5*strokew)*win_w,win_h);
			} else {
				// dp->NewFG(rgbcolor(Red()*255, Green()*255, Blue()*255));
				dp->NewFG(Red(), Green(), Blue(), Alpha());
				dp->drawrectangle(0,0, win_w,win_h, 1);
			}

			//fill color:
			colors = color2;
			if (alpha2) {
				dp->NewFG(.7,.7,.7);
				dp->NewBG(.3,.3,.3);
				dp->drawCheckerboard(win_w*strokew,win_h*strokeh, win_w*(1-strokew),win_h*(1-strokeh), win_h/4, 0,0);
			}
			if (special2 != COLOR_Normal) {
				draw_special_color(dp, special2, 20, win_w*strokew,win_h*strokeh, win_w*(1-strokew),win_h*(1-strokeh));
			} else {
				dp->NewFG(Red(), Green(), Blue(), Alpha());
				// dp->NewFG(rgbcolor(Red()*255, Green()*255, Blue()*255));
				dp->drawrectangle(win_w*strokew,win_h*strokeh, win_w*(1-strokew),win_h*(1-strokeh), 1);
			}
		}

	} else {
		 //single color
		//dp->NewFG(win_themestyle->bg);
		if (colorspecial != COLOR_Normal) {
			draw_special_color(dp, colorspecial, 20, 0,0,win_w,win_h);
		} else {
			dp->NewFG(Red(), Green(), Blue(), Alpha());
			dp->drawrectangle(0,0,win_w,win_h, 1);
		}
	}

	colors = orig_colors;
	colorspecial = orig_special;
	
	SwapBuffers();
	needtodraw=0;
	return;
}

/*! While a button is pressed, pushing r,g,b,c,m,y,k will make that button shift
 * the corresponding color.
 */
int ColorBox::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d)
{
	if (ch=='\t') return anXWindow::CharInput(ch,buffer,len,state,d);
	
	 //check shortcuts
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}


	// *** maybe remove this remapping stuff? not exactly intuitive...
	//
	
	if (!d->paired_mouse) return anXWindow::CharInput(ch,buffer,len,state,d);
	if (!buttondown.any(d->paired_mouse->id)) return anXWindow::CharInput(ch,buffer,len,state,d);

	ch=tolower(ch);
	if (ch>127 || !strchr("argbcmyk",(char)ch)) return anXWindow::CharInput(ch,buffer,len,state,d);

	//remap button+modifier bindings for particular color channels

	int b;
	if (buttondown.isdown(d->paired_mouse->id,LEFTBUTTON)) b=1;
	else if (buttondown.isdown(d->paired_mouse->id,MIDDLEBUTTON)) b=2;
	else  b=3;

	if (state & ShiftMask)   b |= SHIFT;
	if (state & ControlMask) b |= CONTROL;
	if (state & MetaMask)    b |= META;
	if (state & AltMask)     b |= ALT;

	 //clear old mapping
	for (int c=0; c<8; c++) if (colormap[c]==b) colormap[c]=-1;

	 //set new mapping
	if (ch=='a') {
		colormap[ALPHA]=b;
	} else if (ch=='r') {
		colormap[RED]=b;
	} else if (ch=='g') {
		colormap[GREEN]=b;
	} else if (ch=='b') {
		colormap[BLUE]=b;
	} else if (ch=='c') {
		colormap[CYAN]=b;
	} else if (ch=='m') {
		colormap[MAGENTA]=b;
	} else if (ch=='y') {
		colormap[YELLOW]=b;
	} else if (ch=='k') {
		colormap[BLACK]=b;
	} else return 1;


	return 0;
}


int ColorBox::PerformAction(int action)
{
	if (action==COLORBOXA_SelectNormal) {
		SetSpecial(COLOR_Normal);
		return 0;

	} else if (action==COLORBOXA_ToggleNone) {
		if (!(win_style&COLORBOX_ALLOW_NONE)) return 1;
		SetSpecial(colorspecial == COLOR_Normal ? COLOR_None : COLOR_Normal);
		return 0;

	} else if (action==COLORBOXA_SelectNone) {
		if (!(win_style&COLORBOX_ALLOW_NONE)) return 1;
		SetSpecial(COLOR_None);
		return 0;

	} else if (action==COLORBOXA_SelectRegistration) {
		if (!(win_style&COLORBOX_ALLOW_REGISTRATION)) return 1;
		SetSpecial(COLOR_Registration);
		return 0;

	} else if (action==COLORBOXA_SelectKnockout) {
		if (!(win_style&COLORBOX_ALLOW_KNOCKOUT)) return 1;
		SetSpecial(COLOR_Knockout);
		return 0;

	} else if (action==COLORBOXA_SwapColors) {
		if (win_style & COLORBOX_FG) return 1;
		SwapColors();
		needtodraw = 1;
		return 0;
	}

	return 1;
}

Laxkit::ShortcutHandler *ColorBox::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	sc=new ShortcutHandler(whattype());

	sc->Add(COLORBOXA_ToggleNone,          'n',0,0,   "ToggleNone",        _("Toggle normal and \"none\" color"),NULL,0);
	//sc->Add(COLORBOXA_SelectNormal,        'n',0,0,   "SelectNormal",      _("Toggle normal and \"none\" color"),NULL,0);
	//sc->Add(COLORBOXA_SelectNone,          'x',0,0,   "SelectNone",        _("Select \"None\" color"),NULL,0);
	sc->Add(COLORBOXA_SelectRegistration,  'r',0,0,   "SelectRegistration",_("Select registration color"),NULL,0);
	sc->Add(COLORBOXA_SelectKnockout,      'k',0,0,   "SelectKnockout",    _("Select knockout color"),NULL,0);
	sc->Add(COLORBOXA_SwapColors,          'x',0,0,   "SwapColors",        _("Swap colors"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}



} // namespace Laxkit

