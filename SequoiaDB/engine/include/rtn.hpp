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

   Source File Name = rtn.hpp

   Descriptive Name = RunTime Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains declare for runtime
   functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_HPP_
#define RTN_HPP_

#include "core.hpp"
#include "rtnCB.hpp"
#include "../bson/bson.h"
#include "pmdEDU.hpp"
#include "rtnCommand.hpp"
#include "dpsLogWrapper.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "rtnInsertModifier.hpp"
#include "utilRenameLogger.hpp"
#include "utilInsertResult.hpp"
#include "dmsTaskStatus.hpp"
#include "rtnOprHandler.hpp"

#define RTN_SORT_INDEX_NAME "sort"
using namespace bson;

namespace engine
{

#define RTN_MON_LOB_OP_COUNT_INC( _pMonAppCB_, op, delta )                    \
   {                                                                          \
      if ( NULL != _pMonAppCB_ )                                              \
      {                                                                       \
         _pMonAppCB_->monOperationCountInc( op, delta, MON_UPDATE_SESSION ) ; \
      }                                                                       \
   }

#define RTN_MON_LOB_BYTES_COUNT_INC( _pMonAppCB_, op, delta )                 \
   {                                                                          \
      if ( NULL != _pMonAppCB_ )                                              \
      {                                                                       \
         _pMonAppCB_->monByteCountInc( op, delta, MON_UPDATE_SESSION ) ;      \
      }                                                                       \
   }

   class _dmsScanner ;
   class _rtnContextData ;
   class _rtnContextDump ;
   class _SDB_DMSCB;
   typedef _SDB_DMSCB SDB_DMSCB;

   INT32 rtnReallocBuffer ( CHAR **ppBuffer, INT32 *bufferSize,
                            INT32 newLength, INT32 alignmentSize ) ;

   BSONObj rtnUniqueKeyNameObj( const BSONObj &obj ) ;

   BSONObj rtnNullKeyNameObj( const BSONObj &obj ) ;

   INT32 rtnGetIXScanner ( const CHAR *pCollectionShortName,
                           optAccessPlanRuntime *planRuntime,
                           _dmsStorageUnit *su,
                           _dmsMBContext *mbContext,
                           _pmdEDUCB *cb,
                           _dmsScanner **ppScanner,
                           DMS_ACCESS_TYPE accessType,
                           IRtnOprHandler *opHandler = NULL ) ;

   INT32 rtnGetTBScanner ( const CHAR *pCollectionShortName,
                           optAccessPlanRuntime *planRuntime,
                           _dmsStorageUnit *su,
                           _dmsMBContext *mbContext,
                           _pmdEDUCB *cb,
                           _dmsScanner **ppScanner,
                           DMS_ACCESS_TYPE accessType,
                           IRtnOprHandler *opHandler = NULL ) ;

   BSONObj rtnUpdator2Obj( const BSONObj &source, const BSONObj &updator ) ;

   INT32 rtnInsert ( const CHAR *pCollectionName,
                     const BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb,
                     IRtnOprHandler *opHandler = NULL,
                     utilInsertResult *pResult = NULL,
                     const rtnInsertModifier *modifier = NULL ) ;

   // for insert/update/delete, if dpsCB = NULL, that means we don't log
   INT32 rtnInsert ( const CHAR *pCollectionName,
                     const BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                     SDB_DPSCB *dpsCB, INT16 w = 1,
                     IRtnOprHandler *opHandler = NULL,
                     utilInsertResult *pResult = NULL,
                     const rtnInsertModifier *modifier = NULL ) ;

