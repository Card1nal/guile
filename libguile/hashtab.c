/* Copyright (C) 1995,1996,1998,1999,2000,2001, 2003 Free Software Foundation, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 *
 * As a special exception, the Free Software Foundation gives permission
 * for additional uses of the text contained in its release of GUILE.
 *
 * The exception is that, if you link the GUILE library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the GUILE library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the
 * Free Software Foundation under the name GUILE.  If you copy
 * code from other Free Software Foundation releases into a copy of
 * GUILE, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for GUILE, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.  */




#include "libguile/_scm.h"
#include "libguile/alist.h"
#include "libguile/hash.h"
#include "libguile/eval.h"
#include "libguile/root.h"
#include "libguile/vectors.h"
#include "libguile/ports.h"
#include "libguile/weaks.h"

#include "libguile/validate.h"
#include "libguile/hashtab.h"


/* Hash tables are either vectors of association lists or smobs
   containing such vectors.  Currently, the vector version represents
   constant size tables while those wrapped in a smob represents
   resizing tables.

   Growing or shrinking, with following rehashing, is triggered when
   the load factor

     L = N / S    (N: number of items in table, S: bucket vector length)

   passes an upper limit of 0.9 or a lower limit of 0.25.

   The implementation stores the upper and lower number of items which
   trigger a resize in the hashtable object.

   Possible hash table sizes (primes) are stored in the array
   hashtable_size.
 */

/*fixme* Update n_items correctly for weak tables.  This can be done
  by representing such tables with ordinary vectors and adding a scan
  function to the before sweep hook similarly to what is done in weaks.c.
 */

#define SCM_HASHTABLE_P(x)	   SCM_TYP16_PREDICATE (scm_tc16_hashtable, x)
#define SCM_HASHTABLE_VECTOR(x)	   SCM_CELL_OBJECT_1 (x)
#define SCM_SET_HASHTABLE_VECTOR(x, v) SCM_SET_CELL_OBJECT_1 (x, v)
#define SCM_HASHTABLE(x)	   ((scm_t_hashtable *) SCM_CELL_WORD_2 (x))
#define SCM_HASHTABLE_N_ITEMS(x)   (SCM_HASHTABLE (x)->n_items)
#define SCM_HASHTABLE_INCREMENT(x) (SCM_HASHTABLE_N_ITEMS(x)++)
#define SCM_HASHTABLE_DECREMENT(x) (SCM_HASHTABLE_N_ITEMS(x)--)
#define SCM_HASHTABLE_UPPER(x)     (SCM_HASHTABLE (x)->upper)
#define SCM_HASHTABLE_LOWER(x)     (SCM_HASHTABLE (x)->lower)

scm_t_bits scm_tc16_hashtable;

typedef struct scm_t_hashtable {
  unsigned long n_items;	/* number of items in table */
  unsigned long lower;		/* when to shrink */
  unsigned long upper;		/* when to grow */
  int size_index;		/* index into hashtable_size */
} scm_t_hashtable;

#define HASHTABLE_SIZE_N 25

static unsigned long hashtable_size[] = {
  31, 61, 113, 223, 443, 883, 1759, 3517, 7027, 14051, 28099, 56197, 112363,
  224717, 449419, 898823, 1797641, 3595271, 7190537, 14381041, 28762081,
  57524111, 115048217, 230096423, 460192829 /* larger values can't be
					       represented as INUMs */
};

/* Turn an empty vector hash table into an opaque resizable one. */

static char *s_hashtable = "hashtable";

SCM
scm_vector_to_hash_table (SCM vector) {
  SCM table;
  scm_t_hashtable *t = scm_gc_malloc (sizeof (*t), s_hashtable);
  int i = 0, len = SCM_VECTOR_LENGTH (vector);
  while (i < HASHTABLE_SIZE_N && len > hashtable_size[i])
    ++i;
  if (i > 0)
    i = i - 1;
  t->size_index = i;
  t->n_items = 0;
  if (i == 0)
    t->lower = 0;
  else
    t->lower = hashtable_size[i] / 4;
  t->upper = 9 * hashtable_size[i] / 10;
  SCM_NEWSMOB2 (table, scm_tc16_hashtable, vector, t);
  return table;
}

