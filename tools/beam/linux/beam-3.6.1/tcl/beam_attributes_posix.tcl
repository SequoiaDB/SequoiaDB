# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2006-2010 IBM Corporation. All rights reserved.
#
# US Government Users Restricted Rights - Use, duplication or disclosure
# restricted by GSA ADP Schedule Contract with IBM Corp
#
# The source code for this program is not published or otherwise divested
# of its trade secrets, irrespective of what has been deposited within
# the U.S. Copyright Office.
#
#
#    AUTHOR:
#
#        Nate Daly, Florian Krohm
#
#    DESCRIPTION:
#
#        Attributes for POSIX functions that are not already covered by
#        ISO C. Note also that functions marked LEGACY in the standard
#        have been ignored.
#
#    MODIFICATIONS:
#
#        See svn log for more recent modifications.
#

namespace eval beam::attribute {

#------------------------------------------------------------------------------
# Posix functions grouped by header file.
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# aio.h - asynchronous input and output (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# arpa/inet.h - definitions for internet operations (DONE)
#------------------------------------------------------------------------------

  set attr(inet_addr) "pure,
                       buffer ( buffer_index = 1,
                                type = read,
                                string_index = 1,
                                padding = 1 ),
                       no_other_side_effects"

  set attr(inet_ntoa) "const,
                       $returns_static_like,
                       no_other_side_effects"

  set attr(inet_ntop) "buffer ( buffer_index = 2,
                                type = read,
                                padding = 1 ),
                       buffer ( buffer_index = 3,
                                type = write,
                                size_index = 4 ),
                       property ( index = 3,
                                  property_type = requires,
                                  type = input,
                                  test_type = not_equal,
                                  test_value = 0 ),
                       no_other_side_effects"

  set attr(inet_pton) "buffer ( buffer_index = 2,
                                type = read,
                                padding = 1 ),
                       buffer ( buffer_index = 3,
                                type = write,
                                size_index = 4 ),
                       property ( index = 3,
                                  property_type = requires,
                                  type = input,
                                  test_type = not_equal,
                                  test_value = 0 ),
                       no_other_side_effects"

  beam::function_attribute "$attr(const_function)" -signatures \
      "htonl" "htons" "ntohl" "ntohs"

  beam::function_attribute "$attr(inet_addr)" -signatures "inet_addr"
  beam::function_attribute "$attr(inet_ntoa)" -signatures "inet_ntoa"
  beam::function_attribute "$attr(inet_ntop)" -signatures "inet_ntop"
  beam::function_attribute "$attr(inet_pton)" -signatures "inet_pton"

#------------------------------------------------------------------------------
# assert.h - verify program assertion (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# complex.h - complex arithmetic (ALL LIBC)
#------------------------------------------------------------------------------

  # The following are defined with the libc attributes:
  #   cabs, cabsf, cabsl, cacos, cacosf, cacosh, cacoshl, cacosl,
  #   carg, cargf cargl, casin, casinf, casinh, casinhf, casinhl,
  #   casinl, catan, catanf, catanh, catanhf, catanhl, catanl,
  #   ccos, ccosf, ccosh, ccoshf, ccoshl, ccosl, cexp, cexpf,
  #   cexpl, cimag, cimagf, cimagl, clog, clogf, clogl, conj,
  #   conjf, conjl, cpow, cpowf, cpowl, cproj, cprojf, cprojl,
  #   creal, crealf, creall, csin, csinf, csinh, csinhf, csinhl,
  #   csinl, csqrt, csqrtf, csqrtl, ctan, ctanf, ctanh, ctanhf,
  #   ctanhl, ctanl

#------------------------------------------------------------------------------
# cpio.h - cpio archive values (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# ctype.h - character types (DONE)
#------------------------------------------------------------------------------

  beam::function_attribute "$attr(const_function)" -signatures \
      "isascii" "toascii"

  # The following are defined with the libc attributes:
  #   isalnum, isalpha, isblank, iscntrl, isdigit, isgraph, islower
  #   isprint, ispunct, isspace, isupper, isxdigit, tolower, toupper

