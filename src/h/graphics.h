/*
 * graphics.h - macros and types used in Icon's graphics interface.
 */

#ifdef XWindows
   #include "../h/xwin.h"
#endif					/* XWindows */

#ifdef WinGraphics
   #include "../h/mswin.h"
#endif					/* WinGraphics */

#ifndef MAXXOBJS
   #define MAXXOBJS 256
#endif					/* MAXXOBJS */

#ifndef MAXCOLORNAME
   #define MAXCOLORNAME 40
#endif					/* MAXCOLORNAME */

#ifndef MAXFONTWORD
   #define MAXFONTWORD 40
#endif					/* MAXFONTWORD */

#define POLLSLEEP	20	 /* milliseconds sleep while awaiting event */

#define DEFAULTFONTSIZE 14

#define FONTATT_SPACING		0x01000000
#define FONTFLAG_MONO		0x00000001
#define FONTFLAG_PROPORTIONAL	0x00000002

#define FONTATT_SERIF		0x02000000
#define FONTFLAG_SANS		0x00000004
#define FONTFLAG_SERIF		0x00000008

#define FONTATT_SLANT		0x04000000
#define FONTFLAG_ROMAN		0x00000010
#define FONTFLAG_ITALIC		0x00000020
#define FONTFLAG_OBLIQUE	0x00000040

#define FONTATT_WEIGHT		0x08000000
#define FONTFLAG_LIGHT		0x00000100
#define FONTFLAG_MEDIUM		0x00000200
#define FONTFLAG_DEMI		0x00000400
#define FONTFLAG_BOLD		0x00000800

#define FONTATT_WIDTH		0x10000000
#define FONTFLAG_CONDENSED	0x00001000
#define FONTFLAG_NARROW		0x00002000
#define FONTFLAG_NORMAL		0x00004000
#define FONTFLAG_WIDE		0x00008000
#define FONTFLAG_EXTENDED	0x00010000

/*
 * EVENT HANDLING
 *
 * Each window keeps an associated queue of events waiting to be
 * processed.  The queue consists of <eventcode,x,y> triples,
 * where eventcodes are strings for normal keyboard events, and
 * integers for mouse and special keystroke events.
 *
 * The main queue is an icon list.  In addition, there is a queue of
 * old keystrokes maintained for cooked mode operations, maintained
 * in a little circular array of chars.
 */
#define EQ_MOD_CONTROL (1L<<16L)
#define EQ_MOD_META    (1L<<17L)
#define EQ_MOD_SHIFT   (1L<<18L)

#define EVQUESUB(w,i) *evquesub(w,i)
#define EQUEUELEN 256

/*
 * mode bits for the Icon window context (as opposed to X context)
 */

#define ISINITIAL(w)    ((w)->window->bits & 1)
#define ISINITIALW(ws)  ((ws)->bits & 1)
#define ISCURSORON(w)   ((w)->window->bits & 2)
#define ISCURSORONW(ws) ((ws->bits) & 2)
#define ISMAPPED(w)	((w)->window->bits & 4)
#define ISREVERSE(w)    ((w)->context->bits & 8)
#define ISXORREVERSE(w)	((w)->context->bits & 16)
#define ISXORREVERSEW(w) ((w)->bits & 16)
#define ISCLOSED(w)	((w)->window->bits & 64)
#define ISRESIZABLE(w)	((w)->window->bits & 128)
#define ISEXPOSED(w)    ((w)->window->bits & 256)
#define ISCEOLON(w)     ((w)->window->bits & 512)
#define ISECHOON(w)     ((w)->window->bits & 1024)

#define SETCURSORON(w)  ((w)->window->bits |= 2)
/* 4 is available */
#define SETMAPPED(w)   ((w)->window->bits |= 4)
#define SETREVERSE(w)   ((w)->context->bits |= 8)
#define SETXORREVERSE(w) ((w)->context->bits |= 16)
#define SETCLOSED(w)	((w)->window->bits |= 64)
#define SETRESIZABLE(w)	((w)->window->bits |= 128)
#define SETEXPOSED(w)   ((w)->window->bits |= 256)
#define SETCEOLON(w)    ((w)->window->bits |= 512)
#define SETECHOON(w)    ((w)->window->bits |= 1024)

