/*
 * File: rimage.c
 *  Functions and data for reading and writing GIF images
 */

#ifdef Graphics

#define GifSeparator	0x2C	/* (',') beginning of image */
#define GifTerminator	0x3B	/* (';') end of image */
#define GifExtension	0x21	/* ('!') extension block */
#define GifControlExt	0xF9	/*       graphic control extension label */
#define GifEmpty	-1	/* internal flag indicating no prefix */

#define GifTableSize	4096	/* maximum number of entries in table */
#define GifBlockSize	255	/* size of output block */

typedef struct lzwnode {	/* structure of LZW encoding tree node */
   unsigned short tcode;		/* token code */
   unsigned short child;	/* first child node */
   unsigned short sibling;	/* next sibling */
   } lzwnode;

static	int	gfread		(char *fn, int p);
static	int	gfheader	(FILE *f);
static	int	gfskip		(FILE *f);
static	void	gfcontrol	(FILE *f);
static	int	gfimhdr		(FILE *f);
static	int	gfmap		(FILE *f, int p);
static	int	gfsetup		(void);
static	int	gfrdata		(FILE *f);
static	int	gfrcode		(FILE *f);
static	void	gfinsert	(int prev, int c);
static	int	gffirst		(int c);
static	void	gfgen		(int c);
static	void	gfput		(int b);

static	int	gfwrite		(wbp w, char *filename,
				   int x, int y, int width, int height);
static	void	gfmktree	(lzwnode *tree);
static	void	gfout		(int tcode);
static	void	gfdump		(void);

static	int	medcut	(long hlist[], struct palentry plist[], int ncolors);

static FILE *gf_f;			/* input file */

static int gf_gcmap, gf_lcmap;		/* global color map? local color map? */
static int gf_nbits;			/* number of bits per pixel */
static int gf_ilace;			/* interlace flag */
static int gf_width, gf_height;		/* image size */

static short *gf_prefix, *gf_suffix;	/* prefix and suffix tables */
static int gf_free;			/* next free position */

static struct palentry *gf_paltbl;	/* palette table */
static unsigned char *gf_string;	/* incoming image data */
static short *gf_pixels;		/* outgoing image data */
static unsigned char *gf_nxt, *gf_lim;	/* store pointer and its limit */
static int gf_row, gf_step;		/* current row and step size */

static int gf_cdsize;			/* code size */
static int gf_clear, gf_eoi;		/* values of CLEAR and EOI codes */
static int gf_lzwbits, gf_lzwmask;	/* current bits per code */

static unsigned char *gf_obuf;		/* output buffer */
static unsigned long gf_curr;		/* current partial byte(s) */
static int gf_valid;			/* number of valid bits */
static int gf_rem;			/* remaining bytes in this block */

/*
 * readGIF(filename, p, imd) - read GIF file into image data structure
 *
 * p is a palette number to which the GIF colors are to be coerced;
 * p=0 uses the colors exactly as given in the GIF file.
 */
int readGIF(filename, p, imd)
char *filename;
int p;
struct imgdata *imd;
   {
   int r;

   r = gfread(filename, p);			/* read image */

   if (gf_prefix) free((pointer)gf_prefix);	/* deallocate temp memory */
   if (gf_suffix) free((pointer)gf_suffix);
   if (gf_f) fclose(gf_f);

   if (r != Succeeded) {			/* if no success, free mem */
      if (gf_paltbl) free((pointer) gf_paltbl);
      if (gf_string) free((pointer) gf_string);
      return r;					/* return Failed or Error */
      }

   imd->width = gf_width;			/* set return variables */
   imd->height = gf_height;
   imd->paltbl = gf_paltbl;
   imd->data = gf_string;

   return Succeeded;				/* return success */
   }

/*
 * gfread(filename, p) - read GIF file, setting gf_ globals
 */
