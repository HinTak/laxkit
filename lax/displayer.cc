//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/lists.cc>
#include <lax/displayer.h>
#include <lax/doublebbox.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/laxutils.h>
#include <lax/laximages-imlib.h>

#include <cstring>

#include <iostream>
#define DBG 






using namespace std;


namespace Laxkit {


//-------------------------------- Displayer --------------------------
/*! \class Displayer
 * \brief A graphics drawing wrapper.
 *
 * Handles drawing lines, bezier curves, ellipses (in any orientation,
 * not just elongated horizontally and vertically), and viewport scaling, rotating, shifting.
 *
 * Displayer classes are meant to draw onto random buffers, but also have ability to
 * draw right onto windows.
 *
 * There are the screen bounds, which are in screen window coordinates, defined
 * by Minx, Maxx, Miny, and Maxy. Also there are workspace bounds in real coordinates,
 * defined by spaceminx, spacemaxx, spaceminy, and spacemaxy. If the axes are 
 * rotated, then the actual contents of a window may show portions not
 * in the workspace. The bounding rectangle of this rotated space (in screen window coords)
 * is stored in the panner so that scrollers and such have access to something meaningful.
 *
 * Any window tracking the bounds and viewable portion of the displayer are typcially
 * informed via a PanController, which sends out a change message to any windows
 * registered with it.
 * 
 * For any rulers tracking the workspace, care must be taken when the axes are of
 * different lengths, and especially when the axes are rotated. Call GetVMag()
 * to find out the magnification along the specified screen vector. Also when rotated,
 * the workspace is a rotated rectangle, thus its screen bounding rectangle is a
 * larger rectangle. This must be taken into account by any scroll bars that track
 * the workspace. 
 * 
 * \todo *** this could be automated a little more, maybe the ruler could
 *   watch a panner too? or watch a displayer??? maybe not, rulers are doubles, panners are longs
 * \todo *** have to coordinate panner->minsel/maxsel and upperbound/lowerbound.\n
 * \todo ***heell, perhaps combine displayer and panner to DoublePanner? would simplify some things...
 * \todo *** require ctm and ictm be kept up always?? would make this just slightly faster
 *
 *
 * <pre>
 * About the Current Transformation matrix (ctm) and its inverse (ictm):
 *
 *  Postscript:
 *      [ a  b  0 ]
 *  CTM=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * 
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *
 *  Say you want to rotate real, then translate real, then screenp = realp*R*T*CTM.
 *  
 *  In the Cairo library, cairo_matrix_t={ double xx,xy, yx,yy, x0,y0 } corresponds
 *  basically to [xx yx xy yy x0 y0].
 * </pre>
 */
/*! \var int Displayer::Minx
 * \brief Minimum screen x coordinate.
 */
/*! \var int Displayer::Maxx
 * \brief Maximum screen x coordinate.
 */
/*! \var int Displayer::Miny
 * \brief Minimum screen y coordinate (near top of screen).
 */
/*! \var int Displayer::Maxy
 * \brief Maximum screen y coordinate (near bottom of screen).
 */
/*! \var double Displayer::spaceminx
 * \brief Minimum real workspace x coordinate.
 */
/*! \var double Displayer::spacemaxx
 * \brief Maximum real workspace x coordinate.
 */
/*! \var double Displayer::spaceminy
 * \brief Minimum real workspace y coordinate.
 */
/*! \var double Displayer::spacemaxy
 * \brief Maximum real workspace y coordinate.
 */
/*! \var double Displayer::upperbound
 * \brief The maximum screen length of an axis and other items.
 */
/*! \var double Displayer::lowerbound
 * \brief The minimum screen length of an axis and other items.
 */
/*! \var double Displayer::ctm[]
 * \brief Current Transformation Matrix.
 * 
 * This is a six valued array, see intro for discussion.
 * This matrix is not dynamic like ctm. Is computed on the fly as necessary.
 * Only the ctm is pushed and popped.
 *
 * Subclasses should ensure that ctm and ictm are updated as appropriate, in order for
 * the panner sync up to work.
 */
/*! \var double Displayer::ictm[6]
 * \brief Inverse Current Transformation Matrix.
 * 
 * This is a six valued array, see intro for discussion.
 */


Displayer::Displayer(aDrawable *d)
  : PanUser(NULL)
{
	displayer_style=0;

	updatepanner=1;
	dr=d;
	xw=dynamic_cast<anXWindow*>(d);

	upperbound=1e+3;
	lowerbound=1e-5;

	Minx=Maxx=Miny=Maxy=0;
	spaceminx=spaceminy=spacemaxx=spacemaxy=0;

	num_bez_div=30;

	draw_immediately=1;
	real_coordinates=1;
	decimal=0;

	on=0;
}


//! Constructor, set everything to nothing or identity.
/*! upperbound=1000, lowerbound=.00001, ctm/ictm=identity,
 * fgcolor=white, bg=black
 */
Displayer::Displayer(anXWindow *nxw,PanController *pan) 
  : PanUser(pan)
{
	displayer_style=0;

	updatepanner=1;
	xw=nxw;
	dr=nxw;

	upperbound=1e+3;
	lowerbound=1e-5;

	Minx=Maxx=Miny=Maxy=0;
	spaceminx=spaceminy=spacemaxx=spacemaxy=0;

	num_bez_div=30;

	draw_immediately=1;
	real_coordinates=1;
	decimal=0;

	on=0;

	//transform_identity(ctm);
	//transform_identity(ictm);
}

//! Destructor.
Displayer::~Displayer()
{}

/*! \fn void Displayer::show()
 * \brief  Flush waiting composite operation.
 */

/*! \fn void Displayer::SwapBuffers()
 */

/*! \void Displayer::BackBuffer(int on)
 * \brief Turn on or off the usage of double buffering.
 */

/*! \fn int Displayer::activeMask()
 * \brief Return whether there is an active clipping area (0 for no, nonzero for yes).
 */


/*! \fn void Displayer::PushClip(int startfresh)
 * \brief Push the current clip mask onto a stack, make a new one maybe
 */


/*! \fn void Displayer::PopClip()
 * \brief Restore a previous clipping area
 */


/*! \fn int Displayer::Clip(flatpoint *p,int n, int append)
 * \brief Set the clipping area to the path defined by the given polyline.
 *
 * Return 0 for success, nonzero for error.
 */


/*! \fn void Displayer::ClearClip()
 * \brief Clear any clip state.
 */


/*! \fn void Displayer::LineAttributes(double width,int dash,int cap,int join)
 * \brief Set the width, whether solid, line cap and join.
 *
 * width is only in screen pixels for now.
 * See LaxJoinStyle and LaxCapStyle for values for cap and join.
 * dash can be LineSolid, LineOnOffDash, or LineDoubleDash.
 *
 * \todo should be able to set dash pattern somehow, and set line width based
 *   on current transform...
 */


/*! \fn void Displayer::FillAttributes(int fillstyle, int fillrule)
 *
 * fillrule can be EvenOddRule or WindingRule.
 * fillstyle can be FillSolid, FillTiled, FillStippled, or FillOpaqueStippled.
 */


/*! \fn LaxCompositeOp Displayer::BlendMode(LaxCompositeOp mode)
 * \brief Set how to combine drawing elements to the target surface, return the old one.
 *
 * See LaxCompositeOp.
 */


//! Any subsequent calls are using real coordinates
/*! Returns old real_coordinates.
 */
int Displayer::DrawReal()
{
	int r=real_coordinates;
	real_coordinates=1;
	return r;
}

//! Any subsequent calls are using screen coordinates
/*! Returns old real_coordinates.
 */
int Displayer::DrawScreen()
{
	int r=real_coordinates;
	real_coordinates=0;
	return r;
}

//! Interpret all angles as radians.
/*! Sets decimal=0.
 */
void Displayer::Radians()
{
	decimal=0;
}

//! Interpret all angles as radians.
/*! Sets decimal=1.
 */
void Displayer::Degrees()
{
	decimal=1;
}

//! Subclasses must redefine this is modifying the mask is ok.
/*! Default does nothing.
 */
void Displayer::DrawOnMask()
{
	DBG cerr <<" default Displayer::DrawOnMask doesn't do anything!"<<endl;
}

//! Ensure that drawing operations work on source (as opposed to mask).
/*! Default does nothing.
 */
void Displayer::DrawOnSrc()
{
	DBG cerr <<" default Displayer::DrawOnSrc doesn't do anything!"<<endl;
}

//! Do not append path operations, draw them with each call
/*! Sets draw_immediately to yes. Returns old value of draw_immediately.
 */
int Displayer::DrawImmediately(int yes)
{
	int o=draw_immediately;
	draw_immediately=yes;
	return o;
}

//! Return whether the given (screen) point is in the viewable area or not.
int Displayer::onscreen(double x,double y)
{ return x>=Minx && x<=Maxx && y>=Miny && y<=Maxy; }


/*! \fn flatpoint Displayer::realtoscreen(flatpoint p)
 * \brief Convert real point p to screen coordinates.
 *
 * <pre>
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *  </pre>
 * 
 * So basically this should return:
 * \code
 *   return flatpoint(ctm[4] + ctm[0]*p.x + ctm[2]*p.y, ctm[5]+ctm[1]*p.x+ctm[3]*p.y); 
 * \endcode
 */


/*! \fn  const double *Displayer::Getctm()
 * \brief Return a constant pointer to the current transformation matrix.
 */

/*! \fn  const double *Displayer::Getictm()
 * \brief Return a constant pointer to the inverse of the current transformation matrix.
 */


//! Return whether the ctm is (mathematically) right handed or not.
/*! Note that for typical X displays, how it appears to the user is left handed,
 * when mathematically it is really right handed. This function returns whether
 * the ctm is mathematically right handed.
 */
int Displayer::righthanded()
{
	 // is right handed if y*transpose(x)>0
	 //            y=( ctm[2],ctm[3])
	 // transpose(x)=(-ctm[1],ctm[0])

	const double *ctm=Getctm();
	return ctm[3]*ctm[0]-ctm[2]*ctm[1]>0;
}


//! Convert real point (x,y) to screen coordinates.
/*! Just return realtoscreen(flatpoint(x,y)).
 */
flatpoint Displayer::realtoscreen(double x,double y)
{
	const double *ctm=Getctm();
	return flatpoint(ctm[4] + ctm[0]*x + ctm[2]*y, ctm[5]+ctm[1]*x+ctm[3]*y); 
} 

//! Convert screen point (x,y) to real coordinates.
flatpoint Displayer::screentoreal(int x,int y)
{
	const double *ictm=Getictm();
	return flatpoint(ictm[4] + ictm[0]*(double)x + ictm[2]*(double)y, ictm[5]+ictm[1]*(double)x+ictm[3]*(double)y); 
}

//! Convert screen point p to real coordinates.
flatpoint Displayer::screentoreal(flatpoint p)
{
	const double *ictm=Getictm();
	return flatpoint(ictm[4] + ictm[0]*p.x + ictm[2]*p.y, ictm[5]+ictm[1]*p.x+ictm[3]*p.y); 
}

/*! \fn void Displayer::ClearWindow()
 * \brief Clear the window to bgcolor between Min* and Max*. 
 */



//------------------------ Text functions 

/*! \fn int Displayer::textheight()
 *  \brief Return the text height of the current font.
 */

/*! \fn int font(LaxFont *nfont, double size=-1)
 * \brief Set the current font to that specified.
 *
 * The count on nfont must be incremented.
 */

/*! \fn Displayer::font(const char *fontconfigpattern)
 * \brief Set the current font to that specified.
 */

/*! \fn int Displayer::font(const char *family,const char *style,double pixelsize)
 * \brief Set the current font to that specified.
 */

/*! \fn int Displayer::fontsize(double size)
 * \brief Change the current font's size.
 */

/*! \fn double Displayer::textout(double x,double y,const char *str,int len,unsigned long align)
 * \brief Draw text at screen x,y. Return distance advanced.
 */

/*! \fn double Displayer::textout(double *matrix,double x,double y,const char *str,int len,unsigned long align)
 * \brief Draw transformed text starting at x,y.
 */


/*! \fn double Displayer::textout(double angle, double x,double y,const char *str,int len,unsigned long align)
 * \brief Draw text at an angle starting at x,y.
 */

/*! \fn double Displayer::textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real)
 * \brief Return the width of the text.
 *
 * Use thisfont if given (but do not set the default font), or the default font if NULL.
 *
 * Also return width, height, ascent, and descent if not null.
 *
 * If real==0, then return ascent+descent for height. Else return the visual extent of the string.
 *
 * Extents depend on draw_real.
 */

//! Return the width of the text.
/*! This just calls the other textextent().
 * Also return width and height if not null.
 * Extents depend on draw_real.
 */
double Displayer::textextent(const char *str,int len, double *width,double *height)
{
	return textextent(NULL, str,len, width,height, NULL,NULL,0);
}


//--------------------- Line Drawing functions

//! Draw the axes, x=red, y=green, real length len
/*! This will set the color, but not other line attributes.
 */
void Displayer::drawaxes(double len)
{
	int real=DrawReal();

	NewFG(.4,0.,0.);
	drawline(flatpoint(0,0),flatpoint(len,0));
	NewFG(0.,.4,0.);
	drawline(flatpoint(0,0),flatpoint(0,len));

	if (!real) DrawScreen();
}

/*! \fn void Displayer::moveto(flatpoint p)
 * \brief Relocate path building to the point.
 */


/*! \fn void Displayer::lineto(flatpoint p)
 * \brief Add a segment to the current path.
 *
 * If there was no previous point, then this is the same as a moveto(p).
 */


/*! \fn void Displayer::curveto(flatpoint c1,flatpoint c2,flatpoint v)
 * \brief Add a bezier curve to the current path.
 */


/*! \fn void Displayer::closed()
 * \brief Call if current path should be closed, and close at the last added point.
 */


/*! \fn void Displayer::closeopen()
 * \brief Call if current path should be ended, but not closed.
 */


/*! \fn void Displayer::fill(int preserve)
 * \brief Fill any stored path(s). If preserve, then do not clear the path afterward.
 */


/*! \fn void Displayer::stroke(int preserve)
 * \brief Paint any stored path(s). If preserve, then do not clear the path afterward.
 */


/*! \fn void Displayer::drawlines(flatpoint *points,int npoints,char closed,char fill)
 * \brief Draw a polygon, optionally closed, optionally fill, optionally transform.
 *
 * If fill==1 then fill with FG and have no border. If fill==2,
 * then fill with BG and border with FG.
 */


/*! \fn void Displayer::drawline(flatpoint p1,flatpoint p2)
 * \brief Draw a line between real coordinates p1 and p2.
 */


//! Draw a line between coordinates (ax,ay) and (bx,by).
/*! Just calls drawline(flatpoint,flatpoint). Subclasses need not redefine.
 */
void Displayer::drawline(double ax,double ay,double bx,double by)
{ drawline(flatpoint(ax,ay),flatpoint(bx,by)); }


//! Draw a basic rectangle.
/*!
 * If fill==0 then draw only the line.
 * If fill==1 then draw only the fill.
 * If fill==2 then draw line with fgcolor, and fill with bgcolor
 */
void Displayer::drawrectangle(double x,double y,double ww,double hh,int tofill)
{
	moveto(flatpoint(x,y));
	lineto(flatpoint(x+ww,y));
	lineto(flatpoint(x+ww,y+hh));
	lineto(flatpoint(x,y+hh));
	closed();

	if (draw_immediately) {
		if (tofill==0) stroke(0);
		else if (tofill==1) fill(0);
		else {
			unsigned long oldfg=FG();
			NewFG(BG());
			fill(1);
			NewFG(oldfg);
			stroke(0);
		}
	}

}


//! Draw part of an ellipse.
/*! Just calls drawellipse() with no fill. Subclasses need not redefine.
 */
void Displayer::drawarc(flatpoint p,double xr,double yr,double start_angle,double end_angle)
{ drawellipse(p.x,p.y,xr,yr,start_angle,end_angle, 0); }


//! Draw an ellipse in the rectangle spanned by p +/- (xradius,yradius).
/*! If start and end angles don't correspond, then draw a partial ellipse, filled as a pie slice.
 */
void Displayer::drawellipse(flatpoint p,double xradius,double yradius,double start_angle,double end_angle,int fill)
{
	//DBG cerr <<"xr,yr="<<xradius<<", "<<yradius<<endl;

	if (xradius==0 || yradius==0) return;
	if (xradius<0) xradius=-xradius;
	if (yradius<0) yradius=-yradius;
	double f;
	if (xradius>yradius) {
		f=sqrt(xradius*xradius-yradius*yradius);
		drawfocusellipse(p-flatpoint(f,0),p+flatpoint(f,0), 2*xradius, start_angle,end_angle, fill);
	} else {
		f=sqrt(yradius*yradius-xradius*xradius);
		drawfocusellipse(p-flatpoint(0,f),p+flatpoint(0,f), 2*yradius, start_angle,end_angle, fill);
	}
}

//! Draw an ellipse with center at real point p..
/*!  This ellipse is (xr*(x-p.x))^2 + (yr*(y-p.y))^2 = 1.
 * If f is distance from center to a focus, then f=sqrt(xr*xr-yr*yr) if xr>yr,
 * swap if yr<xr.
 *
 * This function just calls drawfocusellipse() with appropriate settings.
 */
void Displayer::drawellipse(double x,double y,double xradius,double yradius,double start_angle,double end_angle,int fill)
{ drawellipse(flatpoint(x,y), xradius,yradius, start_angle,end_angle, fill); }


//! \brief Draw an ellipse based on the foci.
/*! en=st==0 means draw whole thing.
 *
 * If fill==0 then draw only the line.
 * If fill==1 then draw only the fill.
 * If fill==2 then draw line with fgcolor, and fill with bgcolor
 *
 * \todo *** fails on circles
 */
void Displayer::drawfocusellipse(flatpoint focus1,flatpoint focus2,
				double c,          //!< Dist from focus to point on ellipse to other focus is c
				double start_angle,//!< The starting angle
				double end_angle, //!< The ending angle
				int tofill         //!< how to fill
			)
{
	if (c==0) return;
	if (start_angle==end_angle) { end_angle=start_angle+2*M_PI; }
	else if (decimal) { end_angle=end_angle/180.*M_PI; start_angle=start_angle/180.*M_PI; }

	double a,b;
	flatpoint p,x,y,pp;
	p=(focus2+focus1)/2;
	x=focus2-focus1;

	a=c/2;
	b=sqrt(a*a-x*x/4);
	if (x*x) {
		x=x/sqrt(x*x);
	} else x=flatpoint(1,0); //is pure circle
	y=transpose(x);

	flatpoint points[4*3];
	bez_ellipse(points,4, p.x,p.y, a,b, x,y, start_angle,end_angle);

	moveto(points[1]);
	for (int c=1; c<9; c+=3) {
		curveto(points[c+1],points[c+2],points[c+3]);
	}
	curveto(points[11],points[0],points[1]);
	closed();

	if (draw_immediately) {
		if (tofill==0) stroke(0);
		else if (tofill==1) fill(0);
		else {
			unsigned long oldfg=FG();
			NewFG(BG());
			fill(1);
			NewFG(oldfg);
			stroke(0);
		}
	}
}

//! Draw a little arrow near point p in v direction.
/*! p is center of a circle radius rfromp, and the arrow is drawn
 * tangent to the circle. rfromp<0 means arrow is on ccw side.
 *
 * Currently, the length of the arrow is screen pixel length len if reallengthi==0.
 * If reallength==1, then the length is the absolute real length len.
 * If reallength==2, the arrow draws with a length norm(v)*len.
 *
 * This uses drawline() to draw the arrow. Subclasses need not redefine.
 *
 * If portion&1, draw half of the arrow head. If portion&2, draw the other half.
 * So portion==3 draws whole arrow head.
 */
void Displayer::drawarrow(flatpoint p,flatpoint v,int rfromp,double len,char reallength,int portion)
{
	double vv=v*v;
	if (vv==0) return;

	if (real_coordinates && reallength==0) {
		 //need to transform the screen length len to a real length
		flatpoint vv=realtoscreen(p+v);
		flatpoint pp=realtoscreen(p);

		len=len*norm(v)/norm(vv-pp);
	}
	if (reallength==2) len=len*sqrt(vv);
	v=v/sqrt(vv);
	p+=rfromp*transpose(v);
	v=len*v;
	flatpoint p2=v+p;
	drawline(p,p2); //line
	if (portion&1) drawline(p2,p2-v/3+transpose(v)/4); //half of arrow head
	if (portion&2) drawline(p2,p2-v/3-transpose(v)/4); //other half
}

void Displayer::drawthing(double x, double y, double rx, double ry, DrawThingTypes thing,unsigned long fg,unsigned long bg,int lwidth)
{
	if (lwidth>=0) LineAttributes(lwidth,0,CapRound,JoinRound);
	NewFG(bg);
	drawthing(x,y,rx,ry,1,thing);
	NewFG(fg);
	drawthing(x,y,rx,ry,0,thing);
}

//! Draw a little graphic in range X:x-rx..x+rx,  Y:y-ry..y+ry.
/*! This uses Laxkit::draw_thing_coordinates() to draw from.
 */
void Displayer::drawthing(double x, double y, double rx, double ry, int fill, DrawThingTypes thing)
{
	 //use thing_coordinates()
	flatpoint *pts=NULL;
	int n=0;
	pts=draw_thing_coordinates(thing, NULL,-1, &n, 1);
	if (!pts) return;

	int nn=0;
	int pathtype=0;
	int closed=0;
	for (int c=0; c<n; c++) {
		 // transform coordinate
		pts[c].x=x+(2*rx*pts[c].x-rx);
		pts[c].y=y+(2*ry*pts[c].y-ry);

		if (!(pts[c].info & LINE_Closed) && !(pts[c].info & LINE_End)) {
			pathtype=pts[c].info;
			continue;
		}

		if (pts[c].info&LINE_Closed) closed=1; else closed=0;
		if (pathtype==0 || pathtype==LINE_Vertex) drawlines(pts+nn,c-nn+1, closed,fill);
		else drawbez(pts+nn,(c-nn+1)/3, closed,fill);
		nn=c+1;
	}
	delete[] pts;
}

/*! \fn void Displayer::drawpixel(flatpoint p)
 * \brief Draw one screen pixel at coord p. Transform to screen coordinates if real!=0.
 */


//! Draw a little circle at p with screen radius.
/*!If fill==0 then draw only the line.
 * If fill==1 then draw only the fill.
 * If fill==2 then draw line with fgcolor, and fill with bgcolor
 *
 * This just calls the other drawpoint(p.x,p.y,radius,fill).
 */
void Displayer::drawpoint(flatpoint p,double radius, int fill)
{ drawpoint(p.x,p.y,radius,fill); }


/*! \fn void Displayer::drawpoint(double x, double y, double radius, int fill)
 * \brief Draw a little circle at real or screen coord (x,y) with screen radius r.
 */


//! Draw a cubic bezier line.
/*! This is the usual actual bez having f'=3*(control point dist) which
 * displays the remarkable midpoint property....
 *
 * For t in [0,1] from vertex v1, controls c1,c2, then vertex v2:
 * <tt> p(t) =  (1-t)^3 * v1  +  3t(1-t)^2 * c1  +  3*t^2*(1-t) * c2  +  t^3 * v2; </tt>
 * 
 * assumes n=(num vertex points), and that there are n*3 points in bpoints. If i is in range [0..n*3], then
 * i\%3=1 are v, i\%3=0 c1,i\%3=2 c2:  c-v-c - c-v-c - c-v-c ... The first and final control point in bpoints
 * are not accessed unless isclosed!=0.
 *
 * This function uses curveto(), and fill()/stroke() if draw_immediately. Subclasses don't have to redefine
 * if that is sufficient.
 *
 * If fill==0 then draw only the line.
 * If fill==1 then draw only the fill.
 * If fill==2 then draw line with fgcolor, and fill with bgcolor
 */
void Displayer::drawbez(flatpoint *bpoints,int n,int isclosed,int tofill)
{
	if (n<=0) return;

	moveto(bpoints[1]);
	int nn=(n-1)*3;
	for (int c=1; c<nn; c+=3) {
		curveto(bpoints[c+1],bpoints[c+2],bpoints[c+3]);
	}
	if (isclosed) {
		curveto(bpoints[nn+2],bpoints[0],bpoints[1]);
		closed();
	}
	if (draw_immediately) {
		if (tofill==0) stroke(0);
		else if (tofill==1) fill(0);
		else {
			unsigned long oldfg=FG();
			NewFG(BG());
			fill(1);
			NewFG(oldfg);
			stroke(0);
		}
	}
}


//! Draw a line across the whole view, and if num!=-10000000, print the number num where the line hits the sides of the window.
/*! Return the number of intersections the line makes with the screen.
 *
 * This uses drawnum() and drawline(), and subclasses need not redefine.
 */
int Displayer::drawrealline(flatline &ln,int num)
{
	int c=0;
	flatpoint p[2],fp;
	flatline l(real_coordinates?realtoscreen(ln.p):ln.p,
			   real_coordinates?realtoscreen(ln.p+ln.v):ln.p+ln.v);

	if (segmentandline(flatpoint(Minx,Miny),flatpoint(Maxx,Miny),l,fp))
	   { p[c].x=fp.x; p[c].y=fp.y; c++; }
	if (segmentandline(flatpoint(Minx,Maxy),flatpoint(Maxx,Maxy),l,fp))
	   { p[c].x=fp.x; p[c].y=fp.y; c++; }
	if (c<2 && segmentandline(flatpoint(Minx,Miny),flatpoint(Minx,Maxy),l,fp))
	   { p[c].x=fp.x; p[c].y=fp.y; c++; }
	if (c<2 && segmentandline(flatpoint(Maxx,Miny),flatpoint(Maxx,Maxy),l,fp))
	   { p[c].x=fp.x; p[c].y=fp.y; c++; }

	if (c>0 && num!=-10000000) drawnum(num, (int)p[0].x, (int)p[0].y);
	if (c>1) {
		if (num!=-10000000) drawnum(num, (int)p[1].x, (int)p[1].y);
		drawline(p[0],p[1]);
	}
	return c;
}

//! Draw the number num centered at screen coord (x,y).
/*!
 * If (x,y) is not on the screen, draw nothing.
 *
 * If the coordinate is near a screen edge, care is taken so that the number will
 * actually be drawn entirely in the screen, and not bleed off the edge.
 */
void Displayer::drawnum(double x, double y, int num)
{
	if (!onscreen(x,y)) return;
	char ch[20];
	sprintf(ch,"%d",num);

	double wid,h;
	textextent(ch,strlen(ch),&wid,&h);
	x-=wid/2;
	y-=h/2;

	if (x<Minx) x=Minx;
	else if (x>Maxx-wid) x=Maxx-wid;
	if (y<Miny) y=Miny;
	else if (y>Maxy-h) y=Maxy-h;
	textout(x,y,ch,strlen(ch),LAX_CENTER);
}



//------------------------------- Image functions

/*! \fn int Displayer::imageout(LaxImage *image, double x,double y, double w,double h)
 * \brief Output an image into a real space rectangle.
 *
 * This will obey any clipping in place.
 *
 * (x,y) is an additional translation to use. If w>0 AND h>0, then fit the image into
 * a rectangle with those real width and height. If w<=0 or h<=0, then use the
 * image's pixel width and height as bounds.
 *
 * Return 0 for image drawn with no complications.
 *
 * When there is an error and nothing is drawn, a negative value is returned.
 * Return -1 for image==NULL.
 * Return -2 for unknown image type.
 * Return -3 for image not available.
 *
 * When drawing effectively succeeds, but there are extenuating circumstances, a positive value is returned.
 * Return 1 for image not in viewport, so it's not actually drawn.
 * Return 2 for image would be drawn smaller than a pixel, so it's not actually drawn.
 *
 * \todo should be able to do partial drawing when an image is huge, and only a tiny
 *   portion of it actually should be rendered.
 */


/*! \fn void Displayer::imageout(LaxImage *image, double x,double y)
 * \brief Output an image at x,y with no further transform.
 */

/*! \fn void Displayer::imageout(LaxImage *img,double *matrix)
 */

/*! \fn void Displayer::imageout(LaxImage *img, double angle, double x,double y)
 */

/*! \fn void Displayer::imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly)
 */

/*! void Displayer::imageout_skewed(LaxImage *img,double x,double y,double ulx,double uly,double urx,double ury)
 */




//------------------------------ Transform functions

//! Centers the view at real point p.
/*! This finds the position of real point p on screen, then uses ShiftScreen() to center it.
 */
void Displayer::CenterPoint(flatpoint p)
{
	flatpoint s;
	s= flatpoint((Minx+Maxx)/2,(Miny+Maxy)/2)-realtoscreen(p);
	ShiftScreen((int)s.x,(int)s.y);
}

//! Make the center of Minx,Maxx... correspond to the real origin.
/*! This just calls CenterPoint(flatpoint(0,0)).
 */
void Displayer::CenterReal()
{ CenterPoint(flatpoint(0,0)); }


//! Center the bbox bounds, after transforming by m.
/*! Finds transformed bounds, then calls Center(double minx,double maxx,double miny,double maxy).
 */
void Displayer::Center(const double *m,DoubleBBox *bbox)
{
	DoubleBBox bb(transform_point(m,flatpoint(bbox->minx,bbox->miny)));
	bb.addtobounds(transform_point(m,flatpoint(bbox->maxx,bbox->miny)));
	bb.addtobounds(transform_point(m,flatpoint(bbox->maxx,bbox->maxy)));
	bb.addtobounds(transform_point(m,flatpoint(bbox->minx,bbox->maxy)));
	Center(bb.minx,bb.maxx,bb.miny,bb.maxy);
}

//! Center the bbox bounds, which are in real coordinates.
/*! This just passes values on to Center(double,double,double,double).
 */
void Displayer::Center(DoubleBBox *bbox)
{ Center(bbox->minx,bbox->maxx,bbox->miny,bbox->maxy); }

//! Zoom and center the view on the given real bounds.
/*! This will not rotate the display, but will shift and scale to view
 * properly. The axes aspect is maintained. 
 *
 * Uses Zoom() and CenterPoint(). Subclasses need not redefine.
 */
void Displayer::Center(double minx,double maxx,double miny,double maxy)
{
	if (maxx==minx || maxy==miny || Maxx==Minx || Maxy==Miny) return;
	DoubleBBox  bb(realtoscreen(flatpoint(minx,miny)));
	bb.addtobounds(realtoscreen(flatpoint(maxx,miny)));
	bb.addtobounds(realtoscreen(flatpoint(maxx,maxy)));
	bb.addtobounds(realtoscreen(flatpoint(minx,maxy)));

	if ((bb.maxx-bb.minx)/(bb.maxy-bb.miny)>double(Maxx-Minx)/(Maxy-Miny)) Zoom((Maxx-Minx)/(bb.maxx-bb.minx));
	else Zoom((Maxy-Miny)/(bb.maxy-bb.miny));
	CenterPoint(flatpoint((minx+maxx)/2,(miny+maxy)/2));
}


//! Move the viewable portion by dx,dy screen units.
/*! This finds the new transform then sets with NewTransform().
 *
 * In essence, do this:
 * \code
 *   ictm[4]-=dx;
 *   ictm[5]-=dy;
 *   ctm[4]+=dx;
 *   ctm[5]+=dy;
 *   findictm();
 *   syncPanner();
 * \endcode
 */
void Displayer::ShiftScreen(int dx,int dy)
{
	double t[6];
	transform_copy(t,Getctm());

	t[4]+=dx;
	t[5]+=dy;
	NewTransform(t);
}

//! Move the screen by real dx,dy along the real axes.
/*! Finds the appropriate transform, then NewTransform(newctm).
 *
 * The new transform is basically:
 * \code
 *  ctm[4]+=dx*t[0]+dy*t[2];
 *  ctm[5]+=dx*t[1]+dy*t[3];
 * \endcode
 */
void Displayer::ShiftReal(double dx,double dy)
{
	double t[6];
	transform_copy(t,Getctm());

	t[4]+=dx*t[0]+dy*t[2];
	t[5]+=dx*t[1]+dy*t[3];
	NewTransform(t);
}

/*! \fn void Displayer::NewTransform(double a,double b,double c,double d,double x0,double y0)
 * \brief Make the current transform correspond to the values.
 *
 * Replace current transform with new, then must syncPanner().
 */


//! Set the ctm to these 6 numbers, via NewTransform(d[0],d[1],d[2],d[3],d[4],d[5]).
void Displayer::NewTransform(const double *d)
{
	NewTransform(d[0],d[1],d[2],d[3],d[4],d[5]);
}

//! Push axes, and multiply ctm by a new transform.
/*! So newctm=m*ctm. Default is to use Getctm(), then NewTransform(double*) with the new matrix.
 */
void Displayer::PushAndNewTransform(const double *m)
{
	PushAxes();

	double tctm[6];
	transform_mult(tctm,m,Getctm());
	NewTransform(tctm);
}

//! Push axes, and multiply ctm by a new basis, p,x,y are all in real units.
/*! Simply a shortcut for:
 * \code
 *	PushAxes();
 *	NewAxis(p,x,y);
 * \endcode
 */
void Displayer::PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y)
{
	PushAxes();
	NewAxis(p,x,y);
}

