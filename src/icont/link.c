/*
 * link.c -- linker main program that controls the linking process.
 */

#include "link.h"
#include "tproto.h"
#include "tglobals.h"
#include "../h/header.h"

#if UNIX
   #include <sys/types.h>
   #include <sys/stat.h>
#endif					/* UNIX */

#if MSDOS
   #include <fcntl.h>
   extern char pathToIconDOS[];
#endif					/* MSDOS */

#ifdef Header
   #ifndef ShellHeader
      #include "hdr.h"
   #endif					/* ShellHeader */
   #ifndef MaxHeader
      #define MaxHeader MaxHdr
   #endif					/* MaxHeader */
#endif					/* Header */

static	void	setexe	(char *fname);

FILE *infile;				/* input file (.u1 or .u2) */
FILE *outfile;				/* interpreter code output file */

#ifdef DeBugLinker
   FILE *dbgfile;			/* debug file */
   static char dbgname[MaxFileName];	/* debug file name */
#endif					/* DeBugLinker */

struct lfile *llfiles = NULL;		/* List of files to link */

char inname[MaxFileName];		/* input file name */

char icnname[MaxFileName];		/* current icon source file name */
int colmno = 0;				/* current source column number */
int lineno = 0;				/* current source line number */
int fatals = 0;				/* number of errors encountered */

/*
 *  ilink - link a number of files, returning error count
 */
