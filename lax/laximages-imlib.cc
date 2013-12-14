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
//    Copyright (C) 2010 by Tom Lechner
//

#include <X11/Xlib.h>
#include <lax/laximages-imlib.h>
#include <lax/strmanip.h>
#include <lax/vectors.h>
#include <lax/transformmath.h>
#include <lax/anxapp.h>


#include <iostream>
using namespace std;
#define DBG 



namespace Laxkit {



//--------------------------- LaxImlibImage --------------------------------------
/*! \class LaxImlibImage
 * \brief A LaxImage based on Imlib_Images.
 */
/*! \var char LaxImlibImage::flag
 * \brief 1 if the image should be free'd when not in use (assumes filename is good). else 0.
 */
/*! \var char LaxImlibImage::whichimage
 * \brief Which image is in memory. 0 for none, 1 for main, 2 for preview.
 */
/*! \var int LaxImlibImage::width
 * \brief The actual width of the full image.
 */
/*! \var int LaxImlibImage::height
 * \brief The actual height of the full image.
 */
/*! \var int LaxImlibImage::dwidth
 * \brief The actual width of the preview image, or 0 if not loadable.
 */
/*! \var int LaxImlibImage::dheight
 * \brief The actual height of the preview image, or 0 if not loadable.
 */


/*! If fname and img, then assume that img corresponds to fname, read dimensions from img,
 * then free it.
 * If fname and !img, then get the dims from imlib by loading fname and reading off dims.
 * If !fname and img, then set flag=0 (do not free image during LaxImage lifetime).
 *
 * Note that Imlib docs say loading, reading off things, then freeing is a good thing.
 *
 * The preview image is set up only if the main image exists, and only if the image's
 * dimensions exceed maxx or maxy. Also if the main image does not exist, previewfile
 * is set to NULL, and npfile is ignored. Whether the main image exists is by checking 
 * whether width>0, not by whether filename exists, since there might be no associated
 * filename for the image.
 *
 * If npfile is given, but maxx<=0 or maxy<=0, then set up the preview file only if
 * it existed already. A new preview is not generated. A new preview IS generated whenever
 * maxx>0 or maxy>0 AND the main image's dimensions are too big for the specified bounds, 
 * AND npfile cannot be opened as an image by Imlib.
 * If only one of maxx or maxy is greater than 0, then only fit to be within that bound.
 *
 * If del is non-zero, then the previewfile is unlinked in this's destructor.
 * 
 * \todo *** needs to be some error checking when generating new previews
 * \todo scaling to maxx OR maxy if either 0 not implemented. both must currently be nonzero.
 * \todo when generating preview, might be wise to have check for freedesktop thumb locations to enforce
 *    proper sizing
 */
LaxImlibImage::LaxImlibImage(const char *fname,Imlib_Image img,const char *npfile,int maxx,int maxy,char del)
	: LaxImage(fname)
{
	if (maxy==0) maxy=maxx;
	whichimage=0;
	flag=0;
	image=NULL;
	if (!img) {
		if (fname) image=imlib_load_image(fname);
	} else image=img;
	if (image) {
		whichimage=1;
		imlib_context_set_image(image);
		width= imlib_image_get_width();
		height=imlib_image_get_height();
		if (!img || (img && fname)) {
			imlib_free_image();
			image=NULL;
			whichimage=0;
		} else if (fname) flag=1;
	} else {
		width=height=0;
	}

	dwidth=dheight=0;
	previewfile=NULL;

	 // only set up the preview image if the main image exists and is readable
	if (width>0 && npfile && (width>maxx || height>maxy)) { 
		previewfile=newstr(npfile);
		Imlib_Image pimage;
		pimage=imlib_load_image(previewfile);
		if (pimage) {
			DBG cerr <<" = = = Using existing preview \""<<previewfile<<"\" for \""<<(filename?filename:"(unknown)")<<"\""<<endl;
			 // preview image already existed, so use it
			imlib_context_set_image(pimage);
			dwidth= imlib_image_get_width();
			dheight=imlib_image_get_height();
			imlib_free_image();
			delpreview=0;
		} else if (maxx>0 && maxy>0) {
			DBG cerr <<" = = = Making new preview \""<<previewfile<<"\" for \""<<(filename?filename:"(unknown)")<<"\""<<endl;
			 // preview image didn't already existed, so make one
			 
			 //****make sure previewfile is writable
			 
			 //figure out dimensions of new preview
			double a=double(height)/width;
			if (a*maxx>maxy) {
				dheight=maxy;
				dwidth=int(maxy/a);
			} else {
				dwidth=maxx;
				dheight=int(maxx*a);
			}
			generate_preview_image(filename,previewfile,"jpg",dwidth,dheight,0);
			delpreview=del;
		}
	}
}

//! Free image if it happens to exist.
LaxImlibImage::~LaxImlibImage()
{
	if (image) {
		imlib_context_set_image(image);
		imlib_free_image();
		image=NULL;
		whichimage=0;
	}
}

void LaxImlibImage::clear()
{
	if (image) {
		imlib_context_set_image(image);
		imlib_free_image();
		image=NULL;
		whichimage=0;
	}
	if (filename) { delete[] filename; filename=NULL; }
	if (previewfile) { delete[] previewfile; previewfile=NULL; }
	width=height=dwidth=dheight=0;
}

unsigned int LaxImlibImage::imagestate()
{
	return (filename?LAX_IMAGE_HAS_FILE:0) |
		   (dwidth>0?LAX_IMAGE_PREVIEW:0) |
		   (width>0?LAX_IMAGE_METRICS:0) |
		   (whichimage==1?LAX_IMAGE_WHOLE:0) |
		   (whichimage==2?LAX_IMAGE_PREVIEW:0); //****
}


//! Calling this makes it easier on the imlib cache.
/*! Calls <tt>imlib_context_set_image(image)</tt> then <tt>imlib_free_image()</tt>
 * if image was not null and flag==0.
 */
void LaxImlibImage::doneForNow()
{
	if (!image || flag) return;
	imlib_context_set_image(image);
	imlib_free_image();
	image=NULL;
	whichimage=0;
}

//! Return the image. Loads from filename if !image.
/*! Attempts to load the previewfile. If that fails, then
 * attempts to load the real file.
 *
 * which==0 is get default (whichever is already loaded, or if none loaded, then 
 * preview if avaible, else main image), 1 is main, 2 is preview.
 * If 1 and main is unavailable, or 2 and the preview is unavailable, then NULL is
 * returned.
 *
 * \todo note that if image is already loaded, this function does not yet switch to
 *   the proper one..
 */
Imlib_Image LaxImlibImage::Image(int which)
{
	if (image) {
		 // ensure that which is checked against currently loaded image.
		 // If the wrong one is loaded, then unload it.
		if ((which==1 && whichimage==2) || (which==2 && whichimage==1)) {
			imlib_context_set_image(image);
			imlib_free_image();
			image=NULL;
			whichimage=0;
		}
	}
	if (!image) {
		if (previewfile && dwidth>0 && (which==0 || which==2)) {
			 //if request default and preview exists
			image=imlib_load_image(previewfile);
			whichimage=(image?2:0);
		}
		if (which==2 && !image) return NULL;
		if (!image) {
			image=imlib_load_image(filename);
			whichimage=image?1:0;
		}
	} 
	return image;
}

//! Return the buffer from imlib_image_get_data().
unsigned char *LaxImlibImage::getImageBuffer()
{
	if (!image) {
		if (!filename) return NULL;
		image=imlib_load_image(filename);
		if (!image) return NULL;
	}
	imlib_context_set_image(image);
	unsigned char *buffer=(unsigned char *)imlib_image_get_data();
	return buffer;
}

//! Restore data via imlib_image_put_back_data().
/*! WARNING! If image==NULL, then buffer is not put back, and 1 is returned.
 *
 * 0 is returned on success.
 */
int LaxImlibImage::doneWithBuffer(unsigned char *buffer)
{
	if (!image) return 1;
	imlib_context_set_image(image);
	imlib_image_put_back_data((DATA32 *)buffer);
	return 0;
}


//------------------- LaxImlibImage utils --------------------------------

//! Returns LAX_IMAGE_IMLIB.
int laximlib_image_type()
{ return LAX_IMAGE_IMLIB; }

//----------extra utilities

Imlib_Color_Modifier alpha_modifier=NULL;
DATA8 linearmap[256],
	  alphamap[256];
Drawable alternate_drawable=0;
int  lastalpha=-1, 
	 usealpha=0;

//! Override the drawable used for drawing out images
/*! Pass 0 to go back to default behavior.
 */
void laximlib_alternate_drawable(Drawable drawable)
{
	alternate_drawable=drawable;
}

//! Set the imlib color modifier to use transparency.
/*! \ingroup laximages
 *
 * To actually activate or adjust the alpha you must call
 * laximlib_update_alpha().
 */
void laximlib_usealpha(int yes)
{
	usealpha=yes;
	if (yes) laximlib_update_alpha(lastalpha);
	else imlib_context_set_color_modifier(NULL);
}

/*! \ingroup laximages
 * Set alpha_modifier, and the cached alphamap for use later in 
 * image_out() with transparency. If usealpha==0, then the
 * imlib_context_set_color_modifier(NULL) is called, otherwise,
 * the imlib context color modifier is set to use the laximlib alpha modifier.
 */
void laximlib_update_alpha(int alpha)
{
	if (alpha<0) alpha=0;
	if (alpha>255) alpha=255;

	 //init the modifier tables if necessary
	if (alpha_modifier==NULL) {
		alpha_modifier=imlib_create_color_modifier();
		for (int c=0; c<256; c++) {
			alphamap[c]=c*alpha/255;
			linearmap[c]=c;
		}
	}

	 //update the alphamap array if necessary
	if (alpha!=lastalpha) {
		for (int c=0; c<256; c++) alphamap[c]=c*alpha/255;
		lastalpha=alpha;
	}

	imlib_context_set_color_modifier(alpha_modifier);
	imlib_set_color_modifier_tables(linearmap,linearmap,linearmap,alphamap);
	if (!usealpha) imlib_context_set_color_modifier(NULL);
}


