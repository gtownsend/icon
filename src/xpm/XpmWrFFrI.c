/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* XpmWrFFrI.c:                                                                *
*                                                                             *
*  XPM library                                                                *
*  Write an image and possibly its mask to an XPM file                        *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "xpmP.h"
#ifdef VMS
#include "sys$library:string.h"
#else
#if defined(SYSV) || defined(SVR4)
#include <string.h>
#define index strchr
#define rindex strrchr
#else
#include <strings.h>
#endif
#endif

LFUNC(WriteTransparentColor, void, (FILE *file, char **colors,
				    unsigned int cpp, unsigned int mask_pixel,
				    char ***colorTable));

LFUNC(WriteOtherColors, void, (FILE *file, char **colors, XColor *xcolors,
			       unsigned int ncolors, unsigned int cpp,
			       unsigned int mask_pixel, char ***colorTable,
			       unsigned int ncolors2, Pixel *pixels,
			       char *rgb_fname));

LFUNC(WritePixels, int, (FILE *file, unsigned int width, unsigned int height,
			 unsigned int cpp, unsigned int *pixels,
			 char **colors));

LFUNC(WriteExtensions, void, (FILE *file, XpmExtension *ext,
			      unsigned int num));

int
XpmWriteFileFromImage(display, filename, image, shapeimage, attributes)
    Display *display;
    char *filename;
    XImage *image;
    XImage *shapeimage;
    XpmAttributes *attributes;
{
    xpmData mdata;
    char *name, *dot, *s, *new_name = NULL;
    int ErrorStatus;
    xpmInternAttrib attrib;

    if ((ErrorStatus = xpmWriteFile(filename, &mdata)) != XpmSuccess)
	return (ErrorStatus);

    if (filename) {
#ifdef VMS
	name = filename;
#else
	if (!(name = rindex(filename, '/')))
	    name = filename;
	else
	    name++;
#endif
	if (dot = index(name, '.')) {
	    new_name = (char*)strdup(name);
	    if (!new_name) {
		new_name = NULL;
		name = "image_name";
	    } else {
		/* change '.' to '_' to get a valid C syntax name */
		name = s = new_name;
		while (dot = index(s, '.')) {
		    *dot = '_';
		    s = dot;
		}
	    }
	}
    } else
	name = "image_name";

    xpmInitInternAttrib(&attrib);

    /*
     * Scan image then write it out 
     */
    ErrorStatus = xpmScanImage(display, image, shapeimage,
			       attributes, &attrib);

    if (ErrorStatus == XpmSuccess) {
	attrib.name = name;
	ErrorStatus = xpmWriteData(&mdata, &attrib, attributes);
    }
    xpmFreeInternAttrib(&attrib);
    XpmDataClose(&mdata);
    if (new_name)
	free(name);

    return (ErrorStatus);
}


int
xpmWriteData(mdata, attrib, attributes)
    xpmData *mdata;
    xpmInternAttrib *attrib;
    XpmAttributes *attributes;
{
    /* calculation variables */
    unsigned int offset, infos;
    FILE *file;
    int ErrorStatus;

    /* store this to speed up */
    file = mdata->stream.file;

    infos = attributes && (attributes->valuemask & XpmInfos);

    /*
     * print the header line 
     */
    fprintf(file, "/* XPM */\nstatic char * %s[] = {\n", attrib->name);

    /*
     * print the hints line 
     */
    if (infos && attributes->hints_cmt)
	fprintf(file, "/*%s*/\n", attributes->hints_cmt);

    fprintf(file, "\"%d %d %d %d", attrib->width, attrib->height,
	    attrib->ncolors, attrib->cpp);

    if (attributes && (attributes->valuemask & XpmHotspot))
	fprintf(file, " %d %d", attributes->x_hotspot, attributes->y_hotspot);

    if (attributes && (attributes->valuemask & XpmExtensions)
	&& attributes->nextensions)
	fprintf(file, " XPMEXT");

    fprintf(file, "\",\n");

    /*
     * print colors 
     */
    if (infos && attributes->colors_cmt)
	fprintf(file, "/*%s*/\n", attributes->colors_cmt);

    /* transparent color */
    if (attrib->mask_pixel != UNDEF_PIXEL) {
	WriteTransparentColor(file, attrib->colorStrings, attrib->cpp,
			      (infos ? attributes->mask_pixel : 0),
			      (infos ? attributes->colorTable : NULL));
	offset = 1;
    } else
	offset = 0;

    /* other colors */
    WriteOtherColors(file, attrib->colorStrings + offset,
		     attrib->xcolors + offset, attrib->ncolors - offset,
		     attrib->cpp, (infos ? attributes->mask_pixel : 0),
		     (infos ? attributes->colorTable : NULL),
		     (infos ? attributes->ncolors : 0),
		     (infos ? attributes->pixels : NULL),
		     (attributes && (attributes->valuemask & XpmRgbFilename) ?
		      attributes->rgb_fname : NULL));

    /*
     * print pixels 
     */
    if (infos && attributes->pixels_cmt)
	fprintf(file, "/*%s*/\n", attributes->pixels_cmt);

    ErrorStatus = WritePixels(file, attrib->width, attrib->height, attrib->cpp,
			      attrib->pixelindex, attrib->colorStrings);
    if (ErrorStatus != XpmSuccess)
	return(ErrorStatus);

    /*
     * print extensions
     */
    if (attributes && (attributes->valuemask & XpmExtensions)
	&& attributes->nextensions)
	WriteExtensions(file, attributes->extensions, attributes->nextensions);

    /* close the array */
    fprintf(file, "};\n");

    return (XpmSuccess);
}

