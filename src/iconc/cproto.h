/*
 * Prototypes for functions in iconc.
 */
struct sig_lst   *add_sig    (struct code *sig, struct c_fnc *fnc);
void           addlib     (char *libname);
struct code      *alc_ary    (int n);
int               alc_cbufs  (int num, nodeptr lifetime);
int               alc_dtmp   (nodeptr lifetime);
int               alc_itmp   (nodeptr lifetime);
struct code      *alc_lbl    (char *desc, int flag);
int               alc_sbufs  (int num, nodeptr lifetime);
#ifdef OptimizeType
unsigned int   *alloc_mem_typ   (unsigned int n_types);
#endif					/* OptimizeType */
void           arth_anlz  (struct il_code *var1, struct il_code *var2,
                               int *maybe_int, int *maybe_dbl, int *chk1,
                               struct code **conv1p, int *chk2,
                               struct code **conv2p);
struct node      *aug_nd     (nodeptr op, nodeptr arg1, nodeptr arg2);
struct node      *binary_nd  (nodeptr op, nodeptr arg1, nodeptr arg2);
void         bitrange    (int typcd, int *frst_bit, int *last_bit);
nodeptr           buildarray (nodeptr a, nodeptr lb, nodeptr e);
void           callc_add  (struct c_fnc *cont);
void           callo_add  (char *oper_nm, int ret_flag,
                               struct c_fnc *cont, int need_cont,
                               struct code *arglist, struct code *on_ret);
struct node      *case_nd    (nodeptr loc_model, nodeptr expr, nodeptr cases);
int               ccomp      (char *srcname, char *exename);
void           cd_add     (struct code *cd);
struct val_loc   *chk_alc    (struct val_loc *rslt, nodeptr lifetime);
void           chkinv     (void);
void           chkstrinv  (void);
struct node      *c_str_leaf (int type,struct node *loc_model, char *c);
void	          codegen    (struct node *t);
int               cond_anlz  (struct il_code *il, struct code **cdp);
void           const_blks (void);
struct val_loc   *cvar_loc   (char *name);
int               do_inlin   (struct implement *impl, nodeptr n, int *sep_cont,
				struct op_symentry *symtab, int n_va);
void	          doiconx    (char *s);
struct val_loc   *dtmp_loc   (int n);
void          eval_arith (int indx1, int indx2, int *maybe_int, int *maybe_dbl);
int           eval_cnv   (int typcd, int indx, int def, int *cnv_flags);
int	eval_is	(int typcd,int indx);
void           findcases  (struct il_code *il, int has_dflt,
                               struct case_anlz *case_anlz);
void           fix_fncs   (struct c_fnc *fnc);
struct fentry    *flookup    (char *id);
void           gen_inlin  (struct il_code *il, struct val_loc *rslt,
                               struct code **scont_strt,
                               struct code **scont_fail, struct c_fnc *cont,
                               struct implement *impl, int nsyms,
                               struct op_symentry *symtab, nodeptr n,
                               int dcl_var, int n_va);
int	          getopr     (int ac, int *cc);
#ifdef OptimizeType
unsigned int    get_bit_vector (struct typinfo *src, int pos);
#endif					/* OptimizeType */
struct gentry    *glookup    (char *id);
void	          hsyserr    (char **av, char *file);
struct node      *i_str_leaf (int type,struct node *loc_model,char *c, int d);
long              iconint    (char *image);
struct code      *il_copy    (struct il_c *dest, struct val_loc *src);
struct code      *il_cnv     (int typcd, struct il_code *src,
                               struct il_c *dflt, struct il_c *dest);
struct code      *il_dflt    (int typcd, struct il_code *src,
                               struct il_c *dflt, struct il_c *dest);
void           implproto  (struct implement *ip);
void           init       (void);
void           init_proc  (char *name);
void           init_rec   (char *name);
void           init_src   (void);
void           install    (char *name,int flag);
struct gentry    *instl_p    (char *name, int flag);
struct node      *int_leaf   (int type,struct node *loc_model,int c);
struct val_loc   *itmp_loc   (int n);
struct node      *invk_main  (struct pentry *main_proc);
struct node      *invk_nd    (struct node *loc_model, struct node *proc,
                              struct node *args);
