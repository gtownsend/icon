/*
 * mproto.h -- prototypes for functions common to several modules.
 */

#define NewStruct(type) alloc(sizeof(struct type))

pointer	alloc		(unsigned int n);
unsigned short *bitvect	(char *image, int len);
char	*canonize	(char *path);
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
char	*findexe	(char *name, char *buf, size_t len);
char	*findonpath	(char *name, char *buf, size_t len);
char	*followsym	(char *name, char *buf, size_t len);
struct fileparts *fparse(char *s);
void	free_stbl	(void);
void	id_comment	(FILE *f);
void	init_sbuf	(struct str_buf *sbuf);
void	init_str	(void);
long	longwrite	(char *s, long len, FILE *file);
char	*makename	(char *dest, char *d, char *name, char *e);
long	millisec	(void);
struct il_code *new_il	(int il_type, int size);
void	new_sbuf	(struct str_buf *sbuf);
void	nxt_pre		(char *pre, char *nxt, int n);
char	*pathfind	(char *buf, char *path, char *name, char *extn);
int	ppch		(void);
void	ppdef		(char *name, char *value);
void	ppecho		(void);
int	ppinit		(char *fname, char *inclpath, int m4flag);
int	prt_i_str	(FILE *f, char *s, int len);
char	*relfile	(char *prog, char *mod);
char	*salloc		(char *s);
int	smatch		(char *s, char *t);
char	*spec_str	(char *s);
char	*str_install	(struct str_buf *sbuf);
int	tonum		(int c);
void	lear_sbuf	(struct str_buf *sbuf);

#ifndef SysOpt
   int	getopt		(int argc, char * const argv[], const char *optstring);
#endif					/* NoSysOpt */
