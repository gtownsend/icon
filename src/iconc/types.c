/*
 * typinfer.c - routines to perform type inference.
 */
#include "../h/gsupport.h"
#include "../h/lexdef.h"
#include "ctrans.h"
#include "csym.h"
#include "ctree.h"
#include "ctoken.h"
#include "cglobals.h"
#include "ccode.h"
#include "cproto.h"
#ifdef TypTrc
#ifdef HighResTime
#include <sys/time.h>
#include <sys/resource.h>
#endif					/* HighResTime */
#endif					/* TypTrc */

extern unsigned int null_bit;   /* bit for null type */
extern unsigned int str_bit;    /* bit for string type */
extern unsigned int cset_bit;   /* bit for cset type */
extern unsigned int int_bit;    /* bit for integer type */
extern unsigned int real_bit;   /* bit for real type */
extern unsigned int n_icntyp;   /* number of non-variable types */
extern unsigned int n_intrtyp;  /* number of types in intermediate values */
extern unsigned int val_mask;   /* mask for non-var types in last int of type*/
extern struct typ_info *type_array;

/*
 * free_struct_typinfo - frees a struct typinfo structure by placing
 *    it one a list of free structures
 */
#ifdef OptimizeType
extern struct typinfo *start_typinfo;
extern struct typinfo *high_typinfo;
extern struct typinfo *low_typinfo;
extern struct typinfo *free_typinfo;

void free_struct_typinfo(struct typinfo *typ)  {

   typ->bits = (unsigned int *)free_typinfo;
   free_typinfo = typ;
}
#endif					/* OptimizeType */

/*
 * alloc_typ - allocate a compressed type structure and initializes
 *    the members to zero or NULL.
 */
#ifdef OptimizeType
struct typinfo *alloc_typ(n_types)
#else					/* OptimizeType */
unsigned int *alloc_typ(n_types)
#endif					/* OptimizeType */
int n_types;
{
#ifdef OptimizeType
   struct typinfo *typ;
   int i;
   unsigned int init = 0;

   if ((free_typinfo == NULL) && (high_typinfo == low_typinfo))  {
      /*
       * allocate a large block of memory used to parcel out struct typinfo
       *   structures from
       */
      start_typinfo = (struct typinfo *)alloc(sizeof(struct typinfo) * TYPINFO_BLOCK);
      high_typinfo = start_typinfo;
      low_typinfo = start_typinfo + TYPINFO_BLOCK;
      free_typinfo = NULL;
      typ = start_typinfo;
      high_typinfo++;
   }
   else if (free_typinfo != NULL) {
      /*
       * get a typinfo stucture from the list of free structures
       */
      typ = free_typinfo;
      free_typinfo = (struct typinfo *)free_typinfo->bits;
   }
   else  {
      /*
       * get a typinfo structure from the chunk of memory allocated
       *   previously
       */
      typ = high_typinfo;
      high_typinfo++;
   }
   typ->packed = n_types;
   if (!do_typinfer)
      typ->bits = alloc_mem_typ(n_types);
   else
      typ->bits= NULL;
   return typ;
#else					/* OptimizeType */
   int n_ints;
   unsigned int *typ;
   int i;
   unsigned int init = 0;

   n_ints = NumInts(n_types);
   typ = (unsigned int *)alloc((unsigned int)((n_ints)*sizeof(unsigned int)));

   /*
    * Initialization: if we are doing inference, start out assuming no types.
    *  If we are not doing inference, assume any type.
    */
   if (!do_typinfer)
      init = ~init;
   for (i = 0; i < n_ints; ++i)
     typ[i] = init;
   return typ;
#endif					/* OptimizeType */
}

/*
 * alloc_mem_typ - actually allocates a full sized bit vector.
 */
#ifdef OptimizeType
unsigned int *alloc_mem_typ(n_types)
unsigned int n_types;
{
   int n_ints;
   unsigned int *typ;
   int i;
   unsigned int init = 0;

   n_ints = NumInts(n_types);
   typ = (unsigned int *)alloc((unsigned int)((n_ints)*sizeof(unsigned int)));
   if (!do_typinfer)
      init = ~init;
   for(i=0; i < n_ints ;++i)
      typ[i] = init;
   return typ;
}
#endif					/* OptimizeType */