static int
hashtable_print (SCM exp, SCM port, scm_print_state *pstate SCM_UNUSED)
{
  scm_t_hashtable *t = SCM_HASHTABLE (exp);
  scm_puts ("#<resizing-hash-table ", port);
  scm_intprint ((unsigned long)t->n_items, 10, port);
  scm_putc ('/', port);
  scm_intprint ((unsigned long) SCM_VECTOR_LENGTH (SCM_HASHTABLE_VECTOR (exp)),
		10, port);
  scm_puts (">", port);
  return 1;
}

static size_t
hashtable_free (SCM obj)
{
  scm_gc_free (SCM_HASHTABLE (obj), sizeof (scm_t_hashtable), s_hashtable);
  return 0;
}


SCM
scm_c_make_hash_table (unsigned long k)
{
  return scm_c_make_vector (k, SCM_EOL);
}

SCM
scm_c_make_resizing_hash_table ()
{
  return scm_vector_to_hash_table (scm_c_make_vector (31, SCM_EOL));
}

SCM_DEFINE (scm_make_hash_table, "make-hash-table", 0, 1, 0,
	    (SCM n),
	    "Make a hash table with constant number of buckets @var{n}\n"
	    "If called with zero arguments, create a resizing hash table.")
#define FUNC_NAME s_scm_make_hash_table
{
  if (SCM_UNBNDP (n))
    return scm_c_make_resizing_hash_table ();
  else
    {
      int k;
      SCM_VALIDATE_INUM_COPY (1, n, k);
      return scm_c_make_hash_table (k);
    }
}
#undef FUNC_NAME

static void
rehash (SCM table, unsigned long (*hash_fn)(), void *closure)
{
  SCM buckets, new_buckets;
  int i;
  unsigned long old_size;
  unsigned long new_size;
  
  if (SCM_HASHTABLE_N_ITEMS (table) < SCM_HASHTABLE_LOWER (table))
    /* rehashing is never triggered when i == 0 */
    i = --SCM_HASHTABLE (table)->size_index;
  else
    {
      i = SCM_HASHTABLE (table)->size_index + 1;
      if (i < HASHTABLE_SIZE_N)
	SCM_HASHTABLE (table)->size_index = i;
      else
	/* don't rehash */
	return;
    }
  
  new_size = hashtable_size[i];
  if (i == 0)
    SCM_HASHTABLE (table)->lower = 0;
  else
    SCM_HASHTABLE (table)->lower = new_size / 4;
  SCM_HASHTABLE (table)->upper = 9 * new_size / 10;
  buckets = SCM_HASHTABLE_VECTOR (table);
  
  if (SCM_VECTORP (buckets))
    new_buckets = scm_c_make_vector (new_size, SCM_EOL);
  else
    switch (SCM_WVECT_TYPE (buckets)) {
    case 1:
      new_buckets = scm_make_weak_key_hash_table (SCM_MAKINUM (new_size));
      break;
    case 2:
      new_buckets = scm_make_weak_value_hash_table (SCM_MAKINUM (new_size));
      break;
    case 3:
      new_buckets = scm_make_doubly_weak_hash_table (SCM_MAKINUM (new_size));
      break;
    default:
      abort (); /* never reached */
    }

  old_size = SCM_VECTOR_LENGTH (buckets);
  for (i = 0; i < old_size; ++i)
    {
      SCM ls = SCM_VELTS (buckets)[i], handle;
      while (!SCM_NULLP (ls))
	{
	  unsigned long h;
	  if (!SCM_CONSP (ls))
	    break;
	  handle = SCM_CAR (ls);
	  if (!SCM_CONSP (handle))
	    continue;
	  h = hash_fn (SCM_CAR (handle), new_size, closure);
	  if (h >= new_size)
	    scm_out_of_range ("hash_fn_create_handle_x", scm_ulong2num (h));
	  SCM_VECTOR_SET (new_buckets, h,
			  scm_cons (handle, SCM_VELTS (new_buckets)[h]));
	  ls = SCM_CDR (ls);
	}
    }
  SCM_SET_HASHTABLE_VECTOR (table, new_buckets);
}

