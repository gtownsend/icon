/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* create.c:                                                                   *
*                                                                             *
*  XPM library                                                                *
*  Create an X image and possibly its related shape mask                     *
*  from the given xpmInternAttrib.                                            *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "xpmP.h"
#ifdef VMS
#include "sys$library:ctype.h"
#else
#include <ctype.h>
#endif

LFUNC(xpmVisualType, int, (Visual *visual));

LFUNC(SetColor, int, (Display * display, Colormap colormap, char *colorname,
		      unsigned int color_index, Pixel * image_pixel,
		      Pixel * mask_pixel, unsigned int * mask_pixel_index,
		      Pixel ** pixels, unsigned int * npixels,
		      XpmAttributes *attributes));

LFUNC(CreateColors, int, (Display *display, XpmAttributes *attributes,
			  char ***ct, unsigned int ncolors, Pixel *ip,
			  Pixel *mp, unsigned int *mask_pixel, Pixel **pixels,
			  unsigned int *npixels));

LFUNC(CreateXImage, int, (Display * display, Visual * visual,
			  unsigned int depth, unsigned int width,
			  unsigned int height, XImage ** image_return));

LFUNC(SetImagePixels, void, (XImage * image, unsigned int width,
			     unsigned int height, unsigned int *pixelindex,
			     Pixel * pixels));

LFUNC(SetImagePixels32, void, (XImage * image, unsigned int width,
			       unsigned int height, unsigned int *pixelindex,
			       Pixel * pixels));

LFUNC(SetImagePixels16, void, (XImage * image, unsigned int width,
			       unsigned int height, unsigned int *pixelindex,
			       Pixel * pixels));

LFUNC(SetImagePixels8, void, (XImage * image, unsigned int width,
			      unsigned int height, unsigned int *pixelindex,
			      Pixel * pixels));

LFUNC(SetImagePixels1, void, (XImage * image, unsigned int width,
			      unsigned int height, unsigned int *pixelindex,
			      Pixel * pixels));

#ifdef NEED_STRCASECMP

LFUNC(strcasecmp, int, (char *s1, char *s2));

/*
 * in case strcasecmp is not provided by the system here is one
 * which does the trick
 */
static int
strcasecmp(s1, s2)
    register char *s1, *s2;
{
    register int c1, c2;

    while (*s1 && *s2) {
	c1 = isupper(*s1) ? tolower(*s1) : *s1;
	c2 = isupper(*s2) ? tolower(*s2) : *s2;
	if (c1 != c2)
	    return (1);
	s1++;
	s2++;
    }
    if (*s1 || *s2)
	return (1);
    return (0);
}

#endif

/*
 * return the default color key related to the given visual 
 */
static int
xpmVisualType(visual)
    Visual *visual;
{
    switch (visual->class) {
    case StaticGray:
    case GrayScale:
	switch (visual->map_entries) {
	case 2:
	    return (MONO);
	case 4:
	    return (GRAY4);
	default:
	    return (GRAY);
	}
    default:
	return (COLOR);
    }
}

/*
 * set the color pixel related to the given colorname,
 * return 0 if success, 1 otherwise.
 */

