/*
 * Prototypes for run-time functions.
 */

/*
 * Prototypes common to the compiler and interpreter.
 */
void		EVInit		(void);
int		activate	(dptr val, struct b_coexpr *ncp, dptr result);
word		add		(word a,word b);
void		addmem (struct b_set *ps, struct b_selem *pe, union block **pl);
struct astkblk	*alcactiv	(void);
struct b_cset	*alccset	(void);
struct b_file	*alcfile	(FILE *fd,int status,dptr name);
union block	*alchash	(int tcode);
struct b_list	*alclist	(uword size);
struct b_lelem	*alclstb	(uword nslots,uword first,uword nused);
struct b_real	*alcreal	(double val);
struct b_slots	*alcsegment	(word nslots);
struct b_selem	*alcselem	(dptr mbr,uword hn);
char		*alcstr		(char	*s,word slen);
struct b_telem	*alctelem	(void);
struct b_tvtbl	*alctvtbl	(dptr tbl,dptr ref,uword hashnum);
int		anycmp		(dptr dp1,dptr dp2);
int		bfunc		(void);
struct b_proc	*bi_strprc	(dptr s, C_integer arity);
void		c_exit		(int i);
int		c_get		(struct b_list *hp, struct descrip *res);
void		c_put		(struct descrip *l, struct descrip *val);
int		cnv_c_dbl	(dptr s, double *d);
int		cnv_c_int	(dptr s, C_integer *d);
int		cnv_c_str	(dptr s, dptr d);
int		cnv_cset	(dptr s, dptr d);
int		cnv_ec_int	(dptr s, C_integer *d);
int		cnv_eint	(dptr s, dptr d);
int		cnv_int		(dptr s, dptr d);
int		cnv_real	(dptr s, dptr d);
int		cnv_str		(dptr s, dptr d);
int		cnv_tcset	(struct b_cset *cbuf, dptr s, dptr d);
int		cnv_tstr	(char *sbuf, dptr s, dptr d);
int		co_chng		(struct b_coexpr *ncp, struct descrip *valloc,
				   struct descrip *rsltloc,
				   int swtch_typ, int first);
