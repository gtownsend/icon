#ifdef XWindows

#define DRAWOP_AND			GXand
#define DRAWOP_ANDINVERTED		GXandInverted
#define DRAWOP_ANDREVERSE		GXandReverse
#define DRAWOP_CLEAR			GXclear
#define DRAWOP_COPY			GXcopy
#define DRAWOP_COPYINVERTED		GXcopyInverted
#define DRAWOP_EQUIV			GXequiv
#define DRAWOP_INVERT			GXinvert
#define DRAWOP_NAND			GXnand
#define DRAWOP_NOOP			GXnoop
#define DRAWOP_NOR			GXnor
#define DRAWOP_OR			GXor
#define DRAWOP_ORINVERTED		GXorInverted
#define DRAWOP_ORREVERSE		GXorReverse
#define DRAWOP_REVERSE			0x10
#define DRAWOP_SET			GXset
#define DRAWOP_XOR			GXxor

#define XLFD_Foundry	 1
#define XLFD_Family	 2
#define XLFD_Weight	 3
#define XLFD_Slant	 4
#define XLFD_SetWidth	 5
#define XLFD_AddStyle	 6
#define XLFD_Size	 7
#define XLFD_PointSize	 8
#define XLFD_Spacing	11
#define XLFD_CharSet	13

#define TEXTWIDTH(w,s,n) XTextWidth((w)->context->font->fsp, s, n)
#define SCREENDEPTH(w)\
        DefaultDepth((w)->window->display->display, w->window->display->screen)
#define ASCENT(w) ((w)->context->font->fsp->ascent)
#define DESCENT(w) ((w)->context->font->fsp->descent)
#define LEADING(w) ((w)->context->leading)
#define FHEIGHT(w) ((w)->context->font->height)
#define FWIDTH(w) ((w)->context->font->fsp->max_bounds.width)
#define LINEWIDTH(w) ((w)->context->linewidth)
#define DISPLAYHEIGHT(w)\
        DisplayHeight(w->window->display->display, w->window->display->screen)
#define DISPLAYWIDTH(w)\
        DisplayWidth(w->window->display->display, w->window->display->screen)
#define FS_SOLID FillSolid
#define FS_STIPPLE FillStippled
#define hidecrsr(x) /* noop */
#define UpdateCursorPos(x, y) /* noop */
#define showcrsr(x) /* noop */
#define SysColor XColor
#define ARCWIDTH(arc) ((arc).width)
#define ARCHEIGHT(arc) ((arc).height)
#define RECX(rec) ((rec).x)
#define RECY(rec) ((rec).y)
#define RECWIDTH(rec) ((rec).width)
#define RECHEIGHT(rec) ((rec).height)
#define ANGLE(ang) (-(ang) * 180 / Pi * 64)
#define EXTENT(ang) (-(ang) * 180 / Pi * 64)
#define ISICONIC(w) ((w)->window->iconic == IconicState)
#define ISFULLSCREEN(w) (0)
#define ISROOTWIN(w) ((w)->window->iconic == RootState)
#define ISNORMALWINDOW(w) ((w)->window->iconic == NormalState)
#define ICONFILENAME(w) ((w)->window->iconimage)
#define ICONLABEL(w) ((w)->window->iconlabel)
#define WINDOWLABEL(w) ((w)->window->windowlabel)
#define RootState IconicState+1
#define MaximizedState IconicState+2
#define HiddenState IconicState+3

/*
 * The following constants define limitations in the system, gradually being
 * removed as this code is rewritten to use dynamic allocation.
 */
#define WMAXCOLORS	256
#define MAXCOLORNAME	40
#define MAXDISPLAYNAME	64
#define SHARED          0
#define MUTABLE         1
#define NUMCURSORSYMS	78

/*
 * Macros to ease coding in which every X call must be done twice.
 */
#define RENDER2(func,v1,v2) {\
   if (stdwin) func(stddpy, stdwin, stdgc, v1, v2); \
   func(stddpy, stdpix, stdgc, v1, v2);}
#define RENDER3(func,v1,v2,v3) {\
   if (stdwin) func(stddpy, stdwin, stdgc, v1, v2, v3); \
   func(stddpy, stdpix, stdgc, v1, v2, v3);}
#define RENDER4(func,v1,v2,v3,v4) {\
   if (stdwin) func(stddpy, stdwin, stdgc, v1, v2, v3, v4); \
   func(stddpy, stdpix, stdgc, v1, v2, v3, v4);}
#define RENDER6(func,v1,v2,v3,v4,v5,v6) {\
   if (stdwin) func(stddpy, stdwin, stdgc, v1, v2, v3, v4, v5, v6); \
   func(stddpy, stdpix, stdgc, v1, v2, v3, v4, v5, v6);}
