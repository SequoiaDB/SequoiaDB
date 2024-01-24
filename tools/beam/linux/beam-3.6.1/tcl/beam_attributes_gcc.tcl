# -*- Tcl -*-
#
# Licensed Materials - Property of IBM - RESTRICTED MATERIALS OF IBM
#
# IBM Confidential - OCO Source Materials
#
# Copyright (C) 2004-2010 IBM Corporation. All rights reserved.
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
#        Florian Krohm
#
#    DESCRIPTION:
#
#        Attributes for GCC's builtin functions.
#        The list of function is a subset of what GCC 3.4 supports.
#        Below are the function that EDG 3.8 recognizes and for which 
#        we do not have attributes:
#
#        FIXME: this ought to be updated once we move to EDG 3.10
#        FIXME: I was using builtins.def from GCC 4.0.0 to determine
#        FIXME: whether or not a builtin function was available.
#
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        06/02/05  florian  Created
#

namespace eval beam::attribute {

  #----------------------------------------------------------------------------
  # [7.3] Complex arithmetic <complex.h>
  #----------------------------------------------------------------------------

  # [7.3.5] Trigonometric functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_cacos" "__builtin_cacosf" "__builtin_cacosl" \
      "__builtin_casin" "__builtin_casinf" "__builtin_casinl" \
      "__builtin_catan" "__builtin_catanf" "__builtin_catanl" \
      "__builtin_ccos"  "__builtin_ccosf"  "__builtin_ccosl"  \
      "__builtin_csin"  "__builtin_csinf"  "__builtin_csinl"  \
      "__builtin_ctan"  "__builtin_ctanf"  "__builtin_ctanl"

  # [7.3.6] Hyperbolic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_cacosh" "__builtin_cacoshf" "__builtin_cacoshl" \
      "__builtin_casinh" "__builtin_casinhf" "__builtin_casinhl" \
      "__builtin_catanh" "__builtin_catanhf" "__builtin_catanhl" \
      "__builtin_ccosh"  "__builtin_ccoshf"  "__builtin_ccoshl"  \
      "__builtin_csinh"  "__builtin_csinhf"  "__builtin_csinhl"  \
      "__builtin_ctanh"  "__builtin_ctanhf"  "__builtin_ctanhl"           

  # [7.3.7] Exponential and logarithmic functions

  # These are pure because they check the flag_rounding_math global variable
  beam::function_attribute "$attr(pure_function)" -signatures \
      "__builtin_cexp"   "__builtin_cexpf"   "__builtin_cexpl"  \
      "__builtin_cexpi"  "__builtin_cexpif"  "__builtin_cexpil" \
      "__builtin_clog"   "__builtin_clogf"   "__builtin_clogl"  \
      "__builtin_clog10" "__builtin_clog10f" "__builtin_clog10l"

  # [7.3.8] Power and absolute-value functions
      
  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_cabs"  "__builtin_cabsf"  "__builtin_cabsl" \
      "__builtin_cpow"  "__builtin_cpowf"  "__builtin_cpowl" \
      "__builtin_csqrt" "__builtin_csqrtf" "__builtin_csqrtl"

  # [7.3.9] Manipulation functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_carg"  "__builtin_cargf"  "__builtin_cargl"  \
      "__builtin_cimag" "__builtin_cimagf" "__builtin_cimagl" \
      "__builtin_conj"  "__builtin_conjf"  "__builtin_conjl"  \
      "__builtin_cproj" "__builtin_cprojf" "__builtin_cprojl" \
      "__builtin_creal" "__builtin_crealf" "__builtin_creall"


  #----------------------------------------------------------------------------
  # [7.4] Character handling <ctype.h>
  #----------------------------------------------------------------------------

  #* NOTE: The only two function in this section that are _NOT_ affected
  #*       by the current locale are isdigit() and isxdigit(), so all the
  #*       others must be pure instead of const.

  # [7.4.1] Character classification functions

  beam::function_attribute "$attr(pure_function)" -signatures \
      "__builtin_isalnum" "__builtin_isalpha" "__builtin_isblank" \
      "__builtin_iscntrl" "__builtin_isgraph" "__builtin_islower" \
      "__builtin_isprint" "__builtin_ispunct" "__builtin_isspace" \
      "__builtin_isupper" 

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_isdigit" "__builtin_isxdigit"

  # [7.4.2] Character mapping functions

  beam::function_attribute "$attr(pure_function)" -signatures \
      "__builtin_tolower" "__builtin_toupper"


  #----------------------------------------------------------------------------
  # [7.5] Errors <errno.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.6] Floating-point environment <fenv.h>
  #----------------------------------------------------------------------------