SCM
scm_hash_fn_get_handle (SCM table, SCM obj, unsigned long (*hash_fn)(), SCM (*assoc_fn)(), void * closure)
#define FUNC_NAME "scm_hash_fn_get_handle"
{
  unsigned long k;
  SCM h;

  if (SCM_HASHTABLE_P (table))
    table = SCM_HASHTABLE_VECTOR (table);
  else
    SCM_VALIDATE_VECTOR (1, table);
  if (SCM_VECTOR_LENGTH (table) == 0)
    return SCM_BOOL_F;
  k = hash_fn (obj, SCM_VECTOR_LENGTH (table), closure);
  if (k >= SCM_VECTOR_LENGTH (table))
    scm_out_of_range ("hash_fn_get_handle", scm_ulong2num (k));
  h = assoc_fn (obj, SCM_VELTS (table)[k], closure);
  return h;
}
#undef FUNC_NAME


SCM
scm_hash_fn_create_handle_x (SCM table, SCM obj, SCM init, unsigned long (*hash_fn)(),
                             SCM (*assoc_fn)(), void * closure)
#define FUNC_NAME "scm_hash_fn_create_handle_x"
{
  unsigned long k;
  SCM buckets, it;

  if (SCM_HASHTABLE_P (table))
    buckets = SCM_HASHTABLE_VECTOR (table);
  else
    {
      SCM_ASSERT (SCM_VECTORP (table),
		  table, SCM_ARG1, "hash_fn_create_handle_x");
      buckets = table;
    }
  if (SCM_VECTOR_LENGTH (buckets) == 0)
    SCM_MISC_ERROR ("void hashtable", SCM_EOL);

  k = hash_fn (obj, SCM_VECTOR_LENGTH (buckets), closure);
  if (k >= SCM_VECTOR_LENGTH (buckets))
    scm_out_of_range ("hash_fn_create_handle_x", scm_ulong2num (k));
  it = assoc_fn (obj, SCM_VELTS (buckets)[k], closure);
  if (!SCM_FALSEP (it))
    return it;
  else
    {
      SCM old_bucket = SCM_VELTS (buckets)[k];
      SCM new_bucket = scm_acons (obj, init, old_bucket);
      SCM_VECTOR_SET (buckets, k, new_bucket);
      if (table != buckets)
	{
	  SCM_HASHTABLE_INCREMENT (table);
	  if (SCM_HASHTABLE_N_ITEMS (table) > SCM_HASHTABLE_UPPER (table))
	    {
	      rehash (table, hash_fn, closure);
	      buckets = SCM_HASHTABLE_VECTOR (table);
	      k = hash_fn (obj, SCM_VECTOR_LENGTH (buckets), closure);
	      if (k >= SCM_VECTOR_LENGTH (buckets))
		scm_out_of_range ("hash_fn_create_handle_x", scm_ulong2num (k));
	    }
	}
      return SCM_CAR (new_bucket);
    }
}
#undef FUNC_NAME


SCM 
scm_hash_fn_ref (SCM table, SCM obj, SCM dflt, unsigned long (*hash_fn)(),
                 SCM (*assoc_fn)(), void * closure)
{
  SCM it = scm_hash_fn_get_handle (table, obj, hash_fn, assoc_fn, closure);
  if (SCM_CONSP (it))
    return SCM_CDR (it);
  else
    return dflt;
}




SCM 
scm_hash_fn_set_x (SCM table, SCM obj, SCM val, unsigned long (*hash_fn)(),
                   SCM (*assoc_fn)(), void * closure)
{
  SCM it;

  it = scm_hash_fn_create_handle_x (table, obj, SCM_BOOL_F, hash_fn, assoc_fn, closure);
  SCM_SETCDR (it, val);
  return val;
}