#------------------------------------------------------------------------------
# dirent.h - format of directory entries (DONE)
#------------------------------------------------------------------------------

  set attr(closedir)  "$side_effect_on_environment,
                       buffer ( buffer_index = 1,
                                type = read,
                                units = elements,
                                size = 1 ),
                       property ( index = 1,
                                  property_type = requires,
                                  type = input,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" ),
                       property ( index = 1,
                                  property_type = provides,
                                  type = output,
                                  property_name = \"directory state\",
                                  property_value = \"closed\"),
                       no_other_side_effects"

  set attr(opendir)   "$side_effect_on_environment,
                       buffer ( buffer_index = 1,
                                type = read,
                                string_index = 1,
                                padding = 1 ),
                       property ( index = return,
                                  property_type = provides,
                                  type = output,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" )
                             if ( index = return,
                                  type = output,
                                  test_type = not_equal,
                                  test_value = 0 ),
                       anchor ( index = return, fate = wont),
                       no_other_side_effects"

  set attr(readdir)   "$side_effect_on_environment,
                       buffer ( buffer_index = 1,
                                type = read,
                                units = elements,
                                size = 1 ),
                       property ( index = 1,
                                  property_type = requires,
                                  type = input,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" ),
                       no_other_side_effects"

  set attr(readdir_r) "$side_effect_on_environment,
                       thread_safe,
                       buffer ( buffer_index = 1,
                                type = read,
                                units = elements,
                                size = 1 ),
                       buffer ( buffer_index = 2,
                                type = write,
                                units = elements,
                                size = 1 ),
                       buffer ( buffer_index = 3,
                                type = write,
                                units = elements,
                                size = 1 ),
                       anchor ( index = 2 )
                           if ( index = return,
                                type = output,
                                test_type = equal,
                                test_value = 0 ),
                       property ( index = 1,
                                  property_type = requires,
                                  type = input,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" ),
                       no_other_side_effects"

  set attr(rewinddir) "$side_effect_on_environment,
                       buffer ( buffer_index = 1,
                                type = read,
                                units = elements,
                                size = 1 ),
                       property ( index = 1,
                                  property_type = requires,
                                  type = input,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" ),
                       no_other_side_effects"

  set attr(seekdir)   "$side_effect_on_environment,
                       buffer ( buffer_index = 1,
                                type = read,
                                units = elements,
                                size = 1 ),
                       property ( index = 1,
                                  property_type = requires,
                                  type = input,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" ),
                       no_other_side_effects"

  set attr(telldir)   "pure,
                       buffer ( buffer_index = 1,
                                type = read,
                                units = elements,
                                size = 1 ),
                       property ( index = 1,
                                  property_type = requires,
                                  type = input,
                                  property_name = \"directory state\",
                                  property_value = \"opened\" ),
                       no_other_side_effects"

  beam::function_attribute "$attr(closedir)"  -signatures "closedir"
  beam::function_attribute "$attr(opendir)"   -signatures "opendir"
  beam::function_attribute "$attr(readdir)"   -signatures "readdir"
  beam::function_attribute "$attr(readdir_r)" -signatures "readdir_r"
  beam::function_attribute "$attr(rewinddir)" -signatures "rewinddir"
  beam::function_attribute "$attr(seekdir)"   -signatures "seekdir"
  beam::function_attribute "$attr(telldir)"   -signatures "telldir"

#------------------------------------------------------------------------------
# dlfcn.h - dynamic linking (DONE)
#------------------------------------------------------------------------------

  set attr(dlclose) "$fclose_like,
                     property ( index = 1,
                                property_type = requires,
                                type = input,
                                property_name = \"file source\",
                                property_value = \"from dlopen\" ),
                     no_other_side_effects"

  set attr(dlerror) "$side_effect_on_environment,
                     $returns_static_like if ( index = return,
                                               type = output,
                                               test_type = not_equal,
                                               test_value = 0 ),
                     no_other_side_effects"

  set attr(dlopen)  "$side_effect_on_environment,
                     $fopen_like,
                     buffer ( buffer_index = 1,
                              type = read,
                              string_index = 1,
                              padding = 1,
                              allow_null ),
                     property ( index = return,
                                property_type = provides,
                                type = output,
                                property_name = \"file source\",
                                property_value = \"from dlopen\" )
                           if ( index = return,
                                type = output,
                                test_type = not_equal,
                                test_value = 0 ),
                     no_other_side_effects"


  set attr(dlsym)   "$returns_static_like if ( index = return,
                                               type = output,
                                               test_type = not_equal,
                                               test_value = 0 ),
                     buffer ( buffer_index = 2,
                              type = read,
                              string_index = 2,
                              padding = 1 ),
                     property ( index = 1,
                                property_type = requires,
                                type = input,
                                property_name = \"file source\",
                                property_value = \"from dlopen\" ),
                     no_other_side_effects"

  beam::function_attribute "$attr(dlclose)" -signatures "dlclose"
  beam::function_attribute "$attr(dlerror)" -signatures "dlerror"
  beam::function_attribute "$attr(dlopen)"  -signatures "dlopen"
  beam::function_attribute "$attr(dlsym)"   -signatures "dlsym"

#------------------------------------------------------------------------------
# errno.h - system error numbers (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions

#------------------------------------------------------------------------------
# fcntl.h - file control options (DONE)
#------------------------------------------------------------------------------

  set attr(creat) "$side_effect_on_environment,
                   buffer ( buffer_index = 1,
                            type = read,
                            string_index = 1,
                            padding = 1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              test_type = greater_than_or_equal,
                              test_value = -1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              property_name = \"file state\",
                              property_value = \"opened\" )
                         if ( index = return,
                              type = output,
                              test_type = greater_than_or_equal,
                              test_value = 0 ),
                   anchor ( index = return, fate = wont),
                   no_other_side_effects"

  set attr(fcntl) "$side_effect_on_environment,

                  # Command 0 is F_DUPFD, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 0
                              ),

                  # Command 1 is F_GETFD, which doesn't need the third arg
                  vararg ( count_always = 2,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 1
                              ),

                  # Command 2 is F_SETFD, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 2
                              ),

                  # Command 3 is F_GETFL, which doesn't need the third arg
                  vararg ( count_always = 2,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 3
                              ),

                  # Command 4 is F_SETFL, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 4
                              ),

                  # Command 5 is F_GETLK, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 5
                              ),

                  # Command 6 is F_SETLK, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 6
                              ),

                  # Command 7 is F_SETLKW, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 7
                              ),

                  # Command 8 is F_SETOWN, which needs the third arg
                  vararg ( count_always = 3,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 8
                              ),

                  # Command 9 is F_GETOWN, which doesn't need the third arg
                  vararg ( count_always = 2,
                           first_to_check = 1,
                         ) if ( index = 2,
                                type = input,
                                test_type = equal,
                                test_value = 9
                              ),

                  no_other_side_effects"

  # Skipped posix_fadvise and posix_fallocate.

  beam::function_attribute "$attr(creat)"           -signatures "creat" "open"
  beam::function_attribute "$attr(fcntl)"           -signatures "fcntl"

#------------------------------------------------------------------------------
# fenv.h - floating-point environment (ALL LIBC)
#------------------------------------------------------------------------------

  # The following are defined with the libc attributes:
  #   feclearexcept, fegetexceptflag, feraiseexcept, fesetexceptflag,
  #   fetestexcept, fegetround, fesetround, fegetenv, feholdexcept,
  #   fesetenv, feupdateenv

#------------------------------------------------------------------------------
# float.h - floating types (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# fmtmsg.h - message display structures (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# fnmatch.h - filename-matching types (DONE)
#------------------------------------------------------------------------------

  set attr(fnmatch) "pure,
                     buffer ( buffer_index = 1,
                              type = read,
                              string_index = 1,
                              padding = 1 ),
                     buffer ( buffer_index = 2,
                              type = read,
                              string_index = 2,
                              padding = 1 ),
                     no_other_side_effects"

  beam::function_attribute "$attr(fnmatch)" -signatures "fnmatch"

#------------------------------------------------------------------------------
# ftw.h - file tree traversal (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# glob.h - pathname pattern-matching types
#------------------------------------------------------------------------------

  beam::resource_create {
    name = "glob",
    display = "glob",
    allocating_verb = "allocating",
    allocated_verb = "allocated",
    freeing_verb = "deallocating",
    freed_verb = "deallocated",
    use_after_free = "error"
  }

  beam::propinfo_create {
      name = "glob state",  # allocated or deallocated
      invariance = "none",
      dependence = "calls",
      resource   = "glob"
  }

  set attr(glob) "$side_effect_on_environment,
                  buffer ( buffer_index = 1,
                           type = read,
                           string_index = 1,
                           padding = 1 ),
                  buffer ( buffer_index = 3,
                           type = read,
                           units = elements,
                           size = 1,
                           allow_null ),
                  buffer ( buffer_index = 4,
                           type = write,
                           units = elements,
                           size = 1 ),
                  property ( index = 4,                       # The struct
                             num_dereference = 1,             # whose addr
                             property_type = provides,        # is 4-th arg
                             type = output,                   # gets filled
                             property_name = \"glob state\",  # with new
                             property_value = \"allocated\" ) # memory,
                  if ( index = 2,                             # but not if
                       type = input,                          # 2-nd arg is
                       test_type = not_equal,                 # GLOB_APPEND.
                       test_value = 32 ),
                  anchor ( index = 4, fate = wont),
                  no_other_side_effects"

  set attr(globfree) "$side_effect_on_environment,
                      buffer ( buffer_index = 1,
                               type = read,
                               units = elements,
                               size = 1 ),
                      property ( index = 1,
                                 num_dereference = 1,
                                 property_type = provides,
                                 type = output,
                                 property_name = \"glob state\",
                                 property_value = \"deallocated\" ),
                      no_other_side_effects"

  beam::function_attribute "$attr(glob)"     -signatures "glob"
  beam::function_attribute "$attr(globfree)" -signatures "globfree"

#------------------------------------------------------------------------------
# grp.h - group structure (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # struct group  *getgrgid(gid_t);
  # struct group  *getgrnam(const char *);
  # int            getgrgid_r(gid_t, struct group *, char *, size_t, struct group **);
  # int            getgrnam_r(const char *, struct group *, char *, size_t , struct group **);
  # struct group  *getgrent(void);
  # void           endgrent(void);
  # void           setgrent(void);

#------------------------------------------------------------------------------
# iconv.h - codeset conversion facility (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# inttypes.h - fixed size integer types (ALL LIBC)
#------------------------------------------------------------------------------

  # The following are defined with the libc attributes:
  #   imaxabs, imaxdiv, strtoimax, strtoumax, wcstoimax, wcstoumax

#------------------------------------------------------------------------------
# iso646.h - alternative spellings (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# langinfo.h - language information constants (DONE)
#------------------------------------------------------------------------------

  set attr(nl_langinfo) "$side_effect_on_environment,
                         $returns_static_like,
                         no_other_side_effects"

  beam::function_attribute "$attr(nl_langinfo)" -signatures "nl_langinfo"

#------------------------------------------------------------------------------
# libgen.h - definitions for pattern matching functions (DONE)
#------------------------------------------------------------------------------

  set attr(basename) "$returns_static_like,
                      buffer ( buffer_index = 1,
                               type = read,
                               string_index = 1 )
                          if ( index = 1,
                               type = input,
                               test_type = not_equal,
                               test_value = 0 ),
                      buffer ( buffer_index = 1,
                               type = write,
                               string_index = 1 )
                          if ( index = 1,
                               type = input,
                               test_type = not_equal,
                               test_value = 0 ),
                      no_other_side_effects"

  beam::function_attribute "$attr(basename)" -signatures "basename" "dirname"

#------------------------------------------------------------------------------
# limits.h - implementation-defined constants (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# locale.h - category macros (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# math.h - mathematical declarations (DONE)
#------------------------------------------------------------------------------

  # These set errno, so they are not const or pure.
  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "j0" "j1" "jn" "y0" "y1" "yn"

  # The following are defined with the libm attributes:
  #   acos, acosf, acosh, acoshf, acoshl, acosl, asin, asinf, asinh
  #   asinhf, asinhl, asinl, atan, atan2, atan2f, atan2l, atanf,
  #   atanh, atanhf, atanhl, atanl, cbrt,cbrtf, cbrtl, ceil, ceilf,
  #   ceill, copysign, copysignf, copysignl, cos, cosf, cosh, coshf,
  #   coshl, cosl, erf, erfc, erfcf, erfcl, erff, erfl, exp, exp2,
  #   exp2f, exp2l, expf, expl, expm1, expm1f, expm1l, fabs, fabsf,
  #   fabsl, fdim, fdimf, fdiml, floor, floorf, floorl, fma, fmaf,
  #   fmal, fmax, fmaxf, fmaxl, fmin, fminf, fminl, fmod, fmodf,
  #   fmodl, frexp, frexpf, frexpl, hypot, hypotf, hypotl, ilogb,
  #   ilogbf, ilogbl, ldexp, ldexpf, ldexpl, lgamma, lgammaf, lgammal,
  #   llrint, llrintf, llrintl, llround, llroundf, llroundl, log,
  #   log10, log10f, log10l, log1p, log1pf, log1pl, log2, log2f,
  #   log2l, logb, logbf, logbl, logf, logl, lrint, lrintf, lrintl,
  #   lround, lroundf, lroundl, modf, modff, modfl, nan, nanf, nanl,
  #   nearbyint, nearbyintf, nearbyintl, nextafter, nextafterf,
  #   nextafterl, nexttoward, nexttowardf, nexttowardl, pow, powf,
  #   powl, remainder, remainderf, remainderl, remquo, remquof,
  #   remquol, rint, rintf, rintl, round, roundf, roundl, scalb,
  #   scalbln, scalblnf, scalblnl, scalbn, scalbnf, scalbnl, sin,
  #   sinf, sinh, sinhf, sinhl, sinl, sqrt, sqrtf, sqrtl, tan, tanf,
  #   tanh, tanhf, tanhl, tanl, tgamma, tgammaf, tgammal, trunc,
  #   truncf, truncl, fpclassify, isfinite, isinf, isnormal,
  #   signbit, isgreater, isnan, isgreaterequal, islessequal,
  #   islessgreater, isless, isunordered

#------------------------------------------------------------------------------
# monetary.h - monetary types (PENDING - IF EASY TO DO)
#------------------------------------------------------------------------------

  # ssize_t  strfmon(char *restrict, size_t, const char *restrict, ...);

#------------------------------------------------------------------------------
# mqueue.h - message queues (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# ndbm.h - definitions for ndbm database operations (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# net/if.h - sockets local interfaces (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# netdb.h - definitions for network database operations (DONE)
#------------------------------------------------------------------------------

  # FIXME: some of these require one of their arguments to be given in
  # FIXME: network byte order. consider adding properties for this.

  beam::resource_create {
    name = "addrinfo",
    display = "addrinfo",
    allocating_verb = "allocating",
    allocated_verb = "allocated",
    freeing_verb = "deallocating",
    freed_verb = "deallocated",
    use_after_free = "error"
  }


  set attr(endhostent) "$side_effect_on_environment,
                        no_other_side_effects"

  set attr(gethostent) "$side_effect_on_environment,
                        $returns_static_like,
                        no_other_side_effects"

  set attr(getaddrinfo) "$side_effect_on_environment,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  allow_null ),
                         buffer ( buffer_index = 2,
                                  type = read,
                                  string_index = 2,
                                  allow_null ),
                         property ( index = 2,
                                    property_type = requires,
                                    type = input,
                                    test_type = not_equal,
                                    test_value = 0 )
                               if ( index = 1,
                                    type = input,
                                    test_type = equal,
                                    test_value = 0 ),
                         buffer ( buffer_index = 3,
                                  type = read,
                                  units = elements,
                                  size = 1,
                                  allow_null ),
                         buffer ( buffer_index = 4,
                                  type = write,
                                  units = elements,
                                  size = 1 ),
                         allocator (size_index = unset, # size known internally
		                    return_index = 4,
        		            if_out_of_memory = return_null,
	              	            initial_state = initialized_to_unknown,
                                    resource = addrinfo)
                               if ( index = return,
                                    type = output,
                                    test_type = equal,
                                    test_value = 0 ),
                         property ( index = 4,
                                    num_dereference = 1,
                                    property_type = provides,
                                    type = output,
                                    property_name = \"memory allocation source\",
                                    property_value = \"from getaddrinfo\" ),
                         no_other_side_effects"

  set attr(freeaddrinfo) "deallocator ( pointer_index = 1,
                                        resource = addrinfo),
                          property ( index = 1,
                                     property_type = requires,
                                     type = input,
                                     property_name = \"memory allocation source\",
                                     property_value = \"from getaddrinfo\" ),
                          no_other_side_effects"

  set attr(gai_strerror) "$returns_static_like,
                          no_other_side_effects"

  set attr(gethostbyaddr) "$side_effect_on_environment,
                           buffer ( buffer_index = 1,
                                    type = read,
                                    size_index = 2 ),
                           $returns_static_like,
                           no_other_side_effects"

  set attr(gethostbyname) "$side_effect_on_environment,
                           buffer ( buffer_index = 1,
                                    type = read,
                                    string_index = 1,
                                    padding = 1 ),
                           $returns_static_like,
                           no_other_side_effects"

  set attr(getnameinfo)   "$side_effect_on_environment,
                           buffer ( buffer_index = 1,
                                    type = read,
                                    units = elements,
                                    size = 1 ),
                           buffer ( buffer_index = 3,
                                    type = write,
                                    bound_size_index = 4,
                                    allow_null ),
                           buffer ( buffer_index = 5,
                                    type = write,
                                    bound_size_index = 6,
                                    allow_null ),
                           no_other_side_effects"

  set attr(getnetbyaddr)  "$returns_static_like,
                           no_other_side_effects"

  set attr(getnetbyname)  "buffer ( buffer_index = 1,
                                    type = read,
                                    string_index = 1,
                                    padding = 1 ),
                           $returns_static_like,
                           no_other_side_effects"

  set attr(getservbyname) "buffer ( buffer_index = 1,
                                    type = read,
                                    string_index = 1,
                                    padding = 1 ),
                           buffer ( buffer_index = 2,
                                    type = read,
                                    string_index = 2,
                                    padding = 1,
                                    allow_null ),
                           $returns_static_like,
                           no_other_side_effects"

  set attr(getservbyport) "buffer ( buffer_index = 2,
                                    type = read,
                                    string_index = 2,
                                    padding = 1,
                                    allow_null ),
                           $returns_static_like,
                           no_other_side_effects"

  beam::function_attribute "$attr(endhostent)" -signatures \
      "endhostent" "endnetent" "endprotoent" "endservent"  \
      "sethostent" "setnetent" "setprotoent" "setservent"

  beam::function_attribute "$attr(gethostent)" -signatures \
      "gethostent" "getnetent" "getprotoent" "getservent"

  beam::function_attribute "$attr(getaddrinfo)"   -signatures "getaddrinfo"
  beam::function_attribute "$attr(freeaddrinfo)"  -signatures "freeaddrinfo"
  beam::function_attribute "$attr(gai_strerror)"  -signatures "gai_strerror"
  beam::function_attribute "$attr(gethostbyaddr)" -signatures "gethostbyaddr"
  beam::function_attribute "$attr(gethostbyname)" -signatures "gethostbyname"
  beam::function_attribute "$attr(getnameinfo)"   -signatures "getnameinfo"

  beam::function_attribute "$attr(getnetbyaddr)"  -signatures \
      "getnetbyaddr" "getprotobynumber"

  beam::function_attribute "$attr(getnetbyname)"  -signatures \
      "getnetbyname" "getprotobyname"

  beam::function_attribute "$attr(getservbyname)" -signatures "getservbyname"
  beam::function_attribute "$attr(getservbyport)" -signatures "getservbyport"

#------------------------------------------------------------------------------
# netinet/in.h - Internet address family (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# netinet/tcp.h - definitions for the Internet Transmission Control Protocol
#                 (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# nl_types.h - data types (PENDING)
#------------------------------------------------------------------------------

  # int       catclose(nl_catd);
  # char     *catgets(nl_catd, int, int, const char *);
  # nl_catd   catopen(const char *, int);

#------------------------------------------------------------------------------
# poll.h - definitions for the poll() function (PENDING)
#------------------------------------------------------------------------------

  # int   poll(struct pollfd[], nfds_t, int);

#------------------------------------------------------------------------------
# pthread.h - threads (POSTPONED FOR FURTHER DISCUSSION)
#------------------------------------------------------------------------------

  # Postponed until we discuss threads with Dan.

#------------------------------------------------------------------------------
# pwd.h - password structure (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # struct passwd *getpwnam(const char *);
  # struct passwd *getpwuid(uid_t);
  # int            getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
  # int            getpwuid_r(uid_t, struct passwd *, char *, size_t, struct passwd **);
  # void           endpwent(void);
  # struct passwd *getpwent(void);
  # void           setpwent(void);

#------------------------------------------------------------------------------
# regex.h - regular expression matching types (PENDING)
#------------------------------------------------------------------------------

  # int    regcomp(regex_t *restrict, const char *restrict, int);
  # size_t regerror(int, const regex_t *restrict, char *restrict, size_t);
  # int    regexec(const regex_t *restrict, const char *restrict, size_t, regmatch_t[restrict], int);
  # void   regfree(regex_t *);

#------------------------------------------------------------------------------
# sched.h - execution scheduling (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# search.h - search tables (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# semaphore.h - semaphores (POSTPONED FOR FURTHER DISCUSSION)
#------------------------------------------------------------------------------

  # Postponed until we discuss threads with Dan.

#------------------------------------------------------------------------------
# setjmp.h - stack environment declarations (PENDING)
#------------------------------------------------------------------------------

  # void   longjmp(jmp_buf, int);
  # void   siglongjmp(sigjmp_buf, int);
  # void  _longjmp(jmp_buf, int);
  # int    setjmp(jmp_buf);
  # int    sigsetjmp(sigjmp_buf, int);
  # int   _setjmp(jmp_buf);

#------------------------------------------------------------------------------
# signal.h - signals (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # void (*bsd_signal(int, void (*)(int)))(int);
  # int    kill(pid_t, int);
  # int    killpg(pid_t, int);
  # int    pthread_kill(pthread_t, int);
  # int    pthread_sigmask(int, const sigset_t *, sigset_t *);
  # int    raise(int);
  # int    sigaction(int, const struct sigaction *restrict, struct sigaction *restrict);
  # int    sigaddset(sigset_t *, int);
  # int    sigaltstack(const stack_t *restrict, stack_t *restrict);
  # int    sigdelset(sigset_t *, int);
  # int    sigemptyset(sigset_t *);
  # int    sigfillset(sigset_t *);
  # int    sighold(int);
  # int    sigignore(int);
  # int    siginterrupt(int, int);
  # int    sigismember(const sigset_t *, int);
  # void (*signal(int, void (*)(int)))(int);
  # int    sigpause(int);
  # int    sigpending(sigset_t *);
  # int    sigprocmask(int, const sigset_t *restrict, sigset_t *restrict);
  # int    sigqueue(pid_t, int, const union sigval);
  # int    sigrelse(int);
  # void (*sigset(int, void (*)(int)))(int);
  # int    sigsuspend(const sigset_t *);
  # int    sigtimedwait(const sigset_t *restrict, siginfo_t *restrict, const struct timespec *restrict);
  # int    sigwait(const sigset_t *restrict, int *restrict);
  # int    sigwaitinfo(const sigset_t *restrict, siginfo_t *restrict);
  #

#------------------------------------------------------------------------------
# spawn.h - spawn (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# stdarg.h - handle variable argument list (PENDING)
#------------------------------------------------------------------------------

  # void va_start(va_list ap, argN);
  # void va_copy(va_list dest, va_list src);
  # type va_arg(va_list ap, type);
  # void va_end(va_list ap);

#------------------------------------------------------------------------------
# stdbool.h - boolean type and values (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# stddef.h - standard type definitions (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# stdint.h - integer types (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# stdio.h - standard buffered input/output (PENDING)
#------------------------------------------------------------------------------

  # char    *ctermid(char *); # also in dirent.h
  # FILE    *fdopen(int, const char *);
  # int      fileno(FILE *);
  # void     flockfile(FILE *);
  # int      fseeko(FILE *, off_t, int);
  # off_t    ftello(FILE *);
  # int      ftrylockfile(FILE *);
  # void     funlockfile(FILE *);
  # int      getc_unlocked(FILE *);
  # int      getchar_unlocked(void);
  # int      pclose(FILE *);
  # FILE    *popen(const char *, const char *);
  # int      putc_unlocked(int, FILE *);
  # int      putchar_unlocked(int);
  # char    *tempnam(const char *, const char *);

  # The following are defined with the libc attributes:
  #   clearerr, fclose, feof, ferror, fflush, fgetc, fgetpos,
  #   fgets, fopen, fprintf, fputc, fputs, fread, freopen,
  #   fscanf, fseek, fsetpos, ftell, fwrite, getc, getchar,
  #   gets, perror, printf, putc, putchar, puts, remove,
  #   rename, rewind, scanf, setbuf, setvbuf, snprintf, sprintf,
  #   sscanf, tmpfile, tmpnam, ungetc, vfprintf, vfscanf,
  #   vprintf, vscanf, vsnprintf, vsprintf, vsscanf

#------------------------------------------------------------------------------
# stdlib.h - standard library definitions (PENDING)
#------------------------------------------------------------------------------

  # a64l
  # drand48
  # ecvt
  # erand48
  # fcvt
  # gcvt
  # getsubopt
  # grantpt
  # initstate
  # jrand48
  # l64a
  # lcong48
  # lrand48
  # mktemp
  # mkstemp
  # mrand48
  # nrand48
  # posix_memalign
  # posix_openpt
  # ptsname
  # putenv
  # rand_r
  # random
  # realpath
  # seed48
  # setenv
  # setkey
  # setstate
  # srand48
  # srandom
  # unlockpt
  # unsetenv

  # The following are defined with the libc attributes:
  #   _Exit, abort, abs, atexit, atof, atoi, atol, atoll, bsearch,
  #   calloc, div, exit, free, getenv, labs, ldiv, llabs, lldiv,
  #   malloc, mblen, mbstowcs, mbtowc, qsort, rand, realloc, srand,
  #   strtod, strtof, strtol, strtold, strtoll, strtoul, strtoull,
  #   system, wcstombs, wctomb

#------------------------------------------------------------------------------
# string.h - string operations (PENDING)
#------------------------------------------------------------------------------

  # void    *memccpy(void *restrict, const void *restrict, int, size_t);
  # char    *strdup(const char *);
  # int     *strerror_r(int, char *, size_t);
  # char    *strtok_r(char *, const char *, char **);

  # The following are defined with the libc attributes:
  #   memchr, memcmp, memcpy, memmove, memset, strcat, strchr,
  #   strcmp, strcoll, strcpy, strcspn, strerror, strlen,
  #   strncat, strncmp, strncpy, strpbrk, strrchr, strspn
  #   strstr, strtok, strxfrm

#------------------------------------------------------------------------------
# strings.h - string operations (DONE)
#------------------------------------------------------------------------------

  set attr(bcopy)  "buffer ( buffer_index = 1,
                             type = read,
                             size_index = 3 ),
                    buffer ( buffer_index = 2,
                             type = write,
                             size_index = 3 ),
                    no_other_side_effects"

  set attr(bzero)  "buffer ( buffer_index = 1,
                             type = write,
                             size_index = 2 ),
                    no_other_side_effects"

  set attr(ffs)    "const,
                    property ( index = return,
                               property_type = provides,
                               type = output,
                               test_type = greater_than,
                               test_value = 0 )
                          if ( index = 1,
                               type = input,
                               test_type = not_equal,
                               test_value = 0 ),
                    property ( index = return,
                               property_type = provides,
                               type = output,
                               test_type = equal,
                               test_value = 0 )
                          if ( index = 1,
                               type = input,
                               test_type = equal,
                               test_value = 0 ),
                    no_other_side_effects"

  beam::function_attribute "$attr(memcmp)"  -signatures "bcmp"
  beam::function_attribute "$attr(bcopy)"   -signatures "bcopy"
  beam::function_attribute "$attr(bzero)"   -signatures "bzero"
  beam::function_attribute "$attr(ffs)"     -signatures "ffs"
  beam::function_attribute "$attr(strchr)"  -signatures "index" "rindex"
  beam::function_attribute "$attr(strcmp)"  -signatures "strcasecmp"
  beam::function_attribute "$attr(strncmp)" -signatures "strncasecmp"

#------------------------------------------------------------------------------
# stropts.h - STREAMS interface (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # int    isastream(int);
  # int    getmsg(int, struct strbuf *restrict, struct strbuf *restrict, int *restrict);
  # int    getpmsg(int, struct strbuf *restrict, struct strbuf *restrict, int *restrict, int *restrict);
  # int    ioctl(int, int, ... );
  # int    putmsg(int, const struct strbuf *, const struct strbuf *, int);
  # int    putpmsg(int, const struct strbuf *, const struct strbuf *, int, int);
  # int    fattach(int, const char *);
  # int    fdetach(const char *);

#------------------------------------------------------------------------------
# sys/ipc.h - XSI interprocess communication access structure (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/mman.h - memory management declarations (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # int    mlock(const void *, size_t);
  # int    mlockall(int);
  # void  *mmap(void *, size_t, int, int, int, off_t);
  # int    mprotect(void *, size_t, int);
  # int    msync(void *, size_t, int);
  # int    munlock(const void *, size_t);
  # int    munlockall(void);
  # int    munmap(void *, size_t);
  # int    posix_madvise(void *, size_t, int);
  # int    posix_mem_offset(const void *restrict, size_t, off_t *restrict, size_t *restrict, int *restrict);
  # int    posix_typed_mem_get_info(int, struct posix_typed_mem_info *);
  # int    posix_typed_mem_open(const char *, int, int);
  # int    shm_open(const char *, int, mode_t);
  # int    shm_unlink(const char *);

#------------------------------------------------------------------------------
# sys/msg.h - XSI message queue structures (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/resource.h - definitions for XSI resource operations (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/select.h - select types (PENDING)
#------------------------------------------------------------------------------

  # int  pselect(int, fd_set *restrict, fd_set *restrict, fd_set *restrict, const struct timespec *restrict, const sigset_t *restrict);
  # int  select(int, fd_set *restrict, fd_set *restrict, fd_set *restrict, struct timeval *restrict);

#------------------------------------------------------------------------------
# sys/sem.h - XSI semaphore facility (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/shm.h - XSI shared memory facility (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/socket.h - main sockets header (PENDING)
#------------------------------------------------------------------------------

  # int accept(int socket, struct sockaddr *address, socklen_t *address_len);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #
  #  - allocates a new file descriptor

  # int bind(int socket, const struct sockaddr *address,
  #          socklen_t address_len);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires address to be non-null

  # int connect(int socket, const struct sockaddr *address,
  #             socklen_t address_len);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #
  #  - puts     socket into unspecified state if it fails ("socket state" is "unspecified")

  # int getpeername(int socket, struct sockaddr *address,
  #                socklen_t *address_len);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0))
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires address to be non-null

  # int getsockname(int socket, struct sockaddr *address,
  #                 socklen_t *address_len);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires address to be non-null

  # int listen(int socket, int backlog)
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires socket not to be connected already   ("socket state" is "disconnected")
  #  - makes    socket connected                     ("socket state" is "connected")

  # ssize_t recv(int socket, void *buffer, size_t length, int flags);
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires buffer to be non-null
  #  - writes into buffer possibly as many bytes as given by length

  # ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
  #                  struct sockaddr *address, socklen_t *address_len);
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires socket to be connected               ("socket state" is "connected")
  #  - requires buffer to be non-null
  #  - writes into buffer possibly as many bytes as given by length

  # ssize_t recvmsg(int socket, struct msghdr *message, int flags);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires socket to be connected               ("socket state" is "connected")

  # ssize_t send(int socket, const void *buffer, size_t length, int flags);
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")
  #  - requires socket to be connected               ("socket state" is "connected")
  #  - requires buffer to be non-null
  #  - reads from buffer as many bytes as given by length

  # ssize_t sendmsg (int socket, const struct msghdr *message, int flags);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")

  # ssize_t sendto(int socket, const void *message, size_t length, int flags,
  #                const struct sockaddr *dest_addr, socklen_t dest_len);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")

  # int setsockopt(int socket, int level, int option_name, const void
  #                *option_value, socklen_t option_len);
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be opened                  ("file state" is "opened")

  # int shutdown(int socket, int how);
  #
  #  - requires socket to be a valid file descriptor (socket >= 0)
  #  - requires socket to be connected               ("socket state" is "connected")

  # int socket(int domain, int type, int protocol);
  #
  #  - returns a opened file descriptor if >= 0      ("file state"   is "opened")
  #  - returns a disconnected socket    if >= 0      ("socket state" is "disconnected")

#------------------------------------------------------------------------------
# sys/stat.h - data returned by the stat() function (DONE)
#------------------------------------------------------------------------------

  set attr(chmod) "$side_effect_on_environment,
                   buffer ( buffer_index = 1,
                            type = read,
                            string_index = 1,
                            padding = 1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              range_min = -1,
                              range_max = 0 ),
                   no_other_side_effects"

  set attr(fchmod) "$side_effect_on_environment,
                    property ( index = 1,
                               property_type = requires,
                               type = input,
                               test_type = greater_than_or_equal,
                               test_value = 0 ),
                    property ( index = 1,
                               property_type = requires,
                               type = input,
                               property_name = \"file state\",
                               property_value = \"opened\" ),
                    property ( index = return,
                               property_type = provides,
                               type = output,
                               range_min = -1,
                               range_max = 0 ),
                    no_other_side_effects"

  set attr(fstat) "$side_effect_on_environment,
                   property ( index = 1,
                              property_type = requires,
                              type = input,
                              test_type = greater_than_or_equal,
                              test_value = 0 ),
                   property ( index = 1,
                              property_type = requires,
                              type = input,
                              property_name = \"file state\",
                              property_value = \"opened\" ),
                   buffer ( buffer_index = 2,
                            type = write,
                            units = elements,
                            size = 1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              range_min = -1,
                              range_max = 0 ),
                   no_other_side_effects"

  set attr(lstat) "$side_effect_on_environment,
                   buffer ( buffer_index = 1,
                            type = read,
                            string_index = 1,
                            padding = 1 ),
                   buffer ( buffer_index = 2,
                            type = write,
                            units = elements,
                            size = 1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              range_min = -1,
                              range_max = 0 ),
                   no_other_side_effects"

  set attr(mkdir) "$side_effect_on_environment,
                   buffer ( buffer_index = 1,
                            type = read,
                            string_index = 1,
                            padding = 1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              range_min = -1,
                              range_max = 0 ),
                   no_other_side_effects"

  set attr(umask) "$side_effect_on_environment,
                   no_other_side_effects"

  beam::function_attribute "$attr(chmod)"  -signatures "chmod"
  beam::function_attribute "$attr(fchmod)" -signatures "fchmod"
  beam::function_attribute "$attr(fstat)"  -signatures "fstat"
  beam::function_attribute "$attr(lstat)"  -signatures "lstat" "stat"
  beam::function_attribute "$attr(mkdir)"  -signatures "mkdir" "mkfifo" "mknod"
  beam::function_attribute "$attr(umask)"  -signatures "umask"

#------------------------------------------------------------------------------
# sys/statvfs.h - VFS File System information structure (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/time.h - time types (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # int   getitimer(int, struct itimerval *);
  # int   gettimeofday(struct timeval *restrict, void *restrict);
  # int   select(int, fd_set *restrict, fd_set *restrict, fd_set *restrict, struct timeval *restrict);
  # int   setitimer(int, const struct itimerval *restrict, struct itimerval *restrict);
  # int   utimes(const char *, const struct timeval [2]); (LEGACY )

#------------------------------------------------------------------------------
# sys/timeb.h - additional definitions for date and time (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# sys/times.h - file access and modification times structure (PENDING)
#------------------------------------------------------------------------------

  # clock_t times(struct tms *);

#------------------------------------------------------------------------------
# sys/types.h - data types (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# sys/uio.h - definitions for vector I/O operations (PENDING - LOW PRIORITY)
#------------------------------------------------------------------------------

  # ssize_t readv(int, const struct iovec *, int);
  # ssize_t writev(int, const struct iovec *, int);

#------------------------------------------------------------------------------
# sys/un.h - definitions for UNIX domain sockets (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# sys/utsname.h - system name structure (DONE)
#------------------------------------------------------------------------------

  set attr(uname) "$side_effect_on_environment,
                   buffer ( buffer_index = 1,
                            type = write,
                            units = elements,
                            size = 1 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              test_type = greater_than_or_equal,
                              test_value = -1 ),
                   no_other_side_effects"

  beam::function_attribute "$attr(uname)" -signatures "uname"

#------------------------------------------------------------------------------
# sys/wait.h - declarations for waiting (PENDING)
#------------------------------------------------------------------------------

  # pid_t  wait(int *);
  # int    waitid(idtype_t, id_t, siginfo_t *, int);
  # pid_t  waitpid(pid_t, int *, int);

#------------------------------------------------------------------------------
# syslog.h - definitions for system error logging (PENDING)
#------------------------------------------------------------------------------

  # void  closelog(void);
  # void  openlog(const char *, int, int);
  # int   setlogmask(int);
  # void  syslog(int, const char *, ...);

#------------------------------------------------------------------------------
# tar.h - extended tar definitions (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# termios.h - define values for termios (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# tgmath.h - type-generic macros (NO FUNCTIONS)
#------------------------------------------------------------------------------

  # No functions.

#------------------------------------------------------------------------------
# time.h - time types (PENDING)
#------------------------------------------------------------------------------

  # asctime_r
  # clock_getcpuclockid
  # clock_getres
  # clock_gettime
  # clock_nanosleep
  # clock_settime
  # ctime_r
  # getdate
  # gmtime_r
  # localtime_r
  # nanosleep
  # strptime
  # timer_create
  # timer_delete
  # timer_gettime
  # timer_getoverrun
  # timer_settime
  # tzset

  # The following are defined with the libc attributes:
  #   asctime, clock, ctime, difftime, gmtime, localtime, mktime
  #   strftime, time,

#------------------------------------------------------------------------------
# trace.h - tracing (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# ucontext.h - user context (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# ulimit.h - ulimit commands (DONE)
#------------------------------------------------------------------------------

  set attr(ulimit) "$side_effect_on_environment,
                    no_other_side_effects"

  beam::function_attribute "$attr(ulimit)" -signatures "ulimit"

#------------------------------------------------------------------------------
# unistd.h - standard symbolic constants and types (PENDING - PARTIALLY DONE)
#------------------------------------------------------------------------------

  # Note that for functions which take or return file descriptors, the
  # definition of a file descriptor from the standard says:
  # "The value of a file descriptor is from zero to {OPEN_MAX}."

  set attr(access)      "pure,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    range_min = -1,
                                    range_max = 0 ),
                         no_other_side_effects"

  set attr(alarm)       "$side_effect_on_environment,
                         no_other_side_effects"

  set attr(chdir)       "$side_effect_on_environment,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    range_min = -1,
                                    range_max = 0 ),
                         no_other_side_effects"

  set attr(chown)       "$side_effect_on_environment,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    range_min = -1,
                                    range_max = 0 ),
                         no_other_side_effects"

  set attr(close)       "$side_effect_on_environment,
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    test_type = greater_than_or_equal,
                                    test_value = 0 ),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    property_name = \"file state\",
                                    property_value = \"opened\" ),
                         property ( index = 1,
                                    property_type = provides,
                                    type = output,
                                    property_name = \"file state\",
                                    property_value = \"closed\"),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    range_min = -1,
                                    range_max = 0 ),
                         no_other_side_effects"

  set attr(confstr)     "buffer ( buffer_index = 2,
                                  type = write,
                                  size_index = 3 )
                             if ( index = 3,
                                  type = input,
                                  test_type = greater_than,
                                  test_value = 0 ),
                         no_other_side_effects"

  set attr(crypt)       "pure,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         buffer ( buffer_index = 2,
                                  type = read,
                                  string_index = 2,
                                  padding = 1 ),
                         $returns_static_like,
                         no_other_side_effects"

  set attr(ctermid)     "memmod ( index = 1,
                                  first_bit = 0,
                                  num_bits = -1 ),
                         return_overlap ( return_index = return,
                                          points_into_index = 1,
                                          fate = may ),
                         no_other_side_effects"

  set attr(dup)         "property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    property_name = \"file state\",
                                    property_value = \"opened\" )
                               if ( index = return,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = 0),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    property_name = \"file state\",
                                    property_value = \"opened\" ),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    test_type = greater_than_or_equal,
                                    test_value = 0 ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = -1 ),
                         no_other_side_effects"

  set attr(dup2)        "property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    property_name = \"file state\",
                                    property_value = \"opened\" )
                               if ( index = return,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = 0),
                         anchor   ( index = 2)
                               if ( index = return,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = 0),
                         anchor   ( index = return)
                               if ( index = return,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = 0),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    property_name = \"file state\",
                                    property_value = \"opened\" ),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    test_type = greater_than_or_equal,
                                    test_value = 0 ),
                         property ( index = 2,
                                    property_type = provides,
                                    type = output,
                                    property_name = \"file state\",
                                    property_value = \"opened\" )
                               if ( index = return,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = 0),
                         property ( index = 2,
                                    property_type = requires,
                                    type = input,
                                    test_type = greater_than_or_equal,
                                    test_value = 0 ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    test_type = greater_than_or_equal,
                                    test_value = -1 ),
                         no_other_side_effects"

  set attr(encrypt)     "buffer ( buffer_index = 1,
                                  type = read,
                                  size = 64 ),
                         buffer ( buffer_index = 1,
                                  type = write,
                                  size = 64 ),
                         no_other_side_effects"

  set attr(execl)       "$side_effect_on_environment,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         buffer ( buffer_index = 2,
                                  type = read,
                                  string_index = 2,
                                  padding = 1 )"

  set attr(execv)       "$side_effect_on_environment,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         buffer ( buffer_index = 2,
                                  type = read,
                                  units = elements,
                                  size = 1)"

  set attr(execve)      "$side_effect_on_environment,
                         buffer ( buffer_index = 1,
                                  type = read,
                                  string_index = 1,
                                  padding = 1 ),
                         buffer ( buffer_index = 2,
                                  type = read,
                                  units = elements,
                                  size = 1),
                         buffer ( buffer_index = 3,
                                  type = read,
                                  units = elements,
                                  size = 1)"

  set attr(_exit)       "noreturn,
                         no_other_side_effects"

  set attr(fchown)      "$side_effect_on_environment,
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    test_type = greater_than_or_equal,
                                    test_value = 0 ),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    property_name = \"file state\",
                                    property_value = \"opened\" ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    range_min = -1,
                                    range_max = 0 ),
                         no_other_side_effects"

  set attr(fchdir)      "$side_effect_on_environment,
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    test_type = greater_than_or_equal,
                                    test_value = 0 ),
                         property ( index = 1,
                                    property_type = requires,
                                    type = input,
                                    property_name = \"file state\",
                                    property_value = \"opened\" ),
                         property ( index = return,
                                    property_type = provides,
                                    type = output,
                                    range_min = -1,
                                    range_max = 0 ),
                         no_other_side_effects"

  beam::function_attribute "$attr(access)"      -signatures "access"
  beam::function_attribute "$attr(alarm)"       -signatures "alarm"
  beam::function_attribute "$attr(chdir)"       -signatures "chdir"
  beam::function_attribute "$attr(chown)"       -signatures "chown"
  beam::function_attribute "$attr(close)"       -signatures "close"
  beam::function_attribute "$attr(confstr)"     -signatures "confstr"
  beam::function_attribute "$attr(crypt)"       -signatures "crypt"
  beam::function_attribute "$attr(ctermid)"     -signatures "ctermid"
  beam::function_attribute "$attr(dup)"         -signatures "dup"
  beam::function_attribute "$attr(dup2)"        -signatures "dup2"
  beam::function_attribute "$attr(encrypt)"     -signatures "encrypt"
  beam::function_attribute "$attr(execl)"       -signatures "execl" \
                                                            "execle" "execlp"
  beam::function_attribute "$attr(execv)"       -signatures "execv" "execvp"
  beam::function_attribute "$attr(execve)"      -signatures "execve"
  beam::function_attribute "$attr(_exit)"       -signatures "_exit"
  beam::function_attribute "$attr(fchown)"      -signatures "fchown"
  beam::function_attribute "$attr(fchdir)"      -signatures "fchdir"

#------------------------------------------------------------------------------
# utime.h - access and modification times structure (DONE)
#------------------------------------------------------------------------------

  set attr(utime) "$side_effect_on_environment,
                   buffer ( buffer_index = 1,
                            type = read,
                            string_index = 1,
                            padding = 1 ),
                   buffer ( buffer_index = 2,
                            type = read,
                            units = elements,
                            size = 1 )
                       if ( index = 2,
                            type = input,
                            test_type = not_equal,
                            test_value = 0 ),
                   property ( index = return,
                              property_type = provides,
                              type = output,
                              range_min = -1,
                              range_max = 0 ),
                   no_other_side_effects"

  beam::function_attribute "$attr(utime)" -signatures "utime"

#------------------------------------------------------------------------------
# utmpx.h - user accounting database definitions (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# wchar.h - wide-character handling (DONE)
#------------------------------------------------------------------------------

  # Note that the wide character functions depend on the locale settings
  # stored in global variables, even though this is not always explicitly
  # stated in the individual function descriptions.

  set attr(wcswidth) "pure,
                      buffer ( buffer_index = 1,
                               type = read,
                               units = elements,
                               bound_string_index = 1,
                               bound_padding = 1,
                               bound_size_index = 2 ),
                      property ( index = return,
                                 property_type = provides,
                                 type = output,
                                 test_type = greater_than_or_equal,
                                 test_value = -1 ),
                      no_other_side_effects"

  set attr(wcwidth)  "pure,
                      property ( index = return,
                                  property_type = provides,
                                  type = output,
                                  test_type = greater_than_or_equal,
                                  test_value = -1 ),
                      no_other_side_effects"

  beam::function_attribute "$attr(wcswidth)" -signatures "wcswidth"
  beam::function_attribute "$attr(wcwidth)"  -signatures "wcwidth"

  # The following is marked LEGACY in the standard:
  #   wcswcs

  # The following are defined with the libc attributes:
  #   btowc, fgetwc, fgetws, fputwc, fputws, fwide, fwprintf, fwscanf,
  #   getwc, getwchar, iswalnum, iswalpha, iswcntrl, iswctype, iswdigit,
  #   iswgraph, iswlower, iswprint, iswpunct, iswspace, iswupper,
  #   iswxdigit, mbrlen, mbrtowc, mbsinit, mbsrtowcs, putwc, putwchar,
  #   swprintf, swscanf, towlower, towupper, ungetwc, vfwprintf,
  #   vfwscanf, vwprintf, vswprintf, vswscanf, vwscanf, wcrtomb,
  #   wcscat, wcschr, wcscmp, wcscoll, wcscpy, wcscspn, wcsftime,
  #   wcslen, wcsncat, wcsncmp, wcsncpy, wcspbrk, wcsrchr, wcsrtombs,
  #   wcsspn, wcsstr, wcstod, wcstof, wcstok, wcstol, wcstold, wcstoll,
  #   wcstoul, wcstoull, wcsxfrm, wctob, wctype, wmemchr, wmemcmp,
  #   wmemcpy, wmemmove, wmemset, wprintf, wscanf

#------------------------------------------------------------------------------
# wctype.h - wide-character classification and mapping utilities (ALL LIBC)
#------------------------------------------------------------------------------

  # The following are defined with the libc attributes:
  #   iswalnum, iswalpha, iswblank, iswcntrl, iswdigit, iswgraph,
  #   iswlower, iswprint, iswpunct, iswspace, iswupper, iswxdigit,
  #   iswctype, towctrans, towlower, towupper, wctrans, wctype

#------------------------------------------------------------------------------
# wordexp.h - word-expansion types (SKIPPED)
#------------------------------------------------------------------------------

  # Skipped.

#------------------------------------------------------------------------------
# Miscellaneous Security Advisories
#------------------------------------------------------------------------------

  beam::function_attribute {
    advisory (
      explanation = "Function `getwd' may overflow its argument because PATH_MAX may not be bounded. Use `getcwd' instead.",
      category    = "security.buffer"
    )
  } -signatures "getwd"

  beam::function_attribute {
    advisory (
      explanation = "Function `tempnam' is unsafe because of a potential race condition. Use `mkstemp' instead.",
      category    = "security.race"
    )
  } -signatures "tempnam"

  beam::function_attribute {
    advisory (
      explanation = "Function `mktemp' is unsafe because of a potential race condition. Use `mkstemp' instead.",
      category    = "security.race"
    )
  } -signatures "mktemp"

  beam::function_attribute {
    advisory (
      explanation = "Function `chown' is unsafe because of a potential race condition. Use `fchown' instead.",
      category    = "security.race"
    )
  } -signatures "chown"

  beam::function_attribute {
    advisory (
      explanation = "Function `chmod' is unsafe because of a potential race condition. Use `fchmod' instead.",
      category    = "security.race"
    )
  } -signatures "chmod"


####################### ORGINAL CONTENTS ###########################

# The functions given attributes in this file have no other side-effects

 beam::function_attribute  no_other_side_effects  -signatures \
        "alloca"      \
      	"cuserid"     \
      	"exec"        \
      	"getcwd"      \
      	"getwd"       \
      	"read"        \
      	"readlink"    \
      	"setenv"      \
      	"socket"      \
      	"stat"        \
      	"strdup"      \
      	"write"       \


  beam::function_attribute  $alloca_like -signatures "alloca"

  # We can't model the size exactly yet
  beam::function_attribute  $unknown_allocator_like     -signatures "strdup"

  beam::function_attribute  $buffer2_size3_write_like   -signatures "readlink"
  beam::function_attribute  $buffer2_size3_write_like   -signatures "read"
  beam::function_attribute  $buffer2_size3_read_like    -signatures "write"

  beam::function_attribute  $side_effect_on_environment -signatures "exec" "setenv"

  # All functions doing I/O have a side effect on the environment
  beam::function_attribute  $side_effect_on_environment -signatures "read" "write" \
                                                                    "socket"

  beam::function_attribute {
      memmod ( global_variable = NULL,
	       index           = 1,         # assigns the first arg
	       first_bit       = 0,         # assigns buffer from the beginning
	       num_bits        = -1 ),      # assigns unknown size
  } -signatures "cuserid" "getwd"

  beam::function_attribute {
    buffer ( buffer_index = 1, type = write, size_index = 4 ),
    buffer ( buffer_index = 2, type = read, size_index = 4 ),
    no_other_side_effects
  } -signatures "memccpy"

  beam::function_attribute {
      memmod ( global_variable = NULL,
	       index           = 2,         # assigns the second arg
	       first_bit       = 0,         # assigns buffer from the beginning
	       num_bits        = -1 ),      # assigns the whole buffer
  } -signatures "stat"

  # allocators and matching deallocators

  beam::function_attribute {
      property (index = return,             # Which parameter has he property
                property_type = provides,
                type = output,
                property_name = "memory allocation source",
                property_value = "from malloc")
  } -signatures "strdup"



  # fd = open(name, mode) returns a new descriptor to an open file
  # failure to open the file is indicated by returning -1

  beam::function_attribute {

      property (index = return,
                property_type = provides,
                type = output,
                property_name = "file state",
                property_value = "opened")

      if (index = return,
          type = output,
          test_type = greater_than_or_equal,
          test_value = 0),
      anchor ( index = return, fate = wont),
  } -signatures "socket"


  beam::function_attribute "
      $buffer1_size2_write_like
      if (index           = 1,
          type            = input,
          test_type       = not_equal,
          test_value      = 0),

       allocator ( size_index = 2,
                   multiplier_index = unset,
		   return_index = return,
		  #anchored,                               # it needs to be freed
		   if_out_of_memory = return_null,
		   initial_state = initialized_to_unknown, # data is put into the new memory
		   if_size_is_0 = error,
		   if_size_is_negative = error,
                   resource = heap_memory)
      if (index           = 1,
          type            = input,
          test_type       = equal,
          test_value      = 0)
  " -signatures "getcwd"

  # getcwd allocates memory  using malloc (as opposed to xmalloc, etc)
  # which must be freed using free()

  beam::function_attribute {
      property (index = return,          # Which parameter has he property
                property_type = provides,
                type = output,
                property_name = "memory allocation source",
                property_value = "from malloc")
      if (index           = 1,
          type            = input,
          test_type       = equal,
          test_value      = 0)
  } -signatures "getcwd"
}