void		co_init		(struct b_coexpr *sblkp);
void		coclean		(word *old);
void		coacttrace	(struct b_coexpr *ccp,struct b_coexpr *ncp);
void		cofailtrace	(struct b_coexpr *ccp,struct b_coexpr *ncp);
void		corettrace	(struct b_coexpr *ccp,struct b_coexpr *ncp);
int		coswitch	(word *old, word *new, int first);
int		cplist		(dptr dp1,dptr dp2,word i,word j);
int		cpset		(dptr dp1,dptr dp2,word size);
void		cpslots		(dptr dp1,dptr slotptr,word i, word j);
int		csetcmp		(unsigned int *cs1,unsigned int *cs2);
int		cssize		(dptr dp);
word		cvpos		(long pos,long len);
void		datainit	(void);
void		deallocate	(union block *bp);
int		def_c_dbl	(dptr s, double df, double * d);
int		def_c_int	(dptr s, C_integer df, C_integer * d);
int		def_c_str	(dptr s, char * df, dptr d);
int		def_cset	(dptr s, struct b_cset * df, dptr d);
int		def_ec_int	(dptr s, C_integer df, C_integer * d);
int		def_eint	(dptr s, C_integer df, dptr d);
int		def_int		(dptr s, C_integer df, dptr d);
int		def_real	(dptr s, double df, dptr d);
int		def_str		(dptr s, dptr df, dptr d);
int		def_tcset (struct b_cset *cbuf,dptr s,struct b_cset *df,dptr d);
int		def_tstr	(char *sbuf, dptr s, dptr df, dptr d);
word		div3		(word a,word b);
int		doasgn		(dptr dp1,dptr dp2);
int		doimage		(int c,int q);
int		dp_pnmcmp	(struct pstrnm *pne,dptr dp);
void		drunerr		(int n, double v);
void		dumpact		(struct b_coexpr *ce);
void		env_int	(char *name,word *variable,int non_neg, uword limit);
int		equiv		(dptr dp1,dptr dp2);
int		err		(void);
void		err_msg		(int n, dptr v);
void		error		(char *s1, char *s2);
void		fatalerr	(int n,dptr v);
int		findcol		(word *ipc);
char		*findfile	(word *ipc);
int		findipc		(int line);
int		findline	(word *ipc);
int		findloc		(word *ipc);
void		fpetrap		(int);
int		getvar		(char *s,dptr vp);
uword		hash		(dptr dp);
union block	**hchain	(union block *pb,uword hn);
union block	*hgfirst	(union block *bp, struct hgstate *state);
union block	*hgnext		(union block*b,struct hgstate*s,union block *e);
union block	*hmake		(int tcode,word nslots,word nelem);
void		icon_init	(char *name, int *argcp, char *argv[]);
void		iconhost	(char *hostname);
int		idelay		(int n);
int		interp		(int fsig,dptr cargp);
void		irunerr		(int n, C_integer v);
int		lexcmp		(dptr dp1,dptr dp2);
word		longread	(char *s,int width,long len,FILE *fname);
union block	**memb		(union block *pb,dptr x,uword hn, int *res);
void		mksubs		(dptr var,dptr val,word i,word j, dptr result);
word		mod3		(word a,word b);
word		mul		(word a,word b);
word		neg		(word a);
void		new_context	(int fsig, dptr cargp); /* w/o Coexpr: a stub */
int		numcmp		(dptr dp1,dptr dp2,dptr dp3);
void		outimage	(FILE *f,dptr dp,int noimage);
struct b_coexpr	*popact		(struct b_coexpr *ce);
word		prescan		(dptr d);
int		pstrnmcmp	(struct pstrnm *a,struct pstrnm *b);
int		pushact		(struct b_coexpr *ce, struct b_coexpr *actvtr);
int		putstr		(FILE *f,dptr d);
char		*qsearch	(char *key, char *base, int nel, int width,
				   int (*cmp)());
int		qtos		(dptr dp,char *sbuf);
int		 radix		(int sign, register int r, register char *s,
				   register char *end_s, union numeric *result);
char		*reserve	(int region, word nbytes);
void		retderef		(dptr valp, word *low, word *high);
void		segvtrap	(int);
void		stkdump		(int);
word		sub		(word a,word b);
void		syserr		(char *s);
struct b_coexpr	*topact		(struct b_coexpr *ce);
void		xmfree		(void);

#ifdef MultiThread
   void	resolve			(struct progstate *pstate);
   struct b_coexpr *loadicode (char *name, struct b_file *theInput,
      struct b_file *theOutput, struct b_file *theError,
      C_integer bs, C_integer ss, C_integer stk);
   void actparent (int eventcode);
   int mt_activate   (dptr tvalp, dptr rslt, struct b_coexpr *ncp);
#else					/* MultiThread */
   void	resolve			(void);
#endif					/* MultiThread */

#ifdef EventMon
   void EVAsgn	(dptr dx);
#endif					/* EventMon */

#ifdef ExternalFunctions
   dptr	extcall			(dptr x, int nargs, int *signal);
#endif					/* ExternalFunctions */

#ifdef LargeInts
   struct b_bignum *alcbignum	(word n);
   word		bigradix	(int sign, int r, char *s, char *x,
						   union numeric *result);
   double	bigtoreal	(dptr da);
   int		realtobig	(dptr da, dptr dx);
   int		bigtos		(dptr da, dptr dx);
   void		bigprint	(FILE *f, dptr da);
   int		cpbignum	(dptr da, dptr db);
   int		bigadd		(dptr da, dptr db, dptr dx);
   int		bigsub		(dptr da, dptr db, dptr dx);
   int		bigmul		(dptr da, dptr db, dptr dx);
   int		bigdiv		(dptr da, dptr db, dptr dx);
   int		bigmod		(dptr da, dptr db, dptr dx);
   int		bigneg		(dptr da, dptr dx);
   int		bigpow		(dptr da, dptr db, dptr dx);
   int		bigpowri        (double a, dptr db, dptr drslt);
   int		bigand		(dptr da, dptr db, dptr dx);
   int		bigor		(dptr da, dptr db, dptr dx);
   int		bigxor		(dptr da, dptr db, dptr dx);
   int		bigshift	(dptr da, dptr db, dptr dx);
   word		bigcmp		(dptr da, dptr db);
   int		bigrand		(dptr da, dptr dx);
