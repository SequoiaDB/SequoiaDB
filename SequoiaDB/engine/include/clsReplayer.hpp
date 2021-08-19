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

   Source File Name = clsReplayer.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSREPLAYER_HPP_
#define CLSREPLAYER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogRecord.hpp"
#include "rtnBackgroundJob.hpp"
#include "utilCompressor.hpp"
#include "ossMemPool.hpp"
#include "../bson/bsonobj.h"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _clsBucket ;

   /*
      CLS_PARALLA_TYPE define
   */
   enum CLS_PARALLA_TYPE
   {
      CLS_PARALLA_NULL     = 0,
      CLS_PARALLA_REC,
      CLS_PARALLA_CL
   } ;

   /*
      _clsCLParallaInfo define
   */
   class _clsCLParallaInfo : public SDBObject
   {
      public:
         _clsCLParallaInfo() ;
         ~_clsCLParallaInfo() ;

         BOOLEAN           checkParalla( UINT16 type,
                                         DPS_LSN_OFFSET curLSN,
                                         _clsBucket *pBucket ) const ;

         void              updateParalla( CLS_PARALLA_TYPE parallaType,
                                          UINT16 type,
                                          DPS_LSN_OFFSET curLSN ) ;

         CLS_PARALLA_TYPE  getLastParallaType() const ;

         BOOLEAN           isParallaTypeSwitch( CLS_PARALLA_TYPE type ) const ;

         INT32             waitLastLSN( _clsBucket *pBucket ) ;

         void              setPending( DPS_LSN_OFFSET lsn ) ;

#if defined(_DEBUG)
         void              setCollection( const CHAR *collection )
         {
            _collection.assign( collection ) ;
         }
#endif

      protected:
         CLS_PARALLA_TYPE     _parallaType ;
         DPS_LSN_OFFSET       _lastLSN ;
         DPS_LSN_OFFSET       _pendingLSN ;
         // when duplicated key issue happened in record parallel mode,
         // should use collection parallel for at least pendingCount DPS records
         UINT32               _pendingCount ;
         // each duplicated key issue will double the pending count
         // pending index is a power of 2 to quick increase the start pending
         // count
         UINT32               _pendingIndex ;
         // number of records to replay in record parallel mode
         UINT64               _recParallaCount ;
#if defined(_DEBUG)
         ossPoolString        _collection ;
#endif
   } ;

   typedef _clsCLParallaInfo clsCLParallaInfo ;
   typedef ossPoolMap<utilCLUniqueID, clsCLParallaInfo>  MAP_CL_PARALLAINFO ;

   /*
      _clsReplayer define
   */
   class _clsReplayer : public SDBObject
   {
   public:
      _clsReplayer( BOOLEAN useDps = FALSE, BOOLEAN isReplSync = FALSE ) ;
      ~_clsReplayer() ;

      void enableDPS () ;
      void disableDPS () ;
      BOOLEAN isDPSEnabled () const { return _dpsCB ? TRUE : FALSE ; }

   public:
      INT32 replay( dpsLogRecordHeader *recordHeader,
                    pmdEDUCB *eduCB,
                    BOOLEAN incMonCount = TRUE,
                    BOOLEAN ignoreDupKey = FALSE ) ;
      INT32 replayByBucket( dpsLogRecordHeader *recordHeader,
                            _pmdEDUCB *eduCB, _clsBucket *pBucket ) ;

      INT32 replayCrtCS( const CHAR *cs, utilCSUniqueID csUniqueID,
                         INT32 pageSize, INT32 lobPageSize,
                         DMS_STORAGE_TYPE type, _pmdEDUCB *eduCB ) ;

      INT32 replayCrtCollection( const CHAR *collection,
                                 utilCLUniqueID clUniqueID,
                                 UINT32 attributes,
                                 _pmdEDUCB *eduCB,
                                 UTIL_COMPRESSOR_TYPE compType,
                                 const BSONObj *extOptions ) ;

      INT32 replayIXCrt( const CHAR *collection,
                         BSONObj &index,
                         _pmdEDUCB *eduCB ) ;

      INT32 replayInsert( const CHAR *collection,
                          BSONObj &obj,
                          _pmdEDUCB *eduCB ) ;

      INT32 rollback( const dpsLogRecordHeader *recordHeader,
                      _pmdEDUCB *eduCB ) ;

      INT32 replayWriteLob( const CHAR *fullName,
                            const bson::OID &oid,
                            UINT32 sequence,
                            UINT32 offset,
                            UINT32 len,
                            const CHAR *data,
                            _pmdEDUCB *eduCB ) ;

      INT32 rollbackTrans( const dpsLogRecordHeader *recordHeader,
                           pmdEDUCB *eduCB,
                           MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;

      INT32 replayRBPending( const dpsLogRecordHeader *recordHeader,
                             BOOLEAN removeOnly,
                             pmdEDUCB *eduCB,
                             MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;

   protected:
      // rollback INSERT DPS record
      INT32 _rollbackTransInsert( const dpsLogRecordHeader *recordHeader,
                                  pmdEDUCB *eduCB,
                                  MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;
      // rollback UPDATE DPS record
      INT32 _rollbackTransUpdate( const dpsLogRecordHeader *recordHeader,
                                  pmdEDUCB *eduCB,
                                  MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;
      // rollback DELETE DPS record
      INT32 _rollbackTransDelete( const dpsLogRecordHeader *recordHeader,
                                  pmdEDUCB *eduCB,
                                  MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;
      // write DPS transaction rollback log
      INT32 _logTransRollback( pmdEDUCB *eduCB ) ;

      // replay rollback INSERT to construct pending object
      INT32 _replayInsertRBPending( const dpsLogRecordHeader *recordHeader,
                                    BOOLEAN removeOnly,
                                    pmdEDUCB *eduCB,
                                    MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;
      // replay rollback UPDATE to construct pending object
      INT32 _replayUpdateRBPending( const dpsLogRecordHeader *recordHeader,
                                    BOOLEAN removeOnly,
                                    pmdEDUCB *eduCB,
                                    MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;
      // replay rollback UPDATE to construct pending object
      INT32 _replayDeleteRBPending( const dpsLogRecordHeader *recordHeader,
                                    BOOLEAN removeOnly,
                                    pmdEDUCB *eduCB,
                                    MAP_TRANS_PENDING_OBJ &mapPendingObj ) ;
      // helper function: construct new object by modifier
      INT32 _replayUpdateModifier( const bson::BSONObj &updater,
                                   const bson::BSONObj &oldObject,
                                   bson::BSONObj &newObject,
                                   UINT32 logWriteMode ) ;
      // helper function: query DMS record
      INT32 _queryRecord( const CHAR *collection,
                          const bson::BSONObj &matcher,
                          pmdEDUCB *eduCB,
                          bson::BSONObj &object ) ;

      // helper function: calculate bucket ID
      INT32 _calcBucketID( dpsLogRecordHeader *recordHeader,
                           _clsBucket *pBucket,
                           CLS_PARALLA_TYPE &parallaType,
                           UINT32 &bucketID,
                           UINT32 &clHash,
                           utilCLUniqueID &clUniqueID,
                           UINT32 &waitBucketID ) ;

      INT32 _calcDataBucketID( dpsLogRecordHeader *recordHeader,
                               _clsBucket *pBucket,
                               CLS_PARALLA_TYPE &parallaType,
                               UINT32 &bucketID,
                               UINT32 &clHash,
                               utilCLUniqueID &clUniqueID,
                               UINT32 &waitBucketID ) ;

      INT32 _calcLobBucketID( dpsLogRecordHeader *recordHeader,
                              _clsBucket *pBucket,
                              UINT32 &bucketID ) ;

      INT32 _checkCLParalla( dpsLogRecordHeader *recordHeader,
                             _clsBucket *pBucket,
                             const CHAR *fullname,
                             utilCLUniqueID &clUniqueID,
                             CLS_PARALLA_TYPE &parallaType ) ;

   private:
      _SDB_DMSCB              *_dmsCB ;
      _dpsLogWrapper          *_dpsCB ;
      monDBCB                 *_monDBCB ;

      BOOLEAN                 _isReplSync ;
   } ;
   typedef class _clsReplayer clsReplayer ;
}

#endif