static int gfread(filename, p)
char *filename;
int p;
   {
   int i;

   gf_f = NULL;
   gf_prefix = NULL;
   gf_suffix = NULL;
   gf_string = NULL;

   if (!(gf_paltbl = (struct palentry *)malloc(256 * sizeof(struct palentry))))
      return Failed;

   if ((gf_f = fopen(filename, "rb")) == NULL)
      return Failed;

   for (i = 0; i < 256; i++)		/* init palette table */
      gf_paltbl[i].used = gf_paltbl[i].valid = gf_paltbl[i].transpt = 0;

   if (!gfheader(gf_f))			/* read file header */
      return Failed;
   if (gf_gcmap)			/* read global color map, if any */
      if (!gfmap(gf_f, p))
         return Failed;
   if (!gfskip(gf_f))			/* skip to start of image */
      return Failed;
   if (!gfimhdr(gf_f))			/* read image header */
      return Failed;
   if (gf_lcmap)			/* read local color map, if any */
      if (!gfmap(gf_f, p))
         return Failed;
   if (!gfsetup())			/* prepare to read image */
      return Error;
   if (!gfrdata(gf_f))			/* read image data */
      return Failed;
   while (gf_row < gf_height)		/* pad if too short */
      gfput(0);

   return Succeeded;
   }

/*
 * gfheader(f) - read GIF file header; return nonzero if successful
 */
static int gfheader(f)
FILE *f;
   {
   unsigned char hdr[13];		/* size of a GIF header */
   int b;

   if (fread((char *)hdr, sizeof(char), sizeof(hdr), f) != sizeof(hdr))
      return 0;				/* header short or missing */
   if (strncmp((char *)hdr, "GIF", 3) != 0 ||
         !isdigit(hdr[3]) || !isdigit(hdr[4]))
      return 0;				/* not GIFnn */

   b = hdr[10];				/* flag byte */
   gf_gcmap = b & 0x80;			/* global color map flag */
   gf_nbits = (b & 7) + 1;		/* number of bits per pixel */
   return 1;
   }

/*
 * gfskip(f) - skip intermediate blocks and locate image
 */
static int gfskip(f)
FILE *f;
   {
   int c, n;

   while ((c = getc(f)) != GifSeparator) { /* look for start-of-image flag */
      if (c == EOF)
         return 0;
      if (c == GifExtension) {		/* if extension block is present */
         c = getc(f);				/* get label */
	 if ((c & 0xFF) == GifControlExt)
	    gfcontrol(f);			/* process control subblock */
         while ((n = getc(f)) != 0) {		/* read blks until empty one */
            if (n == EOF)
               return 0;
	    n &= 0xFF;				/* ensure positive count */
            while (n--)				/* skip block contents */
               getc(f);
            }
         }
      }
   return 1;
   }

/*
 * gfcontrol(f) - process control extension subblock
 */
static void gfcontrol(f)
FILE *f;
   {
   int i, n, c, t;

   n = getc(f) & 0xFF;				/* subblock length (s/b 4) */
   for (i = t = 0; i < n; i++) {
      c = getc(f) & 0xFF;
      if (i == 0)
	 t = c & 1;				/* transparency flag */
      else if (i == 3 && t != 0) {
	 gf_paltbl[c].transpt = 1;		/* set flag for transpt color */
	 gf_paltbl[c].valid = 0;		/* color is no longer "valid" */
	 }
      }
   }

/*
 * gfimhdr(f) - read image header
 */
static int gfimhdr(f)
FILE *f;
   {
   unsigned char hdr[9];		/* size of image hdr excl separator */
   int b;

   if (fread((char *)hdr, sizeof(char), sizeof(hdr), f) != sizeof(hdr))
      return 0;				/* header short or missing */
   gf_width = hdr[4] + 256 * hdr[5];
   gf_height = hdr[6] + 256 * hdr[7];
   b = hdr[8];				/* flag byte */
   gf_lcmap = b & 0x80;			/* local color map flag */
   gf_ilace = b & 0x40;			/* interlace flag */
   if (gf_lcmap)
      gf_nbits = (b & 7) + 1;		/* if local map, reset nbits also */
   return 1;
   }

/*
 * gfmap(f, p) - read GIF color map into paltbl under control of palette p
 */
static int gfmap(f, p)
FILE *f;
int p;
   {
   int ncolors, i, r, g, b, c;
   struct palentry *stdpal = 0;

   if (p)
      stdpal = palsetup(p);

   ncolors = 1 << gf_nbits;

   for (i = 0; i < ncolors; i++) {
      r = getc(f);
      g = getc(f);
      b = getc(f);
      if (r == EOF || g == EOF || b == EOF)
         return 0;
      if (p) {
         c = *(unsigned char *)(rgbkey(p, r / 255.0, g / 255.0, b / 255.0));
         gf_paltbl[i].clr = stdpal[c].clr;
         }
      else {
         gf_paltbl[i].clr.red   = 257 * r;	/* 257 * 255 -> 65535 */
         gf_paltbl[i].clr.green = 257 * g;
         gf_paltbl[i].clr.blue  = 257 * b;
         }
      if (!gf_paltbl[i].transpt)		/* if not transparent color */
         gf_paltbl[i].valid = 1;		/* mark as valid/opaque */
      }

   return 1;
   }