/*! \fn void Displayer::PushAxes()
 * \brief Push the current axes on the axessstack.
 */


/*! \fn void Displayer::PopAxes()
 * \brief Recover the last pushed axes.
 */


//! Replace current transform wth a new orthogonal basis with origin o, xaxis unit corresponds to (xtip-o).
/*! This just does NewAxis(o,xtip-o,transpose(xtip-o)).
 */
void Displayer::NewAxis(flatpoint o,flatpoint xtip)
{
	NewAxis(o,xtip-o,transpose(xtip-o));
}

//! Replace current transform with origin o, with x and y as the new axes.
/*! If x is parallel to y, then nothing is done.
 *
 * Checks for that, then calls NewTransform( x.x,x.y, y.x,y.y, o.x,o.y,).
 */
void Displayer::NewAxis(flatpoint o,flatvector x,flatvector y)
{
	if (x.y*y.x!=0 && x.y*y.x==x.x*y.y) return; //if x || y
	NewTransform( x.x,x.y, y.x,y.y, o.x,o.y);
}

//! Rotate by angle, about screen coordinate (x,y).
/*! dec nonzero means angle is degrees, otherwise radians.
 */
void Displayer::Rotate(double angle,int x,int y,int dec)
{
	if (dec<0) dec=decimal;
	flatpoint p=screentoreal(x,y);
	Newangle(angle,1,dec);
	p=realtoscreen(p);
	ShiftScreen(x-p.x, y-p.y);
}

