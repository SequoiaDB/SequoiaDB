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

   Source File Name = dmsStorageDataCommon.cpp

   Descriptive Name = Common Data Management Service

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

#include "dmsStorageDataCommon.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageLob.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransLockCallback.hpp"
#include "dpsOp2Record.hpp"
#include "mthModifier.hpp"
#include "ixm.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"
#include "utilCompressor.hpp"
#include "dmsTransLockCallback.hpp"
#include "dmsLightJob.hpp"

using namespace bson ;

namespace engine
{

   #define DMS_MB_FLAG_FREE_STR                       "Free"
   #define DMS_MB_FLAG_USED_STR                       "Used"
   #define DMS_MB_FLAG_DROPED_STR                     "Dropped"
   #define DMS_MB_FLAG_OFFLINE_REORG_STR              "Offline Reorg"
   #define DMS_MB_FLAG_ONLINE_REORG_STR               "Online Reorg"
   #define DMS_MB_FLAG_LOAD_STR                       "Load"
   #define DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY_STR  "Shadow Copy"
   #define DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE_STR     "Truncate"
   #define DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK_STR    "Copy Back"
   #define DMS_MB_FLAG_OFFLINE_REORG_REBUILD_STR      "Rebuild"
   #define DMS_MB_FLAG_LOAD_LOAD_STR                  "Load"
   #define DMS_MB_FLAG_LOAD_BUILD_STR                 "Build"
   #define DMS_MB_FLAG_UNKNOWN                        "Unknown"

   #define DMS_STATUS_SEPARATOR                       " | "