/*
 * gfsetup() - prepare to read GIF data
 */
static int gfsetup()
   {
   int i;
   word len;

   len = (word)gf_width * (word)gf_height;
   gf_string = (unsigned char *)malloc(len);
   gf_prefix = (short *)malloc(GifTableSize * sizeof(short));
   gf_suffix = (short *)malloc(GifTableSize * sizeof(short));
   if (!gf_string || !gf_prefix || !gf_suffix)
      return 0;
   for (i = 0; i < GifTableSize; i++) {
      gf_prefix[i] = GifEmpty;
      gf_suffix[i] = i;
      }

   gf_row = 0;				/* current row is 0 */
   gf_nxt = gf_string;			/* set store pointer */

   if (gf_ilace) {			/* if interlaced */
      gf_step = 8;			/* step rows by 8 */
      gf_lim = gf_string + gf_width;	/* stop at end of one row */
      }
   else {
      gf_lim = gf_string + len;		/* do whole image at once */
      gf_step = gf_height;		/* step to end when full */
      }

   return 1;
   }

/*
 * gfrdata(f) - read GIF data
 */
static int gfrdata(f)
FILE *f;
   {
   int curr, prev, c;

   if ((gf_cdsize = getc(f)) == EOF)
      return 0;
   gf_clear = 1 << gf_cdsize;
   gf_eoi = gf_clear + 1;
   gf_free = gf_eoi + 1;

   gf_lzwbits = gf_cdsize + 1;
   gf_lzwmask = (1 << gf_lzwbits) - 1;

   gf_curr = 0;
   gf_valid = 0;
   gf_rem = 0;

   prev = curr = gfrcode(f);
   while (curr != gf_eoi) {
      if (curr == gf_clear) {		/* if reset code */
         gf_lzwbits = gf_cdsize + 1;
         gf_lzwmask = (1 << gf_lzwbits) - 1;
         gf_free = gf_eoi + 1;
         prev = curr = gfrcode(f);
         gfgen(curr);
         }
      else if (curr < gf_free) {	/* if code is in table */
         gfgen(curr);
         gfinsert(prev, gffirst(curr));
         prev = curr;
         }
      else if (curr == gf_free) {	/* not yet in table */
         c = gffirst(prev);
         gfgen(prev);
         gfput(c);
         gfinsert(prev, c);
         prev = curr;
         }
      else {				/* illegal code */
         if (gf_nxt == gf_lim)
            return 1;			/* assume just extra stuff after end */
         else
            return 0;			/* more badly confused */
         }
      curr = gfrcode(f);
      }

   return 1;
   }

/*
 * gfrcode(f) - read next LZW code
 */
static int gfrcode(f)
FILE *f;
   {
   int c, r;

   while (gf_valid < gf_lzwbits) {
      if (--gf_rem <= 0) {
         if ((gf_rem = getc(f)) == EOF)
            return gf_eoi;
         }
      if ((c = getc(f)) == EOF)
         return gf_eoi;
      gf_curr |= ((c & 0xFF) << gf_valid);
      gf_valid += 8;
      }
   r = gf_curr & gf_lzwmask;
   gf_curr >>= gf_lzwbits;
   gf_valid -= gf_lzwbits;
   return r;
   }

/*
 * gfinsert(prev, c) - insert into table
 */
static void gfinsert(prev, c)
int prev, c;
   {

   if (gf_free >= GifTableSize)		/* sanity check */
      return;

   gf_prefix[gf_free] = prev;
   gf_suffix[gf_free] = c;

   /* increase code size if code bits are exhausted, up to max of 12 bits */
   if (++gf_free > gf_lzwmask && gf_lzwbits < 12) {
      gf_lzwmask = gf_lzwmask * 2 + 1;
      gf_lzwbits++;
      }

   }

/*
 * gffirst(c) - return the first pixel in a map structure
 */
static int gffirst(c)
int c;
   {
   int d;

   if (c >= gf_free)
      return 0;				/* not in table (error) */
   while ((d = gf_prefix[c]) != GifEmpty)
      c = d;
   return gf_suffix[c];
   }

