/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = utilCipher.h

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/26/2018  ZWB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTILCIPHER_H_
#define UTILCIPHER_H_

#include "ossTypes.h"

void   utilCipherGenerateRandomArray( CHAR* array, UINT32 arrayLen ) ;
INT32  utilCipherExtractRandomArray( CHAR *cipherText, UINT32 cipherTextLen,
                                     CHAR *array, UINT32 *cipherTextNewLen,
                                     UINT32 *arrayNewLen ) ;
void   utilCipherInsertRandomArray( CHAR *destArray, UINT32 destArrayLen,
                                    CHAR *randArray, UINT32 randArrayLen,
                                    UINT32 *destArrayNewLen ) ;
INT32  utilCipherEncrypt( const CHAR *clearText, const CHAR *token,
                          CHAR *cipherText, UINT32 cipherTextLen ) ;
INT32  utilCipherDecrypt( const CHAR *cipherText, const CHAR *token,
                          CHAR *clearText ) ;
INT32  utilDecryptUserCipher( const CHAR *user, const CHAR *token,
                              const CHAR *path, CHAR *connectionUser,
                              CHAR *clearText ) ;

#endif /* UTILCIPHER_H_ */