#define RENDER7(func,v1,v2,v3,v4,v5,v6,v7) {\
   if (stdwin) func(stddpy, stdwin, stdgc, v1, v2, v3, v4, v5, v6, v7); \
   func(stddpy, stdpix, stdgc, v1, v2, v3, v4, v5, v6, v7);}

#define MAXDESCENDER(w) (w->context->font->fsp->max_bounds.descent)

/*
 * Macros to perform direct window system calls from graphics routines
 */
#define STDLOCALS(w) \
   wcp wc = (w)->context; \
   wsp ws = (w)->window; \
   wdp wd = ws->display; \
   GC      stdgc  = wc->gc; \
   Display *stddpy = wd->display; \
   Window  stdwin  = ws->win; \
   Pixmap  stdpix  = ws->pix;

#define drawarcs(w, arcs, narcs) \
   { STDLOCALS(w); RENDER2(XDrawArcs,arcs,narcs); }
#define drawlines(w, points, npoints) \
   { STDLOCALS(w); RENDER3(XDrawLines,points,npoints,CoordModeOrigin); }
#define drawpoints(w, points, npoints) \
   { STDLOCALS(w); RENDER3(XDrawPoints,points,npoints,CoordModeOrigin); }
#define drawrectangles(w, recs, nrecs) { \
   STDLOCALS(w); \
   for(i=0; i<nrecs; i++) { \
     RENDER4(XDrawRectangle,recs[i].x,recs[i].y,recs[i].width,recs[i].height);\
     }}

#define drawsegments(w, segs, nsegs) \
   { STDLOCALS(w); RENDER2(XDrawSegments,segs,nsegs); }
#define drawstrng(w, x, y, s, slen) \
   { STDLOCALS(w); RENDER4(XDrawString, x, y, s, slen); }
#define fillarcs(w, arcs, narcs) \
   { STDLOCALS(w); RENDER2(XFillArcs, arcs, narcs); }
#define fillpolygon(w, points, npoints) \
   { STDLOCALS(w); RENDER4(XFillPolygon, points, npoints, Complex, CoordModeOrigin); }

/*
 * "get" means remove them from the Icon list and put them on the ghost queue
 */
#define EVQUEGET(w,d) { \
   int i;\
   wsp ws = (w)->window; \
   if (!c_get((struct b_list *)BlkLoc(ws->listp),&d)) fatalerr(0,NULL); \
   if (Qual(d)) {\
      ws->eventQueue[ws->eQfront++] = *StrLoc(d); \
      if (ws->eQfront >= EQUEUELEN) ws->eQfront = 0; \
      ws->eQback = ws->eQfront; \
      } \
   }
#define EVQUEEMPTY(w) (BlkLoc((w)->window->listp)->list.size == 0)

/*
 * Colors.  These are allocated within displays. Pointers
 * into the display's color table are also kept on a per-window
 * basis so that they may be (de)allocated when a window is cleared.
 * Colors are aliased by r,g,b value.  Allocations by name and r,g,b
 * share when appropriate.
 *
 * Color (de)allocation comprises a simple majority of the space
 * requirements of the current implementation.  A monochrome-only
 * version would take a lot less space.
 *
 * The name field is the string returned by WAttrib.  For a mutable
 * color this is of the form "-47" followed by a second C string
 * containing the current color setting.
 */
typedef struct wcolor {
   unsigned long c;			/* X pixel value */
   int		refcount;		/* reference count */
   int		type;			/* SHARED or MUTABLE */
   int		next;			/* next entry in hash chain */
   unsigned short r, g, b;		/* rgb for colorsearch */
   char		name[6+MAXCOLORNAME];	/* name for WAttrib & WColor reads */
} *wclrp;

/*
 * macros performing row/column to pixel y,x translations
 * computation is 1-based and depends on the current font's size.
 * exception: XTOCOL as defined is 0-based, because that's what its
 * clients seem to need.
 */
#define ROWTOY(w,row) ((row-1) * LEADING(w) + ASCENT(w))
#define COLTOX(w,col) ((col-1) * FWIDTH(w))
#define YTOROW(w,y)   ((y>0) ? ((y) / LEADING(w) + 1) : ((y) / LEADING(w)))
#define XTOCOL(w,x)  (!FWIDTH(w) ? (x) : ((x) / FWIDTH(w)))

#define STDLOCALS(w) \
   wcp wc = (w)->context; \
   wsp ws = (w)->window; \
   wdp wd = ws->display; \
   GC      stdgc  = wc->gc; \
   Display *stddpy = wd->display; \
   Window  stdwin  = ws->win; \
   Pixmap  stdpix  = ws->pix;

#endif					/* XWindows */