  # No builtin for "feclearexcept" available
  # No builtin for "fegetexceptflag" available
  # No builtin for "feraiseexcept" available
  # No builtin for "fesetexceptflag" available
  # No builtin for "fetestexcept" available
  # No builtin for "fegetround" available
  # No builtin for "fesetround" available
  # No builtin for "fegetenv" available
  # No builtin for "feholdexcept" available
  # No builtin for "fesetenv" available
  # No builtin for "feupdateenv" available


  #----------------------------------------------------------------------------
  # [7.7] Characteristics of floating types <float.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.8] Format conversion of interger types <inttypes.h>
  #----------------------------------------------------------------------------

  beam::function_attribute "$attr(imaxabs)" -signatures "__builtin_imaxabs"

  # No builtin for "imaxdiv" available
  # No builtin for "strtoimax" available
  # No builtin for "strtoumax" available


  #----------------------------------------------------------------------------
  # [7.9] Alternative spellings <iso646.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.10] Sizes of integer types <limits.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.11] Localization <locale.h>
  #----------------------------------------------------------------------------

  # No builtin for "setlocale" available
  # No builtin for "localeconf" available

  #----------------------------------------------------------------------------
  # [7.12] Mathematics <math.h>
  #----------------------------------------------------------------------------

  # [7.12.4] Trigonometric functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_acos"  "__builtin_acosf"  "__builtin_acosl"  \
      "__builtin_asin"  "__builtin_asinf"  "__builtin_asinl"  \
      "__builtin_atan"  "__builtin_atanf"  "__builtin_atanl"  \
      "__builtin_atan2" "__builtin_atan2f" "__builtin_atan2l" \
      "__builtin_cos"   "__builtin_cosf"   "__builtin_cosl"   \
      "__builtin_sin"   "__builtin_sinf"   "__builtin_sinl"   \
      "__builtin_tan"   "__builtin_tanf"   "__builtin_tanl"  

  # [7.12.5] Hyperbolic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_acosh" "__builtin_acoshf" "__builtin_acoshl" \
      "__builtin_asinh" "__builtin_asinhf" "__builtin_asinhl" \
      "__builtin_atanh" "__builtin_atanhf" "__builtin_atanhl" \
      "__builtin_cosh"  "__builtin_coshf"  "__builtin_coshl"  \
      "__builtin_sinh"  "__builtin_sinhf"  "__builtin_sinhl"  \
      "__builtin_tanh"  "__builtin_tanhf"  "__builtin_tanhl"

  # [7.12.6] Exponential and logarithmic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_exp"     "__builtin_expf"     "__builtin_expl"    \
      "__builtin_exp2"    "__builtin_exp2f"    "__builtin_exp2l"   \
      "__builtin_expm1"   "__builtin_expm1f"   "__builtin_expm1l"  \
      "__builtin_ilogb"   "__builtin_ilogbf"   "__builtin_ilogbl"  \
      "__builtin_ldexp"   "__builtin_ldexpf"   "__builtin_ldexpl"  \
      "__builtin_log"     "__builtin_logf"     "__builtin_logl"    \
      "__builtin_log10"   "__builtin_log10f"   "__builtin_log10l"  \
      "__builtin_log1p"   "__builtin_log1pf"   "__builtin_log1pl"  \
      "__builtin_log2"    "__builtin_log2f"    "__builtin_log2l"   \
      "__builtin_logb"    "__builtin_logbf"    "__builtin_logbl"   \
      "__builtin_scalbn"  "__builtin_scalbnf"  "__builtin_scalbnl" \
      "__builtin_scalbln" "__builtin_scalblnf" "__builtin_scalblnl"

  beam::function_attribute "$attr(frexp)" -signatures \
      "__builtin_frexp" "__builtin_frexpf" "__builtin_frexpl"

  beam::function_attribute "$attr(modf)" -signatures \
      "__builtin_modf"  "__builtin_modff"  "__builtin_modfl"
  
  # [7.12.7] Power and absolute-value functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_cbrt"  "__builtin_cbrtf"  "__builtin_cbrtl"  \
      "__builtin_fabs"  "__builtin_fabsf"  "__builtin_fabsl"  \
      "__builtin_hypot" "__builtin_hypotf" "__builtin_hypotl" \
      "__builtin_pow"   "__builtin_powf"   "__builtin_powl"   \
      "__builtin_sqrt"  "__builtin_sqrtf"  "__builtin_sqrtl"

  # [7.12.8] Error and gamma functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_erf"    "__builtin_erff"    "__builtin_erfl"    \
      "__builtin_erfc"   "__builtin_erfcf"   "__builtin_erfcl"   \
      "__builtin_lgamma" "__builtin_lgammaf" "__builtin_lgammal" \
      "__builtin_tgamma" "__builtin_tgammaf" "__builtin_tgammal" 