   // for replaying insert operation. Only one record will be inserted in one
   // call.
   INT32 rtnReplayInsert( const CHAR *pCollectionName, const BSONObj &obj,
                          INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                          SDB_DPSCB *dpsCB, INT16 w = 1,
                          utilInsertResult *pResult = NULL,
                          INT64 position = -1,
                          IRtnOprHandler *opHandler = NULL ) ;

   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb,
                     utilUpdateResult *pResult = NULL,
                     const BSONObj *shardingKey = NULL,
                     UINT32 logWriteMod = DPS_LOG_WRITE_MOD_INCREMENT,
                     IRtnOprHandler *opHandler = NULL ) ;

   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w = 1,
                     utilUpdateResult *pResult = NULL,
                     const BSONObj *shardingKey = NULL,
                     UINT32 logWriteMod = DPS_LOG_WRITE_MOD_INCREMENT,
                     IRtnOprHandler *opHandler = NULL ) ;

   INT32 rtnUpdate ( rtnQueryOptions &options, const BSONObj &updator,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w = 1,
                     utilUpdateResult *pResult = NULL,
                     const BSONObj *shardingKey = NULL,
                     UINT32 logWriteMod = DPS_LOG_WRITE_MOD_INCREMENT,
                     IRtnOprHandler *opHandler = NULL ) ;

   INT32 rtnUpsertSet( const BSONElement& setOnInsert, BSONObj& target ) ;

   INT32 rtnDelete ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &hint, INT32 flags, pmdEDUCB *cb,
                     utilDeleteResult *pResult = NULL ) ;

   INT32 rtnDelete ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &hint, INT32 flags, pmdEDUCB *cb,
                     SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB, INT16 w = 1,
                     utilDeleteResult *pResult = NULL ) ;

   INT32 rtnDelete ( rtnQueryOptions &options, pmdEDUCB *cb,
                     SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB, INT16 w = 1,
                     utilDeleteResult *pResult = NULL ) ;

   INT32 rtnTraversalDelete ( const CHAR *pCollectionName,
                              const BSONObj &key,
                              const CHAR *pIndexName,
                              INT32 dir,
                              pmdEDUCB *cb,
                              SDB_DMSCB *dmsCB,
                              SDB_DPSCB *dpsCB,
                              INT16 w = 1 ) ;

   INT32 rtnMsg ( MsgOpMsg *pMsg ) ;

   // pCollectionName : requested collection name
   // selector        : fields want to select
   // matcher         : condition to match
   // orderBy         : orderBy for result
   // hint            : optimizer hint
   // flags           : query flag
   // cb              : EDU control block
   // numToSkip       : number of records to skip
   // numToReturn     : maximum number of records to return
   // dmsCB           : dms control block
   // rtnCB           : runtime control block
   // contextID       : newly created context
   INT32 rtnQuery ( const CHAR *pCollectionName,
                    const BSONObj &selector,
                    const BSONObj &matcher,
                    const BSONObj &orderBy,
                    const BSONObj &hint,
                    SINT32 flags,
                    pmdEDUCB *cb,
                    SINT64 numToSkip,
                    SINT64 numToReturn,
                    SDB_DMSCB *dmsCB,
                    SDB_RTNCB *rtnCB,
                    SINT64 &contextID,
                    rtnContextPtr *ppContext = NULL,
                    BOOLEAN enablePrefetch = FALSE ) ;

   INT32 rtnQuery ( rtnQueryOptions &options,
                    pmdEDUCB *cb,
                    SDB_DMSCB *dmsCB,
                    SDB_RTNCB *rtnCB,
                    SINT64 &contextID,
                    rtnContextPtr *ppContext = NULL,
                    BOOLEAN enablePrefetch = FALSE,
                    const rtnExplainOptions *expOptions = NULL ) ;

   INT32 rtnSort ( rtnContextPtr &pContext,
                   const BSONObj &orderBy,
                   _pmdEDUCB *cb,
                   SINT64 numToSkip,
                   SINT64 numToReturn,
                   SINT64 &contextID,
                   rtnContextPtr *ppContext = NULL ) ;

   // traversal the collection from a given key
   // the key must be normalized by ixm index key generator
   // 1) full collection name
   // 2) normalized index key
   // 3) index key that used for traversal
   // 4) direction (1 or -1 only)
   // ...
   // return context id, which can be used for getmore
   INT32 rtnTraversalQuery ( const CHAR *pCollectionName,
                             const BSONObj &key,
                             const CHAR *pIndexName,
                             INT32 dir,
                             pmdEDUCB *cb,
                             SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB,
                             SINT64 &contextID,
                             rtnContextPtr *ppContext = NULL,
                             BOOLEAN enablePrefetch = FALSE ) ;

   INT32 rtnCreateCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                           pmdEDUCB *cb,
                                           SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                                           utilCSUniqueID csUniqueID,
                                           INT32 pageSize = DMS_PAGE_SIZE_DFT,
                                           INT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ,
                                           DMS_STORAGE_TYPE type = DMS_STORAGE_NORMAL,
                                           BOOLEAN sysCall = FALSE ) ;

   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      UINT32 attributes,
                                      pmdEDUCB *cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      utilCLUniqueID clUniqueID,
                                      UTIL_COMPRESSOR_TYPE compressorType =
                                          UTIL_COMPRESSOR_INVALID,
                                      INT32 flags = 0,
                                      BOOLEAN sysCall = FALSE,
                                      const BSONObj *extOptions = NULL,
                                      const BSONObj *pIdIdxDef = NULL,
                                      BOOLEAN addIdxIDIfNotExist = TRUE ) ;

   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      const BSONObj &shardIdxDef,
                                      UINT32 attributes,
                                      _pmdEDUCB * cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      utilCLUniqueID clUniqueID,
                                      UTIL_COMPRESSOR_TYPE compressorType =
                                          UTIL_COMPRESSOR_INVALID,
                                      INT32 flags = 0,
                                      BOOLEAN sysCall = FALSE,
                                      const BSONObj *extOptions = NULL,
                                      const BSONObj *pIdIdxDef = NULL,
                                      BOOLEAN addIdxIDIfNotExist = FALSE ) ;

   INT32 rtnGetMore ( SINT64 contextID,            // input, context id
                      SINT32 maxNumToReturn,       // input, max record to read
                      rtnContextBuf &buffObj,      // output
                      pmdEDUCB *cb,                // input educb
                      SDB_RTNCB *rtnCB             // input runtimecb
                      ) ;

   INT32 rtnGetMore ( rtnContextPtr &pContext,     // input, context
                      SINT32 maxNumToReturn,       // input, max record to read
                      rtnContextBuf &buffObj,      // output
                      pmdEDUCB *cb,                // input educb
                      SDB_RTNCB *rtnCB             // input runtimecb
                      ) ;

   INT32 rtnAdvance( SINT64 contextID,
                     const BSONObj &arg,
                     const CHAR *pBackData,
                     INT32 backDataSize,
                     pmdEDUCB *cb,
                     SDB_RTNCB *rtnCB
                    ) ;

   INT32 rtnLoadCollectionSpace ( const CHAR *pCSName,
                                  const CHAR *dataPath,
                                  const CHAR *indexPath,
                                  const CHAR *lobPath,
                                  const CHAR *lobMetaPath,
                                  pmdEDUCB *cb,
                                  SDB_DMSCB *dmsCB,
                                  BOOLEAN checkOnly,
                                  utilCSUniqueID *csUniqueIDInCat = NULL,
                                  const BSONObj *clInfoInCat = NULL,
                                  const ossPoolVector<BSONObj> *idxInfoInCat = NULL ) ;

   INT32 rtnLoadCollectionSpaces ( const CHAR *dataPath,
                                   const CHAR *indexPath,
                                   const CHAR *lobPath,
                                   const CHAR *lobMetaPath,
                                   SDB_DMSCB *dmsCB ) ;

   void rtnDelContextForCollectionSpace ( const CHAR *pCollectionSpace,
                                          UINT32 suLogicalID,
                                          _pmdEDUCB *cb ) ;

   INT32 rtnDelCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                        _pmdEDUCB *cb,
                                        SDB_DMSCB *dmsCB,
                                        SDB_DPSCB *dpsCB,
                                        BOOLEAN sysCall,
                                        BOOLEAN dropFile,
                                        BOOLEAN ensureEmpty = FALSE,
                                        dmsDropCSOptions *options = NULL ) ;

   INT32 rtnUnloadCollectionSpace( const CHAR *pCollectionSpace,
                                   _pmdEDUCB *cb,
                                   SDB_DMSCB *dmsCB ) ;

   INT32 rtnUnloadCollectionSpaces( _pmdEDUCB * cb,
                                    SDB_DMSCB * dmsCB ) ;

   INT32 rtnCollectionSpaceLock ( const CHAR *pCollectionSpaceName,
                                  SDB_DMSCB *dmsCB,
                                  BOOLEAN loadFile,
                                  dmsStorageUnit **ppsu,
                                  dmsStorageUnitID &suID,
                                  OSS_LATCH_MODE lockType = SHARED,
                                  INT32 millisec = -1 ) ;

   INT32 rtnResolveCollectionNameAndLock ( const CHAR *pCollectionFullName,
                                           SDB_DMSCB *dmsCB,
                                           dmsStorageUnit **ppsu,
                                           const CHAR **ppCollectionName,
                                           dmsStorageUnitID &suID,
                                           OSS_LATCH_MODE lockType = SHARED,
                                           INT32 millisec = -1 ) ;

   INT32 rtnFindCollection ( const CHAR *pCollection,
                             SDB_DMSCB *dmsCB ) ;

   INT32 rtnKillContexts ( SINT32 numContexts, const SINT64 *pContextIDs,
                           pmdEDUCB *cb, SDB_RTNCB *rtnCB ) ;

   INT32 rtnBackup ( pmdEDUCB *cb, const CHAR *path, const CHAR *backupName,
                     BOOLEAN ensureInc, BOOLEAN rewrite, const CHAR *desp,
                     const BSONObj &option ) ;

   INT32 rtnRemoveBackup ( pmdEDUCB *cb, const CHAR *path,
                           const CHAR *backupName,
                           const BSONObj &option ) ;

   INT32 rtnDumpBackups ( const BSONObj &hint, _rtnContextDump *context ) ;

   BOOLEAN rtnIsInBackup () ;

   // perform collection offline reorg. Note ignoreError usually used by full
   // database rebuild. This option is trying best to ignore data corruption (
   // not all corruptions can be ignored, for example if metadata is corrupted
   // there's nothing we can do ).
   INT32 rtnReorgOffline ( const CHAR *pCollectionName,
                           const BSONObj &hint,
                           pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB ) ;

   INT32 rtnReorgRecover ( const CHAR *pCollectionName,
                           pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB ) ;


   //RTN common fuction declare
   INT32 rtnGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               const CHAR **value ) ;

   INT32 rtnGetSTDStringElement ( const BSONObj &obj, const CHAR *fieldName,
                                  string &value ) ;

   INT32 rtnGetPoolStringElement ( const BSONObj &obj, const CHAR *fieldName,
                                  ossPoolString &value ) ;

   INT32 rtnGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                            BSONObj &value ) ;

   INT32 rtnGetArrayElement ( const BSONObj &obj, const CHAR *fieldName,
                              BSONObj &value ) ;

   INT32 rtnGetIntElement ( const BSONObj &obj, const CHAR *fieldName,
                            INT32 &value ) ;

   INT32 rtnGetBooleanElement ( const BSONObj &obj, const CHAR *fieldName,
                                BOOLEAN &value ) ;

   INT32 rtnGetNumberLongElement ( const BSONObj &obj, const CHAR *fieldName,
                                   INT64 &value ) ;

   INT32 rtnGetDoubleElement ( const BSONObj &obj, const CHAR *fieldName,
                               double &value ) ;

   INT32 rtnCreateIndexCommand ( const CHAR *pCollection,
                                 const BSONObj &indexObj,
                                 _pmdEDUCB *cb,
                                 SDB_DMSCB *dmsCB,
                                 SDB_DPSCB *dpsCB,
                                 BOOLEAN isSys = FALSE,
                                 INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                 utilWriteResult *pResult = NULL,
                                 dmsIdxTaskStatus *pIdxStatus = NULL,
                                 BOOLEAN addUIDIfNotExist = TRUE ) ;
   INT32 rtnCreateIndexCommand ( utilCLUniqueID clUniqID,
                                 const BSONObj &indexObj,
                                 _pmdEDUCB *cb,
                                 SDB_DMSCB *dmsCB,
                                 SDB_DPSCB *dpsCB,
                                 BOOLEAN isSys,
                                 INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                 utilWriteResult *pResult = NULL,
                                 dmsIdxTaskStatus *pIdxStatus = NULL,
                                 BOOLEAN addUIDIfNotExist = TRUE ) ;

   INT32 rtnDropCollectionCommand ( const CHAR *pCollection,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                                    dmsDropCLOptions *options = NULL ) ;

   INT32 rtnRenameCollectionCommand ( const CHAR *csName,
                                      const CHAR *clShortName,
                                      const CHAR *newCLShortName,
                                      _pmdEDUCB *cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      BOOLEAN blockWrite ) ;

   INT32 rtnTruncCollectionCommand( const CHAR *pCollection,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    dmsMBContext *mbContext = NULL,
                                    dmsTruncCLOptions *options = NULL ) ;

   INT32 rtnRenameCollectionSpaceCommand ( const CHAR *pCSName,
                                           const CHAR *pNewCSName,
                                           _pmdEDUCB *cb,
                                           SDB_DMSCB *dmsCB,
                                           SDB_DPSCB *dpsCB,
                                           BOOLEAN blockWrite ) ;

   INT32 rtnReturnCommand( dmsReturnOptions &options,
                           _pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_DPSCB *dpsCB,
                           BOOLEAN blockWrite ) ;

   INT32 rtnDropCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         _pmdEDUCB *cb,
                                         SDB_DMSCB *dmsCB,
                                         SDB_DPSCB *dpsCB,
                                         BOOLEAN   sysCall = FALSE,
                                         BOOLEAN   ensureEmpty = FALSE,
                                         dmsDropCSOptions *options = NULL ) ;

   INT32 rtnDropCollectionSpaceP1 ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall = FALSE );

   INT32 rtnDropCollectionSpaceP2 ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall = FALSE,
                                    dmsDropCSOptions *options = NULL );

   INT32 rtnDropCollectionSpaceP1Cancel ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall = FALSE );

   INT32 rtnDropIndexCommand ( const CHAR *pCollection,
                               const BSONElement &identifier,
                               pmdEDUCB *cb,
                               SDB_DMSCB *dmsCB,
                               SDB_DPSCB *dpsCB,
                               BOOLEAN sysCall = FALSE,
                               dmsIdxTaskStatus *pIdxStatus = NULL,
                               BOOLEAN onlyStandalone = FALSE ) ;
   INT32 rtnDropIndexCommand ( utilCLUniqueID clUniqID,
                               const BSONElement &identifier,
                               pmdEDUCB *cb,
                               SDB_DMSCB *dmsCB,
                               SDB_DPSCB *dpsCB,
                               BOOLEAN sysCall = FALSE,
                               dmsIdxTaskStatus *pIdxStatus = NULL,
                               BOOLEAN onlyStandalone = FALSE ) ;

   INT32 rtnGetCount ( const rtnQueryOptions & options,
                       SDB_DMSCB *dmsCB,
                       _pmdEDUCB *cb,
                       SDB_RTNCB *rtnCB,
                       INT64 *count ) ;

   INT32 rtnGetCount ( const rtnQueryOptions & options,
                       SDB_DMSCB *dmsCB,
                       _pmdEDUCB *cb,
                       SDB_RTNCB *rtnCB,
                       rtnContext *context ) ;

   INT32 rtnGetCommandEntry ( RTN_COMMAND_TYPE command,
                              const rtnQueryOptions & options,
                              pmdEDUCB *cb,
                              SDB_DMSCB *dmsCB,
                              SDB_RTNCB *rtnCB,
                              SINT64 &contextID ) ;

   INT32 rtnGetQueryMeta( const rtnQueryOptions & options,
                          SDB_DMSCB *dmsCB,
                          pmdEDUCB *cb,
                          _rtnContextDump *context ) ;

   INT32 rtnTestCollectionCommand ( const CHAR *pCollection,
                                    SDB_DMSCB *dmsCB,
                                    utilCLUniqueID *pClUniqueID = NULL,
                                    utilCLUniqueID *pCurClUniqueID = NULL ) ;

   INT32 rtnTestCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         SDB_DMSCB *dmsCB,
                                         utilCSUniqueID *pCsUniqueID = NULL,
                                         utilCSUniqueID *pCurCsUniqueID = NULL) ;

   INT32 rtnPopCommand( const CHAR *pCollectionName, INT64 value,
                        pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                        INT8 direction = 1, BOOLEAN byNumber = FALSE ) ;

   INT32 rtnChangeUniqueID( const CHAR* csName, utilCSUniqueID csUniqueID,
                            const BSONObj& clInfoObj, pmdEDUCB* cb,
                            SDB_DMSCB* dmsCB, SDB_DPSCB* dpsCB) ;

   INT32 rtnTestIndex( const CHAR *pCollection,
                       const CHAR *pIndexName,
                       SDB_DMSCB *dmsCB,
                       const BSONObj *pIndexDef = NULL,
                       BOOLEAN *pIsSame = NULL ) ;

   BOOLEAN rtnIsCommand ( const CHAR *name ) ;
   INT32 rtnParserCommand ( const CHAR *name, _rtnCommand **ppCommand ) ;
   INT32 rtnReleaseCommand ( _rtnCommand **ppCommand ) ;
   INT32 rtnInitCommand ( _rtnCommand *pCommand ,INT32 flags, INT64 numToSkip,
                          INT64 numToReturn, const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff, const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;
   INT32 rtnRunCommand ( _rtnCommand *pCommand, INT32 serviceType,
                         _pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                         SDB_RTNCB *rtnCB, SDB_DPSCB *dpsCB,
                         INT16 w = 1, INT64 *pContextID = NULL ) ;

   INT32 rtnTransBegin( _pmdEDUCB *cb,
                        BOOLEAN isAutoCommit = FALSE,
                        DPS_TRANS_ID specID = DPS_INVALID_TRANS_ID ) ;
   INT32 rtnTransPreCommit( _pmdEDUCB *cb, UINT32 nodeNum,
                            const UINT64 *pNodes, INT16 w,
                            SDB_DPSCB *dpsCB ) ;
   INT32 rtnTransCommit( _pmdEDUCB *cb, SDB_DPSCB *dpsCB );
   INT32 rtnTransRollback( _pmdEDUCB * cb, SDB_DPSCB *dpsCB );
   INT32 rtnTransRollbackAll( _pmdEDUCB * cb,
                              UINT64 doRollbackID );
   INT32 rtnTransSaveWaitCommit ( _pmdEDUCB * cb, SDB_DPSCB * dpsCB,
                                  BOOLEAN & savedAsWaitCommit ) ;

   void  rtnUnsetTransContext( _pmdEDUCB *cb, SDB_RTNCB *rtnCB ) ;

   INT32 rtnTransTryOrTestLockCL( const CHAR *pCollection,
                                  INT32 lockType,
                                  BOOLEAN isTest,
                                  _pmdEDUCB *cb ) ;

   INT32 rtnTransTryOrTestLockCS( const CHAR *pSpace,
                                  INT32 lockType,
                                  BOOLEAN isTest,
                                  _pmdEDUCB *cb ) ;

   INT32 rtnTransReleaseLock( const CHAR *pCollection,
                              _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                              SDB_DPSCB *dpsCB );

   INT32 rtnCorrectCollectionSpaceFile( const CHAR *dataPath,
                                        const CHAR *indexPath,
                                        const CHAR *lobPath,
                                        const CHAR *lobMetaPath,
                                        UINT32 sequence,
                                        const utilRenameLog& renameLog ) ;

   INT32 rtnCorrectCollectionSpaceFile( const CHAR* pPath,
                                        const CHAR* pFileName,
                                        const utilRenameLog& renameLog ) ;

   BOOLEAN rtnVerifyCollectionSpaceFileName ( const CHAR *pFileName,
                                              CHAR *pSUName,
                                              UINT32 bufferSize,
                                              UINT32 &sequence,
                                              const CHAR *extFilter =
                                              DMS_DATA_SU_EXT_NAME ) ;

   enum SDB_FILE_TYPE
   {
      SDB_FILE_DATA        = 0,
      SDB_FILE_INDEX       = 1,
      SDB_FILE_LOBM        = 2,
      SDB_FILE_LOBD        = 3,

      SDB_FILE_STARTUP     = 10,
      SDB_FILE_STARTUP_HST = 11,
      SDB_FILE_RENAME_INFO = 12,
      SDB_FILE_UNKNOW      = 255
   } ;

   SDB_FILE_TYPE rtnParseFileName( const CHAR *pFileName ) ;

   string   rtnMakeSUFileName( const string &csName, UINT32 sequence,
                               const string &extName ) ;
   string   rtnFullPathName( const string &path, const string &name ) ;

   INT32    rtnAggregate( const CHAR *pCollectionName, bson::BSONObj &objs,
                          INT32 objNum, SINT32 flags, pmdEDUCB *cb,
                          SDB_DMSCB *dmsCB, SINT64 &contextID ) ;

   INT32 rtnExplain( rtnQueryOptions &options,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                     SDB_RTNCB *rtnCB, INT64 &contextID,
                     rtnContextPtr *ppContext = NULL ) ;

   void rtnGetMergedSelector( const BSONObj &original,
                              const BSONObj &orderBy,
                              BOOLEAN &needReset,
                              BSONObj *mergedSelect = NULL ) ;

   BOOLEAN rtnMergeSelector( BSONObjBuilder &builder,
                             const BSONObj &select,
                             BSONObj &mergedSelect ) ;

   /*
      syncType: =0 ->FALSE, >0 ->TRUE, <0 -> Use system config
      pSpecCSName: not null and empty, will sync the pSpecCSName only
   */
   INT32 rtnSyncDB( pmdEDUCB *cb, INT32 syncType,
                    const CHAR *pSpecCSName = NULL,
                    BOOLEAN block = FALSE ) ;

   INT32 rtnTestAndCreateCL ( const CHAR *pCLFullName, pmdEDUCB *cb,
                              _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB,
                              utilCLUniqueID clUID, BOOLEAN sys ) ;

   INT32 rtnTestAndCreateIndex ( const CHAR *pCLFullName,
                                 const BSONObj &indexDef,
                                 pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                 _dpsLogWrapper *dpsCB, BOOLEAN sys = TRUE,
                                 INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ) ;

   INT32 rtnAnalyze ( const CHAR *pCSName,
                      const CHAR *pCLName,
                      const CHAR *pIXName,
                      const rtnAnalyzeParam &param,
                      pmdEDUCB *cb,
                      _SDB_DMSCB *dmsCB,
                      _SDB_RTNCB *rtnCB,
                      _dpsLogWrapper *dpsCB ) ;

   INT32 rtnReloadCLStats ( dmsStorageUnit *pSU, dmsMBContext *mbContext,
                            pmdEDUCB *cb, _SDB_DMSCB *dmsCB ) ;

   INT32 rtnAnalyzeDpsLog ( const CHAR *pCSName,
                            const CHAR *pCLFullName,
                            const CHAR *pIndexName,
                            _dpsLogWrapper *dpsCB ) ;

   /* Split collection full name to cs name and cl name */
   INT32 rtnResolveCollectionName( const CHAR *pInput, UINT32 inputLen,
                                   CHAR *pSpaceName, UINT32 spaceNameSize,
                                   CHAR *pCollectionName,
                                   UINT32 collectionNameSize ) ;

   /* Split collection full name to find cs name */
   INT32 rtnResolveCollectionSpaceName ( const CHAR *pInput,
                                         UINT32 inputLen,
                                         CHAR *pSpaceName,
                                         UINT32 spaceNameSize ) ;

   /* Check whether collections in the same space */
   INT32 rtnCollectionsInSameSpace ( const CHAR *pCLNameA, UINT32 lengthA,
                                     const CHAR *pCLNameB, UINT32 lengthB,
                                     BOOLEAN &inSameSpace ) ;

   /* Check whether the collections is in the space */
   BOOLEAN rtnCollectionInTheSpace ( const CHAR *pCLName,
                                     const CHAR *pCSName ) ;

   INT32 rtnCheckAndConvertIndexDef( BSONObj& indexDef ) ;

   INT32 rtnIsIndexCBValid( ixmIndexCB *indexCB,
                            dmsExtentID expectedExtentID,
                            const CHAR* expectedIndexName,
                            dmsExtentID expectedIndexLID,
                            dmsStorageUnit *su,
                            dmsMBContext *mbContext ) ;

   INT32 rtnParseCmdLocationMatcher( const BSONObj &query,
                                     BSONObj &nodesMatcher,
                                     BSONObj &newMatcher,
                                     BOOLEAN ignoreNodeParam = FALSE,
                                     BOOLEAN ignoreCtrlParam = FALSE ) ;
}

#endif