//! Rotate around real origin so that the x axis has angle between it and the screen horizontal.
void Displayer::Newangle(double angle,int dir,int dec)
{
	if (dec<0) dec=decimal;
	if (dec) angle*=M_PI/180;

	double ctm[6];
	transform_copy(ctm,Getctm());

	flatpoint rx(ctm[0],ctm[1]),ry(ctm[2],ctm[3]);
	if (dir==0) { /* absolute slant */
      	double angle2=atan2(ctm[1],ctm[0]); // current angle of rx to screen x
		rx=rotate(rx,-angle2+angle);
		ry=rotate(ry,-angle2+angle);
	} else if (dir<0) { /* rotate CW */
		rx=rotate(rx,-angle);
		ry=rotate(ry,-angle);
	} else { /* rotate CCW */
		rx=rotate(rx,angle);
		ry=rotate(ry,angle);
	}
	ctm[0]=rx.x;
	ctm[1]=rx.y;
	ctm[2]=ry.x;
	ctm[3]=ry.y;

	NewTransform(ctm);
}

//! Zoom around real point p.
void Displayer::Zoomr(double m,flatpoint p)
{
	flatpoint po=realtoscreen(p);
	Zoom(m);
	po=realtoscreen(p)-po;
	ShiftScreen(po.x,po.y);
}