  # [7.12.9] Nearest integer functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_ceil"      "__builtin_ceilf"      "__builtin_ceill"      \
      "__builtin_floor"     "__builtin_floorf"     "__builtin_floorl"     \
      "__builtin_nearbyint" "__builtin_nearbyintf" "__builtin_nearbyintl" \
      "__builtin_rint"      "__builtin_rintf"      "__builtin_rintl"      \
      "__builtin_lrint"     "__builtin_lrintf"     "__builtin_lrintl"     \
      "__builtin_llrint"    "__builtin_llrintf"    "__builtin_llrintl"    \
      "__builtin_round"     "__builtin_roundf"     "__builtin_roundl"     \
      "__builtin_lround"    "__builtin_lroundf"    "__builtin_lroundl"    \
      "__builtin_llround"   "__builtin_llroundf"   "__builtin_llroundl"   \
      "__builtin_trunc"     "__builtin_truncf"     "__builtin_truncl"

  # [7.12.10] Remainder functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_fmod"      "__builtin_fmodf"      "__builtin_fmodl"      \
      "__builtin_remainder" "__builtin_remainderf" "__builtin_remainderl" 

  beam::function_attribute "$attr(remquo)" -signatures \
      "__builtin_remquo" "__builtin_remquof" "__builtin_remquol"

  # [7.12.11] Manipulation functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_copysign"   "__builtin_copysignf"   "__builtin_copysignl"   \
      "__builtin_nextafter"  "__builtin_nextafterf"  "__builtin_nextafterl"  \
      "__builtin_nexttoward" "__builtin_nexttowardf" "__builtin_nexttowardl" 

  beam::function_attribute "$attr(nan)" -signatures \
      "__builtin_nan" "__builtin_nanf" "__builtin_nanl"

  # [7.12.12] Maximum, minimum, and positive difference functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_fdim" "__builtin_fdimf" "__builtin_fdiml" \
      "__builtin_fmax" "__builtin_fmaxl" "__builtin_fmaxl" \
      "__builtin_fmin" "__builtin_fminl" "__builtin_fminl"

  # [7.12.13] Floating multiply-add

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_fma" "__builtin_fmaf" "__builtin_fmal"


  # [7.12.14] Comparison macros

  beam::function_attribute "$attr(const_function)" -signatures \
     "__builtin_isgreater" "__builtin_isgreaterequal" \
     "__builtin_isless"    "__builtin_islessequal"    \
     "__builtin_islessgreater" \
     "__builtin_isunordered"


  #----------------------------------------------------------------------------
  # [7.13] Nonlocal jumps <setjmp.h>
  #----------------------------------------------------------------------------

  # [7.13.1] Save calling environment

  beam::function_attribute "$attr(setjmp)"  -signatures "__builtin_setjmp"


  # [7.13.2] Restore calling environment

  beam::function_attribute "$attr(longjmp)" -signatures "__builtin_longjmp"


  #----------------------------------------------------------------------------
  # [7.14] Signal handling <signal.h>
  #----------------------------------------------------------------------------

  # No builtin for "signal" available
  # No builtin for "raise"  available


  #----------------------------------------------------------------------------
  # [7.15] Variable arguments <stdarg.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.16] Boolean types and values <stdbool.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.17] Common definitions <stddef.h>
  #----------------------------------------------------------------------------
  
  # (no functions)


  #----------------------------------------------------------------------------
  # [7.18] Integer types <stdint.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.19] Input/output <stdio.h>
  #----------------------------------------------------------------------------
  
  # [7.19.4] Operations on files

  # No builtin for "remove"  available
  # No builtin for "rename"  available
  # No builtin for "tmpfile"  available
  # No builtin for "tmpnam"  available


  # [7.19.5] File access functions

  # No builtin for "fclose"  available
  # No builtin for "fflush"  available
  # No builtin for "fopen"  available
  # No builtin for "freopen"  available
  # No builtin for "setbuf"  available
  # No builtin for "setvbuf"  available


  # [7.19.6] Formatted input/output functions

  beam::function_attribute "$attr(fprintf)" -signatures \
      "__builtin_fprintf" "__builtin___fprintf_chk"

