/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* xpmP.h:                                                                     *
*                                                                             *
*  XPM library                                                                *
*  Private Include file                                                       *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#ifndef XPMP_h
#define XPMP_h

#ifdef Debug
/* memory leak control tool */
#include <mnemosyne.h>
#endif

#ifdef VMS
#include "decw$include:Xlib.h"
#include "decw$include:Intrinsic.h"
#include "sys$library:stdio.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
/* stdio.h doesn't declare popen on a Sequent DYNIX OS */
#ifdef sequent
extern FILE *popen();
#endif
#endif

#include "xpm.h"

/* we keep the same codes as for Bitmap management */
#ifndef _XUTIL_H_
#ifdef VMS
#include "decw$include:Xutil.h"
#else
#include <X11/Xutil.h>
#endif
#endif

#if defined(SYSV) || defined(SVR4)
#define bcopy(source, dest, count) memcpy(dest, source, count)
#define bzero(addr, count) memset(addr, 0, count)
#endif

typedef struct {
    unsigned int type;
    union {
	FILE *file;
	char **data;
    }     stream;
    char *cptr;
    unsigned int line;
    int CommentLength;
    char Comment[BUFSIZ];
    char *Bcmt, *Ecmt, Bos, Eos;
}      xpmData;

#define XPMARRAY 0
#define XPMFILE  1
#define XPMPIPE  2

typedef unsigned char byte;

#define EOL '\n'
#define TAB '\t'
#define SPC ' '

typedef struct {
    char *type;				/* key word */
    char *Bcmt;				/* string beginning comments */
    char *Ecmt;				/* string ending comments */
    char Bos;				/* character beginning strings */
    char Eos;				/* character ending strings */
    char *Strs;				/* strings separator */
    char *Dec;				/* data declaration string */
    char *Boa;				/* string beginning assignment */
    char *Eoa;				/* string ending assignment */
}      xpmDataType;

extern xpmDataType xpmDataTypes[];

/*
 * rgb values and ascii names (from rgb text file) rgb values,
 * range of 0 -> 65535 color mnemonic of rgb value
 */
typedef struct {
    int r, g, b;
    char *name;
}      xpmRgbName;

/* Maximum number of rgb mnemonics allowed in rgb text file. */
#define MAX_RGBNAMES 1024

extern char *xpmColorKeys[];

#define TRANSPARENT_COLOR "None"	/* this must be a string! */

/* number of xpmColorKeys */
#define NKEYS 5

/*
 * key numbers for visual type, they must fit along with the number key of
 * each corresponding element in xpmColorKeys[] defined in xpm.h
 */
#define MONO	2
#define GRAY4	3
#define GRAY 	4
#define COLOR	5

/* structure containing data related to an Xpm pixmap */
typedef struct {
    char *name;
    unsigned int width;
    unsigned int height;
    unsigned int cpp;
    unsigned int ncolors;
    char ***colorTable;
    unsigned int *pixelindex;
    XColor *xcolors;
    char **colorStrings;
    unsigned int mask_pixel;		/* mask pixel's colorTable index */
}      xpmInternAttrib;

#define UNDEF_PIXEL 0x80000000

/* XPM private routines */

FUNC(xpmWriteData, int, (xpmData * mdata,
		    xpmInternAttrib * attrib, XpmAttributes * attributes));

FUNC(xpmCreateData, int, (char ***data_return,
		    xpmInternAttrib * attrib, XpmAttributes * attributes));

FUNC(xpmParseDataAndCreateImage, int, (xpmData * data,
				       Display * display,
				       XImage ** image_return,
				       XImage ** shapeimage_return,
				       xpmInternAttrib * attrib_return,
				       XpmAttributes * attributes));

FUNC(xpmCreateImage, int, (Display * display,
			   xpmInternAttrib * attrib,
			   XImage ** image_return,
			   XImage ** shapeimage_return,
			   XpmAttributes * attributes));

FUNC(xpmParseData, int, (xpmData * data,
			 xpmInternAttrib * attrib_return,
			 XpmAttributes * attributes));

FUNC(xpmScanImage, int, (Display * display,
			 XImage * image,
			 XImage * shapeimage,
			 XpmAttributes * attributes,
			 xpmInternAttrib * attrib));

FUNC(xpmFreeColorTable, int, (char ***colorTable, int ncolors));

FUNC(xpmInitInternAttrib, int, (xpmInternAttrib * xmpdata));

FUNC(xpmFreeInternAttrib, int, (xpmInternAttrib * xmpdata));

