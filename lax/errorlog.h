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
//    Copyright (C) 2012 by Tom Lechner
//
#ifndef ERRORLOG_H
#define ERRORLOG_H


#include <lax/lists.h>


namespace Laxkit {


//---------------------------------- ErrorLog -----------------------------

/*! ERROR_Ok, means everything checks out.
 * ERROR_Fail is an extreme error which should interrupt whatever you are doing.
 * ERROR_Warning is a generic error which does not halt anything, but users can attend to it.
 * Other values above ERROR_MAX can be used for other warnings.
 */
enum ErrorSeverity {
		ERROR_Unknown =-1,
		ERROR_Ok      =0,
		ERROR_Fail    =1,
		ERROR_Warning =2,
		//ERROR_Missing_Glyphs,
		//ERROR_Unexposed_Text,
		//ERROR_Has_Transparency,
		//ERROR_Broken_Image,
		//ERROR_Broken_Resource,
		//ERROR_Image_Not_At_Desired_Resolution,
		ERROR_MAX

	};

class ErrorLogNode
{
  public:
	char *path;
	char *objectstr_id;
	unsigned int object_id;
	char *description;
	int severity; //"ok", "warning", "fail", "version fail"
	int info, pos,line; //extra info
	ErrorLogNode();
	ErrorLogNode(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int nseverity,int ninfo, int npos,int nline);
	virtual ~ErrorLogNode();
	virtual void Set(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int nseverity,int ninfo, int npos,int nline);
};

class ErrorLog
{
  protected:
	virtual ErrorLogNode *newErrorLogNode();
  public:
	Laxkit::PtrStack<ErrorLogNode> messages;

	ErrorLog();
	virtual ~ErrorLog();
	virtual int AddMessage(const char *desc, int severity, int ninfo=0, int npos=0,int nline=0);
	virtual int AddMessage(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int severity, int ninfo=0, int npos=0,int nline=0);
	virtual const char *Message(int i,int *severity,int *info, int *pos=NULL,int *line=NULL);
	virtual int Total() { return messages.n; }
	virtual ErrorLogNode *Message(int i);
	virtual const char *MessageStr(int i);
	virtual char *FullMessageStr();
	virtual int Warnings(int since=0);
	virtual int Errors(int since=0);
	virtual int Oks(int since=0);
	virtual void Clear();
};

void dumperrorlog(const char *mes,ErrorLog &log);


} //namespace Laxkit


#endif