  beam::function_attribute "$attr(fscanf)"  -signatures "__builtin_fscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `__builtin_fscanf' is undefined when input does not match pattern.",
      category    = "security.pattern"
    )
  } -signatures "__builtin_fscanf"

  beam::function_attribute "$attr(printf)" -signatures \
      "__builtin_printf" "__builtin___printf_chk"

  beam::function_attribute "$attr(scanf)"  -signatures "__builtin_scanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `__builtin_scanf' is undefined when input does not match pattern.",
      category    = "security.pattern"
    )
  } -signatures "__builtin_scanf"

  beam::function_attribute "$attr(snprintf)" -signatures \
      "__builtin_snprintf" "__builtin___snprintf_chk"

  beam::function_attribute "$attr(sprintf)"  -signatures \
      "__builtin_sprintf" "__builtin___sprintf_chk"

  beam::function_attribute {
      advisory (explanation = "Function `__builtin_sprintf' is unsafe because of a potential buffer overrun. Use `__builtin_snprintf' instead.",
                category    = "security.buffer" 
                )
  } -signatures "__builtin_sprintf"
  
  beam::function_attribute "$attr(sscanf)"    -signatures "__builtin_sscanf"

  beam::function_attribute "$attr(vfprintf)"  -signatures \
      "__builtin_vfprintf" "__builtin___vfprintf_chk"

  beam::function_attribute "$attr(vfscanf)"   -signatures "__builtin_vfscanf"

  beam::function_attribute "$attr(vprintf)"   -signatures \
      "__builtin_vprintf" "__builtin___vprintf_chk"

  beam::function_attribute "$attr(vscanf)"    -signatures "__builtin_vscanf"

  beam::function_attribute "$attr(vsnprintf)" -signatures \
      "__builtin_vsnprintf" "__builtin___vsnprintf_chk"

  beam::function_attribute "$attr(vsprintf)"  -signatures \
      "__builtin_vsprintf" "__builtin___vsprintf_chk"

  beam::function_attribute {
      advisory ( explanation = "Function `__builtin_vsprintf' is unsafe because of a potential buffer overrun. Use `__builtin_vsnprintf' instead.",
               category    = "security.buffer" )
  } -signatures "__builtin_vsprintf"

  beam::function_attribute "$attr(vsscanf)"  -signatures "__builtin_vsscanf"


  # [7.19.7] Character input/output functions

  # No builtin for "fgetc"  available
  # No builtin for "fgets"  available

  beam::function_attribute "$attr(fputc)"  -signatures "__builtin_fputc" 
  beam::function_attribute "$attr(fputs)"  -signatures "__builtin_fputs" 

  # No builtin for "getc"  available
  # No builtin for "getchar"  available
  # No builtin for "gets"  available
  # No builtin for "putc"  available

  beam::function_attribute "$attr(putchar)" -signatures "__builtin_putchar" 
  beam::function_attribute "$attr(puts)"    -signatures "__builtin_puts"

  # No builtin for "ungetc"  available


  # [7.19.8] Direct input/output functions

  # No builtin for "fread"  available

  beam::function_attribute "$attr(fwrite)"  -signatures "__builtin_fwrite"

  # [7.19.9] File positioning functions
  
  # No builtin for "fgetpos"  available
  # No builtin for "fseek"    available
  # No builtin for "fsetpos"  available
  # No builtin for "ftell"    available
  # No builtin for "rewind"   available

  # [7.19.10] Error-handling functions

  # No builtin for "clearerr" available
  # No builtin for "feof"     available
  # No builtin for "ferror"   available
  # No builtin for "perror"   available


  #----------------------------------------------------------------------------
  # [7.20] General utilities <stdlib.h>
  #----------------------------------------------------------------------------

  # [7.20.1] Numeric conversion functions

  # No builtin for "atof"  available
  # No builtin for "atoi"  available
  # No builtin for "atol"  available
  # No builtin for "atoll"  available
  # No builtin for "strtod" available
  # No builtin for "strtof" available
  # No builtin for "strtold" available
  # No builtin for "strtol" available
  # No builtin for "strtoll" available
  # No builtin for "strtoul" available
  # No builtin for "strtoull" available


  # [7.20.2] Pseudo-random sequence generation functions

  # No builtin for "rand"  available
  # No builtin for "srand" available


  # [7.20.3] Memory management functions

  beam::function_attribute "$attr(calloc)"  -signatures "__builtin_calloc"
  beam::function_attribute "$attr(malloc)"  -signatures "__builtin_malloc" 
  beam::function_attribute "$attr(free)"    -signatures "__builtin_free"
  beam::function_attribute "$attr(realloc)" -signatures "__builtin_realloc"


  # [7.20.4] Communication with the environment

  beam::function_attribute "$attr(abort)"  -signatures "__builtin_abort"
  beam::function_attribute "$attr(exit)"   -signatures \
      "__builtin_exit" "__builtin__Exit"

  # No builtin for "atexit" available
  # No builtin for "getenv" available
  # No builtin for "system" available


  # [7.20.5] Searching and sorting utilities

  # No builtin for "bsearch" available
  # No builtin for "qsort" available


  # [7.20.6] Integer arithmetic functions

  beam::function_attribute "$attr(abs)" -signatures \
      "__builtin_abs" "__builtin_labs" "__builtin_llabs"

  # No builtin for "div" available
  # No builtin for "ldiv" available
  # No builtin for "lldiv" available


  # [7.20.7] Multibyte/wide character conversion functions

  # No builtin for "mblen"  available
  # No builtin for "mbtowc" available
  # No builtin for "wctomb" available


  # [7.20.8] Multibyte/wide string conversion functions

  # No builtin for "mbstowcs" available
  # No builtin for "wcstombs" available


  #----------------------------------------------------------------------------
  # [7.21] String handling <string.h>
  #----------------------------------------------------------------------------

  # [7.21.2] Copying functions

  beam::function_attribute "$attr(memcpy)" -signatures \
      "__builtin_memcpy" "__builtin_memmove" "__builtin_mempcpy" \
      "__builtin___memcpy_chk" "__builtin___memmove_chk" \
      "__builtin___mempcpy_chk"

  beam::function_attribute "$attr(strcpy)" -signatures \
      "__builtin_strcpy" "__builtin_stpcpy" "__builtin___stpcpy_chk" \
      "__builtin___strcpy_chk"

  beam::function_attribute {
      advisory (explanation = "Function `__builtin_strcpy' is unsafe because of a potential buffer overrun. Use `__builtin_strncpy' instead.",
                category    = "security.buffer" 
                )
  } -signatures "__builtin_strcpy"
  

  beam::function_attribute "$attr(strncpy)" -signatures \
      "__builtin_strncpy" "__builtin_stpncpy" "__builtin___strncpy_chk"

  # [7.21.3] Concatenation functions

  beam::function_attribute "$attr(strcat)" -signatures \
      "__builtin_strcat" "__builtin___strcat_chk"

  beam::function_attribute {
      advisory (explanation = "Function `__builtin_strcat' is unsafe because of a potential buffer overrun. Use `__builtin_strncat' instead.",
                category    = "security.buffer" 
                )
  } -signatures "__builtin_strcat"
  
  beam::function_attribute "$attr(strncat)" -signatures \
      "__builtin_strncat" "__builtin___strncat_chk"


  # [7.21.4] Comparison functions

  beam::function_attribute "$attr(memcmp)"  -signatures "__builtin_memcmp" 

  beam::function_attribute "$attr(strcmp)"  -signatures \
      "__builtin_strcmp" "__builtin_strcasecmp"

  beam::function_attribute "$attr(strncmp)" -signatures \
      "__builtin_strncmp" "__builtin_strncasecmp"

  # No builtin for "strcoll" available
  # No builtin for "strxfrm" available


  # [7.21.5] Search functions

  beam::function_attribute "$attr(memchr)" -signatures "__builtin_memchr"

  beam::function_attribute "$attr(strchr)" -signatures \
      "__builtin_strchr" "__builtin_strrchr" 

  beam::function_attribute "$attr(strcspn)" -signatures \
      "__builtin_strcspn" "__builtin_strspn" 

  beam::function_attribute "$attr(strpbrk)" -signatures \
      "__builtin_strpbrk" "__builtin_strstr" 

  # No builtin for "strtok" available


  # [7.21.6] Miscellaneous functions

  beam::function_attribute "$attr(memset)" -signatures \
      "__builtin_memset" "__builtin___memset_chk"

  beam::function_attribute "$attr(strlen)" -signatures "__builtin_strlen" 

  # No builtin for "strerror" available


  #----------------------------------------------------------------------------
  # [7.22] Type-generic math <tgmath.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.23] Date and time <time.h>
  #----------------------------------------------------------------------------

  # No builtin for "clock"  available
  # No builtin for "difftime"  available
  # No builtin for "mktime"  available
  # No builtin for "time"  available
  # No builtin for "asctime"  available
  # No builtin for "ctime"  available
  # No builtin for "gmtime"  available
  # No builtin for "localtime"  available

  beam::function_attribute "$attr(strftime)" -signatures "__builtin_strftime" 


  #----------------------------------------------------------------------------
  # [7.24] Extended multibyte and wide character utilities <wchar.h>
  #----------------------------------------------------------------------------

  # [7.24.2] Formatted wide character input/output functions

  # No builtin for "fwprintf"  available
  # No builtin for "fwscanf"   available
  # No builtin for "swprintf"  available
  # No builtin for "swscanf"   available
  # No builtin for "vfwprintf" available
  # No builtin for "vfwscanf"  available
  # No builtin for "vswprintf" available
  # No builtin for "vswscanf"  available
  # No builtin for "vwprintf"  available
  # No builtin for "vwscanf"   available
  # No builtin for "wprintf"   available
  # No builtin for "wscanf"    available


  # [7.24.3] Wide character input/output functions

  # No builtin for "fgetwc"   available
  # No builtin for "fgetwc"   available
  # No builtin for "fgetws"   available
  # No builtin for "fputwc"   available
  # No builtin for "fputws"   available
  # No builtin for "fwide"    available
  # No builtin for "getwc"    available
  # No builtin for "getwchar" available
  # No builtin for "putwc"    available
  # No builtin for "putwchar" available
  # No builtin for "ungetwc"  available


  # [7.24.4] General wide string utilities

  # No builtin for "wcstod"   available
  # No builtin for "wcstof"   available
  # No builtin for "wcstold"  available
  # No builtin for "wcstol"   available
  # No builtin for "wcstoll"  available
  # No builtin for "wcstoul"  available
  # No builtin for "wcstoull" available
  # No builtin for "wcscpy"   available
  # No builtin for "wcsncpy"  available
  # No builtin for "wmemcpy"  available
  # No builtin for "wmemmove" available
  # No builtin for "wcscat"   available
  # No builtin for "wcsncat"  available
  # No builtin for "wcscmp"   available
  # No builtin for "wcscoll"  available
  # No builtin for "wcsncmp"  available
  # No builtin for "wcsxfrm"  available
  # No builtin for "wmemcmp"  available
  # No builtin for "wcschr"   available
  # No builtin for "wcscspn"  available
  # No builtin for "wcspbrk"  available
  # No builtin for "wcsrchr"  available
  # No builtin for "wcsspn"   available
  # No builtin for "wcsstr"   available
  # No builtin for "wcstok"   available
  # No builtin for "wmemchr"  available
  # No builtin for "wcslen"   available
  # No builtin for "wmemset"  available


  # [7.24.5] Wide character time conversion functions

  # No builtin for "wcsftime" available


  # [7.24.6] Extended multibyte/wide character conversion utilities

  # No builtin for "btowc"     available
  # No builtin for "wctob"     available
  # No builtin for "mbsinit"   available
  # No builtin for "mbrlen"    available
  # No builtin for "mbrtowc"   available
  # No builtin for "wcrtomb"   available
  # No builtin for "mbsrtowcs" available
  # No builtin for "wcsrtombs" available

  #----------------------------------------------------------------------------
  # [7.25] Wide character classification and mapping utilities <wctype.h>
  #----------------------------------------------------------------------------

    #* NOTE: The only two function in this section that are _NOT_ affected
    #*       by the current locale are iswdigit() and iswxdigit(), so all the
    #*       others must be pure instead of const.

  # [7.25.2] Wide character classification utilities

  beam::function_attribute "$attr(pure_function)" -signatures \
      "__builtin_iswalnum" "__builtin_iswalpha" "__builtin_iswblank" \
      "__builtin_iswcntrl" "__builtin_iswgraph" "__builtin_iswlower" \
      "__builtin_iswprint" "__builtin_iswpunct" "__builtin_iswspace" \
      "__builtin_iswupper" "__builtin_iswctype"

  beam::function_attribute "$attr(const_function)" -signatures \
      "__builtin_iswdigit" "__builtin_iswxdigit"

  # No builtin for "wctype" available


  # [7.25.3] Wide character case mapping utilities

  beam::function_attribute "$attr(pure_function)" -signatures \
      "__builtin_towlower" "__builtin_towupper" "__builtin_towctrans"

  # No builtin for "wctrans" available


