/*
 * A linked list of files named by link declarations is maintained using
 *  lfile structures.
 */
struct lfile {
   char *lf_name;		/* name of the file */
   struct lfile *lf_link;	/* pointer to next file */
   };

extern struct lfile *lfiles;


/*
 * "Invocable" declarations are recorded in a list of invkl structs.
 */
struct invkl {
   char *iv_name;		/* name of global */
   struct invkl *iv_link;	/* link to next entry */
   };

extern struct invkl *invkls;
