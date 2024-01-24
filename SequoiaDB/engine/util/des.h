#ifndef UTIL_DES_H__
#define UTIL_DES_H__

#include "core.h"

SDB_EXTERN_C_START

SDB_EXPORT INT32 desEncryptSize( INT32 expressLen ) ;

SDB_EXPORT INT32 desEncrypt( BYTE *pDesKey, BYTE *pExpress, INT32 expressLen,
                             BYTE *pCiphertext ) ;

SDB_EXTERN_C_END

#endif