   static void appendFlagString( CHAR * pBuffer, INT32 bufSize,
                                 const CHAR *flagStr )
   {
      if ( 0 != *pBuffer )
      {
         ossStrncat( pBuffer, DMS_STATUS_SEPARATOR,
                     bufSize - ossStrlen( pBuffer ) ) ;
      }
      ossStrncat( pBuffer, flagStr, bufSize - ossStrlen( pBuffer ) ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MBFLAG2STRING, "mbFlag2String" )
   void mbFlag2String( UINT16 flag, CHAR * pBuffer, INT32 bufSize )
   {
      PD_TRACE_ENTRY ( SDB__MBFLAG2STRING ) ;
      SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
      ossMemset ( pBuffer, 0, bufSize ) ;
      // Free
      if ( DMS_IS_MB_FREE ( flag ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_FREE_STR ) ;
         goto done ;
      }

      // Used
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_USED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_USED_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_USED ) ;
      }
      // Dropped
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_DROPED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_DROPED_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_DROPED ) ;
      }

      // Offline Reorg
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_OFFLINE_REORG_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG ) ;

         // Shadow Copy
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_SHADOW_COPY ) ;
         }
         // Truncate
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_TRUNCATE ) ;
         }
         // Copy Back
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_COPY_BACK ) ;
         }
         // Rebuild
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_OFFLINE_REORG_REBUILD ) )
         {
            appendFlagString( pBuffer, bufSize,
                              DMS_MB_FLAG_OFFLINE_REORG_REBUILD_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_OFFLINE_REORG_REBUILD ) ;
         }
      }
      // Online Reorg
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_ONLINE_REORG ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_ONLINE_REORG_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_ONLINE_REORG ) ;
      }
      // load
      if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_LOAD ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_LOAD_LOAD_STR ) ;
         OSS_BIT_CLEAR( flag, DMS_MB_FLAG_LOAD ) ;

         // load
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_LOAD_LOAD ) )
         {
            appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_LOAD_LOAD_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_LOAD_LOAD ) ;
         }
         // load build
         if ( OSS_BIT_TEST ( flag, DMS_MB_FLAG_LOAD_BUILD ) )
         {
            appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_LOAD_BUILD_STR ) ;
            OSS_BIT_CLEAR( flag, DMS_MB_FLAG_LOAD_BUILD ) ;
         }
      }

      // Test other bits
      if ( flag )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_UNKNOWN ) ;
      }
   done :
      PD_TRACE2 ( SDB__MBFLAG2STRING,
                  PD_PACK_USHORT ( flag ),
                  PD_PACK_STRING ( pBuffer ) ) ;
      PD_TRACE_EXIT ( SDB__MBFLAG2STRING ) ;
   }

   #define DMS_MB_ATTR_COMPRESSED_STR                        "Compressed"
   #define DMS_MB_ATTR_NOIDINDEX_STR                         "NoIDIndex"
   #define DMS_MB_ATTR_CAPPED_STR                            "Capped"
   #define DMS_MB_ATTR_STRICTDATAMODE_STR                    "StrictDataMode"
   // PD_TRACE_DECLARE_FUNCTION ( SDB__MBATTR2STRING, "mbAttr2String" )
   void mbAttr2String( UINT32 attributes, CHAR * pBuffer, INT32 bufSize )
   {
      PD_TRACE_ENTRY ( SDB__MBATTR2STRING ) ;
      SDB_ASSERT ( pBuffer, "pBuffer can't be NULL" ) ;
      ossMemset ( pBuffer, 0, bufSize ) ;

      if ( OSS_BIT_TEST ( attributes, DMS_MB_ATTR_COMPRESSED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_COMPRESSED_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_COMPRESSED ) ;
      }
      if ( OSS_BIT_TEST ( attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_NOIDINDEX_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_NOIDINDEX ) ;
      }
      if ( OSS_BIT_TEST( attributes, DMS_MB_ATTR_CAPPED ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_CAPPED_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_CAPPED ) ;
      }
      if ( OSS_BIT_TEST( attributes, DMS_MB_ATTR_STRICTDATAMODE ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_STRICTDATAMODE_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_STRICTDATAMODE ) ;
      }

      // Test other bits
      if ( attributes )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_FLAG_UNKNOWN ) ;
      }
      PD_TRACE2 ( SDB__MBATTR2STRING,
                  PD_PACK_UINT ( attributes ),
                  PD_PACK_STRING ( pBuffer ) ) ;
      PD_TRACE_EXIT ( SDB__MBATTR2STRING ) ;
   }

   /*
      _dmsMBContext implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT, "_dmsMBContext::_dmsMBContext" )
   _dmsMBContext::_dmsMBContext ()
   {
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT ) ;
      _reset () ;
      PD_TRACE_EXIT ( SDB__DMSMBCONTEXT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_DESC, "_dmsMBContext::~_dmsMBContext" )
   _dmsMBContext::~_dmsMBContext ()
   {
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_DESC ) ;
      _reset () ;
      PD_TRACE_EXIT ( SDB__DMSMBCONTEXT_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT__RESET, "_dmsMBContext::_reset" )
   void _dmsMBContext::_reset ()
   {
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT__RESET ) ;
      _mb            = NULL ;
      _mbStat        = NULL ;
      _latch         = NULL ;
      _clLID         = DMS_INVALID_CLID ;
      _startLID      = DMS_INVALID_CLID ;
      _mbID          = DMS_INVALID_MBID ;
      _mbLockType    = -1 ;
      _resumeType    = -1 ;
      PD_TRACE_EXIT ( SDB__DMSMBCONTEXT__RESET ) ;
   }

   string _dmsMBContext::toString() const
   {
      stringstream ss ;
      ss << "dms-mb-context[" ;
      if ( _mb )
      {
         ss << "Name: " ;
         ss << _mb->_collectionName ;
         ss << ", " ;
      }
      ss << "ID: " << _mbID ;
      ss << ", LID: " << _clLID ;
      ss << ", StartLID: " << _startLID ;
      ss << ", LockType: " << _mbLockType ;
      ss << ", ResumeType: " << _resumeType ;

      ss << " ]" ;

      return ss.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_PAUSE, "_dmsMBContext::pause" )
   INT32 _dmsMBContext::pause()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_PAUSE ) ;

      _resumeType = _mbLockType ;
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         rc = mbUnlock() ;
      }

      PD_TRACE_EXITRC ( SDB__DMSMBCONTEXT_PAUSE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_RESUME, "_dmsMBContext::resume" )
   INT32 _dmsMBContext::resume()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_RESUME ) ;
      if ( SHARED == _resumeType || EXCLUSIVE == _resumeType )
      {
         INT32 lockType = _resumeType ;
         _resumeType = -1 ;
         rc = mbLock( lockType ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSMBCONTEXT_RESUME, rc ) ;
      return rc ;
   }

   /*
      _dmsRecordRW implement
   */
   _dmsRecordRW::_dmsRecordRW()
   :_ptr( NULL )
   {
      _pData = NULL ;
      _isDirectMem = FALSE ;
   }

   _dmsRecordRW::~_dmsRecordRW()
   {
   }

   BOOLEAN _dmsRecordRW::isEmpty() const
   {
      return _pData ? FALSE : TRUE ;
   }

   _dmsRecordRW _dmsRecordRW::derive( const dmsRecordID &rid ) const
   {
      if ( _pData )
      {
         return _pData->record2RW( rid, _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   _dmsRecordRW _dmsRecordRW::deriveNext() const
   {
      if ( _ptr && DMS_INVALID_OFFSET != _ptr->_nextOffset )
      {
         return _pData->record2RW( dmsRecordID( _rid._extent,
                                                _ptr->_nextOffset ),
                                   _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   _dmsRecordRW _dmsRecordRW::derivePre() const
   {
      if ( _ptr && DMS_INVALID_OFFSET != _ptr->_previousOffset )
      {
         return _pData->record2RW( dmsRecordID( _rid._extent,
                                                _ptr->_previousOffset ),
                                   _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   _dmsRecordRW _dmsRecordRW::deriveOverflow() const
   {
      if ( _ptr && _ptr->isOvf() )
      {
         return _pData->record2RW( _ptr->getOvfRID(), _rw.getCollectionID() ) ;
      }
      return _dmsRecordRW() ;
   }

   void _dmsRecordRW::setNothrow( BOOLEAN nothrow )
   {
      _rw.setNothrow( nothrow ) ;
   }

   BOOLEAN _dmsRecordRW::isNothrow() const
   {
      return _rw.isNothrow() ;
   }

   const dmsRecord* _dmsRecordRW::readPtr( UINT32 len ) const
   {
      if ( !_ptr )
      {
         std::string text = "Point is NULL: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_SYS ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_SYS, text ) ;
      }

      // special case when we don't read through extent but use in memory
      // old version directly
      // record2RW already set the _prt to record directly based on rid
      // here we would use 0 offset directly
      if( _isDirectMem )
      {
         return (const dmsRecord*)_ptr ;
      }

      if ( 0 == len )
      {
         len = _ptr->getSize() ;
      }
      if ( len > DMS_RECORD_MAX_SZ )
      {
         std::string text = "Record size is grater than max record size: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_DMS_CORRUPTED_EXTENT ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_DMS_CORRUPTED_EXTENT, text ) ;
      }
      return (const dmsRecord*)_rw.readPtr( _rid._offset, len ) ;
   }

   dmsRecord* _dmsRecordRW::writePtr( UINT32 len )
   {
      if ( !_ptr )
      {
         std::string text = "Point is NULL: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_SYS ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_SYS, text ) ;
      }

      if ( _isDirectMem )
      {
         return ( dmsRecord* )_ptr ;
      }

      if ( 0 == len )
      {
         len = _ptr->getSize() ;
      }
      if ( len > DMS_RECORD_MAX_SZ )
      {
         std::string text = "Record size is grater than max record size: " ;
         text += toString() ;

         if ( isNothrow() )
         {
            PD_LOG( PDERROR, "Exception: %s", text.c_str() ) ;
            pdSetLastError( SDB_DMS_CORRUPTED_EXTENT ) ;
            return NULL ;
         }
         throw pdGeneralException( SDB_DMS_CORRUPTED_EXTENT, text ) ;
      }
      return (dmsRecord*)_rw.writePtr( _rid._offset, len ) ;
   }

   std::string _dmsRecordRW::toString() const
   {
      std::stringstream ss ;
      ss << "RecordRW(" << _rw.getCollectionID()
         << "," << _rid._extent << "," << _rid._offset << ")" ;
      return ss.str() ;
   }

   void _dmsRecordRW::_doneAddr()
   {
      BOOLEAN oldThrow = _rw.isNothrow() ;
      _rw.setNothrow( TRUE ) ;
      /// Need to read the overflow rid
      UINT32 len = DMS_RECORD_METADATA_SZ + sizeof( dmsRecordID ) ;
      _ptr = (const dmsRecord*)_rw.readPtr( _rid._offset, len ) ;
      /// Restore nothrow
      _rw.setNothrow( oldThrow ) ;
   }

   /*
      _dmsStorageDataCommon implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON, "_dmsStorageDataCommon::_dmsStorageDataCommon" )
   _dmsStorageDataCommon::_dmsStorageDataCommon ( const CHAR *pSuFileName,
                                                  dmsStorageInfo *pInfo,
                                                  _IDmsEventHolder *pEventHolder )
   :_dmsStorageBase( pSuFileName, pInfo ),
    _metadataLatch( MON_LATCH_DMSSTORAGEDATACOMMON_METADATALATCH ),
    _latchContext( MON_LATCH_DMSSDC_LATCHCONTEXT )
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON ) ;
      _dmsMME           = NULL ;
      _pIdxSU           = NULL ;
      _pLobSU           = NULL ;
      _logicalCSID      = 0 ;
      _CSID             = DMS_INVALID_SUID ;
      _mmeSegID         = 0 ;
      _pEventHolder     = pEventHolder ;
      _pExtDataHandler  = NULL ;
      _isCapped         = FALSE;
      for ( UINT16 i = 0; i < DMS_MME_SLOTS; ++i )
      {
         _mblock[i] = monSpinSLatch( MON_LATCH_MBLOCK ) ;
      }
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATACOMMON ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_DESC, "_dmsStorageDataCommon::~_dmsStorageDataCommon" )
   _dmsStorageDataCommon::~_dmsStorageDataCommon ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_DESC ) ;
      _collectionMapCleanup() ;

      vector<dmsMBContext*>::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         SDB_OSS_DEL (*it) ;
         ++it ;
      }
      _vecContext.clear() ;

      _pIdxSU = NULL ;
      _pLobSU = NULL ;

      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATACOMMON_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_SYNCMEMTOMMAP, "_dmsStorageDataCommon::syncMemToMmap" )
   void _dmsStorageDataCommon::syncMemToMmap ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_SYNCMEMTOMMAP ) ;
      // write total count to disk
      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) )
         {
            if ( _dmsMME->_mbList[i]._totalRecords !=
                 _mbStatInfo[i]._totalRecords )
            {
               _dmsMME->_mbList[i]._totalRecords =
                  _mbStatInfo[i]._totalRecords ;
            }
            if ( _dmsMME->_mbList[i]._totalDataPages !=
                 _mbStatInfo[i]._totalDataPages )
            {
               _dmsMME->_mbList[i]._totalDataPages =
                  _mbStatInfo[i]._totalDataPages ;
            }
            if ( _dmsMME->_mbList[i]._totalIndexPages !=
                 _mbStatInfo[i]._totalIndexPages )
            {
               _dmsMME->_mbList[i]._totalIndexPages =
                  _mbStatInfo[i]._totalIndexPages ;
            }
            if ( _dmsMME->_mbList[i]._totalDataFreeSpace !=
                 _mbStatInfo[i]._totalDataFreeSpace )
            {
               _dmsMME->_mbList[i]._totalDataFreeSpace =
                  _mbStatInfo[i]._totalDataFreeSpace ;
            }
            if ( _dmsMME->_mbList[i]._totalIndexFreeSpace !=
                 _mbStatInfo[i]._totalIndexFreeSpace )
            {
               _dmsMME->_mbList[i]._totalIndexFreeSpace =
                  _mbStatInfo[i]._totalIndexFreeSpace ;
            }
            if ( _dmsMME->_mbList[i]._totalLobPages !=
                 _mbStatInfo[i]._totalLobPages )
            {
               _dmsMME->_mbList[i]._totalLobPages =
                  _mbStatInfo[i]._totalLobPages ;
            }
            if ( _dmsMME->_mbList[i]._totalLobs !=
                 _mbStatInfo[i]._totalLobs )
            {
               _dmsMME->_mbList[i]._totalLobs =
                  _mbStatInfo[i]._totalLobs ;
            }
            if ( _dmsMME->_mbList[i]._lastCompressRatio !=
                 _mbStatInfo[i]._lastCompressRatio )
            {
               _dmsMME->_mbList[i]._lastCompressRatio =
                  _mbStatInfo[i]._lastCompressRatio ;
            }
            if ( _dmsMME->_mbList[i]._totalDataLen !=
                 _mbStatInfo[i]._totalDataLen )
            {
               _dmsMME->_mbList[i]._totalDataLen =
                  _mbStatInfo[i]._totalDataLen ;
            }
            if ( _dmsMME->_mbList[i]._totalOrgDataLen !=
                 _mbStatInfo[i]._totalOrgDataLen )
            {
               _dmsMME->_mbList[i]._totalOrgDataLen =
                  _mbStatInfo[i]._totalOrgDataLen ;
            }
            if ( _dmsMME->_mbList[i]._maxGlobTransID !=
                 _mbStatInfo[i]._maxGlobTransID.peek() )
            {
               _dmsMME->_mbList[i]._maxGlobTransID =
                  _mbStatInfo[i]._maxGlobTransID.peek() ;
            }
            if ( _dmsMME->_mbList[i]._commitLSN !=
                 _mbStatInfo[i]._lastLSN.peek() )
            {
               _dmsMME->_mbList[i]._commitLSN =
                  _mbStatInfo[i]._lastLSN.peek() ;
            }
            if ( _dmsMME->_mbList[i]._idxCommitLSN !=
                 _mbStatInfo[i]._idxLastLSN.peek() )
            {
               _dmsMME->_mbList[i]._idxCommitLSN =
                  _mbStatInfo[i]._idxLastLSN.peek() ;
            }
            if ( _dmsMME->_mbList[i]._lobCommitLSN !=
                 _mbStatInfo[i]._lobLastLSN.peek() )
            {
               _dmsMME->_mbList[i]._lobCommitLSN =
                  _mbStatInfo[i]._lobLastLSN.peek() ;
            }
         }
      }
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATACOMMON_SYNCMEMTOMMAP ) ;
   }

   BOOLEAN _dmsStorageDataCommon::isTransSupport() const
   {
      if ( DMS_STORAGE_CAPPED == getStorageType() || !_transSupport )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   INT32 _dmsStorageDataCommon::flushMME( BOOLEAN sync )
   {
      syncMemToMmap() ;
      return flushSegment( _mmeSegID, sync ) ;
   }

   void _dmsStorageDataCommon::_attach( _dmsStorageIndex * pIndexSu )
   {
      SDB_ASSERT( pIndexSu, "Index su can't be NULL" ) ;
      _pIdxSU = pIndexSu ;
   }

   void _dmsStorageDataCommon::_detach ()
   {
      _pIdxSU = NULL ;
   }

   void _dmsStorageDataCommon::_attachLob( _dmsStorageLob * pLobSu )
   {
      SDB_ASSERT( pLobSu, "Lob su can't be NULL" ) ;
      _pLobSU = pLobSu ;
   }

   void _dmsStorageDataCommon::_detachLob()
   {
      _pLobSU = NULL ;
   }

   UINT64 _dmsStorageDataCommon::_dataOffset ()
   {
      return ( DMS_MME_OFFSET + DMS_MME_SZ ) ;
   }

   UINT32 _dmsStorageDataCommon::_curVersion () const
   {
      return DMS_DATASU_CUR_VERSION ;
   }

   INT32 _dmsStorageDataCommon::_checkVersion( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      if ( pHeader->_version > _curVersion() )
      {
         PD_LOG( PDERROR, "Incompatible version: %u", pHeader->_version ) ;
         rc = SDB_DMS_INCOMPATIBLE_VERSION ;
      }
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__ONCREATE, "_dmsStorageDataCommon::_onCreate" )
   INT32 _dmsStorageDataCommon::_onCreate( OSSFILE * file, UINT64 curOffSet )
   {
      INT32 rc          = SDB_OK ;
      _dmsMME           = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__ONCREATE ) ;
      SDB_ASSERT( DMS_MME_OFFSET == curOffSet, "Offset is not MME offset" ) ;

      _dmsMME = SDB_OSS_NEW dmsMetadataManagementExtent ;
      if ( !_dmsMME )
      {
         PD_LOG ( PDSEVERE, "Failed to allocate memory to for dmsMME" ) ;
         PD_LOG ( PDSEVERE, "Requested memory: %d bytes", DMS_MME_SZ ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      _initializeMME () ;

      rc = _writeFile ( file, (CHAR *)_dmsMME, DMS_MME_SZ ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to write to file duirng SU init, rc: %d",
                  rc ) ;
         goto error ;
      }
      SDB_OSS_DEL _dmsMME ;
      _dmsMME = NULL ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__ONCREATE, rc ) ;
      return rc ;
   error:
      if ( _dmsMME )
      {
         SDB_OSS_DEL _dmsMME ;
         _dmsMME = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__INITCOMPRESSORENTRY, "_dmsStorageDataCommon::_initCompressorEntry" )
   INT32 _dmsStorageDataCommon::_initCompressorEntry( UINT16 mbID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__INITCOMPRESSORENTRY ) ;
      const dmsDictExtent *dictExtent = NULL ;
      dmsExtentID dictExtID = _dmsMME->_mbList[mbID]._dictExtentID ;
      UTIL_COMPRESSOR_TYPE type =
            ( UTIL_COMPRESSOR_TYPE )_dmsMME->_mbList[mbID]._compressorType ;
      utilCompressor *compressor = NULL ;

      dmsCompressorGuard compGuard( &_compressorEntry[mbID], EXCLUSIVE ) ;

      // Set the compress flags whether the collection is compressed or not
      _compressorEntry[mbID].setFlags( _dmsMME->_mbList[mbID]._compressFlags ) ;

      /*
       * If the compression type is lzw and the dictionary has not been created,
       * return directly.
       */
      if ( UTIL_COMPRESSOR_LZW == type && DMS_INVALID_EXTENT == dictExtID )
      {
         goto done ;
      }

      compressor = getCompressorByType( type ) ;
      SDB_ASSERT( compressor, "compressor pointer should not be NULL" ) ;
      if ( !compressor )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to get compressor for collection[%s], "
                 "type: %d, rc: %d", _dmsMME->_mbList[mbID]._collectionName,
                 type, rc ) ;
         goto error ;
      }
      _compressorEntry[mbID].setCompressor( compressor ) ;

      if ( DMS_INVALID_EXTENT != dictExtID )
      {
         dmsExtRW rw = extent2RW( dictExtID, mbID ) ;
         rw.setNothrow( TRUE ) ;
         dictExtent = rw.readPtr<dmsDictExtent>() ;
         if ( !dictExtent || !dictExtent->validate( mbID ) )
         {
            PD_LOG( PDERROR, "Dictionary extent is invalid. Extent id: %d",
                    dictExtID ) ;
            rc = SDB_DMS_CORRUPTED_EXTENT ;
            goto error ;
         }

         _compressorEntry[mbID].setDictionary(
            (const utilDictHandle)( beginFixedAddr( dictExtID,
                                                    dictExtent->_blockSize ) +
                                    DMS_DICTEXTENT_HEADER_SZ ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__INITCOMPRESSORENTRY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__SETCOMPRESSORENTRY, "_dmsStorageDataCommon::_setCompressorEntry" )
   INT32 _dmsStorageDataCommon::_setCompressorEntry ( UINT16 mbID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__SETCOMPRESSORENTRY ) ;

      const dmsDictExtent * dictExtent = NULL ;
      dmsExtentID dictExtID = _dmsMME->_mbList[mbID]._dictExtentID ;
      UTIL_COMPRESSOR_TYPE type =
            ( UTIL_COMPRESSOR_TYPE )_dmsMME->_mbList[mbID]._compressorType ;

      dmsCompressorGuard compGuard( &_compressorEntry[mbID], EXCLUSIVE ) ;

      _compressorEntry[ mbID ].setFlags( _dmsMME->_mbList[mbID]._compressFlags ) ;

      if ( UTIL_COMPRESSOR_LZW == type && DMS_INVALID_EXTENT == dictExtID )
      {
         // Push the dictionary job later, set NULL for temporary
         _compressorEntry[ mbID ].setCompressor( NULL ) ;
         goto done ;
      }
      else
      {
         utilCompressor * compressor = getCompressorByType( type ) ;
         _compressorEntry[ mbID ].setCompressor( compressor ) ;
      }

      if ( DMS_INVALID_EXTENT != dictExtID )
      {
         dmsExtRW rw = extent2RW( dictExtID, mbID ) ;
         rw.setNothrow( TRUE ) ;
         dictExtent = rw.readPtr<dmsDictExtent>() ;
         PD_CHECK( NULL != dictExtent && dictExtent->validate( mbID ),
                   SDB_DMS_CORRUPTED_EXTENT, error, PDERROR,
                   "Dictionary extent is invalid. Extent id: %d",
                   dictExtID ) ;

         _compressorEntry[mbID].setDictionary(
            (const utilDictHandle)( beginFixedAddr( dictExtID,
                                                    dictExtent->_blockSize ) +
                                    DMS_DICTEXTENT_HEADER_SZ ) ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__SETCOMPRESSORENTRY, rc ) ;
      return rc ;

   error :
      // On error, set to NULL for safety
      _compressorEntry[ mbID ].setCompressor( NULL ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__ONMAPMETA, "_dmsStorageDataCommon::_onMapMeta" )
   INT32 _dmsStorageDataCommon::_onMapMeta( UINT64 curOffSet )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN upgradeDictInfo = FALSE ;
      BOOLEAN needFlush = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__ONMAPMETA ) ;
      // MME, 4MB
      _mmeSegID = ossMmapFile::segmentSize() ;
      rc = map ( DMS_MME_OFFSET, DMS_MME_SZ, (void**)&_dmsMME ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to map MME: %s", getSuFileName() ) ;
         goto error ;
      }

      if ( _dmsHeader->_version < DMS_COMPRESSION_ENABLE_VER )
      {
         upgradeDictInfo = TRUE ;
      }

      // load collection names in the SU
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         _mbStatInfo[i]._lastWriteTick = ~0 ;
         _mbStatInfo[i]._commitFlag.init( 1 ) ;

         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) )
         {
            _collectionInsert ( _dmsMME->_mbList[i]._collectionName, i,
                                _dmsMME->_mbList[i]._clUniqueID ) ;

            _mbStatInfo[i]._totalRecords = _dmsMME->_mbList[i]._totalRecords ;
            _mbStatInfo[i]._rcTotalRecords.init( _dmsMME->_mbList[i]._totalRecords ) ;
            _mbStatInfo[i]._totalDataPages =
               _dmsMME->_mbList[i]._totalDataPages ;
            _mbStatInfo[i]._totalIndexPages =
               _dmsMME->_mbList[i]._totalIndexPages ;
            _mbStatInfo[i]._totalDataFreeSpace =
               _dmsMME->_mbList[i]._totalDataFreeSpace ;
            _mbStatInfo[i]._totalIndexFreeSpace =
               _dmsMME->_mbList[i]._totalIndexFreeSpace ;
            _mbStatInfo[i]._totalLobPages =
               _dmsMME->_mbList[i]._totalLobPages ;
            _mbStatInfo[i]._totalLobs =
               _dmsMME->_mbList[i]._totalLobs ;
            _mbStatInfo[i]._lastCompressRatio =
               _dmsMME->_mbList[i]._lastCompressRatio ;
            _mbStatInfo[i]._totalDataLen =
               _dmsMME->_mbList[i]._totalDataLen ;
            _mbStatInfo[i]._totalOrgDataLen =
               _dmsMME->_mbList[i]._totalOrgDataLen ;
            _mbStatInfo[i]._startLID =
               _dmsMME->_mbList[i]._logicalID ;
            /*
             * The following branch is for using newer program(SequoiaDB 2.0 or
             * later) with data of elder versions(Before 2.0). As dictionary
             * compression is enabled in 2.0, the data needs to upgrade,
             * because dictionary information such as dictionary extent id is
             * added in MB.
             * As for now, we do not change the version number in dms header, so
             * the second part of the following if statement condition is
             * needed, to avoid changing the dictionary related ids every time.
             * That would be done only the first time after upgrading.
             */
            if ( upgradeDictInfo && ( 0 == _dmsMME->_mbList[i]._dictExtentID ) )
            {
               _dmsMME->_mbList[i]._dictExtentID = DMS_INVALID_EXTENT ;
            }

            /*
             * In version before 2.0, the byte _compressorType is taking now was
             * set to 0. But in the new version, 0 means using snappy to
             * compress. So during the upgrading, set the _comrpessorType to 255
             * ( UTIL_COMPRESSOR_INVALID).
             */
            if ( upgradeDictInfo
                 && !OSS_BIT_TEST( _dmsMME->_mbList[i]._attributes,
                                   DMS_MB_ATTR_COMPRESSED )
                 && ( UTIL_COMPRESSOR_INVALID
                      != _dmsMME->_mbList[i]._compressorType ) )
            {
               _dmsMME->_mbList[i]._compressorType = UTIL_COMPRESSOR_INVALID ;
            }

            /*
               Check the collection is valid
            */
            if ( !isCrashed() )
            {
               if ( 0 == _dmsMME->_mbList[i]._commitFlag )
               {
                  /// upgrade from the old version( _commitLSN = 0 )
                  if ( 0 == _dmsMME->_mbList[i]._commitLSN )
                  {
                     _dmsMME->_mbList[i]._commitLSN =
                        _pStorageInfo->_curLSNOnStart ;
                  }
                  _dmsMME->_mbList[i]._commitFlag = 1 ;
                  needFlush = TRUE ;
               }
               _mbStatInfo[i]._commitFlag.init( 1 ) ;
            }
            else
            {
               _mbStatInfo[i]._commitFlag.init( _dmsMME->_mbList[i]._commitFlag ) ;
            }
            _mbStatInfo[i]._isCrash = ( 0 == _mbStatInfo[i]._commitFlag.peek() ) ?
                                      TRUE : FALSE ;

            // read the max GTID from disk
            _mbStatInfo[i]._maxGlobTransID.init(
                                  _dmsMME->_mbList[i]._maxGlobTransID ) ;

            /// lsn
            _mbStatInfo[i]._lastLSN.init( _dmsMME->_mbList[i]._commitLSN ) ;

            // _mbOptExtentID is added in version 2.9(acoording data version is
            // 3, refer to DMS_DATASU_CUR_VERSION ) when developping capped
            // collection. It's default value is DMS_INVALID_EXTENT, so need
            // to upgrade the existing collections to this default value for
            // those collections which created on cs before this version.
            if ( _dmsHeader->_version < 3 &&
                 DMS_INVALID_EXTENT != _dmsMME->_mbList[i]._mbOptExtentID )
            {
               _dmsMME->_mbList[i]._mbOptExtentID = DMS_INVALID_EXTENT ;
            }
         }
      }

      if ( needFlush )
      {
         flushMME( isSyncDeep() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__ONMAPMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__ONOPENED, "_dmsStorageDataCommon::_onOpened" )
   INT32 _dmsStorageDataCommon::_onOpened()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__ONOPENED ) ;

      /* Initialize compressor entries for collections. */
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              OSS_BIT_TEST( _dmsMME->_mbList[i]._attributes,
                            DMS_MB_ATTR_COMPRESSED ) )
         {
            rc = _initCompressorEntry( i ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to initialize compressor entry for "
                         "collection: %s, rc = %d",
                         _dmsMME->_mbList[i]._collectionName, rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__ONOPENED, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__ONCLOSED, "_dmsStorageDataCommon::_onClosed" )
   void _dmsStorageDataCommon::_onClosed ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__ONCLOSED ) ;
      /// ensure static info will be flushed to file.
      /// do it here first.
      syncMemToMmap() ;

      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATACOMMON__ONCLOSED ) ;
   }

   INT32 _dmsStorageDataCommon::_onFlushDirty( BOOLEAN force, BOOLEAN sync )
   {
      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         _mbStatInfo[i]._commitFlag.init( 1 ) ;
      }
      return SDB_OK ;
   }

   INT32 _dmsStorageDataCommon::_onMarkHeaderValid( UINT64 &lastLSN,
                                                    BOOLEAN sync,
                                                    UINT64 lastTime )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needFlush = FALSE ;
      UINT64 tmpLSN = 0 ;
      UINT32 tmpCommitFlag = 0 ;

      for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
              _mbStatInfo[i]._commitFlag.peek() )
         {
            tmpLSN = _mbStatInfo[i]._lastLSN.peek() ;
            tmpCommitFlag = _mbStatInfo[i]._isCrash ? 0 :
               _mbStatInfo[i]._commitFlag.peek() ;

            if ( tmpLSN != _dmsMME->_mbList[i]._commitLSN ||
                 tmpCommitFlag != _dmsMME->_mbList[i]._commitFlag )
            {
               _dmsMME->_mbList[i]._commitLSN = tmpLSN ;
               _dmsMME->_mbList[i]._commitTime = lastTime ;
               _dmsMME->_mbList[i]._commitFlag = tmpCommitFlag ;
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
         rc = flushMME( sync ) ;
      }
      return rc ;
   }

   INT32 _dmsStorageDataCommon::_onMarkHeaderInvalid( INT32 collectionID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN needSync = FALSE ;

      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         _mbStatInfo[ collectionID ]._lastWriteTick = pmdGetDBTick() ;
         if ( !_mbStatInfo[ collectionID ]._isCrash &&
              _mbStatInfo[ collectionID ]._commitFlag.compareAndSwap( 1, 0 ) )
         {
            needSync = TRUE ;
            _dmsMME->_mbList[ collectionID ]._commitFlag = 0 ;
         }
      }
      else if ( -1 == collectionID )
      {
         for ( UINT16 i = 0 ; i < DMS_MME_SLOTS ; ++i )
         {
            _mbStatInfo[ i ]._lastWriteTick = pmdGetDBTick() ;
            if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) &&
                 !_mbStatInfo[ i ]._isCrash &&
                 _mbStatInfo[ i ]._commitFlag.compareAndSwap( 1, 0 ) )
            {
               needSync = TRUE ;
               _dmsMME->_mbList[ i ]._commitFlag = 0 ;
            }
         }
      }

      if ( needSync )
      {
         rc = flushMME( isSyncDeep() ) ;
      }
      return rc ;
   }

   UINT64 _dmsStorageDataCommon::_getOldestWriteTick() const
   {
      UINT64 oldestWriteTick = ~0 ;
      UINT64 lastWriteTick = 0 ;

      for ( INT32 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         lastWriteTick = _mbStatInfo[i]._lastWriteTick ;
         /// The collection is commit valid, should ignored
         if ( 0 == _mbStatInfo[i]._commitFlag.peek() &&
              lastWriteTick < oldestWriteTick )
         {
            oldestWriteTick = lastWriteTick ;
         }
      }
      return oldestWriteTick ;
   }

   void _dmsStorageDataCommon::_onRestore()
   {
      for ( INT32 i = 0 ; i < DMS_MME_SLOTS ; i++ )
      {
         _mbStatInfo[i]._isCrash = FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__INITMME, "_dmsStorageDataCommon::_initializeMME" )
   void _dmsStorageDataCommon::_initializeMME ()
   {
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__INITMME ) ;
      SDB_ASSERT ( _dmsMME, "MME is NULL" ) ;

      for ( INT32 i = 0; i < DMS_MME_SLOTS ; i++ )
      {
         _dmsMME->_mbList[i].reset () ;
      }
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATACOMMON__INITMME ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__LOGDPS, "_dmsStorageDataCommon::_logDPS" )
   INT32 _dmsStorageDataCommon::_logDPS( SDB_DPSCB * dpsCB,
                                         dpsMergeInfo & info,
                                         pmdEDUCB * cb,
                                         ossSLatch * pLatch,
                                         OSS_LATCH_MODE mode,
                                         BOOLEAN & locked,
                                         UINT32 clLID,
                                         dmsExtentID extLID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__LOGDPS ) ;
      info.setInfoEx( _logicalCSID, clLID, extLID, cb ) ;
      rc = dpsCB->prepare( info ) ;
      if ( rc )
      {
         goto error ;
      }

      // release lock
      if ( pLatch && locked )
      {
         if ( SHARED == mode )
         {
            pLatch->release_shared() ;
         }
         else
         {
            pLatch->release() ;
         }
         locked = FALSE ;
      }
      // write dps
      dpsCB->writeData( info ) ;
   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__LOGDPS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__LOGDPS1, "_dmsStorageDataCommon::_logDPS" )
   INT32 _dmsStorageDataCommon::_logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                                         pmdEDUCB *cb, dmsMBContext *context,
                                         dmsExtentID extLID,
                                         BOOLEAN needUnLock,
                                         DMS_FILE_TYPE type,
                                         UINT32 *clLID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__LOGDPS1 ) ;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET ;
      info.setInfoEx( logicalID(),
                      NULL == clLID ?
                      context->clLID() : *clLID,
                      extLID, cb ) ;
      rc = dpsCB->prepare( info ) ;
      if ( rc )
      {
         goto error ;
      }
      lsn = info.getMergeBlock().record().head()._lsn ;
      context->mbStat()->updateLastLSN( lsn, type ) ;

      // release lock
      if ( needUnLock )
      {
         context->mbUnlock() ;
      }

      // write dps
      dpsCB->writeData( info ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__LOGDPS1, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__ALLOCATEEXTENT, "_dmsStorageDataCommon::_allocateExtent" )
   INT32 _dmsStorageDataCommon::_allocateExtent( dmsMBContext *context,
                                                 UINT16 numPages,
                                                 BOOLEAN deepInit,
                                                 BOOLEAN add2LoadList,
                                                 dmsExtentID *allocExtID )
   {
      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;
      INT32 rc                 = SDB_OK ;
      SINT32 firstFreeExtentID = DMS_INVALID_EXTENT ;
      dmsExtRW extRW ;
      dmsExtent *extAddr       = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__ALLOCATEEXTENT ) ;
      PD_TRACE3 ( SDB__DMSSTORAGEDATACOMMON__ALLOCATEEXTENT,
                  PD_PACK_USHORT ( numPages ),
                  PD_PACK_UINT ( deepInit ),
                  PD_PACK_UINT ( add2LoadList ) ) ;
      if ( numPages > segmentPages() || numPages < 1 )
      {
         PD_LOG ( PDERROR, "Invalid number of pages: %d", numPages ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb lock failed, rc: %d", rc ) ;

      rc = _findFreeSpace ( numPages, firstFreeExtentID, context ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Error find free space for %d pages, rc = %d",
                  numPages, rc ) ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT == firstFreeExtentID )
      {
         // In _findFreeSpace, it decided not to allocate space this time.
         *allocExtID = DMS_INVALID_EXTENT ;
         goto done ;
      }

      extRW = extent2RW( firstFreeExtentID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.writePtr<dmsExtent>() ;
      if ( !extAddr )
      {
         PD_LOG( PDERROR, "Get extent[%d] address failed",
                 firstFreeExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      /// Init
      extAddr->init( numPages, context->mbID(),
                     (UINT32)numPages << pageSizeSquareRoot() ) ;

      // and add the new extent into MB's extent chain
      // now let's change the extent pointer into MB's extent list
      // new extent->preextent always assign to _mbList.lastExtentID
      if ( TRUE == add2LoadList )
      {
         extAddr->_prevExtent = context->mb()->_loadLastExtentID ;
         extAddr->_nextExtent = DMS_INVALID_EXTENT ;
         // if this is the load first extent in this MB, we assign
         // firstExtentID to it
         if ( DMS_INVALID_EXTENT == context->mb()->_loadFirstExtentID )
         {
            context->mb()->_loadFirstExtentID = firstFreeExtentID ;
         }

         if ( DMS_INVALID_EXTENT != extAddr->_prevExtent )
         {
            dmsExtRW prevRW = extent2RW( extAddr->_prevExtent,
                                         context->mbID() ) ;
            dmsExtent *prevExt = prevRW.writePtr<dmsExtent>() ;
            prevExt->_nextExtent = firstFreeExtentID ;
         }

         // MB's last extent always assigned to the new extent
         context->mb()->_loadLastExtentID = firstFreeExtentID ;
      }
      else
      {
         rc = addExtent2Meta( firstFreeExtentID, extAddr, context ) ;
         PD_RC_CHECK( rc, PDERROR, "Add extent to meta failed, rc: %d", rc ) ;
      }

      if ( allocExtID )
      {
         *allocExtID = firstFreeExtentID ;
         PD_TRACE1 ( SDB__DMSSTORAGEDATACOMMON__ALLOCATEEXTENT,
                     PD_PACK_INT ( firstFreeExtentID ) ) ;
      }

      if ( deepInit )
      {
         _onAllocExtent( context, extAddr, firstFreeExtentID ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__ALLOCATEEXTENT, rc ) ;
      return rc ;
   error :
      if ( DMS_INVALID_EXTENT != firstFreeExtentID )
      {
         _freeExtent( firstFreeExtentID, context->mbID() ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT, "_dmsStorageDataCommon::_freeExtent" )
   INT32 _dmsStorageDataCommon::_freeExtent( dmsExtentID extentID,
                                             INT32 collectionID )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW extRW ;
      const dmsExtent *extAddr = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT ) ;
      if ( DMS_INVALID_EXTENT == extentID )
      {
         PD_LOG( PDERROR, "Invalid extent id for free" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      PD_TRACE1 ( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT,
                  PD_PACK_INT ( extentID ) ) ;
      extRW = extent2RW( extentID, collectionID ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.readPtr<dmsExtent>() ;

      if ( !extAddr || !extAddr->validate() )
      {
         PD_LOG ( PDERROR, "Invalid eye catcher or flag for extent %d",
                  extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /*
      * Not to write the extent, so perfermance will improved
      *
      extAddr->_flag = DMS_EXTENT_FLAG_FREED ;
      // change logical id
      extAddr->_logicID = DMS_INVALID_EXTENT ;
      */

      rc = _releaseSpace( extentID, extAddr->_blockSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page, rc = %d", rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT2, "_dmsStorageDataCommon::_freeExtent" )
   INT32 _dmsStorageDataCommon::_freeExtent( dmsMBContext *context,
                                             dmsExtentID extentID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT2 ) ;
      dmsExtRW extRW ;
      dmsExtent *extAddr = NULL ;

      if ( DMS_INVALID_EXTENT == extentID )
      {
         PD_LOG( PDERROR, "Invalid extent id for free" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold exclusive lock" ) ;
         rc = SDB_SYS ;
         goto done ;
      }

      extRW = extent2RW( extentID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      extAddr = extRW.writePtr<dmsExtent>() ;

      if ( !extAddr || !extAddr->validate() )
      {
         PD_LOG ( PDERROR, "Invalid eye catcher or flag for extent %d",
                  extentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = removeExtentFromMeta( context, extentID, extAddr ) ;
      PD_RC_CHECK( rc, PDERROR, "Remove extent from meta failed: %d", rc ) ;

      extAddr->_flag = DMS_EXTENT_FLAG_FREED ;
      extAddr->_logicID = DMS_INVALID_EXTENT ;

      rc = _releaseSpace( extentID, extAddr->_blockSize ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release page, rc = %d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__FREEEXTENT2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECTION, "_dmsStorageDataCommon::_truncateCollection" )
   INT32 _dmsStorageDataCommon::_truncateCollection( dmsMBContext *context,
                                                     BOOLEAN needChangeCLID )
   {
      INT32 rc                     = SDB_OK ;
      dmsExtRW lastRW ;
      dmsExtRW metaRW ;
      dmsExtRW dictRW ;
      dmsExtentID currentExt       = DMS_INVALID_EXTENT ;
      dmsExtentID prevExt          = DMS_INVALID_EXTENT ;
      dmsMetaExtent *metaExt       = NULL ;
      dmsDictExtent *dictExt       = NULL ;
      BOOLEAN reachLast            = FALSE ;
      dmsExtentID nextExt          = DMS_INVALID_EXTENT ;
      const dmsExtent *pCurrExt    = NULL ;

      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECTION ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      currentExt = context->mb()->_lastExtentID ;
      // If there is only one extent, set reachLast as TRUE.
      reachLast = ( currentExt == context->mb()->_firstExtentID ) ?
                  TRUE : FALSE ;

      // reset delete list
      for ( UINT32 i = 0 ; i < dmsMB::_max ; i++ )
      {
         context->mb()->_deleteList[i].reset() ;
      }

      // We should set _totalDataFreeSpace before _freeExtent() which free
      // pages in SME. If not, FreeDataSize calculated by snapshot cs will be
      // larger than the actual one.
      // FreeDataSize = freePages in sme * pageSize + each cl totalDataFreeSpace
      context->mbStat()->_totalDataFreeSpace = 0 ;

      // Free all extent from the end. If the system went down becuase of power
      // cut, the file may be damanged. During the recovery, if we find any
      // damaged extent, try to truncate from the beginning also.
      while ( DMS_INVALID_EXTENT != currentExt )
      {
         try
         {
            lastRW = extent2RW( currentExt, context->mbID() ) ;
            pCurrExt = lastRW.readPtr<dmsExtent>() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception:%s", e.what() ) ;
            break ;
         }

         // get the previous extent
         prevExt = pCurrExt->_prevExtent ;
         // free the extent
         rc = _freeExtent ( currentExt, context->mbID() ) ;
         if ( rc )
         {
            SDB_ASSERT( SDB_OK == rc ||
                        SDB_DB_NORMAL != PMD_DB_STATUS(),
                        "Free extent can't be failure" ) ;
            PD_LOG ( PDERROR, "Failed to free extent[%u], rc: %d", currentExt,
                     rc ) ;
            rc = SDB_DMS_CORRUPTED_EXTENT ;
            break ;
         }

         // set last to previous
         currentExt = prevExt ;
         // update MB
         context->mb()->_lastExtentID = currentExt ;
         if ( currentExt == context->mb()->_firstExtentID )
         {
            // Now ready to release the first(also the last for now) extent.
            reachLast = TRUE ;
         }
      }

      if ( !reachLast )
      {
         // Extent error found, now try to free from the beginning.
         currentExt = context->mb()->_firstExtentID ;
         while ( DMS_INVALID_EXTENT != currentExt )
         {
            try
            {
               lastRW = extent2RW( currentExt, context->mbID() ) ;
               pCurrExt = lastRW.readPtr<dmsExtent>() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Occur exception:%s", e.what() ) ;
               break ;
            }

            nextExt = pCurrExt->_nextExtent ;
            rc = _freeExtent( currentExt, context->mbID() ) ;
            if ( rc )
            {
               SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
               PD_LOG ( PDERROR, "Failed to free extent[%u], rc: %d", currentExt,
                        rc ) ;
               rc = SDB_DMS_CORRUPTED_EXTENT ;
               break ;
            }

            currentExt = nextExt ;
            context->mb()->_firstExtentID = currentExt ;
            if ( currentExt == context->mb()->_lastExtentID )
            {
               break ;
            }
         }
      }

      context->mb()->_firstExtentID = DMS_INVALID_EXTENT ;
      context->mb()->_lastExtentID = DMS_INVALID_EXTENT ;

      // free all load extent
      currentExt = context->mb()->_loadLastExtentID ;
      while ( DMS_INVALID_EXTENT != currentExt )
      {
         lastRW = extent2RW( currentExt, context->mbID() ) ;
         pCurrExt = lastRW.readPtr<dmsExtent>() ;
         prevExt = pCurrExt->_prevExtent ;
         rc = _freeExtent( currentExt, context->mbID() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to free load extent[%u], rc: %d",
                    currentExt, rc ) ;
            SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
         }
         currentExt = prevExt ;
         context->mb()->_loadLastExtentID = currentExt ;
      }
      context->mb()->_loadFirstExtentID = DMS_INVALID_EXTENT ;

      if ( DMS_INVALID_EXTENT != context->mb()->_mbExExtentID )
      {
         metaRW = extent2RW( context->mb()->_mbExExtentID, context->mbID() ) ;
         metaExt = metaRW.writePtr<dmsMetaExtent>() ;
         metaExt = metaRW.writePtr<dmsMetaExtent>( 0,
                                                  (UINT32)metaExt->_blockSize <<
                                                   pageSizeSquareRoot() ) ;
         metaExt->reset() ;
      }

      /*
       * Incase of drop/truncate collection, destroy the compressor and release
       * the dictionary both in memory and on disk.
       */
      if ( DMS_INVALID_EXTENT != context->mb()->_dictExtentID
           && needChangeCLID )
      {
         dictRW = extent2RW( context->mb()->_dictExtentID,
                             context->mbID() ) ;
         dictExt = dictRW.writePtr<dmsDictExtent>() ;
         _releaseSpace( context->mb()->_dictExtentID, dictExt->_blockSize ) ;
         context->mb()->_dictExtentID = DMS_INVALID_EXTENT ;
         context->mb()->_dictVersion = 0 ;
      }

      context->mbStat()->_totalDataFreeSpace = 0 ;
      context->mbStat()->_totalDataPages = 0 ;
      context->mbStat()->_totalRecords = 0 ;
      context->mbStat()->_rcTotalRecords.init( 0 ) ;
      context->mbStat()->_totalDataLen = 0 ;
      context->mbStat()->_totalOrgDataLen = 0 ;

      {
         dmsTransLockCallback callback( pmdGetKRCB()->getTransCB(), NULL ) ;
         callback.onCLTruncated( CSID(), context->mbID() ) ;
      }

      _onCollectionTruncated( context ) ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECITONLOADS, "_dmsStorageDataCommon::_truncateCollectionLoads" )
   INT32 _dmsStorageDataCommon::_truncateCollectionLoads( dmsMBContext * context )
   {
      INT32 rc                     = SDB_OK ;
      dmsExtRW extRW ;
      dmsExtentID lastExt          = DMS_INVALID_EXTENT ;
      dmsExtentID prevExt          = DMS_INVALID_EXTENT ;

      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECITONLOADS ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      // free all load extent
      lastExt = context->mb()->_loadLastExtentID ;
      while ( DMS_INVALID_EXTENT != lastExt )
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         extRW = extent2RW( lastExt, context->mbID() ) ;
         const dmsExtent *pLastExt = extRW.readPtr<dmsExtent>() ;
         prevExt = pLastExt->_prevExtent ;
         rc = _freeExtent( lastExt, context->mbID() ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to free load extent[%u], rc: %d",
                    lastExt, rc ) ;
            SDB_ASSERT( SDB_OK == rc, "Free extent can't be failure" ) ;
         }
         lastExt = prevExt ;
         context->mb()->_loadLastExtentID = lastExt ;

         if ( DMS_INVALID_EXTENT != lastExt )
         {
            context->mbUnlock() ;
         }
      }
      context->mb()->_loadFirstExtentID = DMS_INVALID_EXTENT ;

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECITONLOADS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_ADDEXTENT2META, "_dmsStorageDataCommon::addExtent2Meta" )
   INT32 _dmsStorageDataCommon::addExtent2Meta( dmsExtentID extID,
                                                dmsExtent *extent,
                                                dmsMBContext *context )
   {
      INT32 rc = SDB_OK ;
      dmsExtRW mbExRW ;
      dmsMBEx *mbEx = NULL ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_ADDEXTENT2META ) ;
      UINT32 segID = extent2Segment( extID ) - dataStartSegID() ;
      dmsExtentID lastExtID = DMS_INVALID_EXTENT ;
      dmsExtRW prevRW ;
      dmsExtRW nextRW ;
      dmsExtent *prevExt = NULL ;
      dmsExtent *nextExt = NULL ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      // Currently including system temp su and capped su.
      if ( !isBlockScanSupport() )
      {
         extent->_prevExtent = context->mb()->_lastExtentID ;

         // if this is the first extent in this MB, we assign firstExtentID to
         // it
         if ( DMS_INVALID_EXTENT == context->mb()->_firstExtentID )
         {
            context->mb()->_firstExtentID = extID ;
         }

         // if there's previous record, we reassign the previous
         // extent->nextExtent to this new extent
         if ( DMS_INVALID_EXTENT != extent->_prevExtent )
         {
            prevRW = extent2RW( extent->_prevExtent, context->mbID() ) ;
            dmsExtent *prevExt = prevRW.writePtr<dmsExtent>() ;
            prevExt->_nextExtent = extID ;
            extent->_logicID = prevExt->_logicID + 1 ;
         }
         else
         {
            extent->_logicID = DMS_INVALID_EXTENT + 1 ;
         }

         // MB's last extent always assigned to the new extent
         context->mb()->_lastExtentID = extID ;
      }
      else
      {
         mbExRW = extent2RW( context->mb()->_mbExExtentID,
                             context->mbID() ) ;
         mbExRW.setNothrow( TRUE ) ;
         mbEx = mbExRW.writePtr<dmsMBEx>() ;
         if ( NULL == mbEx )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "dms mb expand extent is invalid: %d",
                    context->mb()->_mbExExtentID ) ;
            goto error ;
         }

         if ( segID >= mbEx->_header._segNum )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid segID: %d, max segNum: %d", segID,
                    mbEx->_header._segNum ) ;
            goto error ;
         }

         /// re-write ptr
         mbEx = mbExRW.writePtr<dmsMBEx>( 0,
                                          (UINT32)mbEx->_header._blockSize <<
                                          pageSizeSquareRoot() ) ;
         mbEx->getLastExtentID( segID, lastExtID ) ;

         if ( DMS_INVALID_EXTENT == lastExtID )
         {
            extent->_logicID = ( segID << _getFactor() ) ;
            mbEx->setFirstExtentID( segID, extID ) ;
            mbEx->setLastExtentID( segID, extID ) ;
            ++(mbEx->_header._usedSegNum) ;

            // find prevExt
            INT32 tmpSegID = segID ;
            dmsExtentID tmpExtID = DMS_INVALID_EXTENT ;
            while ( DMS_INVALID_EXTENT != context->mb()->_firstExtentID &&
                    --tmpSegID >= 0 )
            {
               mbEx->getLastExtentID( tmpSegID, tmpExtID ) ;
               if ( DMS_INVALID_EXTENT != tmpExtID )
               {
                  extent->_prevExtent = tmpExtID ;
                  prevRW = extent2RW( tmpExtID, context->mbID() ) ;
                  prevExt = prevRW.writePtr<dmsExtent>() ;
                  break ;
               }
            }
         }
         else
         {
            mbEx->setLastExtentID( segID, extID ) ;
            extent->_prevExtent = lastExtID ;
            prevRW = extent2RW( lastExtID, context->mbID() ) ;
            prevExt = prevRW.writePtr<dmsExtent>() ;
            extent->_logicID = prevExt->_logicID + 1 ;
         }

         if ( prevExt )
         {
            if ( DMS_INVALID_EXTENT != prevExt->_nextExtent )
            {
               extent->_nextExtent = prevExt->_nextExtent ;
               nextRW = extent2RW( extent->_nextExtent, context->mbID() ) ;
               nextExt = nextRW.writePtr<dmsExtent>() ;
               nextExt->_prevExtent = extID ;
            }
            else
            {
               context->mb()->_lastExtentID = extID ;
            }
            prevExt->_nextExtent = extID ;
         }
         else
         {
            if ( DMS_INVALID_EXTENT != context->mb()->_firstExtentID )
            {
               extent->_nextExtent = context->mb()->_firstExtentID ;
               nextRW = extent2RW( extent->_nextExtent, context->mbID() ) ;
               nextExt = nextRW.writePtr<dmsExtent>() ;
               nextExt->_prevExtent = extID ;
            }
            context->mb()->_firstExtentID = extID ;

            if ( DMS_INVALID_EXTENT == context->mb()->_lastExtentID )
            {
               context->mb()->_lastExtentID = extID ;
            }
         }
      }

      context->mbStat()->_totalDataPages += extent->_blockSize ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_ADDEXTENT2META, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_REMOVEEXTENTFROMMETA, "_dmsStorageDataCommon::removeExtentFromMeta" )
   INT32 _dmsStorageDataCommon::removeExtentFromMeta( dmsMBContext *context,
                                                      dmsExtentID extID,
                                                      dmsExtent *extent )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_REMOVEEXTENTFROMMETA ) ;
      dmsExtRW mbExRW ;
      dmsMBEx *mbEx = NULL ;
      dmsExtentID firstExtID = DMS_INVALID_EXTENT ;
      dmsExtentID lastExtID = DMS_INVALID_EXTENT ;
      dmsExtRW prevRW ;
      dmsExtRW nextRW ;
      dmsExtent *prevExt = NULL ;
      dmsExtent *nextExt = NULL ;

      UINT32 segID = extent2Segment( extID ) - dataStartSegID() ;

      if ( !isBlockScanSupport() )
      {
         if ( DMS_INVALID_EXTENT != extent->_prevExtent )
         {
            prevRW = extent2RW( extent->_prevExtent, context->mbID() ) ;
            prevRW.setNothrow( TRUE ) ;
            prevExt = prevRW.writePtr<dmsExtent>() ;
            if ( !prevExt )
            {
               PD_LOG( PDERROR, "Invalid extent: %d", extent->_prevExtent ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            prevExt->_nextExtent = extent->_nextExtent ;
         }
         else if ( extID == context->mb()->_firstExtentID )
         {
            context->mb()->_firstExtentID = extent->_nextExtent ;
         }

         if ( DMS_INVALID_EXTENT != extent->_nextExtent )
         {
            nextRW = extent2RW( extent->_nextExtent, context->mbID() ) ;
            nextRW.setNothrow( TRUE ) ;
            nextExt = nextRW.writePtr<dmsExtent>() ;
            if ( !nextExt )
            {
               PD_LOG( PDERROR, "Invalid extent: %d", extent->_nextExtent ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            nextExt->_prevExtent = extent->_prevExtent ;
         }
         else if ( extID == context->mb()->_lastExtentID )
         {
            context->mb()->_lastExtentID = extent->_prevExtent ;
         }
      }
      else
      {
         mbExRW = extent2RW( context->mb()->_mbExExtentID, context->mbID() ) ;
         mbExRW.setNothrow( TRUE ) ;
         mbEx = mbExRW.writePtr<dmsMBEx>() ;
         if ( NULL == mbEx )
         {
            PD_LOG( PDERROR, "dms mb expand extent is invalid: %d",
                    context->mb()->_mbExExtentID ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( segID >= mbEx->_header._segNum )
         {
            PD_LOG( PDERROR, "Invalid segID: %d, max segNum: %d", segID,
                    mbEx->_header._segNum ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         mbEx = mbExRW.writePtr<dmsMBEx>( 0,
                                          (UINT32)mbEx->_header._blockSize <<
                                          pageSizeSquareRoot() ) ;
         mbEx->getFirstExtentID( segID, firstExtID ) ;
         mbEx->getLastExtentID( segID, lastExtID ) ;

         // Get the previous and next extents, if exist.
         if ( DMS_INVALID_EXTENT != extent->_prevExtent )
         {
            prevRW = extent2RW( extent->_prevExtent, context->mbID() ) ;
            prevRW.setNothrow( TRUE ) ;
            prevExt = prevRW.writePtr<dmsExtent>() ;
            if ( !prevExt )
            {
               PD_LOG( PDERROR, "Extent[%d] is invalid", extent->_prevExtent ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         if ( DMS_INVALID_EXTENT != extent->_nextExtent )
         {
            nextRW= extent2RW( extent->_nextExtent, context->mbID() ) ;
            nextRW.setNothrow( TRUE ) ;
            nextExt = nextRW.writePtr<dmsExtent>() ;
            if( !nextExt )
            {
               PD_LOG( PDERROR, "Extent[%d] is invalid", extent->_nextExtent ) ;
               rc = SDB_SYS ;
               goto error ;
            }
         }

         // Modify the first/last extent in meta.
         if ( extID == firstExtID && extID == lastExtID )
         {
            mbEx->setFirstExtentID( segID, DMS_INVALID_EXTENT ) ;
            mbEx->setLastExtentID( segID, DMS_INVALID_EXTENT ) ;
            --(mbEx->_header._usedSegNum ) ;
         }
         else if ( extID == firstExtID )
         {
            mbEx->setFirstExtentID( segID, extent->_nextExtent ) ;
         }
         else if ( extID == lastExtID )
         {
            mbEx->setLastExtentID( segID, extent->_prevExtent ) ;
         }

         // Modify the extent list in mb.
         if ( extID == context->mb()->_firstExtentID )
         {
            context->mb()->_firstExtentID = extent->_nextExtent ;
            if ( nextExt )
            {
               nextExt->_prevExtent = DMS_INVALID_EXTENT ;
            }
         }
         else if ( extID == context->mb()->_lastExtentID )
         {
            context->mb()->_lastExtentID = extent->_prevExtent ;
            if ( !prevExt )
            {
               prevExt->_nextExtent = DMS_INVALID_EXTENT ;
            }
         }
         else
         {
            SDB_ASSERT( nextExt && prevExt,
                        "Prev and next extent should not be NULL" ) ;
            prevExt->_nextExtent = extent->_nextExtent ;
            nextExt->_prevExtent = extent->_prevExtent ;
         }
      }

      context->mbStat()->_totalDataPages -= extent->_blockSize ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON_REMOVEEXTENTFROMMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_ADDCOLLECTION, "_dmsStorageDataCommon::addCollection" )
   INT32 _dmsStorageDataCommon::addCollection( const CHAR * pName,
                                               UINT16 * collectionID,
                                               utilCLUniqueID clUniqueID,
                                               UINT32 attributes,
                                               pmdEDUCB * cb,
                                               SDB_DPSCB * dpscb,
                                               UINT16 initPages,
                                               BOOLEAN sysCollection,
                                               UINT8 compressionType,
                                               UINT32 *logicID,
                                               const BSONObj *extOptions )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_ADDCOLLECTION ) ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      UINT16 newCollectionID  = DMS_INVALID_MBID ;
      UINT32 logicalID        = DMS_INVALID_CLID ;
      BOOLEAN metalocked      = FALSE ;
      dmsMB *mb               = NULL ;
      SDB_DPSCB *dropDps      = NULL ;
      dmsMBContext *context   = NULL ;

      UINT32 segNum           = DMS_MAX_PG >> segmentPagesSquareRoot() ;
      UINT32 mbExSize         = (( segNum << 3 ) >> pageSizeSquareRoot()) + 1 ;
      UINT16 optExtSize       = 0 ;
      dmsExtentID mbExExtent  = DMS_INVALID_EXTENT ;
      dmsExtentID mbOptExtent = DMS_INVALID_EXTENT ;
      dmsMetaExtent *mbExtent = NULL ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      // check cl name
      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLCrt2Record( _clFullName(pName, fullName, sizeof(fullName)),
                               clUniqueID, attributes, compressionType,
                               extOptions, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      rc = _prepareAddCollection( extOptions, mbOptExtent, optExtSize ) ;
      PD_RC_CHECK( rc, PDERROR, "onAddCollection operation failed: %d", rc ) ;

      // Only allocate the meta extent when block scan is supported.
      if ( isBlockScanSupport() )
      {
         rc = _findFreeSpace( mbExSize, mbExExtent, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate metablock expand extent failed, "
                      "pageNum: %d, rc: %d", mbExSize, rc ) ;
      }

      // first exclusive latch metadata, this shouldn't be replaced by SHARED to
      // prevent racing with dropCollection
      ossLatch( &_metadataLatch, EXCLUSIVE ) ;
      metalocked = TRUE ;

      // then let's make sure the collection name does not exist
      if ( DMS_INVALID_MBID != _collectionNameLookup ( pName ) )
      {
         rc = SDB_DMS_EXIST ;
         goto error ;
      }
      {
         UINT16 tmpMbID = DMS_INVALID_MBID ;
         tmpMbID = _collectionIdLookup ( clUniqueID ) ;
         if ( DMS_INVALID_MBID != tmpMbID )
         {
            const CHAR* clname = _dmsMME->_mbList[tmpMbID]._collectionName ;
            rc = SDB_DMS_UNIQUEID_CONFLICT ;
            PD_LOG ( PDERROR,
                     "CL unique id[%llu] already exists[name: %s.%s], rc: %d",
                     clUniqueID, getSuName(), clname, rc ) ;
            goto error ;
         }
      }

      if ( DMS_MME_SLOTS <= _dmsHeader->_numMB )
      {
         PD_LOG ( PDERROR, "There is no free slot for extra collection" ) ;
         rc = SDB_DMS_NOSPC ;
         goto error ;
      }

      // find a slot
      for ( UINT32 i = 0 ; i < DMS_MME_SLOTS ; ++i )
      {
         if ( DMS_IS_MB_FREE ( _dmsMME->_mbList[i]._flag ) )
         {
            newCollectionID = i ;
            break ;
         }
      }
      // make sure we find free collection id
      if ( DMS_INVALID_MBID == newCollectionID )
      {
         PD_LOG ( PDERROR, "Unable to find free collection id" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // set mb meta data and header data
      logicalID = _dmsHeader->_MBHWM++ ;
      mb = &_dmsMME->_mbList[newCollectionID] ;
      mb->reset( pName, clUniqueID, newCollectionID, logicalID,
                 attributes, compressionType ) ;
      _mbStatInfo[ newCollectionID ].reset() ;
      _mbStatInfo[ newCollectionID ]._startLID = logicalID ;
      _compressorEntry[ newCollectionID ].reset() ;

      _dmsHeader->_numMB++ ;
      _collectionInsert( pName, newCollectionID, clUniqueID ) ;

      if ( isBlockScanSupport() )
      {
         dmsExtRW rw = extent2RW( mbExExtent, newCollectionID ) ;
         rw.setNothrow( TRUE ) ;
         mbExtent = rw.writePtr<dmsMetaExtent>( 0,
                                                mbExSize <<
                                                pageSizeSquareRoot() ) ;
         PD_CHECK( mbExtent, SDB_SYS, error, PDERROR,
                   "Invalid meta extent[%d]", mbExExtent ) ;
         mbExtent->init( mbExSize, newCollectionID, segNum ) ;
         mb->_mbExExtentID = mbExExtent ;
         mbExExtent = DMS_INVALID_EXTENT ;
      }

      rc = _onAddCollection( extOptions, mbOptExtent,
                             optExtSize, newCollectionID ) ;
      PD_RC_CHECK( rc, PDERROR, "onAddCollection operation failed: %d", rc ) ;
      mb->_mbOptExtentID = mbOptExtent ;

      // lock mb context before release meta lock
      rc = getMBContext( &context, newCollectionID, logicalID, logicalID,
                         EXCLUSIVE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get mb[%u] context, rc: %d",
                 newCollectionID, rc ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            newCollectionID = DMS_INVALID_MBID ;
         }
         goto error ;
      }

      // write dps log
      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE,
                       metalocked, logicalID, DMS_INVALID_EXTENT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLcrt record to log, "
                      "rc = %d", rc ) ;
      }

      // release meta lock
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }

      if ( collectionID )
      {
         *collectionID = newCollectionID ;
      }
      dropDps = dpscb ;

      /// set compressor when snappy
      _setCompressor( context ) ;

      // allocate new extent
      if ( 0 != initPages )
      {
         rc = _allocateExtent( context, initPages, TRUE, FALSE, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Allocate new %u pages of collection[%s] "
                      "failed, rc: %d", initPages, pName, rc ) ;
      }

      // create $id index[s_idKeyObj]
      if ( !OSS_BIT_TEST( attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         rc = _pIdxSU->createIndex( context, ixmGetIDIndexDefine(),
                                    cb, NULL, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Create $id index failed in collection[%s], "
                      "rc: %d", pName, rc ) ;
      }

      if ( logicID )
      {
         *logicID = logicalID ;
      }

      if ( cb && cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

      if ( _pEventHolder )
      {
         dmsEventCLItem clItem( context->mb()->_collectionName,
                                context->mbID(),
                                context->clLID() ) ;
         _pEventHolder->onCreateCL( DMS_EVENT_MASK_ALL, clItem, cb, dpscb ) ;
      }

   done:
      if ( context )
      {
         releaseMBContext( context ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         flushMeta( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_ADDCOLLECTION, rc ) ;
      return rc ;
   error:
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
      }
      if ( DMS_INVALID_EXTENT != mbExExtent )
      {
         _releaseSpace( mbExExtent, mbExSize ) ;
      }
      if ( DMS_INVALID_EXTENT != mbOptExtent )
      {
         _releaseSpace( mbOptExtent, optExtSize ) ;
      }
      if ( DMS_INVALID_MBID != newCollectionID )
      {
         // drop collection
         INT32 rc1 = dropCollection( pName, cb, dropDps, sysCollection,
                                     context ) ;
         if ( rc1 )
         {
            PD_LOG( PDSEVERE, "Failed to clean up bad collection creation[%s], "
                    "rc: %d", pName, rc ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_DROPCOLLECTION, "_dmsStorageDataCommon::dropCollection" )
   INT32 _dmsStorageDataCommon::dropCollection( const CHAR * pName, pmdEDUCB * cb,
                                                SDB_DPSCB * dpscb,
                                                BOOLEAN sysCollection,
                                                dmsMBContext * context )
   {
      INT32 rc                = 0 ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_DROPCOLLECTION ) ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      BOOLEAN getContext      = FALSE ;
      BOOLEAN metalocked      = FALSE ;
      utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
      BOOLEAN isTransLocked   = FALSE ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLDel2Record( _clFullName(pName, fullName, sizeof(fullName)),
                               record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      // lock collection mb exclusive lock
      if ( NULL == context )
      {
         rc = getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get mb[%s] context, rc: %d",
                      pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Collection[%s] mblock failed, rc: %d",
                      pName, rc ) ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      if ( _pEventHolder )
      {
         dmsEventCLItem clItem( context->mb()->_collectionName,
                                context->mbID(),
                                context->clLID() ) ;
         _pEventHolder->onDropCL( DMS_EVENT_MASK_ALL, clItem, cb, dpscb ) ;
      }

      // it is not need to lock that drop temp collection while startup
      // which cb is NULL
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = pTransCB->transLockTryX( cb, _logicalCSID, context->mbID(),
                                       NULL, &lockConflict ) ;
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

         isTransLocked = TRUE ;
      }

      // drop all index
      rc = _pIdxSU->dropAllIndexes( context, cb, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop index for collection[%s], "
                   "rc: %d", pName, rc ) ;

      // truncate the collection
      _rmCompressor( context ) ;

      rc = _truncateCollection( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate the collection[%s], rc: %d",
                   pName, rc ) ;

      // truncate lob
      if ( _pLobSU->isOpened() )
      {
         rc = _pLobSU->truncate( context, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate the collection[%s] lob,"
                      "rc: %d", pName, rc ) ;
      }

      // change mb meta data
      DMS_SET_MB_DROPPED( context->mb()->_flag ) ;
      context->mb()->_logicalID-- ;
      DMS_MB_STATINFO_CLEAR_TRUNCATED( context->mbStat()->_flag ) ;

      if ( DMS_INVALID_EXTENT != context->mb()->_mbExExtentID )
      {
         dmsExtRW rw = extent2RW( context->mb()->_mbExExtentID,
                                  context->mbID() ) ;
         rw.setNothrow( TRUE ) ;
         const dmsMetaExtent *metaExt = rw.readPtr<dmsMetaExtent>() ;
         if ( metaExt )
         {
            _releaseSpace( context->mb()->_mbExExtentID, metaExt->_blockSize ) ;
         }
         context->mb()->_mbExExtentID = DMS_INVALID_EXTENT ;
      }

      // Release the option extent.
      if ( DMS_INVALID_EXTENT != context->mb()->_mbOptExtentID )
      {
         dmsExtRW rw = extent2RW( context->mb()->_mbOptExtentID,
                                  context->mbID() ) ;
         rw.setNothrow( TRUE ) ;
         const dmsOptExtent *optExt = rw.readPtr<dmsOptExtent>() ;
         if ( optExt )
         {
            _releaseSpace( context->mb()->_mbOptExtentID, optExt->_blockSize ) ;
         }
         context->mb()->_mbOptExtentID = DMS_INVALID_EXTENT ;
      }

      // release mb lock
      context->mbUnlock() ;

      // get unique id from mb. Because if the cl is in _collectionIDMap, and
      // we don't erase it, it may cause core dump.
      clUniqueID = context->mb()->_clUniqueID ;

      // change metadata
      ossLatch( &_metadataLatch, EXCLUSIVE ) ;
      metalocked = TRUE ;
      _collectionRemove( pName, clUniqueID ) ;
      DMS_SET_MB_FREE( context->mb()->_flag ) ;
      _dmsHeader->_numMB-- ;

      // write dps log
      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE, metalocked,
                       context->clLID(), DMS_INVALID_EXTENT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLDel record to log, rc: "
                      "%d", rc ) ;
      }

   done:
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, context->mbID() ) ;
         isTransLocked = FALSE ;
      }
      if ( context && getContext )
      {
         releaseMBContext( context ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         flushMeta( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_DROPCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTION, "_dmsStorageDataCommon::truncateCollection" )
   INT32 _dmsStorageDataCommon::truncateCollection( const CHAR *pName,
                                                    pmdEDUCB *cb,
                                                    SDB_DPSCB *dpscb,
                                                    BOOLEAN sysCollection,
                                                    dmsMBContext *context,
                                                    BOOLEAN needChangeCLID,
                                                    BOOLEAN truncateLob )
   {
      INT32 rc           = SDB_OK ;
      BOOLEAN getContext = FALSE ;
      UINT32 newCLID     = DMS_INVALID_CLID ;
      UINT32 oldCLID     = DMS_INVALID_CLID ;
      UINT64 oldRecords  = 0 ;
      UINT64 oldLobs     = 0 ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTION ) ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      IDmsExtDataHandler* handler = NULL ;
      BOOLEAN isTransLocked   = FALSE ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLTrunc2Record( _clFullName(pName, fullName, sizeof(fullName)),
                                 record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      // lock collection mb exclusive lock
      if ( NULL == context )
      {
         rc = getMBContext( &context, pName, EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get mb[%s] context, rc: %d",
                      pName, rc ) ;
         getContext = TRUE ;
      }
      else
      {
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDWARNING, "Collection[%s] mblock failed, rc: %d",
                      pName, rc ) ;
      }

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      // trans lock
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = pTransCB->transLockTryX( cb, _logicalCSID, context->mbID(),
                                       NULL, &lockConflict ) ;
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

         isTransLocked = TRUE ;
      }

      // pause mb lock and change metadata
      if ( needChangeCLID )
      {
         context->pause() ;
         ossLatch( &_metadataLatch, EXCLUSIVE ) ;
         newCLID = _dmsHeader->_MBHWM++ ;
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         // resume context lock
         rc = context->resume() ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context resume falied, rc: %d",
                      rc ) ;
      }

      oldRecords = context->mbStat()->_totalRecords ;
      oldLobs = context->mbStat()->_totalLobs ;

      if ( context->mbStat()->_textIdxNum > 0 )
      {
         handler = getExtDataHandler() ;
         if ( handler )
         {
            rc = handler->onTruncateCL( getSuName(),
                                        context->mb()->_collectionName,
                                        cb, needChangeCLID ) ;
            PD_RC_CHECK( rc, PDERROR, "External operation on truncate "
                         "collection failed, rc: %d", rc ) ;
         }
         else
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "External data handler is NULL" ) ;
            goto error ;
         }
      }

      rc = _pIdxSU->truncateIndexes( context, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] indexes failed, "
                   "rc: %d", pName, rc ) ;

      /*
       * For LZW, the compressor and dictionary should be removed during
       * truncate. In case of snappy, the compressor should be reserved.
       */
      if ( UTIL_COMPRESSOR_LZW ==
           (UTIL_COMPRESSOR_TYPE)(context->mb()->_compressorType) &&
           needChangeCLID )
      {
         _rmCompressor( context ) ;
      }
      rc = _truncateCollection( context, needChangeCLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] data failed, rc: %d",
                   pName, rc ) ;

      if ( truncateLob && _pLobSU->isOpened() )
      {
         rc = _pLobSU->truncate( context, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] lob failed, rc: %d",
                      pName, rc ) ;
      }

      // change mb metadata
      if ( needChangeCLID )
      {
         oldCLID = context->_clLID ;
         context->mb()->_logicalID = newCLID ;
         context->_clLID           = newCLID ;
      }
      DMS_MB_STATINFO_SET_TRUNCATED( context->mbStat()->_flag ) ;

      if ( handler )
      {
         rc = handler->done( DMS_EXTOPR_TYPE_TRUNCATE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "External done operation failed, rc: %d",
                      rc ) ;
      }

      if ( _pEventHolder )
      {
         dmsEventCLItem clItem( context->mb()->_collectionName,
                                context->mbID(),
                                oldCLID ) ;
         _pEventHolder->onTruncateCL( DMS_EVENT_MASK_ALL, clItem, newCLID,
                                      cb, dpscb ) ;
      }

      // write dps log
      if ( dpscb )
      {
         PD_AUDIT_OP_WITHNAME( AUDIT_DML, "TRUNCATE", AUDIT_OBJ_CL,
                               fullName, rc, "RecordNum:%llu, LobNum:%llu",
                               oldRecords, oldLobs ) ;
         rc = _logDPS( dpscb, info, cb, context, DMS_INVALID_EXTENT,
                       TRUE, DMS_FILE_ALL, &oldCLID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLTrunc record to log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_ALL ) ;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, context->mbID() ) ;
         isTransLocked = FALSE ;
      }
      if ( context && getContext )
      {
         releaseMBContext( context ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      if ( SDB_OK == rc )
      {
         /// flush meta
         flushMeta( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTIONLOADS, "_dmsStorageDataCommon::truncateCollectionLoads" )
   INT32 _dmsStorageDataCommon::truncateCollectionLoads( const CHAR *pName,
                                                         dmsMBContext * context )
   {
      INT32 rc           = SDB_OK ;
      BOOLEAN getContext = FALSE ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTIONLOADS ) ;
      if ( NULL == context )
      {
         SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

         rc = getMBContext( &context, pName, -1 ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get mb[%s] context, rc: %d",
                      pName, rc ) ;
         getContext = TRUE ;
      }

      rc = _truncateCollectionLoads( context ) ;
      PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] loads failed, rc: %d",
                   pName, rc ) ;

   done:
      if ( context && getContext )
      {
         releaseMBContext( context ) ;
      }
      if ( SDB_OK == rc )
      {
         /// flush mme
         flushMME( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTIONLOADS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_CHGUID, "_dmsStorageDataCommon::changeCLUniqueID" )
   INT32 _dmsStorageDataCommon::changeCLUniqueID( const MAP_CLNAME_ID& modifyCl,
                                                  utilCSUniqueID csUniqueID,
                                                  BOOLEAN isLoadCS )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_CHGUID ) ;
      BOOLEAN hasChanged = FALSE ;

      ossScopedLock lock( &_metadataLatch, EXCLUSIVE ) ;

      COLNAME_MAP_IT it = _collectionNameMap.begin() ;
      while ( it != _collectionNameMap.end() )
      {
         string clName ( it->first ) ;
         UINT16 mbID = it->second ;
         it++ ;

         utilCLUniqueID orgClUniqueID = UTIL_UNIQUEID_NULL ;
         utilCLUniqueID newClUniqueID = UTIL_UNIQUEID_NULL ;

         // get current unique id
         orgClUniqueID = _dmsMME->_mbList[mbID]._clUniqueID ;

         // get new unique id
         MAP_CLNAME_ID::const_iterator modifyIt = modifyCl.find( clName ) ;
         if ( modifyIt != modifyCl.end() )
         {
            newClUniqueID = modifyIt->second ;
         }
         else
         {
            if ( isLoadCS )
            {
               newClUniqueID = utilBuildCLUniqueID( csUniqueID,
                                                    UTIL_CLINNERID_LOADCS );
            }
            else
            {
               utilCLInnerID orgInnerID = utilGetCLInnerID( orgClUniqueID ) ;
               newClUniqueID = utilBuildCLUniqueID( csUniqueID, orgInnerID ) ;
            }
         }

         // skip when old id equals to new id
         if ( orgClUniqueID == newClUniqueID )
         {
            continue ;
         }
         hasChanged = TRUE ;

         // set new unique id
         _dmsMME->_mbList[mbID]._clUniqueID = newClUniqueID ;

         _collectionRemove ( clName.c_str(), orgClUniqueID ) ;
         _collectionInsert ( clName.c_str(), mbID, newClUniqueID ) ;

         PD_LOG( PDEVENT,
                 "Change cl[%s.%s] unique id from [%llu] to [%llu]",
                 _dmsHeader->_name, clName.c_str(),
                 orgClUniqueID, newClUniqueID ) ;

      }

      if ( hasChanged )
      {
         flushMME( isSyncDeep() ) ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_CHGUID ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_RENAMECOLLECTION, "_dmsStorageDataCommon::renameCollection" )
   INT32 _dmsStorageDataCommon::renameCollection( const CHAR * oldName,
                                                  const CHAR * newName,
                                                  pmdEDUCB * cb,
                                                  SDB_DPSCB * dpscb,
                                                  BOOLEAN sysCollection )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_RENAMECOLLECTION ) ;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      UINT32 logRecSize       = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      BOOLEAN metalocked      = FALSE ;
      BOOLEAN isTransLocked   = FALSE ;
      UINT16  mbID            = DMS_INVALID_MBID ;
      UINT32  clLID           = DMS_INVALID_CLID ;

      PD_TRACE2 ( SDB__DMSSTORAGEDATACOMMON_RENAMECOLLECTION,
                  PD_PACK_STRING ( oldName ),
                  PD_PACK_STRING ( newName ) ) ;
      rc = dmsCheckCLName ( oldName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid old collection name %s, rc: %d",
                   oldName, rc ) ;
      rc = dmsCheckCLName ( newName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid new collection name %s, rc: %d",
                   newName, rc ) ;

      // reserved log-size
      if ( dpscb )
      {
         rc = dpsCLRename2Record( getSuName(), oldName, newName, record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build log record, rc: %d", rc ) ;

         rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d", rc ) ;

         logRecSize = record.alignedLen() ;
         rc = pTransCB->reservedLogSpace( logRecSize, cb );
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                    logRecSize ) ;
            logRecSize = 0 ;
            goto error ;
         }
      }

      // lock metadata
      ossLatch ( &_metadataLatch, EXCLUSIVE ) ;
      metalocked = TRUE ;

      mbID = _collectionNameLookup( oldName ) ;
      if ( DMS_INVALID_MBID == mbID )
      {
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }
      if ( DMS_INVALID_MBID != _collectionNameLookup ( newName ) )
      {
         rc = SDB_DMS_EXIST ;
         goto error ;
      }

      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = pTransCB->transLockTryS( cb, _logicalCSID, mbID,
                                       NULL, &lockConflict ) ;
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

         isTransLocked = TRUE ;
      }

      if ( _pExtDataHandler )
      {
         rc = _pExtDataHandler->onRenameCL( getSuName(), oldName, newName,
                                            cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "External operation on rename cl failed, "
                                   "rc: %d", rc ) ;
      }

      _collectionRemove ( oldName ) ;
      _collectionInsert ( newName, mbID ) ;
      ossMemset ( _dmsMME->_mbList[mbID]._collectionName, 0,
                  DMS_COLLECTION_NAME_SZ ) ;
      ossStrncpy ( _dmsMME->_mbList[mbID]._collectionName, newName,
                   DMS_COLLECTION_NAME_SZ ) ;
      clLID = _dmsMME->_mbList[mbID]._logicalID ;

      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE, metalocked,
                       clLID, DMS_INVALID_EXTENT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert clrename to log, rc = %d",
                      rc ) ;
      }

      if ( _pEventHolder )
      {
         dmsEventCLItem clItem( oldName, mbID, clLID ) ;
         _pEventHolder->onRenameCL( DMS_EVENT_MASK_ALL, clItem, newName, cb, dpscb ) ;
      }

      PD_LOG( PDEVENT, "Rename collection[%s] to [%s] succeed",
              oldName, newName ) ;
   done :
      if ( metalocked )
      {
         ossUnlatch ( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, mbID ) ;
         isTransLocked = FALSE ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( SDB_OK == rc )
      {
         flushMME( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_RENAMECOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_FINDCOLLECTION, "_dmsStorageDataCommon::findCollection" )
   INT32 _dmsStorageDataCommon::findCollection( const CHAR * pName,
                                                UINT16 & collectionID,
                                                utilCLUniqueID *pClUniqueID )
   {
      INT32 rc            = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_FINDCOLLECTION ) ;
      PD_TRACE1 ( SDB__DMSSTORAGEDATACOMMON_FINDCOLLECTION,
                  PD_PACK_STRING ( pName ) ) ;

      ossLatch ( &_metadataLatch, SHARED ) ;
      collectionID = _collectionNameLookup ( pName ) ;

      PD_TRACE1 ( SDB__DMSSTORAGEDATACOMMON_FINDCOLLECTION,
                  PD_PACK_USHORT ( collectionID ) ) ;

      if ( DMS_INVALID_MBID == collectionID )
      {
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }
      else
      {
         if ( pClUniqueID )
         {
            *pClUniqueID = _dmsMME->_mbList[ collectionID ]._clUniqueID ;
         }
      }

   done :
      ossUnlatch ( &_metadataLatch, SHARED ) ;
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_FINDCOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_INSERTRECORD, "_dmsStorageDataCommon::insertRecord" )
   INT32 _dmsStorageDataCommon::insertRecord ( dmsMBContext *context,
                                               const BSONObj &record,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpscb,
                                               BOOLEAN mustOID,
                                               BOOLEAN canUnLock,
                                               INT64 position,
                                               utilInsertResult *insertResult )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_INSERTRECORD ) ;
      UINT32 dmsRecordSize          = 0 ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      BSONObj insertObj             = record ;
      BOOLEAN hasInsert             = FALSE ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      UINT32 logRecSize             = 0 ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord       = info.getMergeBlock().record() ;
      SDB_DPSCB *dropDps            = NULL ;
      // trans related
      DPS_TRANS_ID transID          = cb->getTransID() ;
      DPS_LSN_OFFSET preTransLsn    = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLsn     = cb->getRelatedTransLSN() ;
      BOOLEAN  isTransLocked        = FALSE ;
      // delete record related
      dmsRecordID foundRID ;
      dmsRecordData recordData ;
      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      const dmsExtent *pExtent      = NULL ;
      BOOLEAN newMem                = FALSE ;
      CHAR *pMergedData             = NULL ;
      _dmsCompressorEntry *compressorEntry = &_compressorEntry[context->mbID()] ;
      UINT32 textIdxNum             = 0 ;
      IDmsExtDataHandler * handler  = NULL ;
      BOOLEAN markInsert            = FALSE ;
      dmsTransLockCallback callback( pTransCB, cb ) ;

      if ( !isTransSupport() )
      {
         transID = DPS_INVALID_TRANS_ID ;
         preTransLsn = DPS_INVALID_LSN_OFFSET ;
         relatedLsn = DPS_INVALID_LSN_OFFSET ;
      }

      try
      {
         dpsTransExecutor *pTransExe = cb->getTransExecutor() ;
         /// when is rollback, and the rid is found
         if ( DPS_INVALID_TRANS_ID != transID && cb->isInTransRollback() &&
              pTransExe->getRecord( relatedLsn, foundRID, TRUE ) )
         {
            markInsert = TRUE ;
            const dmsRecord *pRecord = NULL ;

            rc = context->mbLock( EXCLUSIVE ) ;
            PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d",
                         rc ) ;

            recordRW = record2RW( foundRID, context->mbID() ) ;

            /// 1. check status
            pRecord = recordRW.readPtr<dmsRecord>() ;
            if ( !pRecord->isDeleting() )
            {
               SDB_ASSERT( FALSE, "Record is not deleting" ) ;
               markInsert = FALSE ;
            }
            /// 2. check the value is the same
            else
            {
               rc = extractData( context, recordRW, cb, recordData ) ;
               if ( rc )
               {
                  SDB_ASSERT( FALSE, "Extrace data failed" ) ;
                  markInsert = FALSE ;
               }
               else if ( 0 != insertObj.woCompare(BSONObj(recordData.data())) )
               {
                  SDB_ASSERT( FALSE, "Data is not the same" ) ;
                  markInsert = FALSE ;
               }
            }

            context->mbUnlock() ;
            recordData.setData( insertObj.objdata(), insertObj.objsize(),
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;
         }

         if ( !markInsert )
         {
            rc = _prepareInsertData( record, mustOID, cb, recordData,
                                     newMem, position ) ;
            PD_RC_CHECK( rc, PDERROR, "Prepare data for insertion failed, rc: %d",
                         rc ) ;
            if ( newMem )
            {
               pMergedData = (CHAR *)recordData.data() ;
            }

            insertObj = BSONObj( recordData.data() ) ;
            dmsRecordSize = recordData.len() ;

            // check
            if ( recordData.len() + DMS_RECORD_METADATA_SZ >
                 DMS_RECORD_USER_MAX_SZ )
            {
               rc = SDB_DMS_RECORD_TOO_BIG ;
               goto error ;
            }

            {
               /* For concurrency protection with drop CL and set compresor. */
               dmsCompressorGuard compGuard( compressorEntry, SHARED ) ;
               if ( compressorEntry->ready() )
               {
                  const CHAR *compressedData    = NULL ;
                  INT32 compressedDataSize      = 0 ;
                  UINT8 compressRatio           = 0 ;
                  rc = dmsCompress( cb, compressorEntry,
                                    recordData.data(), recordData.len(),
                                    &compressedData, &compressedDataSize,
                                    compressRatio ) ;
                  // Compression is valid and ratio is less the threshold
                  if ( SDB_OK == rc &&
                       compressedDataSize + sizeof(UINT32) <
                       recordData.orgLen() &&
                       compressRatio < UTIL_COMPRESSOR_DFT_MIN_RATIO )
                  {
                     // 4 bytes len + compressed record
                     dmsRecordSize = compressedDataSize + sizeof(UINT32) ;
                     PD_TRACE2 ( SDB__DMSSTORAGEDATACOMMON_INSERTRECORD,
                                 PD_PACK_STRING ( "size after compress" ),
                                 PD_PACK_UINT ( dmsRecordSize ) ) ;

                     // set the compression data
                     recordData.setData( compressedData, compressedDataSize,
                                         compressorEntry->getCompressorType(),
                                         FALSE ) ;
                  }
                  else if ( rc )
                  {
                     // In any case of error, leave it, and use the
                     // original data.
                     if ( SDB_UTIL_COMPRESS_ABORT == rc )
                     {
                        PD_LOG( PDINFO, "Record compression aborted. "
                                "Insert the original data. rc: %d", rc ) ;
                     }
                     else
                     {
                        PD_LOG( PDWARNING, "Record compression failed. "
                                "Insert the original data. rc: %d", rc ) ;
                     }
                     rc = SDB_OK ;
                  }
               }

               /*
                * Release the guard to avoid deadlock with truncate/drop
                * collection.
                */
               compGuard.release() ;
            }

            // Step 2: Calculate the required space size, and allocate it,
            // both for data record and replication log.
            // Get the final recordsize that we have to allocate:
            // reserved space, alignment, etc.
            _finalRecordSize( dmsRecordSize, recordData ) ;
         }

         // calc log reserve
         if ( dpscb )
         {
            _clFullName( context->mb()->_collectionName, fullName,
                         sizeof(fullName) ) ;

            // reserved log-size
            rc = dpsInsert2Record( fullName, insertObj, transID,
                                   preTransLsn, relatedLsn, logRecord ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

            logRecSize = ossAlign4( logRecord.alignedLen() ) ;

            rc = dpscb->checkSyncControl( logRecSize, cb ) ;
            if ( SDB_OK != rc )
            {
               logRecSize = 0 ;
               PD_LOG( PDERROR, "Check sync control failed, rc: %d", rc ) ;
               goto error ;
            }

            rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                       logRecSize ) ;
               logRecSize = 0 ;
               goto error ;
            }
         }

         // lock mb
         rc = context->mbLock( EXCLUSIVE ) ;
         PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

         // then make sure the collection compatiblity
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_INSERT ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }
         else if ( isTransSupport() &&
                   OSS_BIT_TEST( context->mb()->_attributes,
                                 DMS_MB_ATTR_NOIDINDEX ) &&
                   cb->isTransaction() &&
                   !cb->isInTransRollback() )
         {
            // for transaction, we need $id index for rollback
            PD_LOG( PDERROR, "Failed to insert data for transaction when "
                    "autoIndexId is false" ) ;
            rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
            goto error ;
         }

         textIdxNum = context->mbStat()->_textIdxNum ;
         if ( textIdxNum > 0 )
         {
            handler = getExtDataHandler() ;
            if ( handler )
            {
               rc = handler->prepare( DMS_EXTOPR_TYPE_INSERT,
                                      getSuName(),
                                      context->mb()->_collectionName,
                                      NULL, &insertObj, NULL, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "External operation check failed, "
                            "rc: %d", rc ) ;
            }
         }

         if ( markInsert )
         {
            dmsRecord *pRecord = NULL ;
            extRW = extent2RW( foundRID._extent, context->mbID() ) ;
            dmsExtent *pWRExtent = extRW.writePtr< dmsExtent >() ;
            pExtent = pWRExtent ;

            if ( !pExtent->validate( context->mbID() ) )
            {
               rc = SDB_SYS ;
               goto error ;
            }

            recordRW = record2RW( foundRID, context->mbID() ) ;
            pRecord = recordRW.writePtr< dmsRecord >() ;
            pRecord->unsetDeleting() ;

            ++( pWRExtent->_recCount ) ;
            _increaseMBStat( context->mb()->_clUniqueID,
                             &( _mbStatInfo[ context->mbID() ] ), cb ) ;
         }
         else
         {
            if ( position >= 0 )
            {
               rc = _allocRecordSpaceByPos( context, dmsRecordSize, position,
                                            foundRID, cb ) ;
            }
            else
            {
               rc = _allocRecordSpace( context, dmsRecordSize, foundRID, cb ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Allocate space for record failed, "
                         "rc: %d", rc ) ;

            // Step 3: Insert the record into target extent.
            /// validate the extent page information
            extRW = extent2RW( foundRID._extent, context->mbID() ) ;
            pExtent = extRW.readPtr<dmsExtent>() ;
            if ( !pExtent->validate( context->mbID() ) )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            recordRW = record2RW( foundRID, context->mbID() ) ;

            if ( dpscb && isTransSupport() && !cb->isInTransRollback() )
            {
               dpsTransRetInfo lockConflict ;
               callback.setIDInfo( CSID(), context->mbID(), _logicalCSID,
                                   context->clLID() ) ;

               rc = pTransCB->transLockTryX( cb, _logicalCSID,
                                             context->mbID(),
                                             &foundRID, &lockConflict,
                                             &callback ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to insert the record, get "
                           "transaction-X-lock of record failed, "
                           "rc: %d"OSS_NEWLINE
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

               isTransLocked = TRUE ;

               if ( callback.hasError() )
               {
                  rc = callback.getResult() ;
                  goto error ;
               }

               rc = callback.onInsertRecord( context, BSONObj(), foundRID,
                                             NULL, cb ) ;
               if ( rc )
               {
                  goto error ;
               }
            }

            // insert to extent
            rc = _extentInsertRecord( context, extRW, recordRW, recordData,
                                      dmsRecordSize, cb, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to append record, rc: %d", rc ) ;
         }

         hasInsert = TRUE ;
         // update totalInsert monitor counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INSERT, 1 ) ;
         _incWriteRecord() ;

         // insert object's indexes
         rc = _pIdxSU->indexesInsert( context, pExtent->_logicID,
                                      insertObj, foundRID, cb,
                                      dpscb ? &callback : NULL,
                                      insertResult ) ;
         if ( rc )
         {
            if ( insertResult &&
                 insertResult->isMaskEnabled( UTIL_RESULT_MASK_ID ) )
            {
               /// current id
               if ( insertResult->getCurID().isEmpty() )
               {
                  insertResult->setCurrentID( insertObj ) ;
               }
               /// peer id
               if ( insertResult->getPeerID().isEmpty() &&
                    insertResult->getPeerRID().isValid() )
               {
                  BSONObj peerObj ;
                  if ( SDB_OK == fetch( context, insertResult->getPeerRID(),
                                        peerObj, cb, FALSE ) )
                  {
                     insertResult->setPeerID( peerObj ) ;
                  }
               }
            }

            PD_LOG( PDERROR, "Failed to insert to index, rc: %d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      if ( dpscb )
      {
         PD_AUDIT_OP_WITHNAME( AUDIT_INSERT, "INSERT", AUDIT_OBJ_CL,
                               fullName, rc, "%s",
                               insertObj.toString().c_str() ) ;

         /// enable trans
         if ( isTransSupport() )
         {
            info.enableTrans() ;
         }
         rc = _logDPS( dpscb, info, cb, context,
                       pExtent->_logicID, canUnLock,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to insert record into log, "
                       "rc: %d", rc ) ;
         dropDps = dpscb ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

      if ( handler )
      {
         handler->done( DMS_EXTOPR_TYPE_INSERT, cb ) ;
      }

   done:
      // release the lock immediately if it is not transaction-operation,
      // the transaction-operation's lock will release in rollback or commit
      if ( isTransLocked && ( transID == DPS_INVALID_TRANS_ID || rc ) )
      {
         pTransCB->transLockRelease( cb, _logicalCSID, context->mbID(),
                                     &foundRID, &callback ) ;
      }
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      if ( insertResult && SDB_OK == rc )
      {
         insertResult->setReturnIDByObj( insertObj ) ;
         insertResult->incInsertedNum() ;
      }
      if ( pMergedData )
      {
         cb->releaseBuff( pMergedData ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_INSERTRECORD, rc ) ;
      return rc ;
   error:
      ( void )_onInsertFail( context, ( markInsert ? TRUE : hasInsert),
                             foundRID, dropDps,
                             (ossValuePtr)insertObj.objdata(),
                             cb ) ;
      if ( handler )
      {
         handler->abortOperation( DMS_EXTOPR_TYPE_INSERT, cb ) ;
      }
      goto done ;
   }

   // Description:
   //    Based on the passed in record ID, delete a single record, write
   //    corresponding log records and delete related indexes.
   // Input:
   //    RCDoDelete:  in RC isolation level, we can only delete the record
   //                 if caller knows that transaction has committed.
   // Dependency:
   //    Record lock should be held in X. MBlatch should be held.
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_DELETERECORD, "_dmsStorageDataCommon::deleteRecord" )
   INT32 _dmsStorageDataCommon::deleteRecord( dmsMBContext *context,
                                              const dmsRecordID &recordID,
                                              ossValuePtr deletedDataPtr,
                                              pmdEDUCB *cb,
                                              SDB_DPSCB * dpscb,
                                              IDmsOprHandler *pHandler,
                                              const dmsTransRecordInfo *pInfo )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_DELETERECORD ) ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      BOOLEAN isDeleting            = FALSE ;
      BOOLEAN markDeleting          = FALSE ;
      dmsRecordID ovfRID ;
      BSONObj delObject ;
      UINT32 logRecSize             = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record          = info.getMergeBlock().record() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      DPS_TRANS_ID transID          = cb->getTransID() ;
      DPS_LSN_OFFSET preLsn         = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLSN     = cb->getRelatedTransLSN() ;
      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      dmsExtent *pExtent            = NULL ;
      dmsRecord *pRecord            = NULL ;
      dmsRecordData recordData ;
      UINT32 textIdxNum             = 0 ;
      IDmsExtDataHandler *handler   = NULL ;
      BOOLEAN inTrans               = FALSE ;
      BOOLEAN hasWaitLock           = FALSE ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

#ifdef _DEBUG
      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_DELETE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
#endif //_DEBUG

      if ( !isTransSupport() )
      {
         transID = DPS_INVALID_TRANS_ID ;
         preLsn = DPS_INVALID_LSN_OFFSET ;
         relatedLSN = DPS_INVALID_LSN_OFFSET ;
      }

      try
      {
         extRW = extent2RW( recordID._extent, context->mbID() ) ;
         pExtent = extRW.writePtr<dmsExtent>() ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         pRecord = recordRW.writePtr() ;

         if ( pRecord->isOvf() )
         {
            ovfRID = pRecord->getOvfRID() ;
         }
         else if ( pRecord->isOvt() )
         {
            _extentRemoveRecord( context, extRW, recordRW, cb ) ;
            goto done ;
         }
         else if ( pRecord->isDeleted() )
         {
            rc = SDB_DMS_RECORD_NOTEXIST ;
            goto error ;
         }
         else if ( !pRecord->isNormal() )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         // when in transaction(not rollback), we should not immediately
         // delete the record, otherwise TB scan won't be able to find
         // this record even if the current transaction has not committed.
         if ( DPS_INVALID_TRANS_ID != transID && !cb->isInTransRollback() )
         {
            inTrans = TRUE ;
         }

         // don't delete the record, someone are waitting for the record lock,
         // mark the record's attr to DMS_RECORD_FLAG_DELETING and write the log
         // the last one who get the record-X-Lock will delete the record while
         // those who gets record-S-Lock will skip the record.
         if ( pTransCB->hasWait( _logicalCSID, context->mbID(), &recordID ) )
         {
            markDeleting = TRUE ;
            hasWaitLock = TRUE ;
         }
         else if ( inTrans )
         {
            // although this record was inserted by this transaction(itself),
            // we shall not remove it directly from disk; othsewise, in case
            // of rollback, when insertRecord() it doesn't guarantee the same
            // RID can be used/allocated if space allocation is needed.
            // As a result, under condition isolation RC and transwaitlock =
            // TRUE, table scan may wait on that record, index scan will
            // not wait on that record lock.
            markDeleting = TRUE ;
         }

         if ( pRecord->isDeleting() )
         {
            isDeleting = TRUE ;

            if ( !hasWaitLock && markDeleting && pInfo &&
                 pInfo->_refCount == 1 )
            {
               markDeleting = FALSE ;
            }
            else if ( markDeleting )
            {
               goto done ;
            }
         }

         // first time deletion of the record write the LR and delete indexes
         if ( !isDeleting )
         {
            if ( deletedDataPtr )
            {
               recordData.setData( (const CHAR*)deletedDataPtr,
                                   *(UINT32*)deletedDataPtr,
                                   UTIL_COMPRESSOR_INVALID, TRUE ) ;
               if ( pRecord->isCompressed() )
               {
                  recordData.setOrgData( NULL, _getRecordDataLen( pRecord ) ) ;
               }
            }
            else
            {
               rc = extractData( context, recordRW, cb, recordData ) ;
               PD_RC_CHECK( rc, PDERROR, "Extract data failed, rc: %d", rc ) ;
            }

            // delete index keys
            try
            {
               delObject = BSONObj( recordData.data() ) ;

               textIdxNum = context->mbStat()->_textIdxNum ;
               if ( textIdxNum > 0 )
               {
                  handler = getExtDataHandler() ;
                  if ( handler )
                  {
                     rc = handler->prepare( DMS_EXTOPR_TYPE_DELETE,
                                            getSuName(),
                                            context->mb()->_collectionName, NULL,
                                            &delObject, NULL, cb ) ;
                     PD_RC_CHECK( rc, PDERROR, "External operation check failed, "
                                  "rc: %d", rc ) ;
                  }
               }

               // first to reserve dps
               if ( NULL != dpscb )
               {
                  if ( pHandler )
                  {
                     rc = pHandler->onDeleteRecord( context, delObject,
                                                    recordID, &recordRW,
                                                    markDeleting,
                                                    cb ) ;
                     if ( rc )
                     {
                        PD_LOG( PDERROR, "Process delete record(%s) in "
                                "handler failed, rc: %d",
                                delObject.toString().c_str(), rc ) ;
                        goto error ;
                     }
                  }

                  _clFullName( context->mb()->_collectionName, fullName,
                               sizeof(fullName) ) ;

                  // reserved log-size
                  rc = dpsDelete2Record( fullName, delObject, transID,
                                         preLsn, relatedLSN,
                                         record ) ;

                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to build record: %d",rc ) ;
                     goto error ;
                  }

                  rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                               rc ) ;

                  logRecSize = record.alignedLen() ;
                  rc = pTransCB->reservedLogSpace( logRecSize, cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Failed to reserved log space(length=%u)",
                             logRecSize ) ;
                     logRecSize = 0 ;
                     goto error ;
                  }
               }

               // then delete indexes. Note that the old version indexes
               // would be kept in the in memory tree under the cover
               rc = _pIdxSU->indexesDelete( context, pExtent->_logicID,
                                            delObject, recordID, cb,
                                            dpscb ? pHandler : NULL ) ;
               if ( rc )
               {
                  // if index delete fail, let's continue remove the record
                  PD_LOG ( PDERROR, "Failed to delete indexes, rc: %d",rc ) ;
               }
               context->mbStat()->_totalDataLen -= recordData.orgLen() ;
               context->mbStat()->_totalOrgDataLen -= recordData.len() ;
            }
            catch ( std::exception &e )
            {
               PD_LOG ( PDERROR, "Corrupted record: %d:%d: %s",
                        recordID._extent, recordID._offset, e.what() ) ;
               rc = SDB_CORRUPTED_RECORD ;
               goto error ;
            }
         }

         // delete really
         if ( !markDeleting )
         {
            rc = _extentRemoveRecord( context, extRW, recordRW, cb,
                                      !isDeleting ) ;
            PD_RC_CHECK( rc, PDERROR, "Extent remove record failed, "
                         "rc: %d", rc ) ;
            if ( ovfRID.isValid() )
            {
               dmsRecordRW ovfRW = record2RW( ovfRID, context->mbID() ) ;
               _extentRemoveRecord( context, extRW, ovfRW, cb, FALSE ) ;
            }
         }
         // set deleting attr
         else
         {
            pRecord->setDeleting() ;
            // need to dec count
            --( pExtent->_recCount ) ;
            _decreaseMBStat( context->mb()->_clUniqueID,
                             &( _mbStatInfo[ context->mbID() ] ), cb ) ;
            // increase data write counter for deleting marking
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
         }

         if ( !isDeleting )
         {
            // increase conter
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DELETE, 1 ) ;
            _incWriteRecord() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // if we are asked to log
      if ( dpscb && !isDeleting )
      {
         try
         {
            PD_AUDIT_OP_WITHNAME( AUDIT_DELETE, "DELETE", AUDIT_OBJ_CL,
                                  fullName, rc, "%s",
                                  BSONObj( recordData.data() ).toString().c_str() ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Failed to audit delete record: %s", e.what() ) ;
            /// ignore the error
         }

         if ( isTransSupport() )
         {
            info.enableTrans() ;
         }
         rc = _logDPS( dpscb, info, cb, context, pExtent->_logicID, FALSE,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record into log, rc: %d",
                      rc ) ;

         if ( markDeleting && inTrans )
         {
            /// put pair(lsn,record info), when rollback, can insert the record
            /// to the same rid
            cb->getTransExecutor()->putRecord( cb->getEndLsn(), recordID ) ;
         }
      }
      else if ( !isDeleting && cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

      if ( handler )
      {
         handler->done( DMS_EXTOPR_TYPE_DELETE, cb ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      if ( markDeleting && ( !inTrans || isDeleting ) )
      {
         /// start light job to delete the record async
         dmsStartAsyncDeleteRecord( CSID(), context->mbID(), logicalID(),
                                    context->clLID(), recordID ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_DELETERECORD, rc ) ;
      return rc ;
   error :
      if ( handler )
      {
         handler->abortOperation( DMS_EXTOPR_TYPE_DELETE, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_UPDATERECORD, "_dmsStorageDataCommon::updateRecord" )
   INT32 _dmsStorageDataCommon::updateRecord( dmsMBContext *context,
                                              const dmsRecordID &recordID,
                                              ossValuePtr updatedDataPtr,
                                              pmdEDUCB *cb,
                                              SDB_DPSCB *dpscb,
                                              mthModifier &modifier,
                                              BSONObj* newRecord,
                                              IDmsOprHandler *pHandler,
                                              utilUpdateResult *pResult )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_UPDATERECORD ) ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      BSONObj oldMatch, oldChg ;
      BSONObj newMatch, newChg ;
      BSONObj oldShardingKey, newShardingKey ;
      UINT32 logRecSize             = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record = info.getMergeBlock().record() ;
      UINT32 writeMod = DMS_LOG_WRITE_MOD_INCREMENT ;
      UINT32 *pWriteMod = NULL ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      DPS_TRANS_ID transID = cb->getTransID() ;
      DPS_LSN_OFFSET preTransLsn = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLSN = cb->getRelatedTransLSN() ;

      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      const dmsExtent *pExtent  = NULL ;
      const dmsRecord *pRecord = NULL ;
      dmsRecordData recordData ;
      UINT32 textIdxNum = 0 ;
      IDmsExtDataHandler *handler = NULL ;

      rc = _operationPermChk( DMS_ACCESS_TYPE_UPDATE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in permission check of update, rc: %d", rc ) ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "Caller must hold mb exclusive lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !isTransSupport() )
      {
         transID = DPS_INVALID_TRANS_ID ;
         preTransLsn = DPS_INVALID_LSN_OFFSET ;
         relatedLSN = DPS_INVALID_LSN_OFFSET ;
      }

      try
      {
         extRW = extent2RW( recordID._extent, context->mbID() ) ;
         pExtent = extRW.readPtr<dmsExtent>() ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         pRecord = recordRW.readPtr() ;

#ifdef _DEBUG
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_UPDATE ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }
         // validate the extent is valid
         if ( !pExtent->validate( context->mbID() ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
#endif //_DEBUG

         // get data
         if ( updatedDataPtr )
         {
            recordData.setData( (const CHAR*)updatedDataPtr,
                                *(UINT32*)updatedDataPtr,
                                UTIL_COMPRESSOR_INVALID, TRUE ) ;
            if ( pRecord->isCompressed() )
            {
               recordData.setOrgData( NULL, _getRecordDataLen( pRecord ) ) ;
            }
         }
         else
         {
            rc = extractData(  context, recordRW, cb, recordData ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract record data failed, rc: %d",
                         rc ) ;
         }

         try
         {
            BSONObj obj ( recordData.data() ) ;
            // create a new object for updated record
            BSONObj newobj ;

            if ( dpscb )
            {
               if ( DMS_LOG_WRITE_MOD_INCREMENT
                                      == _pStorageInfo->_logWriteMod )
               {
                  rc = modifier.modify ( obj, newobj, &oldMatch, &oldChg,
                                         &newMatch, &newChg,
                                         &oldShardingKey, &newShardingKey ) ;
                  // set to NULL indicate do not write tag DPS_LOG_UPDATE_WRITEMOD
                  pWriteMod = NULL ;
               }
               else
               {
                  writeMod = DMS_LOG_WRITE_MOD_FULL ;
                  rc = modifier.modify ( obj, newobj, &oldMatch, NULL,
                                         &newMatch, NULL,
                                         &oldShardingKey, &newShardingKey ) ;
                  // obj and newobj's life cycle is too short to log dps.
                  oldChg = obj.copy() ;
                  newChg = newobj.copy() ;
                  // others write tag DPS_LOG_UPDATE_WRITEMOD
                  pWriteMod = &writeMod ;
               }

               if ( SDB_OK == rc && pHandler )
               {
                  rc = pHandler->onUpdateRecord( context, obj, newobj,
                                                 recordID, &recordRW, cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Process update record[%s] to [%s] "
                             "in handler failed, rc: %d",
                             obj.toString().c_str(),
                             newobj.toString().c_str(),
                             rc ) ;
                     goto error ;
                  }
               }
               if ( SDB_OK == rc && newChg.isEmpty() )
               {
                  SDB_ASSERT( oldChg.isEmpty(),
                              "Old change must be empty" ) ;
                  goto done ;
               }
            }
            else
            {
               rc = modifier.modify ( obj, newobj ) ;
            }

            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to create modified record, rc: %d",
                        rc ) ;
               if ( pResult && pResult->isMaskEnabled( UTIL_RESULT_MASK_ID ) )
               {
                  pResult->setCurrentID( obj ) ;
               }
               goto error ;
            }
            else if ( !modifier.hasModified() )
            {
               goto done ;
            }

            textIdxNum = context->mbStat()->_textIdxNum ;
            if ( textIdxNum > 0 )
            {
               handler = getExtDataHandler() ;
               if ( handler )
               {
                  rc = handler->prepare( DMS_EXTOPR_TYPE_UPDATE,
                                         getSuName(),
                                         context->mb()->_collectionName, NULL,
                                         &obj, &newobj, cb ) ;
                  PD_RC_CHECK( rc, PDERROR, "External operation check failed, "
                               "rc: %d", rc ) ;
               }
            }

            if ( NULL != dpscb )
            {
               _clFullName( context->mb()->_collectionName, fullName,
                            sizeof(fullName) ) ;

               // reserved log-size
               rc = dpsUpdate2Record( fullName,
                                      oldMatch, oldChg, newMatch, newChg,
                                      oldShardingKey, newShardingKey,
                                      transID, preTransLsn, relatedLSN,
                                      pWriteMod, record ) ;

               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to build record:%d", rc ) ;
                  goto error ;
               }

               rc = dpscb->checkSyncControl( record.alignedLen(), cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Check sync control failed, rc: %d",
                            rc ) ;

               logRecSize = record.alignedLen() ;
               rc = pTransCB->reservedLogSpace( logRecSize, cb );
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to reserved log space(len:%u), "
                          "rc: %d", logRecSize, rc ) ;
                  logRecSize = 0 ;
                  goto error ;
               }
            }

            rc = _extentUpdatedRecord( context, extRW, recordRW,
                                       recordData, newobj, cb,
                                       dpscb ? pHandler : NULL,
                                       pResult ) ;
            if ( rc )
            {
               if ( pResult && pResult->isMaskEnabled( UTIL_RESULT_MASK_ID ) )
               {
                  /// current id
                  if ( pResult->getCurID().isEmpty() )
                  {
                     pResult->setCurrentID( BSONObj( recordData.data() ) ) ;
                  }
                  /// peer id
                  if ( pResult->getPeerID().isEmpty() &&
                       pResult->getPeerRID().isValid() )
                  {
                     BSONObj peerObj ;
                     if ( SDB_OK == fetch( context, pResult->getPeerRID(),
                                           peerObj, cb, FALSE ) )
                     {
                        pResult->setPeerID( peerObj ) ;
                     }
                  }
               }

               PD_LOG ( PDERROR, "Failed to update record from (%s) to (%s), "
                        "rc: %d", obj.toString().c_str(),
                        newobj.toString().c_str(), rc ) ;
               goto error ;
            }

            if ( NULL != newRecord )
            {
               *newRecord = newobj.getOwned() ;
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         // increase update counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_UPDATE, 1 ) ;
         _incWriteRecord() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // log update information
      if ( dpscb )
      {
         PD_LOG ( PDDEBUG, "oldChange: %s,%s\nnewChange: %s,%s",
                  oldMatch.toString().c_str(),
                  oldChg.toString().c_str(),
                  newMatch.toString().c_str(),
                  newChg.toString().c_str() ) ;

         PD_AUDIT_OP_WITHNAME( AUDIT_UPDATE, "UPDATE", AUDIT_OBJ_CL,
                               fullName, rc, "OldMatch:%s, OldChange:%s, "
                               "NewMatch:%s, NewChange:%s",
                               oldMatch.toString().c_str(),
                               oldChg.toString().c_str(),
                               newMatch.toString().c_str(),
                               newChg.toString().c_str() ) ;

         if ( isTransSupport() )
         {
            info.enableTrans() ;
         }
         rc = _logDPS( dpscb, info, cb, context, pExtent->_logicID, FALSE,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert update record into log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
      }

      if ( handler )
      {
         handler->done( DMS_EXTOPR_TYPE_UPDATE, cb ) ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb );
      }
      if ( pResult )
      {
         if ( SDB_OK == rc )
         {
            pResult->incUpdatedNum() ;
            if ( modifier.hasModified() )
            {
               pResult->incModifiedNum() ;
            }
         }
         else
         {
            BSONElement errEle = modifier.getErrorElement() ;
            if ( !errEle.eoo() )
            {
               pResult->setCurrentField( errEle ) ;
            }
         }
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_UPDATERECORD, rc ) ;
      return rc ;
   error :
      if ( handler )
      {
         handler->abortOperation( DMS_EXTOPR_TYPE_UPDATE, cb ) ;
      }
      goto done ;
   }

   INT32 _dmsStorageDataCommon::popRecord( dmsMBContext *context,
                                           INT64 targetID,
                                           pmdEDUCB *cb,
                                           SDB_DPSCB *dpscb,
                                           INT8 direction,
                                           BOOLEAN byNumber )
   {
      INT32 rc = SDB_OK ;

      rc = _operationPermChk( DMS_ACCESS_TYPE_POP ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed in permission check of pop, rc: %d",
                   rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_FETCH, "_dmsStorageDataCommon::fetch" )
   INT32 _dmsStorageDataCommon::fetch( dmsMBContext *context,
                                       const dmsRecordID &recordID,
                                       BSONObj &dataRecord,
                                       pmdEDUCB * cb,
                                       BOOLEAN dataOwned )
   {
      INT32 rc                     = SDB_OK ;
      dmsRecordData recordData ;
      dmsExtRW extRW ;
      dmsRecordRW recordRW ;
      const dmsExtent *pExtent     = NULL ;
      const dmsRecord *pRecord     = NULL ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_FETCH ) ;

      if ( !context->isMBLock() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Caller must hold mb lock[%s]",
                 context->toString().c_str() ) ;
         goto error ;
      }

      try
      {
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_FETCH ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }

         extRW = extent2RW( recordID._extent, context->mbID() ) ;
         recordRW = record2RW( recordID, context->mbID() ) ;
         pExtent = extRW.readPtr<dmsExtent>() ;

         // validate extent
         if ( !pExtent->validate( context->mbID()) )
         {
            PD_LOG ( PDERROR, "Invalid extent[%d]", recordID._extent ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         pRecord = recordRW.readPtr() ;

         // make sure the record is not deleted
         // since we get the RID outside the latch scope, so there could be
         // inconsistency happen, we need to make sure the flag is expected
         if ( pRecord->isDeleted() )
         {
            rc = SDB_DMS_NOTEXIST ;
            goto error ;
         }
         else if ( pRecord->isDeleting() )
         {
            rc = SDB_DMS_DELETING ;
            goto error ;
         }

#ifdef _DEBUG
         // but the status shouldn't be in other flag
         if ( !pRecord->isNormal() && !pRecord->isOvf() )
         {
            PD_LOG ( PDERROR, "Record[%d:%d] flag[%d] error", recordID._extent,
                     recordID._offset, pRecord->getState() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
#endif //_DEBUG

         // if this record is overflow from
         rc = extractData( context, recordRW, cb, recordData ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract record data failed, rc: %d", rc ) ;

         if ( dataOwned )
         {
            dataRecord = BSONObj( recordData.data() ).getOwned() ;
         }
         else
         {
            dataRecord = BSONObj( recordData.data() ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_FETCH, rc ) ;
      return rc ;
   error :
      goto done ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_SETCOMPRESSOR, "_dmsStorageDataCommon::_setCompressor" )
   void _dmsStorageDataCommon::_setCompressor( dmsMBContext *context  )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_SETCOMPRESSOR ) ;

      UINT16 mbID = context->mbID() ;

      if ( OSS_BIT_TEST( context->mb()->_attributes,
                         DMS_MB_ATTR_COMPRESSED ) )
      {
         UTIL_COMPRESSOR_TYPE type =
            (UTIL_COMPRESSOR_TYPE)context->mb()->_compressorType ;

         /// only set when snappy
         if ( UTIL_COMPRESSOR_SNAPPY == type )
         {
            dmsCompressorGuard guard( &_compressorEntry[ mbID ], EXCLUSIVE ) ;
            _compressorEntry[mbID].setCompressor( getCompressorByType( type ) ) ;
         }
      }

      _compressorEntry[mbID].setFlags( context->mb()->_compressFlags ) ;

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_SETCOMPRESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_RMCOMPRESSOR, "_dmsStorageDataCommon::_rmCompressor" )
   void _dmsStorageDataCommon::_rmCompressor( _dmsMBContext *context )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_RMCOMPRESSOR ) ;
      dmsCompressorGuard compGuard( &_compressorEntry[context->mbID()],
                                    EXCLUSIVE ) ;
      utilDictHandle dictAddr =
         _compressorEntry[ context->mbID() ].getDictionary() ;

      if ( UTIL_INVALID_DICT != dictAddr )
      {
         endFixedAddr( (const ossValuePtr)dictAddr -
                       DMS_DICTEXTENT_HEADER_SZ ) ;
      }
      _compressorEntry[context->mbID()].reset() ;
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_RMCOMPRESSOR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_LOADDICTIONARY, "_dmsStorageDataCommon::loadDictionary" )
   INT32 _dmsStorageDataCommon::loadDictionary( dmsMBContext *context,
                                                const CHAR *dictionary,
                                                UINT32 dictLen, BOOLEAN force )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_LOADDICTIONARY ) ;

      dmsExtRW extRW ;
      dmsMB *mb = context->mb() ;
      dmsExtentID dictExtID = DMS_INVALID_EXTENT ;
      dmsDictExtent *dictExtent = NULL ;
      dmsCompressorEntry *compressorEntry =
         &_compressorEntry[ context->mbID() ] ;

      SDB_ASSERT( context, "MB context is NULL" ) ;
      SDB_ASSERT( dictionary && dictLen > 0, "Dictionary is NULL" ) ;

      // Calculate number of pages to store the dictionary, including the extent
      // header.
      UINT32 pageNum = ( sizeof( dmsDictExtent ) + dictLen +
                         ( pageSize() - 1 ) ) / pageSize() ;

      if ( !context->isMBLock( EXCLUSIVE ) )
      {
         PD_LOG( PDERROR, "MB context must be locked in EXCLUSIVE mode" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !dmsAccessAndFlagCompatiblity( mb->_flag,
                                          DMS_ACCESS_TYPE_CRT_DICT ) )
      {
         PD_LOG( PDERROR, "Incompatible collection mode: %d", mb->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      if ( !OSS_BIT_TEST( mb->_attributes, DMS_MB_ATTR_COMPRESSED ) )
      {
         PD_LOG( PDERROR, "Compression is not enabled for collection[%s]",
                 mb->_collectionName ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      if ( UTIL_COMPRESSOR_LZW != mb->_compressorType )
      {
         PD_LOG( PDERROR, "Compression type of collection[%s] is not lzw",
                 mb->_collectionName ) ;
         rc = SDB_OPERATION_INCOMPATIBLE ;
         goto error ;
      }

      if ( DMS_INVALID_EXTENT != mb->_dictExtentID && !force )
      {
         PD_LOG( PDERROR, "Collection[%s] has valid compression dictionary and "
                 "force load is false", mb->_collectionName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _findFreeSpace( pageNum, dictExtID, context ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to allocate space for dictionary "
                   "extent" ) ;

      extRW = extent2RW( dictExtID, context->mbID() ) ;
      extRW.setNothrow( TRUE ) ;
      dictExtent = extRW.writePtr<dmsDictExtent>( 0,
                                                  pageNum <<
                                                  pageSizeSquareRoot() ) ;
      if( !dictExtent )
      {
         PD_LOG( PDERROR, "Get the dict extent[%d] address failed",
                 dictExtent ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // Copy dictionary into extent, and flush to disk.
      dictExtent->init( pageNum, context->mbID() ) ;
      dictExtent->setDict( dictionary, dictLen ) ;
      for ( INT32 i = 0; i < 3; i++ )
      {
         rc = flushPages( dictExtID, pageNum, TRUE ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to flush dictionary. It will be "
                   "created and flushed again next time" ) ;

      // Release the old dictionary in force mode.
      if ( DMS_INVALID_EXTENT != mb->_dictExtentID )
      {
         dmsExtRW oldDictRW ;
         dmsDictExtent *dictExt = NULL ;
         // Remove the current compressor and release the dictionary.
         _rmCompressor( context ) ;
         oldDictRW = extent2RW( mb->_dictExtentID, context->mbID() ) ;
         dictExt = oldDictRW.writePtr<dmsDictExtent>() ;
         _releaseSpace( mb->_dictExtentID, dictExt->_blockSize ) ;
      }

      // Set the dictionary extent id in mb only after the dictionary has been
      // successfully flushed to disk.
      mb->_dictExtentID = dictExtID ;
      mb->_dictVersion = UTIL_LZW_DICT_VERSION ;

      /// Make sure the dict persist
      flushMME( isSyncDeep() ) ;

      {
         UTIL_COMPRESSOR_TYPE type  =
               (UTIL_COMPRESSOR_TYPE)mb->_compressorType ;
         dmsCompressorGuard guard( compressorEntry, EXCLUSIVE ) ;
         compressorEntry->setCompressor( getCompressorByType( type ) ) ;
         compressorEntry->setDictionary(
            (const utilDictHandle)( beginFixedAddr( dictExtID, pageNum ) +
                                    DMS_DICTEXTENT_HEADER_SZ ) ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON_LOADDICTIONARY, rc ) ;
      return rc ;
   error:
      if ( DMS_INVALID_EXTENT != dictExtID )
      {
         _freeExtent( dictExtID, context->mbID() ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_GETDICTIONARY, "_dmsStorageDataCommon::getDictionary" )
   BOOLEAN _dmsStorageDataCommon::getDictionary( dmsMBContext *context,
                                                 const CHAR *&dictionary,
                                                 UINT32 &dictLen )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_GETDICTIONARY ) ;
      BOOLEAN found = FALSE ;
      dmsMB *mb = NULL ;

      if ( !context->isMBLock( SHARED ) )
      {
         PD_LOG( PDERROR, "MB context must be locked in SHARED mode" ) ;
         goto error ;
      }

      mb = context->mb() ;
      dictionary = NULL ;
      dictLen = 0 ;

      if ( DMS_INVALID_EXTENT != mb->_dictExtentID )
      {
         dmsExtRW dictRW = extent2RW( mb->_dictExtentID, context->mbID() ) ;
         const dmsDictExtent *dictExt = dictRW.readPtr<dmsDictExtent>() ;
         dictionary = (CHAR *)dictExt + DMS_DICTEXTENT_HEADER_SZ ;
         dictLen = ( dictExt->_blockSize << pageSizeSquareRoot() ) -
                   DMS_DICTEXTENT_HEADER_SZ ;
         found = TRUE ;
      }

   done:
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_GETDICTIONARY ) ;
      return found ;
   error:
      goto done ;
   }

   UINT32 _dmsStorageDataCommon::_getRecordDataLen( const dmsRecord *pRecord )
   {
      /// if ovf, need to get the ovt's data
      if ( pRecord->isOvf() )
      {
         dmsRecordID ovfRID = pRecord->getOvfRID() ;
         dmsRecordRW ovfRW = record2RW( ovfRID, -1 ) ;
         pRecord = ovfRW.readPtr() ;
      }
      return pRecord->getDataLength() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__INCMBSTAT, "_dmsStorageDataCommon::_increaseMBStat" )
   void _dmsStorageDataCommon::_increaseMBStat ( utilCLUniqueID clUniqueID,
                                                 dmsMBStatInfo * mbStat,
                                                 _pmdEDUCB * cb )
   {
      SDB_ASSERT( NULL != mbStat, "mb stat should not be NULL" ) ;
      SDB_ASSERT( NULL != cb, "EDUCB should not be NULL" ) ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__INCMBSTAT ) ;

      // update meta-block statistics
      ++ ( mbStat->_totalRecords ) ;

      // update meta-block statistics for transaction
      if ( cb->isInTransRollback() )
      {
         // do nothing
      }
      else if ( cb->isTransaction() )
      {
         if ( !cb->getTransExecutor()->incMBTotalRecords(
                           clUniqueID, &( mbStat->_rcTotalRecords ), 1 ) )
         {
            mbStat->_rcTotalRecords.inc() ;
         }
      }
      else
      {
         mbStat->_rcTotalRecords.inc() ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON__INCMBSTAT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__DECMBSTAT, "_dmsStorageDataCommon::_decreaseMBStat" )
   void _dmsStorageDataCommon::_decreaseMBStat ( utilCLUniqueID clUniqueID,
                                                 dmsMBStatInfo * mbStat,
                                                 _pmdEDUCB * cb )
   {
      SDB_ASSERT( NULL != mbStat, "mb stat should not be NULL" ) ;
      SDB_ASSERT( NULL != cb, "EDUCB should not be NULL" ) ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__DECMBSTAT ) ;

      // update meta-block statistics
      -- ( mbStat->_totalRecords ) ;

      // update meta-block statistics for transaction
      if ( cb->isInTransRollback() )
      {
         // do nothing
      }
      else if ( cb->isTransaction() )
      {
         if ( !cb->getTransExecutor()->decMBTotalRecords(
                        clUniqueID, &( mbStat->_rcTotalRecords ), 1 ) )
         {
            mbStat->_rcTotalRecords.dec() ;
         }
      }
      else
      {
         mbStat->_rcTotalRecords.dec() ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON__DECMBSTAT ) ;
   }

   /*
      Tool Fuctions
   */
   BOOLEAN dmsIsKeyNameValid( const BSONObj &obj,
                              const CHAR **pErrStr )
   {
      const CHAR *pTmpStr = NULL ;
      BOOLEAN valid = TRUE ;

      BSONObjIterator itr( obj ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;

         if ( '$' == e.fieldName()[ 0 ] )
         {
            pTmpStr = "field name can't start with \'$\'" ;
            valid = FALSE ;
            break ;
         }
         else if ( ossStrchr( e.fieldName(), '.' ) )
         {
            pTmpStr = "field name can't include \'.\'" ;
            valid = FALSE ;
            break ;
         }
         else if ( e.isABSONObj() &&
                   !dmsIsKeyNameValid( e.embeddedObject(), pErrStr ) )
         {
            valid = FALSE ;
            break ;
         }
      }

      if ( !valid && pErrStr && pTmpStr )
      {
         *pErrStr = pTmpStr ;
      }

      return valid ;
   }

   BOOLEAN dmsIsRecordIDValid( const BSONElement &oidEle,
                               BOOLEAN allowEOO,
                               const CHAR **pErrStr )
   {
      const CHAR *pTmpStr = NULL ;
      BOOLEAN valid = TRUE ;

      switch ( oidEle.type() )
      {
         case EOO :
            if ( !allowEOO )
            {
               pTmpStr = "is not exist" ;
               valid = FALSE ;
            }
            break ;
         case Array :
            pTmpStr = "can't be Array" ;
            valid = FALSE ;
            break ;
         case RegEx:
            pTmpStr = "can't be RegEx" ;
            valid = FALSE ;
            break ;
         case Object :
            valid = dmsIsKeyNameValid( oidEle.embeddedObject(), pErrStr ) ;
            break ;
         default :
            break ;
      }

      if ( !valid && pErrStr && pTmpStr )
      {
         *pErrStr = pTmpStr ;
      }

      return valid ;
   }

}

