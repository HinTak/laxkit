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
//    Copyright (C) 2013 by Tom Lechner
//

#include <lax/newwindowobject.h>
#include <lax/strmanip.h>


namespace Laxkit {

//---------------------------- NewWindowObject ---------------------------------

NewWindowObject::NewWindowObject(const char *nname,const char *ndesc,unsigned long nstyle,NewWindowFunc f)
{
	name=newstr(nname);
	desc=newstr(ndesc);
	style=nstyle; 
	function=f;
}

NewWindowObject::~NewWindowObject()
{
	if (name) delete[] name;
	if (desc) delete[] desc;
}

} //namespace Laxkit