//! Zoom around screen point x,y.
void Displayer::Zoom(double m,int x,int y)
{
	flatpoint po=screentoreal(x,y);
	char udp=updatepanner;
	updatepanner=0;
	DBG cerr <<"\nZoom:"<<m<<"   around real="<<po.x<<","<<po.y<<"  =s:"<<x<<','<<y<<"  ctm4,5="<<Getctm()[4]<<','<<Getctm()[5]<<endl;
	Zoom(m);
	po=realtoscreen(po);
	DBG cerr <<"   shift:"<<po.x-x<<","<<po.y-y<<endl;
	ShiftScreen(x-po.x, y-po.y);
	updatepanner=udp;
}

//! Zoom around the real origin.
void Displayer::Zoom(double m) // zooms with origin constant point
{
	//DBG cerr <<"Displayer zoom: "<<m<<" rx:"<<(ctm[0]*ctm[0]+ctm[1]*ctm[1])<<"  ry:"<<ctm[2]*ctm[2]+ctm[3]*ctm[3]<<endl;
	if (m<=0) return;

	double ctm[6];
	transform_copy(ctm,Getctm());
	if (m<1 && (m*sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1])<lowerbound || 
				m*sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3])<lowerbound)) return;
	if (m>1 && (m*sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1])>upperbound || 
				m*sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3])>upperbound)) return;
	//DBG cerr <<"  adjusting mag..."<<endl;
	ctm[0]*=m;
	ctm[1]*=m;
	ctm[2]*=m;
	ctm[3]*=m;
	NewTransform(ctm);
}