static int
SetColor(display, colormap, colorname, color_index,
	 image_pixel, mask_pixel, mask_pixel_index,
	 pixels, npixels, attributes)
    Display *display;
    Colormap colormap;
    char *colorname;
    unsigned int color_index;
    Pixel *image_pixel, *mask_pixel;
    unsigned int *mask_pixel_index;
    Pixel **pixels;
    unsigned int *npixels;
    XpmAttributes *attributes;
{
    XColor xcolor;

    if (strcasecmp(colorname, TRANSPARENT_COLOR)) {
	if (!XParseColor(display, colormap, colorname, &xcolor)) return(1);
	else if (!XAllocColor(display, colormap, &xcolor))
	{
	  if (attributes && (attributes->valuemask & XpmCloseness) &&
	      attributes->closeness != 0)
          {
	    XColor *cols;
	    unsigned int ncols,i,closepix;
	    long int closediff,closeness = attributes->closeness;

	    if (attributes && attributes->valuemask & XpmDepth)
	      ncols = 1 << attributes->depth;
	    else
	      ncols = 1 << DefaultDepth(display, DefaultScreen(display));

	    cols = (XColor*)calloc(ncols,sizeof(XColor));
	    for (i = 0; i < ncols; ++i) cols[i].pixel = i;
	    XQueryColors(display,colormap,cols,ncols);

	    for (i = 0, closediff = 0x7FFFFFFF; i < ncols; ++i)
	    {
#define COLOR_FACTOR       3
#define BRIGHTNESS_FACTOR  1

	      long int newclosediff = 
		COLOR_FACTOR * (
		  abs((long)xcolor.red   - (long)cols[i].red)   +
	          abs((long)xcolor.green - (long)cols[i].green) +
	          abs((long)xcolor.blue  - (long)cols[i].blue)) +
		BRIGHTNESS_FACTOR * abs(
		  ((long)xcolor.red+(long)xcolor.green+(long)xcolor.blue) -
		  ((long)cols[i].red+(long)cols[i].green+(long)cols[i].blue));

	      if (newclosediff < closediff)
	      { closepix = i; closediff = newclosediff; }
	    }

	    if ((long)cols[closepix].red   >= (long)xcolor.red   - closeness &&
		(long)cols[closepix].red   <= (long)xcolor.red   + closeness &&
		(long)cols[closepix].green >= (long)xcolor.green - closeness &&
		(long)cols[closepix].green <= (long)xcolor.green + closeness &&
		(long)cols[closepix].blue  >= (long)xcolor.blue  - closeness &&
		(long)cols[closepix].blue  <= (long)xcolor.blue  + closeness)
	    {
	      xcolor = cols[closepix]; free(cols);
	      if (!XAllocColor(display, colormap, &xcolor)) return (1);
	    }
	    else { free(cols); return (1); }
	  }
	  else return (1);
	}
	*image_pixel = xcolor.pixel;
	*mask_pixel = 1;
	(*pixels)[*npixels] = xcolor.pixel;
	(*npixels)++;
    } else {
	*image_pixel = 0;
	*mask_pixel = 0;
	*mask_pixel_index = color_index;/* store the color table index */
    }
    return (0);
}