# POSIX standard
  beam::function_attribute "$attr(const_function)" -signatures \
     "__builtin_isascii" "__builtin_toascii"


# The following are GCC extensions or obsolete functions
  set gcc_extensions {
     __builtin_exp10 __builtin_exp10f __builtin_exp10l
     __builtin_pow10 __builtin_pow10f __builtin_pow10l
     __builtin_drem  __builtin_dremf  __builtin_dreml
     __builtin_gamma __builtin_gammaf __builtin_gammal
     __builtin_huge_val __builtin_huge_valf __builtin_huge_vall
     __builtin_inf   __builtin_inff   __builtin_infl
     __builtin_j0    __builtin_j0f    __builtin_j0l
     __builtin_j1    __builtin_j1f    __builtin_j1l
     __builtin_jn    __builtin_jnf    __builtin_jnl
     __builtin_nans  __builtin_nansf  __builtin_nansl
     __builtin_scalb __builtin_scalbf __builtin_scalbl

     __builtin_y0 __builtin_y0f __builtin_y0l
     __builtin_y1 __builtin_y1f __builtin_y1l
     __builtin_yn __builtin_ynf __builtin_ynl

     __builtin_powi  __builtin_powif  __builtin_powil
  }
  
  foreach extension $gcc_extensions {
    beam::function_attribute "$attr(pure_function)" -signatures $extension
  }

  beam::function_attribute $alloca_like -names "__builtin_alloca"

  beam::function_attribute {
      buffer ( buffer_index = 2,
               type = write,        
               units = elements,
               size = 1 ),
      buffer ( buffer_index = 3,
               type = write,        
               units = elements,
               size = 1 ),
      no_other_side_effects 
  } -signatures \
      "__builtin_sincos"   "__builtin_sincosf"  "__builtin_sincosl"