//! Return the magnification along the screen vector (x,y). (screen=mag*real)
/*! This is useful when the axes are of different lengths or are not
 * orthogonal, or are rotated.
 *
 * It is doing this:
 * \code
 *  flatpoint v=screentoreal(x,y),v2=screentoreal(0,0);
 *  return sqrt((x*x+y*y)/((v-v2)*(v-v2)));
 * \endcode
 */
double Displayer::GetVMag(int x,int y)
{
	flatpoint v=screentoreal(x,y),v2=screentoreal(0,0);
	return sqrt((x*x+y*y)/((v-v2)*(v-v2)));
}

//! Return the current magnification, screen=Getmag*real
/*! If y!=0 then return the y scale, default is to return the x scale.
 *
 * Note that this only returns basically length of axis in screen units divided
 * by length of axis in real coords. If the axes are not at a right angle to each
 * other, they do not correspond necessarily to the screen horizontal or vertical
 * magnification. For that, call GetVMag.
 */
double Displayer::Getmag(int y)
{
	const double *ctm=Getctm();
	if (y) return sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]);
	return sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]);
}

//! Set the xscale and the yscale, but preserve orientation, then syncPanner().
/*! If ys<=0, then the y scale is set the same as the xscale.
 */
void Displayer::Newmag(double xs,double ys)//ys=-1
{
	if (xs<=0) return;

	double ctm[6];
	transform_copy(ctm,Getctm());

	flatpoint rx(ctm[0],ctm[1]),ry(ctm[2],ctm[3]);
	rx=(xs/sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]))*rx;

	if (ys<=0) ys=xs;
	ry=(ys/sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]))*ry;
	ctm[0]=rx.x;
	ctm[1]=rx.y;
	ctm[2]=ry.x;
	ctm[3]=ry.y;

	NewTransform(ctm);

	DBG cerr <<"=====Newmag()="<<xs<<" x "<<ys<<endl;
}