void           invoc_grp  (char *grp);
void           invocbl    (nodeptr op, int arity);
struct node      *key_leaf   (nodeptr loc_model, char *keyname);
void           liveness (nodeptr n, nodeptr resumer, nodeptr *failer, int *gen);
struct node      *list_nd    (nodeptr loc_model, nodeptr args);
void           lnkdcl     (char *name);
void	          readdb     (char *db_name);
struct val_loc   *loc_cpy    (struct val_loc *loc, int mod_access);
#ifdef OptimizeType
void           mark_recs (struct fentry *fp, struct typinfo *typ,
                              int *num_offsets, int *offset, int *bad_recs);
#else					/* OptimizeType */
void           mark_recs (struct fentry *fp, unsigned int *typ,
                              int *num_offsets, int *offset, int *bad_recs);
#endif					/* OptimizeType */
struct code      *mk_goto    (struct code *label);
struct node      *multiunary (char *op, nodeptr loc_model, nodeptr oprnd);
struct sig_act   *new_sgact  (struct code *sig, struct code *cd,
                               struct sig_act *next);
int               nextchar   (void);
void           nfatal     (struct node *n, char *s1, char *s2);
int               n_arg_sym  (struct implement *ip);
void           outerfnc   (struct c_fnc *fnc);
int               past_prms  (struct node *n);
void           proccode   (struct pentry *proc);
void           prt_fnc    (struct c_fnc *fnc);
void           prt_frame  (char *prefix, int ntend, int n_itmp,
				int i, int j, int k);
struct centry    *putlit     (char *image,int littype,int len);
struct lentry    *putloc     (char *id,int id_type);
void	          quit       (char *msg);
void	          quitf      (char *msg,char *arg);
void           recconstr  (struct rentry *r);
void           resolve    (struct pentry *proc);
unsigned int      round2     (unsigned int n);
struct code      *sig_cd     (struct code *fail, struct c_fnc *fnc);
void           src_file   (char *name);
struct node      *sect_nd    (nodeptr op, nodeptr arg1, nodeptr arg2,
                               nodeptr arg3);
void           tfatal     (char *s1,char *s2);
struct node      *to_nd      (nodeptr loc_model, nodeptr arg1,
                               nodeptr arg2);
struct node      *toby_nd    (nodeptr loc_model, nodeptr arg1,
                               nodeptr arg2, nodeptr arg3);
int               trans      (void);
struct node      *tree1      (int type);
struct node      *tree2      (int type,struct node *loc_model);
struct node      *tree3      (int type,struct node *loc_model,
                               struct node *c);
struct node      *tree4      (int type, struct node *loc_model,
                               struct node *c, struct node *d);
struct node      *tree5      (int type, struct node *loc_model,
			       struct node *c, struct node *d,
			       struct node *e);
struct node      *tree6      (int type,struct node *loc_model,
			       struct node *c, struct node *d,
			       struct node *e, struct node *f);
void	          tsyserr    (char *s);
void	          twarn      (char *s1,char *s2);
struct code      *typ_chk    (struct il_code *var, int typcd);
int               type_case  (struct il_code *il, int (*fnc)(),
                               struct case_anlz *case_anlz);
void           typeinfer  (void);
struct node      *unary_nd   (nodeptr op, nodeptr arg);
void           var_dcls   (void);
#ifdef OptimizeType
int               varsubtyp  (struct typinfo *typ, struct lentry **single);
#else					/* OptimizeType */
int               varsubtyp  (unsigned int *typ, struct lentry **single);
#endif					/* OptimizeType */
void	          writecheck (int rc);
void	          yyerror    (int tok,struct node *lval,int state);
int               yylex      (void);
int               yyparse    (void);
#ifdef OptimizeType
void         xfer_packed_types (struct typinfo *type);
#endif					/* OptimizeType */

#ifdef DeBug
void symdump (void);
void ldump   (struct lentry **lhash);
void gdump   (void);
void cdump   (void);
void fdump   (void);
void rdump   (void);
#endif					/* DeBug */
