/*
 * Typedefs and macros for filename wildcard expansion for Windows.
 *  The definitions provided here are:
 *
 *    typedef ... FINDFILE_T;
 *    // Current state of the filename wildcard expansion
 *
 *    int FINDFIRST ( char *pattern, FINDFILE_T *pfd );
 *    // Initializes *pfd and returns 1 if a file is found that matches the
 *    // pattern, 0 otherwise
 *
 *    int FINDNEXT ( FINDFILE_T *pfd );
 *    // Assuming that the last FINDFIRST/FINDNEXT call was successful,
 *    // updates *pfd and returns whether another match can be made.
 *
 *    char *FILENAME ( FINDFILE_T *pfd );
 *    // Assuming that the last FINDFIRST/FINDNEXT call was successful,
 *    // returns pointer to last found file name.
 *
 *    void FINDCLOSE ( FINDFILE_T *pfd );
 *    // Does any cleanup required after doing filenaame wildcard expansion.
 *
 * Also, the macro WildCards will be defined to be 1 if there is file
 * pattern matching is supported, 0 otherwise.
 */

#if WildCards
   #if NT
      typedef struct _FINDFILE_TAG {
         HANDLE           handle;
         WIN32_FIND_DATAA fileinfo;
         } FINDDATA_T;
      #define FINDFIRST(pattern, pfd) ( \
         ( (pfd)->handle = FindFirstFileA( (pattern), &(pfd)->fileinfo ) ) != \
         INVALID_HANDLE_VALUE )
      #define FINDNEXT(pfd) \
         ( FindNextFileA( (pfd)->handle, &(pfd)->fileinfo ) != FALSE )
      #define FILENAME(pfd)	( (pfd)->fileinfo.cFileName )
      #define FINDCLOSE(pfd)	FindClose( (pfd)->handle )
   #endif				/* NT */
#endif					/* WildCards */
