/*
 * save(s) -- for systems that support ExecImages
 */

#include "../h/gsupport.h"

#ifdef ExecImages

/*
 * save(s) -- for the Convex.
 */

#ifdef CONVEX
#define TEXT0 0x80001000		/* address of first .text page */
#define DATA0 ((int) &environ & -4096)	/* address of first .data page */
#define START TEXT0			/* start address */

#include <convex/filehdr.h>
#include <convex/opthdr.h>
#include <convex/scnhdr.h>

extern char environ;

wrtexec (ef)
int ef;
{
    struct filehdr filehdr;
    struct opthdr opthdr;
    struct scnhdr texthdr;
    struct scnhdr datahdr;

    int foffs = 0;
    int ooffs = foffs + sizeof filehdr;
    int toffs = ooffs + sizeof opthdr;
    int doffs = toffs + sizeof texthdr;

    int tsize = DATA0 - TEXT0;
    int dsize = (sbrk (0) - DATA0 + 4095) & -4096;

    bzero (&filehdr, sizeof filehdr);
    bzero (&opthdr, sizeof opthdr);
    bzero (&texthdr, sizeof texthdr);
    bzero (&datahdr, sizeof datahdr);
    
    filehdr.h_magic = SOFF_MAGIC;
    filehdr.h_nscns = 2;
    filehdr.h_scnptr = toffs;
    filehdr.h_opthdr = sizeof opthdr;

    opthdr.o_entry = START;
    opthdr.o_flags = OF_EXEC | OF_STRIPPED;

    texthdr.s_vaddr = TEXT0;
    texthdr.s_size = tsize;
    texthdr.s_scnptr = 0x1000;
    texthdr.s_prot = VM_PG_R | VM_PG_E;
    texthdr.s_flags = S_TEXT;

    datahdr.s_vaddr = DATA0;
    datahdr.s_size = dsize;
    datahdr.s_scnptr = 0x1000 + tsize;
    datahdr.s_prot = VM_PG_R | VM_PG_W;
    datahdr.s_flags = S_DATA;

    write (ef, &filehdr, sizeof filehdr);
    write (ef, &opthdr, sizeof opthdr);
    write (ef, &texthdr, sizeof texthdr);
    write (ef, &datahdr, sizeof datahdr);
    lseek (ef, 0x1000, 0);
    write (ef, TEXT0, tsize + dsize);
    close (ef);

    return dsize;
}
#endif					/* CONVEX */

/*
 * save(s) -- for generic BSD systems.
 */

#ifdef GenericBSD
#include <a.out.h>
wrtexec(ef)
int ef;
{
   struct exec hdr;
   extern environ, etext;
   int tsize, dsize;

   /*
    * Construct the header.  The text and data region sizes must be multiples
    *  of 1024.
    */
   hdr.a_magic = ZMAGIC;
   tsize = (int)&etext;
   hdr.a_text = (tsize + 1024) & ~(1024-1);
   dsize = sbrk(0) - (int)&environ;
   hdr.a_data = (dsize + 1024) & ~(1024-1);
   hdr.a_bss = 0;
   hdr.a_syms = 0;
   hdr.a_entry = 0;
   hdr.a_trsize = 0;
   hdr.a_drsize = 0;

   /*
    * Write the header.
    */
   write(ef, &hdr, sizeof(hdr));

   /*
    * Write the text, starting at N_TXTOFF.
    */
   lseek(ef, N_TXTOFF(hdr), 0);
   write(ef, 0, tsize);
   lseek(ef, hdr.a_text - tsize, 1);

   /*
    * Write the data.
    */
   write(ef, &environ, dsize);
   lseek(ef, hdr.a_data - dsize, 1);
   close(ef);
   return hdr.a_data;
}
#endif					/* GenericBSD */

/*
 * save(s) -- for the Encore.
*/

#ifdef MULTIMAX

#include <a.out.h>
#include <sys/file.h>
#include <sgs.h>