# Category: math builtins

  beam::function_attribute "pure,
                            buffer ( buffer_index = 3,
                                     type = write,
                                     units = elements,
                                     size = 1 ),
                            no_other_side_effects" -signatures \
      "__builtin_gamma_r"  "__builtin_gammaf_r"  "__builtin_gammal_r"  \
      "__builtin_lgamma_r" "__builtin_lgammaf_r" "__builtin_lgammal_r"

  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "__builtin_significand" "__builtin_significandf" "__builtin_significandl"

  beam::function_attribute "$attr(const_function)" -signatures      \
      "__builtin_infd32"     "__builtin_infd64"     "__builtin_infd128"  \
      "__builtin_lceil"      "__builtin_lceilf"     "__builtin_lceill"   \
      "__builtin_lfloor"     "__builtin_lfloorf"    "__builtin_lfloorl"  \
      "__builtin_llceil"     "__builtin_llceilf"    "__builtin_llceill"  \
      "__builtin_llfloor"    "__builtin_llfloorf"   "__builtin_llfloorl" \
      "__builtin_nand32"     "__builtin_nand64"     "__builtin_nand128"  \
      "__builtin_signbit"    "__builtin_signbitf"   "__builtin_signbitl" \
      "__builtin_signbitd32" "__builtin_signbitd64" "__builtin_signbitd128"

