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

   Source File Name = dmsStorageIndex.cpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "pmd.hpp"
#include "dpsOp2Record.hpp"
#include "dpsTransCB.hpp"
#include "ixmExtent.hpp"
#include "bpsPrefetch.hpp"
#include "dmsCompress.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "dmsIndexBuilder.hpp"
#include "dmsTransLockCallback.hpp"

using namespace bson ;

#define DMS_MAX_TEXT_IDX_NUM        1

namespace engine
{

   _dmsStorageIndex::_dmsStorageIndex( const CHAR * pSuFileName,
                                       dmsStorageInfo * pInfo,
                                       dmsStorageDataCommon * pDataSu )
   :_dmsStorageBase( pSuFileName, pInfo )
   {
      SDB_ASSERT( pDataSu, "Data Su can't be NULL" ) ;
      // TODO: temporary cast
      _pDataSu = (dmsStorageData *)pDataSu ;

      _pDataSu->_attach( this ) ;
      _idxKeySizeMax = 0 ;
   }

   _dmsStorageIndex::~_dmsStorageIndex()
   {
      _pDataSu->_detach() ;

      _pDataSu = NULL ;
   }

   void _dmsStorageIndex::syncMemToMmap ()
   {
      if ( _pDataSu )
      {
         _pDataSu->syncMemToMmap() ;
         _pDataSu->flushMME( isSyncDeep() ) ;
      }
   }

   dmsPageMapUnit* _dmsStorageIndex::getPageMapUnit()
   {
      return &_mbPageInfo ;
   }

   dmsPageMap* _dmsStorageIndex::getPageMap( UINT16 mbID )
   {
      return _mbPageInfo.getMap( mbID ) ;
   }

   UINT64 _dmsStorageIndex::_dataOffset()
   {
      return ( DMS_SME_OFFSET + DMS_SME_SZ ) ;
   }

   const CHAR* _dmsStorageIndex::_getEyeCatcher() const
   {
      return DMS_INDEXSU_EYECATCHER ;
   }

   UINT32 _dmsStorageIndex::_curVersion() const
   {
      return DMS_INDEXSU_CUR_VERSION ;
   }