//----------------------------------- Colors

/*! \fn unsigned long Displayer::NewFG(double r,double g,double b,double a)
 * \brief Set new foreground color. Typically usage is NewFG(app->rgbcolor(.5,.8,0,1.0)).
 * Component range is [0..1.0].
 */

/*! \fn unsigned long Displayer::NewFG(int r,int g,int b,int a)
 * \brief Set new foreground. Typically usage is NewFG(app->rgbcolor(23,34,234)).
 *  Component range is currently 0..255.... subject to change.
 */

/*! \fn unsigned long Displayer::NewFG(ScreenColor *col)
 * \brief Set new foreground. Color components are 0..0xffff.
 */

/*! \fn unsigned long Displayer::NewFG(unsigned long ncol)
 * \brief Set new foreground. Typically usage is NewFG(app->rgbcolor(23,34,234)).
 */

/*! \fn unsigned long Displayer::NewBG(double r,double g,double b)
 * \brief Set new background color. Typically usage is NewFG(app->rgbcolor(.5,.8,0)).
 * Component range is [0..1.0].
 */

/*! \fn unsigned long Displayer::NewBG(int r,int g,int b)
 * \brief Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
 */
	
/*! \fn unsigned long Displayer::NewBG(unsigned long nc)
 * \brief Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
 */



//------------------------ Panner upkeep

//! Call Updates(0) to disable updating the panner, Updates(1) to reenable it.
/*! This is used during refreshing, when there is constant
 * pushing and popping of the transform. Just sets a flag so that syncPanner
 * returns before doing anything.
 *
 * The programmer is responsible for balancing the push/pop of axes so that 
 * when updates are activated, the displayer and panner are still synced.
 *
 * Returns the old value of updatepanner.
 */
char Displayer::Updates(char toupdatepanner)
{
	char u=updatepanner;
	updatepanner=toupdatepanner;
	return u;
}

//! Using displayer viewable portion settings, set the panner settings.
/*! If all is nonzero, then set curpos and space in panner. Otherwise
 * just set the curpos.
 *
 * panner is not directly accessed, except in WrapWindow, here, and in
 * syncFromPanner. When telling the panner there have been changes (by setting
 * panner stuff here), the displayer does not want to receive a "pan change"
 * from the panner***
 */
void Displayer::syncPanner(int all)//all=0
{
	if (!updatepanner) return;
	DBG cerr<<"---====Displayer syncPanner"<<endl;
	// set the panner selbox values from the current displayer settings.
	// Maxx-Minx  is portion of transformed workspace, this is what to set in Panner
	// screentoreal(Minx,Miny)
	// realtoscreen(spaceminx,spaceminy) etc
	// 
	// If the current displayer settings do not conform to what the
	// panner allows, then call syncToPanner();
	
	if (xw) panner->dontTell(xw);
	long minx,maxx,miny,maxy;
	//**** maybe (&double, &double...) rather than long? check for too big (>LONG_MAX, <LONG_MIN) from limits.h
	GetTransformedSpace(&minx,&maxx,&miny,&maxy);
//	if (all) {//*** any shifting, etc changes the trasformed space, but sel=MinMaxx/y stays same!!
		panner->SetWholebox(minx,maxx,miny,maxy);
		panner->SetSelBounds(1,1,maxx-minx>maxy-miny?maxx-minx:maxy-miny);
		panner->SetSelBounds(2,1,maxx-minx>maxy-miny?maxx-minx:maxy-miny);
//	}
	panner->SetCurPos(1,Minx,Maxx);
	panner->SetCurPos(2,Miny,Maxy);
	panner->dontTell(NULL);
}