/*
 * set_typ - set a particular type bit in a type bit vector.
 */
void set_typ(type, bit)
#ifdef OptimizeType
struct typinfo *type;
#else					/* OptimizeType */
unsigned int *type;
#endif					/* OptimizeType */
unsigned int bit;
{
   unsigned int indx;
   unsigned int mask;

#ifdef OptimizeType
   if (type->bits == NULL)  {
      if (bit == null_bit)
         type->packed |= NULL_T; 
      else if (bit == real_bit)
         type->packed |= REAL_T; 
      else if (bit == int_bit)
         type->packed |= INT_T; 
      else if (bit == cset_bit)
         type->packed |= CSET_T; 
      else if (bit == str_bit)
         type->packed |= STR_T; 
      else {
         /*
          * if the bit to set is not one of the five builtin types
          *   then allocate a whole bit vector, copy the packed
          *   bits over, and set the requested bit
          */
         type->bits = alloc_mem_typ(DecodeSize(type->packed));
         xfer_packed_types(type);
         indx = bit / IntBits;
         mask = 1;
         mask <<= bit % IntBits;
         type->bits[indx] |= mask;
      }
   }
   else  {
      indx = bit / IntBits;
      mask = 1;
      mask <<= bit % IntBits;
      type->bits[indx] |= mask;
   }
#else					/* OptimizeType */
   indx = bit / IntBits;
   mask = 1;
   mask <<= bit % IntBits;
   type[indx] |= mask;
#endif					/* OptimizeType */
}

/*
 * clr_type - clear a particular type bit in a type bit vector.
 */
void clr_typ(type, bit)
#ifdef OptimizeType
struct typinfo *type;
#else					/* OptimizeType */
unsigned int *type;
#endif					/* OptimizeType */
unsigned int bit;
{
   unsigned int indx;
   unsigned int mask;

#ifdef OptimizeType
   if (type->bits == NULL)  {
      /*
       * can only clear one of five builtin types
       */
      if (bit == null_bit)
         type->packed &= ~NULL_T;
      else if (bit == real_bit)
         type->packed &= ~REAL_T;
      else if (bit == int_bit)
         type->packed &= ~INT_T;
      else if (bit == cset_bit)
         type->packed &= ~CSET_T;
      else if (bit == str_bit)
         type->packed &= ~STR_T;
   }
   else  {
      /*
       * build bit mask to clear requested type in full bit vector
       */
      indx = bit / IntBits;
      mask = 1;
      mask <<= bit % IntBits;
      type->bits[indx] &= ~mask;
   }
#else					/* OptimizeType */
   indx = bit / IntBits;
   mask = 1;
   mask <<= bit % IntBits;
   type[indx] &= ~mask;
#endif					/* OptimizeType */
}

/*
 * has_type - determine if a bit vector representing types has any bits
 *  set that correspond to a specific type code from the data base.  Also,
 *  if requested, clear any such bits.
 */
int has_type(typ, typcd, clear)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
int typcd;
int clear;
{
   int frst_bit, last_bit;
   int i;
   int found;

   found = 0;
   bitrange(typcd, &frst_bit, &last_bit);
   for (i = frst_bit; i < last_bit; ++i) {
      if (bitset(typ, i)) {
         found = 1;
         if (clear)
            clr_typ(typ, i);
         }
      }
   return found;
}

/*
 * other_type - determine if a bit vector representing types has any bits
 *  set that correspond to a type *other* than specific type code from the
 *  data base.
 */
int other_type(typ, typcd)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
int typcd;
   {
   int frst_bit, last_bit;
   int i;

   bitrange(typcd, &frst_bit, &last_bit);
   for (i = 0; i < frst_bit; ++i)
      if (bitset(typ, i))
         return 1;
   for (i = last_bit; i < n_intrtyp; ++i)
      if (bitset(typ, i))
         return 1;
   return 0;
   }

/*
 * bitrange - determine the range of bit positions in a type bit vector
 *  that correspond to a type code from the data base.
 */