#define CLRCURSORON(w)  ((w)->window->bits &= ~2)
#define CLRMAPPED(w)    ((w)->window->bits &= ~4)
#define CLRREVERSE(w)   ((w)->context->bits &= ~8)
#define CLRXORREVERSE(w) ((w)->context->bits &= ~16)
#define CLRCLOSED(w)	((w)->window->bits &= ~64)
#define CLRRESIZABLE(w)	((w)->window->bits &= ~128)
#define CLREXPOSED(w)   ((w)->window->bits &= ~256)
#define CLRCEOLON(w)    ((w)->window->bits &= ~512)
#define CLRECHOON(w)    ((w)->window->bits &= ~1024)

#ifdef XWindows
   #define ISZOMBIE(w)     ((w)->window->bits & 1)
   #define SETZOMBIE(w)    ((w)->window->bits |= 1)
   #define CLRZOMBIE(w)    ((w)->window->bits &= ~1)
#endif					/* XWindows */

#ifdef WinGraphics
   #define ISTOBEHIDDEN(ws)  ((ws)->bits & 4096)
   #define SETTOBEHIDDEN(ws) ((ws)->bits |= 4096)
   #define CLRTOBEHIDDEN(ws) ((ws)->bits &= ~4096)
#endif					/* WinGraphics */

/*
 * Window Resources
 * Icon "Resources" are a layer on top of the window system resources,
 * provided in order to facilitate resource sharing and minimize the
 * number of calls to the window system.  Resources are reference counted.
 * These data structures are simple sets of pointers
 * into internal window system structures.
 */

/*
 * Fonts are allocated within displays.
 */
typedef struct _wfont {
   int		refcount;
   int		serial;			/* serial # */
   struct _wfont *previous, *next;
   char		*name;			/* name for WAttrib and fontsearch */

   #ifdef XWindows
      int	height;			/* font height */
      XFontStruct *fsp;			/* X font pointer */
   #endif				/* XWindows */

   #ifdef WinGraphics
      HFONT	font;
      LONG	ascent;
      LONG	descent;
      LONG	charwidth;
      LONG	height;
   #endif				/* WinGraphics */

   } wfont, *wfp;

/*
 * These structures and definitions are used for colors and images.
 */
typedef struct {
   long red, green, blue;		/* color components, linear 0 - 65535*/
   } LinearColor;

struct palentry {			/* entry for one palette member */
   LinearColor clr;			/* RGB value of color */
   char used;				/* nonzero if char is used */
   char valid;				/* nonzero if entry is valid & opaque */
   char transpt;			/* nonzero if char is transparent */
   };

struct imgdata {			/* image loaded from a file */
   int width, height;			/* image dimensions */
   struct palentry *paltbl;		/* pointer to palette table */
   unsigned char *data;			/* pointer to image data */
   };

struct imgmem {
   int x, y, width, height;

   #ifdef XWindows
      XImage *im;
   #endif				/* XWindows */

   #ifdef WinGraphics
      COLORREF *crp;
   #endif				/* WinGraphics */
   };

#define TCH1 '~'			/* usual transparent character */
#define TCH2 0377			/* alternate transparent character */
#define PCH1 ' '			/* punctuation character */
#define PCH2 ','			/* punctuation character */

#define GIFMAX 256			/* maximum colors in a GIF file */

#ifdef XWindows
/*
 * Displays are maintained in a global list in rwinrsc.r.
 */
