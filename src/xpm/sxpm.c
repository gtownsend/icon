/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* sxpm.c:                                                                     *
*                                                                             *
*  Show XPM File program                                                      *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#ifdef VMS
#include "decw$include:Xlib.h"
#include "decw$include:Intrinsic.h"
#include "decw$include:Shell.h"
#include "decw$include:shape.h"
#else
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#endif

#include "xpm.h"

#ifdef Debug
/* memory leak control tool */
#include <mnemosyne.h>
#endif

/* XPM */
/* plaid pixmap */
static char *plaid[] =
{
/* width height ncolors chars_per_pixel */
 "22 22 4 2 XPMEXT",
/* colors */
 "   c red 	m white  s light_color",
 "Y  c green	m black  s lines_in_mix",
 "+  c yellow	m white  s lines_in_dark",
 "x 		m black  s dark_color",
/* pixels */
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "Y Y Y Y Y x Y Y Y Y Y + x + x + x + x + x + ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
 "x x x x x x x x x x x x x x x x x x x x x x ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
"bullshit",
"XPMEXT ext1 data1",
"XPMEXT ext2",
"data2_1",
"data2_2",
"XPMEXT",
"foo",
"",
"XPMEXT ext3",
"data3",
"XPMENDEXT"
};

#define win XtWindow(topw)
#define dpy XtDisplay(topw)
#define screen XtScreen(topw)
#define root XRootWindowOfScreen(screen)
#define xrdb XtDatabase(dpy)
static Colormap colormap;

void Usage();
void ErrorMessage();
void Punt();
void kinput();

#define IWIDTH      50
#define IHEIGHT     50

typedef struct _XpmIcon {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
} XpmIcon;

static char **command;
static Widget topw;
static XpmIcon view, icon;
static XrmOptionDescRec options[] = {
    {"-hints", ".hints", XrmoptionNoArg, (XtPointer) "True"},
    {"-icon", ".icon", XrmoptionSepArg, (XtPointer) NULL},
};

