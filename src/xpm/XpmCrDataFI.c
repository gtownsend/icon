/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* XpmCrDataFI.c:                                                              *
*                                                                             *
*  XPM library                                                                *
*  Scan an image and possibly its mask and create an XPM array                *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "xpmP.h"
#ifdef VMS
#include "sys$library:string.h"
#else
#if defined(SYSV) || defined(SVR4)
#include <string.h>
#else
#include <strings.h>
#endif
#endif

LFUNC(CreateTransparentColor, int, (char **dataptr, unsigned int *data_size,
				    char **colors, unsigned int cpp,
				    unsigned int mask_pixel,
				    char ***colorTable));

LFUNC(CreateOtherColors, int, (char **dataptr, unsigned int *data_size,
			       char **colors, XColor *xcolors,
			       unsigned int ncolors, unsigned int cpp,
			       unsigned int mask_pixel, char ***colorTable,
			       unsigned int ncolors2, Pixel *pixels,
			       char *rgb_fname));

LFUNC(CreatePixels, void, (char **dataptr, unsigned int width,
			   unsigned int height, unsigned int cpp,
			   unsigned int *pixels, char **colors));

LFUNC(CountExtensions, void, (XpmExtension *ext, unsigned int num,
			      unsigned int *ext_size,
			      unsigned int *ext_nlines));

LFUNC(CreateExtensions, void, (char **dataptr, unsigned int offset,
			       XpmExtension *ext, unsigned int num,
			       unsigned int ext_nlines));

int
XpmCreateDataFromImage(display, data_return, image, shapeimage, attributes)
    Display *display;
    char ***data_return;
    XImage *image;
    XImage *shapeimage;
    XpmAttributes *attributes;
{
    int ErrorStatus;
    xpmInternAttrib attrib;

    /*
     * initialize return values 
     */
    if (data_return)
	*data_return = NULL;

    xpmInitInternAttrib(&attrib);

    /*
     * Scan image then create data 
     */
    ErrorStatus = xpmScanImage(display, image, shapeimage,
			       attributes, &attrib);

    if (ErrorStatus == XpmSuccess)
	ErrorStatus = xpmCreateData(data_return, &attrib, attributes);

    xpmFreeInternAttrib(&attrib);

    return (ErrorStatus);
}


#undef RETURN
#define RETURN(status) \
  { if (header) { \
        for (l = 0; l < header_nlines; l++) \
	    if (header[l]) \
		free(header[l]); \
	free(header); \
    } \
    return(status); }

