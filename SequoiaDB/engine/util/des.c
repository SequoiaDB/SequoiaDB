/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

