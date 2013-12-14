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
//    Copyright (c) 2004-2010 Tom Lechner
//
#ifndef _LAX_LINEEDIT_H
#define _LAX_LINEEDIT_H

#include <lax/textxeditbase-utf8.h>
#include <lax/buttondowninfo.h>
	
#include <cstdio>
#include <cctype>
#include <unistd.h>
#include <sys/times.h>

#define LINEEDIT_CNTLTAB_NEXT     (1<<16)
#define LINEEDIT_INT              (1<<17)
#define LINEEDIT_FLOAT            (1<<18)
#define LINEEDIT_FILE             (1<<19)
//#define LINEEDIT_PASSWORD
//#define LINEEDIT_DATE
//#define LINEEDIT_TIME
#define LINEEDIT_SEND_ANY_CHANGE  (1<<20)
#define LINEEDIT_SEND_FOCUS_ON    (1<<21)
#define LINEEDIT_SEND_FOCUS_OFF   (1<<22)
#define LINEEDIT_DESTROY_ON_ENTER (1<<23)
#define LINEEDIT_GRAB_ON_MAP      (1<<24)

namespace Laxkit {

class LineEdit : public TextXEditBaseUtf8
{
  protected:
	ButtonDownInfo buttondown;
	virtual int send(int i);
	virtual void settextrect();
  public:
	const char *qualifier;
	char *blanktext;

	int padx,pady,mostpixwide;
	LineEdit(anXWindow *parnt,const char *nname,const char *ntitle,unsigned int nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner=0,const char *nsend=NULL,
			const char *newtext=NULL,unsigned int ntstyle=0);
	virtual ~LineEdit();
	virtual const char *whattype() { return "LineEdit"; }
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);
	virtual void ControlActivation(int on);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y, unsigned int ,const LaxMouse *d);
	virtual int RBDown(int x,int y, unsigned int ,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int ,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int Idle(int tid=0); // for autoscroll
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int Resize(int nw,int nh);
	
	virtual int LBDblClick(int x,int y, int cntl,const LaxMouse *d);

	virtual void Valid(int v);
	virtual void Valid(int v,unsigned long col);
	virtual int Modified(int m=1);
	virtual long   GetLong(int *error_ret);
	virtual double GetDouble(int *error_ret);
	virtual int Replace(char *newstr,char *what,int all);
	virtual int SetText(int newtext);
	virtual int SetText(double newtext);
	virtual int SetText(const char *newtext);
	virtual long SetCurLine(long nline) { return 0; };
	virtual long SetCurpos(long newcurpos); // removes selection
	virtual int SetSelection(long newss,long newse);

	virtual int inschar(unsigned int ch,char a=1);
	virtual int delchar(int bksp);
	virtual int insstring(const char *blah,int after=0); //after=0
	virtual int delsel();
	virtual int replacesel(char ch) { return TextEditBaseUtf8::replacesel(ch); }
	virtual int replacesel(const char *newt=NULL,int after=0); // after=0, newt=NULL
	virtual int makeinwindow();  // assumes cx,cy already set accurately
	virtual long findpos(int pix);  // find char at pix from left, not use curlineoffset
	virtual void findcaret();
	virtual int Setpixwide(); 
	virtual int SetupMetrics();
	virtual void DrawText(int a=-1);

    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *loadcontext);
};

} // namespace Laxkit


#endif