int
xpmCreateData(data_return, attrib, attributes)
    char ***data_return;
    xpmInternAttrib *attrib;
    XpmAttributes *attributes;
{
    /* calculation variables */
    int ErrorStatus;
    char buf[BUFSIZ];
    char **header = NULL, **data, **sptr, **sptr2, *s;
    unsigned int header_size, header_nlines;
    unsigned int data_size, data_nlines;
    unsigned int extensions = 0, ext_size = 0, ext_nlines = 0;
    unsigned int infos = 0, offset, l, n;

    *data_return = NULL;

    infos = attributes && (attributes->valuemask & XpmInfos);
    extensions = attributes && (attributes->valuemask & XpmExtensions)
	&& attributes->nextensions;

    /* compute the number of extensions lines and size */
    if (extensions)
	CountExtensions(attributes->extensions, attributes->nextensions,
			&ext_size, &ext_nlines);

    /*
     * alloc a temporary array of char pointer for the header section which
     * is the hints line + the color table lines 
     */
    header_nlines = 1 + attrib->ncolors;
    header_size = sizeof(char *) * header_nlines;
    header = (char **) calloc(header_size, sizeof(char *));
    if (!header)
	RETURN(XpmNoMemory);

    /*
     * print the hints line 
     */
    s = buf;
    sprintf(s, "%d %d %d %d", attrib->width, attrib->height,
	    attrib->ncolors, attrib->cpp);
    s += strlen(s);

    if (attributes && (attributes->valuemask & XpmHotspot)) {
	sprintf(s, " %d %d", attributes->x_hotspot, attributes->y_hotspot);
	s += strlen(s);
    }

    if (extensions)
	sprintf(s, " XPMEXT");

    l = strlen(buf) + 1;
    *header = (char *) malloc(l);
    if (!*header)
	RETURN(XpmNoMemory);
    header_size += l;
    strcpy(*header, buf);

    /*
     * print colors 
     */

    /* transparent color */
    if (attrib->mask_pixel != UNDEF_PIXEL) {
	ErrorStatus =
	    CreateTransparentColor(header + 1, &header_size,
				   attrib->colorStrings, attrib->cpp,
				   (infos ? attributes->mask_pixel : 0),
				   (infos ? attributes->colorTable : NULL));
	if (ErrorStatus != XpmSuccess)
	    RETURN(ErrorStatus);

	offset = 1;
    } else
	offset = 0;

    /* other colors */
    ErrorStatus =
	CreateOtherColors(header + 1 + offset, &header_size,
			  attrib->colorStrings + offset,
			  attrib->xcolors + offset, attrib->ncolors - offset,
			  attrib->cpp, (infos ? attributes->mask_pixel : 0),
			  (infos ? attributes->colorTable : NULL),
			  (infos ? attributes->ncolors : 0),
			  (infos ? attributes->pixels : NULL),
			  (attributes &&
			   (attributes->valuemask & XpmRgbFilename) ?
			   attributes->rgb_fname : NULL));
    if (ErrorStatus != XpmSuccess)
	RETURN(ErrorStatus);

    /*
     * now we know the size needed, alloc the data and copy the header lines 
     */
    offset = attrib->width * attrib->cpp + 1;
    data_size = header_size + (attrib->height + ext_nlines) * sizeof(char *)
	+ attrib->height * offset + ext_size;

    data = (char **) malloc(data_size);
    if (!data)
	RETURN(XpmNoMemory);

    data_nlines = header_nlines + attrib->height + ext_nlines;
    *data = (char *) (data + data_nlines);
    n = attrib->ncolors;
    for (l = 0, sptr = data, sptr2 = header; l <= n; l++, sptr++, sptr2++) {
	strcpy(*sptr, *sptr2);
	*(sptr + 1) = *sptr + strlen(*sptr2) + 1;
    }

    /*
     * print pixels 
     */
    data[header_nlines] = (char *) data + header_size +
	(attrib->height + ext_nlines) * sizeof(char *);

    CreatePixels(data + header_nlines, attrib->width, attrib->height,
		 attrib->cpp, attrib->pixelindex, attrib->colorStrings);

    /*
     * print extensions
     */
    if (extensions)
	CreateExtensions(data + header_nlines + attrib->height - 1, offset,
			 attributes->extensions, attributes->nextensions,
			 ext_nlines);

    *data_return = data;

    RETURN(XpmSuccess);
}


static int
CreateTransparentColor(dataptr, data_size, colors, cpp, mask_pixel, colorTable)
char **dataptr;
unsigned int *data_size;
char **colors;
unsigned int cpp;
unsigned int mask_pixel;
char ***colorTable;
{
    char buf[BUFSIZ];
    unsigned int key, l;
    char *s, *s2;

    strncpy(buf, *colors, cpp);
    s = buf + cpp;

    if (colorTable && mask_pixel != UNDEF_PIXEL) {
	for (key = 1; key <= NKEYS; key++) {
	    if (s2 = colorTable[mask_pixel][key]) {
		sprintf(s, "\t%s %s", xpmColorKeys[key - 1], s2);
		s += strlen(s);
	    }
	}
    } else
	sprintf(s, "\tc %s", TRANSPARENT_COLOR);

    l = strlen(buf) + 1;
    s = (char *) malloc(l);
    if (!s)
	return(XpmNoMemory);
    *data_size += l;
    strcpy(s, buf);
    *dataptr = s;
    return(XpmSuccess);
}

static int
CreateOtherColors(dataptr, data_size, colors, xcolors, ncolors, cpp,
		  mask_pixel, colorTable, ncolors2, pixels, rgb_fname)