main(argc, argv)
    int argc;
    char **argv;
{
    int ErrorStatus;
    unsigned int verbose = 0;
    unsigned int stdinf = 1;
    unsigned int stdoutf = 0;
    unsigned int nod = 0;
    unsigned int incResize = 0;
    unsigned int resize = 0;
    unsigned int w_rtn;
    unsigned int h_rtn;
    char *input = NULL;
    char *output = NULL;
    char *iconFile = NULL;
    unsigned int numsymbols = 0;
    XpmColorSymbol symbols[10];
    char *stype;
    XrmValue val;
    unsigned long valuemask = 0;
    int n;
    Arg args[3];

#ifdef Debug2
    char **data;

#endif

    topw = XtInitialize(argv[0], "Sxpm",
			options, XtNumber(options), &argc, argv);

    if (!topw) {
	fprintf(stderr, "Sxpm Error... [ Undefined DISPLAY ]\n");
	exit(1);
    }

    colormap = XDefaultColormapOfScreen(screen);

    /*
     * geometry management 
     */

    if (XrmGetResource(xrdb, NULL, "sxpm.geometry", &stype, &val)
	|| XrmGetResource(xrdb, NULL, "Sxpm.geometry", &stype, &val)) {

	int flags;
	int x_rtn;
	int y_rtn;
	char *geo = NULL;

	geo = (char *) val.addr;
	flags = XParseGeometry(geo, &x_rtn, &y_rtn,
			       (unsigned int *) &w_rtn,
			       (unsigned int *) &h_rtn);
	if (!((WidthValue & flags) && (HeightValue & flags)))
	    resize = 1;
    } else
	resize = 1;

    n = 0;
    if (resize) {
	w_rtn = 0;
	h_rtn = 0;
	XtSetArg(args[n], XtNwidth, 1);
	n++;
	XtSetArg(args[n], XtNheight, 1);
	n++;
    }
    XtSetArg(args[n], XtNmappedWhenManaged, False);
    n++;
    XtSetValues(topw, args, n);

    if ((XrmGetResource(xrdb, "sxpm.hints", "", &stype, &val)
	 || XrmGetResource(xrdb, "Sxpm.hints", "", &stype, &val))
	&& !strcmp((char *) val.addr, "True")) {
	/* gotcha */
	incResize = 1;
	resize = 1;
    }

    /*
     * icon management 
     */

    if (XrmGetResource(xrdb, "sxpm.icon", "", &stype, &val) ||
	XrmGetResource(xrdb, "Sxpm.icon", "", &stype, &val)) {
	iconFile = (char *) val.addr;
    }
    if (iconFile) {

	XColor color, junk;
	Pixel bpix;
	Window iconW;

	if (XAllocNamedColor(dpy, colormap, "black", &color, &junk))
	    bpix = color.pixel;
	else
	    bpix = XBlackPixelOfScreen(screen);

	iconW = XCreateSimpleWindow(dpy, root, 0, 0,
				    IWIDTH, IHEIGHT, 1, bpix, bpix);

	icon.attributes.valuemask = XpmReturnPixels;
	ErrorStatus = XpmReadFileToPixmap(dpy, root, iconFile, &icon.pixmap,
					  &icon.mask, &icon.attributes);
	ErrorMessage(ErrorStatus, "Icon");

	XSetWindowBackgroundPixmap(dpy, iconW, icon.pixmap);

	n = 0;
	XtSetArg(args[n], XtNbackground, bpix);
	n++;
	XtSetArg(args[n], XtNiconWindow, iconW);
	n++;
	XtSetValues(topw, args, n);
    }

    /*
     * arguments parsing 
     */

    command = argv;
    for (n = 1; n < argc; n++) {
	if (strncmp(argv[n], "-plaid", 3) == 0) {
	    stdinf = 0;
	    continue;
	}
	if (argv[n][0] != '-') {
	    stdinf = 0;
	    input = argv[n];
	    continue;
	}
	if ((strlen(argv[n]) == 1) && (argv[n][0] == '-'))
	    /* stdin */
	    continue;
	if (strncmp(argv[n], "-o", 2) == 0) {
	    if (n < argc - 1) {
		if ((strlen(argv[n + 1]) == 1) && (argv[n + 1][0] == '-'))
		    stdoutf = 1;
		else
		    output = argv[n + 1];
		n++;
		continue;
	    } else
		Usage();
	}
        if (strncmp(argv[n], "-nod", 2) == 0) {
            nod = 1;
            continue;
        }
	if (strncmp(argv[n], "-s", 2) == 0) {
	    if (n < argc - 2) {
		valuemask |= XpmColorSymbols;
		symbols[numsymbols].name = argv[++n];
		symbols[numsymbols++].value = argv[++n];
		continue;
	    } else
		Usage();
	}
	if (strncmp(argv[n], "-p", 2) == 0) {
	    if (n < argc - 2) {
		valuemask |= XpmColorSymbols;
		symbols[numsymbols].name = argv[++n];
		symbols[numsymbols].value = NULL;
		symbols[numsymbols++].pixel = atol(argv[++n]);
		continue;
	    }
	}
	if (strcmp(argv[n], "-closecolors") == 0) {
	    valuemask |= XpmCloseness;
	    view.attributes.closeness = 40000;
	    continue;
	}
	if (strncmp(argv[n], "-rgb", 3) == 0) {
	    if (n < argc - 1) {
		valuemask |= XpmRgbFilename;
		view.attributes.rgb_fname = argv[++n];
		continue;
	    } else
		Usage();

	}
        if (strncmp(argv[n], "-v", 2) == 0) {
            verbose = 1;
            continue;
        }
        if (strncmp(argv[n], "-c", 2) == 0) {
	    valuemask |= XpmColormap;
            continue;
        }
	Usage();
    }

    XtRealizeWidget(topw);
    if (valuemask & XpmColormap) {
	colormap = XCreateColormap(dpy, win,
				   DefaultVisual(dpy, DefaultScreen(dpy)),
				   AllocNone);
	view.attributes.colormap = colormap;
	XSetWindowColormap(dpy, win, colormap);
    }
    view.attributes.colorsymbols = symbols;
    view.attributes.numsymbols = numsymbols;
    view.attributes.valuemask = valuemask;

#ifdef Debug2
    /* this is just to test the XpmCreateDataFromPixmap function */

    view.attributes.valuemask |= XpmReturnPixels;
    view.attributes.valuemask |= XpmReturnExtensions;
    ErrorStatus = XpmCreatePixmapFromData(dpy, win, plaid,
					  &view.pixmap, &view.mask,
					  &view.attributes);
    ErrorMessage(ErrorStatus, "Plaid");

    ErrorStatus = XpmCreateDataFromPixmap(dpy, &data, view.pixmap, view.mask,
					  &view.attributes);
    ErrorMessage(ErrorStatus, "Data");
    if (verbose && view.attributes.nextensions) {
	unsigned int i, j;
	for (i = 0; i < view.attributes.nextensions; i++) {
	    fprintf(stderr, "Xpm extension : %s\n",
		    view.attributes.extensions[i].name);
	    for (j = 0; j < view.attributes.extensions[i].nlines; j++)
		fprintf(stderr, "\t\t%s\n",
			view.attributes.extensions[i].lines[j]);
	}
    }

    XFreePixmap(dpy, view.pixmap);
    if (view.mask)
	XFreePixmap(dpy, view.mask);

    XFreeColors(dpy, colormap,
		view.attributes.pixels, view.attributes.npixels, 0);

    XpmFreeAttributes(&view.attributes);
    view.attributes.valuemask = valuemask;
#endif

    if (input || stdinf) {
	view.attributes.valuemask |= XpmReturnInfos;
	view.attributes.valuemask |= XpmReturnPixels;
	view.attributes.valuemask |= XpmReturnExtensions;

#ifdef Debug2
	free(data);

	ErrorStatus = XpmReadFileToData(input, &data);
	ErrorMessage(ErrorStatus, "ReadFileToData");
	ErrorStatus = XpmCreatePixmapFromData(dpy, win, data,
					      &view.pixmap, &view.mask,
					      &view.attributes);
	ErrorMessage(ErrorStatus, "CreatePixmapFromData");
	ErrorStatus = XpmWriteFileFromData("sxpmout.xpm", data);
	ErrorMessage(ErrorStatus, "WriteFileFromData");
	free(data);
#endif
	
	ErrorStatus = XpmReadFileToPixmap(dpy, win, input,
					  &view.pixmap, &view.mask,
					  &view.attributes);
	ErrorMessage(ErrorStatus, "Read");
	if (verbose && view.attributes.nextensions) {
	    unsigned int i, j;
	    for (i = 0; i < view.attributes.nextensions; i++) {
		fprintf(stderr, "Xpm extension : %s\n",
		       view.attributes.extensions[i].name);
		for (j = 0; j < view.attributes.extensions[i].nlines; j++)
		    fprintf(stderr, "\t\t%s\n",
			    view.attributes.extensions[i].lines[j]);
	    }
	}
    } else {
#ifdef Debug2
	ErrorStatus = XpmCreatePixmapFromData(dpy, win, data,
					      &view.pixmap, &view.mask,
					      &view.attributes);
	free(data);
#else
	ErrorStatus = XpmCreatePixmapFromData(dpy, win, plaid,
					      &view.pixmap, &view.mask,
					      &view.attributes);
#endif
	ErrorMessage(ErrorStatus, "Plaid");
    }
    if (output || stdoutf) {
	ErrorStatus = XpmWriteFileFromPixmap(dpy, output, view.pixmap,
					     view.mask, &view.attributes);
	ErrorMessage(ErrorStatus, "Write");
    }
    if (!nod) {

	/*
	 * manage display if requested 
	 */

	XSizeHints size_hints;
	char *xString = NULL;

	if (w_rtn && h_rtn
	    && ((w_rtn < view.attributes.width)
		|| h_rtn < view.attributes.height)) {
	    resize = 1;
	}
	if (resize) {
	    XtResizeWidget(topw,
			 view.attributes.width, view.attributes.height, 1);
	}
	if (incResize) {
	    size_hints.flags = USSize | PMinSize | PResizeInc;
	    size_hints.height = view.attributes.height;
	    size_hints.width = view.attributes.width;
	    size_hints.height_inc = view.attributes.height;
	    size_hints.width_inc = view.attributes.width;
	} else
	    size_hints.flags = PMinSize;

	size_hints.min_height = view.attributes.height;
	size_hints.min_width = view.attributes.width;
	XSetWMNormalHints(dpy, win, &size_hints);

	if (input) {
	    xString = (char *) XtMalloc((sizeof(char) * strlen(input)) + 20);
	    sprintf(xString, "Sxpm: %s\0", input);
	    XStoreName(dpy, XtWindow(topw), xString);
	    XSetIconName(dpy, XtWindow(topw), xString);
	} else if (stdinf) {
	    XStoreName(dpy, XtWindow(topw), "Sxpm: stdin");
	    XSetIconName(dpy, XtWindow(topw), "Sxpm: stdin");
	} else {
	    XStoreName(dpy, XtWindow(topw), "Sxpm");
	    XSetIconName(dpy, XtWindow(topw), "Sxpm");
	}

	XtAddEventHandler(topw, KeyPressMask, False,
			  (XtEventHandler) kinput, NULL);
	XSetWindowBackgroundPixmap(dpy, win, view.pixmap);

	if (view.mask)
	    XShapeCombineMask(dpy, win, ShapeBounding, 0, 0,
			      view.mask, ShapeSet);

	XClearWindow(dpy, win);
	XMapWindow(dpy, win);
	if (xString)
	    XtFree(xString);
	XtMainLoop();
    }
    Punt(0);
}

