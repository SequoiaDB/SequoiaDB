/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/

#ifndef NETWORK_H__
#define NETWORK_H__

#include "core.h"
#include "clientDef.h"
SDB_EXTERN_C_START

typedef struct Socket Socket ;

INT32 clientConnect ( const CHAR *pHostName,
                      const CHAR *pServiceName,
                      BOOLEAN useSSL,
                      Socket** sock ) ;

void clientDisconnect ( Socket** sock ) ;

// timeout for microsecond (1/1000000 sec )
INT32 clientSend ( Socket* sock, const CHAR *pMsg, INT32 len,
                   INT32 *pSentLen, INT32 timeout ) ;
// timeout for microsecond ( 1/1000000 sec )
INT32 clientRecv ( Socket* sock, CHAR *pMsg, INT32 len, 
                   INT32 *pReceivedLen, INT32 timeout ) ;

INT32 disableNagle( Socket* sock ) ;
SOCKET clientGetRawSocket( Socket* sock ) ;
INT32 setKeepAlive( SOCKET sock, INT32 keepAlive, INT32 keepIdle,
                    INT32 keepInterval, INT32 keepCount ) ;

void clientSetInterruptFunc( Socket* sock, socketInterruptFunc func ) ;

SDB_EXTERN_C_END
#endif
