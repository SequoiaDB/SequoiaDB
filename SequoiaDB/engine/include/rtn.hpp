/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "dms.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "../bson/bson.h"
#include "pmdEDU.hpp"
#include "rtnCommand.hpp"
#include "dpsLogWrapper.hpp"
#include "pmd.hpp"
#include "pd.hpp"

#define RTN_SORT_INDEX_NAME "sort"
using namespace bson;

namespace engine
{

   class _dmsScanner ;
   class _rtnContextData ;
   class _rtnContextDump ;

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
                           DMS_ACCESS_TYPE accessType ) ;

   INT32 rtnGetTBScanner ( const CHAR *pCollectionShortName,
                           optAccessPlanRuntime *planRuntime,
                           _dmsStorageUnit *su,
                           _dmsMBContext *mbContext,
                           _pmdEDUCB *cb,
                           _dmsScanner **ppScanner,
                           DMS_ACCESS_TYPE accessType ) ;

   INT32 rtnGetIndexSeps( optAccessPlanRuntime *planRuntime,
                          _dmsStorageUnit *su,
                          _dmsMBContext *mbContext,
                          _pmdEDUCB *cb,
                          std::vector< BSONObj > &idxBlocks,
                          std::vector< dmsRecordID > &idxRIDs ) ;

   class _rtnInternalSorting ;

   INT32 rtnGetIndexSamples ( _dmsStorageUnit *su,
                              ixmIndexCB *indexCB,
                              _pmdEDUCB * cb,
                              UINT32 sampleRecords,
                              UINT64 totalRecords,
                              BOOLEAN fullScan,
                              _rtnInternalSorting &sorter,
                              UINT32 &levels, UINT32 &pages ) ;

   INT32 rtnInsert ( const CHAR *pCollectionName, BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb,
                     INT32 *pInsertedNum = NULL,
                     INT32 *pIgnoredNum = NULL ) ;

   INT32 rtnInsert ( const CHAR *pCollectionName, BSONObj &objs, INT32 objNum,
                     INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                     SDB_DPSCB *dpsCB, INT16 w = 1,
                     INT32 *pInsertedNum = NULL,
                     INT32 *pIgnoredNum = NULL ) ;

   INT32 rtnReplayInsert( const CHAR *pCollectionName, BSONObj &obj,
                          INT32 flags, pmdEDUCB *cb, SDB_DMSCB *dmsCB,
                          SDB_DPSCB *dpsCB, INT16 w = 1 ) ;

   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, INT64 *pUpdateNum = NULL,
                     INT32 *pInsertNum = NULL,
                     const BSONObj *shardingKey = NULL ) ;

   INT32 rtnUpdate ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &updator, const BSONObj &hint, INT32 flags,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w = 1, INT64 *pUpdateNum = NULL,
                     INT32 *pInsertNum = NULL,
                     const BSONObj *shardingKey = NULL ) ;

   INT32 rtnUpdate ( rtnQueryOptions &options, const BSONObj &updator,
                     pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                     INT16 w = 1, INT64 *pUpdateNum = NULL,
                     INT32 *pInsertNum = NULL,
                     const BSONObj *shardingKey = NULL ) ;

   INT32 rtnUpsertSet( const BSONElement& setOnInsert, BSONObj& target ) ;

   INT32 rtnDelete ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &hint, INT32 flags, pmdEDUCB *cb,
                     INT64 *pDelNum = NULL ) ;

   INT32 rtnDelete ( const CHAR *pCollectionName, const BSONObj &matcher,
                     const BSONObj &hint, INT32 flags, pmdEDUCB *cb,
                     SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB, INT16 w = 1,
                     INT64 *pDelNum = NULL ) ;

   INT32 rtnDelete ( rtnQueryOptions &options, pmdEDUCB *cb,
                     SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB, INT16 w = 1,
                     INT64 *pDelNum = NULL ) ;

   INT32 rtnTraversalDelete ( const CHAR *pCollectionName,
                              const BSONObj &key,
                              const CHAR *pIndexName,
                              INT32 dir,
                              pmdEDUCB *cb,
                              SDB_DMSCB *dmsCB,
                              SDB_DPSCB *dpsCB,
                              INT16 w = 1 ) ;

   INT32 rtnMsg ( MsgOpMsg *pMsg ) ;

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
                    rtnContextBase **ppContext = NULL,
                    BOOLEAN enablePrefetch = FALSE ) ;

   INT32 rtnQuery ( rtnQueryOptions &options,
                    pmdEDUCB *cb,
                    SDB_DMSCB *dmsCB,
                    SDB_RTNCB *rtnCB,
                    SINT64 &contextID,
                    rtnContextBase **ppContext = NULL,
                    BOOLEAN enablePrefetch = FALSE,
                    BOOLEAN keepSearchPaths = FALSE ) ;

   INT32 rtnSort ( rtnContext **ppContext,
                   const BSONObj &orderBy,
                   _pmdEDUCB *cb,
                   SINT64 numToSkip,
                   SINT64 numToReturn,
                   SINT64 &contextID ) ;

   INT32 rtnTraversalQuery ( const CHAR *pCollectionName,
                             const BSONObj &key,
                             const CHAR *pIndexName,
                             INT32 dir,
                             pmdEDUCB *cb,
                             SDB_DMSCB *dmsCB,
                             SDB_RTNCB *rtnCB,
                             SINT64 &contextID,
                             _rtnContextData **ppContext = NULL,
                             BOOLEAN enablePrefetch = FALSE ) ;

   INT32 rtnCreateCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                           pmdEDUCB *cb,
                                           SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                                           INT32 pageSize = DMS_PAGE_SIZE_DFT,
                                           INT32 lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ,
                                           DMS_STORAGE_TYPE type = DMS_STORAGE_NORMAL,
                                           BOOLEAN sysCall = FALSE ) ;

   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      UINT32 attributes,
                                      pmdEDUCB *cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      UTIL_COMPRESSOR_TYPE compressorType =
                                          UTIL_COMPRESSOR_INVALID,
                                      INT32 flags = 0,
                                      BOOLEAN sysCall = FALSE,
                                      const BSONObj *extOptions = NULL ) ;

   INT32 rtnCreateCollectionCommand ( const CHAR *pCollection,
                                      const BSONObj &shardingKey,
                                      UINT32 attributes,
                                      _pmdEDUCB * cb,
                                      SDB_DMSCB *dmsCB,
                                      SDB_DPSCB *dpsCB,
                                      UTIL_COMPRESSOR_TYPE compressorType =
                                          UTIL_COMPRESSOR_INVALID,
                                      INT32 flags = 0,
                                      BOOLEAN sysCall = FALSE,
                                      const BSONObj *extOptions = NULL ) ;

   INT32 rtnGetMore ( SINT64 contextID,            // input, context id
                      SINT32 maxNumToReturn,       // input, max record to read
                      rtnContextBuf &buffObj,      // output
                      pmdEDUCB *cb,                // input educb
                      SDB_RTNCB *rtnCB             // input runtimecb
                      ) ;

   INT32 rtnLoadCollectionSpace ( const CHAR *pCSName,
                                  const CHAR *dataPath,
                                  const CHAR *indexPath,
                                  const CHAR *lobPath,
                                  const CHAR *lobMetaPath,
                                  pmdEDUCB *cb,
                                  SDB_DMSCB *dmsCB,
                                  BOOLEAN checkOnly = FALSE ) ;

   INT32 rtnLoadCollectionSpaces ( const CHAR *dataPath,
                                   const CHAR *indexPath,
                                   const CHAR *lobPath,
                                   const CHAR *lobMetaPath,
                                   SDB_DMSCB *dmsCB ) ;

   void rtnDelContextForCollectionSpace ( const CHAR *pCollectionSpace,
                                          _pmdEDUCB *cb ) ;

   INT32 rtnDelCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                        _pmdEDUCB *cb,
                                        SDB_DMSCB *dmsCB,
                                        SDB_DPSCB *dpsCB,
                                        BOOLEAN sysCall,
                                        BOOLEAN dropFile ) ;

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

   INT32 rtnKillContexts ( SINT32 numContexts, SINT64 *pContextIDs,
                           pmdEDUCB *cb, SDB_RTNCB *rtnCB ) ;

   INT32 rtnBackup ( pmdEDUCB *cb, const CHAR *path, const CHAR *backupName,
                     BOOLEAN ensureInc, BOOLEAN rewrite, const CHAR *desp,
                     const BSONObj &option ) ;

   INT32 rtnRemoveBackup ( pmdEDUCB *cb, const CHAR *path,
                           const CHAR *backupName,
                           const BSONObj &option ) ;

   INT32 rtnDumpBackups ( const BSONObj &hint, _rtnContextDump *context ) ;

   BOOLEAN rtnIsInBackup () ;

   INT32 rtnReorgOffline ( const CHAR *pCollectionName,
                           const BSONObj &hint,
                           pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB ) ;

   INT32 rtnReorgRecover ( const CHAR *pCollectionName,
                           pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB ) ;


   INT32 rtnGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               const CHAR **value ) ;

   INT32 rtnGetSTDStringElement ( const BSONObj &obj, const CHAR *fieldName,
                                  string &value ) ;

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
                                 INT32 sortBufferSize = SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ) ;

   INT32 rtnDropCollectionCommand ( const CHAR *pCollection,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB ) ;

   INT32 rtnTruncCollectionCommand( const CHAR *pCollection,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB ) ;

   INT32 rtnDropCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         _pmdEDUCB *cb,
                                         SDB_DMSCB *dmsCB,
                                         SDB_DPSCB *dpsCB,
                                         BOOLEAN   sysCall = FALSE ) ;

   INT32 rtnDropCollectionSpaceP1 ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall = FALSE );

   INT32 rtnDropCollectionSpaceP2 ( const CHAR *pCollectionSpace,
                                    _pmdEDUCB *cb,
                                    SDB_DMSCB *dmsCB,
                                    SDB_DPSCB *dpsCB,
                                    BOOLEAN   sysCall = FALSE );

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
                               BOOLEAN sysCall = FALSE ) ;

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
                                    SDB_DMSCB *dmsCB ) ;

   INT32 rtnTestCollectionSpaceCommand ( const CHAR *pCollectionSpace,
                                         SDB_DMSCB *dmsCB ) ;

   INT32 rtnPopCommand( const CHAR *pCollectionName, INT64 logicalID,
                        pmdEDUCB *cb, SDB_DMSCB *dmsCB, SDB_DPSCB *dpsCB,
                        INT16 w, INT8 direction = 1 ) ;

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

   INT32 rtnTransBegin( _pmdEDUCB *cb );
   INT32 rtnTransCommit( _pmdEDUCB *cb, SDB_DPSCB *dpsCB );
   INT32 rtnTransRollback( _pmdEDUCB * cb, SDB_DPSCB *dpsCB );
   INT32 rtnTransRollbackAll( _pmdEDUCB * cb );
   INT32 rtnTransTryLockCL( const CHAR *pCollection, INT32 lockType,
                           _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                           SDB_DPSCB *dpsCB );
   INT32 rtnTransTryLockCS( const CHAR *pSpace, INT32 lockType,
                           _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                           SDB_DPSCB *dpsCB );
   INT32 rtnTransReleaseLock( const CHAR *pCollection,
                           _pmdEDUCB *cb,SDB_DMSCB *dmsCB,
                           SDB_DPSCB *dpsCB );

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
                     rtnContextBase **ppContext = NULL ) ;

   void rtnNeedResetSelector( const BSONObj &original,
                              const BSONObj &orderBy,
                              BOOLEAN &needReset ) ;

   /*
      syncType: =0 ->FALSE, >0 ->TRUE, <0 -> Use system config
      pSpecCSName: not null and empty, will sync the pSpecCSName only
   */
   INT32 rtnSyncDB( pmdEDUCB *cb, INT32 syncType,
                    const CHAR *pSpecCSName = NULL,
                    BOOLEAN block = FALSE ) ;

   INT32 rtnTestAndCreateCL ( const CHAR *pCLFullName, pmdEDUCB *cb,
                              _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB,
                              BOOLEAN sys = TRUE ) ;

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

}

#endif

