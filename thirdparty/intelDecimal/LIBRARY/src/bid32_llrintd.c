/******************************************************************************
  Copyright (c) 2018, Intel Corp.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors 
      may be used to endorse or promote products derived from this software 
      without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include "bid_internal.h"

/*****************************************************************************
 *  BID32_llrint
 ****************************************************************************/

/*
  DESCRIPTION:
    The llrint function rounds its argument to the nearest integer value of
    type long long int, rounding according to the current rounding direction. 
  RETURN VALUE: 
    If the rounded value is outside the range of the return type or the  
    argument is infinity or NaN, the result is the largest negative value
    and the invalid exception is signaled
  EXCEPTIONS SIGNALED: 
    invalid and inexact
 */

BID_RESTYPE0_FUNCTION_ARGTYPE1(long long int, bid32_llrint, BID_UINT32, x)

  long long int res; // assume sizeof (long long) = 8

  if (rnd_mode == BID_ROUNDING_TO_NEAREST)
    BIDECIMAL_CALL1_NORND (bid32_to_int64_xrnint, res, x);
  else if (rnd_mode == BID_ROUNDING_TIES_AWAY)
    BIDECIMAL_CALL1_NORND (bid32_to_int64_xrninta, res, x);
  else if (rnd_mode == BID_ROUNDING_DOWN)
    BIDECIMAL_CALL1_NORND (bid32_to_int64_xfloor, res, x);
  else if (rnd_mode == BID_ROUNDING_UP)
    BIDECIMAL_CALL1_NORND (bid32_to_int64_xceil, res, x);
  else // if (rnd_mode == BID_ROUNDING_TO_ZERO)
    BIDECIMAL_CALL1_NORND (bid32_to_int64_xint, res, x);
  BID_RETURN (res);
}
