/* ppmtoxpm.c - read a portable pixmap and produce a (version 3) X11 pixmap
**
** Copyright (C) 1990 by Mark W. Snitily
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** This tool was developed for Schlumberger Technologies, ATE Division, and
** with their permission is being made available to the public with the above
** copyright notice and permission notice.
**
** Upgraded to XPM2 by
**   Paul Breslaw, Mecasoft SA, Zurich, Switzerland (paul@mecazh.uu.ch)
**   Thu Nov  8 16:01:17 1990
**
** Upgraded to XPM version 3 by
**   Arnaud Le Hors (lehors@mirsa.inria.fr)
**   Tue Apr 9 1991
**
** Rainer Sinkwitz sinkwitz@ifi.unizh.ch - 21 Nov 91:
**  - Bug fix, should should malloc space for rgbn[j].name+1 in line 441
**    caused segmentation faults
**    
**  - lowercase conversion of RGB names def'ed out,
**    considered harmful.
*/

#include <stdio.h>
#include <ctype.h>
#include "ppm.h"
#include "ppmcmap.h"

#if defined(SYSV) || defined(SVR4)
#include <string.h>
#ifndef index
#define index strchr
#endif
#else					/* SYSV */
#include <strings.h>
#endif					/* SYSV */

/* Max number of colors allowed in ppm input. */
#define MAXCOLORS    256

/* Max number of rgb mnemonics allowed in rgb text file. */
#define MAX_RGBNAMES 1024

/* Lower bound and upper bound of character-pixels printed in XPM output.
   Be careful, don't want the character '"' in this range. */
/*#define LOW_CHAR  '#'  <-- minimum ascii character allowed */
/*#define HIGH_CHAR '~'  <-- maximum ascii character allowed */
#define LOW_CHAR  '`'
#define HIGH_CHAR 'z'

#define max(a,b) ((a) > (b) ? (a) : (b))

void read_rgb_names();			/* forward reference */
void gen_cmap();			/* forward reference */

typedef struct {			/* rgb values and ascii names (from
					 * rgb text file) */
    int r, g, b;			/* rgb values, range of 0 -> 65535 */
    char *name;				/* color mnemonic of rgb value */
}      rgb_names;

typedef struct {			/* character-pixel mapping */
    char *cixel;			/* character string printed for
					 * pixel */
    char *rgbname;			/* ascii rgb color, either color
					 * mnemonic or #rgb value */
}      cixel_map;

pixel **pixels;

main(argc, argv)
    int argc;
    char *argv[];

