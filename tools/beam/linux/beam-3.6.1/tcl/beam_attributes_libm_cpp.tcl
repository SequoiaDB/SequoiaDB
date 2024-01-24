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
#        Attributes for functions from libm in the std:: namespace
#        
#        

namespace eval beam::attribute {

  #----------------------------------------------------------------------------
  # [7.3] Complex arithmetic <complex.h>
  #----------------------------------------------------------------------------

  # not provided

  #----------------------------------------------------------------------------
  # [7.12] Mathematics <math.h>
  #----------------------------------------------------------------------------

  # [7.12.4] Trigonometric functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::acos"  "std::acosf"  "std::acosl"  \
      "std::asin"  "std::asinf"  "std::asinl"  \
      "std::atan"  "std::atanf"  "std::atanl"  \
      "std::atan2" "std::atan2f" "std::atan2l" \
      "std::cos"   "std::cosf"   "std::cosl"   \
      "std::sin"   "std::sinf"   "std::sinl"   \
      "std::tan"   "std::tanf"   "std::tanl"  

  # [7.12.5] Hyperbolic functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::acosh" "std::acoshf" "std::acoshl" \
      "std::asinh" "std::asinhf" "std::asinhl" \
      "std::atanh" "std::atanhf" "std::atanhl" \
      "std::cosh"  "std::coshf"  "std::coshl"  \
      "std::sinh"  "std::sinhf"  "std::sinhl"  \
      "std::tanh"  "std::tanhf"  "std::tanhl"

  # [7.12.6] Exponential and logarithmic functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::exp"     "std::expf"     "std::expl"    \
      "std::exp2"    "std::exp2f"    "std::exp2l"   \
      "std::expm1"   "std::expm1f"   "std::expm1l"  \
      "std::ilogb"   "std::ilogbf"   "std::ilogbl"  \
      "std::ldexp"   "std::ldexpf"   "std::ldexpl"  \
      "std::log"     "std::logf"     "std::logl"    \
      "std::log10"   "std::log10f"   "std::log10l"  \
      "std::log1p"   "std::log1pf"   "std::log1pl"  \
      "std::log2"    "std::log2f"    "std::log2l"   \
      "std::logb"    "std::logbf"    "std::logbl"   \
      "std::scalbn"  "std::scalbnf"  "std::scalbnl" \
      "std::scalbln" "std::scalblnf" "std::scalblnl"

  beam::function_attribute "$attr(frexp)" -names \
      "std::frexp" "std::frexpf" "std::frexpl"

  beam::function_attribute "$attr(modf)" -names \
      "std::modf"  "std::modff"  "std::modfl"
  
  # [7.12.7] Power and absolute-value functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::cbrt"  "std::cbrtf"  "std::cbrtl"  \
      "std::fabs"  "std::fabsf"  "std::fabsl"  \
      "std::hypot" "std::hypotf" "std::hypotl" \
      "std::pow"   "std::powf"   "std::powl"   \
      "std::sqrt"  "std::sqrtf"  "std::sqrtl"

  # [7.12.8] Error and gamma functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::erf"    "std::erff"    "std::erfl"    \
      "std::erfc"   "std::erfcf"   "std::erfcl"   \
      "std::lgamma" "std::lgammaf" "std::lgammal" \
      "std::tgamma" "std::tgammaf" "std::tgammal" 

  # [7.12.9] Nearest integer functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::ceil"      "std::ceilf"      "std::ceill"      \
      "std::floor"     "std::floorf"     "std::floorl"     \
      "std::nearbyint" "std::nearbyintf" "std::nearbyintl" \
      "std::rint"      "std::rintf"      "std::rintl"      \
      "std::lrint"     "std::lrintf"     "std::lrintl"     \
      "std::llrint"    "std::llrintf"    "std::llrintl"    \
      "std::round"     "std::roundf"     "std::roundl"     \
      "std::lround"    "std::lroundf"    "std::lroundl"    \
      "std::llround"   "std::llroundf"   "std::llroundl"   \
      "std::trunc"     "std::truncf"     "std::truncl"

  # [7.12.10] Remainder functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::fmod"      "std::fmodf"      "std::fmodl"      \
      "std::remainder" "std::remainderf" "std::remainderl" 

  beam::function_attribute "$attr(remquo)" -names \
      "std::remquo" "std::remquof" "std::remquol"

  # [7.12.11] Manipulation functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::copysign"   "std::copysignf"   "std::copysignl"   \
      "std::nextafter"  "std::nextafterf"  "std::nextafterl"  \
      "std::nexttoward" "std::nexttowardf" "std::nexttowardl" 

  beam::function_attribute "$attr(nan)" -names \
      "std::nan" "std::nanf" "std::nanl"

  # [7.12.12] Maximum, minimum, and positive difference functions

  beam::function_attribute "$attr(const_function)" -names \
      "std::fdim" "std::fdimf" "std::fdiml" \
      "std::fmax" "std::fmaxl" "std::fmaxl" \
      "std::fmin" "std::fminl" "std::fminl"

  # [7.12.13] Floating multiply-add

  beam::function_attribute "$attr(const_function)" -names \
      "std::fma" "std::fmaf" "std::fmal"
}