void bitrange(typcd, frst_bit, last_bit)
int typcd;
int *frst_bit;
int *last_bit;
   {
   if (typcd == TypVar) {
      /*
       * All variable types.
       */
      *frst_bit = n_icntyp;
      *last_bit = n_intrtyp;
      }
   else {
      *frst_bit = type_array[typcd].frst_bit;
      *last_bit = *frst_bit + type_array[typcd].num_bits;
      }
   }

/*
 * typcd_bits - set the bits of a bit vector corresponding to a type
 *  code from the data base.
 */
void typcd_bits(typcd, typ)
int typcd;
struct type *typ;
   {
   int frst_bit;
   int last_bit;
   int i;

   if (typcd == TypEmpty)
      return;  /* Do nothing.  */

   if (typcd == TypAny) {
      /*
       * Set bits corresponding to first-class types.
       */
#ifdef OptimizeType
      /*
       * allocate a full bit vector and copy over packed types first
       */
      if (typ->bits->bits == NULL)   {
          typ->bits->bits = alloc_mem_typ(DecodeSize(typ->bits->packed));
          xfer_packed_types(typ->bits);
      }
      for (i = 0; i < NumInts(n_icntyp) - 1; ++i)
         typ->bits->bits[i] |= ~(unsigned int)0;
      typ->bits->bits[i] |= val_mask;
#else					/* OptimizeType */
      for (i = 0; i < NumInts(n_icntyp) - 1; ++i)
         typ->bits[i] |= ~(unsigned int)0;
      typ->bits[i] |= val_mask;
#endif					/* OptimizeType */
      return;
      }

   bitrange(typcd, &frst_bit, &last_bit);
#ifdef OptimizeType
   if (last_bit > DecodeSize(typ->bits->packed)) /* bad abstract type computation */
      return;
#endif					/* OptimizeType */
   for (i = frst_bit; i < last_bit; ++i)
      set_typ(typ->bits, i);
   }

/*
 * bitset - determine if a specific bit in a bit vector is set.
 */
int bitset(typ, bit)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
int bit;
{
   int mask;
   int indx;

#ifdef OptimizeType
   if (typ->bits == NULL)  {
      /*
       * check to see if the requested bit is set in the packed representation
       *   if the requested bit is not one of the five builtins then the
       *   lookup fails no matter what
       */
      if (bit == null_bit)
         return (typ->packed & NULL_T);
      else if (bit == real_bit)
         return (typ->packed & REAL_T);
      else if (bit == int_bit)
         return (typ->packed & INT_T);
      else if (bit == cset_bit)
         return (typ->packed & CSET_T);
      else if (bit == str_bit)
         return (typ->packed & STR_T);
      else 
         return 0;
   }
   else  {
      /*
       * create a mask to check to see if the requested type bit is
       *   set on
       */
      indx = bit / IntBits;
      mask = 1;
      mask <<= bit % IntBits;
      return typ->bits[indx] & mask;
   }
#else					/* OptimizeType */
   indx = bit / IntBits;
   mask = 1;
   mask <<= bit % IntBits;
   return typ[indx] & mask;
#endif					/* OptimizeType */
}

/*
 * is_empty - determine if a type bit vector is empty.
 */
int is_empty(typ)
#ifdef OptimizeType
struct typinfo *typ;
#else					/* OptimizeType */
unsigned int *typ;
#endif					/* OptimizeType */
{
   int i;

#ifdef OptimizeType
   if (typ->bits == NULL)  {
      /*
       * if any bits are set on then the vector is not empty
       */
      if (DecodePacked(typ->packed))
         return 0;
      else
         return 1;
   }
   else  {
      for (i = 0; i < NumInts(n_intrtyp); ++i) {
          if (typ->bits[i] != 0)
             return 0;
          }
      return 1;
   }
#else					/* OptimizeType */
   for (i = 0; i < NumInts(n_intrtyp); ++i) {
       if (typ[i] != 0)
          return 0;
       }
   return 1;
#endif					/* OptimizeType */
}

/*
 * xfer_packed_types - transfers the packed type representation
 *   to a full length bit vector representation in the same
 *   struct typinfo structure.
 */
#ifdef OptimizeType
void xfer_packed_types(type)
struct typinfo *type;
{
   unsigned int indx, mask;