typedef struct _wdisplay {
   int		refcount;
   int		serial;			/* serial # */
   char		name[MAXDISPLAYNAME];
   Display *	display;
   Visual *	visual;
   GC		icongc;
   Colormap	cmap;
   double	gamma;
   int		screen;
   int		numFonts;
   wfp		fonts;
   int          numColors;		/* number of allocated color structs */
   int          cpSize;			/* max number of slots before realloc */
   struct wcolor **colrptrs;		/* array of pointers to those colors */
   Cursor	cursors[NUMCURSORSYMS];
   struct _wdisplay *previous, *next;
   } *wdp;
#endif					/* XWindows */

/*
 * "Context" comprises the graphics context, and the font (i.e. text context).
 * Foreground and background colors (pointers into the display color table)
 * are stored here to reduce the number of window system queries.
 * Contexts are allocated out of a global array in rwinrsrc.c.
 */
typedef struct _wcontext {
   int		refcount;
   int		serial;			/* serial # */
   struct _wcontext *previous, *next;
   int		clipx, clipy, clipw, cliph;
   char		*patternname;
   wfp		font;
   int		dx, dy;
   int		fillstyle;
   int		drawop;
   double	gamma;			/* gamma correction value */
   int		bits;			/* context bits */

   #ifdef XWindows
      wdp	display;
      GC	gc;			/* X graphics context */
      wclrp	fg, bg;
      int	linestyle;
      int	linewidth;
      int	leading;		/* inter-line leading */
   #endif				/* XWindows */

   #ifdef WinGraphics
      LOGPEN	pen;
      LOGPEN	bgpen;
      LOGBRUSH	brush;
      LOGBRUSH	bgbrush;
      HRGN	cliprgn;
      HBITMAP	pattern;
      SysColor	fg, bg;
      char	*fgname, *bgname;
      int	leading, bkmode;
   #endif				/* WinGraphics*/

   } wcontext, *wcp;

/*
 * Native facilities include the following child controls (windows) that
 * persist on the canvas and intercept various events.
 */
#ifdef WinGraphics
   #define CHILD_BUTTON 0
   #define CHILD_SCROLLBAR 1
   #define CHILD_EDIT 2
   typedef struct childcontrol {
      int  type;			/* what kind of control? */
      HWND win;				/* child window handle */
      HFONT font;
      char *id;				/* child window string id */
      } childcontrol;
#endif					/* WinGraphics */

/*
 * "Window state" includes the actual X window and references to a large
 * number of resources allocated on a per-window basis.  Windows are
 * allocated out of a global array in rwinrsrc.c.  Windows remember the
 * first WMAXCOLORS colors they allocate, and deallocate them on clearscreen.
 */
typedef struct _wstate {
   int		refcount;		/* reference count */
   int		serial;			/* serial # */
   struct _wstate *previous, *next;
   int		pixheight;		/* backing pixmap height, in pixels */
   int		pixwidth;		/* pixmap width, in pixels */
   char		*windowlabel;		/* window label */
   char		*iconimage;		/* icon pixmap file name */
   char		*iconlabel;		/* icon label */
   struct imgdata initimage;		/* initial image data */
   struct imgdata initicon;		/* initial icon image data */
   int		y, x;			/* current cursor location, in pixels*/
   int		pointery, pointerx;	/* current mouse location, in pixels */
   int		posy, posx;		/* desired upper lefthand corner */
   unsigned int	height;			/* window height, in pixels */
   unsigned int	width;			/* window width, in pixels */
   int		bits;			/* window bits */
   int		theCursor;		/* index into cursor table */
   word		timestamp;		/* last event time stamp */
   char		eventQueue[EQUEUELEN];  /* queue of cooked-mode keystrokes */
   int		eQfront, eQback;
   char		*cursorname;
   struct descrip filep, listp;		/* icon values for this window */

   #ifdef XWindows
      wdp	display;
      Window	win;			/* X window */
      Pixmap	pix;			/* current screen state */
      Pixmap	initialPix;		/* an initial image to display */
      Window	iconwin;		/* icon window */
      Pixmap	iconpix;		/* icon pixmap */
      int	normalx, normaly;	/* pos to remember when maximized */
      int	normalw, normalh;	/* size to remember when maximized */
      int	numColors;		/* allocated color info */
      short	*theColors;		/* indices into display color table */
      int	numiColors;		/* allocated color info for the icon */
      short	*iconColors;		/* indices into display color table */
      int	iconic;			/* window state; icon, window or root*/
      int	iconx, icony;		/* location of icon */
      unsigned int iconw, iconh;	/* width and height of icon */
      long	wmhintflags;		/* window manager hints */
   #endif				/* XWindows */

   #ifdef WinGraphics
      HWND	win;			/* client window */
      HWND	iconwin;		/* client window when iconic */
      HBITMAP	pix;			/* backing bitmap */
      HBITMAP	iconpix;		/* backing bitmap */
      HBITMAP	initialPix;		/* backing bitmap */
      HBITMAP	theOldPix;
      int	hasCaret;
      HCURSOR	curcursor;
      HCURSOR	savedcursor;
      HMENU	menuBar;
      int	nmMapElems;
      char **	menuMap;
      HWND	focusChild;
      int	nChildren;
      childcontrol *child;
   #endif				/* WinGraphics */

   } wstate, *wsp;

