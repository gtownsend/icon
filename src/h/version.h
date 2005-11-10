/*
 * version.h -- version identification
 */

#undef DVersion
#undef Version
#undef UVersion
#undef IVersion

/*
 *  Icon version number and date.
 *  These are the only two entries that change any more.
 */
#define VersionNumber "9.4.3"
#define VersionDate "November 14, 2005"

/*
 * Version number to insure format of data base matches version of iconc
 *  and rtt.
 */
#define DVersion "9.0.00"

#if COMPILER

   /*
    * &version
    */
   #define Version  "Icon Version " VersionNumber "-C, " VersionDate

#else					/* COMPILER */

   /*
    *  &version
    */
   #define Version  "Icon Version " VersionNumber ", " VersionDate

   /*
    * Version numbers to be sure that ucode is compatible with the linker
    * and that icode is compatible with the run-time system.
    */

   #define UVersion "U9.0.00"

   #ifdef FieldTableCompression

      #if IntBits == 32
         #define IVersion "I9.2.00FT/32"
      #endif				/* IntBits == 32 */

      #if IntBits == 64
         #define IVersion "I9.2.00FT/64"
      #endif				/* IntBits == 64 */

   #else				/* FieldTableCompression */

      #if IntBits == 32
         #define IVersion "I9.0.00/32"
      #endif				/* IntBits == 32 */

      #if IntBits == 64
         #define IVersion "I9.0.00/64"
      #endif				/* IntBits == 64 */

   #endif				/* FieldTableCompression */

#endif					/* COMPILER */
