/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* scan.c:                                                                     *
*                                                                             *
*  XPM library                                                                *
*  Scanning utility for XPM file format                                        *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "xpmP.h"

#define MAXPRINTABLE 93			/* number of printable ascii chars
					 * minus \ and " for string compat
					 * and / to avoid comment conflicts. */

static char *printable =
" .XoO+@#$%&*=-;:?>,<1234567890qwertyuipasdfghjklzxcvbnmMNBVCZ\
ASDFGHJKLPIUYTREWQ!~^/()_`'][{}|";

 /*
  * printable begin with a space, so in most case, due to my algorithm, when
  * the number of different colors is less than MAXPRINTABLE, it will give a
  * char follow by "nothing" (a space) in the readable xpm file 
  */


typedef struct {
    Pixel *pixels;
    unsigned int *pixelindex;
    unsigned int size;
    unsigned int ncolors;
    unsigned int mask_pixel;		/* whether there is or not */
}      PixelsMap;

LFUNC(storePixel, int, (Pixel pixel, PixelsMap * pmap,
			unsigned int *index_return));

LFUNC(storeMaskPixel, int, (Pixel pixel, PixelsMap * pmap,
			    unsigned int *index_return));

LFUNC(GetImagePixels, int, (XImage * image, unsigned int width,
			    unsigned int height, PixelsMap * pmap));

LFUNC(GetImagePixels32, int, (XImage * image, unsigned int width,
			      unsigned int height, PixelsMap * pmap));

LFUNC(GetImagePixels16, int, (XImage * image, unsigned int width,
			      unsigned int height, PixelsMap * pmap));

LFUNC(GetImagePixels8, int, (XImage * image, unsigned int width,
			     unsigned int height, PixelsMap * pmap));

LFUNC(GetImagePixels1, int, (XImage * image, unsigned int width,
			     unsigned int height, PixelsMap * pmap,
			     int (*storeFunc) ()));

/*
 * This function stores the given pixel in the given arrays which are grown
 * if not large enough.
 */
static int
storePixel(pixel, pmap, index_return)
    Pixel pixel;
    PixelsMap *pmap;
    unsigned int *index_return;
{
    register unsigned int a;
    register Pixel *p;
    register unsigned int ncolors;

    if (*index_return) {		/* this is a transparent pixel! */
	*index_return = 0;
	return 0;
    }
    ncolors = pmap->ncolors;
    p = &(pmap->pixels[pmap->mask_pixel]);
    for (a = pmap->mask_pixel; a < ncolors; a++, p++)
	if (*p == pixel)
	    break;
    if (a == ncolors) {
	if (ncolors > pmap->size) {

	    pmap->size *= 2;
	    p = (Pixel *) realloc(pmap->pixels, sizeof(Pixel) * pmap->size);
	    if (!p)
		return (1);
	    pmap->pixels = p;

	}
	(pmap->pixels)[ncolors] = pixel;
	pmap->ncolors++;
    }
    *index_return = a;
    return 0;
}

static int
storeMaskPixel(pixel, pmap, index_return)
    Pixel pixel;
    PixelsMap *pmap;
    unsigned int *index_return;
{
    if (!pixel) {
	if (!pmap->ncolors) {
	    pmap->ncolors = 1;
	    (pmap->pixels)[0] = 0;
	    pmap->mask_pixel = 1;
	}
	*index_return = 1;
    } else
	*index_return = 0;
    return 0;
}

/* function call in case of error, frees only locally allocated variables */
#undef RETURN
#define RETURN(status) \
  { if (pmap.pixelindex) free(pmap.pixelindex); \
    if (pmap.pixels) free(pmap.pixels); \
    if (xcolors) free(xcolors); \
    if (colorStrings) { \
	for (a = 0; a < pmap.ncolors; a++) \
	    if (colorStrings[a]) \
		free(colorStrings[a]); \
	free(colorStrings); \
    } \
    return(status); }