void
Usage()
{
    fprintf(stderr, "\nUsage:  %s [options...]\n", command[0]);
    fprintf(stderr, "%s\n", "Where options are:");
    fprintf(stderr, "\n%s\n",
	    "[-d host:display]            Display to connect to.");
    fprintf(stderr, "%s\n",
	    "[-g geom]                    Geometry of window.");
    fprintf(stderr, "%s\n",
	    "[-hints]                     Set ResizeInc for window.");
    fprintf(stderr, "%s\n",
	    "[-icon filename]             Set pixmap for iconWindow.");
    fprintf(stderr, "%s\n",
	    "[-s symbol_name color_name]  Overwrite color defaults.");
    fprintf(stderr, "%s\n",
	    "[-p symbol_name pixel_value] Overwrite color defaults.");
    fprintf(stderr, "%s\n",
	    "[-closecolors]               Try to use `close' colors.");
    fprintf(stderr, "%s\n",
	    "[-plaid]                     Read the included plaid pixmap.");
    fprintf(stderr, "%s\n",
	    "[filename]                   Read from file 'filename', and from \
standard");
    fprintf(stderr, "%s\n",
	    "                             input if 'filename' is '-'.");
    fprintf(stderr, "%s\n",
	    "[-o filename]                Write to file 'filename', and to \
standard");
    fprintf(stderr, "%s\n",
	    "                             output if 'filename' is '-'.");
    fprintf(stderr, "%s\n",
	    "[-nod]                       Don't display in window.");
    fprintf(stderr, "%s\n",
	    "[-rgb filename]              Search color names in the \
rgb text file 'filename'.");
    fprintf(stderr, "%s\n",
	    "[-c]                         Use a private colormap.");
    fprintf(stderr, "%s\n\n",
	    "[-v]                         Verbose - print out extensions.");
    fprintf(stderr, "%s\n\n",
	    "if no input is specified sxpm reads from standard input.");
    exit(0);
}


