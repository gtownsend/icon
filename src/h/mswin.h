/*
 * mswin.h - macros and types used in the MS Windows graphics interface.
 */

#define DRAWOP_AND			R2_MASKPEN
#define DRAWOP_ANDINVERTED		R2_MASKNOTPEN
#define DRAWOP_ANDREVERSE		R2_NOTMASKPEN
#define DRAWOP_CLEAR			R2_BLACK
#define DRAWOP_COPY			R2_COPYPEN
#define DRAWOP_COPYINVERTED		R2_NOTCOPYPEN
#define DRAWOP_EQUIV			R2_NOTXORPEN
#define DRAWOP_INVERT			R2_NOT
#define DRAWOP_NAND			R2_MASKNOTPEN
#define DRAWOP_NOOP			R2_NOP
#define DRAWOP_NOR			R2_MERGENOTPEN
#define DRAWOP_OR			R2_MERGEPEN
#define DRAWOP_ORINVERTED		R2_MERGEPENNOT
#define DRAWOP_ORREVERSE		R2_NOTMERGEPEN
#define DRAWOP_REVERSE			R2_USER1
#define DRAWOP_SET			R2_WHITE
#define DRAWOP_XOR			R2_XORPEN

#define TEXTWIDTH(w,s,n) textWidth(w, s, n)
#define SCREENDEPTH(w) getdepth(w)
#define ASCENT(w)  ((w)->context->font->ascent)
#define ASCENTC(wc)  ((wc)->font->ascent)
#define DESCENT(w) ((w)->context->font->descent)
#define LEADING(w) ((w)->context->leading)
#define FHEIGHT(w) ((w)->context->font->height)
#define FHEIGHTC(wc) ((wc)->font->height)
#define FWIDTH(w)  ((w)->context->font->charwidth)
#define FWIDTHC(wc)  ((wc)->font->charwidth)
#define LINEWIDTH(w) ((w)->context->pen.lopnWidth.x)
#define DISPLAYHEIGHT(w) devicecaps(w, VERTRES)
#define DISPLAYWIDTH(w) devicecaps(w, HORZRES)
#define wsync(w) /* noop */
#define SysColor unsigned long
#define RED(x) GetRValue(x)
#define GREEN(x) GetGValue(x)
#define BLUE(x) GetBValue(x)
#define ARCWIDTH(arc) (arc).width
#define ARCHEIGHT(arc) (arc).height
/*
 * These get fixed up in the window-system-specific code
 */
#define RECX(rec) (rec).left
#define RECY(rec) (rec).top
#define RECWIDTH(rec) (rec).right
#define RECHEIGHT(rec) (rec).bottom
/*
 *
 */
#define ANGLE(ang) (ang)
#define EXTENT(ang) (ang)
#define FULLARC 2 * Pi
#define ISICONIC(w) (IsIconic((w)->window->iconwin))
#define ISFULLSCREEN(w) 0
#define ISROOTWIN(w) (0) 0
#define ISNORMALWINDOW(w) 0
#define ICONFILENAME(w) ""
#define ICONLABEL(w) ((w)->window->iconlabel)
#define WINDOWLABEL(w) ((w)->window->windowlabel)

#define MAXDESCENDER(w) DESCENT(w)

/*
 * gemeotry bitmasks
 */
#define GEOM_WIDTH           1
#define GEOM_HEIGHT          2
#define GEOM_POSX            4
#define GEOM_POSY            8
/*
 * fill styles
 */
#define FS_SOLID             1
#define FS_STIPPLE           2
#define FS_OPAQUESTIPPLE     4
/*
 * the special ROP code for mode reverse
 */
#define R2_USER1            (R2_LAST << 1)
/*
 * window states
 */
#define WS_NORMAL            0
#define WS_MIN               1
#define WS_MAX               2

/*
 * input masks
 */
#define PointerMotionMask    1

/*
 * something I think should be #defined
 */
#define EOS                  '\0'

/* size of the working buffer, used for dialog messages and such */
#define PMSTRBUFSIZE         2048
/*
 * the bitmasks for the modifier keys
 */
#define ControlMask          (1L << 16L)
#define Mod1Mask             (2L << 16L)
#define ShiftMask            (4L << 16L)
#define VirtKeyMask          (8L << 16L)