#define NUMSECS		2		/* Two sections in the image. */
#define TEXTSTART	0		/* Text starts at address 0. */
#define MODSTART	0x20		/* Depends on crt0.s */
#define MODSIZE		0x10		/* Depends on crt0.s */
#define IMAGEPAGE	4096		/* Page size for images.  Found it */
                                        /* with aoutdump(1) */
#define HDRSIZE		(sizeof(struct filehdr)+sizeof(struct aouthdr)+ \
			 NUMSECS*sizeof(struct scnhdr))
#define PAGEROUND(x)	(((x+IMAGEPAGE-1)/IMAGEPAGE)*IMAGEPAGE)

extern etext;		/* ld(1) puts this at the end of the text segment. */
extern environ;		/* ld(1) puts this at the start of the data segment. */

/*
 * wrtexec() - save image in file.
 */
wrtexec(ExecFile)
     int ExecFile;
{
  int Status;		/* For saving status codes. */

  /* Call internal wrtexec2() routine. */
  Status=wrtexec2(ExecFile);

  /* Close the file. */
  close(ExecFile);
  
  return Status;
}

/*
 * wrtexec2 - Code to write the image file.
 */
static
wrtexec2(ExecFile)
     int ExecFile;
{
  struct filehdr FileHeader;	/* File header record. */
  struct aouthdr SystemHeader;	/* System header record. */
  struct scnhdr SectionHeader;	/* Section header record. */

  struct timeval TV;		/* Time value. */
  struct timezone TZ;		/* Time zone. */

  unsigned long TextStart;	/* Start of text. */
  unsigned long TextSize;	/* Size of text. */
  unsigned long TextFPtr;	/* Location of text in image file. */
  unsigned long DataStart;	/* Start of data. */
  unsigned long DataSize;	/* Size of data. */
  unsigned long DataFPtr;	/* Location of data in image file. */

  /* Figure out a few things we need to know. */
  TextStart = TEXTSTART;
  TextSize = (unsigned long)&etext;
  TextFPtr = PAGEROUND(HDRSIZE);

  DataStart = (unsigned long)&environ;
  DataSize = sbrk(0)-DataStart;
  DataFPtr = TextFPtr+PAGEROUND(TextSize);

  /* Write a file header. */
  FileHeader.f_magic = NS32GMAGIC;		/* NS 32k executable. */
  FileHeader.f_nscns = NUMSECS;			/* Three standard sections. */
  gettimeofday(&TV,&TZ);
  FileHeader.f_timdat = TV.tv_sec;		/* Time stamp. */
  FileHeader.f_symptr = 0;			/* No symbols. */
  FileHeader.f_nsyms = 0;			/* No symbols. */
  FileHeader.f_opthdr = sizeof(struct aouthdr);	/* Size of system header. */
  FileHeader.f_flags = F_RELFLG|F_EXEC|F_LNNO|F_LSYMS; /* Misc. Flags. */
  if(write(ExecFile,&FileHeader,sizeof FileHeader)==-1)
    return -1;

  /* Write a system header. */
  SystemHeader.magic = PAGEMAGIC;		/* Normal executable. */
  SystemHeader.vstamp = 0;			/* Ignore this. */
  SystemHeader.tsize = TextSize;		/* Size of text segment. */
  SystemHeader.dsize = DataSize;		/* Size of data segment. */
  SystemHeader.bsize = 0;			/* No bss */
  SystemHeader.msize = MODSIZE;			/* Magic from aoutdump(1). */
  SystemHeader.mod_start = MODSTART;		/* Magic from aoutdump(1). */
  SystemHeader.entry = 0x2;			/* Magic from aoutdump(1). */
  SystemHeader.text_start = TextStart;		/* Magic from aoutdump(1). */
  SystemHeader.data_start = DataStart;		/* Start of data segment. */
  SystemHeader.entry_mod = 0;			/* Unused. */
  SystemHeader.flags = U_SYS_42|U_AL_4096;	/* UMAX 4.2, 4k align. */
  if(write(ExecFile,&SystemHeader,sizeof SystemHeader)==-1)
    return -1;

  /* Write text section header. */
  strcpy(SectionHeader.s_name,_TEXT);		/* Section name. */
  SectionHeader.s_paddr = TextStart;		/* Physical address. */
  SectionHeader.s_vaddr = TextStart;		/* Virtual address. */
  SectionHeader.s_size = TextSize;		/* Section size. */
  SectionHeader.s_scnptr = TextFPtr;		/* File ptr to section. */
  SectionHeader.s_relptr = 0;			/* No relocation data. */
  SectionHeader.s_lnnoptr = 0;			/* No line numbers. */
  SectionHeader.s_nreloc = 0;			/* No relocation data. */
  SectionHeader.s_nlnno = 0;			/* No line numbers. */
  SectionHeader.s_flags = STYP_TEXT;		/* Text section. */
  SectionHeader.s_symptr = 0;			/* No symbol data. */
  SectionHeader.s_modno = 0;			/* Ignore this. */
  SectionHeader.s_pad = 0;			/* Padding. */
  if(write(ExecFile,&SectionHeader,sizeof SectionHeader)==-1)
    return -1;

  /* Write data section header. */
  strcpy(SectionHeader.s_name,_DATA);		/* Section name. */
  SectionHeader.s_paddr = DataStart;		/* Physical address. */
  SectionHeader.s_vaddr = DataStart;		/* Virtual address. */
  SectionHeader.s_size = DataSize;		/* Section size. */
  SectionHeader.s_scnptr = DataFPtr;		/* File ptr to section. */
  SectionHeader.s_relptr = 0;			/* No relocation data. */
  SectionHeader.s_lnnoptr = 0;			/* No line numbers. */
  SectionHeader.s_nreloc = 0;			/* No relocation data. */
  SectionHeader.s_nlnno = 0;			/* No line numbers. */
  SectionHeader.s_flags = STYP_DATA;		/* Data section. */
  SectionHeader.s_symptr = 0;			/* No symbol data. */
  SectionHeader.s_modno = 0;			/* Ignore this. */
  SectionHeader.s_pad = 0;			/* Padding. */
  if(write(ExecFile,&SectionHeader,sizeof SectionHeader)==-1)
    return -1;

  /* Write the text section. */
  if(lseek(ExecFile,TextFPtr,L_SET)==-1)
    return -1;
  if(write(ExecFile,TextStart,TextSize)==-1)
    return -1;

  /* Write the data section. */
  if(lseek(ExecFile,DataFPtr,L_SET)==-1)
     return -1;
  if(write(ExecFile,DataStart,DataSize)==-1)
    return -1;

  return DataSize;
}

