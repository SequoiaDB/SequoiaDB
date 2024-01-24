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
#        Attributes for functions from the C library in the std:: namespace
#
#        See 17.4.1.2 in ISO/IEC 14882:1998(E) aka C++ standard
#        Because it is unspecified whether these functions have "C" or 
#        "C++" linkage (see 17.4.2.2) we use -names instead of -signatures
#        throughout.
#

namespace eval beam::attribute {

  #----------------------------------------------------------------------------
  # [7.2] Diagnostics <assert.h> 
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.3] Complex arithmetic <complex.h>
  #----------------------------------------------------------------------------

  # not included


  #----------------------------------------------------------------------------
  # [7.4] Character handling <ctype.h>
  #----------------------------------------------------------------------------

  # [7.4.1] Character classification functions

  beam::function_attribute "$attr(pure_function)" -names \
      "std::isalnum" "std::isalpha" "std::isblank" "std::iscntrl" "std::isgraph" \
      "std::islower" "std::isprint" "std::ispunct" "std::isspace" "std::isupper" 

  beam::function_attribute "$attr(const_function)" -names \
      "std::isdigit" "std::isxdigit"

  # [7.4.2] Character mapping functions

  beam::function_attribute "$attr(pure_function)" -names \
      "std::tolower" "std::toupper"


  #----------------------------------------------------------------------------
  # [7.5] Errors <errno.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.6] Floating-point environment <fenv.h>
  #----------------------------------------------------------------------------

  # not included

  #----------------------------------------------------------------------------
  # [7.7] Characteristics of floating types <float.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.8] Format conversion of interger types <inttypes.h>
  #----------------------------------------------------------------------------

  # not included


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

  beam::function_attribute "$attr(setlocale)" -names "std::setlocale"

  # [7.11.2] Numeric formatting convention inquiry

  beam::function_attribute "$attr(localeconv)" -names "std::localeconv"


  #----------------------------------------------------------------------------
  # [7.12] Mathematics <math.h>
  #----------------------------------------------------------------------------

  # (see beam_attributes_libm.tcl)


  #----------------------------------------------------------------------------
  # [7.13] Nonlocal jumps <setjmp.h>
  #----------------------------------------------------------------------------

  # [7.13.2] Restore calling environment

  beam::function_attribute "$attr(longjmp)"  -names "std::longjmp"


  #----------------------------------------------------------------------------
  # [7.14] Signal handling <signal.h>
  #----------------------------------------------------------------------------

  beam::function_attribute "$attr(signal)" -names "std::signal"
  beam::function_attribute "$attr(raise)"  -names "std::raise"


  #----------------------------------------------------------------------------
  # [7.15] Variable arguments <stdarg.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.16] Boolean types and values <stdbool.h>
  #----------------------------------------------------------------------------

  # not included


  #----------------------------------------------------------------------------
  # [7.17] Common definitions <stddef.h>
  #----------------------------------------------------------------------------
  
  # (no functions)


  #----------------------------------------------------------------------------
  # [7.18] Integer types <stdint.h>
  #----------------------------------------------------------------------------

  # not included


  #----------------------------------------------------------------------------
  # [7.19] Input/output <stdio.h>
  #----------------------------------------------------------------------------
  
  # [7.19.4] Operations on files

  beam::function_attribute "$attr(remove)"  -names "std::remove"
  beam::function_attribute "$attr(rename)"  -names "std::rename"
  beam::function_attribute "$attr(tmpfile)" -names "std::tmpfile"
  beam::function_attribute "$attr(tmpnam)"  -names "std::tmpnam"

  beam::function_attribute {
    advisory ( explanation = "Function `std::tmpnam' is unsafe because of a potential race condition. Use `std::mkstemp' instead.",
               category    = "security.race" )
  } -names "std::tmpnam"


  # [7.19.5] File access functions

  beam::function_attribute "$attr(fclose)"  -names "std::fclose"
  beam::function_attribute "$attr(fflush)"  -names "std::fflush"
  beam::function_attribute "$attr(fopen)"   -names "std::fopen"
  beam::function_attribute "$attr(freopen)" -names "std::freopen"
  beam::function_attribute "$attr(setbuf)"  -names "std::setbuf" "std::setvbuf"


  # [7.19.6] Formatted input/output functions

  beam::function_attribute "$attr(fprintf)" -names "std::fprintf"
  beam::function_attribute "$attr(fscanf)"  -names "std::fscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `std::fscanf' is undefined when input does not match pattern. Use `std::fgetc' instead.",
      category    = "security.pattern"
    )
  } -names "std::fscanf"
  
  beam::function_attribute "$attr(printf)" -names "std::printf"
  beam::function_attribute "$attr(scanf)"  -names "std::scanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `std::scanf' is undefined when input does not match pattern. Use `std::fgetc' instead.",
      category    = "security.pattern"
    )
  } -names "std::scanf"
  
  beam::function_attribute "$attr(snprintf)" -names "std::snprintf"
  beam::function_attribute "$attr(sprintf)"  -names "std::sprintf"

  beam::function_attribute {
      advisory (explanation = "Function `std::sprintf' is unsafe because of a potential buffer overrun. Use `std::snprintf' instead.",
                category    = "security.buffer" 
                )
  } -names "std::sprintf"
  
  beam::function_attribute "$attr(sscanf)"    -names "std::sscanf"
  beam::function_attribute "$attr(vfprintf)"  -names "std::vfprintf"
  beam::function_attribute "$attr(vfscanf)"   -names "std::vfscanf"
  beam::function_attribute "$attr(vprintf)"   -names "std::vprintf"
  beam::function_attribute "$attr(vscanf)"    -names "std::vscanf"
  beam::function_attribute "$attr(vsnprintf)" -names "std::vsnprintf"
  beam::function_attribute "$attr(vsprintf)"  -names "std::vsprintf"

  beam::function_attribute {
    advisory ( explanation = "Function `std::vsprintf' is unsafe because of a potential buffer overrun. Use `std::vsnprintf' instead.",
               category    = "security.buffer" )
  } -names "std::vsprintf"

  beam::function_attribute "$attr(vsscanf)"  -names "std::vsscanf"


  # [7.19.7] Character input/output functions

  beam::function_attribute "$attr(fgetc)"   -names "std::fgetc"
  beam::function_attribute "$attr(fgets)"   -names "std::fgets"
  beam::function_attribute "$attr(fputc)"   -names "std::fputc"
  beam::function_attribute "$attr(fputs)"   -names "std::fputs"
  beam::function_attribute "$attr(getc)"    -names "std::getc"
  beam::function_attribute "$attr(getchar)" -names "std::getchar"
  beam::function_attribute "$attr(gets)"    -names "std::gets"

  beam::function_attribute {
    advisory ( explanation = "Function `std::gets' may overflow its argument. Use `std::fgets' instead.",
               category    = "security.buffer" )
  } -names "std::gets"

  beam::function_attribute "$attr(putc)"     -names "std::putc"
  beam::function_attribute "$attr(putchar)"  -names "std::putchar"
  beam::function_attribute "$attr(puts)"     -names "std::puts"
  beam::function_attribute "$attr(ungetc)"   -names "std::ungetc" "std::ungetwc"


  # [7.19.8] Direct input/output functions

  beam::function_attribute "$attr(fread)"  -names "std::fread"
  beam::function_attribute "$attr(fwrite)" -names "std::fwrite"


  # [7.19.9] File positioning functions
  
  beam::function_attribute "$attr(fgetpos)" -names "std::fgetpos"
  beam::function_attribute "$attr(fseek)"   -names "std::fseek"
  beam::function_attribute "$attr(rewind)"  -names "std::rewind"
  beam::function_attribute "$attr(fsetpos)" -names "std::fsetpos"
  beam::function_attribute "$attr(ftell)"   -names "std::ftell"


  # [7.19.10] Error-handling functions

  beam::function_attribute "$attr(clearerr)" -names "std::clearerr"
  beam::function_attribute "$attr(feof)"     -names "std::feof" "std::ferror"
  beam::function_attribute "$attr(perror)"   -names "std::perror"


  #----------------------------------------------------------------------------
  # [7.20] General utilities <stdlib.h>
  #----------------------------------------------------------------------------

  # [7.20.1] Numeric conversion functions

  beam::function_attribute "$attr(atoi)" -names \
      "std::atof" "std::atoi" "std::atol" "std::atoll"

  beam::function_attribute "$attr(strtod)" -names \
      "std::strtod" "std::strtof" "std::strtold"

  beam::function_attribute "$attr(strtol)" -names \
      "std::strtol" "std::strtoll" "std::strtoul" "std::strtoull"


  # [7.20.2] Pseudo-random sequence generation functions

  beam::function_attribute "$attr(rand)"  -names "std::rand"
  beam::function_attribute "$attr(srand)" -names "std::srand"

  beam::function_attribute {
    advisory ( explanation = "The std::rand/std::srand random number generator is not secure because it is predictable.\n An example of an exposure involves generating random file names.\n An attacker observing one file name can predict the next one.",
               category    = "security.random" )  
  } -names "std::rand" "std::srand"


  # [7.20.3] Memory management functions

  beam::function_attribute "$attr(calloc)"  -names "std::calloc"
  beam::function_attribute "$attr(free)"    -names "std::free"
  beam::function_attribute "$attr(malloc)"  -names "std::malloc"
  beam::function_attribute "$attr(realloc)" -names "std::realloc"


  # [7.20.4] Communication with the environment

  beam::function_attribute "$attr(abort)"  -names "std::abort"
  beam::function_attribute "$attr(atexit)" -names "std::atexit"
  beam::function_attribute "$attr(exit)"   -names "std::exit" "std::_Exit"
  beam::function_attribute "$attr(getenv)" -names "std::getenv"
  beam::function_attribute "$attr(system)" -names "std::system"


  # [7.20.5] Searching and sorting utilities

  beam::function_attribute "$attr(bsearch)" -names "std::bsearch"
  beam::function_attribute "$attr(qsort)"   -names "std::qsort"


  # [7.20.6] Integer arithmetic functions

  beam::function_attribute "$attr(abs)" -names "std::abs" "std::labs" "std::llabs"
  beam::function_attribute "$attr(div)" -names "std::div" "std::ldiv" "std::lldiv"


  # [7.20.7] Multibyte/wide character conversion functions

  beam::function_attribute "$attr(mblen)"  -names "std::mblen"
  beam::function_attribute "$attr(mbtowc)" -names "std::mbtowc"
  beam::function_attribute "$attr(wctomb)" -names "std::wctomb"


  # [7.20.8] Multibyte/wide string conversion functions

  beam::function_attribute "$attr(mbstowcs)" -names "std::mbstowcs" "std::wcstombs"


  #----------------------------------------------------------------------------
  # [7.21] String handling <string.h>
  #----------------------------------------------------------------------------

  # [7.21.2] Copying functions

  beam::function_attribute "$attr(memcpy)" -names "std::memcpy"  "std::memmove"
  beam::function_attribute "$attr(strcpy)" -names "std::strcpy"

  beam::function_attribute {
      advisory (explanation = "Function `std::strcpy' is unsafe because of a potential buffer overrun. Use `std::strncpy' instead.",
                category    = "security.buffer" 
                )
  } -names "std::strcpy"
  
  beam::function_attribute "$attr(strncpy)" -names "std::strncpy"


  # [7.21.3] Concatenation functions

  beam::function_attribute "$attr(strcat)" -names "std::strcat"

  beam::function_attribute {
      advisory (explanation = "Function `std::strcat' is unsafe because of a potential buffer overrun. Use `std::strncat' instead.",
                category    = "security.buffer" 
                )
  } -names "std::strcat"
  
  beam::function_attribute "$attr(strncat)" -names "std::strncat"

  # [7.21.4] Comparison functions

  beam::function_attribute "$attr(memcmp)"  -names "std::memcmp"
  beam::function_attribute "$attr(strcmp)"  -names "std::strcmp" "std::strcoll"
  beam::function_attribute "$attr(strncmp)" -names "std::strncmp"
  beam::function_attribute "$attr(strxfrm)" -names "std::strxfrm"

  # [7.21.5] Search functions

  beam::function_attribute "$attr(memchr)"  -names "std::memchr"
  beam::function_attribute "$attr(strchr)"  -names "std::strchr" "std::strrchr"
  beam::function_attribute "$attr(strcspn)" -names "std::strcspn" "std::strspn"
  beam::function_attribute "$attr(strpbrk)" -names "std::strpbrk" "std::strstr"
  beam::function_attribute "$attr(strtok)"  -names "std::strtok"

  # [7.21.6] Miscellaneous functions

  beam::function_attribute "$attr(memset)"   -names "std::memset"
  beam::function_attribute "$attr(strerror)" -names "std::strerror"
  beam::function_attribute "$attr(strlen)"   -names "std::strlen"


  #----------------------------------------------------------------------------
  # [7.22] Type-generic math <tgmath.h>
  #----------------------------------------------------------------------------

  # (no functions)


  #----------------------------------------------------------------------------
  # [7.23] Date and time <time.h>
  #----------------------------------------------------------------------------

  # [7.23.2] time manipulation functions

  beam::function_attribute "$attr(pure_function)"  -names "std::clock"
  beam::function_attribute "$attr(const_function)" -names "std::difftime"
  beam::function_attribute "$attr(mktime)"         -names "std::mktime"
  beam::function_attribute "$attr(time)"           -names "std::time"

  # [7.23.3] Time conversion functions

  beam::function_attribute "$attr(asctime)"  -names "std::asctime" "std::ctime"
  beam::function_attribute "$attr(gmtime)"   -names "std::gmtime" "std::localtime"
  beam::function_attribute "$attr(strftime)" -names "std::strftime"
  

  #----------------------------------------------------------------------------
  # [7.24] Extended multibyte and wide character utilities <wchar.h>
  #----------------------------------------------------------------------------

  # [7.24.2] Formatted wide character input/output functions

  beam::function_attribute "$attr(fprintf)" -names "std::fwprintf"
  beam::function_attribute "$attr(fscanf)"  -names "std::fwscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `std::fwscanf' is undefined when input does not match pattern. Use `std::fgetwc' instead.",
      category    = "security.pattern"
    )
  } -names "std::fwscanf"
  
  beam::function_attribute "$attr(snprintf)"  -names "std::swprintf"
  beam::function_attribute "$attr(sscanf)"    -names "std::swscanf"
  beam::function_attribute "$attr(vfprintf)"  -names "std::vfwprintf"
  beam::function_attribute "$attr(vfscanf)"   -names "std::vfwscanf"
  beam::function_attribute "$attr(vsnprintf)" -names "std::vswprintf"
  beam::function_attribute "$attr(vsscanf)"   -names "std::vswscanf"
  beam::function_attribute "$attr(vprintf)"   -names "std::vwprintf"
  beam::function_attribute "$attr(vscanf)"    -names "std::vwscanf"
  beam::function_attribute "$attr(printf)"    -names "std::wprintf"
  beam::function_attribute "$attr(scanf)"     -names "std::wscanf"

  beam::function_attribute {
    advisory (
      explanation = "Behavior of `std::wscanf' is undefined when input does not match pattern. Use `std::fgetwc' instead.",
      category    = "security.pattern"
    )
  } -names "std::wscanf"
  


  # [7.24.3] Wide character input/output functions

  beam::function_attribute "$attr(fgetc)"   -names "std::fgetwc"
  beam::function_attribute "$attr(fgets)"   -names "std::fgetws"
  beam::function_attribute "$attr(fputc)"   -names "std::fputwc"
  beam::function_attribute "$attr(fputs)"   -names "std::fputws"
  beam::function_attribute "$attr(fwide)"   -names "std::fwide"
  beam::function_attribute "$attr(getc)"    -names "std::getwc"
  beam::function_attribute "$attr(getchar)" -names "std::getwchar"
  beam::function_attribute "$attr(putc)"    -names "std::putwc"
  beam::function_attribute "$attr(putchar)" -names "std::putwchar"
  beam::function_attribute "$attr(ungetc)"  -names "std::ungetwc"


  # [7.24.4] General wide string utilities

  beam::function_attribute "$attr(strtod)" -names \
      "std::wcstod" "std::wcstof" "std::wcstold"

  beam::function_attribute "$attr(strtol)" -names \
      "std::wcstol" "std::wcstoll" "std::wcstoul" "std::wcstoull"

  beam::function_attribute "$attr(strcpy)" -names "std::wcscpy"
  beam::function_attribute {
      advisory (explanation = "Function `std::wcscpy' is unsafe because of a potential buffer overrun. Use `std::wcsncpy' instead.",
                category    = "security.buffer" 
                )
  } -names "std::wcscpy"
  
  beam::function_attribute "$attr(strncpy)" -names "std::wcsncpy"

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
     "std::wmemcpy" "std::wmemmove"

  beam::function_attribute "$attr(strcat)" -names "std::wcscat"
  beam::function_attribute {
      advisory (explanation = "Function `std::wcscat' is unsafe because of a potential buffer overrun. Use `std::wcsncat' instead.",
                category    = "security.buffer" 
                )
  } -names "std::wcscat"
  
  beam::function_attribute "$attr(strncat)" -names "std::wcsncat"
  beam::function_attribute "$attr(strcmp)"  -names "std::wcscmp" "std::wcscoll"
  beam::function_attribute "$attr(strncmp)" -names "std::wcsncmp"
  beam::function_attribute "$attr(strxfrm)" -names "std::wcsxfrm"

  beam::function_attribute "pure,
                            buffer ( buffer_index = 1,
                                     units = elements,
                                     type = read, 
                                     size_index = 3 ),
                            buffer ( buffer_index = 2,
                                     units = elements,
                                     type = read,
                                     size_index = 3 ),
                            no_other_side_effects" -signatures "std::wmemcmp"

  beam::function_attribute "$attr(strchr)"  -names "std::wcschr" "std::wcsrchr"
  beam::function_attribute "$attr(strcspn)" -names "std::wcscspn" "std::wcsspn"
  beam::function_attribute "$attr(strpbrk)" -names "std::wcspbrk" "std::wcsstr"
  beam::function_attribute "$attr(strtok)"  -names "std::wcstok"

  beam::function_attribute "pure,
                            buffer ( buffer_index = 1,
                                     units = elements,
                                     type = read,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = may  ),
                            no_other_side_effects" -signatures "std::wmemchr"

  beam::function_attribute "$attr(strlen)"  -names "std::wcslen"

  beam::function_attribute "buffer ( buffer_index = 1,
                                     units = elements,
                                     type = write,
                                     size_index = 3 ),
                    return_overlap ( return_index = return,
                                     points_into_index = 1,
                                     fate = must ),
                            no_other_side_effects" -signatures "std::wmemset"


  # [7.24.5] Wide character time conversion functions

  beam::function_attribute "$attr(strftime)" -names "std::wcsftime"


  # [7.24.6] Extended multibyte/wide character conversion utilities

  beam::function_attribute "$attr(pure_function)" -names "std::btowc" "std::wctob"

  beam::function_attribute "$attr(mbsinit)" -names "std::mbsinit"
  beam::function_attribute "$attr(mbrlen)"  -names "std::mbrlen"
  beam::function_attribute "$attr(mbrtowc)" -names "std::mbrtowc"
  beam::function_attribute "$attr(wcrtomb)" -names "std::wcrtomb"

  #  "mbsrtowcs" "wcsrtombs"  are too complicated


  #----------------------------------------------------------------------------
  # [7.25] Wide character classification and mapping utilities <wctype.h>
  #----------------------------------------------------------------------------

    #* NOTE: The only two function in this section that are _NOT_ affected
    #*       by the current locale are iswdigit() and iswxdigit(), so all the
    #*       others must be pure instead of const.

  # [7.25.2] Wide character classification utilities

  beam::function_attribute "$attr(pure_function)" -names \
      "std::iswalnum" "std::iswalpha" "std::iswblank" "std::iswcntrl"  \
      "std::iswgraph" "std::iswlower" "std::iswprint"  \
      "std::iswpunct" "std::iswspace" "std::iswupper" "std::iswctype"

  beam::function_attribute "$attr(const_function)" -names \
      "std::iswdigit" "std::iswxdigit"

  beam::function_attribute "$attr(wctype)" -names "std::wctype"

  # [7.25.3] Wide character case mapping utilities

  beam::function_attribute "$attr(pure_function)" -names \
      "std::towlower" "std::towupper" "std::towctrans"

  beam::function_attribute "$attr(wctrans)" -names "std::wctrans"

}
