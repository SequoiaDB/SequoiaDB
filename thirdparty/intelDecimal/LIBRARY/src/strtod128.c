/******************************************************************************
  Copyright (c) 2007-2018, Intel Corp.
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

#include "bid_strtod.h"



DFP_WRAPFN_TYPE1_TYPE2(128, bid_strtod128, const char* RESTRICT , char** RESTRICT)

BID_UINT128 bid_strtod128(const char* RESTRICT ps_in, char** RESTRICT endptr)
{
char * ps0;
BID_UINT128 DR;
#if !DECIMAL_GLOBAL_EXCEPTION_FLAGS
unsigned fpsc=0, *pfpsf=&fpsc;
#endif
#if !DECIMAL_GLOBAL_ROUNDING
unsigned rnd_mode=0;
#endif

   ps0 = strtod_conversion(ps_in, endptr);

   if(!ps0) {
	   DR.w[BID_HIGH_128W] = 0x3040000000000000ull;
	   DR.w[BID_LOW_128W] = 0ull;
	   return DR;   // 0.0
   }

   BIDECIMAL_CALL1_RESARG (bid128_from_string, DR, ps0);

   free(ps0);

   return DR;

}



 