   INT32 _dmsStorageIndex::_checkVersion( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;
      if ( pHeader->_version > _curVersion() )
      {
         PD_LOG( PDERROR, "Incompatible version: %u", pHeader->_version ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      }
      else if ( pHeader->_secretValue != _pStorageInfo->_secretValue )
      {
         PD_LOG( PDERROR, "Secret value[%llu] not the same with data su[%llu]",
                 pHeader->_secretValue, _pStorageInfo->_secretValue ) ;
         rc = SDB_DMS_SECRETVALUE_NOT_SAME ;
      }
      return rc ;
   }

   INT32 _dmsStorageIndex::_onCreate( OSSFILE * file, UINT64 curOffSet )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageIndex::_onMapMeta( UINT64 curOffSet )
   {
      return SDB_OK ;
   }

   INT32 _dmsStorageIndex::_onOpened()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needFlushMME = FALSE ;
      IDmsExtDataHandler *extHandler = _pDataSu->getExtDataHandler() ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         _pDataSu->_mbStatInfo[i]._idxLastWriteTick = ~0 ;
         _pDataSu->_mbStatInfo[i]._idxCommitFlag.init( 1 ) ;

         if ( DMS_IS_MB_INUSE ( _pDataSu->_dmsMME->_mbList[i]._flag ) )
         {
            /*
               Check the collection is valid
            */
            if ( !isCrashed() )
            {
               if ( 0 == _pDataSu->_dmsMME->_mbList[i]._idxCommitFlag )
               {
                  /// upgrade from the old version( _commitLSN = 0 )
                  if ( 0 == _pDataSu->_dmsMME->_mbList[i]._commitLSN )
                  {
                     _pDataSu->_dmsMME->_mbList[i]._commitLSN =
                        _pStorageInfo->_curLSNOnStart ;
                  }
                  _pDataSu->_dmsMME->_mbList[i]._idxCommitFlag = 1 ;
                  needFlushMME = TRUE ;
               }
               _pDataSu->_mbStatInfo[i]._idxCommitFlag.init( 1 ) ;
            }
            else
            {
               _pDataSu->_mbStatInfo[i]._idxCommitFlag.init(
                  _pDataSu->_dmsMME->_mbList[i]._idxCommitFlag ) ;
            }
            _pDataSu->_mbStatInfo[i]._idxIsCrash =
               ( 0 == _pDataSu->_mbStatInfo[i]._idxCommitFlag.peek() ) ?
                                      TRUE : FALSE ;
            _pDataSu->_mbStatInfo[i]._idxLastLSN.init(
               _pDataSu->_dmsMME->_mbList[i]._idxCommitLSN ) ;

            // analyze the unique index number
            for ( UINT32 j = 0 ; j < DMS_COLLECTION_MAX_INDEX ; ++j )
            {
               if ( DMS_INVALID_EXTENT ==
                    _pDataSu->_dmsMME->_mbList[i]._indexExtent[ j ] )
               {
                  break ;
               }
               ixmIndexCB indexCB( _pDataSu->_dmsMME->_mbList[i]._indexExtent[ j ],
                                   this, NULL ) ;
               if ( indexCB.isInitialized() )
               {
                  if ( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT,
                                            indexCB.getIndexType() ) )
                  {
                     _pDataSu->_mbStatInfo[i]._textIdxNum++ ;
                     // If there is any text indices, register the external
                     // data handler, and invoke the onOpenTextIdx method.
                     if ( !extHandler )
                     {
                        SDB_ASSERT( _pStorageInfo->_extDataHandler,
                                    "External data handler in storage info is "
                                    "NULL" ) ;
                        _pDataSu->regExtDataHandler( _pStorageInfo->_extDataHandler ) ;
                        extHandler = _pDataSu->getExtDataHandler() ;
                     }
                     if ( extHandler )
                     {
                        rc = extHandler->onOpenTextIdx( getSuName(),
                                                        _pDataSu->_dmsMME->_mbList[i]._collectionName,
                                                        indexCB ) ;
                        PD_RC_CHECK( rc, PDERROR, "External on text index open "
                                     "failed[ %d ]", rc ) ;
                     }
                  }
                  if ( indexCB.unique() )
                  {
                     _pDataSu->_mbStatInfo[i]._uniqueIdxNum++ ;
                  }
               }
            }
         }
      }

      if ( needFlushMME )
      {
         _pDataSu->flushMME( isSyncDeep() ) ;
      }

      _idxKeySizeMax = OSS_MIN( _pageSize / ( IXM_KEY_NODE_NUM_MIN + 1 ),
                                IXM_KEY_SIZE_LIMIT ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   void _dmsStorageIndex::_onClosed()
   {
      /// Flush all pageMap to disk
      UINT16 pos = 0 ;
      dmsPageMap *pPageMap = NULL ;
      dmsPageMap::MAP_PAGES_IT it ;

      pPageMap = _mbPageInfo.beginNonEmpty( pos ) ;
      while( pPageMap )
      {
         it = pPageMap->begin() ;
         while( it != pPageMap->end() )
         {
            ixmExtent extent( it->first, this ) ;
            extent.setParent( it->second, FALSE ) ;
            ++it ;
         }
         pPageMap->clear() ;

         pPageMap = _mbPageInfo.nextNonEmpty( pos ) ;
      }

      {
         dmsTransLockCallback callback( pmdGetKRCB()->getTransCB(), NULL ) ;
         callback.onCSClosed( _pDataSu->CSID() ) ;
      }
   }

   INT32 _dmsStorageIndex::_onFlushDirty( BOOLEAN force, BOOLEAN sync )
   {
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _pDataSu->_mbStatInfo[i]._idxCommitFlag.init( 1 ) ;
      }

      UINT16 pos = 0 ;
      BOOLEAN locked = FALSE ;
      dmsPageMap *pPageMap = NULL ;
      dmsPageMap::MAP_PAGES_IT it ;

      pPageMap = _mbPageInfo.beginNonEmpty( pos ) ;
      while( pPageMap )
      {
         while( !pPageMap->isEmpty() )
         {
            /// lock
            _pDataSu->_mblock[ pos ].get() ;
            locked = TRUE ;

            it = pPageMap->begin() ;
            if( it != pPageMap->end() )
            {
               ixmExtent extent( it->first, this ) ;
               extent.setParent( it->second, FALSE ) ;
               pPageMap->erase( it ) ;
            }
            else
            {
               break ;
            }

            if ( _pDataSu->_mbStatInfo[pos]._idxCommitFlag.compare( 0 ) )
            {
               break ;
            }
            /// unlock
            _pDataSu->_mblock[ pos ].release() ;
            locked = FALSE ;
         }

         if ( locked )
         {
            /// unlock
            _pDataSu->_mblock[ pos ].release() ;
            locked = FALSE ;
         }

         pPageMap = _mbPageInfo.nextNonEmpty( pos ) ;
      }

      return SDB_OK ;
   }

   INT32 _dmsStorageIndex::_onMarkHeaderValid( UINT64 &lastLSN,
                                               BOOLEAN sync,
                                               UINT64 lastTime )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needFlush = FALSE ;
      UINT64 tmpLSN = 0 ;
      UINT32 tmpCommitFlag = 0 ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _pDataSu->_dmsMME->_mbList[i]._flag ) &&
              _pDataSu->_mbStatInfo[i]._idxCommitFlag.peek() )
         {
            tmpLSN = _pDataSu->_mbStatInfo[i]._idxLastLSN.peek() ;
            tmpCommitFlag = _pDataSu->_mbStatInfo[i]._idxIsCrash ?
               0 : _pDataSu->_mbStatInfo[i]._idxCommitFlag.peek() ;

            if ( tmpLSN != _pDataSu->_dmsMME->_mbList[i]._idxCommitLSN ||
                 tmpCommitFlag != _pDataSu->_dmsMME->_mbList[i]._idxCommitFlag )
            {
               _pDataSu->_dmsMME->_mbList[i]._idxCommitLSN = tmpLSN ;
               _pDataSu->_dmsMME->_mbList[i]._idxCommitTime = lastTime ;
               _pDataSu->_dmsMME->_mbList[i]._idxCommitFlag = tmpCommitFlag ;
               needFlush = TRUE ;
            }

            /// update last lsn
            if ( (UINT64)~0 == lastLSN ||
                 ( (UINT64)~0 != tmpLSN && lastLSN < tmpLSN ) )
            {
               lastLSN = tmpLSN ;
            }
         }
      }

      if ( needFlush )
      {
         rc = _pDataSu->flushMME( sync ) ;
      }
      return rc ;
   }

   INT32 _dmsStorageIndex::_onMarkHeaderInvalid( INT32 collectionID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needSync = FALSE ;

      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         _pDataSu->_mbStatInfo[ collectionID ]._idxLastWriteTick =
            pmdGetDBTick() ;
         if ( !_pDataSu->_mbStatInfo[ collectionID ]._idxIsCrash &&
              _pDataSu->_mbStatInfo[ collectionID
              ]._idxCommitFlag.compareAndSwap( 1, 0 ) )
         {
            needSync = TRUE ;
            _pDataSu->_dmsMME->_mbList[ collectionID ]._idxCommitFlag = 0 ;
         }
      }
      else if ( -1 == collectionID )
      {
         for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
         {
            _pDataSu->_mbStatInfo[ i ]._idxLastWriteTick = pmdGetDBTick() ;
            if ( DMS_IS_MB_INUSE ( _pDataSu->_dmsMME->_mbList[i]._flag ) &&
                 !_pDataSu->_mbStatInfo[ i ]._idxIsCrash &&
                 _pDataSu->_mbStatInfo[ i
                 ]._idxCommitFlag.compareAndSwap( 1, 0 ) )
            {
               needSync = TRUE ;
               _pDataSu->_dmsMME->_mbList[ i ]._idxCommitFlag = 0 ;
            }
         }
      }

      if ( needSync )
      {
         rc = _pDataSu->flushMME( isSyncDeep() ) ;
      }
      return rc ;
   }

   UINT64 _dmsStorageIndex::_getOldestWriteTick() const
   {
      UINT64 oldestWriteTick = ~0 ;
      UINT64 lastWriteTick = 0 ;

      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         lastWriteTick = _pDataSu->_mbStatInfo[i]._idxLastWriteTick ;
         /// The collection is commit valid, should ignored
         if ( 0 == _pDataSu->_mbStatInfo[i]._idxCommitFlag.peek() &&
              lastWriteTick < oldestWriteTick )
         {
            oldestWriteTick = lastWriteTick ;
         }
      }
      return oldestWriteTick ;
   }

   void _dmsStorageIndex::_onRestore()
   {
      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         _pDataSu->_mbStatInfo[i]._idxIsCrash = FALSE ;
      }
   }

   // Find a free slot in mb index extent array for the new index. Duplication
   // of index name and definition will be checked.
   INT32 _dmsStorageIndex::_allocateIdxID( dmsMBContext *context,
                                           const CHAR *indexName,
                                           const BSONObj &index,
                                           INT32 &indexID )
   {
      INT32 rc = SDB_OK ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; indexID++ )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB curIdxCB( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         BOOLEAN sameName = ( 0 == ossStrncmp( indexName,
                                               curIdxCB.getName(),
                                               IXM_INDEX_NAME_SIZE ) ) ;
         if ( sameName )
         {
            if ( curIdxCB.isSameDef( index, TRUE ) )
            {
               PD_LOG( PDERROR, "Same index defined already:[%s:%s]",
                       curIdxCB.getName(),
                       index.getStringField( IXM_FIELD_NAME_NAME ) ) ;
               rc = SDB_IXM_REDEF ;
            }
            else
            {
               PD_LOG ( PDINFO, "Duplicate index name: %s",
                        index.getStringField( IXM_FIELD_NAME_NAME ) );
               rc = SDB_IXM_EXIST;
            }
            goto error ;
         }
         else if ( curIdxCB.isSameDef( index ) )
         {
            PD_LOG ( PDERROR, "Duplicate index define: %s",
                     index.getStringField( IXM_FIELD_NAME_NAME ) );
            rc = SDB_IXM_EXIST_COVERD_ONE ;
            goto error ;
         }
         else
         {
            continue ;
         }
      }
      if ( DMS_COLLECTION_MAX_INDEX == indexID )
      {
         rc = SDB_DMS_MAX_INDEX ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageIndex::reserveExtent( UINT16 mbID, dmsExtentID &extentID,
                                          _dmsContext * context )
   {
      SDB_ASSERT( mbID < DMS_MME_SLOTS, "Invalid metadata block ID" ) ;

      INT32 rc                = SDB_OK ;
      dmsExtRW extRW ;
      dmsExtent *extAddr      = NULL ;
      extentID                = DMS_INVALID_EXTENT ;

      rc = _findFreeSpace ( 1, extentID, NULL/*context*/ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error find free space for %d pages, rc = %d",
                  1, rc ) ;
         goto error ;
      }

      extRW = extent2RW( extentID, context->mbID() ) ;
      extAddr = extRW.writePtr<dmsExtent>() ;
      extAddr->init( 1, mbID, pageSize() ) ;

      _pDataSu->_mbStatInfo[mbID]._totalIndexPages += 1 ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::releaseExtent( dmsExtentID extentID,
                                          BOOLEAN setFlag )
   {
      INT32 rc                   = SDB_OK ;
      dmsExtRW extRW ;
      const dmsExtent *extAddr   = NULL ;

      extRW = extent2RW( extentID ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.readPtr<dmsExtent>() ;
      if ( !extAddr || DMS_EXTENT_FLAG_INUSE != extAddr->_flag )
      {
         PD_LOG ( PDERROR, "extent id %d is not in use", extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /*
       * To improve the perfomance, so we need not change the page info
       * when setFlag == FALSE
      */
      if ( setFlag )
      {
         dmsExtent *writeExtent = NULL ;
         writeExtent = extRW.writePtr<dmsExtent>() ;
         writeExtent->_flag = DMS_EXTENT_FLAG_FREED ;
      }

      _pDataSu->_mbStatInfo[extAddr->_mbID]._totalIndexPages -= 1 ;
      rc = _releaseSpace( extentID, 1 ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page, rc = %d", rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::createIndex( dmsMBContext *context,
                                        const BSONObj &index,
                                        pmdEDUCB * cb,
                                        SDB_DPSCB *dpscb,
                                        BOOLEAN isSys,
                                        INT32 sortBufferSize,
                                        utilWriteResult *pResult )
   {
      INT32 rc                     = SDB_OK ;
      dmsExtentID metaExtentID     = DMS_INVALID_EXTENT ;
      dmsExtentID rootExtentID     = DMS_INVALID_EXTENT ;
      BOOLEAN ready                = FALSE ;
      UINT16 indexType             = 0 ;

      if ( !ixmIndexCB::validateKey ( index, isSys ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Index pattern is not valid, rc=%d", rc ) ;
         goto error ;
      }

      // Generate the index type out side of the mb lock. Depending on the type,
      // different actions will be taken.
      if ( !ixmIndexCB::generateIndexType( index, indexType ) )
      {
         PD_LOG_MSG( PDERROR, "Generate index type failed" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // let's first reserve extent
      rc = reserveExtent ( context->mbID(), metaExtentID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reserve extent for collection[%u], "
                   "rc: %d", context->mbID(), rc ) ;

      // then let's reserve another extent for root extent ID
      rc = reserveExtent ( context->mbID(), rootExtentID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reserve root extent for collection"
                   "[%u], rc: %d", context->mbID(), rc ) ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_CRT_INDEX ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      // Once we are ready, the internal function should release the extents by
      // themselves in case of any error.
      ready = TRUE ;

      if ( IXM_EXTENT_HAS_TYPE( IXM_EXTENT_TYPE_TEXT, indexType ) )
      {
         rc = _createTextIdx( context, index, metaExtentID,
                              rootExtentID, cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Create text index failed, rc: %d", rc ) ;
      }
      else
      {
         rc = _createIndex( context, index, metaExtentID, rootExtentID,
                            indexType, cb, dpscb, isSys, sortBufferSize,
                            pResult ) ;
         PD_RC_CHECK (rc, PDERROR, "Create index failed, rc: %d", rc ) ;
      }

   done :
      return rc ;
   error :
      if ( !ready )
      {
         if ( DMS_INVALID_EXTENT != metaExtentID )
         {
            releaseExtent ( metaExtentID, TRUE ) ;
         }
         if ( DMS_INVALID_EXTENT != rootExtentID )
         {
            releaseExtent ( rootExtentID ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEINDEX_DROPALLIDXES, "_dmsStorageIndex::dropAllIndexes" )
   INT32 _dmsStorageIndex::dropAllIndexes( dmsMBContext *context, pmdEDUCB *cb,
                                           SDB_DPSCB * dpscb )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEINDEX_DROPALLIDXES );
      INT32 rc = SDB_OK ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      while ( DMS_INVALID_EXTENT != context->mb()->_indexExtent[0] )
      {
         ixmIndexCB indexCB( context->mb()->_indexExtent[0], this, context ) ;
         rc = dropIndex( context, 0, indexCB.getLogicalID(), cb,
                         dpscb, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Drop index[%d] failed, rc: %d", 0,
                      rc ) ;
      }
      context->mbStat()->_totalIndexPages = 0 ;
      context->mbStat()->_totalIndexFreeSpace = 0 ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEINDEX_DROPALLIDXES, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEINDEX_DROPIDX1, "_dmsStorageIndex::dropIndex" )
   INT32 _dmsStorageIndex::dropIndex( dmsMBContext *context, OID &indexOID,
                                      pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                      BOOLEAN isSys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEINDEX_DROPIDX1 );
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      OID oid ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_DROP_INDEX ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID],
                             this, context ) ;
         rc = indexCB.getIndexID ( oid ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to get oid for %d index", indexID ) ;
            goto error ;
         }
         if ( indexOID == oid )
         {
            found = TRUE ;

            if ( _pDataSu->_pEventHolder )
            {
               dmsEventCLItem clItem( context->mb()->_collectionName,
                                      context->mbID(),
                                      context->clLID() ) ;
               dmsEventIdxItem idxItem( indexCB.getName(),
                                        indexCB.getLogicalID(),
                                        indexCB.getDef() ) ;
               _pDataSu->_pEventHolder->onDropIndex( DMS_EVENT_MASK_ALL,
                                                     clItem, idxItem, cb,
                                                     dpscb ) ;
            }

            rc = dropIndex ( context, indexID, indexCB.getLogicalID(),
                             cb, dpscb, isSys ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to drop index %d:%s", context->mbID(),
                        indexOID.toString().c_str() ) ;
               goto error ;
            }
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEINDEX_DROPIDX1, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::dropIndex( dmsMBContext *context,
                                      const CHAR *indexName,
                                      pmdEDUCB *cb, SDB_DPSCB * dpscb,
                                      BOOLEAN isSys )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      dpsTransCB *transCB          = sdbGetTransCB() ;
      BOOLEAN lockedCL             = FALSE ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_DROP_INDEX ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      else if ( 0 == ossStrcmp( IXM_ID_KEY_NAME, indexName ) &&
                _pDataSu->isTransSupport() &&
                NULL != cb )
      {
         if ( transCB->isTransOn() &&
              cb->getTransExecutor()->useTransLock() )
         {
            // if transaction is on, need to get S lock of collection to
            // avoid transactions have inserted/updated/deleted records
            // ( including this session itself if it is in transaction )
            // on the same collection ( if $id index is dropped, they won't
            // be able to rollback )
            dpsTransRetInfo lockConflict ;
            rc = transCB->transLockTryS( cb, _pDataSu->_logicalCSID,
                                         context->mbID(),  NULL,
                                         &lockConflict ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to lock the collection, rc: %d"OSS_NEWLINE
                         "Conflict( representative ):"OSS_NEWLINE
                         "   EDUID:  %llu"OSS_NEWLINE
                         "   TID:    %u"OSS_NEWLINE
                         "   LockId: %s"OSS_NEWLINE
                         "   Mode:   %s"OSS_NEWLINE,
                         rc,
                         lockConflict._eduID,
                         lockConflict._tid,
                         lockConflict._lockID.toString().c_str(),
                         lockModeToString( lockConflict._lockType ) ) ;

            lockedCL = TRUE ;
         }
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         if ( 0 == ossStrncmp ( indexName, indexCB.getName(),
                                IXM_INDEX_NAME_SIZE ) )
         {
            found = TRUE ;

            if ( _pDataSu->_pEventHolder )
            {
               dmsEventCLItem clItem( context->mb()->_collectionName,
                                      context->mbID(),
                                      context->clLID() ) ;
               dmsEventIdxItem idxItem( indexCB.getName(),
                                        indexCB.getLogicalID(),
                                        indexCB.getDef() ) ;
               _pDataSu->_pEventHolder->onDropIndex( DMS_EVENT_MASK_ALL,
                                                     clItem, idxItem, cb,
                                                     dpscb ) ;
            }

            rc = dropIndex ( context, indexID, indexCB.getLogicalID(),
                             cb, dpscb, isSys ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to drop index %d:%s", context->mbID(),
                        indexName ) ;
               goto error ;
            }
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
      }

   done :
      if ( lockedCL )
      {
         transCB->transLockRelease( cb, _pDataSu->logicalID(),
                                    context->mbID(), NULL, NULL ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEINDEX_DROPIDX2, "_dmsStorageIndex::dropIndex" )
   INT32 _dmsStorageIndex::dropIndex( dmsMBContext *context, INT32 indexID,
                                      dmsExtentID indexLID, pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb,
                                      BOOLEAN isSys )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEINDEX_DROPIDX2 );

      INT32 rc                     = SDB_OK ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      dpsTransCB *pTransCB         = pmdGetKRCB()->getTransCB() ;
      dpsMergeInfo info ;
      dpsLogRecord &record  = info.getMergeBlock().record() ;
      UINT32 logRecSize            = 0 ;
      BSONObj indexDef ;
      IDmsExtDataHandler *extDataHandler = NULL ;

      dmsTransLockCallback callback( pmdGetKRCB()->getTransCB(),
                                     cb ) ;

      PD_TRACE2( SDB__DMSSTORAGEINDEX_DROPIDX2,
                 PD_PACK_INT(indexID),
                 PD_PACK_INT(indexLID) );

      rc = context->mbLock( EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to lock mb context[%s], rc: %d",
                 context->toString().c_str(), rc ) ;
         goto error ;
      }

      if ( indexID >= DMS_COLLECTION_MAX_INDEX )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }

      {
         // get index control block
         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         // verify the index control block is initialized
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         if ( indexLID != indexCB.getLogicalID() )
         {
            PD_LOG( PDERROR, "Index logical id not same, cur id: %d, "
                    "expect id: %d", indexLID, indexCB.getLogicalID() ) ;
            rc = SDB_IXM_NOTEXIST ;
            goto error ;
         }
         if ( IXM_INDEX_FLAG_NORMAL != indexCB.getFlag() &&
              IXM_INDEX_FLAG_INVALID != indexCB.getFlag() )
         {
            PD_LOG ( PDWARNING, "Index is either creating or dropping: %d(%s)",
                     (INT32)indexCB.getFlag(),
                     ixmGetIndexFlagDesp( indexCB.getFlag() ) ) ;
         }
         if ( !isSys && 0 == ossStrcmp ( indexCB.getName(), IXM_ID_KEY_NAME ) )
         {
            PD_LOG ( PDERROR, "Cannot drop $id index" ) ;
            rc = SDB_IXM_DROP_ID ;
            goto error ;
         }
         if ( !isSys && 0 == ossStrcmp ( indexCB.getName(),
                                         IXM_SHARD_KEY_NAME ) )
         {
            PD_LOG ( PDERROR, "Cannot drop $shard index" ) ;
            rc = SDB_IXM_DROP_SHARD ;
            goto error ;
         }

         if ( isSys && 0 == ossStrcmp( indexCB.getName(),
                                       IXM_ID_KEY_NAME ) )
         {
            OSS_BIT_SET( context->mb()->_attributes,
                         DMS_MB_ATTR_NOIDINDEX ) ;
         }

         _pDataSu->_clFullName( context->mb()->_collectionName, fullName,
                                sizeof(fullName) ) ;
         // reserved log-size
         if ( dpscb )
         {
            indexDef = indexCB.getDef().getOwned() ;

            rc = dpsIXDel2Record( fullName, indexDef, record ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

            rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                         rc ) ;

            logRecSize = record.alignedLen() ;
            rc = pTransCB->reservedLogSpace( logRecSize, cb );
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                       logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // If it's text index, let's gather information for external operation.
         if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                   IXM_EXTENT_TYPE_TEXT ) )
         {
            extDataHandler = _pDataSu->getExtDataHandler() ;
            if ( !extDataHandler )
            {
               SDB_ASSERT( FALSE, "External data handler is NULL" ) ;
               PD_LOG( PDERROR, "External data handler is NULL" ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            rc = extDataHandler->onDropTextIdx( indexCB.getExtDataName(), cb ) ;
            if ( SDB_DMS_CS_NOTEXIST == rc )
            {
               // if capped cs doesn't exist, we just ignor error
               rc = SDB_OK ;
            }
            if ( rc )
            {
               PD_LOG( PDERROR, "External process of dropping text index[%s]"
                                "failed[%d]", indexCB.getName(), rc ) ;
               // Only return error when the index is in normal state.
               // Otherwise, meta of the index should always be deleted.
               if ( IXM_INDEX_FLAG_NORMAL == indexCB.getFlag() )
               {
                  goto error ;
               }
            }
         }

         callback.setIDInfo( _pDataSu->CSID(), context->mbID(),
                             _pDataSu->logicalID(),
                             context->clLID() ) ;
         rc = callback.onDropIndex( context, &indexCB, cb ) ;
         if ( rc )
         {
            goto error ;
         }

         // truncate index, do remove root
         rc = indexCB.truncate ( TRUE, IXM_INDEX_FLAG_DROPPING ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index, rc: %d", rc ) ;
            goto error ;
         }
         // set to dropping
         indexCB.clearLogicID() ;

         if ( indexCB.unique() )
         {
            context->mbStat()->_uniqueIdxNum-- ;
         }

         // release index control block extent
         rc = releaseExtent ( context->mb()->_indexExtent[indexID], TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to release indexCB extent: %d",
                     context->mb()->_indexExtent[indexID] ) ;
            indexCB.setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            goto error ;
         }

         // copy back
         ossMemmove (&context->mb()->_indexExtent[indexID],
                     &context->mb()->_indexExtent[indexID+1],
                     sizeof(dmsExtentID)*(DMS_COLLECTION_MAX_INDEX-indexID-1));
         context->mb()->_indexExtent[DMS_COLLECTION_MAX_INDEX-1] =
            DMS_INVALID_EXTENT ;
         if ( extDataHandler )
         {
            rc = extDataHandler->done( DMS_EXTOPR_TYPE_DROPIDX, cb ) ;
            if ( rc )
            {
               // Report the error, but not go to error. Manually cleanup maybe
               // needed.
               PD_LOG( PDERROR, "External operation failed, rc: %d", rc ) ;
            }

            // Maybe we are deleting the index because of error when creating it.
            // In that case, the _textIdxNum may have not increase. So we only need
            // to decrease it when it's greater than 0.
            if ( context->mbStat()->_textIdxNum > 0 )
            {
               SDB_ASSERT( 1 == context->mbStat()->_textIdxNum,
                           "Only support 1 text index now" ) ;
               context->mbStat()->_textIdxNum-- ;
            }
         }
      }

      context->mb()->_numIndexes -- ;

      // log it
      if ( dpscb )
      {
         rc = _pDataSu->_logDPS( dpscb, info, cb, context,
                                 DMS_INVALID_EXTENT, TRUE,
                                 DMS_FILE_IDX ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert ixdel into log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_IDX,
                                                   cb->isDoRollback() ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      if ( SDB_OK == rc )
      {
         _pDataSu->flushMME( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEINDEX_DROPIDX2, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::_createIndex( dmsMBContext *context,
                                         const BSONObj &index,
                                         dmsExtentID metaExtentID,
                                         dmsExtentID rootExtentID,
                                         UINT16 indexType,
                                         pmdEDUCB *cb,
                                         SDB_DPSCB *dpscb,
                                         BOOLEAN isSys,
                                         INT32 sortBufferSize,
                                         utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      INT32 indexID = 0 ;
      const CHAR *indexName = NULL ;
      BSONObj indexDef ;
      dmsExtentID indexLID = DMS_INVALID_EXTENT ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      UINT32 logRecSize = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      SDB_DPSCB *dropDps = NULL ;
      OID indexOID ;    // Used for dropping THIS index in case of error.
      dmsTransLockCallback callback( pmdGetKRCB()->getTransCB(), cb ) ;
      IDmsOprHandler *pOprHandler = NULL ;

      SDB_ASSERT( context->isMBLock(), "Caller should hold mb lock" ) ;
      SDB_ASSERT( DMS_INVALID_EXTENT != metaExtentID,
                  "meta extent id is invalid" )  ;
      SDB_ASSERT( DMS_INVALID_EXTENT != metaExtentID,
                  "root extent id is invalid" )  ;

      indexName = index.getStringField( IXM_FIELD_NAME_NAME ) ;

      rc = _allocateIdxID( context, indexName, index, indexID ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate index id failed: %d", rc ) ;

      {
         // initialize index control block, set flag to invalid
         ixmIndexCB indexCB ( metaExtentID, index, context->mbID(),
                              this, context ) ;
         // verify the index control block is initialized
         if ( !indexCB.isInitialized() )
         {
            // if we can't initialize control block, we shouldn't call dropIndex
            // at the moment. So let's just reset _indexExtent to invalid and
            // free the extents
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         //set logical id
         indexLID = context->mb()->_indexHWCount ;
         indexCB.setLogicalID( indexLID ) ;
         // Get the index definition from indexCB instead of using index
         // directly, because extra fields may have been added to the
         // definition. For example, _id is added on primary node.
         indexDef = indexCB.getDef().getOwned() ;
         indexCB.getIndexID( indexOID ) ;

         // calc the reserve size
         if ( dpscb )
         {
            // invoke callback function
            pOprHandler = &callback ;
            callback.setIDInfo( _pDataSu->CSID(), context->mbID(),
                                _pDataSu->logicalID(),
                                context->clLID() ) ;
            rc = callback.onCreateIndex( context, &indexCB, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Create index failed to invoke callback, "
                       "rc = %d", rc ) ;
               goto error ;
            }

            _pDataSu->_clFullName( context->mb()->_collectionName, fullName,
                                   sizeof(fullName) ) ;

            rc = dpsIXCrt2Record( fullName, indexDef, record ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record:%d", rc ) ;

            rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                         rc ) ;

            logRecSize = record.alignedLen() ;
            rc = pTransCB->reservedLogSpace( logRecSize, cb );
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                       logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // initialize the root extent
         {
            // once the control block is allocated, let's do root extent
            ixmExtent idx( rootExtentID, context->mbID(), this ) ;
         }
         indexCB.setRoot ( rootExtentID ) ;

         if ( indexCB.unique() )
         {
            context->mbStat()->_uniqueIdxNum++ ;
         }
      }

      // change mb metadata
      context->mb()->_indexExtent[indexID] = metaExtentID ;
      context->mb()->_numIndexes ++ ;
      context->mb()->_indexHWCount++ ;

      // create index callback
      if ( _pDataSu->_pEventHolder )
      {
         dmsEventCLItem clItem( context->mb()->_collectionName,
                                context->mbID(),
                                context->clLID() ) ;
         dmsEventIdxItem idxItem ( indexName, indexLID, index ) ;
         _pDataSu->_pEventHolder->onCreateIndex( DMS_EVENT_MASK_ALL,
                                                 clItem, idxItem,
                                                 cb, dpscb ) ;
      }

      // Note:
      // The mb lock will be released after dps log is written. The collection
      // and index are out of protection. So the collection as well the index
      // may be modified, dropped, or re-created. Be sure to consider all of
      // these possibilities!
      if ( dpscb )
      {
         rc = _pDataSu->_logDPS( dpscb, info, cb, context, DMS_INVALID_EXTENT,
                                 TRUE, DMS_FILE_IDX ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to insert ixcrt into log, rc = %d", rc ) ;
            goto error_after_create ;
         }
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_IDX,
                                                   cb->isDoRollback() ) ;
      }
      dropDps = dpscb ;

      /// flush some page
      flushPages( metaExtentID, 1, isSyncDeep() ) ;
      flushPages( rootExtentID, 1, isSyncDeep() ) ;

      // now we finished allocation part, let's get into build part
      // As the mb lock has been released, the rebuild implementation should use
      // the context and indexLID to check if it's processing the right index.
      rc = _rebuildIndex( context, metaExtentID, indexLID,
                          cb, sortBufferSize, indexType,
                          pOprHandler, pResult ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to build index[%s], rc = %d",
                 indexDef.toString().c_str(), rc ) ;
         goto error_after_create ;
      }

      rc = context->mbLock( EXCLUSIVE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to lock mb:%d", rc ) ;
         goto error_after_create ;
      }

      // Check once agin if the index is still the one we are creating. Be sure
      // we are not doing anything wrong relatived with the index.
      {
         ixmIndexCB indexCB( metaExtentID, this, context ) ;
         if ( !indexCB.isInitialized() || !indexCB.isStillValid( indexOID ) )
         {
            rc = SDB_DMS_INVALID_INDEXCB ;
            PD_LOG( PDERROR, "Original indexCB is no longer valid. The index "
                             "may have been dropped by someone else" ) ;
            goto error_after_create ;
         }
      }

      // if it is $oid, set DMS_MB_ATTR_NOIDINDEX with false
      if ( isSys && 0 == ossStrcmp( indexName, IXM_ID_KEY_NAME ) )
      {
         OSS_BIT_CLEAR( context->mb()->_attributes,
                        DMS_MB_ATTR_NOIDINDEX ) ;
      }

      // rebuild index callback
      if ( _pDataSu->_pEventHolder )
      {
         dmsEventCLItem clItem( context->mb()->_collectionName,
                                context->mbID(),
                                context->clLID() ) ;
         dmsEventIdxItem idxItem ( indexName, indexLID, index ) ;
         _pDataSu->_pEventHolder->onRebuildIndex( DMS_EVENT_MASK_ALL,
                                                  clItem, idxItem,
                                                  cb, dpscb ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         _pDataSu->flushMME( isSyncDeep() ) ;
      }
      return rc ;
   error :

      releaseExtent ( metaExtentID, TRUE ) ;
      releaseExtent ( rootExtentID ) ;
      goto done ;
   error_after_create :
      INT32 rc1 = SDB_OK ;
      // Use the OID for dropping to avoid dropping the wrong index created by
      // others in concurrent scenario.
      rc1 = dropIndex ( context, indexOID, cb, dropDps, isSys ) ;
      if ( rc1 )
      {
         PD_LOG ( PDERROR, "Failed to clean up invalid index, rc = %d", rc1 ) ;
      }
      goto done ;
   }

   INT32 _dmsStorageIndex::_checkForCrtTextIdx( dmsMBContext *context,
                                                const BSONObj &index )
   {
      INT32 rc = SDB_OK ;

      // Number of text index on colleection is limited.
      if ( context->mbStat()->_textIdxNum >= DMS_MAX_TEXT_IDX_NUM )
      {
         rc = SDB_DMS_MAX_INDEX ;
         PD_LOG( PDERROR, "Max number[%d] of text indexes have been created on "
                          "collection[%s] already", DMS_MAX_TEXT_IDX_NUM,
                          context->mb()->_collectionName ) ;
         goto error ;
      }

      // We need the unique id to generate the capped collection name. Not able
      // to do that if the id is invalid.
      if ( UTIL_UNIQUEID_NULL == context->mb()->_clUniqueID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Cannot create text index on collection[%s] as "
                          "its unique id is invalid",
                          context->mb()->_collectionName ) ;
         goto error ;
      }

      {
         BSONObj keys = index.getObjectField( IXM_KEY_FIELD ) ;
         if ( keys.hasField( "_id" ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Text index can't include _id field" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageIndex::_createTextIdx( dmsMBContext *context,
                                           const BSONObj &index,
                                           dmsExtentID metaExtentID,
                                           dmsExtentID rootExtentID,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpscb )
   {
      INT32 rc = SDB_OK ;
      INT32 indexID = 0 ;
      const CHAR *indexName = NULL ;
      BSONObj indexDef ;
      dmsExtentID indexLID = DMS_INVALID_EXTENT ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      UINT32 logRecSize = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      SDB_DPSCB *dropDps = NULL ;
      OID indexOID ;    // Used for dropping THIS index in case of error.
      IDmsExtDataHandler *handler  = _pStorageInfo->_extDataHandler ;

      SDB_ASSERT( handler, "External handler is NULL" ) ;
      SDB_ASSERT( context && context->isMBLock(),
                  "Caller should hold mb lock" ) ;
      SDB_ASSERT( DMS_INVALID_EXTENT != metaExtentID,
                  "meta extent id is invalid" )  ;
      SDB_ASSERT( DMS_INVALID_EXTENT != metaExtentID,
                  "root extent id is invalid" )  ;

      indexName = index.getStringField( IXM_FIELD_NAME_NAME ) ;

      rc = _checkForCrtTextIdx( context, index ) ;
      PD_RC_CHECK( rc, PDERROR, "Check for creating text index failed[%d]",
                   rc ) ;

      rc = _allocateIdxID( context, indexName, index, indexID ) ;
      PD_RC_CHECK( rc, PDERROR, "Allocate index id failed: %d", rc ) ;

      if ( NULL == _pDataSu->getExtDataHandler() )
      {
         _pDataSu->regExtDataHandler( handler ) ;
      }

      {
         // initialize index control block, set flag to invalid
         ixmIndexCB indexCB( metaExtentID, index, context->mbID(), this,
                             context ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG( PDERROR, "Initialize index failed" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         indexLID = context->mb()->_indexHWCount ;
         indexCB.setLogicalID( indexLID ) ;
         indexDef = indexCB.getDef().getOwned() ;

         // For veriication later.
         indexCB.getIndexID( indexOID ) ;

         if ( dpscb )
         {
            _pDataSu->_clFullName( context->mb()->_collectionName, fullName,
                                   sizeof( fullName ) ) ;

            rc = dpsIXCrt2Record( fullName, indexDef, record ) ;
            PD_RC_CHECK( rc, PDERROR, "Build record failed[%d]", rc ) ;

            rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Check sync control failed[%d]", rc ) ;

            logRecSize = record.alignedLen() ;
            rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Reserved log space failed[%d]. Length[%u]",
                       rc, logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // For text index, the root extent is not used. But to be unified,
         // initialize it too.
         {
            ixmExtent idx( rootExtentID, context->mbID(), this ) ;
         }
         indexCB.setRoot( rootExtentID ) ;
         context->mb()->_indexExtent[ indexID ] = metaExtentID ;
         context->mb()->_numIndexes++ ;
         context->mb()->_indexHWCount++ ;
         context->mbStat()->_textIdxNum++ ;

         rc = handler->onCrtTextIdx( context, getSuName(), indexCB, cb, NULL ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "External operation on creating text index "
                             "failed[%d]", rc ) ;
            goto error_after_create ;
         }

         rc = handler->done( DMS_EXTOPR_TYPE_CRTIDX, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "External done operation on creating text index "
                             "failed[%d]", rc ) ;
            goto error_after_create ;
         }

         // create index callback
         if ( _pDataSu->_pEventHolder )
         {
            dmsEventCLItem clItem( context->mb()->_collectionName,
                                   context->mbID(),
                                   context->clLID() ) ;
            dmsEventIdxItem idxItem ( indexName, indexLID, index ) ;
            _pDataSu->_pEventHolder->onCreateIndex( DMS_EVENT_MASK_ALL, clItem,
                                                    idxItem, cb, dpscb ) ;
         }
      }

      if ( dpscb )
      {
         rc = _pDataSu->_logDPS( dpscb, info, cb, context, DMS_INVALID_EXTENT,
                                 TRUE, DMS_FILE_IDX ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Insert ixcrt into log failed[%d]", rc ) ;
            goto error_after_create ;
         }
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_IDX,
                                                   cb->isDoRollback() ) ;
      }
      dropDps = dpscb ;

      flushPages( metaExtentID, 1, isSyncDeep() ) ;
      flushPages( rootExtentID, 1, isSyncDeep() ) ;

      rc = context->mbLock( EXCLUSIVE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Lock mb failed[%d]", rc ) ;
         goto error_after_create ;
      }

      {
         // Check for the last time if this is the index we are creating.
         ixmIndexCB indexCB( metaExtentID, this, context ) ;
         if ( !indexCB.isInitialized() || !indexCB.isStillValid( indexOID ) )
         {
            rc = SDB_DMS_INVALID_INDEXCB ;
            PD_LOG( PDERROR, "Original indexCB is no longer valid. The index "
                             "may have been dropped by someone else" ) ;
            goto error_after_create ;
         }
      }

      // rebuild index callback
      if ( _pDataSu->_pEventHolder )
      {
         dmsEventCLItem clItem( context->mb()->_collectionName,
                                context->mbID(),
                                context->clLID() ) ;
         dmsEventIdxItem idxItem ( indexName, indexLID, index ) ;
         _pDataSu->_pEventHolder->onRebuildIndex( DMS_EVENT_MASK_ALL, clItem,
                                                  idxItem, cb, dpscb ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         _pDataSu->flushMME( isSyncDeep() ) ;
      }
      return rc ;
   error :
      SDB_ASSERT( SDB_DMS_INVALID_INDEXCB != rc, "Index cb is invalid" ) ;
      releaseExtent ( metaExtentID, TRUE ) ;
      releaseExtent ( rootExtentID ) ;
      goto done ;
   error_after_create :
      // Whether replication log will be written depends on the value of
      // dropDps. Only when the creation has been logged that it has valid
      // value.
      INT32 rc1 = SDB_OK ;
      rc1 = dropIndex ( context, indexOID, cb, dropDps, FALSE ) ;
      if ( rc1 )
      {
         PD_LOG ( PDERROR, "Failed to clean up invalid index, rc = %d", rc1 ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEINDEX__REBUILDINDEX, "_dmsStorageIndex::_rebuildIndex" )
   INT32 _dmsStorageIndex::_rebuildIndex( dmsMBContext *context,
                                          dmsExtentID indexExtentID,
                                          dmsExtentID indexLID,
                                          pmdEDUCB *cb,
                                          INT32 sortBufferSize,
                                          UINT16 indexType,
                                          IDmsOprHandler *pOprHandle,
                                          utilWriteResult *pResult,
                                          dmsDupKeyProcessor *dkProcessor )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEINDEX__REBUILDINDEX );

      INT32 rc = SDB_OK ;
      dmsIndexBuilder* builder = NULL ;
      BOOLEAN needUnlock = FALSE ;

      if ( sortBufferSize < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "invalid index sort buffer size, rc=%d", rc ) ;
         goto error ;
      }

      // invoke callback function
      if ( pOprHandle )
      {
         // indexCB need context lock
         if ( !context->isMBLock() )
         {
            rc = context->mbLock( SHARED ) ;
            PD_RC_CHECK( rc, PDERROR, "Lock mb failed[%d]", rc ) ;
            needUnlock = TRUE ;
         }
         ixmIndexCB indexCB ( indexExtentID, this, context ) ;
         PD_CHECK( indexCB.isInitialized(),
                   SDB_DMS_INIT_INDEX, error, PDERROR,
                   "Failed to initialize index" ) ;
         PD_CHECK( indexLID == indexCB.getLogicalID(),
                   SDB_DMS_INVALID_INDEXCB, error, PDERROR,
                   "Index logical id[%d] is not as expected[%d]. The index may "
                   "have been recreated", indexCB.getLogicalID(), indexLID ) ;

         rc = pOprHandle->onRebuildIndex( context, &indexCB, cb, pResult ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Rebuild failed to invoke callback, rc = %d",
                    rc ) ;
            goto error ;
         }

         if ( needUnlock )
         {
            context->mbUnlock() ;
            needUnlock = FALSE ;
         }
      }
      if ( sortBufferSize > 0 &&
           sortBufferSize < DMS_INDEX_SORT_BUFFER_MIN_SIZE )
      {
         sortBufferSize = DMS_INDEX_SORT_BUFFER_MIN_SIZE ;
      }

      builder = dmsIndexBuilder::createInstance( this, _pDataSu, context, cb,
                                                 indexExtentID, indexLID,
                                                 sortBufferSize, indexType,
                                                 pOprHandle,
                                                 pResult, dkProcessor ) ;
      if ( NULL == builder )
      {
         PD_LOG ( PDERROR,
                  "Failed to get index builder instance, sort buffer size: %d",
                  sortBufferSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = builder->build() ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to build index: %d", rc ) ;
         goto error ;
      }

   done :
      if ( needUnlock )
      {
         context->mbUnlock() ;
         needUnlock = FALSE ;
      }
      if ( NULL != builder )
      {
         dmsIndexBuilder::releaseInstance( builder ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEINDEX__REBUILDINDEX, rc );
      return rc ;
   error :
      PD_LOG( PDERROR, "Failed to rebuild index[%d], rc = %d",
              indexLID, rc ) ;

      goto done ;
   }

   INT32 _dmsStorageIndex::rebuildIndexes( dmsMBContext *context, pmdEDUCB *cb,
                                           INT32 sortBufferSize,
                                           _dmsDupKeyProcessor *dkProcessor )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      INT32 totalIndexNum          = 0 ;

      rc = truncateIndexes( context, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "truncate indexes failed, rc: %d", rc ) ;

      // need to lock mb
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ++totalIndexNum ;
      }

      PD_LOG ( PDEVENT, "Totally %d indexes to rebuild for collection %d",
               totalIndexNum, context->mbID() ) ;

      for ( indexID = 0 ; indexID < totalIndexNum ; ++indexID )
      {
         PD_LOG ( PDEVENT, "Rebuilding index %d for collection %d",
                  indexID, context->mbID() ) ;
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         PD_CHECK( indexCB.isInitialized(), SDB_DMS_INIT_INDEX, error, PDERROR,
                   "Failed to initialize index, index extent id: %d ",
                   context->mb()->_indexExtent[indexID] ) ;

         rc = _rebuildIndex( context, context->mb()->_indexExtent[ indexID ],
                             indexCB.getLogicalID(), cb, sortBufferSize,
                             indexCB.getIndexType(), NULL, NULL,
                             dkProcessor ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to rebuild index %d, rc: %d", indexID,
                     rc ) ;
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::_indexInsert( _ixmIndexCB *indexCB,
                                         const ixmKey &key,
                                         const dmsRecordID &rid,
                                         const Ordering& order,
                                         _pmdEDUCB *cb,
                                         BOOLEAN dupAllowed,
                                         BOOLEAN dropDups,
                                         utilWriteResult *pResult )
   {
      INT32 rc = SDB_OK ;
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      // get root in each loop, since root page may change after each
      // insert (root split)
      ixmExtent rootidx ( indexCB->getRoot(), this ) ;

      // adjust allow duplicated flag
      // - doing DPS log rollback: allow duplicated
      // - doing transaction rollback on non-id index: allow duplicated
      dupAllowed = ( NULL != cb &&
                     ( cb->isDoRollback() ||
                     ( cb->isInTransRollback() &&
                           !indexCB->isSysIndex() ) ) ) ? TRUE : dupAllowed ;

      rc = rootidx.insert ( key, rid, order, dupAllowed, indexCB, pResult ) ;
      if ( rc )
      {
         if ( pResult )
         {
            if ( pResult->getCurRID().isNull() )
            {
               pResult->setCurRID( rid ) ;
            }
            INT32 rcTmp = pResult->setIndexErrInfo( indexCB->getName(),
                                                    indexCB->keyPattern(),
                                                    key.toBson() ) ;
            if ( rcTmp )
            {
               rc = rcTmp ;
            }
         }

         PD_LOG ( PDERROR, "Failed to insert index, key[%s], rid[%d:%d], rc: %d",
                  key.toString( FALSE, TRUE ).c_str(), rid._extent,
                  rid._offset, rc ) ;
         goto error ;
      }
      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsStorageIndex::_indexInsert( dmsMBContext *context,
                                         ixmIndexCB *indexCB,
                                         BSONObj &inputObj,
                                         const dmsRecordID &rid,
                                         pmdEDUCB *cb,
                                         BOOLEAN dupAllowed,
                                         BOOLEAN dropDups,
                                         IDmsOprHandler *pOprHandle,
                                         utilWriteResult *pResult )
   {
      SDB_ASSERT ( indexCB, "indexCB can't be NULL" ) ;
      INT32 rc = SDB_OK ;
      BSONObjSet keySet ;

      rc = indexCB->getKeysFromObject ( inputObj, keySet ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get keys from object %s",
                    inputObj.toString().c_str() ) ;
      {
         BSONObjSet::iterator it ;
         Ordering order = Ordering::make( indexCB->keyPattern() ) ;

         if ( pOprHandle )
         {
            rc = pOprHandle->onInsertIndex( context, indexCB, !dupAllowed,
                                            indexCB->enforced(),
                                            keySet, rid, cb,
                                            pResult ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         for ( it = keySet.begin() ; it != keySet.end() ; ++it )
         {
#ifdef _DEBUG
            PD_LOG ( PDDEBUG, "Insert key: %s", (*it).toString().c_str() ) ;
#endif
            ixmKeyOwned ko ((*it)) ;
            rc = _indexInsert ( indexCB, ko, rid, order, cb,
                                dupAllowed, dropDups, pResult ) ;
            if ( rc )
            {
               if ( pResult )
               {
                  pResult->setCurrentID( inputObj ) ;
               }
               PD_LOG ( PDERROR, "Insert index key(%s) with rid(%d, %d) "
                        "failed, rc: %d", it->toString().c_str(),
                        rid._extent, rid._offset, rc ) ;
               goto error ;
            }
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // caller is responsible to rollback the change if indexesInsert fail
   INT32 _dmsStorageIndex::indexesInsert( dmsMBContext *context,
                                          dmsExtentID extLID,
                                          BSONObj & inputObj,
                                          const dmsRecordID &rid,
                                          pmdEDUCB * cb,
                                          IDmsOprHandler *pOprHandle,
                                          utilWriteResult *pResult )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;
      BOOLEAN unique               = FALSE ;
      BOOLEAN dropDups             = FALSE ;
      vector<ixmIndexCB> textIdxCBs ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // loops through all potential indexes for the record
      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         PD_CHECK ( indexCB.isInitialized(), SDB_DMS_INIT_INDEX, error,
                    PDERROR, "Failed to init index" ) ;

         // if index is 'IXM_INDEX_FLAG_CREATING', then judge extent LID
         if ( IXM_INDEX_FLAG_CREATING == indexCB.getFlag() &&
              extLID > indexCB.scanExtLID() )
         {
            continue ;
         }
         // only attempt to insert into normal and creating indexes
         else if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL &&
                   indexCB.getFlag() != IXM_INDEX_FLAG_CREATING )
         {
            continue ;
         }
         unique = indexCB.unique() ;
         dropDups = indexCB.dropDups() ;

         // If it's text index
         if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                   IXM_EXTENT_TYPE_TEXT )
              && IXM_INDEX_FLAG_NORMAL == indexCB.getFlag() )
         {
            textIdxCBs.push_back( indexCB ) ;
         }
         else
         {
            rc = _indexInsert ( context, &indexCB, inputObj, rid, cb, !unique,
                                dropDups, pOprHandle, pResult ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to insert object(%s) index(%s), "
                          "rc: %d", inputObj.toString().c_str(),
                          indexCB.getDef().toString().c_str(), rc ) ;
         }
      }

      if ( textIdxCBs.size() > 0 )
      {
         IDmsExtDataHandler *handler = _pDataSu->getExtDataHandler() ;
         if ( !handler )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "External operation handler is invalid" ) ;
            goto error ;
         }

         // Insert into text index at last.
         for ( vector<ixmIndexCB>::iterator itr = textIdxCBs.begin();
               itr != textIdxCBs.end(); ++itr )
         {
            rc = handler->onInsert( itr->getExtDataName(), inputObj, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Insert on text index failed[ %d ]", rc ) ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // Description:
   //    Given original object and new object, update the on disk index slots
   // for a specific index.
   // Input:
   //    context:  DMS Meta data block information
   //    indexCB:  The index to update
   //    originalObj: original data
   //    newObj: new data value
   //    dmsRecordID:  record ID
   //    pmdEDUCB:  edu CB
   //    isRollback: is this rollback or not
   // Return:
   //    SDB_OK:  normal return
   // Dependency:
   //
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEINDEX__INDEXUPDATE, "_dmsStorageIndex::_indexUpdate" )
   INT32 _dmsStorageIndex::_indexUpdate( dmsMBContext *context,
                                         ixmIndexCB *indexCB,
                                         BSONObj &originalObj,
                                         BSONObj &newObj,
                                         const dmsRecordID &rid,
                                         pmdEDUCB *cb,
                                         BOOLEAN isRollback,
                                         IDmsOprHandler *pOprHandle,
                                         utilWriteResult *pResult )
   {
      INT32 rc             = SDB_OK ;
      BSONObjSet keySetOri ;
      BSONObjSet keySetNew ;
      BOOLEAN unique       = FALSE ;
      BOOLEAN found        = FALSE ;
      BOOLEAN dupAllowed   = FALSE ;
      monAppCB * pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEINDEX__INDEXUPDATE );
      SDB_ASSERT ( indexCB, "indexCB can't be NULL" ) ;

      rc = indexCB->getKeysFromObject( originalObj, keySetOri ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get keys from org object %s",
                  originalObj.toString().c_str() ) ;
         goto error ;
      }

      unique = indexCB->unique() ;

      // adjust allow duplicated flag
      // - doing DPS log rollback: allow duplicated
      // - doing transaction rollback on non-id index: allow duplicated
      // - non-unique index: allow duplicated
      dupAllowed = ( NULL != cb &&
                     ( cb->isDoRollback() ||
                     ( cb->isInTransRollback() &&
                           !indexCB->isSysIndex() ) ) ) ? TRUE : !unique ;

      rc = indexCB->getKeysFromObject ( newObj, keySetNew ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get keys from new object %s",
                  newObj.toString().c_str() ) ;
         goto error ;
      }

      if ( pOprHandle )
      {
         rc = pOprHandle->onUpdateIndex( context, indexCB, unique,
                                         indexCB->enforced(), keySetOri,
                                         keySetNew, rid, isRollback, cb,
                                         pResult ) ;
         if ( rc )
         {
            goto error ;
         }
      }

#if defined (_DEBUG)
      PD_LOG ( PDDEBUG, "IndexUpdate\nIndex: %s\nFrom Record: %s\nTo Record %s",
               indexCB->keyPattern().toString().c_str(),
               originalObj.toString().c_str(),
               newObj.toString().c_str() ) ;
#endif

      // do merge scan for two sets, unindex the keys if the one in keySetOri
      // doesn't appear in keySetNew, and insert the one in keySetNew doesn't
      // appear in keySetOri
      {
         BSONObjSet::iterator itori ;
         BSONObjSet::iterator itnew ;
         Ordering order = Ordering::make(indexCB->keyPattern()) ;

         itori = keySetOri.begin() ;
         itnew = keySetNew.begin() ;
         while ( keySetOri.end() != itori && keySetNew.end() != itnew )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key From %s\nKey To %s",
                     (*itori).toString().c_str(),
                     (*itnew).toString().c_str() ) ;
#endif
            INT32 result = (*itori).woCompare((*itnew), BSONObj(), FALSE ) ;
            if ( 0 == result )
            {
               // new and original are the same, we don't need to change
               // anything in the index
               itori++ ;
               itnew++ ;
               continue ;
            }
            else if ( result < 0 )
            {
               ixmExtent rootidx ( indexCB->getRoot(), this ) ;
               ixmKeyOwned ko ((*itori)) ;
               rc = rootidx.unindex ( ko, rid, order, indexCB, found ) ;
               if ( rc )
               {
                  PD_LOG ( PDERROR, "Delete index key(%s) with rid(%d, %d) "
                           "failed, rc: %d", (*itori).toString().c_str(),
                           rid._extent, rid._offset, rc ) ;
                  goto error ;
               }
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
               // during rollback, since the previous change may half-way
               // completed, there could be some keys that has not been
               // inserted. So if we found any rid+key that does not in the
               // index, that means we've finished rollback
               if ( !found && isRollback )
               {
                  goto done ;
               }
               itori++ ;
               continue ;
            }
            else
            {
               // new smaller than original, that means the new one doesn't
               // appear in the original list, let's add it
               ixmExtent rootidx ( indexCB->getRoot(), this ) ;
               ixmKeyOwned ko ((*itnew)) ;
               rc = rootidx.insert ( ko, rid, order, dupAllowed, indexCB,
                                     pResult ) ;
               if ( rc )
               {
                  // during rollback, since the previous change may half-way
                  // completed, there could be some keys that has not been
                  // removed. So if we hit error indicating the key and rid are
                  // identical, that means we've finished rollback
                  if ( SDB_IXM_IDENTICAL_KEY == rc && isRollback )
                  {
                     rc = SDB_OK ;
                     goto done ;
                  }
                  else if ( rc && pResult )
                  {
                     INT32 rcTmp = pResult->setIndexErrInfo( indexCB->getName(),
                                                             indexCB->keyPattern(),
                                                             *itnew,
                                                             originalObj ) ;
                     if ( rcTmp )
                     {
                        rc = rcTmp ;
                     }
                  }

                  PD_LOG ( PDERROR, "Failed to insert index(%s) with "
                           "rid(%d, %d), rc: %d", (*itnew).toString().c_str(),
                           rid._extent, rid._offset, rc ) ;
                  goto error ;
               }
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
               itnew++ ;
               continue ;
            }
         }

         // delete reset of itori
         while ( keySetOri.end() != itori )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key From %s", (*itori).toString().c_str() ) ;
#endif
            ixmExtent rootidx ( indexCB->getRoot(), this ) ;
            ixmKeyOwned ko ((*itori)) ;
            rc = rootidx.unindex ( ko, rid, order, indexCB, found ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Delete index key(%s) with rid(%d, %d) "
                        "failed, rc: %d", (*itori).toString().c_str(),
                        rid._extent, rid._offset, rc ) ;
               goto error ;
            }
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
            // during rollback, since the previous change may half-way
            // completed, there could be some keys that has not been
            // inserted. So if we found any rid+key that does not in the
            // index, that means we've finished rollback
            if ( !found && isRollback )
            {
               goto done ;
            }
            itori++ ;
         }

         // insert rest of itnew
         while ( keySetNew.end() != itnew )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Key To %s", (*itnew).toString().c_str() ) ;
#endif
            ixmExtent rootidx ( indexCB->getRoot(), this ) ;
            ixmKeyOwned ko ((*itnew)) ;
            rc = rootidx.insert ( ko, rid, order, dupAllowed, indexCB, pResult ) ;
            if ( rc )
            {
               // during rollback, since the previous change may half-way
               // completed, there could be some keys that has not been
               // removed. So if we hit error indicating the key and rid are
               // identical, that means we've finished rollback
               if ( SDB_IXM_IDENTICAL_KEY == rc && isRollback )
               {
                  rc = SDB_OK ;
                  goto done ;
               }
               else if ( rc && pResult )
               {
                  INT32 rcTmp = pResult->setIndexErrInfo( indexCB->getName(),
                                                          indexCB->keyPattern(),
                                                          *itnew,
                                                          originalObj ) ;
                  if ( rcTmp )
                  {
                     rc = rcTmp ;
                  }
               }

               PD_LOG ( PDERROR, "Failed to insert index(%s) with "
                        "rid(%d, %d), rc: %d", (*itnew).toString().c_str(),
                        rid._extent, rid._offset, rc ) ;
               goto error ;
            }
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
            itnew++ ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEINDEX__INDEXUPDATE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // Description:
   //    Based on the passed in original and new object, update all the
   // corresponding indexes.
   // caller is responsible to rollback the change by calling the function
   // with reversed object.
   INT32 _dmsStorageIndex::indexesUpdate( dmsMBContext *context,
                                          dmsExtentID extLID,
                                          BSONObj &originalObj,
                                          BSONObj &newObj,
                                          const dmsRecordID &rid,
                                          pmdEDUCB *cb,
                                          BOOLEAN isRollback,
                                          IDmsOprHandler *pOprHandle,
                                          utilWriteResult *pResult )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;
      vector<ixmIndexCB> textIdxCBs ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      for ( indexID=0; indexID < DMS_COLLECTION_MAX_INDEX; indexID++ )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         PD_CHECK ( indexCB.isInitialized(), SDB_DMS_INIT_INDEX,
                    error, PDERROR, "Failed to init index" ) ;

         if ( IXM_INDEX_FLAG_CREATING == indexCB.getFlag() &&
              extLID > indexCB.scanExtLID() )
         {
            continue ;
         }
         // only attempt to insert into normal and creating indexes
         else if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL &&
                   indexCB.getFlag() != IXM_INDEX_FLAG_CREATING )
         {
            continue ;
         }

         if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                   IXM_EXTENT_TYPE_TEXT )
              && IXM_INDEX_FLAG_NORMAL == indexCB.getFlag() )
         {
            textIdxCBs.push_back( indexCB ) ;
         }
         else
         {
            rc = _indexUpdate ( context, &indexCB, originalObj, newObj,
                                rid, cb, isRollback, pOprHandle, pResult ) ;
            PD_RC_CHECK ( rc, PDERROR, "Failed to update obj(%s) index(%s), "
                          "rc: %d", newObj.toString().c_str(),
                          indexCB.getDef().toString().c_str(), rc ) ;
         }
      }

      if ( textIdxCBs.size() > 0 )
      {
         IDmsExtDataHandler *handler = _pDataSu->getExtDataHandler() ;
         if ( !handler )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "External operation handler is invalid" ) ;
            goto error ;
         }

         // Insert into text index at last.
         for ( vector<ixmIndexCB>::iterator itr = textIdxCBs.begin();
               itr != textIdxCBs.end(); ++itr )
         {
            rc = handler->onUpdate( itr->getExtDataName(), originalObj,
                                    newObj, cb, isRollback ) ;
            PD_RC_CHECK( rc, PDERROR, "Update on text index failed[ %d ]",
                         rc ) ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // Description: delete all the indexes from the record to be deleted
   // Input:
   //    inputObj: BSON containing the record
   //    rid:      The record ID to be deleted
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEINDEX__INDEXDELETE, "_dmsStorageIndex::_indexDelete" )
   INT32 _dmsStorageIndex::_indexDelete( dmsMBContext *context,
                                         ixmIndexCB *indexCB,
                                         BSONObj &inputObj,
                                         const dmsRecordID &rid,
                                         pmdEDUCB * cb,
                                         IDmsOprHandler *pOprHandle )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEINDEX__INDEXDELETE ) ;
      INT32       rc          = SDB_OK ;
      BSONObjSet  keySet ;
      BOOLEAN     result      = FALSE ;
      monAppCB   *pMonAppCB   = cb ? cb->getMonAppCB() : NULL ;

      SDB_ASSERT ( indexCB, "indexCB can't be NULL" ) ;

      rc = indexCB->getKeysFromObject ( inputObj, keySet ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to get keys from object %s",
                  inputObj.toString().c_str() ) ;
         goto error ;
      }

      if ( pOprHandle )
      {
         rc = pOprHandle->onDeleteIndex( context, indexCB,
                                         indexCB->unique(), keySet,
                                         rid, cb ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      {
         BSONObjSet::iterator it ;
         Ordering order = Ordering::make(indexCB->keyPattern()) ;

         // go through each index in the set
         for ( it = keySet.begin() ; it != keySet.end() ; it++ )
         {
#if defined (_DEBUG)
            PD_LOG ( PDDEBUG, "Delete key: %s", (*it).toString().c_str() ) ;
#endif
            // get root in each loop, since root page may change after each
            // insert (root split)
            ixmExtent rootidx ( indexCB->getRoot(), this ) ;
            ixmKeyOwned ko ((*it)) ;
            rc = rootidx.unindex ( ko, rid, order, indexCB, result ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Delete index key(%s) with rid(%d, %d) "
                        "failed, rc: %d", it->toString().c_str(),
                        rid._extent, rid._offset, rc ) ;
               goto error ;
            }
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INDEX_WRITE, 1 ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEINDEX__INDEXDELETE, rc ) ;
      return rc ;
   error :
      PD_LOG ( PDERROR, "Failed to deleteindex, rc: %d", rc ) ;
      goto done ;
   }

   // delete all indexes for an oject
   INT32 _dmsStorageIndex::indexesDelete( dmsMBContext *context,
                                          dmsExtentID extLID,
                                          BSONObj &inputObj,
                                          const dmsRecordID &rid,
                                          pmdEDUCB * cb,
                                          IDmsOprHandler *pOprHandle )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;
      vector<ixmIndexCB> textIdxCBs ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }

         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to init index" ) ;
            rc = SDB_DMS_INIT_INDEX ;
            goto error ;
         }
         if ( IXM_INDEX_FLAG_CREATING == indexCB.getFlag() &&
              extLID > indexCB.scanExtLID() )
         {
            continue ;
         }
         // only attempt to insert into normal and creating indexes
         else if ( indexCB.getFlag() != IXM_INDEX_FLAG_NORMAL &&
                   indexCB.getFlag() != IXM_INDEX_FLAG_CREATING )
         {
            continue ;
         }

         if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                   IXM_EXTENT_TYPE_TEXT )
              && IXM_INDEX_FLAG_NORMAL == indexCB.getFlag() )
         {
            textIdxCBs.push_back( indexCB ) ;
         }
         else
         {
            rc = _indexDelete ( context, &indexCB, inputObj,
                                rid, cb, pOprHandle ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to delete object(%s) index(%s), "
                        "rc: %d", inputObj.toString().c_str(),
                        indexCB.getDef().toString().c_str(), rc ) ;
               goto error ;
            }
         }
      }

      if ( textIdxCBs.size() > 0 )
      {
         IDmsExtDataHandler *handler = _pDataSu->getExtDataHandler() ;
         if ( !handler )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "External operation handler is invalid" );
            goto error ;
         }

         // Insert into text index at last.
         for ( vector<ixmIndexCB>::iterator itr = textIdxCBs.begin();
               itr != textIdxCBs.end(); ++itr )
         {
            rc = handler->onDelete( itr->getExtDataName(), inputObj, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Delete on text index failed[ %d ]",
                         rc ) ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::truncateIndexes( dmsMBContext * context,
                                            pmdEDUCB *cb )
   {
      INT32 rc                     = SDB_OK ;
      INT32 indexID                = 0 ;

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB ( context->mb()->_indexExtent[indexID], this,
                              context ) ;
         if ( !indexCB.isInitialized() )
         {
            PD_LOG ( PDERROR, "Failed to initialize index" ) ;
            rc = SDB_DMS_INIT_INDEX;
            goto error ;
         }

         if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                   IXM_EXTENT_TYPE_TEXT ) )
         {
            continue ;
         }
         // we don't check index flag since we are doing full index rebuild now
         // truncate index, do not remove root
         rc = indexCB.truncate ( FALSE, IXM_INDEX_FLAG_NORMAL ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index(%s), rc: %d",
                     indexCB.getDef().toString().c_str(), rc ) ;
            goto error ;
         }
      }

      context->mbStat()->_totalIndexPages = indexID << 1 ;
      context->mbStat()->_totalIndexFreeSpace =
      indexID * ( pageSize()-1-sizeof(ixmExtentHead) ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::getIndexCBExtent( dmsMBContext *context,
                                             const CHAR *indexName,
                                             dmsExtentID &indexExtent )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      BOOLEAN hasLocked            = FALSE ;

      SDB_ASSERT ( indexName, "index name can't be NULL" ) ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         hasLocked = TRUE ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         if ( 0 == ossStrncmp ( indexName, indexCB.getName(),
                                IXM_INDEX_NAME_SIZE ) )
         {
            indexExtent = context->mb()->_indexExtent[indexID] ;
            found = TRUE ;
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
      }

   done :
      if ( hasLocked )
      {
         context->mbUnlock() ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::getIndexCBExtent( dmsMBContext *context,
                                             const OID &indexOID,
                                             dmsExtentID &indexExtent )
   {
      INT32 rc                     = SDB_OK ;
      INT32  indexID               = 0 ;
      BOOLEAN found                = FALSE ;
      BOOLEAN hasLocked            = FALSE ;

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         hasLocked = TRUE ;
      }

      for ( indexID = 0 ; indexID < DMS_COLLECTION_MAX_INDEX ; ++indexID )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[indexID] )
         {
            break ;
         }
         ixmIndexCB indexCB( context->mb()->_indexExtent[indexID], this,
                             context ) ;
         OID id ;
         indexCB.getIndexID( id ) ;
         if ( indexOID == id )
         {
            indexExtent = context->mb()->_indexExtent[indexID] ;
            found = TRUE ;
            break ;
         }
      }

      if ( !found )
      {
         rc = SDB_IXM_NOTEXIST ;
      }

   done :
      if ( hasLocked )
      {
         context->mbUnlock() ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _dmsStorageIndex::getIndexCBExtent( dmsMBContext *context,
                                             INT32 indexID,
                                             dmsExtentID &indexExtent )
   {
      INT32 rc                      = SDB_OK ;
      BOOLEAN hasLocked             = FALSE ;

      if ( indexID >= DMS_COLLECTION_MAX_INDEX )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !context->isMBLock() )
      {
         rc = context->mbLock( SHARED ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         hasLocked = FALSE ;
      }

      if ( context->mb()->_indexExtent[indexID] == DMS_INVALID_EXTENT )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }
      indexExtent = context->mb()->_indexExtent[indexID] ;

   done:
      if ( hasLocked )
      {
         context->mbUnlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   void _dmsStorageIndex::addStatFreeSpace( UINT16 mbID, UINT16 size )
   {
      if ( mbID < DMS_MME_SLOTS && _pDataSu )
      {
         _pDataSu->_mbStatInfo[mbID]._totalIndexFreeSpace += size ;
      }
   }

   void _dmsStorageIndex::decStatFreeSpace( UINT16 mbID, UINT16 size )
   {
      if ( mbID < DMS_MME_SLOTS && _pDataSu )
      {
         _pDataSu->_mbStatInfo[mbID]._totalIndexFreeSpace -= size ;
      }
   }
}


