/* This file defines the procedures related to one type of homogenous
   numeric vector.  It is included multiple time in srfi-4.c, once for
   each type.

   Before inclusion, the following macros must be defined.  They are
   undefined at the end of this file to get back to a clean slate for
   the next inclusion.

   - TYPE

   The type tag of the vector, for example SCM_UVEC_U8

   - TAG

   The tag name of the vector, for example u8.  The tag is used to
   form the function names and is included in the docstrings, for
   example.
*/

/* The first level does not expand macros in the arguments. */
#define paste(a1,a2,a3)   a1##a2##a3
#define s_paste(a1,a2,a3) s_##a1##a2##a3
#define stringify(a)      #a

/* But the second level does. */
#define F(pre,T,suf)   paste(pre,T,suf)
#define s_F(pre,T,suf) s_paste(pre,T,suf)
#define S(T)           stringify(T)

SCM_DEFINE (F(scm_,TAG,vector_p), S(TAG)"vector?", 1, 0, 0,
            (SCM obj),
	    "Return @code{#t} if @var{obj} is a vector of type " S(TAG) ",\n"
	    "@code{#f} otherwise.")
#define FUNC_NAME s_F(scm_, TAG, vector_p)
{
  return uvec_p (TYPE, obj);
}
#undef FUNC_NAME

SCM_DEFINE (F(scm_make_,TAG,vector), "make-"S(TAG)"vector", 1, 1, 0,
            (SCM len, SCM fill),
	    "Return a newly allocated homogeneous numeric vector which can\n"
	    "hold @var{len} elements.  If @var{fill} is given, it is used to\n"
	    "initialize the elements, otherwise the contents of the vector\n"
	    "is unspecified.")
#define FUNC_NAME s_S(scm_make_,TAG,vector)
{
  return make_uvec (TYPE, len, fill);
}
#undef FUNC_NAME

SCM_DEFINE (F(scm_,TAG,vector), S(TAG)"vector", 0, 0, 1,
            (SCM l),
	    "Return a newly allocated homogeneous numeric vector containing\n"
	    "all argument values.")
#define FUNC_NAME s_F(scm_,TAG,vector)
{
  return list_to_uvec (TYPE, l);
}
#undef FUNC_NAME


SCM_DEFINE (F(scm_,TAG,vector_length), S(TAG)"vector-length", 1, 0, 0,
            (SCM uvec),
	    "Return the number of elements in the homogeneous numeric vector\n"
	    "@var{uvec}.")
#define FUNC_NAME s_F(scm_,TAG,vector_length)
{
  return uvec_length (TYPE, uvec);
}
#undef FUNC_NAME


SCM_DEFINE (F(scm_,TAG,vector_ref), S(TAG)"vector-ref", 2, 0, 0,
            (SCM uvec, SCM index),
	    "Return the element at @var{index} in the homogeneous numeric\n"
	    "vector @var{uvec}.")
#define FUNC_NAME s_F(scm_,TAG,vector_ref)
{
  return uvec_ref (TYPE, uvec, index);
}
#undef FUNC_NAME


SCM_DEFINE (F(scm_,TAG,vector_set_x), S(TAG)"vector-set!", 3, 0, 0,
            (SCM uvec, SCM index, SCM value),
	    "Set the element at @var{index} in the homogeneous numeric\n"
	    "vector @var{uvec} to @var{value}.  The return value is not\n"
	    "specified.")
#define FUNC_NAME s_F(scm_,TAG,vector_set_x)
{
  return uvec_set_x (TYPE, uvec, index, value);
}
#undef FUNC_NAME


SCM_DEFINE (F(scm_,TAG,vector_to_list), S(TAG)"vector->list", 1, 0, 0,
            (SCM uvec),
	    "Convert the homogeneous numeric vector @var{uvec} to a list.")
#define FUNC_NAME s_F(scm_,TAG,vector_to_list)
{
  return uvec_to_list (TYPE, uvec);
}
#undef FUNC_NAME


SCM_DEFINE (F(scm_list_to_,TAG,vector), "list->"S(TAG)"vector", 1, 0, 0,
            (SCM l),
	    "Convert the list @var{l} to a numeric homogeneous vector.")
#define FUNC_NAME s_F(scm_list_to_,TAG,vector)
{
  return list_to_uvec (TYPE, l);
}
#undef FUNC_NAME

#undef paste
#undef s_paste
#undef stringify
#undef F
#undef s_F
#undef S

#undef TYPE
#undef TAG