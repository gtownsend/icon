/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* XpmRdFToI.c:                                                                *
*                                                                             *
*  XPM library                                                                *
*  Parse an XPM file and create the image and possibly its mask               *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "xpmP.h"

xpmDataType xpmDataTypes[] =
{
 "", "!", "\n", '\0', '\n', "", "", "", "",	/* Natural type */
 "C", "/*", "*/", '"', '"', ",\n", "static char *", "[] = {\n", "};\n",
 "Lisp", ";", "\n", '"', '"', "\n", "(setq ", " '(\n", "))\n",
#ifdef VMS
 NULL
#else
 NULL, NULL, NULL, 0, 0, NULL, NULL, NULL, NULL
#endif
};

int
XpmReadFileToImage(display, filename, image_return,
		   shapeimage_return, attributes)
    Display *display;
    char *filename;
    XImage **image_return;
    XImage **shapeimage_return;
    XpmAttributes *attributes;
{
    xpmData mdata;
    char buf[BUFSIZ];
    int l, n = 0;
    int ErrorStatus;
    xpmInternAttrib attrib;

    /*
     * initialize return values 
     */
    if (image_return)
	*image_return = NULL;
    if (shapeimage_return)
	*shapeimage_return = NULL;

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

	    ErrorStatus = xpmParseData(&mdata, &attrib, attributes);

	    if (ErrorStatus == XpmSuccess)
		ErrorStatus = xpmCreateImage(display, &attrib, image_return,
                                             shapeimage_return, attributes);
	} else
	    ErrorStatus = XpmFileInvalid;
    } else
	ErrorStatus = XpmFileInvalid;

    if (ErrorStatus >= 0)
	xpmSetAttributes(&attrib, attributes);
    else if (attributes)
	XpmFreeAttributes(attributes);

    xpmFreeInternAttrib(&attrib);
    XpmDataClose(&mdata);

    return (ErrorStatus);
}
