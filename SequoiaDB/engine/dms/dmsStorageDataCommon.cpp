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
#include "dpsUtil.hpp"
#include "pdSecure.hpp"

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
   #define DMS_MB_ATTR_NOTRANS_STR                           "NoTrans"

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
      if ( OSS_BIT_TEST( attributes, DMS_MB_ATTR_NOTRANS ) )
      {
         appendFlagString( pBuffer, bufSize, DMS_MB_ATTR_NOTRANS_STR ) ;
         OSS_BIT_CLEAR( attributes, DMS_MB_ATTR_NOTRANS ) ;
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
      _pSubContext    = NULL ;
      _collPtr.reset() ;
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

   void _dmsMBContext::setSubContext( _IContext *subContext )
   {
      _pSubContext = subContext ;
   }

   void _dmsMBContext::swap( _dmsMBContext &other )
   {
      _dmsMBContext temp ;

      temp._mb             = other._mb ;
      temp._mbStat         = other._mbStat ;
      temp._latch          = other._latch ;
      temp._clLID          = other._clLID ;
      temp._startLID       = other._startLID ;
      temp._mbID           = other._mbID ;
      temp._mbLockType     = other._mbLockType ;
      temp._resumeType     = other._resumeType ;
      temp._pSubContext    = other._pSubContext ;
      temp._collPtr.swap( other._collPtr ) ;

      other._mb            = _mb ;
      other._mbStat        = _mbStat ;
      other._latch         = _latch ;
      other._clLID         = _clLID ;
      other._startLID      = _startLID ;
      other._mbID          = _mbID ;
      other._mbLockType    = _mbLockType ;
      other._resumeType    = _resumeType ;
      other._pSubContext   = _pSubContext ;
      other._collPtr.swap( _collPtr ) ;

      _mb                  = temp._mb ;
      _mbStat              = temp._mbStat ;
      _latch               = temp._latch ;
      _clLID               = temp._clLID ;
      _startLID            = temp._startLID ;
      _mbID                = temp._mbID ;
      _mbLockType          = temp._mbLockType ;
      _resumeType          = temp._resumeType ;
      _pSubContext         = temp._pSubContext ;
      _collPtr.swap( temp._collPtr ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSMBCONTEXT_PAUSE, "_dmsMBContext::pause" )
   INT32 _dmsMBContext::pause()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isSubPaused = FALSE ;
      PD_TRACE_ENTRY ( SDB__DMSMBCONTEXT_PAUSE ) ;

      if ( NULL != _pSubContext )
      {
         rc = _pSubContext->pause() ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to pause subContext, rc: %d",
                      rc ) ;
         isSubPaused = TRUE ;
      }

      _resumeType = _mbLockType ;
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         rc = mbUnlock() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to pause mb lock, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSMBCONTEXT_PAUSE, rc ) ;
      return rc ;
   error:
      if ( isSubPaused )
      {
         INT32 rcTmp = _pSubContext->resume() ;
         SDB_ASSERT( SDB_OK == rcTmp, "Must resume success" ) ;
         isSubPaused = FALSE ;
      }

      goto done ;
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
         PD_RC_CHECK( rc, PDERROR, "Failed to resume mb lock, rc: %d", rc ) ;
      }

      if ( NULL != _pSubContext )
      {
         rc = _pSubContext->resume() ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to resume sub context, rc: %d",
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__DMSMBCONTEXT_RESUME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _dmsMBContextSubScope implement
   */
   _dmsMBContextSubScope::_dmsMBContextSubScope( _dmsMBContext* mbContext,
                                                 _IContext *subContext )
   {
      _mbContext = mbContext ;
      _mbContext->setSubContext( subContext ) ;
   }

   _dmsMBContextSubScope::~_dmsMBContextSubScope()
   {
      _mbContext->setSubContext( NULL ) ;
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

   _dmsRecordRW::_dmsRecordRW( const _dmsRecordRW &recordRW )
   : _isDirectMem( recordRW._isDirectMem ),
     _ptr( recordRW._ptr ),
     _rid( recordRW._rid._extent, recordRW._rid._offset ),
     _rw( recordRW._rw ),
     _pData( recordRW._pData )
   {
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
   _dmsStorageDataCommon::_dmsStorageDataCommon ( IStorageService *service,
                                                  dmsSUDescriptor *suDescriptor,
                                                  const CHAR *pSuFileName,
                                                  _IDmsEventHolder *pEventHolder )
   :_dmsStorageBase( service, suDescriptor, pSuFileName ),
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
                 _mbStatInfo[i]._totalRecords.fetch() )
            {
               _dmsMME->_mbList[i]._totalRecords =
                  _mbStatInfo[i]._totalRecords.fetch() ;
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
                 _mbStatInfo[i]._totalLobPages.fetch() )
            {
               _dmsMME->_mbList[i]._totalLobPages =
                  _mbStatInfo[i]._totalLobPages.fetch() ;
            }
            if ( _dmsMME->_mbList[i]._totalLobs !=
                 _mbStatInfo[i]._totalLobs.fetch() )
            {
               _dmsMME->_mbList[i]._totalLobs =
                  _mbStatInfo[i]._totalLobs.fetch() ;
            }
            if ( _dmsMME->_mbList[i]._lastCompressRatio !=
                 _mbStatInfo[i]._lastCompressRatio )
            {
               _dmsMME->_mbList[i]._lastCompressRatio =
                  _mbStatInfo[i]._lastCompressRatio ;
            }
            if ( _dmsMME->_mbList[i]._totalDataLen !=
                 _mbStatInfo[i]._totalDataLen.fetch() )
            {
               _dmsMME->_mbList[i]._totalDataLen =
                  _mbStatInfo[i]._totalDataLen.fetch() ;
            }
            if ( _dmsMME->_mbList[i]._totalLobSize !=
                 _mbStatInfo[i]._totalLobSize.fetch() )
            {
               _dmsMME->_mbList[i]._totalLobSize =
                 _mbStatInfo[i]._totalLobSize.fetch() ;
            }
            if ( _dmsMME->_mbList[i]._totalValidLobSize !=
                 _mbStatInfo[i]._totalValidLobSize.fetch() )
            {
               _dmsMME->_mbList[i]._totalValidLobSize =
                 _mbStatInfo[i]._totalValidLobSize.fetch() ;
            }
            if ( _dmsMME->_mbList[i]._totalOrgDataLen !=
                 _mbStatInfo[i]._totalOrgDataLen.fetch() )
            {
               _dmsMME->_mbList[i]._totalOrgDataLen =
                  _mbStatInfo[i]._totalOrgDataLen.fetch() ;
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
            if ( _dmsMME->_mbList[i]._ridGen !=
                   _mbStatInfo[i]._ridGen.peek() )
            {
               _dmsMME->_mbList[i]._ridGen =
                  _mbStatInfo[i]._ridGen.peek() ;
            }
         }
      }
      PD_TRACE_EXIT ( SDB__DMSSTORAGEDATACOMMON_SYNCMEMTOMMAP ) ;
   }

   BOOLEAN _dmsStorageDataCommon::isTransSupport( dmsMBContext *context ) const
   {
      if ( DMS_STORAGE_CAPPED == getStorageType() || !_transSupport )
      {
         return FALSE ;
      }
      else if ( ( NULL != context ) &&
                ( OSS_BIT_TEST( context->mb()->_attributes,
                                DMS_MB_ATTR_NOTRANS ) ) )
      {
         return FALSE ;
      }
      return TRUE ;
   }

   BOOLEAN _dmsStorageDataCommon::isTransLockRequired( dmsMBContext *context ) const
   {
      if ( DMS_STORAGE_CAPPED == getStorageType() )
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

   UINT32 _dmsStorageDataCommon::_maxSupportedVersion() const
   {
      return DMS_DATASU_MAX_VERSION ;
   }

   INT32 _dmsStorageDataCommon::_checkVersion( dmsStorageUnitHeader * pHeader )
   {
      INT32 rc = SDB_OK ;

      if ( pHeader->_version > _maxSupportedVersion() )
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

            _mbStatInfo[i]._totalRecords.init( _dmsMME->_mbList[i]._totalRecords ) ;
            _mbStatInfo[i]._rcTotalRecords.init( _dmsMME->_mbList[i]._totalRecords ) ;
            _mbStatInfo[i]._totalDataPages =
               _dmsMME->_mbList[i]._totalDataPages ;
            _mbStatInfo[i]._totalIndexPages =
               _dmsMME->_mbList[i]._totalIndexPages ;
            _mbStatInfo[i]._totalDataFreeSpace =
               _dmsMME->_mbList[i]._totalDataFreeSpace ;
            _mbStatInfo[i]._totalIndexFreeSpace =
               _dmsMME->_mbList[i]._totalIndexFreeSpace ;
            _mbStatInfo[ i ]._totalLobPages.init( _dmsMME->_mbList[ i ]._totalLobPages ) ;
            _mbStatInfo[ i ]._totalLobs.init( _dmsMME->_mbList[ i ]._totalLobs ) ;
            _mbStatInfo[i]._lastCompressRatio =
               _dmsMME->_mbList[i]._lastCompressRatio ;
            _mbStatInfo[i]._totalDataLen.init( _dmsMME->_mbList[i]._totalDataLen ) ;
            _mbStatInfo[ i ]._totalLobSize.init( _dmsMME->_mbList[ i ]._totalLobSize ) ;
            _mbStatInfo[ i ]._totalValidLobSize.init( _dmsMME->_mbList[ i ]._totalValidLobSize ) ;
            _mbStatInfo[i]._totalOrgDataLen.init( _dmsMME->_mbList[i]._totalOrgDataLen ) ;
            _mbStatInfo[i]._startLID =
               _dmsMME->_mbList[i]._logicalID ;

            _mbStatInfo[i]._createTime = _dmsMME->_mbList[i]._createTime ;
            _mbStatInfo[i]._updateTime = _dmsMME->_mbList[i]._updateTime ;
            _mbStatInfo[i]._ridGen.init( _dmsMME->_mbList[i]._ridGen ) ;

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
                  /// upgrade from the old version which has no
                  /// _commitLSN/_idxCommitLSN/_lobCommitLSN in mb block,
                  /// so the value of _commitLSN/_idxCommitLSN/_lobCommitLSN is 0
                  if ( 0 == _dmsMME->_mbList[i]._commitLSN )
                  {
                     _dmsMME->_mbList[i]._commitLSN =
                        _suDescriptor->getStorageInfo()._curLSNOnStart ;
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
         if ( DMS_IS_MB_INUSE ( _dmsMME->_mbList[i]._flag ) )
         {
            if ( _service )
            {
               dmsCLMetadata metadata( _suDescriptor,
                                       &( _dmsMME->_mbList[ i ] ),
                                       &( _mbStatInfo[ i ] ) ) ;
               std::shared_ptr<ICollection> collPtr ;
               _service->loadCollection( metadata, pmdGetThreadEDUCB(), collPtr ) ;
            }
         }
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__ONOPENED, rc ) ;

      return rc ;
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
                                                    UINT64 lastTime,
                                                    BOOLEAN &setHeadCommFlgValid )
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

               if ( _mbStatInfo[i]._writePtrCount.fetch() > 0 && !isClosed() )
               {
                  // Don't set _dmsMME->_mbList[i]._commitFlag to 1
                  // Don't set header commitFlag to 1
                  // Because the current write op has not completed( _writePtrCount > 0 )
                  setHeadCommFlgValid = FALSE ;
               }
               else
               {
                  _dmsMME->_mbList[i]._commitFlag = tmpCommitFlag ;
               }
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

   void _dmsStorageDataCommon::incWritePtrCount( INT32 collectionID )
   {
      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         _mbStatInfo[ collectionID ]._writePtrCount.inc() ;
      }
   }

   void _dmsStorageDataCommon::decWritePtrCount( INT32 collectionID )
   {
      if ( collectionID >= 0 && collectionID < DMS_MME_SLOTS )
      {
         _mbStatInfo[ collectionID ]._writePtrCount.dec() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMM_EXTRACTDATA, "_dmsStorageDataCommon::extractData" )
   INT32 _dmsStorageDataCommon::extractData( const dmsMBContext *mbContext,
                                             const dmsRecordID &recordID,
                                             _pmdEDUCB *cb,
                                             dmsRecordData &recordData,
                                             BOOLEAN needIncDataRead,
                                             BOOLEAN needGetOwned )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMM_EXTRACTDATA ) ;

      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      recordData.reset() ;

      if ( !mbContext->isMBLock() )
      {
         PD_LOG( PDERROR, "MB Context must be locked" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = mbContext->getCollPtr()->extractRecord( recordID, recordData, needGetOwned, cb ) ;
      if ( SDB_DMS_RECORD_NOTEXIST == rc )
      {
         PD_LOG( PDDEBUG, "Failed to extract record from "
                 "collection [%s.%s] with record ID "
                 "[ extent: %u, offset: %u ], rc: %d",
                 _suDescriptor->getSUName(), mbContext->clName(),
                 recordID._extent, recordID._offset, rc ) ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to extract record from "
                   "collection [%s.%s] with record ID "
                   "[ extent: %u, offset: %u ], rc: %d",
                   _suDescriptor->getSUName(), mbContext->clName(),
                   recordID._extent, recordID._offset, rc ) ;

      if( needIncDataRead )
      {
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_READ, 1 ) ;
      }

      DMS_MON_OP_COUNT_INC( pMonAppCB, MON_READ, 1 ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMM_EXTRACTDATA, rc ) ;
      return rc ;

   error:
      goto done ;
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
                                         dmsExtentID extID,
                                         dmsOffset extOffset )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__LOGDPS ) ;
      info.setInfoEx( _logicalCSID, clLID, extID, extOffset, cb ) ;
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
                                         dmsExtentID extID, dmsOffset extOffset,
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
                      extID, extOffset, cb ) ;
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
      dmsExtentID firstFreeExtentID = DMS_INVALID_EXTENT ;
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

      SDB_ASSERT( context, "dms mb context can't be NULL" ) ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON__TRUNCATECOLLECTION ) ;
      rc = context->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      // reset delete list
      for ( UINT32 i = 0 ; i < dmsMB::_max ; i++ )
      {
         context->mb()->_deleteList[i].reset() ;
      }

      context->mb()->_firstExtentID = DMS_INVALID_EXTENT ;
      context->mb()->_lastExtentID = DMS_INVALID_EXTENT ;

      context->mb()->_ridGen = 0 ;
      context->mbStat()->_ridGen.poke( 0 ) ;

      context->mbStat()->_totalDataFreeSpace = 0 ;
      context->mbStat()->_totalDataPages = 0 ;
      context->mbStat()->_totalRecords.init( 0 ) ;
      context->mbStat()->_rcTotalRecords.init( 0 ) ;
      context->mbStat()->_totalDataLen.init( 0 ) ;
      context->mbStat()->_totalOrgDataLen.init( 0 ) ;
      context->mbStat()->_blockIndexCreatingCount = 0 ;
      context->mbStat()->_lastSearchSlot = dmsMB::_max ;
      context->mbStat()->_lastSearchRID.reset() ;

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
            if ( prevExt )
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
                                               const BSONObj *extOptions,
                                               const BSONObj *pIdIdxDef,
                                               BOOLEAN addIdxIDIfNotExist )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_ADDCOLLECTION ) ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;
      UINT32 logRecSize       = 0 ;
      dpsTransCB *pTransCB    = pmdGetKRCB()->getTransCB() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      UINT16 newCollectionID  = DMS_INVALID_MBID ;
      UINT32 logicalID        = DMS_INVALID_CLID ;
      BOOLEAN metalocked      = FALSE ;
      dmsMB *mb               = NULL ;
      dmsMBStatInfo *mbStat   = NULL ;
      SDB_DPSCB *dropDps      = NULL ;
      dmsMBContext *context   = NULL ;

      INT32 testTransLockRC   = SDB_OK ;
      dmsCreateCLOptions options ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      // check cl name
      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      _clFullName( pName, fullName, sizeof(fullName) ) ;

      // fix uniqueID
      if ( utilGetCSUniqueID( clUniqueID ) != _suDescriptor->getCSUniqueID() )
      {
         clUniqueID = utilBuildCLUniqueID( _suDescriptor->getCSUniqueID(),
                                           utilGetCLInnerID( clUniqueID ) ) ;
      }
      if ( !UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         ossLatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = TRUE ;

         if ( (utilCLInnerID)( ossAtomicFetch32( &_dmsHeader->_clInnderHWM ) ) >= UTIL_CLINNERID_MAX )
         {
            PD_LOG( PDWARNING, "Failed to allocate collection unique ID" ) ;
            rc = SDB_CAT_CL_UNIQUEID_EXCEEDED ;
            goto error ;
         }

         // start from 1
         utilCLInnerID innerID =
               (utilCLInnerID)(
                  ossFetchAndIncrement32( &( _dmsHeader->_clInnderHWM ) ) + 1 ) ;

         if ( innerID > UTIL_CLINNERID_MAX )
         {
            PD_LOG( PDWARNING, "Failed to allocate collection unique ID" ) ;
            rc = SDB_CAT_CL_UNIQUEID_EXCEEDED ;
            goto error ;
         }

         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;

         OSS_BIT_SET( innerID, UTIL_UNIQUEID_LOCAL_BIT ) ;
         clUniqueID = utilBuildCLUniqueID( _suDescriptor->getCSUniqueID(), innerID ) ;
      }

      // calc the reserve dps size
      if ( dpscb )
      {
         rc = dpsCLCrt2Record( fullName, clUniqueID, attributes,
                               compressionType, extOptions, pIdIdxDef, record ) ;
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

      rc = _prepareAddCollection( extOptions, options ) ;
      PD_RC_CHECK( rc, PDERROR, "onAddCollection operation failed: %d", rc ) ;

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
            // trans lock
            if ( cb && cb->getTransExecutor()->useTransLock() )
            {
               dpsTransRetInfo lockConflict ;
               // NOTE: acquired meta lock and su lock,
               // no need to test upper lock
               testTransLockRC = pTransCB->transLockTestX( cb,
                                                           _logicalCSID,
                                                           i,
                                                           NULL,
                                                           &lockConflict,
                                                           NULL,
                                                           FALSE ) ;
               if ( SDB_OK != testTransLockRC )
               {
                  PD_LOG( PDDEBUG,
                          "Failed to test X lock on collection slot, "
                          "rc: %d" OSS_NEWLINE
                          "Conflict( representative ):" OSS_NEWLINE
                          "   EDUID:  %llu" OSS_NEWLINE
                          "   TID:    %u" OSS_NEWLINE
                          "   LockId: %s" OSS_NEWLINE
                          "   Mode:   %s" OSS_NEWLINE,
                          testTransLockRC,
                          lockConflict._eduID,
                          lockConflict._tid,
                          lockConflict._lockID.toString().c_str(),
                          lockModeToString( lockConflict._lockType ) ) ;
                  continue ;
               }
            }
            newCollectionID = i ;
            break ;
         }
      }
      // make sure we find free collection id
      if ( DMS_INVALID_MBID == newCollectionID )
      {
         if ( SDB_OK != testTransLockRC )
         {
            PD_LOG( PDERROR, "Failed to test transaction on free slots, "
                    "rc: %d", testTransLockRC ) ;
            rc = testTransLockRC ;
         }
         else
         {
           PD_LOG ( PDERROR, "Unable to find free collection id" ) ;
           rc = SDB_SYS ;
         }
         goto error ;
      }

      rc = _collectionInsert( pName, newCollectionID, clUniqueID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG(PDERROR, "Failed to insert collectionID into map, rc:%d", rc ) ;
         newCollectionID = DMS_INVALID_MBID ;
         goto error ;
      }

      // set mb meta data and header data
      logicalID = ossFetchAndIncrement32( &( _dmsHeader->_MBHWM ) ) ;
      mb = &_dmsMME->_mbList[newCollectionID] ;
      mb->reset( pName, clUniqueID, newCollectionID, logicalID,
                 attributes, compressionType ) ;

      if ( OSS_BIT_TEST( attributes, DMS_MB_ATTR_CAPPED ) )
      {
         mb->_maxSize = options._cappedOptions._maxSize ;
         mb->_maxRecNum = options._cappedOptions._maxRecNum ;
         mb->_overwrite = options._cappedOptions._overwrite ;
      }

      mb->_createTime = ossGetCurrentMilliseconds() ;
      mb->_updateTime = mb->_createTime ;
      mbStat = &( _mbStatInfo[ newCollectionID ] ) ;
      mbStat->reset() ;
      mbStat->_startLID = logicalID ;
      mbStat->_createTime = mb->_createTime ;
      mbStat->_updateTime = mb->_updateTime ;
      mbStat->_ridGen.init( mb->_ridGen ) ;

      _dmsHeader->_numMB++ ;
      _onHeaderUpdated() ;

      if ( _service )
      {
         dmsCLMetadata metadata( _suDescriptor, mb, mbStat ) ;
         options._compressorType = compressionType ;
         rc = _service->createCL( metadata, options, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create collection [%s] on "
                      "engine [%s], rc: %d", pName,
                      dmsGetStorageEngineName( _service->getEngineType() ),
                      rc ) ;
      }

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
                       metalocked, logicalID, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLcrt record to log, "
                      "rc = %d", rc ) ;
      }
      else if ( NULL != cb )
      {
         cb->setDataExInfo( fullName, _logicalCSID, logicalID,
                            DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
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

      // create $id index[s_idKeyObj]
      if ( !OSS_BIT_TEST( attributes, DMS_MB_ATTR_NOIDINDEX ) )
      {
         rc = _pIdxSU->createIndex( context,
                                    pIdIdxDef ? *pIdIdxDef : ixmGetIDIndexDefine(),
                                    cb, NULL, TRUE,
                                    SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE,
                                    NULL, NULL, FALSE, addIdxIDIfNotExist ) ;
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
      if ( DMS_INVALID_MBID != newCollectionID )
      {
         // drop collection
         INT32 rc1 = dropCollection( pName, cb, dropDps, sysCollection,
                                     context ) ;
         if ( rc1 )
         {
            PD_LOG( PDSEVERE, "Failed to clean up bad collection creation[%s], "
                    "rc: %d", pName, rc1 ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_DROPCOLLECTION, "_dmsStorageDataCommon::dropCollection" )
   INT32 _dmsStorageDataCommon::dropCollection( const CHAR * pName, pmdEDUCB * cb,
                                                SDB_DPSCB * dpscb,
                                                BOOLEAN sysCollection,
                                                dmsMBContext * context,
                                                dmsDropCLOptions *options )
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

      dmsEventCLItem clItem ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

      _clFullName( pName, fullName, sizeof(fullName) ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         BSONObj *boOptions = NULL ;

         if ( NULL != options )
         {
            rc = options->prepareOptions() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to prepare drop "
                         "collection options, rc: %d", rc ) ;

            boOptions = &( options->_boOptions ) ;
         }

         rc = dpsCLDel2Record( fullName, boOptions, record ) ;
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

      clItem.init( pName, _logicalCSID, context->mbID(), context->clLID(),
                   context ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      // it is not need to lock that drop temp collection while startup
      // which cb is NULL
      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = pTransCB->transLockTryZ( cb, clItem._logicCSID, clItem._mbID,
                                       NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to lock the collection, rc: %d" OSS_NEWLINE
                      "Conflict( representative ):" OSS_NEWLINE
                      "   EDUID:  %llu" OSS_NEWLINE
                      "   TID:    %u" OSS_NEWLINE
                      "   LockId: %s" OSS_NEWLINE
                      "   Mode:   %s" OSS_NEWLINE,
                      rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;

         isTransLocked = TRUE ;
      }

      if ( _pEventHolder )
      {
         rc = _pEventHolder->onDropCL( DMS_EVENT_MASK_ALL, SDB_EVT_OCCUR_BEFORE,
                                       clItem, options, cb, dpscb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to call before drop "
                      "collection events, rc: %d", rc ) ;
      }

      if ( ( NULL == options ) ||
           ( !( options->isTakenOver() ) ) )
      {
         // drop all index
         rc = _pIdxSU->dropAllIndexes( context, cb, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to drop index for collection[%s], "
                      "rc: %d", pName, rc ) ;

         rc = _truncateCollection( context ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to truncate the collection[%s], rc: %d",
                      pName, rc ) ;

         // truncate lob
         if ( _pLobSU->isOpened() )
         {
            context->mbStat()->_totalLobPages.poke(0) ;
            context->mbStat()->_totalLobs.poke(0) ;
            context->mbStat()->resetTotalLobSize() ;
            context->mbStat()->resetTotalValidLobSize() ;
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

         // get unique id from mb. Because if the cl is in _collectionIDMap, and
         // we don't erase it, it may cause core dump.
         clUniqueID = context->mb()->_clUniqueID ;

         if ( _service )
         {
            dmsCLMetadata metadata( _suDescriptor,
                                    context->mb(),
                                    context->mbStat() ) ;
            dmsDropCLOptions tmpOptions ;
            if ( NULL == options )
            {
               options = &tmpOptions ;
            }
            INT32 tmpRC = _service->dropCL( metadata, *options, context, cb ) ;
            if ( SDB_OK != tmpRC )
            {
               PD_LOG( PDWARNING, "Failed to drop collection [%s] on engine [%s], "
                       "rc: %d", pName,
                       dmsGetStorageEngineName( _service->getEngineType() ),
                       tmpRC ) ;
            }
            if ( options == &tmpOptions )
            {
               options = NULL ;
            }
         }

         // release mb lock
         context->mbUnlock() ;

         // change metadata
         ossLatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = TRUE ;
         _collectionRemove( pName, clUniqueID ) ;
         DMS_SET_MB_FREE( context->mb()->_flag ) ;
         _dmsHeader->_numMB-- ;
         _onHeaderUpdated() ;
      }

      if ( _pEventHolder )
      {
         _pEventHolder->onDropCL( DMS_EVENT_MASK_ALL, SDB_EVT_OCCUR_AFTER,
                                  clItem, options, cb, dpscb ) ;
      }

      // write dps log
      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE, metalocked,
                       context->clLID(), DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert CLDel record to log, rc: "
                      "%d", rc ) ;
      }
      else if ( NULL != cb )
      {
         cb->setDataExInfo( fullName, _logicalCSID, context->clLID(),
                            DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
      }

   done:
      if ( metalocked )
      {
         ossUnlatch( &_metadataLatch, EXCLUSIVE ) ;
         metalocked = FALSE ;
      }
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, clItem._logicCSID, clItem._mbID ) ;
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
                                                    BOOLEAN truncateLob,
                                                    dmsTruncCLOptions *options )
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

      dmsEventCLItem clItem ;

      SDB_ASSERT( pName, "Collection name cat't be NULL" ) ;

      rc = dmsCheckCLName ( pName, sysCollection ) ;
      PD_RC_CHECK( rc, PDERROR, "Invalid collection name %s, rc: %d",
                   pName, rc ) ;

       _clFullName( pName, fullName, sizeof(fullName) ) ;

      // calc the reserve dps size
      if ( dpscb )
      {
         BSONObj *boOptions = NULL ;

         if ( NULL != options )
         {
            rc = options->prepareOptions() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to prepare truncate "
                         "collection options, rc: %d", rc ) ;

            boOptions = &( options->_boOptions ) ;
         }

         rc = dpsCLTrunc2Record( fullName, boOptions, record ) ;
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

      clItem.init( pName, _logicalCSID, context->mbID(), context->clLID(),
                   context ) ;

      if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                           DMS_ACCESS_TYPE_TRUNCATE ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  context->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }

      {
         dmsWriteGuard writeGuard( _service, this, context, cb, TRUE, FALSE, TRUE ) ;

         rc = writeGuard.begin() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to begin write guard, rc: %d", rc ) ;

         // trans lock
         if ( cb && cb->getTransExecutor()->useTransLock() )
         {
            dpsTransRetInfo lockConflict ;
            rc = pTransCB->transLockTryZ( cb, clItem._logicCSID, clItem._mbID,
                                          NULL, &lockConflict ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to lock the collection, rc: %d" OSS_NEWLINE
                         "Conflict( representative ):" OSS_NEWLINE
                         "   EDUID:  %llu" OSS_NEWLINE
                         "   TID:    %u" OSS_NEWLINE
                         "   LockId: %s" OSS_NEWLINE
                         "   Mode:   %s" OSS_NEWLINE,
                         rc,
                         lockConflict._eduID,
                         lockConflict._tid,
                         lockConflict._lockID.toString().c_str(),
                         lockModeToString( lockConflict._lockType ) ) ;

            isTransLocked = TRUE ;
         }

         if ( _pEventHolder )
         {
            rc = _pEventHolder->onTruncateCL( DMS_EVENT_MASK_ALL,
                                              SDB_EVT_OCCUR_BEFORE, clItem,
                                              options, cb, dpscb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to call before truncate "
                         "collection events, rc: %d", rc ) ;
         }

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

         if ( ( NULL == options ) ||
              ( !( options->isTakenOver() ) ) )
         {
            if ( needChangeCLID )
            {
               // use atomic increment to avoid lock on meta data
               newCLID = ossFetchAndIncrement32( &( _dmsHeader->_MBHWM ) ) ;
            }

            oldRecords = context->mbStat()->_totalRecords.fetch() ;
            oldLobs = context->mbStat()->_totalLobs.fetch() ;

            rc = _pIdxSU->truncateIndexes( context, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Truncate collection[%s] indexes failed, "
                         "rc: %d", pName, rc ) ;

            dmsTruncCLOptions tmpOptions ;
            if ( NULL == options )
            {
               options = &tmpOptions ;
            }
            rc = context->getCollPtr()->truncate( *options, cb ) ;
            if ( options == &tmpOptions )
            {
               options = NULL ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection [%s] data, "
                         "rc: %d", pName, rc ) ;

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
         }

         if ( handler )
         {
            rc = handler->done( DMS_EXTOPR_TYPE_TRUNCATE, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "External done operation failed, rc: %d",
                         rc ) ;
         }

         if ( _pEventHolder )
         {
            _pEventHolder->onTruncateCL( DMS_EVENT_MASK_ALL, SDB_EVT_OCCUR_AFTER,
                                         clItem, options, cb, dpscb ) ;
         }

         // write dps log
         if ( dpscb )
         {
            PD_AUDIT_OP_WITHNAME( AUDIT_DML, "TRUNCATE", AUDIT_OBJ_CL,
                                  fullName, rc, "RecordNum:%llu, LobNum:%llu",
                                  oldRecords, oldLobs ) ;
            rc = _logDPS( dpscb, info, cb, context, DMS_INVALID_EXTENT,
                          DMS_INVALID_OFFSET, FALSE, DMS_FILE_ALL, &oldCLID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to insert CLTrunc record to log, "
                         "rc: %d", rc ) ;
         }
         else if ( cb->getLsnCount() > 0 )
         {
            context->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_ALL ) ;
            cb->setDataExInfo( fullName, logicalID(), oldCLID,
                               DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
         }

         rc = writeGuard.commit() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDSEVERE, "Failed to commit write guard, rc: %d", rc ) ;
            ossPanic() ;
         }
      }

      if ( SDB_OK == context->mbLock( EXCLUSIVE ) )
      {
         dmsCompactCLOptions options ;
         context->getCollPtr()->compact( options, cb ) ;
      }

   done:
      if ( isTransLocked )
      {
         pTransCB->transLockRelease( cb, clItem._logicCSID, clItem._mbID ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_PREPARECOLLECTIONLOADS, "_dmsStorageDataCommon::prepareCollectionLoads" )
   INT32 _dmsStorageDataCommon::prepareCollectionLoads( dmsMBContext *context,
                                                        const BSONObj &record,
                                                        BOOLEAN isLast,
                                                        BOOLEAN isAsynchr,
                                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_PREPARECOLLECTIONLOADS ) ;

      dmsRecordData recordData( record.objdata(), record.objsize() ) ;
      rc = context->getCollPtr()->prepareLoads( recordData, isLast, isAsynchr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare collection [%s.%s] loads, "
                   "rc: %d", getSuName(), context->clName(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_PREPARECOLLECTIONLOADS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTIONLOADS, "_dmsStorageDataCommon::truncateCollectionLoads" )
   INT32 _dmsStorageDataCommon::truncateCollectionLoads( dmsMBContext * context,
                                                         pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_TRUNCATECOLLECTIONLOADS ) ;

      rc = context->getCollPtr()->truncateLoads( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate collection [%s.%s] loads, rc: %d",
                   getSuName(), context->clName(), rc ) ;

   done:
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_BUILDCOLLECTIONLOADS, "_dmsStorageDataCommon::buildCollectionLoads" )
   INT32 _dmsStorageDataCommon::buildCollectionLoads( dmsMBContext * context,
                                                      BOOLEAN isAsynchr,
                                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_BUILDCOLLECTIONLOADS ) ;

      rc = context->getCollPtr()->buildLoads( isAsynchr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build collection [%s.%s] loads, rc: %d",
                   getSuName(), context->clName(), rc ) ;

   done:
      if ( SDB_OK == rc )
      {
         /// flush mme
         flushMME( isSyncDeep() ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_BUILDCOLLECTIONLOADS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_CHGUID, "_dmsStorageDataCommon::changeCLUniqueID" )
   INT32 _dmsStorageDataCommon::changeCLUniqueID( const MAP_CLNAME_ID& modifyCl,
                                                  BOOLEAN changeOtherCL,
                                                  utilCSUniqueID csUniqueID,
                                                  BOOLEAN isLoadCS,
                                                  ossPoolVector<ossPoolString>& clVec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_CHGUID ) ;
      BOOLEAN hasChanged = FALSE ;

      ossScopedLock lock( &_metadataLatch, EXCLUSIVE ) ;

      COLNAME_MAP_IT it = _collectionNameMap.begin() ;
      while ( it != _collectionNameMap.end() )
      {
         const CHAR* clName = it->first ;
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
            if ( !changeOtherCL )
            {
               continue ;
            }
            if ( isLoadCS )
            {
               newClUniqueID = utilBuildCLUniqueID( csUniqueID,
                                                    UTIL_CLINNERID_LOADCS ) ;
            }
            else
            {
               utilCLInnerID orgInnerID = utilGetCLInnerID( orgClUniqueID ) ;
               newClUniqueID = utilBuildCLUniqueID( csUniqueID, orgInnerID ) ;
            }
         }

         try
         {
            clVec.push_back( clName ) ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }

         // skip when old id equals to new id
         if ( orgClUniqueID == newClUniqueID )
         {
            continue ;
         }
         hasChanged = TRUE ;

         // set new unique id
         _dmsMME->_mbList[mbID]._clUniqueID = newClUniqueID ;

         _collectionRemove ( clName, orgClUniqueID ) ;
         _collectionInsert ( clName, mbID, newClUniqueID ) ;

         PD_LOG( PDEVENT,
                 "Change cl[%s.%s] unique id from [%llu] to [%llu]",
                 _dmsHeader->_name, clName,
                 orgClUniqueID, newClUniqueID ) ;

      }

      if ( hasChanged )
      {
         flushMME( isSyncDeep() ) ;
      }

   done:
      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_CHGUID ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_RENAMECOLLECTION, "_dmsStorageDataCommon::renameCollection" )
   INT32 _dmsStorageDataCommon::renameCollection( const CHAR * oldName,
                                                  const CHAR * newName,
                                                  pmdEDUCB * cb,
                                                  SDB_DPSCB * dpscb,
                                                  BOOLEAN sysCollection,
                                                  utilCLUniqueID newCLUniqueID,
                                                  UINT32 *newStartLID )
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
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;

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
         rc = pTransCB->transLockTrySAgainstWrite( cb, _logicalCSID, mbID,
                                                   NULL, &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to lock the collection, rc: %d" OSS_NEWLINE
                      "Conflict( representative ):" OSS_NEWLINE
                      "   EDUID:  %llu" OSS_NEWLINE
                      "   TID:    %u" OSS_NEWLINE
                      "   LockId: %s" OSS_NEWLINE
                      "   Mode:   %s" OSS_NEWLINE,
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

      _collectionRemove ( oldName,
                          UTIL_UNIQUEID_NULL == newCLUniqueID ?
                                UTIL_UNIQUEID_NULL :
                                _dmsMME->_mbList[ mbID ]._clUniqueID ) ;
      _collectionInsert ( newName, mbID,
                          UTIL_UNIQUEID_NULL == newCLUniqueID ?
                                UTIL_UNIQUEID_NULL :
                                newCLUniqueID ) ;
      ossMemset ( _dmsMME->_mbList[mbID]._collectionName, 0,
                  DMS_COLLECTION_NAME_SZ ) ;
      ossStrncpy ( _dmsMME->_mbList[mbID]._collectionName, newName,
                   DMS_COLLECTION_NAME_SZ ) ;
      if ( UTIL_UNIQUEID_NULL != newCLUniqueID )
      {
         _dmsMME->_mbList[ mbID ]._clUniqueID = newCLUniqueID ;
      }
      clLID = _dmsMME->_mbList[mbID]._logicalID ;
      if ( NULL != newStartLID )
      {
         _mbStatInfo[ mbID ]._startLID = *newStartLID ;
      }

      // on metadata updated
      _onMBUpdated( mbID ) ;

      if ( dpscb )
      {
         rc = _logDPS( dpscb, info, cb, &_metadataLatch, EXCLUSIVE, metalocked,
                       clLID, DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert clrename to log, rc = %d",
                      rc ) ;
      }
      else if ( NULL != cb )
      {
         _clFullName( newName, fullName, sizeof(fullName) ) ;
         cb->setDataExInfo( fullName, _logicalCSID, clLID,
                            DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_COPYCOLLECTION, "_dmsStorageDataCommon::copyCollection" )
   INT32 _dmsStorageDataCommon::copyCollection( dmsMBContext *oldMBContext,
                                                const CHAR *newName,
                                                utilCLUniqueID newCLUniqueID,
                                                pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_COPYCOLLECTION ) ;

      SDB_ASSERT( oldMBContext->isMBLock(), "mb context should be locked" ) ;

      const CHAR *oldName = oldMBContext->mb()->_collectionName ;
      BSONObj extOptions ;
      UINT16 newMBID = DMS_INVALID_MBID ;
      UINT32 newCLLID = DMS_INVALID_CLID ;
      BOOLEAN added = FALSE ;
      dmsMBContext *tmpMBContext = NULL ;
      UINT32 attributes = oldMBContext->mb()->_attributes ;
      UINT8 compressorType = oldMBContext->mb()->_compressorType ;
      ossPoolVector< BSONObj > droppedIndexList ;

      PD_LOG( PDDEBUG, "Start copy collection [from: %s.%s, "
              "to %s.%s]", getSuName(), oldName, getSuName(), newName ) ;

      rc = dumpExtOptions( oldMBContext, extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get ext options for "
                   "collection [%s], rc: %d", oldName, rc ) ;

      // not create id index, will copy index later
      OSS_BIT_SET( attributes, DMS_MB_ATTR_NOIDINDEX ) ;

      rc = addCollection( newName, &newMBID, newCLUniqueID, attributes, cb,
                          NULL, 0, FALSE, compressorType, &newCLLID,
                          &extOptions ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add collection [%s], rc: %d",
                   newName, rc ) ;

      added = TRUE ;

      rc = getMBContext( &tmpMBContext, newMBID, DMS_INVALID_CLID,
                         DMS_INVALID_CLID, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get mb context for copy collection "
                   "[%s], rc: %d", newName, rc ) ;

      // drop indexes with external data ( text and global index )
      rc = _dropIndexesWithTypes( oldMBContext, cb,
                                  ( IXM_EXTENT_TYPE_TEXT |
                                    IXM_EXTENT_TYPE_GLOBAL ),
                                  &droppedIndexList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop text and global index "
                   "from collection [%s], rc: %d", oldName, rc ) ;

      // copy indexes without external data ( text and global index )
      rc = _copyIndexesWithoutTypes( oldMBContext, tmpMBContext, cb,
                                     ( IXM_EXTENT_TYPE_TEXT |
                                       IXM_EXTENT_TYPE_GLOBAL ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to copy indexes for collection [%s], "
                   "rc: %d", newName, rc ) ;

      // copy the monitor metrics
      tmpMBContext->mbStat()->_crudCB.set( oldMBContext->mbStat()->_crudCB ) ;

      for ( ossPoolVector< BSONObj >::iterator iter = droppedIndexList.begin() ;
            iter != droppedIndexList.end() ;
            ++ iter )
      {
         BSONObj indexDef = *iter ;
         // copy text and global index to new collection
         INT32 tmpRC = _pIdxSU->createIndex( tmpMBContext, indexDef, cb, NULL ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to create index [%s] to collection "
                    "[%s.%s], rc: %d", indexDef.toPoolString().c_str(),
                    getSuName(), newName, tmpRC ) ;
         }
      }

      oldMBContext->swap( *tmpMBContext ) ;

      PD_LOG( PDDEBUG, "Finish copy collection [from: %s.%s, "
              "to: %s.%s]", getSuName(), oldName, getSuName(), newName ) ;

   done:
      if ( NULL != tmpMBContext )
      {
         releaseMBContext( tmpMBContext ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON_COPYCOLLECTION, rc ) ;
      return rc ;

   error:
      if ( added )
      {
         INT32 tmpRC = dropCollection( newName, cb, NULL, FALSE,
                                       tmpMBContext ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG ( PDWARNING, "Failed to drop collection [%s], rc: %d",
                     newName, tmpRC ) ;
         }
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_RECYCOLLECTION, "_dmsStorageDataCommon::recycleCollection" )
   INT32 _dmsStorageDataCommon::recycleCollection( dmsMBContext *mbContext,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_RECYCOLLECTION ) ;

      SDB_ASSERT( mbContext->isMBLock(), "mb context should be locked" ) ;

      // drop all indexes with external data ( text index and global index )
      rc = _dropIndexesWithTypes( mbContext, cb,
                                  ( IXM_EXTENT_TYPE_TEXT |
                                    IXM_EXTENT_TYPE_GLOBAL ) ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to drop indexes with external data "
                   "from collection [%s.%s], rc: %d", getSuName(),
                   mbContext->mb()->_collectionName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON_RECYCOLLECTION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__COPYINDEXESWITHOUTTYPES, "_dmsStorageDataCommon::_copyIndexesWithoutTypes" )
   INT32 _dmsStorageDataCommon::_copyIndexesWithoutTypes( dmsMBContext *oldContext,
                                                          dmsMBContext *newContext,
                                                          _pmdEDUCB *cb,
                                                          UINT16 types )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__COPYINDEXESWITHOUTTYPES ) ;

      UINT32 idxSlot = 0 ;

      const CHAR *oldName = oldContext->mb()->_collectionName ;
      const CHAR *newName = newContext->mb()->_collectionName ;

      while ( idxSlot < DMS_COLLECTION_MAX_INDEX &&
              DMS_INVALID_EXTENT != oldContext->mb()->_indexExtent[ idxSlot ] )
      {
         INT32 tmpRC = SDB_OK ;
         ixmIndexCB indexCB( oldContext->mb()->_indexExtent[ idxSlot ],
                             _pIdxSU, oldContext ) ;
         BSONObj indexDef ;
         BOOLEAN isSysIndex = FALSE ;

         if ( !indexCB.isInitialized() )
         {
            PD_LOG( PDWARNING, "Failed to get index on slot [%u] of "
                    "collection [%s.%s], it is not initialized", idxSlot,
                    getSuName(), oldName ) ;
            ++ idxSlot ;
            continue ;
         }
         else if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(), types ) )
         {
            ++ idxSlot ;
            continue ;
         }

         // copy index definition
         try
         {
            if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                      IXM_EXTENT_TYPE_TEXT ) )
            {
               BSONObjBuilder builder ;
               BSONObjIterator iter( indexCB.getDef() ) ;
               while ( iter.more() )
               {
                  BSONElement element = iter.next() ;
                  if ( 0 != ossStrcmp( FIELD_NAME_EXT_DATA_NAME,
                                       element.fieldName() ) )
                  {
                     builder.append( element ) ;
                  }
               }
               indexDef = builder.obj() ;
            }
            else
            {
               indexDef = indexCB.getDef().copy() ;
            }
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to build copy index define BSON, "
                    "occur exception %s", e.what() ) ;
            ++ idxSlot ;
            continue ;
         }

         PD_LOG( PDDEBUG, "Copy index [%s] to [%s]",
                 indexDef.toPoolString().c_str(), newName ) ;

         isSysIndex = dmsIsSysIndexName( indexCB.getName() ) ;

         // copy index to new collection
         tmpRC = _pIdxSU->createIndex( newContext, indexDef, cb, NULL,
                                       isSysIndex ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to create index [%s] to collection "
                    "[%s.%s], rc: %d", indexDef.toPoolString().c_str(),
                    getSuName(), newName, tmpRC ) ;
         }

         ++ idxSlot ;
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__COPYINDEXESWITHOUTTYPES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__DROPINDEXESWITHTYPES, "_dmsStorageDataCommon::_dropIndexesWithTypes" )
   INT32 _dmsStorageDataCommon::_dropIndexesWithTypes( dmsMBContext *context,
                                                       _pmdEDUCB *cb,
                                                       UINT16 types,
                                                       ossPoolVector< BSONObj > *droppedIndexList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__DROPINDEXESWITHTYPES ) ;

      UINT32 idxSlot = 0 ;

      // text index and global index will be rebuild, so drop the old ones
      while ( idxSlot < DMS_COLLECTION_MAX_INDEX )
      {
         if ( DMS_INVALID_EXTENT == context->mb()->_indexExtent[ idxSlot ] )
         {
            break ;
         }
         else
         {
            dmsExtentID indexExtentID =
                              context->mb()->_indexExtent[ idxSlot ] ;
            ixmIndexCB indexCB( indexExtentID, _pIdxSU, context ) ;

            if ( ( indexCB.isInitialized() ) &&
                 ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(), types ) ) )
            {
               INT32 tmpRC = SDB_OK ;

               if ( NULL != droppedIndexList )
               {
                  // copy index definition
                  try
                  {
                     BSONObj indexDef ;

                     if ( IXM_EXTENT_HAS_TYPE( indexCB.getIndexType(),
                                               IXM_EXTENT_TYPE_TEXT ) )
                     {
                        BSONObjBuilder builder ;
                        BSONObjIterator iter( indexCB.getDef() ) ;
                        while ( iter.more() )
                        {
                           BSONElement element = iter.next() ;
                           if ( 0 != ossStrcmp( FIELD_NAME_EXT_DATA_NAME,
                                                element.fieldName() ) )
                           {
                              builder.append( element ) ;
                           }
                        }
                        indexDef = builder.obj() ;
                     }
                     else
                     {
                        indexDef = indexCB.getDef().copy() ;
                     }

                     droppedIndexList->push_back( indexDef ) ;
                  }
                  catch ( exception &e )
                  {
                     PD_LOG( PDERROR, "Failed to build define BSON, "
                             "occur exception %s", e.what() ) ;
                     // Failed to copy ignore this index
                     ++ idxSlot ;
                     continue ;
                  }
               }

               tmpRC = _pIdxSU->dropIndex( context, idxSlot,
                                           indexCB.getLogicalID(), cb,
                                           NULL ) ;
               if ( SDB_OK != tmpRC )
               {
                  PD_LOG( PDWARNING, "Failed to drop index on slot [%u], "
                          "rc: %d", idxSlot, tmpRC ) ;
               }
               else
               {
                  continue ;
               }
            }
         }

         ++ idxSlot ;
      }

      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON__DROPINDEXESWITHTYPES, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_RTRNCL, "_dmsStorageDataCommon::returnCollection" )
   INT32 _dmsStorageDataCommon::returnCollection( const CHAR *originName,
                                                  const CHAR *recycleName,
                                                  dmsReturnOptions &options,
                                                  pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB,
                                                  dmsMBContext **returnedMBContext )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_RTRNCL ) ;

      dpsTransCB *transCB     = pmdGetKRCB()->getTransCB() ;
      UINT32 logRecSize       = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record    = info.getMergeBlock().record() ;

      dmsMBContext *mbContext = NULL ;
      BOOLEAN isLogReserved = FALSE, isTransLocked = FALSE ;
      UINT16 recyMBID = DMS_INVALID_MBID ;
      utilCLUniqueID newUniqueID = UTIL_UNIQUEID_NULL ;
      UINT32 newStartLID = DMS_INVALID_CLID ;
      CHAR fullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = {0} ;

      PD_LOG( PDDEBUG, "Start return collection [origin: %s.%s, "
              "recycle %s.%s]", getSuName(), originName, getSuName(),
              recycleName ) ;

      // reserved log-size
      if ( dpsCB )
      {
         rc = options.prepareOptions() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare return options, "
                      "rc: %d", rc ) ;

         rc = dpsReturn2Record( &( options._boOptions ), record ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build log record, rc: %d", rc ) ;

         rc = dpsCB->checkSyncControl( record.alignedLen(), cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check sync control, rc: %d",
                      rc ) ;

         logRecSize = record.alignedLen() ;
         rc = transCB->reservedLogSpace( logRecSize, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to reserve log space [length %u], "
                      "rc: %d", rc ) ;

         isLogReserved = TRUE ;
      }

      // drop origin collection
      if ( UTIL_RECYCLE_OP_TRUNCATE == options._recycleItem.getOpType() )
      {
         rc = dropCollection( originName, cb, NULL ) ;
         if ( SDB_DMS_NOTEXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop collection [%s], rc: %d",
                      originName, rc ) ;
      }

      rc = getMBContext( &mbContext, recycleName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get metadata block for collection "
                   "[%s], rc: %d", recycleName, rc ) ;

      recyMBID = mbContext->mbID() ;

      if ( cb && cb->getTransExecutor()->useTransLock() )
      {
         dpsTransRetInfo lockConflict ;
         rc = transCB->transLockTryX( cb, _logicalCSID, recyMBID, NULL,
                                      &lockConflict ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to lock the collection, rc: %d" OSS_NEWLINE
                      "Conflict( representative ):" OSS_NEWLINE
                      "   EDUID:  %llu" OSS_NEWLINE
                      "   TID:    %u" OSS_NEWLINE
                      "   LockId: %s" OSS_NEWLINE
                      "   Mode:   %s" OSS_NEWLINE,
                      rc,
                      lockConflict._eduID,
                      lockConflict._tid,
                      lockConflict._lockID.toString().c_str(),
                      lockModeToString( lockConflict._lockType ) ) ;
         isTransLocked = TRUE ;
      }

      newUniqueID = (utilCLUniqueID)( options._recycleItem.getOriginID() ) ;
      newStartLID = mbContext->mb()->_logicalID ;

      // release mb context during rename
      // NOTE: should block both origin and recycle collections
      releaseMBContext( mbContext ) ;

      rc = renameCollection( recycleName, originName, cb, NULL, TRUE,
                             newUniqueID, &newStartLID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to rename collection, rc: %d", rc ) ;

      // re-fetch mb context during rename
      // NOTE: should block both origin and recycle collections
      rc = getMBContext( &mbContext, originName, EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get metadata block for collection "
                   "[%s], rc: %d", originName, rc ) ;

      // clear truncate flag
      DMS_MB_STATINFO_CLEAR_TRUNCATED( mbContext->mbStat()->_flag ) ;

      if ( dpsCB )
      {
         rc = _logDPS( dpsCB, info, cb, mbContext, DMS_INVALID_EXTENT,
                       DMS_INVALID_OFFSET, FALSE, DMS_FILE_ALL, NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to write recycle record to log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         mbContext->mbStat()->updateLastLSN( cb->getEndLsn(), DMS_FILE_ALL ) ;
         _clFullName( originName, fullName, sizeof(fullName) ) ;
         cb->setDataExInfo( fullName, logicalID(), mbContext->clLID(),
                            DMS_INVALID_EXTENT, DMS_INVALID_OFFSET ) ;
      }

      if ( NULL != mbContext )
      {
         if ( NULL != returnedMBContext )
         {
            *returnedMBContext = mbContext ;
            mbContext = NULL ;
         }
      }

      PD_LOG( PDDEBUG, "Finish return collection [origin: %s.%s, "
              "recycle %s.%s]", getSuName(), originName, getSuName(),
              recycleName ) ;

   done:
      if ( NULL != mbContext )
      {
         releaseMBContext( mbContext ) ;
      }
      if ( isTransLocked )
      {
         transCB->transLockRelease( cb, _logicalCSID, recyMBID ) ;
         isTransLocked = FALSE ;
      }
      if ( isLogReserved )
      {
         transCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__DMSSTORAGEDATACOMMON_RTRNCL, rc ) ;
      return rc ;

   error:
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

   INT32 _dmsStorageDataCommon::_insertIndexes( dmsMBContext *context,
                                                dmsExtentID extLID,
                                                BSONObj &inputObj,
                                                const dmsRecordID &rid,
                                                pmdEDUCB * cb,
                                                IDmsOprHandler *pOprHandle,
                                                dmsWriteGuard &writeGuard,
                                                utilWriteResult *insertResult,
                                                dpsUnqIdxHashArray *pUnqIdxHashArray )
   {
      INT32 rc = SDB_OK ;
      // insert object's indexes
      rc = _pIdxSU->indexesInsert( context, extLID, inputObj, rid, cb,
                                   pOprHandle, writeGuard, insertResult,
                                   pUnqIdxHashArray ) ;
      if ( rc )
      {
         if ( insertResult &&
              insertResult->isMaskEnabled( UTIL_RESULT_MASK_ID ) )
         {
            /// current id
            if ( insertResult->getCurID().isEmpty() )
            {
               insertResult->setCurrentID( inputObj ) ;
            }
            /// peer id
            if ( insertResult->getPeerID().isEmpty() &&
                 insertResult->getPeerRID().isValid() )
            {
               dmsRecordData peerData ;
               if ( SDB_OK == fetch( context, insertResult->getPeerRID(),
                                     peerData, cb ) )
               {
                  BSONObj peerObj( peerData.data() ) ;
                  insertResult->setPeerID( peerObj ) ;
               }
            }
         }

         PD_LOG( PDERROR, "Failed to insert to index, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_INSERTRECORD, "_dmsStorageDataCommon::insertRecord" )
   INT32 _dmsStorageDataCommon::insertRecord( dmsMBContext *context,
                                              const BSONObj &record,
                                              pmdEDUCB *cb,
                                              SDB_DPSCB *dpsCB,
                                              BOOLEAN mustOID,
                                              BOOLEAN canUnLock,
                                              INT64 position,
                                              utilInsertResult *insertResult )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_INSERTRECORD ) ;

      UINT32 dmsRecordSize          = 0 ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      BSONObj insertObj             = record ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      UINT32 logRecSize             = 0 ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      dpsMergeInfo info ;
      dpsLogRecord &logRecord       = info.getMergeBlock().record() ;
      // trans related
      DPS_TRANS_ID transID          = cb->getTransID() ;
      DPS_LSN_OFFSET preTransLsn    = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLsn     = cb->getRelatedTransLSN() ;
      BOOLEAN  isTransLocked        = FALSE ;
      // delete record related
      dmsRecordID foundRID ;
      dmsRecordData recordData ;
      IDmsExtDataHandler * handler  = NULL ;
      BOOLEAN newMem = FALSE ;
      CHAR *pMergedData = NULL ;
      dmsTransLockCallback callback( pTransCB, cb ) ;

      _sdbRemoteOpCtrlAssist ctrlAssist( cb->getRemoteOpCtrl() ) ;

      dpsUnqIdxHashArray unqIdxHashArray ;
      dpsUnqIdxHashArray *pUnqIdxHashArray = NULL ;

      dmsWriteGuard writeGuard( _service, this, context, cb ) ;

      if ( !isTransSupport( context ) )
      {
         transID = DPS_INVALID_TRANS_ID ;
         preTransLsn = DPS_INVALID_LSN_OFFSET ;
         relatedLsn = DPS_INVALID_LSN_OFFSET ;
      }

      rc = _checkReusePosition( context, transID, cb, position, foundRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check reuse position, rc: %d", rc ) ;

      rc = _checkInsertData( record, mustOID, cb, recordData, newMem, position ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check insert data, rc: %d", rc ) ;
      if ( newMem )
      {
         pMergedData = (CHAR *)( recordData.data() ) ;
      }

      try
      {
         insertObj = BSONObj( recordData.data() ) ;
         dmsRecordSize = recordData.len() ;

         // check
         if ( recordData.len() + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
         {
            rc = SDB_DMS_RECORD_TOO_BIG ;
            goto error ;
         }

         // Step 2: Calculate the required space size, and allocate it,
         // both for data record and replication log.
         // Get the final recordsize that we have to allocate:
         // reserved space, alignment, etc.
         _finalRecordSize( dmsRecordSize, recordData ) ;

         _clFullName( context->mb()->_collectionName, fullName, sizeof(fullName) ) ;

         // calc log reserve
         if ( dpsCB )
         {
            if ( ( !cb->isInTransRollback() ) &&
                 ( !OSS_BIT_TEST( context->mb()->_attributes,
                                  DMS_MB_ATTR_NOIDINDEX ) ) &&
                 ( context->mbStat()->_uniqueIdxNum > 1 ) )
            {
               // need save 1 value for each unique index ( except for $id
               // index )
               // NOTE:
               // - for insert, it can without $id index, if it doesn't
               //   have $id index, the secondary nodes will not replay
               //   in parallel, so it can without hash array
               // - two reasons we don't append index hash for
               //   transaction rollback
               //   + index hash will enlarge the size of DPS record
               //     against the origin transaction DPS record.
               //   + In secondary nodes, it will allow duplicated keys
               //     during transaction rollback, so the index hash is
               //     useless
               rc = unqIdxHashArray.prepare(
                                 context->mbStat()->_uniqueIdxNum - 1, TRUE ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to prepare hash list for "
                            "unique index [%u], rc: %d",
                            context->mbStat()->_uniqueIdxNum, rc ) ;

               pUnqIdxHashArray = &unqIdxHashArray ;
            }

            // reserved log-size
            rc = dpsInsert2Record( fullName, insertObj, pUnqIdxHashArray, transID,
                                   preTransLsn, relatedLsn, logRecord ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build record, rc: %d", rc ) ;

            logRecSize = ossAlign4( logRecord.alignedLen() ) ;

            rc = dpsCB->checkSyncControl( logRecSize, cb ) ;
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
         if ( cb->getTransExecutor()->useTransLock() )
         {
            rc = context->mbLock( getWriteLockType() ) ;
         }
         else
         {
            rc = context->mbLock( EXCLUSIVE ) ;
         }
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
         else if ( isTransSupport( context ) &&
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


         if ( context->mbStat()->_textIdxNum > 0 )
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

         if ( !foundRID.isValid() )
         {
            rc = _allocRecordSpace( context, dmsRecordSize, foundRID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate space for record, rc: %d", rc ) ;
         }
         else
         {
            rc = _checkRecordSpace( context, dmsRecordSize, foundRID, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to chec space for record, rc: %d", rc ) ;
         }

         // NOTE: we still need transaction locks during rollback
         // the insert record to rollback delete operation may insert
         // to a new place
         if ( isTransLockRequired( context ) &&
               NULL != cb &&
               cb->getTransExecutor()->useTransLock() )
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
                        "rc: %d" OSS_NEWLINE
                        "Conflict( representative ):" OSS_NEWLINE
                        "   EDUID:  %llu" OSS_NEWLINE
                        "   TID:    %u" OSS_NEWLINE
                        "   LockId: %s" OSS_NEWLINE
                        "   Mode:   %s" OSS_NEWLINE,
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

         rc = _pIdxSU->checkProcess( context, foundRID, writeGuard.getIndexWriteGuard() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check index process, rc: %d", rc ) ;

         rc = writeGuard.begin() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to begin write guard, rc: %d", rc ) ;

         rc = _prepareInsert( foundRID, recordData ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to prepare insert, rc: %d", rc ) ;

         // insert to extent
         rc = context->getCollPtr()->insertRecord( foundRID, recordData, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record, rc: %d", rc ) ;

         writeGuard.getPersistGuard().incRecordCount() ;
         writeGuard.getPersistGuard().incOrgDataLen( recordData.len() ) ;
         writeGuard.getPersistGuard().incDataLen( recordData.len() ) ;

         //increase data write counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
         // update totalInsert monitor counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_INSERT, 1 ) ;
         _incWriteRecord() ;

         rc = _insertIndexes( context, foundRID._extent, insertObj,
                              foundRID, cb, dpsCB ? &callback : NULL,
                              writeGuard, insertResult, pUnqIdxHashArray ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert indexes, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      if ( dpsCB )
      {
         PD_AUDIT_OP_WITHNAME( AUDIT_INSERT, "INSERT", AUDIT_OBJ_CL,
                               fullName, rc, "%s",
                               insertObj.toString().c_str() ) ;

         /// enable trans
         if ( isTransSupport( context ) )
         {
            info.enableTrans() ;
         }
         rc = _logDPS( dpsCB, info, cb, context,
                       foundRID._extent, foundRID._offset, canUnLock,
                       DMS_FILE_DATA ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to insert record into log, "
                       "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
         cb->setDataExInfo( fullName, logicalID(), context->clLID(),
                            foundRID._extent, foundRID._offset ) ;
      }

      if ( handler )
      {
         handler->done( DMS_EXTOPR_TYPE_INSERT, cb ) ;
      }

      rc = writeGuard.commit() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Failed to commit write guard, rc: %d", rc ) ;
         ossPanic() ;
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
      ctrlAssist.switchToUndo() ;
      {
         INT32 tmpRC = _pIdxSU->indexesDelete( context, foundRID._extent,
                                               insertObj, foundRID, cb,
                                               NULL, writeGuard, TRUE,
                                               pUnqIdxHashArray ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to undo indexes, rc: %d", tmpRC ) ;
         }
      }
      if ( !ctrlAssist.isUndoFinished() )
      {
         // undo is not finished
         if ( SDB_OK == cb->getTransRC() )
         {
            cb->setTransRC( rc ) ;
         }
      }

      if ( handler )
      {
         handler->abortOperation( DMS_EXTOPR_TYPE_INSERT, cb ) ;
      }

      {
         INT32 tmpRC = writeGuard.abort() ;
         if ( tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to abort write guard, rc: %d", tmpRC ) ;
         }
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
                                              const dmsTransRecordInfo *pInfo,
                                              BOOLEAN isUndo )
   {
      INT32 rc                      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__DMSSTORAGEDATACOMMON_DELETERECORD ) ;
      dpsTransCB *pTransCB          = pmdGetKRCB()->getTransCB() ;
      monAppCB * pMonAppCB          = cb ? cb->getMonAppCB() : NULL ;
      BSONObj delObject ;
      UINT32 logRecSize             = 0 ;
      dpsMergeInfo info ;
      dpsLogRecord &record          = info.getMergeBlock().record() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      DPS_TRANS_ID transID          = cb->getTransID() ;
      DPS_LSN_OFFSET preLsn         = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLSN     = cb->getRelatedTransLSN() ;
      dmsRecordData recordData ;
      IDmsExtDataHandler *handler   = NULL ;
      _sdbRemoteCountAssist countAssist ;
      BOOLEAN inTrans               = FALSE ;
      BOOLEAN needSetTransRC        = FALSE ;

      dpsUnqIdxHashArray unqIdxHashArray ;
      dpsUnqIdxHashArray *pUnqIdxHashArray = NULL ;
      INT64 delPosition = -1 ;

      dmsWriteGuard writeGuard( _service, this, context, cb ) ;

      if ( !context->isMBLock() )
      {
         PD_LOG( PDERROR, "Caller must hold mb lock[%s]",
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

      if ( !isTransSupport( context ) )
      {
         transID = DPS_INVALID_TRANS_ID ;
         preLsn = DPS_INVALID_LSN_OFFSET ;
         relatedLSN = DPS_INVALID_LSN_OFFSET ;
      }

      try
      {
         rc = _pIdxSU->checkProcess( context, recordID, writeGuard.getIndexWriteGuard() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check index process, rc: %d", rc ) ;

         rc = writeGuard.begin() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to begin write guard, rc: %d", rc ) ;

         // when in transaction(not rollback), we should not immediately
         // delete the record, otherwise TB scan won't be able to find
         // this record even if the current transaction has not committed.
         if ( DPS_INVALID_TRANS_ID != transID && !cb->isInTransRollback() )
         {
            inTrans = TRUE ;
         }

         // first time deletion of the record write the LR and delete indexes
         if ( deletedDataPtr )
         {
            recordData.setData( (const CHAR*)deletedDataPtr,
                                 *(UINT32*)deletedDataPtr ) ;
         }
         else
         {
            rc = extractData( context, recordID, cb, recordData ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract data failed, rc: %d", rc ) ;
         }

         // get record position
         rc = _getRecordPosition( context, recordID, recordData, cb, delPosition ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get record position, rc: %d", rc ) ;

         // delete index keys
         try
         {
            delObject = BSONObj( recordData.data() ) ;

            if ( context->mbStat()->_textIdxNum > 0 )
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

            _clFullName( context->mb()->_collectionName, fullName,
                           sizeof(fullName) ) ;

            // first to reserve dps
            if ( NULL != dpscb )
            {
               if ( pHandler )
               {
                  rc = pHandler->onDeleteRecord( context, delObject,
                                                 recordID, NULL,
                                                 FALSE,
                                                 cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Process delete record(%s) in "
                             "handler failed, rc: %d",
                             PD_SECURE_OBJ( delObject ), rc ) ;
                     goto error ;
                  }
               }

               if ( ( !cb->isInTransRollback() ) &&
                     ( context->mbStat()->_uniqueIdxNum > 1 ) )
               {
                  // need save 1 value for each unique index ( except
                  // for $id index )
                  // NOTE:
                  // - for delete, it can not without $id index, so we
                  //   can exclude one $id unique index
                  // - two reasons we don't append index hash for
                  //   transaction rollback
                  //   + index hash will enlarge the size of DPS record
                  //     against the origin transaction DPS record.
                  //   + In secondary nodes, it will allow duplicated keys
                  //     during transaction rollback, so the index hash is
                  //     useless
                  rc = unqIdxHashArray.prepare(
                        context->mbStat()->_uniqueIdxNum - 1, FALSE ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to prepare hash "
                               "list for unique index [%u], rc: %d",
                               context->mbStat()->_uniqueIdxNum, rc ) ;

                  pUnqIdxHashArray = &unqIdxHashArray ;
               }

               // reserved log-size
               // NOTE: only append position if mark deleting during
               //       transaction ( not including rollback phase )
               rc = dpsDelete2Record( fullName, delObject, pUnqIdxHashArray,
                                      inTrans ? &delPosition : NULL, transID,
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

            countAssist.setCounts( cb->getRemoteSucCount(),
                                   cb->getRemoteFailureCount() ) ;
            // then delete indexes. Note that the old version indexes
            // would be kept in the in memory tree under the cover
            rc = _pIdxSU->indexesDelete( context, recordID._extent,
                                         delObject, recordID, cb,
                                         dpscb ? pHandler : NULL,
                                         writeGuard, isUndo,
                                         pUnqIdxHashArray ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Failed to delete indexes, rc: %d",rc ) ;
               if ( !isUndo && countAssist.isPartialFailure(
                                                cb->getRemoteSucCount(),
                                                cb->getRemoteFailureCount() ) )
               {
                  // in normal flow, paritial failure can't be resolved.
                  // we should rollback this transaction to restore global
                  // index
                  needSetTransRC = TRUE ;
                  goto error ;
               }

               if ( !context->isMBLock() )
               {
                  // context may be paused in _pIdxSU->indexesDelete
                  PD_LOG( PDERROR, "context is paused[%s]",
                          context->toString().c_str() ) ;
                  goto error ;
               }
               // in undo flow, let's continue remove the record.

               // if local index delete fail, let's continue remove the record
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Corrupted record: %d:%d: %s",
                     recordID._extent, recordID._offset, e.what() ) ;
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }

         // delete really
         rc = context->getCollPtr()->removeRecord( recordID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove record, rc: %d", rc ) ;

         //here when we sub the remove data info,
         //if the record has compresssed,the orgLen mean the record size
         //in DB,len mean the uncompress size. So when we substract the
         //size,we should swap them.
         writeGuard.getPersistGuard().decRecordCount() ;
         writeGuard.getPersistGuard().decDataLen( recordData.len() ) ;
         writeGuard.getPersistGuard().decOrgDataLen( recordData.len() ) ;

         //increase data write counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;

         // increase conter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DELETE, 1 ) ;
         _incWriteRecord() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = pdGetLastError() ? pdGetLastError() : SDB_SYS ;
         goto error ;
      }

      // if we are asked to log
      if ( dpscb )
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

         if ( isTransSupport( context ) )
         {
            info.enableTrans() ;
         }
         rc = _logDPS( dpscb, info, cb, context, recordID._extent,
                       recordID._offset, FALSE, DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record into log, rc: %d",
                      rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
         cb->setDataExInfo( fullName, logicalID(), context->clLID(),
                            recordID._extent, recordID._offset ) ;
      }

      if ( handler )
      {
         handler->done( DMS_EXTOPR_TYPE_DELETE, cb ) ;
      }

      rc = writeGuard.commit() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Failed to commit write guard, rc: %d", rc ) ;
         ossPanic() ;
      }

   done :
      if ( 0 != logRecSize )
      {
         pTransCB->releaseLogSpace( logRecSize, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB__DMSSTORAGEDATACOMMON_DELETERECORD, rc ) ;
      return rc ;
   error :
      if ( handler )
      {
         handler->abortOperation( DMS_EXTOPR_TYPE_DELETE, cb ) ;
      }
      if ( needSetTransRC && SDB_OK == cb->getTransRC() )
      {
         cb->setTransRC( rc ) ;
      }
      {
         INT32 tmpRC = writeGuard.abort() ;
         if ( tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to abort write guard, rc: %d", tmpRC ) ;
         }
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
      UINT32 writeMod = DPS_LOG_WRITE_MOD_INCREMENT ;
      UINT32 *pWriteMod = NULL ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      CHAR fullName[DMS_COLLECTION_FULL_NAME_SZ + 1] = {0} ;
      DPS_TRANS_ID transID = cb->getTransID() ;
      DPS_LSN_OFFSET preTransLsn = cb->getCurTransLsn() ;
      DPS_LSN_OFFSET relatedLSN = cb->getRelatedTransLSN() ;

      dmsRecordData recordData, newRecordData ;
      IDmsExtDataHandler *handler = NULL ;

      // create a new object for updated record
      BSONObj newobj ;

      dpsUnqIdxHashArray newUnqIdxHashArray, oldUnqIdxHashArray ;
      dpsUnqIdxHashArray *pNewUnqIdxHashArray = NULL ;
      dpsUnqIdxHashArray *pOldUnqIdxHashArray = NULL ;
      BOOLEAN needUndoIndex = FALSE ;

      _sdbRemoteOpCtrlAssist ctrlAssist( cb->getRemoteOpCtrl() ) ;
      dmsWriteGuard writeGuard( _service, this, context, cb ) ;

      rc = _operationPermChk( DMS_ACCESS_TYPE_UPDATE ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed in permission check of update, rc: %d", rc ) ;

      if ( !context->isMBLock() )
      {
         PD_LOG( PDERROR, "Caller must hold mb lock[%s]",
                 context->toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !isTransSupport( context ) )
      {
         transID = DPS_INVALID_TRANS_ID ;
         preTransLsn = DPS_INVALID_LSN_OFFSET ;
         relatedLSN = DPS_INVALID_LSN_OFFSET ;
      }

      try
      {
#ifdef _DEBUG
         if ( !dmsAccessAndFlagCompatiblity ( context->mb()->_flag,
                                              DMS_ACCESS_TYPE_UPDATE ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     context->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }
#endif //_DEBUG

         // get data
         if ( updatedDataPtr )
         {
            recordData.setData( (const CHAR*)updatedDataPtr,
                                *(UINT32*)updatedDataPtr ) ;
         }
         else
         {
            rc = extractData( context, recordID, cb, recordData ) ;
            PD_RC_CHECK( rc, PDERROR, "Extract data failed, rc: %d", rc ) ;
         }

         try
         {
            BSONObj obj ( recordData.data() ) ;

            if ( dpscb )
            {
               if ( DPS_LOG_WRITE_MOD_INCREMENT == cb->getLogWriteMod() )
               {
                  rc = modifier.modify ( obj, newobj, &oldMatch, &oldChg,
                                         &newMatch, &newChg,
                                         &oldShardingKey, &newShardingKey ) ;
                  // set to NULL indicate do not write tag DPS_LOG_UPDATE_WRITEMOD
                  pWriteMod = NULL ;
               }
               else
               {
                  writeMod = DPS_LOG_WRITE_MOD_FULL ;
                  rc = modifier.modify ( obj, newobj, &oldMatch, NULL,
                                         &newMatch, NULL,
                                         &oldShardingKey, &newShardingKey ) ;
                  // obj and newobj's life cycle is too short to log dps.
                  oldChg = obj.getOwned() ;
                  newChg = newobj.getOwned() ;
                  // others write tag DPS_LOG_UPDATE_WRITEMOD
                  pWriteMod = &writeMod ;
               }

               if ( SDB_OK == rc && pHandler )
               {
                  rc = pHandler->onUpdateRecord( context, obj, newobj,
                                                 recordID, NULL, cb ) ;
                  if ( rc )
                  {
                     PD_LOG( PDERROR, "Process update record[%s] to [%s] "
                             "in handler failed, rc: %d",
                             PD_SECURE_OBJ( obj ), PD_SECURE_OBJ( newobj ), rc ) ;
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

            // Check the new object size
            if ( newobj.objsize() + DMS_RECORD_METADATA_SZ > DMS_RECORD_USER_MAX_SZ )
            {
               PD_LOG ( PDERROR, "record is too big: %d", newobj.objsize() ) ;
               rc = SDB_DMS_RECORD_TOO_BIG ;
               goto error ;
            }
            // check ID index for normal update
            // NOTE: for sequoiadb upgrade, if the old data before upgrade
            //       contains invalid _id field, we could not report error,
            //       we need to allow update if _id field is not changed
            if( !cb->isDoReplay() &&
                !cb->isInTransRollback() &&
                !cb->isDoRollback() )
            {
               BSONElement newId = newobj.getField( DMS_ID_KEY_NAME ) ;
               const CHAR *pCheckErr = "" ;
               if( !dmsIsRecordIDValid( newId, TRUE, &pCheckErr ) )
               {
                  if( Array == newId.type() &&
                      0 == newId.woCompare( obj.getField( DMS_ID_KEY_NAME ) ) )
                  {
                  }
                  else
                  {
                     PD_LOG_MSG( PDERROR, "_id is error: %s", pCheckErr ) ;
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
               }
            }

            newRecordData.setData( newobj.objdata(), newobj.objsize() ) ;

            rc = _pIdxSU->checkProcess( context, recordID, writeGuard.getIndexWriteGuard() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check index process, rc: %d", rc ) ;

            rc = writeGuard.begin() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to begin write guard, rc: %d", rc ) ;

            if ( context->mbStat()->_textIdxNum > 0 )
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

            _clFullName( context->mb()->_collectionName, fullName,
                         sizeof(fullName) ) ;

            if ( NULL != dpscb )
            {
               if ( ( !cb->isInTransRollback() ) &&
                    ( context->mbStat()->_uniqueIdxNum > 1 ) )
               {
                  // may save 2 keys for update, both new and old keys for
                  // each unique index ( except for $id index )
                  // NOTE:
                  // - for update, it can not without $id index, so we can
                  //   exclude one $id unique index
                  // - two reasons we don't append index hash for
                  //   transaction rollback
                  //   + index hash will enlarge the size of DPS record
                  //     against the origin transaction DPS record.
                  //   + In secondary nodes, it will allow duplicated keys
                  //     during transaction rollback, so the index hash is
                  //     useless
                  rc = newUnqIdxHashArray.prepare(
                        context->mbStat()->_uniqueIdxNum - 1, TRUE ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to prepare hash list for "
                               "new unique index [%u], rc: %d",
                               context->mbStat()->_uniqueIdxNum, rc ) ;
                  rc = oldUnqIdxHashArray.prepare(
                        context->mbStat()->_uniqueIdxNum - 1, FALSE ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to prepare hash list for "
                               "old unique index [%u], rc: %d",
                               context->mbStat()->_uniqueIdxNum, rc ) ;
                  pNewUnqIdxHashArray = &newUnqIdxHashArray ;
                  pOldUnqIdxHashArray = &oldUnqIdxHashArray ;
               }

               // reserved log-size
               rc = dpsUpdate2Record( fullName,
                                      oldMatch, oldChg, newMatch, newChg,
                                      oldShardingKey, newShardingKey,
                                      pNewUnqIdxHashArray, pOldUnqIdxHashArray,
                                      transID, preTransLsn,
                                      relatedLSN, pWriteMod, record ) ;

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

            rc = _pIdxSU->indexesUpdate( context, recordID._extent, obj, newobj,
                                         recordID, cb, FALSE, pHandler, writeGuard,
                                         modifier.getIdxHashBitmap(), pResult,
                                         pNewUnqIdxHashArray,
                                         pOldUnqIdxHashArray ) ;
            needUndoIndex = TRUE ;
            if ( rc )
            {
               PD_LOG ( PDWARNING, "Failed to update object(%s) index, rc: %d",
                        PD_SECURE_OBJ( newobj ), rc ) ;
               goto error ;
            }

            rc = context->getCollPtr()->updateRecord( recordID, newRecordData, cb ) ;
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
                     dmsRecordData peerData ;
                     if ( SDB_OK == fetch( context, pResult->getPeerRID(),
                                           peerData, cb ) )
                     {
                        BSONObj peerObj( peerData.data() ) ;
                        pResult->setPeerID( peerObj ) ;
                     }
                  }
               }

               PD_LOG ( PDERROR,
                        "Failed to update record from (%s) to (%s), rc: %d",
                        PD_SECURE_OBJ( obj ), PD_SECURE_OBJ( newobj ), rc ) ;
               goto error ;
            }

            context->mbStat()->_totalDataLen.sub( recordData.len() ) ;
            context->mbStat()->_totalOrgDataLen.sub( recordData.len() ) ;
            context->mbStat()->_totalDataLen.add( newRecordData.len() ) ;
            context->mbStat()->_totalOrgDataLen.add( newRecordData.len() ) ;

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

         //increase data write counter
         DMS_MON_OP_COUNT_INC( pMonAppCB, MON_DATA_WRITE, 1 ) ;
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
                  PD_SECURE_OBJ( oldMatch ), PD_SECURE_OBJ( oldChg ),
                  PD_SECURE_OBJ( newMatch ), PD_SECURE_OBJ( newChg ) ) ;

         PD_AUDIT_OP_WITHNAME( AUDIT_UPDATE, "UPDATE", AUDIT_OBJ_CL,
                               fullName, rc, "OldMatch:%s, OldChange:%s, "
                               "NewMatch:%s, NewChange:%s",
                               oldMatch.toString().c_str(),
                               oldChg.toString().c_str(),
                               newMatch.toString().c_str(),
                               newChg.toString().c_str() ) ;

         if ( isTransSupport( context ) )
         {
            info.enableTrans() ;
         }
         rc = _logDPS( dpscb, info, cb, context, recordID._extent,
                       recordID._offset, FALSE, DMS_FILE_DATA ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert update record into log, "
                      "rc: %d", rc ) ;
      }
      else if ( cb->getLsnCount() > 0 )
      {
         context->mbStat()->updateLastLSNWithComp( cb->getEndLsn(),
                                                   DMS_FILE_DATA,
                                                   cb->isDoRollback() ) ;
         cb->setDataExInfo( fullName, logicalID(), context->clLID(),
                            recordID._extent, recordID._offset ) ;
      }

      if ( handler )
      {
         handler->done( DMS_EXTOPR_TYPE_UPDATE, cb ) ;
      }

      rc = writeGuard.commit() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Failed to commit write guard, rc: %d", rc ) ;
         ossPanic() ;
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
      if( needUndoIndex )
      {
         ctrlAssist.switchToUndo() ;
         BSONObj oriObj( recordData.data() ) ;
         BSONObj newObj( newRecordData.data() ) ;
         // rollback the change on index by switching obj and oriObj
         INT32 rc1 = _pIdxSU->indexesUpdate( context, recordID._extent,
                                             newObj, oriObj, recordID, cb,
                                             TRUE, NULL, writeGuard,
                                             modifier.getIdxHashBitmap() ) ;
         if ( rc1 )
         {
            if ( !ctrlAssist.isUndoFinished() )
            {
               // undo is not finished
               if ( SDB_OK == cb->getTransRC() )
               {
                  cb->setTransRC( rc ) ;
               }
            }
            PD_LOG ( PDERROR, "Failed to rollback update due to rc %d", rc1 ) ;
         }
      }
      if ( handler )
      {
         handler->abortOperation( DMS_EXTOPR_TYPE_UPDATE, cb ) ;
      }
      {
         INT32 tmpRC = writeGuard.abort() ;
         if ( tmpRC )
         {
            PD_LOG( PDWARNING, "Failed to abort write guard, rc: %d", tmpRC ) ;
         }
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
                                       dmsRecordData &recordData,
                                       pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

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

         // if this record is overflow from
         rc = extractData( context, recordID, cb, recordData ) ;
         PD_RC_CHECK( rc, PDERROR, "Extract record data failed, rc: %d", rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_INCMBSTAT, "_dmsStorageDataCommon::increaseMBStat" )
   void _dmsStorageDataCommon::increaseMBStat ( utilCLUniqueID clUniqueID,
                                                dmsMBStatInfo * mbStat,
                                                UINT64 delta,
                                                _pmdEDUCB * cb )
   {
      SDB_ASSERT( NULL != mbStat, "mb stat should not be NULL" ) ;
      SDB_ASSERT( NULL != cb, "EDUCB should not be NULL" ) ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_INCMBSTAT ) ;

      // update meta-block statistics
      mbStat->_totalRecords.add( delta ) ;

      // update meta-block statistics for transaction RC counter
      if ( cb->isDoReplay() || cb->isTakeOverTransRB() )
      {
         // two special cases need update RC counter with record counter
         // - replay thread in secondary node
         // - primary switch is running
         // in these cases, no transactions can query the collection, so it
         // is safe to update the RC counter
         ossScopedLock lock( &_mbStatLatch ) ;
         mbStat->_rcTotalRecords.swap( mbStat->_totalRecords.fetch() ) ;
      }
      else if ( cb->isInTransRollback() )
      {
         // in transaction rollback, do nothing
      }
      else if ( cb->isTransaction() )
      {
         // in transaction, update the RC counter in transaction executor
         // first
         if ( !cb->getTransExecutor()->incMBTotalRecords(
                           clUniqueID, &( mbStat->_rcTotalRecords ), delta ) )
         {
            // failed to update the RC counter in transaction executor, which
            // means the collection unique ID may be invalid, update the
            // RC counter directly
            mbStat->_rcTotalRecords.add( delta ) ;
         }
      }
      else
      {
         // not a transaction, update the RC counter
         mbStat->_rcTotalRecords.add( delta ) ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_INCMBSTAT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON_DECMBSTAT, "_dmsStorageDataCommon::decreaseMBStat" )
   void _dmsStorageDataCommon::decreaseMBStat ( utilCLUniqueID clUniqueID,
                                                dmsMBStatInfo * mbStat,
                                                UINT64 delta,
                                                _pmdEDUCB * cb )
   {
      SDB_ASSERT( NULL != mbStat, "mb stat should not be NULL" ) ;
      SDB_ASSERT( NULL != cb, "EDUCB should not be NULL" ) ;

      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON_DECMBSTAT ) ;

      // update meta-block statistics
      mbStat->_totalRecords.sub( delta ) ;

      // update meta-block statistics for transaction RC counter
      if ( cb->isDoReplay() || sdbGetTransCB()->isDoRollback() )
      {
         // two special cases need update RC counter with record counter
         // - replay thread in secondary node
         // - primary switch is running
         // in these cases, no transactions can query the collection, so it
         // is safe to update the RC counter
         ossScopedLock lock( &_mbStatLatch ) ;
         mbStat->_rcTotalRecords.swap( mbStat->_totalRecords.fetch() ) ;
      }
      else if ( cb->isInTransRollback() )
      {
         // in transaction rollback, do nothing
      }
      else if ( cb->isTransaction() )
      {
         // in transaction, update the RC counter in transaction executor
         // first
         if ( !cb->getTransExecutor()->decMBTotalRecords(
                        clUniqueID, &( mbStat->_rcTotalRecords ), delta ) )
         {
            // failed to update the RC counter in transaction executor, which
            // means the collection unique ID may be invalid, update the
            // RC counter directly
            mbStat->_rcTotalRecords.sub( delta ) ;
         }
      }
      else
      {
         // not a transaction, update the RC counter
         mbStat->_rcTotalRecords.sub( delta ) ;
      }

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON_DECMBSTAT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSSTORAGEDATACOMMON__ONMBUPDATED, "_dmsStorageDataCommon::_onMBUpdated" )
   void _dmsStorageDataCommon::_onMBUpdated( UINT16 mbID )
   {
      PD_TRACE_ENTRY( SDB__DMSSTORAGEDATACOMMON__ONMBUPDATED ) ;

      SDB_ASSERT( mbID < DMS_MME_SLOTS, "mb ID is invalid" ) ;

      UINT64 updateTime = ossGetCurrentMilliseconds() ;

      _dmsMME->_mbList[ mbID ]._updateTime = updateTime ;
      _mbStatInfo[ mbID ]._updateTime = updateTime ;

      // update on storage unit
      _onHeaderUpdated( updateTime ) ;

      PD_TRACE_EXIT( SDB__DMSSTORAGEDATACOMMON__ONMBUPDATED ) ;
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

