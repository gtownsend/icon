/* Copyright 1990,91 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* XpmRdFToData.c:                                                             *
*                                                                             *
*  XPM library                                                                *
*  Parse an XPM file and create an array of strings corresponding to it.      *
*                                                                             *
*  Developed by Dan Greening dgreen@cs.ucla.edu / dgreen@sti.com              *
\*****************************************************************************/

#include "xpmP.h"

int
XpmReadFileToData(filename, data_return)
    char *filename;
    char ***data_return;
{
    xpmData mdata;
    char buf[BUFSIZ];
    int l, n = 0;
    XpmAttributes attributes;
    xpmInternAttrib attrib;
    int ErrorStatus;
    XGCValues gcv;
    GC gc;

    attributes.valuemask = XpmReturnPixels|XpmReturnInfos|XpmReturnExtensions;
    /*
     * initialize return values 
     */
    if (data_return) {
	*data_return = NULL;
    }

    if ((ErrorStatus = xpmReadFile(filename, &mdata)) != XpmSuccess)
	return (ErrorStatus);
    xpmInitInternAttrib(&attrib);
    /*
     * parse the header file 
     */
    mdata.Bos = '\0';
    mdata.Eos = '\n';
    mdata.Bcmt = mdata.Ecmt = NULL;
    xpmNextWord(&mdata, buf);		/* skip the first word */
    l = xpmNextWord(&mdata, buf);	/* then get the second word */
    if ((l == 3 && !strncmp("XPM", buf, 3)) ||
	(l == 4 && !strncmp("XPM2", buf, 4))) {
	if (l == 3)
	    n = 1;			/* handle XPM as XPM2 C */
	else {
	    l = xpmNextWord(&mdata, buf); /* get the type key word */

	    /*
	     * get infos about this type 
	     */
	    while (xpmDataTypes[n].type
		   && strncmp(xpmDataTypes[n].type, buf, l))
		n++;
	}
	if (xpmDataTypes[n].type) {
	    if (n == 0) {		/* natural type */
		mdata.Bcmt = xpmDataTypes[n].Bcmt;
		mdata.Ecmt = xpmDataTypes[n].Ecmt;
		xpmNextString(&mdata);	/* skip the end of headerline */
		mdata.Bos = xpmDataTypes[n].Bos;
	    } else {
		xpmNextString(&mdata);	/* skip the end of headerline */
		mdata.Bcmt = xpmDataTypes[n].Bcmt;
		mdata.Ecmt = xpmDataTypes[n].Ecmt;
		mdata.Bos = xpmDataTypes[n].Bos;
		mdata.Eos = '\0';
		xpmNextString(&mdata);	/* skip the assignment line */
	    }
	    mdata.Eos = xpmDataTypes[n].Eos;

	    ErrorStatus = xpmParseData(&mdata, &attrib, &attributes);
	} else
	    ErrorStatus = XpmFileInvalid;
    } else
	ErrorStatus = XpmFileInvalid;

    if (ErrorStatus == XpmSuccess) {
	int i;
	
	/* maximum of allocated pixels will be the number of colors */
	attributes.pixels = (Pixel *) malloc(sizeof(Pixel) * attrib.ncolors);
	attrib.xcolors = (XColor*) malloc(sizeof(XColor) * attrib.ncolors);

	if (!attributes.pixels || !attrib.xcolors)
	    ErrorStatus = XpmNoMemory;
	else {
	    for (i = 0; i < attrib.ncolors; i++) {
		/* Fake colors */
		attrib.xcolors[i].pixel = attributes.pixels[i] = i + 1;
	    }
	    xpmSetAttributes(&attrib, &attributes);
	    if (!(attrib.colorStrings =
		  (char**) malloc(attributes.ncolors * sizeof(char*))))
		ErrorStatus = XpmNoMemory;
	    else {
		attrib.ncolors = attributes.ncolors;
		attributes.mask_pixel = attrib.mask_pixel;
		for (i = 0; i < attributes.ncolors; i++)
		    attrib.colorStrings[i] = attributes.colorTable[i][0];
	    }
	}
    }
    if (ErrorStatus == XpmSuccess)
	ErrorStatus = xpmCreateData(data_return, &attrib, &attributes);
    XpmFreeAttributes(&attributes);
    xpmFreeInternAttrib(&attrib);
    XpmDataClose(&mdata);

    return (ErrorStatus);
}