{
    FILE *ifd;
    register pixel *pP;
    int argn, rows, cols, ncolors, row, col, i;
    pixval maxval;			/* pixval == unsigned char or
					 * unsigned short */
    colorhash_table cht;
    colorhist_vector chv;

    /* Used for rgb value -> rgb mnemonic mapping */
    int map_rgb_names = 0;
    rgb_names rgbn[MAX_RGBNAMES];
    int rgbn_max;

    /* Used for rgb value -> character-pixel string mapping */
    cixel_map cmap[MAXCOLORS];
    int charspp;			/* chars per pixel */

    char out_name[100], rgb_fname[100], *cp;
    char *usage = "[-name <xpm-name>] [-rgb <rgb-textfile>] [ppmfile]";

    ppm_init(&argc, argv);
    out_name[0] = rgb_fname[0] = '\0';

    argn = 1;

    /* Check for command line options. */
    while (argn < argc && argv[argn][0] == '-') {

	/* Case "-", use stdin for input. */
	if (argv[argn][1] == '\0')
	    break;

	/* Case "-name <xpm-filename>", get output filename. */
	if (strncmp(argv[argn], "-name", max(strlen(argv[argn]), 2)) == 0) {
	    argn++;
	    if (argn == argc || sscanf(argv[argn], "%s", out_name) != 1)
		pm_usage(usage);
	}
	/* Case "-rgb <rgb-filename>", get rgb mnemonics filename. */
	else if (strncmp(argv[argn], "-rgb", max(strlen(argv[argn]), 2)) == 0) {
	    argn++;
	    if (argn == argc || sscanf(argv[argn], "%s", rgb_fname) != 1)
		pm_usage(usage);
	    map_rgb_names = 1;
	}
	/* Nothing else allowed... */
	else
	    pm_usage(usage);

	argn++;
    }

    /* Input file specified, open it and set output filename if necessary. */
    if (argn < argc) {

	/* Open the input file. */
	ifd = pm_openr(argv[argn]);

	/* If output filename not specified, use input filename as default. */
	if (out_name[0] == '\0') {
	    strcpy(out_name, argv[argn]);
	    if (cp = index(out_name, '.'))
		*cp = '\0';		/* remove extension */
	}

	/*
	 * If (1) input file was specified as "-" we're using stdin, or (2)
	 * output filename was specified as "-", set output filename to the
	 * default. 
	 */
	if (!strcmp(out_name, "-"))
	    strcpy(out_name, "noname");

	argn++;
    }
    /* No input file specified.  Using stdin so set default output filename. */
    else {
	ifd = stdin;
	if (out_name[0] == '\0')
	    strcpy(out_name, "noname");
    }

    /* Only 0 or 1 input files allowed. */
    if (argn != argc)
	pm_usage(usage);

    /*
     * "maxval" is the largest value that can be be found in the ppm file.
     * All pixel components are relative to this value. 
     */
    pixels = ppm_readppm(ifd, &cols, &rows, &maxval);
    pm_close(ifd);

    /* Figure out the colormap. */
    fprintf(stderr, "(Computing colormap...");
    fflush(stderr);
    chv = ppm_computecolorhist(pixels, cols, rows, MAXCOLORS, &ncolors);
    if (chv == (colorhist_vector) 0)
	pm_error(
	 "too many colors - try running the pixmap through 'ppmquant 256'",
		 0, 0, 0, 0, 0);
    fprintf(stderr, "  Done.  %d colors found.)\n", ncolors);

    /* Make a hash table for fast color lookup. */
    cht = ppm_colorhisttocolorhash(chv, ncolors);

    /*
     * If a rgb text file was specified, read in the rgb mnemonics. Does not
     * return if fatal error occurs. 
     */
    if (map_rgb_names)
	read_rgb_names(rgb_fname, rgbn, &rgbn_max);

    /* Now generate the character-pixel colormap table. */
    gen_cmap(chv, ncolors, maxval, map_rgb_names, rgbn, rgbn_max,
	     cmap, &charspp);

    /* Write out the XPM file. */

    printf("/* XPM */\n");
    printf("static char *%s[] = {\n", out_name);
    printf("/* width height ncolors chars_per_pixel */\n");
    printf("\"%d %d %d %d\",\n", cols, rows, ncolors, charspp);
    printf("/* colors */\n");
    for (i = 0; i < ncolors; i++) {
	printf("\"%s c %s\",\n", cmap[i].cixel, cmap[i].rgbname);
    }
    printf("/* pixels */\n");
    for (row = 0; row < rows; row++) {
	printf("\"");
	for (col = 0, pP = pixels[row]; col < cols; col++, pP++) {
	    printf("%s", cmap[ppm_lookupcolor(cht, pP)].cixel);
	}
	printf("\"%s\n", (row == (rows - 1) ? "" : ","));
    }
    printf("};\n");

    exit(0);

}					/* main */

/*---------------------------------------------------------------------------*/
/* This routine reads a rgb text file.  It stores the rgb values (0->65535)
   and the rgb mnemonics (malloc'ed) into the "rgbn" array.  Returns the
   number of entries stored in "rgbn_max". */
void
read_rgb_names(rgb_fname, rgbn, rgbn_max)
    char *rgb_fname;
    rgb_names rgbn[MAX_RGBNAMES];
int *rgbn_max;

