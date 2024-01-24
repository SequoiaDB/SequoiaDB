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
#        Frank Wallingford, Florian Krohm, Nate Daly
#
#    DESCRIPTION:
#
#        Attributes that relate to libm.
#        
#        
#    MODIFICATIONS:
#
#        Date      UserID   Remark (newest to oldest)
#        --------  -------  ---------------------------------------------------
#        04/13/04  fwalling Broken out from beam_c_attributes.tcl
#

namespace eval beam::attribute {

  #----------------------------------------------------------------------------
  # [7.3] Complex arithmetic <complex.h>
  #----------------------------------------------------------------------------

  # [7.3.5] Trigonometric functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "cacos" "cacosf" "cacosl" \
      "casin" "casinf" "casinl" \
      "catan" "catanf" "catanl" \
      "ccos"  "ccosf"  "ccosl"  \
      "csin"  "csinf"  "csinl"  \
      "ctan"  "ctanf"  "ctanl"

  # [7.3.6] Hyperbolic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "cacosh" "cacoshf" "cacoshl" \
      "casinh" "casinhf" "casinhl" \
      "catanh" "catanhf" "catanhl" \
      "ccosh"  "ccoshf"  "ccoshl"  \
      "csinh"  "csinhf"  "csinhl"  \
      "ctanh"  "ctanhf"  "ctanhl"           

  # [7.3.7] Exponential and logarithmic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "cexp"  "cexpf"  "cexpl" \
      "clog"  "clogf"  "clogl"

  # [7.3.8] Power and absolute-value functions
      
  beam::function_attribute "$attr(const_function)" -signatures \
      "cabs"  "cabsf"  "cabsl" \
      "cpow"  "cpowf"  "cpowl" \
      "csqrt" "csqrtf" "csqrtl"

  # [7.3.9] Manipulation functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "carg"  "cargf"  "cargl"  \
      "cimag" "cimagf" "cimagl" \
      "conj"  "conjf"  "conjl"  \
      "conj"  "conjf"  "conjl"  \
      "cproj" "cprojf" "cprojl" \
      "creal" "crealf" "creall"

  #----------------------------------------------------------------------------
  # [7.12] Mathematics <math.h>
  #----------------------------------------------------------------------------

  # [7.12.4] Trigonometric functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "acos"  "acosf"  "acosl"  \
      "asin"  "asinf"  "asinl"  \
      "atan"  "atanf"  "atanl"  \
      "atan2" "atan2f" "atan2l" \
      "cos"   "cosf"   "cosl"   \
      "sin"   "sinf"   "sinl"   \
      "tan"   "tanf"   "tanl"  

  # [7.12.5] Hyperbolic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "acosh" "acoshf" "acoshl" \
      "asinh" "asinhf" "asinhl" \
      "atanh" "atanhf" "atanhl" \
      "cosh"  "coshf"  "coshl"  \
      "sinh"  "sinhf"  "sinhl"  \
      "tanh"  "tanhf"  "tanhl"

  # [7.12.6] Exponential and logarithmic functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "exp"     "expf"     "expl"    \
      "exp2"    "exp2f"    "exp2l"   \
      "expm1"   "expm1f"   "expm1l"  \
      "ilogb"   "ilogbf"   "ilogbl"  \
      "ldexp"   "ldexpf"   "ldexpl"  \
      "log"     "logf"     "logl"    \
      "log10"   "log10f"   "log10l"  \
      "log1p"   "log1pf"   "log1pl"  \
      "log2"    "log2f"    "log2l"   \
      "logb"    "logbf"    "logbl"   \
      "scalbn"  "scalbnf"  "scalbnl" \
      "scalbln" "scalblnf" "scalblnl"

  beam::function_attribute "$attr(frexp)" -signatures \
      "frexp" "frexpf" "frexpl"

  beam::function_attribute "$attr(modf)" -signatures \
      "modf"  "modff"  "modfl"
  
  # [7.12.7] Power and absolute-value functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "cbrt"  "cbrtf"  "cbrtl"  \
      "fabs"  "fabsf"  "fabsl"  \
      "hypot" "hypotf" "hypotl" \
      "pow"   "powf"   "powl"   \
      "sqrt"  "sqrtf"  "sqrtl"

  # [7.12.8] Error and gamma functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "erf"    "erff"    "erfl"    \
      "erfc"   "erfcf"   "erfcl"   \
      "lgamma" "lgammaf" "lgammal" \
      "tgamma" "tgammaf" "tgammal" 

  # [7.12.9] Nearest integer functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "ceil"      "ceilf"      "ceill"      \
      "floor"     "floorf"     "floorl"     \
      "nearbyint" "nearbyintf" "nearbyintl" \
      "rint"      "rintf"      "rintl"      \
      "lrint"     "lrintf"     "lrintl"     \
      "llrint"    "llrintf"    "llrintl"    \
      "round"     "roundf"     "roundl"     \
      "lround"    "lroundf"    "lroundl"    \
      "llround"   "llroundf"   "llroundl"   \
      "trunc"     "truncf"     "truncl"

  # [7.12.10] Remainder functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "fmod"      "fmodf"      "fmodl"      \
      "remainder" "remainderf" "remainderl" 

  beam::function_attribute "$attr(remquo)" -signatures \
      "remquo" "remquof" "remquol"

  # [7.12.11] Manipulation functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "copysign"   "copysignf"   "copysignl"   \
      "nextafter"  "nextafterf"  "nextafterl"  \
      "nexttoward" "nexttowardf" "nexttowardl" 

  beam::function_attribute "$attr(nan)" -signatures \
      "nan" "nanf" "nanl"

  # [7.12.12] Maximum, minimum, and positive difference functions

  beam::function_attribute "$attr(const_function)" -signatures \
      "fdim" "fdimf" "fdiml" \
      "fmax" "fmaxl" "fmaxl" "fmaxf" \
      "fmin" "fminl" "fminl" "fminf"

  # [7.12.13] Floating multiply-add

  beam::function_attribute "$attr(const_function)" -signatures \
      "fma" "fmaf" "fmal"

  # [7.12.14] Comparison macros (this section is all macros)
}
