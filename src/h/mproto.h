/*
 * mproto.h -- prototypes for functions common to several modules.
 */

pointer	alloc		(unsigned int n);
unsigned short *bitvect	(char *image, int len);
void	clear_sbuf	(struct str_buf *sbuf);
int	cmp_pre		(char *pre1, char *pre2);
void	cset_init	(FILE *f, unsigned short *bv);
void	db_chstr	(char *s1, char *s2);
void	db_close	(void);
void	db_code		(struct implement *ip);
void	db_dscrd	(struct implement *ip);
void	db_err1		(int fatal, char *s1);
void	db_err2		(int fatal, char *s1, char *s2);
struct implement *db_ilkup (char *id, struct implement **tbl);
struct implement *db_impl  (int oper_typ);
int	db_open		(char *s, char **lrgintflg);
char	*db_string	(void);
int	db_tbl		(char *section, struct implement **tbl);
struct fileparts *fparse(char *s);
void	free_stbl	(void);
void	id_comment	(FILE *f);
void	init_sbuf	(struct str_buf *sbuf);
void	init_str	(void);
long	longwrite	(char *s,long len,FILE *file);
char	*makename	(char *dest,char *d,char *name,char *e);
long	millisec	(void);
struct il_code *new_il	(int il_type, int size);
void	new_sbuf	(struct str_buf *sbuf);
void	nxt_pre		(char *pre, char *nxt, int n);
char	*pathfind	(char *buf, char *path, char *name, char *extn);
int	ppch		(void);
void	ppdef		(char *name, char *value);
void	ppecho		(void);
int	ppinit		(char *fname, int m4flag);
int	prt_i_str	(FILE *f, char *s, int len);
int	redirerr	(char *p);
char	*salloc		(char *s);
int	smatch		(char *s,char *t);
char	*spec_str	(char *s);
char	*str_install	(struct str_buf *sbuf);
int	tonum		(int c);
void 	lear_sbuf	(struct str_buf *sbuf);

#ifdef ConsoleWindow
   int Consolefprintf(FILE *file, char *format, ...);
   int Consoleprintf(char *format, ...);
   int Consoleputc(int c, FILE *file);
   int Consolefflush(FILE *file);
#endif					/* ConsoleWindow */

#ifndef SysOpt
   int	getopt		(int argc, char * const argv[], const char *optstring);
#endif					/* NoSysOpt */

#if IntBits == 16
   long	lstrlen		(char *s);
   void	lqsort		(char *base, int nel, int width, int (*cmp)());
#endif					/* IntBits == 16 */

#define NewStruct(type)\
   (struct type *)alloc((unsigned int) sizeof (struct type))