 //----------default image function replacements

/*! \ingroup laximages
 * Set image, set drawable, then uses imlib_render_image_on_drawable().
 */
void laximlib_image_out(LaxImage *image, aDrawable *win, int ulx, int uly)
{
	if (image->imagetype()!=LAX_IMAGE_IMLIB) return;

	//if (alpha>=0) laximage_set_alpha(int(alpha*255+.5));

	imlib_context_set_image(static_cast<LaxImlibImage *>(image)->Image());
	imlib_context_set_drawable(alternate_drawable?alternate_drawable:win->xlibDrawable());
	imlib_render_image_on_drawable(ulx,uly);
}

/*! \ingroup laximages
 * Set image, set drawable, then uses imlib_render_image_on_drawable_at_angle().
 */
void laximlib_image_out_rotated(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury)
{
	if (image->imagetype()!=LAX_IMAGE_IMLIB) return;
	imlib_context_set_image(static_cast<LaxImlibImage *>(image)->Image());
	imlib_context_set_drawable(alternate_drawable?alternate_drawable:win->xlibDrawable());
	imlib_render_image_on_drawable_at_angle(0,0, 
								imlib_image_get_width(),imlib_image_get_height(),
								ulx,uly,
								urx,ury);
}

/*! \ingroup laximages
 * Set image, set drawable, then uses imlib_render_image_on_drawable_skewed().
 */
void laximlib_image_out_skewed(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury, int llx, int lly)
{
	if (image->imagetype()!=LAX_IMAGE_IMLIB) return;
	imlib_context_set_image(static_cast<LaxImlibImage *>(image)->Image());
	imlib_context_set_drawable(alternate_drawable?alternate_drawable:win->xlibDrawable());
	imlib_render_image_on_drawable_skewed(0,0, 
								imlib_image_get_width(),imlib_image_get_height(),
								ulx,uly,
								urx,ury,
								llx,lly);
}

/*! \ingroup laximages
 * Set image, set drawable, then uses imlib_render_image_on_drawable_skewed() according to 
 * the affine matrix m.
 */
void laximlib_image_out_matrix(LaxImage *image, aDrawable *win, double *m)
{
	if (image->imagetype()!=LAX_IMAGE_IMLIB) return;

	imlib_context_set_image(static_cast<LaxImlibImage *>(image)->Image());
	imlib_context_set_drawable(alternate_drawable?alternate_drawable:win->xlibDrawable());

	int w,h;
	w=imlib_image_get_width();
	h=imlib_image_get_height();

	flatpoint ul,ur,ll;
	ul=transform_point(m, 0,0);
	ur=transform_point(m, w,0)-ul;
	ll=transform_point(m, 0,h)-ul;

	imlib_render_image_on_drawable_skewed(0,0, 
										w,h,
										(int)ul.x,(int)ul.y,
										(int)ur.x,(int)ur.y,
										(int)ll.x,(int)ll.y);
}

//! Simply return a new imlib image.
/*! Note that this image will not be cached, since it is not associated
 * with a file.
 */
LaxImage *create_new_imlib_image(int w, int h)
{
	Imlib_Image image=imlib_create_image(w,h);
	imlib_context_set_image(image);
	imlib_image_set_has_alpha(1);
	return new LaxImlibImage(NULL,image);
}

//! Basically create a nem Imlib_Image, and copy buffer to its data.
/*! buffer is assumed to be ARGB 8 bit data.
 *
 * Note that this image will not be cached, since it is not associated
 * with a file.
 */
LaxImage *image_from_buffer_imlib(unsigned char *buffer, int w, int h, int stride)
{
	Imlib_Image image=imlib_create_image(w,h);
	imlib_context_set_image(image);
	imlib_image_set_has_alpha(1);
	DATA32 *data=imlib_image_get_data();
	memcpy(data,buffer,w*h*4);
	imlib_image_put_back_data(data);
	LaxImlibImage *img=new LaxImlibImage(NULL,image);
	return img;
}


//! Function that returns a new LaxImlibImage.
/*! \ingroup laximages
 *  This loads the image, grabs the dimensions and does LaxImlibImage::doneForNow().
 *
 * To use a preview image, see _load_imlib_image_with_preview().
 */
LaxImage *load_imlib_image(const char *filename)
{
	if (!filename) return NULL;
	Imlib_Image image;
	image=imlib_load_image(filename);
	if (!image) return NULL;
	LaxImlibImage *img=new LaxImlibImage(filename,image);
	img->doneForNow();
	return img;
}

//! Function that returns a new LaxImlibImage with preview.
/*! \ingroup laximages
 *  This loads the images, grabs the dimensions. If the preview path does not exist,
 *  then a new preview is generated that is within the bounds of maxx and maxy,
 *  and saved to the path if possible. 
 *
 *  If the preview already exists, then use it, without regenerating. If the preview path
 *  is not a valid image, then previewfile is set to NULL, but filename is still used as possible.
 *
 *  LaxImlibImage::doneForNow() is finally called.
 */
LaxImage *load_imlib_image_with_preview(const char *filename,const char *previewfile,
										 int maxx,int maxy,char del)
{
	Imlib_Image image;
	image=imlib_load_image(filename);
	if (!image) return NULL;
	LaxImlibImage *img=new LaxImlibImage(filename,image,previewfile,maxx,maxy,del);
	img->doneForNow();
	return img;
}

//! Generate a preview image. Return 0 for success.
/*! WARNING: this does no sanity checking on file names, and will force an overwrite.
 * It is the responsibility of the calling code to do those things, and to
 * ensure that preview is in fact a writable path.
 * 
 * \todo *** afterwards, make sure preview was actually written
 * \todo if making a freedesktop preview, must add proper tags to the resulting png
 */
int laximlib_generate_preview(const char *original_file,
						   const char *to_preview_file, 
						   const char *format, 
						   int width, int height, int fit)
{
	Imlib_Image image=NULL, pimage=NULL;
	if (!image) image=imlib_load_image(original_file);
	if (!image) return 1;
	imlib_context_set_image(image);
	int owidth= imlib_image_get_width(),
	    oheight=imlib_image_get_height();
	
	if (fit) {
		 //figure out dimensions of new preview
		double a=double(oheight)/owidth;
		int ww=width, hh=height;
		if (a*ww>hh) {
			height=hh;
			width=int(hh/a);
		} else {
			width=ww;
			height=int(ww*a);
		}
		if (height==0) height=1;
		if (width ==0) width =1;
	}

	if (width>0 && height>0)
		pimage=imlib_create_cropped_scaled_image(0,0, owidth,oheight, width,height);
	imlib_free_image();
	image=NULL;
	if (width<=0 || height<=0) return 2;

	imlib_context_set_image(pimage);
	imlib_image_set_format(format); //*** should have option for this
	 //imlib_save_image() is a void f(), which sucks because we 
	 //don't know if it was actually written
	imlib_save_image(to_preview_file);
	imlib_free_image();

	return 0;
}





} //namespace Laxkit