#endif					/* MULTIMAX */

/*
 * save(s) -- for Sun Workstations.
 */

#ifdef SUN
#include <a.out.h>
wrtexec(ef)
int ef;
{
   struct exec *hdrp, hdr;
   extern environ, etext;
   int tsize, dsize;

   hdrp = (struct exec *)PAGSIZ;
	
   /*
    * This code only handles the ZMAGIC format...
    */
   if (hdrp->a_magic != ZMAGIC)
      syserr("executable is not ZMAGIC format");
   /*
    * Construct the header by copying in the header in core and fixing
    *  up values as necessary.
    */
   hdr = *hdrp;
   tsize = (char *)&etext - (char *)hdrp;
   hdr.a_text = (tsize + PAGSIZ) & ~(PAGSIZ-1);
   dsize = sbrk(0) - (int)&environ;
   hdr.a_data = (dsize + PAGSIZ) & ~(PAGSIZ-1);
   hdr.a_bss = 0;
   hdr.a_syms = 0;
   hdr.a_trsize = 0;
   hdr.a_drsize = 0;

   /*
    * Write the text.
    */
   write(ef, hdrp, tsize);
   lseek(ef, hdr.a_text, 0);

   /*
    * Write the data.
    */
   write(ef, &environ, dsize);
   lseek(ef, hdr.a_data - dsize, 1);

   /*
    * Write the header.
    */
   lseek(ef, 0, 0);
   write(ef, &hdr, sizeof(hdr));

   close(ef);
   return hdr.a_data;
}
#endif					/* SUN */

#else					/* ExecImages */
static char junk;
#endif					/* ExecImages */
