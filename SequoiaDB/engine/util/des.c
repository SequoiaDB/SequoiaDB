/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = des.c

   Descriptive Name = des in c

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component. This file contains declare of json2rawbson. Note
   this function should NEVER be directly called other than fromjson.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/01/2017  JWH Initial Draft

   Last Changed =

*******************************************************************************/

#include "des.h"
#include "ossUtil.h"
#include "ossMem.h"
#include <openssl/des.h>


INT32 desEncryptSize( INT32 expressLen )
{
   return expressLen + ( 8 - expressLen % 8 ) ;
}

INT32 desEncrypt( BYTE *pDesKey, BYTE *pExpress, INT32 expressLen,
                  BYTE *pCiphertext )
{
   INT32 rc = SDB_OK ;
   INT32 padding = 8 - expressLen % 8 ;
   DES_cblock key ;
   DES_key_schedule keySchedule ;

   ossMemset( key, 0, sizeof( DES_cblock ) ) ;
   ossMemcpy( key, pDesKey, sizeof( DES_cblock ) ) ;

   DES_set_key_unchecked( &key, &keySchedule ) ;

   while( expressLen >= 0 )
   {
      const_DES_cblock input ;
      DES_cblock result ;

      ossMemset( &input, padding, sizeof( const_DES_cblock ) ) ;
      ossMemcpy( &input, pExpress, expressLen > 8 ? 8 : expressLen ) ;

      DES_ecb_encrypt( &input, &result, &keySchedule, DES_ENCRYPT ) ;

      ossMemcpy( pCiphertext, &result, sizeof( DES_cblock ) ) ;

      pExpress += 8 ;
      pCiphertext += 8 ;
      expressLen -= 8 ;
   }

   return rc ;
}

