/*******************************************************************************
   Copyright (C) 2023-present SequoiaDB Ltd.

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
#ifndef CLIENTIMPL_HPP__
#define CLIENTIMPL_HPP__
#include "core.hpp"
#include "client.hpp"
#include "common.h"
#include "ossSocket.hpp"
#include <set>
#include "ossUtil.hpp"
#include "utilAuthSCRAMSHA.hpp"
#include "ossLatch.hpp"

using namespace bson ;

namespace sdbclient
{
#define CLIENT_COLLECTION_NAMESZ           127
#define CLIENT_CS_NAMESZ                   127
#define CLIENT_REPLICAGROUP_NAMESZ         127
#define CLIENT_DOMAIN_NAMESZ               127
#define CLIENT_DC_NAMESZ                   127
#define CLIENT_CL_FULLNAME_SZ              ( CLIENT_COLLECTION_NAMESZ + CLIENT_CS_NAMESZ + 1 )
#define CLIENT_DATASOURCE_NAMESZ           127

   class _sdbCollectionSpaceImpl ;
   class _sdbCollectionImpl ;
   class _sdbReplicaGroupImpl ;
   class _sdbNodeImpl ;
   class _sdbDomainImpl ;
   class _sdbDataCenterImpl ;
   class _sdbRecycleBinImpl ;
   class _sdbLobImpl ;
   class _sdbImpl ;
   class _sdbMsgConvertor ;

   /*
      CLIENT_CLASS_TYPE define
   */
   enum CLIENT_CLASS_TYPE
   {
      CLIENT_CLASS_SDB         = 0,
      CLIENT_CLASS_CS          = 1,
      CLIENT_CLASS_CL          = 2,
      CLIENT_CLASS_CURSOR      = 3,
      CLIENT_CLASS_RG          = 4,
      CLIENT_CLASS_NODE        = 5,
      CLIENT_CLASS_LOB         = 6,
      CLIENT_CLASS_DOMAIN      = 7,
      CLIENT_CLASS_DC          = 8,  // data center
      CLIENT_CLASS_SQ          = 9,  // sequeue
      CLIENT_CLASS_DS          = 10, // datasource
      CLIENT_CLASS_RB          = 11  // recycle bin
   } ;

   /*
      _sdbBase define
   */
   class _sdbBase
   {
   public :
      _sdbBase(CLIENT_CLASS_TYPE type) ;
      virtual ~_sdbBase() {}

   public :
      virtual INT32    _setConnection( _sdbImpl *connection ) = 0 ;
      virtual void     _dropConnection() = 0 ;

   protected :
      /**
       * set connection handle to current object
       * and register current object to connection
       */
      INT32            _regHandle( _sdbImpl *connection,
                                   ossValuePtr ptr ) ;
      /**
       * when the connection is destroyed, the current object will be
       * unregister from the connection, and the connection handle
       * in current object will set to NULL
       */
      void             _unregHandle( ossValuePtr ptr ) ;

   private :
      /**
       * clean Error and Result external buffer pointer.
       */
      void             _resetErrorAndResultBuf() ;

   protected :
      CLIENT_CLASS_TYPE     _type ;
      _sdbImpl             *_connection ;
      CHAR                 *_pSendBuffer ;
      INT32                 _sendBufferSize ;
      CHAR                 *_pReceiveBuffer ;
      INT32                 _receiveBufferSize ;
   } ;

   /*
      _sdbCursorImpl
   */
   class _sdbCursorImpl : public _sdbCursor, public _sdbBase
   {
   private :
      _sdbCursorImpl ( const _sdbCursorImpl& other ) ;
      _sdbCursorImpl& operator=( const _sdbCursorImpl& ) ;

      SINT64               _contextID ;
      BOOLEAN              _isClosed ;

      INT64                _totalRead ;
      INT32                _offset ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32    _killCursor () ;
      INT32    _readNextBuffer () ;
      void     _close() ;

      friend class _sdbCollectionImpl ;
      friend class _sdbNodeImpl ;
      friend class _sdbImpl ;

   public :
      _sdbCursorImpl () ;
      ~_sdbCursorImpl () ;

      INT32 next          ( BSONObj &obj, BOOLEAN getOwned = TRUE ) ;
      INT32 current       ( BSONObj &obj, BOOLEAN getOwned = TRUE ) ;
      INT32 close () ;
      INT32 advance       ( const BSONObj &option, BSONObj *pResult = NULL ) ;
   } ;

   typedef class _sdbCursorImpl sdbCursorImpl ;

   /*
      _sdbCollectionImpl
   */
   class _sdbCollectionImpl : public _sdbCollection, public _sdbBase
   {
   private :
      _sdbCollectionImpl ( const _sdbCollectionImpl& other ) ;
      _sdbCollectionImpl& operator=( const _sdbCollectionImpl& ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch            _mutex ;
#endif
      CHAR                    *_pAppendOIDBuffer ;
      INT32                    _appendOIDBufferSize ;
      INT32                   _version ;

      CHAR _collectionSpaceName [ CLIENT_CS_NAMESZ+1 ] ;
      CHAR _collectionName      [ CLIENT_COLLECTION_NAMESZ+1 ] ;
      CHAR _collectionFullName  [ CLIENT_CL_FULLNAME_SZ + 1 ] ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32    _setName ( const CHAR *pCollectionFullName ) ;
      void*    _getConnection () ;

      INT32    _queryAndModify( _sdbCursor **cursor,
                                const BSONObj &condition,
                                const BSONObj &selected,
                                const BSONObj &orderBy,
                                const BSONObj &hint,
                                const BSONObj &update,
                                INT64 numToSkip,
                                INT64 numToReturn,
                                INT32 flag,
                                BOOLEAN isUpdate,
                                BOOLEAN returnNew ) ;

      INT32 _update ( const BSONObj &rule,
                      const BSONObj &condition,
                      const BSONObj &hint,
                      INT32 flag,
                      BSONObj *pResult ) ;

      INT32 _appendOID ( const BSONObj &input,
                         BSONObj &output ) ;

      INT32 _runCmdOfLob ( const CHAR *cmd, const BSONObj *query,
                           const BSONObj *selector, const BSONObj *orderBy,
                           const BSONObj *hint, INT64 skip, INT64 returnNum,
                           _sdbCursor **cursor ) ;

      INT32 _getRetLobInfo( CHAR **ppBuffer,
                            INT32 *pBufSize,
                            const OID *oid,
                            INT32 mode,
                            SINT64 contextID,
                            _sdbLob **lob ) ;

      INT32 _createLob( _sdbLob **lob, const bson::OID *oid,
                        BOOLEAN *isOldVersionLobServer = NULL ) ;

#if defined CLIENT_THREAD_SAFE
      void lock ()
      {
         _mutex.get () ;
      }
      void unlock ()
      {
         _mutex.release () ;
      }
#else
      void lock ()
      {
      }
      void unlock ()
      {
      }
#endif
      friend class _sdbCollectionSpaceImpl ;
      friend class _sdbImpl ;
      friend class _sdbCursorImpl ;

   public :
      _sdbCollectionImpl () ;
      _sdbCollectionImpl ( CHAR *pCollectionFullName ) ;
      _sdbCollectionImpl ( CHAR *pCollectionSpaceName,
                           CHAR *pCollectionName ) ;
      ~_sdbCollectionImpl () ;
      // get the total number of records for a given condition, if the condition
      // is NULL then match all records in the collection
      INT32 getCount ( SINT64 &count,
                       const BSONObj &condition,
                       const BSONObj &hint = _sdbStaticObject ) ;
      // insert a bson object into current collection
      // given:
      // object ( required )
      INT32 insert ( const BSONObj &obj, OID *id ) ;
      INT32 insert ( const bson::BSONObj &obj,
                     INT32 flags,
                     bson::BSONObj *pResult = NULL ) ;
      INT32 insert ( const bson::BSONObj &obj,
                     const bson::BSONObj &hint,
                     INT32 flags,
                     bson::BSONObj *pResult = NULL ) ;
      INT32 insert ( std::vector<bson::BSONObj> &objs,
                     const bson::BSONObj &hint,
                     INT32 flags = 0,
                     bson::BSONObj *pResult = NULL ) ;
      INT32 insert ( std::vector<bson::BSONObj> &objs,
                     INT32 flags = 0,
                     bson::BSONObj *pResult = NULL ) ;
      INT32 insert ( const bson::BSONObj objs[],
                     INT32 size,
                     INT32 flags = 0,
                     bson::BSONObj *pResult = NULL ) ;
      INT32 bulkInsert ( SINT32 flags,
                         vector<BSONObj> &obj ) ;
      // update bson object from current collection
      // given:
      // update rule ( required )
      // update condition ( optional )
      // hint ( optional )
      // flag ( optional )
      // pResult ( optional )
      INT32 update ( const BSONObj &rule,
                     const BSONObj &condition = _sdbStaticObject,
                     const BSONObj &hint = _sdbStaticObject,
                     INT32 flag                 = 0,
                     BSONObj *pResult           = NULL
                   ) ;

      // update bson object from current collection, if no record has been
      // updated, it will insert a new record that modified from an empty bson
      // object
      // given:
      // update rule ( required )
      // update condition ( optional )
      // hint ( optional )
      // setOnInsert ( optional )
      // flag ( optional )
      // pResult ( optional )
      INT32 upsert ( const BSONObj &rule,
                     const BSONObj &condition = _sdbStaticObject,
                     const BSONObj &hint = _sdbStaticObject,
                     const BSONObj &setOnInsert = _sdbStaticObject,
                     INT32 flag                 = 0,
                     BSONObj *pResult           = NULL
                   ) ;
      // delete bson objects from current collection
      // given:
      // delete condition ( optional )
      // hint ( optional )
      // flag ( optional )
      // pResult ( optional )
      INT32 del    ( const BSONObj &condition = _sdbStaticObject,
                     const BSONObj &hint        = _sdbStaticObject,
                     INT32 flag                 = 0,
                     BSONObj *pResult           = NULL
                   ) ;

      // pop bson objects from current collection
      // given:
      // pop condition ( required )
      INT32 pop    ( const BSONObj &option = _sdbStaticObject ) ;

      // query objects from current collection
      // given:
      // query condition ( optional )
      // query selected def ( optional )
      // query orderby ( optional )
      // hint ( optional )
      // output: sdbCursor ( required )
      INT32 query  ( _sdbCursor **cursor,
                     const BSONObj &condition = _sdbStaticObject,
                     const BSONObj &selected  = _sdbStaticObject,
                     const BSONObj &orderBy   = _sdbStaticObject,
                     const BSONObj &hint      = _sdbStaticObject,
                     INT64 numToSkip          = 0,
                     INT64 numToReturn        = -1,
                     INT32 flag               = 0
                   ) ;

      INT32 query  ( sdbCursor &cursor,
                     const BSONObj &condition = _sdbStaticObject,
                     const BSONObj &selected  = _sdbStaticObject,
                     const BSONObj &orderBy   = _sdbStaticObject,
                     const BSONObj &hint      = _sdbStaticObject,
                     INT64 numToSkip          = 0,
                     INT64 numToReturn        = -1,
                     INT32 flag               = 0
                   )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return query ( &cursor.pCursor,
                        condition, selected, orderBy, hint,
                        numToSkip, numToReturn, flag ) ;
      }

      INT32 queryOne( bson::BSONObj &obj,
                      const bson::BSONObj &condition = _sdbStaticObject,
                      const bson::BSONObj &selected  = _sdbStaticObject,
                      const bson::BSONObj &orderBy   = _sdbStaticObject,
                      const bson::BSONObj &hint      = _sdbStaticObject,
                      INT64 numToSkip    = 0,
                      INT32 flag = 0 ) ;

      // query objects from current collection and update
      // given:
      // update rule ( required )
      // query condition ( optional )
      // query selected def ( optional )
      // query orderby ( optional )
      // hint ( optional )
      // flag( optional )
      // returnNew ( optioinal )
      // output: sdbCursor ( required )
      INT32 queryAndUpdate  ( _sdbCursor **cursor,
                              const BSONObj &update,
                              const BSONObj &condition = _sdbStaticObject,
                              const BSONObj &selected  = _sdbStaticObject,
                              const BSONObj &orderBy   = _sdbStaticObject,
                              const BSONObj &hint      = _sdbStaticObject,
                              INT64 numToSkip          = 0,
                              INT64 numToReturn        = -1,
                              INT32 flag               = 0,
                              BOOLEAN returnNew        = FALSE )
      {
         return _queryAndModify( cursor, condition, selected, orderBy,
                                 hint, update, numToSkip, numToReturn,
                                 flag, TRUE, returnNew ) ;
      }

      // query objects from current collection and remove
      // given:
      // query condition ( optional )
      // query selected def ( optional )
      // query orderby ( optional )
      // hint ( optional )
      // flag ( optional )
      // output: sdbCursor ( required )
      INT32 queryAndRemove  ( _sdbCursor **cursor,
                              const BSONObj &condition = _sdbStaticObject,
                              const BSONObj &selected  = _sdbStaticObject,
                              const BSONObj &orderBy   = _sdbStaticObject,
                              const BSONObj &hint      = _sdbStaticObject,
                              INT64 numToSkip          = 0,
                              INT64 numToReturn        = -1,
                              INT32 flag               = 0 )
      {
         return _queryAndModify( cursor, condition, selected, orderBy,
                                 hint, _sdbStaticObject, numToSkip, numToReturn,
                                 flag, FALSE, FALSE ) ;
      }

      //INT32 rename ( const CHAR *pNewName ) ;
      // create an index for the current collection
      // given:
      // index definition ( required )
      // index name ( required )
      // uniqueness ( required )
      INT32 createIndex ( const BSONObj &indexDef,
                          const CHAR *pName,
                          BOOLEAN isUnique,
                          BOOLEAN isEnforced,
                          INT32 sortBufferSize ) ;
      INT32 createIndex ( const BSONObj &indexDef,
                          const CHAR *pIndexName,
                          const BSONObj &indexAttr = _sdbStaticObject,
                          const BSONObj &option = _sdbStaticObject ) ;
      INT32 createIndexAsync ( SINT64 &taskID,
                               const BSONObj &indexDef,
                               const CHAR *pIndexName,
                               const BSONObj &indexAttr = _sdbStaticObject,
                               const BSONObj &option = _sdbStaticObject ) ;
      INT32 snapshotIndexes ( _sdbCursor **cursor,
                              const BSONObj &condition = _sdbStaticObject,
                              const BSONObj &selector = _sdbStaticObject,
                              const BSONObj &orderby = _sdbStaticObject,
                              const BSONObj &hint = _sdbStaticObject,
                              INT64 numToSkip = 0,
                              INT64 numToReturn = -1 ) ;
      INT32 getIndexes ( _sdbCursor **cursor,
                         const CHAR *pName ) ;
      INT32 getIndexes ( sdbCursor &cursor,
                         const CHAR *pName )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return getIndexes ( &cursor.pCursor, pName ) ;
      }
      INT32 getIndexes ( std::vector<bson::BSONObj> &infos ) ;
      INT32 getIndex ( const CHAR *pName, bson::BSONObj &info ) ;
      INT32 dropIndex ( const CHAR *pIndexName ) ;
      INT32 dropIndexAsync ( SINT64 &taskID, const CHAR *pIndexName ) ;
      INT32 copyIndex ( const CHAR *subClFullName,
                        const CHAR *pIndexName ) ;
      INT32 copyIndexAsync ( SINT64 &taskID, const CHAR *subClFullName,
                             const CHAR *pIndexName ) ;
      INT32 create () ;
      INT32 drop () ;
      const CHAR *getCollectionName ()
      {
         return &_collectionName[0] ;
      }
      const CHAR *getCSName ()
      {
         return &_collectionSpaceName[0] ;
      }
      const CHAR *getFullName ()
      {
         return &_collectionFullName[0] ;
      }
      INT32 split ( const CHAR *pSourceReplicaGroupName,
                    const CHAR *pTargetReplicaGroupName,
                    const BSONObj &splitCondition,
                    const bson::BSONObj &splitEndCondition = _sdbStaticObject ) ;
      INT32 split ( const CHAR *pSourceReplicaGroupName,
                    const CHAR *pTargetReplicaGroupName,
                    FLOAT64 percent ) ;
      INT32 splitAsync ( SINT64 &taskID,
                         const CHAR *pSourceReplicaGroupName,
                         const CHAR *pTargetReplicaGroupName,
                         const bson::BSONObj &splitCondition,
                         const bson::BSONObj &splitEndCondition = _sdbStaticObject ) ;
      INT32 splitAsync ( const CHAR *pSourceReplicaGroupName,
                         const CHAR *pTargetReplicaGroupName,
                         FLOAT64 percent,
                         SINT64 &taskID ) ;
      // aggregate
      INT32 aggregate ( _sdbCursor **cursor,
                        std::vector<bson::BSONObj> &obj ) ;
      INT32 aggregate ( sdbCursor &cursor,
                        std::vector<bson::BSONObj> &obj )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return aggregate ( &cursor.pCursor, obj ) ;
      }
      INT32 getQueryMeta  ( _sdbCursor **cursor,
                            const BSONObj &condition = _sdbStaticObject,
                            const BSONObj &orderBy = _sdbStaticObject,
                            const BSONObj &hint = _sdbStaticObject,
                            INT64 numToSkip = 0,
                            INT64 numToReturn = -1 ) ;
      INT32 getQueryMeta  ( sdbCursor &cursor,
                            const BSONObj &condition = _sdbStaticObject,
                            const BSONObj &orderBy = _sdbStaticObject,
                            const BSONObj &hint = _sdbStaticObject,
                            INT64 numToSkip = 0,
                            INT64 numToReturn = -1 )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return getQueryMeta ( &cursor.pCursor,
                               condition, orderBy, hint,
                               numToSkip, numToReturn ) ;
      }

      INT32 attachCollection ( const CHAR *subClFullName,
                               const bson::BSONObj &options) ;
      INT32 detachCollection ( const CHAR *subClFullName) ;

      INT32 alterCollection ( const bson::BSONObj &options ) ;

      /// explain
      INT32 explain ( _sdbCursor **cursor,
                      const bson::BSONObj &condition = _sdbStaticObject,
                      const bson::BSONObj &select    = _sdbStaticObject,
                      const bson::BSONObj &orderBy   = _sdbStaticObject,
                      const bson::BSONObj &hint      = _sdbStaticObject,
                      INT64 numToSkip                = 0,
                      INT64 numToReturn              = -1,
                      INT32 flag                     = 0,
                      const bson::BSONObj &options   = _sdbStaticObject ) ;

      virtual INT32 explain ( sdbCursor &cursor,
                              const bson::BSONObj &condition = _sdbStaticObject,
                              const bson::BSONObj &select    = _sdbStaticObject,
                              const bson::BSONObj &orderBy   = _sdbStaticObject,
                              const bson::BSONObj &hint      = _sdbStaticObject,
                              INT64 numToSkip                = 0,
                              INT64 numToReturn              = -1,
                              INT32 flag                     = 0,
                              const bson::BSONObj &options   = _sdbStaticObject )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return explain( &cursor.pCursor, condition, select, orderBy, hint,
                         numToSkip, numToReturn, flag, options ) ;
      }

      /// lob
      INT32 createLob( _sdbLob **lob, const bson::OID *oid = NULL ) ;

      virtual INT32 createLob( sdbLob &lob, const bson::OID *oid = NULL )
      {
         RELEASE_INNER_HANDLE( lob.pLob ) ;
         return createLob( &lob.pLob, oid ) ;
      }

      virtual INT32 removeLob( const bson::OID &oid ) ;

      virtual INT32 truncateLob( const bson::OID &oid, INT64 length ) ;

      INT32 openLob( _sdbLob **lob, const bson::OID &oid, INT32 mode ) ;

      virtual INT32 openLob( sdbLob &lob, const bson::OID &oid,
                             SDB_LOB_OPEN_MODE mode = SDB_LOB_READ )
      {
         RELEASE_INNER_HANDLE( lob.pLob ) ;
         return openLob( &lob.pLob, oid, mode ) ;
      }

      virtual INT32 openLob( sdbLob &lob, const bson::OID &oid, INT32 mode )
      {
         RELEASE_INNER_HANDLE( lob.pLob ) ;
         return openLob( &lob.pLob, oid, mode ) ;
      }

      INT32 listLobs( _sdbCursor **cursor,
                      const bson::BSONObj &condition = _sdbStaticObject,
                      const bson::BSONObj &selected  = _sdbStaticObject,
                      const bson::BSONObj &orderBy   = _sdbStaticObject,
                      const bson::BSONObj &hint      = _sdbStaticObject,
                      INT64 numToSkip = 0, INT64 numToReturn = -1 ) ;

      virtual INT32 listLobs( sdbCursor &cursor,
                              const bson::BSONObj &condition = _sdbStaticObject,
                              const bson::BSONObj &selected  = _sdbStaticObject,
                              const bson::BSONObj &orderBy   = _sdbStaticObject,
                              const bson::BSONObj &hint      = _sdbStaticObject,
                              INT64 numToSkip = 0, INT64 numToReturn = -1)
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listLobs( &cursor.pCursor, condition, selected, orderBy, hint,
                          numToSkip, numToReturn ) ;
      }

      virtual INT32 createLobID( bson::OID &oid, const CHAR *pTimestamp = NULL ) ;

      virtual INT32 listLobPieces(
                              _sdbCursor **cursor,
                              const bson::BSONObj &condition = _sdbStaticObject,
                              const bson::BSONObj &selected  = _sdbStaticObject,
                              const bson::BSONObj &orderBy   = _sdbStaticObject,
                              const bson::BSONObj &hint      = _sdbStaticObject,
                              INT64 numToSkip = 0,
                              INT64 numToReturn = -1 ) ;

      virtual INT32 listLobPieces(
                              sdbCursor &cursor,
                              const bson::BSONObj &condition = _sdbStaticObject,
                              const bson::BSONObj &selected  = _sdbStaticObject,
                              const bson::BSONObj &orderBy   = _sdbStaticObject,
                              const bson::BSONObj &hint      = _sdbStaticObject,
                              INT64 numToSkip = 0,
                              INT64 numToReturn = -1 )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listLobPieces( &cursor.pCursor, condition, selected, orderBy,
                               hint, numToSkip, numToReturn ) ;
      }
      /// truncate
      INT32 truncate( const bson::BSONObj &options = _sdbStaticObject ) ;

      /// create/drop index
      INT32 createIdIndex( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 dropIdIndex() ;

      INT32 createAutoIncrement( const bson::BSONObj &options ) ;

      INT32 createAutoIncrement( const std::vector<bson::BSONObj> &options ) ;

      INT32 dropAutoIncrement( const CHAR* fieldName );

      INT32 dropAutoIncrement( const std::vector<const CHAR*> &fieldNames ) ;

      INT32 enableSharding ( const bson::BSONObj & options ) ;

      INT32 disableSharding () ;

      INT32 enableCompression ( const bson::BSONObj & options = _sdbStaticObject ) ;

      INT32 disableCompression () ;

      INT32 setAttributes ( const bson::BSONObj & options ) ;

      INT32 getDetail ( _sdbCursor **cursor ) ;

      INT32 getDetail ( sdbCursor &cursor )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return getDetail ( &cursor.pCursor ) ;
      }

      INT32 getCollectionStat ( bson::BSONObj &result ) ;

      INT32 getIndexStat ( const CHAR *pIndexName, bson::BSONObj &result,
                           BOOLEAN detail = FALSE ) ;

      void  setVersion ( INT32 clVersion ) ;
      INT32 getVersion () ;
      INT32 setConsistencyStrategy( INT32 value ) ;

   private:
      INT32 _alterCollection1( const bson::BSONObj &options ) ;
      INT32 _alterCollection2( const bson::BSONObj &options ) ;
      INT32 _createIndex ( const BSONObj &indexDef, const CHAR *pName,
                           BOOLEAN isUnique, BOOLEAN isEnforced,
                           INT32 sortBufferSize,
                           SINT64 *pTaskID = NULL ) ;
      INT32 _createIndex ( const BSONObj &indexDef, const CHAR *pIndexName,
                           const BSONObj &indexAttr,
                           const BSONObj &option,
                           SINT64 *pTaskID = NULL ) ;
      INT32 _dropIndex ( const CHAR *pIndexName,
                         SINT64 *pTaskID = NULL ) ;
      INT32 _copyIndex ( const CHAR *subClFullName,
                         const CHAR *pIndexName,
                         SINT64 *pTaskID = NULL ) ;

      INT32 _alterInternal ( const CHAR * taskName,
                             const bson::BSONObj * argument,
                             BOOLEAN allowNullArgs ) ;

      INT32 _getRetVersion () ;

      INT32 _runCommand ( const CHAR *pString,
                          const BSONObj *arg1 = NULL,
                          const BSONObj *arg2 = NULL,
                          const BSONObj *arg3 = NULL,
                          const BSONObj *arg4 = NULL,
                          SINT32 flag = 0,
                          UINT64 reqID = 0,
                          SINT64 numToSkip = 0,
                          SINT64 numToReturn = -1,
                          _sdbCursor **ppCursor = NULL ) ;

      INT32 _insert ( const BSONObj &obj,
                      const BSONObj &hint,
                      INT32 flags,
                      BSONObj &newObj,
                      BSONObj *pResult = NULL ) ;

      INT32 _query ( _sdbCursor **cursor,
                     const BSONObj &condition = _sdbStaticObject,
                     const BSONObj &selected  = _sdbStaticObject,
                     const BSONObj &orderBy   = _sdbStaticObject,
                     const BSONObj &hint      = _sdbStaticObject,
                     INT64 numToSkip          = 0,
                     INT64 numToReturn        = -1,
                     INT32 flag               = 0
                   ) ;
   } ;

   typedef class _sdbCollectionImpl sdbCollectionImpl ;

   /*
      _sdbNodeImpl
   */
   #define SDB_NODE_INVALID_NODEID     -1

   class _sdbNodeImpl : public _sdbNode, public _sdbBase
   {
   private :
      _sdbNodeImpl ( const _sdbNodeImpl& other ) ;
      _sdbNodeImpl& operator=( const _sdbNodeImpl& ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch _mutex ;
#endif

      CHAR                     _hostName [ OSS_MAX_HOSTNAME + 1 ] ;
      CHAR                     _serviceName [ OSS_MAX_SERVICENAME + 1 ] ;
      CHAR                     _nodeName [ OSS_MAX_HOSTNAME +
                                           OSS_MAX_SERVICENAME + 2 ] ;
      INT32                    _replicaGroupID ;
      INT32                    _nodeID ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32 _stopStart ( BOOLEAN start ) ;

      friend class _sdbReplicaGroupImpl ;
      friend class _sdbImpl ;

   public :
      _sdbNodeImpl () ;
      ~_sdbNodeImpl () ;
      // directly connect to the current node
      INT32 connect ( _sdb **dbConn ) ;
      INT32 connect ( sdb &dbConn )
      {
         RELEASE_INNER_HANDLE( dbConn.pSDB ) ;
         return connect ( &dbConn.pSDB ) ;
      }

      // get status of the current node
      sdbNodeStatus getStatus () ;

      // get the hostname
      const CHAR *getHostName ()
      {
         return _hostName ;
      }

      // get the service name
      const CHAR *getServiceName ()
      {
         return _serviceName ;
      }

      // get the node id
      const CHAR *getNodeName ()
      {
         return _nodeName ;
      }

      INT32 getNodeID( INT32 &nodeID ) const
      {
         nodeID = _nodeID ;
         return SDB_OK ;
      }

      // stop the node
      INT32 stop () { return _stopStart ( FALSE ) ; }

      // start the node
      INT32 start () { return _stopStart ( TRUE ) ; }

   protected:
      INT32 _innerAlter ( const CHAR * taskName, const BSONObj * options ) ;

   public:
      INT32 setLocation ( const CHAR *pLocation ) ;

      INT32 setAttributes ( const BSONObj & options ) ;

   } ;

   typedef class _sdbNodeImpl sdbNodeImpl ;

   /*
      _sdbReplicaGroupImpl
   */
   class _sdbReplicaGroupImpl : public _sdbReplicaGroup, public _sdbBase
   {
   private :
      _sdbReplicaGroupImpl ( const _sdbReplicaGroupImpl& other ) ;
      _sdbReplicaGroupImpl& operator=( const _sdbReplicaGroupImpl& ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch _mutex ;
#endif

      BOOLEAN                 _isCatalog ;
      INT32                   _replicaGroupID ;
      CHAR                    _replicaGroupName [ CLIENT_REPLICAGROUP_NAMESZ+1 ] ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32 _stopStart ( BOOLEAN start ) ;
      INT32 _extractNode ( _sdbNode **node,
                           const CHAR *primaryData ) ;

      friend class _sdbImpl ;

   public :
      _sdbReplicaGroupImpl () ;
      ~_sdbReplicaGroupImpl () ;

      // get number of logical nodes
      INT32 getNodeNum ( sdbNodeStatus status, INT32 *num ) ;

      // list all nodes in the current replica group
      INT32 getDetail ( BSONObj &result ) ;

      INT32 getMaster ( _sdbNode **node ) ;
      INT32 getMaster ( sdbNode &node )
      {
         RELEASE_INNER_HANDLE( node.pNode ) ;
         return getMaster ( &node.pNode ) ;
      }

      INT32 getSlave ( _sdbNode **node,
                       const vector<INT32>& positions = _sdbStaticVec ) ;
      INT32 getSlave ( sdbNode &node,
                       const vector<INT32>& positions = _sdbStaticVec )
      {
         RELEASE_INNER_HANDLE( node.pNode ) ;
         return getSlave( &node.pNode, positions ) ;
      }

      INT32 getNode ( const CHAR *pNodeName,
                      _sdbNode **node ) ;
      INT32 getNode ( const CHAR *pNodeName,
                      sdbNode &node )
      {
         RELEASE_INNER_HANDLE( node.pNode ) ;
         return getNode ( pNodeName, &node.pNode ) ;
      }

      INT32 getNode ( const CHAR *pHostName,
                      const CHAR *pServiceName,
                      _sdbNode **node ) ;
      INT32 getNode ( const CHAR *pHostName,
                      const CHAR *pServiceName,
                      sdbNode &node )
      {
         RELEASE_INNER_HANDLE( node.pNode ) ;
         return getNode ( pHostName, pServiceName, &node.pNode ) ;
      }
      // create a new node in current replica group
      INT32 createNode ( const CHAR *pHostName,
                         const CHAR *pServiceName,
                         const CHAR *pDatabasePath,
                         std::map<std::string,std::string> &config,
                         _sdbNode **ppNode = NULL ) ;

      INT32 createNode ( const CHAR *pHostName,
                         const CHAR *pServiceName,
                         const CHAR *pDatabasePath,
                         const bson::BSONObj &options = _sdbStaticObject,
                         _sdbNode **ppNode = NULL ) ;

      // remove the specified node in current replica group
      INT32 removeNode ( const CHAR *pHostName,
                         const CHAR *pServiceName,
                         const BSONObj &configure = _sdbStaticObject ) ;

      // activate the replica group
      INT32 start () ;

      // stop the entire replica group
      INT32 stop () ;

      // get replica group name
      const CHAR *getName ()
      {
         return _replicaGroupName ;
      }

      // whether the current replica group is catalog replica group or not
      BOOLEAN isCatalog ()
      {
         return _isCatalog ;
      }

      INT32 attachNode( const CHAR *pHostName,
                        const CHAR *pSvcName,
                        const bson::BSONObj &options = _sdbStaticObject ) ;
      INT32 detachNode( const CHAR *pHostName,
                        const CHAR *pSvcName,
                        const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 reelect( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 reelectLocation( const CHAR *pLocation,
                             const BSONObj &options = _sdbStaticObject ) ;

      INT32 setActiveLocation ( const CHAR *pLocation ) ;

      INT32 setAttributes ( const BSONObj & options = _sdbStaticObject ) ;

      INT32 startCriticalMode( const BSONObj & options = _sdbStaticObject ) ;

      INT32 stopCriticalMode() ;

      INT32 startMaintenanceMode( const BSONObj & options = _sdbStaticObject ) ;

      INT32 stopMaintenanceMode( const BSONObj & options = _sdbStaticObject ) ;

   protected:
      INT32 _innerAlter ( const CHAR * taskName,
                          const BSONObj * pOptions,
                          BOOLEAN allowNullArgs ) ;
   } ;

   typedef class _sdbReplicaGroupImpl sdbReplicaGroupImpl ;

   /*
      _sdbCollectionSpaceImpl
   */
   class _sdbCollectionSpaceImpl : public _sdbCollectionSpace, public _sdbBase
   {
   private :
      _sdbCollectionSpaceImpl ( const _sdbCollectionSpaceImpl& other ) ;
      _sdbCollectionSpaceImpl& operator=( const _sdbCollectionSpaceImpl& ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch _mutex ;
#endif

      CHAR _collectionSpaceName [ CLIENT_CS_NAMESZ+1 ] ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32 _setName ( const CHAR *pCollectionSpaceName ) ;
      INT32 _getRetVersion () ;
      INT32 _runCommand ( const CHAR *pString,
                          const BSONObj *arg1 = NULL,
                          const BSONObj *arg2 = NULL,
                          const BSONObj *arg3 = NULL,
                          const BSONObj *arg4 = NULL,
                          SINT32 flag = 0,
                          UINT64 reqID = 0,
                          SINT64 numToSkip = 0,
                          SINT64 numToReturn = -1,
                          _sdbCursor **ppCursor = NULL ) ;

      friend class _sdbImpl ;

   public :
      _sdbCollectionSpaceImpl () ;
      _sdbCollectionSpaceImpl ( CHAR *pCollectionSpaceName ) ;
      ~_sdbCollectionSpaceImpl () ;
      // get a collection object
      INT32 getCollection ( const CHAR *pCollectionName,
                            _sdbCollection **collection,
                            BOOLEAN checkExist = TRUE ) ;
      INT32 getCollection ( const CHAR *pCollectionName,
                            sdbCollection &collection,
                            BOOLEAN checkExist = TRUE )
      {
         RELEASE_INNER_HANDLE( collection.pCollection ) ;
         return getCollection ( pCollectionName,
                                &collection.pCollection,
                                checkExist ) ;
      }
      // create a new collection object
      INT32 createCollection ( const CHAR *pCollection,
                               _sdbCollection **collection ) ;
      INT32 createCollection ( const CHAR *pCollection,
                               sdbCollection &collection )
      {
         RELEASE_INNER_HANDLE( collection.pCollection ) ;
         return createCollection ( pCollection,
                                   &collection.pCollection ) ;
      }
      // create a new shareded collection object
      INT32 createCollection ( const CHAR *pCollection,
                               const BSONObj &options,
                               _sdbCollection **collection ) ;
      INT32 createCollection ( const CHAR *pCollection,
                               const BSONObj &options,
                               sdbCollection &collection )
      {
         RELEASE_INNER_HANDLE( collection.pCollection ) ;
         return createCollection ( pCollection,
                                   options,
                                   &collection.pCollection ) ;
      }

      // drop an existing collection with options
      INT32 dropCollection( const CHAR *pCollection,
                            const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 listCollections ( _sdbCursor **cursor ) ;

      INT32 listCollections ( sdbCursor &cursor )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listCollections ( &cursor.pCursor ) ;
      }
      // create a collection space with current collection space name
      INT32 create () ;
      // drop a collection space with current collection space name
      INT32 drop () ;

      const CHAR *getCSName ()
      {
         return &_collectionSpaceName[0] ;
      }

      INT32 renameCollection( const CHAR * oldName, const CHAR * newName,
                              const BSONObj & options = _sdbStaticObject ) ;

      INT32 alterCollectionSpace ( const BSONObj & options ) ;

      INT32 setDomain ( const BSONObj & options ) ;

      INT32 getDomainName ( CHAR *result, INT32 resultLen ) ;

      INT32 removeDomain () ;

      INT32 enableCapped () ;

      INT32 disableCapped () ;

      INT32 setAttributes ( const bson::BSONObj & options ) ;

   protected :
      INT32 _alterInternal ( const CHAR * taskName,
                             const BSONObj * arguments,
                             BOOLEAN allowNullArgs ) ;
   } ;

   typedef class _sdbCollectionSpaceImpl sdbCollectionSpaceImpl ;

   /*
      _sdbDomainImpl
   */
   class _sdbDomainImpl : public _sdbDomain, public _sdbBase
   {
   private :
      _sdbDomainImpl ( const _sdbDomainImpl& other ) ;
      _sdbDomainImpl& operator= ( const _sdbDomainImpl& other ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch           _mutex ;
#endif

      CHAR _domainName[ CLIENT_DOMAIN_NAMESZ+1 ] ;

    private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }
   private:
      INT32 _setName ( const CHAR *pDomainName ) ;

      friend class _sdbImpl ;
   public :
      _sdbDomainImpl () ;
      _sdbDomainImpl ( const CHAR *pDomainName ) ;
      ~_sdbDomainImpl () ;

      const CHAR* getName ()
      {
         return _domainName ;
      }

      INT32 alterDomain ( const bson::BSONObj &options ) ;

      INT32 listCollectionSpacesInDomain ( _sdbCursor **cursor ) ;

      INT32 listCollectionSpacesInDomain ( sdbCursor &cursor )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listCollectionSpacesInDomain ( &cursor.pCursor ) ;
      }

      INT32 listCollectionsInDomain ( _sdbCursor **cursor ) ;

      INT32 listCollectionsInDomain ( sdbCursor &cursor )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listCollectionsInDomain ( &cursor.pCursor ) ;
      }

      INT32 listReplicaGroupInDomain( _sdbCursor **cursor ) ;

      INT32 listReplicaGroupInDomain( sdbCursor &cursor )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listReplicaGroupInDomain ( &cursor.pCursor ) ;
      }

      INT32 addGroups ( const bson::BSONObj & options ) ;

      INT32 setGroups ( const bson::BSONObj & options ) ;

      INT32 removeGroups ( const bson::BSONObj & options ) ;

      INT32 setActiveLocation ( const CHAR *pLocation ) ;

      INT32 setLocation ( const CHAR *pHostName, const CHAR *pLocation ) ;

      INT32 setAttributes ( const bson::BSONObj & options ) ;

   protected :
      INT32 _alterInternal ( const CHAR * taskName,
                             const bson::BSONObj * arguments,
                             BOOLEAN allowNullArgs ) ;
      INT32 _alterDomainV1 ( const bson::BSONObj & options ) ;
      INT32 _alterDomainV2 ( const bson::BSONObj & options ) ;
   } ;
   typedef class _sdbDomainImpl sdbDomainImpl ;

   /*
      _sdbDataCenterImpl
   */
   class _sdbDataCenterImpl : public _sdbDataCenter, public _sdbBase
   {
      friend class _sdbImpl ;

   private :
      _sdbDataCenterImpl ( const _sdbDataCenterImpl& other ) ;
      _sdbDataCenterImpl& operator= ( const _sdbDataCenterImpl& other ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch           _mutex ;
#endif

      CHAR _dcName[ CLIENT_DC_NAMESZ+1 ] ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32 _setName ( const CHAR *pClusterName,
                       const CHAR *pBusinessName ) ;

   public :
      _sdbDataCenterImpl () ;
      ~_sdbDataCenterImpl () ;

   public :
      const CHAR* getName ()
      {
         return _dcName ;
      }
      INT32 getDetail( bson::BSONObj &retInfo ) ;
      INT32 activateDC() ;
      INT32 deactivateDC() ;
      INT32 enableReadOnly( BOOLEAN isReadOnly ) ;
      INT32 createImage( const CHAR *pCataAddrList ) ;
      INT32 removeImage() ;
      INT32 enableImage() ;
      INT32 disableImage() ;
      INT32 attachGroups( const bson::BSONObj &info ) ;
      INT32 detachGroups( const bson::BSONObj &info ) ;
      INT32 setActiveLocation ( const CHAR *pLocation ) ;
      INT32 setLocation ( const CHAR *pHostName, const CHAR *pLocation ) ;
      INT32 startMaintenanceMode( const BSONObj & options ) ;
      INT32 stopMaintenanceMode( const BSONObj & options ) ;

   private :
      INT32 _innerAlter( const CHAR *pValue,
                         const bson::BSONObj *pInfo = NULL ) ;

   } ;
   typedef class _sdbDataCenterImpl sdbDataCenterImpl ;

   /*
      _sdbRecycleBin
    */
   class _sdbRecycleBinImpl : public _sdbRecycleBin, public _sdbBase
   {
      friend class _sdbImpl ;

   private:
      _sdbRecycleBinImpl( const _sdbRecycleBinImpl &other ) ;
      _sdbRecycleBinImpl &operator =( const _sdbRecycleBinImpl &other ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch           _mutex ;
#endif

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   protected:
      INT32 _innerAlter( const bson::BSONObj &options ) ;
      INT32 _innerCMD( const CHAR *command,
                       const bson::BSONObj &options,
                       _sdbCursor **cursor = NULL ) ;

   public:
      _sdbRecycleBinImpl() ;
      virtual ~_sdbRecycleBinImpl() ;

      virtual INT32 getDetail( bson::BSONObj &retInfo ) ;
      virtual INT32 enable() ;
      virtual INT32 disable() ;
      virtual INT32 setAttributes( const bson::BSONObj &options ) ;
      virtual INT32 alter( const bson::BSONObj &options ) ;
      virtual INT32 returnItem( const CHAR *recycleName,
                                const bson::BSONObj &options = _sdbStaticObject,
                                bson::BSONObj *result = NULL ) ;
      virtual INT32 returnItemToName( const CHAR *recycleName,
                                      const CHAR *returnName,
                                      const bson::BSONObj &options = _sdbStaticObject,
                                      bson::BSONObj *result = NULL ) ;
      virtual INT32 dropItem( const CHAR *recycleName,
                              const bson::BSONObj &options = _sdbStaticObject ) ;
      virtual INT32 dropAll( const bson::BSONObj &options = _sdbStaticObject ) ;
      virtual INT32 list( _sdbCursor **cursor,
                          const bson::BSONObj &condition = _sdbStaticObject,
                          const bson::BSONObj &selector = _sdbStaticObject,
                          const bson::BSONObj &orderBy = _sdbStaticObject,
                          const bson::BSONObj &hint = _sdbStaticObject,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) ;
      virtual INT32 list( sdbCursor &cursor,
                          const bson::BSONObj &condition = _sdbStaticObject,
                          const bson::BSONObj &selector = _sdbStaticObject,
                          const bson::BSONObj &orderBy = _sdbStaticObject,
                          const bson::BSONObj &hint = _sdbStaticObject,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return list( &( cursor.pCursor ),
                      condition,
                      selector,
                      orderBy,
                      hint,
                      numToSkip,
                      numToReturn ) ;
      }
      virtual INT32 snapshot( _sdbCursor **cursor,
                              const bson::BSONObj &condition = _sdbStaticObject,
                              const bson::BSONObj &selector = _sdbStaticObject,
                              const bson::BSONObj &orderBy = _sdbStaticObject,
                              const bson::BSONObj &hint = _sdbStaticObject,
                              INT64 numToSkip = 0,
                              INT64 numToReturn = -1 ) ;
      virtual INT32 snapshot( sdbCursor &cursor,
                              const bson::BSONObj &condition = _sdbStaticObject,
                              const bson::BSONObj &selector = _sdbStaticObject,
                              const bson::BSONObj &orderBy = _sdbStaticObject,
                              const bson::BSONObj &hint = _sdbStaticObject,
                              INT64 numToSkip = 0,
                              INT64 numToReturn = -1 )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return snapshot( &( cursor.pCursor ),
                          condition,
                          selector,
                          orderBy,
                          hint,
                          numToSkip,
                          numToReturn ) ;
      }

      virtual INT32 getCount( INT64 &count,
                              const bson::BSONObj &condition = _sdbStaticObject ) ;
   } ;
   typedef class _sdbRecycleBinImpl sdbRecycleBinImpl ;

   /*
      _sdbLobImpl
   */
   class _sdbLobImpl : public _sdbLob, public _sdbBase
   {
   private :
      _sdbLobImpl ( const _sdbLobImpl& other ) ;
      _sdbLobImpl& operator= ( const _sdbLobImpl& other ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch           _mutex ;
#endif

      BOOLEAN                 _isOpen ;
      SINT64                  _contextID ;
      INT32                   _mode ;
      bson::OID                _oid ;
      UINT64                  _createTime ;
      UINT64                  _modificationTime ;
      SINT64                  _lobSize ;
      SINT64                  _currentOffset ;
      SINT64                  _cachedOffset ;
      UINT32                  _cachedSize ;
      UINT32                  _pageSize ;
      UINT32                  _flag ;
      INT32                   _piecesInfoNum ;
      bson::BSONArray         _piecesInfo ;
      const CHAR              *_dataCache ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private :
      void _close () ;
      BOOLEAN _dataCached() ;
      void _readInCache( void *buf, UINT32 len, UINT32 *read ) ;
      UINT32 _reviseReadLen( UINT32 needLen ) ;
      INT32 _onceRead( CHAR *buf, UINT32 len, UINT32 *read ) ;

      friend class _sdbImpl ;
      friend class _sdbCollectionImpl ;
   public :
      _sdbLobImpl () ;
      ~_sdbLobImpl () ;

      virtual INT32 close () ;

      virtual INT32 read ( UINT32 len, CHAR *buf, UINT32 *read ) ;

      virtual INT32 write ( const CHAR *buf, UINT32 len ) ;

      virtual INT32 seek ( SINT64 size, SDB_LOB_SEEK whence ) ;

      virtual INT32 lock( INT64 offset, INT64 length ) ;

      virtual INT32 lockAndSeek( INT64 offset, INT64 length ) ;

      virtual INT32 isClosed( BOOLEAN &flag ) ;

      virtual INT32 getOid( bson::OID &oid ) ;

      virtual INT32 getSize( SINT64 *size ) ;

      virtual INT32 getCreateTime ( UINT64 *millis ) ;

      virtual BOOLEAN isClosed() ;

      virtual bson::OID getOid() ;

      virtual SINT64 getSize() ;

      virtual UINT64 getCreateTime () ;

      virtual UINT64 getModificationTime() ;

      virtual INT32 getPiecesInfoNum() ;

      virtual bson::BSONArray getPiecesInfo() ;

      virtual BOOLEAN isEof() ;

      virtual INT32 getRunTimeDetail( bson::BSONObj &detail ) ;

   } ;

   typedef class _sdbLobImpl sdbLobImpl ;

   /*
      _sdbSequenceImpl
   */
   class _sdbSequenceImpl : public _sdbSequence, public _sdbBase
   {
      friend class _sdbImpl ;

   private :
      _sdbSequenceImpl ( const _sdbSequenceImpl &other ) ;
      _sdbSequenceImpl& operator=( const _sdbSequenceImpl& ) ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32 _setName ( const CHAR *pSequenceName ) ;
      INT32 _alterInternal ( const CHAR *actionName,
                             const bson::BSONObj &arguments ) ;

   private:
#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch           _mutex ;
#endif

      CHAR                    *_pSequenceName ;

   public :
      _sdbSequenceImpl () ;
      ~_sdbSequenceImpl () ;

      INT32 setAttributes ( const bson::BSONObj &options ) ;

      INT32 getNextValue ( INT64 &value ) ;

      INT32 getCurrentValue ( INT64 &value ) ;

      INT32 setCurrentValue ( const INT64 value ) ;

      INT32 fetch( const INT32 fetchNum, INT64 &nextValue,
                   INT32 &returnNum, INT32 &Increment ) ;

      INT32 restart( const INT64 startValue ) ;
   } ;
   typedef class _sdbSequenceImpl sdbSequenceImpl ;

   class _sdbDataSourceImpl : public _sdbDataSource, public _sdbBase
   {
   private:
      _sdbDataSourceImpl( const _sdbDataSourceImpl& other ) ;
      _sdbDataSourceImpl& operator=( const _sdbDataSourceImpl& ) ;

#if defined CLIENT_THREAD_SAFE
      ossSpinSLatch            _mutex ;
#endif

      CHAR               _dataSourceName[ CLIENT_DATASOURCE_NAMESZ + 1 ] ;

   private:
      virtual INT32 _setConnection( _sdbImpl *connection )
      {
         return _regHandle( connection, (ossValuePtr)this ) ;
      }
      virtual void _dropConnection()
      {
         _unregHandle( (ossValuePtr)this ) ;
      }

   private:
      INT32 _setName( const CHAR *pDataSourceName ) ;
      INT32 _appendOptions( BSONObjBuilder &builder, const BSONObj &options ) ;

      friend class _sdbImpl ;

   public:
      _sdbDataSourceImpl() ;
      _sdbDataSourceImpl( const CHAR *pDataSourceName ) ;
      ~_sdbDataSourceImpl() ;

      INT32 alterDataSource( const bson::BSONObj &options = _sdbStaticObject ) ;
      const CHAR *getName()
      {
         return _dataSourceName ;
      }
   } ;

   typedef _sdbDataSourceImpl sdbDataSourceImpl ;

   /*
      _sdbImpl
   */
   class _sdbImpl : public _sdb
   {
   private :
      _sdbImpl ( const _sdbImpl& other ) ;
      _sdbImpl& operator=( const _sdbImpl& ) ;

      ossSpinSLatch            _mutex ;
      ossSocket               *_sock ;
      CHAR                     _hostName [ OSS_MAX_HOSTNAME + 1 ] ;
      CHAR                     _serviceName [ OSS_MAX_SERVICENAME + 1 ] ;
      CHAR                     _address [ OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 2 ] ;
      CHAR                    *_pSendBuffer ;
      INT32                    _sendBufferSize ;
      CHAR                    *_pReceiveBuffer ;
      INT32                    _receiveBufferSize ;
      BOOLEAN                  _endianConvert ;
      UINT64                   _dbStartTime ;
      UINT8                    _version ;
      UINT8                    _subVersion ;
      UINT8                    _fixVersion ;
      BOOLEAN                  _useSSL ;
      std::set<ossValuePtr>    _cursors ;
      std::set<ossValuePtr>    _collections ;
      std::set<ossValuePtr>    _collectionspaces ;
      std::set<ossValuePtr>    _nodes ;
      std::set<ossValuePtr>    _replicaGroups ;
      std::set<ossValuePtr>    _domains ;
      std::set<ossValuePtr>    _dataCenters ;
      std::set<ossValuePtr>    _lobs ;
      std::set<ossValuePtr>    _sequences ;
      std::set<ossValuePtr>    _dataSources ;
      std::set<ossValuePtr>    _recycleBinSet ;
      hashTable               *_tb ;
      // If the authVersion is 0, we use MD5 authentication.
      // And if the authVersion is 1, we use SCRAM-SHA256 authentication.
      INT32                    _authVersion ;
      INT16                    _peerProtocolVersion ;
      _sdbMsgConvertor        *_msgConvertor ;
      bson::BSONObj            _attributeCache ;

      const CHAR*              _pErrorBuf ;
      INT32                    _errorBufSize ;

      const CHAR*              _pResultBuf ;
      INT32                    _resultBufSize ;

      // last send or recive time
      ossTimestamp             _lastAliveTime ;

      BOOLEAN                  _isOldVersionLobServer ;

      void _disconnect () ;
      void _removeObjects() ;
      void _setErrorBuffer( const CHAR *pBuf, INT32 bufSize ) ;
      void _setResultBuffer( const CHAR *pBuf, INT32 bufSize ) ;
      INT32 _send ( CHAR *pBuffer ) ;
      INT32 _recv ( CHAR **ppBuffer, INT32 *size ) ;

      INT32 _recvExtract ( CHAR **ppBuffer,
                           INT32 *size,
                           SINT64 &contextID,
                           BOOLEAN *pRemoteErr = NULL,
                           BOOLEAN *pHasRecv = NULL ) ;

      INT32 _reallocBuffer ( CHAR **ppBuffer,
                             INT32 *size,
                             INT32 newSize ) ;

      INT32 _getRetInfo ( CHAR **ppBuffer,
                          INT32 *size,
                          SINT64 contextID,
                          _sdbCursor **ppCursor ) ;
      INT32 _getRetVersion () ;

      INT32 _runCommand ( const CHAR *pString,
                          const BSONObj *arg1 = NULL,
                          const BSONObj *arg2 = NULL,
                          const BSONObj *arg3 = NULL,
                          const BSONObj *arg4 = NULL,
                          SINT32 flag = 0,
                          UINT64 reqID = 0,
                          SINT64 numToSkip = 0,
                          SINT64 numToReturn = -1,
                          _sdbCursor **ppCursor = NULL ) ;

      INT32 _sendAndRecv( const CHAR *pSendBuf,
                          CHAR **ppRecvBuf,
                          INT32 *recvBufSize,
                          _sdbCursor **ppCursor = NULL,
                          BOOLEAN needLock = TRUE ) ;

      INT32 _buildEmptyCursor( _sdbCursor **ppCursor ) ;
      INT32 _requestSysInfo () ;
      INT32 _regAndUnregHandle ( CLIENT_CLASS_TYPE type,
                                 ossValuePtr handle,
                                 BOOLEAN isRegister ) ;
      INT32 _registerHandle ( CLIENT_CLASS_TYPE type, ossValuePtr handle ) ;
      INT32 _unregisterHandle ( CLIENT_CLASS_TYPE type, ossValuePtr handle ) ;

      hashTable* _getCachedContainer() const ;

      INT32 _connect( const CHAR *pHostName, UINT16 port ) ;

      INT32 _traceStrtok( BSONArrayBuilder &arrayBuilder, const CHAR* pLine ) ;

      void _clearSessionAttrCache ( BOOLEAN needLock ) ;

      void _setSessionAttrCache ( const bson::BSONObj & attribute ) ;

      void _getSessionAttrCache ( bson::BSONObj & attribute ) ;

      BOOLEAN _getIsOldVersionLobServer() ;
      void _setIsOldVersionLobServer( BOOLEAN isOldVersionLobServer ) ;

      INT32 _authVer0MsgProcess( const CHAR *pUsrName, const CHAR *pPasswd ) ;

      INT32 _authVer1MsgProcess( const CHAR *pUsrName, const CHAR *pPasswd ) ;
      INT32 _step1( const CHAR *pUsrName, const CHAR *pPasswd,
                    UINT32 &iterationCount,
                    string &saltBase64,
                    string &combineNonceBase64,
                    BOOLEAN &needAuth ) ;
      INT32 _step2( const CHAR *pUsrName,
                    const CHAR *pPasswd,
                    UINT32 iterationCount,
                    const string &saltBase64,
                    const string &combineNonceBase64,
                    const string &clientProofBase64 ) ;

      friend class _sdbBase ;
      friend class _sdbCollectionSpaceImpl ;
      friend class _sdbCollectionImpl ;
      friend class _sdbCursorImpl ;
      friend class _sdbNodeImpl ;
      friend class _sdbReplicaGroupImpl ;
      friend class _sdbDomainImpl ;
      friend class _sdbDataCenterImpl ;
      friend class _sdbLobImpl ;
      friend class _sdbSequenceImpl ;
      friend class _sdbDataSourceImpl ;
      friend class _sdbRecycleBinImpl ;
   public :
      _sdbImpl ( BOOLEAN useSSL = FALSE ) ;
      ~_sdbImpl () ;
      INT32 connect ( const CHAR *pHostName,
                      UINT16 port ) ;
      INT32 connect ( const CHAR *pHostName,
                      UINT16 port,
                      const CHAR *pUsrName,
                      const CHAR *pPasswd ) ;
      INT32 connect ( const CHAR *pHostName,
                      const CHAR *pServiceName ) ;
      INT32 connect ( const CHAR *pHostName,
                      const CHAR *pServiceName,
                      const CHAR *pUsrName,
                      const CHAR *pPasswd ) ;
      INT32 connect ( const CHAR **pConnAddrs,
                      INT32 arrSize,
                      const CHAR *pUsrName,
                      const CHAR *pPasswd ) ;
      INT32 connect ( const CHAR **pConnAddrs,
                      INT32 arrSize,
                      const CHAR *pUsrName,
                      const CHAR *pToken,
                      const CHAR *pCipherFile ) ;

      void disconnect () ;
      BOOLEAN isConnected ()
      { return NULL != _sock ; }

      UINT64 getDbStartTime() { return _dbStartTime ; }

      const CHAR *getAddress()
      {
         return _address ;
      }

      void getVersion( UINT8 &version, UINT8 &subVersion, UINT8 &fixVersion )
      {
         version = _version ;
         subVersion = _subVersion ;
         fixVersion = _fixVersion ;
      }

      void initCacheStrategy( BOOLEAN enableCacheStrategy,
                              const UINT32 cacheTimeInterval,
                              const UINT32 maxCacheSlotCount ) ;

      INT32 createUsr( const CHAR *pUsrName,
                       const CHAR *pPasswd,
                       const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 alterUsr( const CHAR *pUsrName,
                      const CHAR *pAction,
                      const bson::BSONObj &options ) ;

      INT32 changeUsrPasswd( const CHAR *pUsrName,
                             const CHAR *pOldPasswd,
                             const CHAR *pNewPasswd ) ;

      INT32 removeUsr( const CHAR *pUsrName,
                       const CHAR *pPasswd ) ;

      INT32 getSnapshot ( _sdbCursor **cursor,
                          INT32 snapType,
                          const BSONObj &condition = _sdbStaticObject,
                          const BSONObj &selector  = _sdbStaticObject,
                          const BSONObj &orderBy   = _sdbStaticObject,
                          const BSONObj &hint      = _sdbStaticObject,
                          INT64 numToSkip    = 0,
                          INT64 numToReturn  = -1 ) ;

      INT32 getSnapshot ( sdbCursor &cursor,
                          INT32 snapType,
                          const BSONObj &condition = _sdbStaticObject,
                          const BSONObj &selector  = _sdbStaticObject,
                          const BSONObj &orderBy   = _sdbStaticObject,
                          const BSONObj &hint      = _sdbStaticObject,
                          INT64 numToSkip    = 0,
                          INT64 numToReturn  = -1 )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return getSnapshot ( &cursor.pCursor,
                              snapType,
                              condition, selector, orderBy, hint,
                              numToSkip, numToReturn ) ;
      }

      INT32 getList ( _sdbCursor **cursor,
                      INT32 listType,
                      const BSONObj &condition  = _sdbStaticObject,
                      const BSONObj &selector   = _sdbStaticObject,
                      const BSONObj &orderBy    = _sdbStaticObject,
                      const bson::BSONObj &hint = _sdbStaticObject,
                      INT64 numToSkip   = 0,
                      INT64 numToReturn = -1
                    ) ;

      INT32 getList ( sdbCursor &cursor,
                      INT32 listType,
                      const BSONObj &condition  = _sdbStaticObject,
                      const BSONObj &selector   = _sdbStaticObject,
                      const BSONObj &orderBy    = _sdbStaticObject,
                      const bson::BSONObj &hint = _sdbStaticObject,
                      INT64 numToSkip   = 0,
                      INT64 numToReturn = -1
                    )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return getList ( &cursor.pCursor, listType, condition,
                          selector, orderBy, hint,
                          numToSkip, numToReturn ) ;
      }

      INT32 resetSnapshot ( const BSONObj &options = _sdbStaticObject ) ;

      #if defined CLIENT_THREAD_SAFE
      void lock ()
      {
         _mutex.get () ;
      }
      void unlock ()
      {
         _mutex.release () ;
      }
      #else
      void lock ()
      {
      }
      void unlock ()
      {
      }
      #endif
      INT32 getCollection ( const CHAR *pCollectionFullName,
                            _sdbCollection **collection,
                            BOOLEAN checkExist = TRUE ) ;

      INT32 getCollection ( const CHAR *pCollectionFullName,
                            sdbCollection &collection,
                            BOOLEAN checkExist = TRUE )
      {
         RELEASE_INNER_HANDLE( collection.pCollection ) ;
         return getCollection ( pCollectionFullName,
                                &collection.pCollection,
                                checkExist ) ;
      }

      INT32 getCollectionSpace ( const CHAR *pCollectionSpaceName,
                                 _sdbCollectionSpace **cs,
                                 BOOLEAN checkExist = TRUE ) ;

      INT32 getCollectionSpace ( const CHAR *pCollectionSpaceName,
                                 sdbCollectionSpace &cs,
                                 BOOLEAN checkExist = TRUE )
      {
         RELEASE_INNER_HANDLE( cs.pCollectionSpace ) ;
         return getCollectionSpace ( pCollectionSpaceName,
                                     &cs.pCollectionSpace,
                                     checkExist) ;
      }

      INT32 createCollectionSpace ( const CHAR *pCollectionSpaceName,
                                    INT32 iPageSize,
                                    _sdbCollectionSpace **cs ) ;

      INT32 createCollectionSpace ( const CHAR *pCollectionSpaceName,
                                    INT32 iPageSize,
                                    sdbCollectionSpace &cs )
      {
         RELEASE_INNER_HANDLE( cs.pCollectionSpace ) ;
         return createCollectionSpace ( pCollectionSpaceName, iPageSize,
                                        &cs.pCollectionSpace ) ;
      }

      INT32 createCollectionSpace ( const CHAR *pCollectionSpaceName,
                                    const bson::BSONObj &options,
                                    _sdbCollectionSpace **cs
                                  ) ;

      INT32 createCollectionSpace ( const CHAR *pCollectionSpaceName,
                                    const bson::BSONObj &options,
                                    sdbCollectionSpace &cs
                                  )
      {
         RELEASE_INNER_HANDLE( cs.pCollectionSpace ) ;
         return createCollectionSpace ( pCollectionSpaceName, options,
                                        &cs.pCollectionSpace ) ;
      }

      INT32 dropCollectionSpace (
                             const CHAR *pCollectionSpaceName,
                             const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 listCollectionSpaces ( _sdbCursor **result ) ;

      INT32 listCollectionSpaces ( sdbCursor &result )
      {
         RELEASE_INNER_HANDLE( result.pCursor ) ;
         return listCollectionSpaces ( &result.pCursor ) ;
      }

      INT32 listCollections ( _sdbCursor **result ) ;

      INT32 listCollections ( sdbCursor &result )
      {
         RELEASE_INNER_HANDLE( result.pCursor ) ;
         return listCollections ( &result.pCursor ) ;
      }

      INT32 listSequences ( _sdbCursor **result ) ;

      INT32 listSequences ( sdbCursor &result )
      {
         RELEASE_INNER_HANDLE( result.pCursor ) ;
         return listSequences( &result.pCursor ) ;
      }

      INT32 listReplicaGroups ( _sdbCursor **result ) ;

      INT32 listReplicaGroups ( sdbCursor &result )
      {
         RELEASE_INNER_HANDLE( result.pCursor ) ;
         return listReplicaGroups ( &result.pCursor ) ;
      }

      INT32 getReplicaGroup ( const CHAR *pName, _sdbReplicaGroup **result ) ;

      INT32 getReplicaGroup ( const CHAR *pName, sdbReplicaGroup &result )
      {
         RELEASE_INNER_HANDLE( result.pReplicaGroup ) ;
         return getReplicaGroup ( pName, &result.pReplicaGroup ) ;
      }

      INT32 getReplicaGroup ( INT32 id, _sdbReplicaGroup **result ) ;

      INT32 getReplicaGroup ( INT32 id, sdbReplicaGroup &result )
      {
         RELEASE_INNER_HANDLE( result.pReplicaGroup ) ;
         return getReplicaGroup ( id, &result.pReplicaGroup ) ;
      }

      INT32 createReplicaGroup ( const CHAR *pName,
                                 _sdbReplicaGroup **replicaGroup ) ;

      INT32 createReplicaGroup ( const CHAR *pName,
                                 sdbReplicaGroup &replicaGroup )
      {
         RELEASE_INNER_HANDLE( replicaGroup.pReplicaGroup ) ;
         return createReplicaGroup ( pName, &replicaGroup.pReplicaGroup ) ;
      }

      INT32 removeReplicaGroup ( const CHAR *pName ) ;

      INT32 createReplicaCataGroup (  const CHAR *pHostName,
                                      const CHAR *pServiceName,
                                      const CHAR *pDatabasePath,
                                      const BSONObj &configure ) ;

      INT32 activateReplicaGroup ( const CHAR *pName,
                                   _sdbReplicaGroup **replicaGroup ) ;
      INT32 activateReplicaGroup ( const CHAR *pName,
                                   sdbReplicaGroup &replicaGroup )
      {
         RELEASE_INNER_HANDLE( replicaGroup.pReplicaGroup ) ;
         return activateReplicaGroup( pName, &replicaGroup.pReplicaGroup ) ;
      }

      // sql
      INT32 execUpdate( const CHAR *sql, bson::BSONObj *pResult = NULL ) ;
      INT32 exec( const CHAR *sql, sdbCursor &result )
      {
         RELEASE_INNER_HANDLE( result.pCursor ) ;
         return exec( sql, &result.pCursor ) ;
      }
      INT32 exec( const CHAR *sql, _sdbCursor **result ) ;

      // transation
      INT32 transactionBegin() ;
      INT32 transactionCommit( const bson::BSONObj &hint =  _sdbStaticObject ) ;
      INT32 transactionRollback() ;

      // flush config file
      INT32 flushConfigure( const bson::BSONObj &options ) ;

      // stored procedure
      INT32 crtJSProcedure ( const CHAR *code ) ;
      INT32 rmProcedure( const CHAR *spName ) ;
      INT32 listProcedures( _sdbCursor **cursor,
                            const bson::BSONObj &condition ) ;
      INT32 listProcedures( sdbCursor &cursor,
                            const bson::BSONObj &condition )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listProcedures ( &cursor.pCursor, condition ) ;
      }

      INT32 evalJS( const CHAR *code,
                    SDB_SPD_RES_TYPE &type,
                    _sdbCursor **cursor,
                    bson::BSONObj &errmsg ) ;
      INT32 evalJS( const CHAR *code,
                    SDB_SPD_RES_TYPE &type,
                    sdbCursor &cursor,
                    bson::BSONObj &errmsg )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return evalJS( code, type, &cursor.pCursor, errmsg ) ;
      }

      // bakeup
      INT32 backup ( const bson::BSONObj &options) ;
      INT32 listBackup ( _sdbCursor **cursor,
                         const bson::BSONObj &options,
                         const bson::BSONObj &condition = _sdbStaticObject,
                         const bson::BSONObj &selector = _sdbStaticObject,
                         const bson::BSONObj &orderBy = _sdbStaticObject) ;
      INT32 listBackup ( sdbCursor &cursor,
                         const bson::BSONObj &options,
                         const bson::BSONObj &condition = _sdbStaticObject,
                         const bson::BSONObj &selector = _sdbStaticObject,
                         const bson::BSONObj &orderBy = _sdbStaticObject)
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listBackup ( &cursor.pCursor, options, condition, selector, orderBy ) ;
      }
      INT32 removeBackup ( const bson::BSONObj &options ) ;

      // task
      INT32 listTasks ( _sdbCursor **cursor,
                        const bson::BSONObj &condition = _sdbStaticObject,
                        const bson::BSONObj &selector = _sdbStaticObject,
                        const bson::BSONObj &orderBy = _sdbStaticObject,
                        const bson::BSONObj &hint = _sdbStaticObject) ;
      INT32 listTasks ( sdbCursor &cursor,
                        const bson::BSONObj &condition = _sdbStaticObject,
                        const bson::BSONObj &selector = _sdbStaticObject,
                        const bson::BSONObj &orderBy = _sdbStaticObject,
                        const bson::BSONObj &hint = _sdbStaticObject)
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listTasks ( &cursor.pCursor, condition,
                             selector, orderBy, hint ) ;
      }

      INT32 waitTasks ( const SINT64 *taskIDs, SINT32 num ) ;

      INT32 cancelTask ( SINT64 taskID, BOOLEAN isAsync ) ;
      // set session attribute
      INT32 setSessionAttr ( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 getSessionAttr ( bson::BSONObj &attribute, BOOLEAN useCache = TRUE ) ;

      // close all cursors
      INT32 closeAllCursors ();

      // interrupt
      INT32 interrupt() ;
      INT32 interruptOperation() ;

      // connection is closed
      INT32   isValid( BOOLEAN *result ) ;
      BOOLEAN isValid() ;
      BOOLEAN isClosed() ;

      // domain
      INT32 createDomain ( const CHAR *pDomainName,
                           const bson::BSONObj &options,
                           _sdbDomain **domain ) ;

      INT32 createDomain ( const CHAR *pDomainName,
                           const bson::BSONObj &options,
                           sdbDomain &domain )
      {
         RELEASE_INNER_HANDLE( domain.pDomain ) ;
         return createDomain ( pDomainName, options, &domain.pDomain ) ;
      }

      INT32 dropDomain ( const CHAR *pDomainName ) ;

      INT32 getDomain ( const CHAR *pDomainName,
                        _sdbDomain **domain ) ;

      INT32 getDomain ( const CHAR *pDomainName,
                        sdbDomain &domain )
      {
         RELEASE_INNER_HANDLE( domain.pDomain ) ;
         return getDomain ( pDomainName, &domain.pDomain ) ;
      }

      INT32 listDomains ( _sdbCursor **cursor,
                          const bson::BSONObj &condition,
                          const bson::BSONObj &selector,
                          const bson::BSONObj &orderBy,
                          const bson::BSONObj &hint
                         ) ;

      INT32 listDomains ( sdbCursor &cursor,
                          const bson::BSONObj &condition,
                          const bson::BSONObj &selector,
                          const bson::BSONObj &orderBy,
                          const bson::BSONObj &hint
                         )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return listDomains ( &cursor.pCursor, condition, selector, orderBy, hint ) ;
      }

      INT32 getDC( _sdbDataCenter **dc ) ;

      INT32 getDC( sdbDataCenter &dc )
      {
         RELEASE_INNER_HANDLE( dc.pDC ) ;
         return getDC( &dc.pDC ) ;
      }

      INT32 getRecycleBin( _sdbRecycleBin **recycleBin ) ;

      INT32 getRecycleBin( sdbRecycleBin &recycleBin )
      {
         RELEASE_INNER_HANDLE( recycleBin.pRecycleBin ) ;
         return getRecycleBin( &( recycleBin.pRecycleBin ) ) ;
      }

      // get last alive time
      UINT64 getLastAliveTime() const { return _lastAliveTime.time; }

      INT32 syncDB( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 analyze( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 forceSession( SINT64 sessionID,
                          const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 forceStepUp( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 invalidateCache( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 reloadConfig( const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 updateConfig( const bson::BSONObj &configs,
                          const bson::BSONObj &options ) ;

      INT32 deleteConfig( const bson::BSONObj &configs,
                          const bson::BSONObj &options ) ;

      INT32 setPDLevel( INT32 level,
                        const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 memTrim( const CHAR *maskStr = "",
                     const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 msg( const CHAR* msg ) ;

      INT32 loadCS( const CHAR* csName,
                    const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 unloadCS( const CHAR* csName,
                      const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 traceStart( UINT32 traceBufferSize, const CHAR* component = NULL,
                        const CHAR* breakpoint = NULL,
                        const vector<UINT32> &tidVec = _sdbStaticUINT32Vec ) ;

      INT32 traceStart( UINT32 traceBufferSize,
                        const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 traceStop( const CHAR* dumpFileName ) ;

      INT32 traceResume() ;

      INT32 traceStatus( _sdbCursor** cursor ) ;

      INT32 traceStatus( sdbCursor& cursor )
      {
         RELEASE_INNER_HANDLE( cursor.pCursor ) ;
         return traceStatus( &(cursor.pCursor) ) ;
      }

      INT32 renameCollectionSpace( const CHAR* oldName,
                                   const CHAR* newName,
                                   const bson::BSONObj &options = _sdbStaticObject ) ;

      INT32 getLastErrorObj( bson::BSONObj &result ) ;
      void  cleanLastErrorObj() ;

      INT32 getLastResultObj( bson::BSONObj &result,
                              BOOLEAN getOwned = FALSE ) const ;

      INT32 createSequence( const CHAR *pSequenceName,
                            const bson::BSONObj &options,
                            _sdbSequence **sequence ) ;

      INT32 createSequence( const CHAR *pSequenceName,
                            const bson::BSONObj &options,
                            sdbSequence &sequence )
      {
         RELEASE_INNER_HANDLE( sequence.pSequence ) ;
         return createSequence( pSequenceName, options, &sequence.pSequence ) ;
      }

      INT32 createSequence( const CHAR *pSequenceName,
                            sdbSequence &sequence )
      {
         return createSequence( pSequenceName, _sdbStaticObject, sequence ) ;
      }

      INT32 getSequence( const CHAR *pSequenceName, _sdbSequence **sequence ) ;

      INT32 getSequence( const CHAR *pSequenceName, sdbSequence &sequence )
      {
         RELEASE_INNER_HANDLE( sequence.pSequence ) ;
         return getSequence( pSequenceName, &sequence.pSequence ) ;
      }

      INT32 renameSequence( const CHAR *pOldName, const CHAR *pNewName ) ;

      INT32 dropSequence( const CHAR *pSequenceName ) ;

      INT32 createDataSource( _sdbDataSource **dataSource,
                              const CHAR *pDataSourceName,
                              const CHAR *addresses,
                              const CHAR *user = NULL,
                              const CHAR *password = NULL,
                              const CHAR *type = NULL,
                              const bson::BSONObj *options = NULL ) ;

      INT32 dropDataSource( const CHAR *pDataSourceName ) ;

      INT32 getDataSource( const CHAR *pDataSourceName,
                           _sdbDataSource **dataSource ) ;

      INT32 listDataSources( _sdbCursor **cursor,
                             const bson::BSONObj &condition = _sdbStaticObject,
                             const bson::BSONObj &selector = _sdbStaticObject,
                             const bson::BSONObj &orderBy = _sdbStaticObject,
                             const bson::BSONObj &hint = _sdbStaticObject ) ;

      INT32 createRole( const bson::BSONObj &role );

      INT32 dropRole( const CHAR *pRoleName );

      INT32 getRole( const CHAR *pRoleName, const bson::BSONObj &options, bson::BSONObj &role );

      INT32 listRoles( _sdbCursor **result, const bson::BSONObj &options );

      INT32 updateRole( const CHAR *pRoleName, const bson::BSONObj &role );

      INT32 grantPrivilegesToRole( const CHAR *pRoleName, const bson::BSONObj &privileges );

      INT32 revokePrivilegesFromRole( const CHAR *pRoleName, const bson::BSONObj &privileges );

      INT32 grantRolesToRole( const CHAR *pRoleName, const bson::BSONObj &roles );

      INT32 revokeRolesFromRole( const CHAR *pRoleName, const bson::BSONObj &roles );

      INT32 grantRolesToUser( const CHAR *pUsrName, const bson::BSONObj &roles );

      INT32 revokeRolesFromUser( const CHAR *pUsrName, const bson::BSONObj &roles );

      INT32 getUser( const CHAR *pUserName, const bson::BSONObj &options, bson::BSONObj &user );

      INT32 invalidateUserCache( const CHAR *pUserName = NULL,
                                 const bson::BSONObj &options = _sdbStaticObject );
   } ;
   typedef class _sdbImpl sdbImpl ;

   class _sdbMsgConvertor
   {
   public:
      _sdbMsgConvertor() ;
      ~_sdbMsgConvertor() ;

      void reset( BOOLEAN releaseBuff = FALSE ) ;
      INT32 push( const CHAR *data, UINT32 size ) ;
      INT32 output( CHAR *&data, UINT32 &len ) ;

   private:
      INT32 _downgradeRequest( MsgHeader *header ) ;
      INT32 _upgradeReply( MsgOpReplyV1 *reply ) ;
      INT32 _ensureBuff( UINT32 size ) ;

   private:
      BOOLEAN     _hasData ;
      CHAR       *_buff ;
      UINT32      _buffSize ;
   } ;
   typedef class _sdbMsgConvertor sdbMsgConvertor ;
}

#endif //CLIENTIMPL_HPP__