static int
CreateColors(display, attributes, ct, ncolors,
	     ip, mp, mask_pixel, pixels, npixels)
    Display *display;
    XpmAttributes *attributes;
    char ***ct;
    unsigned int ncolors;
    Pixel *ip;
    Pixel *mp;
    unsigned int *mask_pixel;		/* mask pixel index */
    Pixel **pixels;			/* allocated pixels */
    unsigned int *npixels;		/* number of allocated pixels */
{
    /* variables stored in the XpmAttributes structure */
    Visual *visual;
    Colormap colormap;
    XpmColorSymbol *colorsymbols;
    unsigned int numsymbols;

    char *colorname;
    unsigned int a, b, l;
    Boolean pixel_defined;
    unsigned int key;
    XpmColorSymbol *cs;
    char **cts;
    int ErrorStatus = XpmSuccess;
    char *s;
    int cts_index;

    /*
     * retrieve information from the XpmAttributes 
     */
    if (attributes && attributes->valuemask & XpmColorSymbols) {
	colorsymbols = attributes->colorsymbols;
	numsymbols = attributes->numsymbols;
    } else
	numsymbols = 0;

    if (attributes && attributes->valuemask & XpmVisual)
	visual = attributes->visual;
    else
	visual = DefaultVisual(display, DefaultScreen(display));

    if (attributes && attributes->valuemask & XpmColormap)
	colormap = attributes->colormap;
    else
	colormap = DefaultColormap(display, DefaultScreen(display));

    key = xpmVisualType(visual);
    switch(key)
    {
     case MONO:  cts_index = 2; break;
     case GRAY4: cts_index = 3; break;
     case GRAY:  cts_index = 4; break;
     case COLOR: cts_index = 5; break;
    }

    for (a = 0; a < ncolors; a++, ct++, ip++, mp++) {
	colorname = NULL;
	pixel_defined = False;
	cts = *ct;

	/*
	 * look for a defined symbol 
	 */
	if (numsymbols && cts[1]) {
	    s = cts[1];
	    for (l = 0, cs = colorsymbols; l < numsymbols; l++, cs++)
		if ((!cs->name && cs->value && cts[cts_index] &&
		     !strcasecmp(cs->value,cts[cts_index])) ||
		    cs->name && !strcmp(cs->name, s))
		    break;
	    if (l != numsymbols) {
		if (cs->name && cs->value)
		    colorname = cs->value;
		else
		    pixel_defined = True;
	    }
	}
	if (!pixel_defined) {		/* pixel not given as symbol value */
	    if (colorname) {		/* colorname given as symbol value */
		if (!SetColor(display, colormap, colorname, a, ip, mp,
			      mask_pixel, pixels, npixels, attributes))
		    pixel_defined = True;
		else
		    ErrorStatus = XpmColorError;
	    }
	    b = key;
	    while (!pixel_defined && b > 1) {
		if (cts[b]) {
		    if (!SetColor(display, colormap, cts[b], a, ip, mp,
				  mask_pixel, pixels, npixels, attributes)) {
			pixel_defined = True;
			break;
		    } else
			ErrorStatus = XpmColorError;
		}
		b--;
	    }
	    b = key + 1;
	    while (!pixel_defined && b < NKEYS + 1) {
		if (cts[b]) {
		    if (!SetColor(display, colormap, cts[b], a, ip, mp,
				  mask_pixel, pixels, npixels, attributes)) {
			pixel_defined = True;
			break;
		    } else
			ErrorStatus = XpmColorError;
		}
		b++;
	    }
	    if (!pixel_defined)
		return (XpmColorFailed);
	} else {
	    *ip = colorsymbols[l].pixel;
	    *mp = 1;
	}
    }
    return (ErrorStatus);
}    

/* function call in case of error, frees only locally allocated variables */
#undef RETURN
#ifdef Debug
/*
 * XDestroyImage free the image data but mnemosyne don't know about it
 * so I free them by hand to avoid mnemalyse report it as lost data.
 */
#define RETURN(status) \
    { if (image) { \
	free(image->data); \
	XDestroyImage(image); } \
    if (shapeimage) { \
	free(shapeimage->data); \
	XDestroyImage(shapeimage); } \
    if (image_pixels) free(image_pixels); \
    if (mask_pixels) free(mask_pixels); \
    if (npixels) XFreeColors(display, colormap, pixels, npixels, 0); \
    if (pixels) free(pixels); \
    return (status); }

#else

#define RETURN(status) \
    { if (image) XDestroyImage(image); \
    if (shapeimage) XDestroyImage(shapeimage); \
    if (image_pixels) free(image_pixels); \
    if (mask_pixels) free(mask_pixels); \
    if (npixels) XFreeColors(display, colormap, pixels, npixels, 0); \
    if (pixels) free(pixels); \
    return (status); }

#endif

