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

   Source File Name = dmsIndexBuilder.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_INDEX_BUILDER_HPP_
#define DMS_INDEX_BUILDER_HPP_

#include "dmsStorageBase.hpp"
#include "ixmKey.hpp"
#include "dmsExtent.hpp"
#include "dmsOprHandler.hpp"
#include "clsRemoteOperator.hpp"
#include "dmsTaskStatus.hpp"
#include "dmsScanner.hpp"
#include "rtnTBScanner.hpp"
#include "dmsWriteGuard.hpp"

namespace engine
{
   class _dmsMBContext ;
   class _dmsStorageIndex ;
   class _dmsStorageDataCommon ;
   class _pmdEDUCB ;
   class _ixmIndexCB ;
   class _dmsMBContext ;

   // 10 ms
   #define DMS_INDEX_WAITBLOCK_INTERVAL       ( 10 )

   /*
      _dmsDupKeyProcessor define
    */
   class _dmsDupKeyProcessor : public SDBObject
   {
   public:
      _dmsDupKeyProcessor()
      {
      }

      virtual ~_dmsDupKeyProcessor()
      {
      }

   public:
      virtual INT32 processDupKeyRecord( _dmsStorageDataCommon *suData,
                                         _dmsMBContext *mbContext,
                                         const dmsRecordID &recordID,
                                         ossValuePtr recordDataPtr,
                                         _pmdEDUCB *eduCB ) = 0 ;
   } ;

   typedef class _dmsDupKeyProcessor dmsDupKeyProcessor ;

   /*
      _dmsIndexBuilder define
    */
   class _dmsIndexBuilder: public utilPooledObject
   {
   public:
      _dmsIndexBuilder( _dmsStorageUnit* su,
                        _dmsMBContext* mbContext,
                        _pmdEDUCB* eduCB,
                        dmsExtentID indexExtentID,
                        dmsExtentID indexLogicID,
                        dmsIndexBuildGuardPtr &guardPtr,
                        dmsDupKeyProcessor *dkProcessor,
                        dmsIdxTaskStatus* pIdxStatus = NULL ) ;
      virtual ~_dmsIndexBuilder() ;
      INT32 build() ;

      void  setOprHandler( IDmsOprHandler *pOprHander ) ;
      void  setWriteResult( utilWriteResult *pResult ) ;

   protected:
      virtual INT32 _build() = 0 ;
      virtual INT32 _onInit() ;

      // make sure the mbContext is locked before call _beforeExtent()/_afterExtent()
      #define _DMS_SKIP_EXTENT 1
      virtual INT32 _beforeExtent() ;
      virtual INT32 _afterExtent( const dmsRecordID &lastRID,
                                  UINT64 scannedNum,
                                  BOOLEAN isEOF ) ;

      INT32 _getKeySet( ossValuePtr recordDataPtr, BSONObjSet& keySet ) ;
      INT32 _insertKey( ossValuePtr recordDataPtr, const dmsRecordID &rid, const Ordering& ordering ) ;
      INT32 _insertKey( const bson::BSONObj &key, const dmsRecordID &rid, const Ordering& ordering ) ;
      INT32 _checkIndexAfterLock( INT32 lockType ) ;
      INT32 _mbLockAndCheck( INT32 lockType ) ;

      INT32 _checkInterrupt() ;
      INT32 _createScannerChecker() ;
      void _releaseScannerChecker() ;

   private:
      INT32 _init() ;
      INT32 _finish() ;

   protected:
      _dmsStorageUnit *  _su ;
      _dmsStorageIndex*  _suIndex ;
      _dmsStorageDataCommon* _suData ;
      _dmsMBContext*     _mbContext ;
      _pmdEDUCB*         _eduCB ;
      dmsIndexBuildGuardPtr _buildGuardPtr ;
      std::shared_ptr<IIndex> _idxPtr ;
      dmsExtentID        _indexExtentID ;
      dmsExtentID        _indexLID ;
      _ixmIndexCB*       _indexCB ;
      OID                _indexOID ;
      dmsRecordID        _scanRID ;
      BOOLEAN            _unique ;
      BOOLEAN            _dropDups ;

      IRemoteOperator    *_remoteOperator ;

      IDmsOprHandler     *_pOprHandler ;
      utilWriteResult    *_pResult ;
      bson::BufBuilder   _bufBuilder ;
      dmsDupKeyProcessor *_dkProcessor ;
      dmsIdxTaskStatus* _pIdxStatus ;

      // index key generator
      ixmIndexKeyGen     _keyGen ;

      // scanner checker
      IDmsScannerChecker * _checker ;

   public:
      static _dmsIndexBuilder* createInstance( _dmsSUDescriptor *su,
                                               _dmsMBContext* mbContext,
                                               _pmdEDUCB* eduCB,
                                               dmsExtentID indexExtentID,
                                               dmsExtentID indexLogicID,
                                               INT32 sortBufferSize,
                                               UINT16 indexType,
                                               dmsIndexBuildGuardPtr &guardPtr,
                                               IDmsOprHandler *pOprHandler,
                                               utilWriteResult *pResult,
                                               dmsDupKeyProcessor *dkProcessor,
                                               dmsIdxTaskStatus* pIdxStatus = NULL
                                             ) ;

      static void releaseInstance( _dmsIndexBuilder* builder ) ;
   } ;
   typedef class _dmsIndexBuilder dmsIndexBuilder ;
}

#endif /* DMS_INDEX_BUILDER_HPP_ */

