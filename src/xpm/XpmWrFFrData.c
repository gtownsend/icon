/* Copyright 1990,91 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* XpmWrFFrData.c:                                                             *
*                                                                             *
*  XPM library                                                                *
*  Parse an Xpm array and write a file that corresponds to it.                *
*                                                                             *
*  Developed by Dan Greening dgreen@cs.ucla.edu / dgreen@sti.com              *
\*****************************************************************************/

#include "xpmP.h"
#ifdef VMS
#include "sys$library:string.h"
#else
#ifdef SYSV
#include <string.h>
#define index strchr
#define rindex strrchr
#else
#include <strings.h>
#endif
#endif

int
XpmWriteFileFromData(filename, data)
    char *filename;
    char **data;
{
    xpmData mdata, mfile;
    char *name, *dot, *s, *new_name = NULL;
    int ErrorStatus;
    XpmAttributes attributes;
    xpmInternAttrib attrib;
    int i;

    attributes.valuemask = XpmReturnPixels|XpmReturnInfos|XpmReturnExtensions;
    if ((ErrorStatus = xpmWriteFile(filename, &mfile)) != XpmSuccess)
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
     * Parse data then write it out 
     */

    xpmOpenArray(data, &mdata);

    ErrorStatus = xpmParseData(&mdata, &attrib, &attributes);
    if (ErrorStatus == XpmSuccess) {
	attributes.mask_pixel = UNDEF_PIXEL;

	/* maximum of allocated pixels will be the number of colors */
	attributes.pixels = (Pixel *) malloc(sizeof(Pixel) * attrib.ncolors);
	attrib.xcolors = (XColor*) malloc(sizeof(XColor) * attrib.ncolors);

	if (!attributes.pixels || !attrib.xcolors)
	    ErrorStatus == XpmNoMemory;
	else {
	    int i;

	    for (i = 0; i < attrib.ncolors; i++) {
		/* Fake colors */
		attrib.xcolors[i].pixel = attributes.pixels[i] = i + 1;
	    }
	    xpmSetAttributes(&attrib, &attributes);
	    if (!(attrib.colorStrings =
		  (char**) malloc(attributes.ncolors * sizeof(char*))))
		ErrorStatus == XpmNoMemory;
	    else {
		attrib.ncolors = attributes.ncolors;
		for (i = 0; i < attributes.ncolors; i++)
		    attrib.colorStrings[i] = attributes.colorTable[i][0];

		attrib.name = name;
		ErrorStatus = xpmWriteData(&mfile, &attrib, &attributes);
	    }
	}
    }
    if (new_name)
	free(name);
    XpmFreeAttributes(&attributes);
    xpmFreeInternAttrib(&attrib);
    XpmDataClose(&mfile);
    XpmDataClose(&mdata);

    return (ErrorStatus);
}