# Category: string/memory builtins

  beam::function_attribute "$attr(memcmp)"  -signatures "__builtin_bcmp"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type = read,
                                     size_index = 3 ),
                            buffer ( buffer_index = 2,
                                     type = write,
                                     size_index = 3 ),
                                     no_other_side_effects" -signatures \
    "__builtin_bcopy"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type = write,
                                     size_index = 2 ),
                                     no_other_side_effects" -signatures \
    "__builtin_bzero"

 beam::function_attribute "$attr(strchr)" -signatures \
     __builtin_index \
     __builtin_rindex

 beam::function_attribute "$attr(const_function)" -signatures \
     __builtin_ffs     \
     __builtin_ffsl    \
     __builtin_ffsll   \
     __builtin_ffsimax

# Category: stdio builtins

 beam::function_attribute "$attr(putc)" -signatures \
      "__builtin_putc" "__builtin_putc_unlocked"

 beam::function_attribute "$attr(fprintf)" -signatures \
     __builtin_fprintf_unlocked

 beam::function_attribute "$attr(printf)" -signatures \
     __builtin_printf_unlocked

 beam::function_attribute "$attr(fputc)"   -signatures __builtin_fputc_unlocked
 beam::function_attribute "$attr(fputs)"   -signatures __builtin_fputs_unlocked
 beam::function_attribute "$attr(puts)"    -signatures __builtin_puts_unlocked
 beam::function_attribute "$attr(fwrite)"  -signatures __builtin_fwrite_unlocked
 beam::function_attribute "$attr(putchar)" -signatures \
     __builtin_putchar_unlocked

# Category: object size checking builtins
 beam::function_attribute "$attr(pure_function)" -signatures \
      "__builtin_object_size"