//! Using the settings in the panner, try to set the Displayer bounds.
/*! This would occur, for instance, on a "pan change" event resulting from
 * a scroller move. The panner does not implement rotating, so any changes would
 * be from a page size change or a straight move in screen x or y directions.
 * Thus the panner wholespace has not changed its values, but the panner selbox
 * has.  
 * 
 * panner contains the transformed workspace coords. Real space=ictm*twc.
 * So we must find a new transform so that the space fits the panner settings.
 * If the displayer's axes must remain the same length, care must be taken to 
 * ensure that, since the panner uses integers, which introduce rounding errors.
 *
 * This function is not called from within the Displayer. A holding window
 * (such as ViewportWindow) would call it upon receipt of a "pan change".
 *
 * Subclasses should call this, then update from ctm/ictm if necessary.
 */
void Displayer::syncFromPanner(int all)
{ //***
	DBG cerr <<"-=-= syncFromPanner"<<endl;
	//***if (!on) return;
	
	// panner->[start, end] must correspond to this->[Minx,Maxx]
	// transformed spaceminx/y still map to panner->min/max, but
	// they will not map after syncing, though they should have the same w and h.
	//
	// screen = real * ctm
	// real ---> transformed
	
	long xs,xe,ys,ye;
	panner->GetCurPos(1,&xs,&xe);
	panner->GetCurPos(2,&ys,&ye);

	double ctm[6];
	transform_copy(ctm,Getctm());

	// the transform is a rotation/scale/shear/translation. The rotation has stayed
	// the same. Using the old inverse transform, find the real points corresponding
	// to the new panner selbox:
	//*** this is really the long way around, and probably propagates panner rounding errors..
	DBG dumpctm(ctm);
	flatpoint r1,r2,r3, s1,s2,s3;
	s1=flatpoint(xs,ys);
	s2=flatpoint(xe,ys);
	s3=flatpoint(xe,ye);
	r1=screentoreal(s1); // --> this must map back to (Minx,Miny)
	r2=screentoreal(s2); // --> this must map back to (Maxx,Miny)
	r3=screentoreal(s3); // --> this must map back to (Maxx,Maxy)
	s1=flatpoint(Minx,Miny);
	s2=flatpoint(Maxx,Miny);
	s3=flatpoint(Maxx,Maxy);
	 // Those 3 real points are transformed by [A B C D X Y] to 3 screen points.
	 // 6 linear equations, 6 unknowns, viola! (Really 2 sets of 3 eq/3 unks)
	 // {{(e*i-f*h)/(--),(c*h-b*i)/(--),(b*f-c*e)/(--)
	 // ,{(f*g-d*i)/(--),(a*i-c*g)/(--),(c*d-a*f)/(--)
	 // ,{(d*h-e*g)/(--),(b*g-a*h)/(--),(a*e-b*d)/(--)
	 //
	 // i,f,c==1
	 // (--)=a*e*i-a*f*h+c*d*h-b*d*i+b*f*g-c*e*g
	 //           (a*e-a*h +         d*h-b*d +          b*g-e*g)
	 //          a*(e   -h) +       d*(h   -b) +       g*(b   -e)
	double dd=r1.x*(r2.y-r3.y) + r1.y*(r3.x-r2.x) + r2.x*r3.y-r2.y*r3.x;
	DBG cerr <<"dd="<<dd<<endl;

	//**** something screwed here.
	ctm[0]=(s1.x*(r2.y-r3.y) +           s2.x*(r3.y-r1.y) +           s3.x*(r1.y-r2.y))/dd; // s1.x  s2.x  s3.x
	ctm[2]=(s1.x*(r3.x-r2.x) +           s2.x*(r1.x-r3.x) +           s3.x*(r2.x-r1.x))/dd;
	ctm[4]=(s1.x*(r2.x*r3.y-r2.y*r3.x) + s2.x*(r1.y*r3.x-r1.x*r3.y) + s3.x*(r1.x*r2.y-r1.y*r2.x))/dd;
	
	ctm[1]=(s1.y*(r2.y-r3.y) +           s2.y*(r3.y-r1.y) +           s3.y*(r1.y-r2.y))/dd; // s1.y  s2.y  s3.y
	ctm[3]=(s1.y*(r3.x-r2.x) +           s2.y*(r1.x-r3.x) +           s3.y*(r2.x-r1.x))/dd;
	ctm[5]=(s1.y*(r2.x*r3.y-r2.y*r3.x) + s2.y*(r1.y*r3.x-r1.x*r3.y) + s3.y*(r1.x*r2.y-r1.y*r2.x))/dd;

	DBG dumpctm(ctm);

	if (displayer_style&DISPLAYER_NO_SHEAR) {
		if (ctm[3]*ctm[0]-ctm[2]*ctm[1]>0) {
		 	 //if right handed
			ctm[2]=-ctm[1];
			ctm[3]= ctm[0];
		} else {
			ctm[2]= ctm[1];
			ctm[3]=-ctm[0];
		}
	}
	NewTransform(ctm);

	//*** should do final check to cut off any rounding errors; must preserve relative
	//scaling from before adjusting transform...
	syncPanner(); // This is necessary because the selbox in the panner always corresponds to 
				//   Minx,Miny..Maxx,Maxy, which is not what is in the panner before this line.
				//   ***IF the wholebox and selbox were reversed, ..... more thought here!!
				//   this is too complicated!
}

//! Set the view area to this section of the screen.
/*! Basically, say there's a box in screen coordinates that you want to zoom in on, within the screen
 * bounds given. The viewport will be zoomed so that that area is expanded (or shrunk) to fill the viewport.
 *
 * Transforms the screen box into a real box, then uses
 * Center(double minx,double maxx,double miny,double maxy).
 *
 * Min/Maxx/y will not be changed, but 
 * afterwards they will correspond to these bounds.
 * This is essentially a more complicated form of zooming.
 *
 * \todo *** this needs testing
 */
void Displayer::SetView(double minx,double maxx,double miny,double maxy)
{
	if (minx>maxx || miny>maxy) return;

	DoubleBBox bbox(minx,maxx,miny,maxy),bbox2;
	const double *ictm=Getictm();
	bbox2.addtobounds(ictm,&bbox);
	Center(bbox2.minx,bbox2.maxx,bbox2.miny,bbox2.maxy);
}

//! Set the workspace bounds, return nonzero if successful.
/*! Returns 1 if successful and viewable area not changed.\n
 * Returns 2 if successful and viewable are is changed.
 *
 * Please note that checking whether the viewable area is changed is not
 * implemented. Currently, assumes that it is and returns 2.
 *
 * Basically sets spacemin/max (ensuring that min is actually < max),
 * then calles syncPanner(1). Subclasses should redefine
 * if special care is needed.
 */
int Displayer::SetSpace(double minx,double maxx,double miny,double maxy)
{
	if (minx>maxx) { double t=minx; minx=maxx; maxx=t; }
	if (miny>maxy) { double t=miny; miny=maxy; maxy=t; }
	spaceminx=minx;
	spacemaxx=maxx;
	spaceminy=miny;
	spacemaxy=maxy;
	
	DBG cerr <<"--displayer space: ["<<spaceminx<<','<<spacemaxx<<"] ["<<spaceminy<<','<<spacemaxy<<']'<<endl;
	DBG cerr <<"--displayer win:   ["<<Minx<<','<<Maxx<<"] ["<<Miny<<','<<Maxy<<']'<<endl;

	syncPanner(1);
	return 2;
}