void
ErrorMessage(ErrorStatus, tag)
    int ErrorStatus;
    char *tag;
{
    char *error = NULL;
    char *warning = NULL;

    switch (ErrorStatus) {
    case XpmSuccess:
	return;
    case XpmColorError:
	warning = "Could not parse or alloc requested color";
	break;
    case XpmOpenFailed:
	error = "Cannot open file";
	break;
    case XpmFileInvalid:
	error = "Invalid XPM file";
	break;
    case XpmNoMemory:
	error = "Not enough memory";
	break;
    case XpmColorFailed:
	error = "Failed to parse or alloc some color";
	break;
    }

    if (warning)
	printf("%s Xpm Warning: %s.\n", tag, warning);

    if (error) {
	printf("%s Xpm Error: %s.\n", tag, error);
	Punt(1);
    }
}

void
Punt(i)
    int i;
{
    if (icon.pixmap) {
	XFreePixmap(dpy, icon.pixmap);
	if (icon.mask)
	    XFreePixmap(dpy, icon.mask);

	XFreeColors(dpy, colormap,
		    icon.attributes.pixels, icon.attributes.npixels, 0);

	XpmFreeAttributes(&icon.attributes);
    }
    if (view.pixmap) {
	XFreePixmap(dpy, view.pixmap);
	if (view.mask)
	    XFreePixmap(dpy, view.mask);

	XFreeColors(dpy, colormap,
		    view.attributes.pixels, view.attributes.npixels, 0);

	XpmFreeAttributes(&view.attributes);
    }
    exit(i);
}

void
kinput(widget, tag, xe, b)
    Widget widget;
    char *tag;
    XEvent *xe;
    Boolean *b;
{
    char c = '\0';

    XLookupString(&(xe->xkey), &c, 1, NULL, NULL);
    if (c == 'q' || c == 'Q')
	Punt(0);
}
