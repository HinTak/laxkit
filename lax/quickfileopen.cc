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
//    Copyright (C) 2004-2006,2010 by Tom Lechner
//

#include <lax/language.h>
#include <lax/quickfileopen.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

/*! \class QuickFileOpen
 * \brief A button with "..." on it that pops up a FileDialog sporting a menu with FileMenuItem entries.
 *
 * A StrEventData with the chosen file is sent to the owner (which is
 * passed in to the constructor).
 */

//! nowner is who gets sent the StrEventData of the file chosen.
QuickFileOpen::QuickFileOpen(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						unsigned int ntype,
						LineInput *npath,
						const char *ndir,
						const char *nlabel)
	: Button(parnt,nname,ntitle,(nstyle&(0xffff))|BUTTON_MOMENTARY,
			 xx,yy,ww,hh,brder,prev,nowner,nsendmes,
			 0,
			 nlabel?nlabel:"...",NULL,NULL)
{
	win_style = nstyle | BUTTON_MOMENTARY;
	type = ntype;
	path = npath;
	dir = newstr(ndir);
	if (!dir && !path) {
		dir = lax_dirname(path->GetCText(),1);
	}
}

QuickFileOpen::~QuickFileOpen()
{
	if (dir) delete[] dir;
}

//! Set the current path to some non-null path. Ok to be "".
void QuickFileOpen::SetDir(const char *ndir)
{
	if (ndir) makestr(dir,ndir);
}

	
//! Called when mouse is up, the pops up the PopupMenu via app->rundialog(new PopupMenu).
int QuickFileOpen::send(int deviceid,int direction)
{	
	char *ddir=newstr(dir);
	if (!ddir && path) ddir=lax_dirname(path->GetCText(),1);

	const char *title=(path ? path->WindowTitle() : NULL);
	if (!title) title=_("File Popup Menu");

	app->rundialog(new FileDialog(NULL,title,title, ANXWIN_REMEMBER,
								0,0,0,0,1, 
								win_owner,win_sendthis, 
								type | FILES_PREVIEW,
								path?path->GetCText():NULL,
								ddir),
					NULL,1);
	delete[] ddir;
	return 0;
}

int QuickFileOpen::MoveResize(int nx,int ny,int nw,int nh)
{
	return anXWindow::MoveResize(nx,ny,nw,nh);
}
int QuickFileOpen::Resize(int nw,int nh)
{
	return anXWindow::Resize(nw,nh);
}



} // namespace Laxkit