//! Get the real, not screen, workspace bounds.
/*! Note that if the real space is rotated, the returned values here are the actual real coordinates
 * for the bounds. For a rectangled aligned with the screen, use GetTransformedSpace().
 */
void Displayer::GetSpace(double *minx,double *maxx,double *miny, double *maxy)
{
	if (minx) *minx=spaceminx;
	if (maxx) *maxx=spacemaxx;
	if (miny) *miny=spaceminy;
	if (maxy) *maxy=spacemaxy;
}

//! Find the mins and maxes of the transformed workspace.
/*! It is ok to have some of the pointers NULL.
 * 
 * Note that these will be different values from GetSpace() when the viewing space is rotated.
 */
void Displayer::GetTransformedSpace(long *minx,long *maxx,long *miny,long *maxy)
{
	flatvector  v1=realtoscreen(flatpoint(spaceminx,spaceminy)),
				v2=realtoscreen(flatpoint(spacemaxx,spaceminy)),
				v3=realtoscreen(flatpoint(spacemaxx,spacemaxy)),
				v4=realtoscreen(flatpoint(spaceminx,spacemaxy)),
				spanmin,spanmax;
	spanmin=spanmax=v1;
	if (v2.x<spanmin.x) spanmin.x=v2.x;
	if (v3.x<spanmin.x) spanmin.x=v3.x;
	if (v4.x<spanmin.x) spanmin.x=v4.x;
	
	if (v2.y<spanmin.y) spanmin.y=v2.y;
	if (v3.y<spanmin.y) spanmin.y=v3.y;
	if (v4.y<spanmin.y) spanmin.y=v4.y;
	
	if (v2.x>spanmax.x) spanmax.x=v2.x;
	if (v3.x>spanmax.x) spanmax.x=v3.x;
	if (v4.x>spanmax.x) spanmax.x=v4.x;
	
	if (v2.y>spanmax.y) spanmax.y=v2.y;
	if (v3.y>spanmax.y) spanmax.y=v3.y;
	if (v4.y>spanmax.y) spanmax.y=v4.y;

	if (minx) *minx=(long) spanmin.x;
	if (maxx) *maxx=(long) spanmax.x;
	if (miny) *miny=(long) spanmin.y;
	if (maxy) *maxy=(long) spanmax.y;
}

//! Use the supplied PanController for bounds control.
/*! Displayer overwrites the values in the panner to correspond to
 * the displayer values. The whole space of the panner corresponds to 
 * the transformed displayer workspace in screen window coords
 * scaled to [-1000000,1000000].
 *
 * Basically, dose UseThisPanner() then syncPanner(1).
 *
 * \todo wth? is this doc still true???
 */
void Displayer::UseThisPanner(PanController *npanner)
{
	PanUser::UseThisPanner(npanner);
	syncPanner(1);
}

//! Set the viewable portion of the displayer to correspond with the window's win_w and win_h
/*! So Minx,Maxx,Miny,Maxy==0,win_w, 0,win_h
 * Also sets the panner boxaspect to correspond to the window proportions.
 * Call syncPanner() to update anything attached to it.
 *
 *  If the spacemin/max seem not to be set, then set things so that the whole space is
 *  10 times the window size, and the viewable portion is in the middle.
 *
 *  PLEASE NOTE this does not do anything but set Min/Max, spacemin/max, mainly for panner upkeep.
 *  Other drawing targets or other state would be done in StartDrawing().
 *
 *  \todo seems silly to wrap to this window, but not make all the other properties correspond to nw,
 *    such as connecting the drawing surface to the window.. needs more thought here.
 */
void Displayer::WrapWindow(anXWindow *nw)
{
	if (!nw || !nw->xlibDrawable()) return;
	Minx=0;
	Miny=0;

	//Window rootwin;
	//int x,y;
	//unsigned int width,h,bwidth,depth;
	//XGetGeometry(nw->app->dpy,nw->xlib_window,&rootwin,&x,&y,&width,&h,&bwidth,&depth);
	Maxx=nw->win_w;
	Maxy=nw->win_h;
	if (Maxx<Minx) Maxx=Minx;
	if (Maxy<Miny) Maxy=Miny;
	if (spaceminx==spacemaxx) {
		spaceminx=-(Maxx-Minx)*5;
		spacemaxx=(Maxx-Minx)*5;
	}
	if (spaceminy==spacemaxy) {
		spaceminy=-(Maxy-Miny)*5;
		spacemaxy=(Maxy-Miny)*5;
	}
	panner->dontTell(xw);
	panner->SetBoxAspect(Maxx-Minx+1,Maxy-Miny+1);//***
	panner->dontTell(NULL);
	syncPanner();

	//DBG 
	//DBG cerr <<"-----displayer WrapWindow:MinMaxx,:["<<Minx<<","<<Maxx<<"] MinMaxy:["
	//DBG		<<Miny<<","<<Maxy<<"] x0,y0:"<<ctm[4]<<","<<ctm[5]<<endl;
}


/*! \fn int Displayer::StartDrawing(aDrawable *buffer)//buffer=0
 * \brief Set up to be drawing on a buffer.
 *
 * An important trait of starting drawing with this is that Updates(0) is always in effect until 
 * EndDrawing is called. Calling this start is when you want to draw a bunch of stuff
 * on any old pixmap, and not fuss with the panner things.
 *
 * Sets Minx=Miny=0, Maxx=width, Maxy=height of drawable.
 * 
 * If xw==NULL when EndDrawing() is called, then Updates(1) is called. Please remember, however,
 * that the Minx,Maxx,... are screwed up by then, and you must sync those yourself.
 */


/*! \fn int Displayer::MakeCurrent(aDrawable *buffer)
 * \brief Update the drawing context so that all drawing operations will now operate on buffer.
 */

/*! \fn int Displayer::ClearDrawable(aDrawable *buffer)
 * \brief Free any data associated with drawable. Return 1 for drawable not currently used, or 0 for cleared.
 */

/*! \fn LaxImage *Displayer::GetSurface()
 * \brief Return a new LaxImage that is copied from the current buffer.
 *
 * Returned image has count of 1.
 *
 * The particular type of LaxImage will usually depend on the particular type of Displayer.
 */


/*! \fn int Displayer::CreateSurface(int width,int height, int type)
 * \brief Clear the old surface, and create a fresh surface to perform drawing operations on.
 * 
 * Return 0 for successful creation, or nonzero for unable to do so.
 */


/*! \fn int Displayer::ResizeSurface(int width, int height)
 * \brief Resize an internal drawing surface.
 *
 * If you had previously called CreateSurface(), this will resize that
 * surface. If the target surface is an external surface, then nothing is done.
 *
 * Return 0 for success, or 1 if not using an internal surface, and nothing done.
 *
 * \todo maybe preserve and position contents of old surface? like canvas resize??
 */


/*! \fn int Displayer::EndDrawing()
 * \brief Set everything assuming we do not need to draw on the current surface for the time being.
 *
 * Updates(1) must be called, so that further changes to the viewport will be sent to the panner.
 *
 * This resets all the drawing bits to 0 to prevent calls made from non-window elements
 * trying to use windows that might have been destroyed. Unfortunately, such error checking for
 * drawing bits set to 0 is hardly implemented in Displayer drawing functions.. Displayer needs
 * more work...
 *
 * If xw==NULL, then call Updates(1);
 */





} // namespace Laxkit