{
    FILE *rgbf;
    int i, items, red, green, blue;
    char line[512], name[512], *rgbname, *n, *m;

    /* Open the rgb text file.  Abort if error. */
    if ((rgbf = fopen(rgb_fname, "r")) == NULL)
	pm_error("error opening rgb text file \"%s\"", rgb_fname, 0, 0, 0, 0);

    /* Loop reading each line in the file. */
    for (i = 0; fgets(line, sizeof(line), rgbf); i++) {

	/* Quit if rgb text file is too large. */
	if (i == MAX_RGBNAMES) {
	    fprintf(stderr,
	    "Too many entries in rgb text file, truncated to %d entries.\n",
		    MAX_RGBNAMES);
	    fflush(stderr);
	    break;
	}
	/* Read the line.  Skip if bad. */
	items = sscanf(line, "%d %d %d %[^\n]\n", &red, &green, &blue, name);
	if (items != 4) {
	    fprintf(stderr, "rgb text file syntax error on line %d\n", i + 1);
	    fflush(stderr);
	    i--;
	    continue;
	}
	/* Make sure rgb values are within 0->255 range.  Skip if bad. */
	if (red < 0 || red > 0xFF ||
	    green < 0 || green > 0xFF ||
	    blue < 0 || blue > 0xFF) {
	    fprintf(stderr, "rgb value for \"%s\" out of range, ignoring it\n",
		    name);
	    fflush(stderr);
	    i--;
	    continue;
	}
	/* Allocate memory for ascii name.  Abort if error. */
	if (!(rgbname = (char *) malloc(strlen(name) + 1)))
	    pm_error("out of memory allocating rgb name", 0, 0, 0, 0, 0);
	    
#ifdef NAMESLOWCASE
	/* Copy string to ascii name and lowercase it. */
	for (n = name, m = rgbname; *n; n++)
	    *m++ = isupper(*n) ? tolower(*n) : *n;
	*m = '\0';
#else
	strcpy(rgbname, name);
#endif

	/* Save the rgb values and ascii name in the array. */
	rgbn[i].r = red << 8;
	rgbn[i].g = green << 8;
	rgbn[i].b = blue << 8;
	rgbn[i].name = rgbname;
    }

    /* Return the max number of rgb names. */
    *rgbn_max = i - 1;

    fclose(rgbf);

}					/* read_rgb_names */

/*---------------------------------------------------------------------------*/
/* Given a number and a base, (base == HIGH_CHAR-LOW_CHAR+1), this routine
   prints the number into a malloc'ed string and returns it.  The length of
   the string is specified by "digits".  The ascii characters of the printed
   number range from LOW_CHAR to HIGH_CHAR.  The string is LOW_CHAR filled,
   (e.g. if LOW_CHAR==0, HIGH_CHAR==1, digits==5, i=3, routine would return
   the malloc'ed string "00011"). */
char *
gen_numstr(i, base, digits)
    int i, base, digits;
{
    char *str, *p;
    int d;

    /* Allocate memory for printed number.  Abort if error. */
    if (!(str = (char *) malloc(digits + 1)))
	pm_error("out of memory", 0, 0, 0, 0, 0);

    /* Generate characters starting with least significant digit. */
    p = str + digits;
    *p-- = '\0';			/* nul terminate string */
    while (p >= str) {
	d = i % base;
	i /= base;
	*p-- = (char) ((int) LOW_CHAR + d);
    }

    return str;

}					/* gen_numstr */

/*---------------------------------------------------------------------------*/
/* This routine generates the character-pixel colormap table. */
void
gen_cmap(chv, ncolors, maxval, map_rgb_names, rgbn, rgbn_max,
	 cmap, charspp)
/* input: */
    colorhist_vector chv;		/* contains rgb values for colormap */
    int ncolors;			/* number of entries in colormap */
    pixval maxval;			/* largest color value, all rgb
					 * values relative to this, (pixval
					 * == unsigned short) */
    int map_rgb_names;			/* == 1 if mapping rgb values to rgb
					 * mnemonics */
    rgb_names rgbn[MAX_RGBNAMES];	/* rgb mnemonics from rgb text file */
int rgbn_max;				/* number of rgb mnemonics in table */

/* output: */
cixel_map cmap[MAXCOLORS];		/* pixel strings and ascii rgb
					 * colors */
int *charspp;				/* characters per pixel */

