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
#        Frank Wallingford, Florian Krohm, Nate Daly, Dan Brand
#
#    DESCRIPTION:
#
#        Attributes for functions defined by the C standard.
#
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        04/13/04  fwalling Renamed from beam_c_attributes.tcl, weeded down 
#                           to libc
#

namespace eval beam::attribute {

  #----------------------------------------------------------------------------
  # [7.2] Diagnostics <assert.h> 
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.3] Complex arithmetic <complex.h>
  #----------------------------------------------------------------------------

  # (see beam_attributes_libm.tcl)


  #----------------------------------------------------------------------------
  # [7.4] Character handling <ctype.h>
  #----------------------------------------------------------------------------

  # [7.4.1] Character classification functions

  beam::function_attribute "$attr(pure_function)" -signatures \
      "isalnum" "isalpha" "isblank" "iscntrl" "isgraph" \
      "islower" "isprint" "ispunct" "isspace" "isupper" 

  beam::function_attribute "$attr(const_function)" -signatures \
      "isdigit" "isxdigit"

  # [7.4.2] Character mapping functions

  beam::function_attribute "$attr(pure_function)" -signatures \
      "tolower" "toupper"


  #----------------------------------------------------------------------------
  # [7.5] Errors <errno.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.6] Floating-point environment <fenv.h>
  #----------------------------------------------------------------------------

  # [7.6.2] Floating-point exceptions

  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "feclearexcept"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type = write,
                                     size = 1,
                                     units = elements),
                            no_other_side_effects" -signatures \
      "fegetexceptflag"

  beam::function_attribute "$side_effect_on_environment,
                            buffer ( buffer_index = 1,
                                     type = read,
                                     size = 1,
                                     units = elements ),
                            no_other_side_effects" -signatures \
      "fesetexceptflag"
  
  beam::function_attribute "$attr(pure_function)" -signatures \
      "fetestexcept"

  # [7.6.3] Rounding

  beam::function_attribute "$attr(pure_function)" -signatures \
      "fegetround"

  beam::function_attribute "$side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "fesetround"

  # [7.6.4] Environment

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type  = write,
                                     size  = 1,
                                     units = elements ),
                            no_other_side_effects" -signatures \
      "fegetenv"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type  = write,
                                     size  = 1,
                                     units = elements ),
                            $side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "feholdexcept"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type  = read,
                                     size  = 1,
                                     units = elements ),
                            $side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "fesetenv"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     type  = read,
                                     size  = 1,
                                     units = elements  ),
                            $side_effect_on_environment,
                            no_other_side_effects" -signatures \
      "feupdateenv"


  #----------------------------------------------------------------------------
  # [7.7] Characteristics of floating types <float.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.8] Format conversion of interger types <inttypes.h>
  #----------------------------------------------------------------------------

  # [7.8.2] Functions for greatest-width integer types

  beam::function_attribute "$attr(imaxabs)" -signatures \
      "imaxabs"

  beam::function_attribute "const,
                            property ( index = 2,
                                       property_type = requires,
                                       type = input,
                                       test_type = not_equal,
                                       test_value = 0 ),
                            no_other_side_effects" -signatures \
      "imaxdiv"

  beam::function_attribute "$attr(strtol)" -signatures \
      "strtoimax" "strtoumax" "wcstoimax" "wcstoumax"


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

  # [7.11.1] Locale control

  beam::function_attribute "$attr(setlocale)" -signatures "setlocale"

  # [7.11.2] Numeric formatting convention inquiry

  beam::function_attribute "$attr(localeconv)" -signatures "localeconv"


  #----------------------------------------------------------------------------
  # [7.12] Mathematics <math.h>
  #----------------------------------------------------------------------------

  # (see beam_attributes_libm.tcl)


  #----------------------------------------------------------------------------
  # [7.13] Nonlocal jumps <setjmp.h>
  #----------------------------------------------------------------------------

  beam::function_attribute "$attr(setjmp)"  -signatures "setjmp"
  beam::function_attribute "$attr(setjmp)"  -signatures "_setjmp"
  beam::function_attribute "$attr(setjmp)"  -signatures "sigsetjmp"

  # [7.13.2] Restore calling environment

  beam::function_attribute "$attr(longjmp)"  -signatures "longjmp"
  beam::function_attribute "$attr(longjmp)"  -signatures "_longjmp"
  beam::function_attribute "$attr(longjmp)"  -signatures "siglongjmp"


  #----------------------------------------------------------------------------
  # [7.14] Signal handling <signal.h>
  #----------------------------------------------------------------------------

  beam::function_attribute "$attr(signal)" -signatures "signal"
  beam::function_attribute "$attr(raise)"  -signatures "raise"


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

  beam::function_attribute "$attr(remove)"  -signatures "remove"
  beam::function_attribute "$attr(rename)"  -signatures "rename"
  beam::function_attribute "$attr(tmpfile)" -signatures "tmpfile"
  beam::function_attribute "$attr(tmpnam)"  -signatures "tmpnam"

  beam::function_attribute {
    advisory ( explanation = "Function `tmpnam' is unsafe because of a potential race condition. Use `mkstemp' instead.",
               category    = "security.race" )
  } -signatures "tmpnam"


  # [7.19.5] File access functions

  beam::function_attribute "$attr(fclose)"  -signatures "fclose"
  beam::function_attribute "$attr(fflush)"  -signatures "fflush"
  beam::function_attribute "$attr(fopen)"   -signatures "fopen"
  beam::function_attribute "$attr(freopen)" -signatures "freopen"
  beam::function_attribute "$attr(setbuf)"  -signatures "setbuf" "setvbuf"


  # [7.19.6] Formatted input/output functions

  beam::function_attribute "$attr(fprintf)" -signatures "fprintf"
  beam::function_attribute "$attr(fscanf)"  -signatures "fscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `fscanf' is undefined when input does not match pattern. Use `fgetc' instead.",
      category    = "security.pattern"
    )
  } -signatures "fscanf"
  
  beam::function_attribute "$attr(printf)" -signatures "printf"
  beam::function_attribute "$attr(scanf)"  -signatures "scanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `scanf' is undefined when input does not match pattern. Use `fgetc' instead.",
      category    = "security.pattern"
    )
  } -signatures "scanf"
  
  beam::function_attribute "$attr(snprintf)" -signatures "snprintf"
  beam::function_attribute "$attr(sprintf)"  -signatures "sprintf"

  beam::function_attribute {
      advisory (explanation = "Function `sprintf' is unsafe because of a potential buffer overrun. Use `snprintf' instead.",
                category    = "security.buffer" 
                )
  } -signatures "sprintf"
  
  beam::function_attribute "$attr(sscanf)"    -signatures "sscanf"
  beam::function_attribute "$attr(vfprintf)"  -signatures "vfprintf"
  beam::function_attribute "$attr(vfscanf)"   -signatures "vfscanf"
  beam::function_attribute "$attr(vprintf)"   -signatures "vprintf"
  beam::function_attribute "$attr(vscanf)"    -signatures "vscanf"
  beam::function_attribute "$attr(vsnprintf)" -signatures "vsnprintf"
  beam::function_attribute "$attr(vsprintf)"  -signatures "vsprintf"

  beam::function_attribute {
    advisory ( explanation = "Function `vsprintf' is unsafe because of a potential buffer overrun. Use `vsnprintf' instead.",
               category    = "security.buffer" )
  } -signatures "vsprintf"

  beam::function_attribute "$attr(vsscanf)"  -signatures "vsscanf"


  # [7.19.7] Character input/output functions

  beam::function_attribute "$attr(fgetc)"   -signatures "fgetc"
  beam::function_attribute "$attr(fgets)"   -signatures "fgets"
  beam::function_attribute "$attr(fputc)"   -signatures "fputc"
  beam::function_attribute "$attr(fputs)"   -signatures "fputs"
  beam::function_attribute "$attr(getc)"    -signatures "getc"
  beam::function_attribute "$attr(getchar)" -signatures "getchar"
  beam::function_attribute "$attr(gets)"    -signatures "gets"

  beam::function_attribute {
    advisory ( explanation = "Function `gets' may overflow its argument. Use `fgets' instead.",
               category    = "security.buffer" )
  } -signatures "gets"

  beam::function_attribute "$attr(putc)"     -signatures "putc"
  beam::function_attribute "$attr(putchar)"  -signatures "putchar"
  beam::function_attribute "$attr(puts)"     -signatures "puts"
  beam::function_attribute "$attr(ungetc)"   -signatures "ungetc" "ungetwc"


  # [7.19.8] Direct input/output functions

  beam::function_attribute "$attr(fread)"  -signatures "fread"
  beam::function_attribute "$attr(fwrite)" -signatures "fwrite"


  # [7.19.9] File positioning functions
  
  beam::function_attribute "$attr(fgetpos)" -signatures "fgetpos"
  beam::function_attribute "$attr(fseek)"   -signatures "fseek"
  beam::function_attribute "$attr(rewind)"  -signatures "rewind"
  beam::function_attribute "$attr(fsetpos)" -signatures "fsetpos"
  beam::function_attribute "$attr(ftell)"   -signatures "ftell"


  # [7.19.10] Error-handling functions

  beam::function_attribute "$attr(clearerr)" -signatures "clearerr"
  beam::function_attribute "$attr(feof)"     -signatures "feof" "ferror"
  beam::function_attribute "$attr(perror)"   -signatures "perror"


  #----------------------------------------------------------------------------
  # [7.20] General utilities <stdlib.h>
  #----------------------------------------------------------------------------

  # [7.20.1] Numeric conversion functions

  beam::function_attribute "$attr(atoi)" -signatures \
      "atof" "atoi" "atol" "atoll"

  beam::function_attribute "$attr(strtod)" -signatures \
      "strtod" "strtof" "strtold"

  beam::function_attribute "$attr(strtol)" -signatures \
      "strtol" "strtoll" "strtoul" "strtoull"


  # [7.20.2] Pseudo-random sequence generation functions

  beam::function_attribute "$attr(rand)"  -signatures "rand"
  beam::function_attribute "$attr(srand)" -signatures "srand"

  beam::function_attribute {
    advisory ( explanation = "The rand/srand random number generator is not secure because it is predictable.\n An example of an exposure involves generating random file names.\n An attacker observing one file name can predict the next one.",
               category    = "security.random" )  
  } -signatures "rand" "srand"


  # [7.20.3] Memory management functions

  beam::function_attribute "$attr(calloc)"  -signatures "calloc"
  beam::function_attribute "$attr(free)"    -signatures "free"
  beam::function_attribute "$attr(malloc)"  -signatures "malloc"
  beam::function_attribute "$attr(realloc)" -signatures "realloc"


  # [7.20.4] Communication with the environment

  beam::function_attribute "$attr(abort)"  -signatures "abort"
  beam::function_attribute "$attr(atexit)" -signatures "atexit"
  beam::function_attribute "$attr(exit)"   -signatures "exit" "_Exit"
  beam::function_attribute "$attr(getenv)" -signatures "getenv"
  beam::function_attribute "$attr(system)" -signatures "system"


  # [7.20.5] Searching and sorting utilities

  beam::function_attribute "$attr(bsearch)" -signatures "bsearch"
  beam::function_attribute "$attr(qsort)"   -signatures "qsort"


  # [7.20.6] Integer arithmetic functions

  beam::function_attribute "$attr(abs)" -signatures "abs" "labs" "llabs"
  beam::function_attribute "$attr(div)" -signatures "div" "ldiv" "lldiv"


  # [7.20.7] Multibyte/wide character conversion functions

  beam::function_attribute "$attr(mblen)"  -signatures "mblen"
  beam::function_attribute "$attr(mbtowc)" -signatures "mbtowc"
  beam::function_attribute "$attr(wctomb)" -signatures "wctomb"


  # [7.20.8] Multibyte/wide string conversion functions

  beam::function_attribute "$attr(mbstowcs)" -signatures "mbstowcs" "wcstombs"


  #----------------------------------------------------------------------------
  # [7.21] String handling <string.h>
  #----------------------------------------------------------------------------

  # [7.21.2] Copying functions

  beam::function_attribute "$attr(memcpy)" -signatures "memcpy"  "memmove"
  beam::function_attribute "$attr(strcpy)" -signatures "strcpy"

  beam::function_attribute {
      advisory (explanation = "Function `strcpy' is unsafe because of a potential buffer overrun. Use `strncpy' instead.",
                category    = "security.buffer" 
                )
  } -signatures "strcpy"
  
  beam::function_attribute "$attr(strncpy)" -signatures "strncpy"


  # [7.21.3] Concatenation functions

  beam::function_attribute "$attr(strcat)" -signatures "strcat"

  beam::function_attribute {
      advisory (explanation = "Function `strcat' is unsafe because of a potential buffer overrun. Use `strncat' instead.",
                category    = "security.buffer" 
                )
  } -signatures "strcat"
  
  beam::function_attribute "$attr(strncat)" -signatures "strncat"

  # [7.21.4] Comparison functions

  beam::function_attribute "$attr(memcmp)"  -signatures "memcmp"
  beam::function_attribute "$attr(strcmp)"  -signatures "strcmp" "strcoll"
  beam::function_attribute "$attr(strncmp)" -signatures "strncmp"
  beam::function_attribute "$attr(strxfrm)" -signatures "strxfrm"

  # [7.21.5] Search functions

  beam::function_attribute "$attr(memchr)"  -signatures "memchr"
  beam::function_attribute "$attr(strchr)"  -signatures "strchr" "strrchr"
  beam::function_attribute "$attr(strcspn)" -signatures "strcspn" "strspn"
  beam::function_attribute "$attr(strpbrk)" -signatures "strpbrk" "strstr"
  beam::function_attribute "$attr(strtok)"  -signatures "strtok"

  # [7.21.6] Miscellaneous functions

  beam::function_attribute "$attr(memset)"   -signatures "memset"
  beam::function_attribute "$attr(strerror)" -signatures "strerror"
  beam::function_attribute "$attr(strlen)"   -names "strlen"


  #----------------------------------------------------------------------------
  # [7.22] Type-generic math <tgmath.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.23] Date and time <time.h>
  #----------------------------------------------------------------------------

  # [7.23.2] time manipulation functions

  beam::function_attribute "$attr(pure_function)"  -signatures "clock"
  beam::function_attribute "$attr(const_function)" -signatures "difftime"
  beam::function_attribute "$attr(mktime)"         -signatures "mktime"
  beam::function_attribute "$attr(time)"           -signatures "time"

  # [7.23.3] Time conversion functions

  beam::function_attribute "$attr(asctime)"  -signatures "asctime" "ctime"
  beam::function_attribute "$attr(gmtime)"   -signatures "gmtime" "localtime"
  beam::function_attribute "$attr(strftime)" -signatures "strftime"
  

  #----------------------------------------------------------------------------
  # [7.24] Extended multibyte and wide character utilities <wchar.h>
  #----------------------------------------------------------------------------

  # [7.24.2] Formatted wide character input/output functions

  beam::function_attribute "$attr(fprintf)" -signatures "fwprintf"
  beam::function_attribute "$attr(fscanf)"  -signatures "fwscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `fwscanf' is undefined when input does not match pattern. Use `fgetwc' instead.",
      category    = "security.pattern"
    )
  } -signatures "fwscanf"
  
  beam::function_attribute "$attr(snprintf)"  -signatures "swprintf"
  beam::function_attribute "$attr(sscanf)"    -signatures "swscanf"
  beam::function_attribute "$attr(vfprintf)"  -signatures "vfwprintf"
  beam::function_attribute "$attr(vfscanf)"   -signatures "vfwscanf"
  beam::function_attribute "$attr(vsnprintf)" -signatures "vswprintf"
  beam::function_attribute "$attr(vsscanf)"   -signatures "vswscanf"
  beam::function_attribute "$attr(vprintf)"   -signatures "vwprintf"
  beam::function_attribute "$attr(vscanf)"    -signatures "vwscanf"
  beam::function_attribute "$attr(printf)"    -signatures "wprintf"
  beam::function_attribute "$attr(scanf)"     -signatures "wscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `wscanf' is undefined when input does not match pattern. Use `fgetwc' instead.",
      category    = "security.pattern"
    )
  } -signatures "wscanf"
  


  # [7.24.3] Wide character input/output functions

  beam::function_attribute "$attr(fgetc)"   -signatures "fgetwc"
  beam::function_attribute "$attr(fgets)"   -signatures "fgetws"
  beam::function_attribute "$attr(fputc)"   -signatures "fputwc"
  beam::function_attribute "$attr(fputs)"   -signatures "fputws"
  beam::function_attribute "$attr(fwide)"   -signatures "fwide"
  beam::function_attribute "$attr(getc)"    -signatures "getwc"
  beam::function_attribute "$attr(getchar)" -signatures "getwchar"
  beam::function_attribute "$attr(putc)"    -signatures "putwc"
  beam::function_attribute "$attr(putchar)" -signatures "putwchar"
  beam::function_attribute "$attr(ungetc)"  -signatures "ungetwc"


  # [7.24.4] General wide string utilities

  beam::function_attribute "$attr(strtod)" -signatures \
      "wcstod" "wcstof" "wcstold"

  beam::function_attribute "$attr(strtol)" -signatures \
      "wcstol" "wcstoll" "wcstoul" "wcstoull"

  beam::function_attribute "$attr(strcpy)" -signatures "wcscpy"
  beam::function_attribute {
      advisory (explanation = "Function `wcscpy' is unsafe because of a potential buffer overrun. Use `wcsncpy' instead.",
                category    = "security.buffer" 
                )
  } -signatures "wcscpy"
  
  beam::function_attribute "$attr(strncpy)" -signatures "wcsncpy"

  beam::function_attribute "buffer ( buffer_index = 2,
                                     units = elements,
                                     type = read,
                                     size_index = 3 ),
                            buffer ( buffer_index = 1,
                                     units = elements,
                                     type = write,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects" -signatures \
     "wmemcpy" "wmemmove"

  beam::function_attribute "$attr(strcat)" -signatures "wcscat"
  beam::function_attribute {
      advisory (explanation = "Function `wcscat' is unsafe because of a potential buffer overrun. Use `wcsncat' instead.",
                category    = "security.buffer" 
                )
  } -signatures "wcscat"
  
  beam::function_attribute "$attr(strncat)" -signatures "wcsncat"
  beam::function_attribute "$attr(strcmp)"  -signatures "wcscmp" "wcscoll"
  beam::function_attribute "$attr(strncmp)" -signatures "wcsncmp"
  beam::function_attribute "$attr(strxfrm)" -signatures "wcsxfrm"

  beam::function_attribute "pure,
                            buffer ( buffer_index = 1,
                                     units = elements,
                                     type = read, 
                                     size_index = 3 ),
                            buffer ( buffer_index = 2,
                                     units = elements,
                                     type = read,
                                     size_index = 3 ),
                            no_other_side_effects" -signatures "wmemcmp"

  beam::function_attribute "$attr(strchr)"  -signatures "wcschr" "wcsrchr"
  beam::function_attribute "$attr(strcspn)" -signatures "wcscspn" "wcsspn"
  beam::function_attribute "$attr(strpbrk)" -signatures "wcspbrk" "wcsstr"
  beam::function_attribute "$attr(strtok)"  -signatures "wcstok"

  beam::function_attribute "pure,
                            buffer ( buffer_index = 1,
                                     units = elements,
                                     type = read,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects" -signatures "wmemchr"

  beam::function_attribute "$attr(strlen)"  -signatures "wcslen"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     units = elements,
                                     type = write,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects" -signatures "wmemset"


  # [7.24.5] Wide character time conversion functions

  beam::function_attribute "$attr(strftime)" -signatures "wcsftime"


  # [7.24.6] Extended multibyte/wide character conversion utilities

  beam::function_attribute "$attr(pure_function)" -signatures "btowc" "wctob"

  beam::function_attribute "$attr(mbsinit)" -signatures "mbsinit"
  beam::function_attribute "$attr(mbrlen)"  -signatures "mbrlen"
  beam::function_attribute "$attr(mbrtowc)" -signatures "mbrtowc"
  beam::function_attribute "$attr(wcrtomb)" -signatures "wcrtomb"

  #  "mbsrtowcs" "wcsrtombs"  are too complicated


  #----------------------------------------------------------------------------
  # [7.25] Wide character classification and mapping utilities <wctype.h>
  #----------------------------------------------------------------------------

    #* NOTE: The only two function in this section that are _NOT_ affected
    #*       by the current locale are iswdigit() and iswxdigit(), so all the
    #*       others must be pure instead of const.

  # [7.25.2] Wide character classification utilities

  beam::function_attribute "$attr(pure_function)" -signatures \
      "iswalnum" "iswalpha" "iswblank" "iswcntrl"  \
      "iswgraph" "iswlower" "iswprint"  \
      "iswpunct" "iswspace" "iswupper" "iswctype"

  beam::function_attribute "$attr(const_function)" -signatures \
      "iswdigit" "iswxdigit"

  beam::function_attribute "$attr(wctype)" -signatures "wctype"

  # [7.25.3] Wide character case mapping utilities

  beam::function_attribute "$attr(pure_function)" -signatures \
      "towlower" "towupper" "towctrans"

  beam::function_attribute "$attr(wctrans)" -signatures "wctrans"

}