   /*
    * for each IF statement built a mask to set each of the five builtins
    *   if they are present in the packed representation
    */
   if (type->packed & NULL_T)  {
      indx = null_bit / IntBits;
      mask = 1;
      mask <<= null_bit % IntBits;
      type->bits[indx] |= mask;
   }
   if (type->packed & REAL_T)  {
      indx = real_bit / IntBits;
      mask = 1;
      mask <<= real_bit % IntBits;
      type->bits[indx] |= mask;
   }
   if (type->packed & INT_T)  {
      indx = int_bit / IntBits;
      mask = 1;
      mask <<= int_bit % IntBits;
      type->bits[indx] |= mask;
   }
   if (type->packed & CSET_T)  {
      indx = cset_bit / IntBits;
      mask = 1;
      mask <<= cset_bit % IntBits;
      type->bits[indx] |= mask;
   }
   if (type->packed & STR_T)  {
      indx = str_bit / IntBits;
      mask = 1;
      mask <<= str_bit % IntBits;
      type->bits[indx] |= mask;
   }
}

/*
 * xfer_packed_to_bits - sets those type bits from the src typinfo structure
 *     to the dest typinfo structure AND the src is a packed representation
 *     while the dest is a bit vector. Returns the number of new bits that
 *     were set in the destination.
 */
int xfer_packed_to_bits(src, dest, nsize)
struct typinfo *src;
struct typinfo *dest;
int nsize;
{
   unsigned int indx, mask, old, rnsize;
   int changes[5] = {-1,-1,-1,-1,-1};
   int ix, membr = 0, i;

   ix = 0;
   rnsize = NumInts(nsize);
   /*
    * for each possible type set in the packed vector, create a mask
    *   and apply it to the dest.  check to see if there was actually
    *   a change in the dest vector.
    */
   if (src->packed & NULL_T)  {
      indx = null_bit / IntBits;
      if (indx < rnsize)  {
         mask = 1;
         mask <<= null_bit % IntBits;
         old = dest->bits[indx];
         dest->bits[indx] |= mask;
         if (old != dest->bits[indx])  {
            membr = 0;
            for (i=0; i < 5 ;i++)
               /*
                * checks to see if the bit just set happens to be in the
                *   same word as any other of the five builtins. if they
                *   are then we only want to count this as one change 
                */
               if (indx == changes[i]) {
                  membr = 1; break;
               }
            if (!membr)
               changes[ix++] = indx;
         }
      }
   }
   if (src->packed & REAL_T)  {
      indx = real_bit / IntBits;
      if (indx < rnsize)  {
         mask = 1;
         mask <<= real_bit % IntBits;
         old = dest->bits[indx];
         dest->bits[indx] |= mask;
         if (old != dest->bits[indx])  {
            membr = 0;
            for (i=0; i < 5 ;i++)
               if (indx == changes[i]) {
                  membr = 1; break;
               }
            if (!membr)
               changes[ix++] = indx;
         }
      }
   }
   if (src->packed & INT_T)  {
      indx = int_bit / IntBits;
      if (indx < rnsize)  {
         mask = 1;
         mask <<= int_bit % IntBits;
         old = dest->bits[indx];
         dest->bits[indx] |= mask;
         if (old != dest->bits[indx])  {
            membr = 0;
            for (i=0; i < 5 ;i++)
               if (indx == changes[i]) {
                  membr = 1; break;
               }
            if (!membr)
               changes[ix++] = indx;
         }
      }
   }
   if (src->packed & CSET_T)  {
      indx = cset_bit / IntBits;
      if (indx < rnsize)  {
         mask = 1;
         mask <<= cset_bit % IntBits;
         old = dest->bits[indx];
         dest->bits[indx] |= mask;
         if (old != dest->bits[indx])  {
            membr = 0;
            for (i=0; i < 5 ;i++)
               if (indx == changes[i]) {
                  membr = 1; break;
               }
            if (!membr)
               changes[ix++] = indx;
         }
      }
   }
   if (src->packed & STR_T)  {
      indx = str_bit / IntBits;
      if (indx < rnsize)  {
         mask = 1;
         mask <<= str_bit % IntBits;
         old = dest->bits[indx];
         dest->bits[indx] |= mask;
         if (old != dest->bits[indx])  {
            membr = 0;
            for (i=0; i < 5 ;i++)
               if (indx == changes[i]) {
                  membr = 1; break;
               }
            if (!membr)
               changes[ix++] = indx;
         }
      }
   }
   return ix;
}

