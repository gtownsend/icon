/*
 * This file does most of the memory management.
 */

#include "../preproc/preproc.h"
#include "../preproc/ptoken.h"

struct src *src_stack = NULL;  /* stack of token sources */

#include "../preproc/pproto.h"

/*
 * new_macro - allocate a new entry for the macro symbol table.
 */
struct macro *new_macro(mname, category, multi_line, prmlst, body)
char *mname;
int category;
int multi_line;
struct id_lst *prmlst;
struct tok_lst *body;
   {
   struct macro *mp;

   mp = NewStruct(macro);
   mp->mname = mname;
   mp->category = category;
   mp->multi_line = multi_line;
   mp->prmlst = prmlst;
   mp->body = body;
   mp->ref_cnt = 1;
   mp->recurse = 0;
   mp->next = NULL;
   return mp;
   }

/*
 * new_token - allocate a new token.
 */
struct token *new_token(id, image, fname, line)
int id;
char *image;
char *fname;
int line;
   {
   struct token *t;

   t = NewStruct(token);
   t->tok_id = id;
   t->image = image;
   t->fname = fname;
   t->line = line;
   t->flag = 0;
   return t;
   }

/*
 * copy_t - make a copy of a token.
 */
struct token *copy_t(t)
struct token *t;
   {
   struct token *t1;

   if (t == NULL)
      return NULL;

   t1 = NewStruct(token);
   *t1 = *t;
   return t1;
   }

/*
 * new_t_lst - allocate a new element for a token list.
 */
struct tok_lst *new_t_lst(tok)
struct token *tok;
   {
   struct tok_lst *tlst;

   tlst = NewStruct(tok_lst);
   tlst->t = tok;
   tlst->next = NULL;
   return tlst;
   }

/*
 * new_id_lst - allocate a new element for an identifier list.
 */
struct id_lst *new_id_lst(id)
char *id;
   {
   struct id_lst *ilst;

   ilst = NewStruct(id_lst);
   ilst->id = id;
   ilst->next = NULL;
   return ilst;
   }

/*
 * new_cs - allocate a new structure for a source of tokens created from
 *  characters.
 */
struct char_src *new_cs(fname, f, bufsize)
char *fname;
FILE *f;
int bufsize;
   {
   struct char_src *cs;

   cs = NewStruct(char_src);
   cs->char_buf = alloc(bufsize * sizeof(int));
   cs->line_buf = alloc(bufsize * sizeof(int));
   cs->bufsize = bufsize;
   cs->fname = fname;
   cs->f = f;
   cs->line_adj = 0;
   cs->tok_sav = NULL;
   cs->dir_state = CanStart;

   return cs;
   }

/*
 * new_me - allocate a new structure for a source of tokens derived
 *  from macro expansion.
 */
struct mac_expand *new_me(m, args, exp_args)
struct macro *m;
struct tok_lst **args;
struct tok_lst **exp_args;
   {
   struct mac_expand *me;

   me = NewStruct(mac_expand);
   me->m = m;
   me->args = args;
   me->exp_args = exp_args;
   me->rest_bdy = m->body;
   return me;
   }

/*
 * new_plsts - allocate a element for a list of token lists used as
 *  as source of tokens derived from a sequence of token pasting
 *  operations.
 */
struct paste_lsts *new_plsts(trigger, tlst, plst)
struct token *trigger;
struct tok_lst *tlst;
struct paste_lsts *plst;
   {
   struct paste_lsts *plsts;

   plsts = NewStruct(paste_lsts);
   plsts->trigger = trigger;
   plsts->tlst = tlst;
   plsts->next = plst;
   return plsts;
   }

/*
 * get_sbuf - dynamically allocate a string buffer.
 */
struct str_buf *get_sbuf()
   {
   struct str_buf *sbuf;

   sbuf = NewStruct(str_buf);
   init_sbuf(sbuf);
   return sbuf;
   }

/*
 * push_src - push an entry on the stack of tokens sources. This entry
 *  becomes the current source.
 */
void push_src(flag, ref)
int flag;
union src_ref *ref;
   {
   struct src *sp;

   sp = NewStruct(src);
   sp->flag = flag;
   sp->cond = NULL;
   sp->u = *ref;
   sp->ntoks = 0;

   if (src_stack->flag == CharSrc)
      src_stack->u.cs->next_char = next_char;
   sp->next = src_stack;
   src_stack = sp;
   }

/*
 * free_t - free a token.
 */
void free_t(t)
struct token *t;
   {
   if (t != NULL)
      free((char *)t);
   }

/*
 * free_t_lst - free a token list.
 */
void free_t_lst(tlst)
struct tok_lst *tlst;
   {
   if (tlst == NULL)
      return;
   free_t(tlst->t);
   free_t_lst(tlst->next);
   free((char *)tlst);
   }

/*
 * free_id_lst - free an identifier list.
 */
void free_id_lst(ilst)
struct id_lst *ilst;
   {
   if (ilst == NULL)
       return;
   free_id_lst(ilst->next);
   free((char *)ilst);
   }

/*
 * free_m - if there are no more pointers to this macro entry, free it
 *  and other associated storage.
 */
void free_m(m)
struct macro *m;
   {
   if (--m->ref_cnt != 0)
      return;
   free_id_lst(m->prmlst);
   free_t_lst(m->body);
   free((char *)m);
   }

/*
 * free_m_lst - free a hash chain of macro symbol table entries.
 */
void free_m_lst(m)
struct macro *m;
   {
   if (m == NULL)
      return;
   free_m_lst(m->next);
   free_m(m);
   }

/*
 * free_plsts - free an entry from a list of token lists used in
 *  token pasting.
 */
void free_plsts(plsts)
struct paste_lsts *plsts;
   {
   free((char *)plsts);
   }

/*
 * rel_sbuf - free a string buffer.
 */
void rel_sbuf(sbuf)
struct str_buf *sbuf;
   {
   free((char *)sbuf);
   }

/*
 * pop_src - pop the top entry from the stack of tokens sources.
 */
void pop_src()
   {
   struct src *sp;
   struct char_src *cs;
   struct mac_expand *me;
   int i;

   if (src_stack->flag == DummySrc)
      return; /* bottom of stack */

   sp = src_stack;
   src_stack = sp->next; /* pop */

   /*
    * If the new current source is a character source, reload global
    *  variables used in tokenizing the characters.
    */
   if (src_stack->flag == CharSrc) {
      first_char = src_stack->u.cs->char_buf;
      next_char = src_stack->u.cs->next_char;
      last_char = src_stack->u.cs->last_char;
      }

   /*
    * Make sure there is no unclosed conditional compilation in the
    *  source we are poping.
    */
   if (sp->cond != NULL)
      errt2(sp->cond->t, "no matching #endif for #", sp->cond->t->image);

   /*
    * Free any storage that the stack entry still references.
    */
   switch (sp->flag) {
      case CharSrc:
         cs = sp->u.cs;
         if (cs->f != NULL)
            fclose(cs->f);
         free((char *)cs);
         break;
      case MacExpand:
         me = sp->u.me;
         if (me->args != NULL) {
            for (i = 0; i < me->m->category; i++) {
               free_t_lst(me->args[i]);
               free_t_lst(me->exp_args[i]);
               }
            free((char *)me->args);
            free((char *)me->exp_args);
            }
         --me->m->recurse;
         free_m(me->m);
         free((char *)me);
         break;
      }

   /*
    * Free the stack entry.
    */
   free((char *)sp);
   }