/*
 * gfgen(c) - generate and output prefix
 */
static void gfgen(c)
int c;
   {
   int d;

   if ((d = gf_prefix[c]) != GifEmpty)
      gfgen(d);
   gfput(gf_suffix[c]);
   }

/*
 * gfput(b) - add a byte to the output string
 */
static void gfput(b)
int b;
   {
   if (gf_nxt >= gf_lim) {		/* if current row is full */
      gf_row += gf_step;
      while (gf_row >= gf_height && gf_ilace && gf_step > 2) {
         if (gf_step == 4) {
            gf_row = 1;
            gf_step = 2;
            }
	 else if ((gf_row % 8) != 0) {
            gf_row = 2;
            gf_step = 4;
            }
         else {
            gf_row = 4;
	    /* gf_step remains 8 */
	    }
	 }

      if (gf_row >= gf_height) {
	 gf_step = 0;
	 return;			/* too much data; ignore it */
	 }
      gf_nxt = gf_string + ((word)gf_row * (word)gf_width);
      gf_lim = gf_nxt + gf_width;
      }

   *gf_nxt++ = b;			/* store byte */
   gf_paltbl[b].used = 1;		/* mark color entry as used */
   }

/*
 * writeGIF(w, filename, x, y, width, height) - write GIF image
 *
 * Returns Succeeded, Failed, or Error.
 * We assume that the area specified is within the window.
 */
int writeGIF(w, filename, x, y, width, height)
wbp w;
char *filename;
int x, y, width, height;
   {
   int r;

   r = gfwrite(w, filename, x, y, width, height);
   if (gf_f) fclose(gf_f);
   if (gf_pixels) free((pointer)gf_pixels);
   return r;
   }

/*
 * gfwrite(w, filename, x, y, width, height) - write GIF file
 *
 * We write GIF87a format (not 89a) for maximum acceptability and because
 * we don't need any of the extensions of GIF89.
 */

static int gfwrite(w, filename, x, y, width, height)
wbp w;
char *filename;
int x, y, width, height;
   {
   unsigned char obuf[GifBlockSize];
   short *p, *q;
   int i, c, cur, nc;
   long h, npixels, hlist[1<<15];
   LinearColor *cp;
   struct palentry paltbl[GIFMAX];
   lzwnode tree[GifTableSize + 1];

   npixels = (long)width * (long)height;	/* total length of data */

   if (!(gf_f = fopen(filename, "wb")))
      return Failed;
   if (!(gf_pixels = malloc(npixels * sizeof(short))))
      return Error;

   if (!capture(w, x, y, width, height, gf_pixels))	/* get data (rgb15) */
      return Error;

   memset(hlist, 0, sizeof(hlist));
   for (h = 0; h < npixels; h++)	/* make histogram */
      hlist[gf_pixels[h]]++;

   nc = medcut(hlist, paltbl, GIFMAX);	/* make palette using median cut alg */
   if (nc == 0)
      return Error;

   gf_nbits = 1;			/* figure out gif bits for nc colors */
   while ((1 << gf_nbits) < nc)
      gf_nbits++;
   if (gf_nbits < 2)
      gf_cdsize = 2;
   else
      gf_cdsize = gf_nbits;

   gf_clear = 1 << gf_cdsize;		/* set encoding variables */
   gf_eoi = gf_clear + 1;
   gf_free = gf_eoi + 1;
   gf_lzwbits = gf_cdsize + 1;

   /*
    * Write the header, global color table, and image descriptor.
    */

   fprintf(gf_f, "GIF87a%c%c%c%c%c%c%c", width, width >> 8, height, height >> 8,
      0x80 | ((gf_nbits - 1) << 4) | (gf_nbits - 1), 0, 0);

   for (i = 0; i < (1 << gf_nbits); i++) {	/* output color table */
      if (i < GIFMAX && i < nc) {
         cp = &paltbl[i].clr;
         putc(cp->red >> 8, gf_f);
         putc(cp->green >> 8, gf_f);
         putc(cp->blue >> 8, gf_f);
         }
      else {
         putc(0, gf_f);
         putc(0, gf_f);
         putc(0, gf_f);
         }
      }

   fprintf(gf_f, "%c%c%c%c%c%c%c%c%c%c%c", GifSeparator, 0, 0, 0, 0,
      width, width >> 8, height, height >> 8, gf_nbits - 1, gf_cdsize);

   /*
    * Encode and write the image.
    */
   gf_obuf = obuf;			/* initialize output state */
   gf_curr = 0;
   gf_valid = 0;
   gf_rem = GifBlockSize;

   gfmktree(tree);			/* initialize encoding tree */

   gfout(gf_clear);			/* start with CLEAR code */

   p = gf_pixels;
   q = p + npixels;
   cur = hlist[*p++];			/* first pixel is special */
   while (p < q) {
      c = hlist[*p++];			/* get code */
      for (i = tree[cur].child; i != 0; i = tree[i].sibling)
         if (tree[i].tcode == c)	/* find as suffix of previous string */
            break;
      if (i != 0) {			/* if found in encoding tree */
         cur = i;			/* note where */
         continue;			/* and accumulate more */
         }
      gfout(cur);			/* new combination -- output prefix */
      tree[gf_free].tcode = c;		/* make node for new combination */
      tree[gf_free].child = 0;
      tree[gf_free].sibling = tree[cur].child;
      tree[cur].child = gf_free;
      cur = c;				/* restart string from single pixel */
      ++gf_free;			/* grow tree to account for new node */
      if (gf_free > (1 << gf_lzwbits)) {
         if (gf_free > GifTableSize) {
            gfout(gf_clear);		/* table is full; reset to empty */
            gf_lzwbits = gf_cdsize + 1;
            gfmktree(tree);
            }
         else
            gf_lzwbits++;		/* time to make output one bit wider */
         }
      }

   /*
    * Finish up.
    */
   gfout(cur);				/* flush accumulated prefix */
   gfout(gf_eoi);			/* send EOI code */
   gf_lzwbits = 7;
   gfout(0);				/* force out last partial byte */
   gfdump();				/* dump final block */
   putc(0, gf_f);			/* terminate image (block of size 0) */
   putc(GifTerminator, gf_f);		/* terminate file */

   fflush(gf_f);
   if (ferror(gf_f))
      return Failed;
   else
      return Succeeded;			/* caller will close file */
   }