static void
WriteTransparentColor(file, colors, cpp, mask_pixel, colorTable)
FILE *file;
char **colors;
unsigned int cpp;
unsigned int mask_pixel;
char ***colorTable;
{
    unsigned int key, i;
    char *s;

    putc('"', file);
    for (i = 0, s = *colors; i < cpp; i++, s++)
	putc(*s, file);

    if (colorTable && mask_pixel != UNDEF_PIXEL) {
	for (key = 1; key <= NKEYS; key++) {
	    if (s = colorTable[mask_pixel][key])
		fprintf(file, "\t%s %s", xpmColorKeys[key - 1], s);
	}
    } else
	fprintf(file, "\tc %s", TRANSPARENT_COLOR);

    fprintf(file, "\",\n");
}

static void
WriteOtherColors(file, colors, xcolors, ncolors, cpp, mask_pixel, colorTable,
		 ncolors2, pixels, rgb_fname)
FILE *file;
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
    unsigned int a, b, c, d, key;
    char *s, *colorname;
    xpmRgbName rgbn[MAX_RGBNAMES];
    int rgbn_max = 0;

    /* read the rgb file if any was specified */
    if (rgb_fname)
	rgbn_max = xpmReadRgbNames(rgb_fname, rgbn);

    for (a = 0; a < ncolors; a++, colors++, xcolors++) {

	putc('"', file);
	for (b = 0, s = *colors; b < cpp; b++, s++)
	    putc(*s, file);

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
		    if (s = colorTable[b][key])
			fprintf(file, "\t%s %s", xpmColorKeys[key - 1], s);
		}
	    }
	}
	if (c) {
	    colorname = NULL;
	    if (rgbn_max)
		colorname = xpmGetRgbName(rgbn, rgbn_max, xcolors->red,
					  xcolors->green, xcolors->blue);
	    if (colorname)
		fprintf(file, "\tc %s", colorname);
	    else
		fprintf(file, "\tc #%04X%04X%04X", xcolors->red,
			xcolors->green, xcolors->blue);
	}
	fprintf(file, "\",\n");
    }
    xpmFreeRgbNames(rgbn, rgbn_max);
}


static int
WritePixels(file, width, height, cpp, pixels, colors)
FILE *file;
unsigned int width;
unsigned int height;
unsigned int cpp;
unsigned int *pixels;
char **colors;
{
    char *s, *p, *buf;
    unsigned int x, y, h;

    h = height - 1;
    p = buf = (char *) malloc(width * cpp + 3);
    *buf = '"';
    if (!buf)
	return(XpmNoMemory);
    p++;
    for (y = 0; y < h; y++) {
	s = p;
	for (x = 0; x < width; x++, pixels++) {
	    strncpy(s, colors[*pixels], cpp);
	    s += cpp;
	}
	*s++ = '"';
	*s = '\0';
	fprintf(file, "%s,\n", buf);
    }
    /* duplicate some code to avoid a test in the loop */
    s = p;
    for (x = 0; x < width; x++, pixels++) {
	strncpy(s, colors[*pixels], cpp);
	s += cpp;
    }
    *s++ = '"';
    *s = '\0';
    fprintf(file, "%s", buf);

    free(buf);
    return(XpmSuccess);
}

static void
WriteExtensions(file, ext, num)
FILE *file;
XpmExtension *ext;
unsigned int num;
{
    unsigned int x, y, n;
    char **line;

    for (x = 0; x < num; x++, ext++) {
	fprintf(file, ",\n\"XPMEXT %s\"", ext->name);
	n = ext->nlines;
	for (y = 0, line = ext->lines; y < n; y++, line++)
	    fprintf(file, ",\n\"%s\"", *line);
    }
    fprintf(file, ",\n\"XPMENDEXT\"");
}