# Category: miscellaneous builtins

 beam::function_attribute "$attr(exit)" -signatures __builtin__exit

 beam::function_attribute "$attr(const_function)" -signatures \
     __builtin_bswap32 \
     __builtin_bswap64 \
     __builtin_clz \
     __builtin_clzl \
     __builtin_clzll \
     __builtin_clzimax \
     __builtin_ctz \
     __builtin_ctzl \
     __builtin_ctzll \
     __builtin_ctzimax \
     __builtin_popcount \
     __builtin_popcountl \
     __builtin_popcountll \
     __builtin_popcountimax \
     __builtin_parity \
     __builtin_parityl \
     __builtin_parityll \
     __builtin_parityimax \
     __builtin_constant_p \
     __builtin_finite \
     __builtin_finitef \
     __builtin_finitel \
     __builtin_finited32 \
     __builtin_finited64 \
     __builtin_finited128 \
     __builtin_isfinite \
     __builtin_isinf \
     __builtin_isinff \
     __builtin_isinfl \
     __builtin_isinfd32 \
     __builtin_isinfd64 \
     __builtin_isinfd128 \
     __builtin_isnan \
     __builtin_isnanf \
     __builtin_isnanl \
     __builtin_isnand32 \
     __builtin_isnand64 \
     __builtin_isnand128 \
     __builtin_isnormal

  beam::function_attribute "$attr(pure_function)" -signatures "__builtin_extend_pointer"

   
  # NOTE: These were skipped intentionally and are included here simply
  # to document that fact for future readers of this file.
  #
  # __builtin___emutls_get_address
  # __builtin___emutls_register_common
  # __builtin_classify_type
  # __builtin_aggregate_incoming_address
  # __builtin_apply
  # __builtin_apply_args
  # __builtin_va_arg_pack
  # __builtin_va_arg_pack_len
  # __builtin_va_copy
  # __builtin_va_end
  # __builtin_va_start
  # __builtin_classify_type
  # __builtin_return
  # __builtin_return_address
  # __builtin_args_info
  # __builtin___clear_cache
  # __builtin_dcgettext
  # __builtin_dgettext
  # __builtin_dwarf_cfa
  # __builtin_dwarf_sp_column
  # __builtin_eh_return
  # __builtin_eh_return_data_regno
  # __builtin_extract_return_addr
  # __builtin_frame_address
  # __builtin_frob_return_addr
  # __builtin_gettext
  # __builtin_init_dwarf_reg_size_table
  # __builtin_next_arg
  # __builtin_prefetch
  # __builtin_saveregs
  # __builtin_stdarg_start
  # __builtin_trap
  # __builtin_unwind_init
  # __builtin_update_setjmp_buf


  #
  # NOTE: The return value of __builtin_expect, which is a long, is the
  # same as the first argument. We have no way to express that currently,
  # so there is a workaround in beam_configure.pl to define a macro for
  # __builtin_expect so that BEAM knows.
  #
  beam::function_attribute "$attr(pure_function)" -signatures __builtin_expect


  # Synchronization Primitives
  # NOTE: These need to be revisited. See Bug 2726.

  # __sync_fetch_and_add
  # __sync_fetch_and_add_1
  # __sync_fetch_and_add_2
  # __sync_fetch_and_add_4
  # __sync_fetch_and_add_8
  # __sync_fetch_and_add_16
  # __sync_fetch_and_sub
  # __sync_fetch_and_sub_1
  # __sync_fetch_and_sub_2
  # __sync_fetch_and_sub_4
  # __sync_fetch_and_sub_8
  # __sync_fetch_and_sub_16
  # __sync_fetch_and_or
  # __sync_fetch_and_or_1
  # __sync_fetch_and_or_2
  # __sync_fetch_and_or_4
  # __sync_fetch_and_or_8
  # __sync_fetch_and_or_16
  # __sync_fetch_and_and
  # __sync_fetch_and_and_1
  # __sync_fetch_and_and_2
  # __sync_fetch_and_and_4
  # __sync_fetch_and_and_8
  # __sync_fetch_and_and_16
  # __sync_fetch_and_xor
  # __sync_fetch_and_xor_1
  # __sync_fetch_and_xor_2
  # __sync_fetch_and_xor_4
  # __sync_fetch_and_xor_8
  # __sync_fetch_and_xor_16
  # __sync_fetch_and_nand
  # __sync_fetch_and_nand_1
  # __sync_fetch_and_nand_2
  # __sync_fetch_and_nand_4
  # __sync_fetch_and_nand_8
  # __sync_fetch_and_nand_16
  # __sync_add_and_fetch
  # __sync_add_and_fetch_1
  # __sync_add_and_fetch_2
  # __sync_add_and_fetch_4
  # __sync_add_and_fetch_8
  # __sync_add_and_fetch_16
  # __sync_sub_and_fetch
  # __sync_sub_and_fetch_1
  # __sync_sub_and_fetch_2
  # __sync_sub_and_fetch_4
  # __sync_sub_and_fetch_8
  # __sync_sub_and_fetch_16
  # __sync_or_and_fetch
  # __sync_or_and_fetch_1
  # __sync_or_and_fetch_2
  # __sync_or_and_fetch_4
  # __sync_or_and_fetch_8
  # __sync_or_and_fetch_16
  # __sync_and_and_fetch
  # __sync_and_and_fetch_1
  # __sync_and_and_fetch_2
  # __sync_and_and_fetch_4
  # __sync_and_and_fetch_8
  # __sync_and_and_fetch_16
  # __sync_xor_and_fetch
  # __sync_xor_and_fetch_1
  # __sync_xor_and_fetch_2
  # __sync_xor_and_fetch_4
  # __sync_xor_and_fetch_8
  # __sync_xor_and_fetch_16
  # __sync_nand_and_fetch
  # __sync_nand_and_fetch_1
  # __sync_nand_and_fetch_2
  # __sync_nand_and_fetch_4
  # __sync_nand_and_fetch_8
  # __sync_nand_and_fetch_16
  # __sync_bool_compare_and_swap
  # __sync_bool_compare_and_swap_1
  # __sync_bool_compare_and_swap_2
  # __sync_bool_compare_and_swap_4
  # __sync_bool_compare_and_swap_8
  # __sync_bool_compare_and_swap_16
  # __sync_val_compare_and_swap
  # __sync_val_compare_and_swap_1
  # __sync_val_compare_and_swap_2
  # __sync_val_compare_and_swap_4
  # __sync_val_compare_and_swap_8
  # __sync_val_compare_and_swap_16
  # __sync_lock_test_and_set
  # __sync_lock_test_and_set_1
  # __sync_lock_test_and_set_2
  # __sync_lock_test_and_set_4
  # __sync_lock_test_and_set_8
  # __sync_lock_test_and_set_16
  # __sync_lock_release
  # __sync_lock_release_1
  # __sync_lock_release_2
  # __sync_lock_release_4
  # __sync_lock_release_8
  # __sync_lock_release_16
  # __sync_synchronize

}