#endif					/* LargeInts */

#ifdef FAttrib
   char *make_mode(mode_t st_mode);
#endif					/* FAttrib */

#ifdef Graphics
   /*
    * portable graphics routines in rwindow.r and rwinrsc.r
    */
   wcp	alc_context	(wbp w);
   wbp	alc_wbinding	(void);
   wsp	alc_winstate	(void);
   int	atobool		(char *s);
   void	c_push		(dptr l,dptr val);  /* in fstruct.r */
   int	docircles	(wbp w, int argc, dptr argv, int fill);
   void	drawCurve	(wbp w, XPoint *p, int n);
   char	*evquesub	(wbp w, int i);
   void	genCurve	(wbp w, XPoint *p, int n, void (*h)());
   wsp	getactivewindow	(void);
   int	getpattern	(wbp w, char *answer);
   struct palentry *palsetup(int p);
   int	palnum		(dptr d);
   int	parsecolor	(wbp w, char *s, long *r, long *g, long *b);
   int	parsefont	(char *s, char *fam, int *sty, int *sz);
   int	parsegeometry	(char *buf, SHORT *x, SHORT *y, SHORT *w, SHORT *h);
   int	parsepattern	(char *s, int len, int *w, int *nbits, C_integer *bits);
   void	qevent		(wsp ws, dptr e, int x, int y, uword t, long f);
   int	readGIF		(char *fname, int p, struct imgdata *d);
   int	rectargs	(wbp w, int argc, dptr argv, int i,
			   word *px, word *py, word *pw, word *ph);
   char	*rgbkey		(int p, double r, double g, double b);
   int	setsize		(wbp w, char *s);
   char	*si_i2s		(siptr sip, int i);
   int	si_s2i		(siptr sip, char *s);
   int	ulcmp		(pointer p1, pointer p2);
   int	wattrib		(wbp w, char *s, long len, dptr answer, char *abuf);
   int	wgetche		(wbp w, dptr res);
   int	wgetchne	(wbp w, dptr res);
   int	wgetevent	(wbp w, dptr res);
   int	wgetstrg	(char *s, long maxlen, FILE *f);
   void	wgoto		(wbp w, int row, int col);
   int	wlongread	(char *s, int elsize, int nelem, FILE *f);
   void	wputstr		(wbp w, char *s, int len);
   int	writeGIF	(wbp w, char *filename,
			  int x, int y, int width, int height);
   int	xyrowcol	(dptr dx);

   /*
    * graphics implementation routines supplied for each platform
    * (excluding those defined as macros for X-windows)
    */
   int	SetPattern	(wbp w, char *name, int len);
   int	SetPatternBits	(wbp w, int width, C_integer *bits, int nbits);
   int	allowresize	(wbp w, int on);
   int	blimage		(wbp w, int x, int y, int wd, int h,
			  int ch, unsigned char *s, word len);
   int	capture		(wbp w, int x, int y, int width, int hgt, short *data);
   wcp	clone_context	(wbp w);
   int	copyArea	(wbp w,wbp w2,int x,int y,int wd,int h,int x2,int y2);
   int	do_config	(wbp w, int status);
   int	dumpimage	(wbp w, char *filename, unsigned int x, unsigned int y,
			   unsigned int width, unsigned int height);
   void	eraseArea	(wbp w, int x, int y, int width, int height);
   void	fillrectangles	(wbp w, XRectangle *recs, int nrecs);
   void	free_binding	(wbp w);
   void	free_context	(wcp wc);
   void	free_mutable	(wbp w, int mute_index);
   int	free_window	(wsp ws);
   void	freecolor	(wbp w, char *s);
   char	*get_mutable_name (wbp w, int mute_index);
   void	getbg		(wbp w, char *answer);
   void	getcanvas	(wbp w, char *s);
   int	getdefault	(wbp w, char *prog, char *opt, char *answer);
   void	getdisplay	(wbp w, char *answer);
   void	getdrawop	(wbp w, char *answer);
   void	getfg		(wbp w, char *answer);
   void	getfntnam	(wbp w, char *answer);
   void	geticonic	(wbp w, char *answer);
   int	geticonpos	(wbp w, char *s);
   void	getlinestyle	(wbp w, char *answer);
   int	getpixel_init	(wbp w, struct imgmem *imem);
   int	getpixel_term	(wbp w, struct imgmem *imem);
   int	getpixel	(wbp w,int x,int y,long *rv,char *s,struct imgmem *im);
   void	getpointername	(wbp w, char *answer);
   int	getpos		(wbp w);
   int	getvisual	(wbp w, char *answer);
   int	isetbg		(wbp w, int bg);
   int	isetfg		(wbp w, int fg);
   int	lowerWindow	(wbp w);
   int	mutable_color	(wbp w, dptr argv, int ac, int *retval);
   int	nativecolor	(wbp w, char *s, long *r, long *g, long *b);

   int pollevent	(void);
   void wflush		(wbp w);

   int	query_pointer	(wbp w, XPoint *pp);
   int	query_rootpointer (XPoint *pp);
   int	raiseWindow	(wbp w);
   int	readimage	(wbp w, char *filename, int x, int y, int *status);
   int	rebind		(wbp w, wbp w2);
   int	set_mutable	(wbp w, int i, char *s);
   int	setbg		(wbp w, char *s);
   int	setcanvas	(wbp w, char *s);
   void	setclip		(wbp w);
   int	setcursor	(wbp w, int on);
   int	setdisplay	(wbp w, char *s);
   int	setdrawop	(wbp w, char *val);
   int	setfg		(wbp w, char *s);
   int	setfillstyle	(wbp w, char *s);
   int	setfont		(wbp w, char **s);
   int	setgamma	(wbp w, double gamma);
   int	setgeometry	(wbp w, char *geo);
   int	setheight	(wbp w, SHORT new_height);
   int	seticonicstate	(wbp w, char *s);
   int	seticonlabel	(wbp w, char *val);
   int	seticonpos	(wbp w, char *s);
   int	setimage	(wbp w, char *val);
   int	setleading	(wbp w, int i);
   int	setlinestyle	(wbp w, char *s);
   int	setlinewidth	(wbp w, LONG linewid);
   int	setpointer	(wbp w, char *val);
   int	setwidth	(wbp w, SHORT new_width);
   int	setwindowlabel	(wbp w, char *val);
   int	strimage	(wbp w, int x, int y, int width, int height,
			   struct palentry *e, unsigned char *s,
			   word len, int on_icon);
   void	toggle_fgbg	(wbp w);
   int	walert		(wbp w, int volume);
   void	warpPointer	(wbp w, int x, int y);
   int	wclose		(wbp w);
   void	wflush		(wbp w);
   int	wgetq		(wbp w, dptr res);
   FILE	*wopen		(char *nm, struct b_list *hp, dptr attr, int n, int *e);
   int	wputc		(int ci, wbp w);
   #ifndef wsync
   void	wsync		(wbp w);
   #endif				/* wsync */
   void	xdis		(wbp w, char *s, int n);

   #ifdef XWindows
      /*
       * Implementation routines specific to X-Windows
       */
      void	unsetclip		(wbp w);
      int	moveResizeWindow	(wbp w, int x, int y, int wd, int h);
      int	resetfg			(wbp w);
      int	setfgrgb		(wbp w, int r, int g, int b);
      int	setbgrgb		(wbp w, int r, int g, int b);

      XColor	xcolor			(wbp w, LinearColor clr);
      LinearColor	lcolor		(wbp w, XColor color);
      int	pixmap_open		(wbp w, dptr attribs, int argc);
      int	pixmap_init		(wbp w);
      int	remap			(wbp w, int x, int y);
      int	seticonimage		(wbp w, dptr dp);
      int	translate_key_event	(XKeyEvent *k1, char *s, KeySym *k2);
      wdp	alc_display		(char *s);
      void	free_display		(wdp wd);
      wfp	alc_font		(wbp w, char **s);
      wfp	tryfont			(wbp w, char *s);
      wclrp	alc_rgb			(wbp w, char *s, unsigned int r,
					   unsigned int g, unsigned int b,
					   int is_iconcolor);
      int	alc_centry		(wdp wd);
      wclrp	alc_color		(wbp w, char *s);
      void	copy_colors		(wbp w1, wbp w2);
      void	free_xcolor		(wbp w, unsigned long c);
      void	free_xcolors		(wbp w, int extent);
      int	go_virtual		(wbp w);
      int	resizePixmap		(wbp w, int width, int height);
      void	wflushall		(void);
   #endif				/* XWindows */

   #ifdef WinGraphics
      /*
       * Implementation routines specific to MS Windows
       */
      int playmedia		(wbp w, char *s);
      char *nativecolordialog	(wbp w,long r,long g, long b,char *s);
      int nativefontdialog	(wbp w, char *buf, int flags, int fheight);
      char *nativeopendialog	(wbp w,char *s1,char *s2,char *s3,int i,int j);
      char *nativeselectdialog	(wbp w,struct b_list *,char *s);
      char *nativesavedialog	(wbp w,char *s1,char *s2,char *s3,int i,int j);
      HFONT mkfont		(char *s);
      int sysTextWidth		(wbp w, char *s, int n);
      int sysFontHeight		(wbp w);
      int mswinsystem		(char *s);
      void UpdateCursorPos	(wsp ws, wcp wc);
      LRESULT_CALLBACK WndProc	(HWND, UINT, WPARAM, LPARAM);
      HDC CreateWinDC		(wbp);
      HDC CreatePixDC		(wbp, HDC);
      HBITMAP loadimage	(wbp wb, char *filename, unsigned int *width,
			unsigned int *height, int atorigin, int *status);
      void wfreersc();
      int getdepth(wbp w);
      HBITMAP CreateBitmapFromData(char *data);
      int resizePixmap(wbp w, int width, int height);
      int textWidth(wbp w, char *s, int n);
      int	seticonimage		(wbp w, dptr dp);
      int devicecaps(wbp w, int i);
      void fillarcs(wbp wb, XArc *arcs, int narcs);
      void drawarcs(wbp wb, XArc *arcs, int narcs);
      void drawlines(wbinding *wb, XPoint *points, int npoints);
      void drawpoints(wbinding *wb, XPoint *points, int npoints);
      void drawrectangles(wbp wb, XRectangle *recs, int nrecs);
      void fillpolygon(wbp w, XPoint *pts, int npts);
      void drawsegments(wbinding *wb, XSegment *segs, int nsegs);
      void drawstrng(wbinding *wb, int x, int y, char *s, int slen);
      void unsetclip(wbp w);

   #endif				/* WinGraphics */

