/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* misc.c:                                                                     *
*                                                                             *
*  XPM library                                                               *
*  Miscellaneous utilities                                                    *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "xpmP.h"

/*
 * Free the computed color table
 */

xpmFreeColorTable(colorTable, ncolors)
    char ***colorTable;
    int ncolors;
{
    int a, b;
    char ***ct, **cts;

    if (colorTable) {
	for (a = 0, ct = colorTable; a < ncolors; a++, ct++)
	    if (*ct) {
		for (b = 0, cts = *ct; b <= NKEYS; b++, cts++)
		    if (*cts)
			free(*cts);
		free(*ct);
	    }
	free(colorTable);
    }
}


/*
 * Intialize the xpmInternAttrib pointers to Null to know
 * which ones must be freed later on.
 */

xpmInitInternAttrib(attrib)
    xpmInternAttrib *attrib;
{
    attrib->ncolors = 0;
    attrib->colorTable = NULL;
    attrib->pixelindex = NULL;
    attrib->xcolors = NULL;
    attrib->colorStrings = NULL;
    attrib->mask_pixel = UNDEF_PIXEL;
}


/*
 * Free the xpmInternAttrib pointers which have been allocated
 */

xpmFreeInternAttrib(attrib)
    xpmInternAttrib *attrib;
{
    unsigned int a, ncolors;
    char **sptr;

    if (attrib->colorTable)
	xpmFreeColorTable(attrib->colorTable, attrib->ncolors);
    if (attrib->pixelindex)
	free(attrib->pixelindex);
    if (attrib->xcolors)
	free(attrib->xcolors);
    if (attrib->colorStrings) {
	ncolors = attrib->ncolors;
	for (a = 0, sptr = attrib->colorStrings; a < ncolors; a++, sptr++)
	    if (*sptr)
		free(*sptr);
	free(attrib->colorStrings);
    }
}


/*
 * Free array of extensions
 */
XpmFreeExtensions(extensions, nextensions)
    XpmExtension *extensions;
    int          nextensions;
{
    unsigned int i, j, nlines;
    XpmExtension *ext;
    char **sptr;

    if (extensions) {
	for (i = 0, ext = extensions; i < nextensions; i++, ext++) {
	    if (ext->name)
		free(ext->name);
	    nlines = ext->nlines;
	    for (j = 0, sptr = ext->lines; j < nlines; j++, sptr++)
		if (*sptr)
		    free(*sptr);
	    if (ext->lines)
		free(ext->lines);
	}
	free(extensions);
    }
}


/*
 * Return the XpmAttributes structure size
 */

XpmAttributesSize()
{
    return sizeof(XpmAttributes);
}


/*
 * Free the XpmAttributes structure members
 * but the structure itself
 */

XpmFreeAttributes(attributes)
    XpmAttributes *attributes;
{
    if (attributes) {
	if (attributes->valuemask & XpmReturnPixels && attributes->pixels) {
	    free(attributes->pixels);
	    attributes->pixels = NULL;
	    attributes->npixels = 0;
	}
	if (attributes->valuemask & XpmInfos) {
	    if (attributes->colorTable) {
		xpmFreeColorTable(attributes->colorTable, attributes->ncolors);
		attributes->colorTable = NULL;
		attributes->ncolors = 0;
	    }
	    if (attributes->hints_cmt) {
		free(attributes->hints_cmt);
		attributes->hints_cmt = NULL;
	    }
	    if (attributes->colors_cmt) {
		free(attributes->colors_cmt);
		attributes->colors_cmt = NULL;
	    }
	    if (attributes->pixels_cmt) {
		free(attributes->pixels_cmt);
		attributes->pixels_cmt = NULL;
	    }
	    if (attributes->pixels) {
		free(attributes->pixels);
		attributes->pixels = NULL;
	    }
	}
	if (attributes->valuemask & XpmReturnExtensions
	    && attributes->nextensions) {
            XpmFreeExtensions(attributes->extensions, attributes->nextensions);
	    attributes->nextensions = 0;
	    attributes->extensions = NULL;
	}
	attributes->valuemask = 0;
    }
}


/*
 * Store into the XpmAttributes structure the required informations stored in
 * the xpmInternAttrib structure.
 */

xpmSetAttributes(attrib, attributes)
    xpmInternAttrib *attrib;
    XpmAttributes *attributes;
{
    if (attributes) {
	if (attributes->valuemask & XpmReturnInfos) {
	    attributes->cpp = attrib->cpp;
	    attributes->ncolors = attrib->ncolors;
	    attributes->colorTable = attrib->colorTable;

	    attrib->ncolors = 0;
	    attrib->colorTable = NULL;
	}
	attributes->width = attrib->width;
	attributes->height = attrib->height;
	attributes->valuemask |= XpmSize;
    }
}

#ifdef NEED_STRDUP

/*
 * in case strdup is not provided by the system here is one
 * which does the trick
 */
char *
strdup (s1)
    char *s1;
{
    char *s2;
    int l = strlen(s1) + 1;
    if (s2 = (char *) malloc(l))
	strncpy(s2, s1, l);
    return s2;
}

#endif
