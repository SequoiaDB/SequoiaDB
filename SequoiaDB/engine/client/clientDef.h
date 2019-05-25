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

 *******************************************************************************/


/** \file clientDef.h
    \brief common include file C++ Client Driver
*/

#ifndef CLIENT_DEFINITION_H__
#define CLIENT_DEFINITION_H__

#include "ossTypes.h"

#define SDB_SNAP_CONTEXTS         0
#define SDB_SNAP_CONTEXTS_CURRENT 1
#define SDB_SNAP_SESSIONS         2
#define SDB_SNAP_SESSIONS_CURRENT 3
#define SDB_SNAP_COLLECTIONS      4
#define SDB_SNAP_COLLECTIONSPACES 5
#define SDB_SNAP_DATABASE         6
#define SDB_SNAP_SYSTEM           7
#define SDB_SNAP_CATALOG          8
#define SDB_SNAP_TRANSACTIONS     9
#define SDB_SNAP_TRANSACTIONS_CURRENT 10
#define SDB_SNAP_ACCESSPLANS      11
#define SDB_SNAP_HEALTH           12
#define SDB_SNAP_CONFIG           13

#define SDB_LIST_CONTEXTS         0
#define SDB_LIST_CONTEXTS_CURRENT 1
#define SDB_LIST_SESSIONS         2
#define SDB_LIST_SESSIONS_CURRENT 3
#define SDB_LIST_COLLECTIONS      4
#define SDB_LIST_COLLECTIONSPACES 5
#define SDB_LIST_STORAGEUNITS     6
#define SDB_LIST_GROUPS           7
#define SDB_LIST_STOREPROCEDURES  8
#define SDB_LIST_DOMAINS          9
#define SDB_LIST_TASKS            10
#define SDB_LIST_TRANSACTIONS     11
#define SDB_LIST_TRANSACTIONS_CURRENT 12
#define SDB_LIST_CL_IN_DOMAIN     129
#define SDB_LIST_CS_IN_DOMAIN     130



typedef BOOLEAN (*socketInterruptFunc)(void) ;

typedef struct _sdbClientConf
{
   BOOLEAN enableCacheStrategy ; /**< The flag to OPEN the cache strategy */
   UINT32  cacheTimeInterval ;   /**< The life cycle(in seconds) of cached object */
} sdbClientConf ;

#endif
