/*
 * Sun Mar 23 09:43:59 2008
 * This file was produced by
 *   rtt: Icon Version 9.5.a-C, Autumn, 2007
 */
// and then modified by cs 

 
#define COMPILER 0
extern "C" {
#include RTT
}

//#line 42 "fload.r"

//int glue();					//cs
//int makefunc(dptr d, char *name, int (*func)());	//cs
//int Zloadfunc (dptr r_args);	//cs
//FncBlock(loadfunc, 2, 0)		//cs

//cs  new makefunc that allocates a proc_block
static int newmakefunc(dptr d, char *name, int (*func)(), int arity) {
	value nom(NewString,name);
	proc_block* pbp;
	if( arity < 0 ) pbp = new proc_block(nom, (iconfvbl*)func);
			else pbp = new proc_block(nom, (iconfunc*)func, arity);
	if( pbp==0 ) return 0;
	d->dword = D_Proc;
	d->vword.bptr = (union block *)pbp;	
	return 1;
}
//cs  end of new makefunc

//int Zloadfunc(r_args)		//cs
//dptr r_args;				//cs
inline int Z_loadfunc(dptr r_args)		//cs
   {
   if (!cnv_c_str(&(r_args[1]), &(r_args[1]))) {
      {
      err_msg(

//#line 50 "fload.r"

      103, &(r_args[1]));
      return A_Resume;
      }
      }

//#line 51 "fload.r"

   if (!cnv_c_str(&(r_args[2]), &(r_args[2]))) {
      {
      err_msg(

//#line 52 "fload.r"

      103, &(r_args[2]));
      return A_Resume;
      }
      }
//cs new third parameter: arity
   C_integer r_i2;
   if (!cnv_c_int(&(r_args[3]), &(r_i2))) {
      err_msg(101, &(r_args[3]));
      return A_Resume;
   }
//cs end new third arity parameter

//#line 58 "fload.r"

   {
   int (*func)(); 
   static char *curfile; 
   static void *handle; 
   char *funcname2;

//#line 67 "fload.r"

   if (!handle || !curfile || strcmp(r_args[1].vword.sptr, curfile) != 0) {
      if (curfile) 
         free((pointer)curfile);
      curfile = salloc(r_args[1].vword.sptr);
      handle = dlopen(r_args[1].vword.sptr, 1 | RTLD_GLOBAL);
      }

//#line 76 "fload.r"

   if (handle) {
      func = (int (*)())dlsym(handle, r_args[2].vword.sptr);
      if (!func) {

//#line 83 "fload.r"

         //funcname2 = malloc(strlen(r_args[2].vword.sptr) + 2);		//cs
         funcname2 = (char*)malloc(strlen(r_args[2].vword.sptr) + 2);	//cs
         if (funcname2) {
            *funcname2 = '_';
            strcpy(funcname2 + 1, r_args[2].vword.sptr);
            func = (int (*)())dlsym(handle, funcname2);
            free(funcname2);
            }
         }
      }
   if (!handle || !func) {
      //fprintf(stderr, "\nloadfunc(\"%s\",\"%s\"): %s\n", 	//cs
      fprintf(stderr, "\nloadfuncpp(\"%s\",\"%s\"): %s\n",	//cs 
      r_args[1].vword.sptr, r_args[2].vword.sptr, dlerror());
      {
      err_msg(

//#line 95 "fload.r"

      216, NULL);
      return A_Resume;
      }
      }

//   if (!makefunc(&r_args[0], r_args[2].vword.sptr, func))		//cs 
   if (!newmakefunc(&r_args[0], r_args[2].vword.sptr, func, r_i2))	//cs 
      {
      err_msg(

//#line 101 "fload.r"

      305, NULL);
      return A_Resume;
      }
   {
   return A_Continue;
   }
   }
   }


#if 0	//cs --- not used: we use a proc_block constructor, and no glue

//#line 111 "fload.r"

int makefunc(d, name, func) 
dptr d; 
char *name; 
int (*func)(); 
   {
   struct b_proc *blk;

   blk = (struct b_proc *)malloc(sizeof(struct b_proc));
   if (!blk) 
      return 0;
   blk->title = T_Proc;
   blk->blksize = sizeof(struct b_proc);

//#line 127 "fload.r"

   blk->entryp.ccode = glue;

//#line 130 "fload.r"

   blk->nparam = -1;
   blk->ndynam = -1;
   blk->nstatic = 0;
   blk->fstatic = 0;
   blk->pname.dword = strlen(name);
   blk->pname.vword.sptr = salloc(name);
   blk->lnames[0].dword = 0;
   blk->lnames[0].vword.sptr = (char *)func;

   d->dword = D_Proc;
   d->vword.bptr = (union block *)blk;
   return 1;
   }

//#line 190 "fload.r"

int glue(argc, dargv) 
int argc; 
dptr dargv; 
   {
   int status, (*func)(); 
   struct b_proc *blk; 
   struct descrip r;
   struct {
      struct tend_desc *previous;
      int num;
      struct descrip d[1];
      } r_tend;

   r_tend.d[0].dword = D_Null;
   r_tend.num = 1;
   r_tend.previous = tend;
   tend = (struct tend_desc *)&r_tend;

//#line 199 "fload.r"

   blk = (struct b_proc *)dargv[0].vword.bptr;
   func = (int (*)())blk->lnames[0].vword.sptr;

   r_tend.d[0] = dargv[0];
   dargv[0] = nulldesc;
   status = (*func)(argc, dargv);

   if (status == 0) {
      tend = r_tend.previous;

//#line 207 "fload.r"

      return A_Continue;
      }

//#line 208 "fload.r"

   if (status < 0) {
      tend = r_tend.previous;

//#line 209 "fload.r"

      return A_Resume;
      }
   r = dargv[0];
   dargv[0] = r_tend.d[0];
   if (((r).dword == D_Null)) 
      do {err_msg((int)status, NULL);{
         tend = r_tend.previous;
         return A_Resume;
         }
         }
      while (0);

//#line 215 "fload.r"

   do {err_msg((int)status, &r);{
      tend = r_tend.previous;
      return A_Resume;
      }
      }
   while (0);
   }

#endif //cs unused