xpmCreateImage(display, attrib, image_return, shapeimage_return, attributes)
    Display *display;
    xpmInternAttrib *attrib;
    XImage **image_return;
    XImage **shapeimage_return;
    XpmAttributes *attributes;
{
    /* variables stored in the XpmAttributes structure */
    Visual *visual;
    Colormap colormap;
    unsigned int depth;

    /* variables to return */
    XImage *image = NULL;
    XImage *shapeimage = NULL;
    unsigned int mask_pixel;
    int ErrorStatus;

    /* calculation variables */
    Pixel *image_pixels = NULL;
    Pixel *mask_pixels = NULL;
    Pixel *pixels = NULL;		/* allocated pixels */
    unsigned int npixels = 0;		/* number of allocated pixels */

    /*
     * retrieve information from the XpmAttributes 
     */
    if (attributes && attributes->valuemask & XpmVisual)
	visual = attributes->visual;
    else
	visual = DefaultVisual(display, DefaultScreen(display));

    if (attributes && attributes->valuemask & XpmColormap)
	colormap = attributes->colormap;
    else
	colormap = DefaultColormap(display, DefaultScreen(display));

    if (attributes && attributes->valuemask & XpmDepth)
	depth = attributes->depth;
    else
	depth = DefaultDepth(display, DefaultScreen(display));

    ErrorStatus = XpmSuccess;

    /*
     * malloc pixels index tables 
     */

    image_pixels = (Pixel *) malloc(sizeof(Pixel) * attrib->ncolors);
    if (!image_pixels)
	return(XpmNoMemory);

    mask_pixels = (Pixel *) malloc(sizeof(Pixel) * attrib->ncolors);
    if (!mask_pixels)
	RETURN(ErrorStatus);

    mask_pixel = UNDEF_PIXEL;

    /* maximum of allocated pixels will be the number of colors */
    pixels = (Pixel *) malloc(sizeof(Pixel) * attrib->ncolors);
    if (!pixels)
	RETURN(ErrorStatus);

    /*
     * get pixel colors, store them in index tables 
     */

    ErrorStatus = CreateColors(display, attributes, attrib->colorTable,
			       attrib->ncolors, image_pixels, mask_pixels,
			       &mask_pixel, &pixels, &npixels);
    if (ErrorStatus != XpmSuccess && (ErrorStatus < 0 || attributes &&
	(attributes->valuemask & XpmExactColors) && attributes->exactColors))
	RETURN(ErrorStatus);

    /*
     * create the image 
     */
    if (image_return) {
	ErrorStatus = CreateXImage(display, visual, depth,
				    attrib->width, attrib->height, &image);
	if (ErrorStatus != XpmSuccess)
	    RETURN(ErrorStatus);

	/*
	 * set the image data 
	 *
	 * In case depth is 1 or bits_per_pixel is 4, 6, 8, 24 or 32 use
	 * optimized functions, otherwise use slower but sure general one. 
	 *
	 */

	if (image->depth == 1)
	    SetImagePixels1(image, attrib->width, attrib->height,
			    attrib->pixelindex, image_pixels);
	else if (image->bits_per_pixel == 8)
	    SetImagePixels8(image, attrib->width, attrib->height,
			    attrib->pixelindex, image_pixels);
	else if (image->bits_per_pixel == 16)
	    SetImagePixels16(image, attrib->width, attrib->height,
			     attrib->pixelindex, image_pixels);
	else if (image->bits_per_pixel == 32)
	    SetImagePixels32(image, attrib->width, attrib->height,
			     attrib->pixelindex, image_pixels);
	else
	    SetImagePixels(image, attrib->width, attrib->height,
			   attrib->pixelindex, image_pixels);
    }

    /*
     * create the shape mask image 
     */
    if (mask_pixel != UNDEF_PIXEL && shapeimage_return) {
	ErrorStatus = CreateXImage(display, visual, 1, attrib->width,
				    attrib->height, &shapeimage);
	if (ErrorStatus != XpmSuccess)
	    RETURN(ErrorStatus);

	SetImagePixels1(shapeimage, attrib->width, attrib->height,
			attrib->pixelindex, mask_pixels);
    }
    free(mask_pixels);
    free(pixels);

    /*
     * if requested store used pixels in the XpmAttributes structure 
     */
    if (attributes &&
	(attributes->valuemask & XpmReturnInfos
	 || attributes->valuemask & XpmReturnPixels)) {
	if (mask_pixel != UNDEF_PIXEL) {
	    Pixel *pixels, *p1, *p2;
	    unsigned int a;

	    attributes->npixels = attrib->ncolors - 1;
	    pixels = (Pixel *) malloc(sizeof(Pixel) * attributes->npixels);
	    if (pixels) {
		p1 = image_pixels;
		p2 = pixels;
		for (a = 0; a < attrib->ncolors; a++, p1++)
		    if (a != mask_pixel)
			*p2++ = *p1;
		attributes->pixels = pixels;
	    } else {
		/* if error just say we can't return requested data */
		attributes->valuemask &= ~XpmReturnPixels;
		attributes->valuemask &= ~XpmReturnInfos;
		attributes->pixels = NULL;
		attributes->npixels = 0;
	    }
	    free(image_pixels);
	} else {
	    attributes->pixels = image_pixels;
	    attributes->npixels = attrib->ncolors;
	}
	attributes->mask_pixel = mask_pixel;
    } else
	free(image_pixels);


    /*
     * return created images 
     */
    if (image_return)
	*image_return = image;

    if (shapeimage_return)
	*shapeimage_return = shapeimage;

    return (ErrorStatus);
}