/*
 * Icon window file variables are actually pointers to "bindings"
 * of a window and a context.  They are allocated out of a global
 * array in rwinrsrc.c.  There is one binding per Icon window value.
 */
typedef struct _wbinding {
   int refcount;
   int serial;
   struct _wbinding *previous, *next;
   wcp context;
   wsp window;
   } wbinding, *wbp;

/*
 * Table entry for string <-> integer mapping.
 */
typedef struct {
   char *s;
   int i;
   } stringint, *siptr;


/*
 * Gamma Correction value to compensate for nonlinear monitor color response
 */
#ifndef GammaCorrection
   #define GammaCorrection 2.5
#endif					/* GammaCorrection */

/*
 * Attributes
 */
#define A_ASCENT	1
#define A_BG		2
#define A_CANVAS	3
#define A_CEOL		4
#define A_CLIPH		5
#define A_CLIPW		6
#define A_CLIPX		7
#define A_CLIPY		8
#define A_COL		9
#define A_COLUMNS	10
#define A_CURSOR	11
#define A_DEPTH		12
#define A_DESCENT	13
#define A_DISPLAY	14
#define A_DISPLAYHEIGHT	15
#define A_DISPLAYWIDTH	16
#define A_DRAWOP	17
#define A_DX		18
#define A_DY		19
#define A_ECHO		20
#define A_FG		21
#define A_FHEIGHT	22
#define A_FILLSTYLE	23
#define A_FONT		24
#define A_FWIDTH	25
#define A_GAMMA		26
#define A_GEOMETRY	27
#define A_HEIGHT	28
#define A_ICONIC	29
#define A_ICONIMAGE     30
#define A_ICONLABEL	31
#define A_ICONPOS	32
#define A_IMAGE		33
#define A_LABEL		34
#define A_LEADING	35
#define A_LINES		36
#define A_LINESTYLE	37
#define A_LINEWIDTH	38
#define A_PATTERN	39
#define A_POINTERCOL	40
#define A_POINTERROW	41
#define A_POINTERX	42
#define A_POINTERY	43
#define A_POINTER	44
#define A_POS		45
#define A_POSX		46
#define A_POSY		47
#define A_RESIZE	48
#define A_REVERSE	49
#define A_ROW		50
#define A_ROWS		51
#define A_SIZE		52
#define A_VISUAL	53
#define A_WIDTH		54
#define A_WINDOWLABEL   55
#define A_X		56
#define A_Y		57

#define NUMATTRIBS	57

/*
 * flags for ConsoleFlags
 */
/* I/O redirection flags */
#define StdOutRedirect        1
#define StdErrRedirect        2
#define StdInRedirect         4
#define OutputToBuf           8