/* some macros for Windows */

#define MAKERGB(r,g,b) RGB(r,g,b)
#define RGB16TO8(x) if ((x) > 0xff) (x) = (((x) >> 8) & 0xff)
#define hidecrsr(ws) if (ws->hasCaret) HideCaret(ws->iconwin)
#define showcrsr(ws) if (ws->hasCaret) ShowCaret(ws->iconwin)
#define FNTWIDTH(size) ((size) & 0xFFFF)
#define FNTHEIGHT(size) ((size) >> 16)
#define MAKEFNTSIZE(height, width) (((height) << 16) | (width))
#define WaitForEvent(msgnum, msgstruc) ObtainEvents(NULL, WAIT_EVT, msgnum, msgstruc)

/*
 * "get" means remove them from the Icon list and put them on the ghost que
 */
#define EVQUEGET(ws,d) { \
  int i;\
  if (!c_get((struct b_list *)BlkLoc((ws)->listp),&d)) fatalerr(0,NULL); \
  if (Qual(d)) {\
      (ws)->eventQueue[(ws)->eQfront++] = *StrLoc(d); \
      if ((ws)->eQfront >= EQUEUELEN) (ws)->eQfront = 0; \
      (ws)->eQback = (ws)->eQfront; \
      } \
  }
#define EVQUEEMPTY(ws) (BlkLoc((ws)->listp)->list.size == 0)

#define SHARED          0
#define MUTABLE         1
#define MAXCOLORNAME	40
/*
 * color structure, inspired by X code (xwin.h)
 */
typedef struct wcolor {
  int		refcount;
  char		name[6+MAXCOLORNAME];	/* name for WAttrib & WColor reads */
  SysColor	c;
  int           type;			/* SHARED or MUTABLE */
} *wclrp;

/*
 * we make the segment structure look like this so that we can
 * cast it to POINTL structures that can be passed to GpiPolyLineDisjoint
 */
typedef struct {
   LONG x1, y1;
   LONG x2, y2;
   } XSegment;

typedef POINT XPoint;
typedef RECT XRectangle;

typedef struct {
  LONG x, y;
  LONG width, height;
  double angle1, angle2;
  } XArc;

/*
 * macros performing row/column to pixel y,x translations
 * computation is 1-based and depends on the current font's size.
 * exception: XTOCOL as defined is 0-based, because that's what its
 * clients seem to need.
 */
#define ROWTOY(wb, row)  ((row - 1) * LEADING(wb) + ASCENT(wb))
#define COLTOX(wb, col)  ((col - 1) * FWIDTH(wb))
#define YTOROW(wb, y)    (((y) - ASCENT(w)) /  LEADING(wb) + 1)
#define XTOCOL(w,x)  (!FWIDTH(w) ? (x) : ((x) / FWIDTH(w)))

/*
 * system size values
 */
#define BORDERWIDTH      (GetSystemMetrics(SM_CXBORDER)) /* 1 */
#define BORDERHEIGHT     (GetSystemMetrics(SM_CYBORDER)) /* 1 */
#define TITLEHEIGHT      (GetSystemMetrics(SM_CYCAPTION)) /* 20 */
#define FRAMEWIDTH	 (GetSystemMetrics(SM_CXFRAME))   /* 4 */
#define FRAMEHEIGHT	 (GetSystemMetrics(SM_CYFRAME))   /* 4 */

#define STDLOCALS(w) \
   wcp wc = (w)->context;\
   wsp ws = (w)->window;\
   HWND stdwin = ws->win;\
   HBITMAP stdpix = ws->pix;\
   HDC stddc = CreateWinDC(w);\
   HDC pixdc = CreatePixDC(w, stddc);

#define STDFONT \
   { if(stdwin)SelectObject(stddc, wc->font->font); SelectObject(pixdc,wc->font->font); }

#define FREE_STDLOCALS(w) do { SelectObject(pixdc, (w)->window->theOldPix); ReleaseDC((w)->window->iconwin, stddc); DeleteDC(pixdc); } while (0)

#define MAXXOBJS 8

#define GammaCorrection 1.0