/*
 * Create an XImage
 */
static int
CreateXImage(display, visual, depth, width, height, image_return)
    Display *display;
    Visual *visual;
    unsigned int depth;
    unsigned int width;
    unsigned int height;
    XImage **image_return;
{
    int bitmap_pad;

    /* first get bitmap_pad */
    if (depth > 16)
	bitmap_pad = 32;
    else if (depth > 8)
	bitmap_pad = 16;
    else
	bitmap_pad = 8;

    /* then create the XImage with data = NULL and bytes_per_line = 0 */

    *image_return = XCreateImage(display, visual, depth, ZPixmap, 0, 0,
				 width, height, bitmap_pad, 0);
    if (!*image_return)
	return (XpmNoMemory);

    /* now that bytes_per_line must have been set properly alloc data */

    (*image_return)->data =
	(char *) malloc((*image_return)->bytes_per_line * height);

    if (!(*image_return)->data) {
	XDestroyImage(*image_return);
	*image_return = NULL;
	return (XpmNoMemory);
    }
    return (XpmSuccess);
}


/*
 * The functions below are written from X11R5 MIT's code (XImUtil.c)
 *
 * The idea is to have faster functions than the standard XPutPixel function
 * to build the image data. Indeed we can speed up things by suppressing tests
 * performed for each pixel. We do the same tests but at the image level.
 * We also assume that we use only ZPixmap images with null offsets.
 */

LFUNC(_putbits, void, (register char *src, int dstoffset,
		       register int numbits, register char *dst));

LFUNC(_XReverse_Bytes, int, (register unsigned char *bpt, register int nb));