/*
 * and_bits_to_packed - performs a bitwise AND of two typinfo structures
 *    taking into account of packed or full bit representation.
 */
void and_bits_to_packed(src, dest, size)
struct typinfo *src;
struct typinfo *dest;
int size;
{
   unsigned int indx, mask, val, destsz;
   int  i;

   if ((src->bits == NULL) && (dest->bits == NULL)) 
      /* Both are packed */
      dest->packed &= (0xFF000000 | src->packed);
   else if ((src->bits == NULL) && (dest->bits != NULL))  {
      /*
       * built a bit mask for each type in the src and AND it too
       *  the bit vector in dest
       */
      for (i=0; i < NumInts(size) ;i++) {
         val = get_bit_vector(src,i);
         dest->bits[i] &= val; 
      }
   }
   else if ((src->bits != NULL) && (dest->bits == NULL))  {
      /*
       * because an AND is being performed only those bits in the dest
       *  have the possibility of remaining set (i.e. five builtins)
       *  therefore if the bit is set in the packed check to see if 
       *  it is also set in the full vector, if so then set it in the
       *  resulting vector, otherwise don't
       */
      destsz = DecodeSize(dest->packed);
      mask = 1; val = 0;
      if (dest->packed & NULL_T) { 
         mask <<= (null_bit % IntBits);
         if (src->bits[(null_bit/IntBits)] & mask)
            val |= NULL_T;
      }
      mask = 1;
      if (dest->packed & REAL_T) { 
         mask <<= (real_bit % IntBits);
         if (src->bits[(real_bit/IntBits)] & mask)
            val |= REAL_T;
      }
      mask = 1;
      if (dest->packed & INT_T) {
         mask <<= (int_bit % IntBits);
         if (src->bits[(int_bit/IntBits)] & mask)
            val |= INT_T;
      }
      mask = 1;
      if (dest->packed & CSET_T) {
         mask <<= (cset_bit % IntBits);
         if (src->bits[(cset_bit/IntBits)] & mask)
            val |= CSET_T;
      }
      mask = 1;
      if (dest->packed & STR_T) { 
         mask <<= (str_bit % IntBits);
         if (src->bits[(str_bit/IntBits)] & mask)
            val |= STR_T;
      }
      dest->packed = val | destsz;
   }
   else
      for (i=0; i < NumInts(size) ;i++)
         dest->bits[i] &= src->bits[i];
}


/*
 * get_bit_vector - returns a bit mask from the selected word of a bit
 *    vector.  e.g. if pos == 2, then check to see if any of the five
 *    builtins fall in the second word of a normal bit vector, if so
 *    create a bit mask with those types that fall in that word.
 */

unsigned int get_bit_vector(src, pos)
struct typinfo *src;
int pos;
{
   unsigned int val = 0, mask;

   val = 0;
   if ((src->packed & NULL_T) && ((null_bit / IntBits) == pos))  {
      mask = 1;
      mask <<= null_bit % IntBits;
      val |= mask; 
   }
   if ((src->packed & REAL_T) && ((real_bit / IntBits) == pos))  {
      mask = 1;
      mask <<= real_bit % IntBits;
      val |= mask; 
   }
   if ((src->packed & INT_T) && ((int_bit / IntBits) == pos))  {
      mask = 1;
      mask <<= int_bit % IntBits;
      val |= mask; 
   }
   if ((src->packed & CSET_T) && ((cset_bit / IntBits) == pos))  {
      mask = 1;
      mask <<= cset_bit % IntBits;
      val |= mask; 
   }
   if ((src->packed & STR_T) && ((str_bit / IntBits) == pos))  {
      mask = 1;
      mask <<= str_bit % IntBits;
      val |= mask; 
   }
   return val;
}


/*
 * clr_packed - clears all bits within the nsize-th word for a packed
 *     representation.
 */