#endif					/* Graphics */

/*
 * Prototypes for the run-time system.
 */

struct b_external *alcextrnl	(int n);
struct b_record *alcrecd	(int nflds,union block *recptr);
struct b_tvsubs *alcsubs	(word len,word pos,dptr var);
int	bfunc		(void);
long	ckadd		(long i, long j);
long	ckmul		(long i, long j);
long	cksub		(long i, long j);
void	cmd_line	(int argc, char **argv, dptr rslt);
struct b_coexpr *create	(continuation fnc,struct b_proc *p,int ntmp,int wksz);
int	collect		(int region);
void	cotrace		(struct b_coexpr *ccp, struct b_coexpr *ncp,
			   int swtch_typ, dptr valloc);
int	cvcset		(dptr dp,int * *cs,int *csbuf);
int	cvnum		(dptr dp,union numeric *result);
int	cvreal		(dptr dp,double *r);
void	deref		(dptr dp1, dptr dp2);
void	envset		(void);
int	eq		(dptr dp1,dptr dp2);
int	get_name	(dptr dp1, dptr dp2);
int	getch		(void);
int	getche		(void);
double	getdbl		(dptr dp);
int	getimage	(dptr dp1, dptr dp2);
int	getstrg		(char *buf, int maxi, struct b_file *fbp);
void	hgrow		(union block *bp);
void	hshrink		(union block *bp);
C_integer iipow		(C_integer n1, C_integer n2);
void	init		(char *name, int *argcp, char *argv[], int trc_init);
int	kbhit		(void);
int	mkreal		(double r,dptr dp);
int	nthcmp		(dptr d1,dptr d2);
void	nxttab		(C_integer *col, dptr *tablst, dptr endlst,
			   C_integer *last, C_integer *interval);