/*
 * This function scans the given image and stores the found informations in
 * the xpmInternAttrib structure which is returned.
 */
int
xpmScanImage(display, image, shapeimage, attributes, attrib)
    Display *display;
    XImage *image;
    XImage *shapeimage;
    XpmAttributes *attributes;
    xpmInternAttrib *attrib;

{
    /* variables stored in the XpmAttributes structure */
    Colormap colormap;
    unsigned int cpp;

    /* variables to return */
    PixelsMap pmap;
    char **colorStrings = NULL;
    XColor *xcolors = NULL;
    int ErrorStatus;

    /* calculation variables */
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int cppm;			/* minimum chars per pixel */
    unsigned int a, b, c;
    register char *s;

    /* initialize pmap */
    pmap.pixels = NULL;
    pmap.pixelindex = NULL;
    pmap.size = 256;			/* should be enough most of the time */
    pmap.ncolors = 0;
    pmap.mask_pixel = 0;

    /*
     * get geometry 
     */
    if (image) {
	width = image->width;
	height = image->height;
    } else if (shapeimage) {
	width = shapeimage->width;
	height = shapeimage->height;
    }

    /*
     * retrieve information from the XpmAttributes 
     */
    if (attributes && attributes->valuemask & XpmColormap)
	colormap = attributes->colormap;
    else
	colormap = DefaultColormap(display, DefaultScreen(display));

    if (attributes && (attributes->valuemask & XpmCharsPerPixel
		       || attributes->valuemask & XpmInfos))
	cpp = attributes->cpp;
    else
	cpp = 0;

    pmap.pixelindex =
	(unsigned int *) calloc(width * height, sizeof(unsigned int));
    if (!pmap.pixelindex)
	RETURN(XpmNoMemory);

    pmap.pixels = (Pixel *) malloc(sizeof(Pixel) * pmap.size);
    if (!pmap.pixels)
	RETURN(XpmNoMemory);

    /*
     * scan shape mask if any 
     */
    if (shapeimage) {
	ErrorStatus = GetImagePixels1(shapeimage, width, height, &pmap,
				      storeMaskPixel);
	if (ErrorStatus != XpmSuccess)
	    RETURN(ErrorStatus);
    }

    /*
     * scan the image data 
     *
     * In case depth is 1 or bits_per_pixel is 4, 6, 8, 24 or 32 use optimized
     * functions, otherwise use slower but sure general one. 
     *
     */

    if (image) {
	if (image->depth == 1)
	    ErrorStatus = GetImagePixels1(image, width, height, &pmap,
					  storePixel);
	else if (image->bits_per_pixel == 8)
	    ErrorStatus = GetImagePixels8(image, width, height, &pmap);
	else if (image->bits_per_pixel == 16)
	    ErrorStatus = GetImagePixels16(image, width, height, &pmap);
	else if (image->bits_per_pixel == 32)
	    ErrorStatus = GetImagePixels32(image, width, height, &pmap);
	else
	    ErrorStatus = GetImagePixels(image, width, height, &pmap);

	if (ErrorStatus != XpmSuccess)
	    RETURN(ErrorStatus);
    }

    /*
     * get rgb values and a string of char for each color 
     */

    xcolors = (XColor *) malloc(sizeof(XColor) * pmap.ncolors);
    if (!xcolors)
	RETURN(XpmNoMemory);
    colorStrings = (char **) calloc(pmap.ncolors, sizeof(char *));
    if (!colorStrings)
	RETURN(XpmNoMemory);

    for (cppm = 1, c = MAXPRINTABLE; pmap.ncolors > c; cppm++)
	c *= MAXPRINTABLE;
    if (cpp < cppm)
	cpp = cppm;

    for (a = 0; a < pmap.ncolors; a++) {
	if (!(s = colorStrings[a] = (char *) malloc(cpp)))
	    RETURN(XpmNoMemory);
	*s++ = printable[c = a % MAXPRINTABLE];
	for (b = 1; b < cpp; b++, s++)
	    *s = printable[c = ((a - c) / MAXPRINTABLE) % MAXPRINTABLE];
	xcolors[a].pixel = pmap.pixels[a];
    }

    XQueryColors(display, colormap, xcolors, pmap.ncolors);

    /*
     * store found informations in the xpmInternAttrib structure 
     */
    attrib->width = width;
    attrib->height = height;
    attrib->cpp = cpp;
    attrib->ncolors = pmap.ncolors;
    attrib->mask_pixel = pmap.mask_pixel ? 0 : UNDEF_PIXEL;
    attrib->pixelindex = pmap.pixelindex;
    attrib->xcolors = xcolors;
    attrib->colorStrings = colorStrings;

    free(pmap.pixels);
    return (XpmSuccess);
}