/*
 * gfmktree() - initialize or reinitialize encoding tree
 */

static void gfmktree(tree)
lzwnode *tree;
   {
   int i;

   for (i = 0; i < gf_clear; i++) {	/* for each basic entry */
      tree[i].tcode = i;			/* code is pixel value */
      tree[i].child = 0;		/* no suffixes yet */
      tree[i].sibling = i + 1;		/* next code is sibling */
      }
   tree[gf_clear - 1].sibling = 0;	/* last entry has no sibling */
   gf_free = gf_eoi + 1;		/* reset next free entry */
   }

/*
 * gfout(code) - output one LZW token
 */
static void gfout(tcode)
int tcode;
   {
   gf_curr |= tcode << gf_valid;		/* add to current word */
   gf_valid += gf_lzwbits;		/* count the bits */
   while (gf_valid >= 8) {		/* while we have a byte to output */
      gf_obuf[GifBlockSize - gf_rem] = gf_curr;	/* put in buffer */
      gf_curr >>= 8;				/* remove from word */
      gf_valid -= 8;
      if (--gf_rem == 0)			/* flush buffer when full */
         gfdump();
      }
   }

/*
 * gfdump() - dump output buffer
 */
static void gfdump()
   {
   int n;

   n = GifBlockSize - gf_rem;
   putc(n, gf_f);			/* write block size */
   fwrite((pointer)gf_obuf, 1, n, gf_f); /*write block */
   gf_rem = GifBlockSize;		/* reset buffer to empty */
   }

/*
 * Median cut quantization code, based on the classic algorithm from
 *	Color Image Quantization for Frame Buffer Display
 *	Paul Heckbert
 *	SIGGRAPH '82, July 1982 (vol 16 no 3), pp297-307
 */

typedef struct box {	/* 3-D RGB region for median cut algorithm */
   struct box *next;		/* next box in chain */
   long count;			/* number of occurrences in this region */
   char maxaxis;		/* indication of longest axis */
   char maxdim;			/* length along longest axis */
   char cutpt;			/* cut point along that axis */
   char rmin, gmin, bmin;	/* minimum r, g, b values (5-bit color) */
   char rmax, gmax, bmax;	/* maximum r, g, b values (5-bit color) */
   } box;