static unsigned char Const _reverse_byte[0x100] = {
			    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
			    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
			    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
			    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
			    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
			    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
			    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
			    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
			    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
			    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
			    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
			    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
			    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
			    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
			    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
			    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
			    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
			    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
			    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
			    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
			    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
			    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
			    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
			    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
			    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
			    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
			    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
			    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
			    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
			    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
			    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
			     0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static int
_XReverse_Bytes(bpt, nb)
    register unsigned char *bpt;
    register int nb;
{
    do {
	*bpt = _reverse_byte[*bpt];
	bpt++;
    } while (--nb > 0);
    return 0;
}


void
xpm_xynormalizeimagebits(bp, img)
    register unsigned char *bp;
    register XImage *img;
{
    register unsigned char c;

    if (img->byte_order != img->bitmap_bit_order) {
	switch (img->bitmap_unit) {

	case 16:
	    c = *bp;
	    *bp = *(bp + 1);
	    *(bp + 1) = c;
	    break;

	case 32:
	    c = *(bp + 3);
	    *(bp + 3) = *bp;
	    *bp = c;
	    c = *(bp + 2);
	    *(bp + 2) = *(bp + 1);
	    *(bp + 1) = c;
	    break;
	}
    }
    if (img->bitmap_bit_order == MSBFirst)
	_XReverse_Bytes(bp, img->bitmap_unit >> 3);
}

void
xpm_znormalizeimagebits(bp, img)
    register unsigned char *bp;
    register XImage *img;
{
    register unsigned char c;

    switch (img->bits_per_pixel) {

    case 4:
	*bp = ((*bp >> 4) & 0xF) | ((*bp << 4) & ~0xF);
	break;

    case 16:
	c = *bp;
	*bp = *(bp + 1);
	*(bp + 1) = c;
	break;

    case 24:
	c = *(bp + 2);
	*(bp + 2) = *bp;
	*bp = c;
	break;

    case 32:
	c = *(bp + 3);
	*(bp + 3) = *bp;
	*bp = c;
	c = *(bp + 2);
	*(bp + 2) = *(bp + 1);
	*(bp + 1) = c;
	break;
    }
}

static unsigned char Const _lomask[0x09] = {
		     0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
static unsigned char Const _himask[0x09] = {
		     0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};

static void
_putbits(src, dstoffset, numbits, dst)
    register char *src;			/* address of source bit string */
    int dstoffset;			/* bit offset into destination;
					 * range is 0-31 */
    register int numbits;		/* number of bits to copy to
					 * destination */
    register char *dst;			/* address of destination bit string */
{
    register unsigned char chlo, chhi;
    int hibits;

    dst = dst + (dstoffset >> 3);
    dstoffset = dstoffset & 7;
    hibits = 8 - dstoffset;
    chlo = *dst & _lomask[dstoffset];
    for (;;) {
	chhi = (*src << dstoffset) & _himask[dstoffset];
	if (numbits <= hibits) {
	    chhi = chhi & _lomask[dstoffset + numbits];
	    *dst = (*dst & _himask[dstoffset + numbits]) | chlo | chhi;
	    break;
	}
	*dst = chhi | chlo;
	dst++;
	numbits = numbits - hibits;
	chlo = (unsigned char) (*src & _himask[hibits]) >> hibits;
	src++;
	if (numbits <= dstoffset) {
	    chlo = chlo & _lomask[numbits];
	    *dst = (*dst & _himask[numbits]) | chlo;
	    break;
	}
	numbits = numbits - dstoffset;
    }
}

/*
 * Default method to write pixels into a Z image data structure.
 * The algorithm used is:
 *
 *	copy the destination bitmap_unit or Zpixel to temp
 *	normalize temp if needed
 *	copy the pixel bits into the temp
 *	renormalize temp if needed
 *	copy the temp back into the destination image data
 */

static void
SetImagePixels(image, width, height, pixelindex, pixels)
    XImage *image;
    unsigned int width;
    unsigned int height;
    unsigned int *pixelindex;
    Pixel *pixels;
{
    register char *src;
    register char *dst;
    register unsigned int *iptr;
    register int x, y, i;
    register char *data;
    Pixel pixel, px;
    int nbytes, depth, ibu, ibpp;

    data = image->data;
    iptr = pixelindex;
    depth = image->depth;
    if (image->depth == 1) {
	ibu = image->bitmap_unit;
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		pixel = pixels[*iptr];
		for (i = 0, px = pixel;
		     i < sizeof(unsigned long); i++, px >>= 8)
		    ((unsigned char *) &pixel)[i] = px;
		src = &data[XYINDEX(x, y, image)];
		dst = (char *) &px;
		px = 0;
		nbytes = ibu >> 3;
		for (i = nbytes; --i >= 0;)
		    *dst++ = *src++;
		XYNORMALIZE(&px, image);
		_putbits((char *) &pixel, (x % ibu), 1, (char *) &px);
		XYNORMALIZE(&px, image);
		src = (char *) &px;
		dst = &data[XYINDEX(x, y, image)];
		for (i = nbytes; --i >= 0;)
		    *dst++ = *src++;
	    }
    } else {
	ibpp = image->bits_per_pixel;
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		pixel = pixels[*iptr];
		if (depth == 4)
		    pixel &= 0xf;
		for (i = 0, px = pixel;
		     i < sizeof(unsigned long); i++, px >>= 8)
		    ((unsigned char *) &pixel)[i] = px;
		src = &data[ZINDEX(x, y, image)];
		dst = (char *) &px;
		px = 0;
		nbytes = (ibpp + 7) >> 3;
		for (i = nbytes; --i >= 0;)
		    *dst++ = *src++;
		ZNORMALIZE(&px, image);
		_putbits((char *) &pixel, (x * ibpp) & 7, ibpp, (char *) &px);
		ZNORMALIZE(&px, image);
		src = (char *) &px;
		dst = &data[ZINDEX(x, y, image)];
		for (i = nbytes; --i >= 0;)
		    *dst++ = *src++;
	    }
    }
}

