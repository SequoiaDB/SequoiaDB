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

#ifndef CLIENT_INTERNAL_H__
#define CLIENT_INTERNAL_H__

#include "core.h"
#include "client.h"
#include "network.h"
#include "oss.h"
#include "common.h"
#define SDB_HANDLE_TYPE_INVALID      0
#define SDB_HANDLE_TYPE_CONNECTION   1
#define SDB_HANDLE_TYPE_COLLECTION   2
#define SDB_HANDLE_TYPE_CS           3
#define SDB_HANDLE_TYPE_CURSOR       4
#define SDB_HANDLE_TYPE_REPLICAGROUP 5
#define SDB_HANDLE_TYPE_REPLICANODE  6
#define SDB_HANDLE_TYPE_DOMAIN       7
#define SDB_HANDLE_TYPE_LOB          8
#define SDB_HANDLE_TYPE_DC           9

struct _Node
{
   ossValuePtr data ;
   struct _Node *next ;
} ;
typedef struct _Node Node ;

struct _sdbConnectionStruct
{
   INT32 _handleType ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;
   Node *_cursors ;
   Node *_sockets ;
   hashTable *_tb ;
   bson *_attributeCache ;

   UINT64 reserveSpace1 ;
   ossMutex _sockMutex ;
} ;
typedef struct _sdbConnectionStruct sdbConnectionStruct ;

#define CLIENT_RG_NAMESZ         127
#define CLIENT_MAX_HOSTNAME      255
#define CLIENT_MAX_SERVICENAME   63
struct _sdbRGStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   CHAR  _replicaGroupName [ CLIENT_RG_NAMESZ+1 ] ;
   BOOLEAN _isCatalog ;
} ;
typedef struct _sdbRGStruct sdbRGStruct ;

#define SDB_REPLICA_NODE_INVALID_NODEID -1
struct _sdbRNStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   CHAR _hostName [ CLIENT_MAX_HOSTNAME + 1 ] ;
   CHAR _serviceName [ CLIENT_MAX_SERVICENAME + 1 ] ;
   CHAR _nodeName [ CLIENT_MAX_HOSTNAME + CLIENT_MAX_SERVICENAME + 2 ] ;
   INT32 _nodeID ;
} ;
typedef struct _sdbRNStruct sdbRNStruct ;

#define CLIENT_COLLECTION_NAMESZ 127
#define CLIENT_CS_NAMESZ         127
struct _sdbCSStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   CHAR _CSName [ CLIENT_CS_NAMESZ + 1 ] ;
} ;
typedef struct _sdbCSStruct sdbCSStruct ;

struct _sdbCollectionStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   CHAR _collectionName [ CLIENT_COLLECTION_NAMESZ + 1 ] ;
   CHAR _CSName [ CLIENT_CS_NAMESZ + 1 ] ;
   CHAR _collectionFullName [ CLIENT_CS_NAMESZ + CLIENT_COLLECTION_NAMESZ + 2 ];
} ;
typedef struct _sdbCollectionStruct sdbCollectionStruct ;

struct _sdbCursorStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   INT32 _offset ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _isClosed ;
   BOOLEAN _endianConvert ;

   SINT64 _contextID ;
   SINT64 _totalRead ;
   CHAR _collectionFullName [ CLIENT_CS_NAMESZ + CLIENT_COLLECTION_NAMESZ + 2 ];
} ;
typedef struct _sdbCursorStruct sdbCursorStruct ;

#define CLIENT_DOMAIN_NAMESZ 127
struct _sdbDomainStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   CHAR _domainName [ CLIENT_DOMAIN_NAMESZ + 1 ] ;
} ;
typedef struct _sdbDomainStruct sdbDomainStruct ;

enum LOB_SEEK
{
   LOB_SEEK_SET = 0,
   LOB_SEEK_CUR,
   LOB_SEEK_END,
} ;

struct _sdbLobStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   INT32 _offset ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   SINT64 _contextID ;
   INT32 _mode ;
   CHAR _oid[12] ; //DMS_LOB_OID_LEN
   UINT64 _createTime ;
   UINT64 _modificationTime ;
   SINT64 _lobSize ;
   SINT64 _currentOffset ;
   SINT64 _cachedOffset ;
   UINT32 _cachedSize ;
   UINT32 _pageSize ;
   BOOLEAN _seekWrite ;
   const CHAR *_dataCache ;
} ;
typedef struct _sdbLobStruct sdbLobStruct ;

#define CLIENT_DC_NAMESZ 127
struct _sdbDCStruct
{
   INT32 _handleType ;
   sdbConnectionHandle _connection ;
   Socket* _sock ;
   CHAR *_pSendBuffer ;
   INT32 _sendBufferSize ;
   CHAR *_pReceiveBuffer ;
   INT32 _receiveBufferSize ;
   BOOLEAN _endianConvert ;

   CHAR _name[ CLIENT_DC_NAMESZ + 1 ] ;
} ;
typedef struct _sdbDCStruct sdbDCStruct ;


#endif