SCM 
scm_hash_fn_remove_x (SCM table, SCM obj, unsigned long (*hash_fn)(), SCM (*assoc_fn)(),
                      SCM (*delete_fn)(), void * closure)
{
  unsigned long k;
  SCM buckets, h;

  if (SCM_HASHTABLE_P (table))
    buckets = SCM_HASHTABLE_VECTOR (table);
  else
    {
      SCM_ASSERT (SCM_VECTORP (table), table, SCM_ARG1, "hash_fn_remove_x");
      buckets = table;
    }
  if (SCM_VECTOR_LENGTH (table) == 0)
    return SCM_EOL;

  k = hash_fn (obj, SCM_VECTOR_LENGTH (buckets), closure);
  if (k >= SCM_VECTOR_LENGTH (buckets))
    scm_out_of_range ("hash_fn_remove_x", scm_ulong2num (k));
  h = assoc_fn (obj, SCM_VELTS (buckets)[k], closure);
  if (!SCM_FALSEP (h))
    {
      SCM_VECTOR_SET (buckets, k, delete_fn (h, SCM_VELTS (buckets)[k]));
      if (table != buckets)
	{
	  SCM_HASHTABLE_DECREMENT (table);
	  if (SCM_HASHTABLE_N_ITEMS (table) < SCM_HASHTABLE_LOWER (table))
	    rehash (table, hash_fn, closure);
	}
    }
  return h;
}




SCM_DEFINE (scm_hashq_get_handle, "hashq-get-handle", 2, 0, 0,
            (SCM table, SCM key),
	    "This procedure returns the @code{(key . value)} pair from the\n"
	    "hash table @var{table}.  If @var{table} does not hold an\n"
	    "associated value for @var{key}, @code{#f} is returned.\n"
	    "Uses @code{eq?} for equality testing.")
