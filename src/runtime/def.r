/*
 * def.r -- defaulting conversion routines.
 */

/*
 * DefConvert - macro for general form of defaulting conversion.
 */
#begdef DefConvert(default, dftype, destype, converter, body)
int default(s,df,d)
dptr s;
dftype df;
destype d;
   {
   if (is:null(*s)) {
      body
      return 1;
      }
   else
      return converter(s,d); /* I really mean cnv:type */
   }
#enddef

/*
 * def_c_dbl - def:C_double(*s, df, *d), convert to C double with a
 *  default value. Default is of type C double; if used, just copy to
 *  destination.
 */

#begdef C_DblAsgn
   *d = df;
#enddef

DefConvert(def_c_dbl, double, double *, cnv_c_dbl, C_DblAsgn)

/*
 * def_c_int - def:C_integer(*s, df, *d), convert to C_integer with a
 *  default value. Default type C_integer; if used, just copy to
 *  destination.
 */
#begdef C_IntAsgn
   *d = df;
#enddef

DefConvert(def_c_int, C_integer, C_integer *, cnv_c_int, C_IntAsgn)

/*
 * def_c_str - def:C_string(*s, df, *d), convert to (tended) C string with
 *  a default value. Default is of type "char *"; if used, point destination
 *  descriptor to it.
 */

#begdef C_StrAsgn
   StrLen(*d) = strlen(df);
   StrLoc(*d) = (char *)df;
#enddef

DefConvert(def_c_str, char *, dptr, cnv_c_str, C_StrAsgn)

/*
 * def_cset - def:cset(*s, *df, *d), convert to cset with a default value.
 *  Default is of type "struct b_cset *"; if used, point destination descriptor
 *  to it.
 */

#begdef CsetAsgn
   d->dword = D_Cset;
   BlkLoc(*d) = (union block *)df;
#enddef

DefConvert(def_cset, struct b_cset *, dptr, cnv_cset, CsetAsgn)

/*
 * def_ec_int - def:(exact)C_integer(*s, df, *d), convert to C Integer
 *  with a default value, but disallow conversions from reals. Default
 *  is of type C_Integer; if used, just copy to destination.
 */

#begdef EC_IntAsgn
   *d = df;
#enddef

DefConvert(def_ec_int, C_integer, C_integer *, cnv_ec_int, EC_IntAsgn)

/*
 * def_eint - def:(exact)integer(*s, df, *d), convert to C_integer
 *  with a default value, but disallow conversions from reals. Default
 *  is of type C_Integer; if used, assign it to the destination descriptor.
 */

#begdef EintAsgn
   d->dword = D_Integer;
   IntVal(*d) = df;
#enddef

DefConvert(def_eint, C_integer, dptr, cnv_eint, EintAsgn)

/*
 * def_int - def:integer(*s, df, *d), convert to integer with a default
 *  value. Default is of type C_integer; if used, assign it to the
 *  destination descriptor.
 */

#begdef IntAsgn
   d->dword = D_Integer;
   IntVal(*d) = df;
#enddef

DefConvert(def_int, C_integer, dptr, cnv_int, IntAsgn)

/*
 * def_real - def:real(*s, df, *d), convert to real with a default value.
 *  Default is of type double; if used, allocate real block and point
 *  destination descriptor to it.
 */

#begdef RealAsgn
   Protect(BlkLoc(*d) = (union block *)alcreal(df), fatalerr(0,NULL));
   d->dword = D_Real;
#enddef

DefConvert(def_real, double, dptr, cnv_real, RealAsgn)

/*
 * def_str - def:string(*s, *df, *d), convert to string with a default
 *  value. Default is of type "struct descrip *"; if used, copy the
 *  decriptor value to the destination.
 */

#begdef StrAsgn
   *d = *df;
#enddef

DefConvert(def_str, dptr, dptr, cnv_str, StrAsgn)

/*
 * def_tcset - def:tmp_cset(*s, *df, *d), conversion to temporary cset with
 *  a default value. Default is of type "struct b_cset *"; if used,
 *  point destination descriptor to it. Note that this routine needs
 *  a cset buffer (cset block) to perform an actual conversion.
 */
int def_tcset(cbuf, s, df, d)
struct b_cset *cbuf, *df;
dptr s, d;
{
   if (is:null(*s)) {
      d->dword = D_Cset;
      BlkLoc(*d) = (union block *)df;
      return 1;
      }
   return cnv_tcset(cbuf, s, d);
   }

/*
 * def_tstr - def:tmp_string(*s, *df, *d), conversion to temporary string
 *  with a default value. Default is of type "struct descrip *"; if used,
 *  copy it to destination descriptor. Note that this routine needs
 *  a string buffer to perform an actual conversion.
 */
int def_tstr(sbuf, s, df, d)
char *sbuf;
dptr s, df, d;
   {
   if (is:null(*s)) {
      *d = *df;
      return 1;
      }
   return cnv_tstr(sbuf, s, d);
   }