#define MC_QUANT 5		/* quantize colors to 5 bits for median cut */
#define MC_MAXC ((1 << MC_QUANT) - 1)	/* so the maximum color value is 31 */

#define MC_RED (2 * MC_QUANT)	/* red shift */
#define MC_GRN (1 * MC_QUANT)	/* green shift */
#define MC_BLU (0 * MC_QUANT)	/* blue shift */

static void mc_shrink(box *bx);
static void mc_cut(box *bx);
static void mc_setcolor(box *bx, struct palentry *pe, int i);
static void mc_median(box *bx, int axis, long counts[], int min, int max);
static void mc_remove(box *bx);
static void mc_insert(box *bx);

static long *mc_hlist;		/* current histogram list */
static box *mc_blist;		/* current box list */
static int mc_nboxes = 0;	/* number of boxes allocated so far */

static box *mc_bfirst;		/* first box on linked list */

/*
 * medcut(hlist, plist, n) -- perform median-cut color quantization.
 *
 * On entry, hlist is a histogram of 32768 entries (5-bit color),
 *  plist is an array of n palentry structs to be filled in,
 *  and n is the number of colors desired in the result.
 *
 * On exit, up to n entries in plist have been filled in, and each
 *  hlist entry is an index into plist for the corresponding color.
 *
 * medcut returns the number of entries actually used.
 *  This is usually n if the histogram has that many nonzero entries.
 *  A return code of 0 indicates an allocation failure.
 */
int medcut(long hlist[], struct palentry plist[], int ncolors) {
   box *bx;
   int i;

   if ((mc_blist = malloc(ncolors * sizeof(box))) == NULL)
      return 0;
   mc_nboxes = 0;
   mc_hlist = hlist;

   bx = &mc_blist[mc_nboxes++];		/* create initial box */
   bx->next = NULL;
   bx->rmin = bx->gmin = bx->bmin = 0;
   bx->rmax = bx->gmax = bx->bmax = 31;
   mc_shrink(bx);			/* set box statistics */
   mc_bfirst = bx;			/* put as first and only box on chain */

   while (mc_nboxes < ncolors && mc_bfirst->maxdim > 1)
      mc_cut(mc_bfirst);		/* split box with longest dimension */

   for (i = 0; i < mc_nboxes; i++)		/* for every box created */
      mc_setcolor(&mc_blist[i], &plist[i], i);	/* set palette entry */

   free(mc_blist);
   return mc_nboxes;
   }

/*
 * mc_shrink(bx) -- shrink a box to tightly enclose its contents.
 *
 *  Adjusts rmin, gmin, bmin, rmax, gmax, bmax. 
 *  Calculates count, maxaxis, maxdim, and cutpt
 *  (while the necessary statistics are handy).
 */
static void mc_shrink(box *bx) {
   int i, n, r, g, b, t, dr, dg, db;
   long rcounts[MC_MAXC+1], gcounts[MC_MAXC+1], bcounts[MC_MAXC+1];

   memset(rcounts, 0, (MC_MAXC + 1) * sizeof(long));
   memset(gcounts, 0, (MC_MAXC + 1) * sizeof(long));
   memset(bcounts, 0, (MC_MAXC + 1) * sizeof(long));

   /*
    * Simultaneously count cross-sections along r, g, and b axes.
    */
   t = n = 0;
   for (r = bx->rmin; r <= bx->rmax; r++) {
      for (g = bx->gmin; g <= bx->gmax; g++) {
         for (b = bx->bmin; b <= bx->bmax; b++) {
            i = (r << MC_RED) + (g << MC_GRN) + (b << MC_BLU);
            n = mc_hlist[i];
            t += n;
            rcounts[r] += n;
            gcounts[g] += n;
            bcounts[b] += n;
            }
         }
      }
   bx->count = t;

   /*
    * Adjust min/mas bounds to tightly enclose the data we found.
    */
   while (rcounts[bx->rmin] == 0)  bx->rmin++;
   while (rcounts[bx->rmax] == 0)  bx->rmax--;
   while (gcounts[bx->gmin] == 0)  bx->gmin++;
   while (gcounts[bx->gmax] == 0)  bx->gmax--;
   while (bcounts[bx->bmin] == 0)  bx->bmin++;
   while (bcounts[bx->bmax] == 0)  bx->bmax--;

   /*
    * Find and record the axis of longest dimension.
    */
   dr = bx->rmax - bx->rmin;
   dg = bx->gmax - bx->gmin;
   db = bx->bmax - bx->bmin;
   if (db > dg && db > dr)
      mc_median(bx, MC_BLU, bcounts, bx->bmin, bx->bmax);
   else if (dr > dg)
      mc_median(bx, MC_RED, rcounts, bx->rmin, bx->rmax);
   else
      mc_median(bx, MC_GRN, gcounts, bx->gmin, bx->gmax);
   }

