void           add_dpnd  (struct srcfile *sfile, char *objname);
int               alloc_tnd (int typ, struct node *init, int lvl);
struct node      *arith_nd (struct token *tok, struct node *p1,
                             struct node *p2, struct node *c_int,
                             struct node *ci_act, struct node *intgr,
                             struct node *i_act, struct node *dbl,
                             struct node *d_act);
struct il_c      *bdy_prm   (int addr_of, int just_desc, struct sym_entry *sym, int may_mod);
int               c_walk    (struct node *n, int indent, int brace);
int               call_ret  (struct node *n);
struct token     *chk_exct  (struct token *tok);
void           chkabsret (struct token *tok, int ret_typ);
void           clr_def   (void);
void           clr_dpnd  (char *srcname);
void           clr_prmloc (void);
struct token     *cnv_to_id (struct token *t);
char             *cnv_name  (int typcd, struct node *dflt, int *dflt_to_ptr);
struct node      *comp_nd   (struct token *tok, struct node *dcls,
                              struct node *stmts);
int               creat_obj (void);
void           d_lst_typ (struct node *dcls);
void           dclout    (struct node *n);
struct node      *dest_node (struct token *tok);
void           dst_alloc (struct node *cnv_typ, struct node *var);
void           dumpdb    (char *dbname);
void           fncout    (struct node *head, struct node *prm_dcl,
                              struct node *block);
void           force_nl  (int indent);
void           free_sym  (struct sym_entry *sym);
void           free_tree (struct node *n);
void           free_tend (void);
void           full_lst  (char *fname);
void           func_def  (struct node *dcltor);
void           id_def    (struct node *dcltor, struct node *x);
void           keepdir   (struct token *s);
int               icn_typ   (struct node *n);
struct il_c      *ilc_dcl   (struct node *tqual, struct node *dcltor,
                              struct node *init);
void           impl_fnc  (struct token *name);
void           impl_key  (struct token *name);
void           impl_op   (struct token *op_sym, struct token *name);
void           init_lex  (void);
void           init_sym  (void);
struct il_c      *inlin_c   (struct node *n, int may_mod);
void           in_line   (struct node *n);
void           just_type (struct node *typ, int indent, int ilc);
void           keyconst  (struct token *t);
struct node      *lbl       (struct token *t);
void           ld_prmloc (struct parminfo *parminfo);
void           loaddb    (char *db);
void           mrg_prmloc (struct parminfo *parminfo);
struct parminfo  *new_prmloc (void);
struct node      *node0     (int id, struct token *tok);
struct node      *node1     (int id, struct token *tok, struct node *n1);
struct node      *node2     (int id, struct token *tok, struct node *n1,
                              struct node *n2);
struct node      *node3     (int id, struct token *tok, struct node *n1,
                              struct node *n2, struct node *n3);
struct node      *node4     (int id, struct token *tok, struct node *n1,
                              struct node *n2, struct node *n3,
                              struct node *n4);
struct il_c      *parm_dcl  (int addr_of, struct sym_entry *sym);
void	pop_cntxt	(void);
void           pop_lvl   (void);
void           prologue  (void);
void           prt_str   (char *s, int indent);
void		  ptout	    (struct token * x);
void	push_cntxt	(int lvl_incr);
void           push_lvl  (void);
void           put_c_fl  (char *fname, int keep);
void           defout    (struct node *n);
void           set_r_seq (long min, long max, int resume);
struct il_c      *simpl_dcl (char *tqual, int addr_of, struct sym_entry *sym);
void           spcl_dcls (struct sym_entry *op_params);
struct srcfile   *src_lkup  (char *srcname);
void           strt_def  (void);
void           sv_prmloc (struct parminfo *parminfo);
struct sym_entry *sym_add  (int tok_id, char *image, int id_type, int nest_lvl);
struct sym_entry *sym_lkup  (char *image);
struct node      *sym_node  (struct token *tok);
void           s_prm_def (struct token *u_ident, struct token *d_ident);
void           tnd_char  (void);
void           tnd_strct (struct token *t);
void           tnd_union (struct token *t);
void           trans     (char *src_file);
long              ttol      (struct token *t);
char             *typ_name  (int typ, struct token *tok);
void           unuse     (struct init_tend *t_lst, int lvl);
void           var_args  (struct token *ident);
void           yyerror   (char *s);
int               yylex     (void);
int               yyparse   (void);
