/*
 * Icon configuration file for a generic UNIX system.
 */

/*
 * Most definitions used to configure Icon source code have defaults
 * that only need to be overriden in specific cases. The defaults are
 * listed in the configuration manual (IPD238).
 *
 * In cases where the defaults are not appropriate, alternative
 * definitions should be given here.
 *
 * The following definitions get things off the ground; they should
 * be altered as needed.  This comment itself should be removed when
 * the installation is complete.
 */

/*
 * Do not remove the following definition. It controls many aspects
 * of conditional assembly that are specific to UNIX systems.
 */
#define UNIX 1

/*
 * Modern Unix systems have dynamic loading functions and getopt().
 */
#define LoadFunc
#define SysOpt

/*
 * Co-expressions require special code.  
 * Get everything else working first.
 */
#define NoCoexpr