/*
 * mc_median(bx, axis, counts, cmin, cmax) -- find median and set box values.
 */
static void mc_median(box *bx, int axis, long counts[], int cmin, int cmax) {
   int lower, upper;

   bx->maxaxis = axis;
   bx->maxdim = cmax - cmin + 1;
   lower = counts[cmin];
   upper = counts[cmax];

   /*
    * Approach from both ends to find the median bin.
    */
   while (cmin < cmax) {
      if (lower < upper)
         lower += counts[++cmin];
      else
         upper += counts[--cmax];
      }

   /*
    * Have counted the median bin in both upper and lower halves.
    *  Remove it from the larger of those two.
    */
   if (lower < upper)
      upper -= counts[cmax++];
   else
      lower -= counts[cmin--];

   bx->cutpt = cmax;
   bx->count = lower + upper;
   }

/*
 * mc_cut(bx) -- split box at previously recorded cutpoint.
 */
static void mc_cut(box *b1) {
   box *b2;

   mc_remove(b1);			/* unlink box */
   b2 = &mc_blist[mc_nboxes++];		/* allocate new box */
   *b2 = *b1;				/* duplicate the contents */

   switch (b1->maxaxis) {
      case MC_RED:  b1->rmax = b1->cutpt - 1;  b2->rmin = b2->cutpt;  break;
      case MC_GRN:  b1->gmax = b1->cutpt - 1;  b2->gmin = b2->cutpt;  break;
      case MC_BLU:  b1->bmax = b1->cutpt - 1;  b2->bmin = b2->cutpt;  break;
      }
   mc_shrink(b1);			/* recomputes box statistics */
   mc_shrink(b2);

   mc_insert(b1);			/* put both boxes back on list */
   mc_insert(b2);
   }

/*
 * mc_remove(bx) -- remove box from global linked list.
 *
 *  This is fast in practice because we always remove the first entry.
 */
static void mc_remove(box *bx) {
   box **bp;

   for (bp = &mc_bfirst; *bp != NULL; bp = &(*bp)->next) {
      if (*bp == bx) {
         *bp = bx->next;
         return;
         }
      }
   }

/*
 * mc_insert(bx) -- insert box in list, preserving decreasing maxdim ordering.
 */
static void mc_insert(box *bx) {
   box **bp;

   for (bp = &mc_bfirst; *bp != NULL; bp = &(*bp)->next) {
      if (bx->maxdim > (*bp)->maxdim
      || (bx->maxdim == (*bp)->maxdim && bx->count >= (*bp)->count)) 
	 break;
      }
   bx->next = *bp;
   *bp = bx;
   }

/*
 * mc_setcolor(bx, pe, i) -- set palette entry to box color.
 *
 *  Also sets the associated hlist entries to i, the palette index.
 */
static void mc_setcolor(box *bx, struct palentry *pe, int i) {
   int j, r, g, b;
   long n, t = 0, rtotal = 0, gtotal = 0, btotal = 0;

   /*
    * Calculate a weighted sum of the colors in the box.
    */
   for (r = bx->rmin; r <= bx->rmax; r++) {
      for (g = bx->gmin; g <= bx->gmax; g++) {
         for (b = bx->bmin; b <= bx->bmax; b++) {
            j = (r << MC_RED) + (g << MC_GRN) + (b << MC_BLU);
            n = mc_hlist[j];
            t += n;
            rtotal += n * r;
            gtotal += n * g;
            btotal += n * b;
            mc_hlist[j] = i;
            }
         }
      }

   /*
    * Scale colors using floating arithmetic to avoid overflow.
    */
   pe->clr.red = (65535. / MC_MAXC) * rtotal / t + 0.5;
   pe->clr.green = (65535. / MC_MAXC) * gtotal / t + 0.5;
   pe->clr.blue = (65535. / MC_MAXC) * btotal / t + 0.5;
   pe->used = 1;
   pe->valid = 1;
   pe->transpt = 0;
   }

#endif					/* Graphics */