/*
 * The functions below are written from X11R5 MIT's code (XImUtil.c)
 *
 * The idea is to have faster functions than the standard XGetPixel function
 * to scan the image data. Indeed we can speed up things by suppressing tests
 * performed for each pixel. We do exactly the same tests but at the image
 * level. Assuming that we use only ZPixmap images. 
 */

static unsigned long Const low_bits_table[] = {
			    0x00000000, 0x00000001, 0x00000003, 0x00000007,
			    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
			    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
			    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
			    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
			    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
			    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
			    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
					       0xffffffff
};

/*
 * Default method to scan pixels of a Z image data structure.
 * The algorithm used is:
 *
 *	copy the source bitmap_unit or Zpixel into temp
 *	normalize temp if needed
 *	extract the pixel bits into return value
 *
 */

static int
GetImagePixels(image, width, height, pmap)
    XImage *image;
    unsigned int width;
    unsigned int height;
    PixelsMap *pmap;
{
    register char *src;
    register char *dst;
    register unsigned int *iptr;
    register char *data;
    register int x, y, i;
    int bits, depth, ibu, ibpp;
    unsigned long lbt;
    Pixel pixel, px;

    data = image->data;
    iptr = pmap->pixelindex;
    depth = image->depth;
    lbt = low_bits_table[depth];
    ibpp = image->bits_per_pixel;
    if (image->depth == 1) {
	ibu = image->bitmap_unit;
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		src = &data[XYINDEX(x, y, image)];
		dst = (char *) &pixel;
		pixel = 0;
		for (i = ibu >> 3; --i >= 0;)
		    *dst++ = *src++;
		XYNORMALIZE(&pixel, image);
		bits = x % ibu;
		pixel = ((((char *) &pixel)[bits >> 3]) >> (bits & 7)) & 1;
		if (ibpp != depth)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    } else {
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		src = &data[ZINDEX(x, y, image)];
		dst = (char *) &px;
		px = 0;
		for (i = (ibpp + 7) >> 3; --i >= 0;)
		    *dst++ = *src++;
		ZNORMALIZE(&px, image);
		pixel = 0;
		for (i = sizeof(unsigned long); --i >= 0;)
		    pixel = (pixel << 8) | ((unsigned char *) &px)[i];
		if (ibpp == 4) {
		    if (x & 1)
			pixel >>= 4;
		    else
			pixel &= 0xf;
		}
		if (ibpp != depth)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    }
    return(XpmSuccess);
}

/*
 * scan pixels of a 32-bits Z image data structure
 */

#ifndef WORD64
static unsigned long byteorderpixel = MSBFirst << 24;

#endif

static int
GetImagePixels32(image, width, height, pmap)
    XImage *image;
    unsigned int width;
    unsigned int height;
    PixelsMap *pmap;
{
    register unsigned char *addr;
    register unsigned char *data;
    register unsigned int *iptr;
    register int x, y;
    unsigned long lbt;
    Pixel pixel;
    int depth;

    data = (unsigned char *) image->data;
    iptr = pmap->pixelindex;
    depth = image->depth;
    lbt = low_bits_table[depth];
#ifndef WORD64
    if (*((char *) &byteorderpixel) == image->byte_order) {
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX32(x, y, image)];
		pixel = *((unsigned long *)addr);
		if (depth != 32)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    } else