#define FUNC_NAME s_scm_hashq_get_handle
{
  return scm_hash_fn_get_handle (table, key, scm_ihashq, scm_sloppy_assq, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hashq_create_handle_x, "hashq-create-handle!", 3, 0, 0,
            (SCM table, SCM key, SCM init),
	    "This function looks up @var{key} in @var{table} and returns its handle.\n"
	    "If @var{key} is not already present, a new handle is created which\n"
	    "associates @var{key} with @var{init}.")
#define FUNC_NAME s_scm_hashq_create_handle_x
{
  return scm_hash_fn_create_handle_x (table, key, init, scm_ihashq, scm_sloppy_assq, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hashq_ref, "hashq-ref", 2, 1, 0,
            (SCM table, SCM key, SCM dflt),
	    "Look up @var{key} in the hash table @var{table}, and return the\n"
	    "value (if any) associated with it.  If @var{key} is not found,\n"
	    "return @var{default} (or @code{#f} if no @var{default} argument\n"
	    "is supplied).  Uses @code{eq?} for equality testing.")
#define FUNC_NAME s_scm_hashq_ref
{
  if (SCM_UNBNDP (dflt))
    dflt = SCM_BOOL_F;
  return scm_hash_fn_ref (table, key, dflt, scm_ihashq, scm_sloppy_assq, 0);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hashq_set_x, "hashq-set!", 3, 0, 0,
            (SCM table, SCM key, SCM val),
	    "Find the entry in @var{table} associated with @var{key}, and\n"
	    "store @var{value} there. Uses @code{eq?} for equality testing.")
#define FUNC_NAME s_scm_hashq_set_x
{
  return scm_hash_fn_set_x (table, key, val, scm_ihashq, scm_sloppy_assq, 0);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hashq_remove_x, "hashq-remove!", 2, 0, 0,
            (SCM table, SCM key),
	    "Remove @var{key} (and any value associated with it) from\n"
	    "@var{table}.  Uses @code{eq?} for equality tests.")
#define FUNC_NAME s_scm_hashq_remove_x
{
  return scm_hash_fn_remove_x (table, key, scm_ihashq, scm_sloppy_assq,
			       scm_delq_x, 0);
}
#undef FUNC_NAME




SCM_DEFINE (scm_hashv_get_handle, "hashv-get-handle", 2, 0, 0,
            (SCM table, SCM key),
	    "This procedure returns the @code{(key . value)} pair from the\n"
	    "hash table @var{table}.  If @var{table} does not hold an\n"
	    "associated value for @var{key}, @code{#f} is returned.\n"
	    "Uses @code{eqv?} for equality testing.")
#define FUNC_NAME s_scm_hashv_get_handle
{
  return scm_hash_fn_get_handle (table, key, scm_ihashv, scm_sloppy_assv, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hashv_create_handle_x, "hashv-create-handle!", 3, 0, 0,
            (SCM table, SCM key, SCM init),
	    "This function looks up @var{key} in @var{table} and returns its handle.\n"
	    "If @var{key} is not already present, a new handle is created which\n"
	    "associates @var{key} with @var{init}.")
#define FUNC_NAME s_scm_hashv_create_handle_x
{
  return scm_hash_fn_create_handle_x (table, key, init, scm_ihashv,
				      scm_sloppy_assv, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hashv_ref, "hashv-ref", 2, 1, 0,
            (SCM table, SCM key, SCM dflt),
	    "Look up @var{key} in the hash table @var{table}, and return the\n"
	    "value (if any) associated with it.  If @var{key} is not found,\n"
	    "return @var{default} (or @code{#f} if no @var{default} argument\n"
	    "is supplied).  Uses @code{eqv?} for equality testing.")
#define FUNC_NAME s_scm_hashv_ref
{
  if (SCM_UNBNDP (dflt))
    dflt = SCM_BOOL_F;
  return scm_hash_fn_ref (table, key, dflt, scm_ihashv, scm_sloppy_assv, 0);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hashv_set_x, "hashv-set!", 3, 0, 0,
            (SCM table, SCM key, SCM val),
	    "Find the entry in @var{table} associated with @var{key}, and\n"
	    "store @var{value} there. Uses @code{eqv?} for equality testing.")
#define FUNC_NAME s_scm_hashv_set_x
{
  return scm_hash_fn_set_x (table, key, val, scm_ihashv, scm_sloppy_assv, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hashv_remove_x, "hashv-remove!", 2, 0, 0,
            (SCM table, SCM key),
	    "Remove @var{key} (and any value associated with it) from\n"
	    "@var{table}.  Uses @code{eqv?} for equality tests.")
#define FUNC_NAME s_scm_hashv_remove_x
{
  return scm_hash_fn_remove_x (table, key, scm_ihashv, scm_sloppy_assv,
			       scm_delv_x, 0);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hash_get_handle, "hash-get-handle", 2, 0, 0,
            (SCM table, SCM key),
	    "This procedure returns the @code{(key . value)} pair from the\n"
	    "hash table @var{table}.  If @var{table} does not hold an\n"
	    "associated value for @var{key}, @code{#f} is returned.\n"
	    "Uses @code{equal?} for equality testing.")
#define FUNC_NAME s_scm_hash_get_handle
{
  return scm_hash_fn_get_handle (table, key, scm_ihash, scm_sloppy_assoc, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hash_create_handle_x, "hash-create-handle!", 3, 0, 0,
            (SCM table, SCM key, SCM init),
	    "This function looks up @var{key} in @var{table} and returns its handle.\n"
	    "If @var{key} is not already present, a new handle is created which\n"
	    "associates @var{key} with @var{init}.")
#define FUNC_NAME s_scm_hash_create_handle_x
{
  return scm_hash_fn_create_handle_x (table, key, init, scm_ihash, scm_sloppy_assoc, 0);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hash_ref, "hash-ref", 2, 1, 0,
            (SCM table, SCM key, SCM dflt),
	    "Look up @var{key} in the hash table @var{table}, and return the\n"
	    "value (if any) associated with it.  If @var{key} is not found,\n"
	    "return @var{default} (or @code{#f} if no @var{default} argument\n"
	    "is supplied).  Uses @code{equal?} for equality testing.")
#define FUNC_NAME s_scm_hash_ref
{
  if (SCM_UNBNDP (dflt))
    dflt = SCM_BOOL_F;
  return scm_hash_fn_ref (table, key, dflt, scm_ihash, scm_sloppy_assoc, 0);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hash_set_x, "hash-set!", 3, 0, 0,
            (SCM table, SCM key, SCM val),
	    "Find the entry in @var{table} associated with @var{key}, and\n"
	    "store @var{value} there. Uses @code{equal?} for equality\n"
	    "testing.")
#define FUNC_NAME s_scm_hash_set_x
{
  return scm_hash_fn_set_x (table, key, val, scm_ihash, scm_sloppy_assoc, 0);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hash_remove_x, "hash-remove!", 2, 0, 0,
            (SCM table, SCM key),
	    "Remove @var{key} (and any value associated with it) from\n"
	    "@var{table}.  Uses @code{equal?} for equality tests.")
#define FUNC_NAME s_scm_hash_remove_x
{
  return scm_hash_fn_remove_x (table, key, scm_ihash, scm_sloppy_assoc,
			       scm_delete_x, 0);
}
#undef FUNC_NAME




typedef struct scm_t_ihashx_closure
{
  SCM hash;
  SCM assoc;
  SCM delete;
} scm_t_ihashx_closure;



static unsigned long
scm_ihashx (SCM obj, unsigned long n, scm_t_ihashx_closure *closure)
{
  SCM answer = scm_call_2 (closure->hash,
			   obj,
			   scm_ulong2num ((unsigned long) n));
  return SCM_INUM (answer);
}



static SCM
scm_sloppy_assx (SCM obj, SCM alist, scm_t_ihashx_closure *closure)
{
  return scm_call_2 (closure->assoc, obj, alist);
}




static SCM
scm_delx_x (SCM obj, SCM alist, scm_t_ihashx_closure *closure)
{
  return scm_call_2 (closure->delete, obj, alist);
}



SCM_DEFINE (scm_hashx_get_handle, "hashx-get-handle", 4, 0, 0, 
            (SCM hash, SCM assoc, SCM table, SCM key),
	    "This behaves the same way as the corresponding\n"
	    "@code{-get-handle} function, but uses @var{hash} as a hash\n"
	    "function and @var{assoc} to compare keys.  @code{hash} must be\n"
	    "a function that takes two arguments, a key to be hashed and a\n"
	    "table size.  @code{assoc} must be an associator function, like\n"
	    "@code{assoc}, @code{assq} or @code{assv}.")
#define FUNC_NAME s_scm_hashx_get_handle
{
  scm_t_ihashx_closure closure;
  closure.hash = hash;
  closure.assoc = assoc;
  return scm_hash_fn_get_handle (table, key, scm_ihashx, scm_sloppy_assx,
				 (void *) &closure);
}
#undef FUNC_NAME


SCM_DEFINE (scm_hashx_create_handle_x, "hashx-create-handle!", 5, 0, 0, 
            (SCM hash, SCM assoc, SCM table, SCM key, SCM init),
	    "This behaves the same way as the corresponding\n"
	    "@code{-create-handle} function, but uses @var{hash} as a hash\n"
	    "function and @var{assoc} to compare keys.  @code{hash} must be\n"
	    "a function that takes two arguments, a key to be hashed and a\n"
	    "table size.  @code{assoc} must be an associator function, like\n"
	    "@code{assoc}, @code{assq} or @code{assv}.")
#define FUNC_NAME s_scm_hashx_create_handle_x
{
  scm_t_ihashx_closure closure;
  closure.hash = hash;
  closure.assoc = assoc;
  return scm_hash_fn_create_handle_x (table, key, init, scm_ihashx,
				      scm_sloppy_assx, (void *)&closure);
}
#undef FUNC_NAME



SCM_DEFINE (scm_hashx_ref, "hashx-ref", 4, 1, 0, 
            (SCM hash, SCM assoc, SCM table, SCM key, SCM dflt),
	    "This behaves the same way as the corresponding @code{ref}\n"
	    "function, but uses @var{hash} as a hash function and\n"
	    "@var{assoc} to compare keys.  @code{hash} must be a function\n"
	    "that takes two arguments, a key to be hashed and a table size.\n"
	    "@code{assoc} must be an associator function, like @code{assoc},\n"
	    "@code{assq} or @code{assv}.\n"
	    "\n"
	    "By way of illustration, @code{hashq-ref table key} is\n"
	    "equivalent to @code{hashx-ref hashq assq table key}.")
#define FUNC_NAME s_scm_hashx_ref
{
  scm_t_ihashx_closure closure;
  if (SCM_UNBNDP (dflt))
    dflt = SCM_BOOL_F;
  closure.hash = hash;
  closure.assoc = assoc;
  return scm_hash_fn_ref (table, key, dflt, scm_ihashx, scm_sloppy_assx,
			  (void *)&closure);
}
#undef FUNC_NAME




SCM_DEFINE (scm_hashx_set_x, "hashx-set!", 5, 0, 0,
            (SCM hash, SCM assoc, SCM table, SCM key, SCM val),
	    "This behaves the same way as the corresponding @code{set!}\n"
	    "function, but uses @var{hash} as a hash function and\n"
	    "@var{assoc} to compare keys.  @code{hash} must be a function\n"
	    "that takes two arguments, a key to be hashed and a table size.\n"
	    "@code{assoc} must be an associator function, like @code{assoc},\n"
	    "@code{assq} or @code{assv}.\n"
	    "\n"
	    " By way of illustration, @code{hashq-set! table key} is\n"
	    "equivalent to @code{hashx-set!  hashq assq table key}.")
#define FUNC_NAME s_scm_hashx_set_x
{
  scm_t_ihashx_closure closure;
  closure.hash = hash;
  closure.assoc = assoc;
  return scm_hash_fn_set_x (table, key, val, scm_ihashx, scm_sloppy_assx,
			    (void *)&closure);
}
#undef FUNC_NAME



SCM
scm_hashx_remove_x (SCM hash, SCM assoc, SCM delete, SCM table, SCM obj)
{
  scm_t_ihashx_closure closure;
  closure.hash = hash;
  closure.assoc = assoc;
  closure.delete = delete;
  return scm_hash_fn_remove_x (table, obj, scm_ihashx, scm_sloppy_assx, scm_delx_x, 0);
}

static SCM
fold_proc (void *proc, SCM key, SCM data, SCM value)
{
  return scm_call_3 (SCM_PACK (proc), key, data, value);
}

SCM_DEFINE (scm_hash_fold, "hash-fold", 3, 0, 0, 
            (SCM proc, SCM init, SCM table),
	    "An iterator over hash-table elements.\n"
            "Accumulates and returns a result by applying PROC successively.\n"
            "The arguments to PROC are \"(key value prior-result)\" where key\n"
            "and value are successive pairs from the hash table TABLE, and\n"
            "prior-result is either INIT (for the first application of PROC)\n"
            "or the return value of the previous application of PROC.\n"
            "For example, @code{(hash-fold acons '() tab)} will convert a hash\n"
            "table into an a-list of key-value pairs.")
#define FUNC_NAME s_scm_hash_fold
{
  SCM_VALIDATE_PROC (1, proc);
  if (!SCM_HASHTABLE_P (table))
    SCM_VALIDATE_VECTOR (3, table);
  return scm_internal_hash_fold (fold_proc, (void *) SCM_UNPACK (proc), init, table);
}
#undef FUNC_NAME

SCM
scm_internal_hash_fold (SCM (*fn) (), void *closure, SCM init, SCM table)
{
  long i, n;
  SCM buckets, result = init;
  
  if (SCM_HASHTABLE_P (table))
    buckets = SCM_HASHTABLE_VECTOR (table);
  else
    buckets = table;
  
  n = SCM_VECTOR_LENGTH (buckets);
  for (i = 0; i < n; ++i)
    {
      SCM ls = SCM_VELTS (buckets)[i], handle;
      while (!SCM_NULLP (ls))
	{
	  if (!SCM_CONSP (ls))
	    scm_wrong_type_arg (s_scm_hash_fold, SCM_ARG3, buckets);
	  handle = SCM_CAR (ls);
	  if (!SCM_CONSP (handle))
	    scm_wrong_type_arg (s_scm_hash_fold, SCM_ARG3, buckets);
	  result = fn (closure, SCM_CAR (handle), SCM_CDR (handle), result);
	  ls = SCM_CDR (ls);
	}
    }

  return result;
}




void
scm_init_hashtab ()
{
  scm_tc16_hashtable = scm_make_smob_type (s_hashtable, 0);
  scm_set_smob_mark (scm_tc16_hashtable, scm_markcdr);
  scm_set_smob_print (scm_tc16_hashtable, hashtable_print);
  scm_set_smob_free (scm_tc16_hashtable, hashtable_free);
#include "libguile/hashtab.x"
}

/*
  Local Variables:
  c-file-style: "gnu"
  End:
*/