int	order		(dptr dp);
int	printable	(int c);
int	ripow		(double r, C_integer n, dptr rslt);
void	rtos		(double n,dptr dp,char *s);
int	sig_rsm		(void);
struct b_proc *strprc	(dptr s, C_integer arity);
int	subs_asgn	(dptr dest, const dptr src);
int	trcmp3		(struct dpair *dp1,struct dpair *dp2);
int	trefcmp		(dptr d1,dptr d2);
int	tvalcmp		(dptr d1,dptr d2);
int	tvcmp4		(struct dpair *dp1,struct dpair *dp2);
int	tvtbl_asgn	(dptr dest, const dptr src);
void	varargs		(dptr argp, int nargs, dptr rslt);

#ifdef MultiThread
   struct b_coexpr *alccoexp (long icodesize, long stacksize);
#else					/* MultiThread */
   struct b_coexpr *alccoexp (void);
#endif					/* MultiThread */

#if COMPILER

   struct b_refresh *alcrefresh	(int na, int nl, int nt, int wk_sz);
   void	atrace			(void);
   void	ctrace			(void);
   void	failtrace		(void);
   void	initalloc		(void);
   int	invoke			(int n, dptr args, dptr rslt, continuation c);
   void	rtrace			(void);
   void	strace			(void);
   void	tracebk			(struct p_frame *lcl_pfp, dptr argp);
   int	xdisp			(struct p_frame *fp, dptr dp, int n, FILE *f);

#else					/* COMPILER */

   struct b_refresh *alcrefresh	(word *e, int nl, int nt);
   void	atrace			(dptr dp);
   void	ctrace			(dptr dp, int nargs, dptr arg);
   void	failtrace		(dptr dp);
   int	invoke			(int nargs, dptr *cargs, int *n);
   void	rtrace			(dptr dp, dptr rval);
   void	strace			(dptr dp, dptr rval);
   void	tracebk			(struct pf_marker *lcl_pfp, dptr argp);
   int	xdisp			(struct pf_marker *fp, dptr dp, int n, FILE *f);

   #define Fargs dptr cargp
   int	Obscan			(int nargs, Fargs);
   int	Ocreate			(word *entryp, Fargs);
   int	Oescan			(int nargs, Fargs);
   int	Ofield			(int nargs, Fargs);
   int	Olimit			(int nargs, Fargs);
   int	Ollist			(int nargs, Fargs);
   int	Omkrec			(int nargs, Fargs);

   #ifdef MultiThread
      void	initalloc	(word codesize, struct progstate *p);
   #else				/* MultiThread */
      void	initalloc	(word codesize);
   #endif				/* MultiThread */

#endif					/* COMPILER */
