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
//    Copyright (C) 2015 by Tom Lechner
//

#include <lax/strmanip.h>
#include <lax/objectfactory.h>


namespace Laxkit {


//---------------------------- ObjectFactoryNode ---------------------------------
/*! \class ObjectFactoryNode
 * \brief Internal node for ObjectFactory.
 */


//---------------------------- ObjectFactory ---------------------------------
/*! \class ObjectFactory
 * \brief Class to get instances of interface data.
 *
 * This class makes it unnecessary to have a complicated system of function pointers
 * to add, remove, and save objects.
 */
/*! \fn int ObjectFactory::delObject(T *obj)
 * \brief Currently does nothing and is not called from any Lax classes...
 */
/*! \fn ObjectFactory::~ObjectFactory()
 * \brief Empty virtual destructor.
 */


///*! Static default factory retrieval function.
// */
//ObjectFactory *ObjectFactory::GetDefault(bool create_if_null)
//{
//	if (!default_factory && create_if_null) {
//		default_factory = new ObjectFactory();
//	}
//
//	return default_factory;
//}
//
///*! If you pass in NULL, old one is removed and NULL takes its place.
// */
//void ObjectFactory::SetDefault(ObjectFactory *newfactory)
//{
//	if (default_factory) delete default_factory;
//	default_factory=newfactory;
//}



ObjectFactoryNode *ObjectFactory::newObjectFactoryNode()
{
	return new ObjectFactoryNode();
}


//! Add ability to make a new type of object.
/*! If newname already exists, then do nothing if force_new == false and return -1. If
 * force_new == true, then replace the old definition with the new definition.
 * 
 * If newname not found, return the index on types
 * that the new definition is pushed.
 */
int ObjectFactory::DefineNewObject(int newid, const char *newname, NewObjectFunc newfunc,DelObjectFunc delfunc,
					int param, bool force_new)
{
	int exists = 0;
	int i = findPosition(newname,&exists);
	if (exists) {
		if (!force_new) return -1;
		types.remove(i);
	}

	ObjectFactoryNode *node = newObjectFactoryNode();
	node->name = newstr(newname);
	node->id = newid;
	node->newfunc = newfunc;
	node->delfunc = delfunc;
	node->parameter = param;
	
	return types.push(node,1,i);
}

/*! Return index in list of name, or -1 if not found.
 */
int ObjectFactory::FindType(const char *name)
{
	int exists = 0;
	int c = findPosition(name, &exists);
	if (exists) return c;
	return -1;
}

/*! Assuming types is sorted, do a binary search for name.
 * If not actually there, return index where it would be.
 */
int ObjectFactory::findPosition(const char *name, int *exists)
{
	*exists=0;
	if (types.n==0) return 0;

	int s=0, e=types.n-1, cmp, m;
	do {
		cmp=strcmp(name, types.e[s]->name);
		if (cmp<0) return s;
		if (cmp==0) { *exists=1; return s; }

		cmp=strcmp(name, types.e[e]->name);
		if (cmp>0) return e+1;
		if (cmp==0) { *exists=1; return e; }

		m=(s+e)/2;
		cmp=strcmp(name, types.e[m]->name);
		if (cmp==0) { *exists=1; return m; }

		 //we know name is greater than s and less than e
		if (s==e-1) return e;
		if (cmp<0) {
			s=s+1;
			e=m-1;
		} else {
			s=m+1;
			e=e-1;
		}
		
		if (s==e) {
			cmp=strcmp(name, types.e[s]->name);
			if (cmp<0) return s;
			if (cmp==0) { *exists=1; return s; }
			return s+1;
		}

	} while (s<e);

	return s;
}

//! Return object based on id number objtype.
anObject *ObjectFactory::NewObject(int objtype, anObject *refobj)
{
	for (int c=0; c<types.n; c++) {
		if (objtype == types.e[c]->id) {
			return types.e[c]->newfunc(types.e[c]->parameter, refobj);
		}
	}
	return NULL;
}

//! Return object based on name, which should be the same as object->whattype().
anObject *ObjectFactory::NewObject(const char *objtype, anObject *refobj)
{
	int exists=0;
	int i=findPosition(objtype, &exists);
	if (exists && i>=0) return types.e[i]->newfunc(types.e[i]->parameter, refobj);
	return NULL;
}

const char *ObjectFactory::TypeStr(int which)
{
	if (which>=0 && which<types.n) return types.e[which]->name;
	return NULL;
}

int ObjectFactory::TypeId(int which)
{
	if (which>=0 && which<types.n) return types.e[which]->id;
	return 0;
}


/*! Abbreviated dump_out here mainly for debugging purposes..
 */
void ObjectFactory::dump_out(FILE *f, int indent)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0';

	fprintf(f,"%sObjectFactory\n",spc);
	for (int c=0; c<types.n; c++) {
		fprintf(f,"%s  %s  %d %d\n",spc, types.e[c]->name, types.e[c]->id, types.e[c]->info);
	}
}



} //namespace Laxkit