int ilink(ifiles,outname)
char **ifiles;
char *outname;
   {
   int i;
   struct lfile *lf,*lfls;
   char *filename;			/* name of current input file */

   linit();				/* initialize memory structures */
   while (*ifiles)
      alsolink(*ifiles++);		/* make initial list of files */

   /*
    * Phase I: load global information contained in .u2 files into
    *  data structures.
    *
    * The list of files to link is maintained as a queue with llfiles
    *  as the base.  lf moves along the list.  Each file is processed
    *  in turn by forming .u2 and .icn names from each file name, each
    *  of which ends in .u1.  The .u2 file is opened and globals is called
    *  to process it.  When the end of the list is reached, lf becomes
    *  NULL and the loop is terminated, completing phase I.  Note that
    *  link instructions in the .u2 file cause files to be added to list
    *  of files to link.
    */
   for (lf = llfiles; lf != NULL; lf = lf->lf_link) {
      filename = lf->lf_name;
      makename(inname, SourceDir, filename, U2Suffix);
      makename(icnname, TargetDir, filename, SourceSuffix);
      infile = fopen(inname, ReadText);
      if (infile == NULL)
         quitf("cannot open %s",inname);
      readglob();
      fclose(infile);
      }

   /* Phase II (optional): scan code and suppress unreferenced procs. */
   if (!strinv)
      scanrefs();

   /* Phase III: resolve undeclared variables and generate code. */

   /*
    * Open the output file.
    */
   outfile = fopen(outname, WriteBinary);

   if (outfile == NULL) {		/* may exist, but can't open for "w" */
      ofile = NULL;			/* so don't delete if it's there */
      quitf("cannot create %s",outname);
      }

   #if MSDOS && (!NT)
   if (makeExe) {

      /*
       * This prepends ixhdr.exe to outfile, so it'll be executable.
       *
       * I don't know what that #if Header stuff was about since my MSDOS
       * distribution didn't include "hdr.h", but it looks very similar to
       * what I'm doing, so I'll put my stuff here, & if somebody who
       * understands all the multi-operating-system porting thinks my code could
       * be folded into it, having it here should make it easy.
       * -- Will Mengarini.
       */

      FILE *fIconDOS = fopen(pathToIconDOS, "rb");
      char bytesThatBeginEveryExe[2] = {0,0}, oneChar;
      unsigned short originalExeBytesMod512, originalExePages;
      unsigned long originalExeBytes, byteCounter;

      if (!fIconDOS)
         quit("unable to find ixhdr.exe in same dir as icont");
      if (setvbuf(fIconDOS, 0, _IOFBF, 4096))
         if (setvbuf(fIconDOS, 0, _IOFBF, 128))
            quit("setvbuf() failure");
      fread (&bytesThatBeginEveryExe, 2, 1, fIconDOS);
      if (bytesThatBeginEveryExe[0] != 'M' ||
          bytesThatBeginEveryExe[1] != 'Z')
         quit("ixhdr header is corrupt");
      fread (&originalExeBytesMod512, sizeof originalExeBytesMod512,
            1, fIconDOS);
      fread (&originalExePages,       sizeof originalExePages,
            1, fIconDOS);
      originalExeBytes = (originalExePages - 1)*512 + originalExeBytesMod512;

      if (ferror(fIconDOS) || feof(fIconDOS) || !originalExeBytes)
         quit("ixhdr header is corrupt");
      fseek (fIconDOS, 0, 0);

      #ifdef MSWindows
         for (oneChar=fgetc(fIconDOS);!feof(fIconDOS);oneChar=fgetc(fIconDOS)) {
            if (ferror(fIconDOS) || ferror(outfile)) {
               quit("Error copying ixhdr.exe");
	       }
            fputc (oneChar, outfile);
            }
      #else				/* MSWindows */

         for (byteCounter = 0; byteCounter < originalExeBytes; byteCounter++) {
            oneChar = fgetc (fIconDOS);
            if (ferror(fIconDOS) || feof(fIconDOS) || ferror(outfile)) {
               quit("Error copying ixhdr.exe");
	       }
            fputc (oneChar, outfile);
            }
      #endif				/* MSWindows */

      fclose (fIconDOS);
      fileOffsetOfStuffThatGoesInICX = ftell (outfile);
      }
   #endif				/* MSDOS && (!NT) */

   #ifdef Header
      /*
       * Write the bootstrap header to the output file.
       */

      #ifdef ShellHeader
         /*
          * Write a short shell header terminated by \n\f\n\0.
          * Use magic "#!/bin/sh" to ensure that $0 is set when run via $PATH.
          * Pad header to a multiple of 8 characters.
          */

         #if NT
            /*
             * The NT and Win95 direct execution batch file turns echoing off,
             * launches wiconx, attempts to terminate softly via noop.bat,
             * and terminates the hard way (by exiting the DOS shell) if that
             * fails, rather than fall through and start executing machine code
             * as if it were batch commands.
             */
	    {
            char script[2 * MaxPath + 200];
            sprintf(script,
               "@echo off\r\n%s %%0 %%1 %%2 %%3 %%4 %%5 %%6 %%7 %%8 %%9\r\n",
               ((iconxloc!=NULL)?iconxloc:"wiconx"));
            strcat(script,"noop.bat\r\n@echo on\r\n");
            strcat(script,
	       "pause missing noop.bat - press ^c or shell will exit\r\n");
            strcat(script,"exit\r\nrem [executable Icon binary follows]\r\n");
            strcat(script, "        \n\f\n" + ((int)(strlen(script) + 4) % 8));
            hdrsize = strlen(script) + 1;	/* length includes \0 at end */
            fwrite(script, hdrsize, 1, outfile);	/* write header */
	    }
         #endif				/* NT */

         #if UNIX
            /*
             *  Generate a shell header that searches for iconx in this order:
             *     a.  location specified by ICONX environment variable
             *         (if specified, this MUST work, else the script exits)
             *     b.  iconx in same directory as executing binary
             *     c.  location specified in script
             *         (as generated by icont or as patched later)
             *     d.  iconx in $PATH
             *
             *  The ugly ${1+"$@"} is a workaround for non-POSIX handling
             *  of "$@" by some shells in the absence of any arguments.
             *  Thanks to the Unix-haters handbook for this trick.
             */
	    {
            char script[2 * MaxPath + 300];
            sprintf(script, "%s\n%s%-72s\n%s\n\n%s\n%s\n%s\n%s\n\n%s\n",
               "#!/bin/sh",
               "IXBIN=", iconxloc,
               "IXLCL=`echo $0 | sed 's=[^/]*$=iconx='`",
               "[ -n \"$ICONX\" ] && exec \"$ICONX\" $0 ${1+\"$@\"}",
               "[ -x $IXLCL ] && exec $IXLCL $0 ${1+\"$@\"}",
               "[ -x $IXBIN ] && exec $IXBIN $0 ${1+\"$@\"}",
               "exec iconx $0 ${1+\"$@\"}",
               "[executable Icon binary follows]");
            strcat(script, "        \n\f\n" + ((int)(strlen(script) + 4) % 8));
            hdrsize = strlen(script) + 1;	/* length includes \0 at end */
            fwrite(script, hdrsize, 1, outfile);	/* write header */
	    }
         #endif				/* UNIX */

      #else				/* ShellHeader */
         /*
          *  Always write MaxHeader bytes.
          */
         fwrite(iconxhdr, sizeof(char), MaxHeader, outfile);
         hdrsize = MaxHeader;
      #endif				/* ShellHeader */
   #endif				/* Header */

   for (i = sizeof(struct header); i--;)
      putc(0, outfile);
   fflush(outfile);
   if (ferror(outfile) != 0)
      quit("unable to write to icode file");

   #ifdef DeBugLinker
      /*
       * Open the .ux file if debugging is on.
       */
      if (Dflag) {
         makename(dbgname, TargetDir, llfiles->lf_name, ".ux");
         dbgfile = fopen(dbgname, WriteText);
         if (dbgfile == NULL)
            quitf("cannot create %s", dbgname);
         }
   #endif				/* DeBugLinker */

   /*
    * Loop through input files and generate code for each.
    */
   lfls = llfiles;
   while ((lf = getlfile(&lfls)) != 0) {
      filename = lf->lf_name;
      makename(inname, SourceDir, filename, U1Suffix);
      makename(icnname, TargetDir, filename, SourceSuffix);
      infile = fopen(inname, ReadText);
      if (infile == NULL)
         quitf("cannot open %s", inname);
      gencode();
      fclose(infile);
      }

   gentables();		/* Generate record, field, global, global names,
			   static, and identifier tables. */

   fclose(outfile);
   lmfree();
   if (fatals > 0)
      return fatals;
   setexe(outname);
   return 0;
   }

