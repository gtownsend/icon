/*
 * Intermediate program to convert iconx.hdr into a header file for inclusion
 * in icont.  This eliminates a compile-time file search on Unix systems.
 * Definition of BinaryHeader activates the inclusion.
 */

#include "../h/gsupport.h"

void putbyte(FILE *fout, int b);

int main(int argc, char *argv[])
   {
   static const char Usage[] = "Usage %s [Filename]\n";
   int b, n;
   char *ifile = NULL;
   char *ofile = NULL;
   FILE *fin, *fout;

   n = 1;
   if (((n + 1) < argc) && !strcmp(argv[n], "-o")) {
      ofile = argv[++n];
      ++n;
      }
   if (n < argc)
      ifile = argv[n++];

   if (ifile == NULL)
      fin = stdin;
   else if ((fin = fopen(ifile, "rb")) == NULL) {
      fprintf(stderr, "Cannot open \"%s\" for input\n\n", ifile);
      fprintf(stderr, Usage, argv[0]);
      return EXIT_FAILURE;
      }

   if (ofile == NULL)
      fout = stdout;
   else if ((fout = fopen(ofile, "w")) == NULL) {
      fprintf(stderr, "Cannot open \"%s\" for output\n\n", ofile);
      fprintf(stderr, Usage, argv[0]);
      return EXIT_FAILURE;
      }

   /*
    * Create an array large enough to hold iconx.hdr (+1 for luck)
    * This array shall be included by link.c (and is nominally called
    * hdr.h)
    */ 
   fprintf(fout, "static unsigned char iconxhdr[MaxHdr+1] = {\n");

   /*
    * Recreate iconx.hdr as a series of hex constants, padded with zero bytes.
    */
   for (n = 0; (b = getc(fin)) != EOF; n++)
      putbyte(fout, b);

   /*
    * If header is to be used, make sure it fits.
    */
   #ifdef BinaryHeader
      if (n > MaxHdr) {
         fprintf(stderr, "%s: file size is %d bytes but MaxHdr is only %d\n",
            argv[0], n, MaxHdr);
         if (ofile != NULL) {
            fclose(fout);
            unlink(ofile);
            }
         return EXIT_FAILURE;
         }
   #endif				/* BinaryHeader */

   while (n++ < MaxHdr)
      putbyte(fout, 0);
   fprintf(fout,"0x00};\n");	/* one more, sans comma, and finish */

   return EXIT_SUCCESS;
   }

/*
 * putbyte(b) - output byte b as two hex digits
 */
void putbyte(FILE *fout, int b)
   {
   static int n = 0;

   fprintf(fout, "0x%02x,", b & 0xFF);
   if (++n == 16) {
      fprintf(fout, "\n");
      n = 0;
      }
   }