void clr_packed(src, nsize)
struct typinfo *src;
int nsize;
{
   unsigned int rnsize;

   rnsize = NumInts(nsize);
   if ((null_bit / IntBits) < rnsize)
      src->packed &= ~NULL_T;
   if ((real_bit / IntBits) < rnsize)
      src->packed &= ~REAL_T;
   if ((int_bit / IntBits) < rnsize)
      src->packed &= ~INT_T;
   if ((cset_bit / IntBits) < rnsize)
      src->packed &= ~CSET_T;
   if ((str_bit / IntBits) < rnsize)
      src->packed &= ~STR_T;
}

/*
 * cpy_packed_to_packed - copies the packed bits from one bit vector
 *     to another.
 */

void cpy_packed_to_packed(src, dest, nsize)
struct typinfo *src;
struct typinfo *dest;
int nsize;
{
   unsigned int indx, rnsize;

   rnsize = NumInts(nsize);
   /*
    * for each of the possible builtin types, check to see if the bit is
    *  set in the src and if present set it in the dest.
    */
   dest->packed = DecodeSize(dest->packed);
   if (src->packed & NULL_T)  {
      indx = null_bit / IntBits;
      if (indx < rnsize)
          dest->packed |= NULL_T; 
   }
   if (src->packed & REAL_T)  {
      indx = real_bit / IntBits;
      if (indx < rnsize)
          dest->packed |= REAL_T; 
   }
   if (src->packed & INT_T)  {
      indx = int_bit / IntBits;
      if (indx < rnsize)
          dest->packed |= INT_T; 
   }
   if (src->packed & CSET_T)  {
      indx = cset_bit / IntBits;
      if (indx < rnsize)
          dest->packed |= CSET_T; 
   }
   if (src->packed & STR_T)  {
      indx = str_bit / IntBits;
      if (indx < rnsize)
          dest->packed |= STR_T; 
   }
}


/*
 * mrg_packed_to_packed - merges the packed type bits of a src and dest
 *     bit vector.
 */
int mrg_packed_to_packed(src, dest, nsize)
struct typinfo *src;
struct typinfo *dest;
int nsize;
{
   unsigned int indx, rnsize;
   int changes[5] = {-1,-1,-1,-1,-1};
   int ix = 0, membr = 0, i;

   rnsize = NumInts(nsize);
   /*
    * for each of the five possible types in the src, check to see if it
    *  is set in the src and not set in the dest.  if so then set it in
    *  the dest vector.
    */
   if ((src->packed & NULL_T) && !(dest->packed & NULL_T))  {
      indx = null_bit / IntBits;
      if (indx < rnsize) {
         dest->packed |= NULL_T;
         for(i=0; i<5 ;i++)  {
            if (indx == changes[i])  {
               membr = 1; break;
            }
         }
         if (!membr)
            changes[ix++] = indx;
      }
   }
   if ((src->packed & REAL_T) && !(dest->packed & REAL_T))  {
      indx = real_bit / IntBits;
      if (indx < rnsize)  {
         dest->packed |= REAL_T;
         for(i=0; i<5 ;i++)  {
            if (indx == changes[i])  {
               membr = 1; break;
            }
         }
         if (!membr)
            changes[ix++] = indx;
      }
   }
   if ((src->packed & INT_T) && !(dest->packed & INT_T)){
      indx = int_bit / IntBits;
      if (indx < rnsize) {
         dest->packed |= INT_T;
         for(i=0; i<5 ;i++)  {
            if (indx == changes[i])  {
               membr = 1; break;
            }
         }
         if (!membr)
            changes[ix++] = indx;
      }
   }
   if ((src->packed & CSET_T) && !(dest->packed & CSET_T))  {
      indx = cset_bit / IntBits;
      if (indx < rnsize)  {
         dest->packed |= CSET_T;
         for(i=0; i<5 ;i++)  {
            if (indx == changes[i])  {
               membr = 1; break;
            }
         }
         if (!membr)
            changes[ix++] = indx;
      }
   }
   if ((src->packed & STR_T) && !(dest->packed & STR_T))  {
      indx = str_bit / IntBits;
      if (indx < rnsize)  {
         dest->packed |= STR_T;
         for(i=0; i<5 ;i++)  {
            if (indx == changes[i])  {
               membr = 1; break;
            }
         }
         if (!membr)
            changes[ix++] = indx;
      }
   }
   return ix;
}
#endif					/* OptimizeType */