{
    int i, j, base, cpp, mval, red, green, blue, r, g, b, matched;
    char *str;

    /*
     * Figure out how many characters per pixel we'll be using.  Don't want
     * to be forced to link with libm.a, so using a division loop rather
     * than a log function. 
     */
    base = (int) HIGH_CHAR - (int) LOW_CHAR + 1;
    for (cpp = 0, j = ncolors; j; cpp++)
	j /= base;
    *charspp = cpp;

    /*
     * Determine how many hex digits we'll be normalizing to if the rgb
     * value doesn't match a color mnemonic. 
     */
    mval = (int) maxval;
    if (mval <= 0x000F)
	mval = 0x000F;
    else if (mval <= 0x00FF)
	mval = 0x00FF;
    else if (mval <= 0x0FFF)
	mval = 0x0FFF;
    else
	mval = 0xFFFF;

    /*
     * Generate the character-pixel string and the rgb name for each
     * colormap entry. 
     */
    for (i = 0; i < ncolors; i++) {

	/*
	 * The character-pixel string is simply a printed number in base
	 * "base" where the digits of the number range from LOW_CHAR to
	 * HIGH_CHAR and the printed length of the number is "cpp". 
	 */
	cmap[i].cixel = gen_numstr(i, base, cpp);

	/* Fetch the rgb value of the current colormap entry. */
	red = PPM_GETR(chv[i].color);
	green = PPM_GETG(chv[i].color);
	blue = PPM_GETB(chv[i].color);

	/*
	 * If the ppm color components are not relative to 15, 255, 4095,
	 * 65535, normalize the color components here. 
	 */
	if (mval != (int) maxval) {
	    red = (red * mval) / (int) maxval;
	    green = (green * mval) / (int) maxval;
	    blue = (blue * mval) / (int) maxval;
	}

	/*
	 * If the "-rgb <rgbfile>" option was specified, attempt to map the
	 * rgb value to a color mnemonic. 
	 */
	if (map_rgb_names) {

	    /*
	     * The rgb values of the color mnemonics are normalized relative
	     * to 255 << 8, (i.e. 0xFF00).  [That's how the original MIT
	     * code did it, really should have been "v * 65535 / 255"
	     * instead of "v << 8", but have to use the same scheme here or
	     * else colors won't match...]  So, if our rgb values aren't
	     * already 16-bit values, need to shift left. 
	     */
	    if (mval == 0x000F) {
		r = red << 12;
		g = green << 12;
		b = blue << 12;
		/* Special case hack for "white". */
		if (0xF000 == r && r == g && g == b)
		    r = g = b = 0xFF00;
	    } else if (mval == 0x00FF) {
		r = red << 8;
		g = green << 8;
		b = blue << 8;
	    } else if (mval == 0x0FFF) {
		r = red << 4;
		g = green << 4;
		b = blue << 4;
	    } else {
		r = red;
		g = green;
		b = blue;
	    }

	    /*
	     * Just perform a dumb linear search over the rgb values of the
	     * color mnemonics.  One could speed things up by sorting the
	     * rgb values and using a binary search, or building a hash
	     * table, etc... 
	     */
	    for (matched = 0, j = 0; j <= rgbn_max; j++)
		if (r == rgbn[j].r && g == rgbn[j].g && b == rgbn[j].b) {

		    /* Matched.  Allocate string, copy mnemonic, and exit. */
		    if (!(str = (char *) malloc(strlen(rgbn[j].name) + 1)))
			pm_error("out of memory", 0, 0, 0, 0, 0);
		    strcpy(str, rgbn[j].name);
		    cmap[i].rgbname = str;
		    matched = 1;
		    break;
		}
	    if (matched)
		continue;
	}

	/*
	 * Either not mapping to color mnemonics, or didn't find a match.
	 * Generate an absolute #RGB value string instead. 
	 */
	if (!(str = (char *) malloc(mval == 0x000F ? 5 :
				    mval == 0x00FF ? 8 :
				    mval == 0x0FFF ? 11 :
				    14)))
	    pm_error("out of memory", 0, 0, 0, 0, 0);

	sprintf(str, mval == 0x000F ? "#%X%X%X" :
		mval == 0x00FF ? "#%02X%02X%02X" :
		mval == 0x0FFF ? "#%03X%03X%03X" :
		"#%04X%04X%04X", red, green, blue);
	cmap[i].rgbname = str;
    }

}					/* gen_cmap */
