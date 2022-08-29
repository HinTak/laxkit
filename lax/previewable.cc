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
//    Copyright (C) 2016 by Tom Lechner
//

#include <lax/previewable.h>

#include <iostream>
using namespace std;

#define DBG


namespace Laxkit {

//--------------------------------- Previewable ----------------------------

/*! \class Previewable
 *
 * Standardizes getting little preview images from various kinds of objects.
 */

Previewable::Previewable()
{
	preview    = NULL;
	previewtime= 0; //times() at which preview was last rendered
    modtime    = 0; //times() of most recent modification that should trigger a preview rerender
}

Previewable::~Previewable()
{
	if (preview) preview->dec_count();
}

/*! Call this whenever there's reason to think the preview needs to be updated.
 * Sets modtime to be the current time, but does NOT actually do a preview render.
 */
void Previewable::touchContents()
{
    previewtime = 0;
    tms tms_;
    modtime     = times(&tms_);
}

LaxImage *Previewable::GetPreview()
{
	if (previewtime < modtime || !preview) GeneratePreview(-1,-1);
    return preview;
}

int Previewable::maxPreviewSize()
{
	return 200;
} 

/*! Set up a LaxImage to hold a preview, then call renderToBufferImage() to
 * actually render the preview.
 */
int Previewable::GeneratePreview(int w, int h)
{
   if (preview) {
        if (w<=0 && h<=0) {
            w=preview->w();
            h=preview->h();
        }
         //make the desired preview dimensions approximately sympatico with
         //the actual dimensions of the object
        if ((w>h && (maxx-minx)<(maxy-miny))) h=0;
        else if ((w<h && (maxx-minx)>(maxy-miny))) w=0;
    }

    int maxdim = maxPreviewSize();

    if (w<=0 && h<=0) {
        if (maxx-minx>maxy-miny) w=maxdim;
		else h=maxdim;
    }

    if (w<=0 && h>0) w=(maxx-minx)*h/(maxy-miny);
    else if (w>0 && h<=0) h=(maxy-miny)*w/(maxx-minx);

    if (w<=0) w=1;
    if (h<=0) h=1;

     //protect against growing sizes...
    if (w > maxdim) {
        double aspect=(double)h/w;
        w = maxdim;
        h = maxdim*aspect;
        if (h <= 0) h = 1;
    }
    if (h>maxdim) {
        double aspect=(double)w/h;
        h = maxdim;
        w = maxdim*aspect;
        if (w <= 0) w = 1;
    }

 	//if (preview && (w!=preview->w() || h!=preview->h())) {
    if (preview && ((float)w/preview->w()>1.05 || (float)w/preview->w()<.95 ||
                    (float)h/preview->h()>1.05 || (float)h/preview->h()<.95)) {
         //delete old preview and make new only when changing size of preview more that 5%-ish in x or y
        preview->dec_count();
        preview = NULL;
        DBG cerr <<"removed old preview..."<<endl;
    }

    if (!preview && CanRenderPreview()) {
        DBG cerr <<"old preview didn't exist, so creating new one..."<<endl;
        preview = ImageLoader::NewImage(w,h);
    }

    if (preview && renderToBufferImage(preview)==0) {
         tms tms_;
		 previewtime = times(&tms_);

	} else {
         //render direct to image didn't work, so try the old style render to char[] buffer...
		previewtime = 0;
        //unsigned char *buffer = preview->getImageBuffer();
        //renderToBuffer(buffer,w,h,w*4,8,4);
        //preview->doneWithBuffer(buffer);
    	//previewtime=times(NULL);

    }

	return (previewtime >= modtime);
}


} //namespace Laxkit