#endif
    if (image->byte_order == MSBFirst)
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX32(x, y, image)];
		pixel = ((unsigned long) addr[0] << 24 |
			 (unsigned long) addr[1] << 16 |
			 (unsigned long) addr[2] << 8 |
			 addr[4]);
		if (depth != 32)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    else
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX32(x, y, image)];
		pixel = (addr[0] |
			 (unsigned long) addr[1] << 8 |
			 (unsigned long) addr[2] << 16 |
			 (unsigned long) addr[3] << 24);
		if (depth != 32)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    return(XpmSuccess);
}

/*
 * scan pixels of a 16-bits Z image data structure
 */

static int
GetImagePixels16(image, width, height, pmap)
    XImage *image;
    unsigned int width;
    unsigned int height;
    PixelsMap *pmap;
{
    register unsigned char *addr;
    register unsigned char *data;
    register unsigned int *iptr;
    register int x, y;
    unsigned long lbt;
    Pixel pixel;
    int depth;

    data = (unsigned char *) image->data;
    iptr = pmap->pixelindex;
    depth = image->depth;
    lbt = low_bits_table[depth];
    if (image->byte_order == MSBFirst)
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX16(x, y, image)];
		pixel = addr[0] << 8 | addr[1];
		if (depth != 16)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    else
	for (y = 0; y < height; y++)
	    for (x = 0; x < width; x++, iptr++) {
		addr = &data[ZINDEX16(x, y, image)];
		pixel = addr[0] | addr[1] << 8;
		if (depth != 16)
		    pixel &= lbt;
		if (storePixel(pixel, pmap, iptr))
		    return (XpmNoMemory);
	    }
    return(XpmSuccess);
}

/*
 * scan pixels of a 8-bits Z image data structure
 */

static int
GetImagePixels8(image, width, height, pmap)
    XImage *image;
    unsigned int width;
    unsigned int height;
    PixelsMap *pmap;
{
    register unsigned int *iptr;
    register unsigned char *data;
    register int x, y;
    unsigned long lbt;
    Pixel pixel;
    int depth;

    data = (unsigned char *) image->data;
    iptr = pmap->pixelindex;
    depth = image->depth;
    lbt = low_bits_table[depth];
    for (y = 0; y < height; y++)
	for (x = 0; x < width; x++, iptr++) {
	    pixel = data[ZINDEX8(x, y, image)];
	    if (depth != 8)
		pixel &= lbt;
	    if (storePixel(pixel, pmap, iptr))
		return (XpmNoMemory);
	}
    return(XpmSuccess);
}

/*
 * scan pixels of a 1-bit depth Z image data structure
 */

static int
GetImagePixels1(image, width, height, pmap, storeFunc)
    XImage *image;
    unsigned int width;
    unsigned int height;
    PixelsMap *pmap;
    int (*storeFunc) ();

{
    register unsigned int *iptr;
    register int x, y;
    register char *data;
    Pixel pixel;

    if (image->byte_order != image->bitmap_bit_order)
	return(GetImagePixels(image, width, height, pmap));
    else {
	data = image->data;
	iptr = pmap->pixelindex;
	if (image->bitmap_bit_order == MSBFirst)
	    for (y = 0; y < height; y++)
		for (x = 0; x < width; x++, iptr++) {
		    pixel = (data[ZINDEX1(x, y, image)] & (0x80 >> (x & 7)))
			? 1 : 0;
		    if ((*storeFunc) (pixel, pmap, iptr))
			return (XpmNoMemory);
		}
	else
	    for (y = 0; y < height; y++)
		for (x = 0; x < width; x++, iptr++) {
		    pixel = (data[ZINDEX1(x, y, image)] & (1 << (x & 7)))
			? 1 : 0;
		    if ((*storeFunc) (pixel, pmap, iptr))
			return (XpmNoMemory);
		}
    }
    return(XpmSuccess);
}
