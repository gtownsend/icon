void            addrmlst     (char *fname, FILE *f);
void            advance_tok  (struct token **tp);
int                chk_eq_sign  (void);
long               conditional  (struct token **tp, struct token *trigger);
struct token      *copy_t       (struct token *t);
void            err1         (char *s);
void            err2         (char *s1, char *s2);
void            errfl1       (char *f, int l, char *s);
void            errfl2       (char *f, int l, char *s1, char *s2);
void            errfl3       (char *f, int l, char *s1, char *s2, char *s3);
void            errt1        (struct token *t, char *s);
void            errt2        (struct token *t, char *s1, char *s2);
void            errt3        (struct token *t, char *s1, char *s2, char *s3);
int                eval         (struct token *trigger);
void            fill_cbuf    (void);
void            free_id_lst  (struct id_lst *ilst);
void            free_plsts   (struct paste_lsts *plsts);
void            free_m       (struct macro *m);
void            free_m_lst   (struct macro *m);
void            free_t       (struct token *t);
void            free_t_lst   (struct tok_lst *tlst);
struct str_buf    *get_sbuf     (void);
void            include      (struct token *trigger, char *fname, int start);
void	init_files	(char *opt_lst,char * *opt_args);
void	init_files	(char *opt_lst,char * *opt_args);
void            init_macro   (void);
void            init_preproc (char *fname, char *opt_lst, char **opt_args);
void            init_sys     (char *fname, int argc, char *argv[]);
void            init_tok     (void);
struct token      *interp_dir   (void);
struct token      *mac_tok      (void);
void            merge_whsp   (struct token **whsp, struct token **next_t,
                                  struct token *(*t_src)(void));
void            m_delete     (struct token *mname);
void            m_install    (struct token *mname, int category,
                                  int multi_line, struct id_lst *prmlst,
                                  struct tok_lst *body);
struct macro      *m_lookup     (struct token *mname);
struct char_src   *new_cs       (char *fname, FILE *f, int bufsize);
struct id_lst     *new_id_lst   (char *id);
struct macro      *new_macro    (char *mname, int category,
                                  int multi_line, struct id_lst *prmlst,
                                  struct tok_lst *body);
struct mac_expand *new_me       (struct macro *m, struct tok_lst **args,
                                   struct tok_lst **exp_args);
struct paste_lsts *new_plsts    (struct token *trigger,
                                  struct tok_lst *tlst,
                                  struct paste_lsts *plst);
struct token      *new_token    (int id, char *image, char *fname,
                                  int line);
struct tok_lst    *new_t_lst    (struct token *tok);
struct token      *next_tok     (void);
void            nxt_non_wh   (struct token **tp);
void            output       (FILE *out_file);
struct token      *paste        (void);
void            pop_src      (void);
struct token      *preproc      (void);
void            push_src     (int flag, union src_ref *ref);
void            rel_sbuf     (struct str_buf *sbuf);
int                rt_state     (int tok_id);
void            show_usage   (void);
void            source       (char *fname);
void            str_src      (char *src_name, char *s, int len);
struct token      *tokenize     (void);