char **dataptr;
unsigned int *data_size;
char **colors;
XColor *xcolors;
unsigned int ncolors;
unsigned int cpp;
unsigned int mask_pixel;
char ***colorTable;
unsigned int ncolors2;
Pixel *pixels;
char *rgb_fname;
{
    char buf[BUFSIZ];
    unsigned int a, b, c, d, key, l;
    char *s, *s2, *colorname;
    xpmRgbName rgbn[MAX_RGBNAMES];
    int rgbn_max = 0;

    /* read the rgb file if any was specified */
    if (rgb_fname)
	rgbn_max = xpmReadRgbNames(rgb_fname, rgbn);

    for (a = 0; a < ncolors; a++, colors++, xcolors++, dataptr++) {

	strncpy(buf, *colors, cpp);
	s = buf + cpp;

	c = 1;
	if (colorTable) {
	    d = 0;
	    for (b = 0; b < ncolors2; b++) {
		if (b == mask_pixel) {
		    d = 1;
		    continue;
		}
		if (pixels[b - d] == xcolors->pixel)
		    break;
	    }
	    if (b != ncolors2) {
		c = 0;
		for (key = 1; key <= NKEYS; key++) {
		    if (s2 = colorTable[b][key]) {
			sprintf(s, "\t%s %s", xpmColorKeys[key - 1], s2);
			s += strlen(s);
		    }
		}
	    }
	}
	if (c) {
	    colorname = NULL;
	    if (rgbn_max)
		colorname = xpmGetRgbName(rgbn, rgbn_max, xcolors->red,
					  xcolors->green, xcolors->blue);
	    if (colorname)
		sprintf(s, "\tc %s", colorname);
	    else
		sprintf(s, "\tc #%04X%04X%04X",
			xcolors->red, xcolors->green, xcolors->blue);
	    s += strlen(s);
	}
	l = strlen(buf) + 1;
	s = (char *) malloc(l);
	if (!s)
	    return(XpmNoMemory);
	*data_size += l;
	strcpy(s, buf);
	*dataptr = s;
    }
    xpmFreeRgbNames(rgbn, rgbn_max);
    return(XpmSuccess);
}

static void
CreatePixels(dataptr, width, height, cpp, pixels, colors)
char **dataptr;
unsigned int width;
unsigned int height;
unsigned int cpp;
unsigned int *pixels;
char **colors;
{
    char *s;
    unsigned int x, y, h, offset;

    h = height - 1;
    offset = width * cpp + 1;
    for (y = 0; /* test is inside loop */ ; y++, dataptr++) {
	s = *dataptr;
	for (x = 0; x < width; x++, pixels++) {
	    strncpy(s, colors[*pixels], cpp);
	    s += cpp;
	}
	*s = '\0';
	if (y >= h)
	    break;		/* LEAVE LOOP */
	*(dataptr + 1) = *dataptr + offset;
    }
}

static void
CountExtensions(ext, num, ext_size, ext_nlines)
XpmExtension *ext;
unsigned int num;
unsigned int *ext_size;
unsigned int *ext_nlines;
{
    unsigned int x, y, a, size, nlines;
    char **lines;

    size = 0;
    nlines = 0;
    for (x = 0; x < num; x++, ext++) {
	/* "+ 2" is for the name and the ending 0 */
	nlines += ext->nlines + 2;
	/* 8 = 7 (for "XPMEXT ") + 1 (for 0) */
	size += strlen(ext->name) + 8;
	a = ext->nlines;
	for (y = 0, lines = ext->lines; y < a; y++, lines++)
	    size += strlen(*lines) + 1;
    }
    *ext_size = size;
    *ext_nlines = nlines;
}

static void
CreateExtensions(dataptr, offset, ext, num, ext_nlines)
char **dataptr;
unsigned int offset;
XpmExtension *ext;
unsigned int num;
unsigned int ext_nlines;
{
    unsigned int x, y, a, b;
    char **sptr;

    *(dataptr + 1) = *dataptr + offset;
    dataptr++;
    a = 0;
    for (x = 0; x < num; x++, ext++) {
	sprintf(*dataptr, "XPMEXT %s", ext->name);
	a++;
	if (a < ext_nlines)
	    *(dataptr + 1) = *dataptr + strlen(ext->name) + 8;
	dataptr++;
	b = ext->nlines;
	for (y = 0, sptr = ext->lines; y < b; y++, sptr++) {
	    strcpy(*dataptr, *sptr);
	    a++;
	    if (a < ext_nlines)
		*(dataptr + 1) = *dataptr + strlen(*sptr) + 1;
	    dataptr++;
	}
    }
    *dataptr = 0;
}
