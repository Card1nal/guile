/* classes: h_files */

#ifndef SCM_MACROS_H
#define SCM_MACROS_H

/* Copyright (C) 1998,2000,2001,2002,2003, 2006, 2008, 2009, 2010 Free Software Foundation, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */



#include "libguile/__scm.h"



#define SCM_ASSYNT(_cond, _msg, _subr) \
  if (!(_cond)) scm_misc_error (_subr, _msg, SCM_EOL);

#define SCM_MACRO_TYPE_BITS  (3)
#define SCM_MACRO_TYPE_MASK  ((1<<SCM_MACRO_TYPE_BITS)-1)
#define SCM_F_MACRO_EXTENDED (1<<SCM_MACRO_TYPE_BITS)

#define SCM_MACROP(x) SCM_SMOB_PREDICATE (scm_tc16_macro, (x))
#define SCM_MACRO_TYPE(m) (SCM_SMOB_FLAGS (m) & SCM_MACRO_TYPE_MASK)
#define SCM_MACRO_IS_EXTENDED(m) (SCM_SMOB_FLAGS (m) & SCM_F_MACRO_EXTENDED)
#define SCM_BUILTIN_MACRO_P(x) (SCM_MACROP (x) && SCM_MACRO_TYPE (x) == 3)
#define SCM_SYNCASE_MACRO_P(x) (SCM_MACROP (x) && SCM_MACRO_TYPE (x) == 4)
#define SCM_MACRO_CODE(m) SCM_SMOB_OBJECT (m)

SCM_API scm_t_bits scm_tc16_macro;

SCM_INTERNAL SCM scm_i_makbimacro (SCM code);
SCM_API SCM scm_makmmacro (SCM code);
SCM_API SCM scm_make_syncase_macro (SCM type, SCM binding);
SCM_API SCM scm_make_extended_syncase_macro (SCM builtin, SCM type,
                                             SCM binding);
SCM_API SCM scm_macro_p (SCM obj);
SCM_API SCM scm_macro_type (SCM m);
SCM_API SCM scm_macro_name (SCM m);
SCM_API SCM scm_macro_transformer (SCM m);
SCM_API SCM scm_syncase_macro_type (SCM m);
SCM_API SCM scm_syncase_macro_binding (SCM m);
SCM_API SCM scm_make_synt (const char *name,
			   SCM (*macroizer) (SCM),
			   SCM (*fcn) ());
SCM_INTERNAL void scm_init_macros (void);


#endif  /* SCM_MACROS_H */

/*
  Local Variables:
  c-file-style: "gnu"
  End:
*/