#ifdef ConsoleWindow
   extern FILE *flog;
#endif					/* ConsoleWindow */

/*
 * lwarn - issue a linker warning message.
 */
void lwarn(s1, s2, s3)
char *s1, *s2, *s3;
   {

   #ifdef ConsoleWindow
      if (flog != NULL) {
         fprintf(flog, "%s: ", icnname);
         if (lineno)
            fprintf(flog, "Line %d # :", lineno);
         fprintf(flog, "\"%s\": %s%s\n", s1, s2, s3);
         fflush(flog);
         return;
         }
   #endif				/* ConsoleWindow */
   fprintf(stderr, "%s: ", icnname);
   if (lineno)
      fprintf(stderr, "Line %d # :", lineno);
   fprintf(stderr, "\"%s\": %s%s\n", s1, s2, s3);
   fflush(stderr);
   }

/*
 * lfatal - issue a fatal linker error message.
 */

void lfatal(s1, s2)
char *s1, *s2;
   {

   fatals++;
   #ifdef ConsoleWindow
      if (flog != NULL) {
         fprintf(flog, "%s: ", icnname);
         if (lineno)
            fprintf(flog, "Line %d # : ", lineno);
         fprintf(flog, "\"%s\": %s\n", s1, s2);
         return;
         }
   #endif				/* ConsoleWindow */
   fprintf(stderr, "%s: ", icnname);
   if (lineno)
      fprintf(stderr, "Line %d # : ", lineno);
   fprintf(stderr, "\"%s\": %s\n", s1, s2);
   }

/*
 * setexe - mark the output file as executable
 */

static void setexe(fname)
char *fname;
   {

   #if MSDOS
      chmod(fname,0755);	/* probably could be smarter... */
   #endif				/* MSDOS */

   #if UNIX
      struct stat stbuf;
      int u, r, m;
      /*
       * Set each of the three execute bits (owner,group,other) if allowed by
       *  the current umask and if the corresponding read bit is set; do not
       *  clear any bits already set.
       */
      umask(u = umask(0));		/* get and restore umask */
      if (stat(fname,&stbuf) == 0)  {	/* must first read existing mode */
         r = (stbuf.st_mode & 0444) >> 2;	/* get & position read bits */
         m = stbuf.st_mode | (r & ~u);		/* set execute bits */
         chmod(fname,m);		 /* change file mode */
         }
   #endif				/* UNIX */

   }