/*
 * write pixels into a 32-bits Z image data structure
 */

#ifndef WORD64
static unsigned long byteorderpixel = MSBFirst << 24;

#endif

static void
SetImagePixels32(image, width, height, pixelindex, pixels)
    XImage *image;
    unsigned int width;
    unsigned int height;
    unsigned int *pixelindex;
    Pixel *pixels;
{
    register unsigned char *addr;
    register unsigned char *data;
    register unsigned int *iptr;
    register int x, y;
    Pixel pixel;

    data = (unsigned char *) image->data;
    iptr = pixelindex;
#ifndef WORD64
    if (*((char *) &byteorderpixel) == image->byte_order) {
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX32(x, y, image)];
		*((unsigned long *)addr) = pixels[*iptr];
	    }
    } else
#endif
    if (image->byte_order == MSBFirst)
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX32(x, y, image)];
		pixel = pixels[*iptr];
		addr[0] = pixel >> 24;
		addr[1] = pixel >> 16;
		addr[2] = pixel >> 8;
		addr[3] = pixel;
	    }
    else
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX32(x, y, image)];
		pixel = pixels[*iptr];
		addr[0] = pixel;
		addr[1] = pixel >> 8;
		addr[2] = pixel >> 16;
		addr[3] = pixel >> 24;
	    }
}

/*
 * write pixels into a 16-bits Z image data structure
 */

static void
SetImagePixels16(image, width, height, pixelindex, pixels)
    XImage *image;
    unsigned int width;
    unsigned int height;
    unsigned int *pixelindex;
    Pixel *pixels;
{
    register unsigned char *addr;
    register unsigned char *data;
    register unsigned int *iptr;
    register int x, y;

    data = (unsigned char *) image->data;
    iptr = pixelindex;
    if (image->byte_order == MSBFirst)
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX16(x, y, image)];
		addr[0] = pixels[*iptr] >> 8;
		addr[1] = pixels[*iptr];
	    }
    else
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX16(x, y, image)];
		addr[0] = pixels[*iptr];
		addr[1] = pixels[*iptr] >> 8;
	    }
}

/*
 * write pixels into a 8-bits Z image data structure
 */

static void
SetImagePixels8(image, width, height, pixelindex, pixels)
    XImage *image;
    unsigned int width;
    unsigned int height;
    unsigned int *pixelindex;
    Pixel *pixels;
{
    register char *data;
    register unsigned int *iptr;
    register int x, y;

    data = image->data;
    iptr = pixelindex;
    for (y = 0; y < height; y++)
	for (x = 0; x < width; x++, iptr++)
	    data[ZINDEX8(x, y, image)] = pixels[*iptr];
}

/*
 * write pixels into a 1-bit depth image data structure and **offset null**
 */

static void
SetImagePixels1(image, width, height, pixelindex, pixels)
    XImage *image;
    unsigned int width;
    unsigned int height;
    unsigned int *pixelindex;
    Pixel *pixels;
{
    register unsigned int *iptr;
    register int x, y;
    register char *data;

    if (image->byte_order != image->bitmap_bit_order)
	SetImagePixels(image, width, height, pixelindex, pixels);
    else {
	data = image->data;
	iptr = pixelindex;
	if (image->bitmap_bit_order == MSBFirst)
	    for (y = 0; y < height; y++)
		for (x = 0; x < width; x++, iptr++) {
		    if (pixels[*iptr] & 1)
			data[ZINDEX1(x, y, image)] |= 0x80 >> (x & 7);
		    else
			data[ZINDEX1(x, y, image)] &= ~(0x80 >> (x & 7));
		}
	else
	    for (y = 0; y < height; y++)
		for (x = 0; x < width; x++, iptr++) {
		    if (pixels[*iptr] & 1)
			data[ZINDEX1(x, y, image)] |= 1 << (x & 7);
		    else
			data[ZINDEX1(x, y, image)] &= ~(1 << (x & 7));
		}
    }
}