FUNC(xpmSetAttributes, int, (xpmInternAttrib * attrib,
			     XpmAttributes * attributes));

FUNC(xpmGetAttributes, int, (XpmAttributes * attributes,
			     xpmInternAttrib * attrib));

/* I/O utility */

FUNC(xpmNextString, int, (xpmData * mdata));
FUNC(xpmNextUI, int, (xpmData * mdata, unsigned int *ui_return));

#define xpmGetC(mdata) \
	(mdata->type ? (getc(mdata->stream.file)) : (*mdata->cptr++))

FUNC(xpmNextWord, unsigned int, (xpmData * mdata, char *buf));
FUNC(xpmGetCmt, int, (xpmData * mdata, char **cmt));
FUNC(xpmReadFile, int, (char *filename, xpmData * mdata));
FUNC(xpmWriteFile, int, (char *filename, xpmData * mdata));
FUNC(xpmOpenArray, void, (char **data, xpmData * mdata));
FUNC(XpmDataClose, int, (xpmData * mdata));

/* RGB utility */

FUNC(xpmReadRgbNames, int, (char *rgb_fname, xpmRgbName * rgbn));
FUNC(xpmGetRgbName, char *, (xpmRgbName * rgbn, int rgbn_max,
			     int red, int green, int blue));
FUNC(xpmFreeRgbNames, void, (xpmRgbName * rgbn, int rgbn_max));

FUNC(xpm_xynormalizeimagebits, void, (register unsigned char *bp,
				      register XImage * img));
FUNC(xpm_znormalizeimagebits, void, (register unsigned char *bp,
				     register XImage * img));

/*
 * Macros
 * 
 * The XYNORMALIZE macro determines whether XY format data requires 
 * normalization and calls a routine to do so if needed. The logic in
 * this module is designed for LSBFirst byte and bit order, so 
 * normalization is done as required to present the data in this order.
 *
 * The ZNORMALIZE macro performs byte and nibble order normalization if 
 * required for Z format data.
 * 
 * The XYINDEX macro computes the index to the starting byte (char) boundary
 * for a bitmap_unit containing a pixel with coordinates x and y for image
 * data in XY format.
 * 
 * The ZINDEX* macros compute the index to the starting byte (char) boundary 
 * for a pixel with coordinates x and y for image data in ZPixmap format.
 * 
 */

#define XYNORMALIZE(bp, img) \
    if ((img->byte_order == MSBFirst) || (img->bitmap_bit_order == MSBFirst)) \
	xpm_xynormalizeimagebits((unsigned char *)(bp), img)

#define ZNORMALIZE(bp, img) \
    if (img->byte_order == MSBFirst) \
	xpm_znormalizeimagebits((unsigned char *)(bp), img)

#define XYINDEX(x, y, img) \
    ((y) * img->bytes_per_line) + \
    (((x) + img->xoffset) / img->bitmap_unit) * (img->bitmap_unit >> 3)

#define ZINDEX(x, y, img) ((y) * img->bytes_per_line) + \
    (((x) * img->bits_per_pixel) >> 3)

#define ZINDEX32(x, y, img) ((y) * img->bytes_per_line) + ((x) << 2)

#define ZINDEX16(x, y, img) ((y) * img->bytes_per_line) + ((x) << 1)

#define ZINDEX8(x, y, img) ((y) * img->bytes_per_line) + (x)

#define ZINDEX1(x, y, img) ((y) * img->bytes_per_line) + ((x) >> 3)

#if __STDC__
#define Const const
#else
#define Const				/**/
#endif

/*
 * there are structures and functions related to hastable code
 */

typedef struct _xpmHashAtom {
    char *name;
    void *data;
}      *xpmHashAtom;

typedef struct {
    int size;
    int limit;
    int used;
    xpmHashAtom *atomTable;
} xpmHashTable;

FUNC(xpmHashTableInit, int, (xpmHashTable *table));
FUNC(xpmHashTableFree, void, (xpmHashTable *table));
FUNC(xpmHashSlot, xpmHashAtom *, (xpmHashTable *table, char *s));
FUNC(xpmHashIntern, int, (xpmHashTable *table, char *tag, void *data));

#define HashAtomData(i) ((void *)i)
#define HashColorIndex(slot) ((unsigned int)(unsigned long)((*slot)->data))
#define USE_HASHTABLE (cpp > 2 && ncolors > 4)

#ifdef NEED_STRDUP
FUNC(strdup, char *, (char *s1));
#endif

#endif
