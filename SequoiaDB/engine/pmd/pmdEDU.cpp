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

   Source File Name = pmdEDU.cpp

   Descriptive Name = Process MoDel Agent Engine Dispatchable Unit

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for EDU processing.
   EDU thread is a wrapper of all threads. It will call each entry function
   depends on the EDU type.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <stdio.h>
#include "pd.hpp"
#include "ossMem.hpp"
#include "pmdEDU.hpp"
#include "pmdEntryPoint.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "utilUniqueID.hpp"
#include "dpsUtil.hpp"
#include "msgConvertorImpl.hpp"
#include "msgMessageFormat.hpp"
#include <map>

namespace engine
{

   /*
      TOOL FUNCTIONS
   */
   const CHAR * getEDUStatusDesp ( EDU_STATUS status )
   {
      const CHAR *desp = "Unknown" ;

      switch ( status )
      {
         case PMD_EDU_CREATING :
            desp = "Creating" ;
            break ;
         case PMD_EDU_RUNNING :
            desp = "Running" ;
            break ;
         case PMD_EDU_WAITING :
            desp = "Waiting" ;
            break ;
         case PMD_EDU_IDLE :
            desp = "Idle" ;
            break ;
         default :
            break ;
      }

      return desp ;
   }

   /*
      _pmdEDUCB implement
   */
   _pmdEDUCB::_pmdEDUCB( _pmdEDUMgr *mgr, INT32 type )
   :_mutex( MON_LATCH_PMDEDUCB_MUTEX ),
    _dumpTransCount( 0 )
#if defined ( SDB_ENGINE )
    ,_transExecutor( this, pmdGetKRCB()->getMonMgr() ),
    _urgentEventCount( 0 ),
    _needCheckUrgentQueue( TRUE )
#endif // SDB_ENGINE

   {
      _eduMgr           = mgr ;
      _eduID            = PMD_INVALID_EDUID ;
      _tid              = 0 ;
      _hasSetTid        = FALSE ;
      _status           = PMD_EDU_UNKNOW ;
      _eduType          = type ;
      _isLocked         = FALSE ;
      _ctrlFlag         = 0 ;
      _isInterruptSelf  = FALSE ;
      _interruptRC      = SDB_OK ;
      _writingDB        = FALSE ;
      _writingID        = 0 ;
      _blockType        = EDU_BLOCK_NONE ;
      _processEventCount= 0 ;
      ossMemset( _name, 0, sizeof( _name ) ) ;
      ossMemset( _source, 0, sizeof( _source ) ) ;
      _threadHdl        = 0 ;
      _pSession         = NULL ;
      _pRemoteSite      = NULL ;
      _pCompressBuff    = NULL ;
      _compressBuffLen  = 0 ;
      _pUncompressBuff  = NULL ;
      _uncompressBuffLen= 0 ;
      _isDoTransRollback= FALSE ;
      _isTakeOverTransRB= FALSE ;
      _pClientSock      = NULL ;

      _alignedMem       = NULL ;
      _alignedMemSize   = 0 ;

      _pBuffer          = NULL ;
      _buffSize         = 0 ;

      _beginLsn         = 0 ;
      _endLsn           = 0 ;
      _lsnNumber        = 0 ;
      _doRollback       = FALSE ;

      _curTransLSN      = DPS_INVALID_LSN_OFFSET ;
      _curTransID       = DPS_INVALID_TRANS_ID ;

#if defined (_LINUX)
      _threadID         = 0 ;
#endif // _LINUX

#if defined ( SDB_ENGINE )
      _relatedTransLSN  = DPS_INVALID_LSN_OFFSET ;
      _transRC          = SDB_OK ;
      _transStatus      = DPS_TRANS_UNKNOWN ;

      _curRequestID     = 1 ;
      _confChangeID     = 0 ;

      _pRemoteOperator  = NULL ;
#endif // SDB_ENGINE

      _pErrorBuff = (CHAR *)SDB_OSS_MALLOC( EDU_ERROR_BUFF_SIZE + 1 ) ;
      if ( _pErrorBuff )
      {
         ossMemset( _pErrorBuff, 0, EDU_ERROR_BUFF_SIZE + 1 ) ;
      }
      ossMemset( _doingBuff, 0, sizeof( _doingBuff ) ) ;

      _curAutoTransCtxID = -1 ;
      _currentContextID = -1 ;

      ossMemset( _curProcessName, 0, sizeof( _curProcessName ) ) ;
      ossMemset( _curMainCLName, 0, sizeof( _curMainCLName ) ) ;

      _pMemPool = NULL ;
      _monQueryCB = NULL ;

      _isAffectGIndex = FALSE ;

      _doReplay = FALSE ;
   }

   _pmdEDUCB::~_pmdEDUCB ()
   {
      if ( _pErrorBuff )
      {
         SDB_OSS_FREE ( _pErrorBuff ) ;
         _pErrorBuff = NULL ;
      }

      clear() ;

      _pMemPool = NULL ;
   }

   void _pmdEDUCB::clear()
   {
      // clear all queue msg
      pmdEDUEvent data ;
      while ( _queue.try_pop( data ) )
      {
         pmdEduEventRelease( data, this ) ;
      }
      _processEventCount = 0 ;
      _clearUrgentEvents() ;
      ossMemset( _name, 0, sizeof( _name ) ) ;
      ossMemset( _source, 0, sizeof( _source ) ) ;
      _userName = "" ;
      _passWord = "" ;
      _isLocked = FALSE ;
      _dumpTransCount.swap( 0 ) ;

      _ctrlFlag = 0 ;
      _isInterruptSelf = FALSE ;
      _interruptRC = SDB_OK ;
      resetLsn() ;
      resetMon() ;
      writingDB( FALSE ) ;
      unsetBlock() ;
      releaseAlignedBuff() ;
      releaseBuffer() ;

      resetLocks() ;

#if defined ( SDB_ENGINE )
      clearTransInfo() ;
      SAFE_OSS_DELETE( _pRemoteOperator ) ;
      _remoteOpCtrl.reset() ;
#endif // SDB_ENGINE

      // release buff
      if ( _pCompressBuff )
      {
         releaseBuff( _pCompressBuff ) ;
         _pCompressBuff = NULL ;
      }
      _compressBuffLen = 0 ;
      if ( _pUncompressBuff )
      {
         releaseBuff( _pUncompressBuff ) ;
         _pUncompressBuff = NULL ;
      }
      _uncompressBuffLen = 0 ;

      _curAutoTransCtxID = -1 ;
      if ( _pMemPool )
      {
         _pMemPool->clear() ;
         _pMemPool->setName( "PoolIdle" ) ;
      }

      _operator.clearMsg() ;
   }

   void _pmdEDUCB::shrink()
   {
      if ( _pMemPool )
      {
         _pMemPool->shrink() ;
      }
   }

   string _pmdEDUCB::toString() const
   {
      stringstream ss ;

      try
      {
         ss << "ID: " << _eduID << ", Type: " << _eduType << "["
            << getEDUName( _eduType ) << "], TID: " << _tid ;

         if ( _pSession )
         {
            ss << ", Session: " << _pSession->sessionName() ;
         }
         return ss.str() ;
      }
      catch( std::exception &e )
      {
         try
         {
            ss.str( "" ) ;
            ss << e.what() ;
            return ss.str() ;
         }
         catch (...)
         {
            return "Out-of-memory" ;
         }
      }
   }

   const CHAR* _pmdEDUCB::getName () const
   {
      return _name ;
   }

   const CHAR* _pmdEDUCB::getSource() const
   {
      return _source ;
   }

   void _pmdEDUCB::attachSession( ISession *pSession )
   {
      _pSession = pSession ;
   }

   void _pmdEDUCB::detachSession()
   {
#if defined ( SDB_ENGINE )
      SAFE_OSS_DELETE( _pRemoteOperator ) ;
      _remoteOpCtrl.reset() ;
#endif // SDB_ENGINE

      ossScopedLock lock( &_mutex, EXCLUSIVE ) ;
      _pSession = NULL ;
   }

   void _pmdEDUCB::attachRemoteSite( IRemoteSite *pSite )
   {
      _pRemoteSite = pSite ;
   }

   void _pmdEDUCB::detachRemoteSite()
   {
      _pRemoteSite = NULL ;
   }

   void _pmdEDUCB::setType ( INT32 type )
   {
      _eduType = type ;
   }

   void _pmdEDUCB::interrupt( BOOLEAN onlySelf, INT32 interruptRC )
   {
      _ctrlFlag |= EDU_CTRL_INTERRUPTED ;
      _isInterruptSelf = onlySelf ;
      _interruptRC = interruptRC ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_DISCONNECT, "_pmdEDUCB::disconnect" )
   void _pmdEDUCB::disconnect ()
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_DISCONNECT );
      interrupt () ;
      _ctrlFlag |= EDU_CTRL_DISCONNECTED ;
      // reset return code to disconnected
      _interruptRC = SDB_APP_DISCONNECT ;
      postEvent ( pmdEDUEvent ( PMD_EDU_EVENT_TERM ) ) ;
      PD_TRACE_EXIT ( SDB__PMDEDUCB_DISCONNECT );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_FORCE, "_pmdEDUCB::force" )
   void _pmdEDUCB::force ()
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_FORCE );
      disconnect () ;
      _ctrlFlag |= EDU_CTRL_FORCED ;
      // reset return code to forced
      _interruptRC = SDB_APP_FORCED ;
      PD_TRACE_EXIT ( SDB__PMDEDUCB_FORCE );
   }

   void _pmdEDUCB::resetInterrupt ()
   {
      _ctrlFlag &= ~EDU_CTRL_INTERRUPTED ;
      _isInterruptSelf = FALSE ;
      _interruptRC = SDB_OK ;
   }

   void _pmdEDUCB::resetDisconnect ()
   {
      resetInterrupt () ;
      _ctrlFlag &= ~EDU_CTRL_DISCONNECTED ;
   }

   void _pmdEDUCB::setUserInfo( const string & userName,
                                const string & password )
   {
      _userName = userName ;
      _passWord = password ;
   }

   void _pmdEDUCB::setName ( const CHAR * name )
   {
      ossStrncpy ( _name, name, PMD_EDU_NAME_LENGTH ) ;
      _name[PMD_EDU_NAME_LENGTH] = 0 ;
   }

   void _pmdEDUCB::setSource( const CHAR *pSource )
   {
      ossStrncpy( _source, pSource, PMD_EDU_NAME_LENGTH ) ;
      _source[ PMD_EDU_NAME_LENGTH ] = 0 ;
   }

   CHAR *_pmdEDUCB::_getBuffInfo ( EDU_INFO_TYPE type, UINT32 & size )
   {
      CHAR *buff = NULL ;
      switch ( type )
      {
         case EDU_INFO_ERROR :
            buff = _pErrorBuff ;
            size = EDU_ERROR_BUFF_SIZE ;
            break ;
         case EDU_INFO_DOING :
            buff = _doingBuff ;
            size = EDU_DOING_BUFF_SIZE ;
         default :
            break ;
      }

      return buff ;
   }

   INT32 _pmdEDUCB::appendInfo( EDU_INFO_TYPE type, const CHAR * format, ...)
   {
      INT32 rc = SDB_OK ;
      //do not have info yet, do nothing
      if ( !getInfo ( type ) )
      {
         goto done ;
      }

      {
         UINT32 buffSize = 0 ;
         CHAR *buff = _getBuffInfo ( type, buffSize ) ;
         INT32 len = 0 ;
         CHAR *leftBuff = NULL ;
         INT32 leftLen = 0 ;

         if ( NULL == buff || buffSize == 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         len = ossStrlen( buff ) ;
         if ( (INT32)buffSize <= len )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         leftLen = buffSize - len ;
         leftBuff = buff + len ;

         va_list ap ;
         va_start ( ap, format ) ;
         vsnprintf ( leftBuff, leftLen, format, ap ) ;
         va_end ( ap ) ;

         leftBuff[ leftLen ] = 0 ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_PRINTINFO, "_pmdEDUCB::printInfo" )
   INT32 _pmdEDUCB::printInfo ( EDU_INFO_TYPE type, const CHAR * format, ... )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_PRINTINFO );
      //already exist, return ok
      if ( getInfo ( type ) )
      {
         goto done ;
      }

      {
      UINT32 buffSize = 0 ;
      CHAR *buff = _getBuffInfo ( type, buffSize ) ;

      if ( NULL == buff || buffSize == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      va_list ap ;
      va_start ( ap, format ) ;
      vsnprintf ( buff, buffSize, format, ap ) ;
      va_end ( ap ) ;

      buff[ buffSize ] = 0 ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__PMDEDUCB_PRINTINFO, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_GETINFO, "_pmdEDUCB::getInfo" )
   const CHAR *_pmdEDUCB::getInfo ( EDU_INFO_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_GETINFO );
      UINT32 buffSize = 0 ;
      CHAR *buff = _getBuffInfo ( type, buffSize ) ;
      if ( buff && buff[0] != 0 )
      {
         PD_TRACE_EXIT ( SDB__PMDEDUCB_GETINFO );
         return buff ;
      }
      PD_TRACE_EXIT ( SDB__PMDEDUCB_GETINFO );
      return NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_RESETINFO, "_pmdEDUCB::resetInfo" )
   void _pmdEDUCB::resetInfo ( EDU_INFO_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_RESETINFO );
      UINT32 buffSize = 0 ;
      CHAR *buff = _getBuffInfo ( type, buffSize ) ;
      if ( buff )
      {
         buff[0] = 0 ;
      }
      PD_TRACE_EXIT ( SDB__PMDEDUCB_RESETINFO );
   }

   CHAR* _pmdEDUCB::getCompressBuff( UINT32 len )
   {
      if ( _compressBuffLen < len )
      {
         if ( _pCompressBuff )
         {
            releaseBuff( _pCompressBuff ) ;
            _pCompressBuff = NULL ;
         }
         _compressBuffLen = 0 ;

         allocBuff( len, &_pCompressBuff, &_compressBuffLen ) ;
      }

      return _pCompressBuff ;
   }

   CHAR* _pmdEDUCB::getUncompressBuff( UINT32 len )
   {
      if ( _uncompressBuffLen < len )
      {
         if ( _pUncompressBuff )
         {
            releaseBuff( _pUncompressBuff ) ;
            _pUncompressBuff = NULL ;
         }
         _uncompressBuffLen = 0 ;

         allocBuff( len, &_pUncompressBuff, &_uncompressBuffLen ) ;
      }

      return _pUncompressBuff ;
   }

   /*
      Interface impelent
   */
   void *_pmdEDUCB::getAlignedBuff( UINT32 size,
                                    UINT32 *pRealSize,
                                    UINT32 alignment )
   {
      SDB_ASSERT( alignment == OSS_FILE_DIRECT_IO_ALIGNMENT,
                  "rewrite this function if u want to use new alignment" ) ;
      if ( _alignedMemSize < size )
      {
         if ( NULL != _alignedMem )
         {
            SDB_OSS_ORIGINAL_FREE( _alignedMem ) ;
            _alignedMemSize = 0 ;
            _alignedMem = NULL ;
         }

         size = ossRoundUpToMultipleX( size, alignment ) ;
         _alignedMem = ossAlignedAlloc( alignment, size ) ;
         if ( NULL != _alignedMem )
         {
            _alignedMemSize = size ;
         }
      }

      if ( pRealSize )
      {
         *pRealSize = _alignedMemSize ;
      }

      return _alignedMem ;
   }

   void _pmdEDUCB::releaseAlignedBuff()
   {
      if ( NULL != _alignedMem )
      {
         SDB_OSS_ORIGINAL_FREE( _alignedMem ) ;
         _alignedMem = NULL ;
         _alignedMemSize = 0 ;
      }
   }

   CHAR* _pmdEDUCB::getBuffer( UINT32 len )
   {
      if ( _buffSize < len )
      {
         releaseBuffer() ;
         allocBuff( len, &_pBuffer, &_buffSize ) ;
      }

      return _pBuffer ;
   }

   void _pmdEDUCB::releaseBuffer()
   {
      if ( _pBuffer )
      {
         releaseBuff( _pBuffer ) ;
         _pBuffer = NULL ;
         _buffSize = 0 ;
      }
   }

   INT32 _pmdEDUCB::allocBuff( UINT32 len,
                               CHAR **ppBuff,
                               UINT32 *pRealSize )
   {
      INT32 rc = SDB_OK ;

      *ppBuff = (CHAR*)SDB_THREAD_ALLOC2( len, pRealSize ) ;
      if ( !(*ppBuff) )
      {
         PD_LOG( PDERROR, "Edu[%s] malloc memory[size: %u] failed",
                 toString().c_str(), len ) ;
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdEDUCB::reallocBuff( UINT32 len,
                                 CHAR **ppBuff,
                                 UINT32 *pRealSize )
   {
      INT32 rc = SDB_OK ;

      *ppBuff = (CHAR*)SDB_THREAD_REALLOC2( (void*)(*ppBuff), len, pRealSize ) ;
      if ( !(*ppBuff) )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to realloc memory, size: %u", len ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdEDUCB::releaseBuff( CHAR *pBuff )
   {
      SDB_THREAD_FREE( pBuff ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_ISINT, "_pmdEDUCB::isInterrupted" )
   BOOLEAN _pmdEDUCB::isInterrupted ( BOOLEAN onlyFlag )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_ISINT );
      BOOLEAN ret = FALSE ;

      /// check urgent events
      checkUrgentEvents() ;

      // mask interrupt while doing rollback
      if ( !onlyFlag && _isDoTransRollback )
      {
         goto done;
      }
      if ( _ctrlFlag & EDU_CTRL_INTERRUPTED )
      {
         ret = TRUE ;
         goto done ;
      }
      else if ( !onlyFlag && _pClientSock )
      {
         if ( _pClientSock->isClosed() )
         {
            _ctrlFlag |= ( EDU_CTRL_INTERRUPTED | EDU_CTRL_DISCONNECTED ) ;
            _isInterruptSelf = FALSE ;
            ret = TRUE ;
         }
         else
         {
            INT32 receivedLen ;
            MsgHeader header ;
            INT32 opCode = 0 ;
            INT32 headerLen = 0 ;
            SDB_PROTOCOL_VERSION version =
                  _pSession->getClient()->getClientVersion() ;
            headerLen = ( SDB_PROTOCOL_VER_1 == version ) ?
                  sizeof(MsgHeaderV1) : sizeof(MsgHeader) ;

            INT32 rc = _pClientSock->recv( (CHAR*)&header , headerLen,
                                           receivedLen, 0, MSG_PEEK, TRUE, FALSE ) ;
            if ( receivedLen >= headerLen )
            {
               opCode = ( SDB_PROTOCOL_VER_1 == version ) ?
                        ( (MsgHeaderV1 *)(&header) )->opCode : header.opCode ;
            }

            if ( ( receivedLen >= headerLen &&
                   MSG_BS_DISCONNECT == opCode ) ||
                 SDB_NETWORK_CLOSE == rc ||
                 SDB_NETWORK == rc )
            {
               _ctrlFlag |= ( EDU_CTRL_INTERRUPTED | EDU_CTRL_DISCONNECTED ) ;
               _isInterruptSelf = FALSE ;
               ret = TRUE ;
            }
            else if ( receivedLen >= headerLen &&
                      ( MSG_BS_INTERRUPTE == opCode ||
                        MSG_BS_INTERRUPTE_SELF == opCode ) )
            {
               _ctrlFlag |= EDU_CTRL_INTERRUPTED ;
               _isInterruptSelf = MSG_BS_INTERRUPTE_SELF == opCode ?
                                  TRUE : FALSE ;
               ret = TRUE ;
            }
         }
      }
   done :
      PD_TRACE1 ( SDB__PMDEDUCB_ISINT, PD_PACK_INT(ret) );
      PD_TRACE_EXIT ( SDB__PMDEDUCB_ISINT );
      return ret ;
   }

   BOOLEAN _pmdEDUCB::isOnlySelfWhenInterrupt() const
   {
      return _isInterruptSelf ;
   }

   INT32 _pmdEDUCB::getInterruptRC() const
   {
      return _interruptRC ;
   }

   BOOLEAN _pmdEDUCB::isDisconnected ()
   {
      return ( _ctrlFlag & EDU_CTRL_DISCONNECTED ) ? TRUE : FALSE ;
   }

   BOOLEAN _pmdEDUCB::isForced ()
   {
      return ( _ctrlFlag & EDU_CTRL_FORCED ) ? TRUE : FALSE ;
   }

   void _pmdEDUCB::writingDB( BOOLEAN writing, const CHAR* name )
   {
      if ( _writingDB == writing ) return ;

      if ( writing )
      {
         _writingDB = TRUE ;
         _writingID = pmdAcquireGlobalID() ;
      }
      else if ( !writing && 0 == getLockItem(SDB_LOCK_DMS)->lockCount() )
      {
         _writingDB = FALSE ;
         _writingID = 0 ;
      }
   }

   void _pmdEDUCB::setBlock( EDU_BLOCK_TYPE type, const CHAR *pBlockDesp )
   {
      _blockType = type ;
      /// reset
      resetInfo( EDU_INFO_DOING ) ;

      if ( pBlockDesp )
      {
         printInfo( EDU_INFO_DOING, "%s", pBlockDesp ) ;
      }
   }

   void _pmdEDUCB::unsetBlock()
   {
      _blockType = EDU_BLOCK_NONE ;

      resetInfo( EDU_INFO_DOING ) ;
   }

   EDU_BLOCK_TYPE _pmdEDUCB::getBlockType() const
   {
      return _blockType ;
   }

   BOOLEAN _pmdEDUCB::isBlocked() const
   {
      return EDU_BLOCK_NONE != _blockType ? TRUE : FALSE ;
   }

   void _pmdEDUCB::resetLsn()
   {
      _beginLsn = ~0 ;
      _endLsn = ~0 ;
      _lsnNumber = 0 ;
      _doRollback = FALSE ;
   }

   void _pmdEDUCB::insertLsn( UINT64 lsn, BOOLEAN isRollback )
   {
      if ( _beginLsn == (UINT64)~0 )
      {
         _beginLsn = lsn ;
      }
      _endLsn = lsn ;
      _lsnNumber++ ;
      _doRollback = isRollback ;
   }

   void _pmdEDUCB::setTransID( UINT64 transID )
   {
#if defined ( SDB_ENGINE )
      if ( DPS_INVALID_TRANS_ID == _curTransID &&
           DPS_INVALID_TRANS_ID != transID )
      {
         /// begin trans
         _transStatus = DPS_TRANS_DOING ;
         _transRC = SDB_OK ;
      }
      else if ( DPS_INVALID_TRANS_ID == transID )
      {
         /// end trans
         _transStatus = DPS_TRANS_UNKNOWN ;
      }
#endif //SDB_ENGINE
      if ( DPS_INVALID_TRANS_ID == transID )
      {
         _curAutoTransCtxID = -1 ;
      }
      _curTransID = transID ;
   }

   void _pmdEDUCB::setCurTransLsn( UINT64 lsn )
   {
      _curTransLSN = lsn ;
   }

   BOOLEAN _pmdEDUCB::contextInsert( INT64 contextID )
   {
      BOOLEAN result = FALSE ;
      try
      {
         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
         _contextList.insert ( contextID ) ;
         setCurrentContextID( contextID ) ;
         result = TRUE ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to insert context, occur exception %s",
                 e.what() ) ;
      }
      return result ;
   }

   void _pmdEDUCB::contextDelete( INT64 contextID )
   {
      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
      _contextList.erase ( contextID ) ;
      if ( _curAutoTransCtxID == contextID )
      {
         _curAutoTransCtxID = -1 ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_CONTXTPEEK, "_pmdEDUCB::contextPeek" )
   INT64 _pmdEDUCB::contextPeek()
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_CONTXTPEEK );
      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
      SINT64 contextID = -1 ;
      SET_CONTEXT::const_iterator it ;
      if ( _contextList.empty() )
      {
         goto done ;
      }
      it = _contextList.begin() ;
      contextID = (*it) ;
      _contextList.erase(it) ;

      if ( _curAutoTransCtxID == contextID )
      {
         _curAutoTransCtxID = -1 ;
      }

   done :
      PD_TRACE1 ( SDB__PMDEDUCB_CONTXTPEEK, PD_PACK_LONG(contextID) );
      PD_TRACE_EXIT ( SDB__PMDEDUCB_CONTXTPEEK );
      return contextID ;
   }

   BOOLEAN _pmdEDUCB::contextFind( INT64 contextID )
   {
      ossScopedLock _lock(
            pmdGetThreadEDUCB() == this ? NULL : &_mutex, SHARED ) ;
      return _contextList.end() != _contextList.find( contextID ) ;
   }

   UINT32 _pmdEDUCB::contextNum()
   {
      ossScopedLock _lock(
            pmdGetThreadEDUCB() == this ? NULL : &_mutex, SHARED ) ;
      return _contextList.size() ;
   }

   BOOLEAN _pmdEDUCB::isLogTimeOn() const
   {
#if defined ( SDB_ENGINE )
      return _logConfig.isLogTimeOn() ;
#else
      return FALSE ;
#endif
   }

   UINT32 _pmdEDUCB::getLogWriteMod() const
   {
#if defined ( SDB_ENGINE )
      return _logConfig.getLogWriteMod() ;
#else
      return 0 ;
#endif
   }

   void _pmdEDUCB::setCurAutoTransCtxID( INT64 contextID )
   {
      _curAutoTransCtxID = contextID ;
   }

   INT64 _pmdEDUCB::getCurAutoTransCtxID() const
   {
      return _curAutoTransCtxID ;
   }

   BOOLEAN _pmdEDUCB::isAffectGIndex() const
   {
      return _isAffectGIndex ;
   }

   void _pmdEDUCB::setIsAffectGIndex( BOOLEAN isAffect )
   {
      _isAffectGIndex = isAffect ;
   }

   BOOLEAN _pmdEDUCB::isTransRBPending() const
   {
      return DPS_TRANS_IS_RBPENDING( _curTransID ) ? TRUE : FALSE ;
   }

   void _pmdEDUCB::setTransRBPending()
   {
      SDB_ASSERT( DPS_TRANS_IS_ROLLBACK( _curTransID ),
                  "Current transaction is not rollback" ) ;
      DPS_TRANS_SET_RBPENDING( _curTransID ) ;
   }

   void _pmdEDUCB::clearTransRBPending()
   {
      SDB_ASSERT( DPS_TRANS_IS_ROLLBACK( _curTransID ),
                  "Current transaction is not rollback" ) ;
      DPS_TRANS_CLEAR_RBPENDING( _curTransID ) ;
   }

   void _pmdEDUCB::_contextCopy( _pmdEDUCB::SET_CONTEXT &contextList )
   {
      try
      {
         contextList = _contextList ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to copy context list, "
                 "occur exception %s", e.what() ) ;
      }
   }

   void _pmdEDUCB::contextCopy( _pmdEDUCB::SET_CONTEXT &contextList )
   {
      ossScopedLock _lock(
                  pmdGetThreadEDUCB() == this ? NULL : &_mutex, SHARED ) ;
      _contextCopy( contextList ) ;
   }

   void _pmdEDUCB::initMonAppCB()
   {
      _monApplCB.reset() ;
      if ( _monCfgCB.timestampON )
      {
         _monApplCB.recordConnectTimestamp() ;
      }
   }

   void _pmdEDUCB::initConf()
   {
#if defined ( SDB_ENGINE )
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      _confChangeID = optCB->getChangeID() ;
      if ( optCB->transactionOn() )
      {
         _transExecutor.initTransConf( optCB->transIsolation(),
                                       optCB->transTimeout() * OSS_ONE_SEC,
                                       optCB->transLockwait(),
                                       optCB->transAutoCommit(),
                                       optCB->transAutoRollback(),
                                       optCB->transUseRBS(),
                                       optCB->transRCCount(),
                                       optCB->transAllowLockEscalation(),
                                       optCB->transMaxLockNum(),
                                       optCB->transMaxLogSpaceRatio(),
                                       optCB->getTotalLogSpace() ) ;
      }
      else
      {
         _transExecutor.setTransAutoCommit( FALSE, FALSE ) ;
      }
      _logConfig.updateConf( optCB->logTimeOn(),
                             optCB->logWriteMod() ) ;

      // make sure meta-block statistics are cleared
      if ( !_transExecutor.isMBStatsEmpty() )
      {
         PD_LOG( PDWARNING, "transaction mb statistics is not empty" ) ;
         _transExecutor.clearMBStats() ;
      }
#endif //SDB_ENGINE
   }

   void _pmdEDUCB::initOperator()
   {
      MsgGlobalID globalID ;
      MsgQueryID queryID ;
      UINT16 nodeID = pmdGetNodeID().columns.nodeID ;
      queryID.init( _tid, nodeID, (UINT16)ossRand() ) ;
      globalID.set( queryID, 0 ) ;
      _operator.updateGlobalID( globalID ) ;
   }

   void _pmdEDUCB::updateConf()
   {
#if defined ( SDB_ENGINE )
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      UINT32 confChangeID = optCB->getChangeID() ;

      if ( confChangeID != _confChangeID )
      {
         BOOLEAN needUpdateChangeID = TRUE ;
         if ( optCB->transactionOn() )
         {
            // update transaction config
            if ( !_transExecutor.updateTransConf(
                                          optCB->transIsolation(),
                                          optCB->transTimeout() * OSS_ONE_SEC,
                                          optCB->transLockwait(),
                                          optCB->transAutoCommit(),
                                          optCB->transAutoRollback(),
                                          optCB->transUseRBS(),
                                          optCB->transRCCount(),
                                          optCB->transAllowLockEscalation(),
                                          optCB->transMaxLockNum(),
                                          optCB->transMaxLogSpaceRatio(),
                                          optCB->getTotalLogSpace() ) )
            {
               // failed to update, wait for next round
               needUpdateChangeID = FALSE ;
            }
         }

         // update DPS log config
         if ( !_logConfig.updateConf( optCB->logTimeOn(),
                                      optCB->logWriteMod(),
                                      isTransaction() ) )
         {
            // failed to update, wait for next round
            needUpdateChangeID = FALSE ;
         }

         if ( needUpdateChangeID )
         {
            _confChangeID = confChangeID ;
         }
      }
#endif //SDB_ENGINE
   }

   void _pmdEDUCB::incEventCount( UINT32 step )
   {
      _processEventCount += step ;
   }

   UINT32 _pmdEDUCB::getQueSize()
   {
      return _queue.size() ;
   }

   sdbLockItem* _pmdEDUCB::getLockItem( SDB_LOCK_TYPE lockType )
   {
      SDB_ASSERT( lockType >= SDB_LOCK_DMS && lockType < SDB_LOCK_MAX,
                  "lockType error" ) ;
      return &_lockInfo[ (INT32)lockType ] ;
   }

   void _pmdEDUCB::_clearUrgentEvents()
   {
#if defined ( SDB_ENGINE )
      pmdEDUEvent data ;
      while ( _urgentQueue.try_pop( data ) )
      {
         _urgentEventCount.dec() ;
         pmdEduEventRelease( data, this ) ;
      }
#endif
   }

   void _pmdEDUCB::enableCheckUrgentEvent()
   {
#if defined ( SDB_ENGINE )
      _needCheckUrgentQueue = TRUE ;
#endif
   }

   void _pmdEDUCB::disableCheckUrgentEvent()
   {
#if defined ( SDB_ENGINE )
      _needCheckUrgentQueue = FALSE ;
#endif
   }

   BOOLEAN _pmdEDUCB::needCheckUrgentEvent() const
   {
#if defined ( SDB_ENGINE )
      return _needCheckUrgentQueue ;
#else
      return FALSE ;
#endif
   }

   void _pmdEDUCB::checkUrgentEvents()
   {
#if defined ( SDB_ENGINE )
      if ( needCheckUrgentEvent() &&
           _urgentEventCount.peek() > 0 )
      {
         // shield to avoid calling check urgent event recursively
         pmdUrgentEventShield _shield( this ) ;

         pmdEDUEvent event ;
         IContextMgr *ctxMgr = pmdGetKRCB()->getContextMgr() ;
         SDB_ASSERT( NULL != ctxMgr, "context manager should be valid" ) ;
         while ( _urgentQueue.try_pop( event ) )
         {
            _urgentEventCount.dec() ;
            ++ _processEventCount ;
            if ( PMD_EDU_EVENT_KILLCONTEXT == event._eventType &&
                 NULL != ctxMgr )
            {
               INT64 contextID = (INT64)( event._userData ) ;
               ctxMgr->contextDelete( contextID, this ) ;
            }
            pmdEduEventRelease( event, this ) ;
         }
      }
#endif // SDB_ENGINE
   }

   void _pmdEDUCB::assertLocks()
   {
      for ( INT32 i = 0 ; i < SDB_LOCK_MAX ; ++i )
      {
         SDB_ASSERT( 0 == _lockInfo[i].lockCount(),
                     "Lock count must be 0" ) ;
      }

#if defined ( SDB_ENGINE )
      _transExecutor.assertLocks() ;
#endif //SDB_ENGINE
   }

   void _pmdEDUCB::resetLocks()
   {
      for ( INT32 i = 0 ; i < SDB_LOCK_MAX ; ++i )
      {
         _lockInfo[i].reset() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB___PMDEDUCB_DUMPINFO, "_pmdEDUCB::dumpInfo" )
   void _pmdEDUCB::dumpInfo ( monEDUSimple &simple )
   {
      PD_TRACE_ENTRY ( SDB___PMDEDUCB_DUMPINFO );
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      simple._eduStatus[0] = 0 ;
      simple._eduType[0] = 0 ;
      simple._eduName[0] = 0 ;
      simple._source[0] = 0 ;
      simple._eduID = _eduID ;
      simple._tid = _tid ;
      ossStrncpy ( simple._eduStatus, getEDUStatusDesp(_status),
                   MON_EDU_STATUS_SZ ) ;
      ossStrncpy ( simple._eduType, getEDUName (_eduType), MON_EDU_TYPE_SZ ) ;
      simple._eduType[ MON_EDU_TYPE_SZ ] = 0 ;
      ossStrncpy ( simple._eduName, _name, MON_EDU_NAME_SZ ) ;
      simple._eduName[ MON_EDU_NAME_SZ ] = 0 ;
      ossStrncpy ( simple._source, _source, MON_EDU_NAME_SZ ) ;
      simple._source[ MON_EDU_NAME_SZ ] = 0 ;

      if ( _pSession )
      {
         simple._relatedNID = _pSession->identifyID() ;
         simple._relatedEDUID = _pSession->identifyEDUID() ;
         simple._relatedTID = _pSession->identifyTID() ;
      }
      else
      {
         simple._relatedEDUID = _eduID ;
         simple._relatedTID = _tid ;
         simple._relatedNID = 0 ;
      }

      PD_TRACE_EXIT ( SDB___PMDEDUCB_DUMPINFO );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB___PMDEDUCB_DUMPINFO2, "_pmdEDUCB::dumpInfo" )
   void _pmdEDUCB::dumpInfo ( monEDUFull &full )
   {
      PD_TRACE_ENTRY ( SDB___PMDEDUCB_DUMPINFO2 );
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      full._eduStatus[0] = 0 ;
      full._eduType[0] = 0 ;
      full._eduName[0] = 0 ;
      full._doing[0] = 0 ;
      full._source[0] = 0 ;
      full._eduID = _eduID ;
      full._tid = _tid ;
      full._processEventCount = _processEventCount ;
      full._isBlock = isBlocked() ;
      full._queueSize = _queue.size() ;
      if ( _pMemPool )
      {
         full._memPoolSize = _pMemPool->getCacheSize() ;
      }
      else
      {
         full._memPoolSize = 0 ;
      }
      ossStrncpy ( full._eduStatus, getEDUStatusDesp(_status),
                   MON_EDU_STATUS_SZ ) ;
      full._eduStatus[ MON_EDU_STATUS_SZ ] = 0 ;
      ossStrncpy ( full._eduType, getEDUName (_eduType), MON_EDU_TYPE_SZ ) ;
      full._eduType[ MON_EDU_TYPE_SZ ] = 0 ;
      ossStrncpy ( full._eduName, _name, MON_EDU_NAME_SZ ) ;
      full._eduName[ MON_EDU_NAME_SZ ] = 0 ;
      ossStrncpy( full._doing, _doingBuff, MON_EDU_DOING_SZ ) ;
      full._doing[ MON_EDU_DOING_SZ ] = 0 ;
      ossStrncpy ( full._source, _source, MON_EDU_NAME_SZ ) ;
      full._source[ MON_EDU_NAME_SZ ] = 0 ;

      full._monApplCB = _monApplCB ;
      full._threadHdl = _threadHdl ;

      _contextCopy( full._eduContextList ) ;

      if ( _pSession )
      {
         full._relatedNID = _pSession->identifyID() ;
         full._relatedEDUID = _pSession->identifyEDUID() ;
         full._relatedTID = _pSession->identifyTID() ;
      }
      else
      {
         full._relatedEDUID = _eduID ;
         full._relatedTID = _tid ;
         full._relatedNID = 0 ;
      }

      PD_TRACE_EXIT ( SDB___PMDEDUCB_DUMPINFO2 );
   }

#if defined ( SDB_ENGINE )
   void _pmdEDUCB::updateTransConfByMask( const dpsTransConfItem &conf )
   {
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      _transExecutor.updateTransConfByMask( conf, optCB->getTotalLogSpace() ) ;
   }

   void _pmdEDUCB::copyTransConf( const dpsTransConfItem &conf )
   {
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      _transExecutor.copyTransConf( conf, optCB->getTotalLogSpace() ) ;
   }

   void _pmdEDUCB::clearTransInfo()
   {
      _curTransID = DPS_INVALID_TRANS_ID ;
      _relatedTransLSN = DPS_INVALID_LSN_OFFSET ;
      _curTransLSN = DPS_INVALID_LSN_OFFSET ;
      _transRC = SDB_OK ;
      _transStatus = DPS_TRANS_UNKNOWN ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      if ( pTransCB )
      {
         pTransCB->transLockReleaseAll( this, NULL ) ;
      }
   }

   BOOLEAN _pmdEDUCB::isTransaction() const
   {
      return ( DPS_INVALID_TRANS_ID != _curTransID ) ? TRUE : FALSE ;
   }

   BOOLEAN _pmdEDUCB::isTransRU () const
   {
      return isTransaction() &&
             _transExecutor.getTransIsolation() == TRANS_ISOLATION_RU ;
   }

   BOOLEAN _pmdEDUCB::isTransRC () const
   {
      return isTransaction() &&
             _transExecutor.getTransIsolation() == TRANS_ISOLATION_RC ;
   }

   BOOLEAN _pmdEDUCB::isTransRS () const
   {
      return isTransaction() &&
             _transExecutor.getTransIsolation() == TRANS_ISOLATION_RS ;
   }

   BOOLEAN _pmdEDUCB::isAutoCommitTrans() const
   {
      return DPS_TRANS_IS_AUTOCOMMIT( _curTransID ) ? TRUE : FALSE ;
   }

   void _pmdEDUCB::dumpTransInfo( monTransInfo &transInfo )
   {
      transInfo._eduID        = _eduID ;
      transInfo._transID      = _curTransID ;
      transInfo._curTransLsn  = _curTransLSN ;
      transInfo._lockEscalated =
            _transExecutor.isLockEscalated( LOCKMGR_TRANS_LOCK ) ;
      transInfo._usedLogSpace = _transExecutor.getUsedSpace() ;
      transInfo._reservedLogSpace = _transExecutor.getReservedSpace() ;

      {
         ossScopedLock lock( &_mutex, SHARED ) ;
         if ( _pSession )
         {
            transInfo._relatedNID = _pSession->identifyID() ;
            transInfo._relatedEDUID = _pSession->identifyEDUID() ;
            transInfo._relatedTID = _pSession->identifyTID() ;
         }
         else
         {
            transInfo._relatedEDUID = _eduID ;
            transInfo._relatedTID = _tid ;
            transInfo._relatedNID = 0 ;
         }
      }
   }

   pmdTransExecutor* _pmdEDUCB::getTransExecutor()
   {
      return &_transExecutor ;
   }

   UINT64 _pmdEDUCB::getRemoteSucCount()
   {
      if ( NULL != _pRemoteOperator )
      {
         return _pRemoteOperator->getSucCount() ;
      }

      return 0 ;
   }

   UINT64 _pmdEDUCB::getRemoteFailureCount()
   {
      if ( NULL != _pRemoteOperator )
      {
         return _pRemoteOperator->getFailureCount() ;
      }

      return 0 ;
   }

   sdbRemoteOpCtrl* _pmdEDUCB::getRemoteOpCtrl()
   {
      return &_remoteOpCtrl ;
   }

   IRemoteOperator* _pmdEDUCB::getRemoteOperator()
   {
      return _pRemoteOperator ;
   }

   INT32 _pmdEDUCB::getOrCreateRemoteOperator( IRemoteOperator **ppOperator )
   {
      INT32 rc = SDB_OK ;
      _clsRemoteOperator *tmp = NULL ;
      if ( NULL != _pRemoteOperator )
      {
         *ppOperator = _pRemoteOperator ;
         goto done ;
      }

      tmp = SDB_OSS_NEW _clsRemoteOperator() ;
      PD_CHECK( NULL != tmp, SDB_OOM, error, PDERROR,
                "Failed to malloc remote operator:rc=%d", rc ) ;

      rc = tmp->init( this ) ;
      if ( SDB_OK != rc )
      {
         SAFE_OSS_DELETE( tmp ) ;
         PD_LOG( PDERROR, "Failed to begin transaction:rc=%d", rc ) ;
         goto error ;
      }

      _pRemoteOperator = tmp ;
      *ppOperator = _pRemoteOperator ;

   done:
      return rc ;
   error:
      goto done ;
   }

#endif // SDB_ENGINE

   static OSS_THREAD_LOCAL _pmdEDUCB *__eduCB ;
   extern OSS_THREAD_LOCAL IExecutor *__executor ;

   _pmdEDUCB *pmdGetThreadEDUCB ()
   {
      return __eduCB ;
   }

   _pmdEDUCB *pmdDeclareEDUCB ( _pmdEDUCB *p )
   {
      __executor = p ;
      __eduCB = p ;
      return __eduCB ;
   }

   void pmdUndeclareEDUCB ()
   {
      __executor = NULL ;
      __eduCB = NULL ;
   }

   // for pmdRecv, we wait indefinitely until the agent is forced, because
   // client may not send us anything due to idle of user activities
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDRECV, "pmdRecv" )
   INT32 pmdRecv ( CHAR *pBuffer, INT32 recvSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout,
                   INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( sock, "Socket is NULL" ) ;
      SDB_ASSERT ( cb, "cb is NULL" ) ;
      PD_TRACE_ENTRY ( SDB_PMDRECV );
      INT32 receivedSize = 0 ;
      INT32 totalReceivedSize = 0 ;
      while ( TRUE )
      {
         if ( cb->isForced () )
         {
            rc = SDB_APP_FORCED ;
            goto done ;
         }
         rc = sock->recv ( &pBuffer[totalReceivedSize],
                           recvSize-totalReceivedSize,
                           receivedSize, timeout ) ;
         totalReceivedSize += receivedSize ;
         if ( SDB_TIMEOUT == rc )
         {
            if ( forceTimeout > 0 && timeout > 0 )
            {
               if ( forceTimeout > timeout )
               {
                  forceTimeout -= timeout ;
               }
               else
               {
                  break ;
               }
            }
            continue ;
         }
         goto done ;
      }
   done :
#if defined ( SDB_ENGINE )
      if ( totalReceivedSize > 0 )
      {
         pmdGetKRCB()->getMonDBCB()->svcNetInAdd( totalReceivedSize ) ;
      }
#endif // SDB_ENGINE
      PD_TRACE_EXITRC ( SDB_PMDRECV, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDSEND, "pmdSend" )
   INT32 pmdSend ( const CHAR *pBuffer, INT32 sendSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( sock, "Socket is NULL" ) ;
      SDB_ASSERT ( cb, "cb is NULL" ) ;
      PD_TRACE_ENTRY ( SDB_PMDSEND );
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;
      while ( true )
      {
         if ( cb->isForced () )
         {
            rc = SDB_APP_FORCED ;
            goto done ;
         }
         rc = sock->send ( &pBuffer[totalSentSize],
                           sendSize-totalSentSize,
                           sentSize, timeout ) ;
         totalSentSize += sentSize ;
         if ( SDB_TIMEOUT == rc )
            continue ;
         goto done ;
      }
   done :
#if defined ( SDB_ENGINE )
      if ( totalSentSize > 0 )
      {
         pmdGetKRCB()->getMonDBCB()->svcNetOutAdd( totalSentSize ) ;
      }
#endif // SDB_ENGINE
      PD_TRACE_EXITRC ( SDB_PMDSEND, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDCONVERTMSG, "_pmdConvertMsg" )
   static INT32 _pmdConvertMsg( IMsgConvertor *msgConvertor,
                                const MsgHeader *pMsg,
                                CHAR *&outMsg, UINT32 &outMsgLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__PMDCONVERTMSG ) ;

      SDB_ASSERT( msgConvertor && pMsg, "Argument invalid" ) ;

      msgConvertor->reset( FALSE ) ;
      rc = msgConvertor->push( (CHAR *)pMsg, pMsg->messageLength ) ;
      PD_RC_CHECK( rc, PDERROR, "Push message[opCode: %d] into message "
                   "convertor failed[%d]", pMsg->opCode, rc ) ;
      rc = msgConvertor->output( outMsg, outMsgLen ) ;
      PD_RC_CHECK( rc, PDERROR, "Get converted message[opCode: %d] "
                   "from the message convertor failed[%d]",
                   pMsg->opCode, rc ) ;
      SDB_ASSERT( outMsgLen == *(UINT32 *)outMsg,
                  "Converted message length is not as expected" ) ;

   done:
      PD_TRACE_EXITRC( SDB__PMDCONVERTMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDSYNCSENDMSG, "pmdSyncSendMsg" )
   INT32 pmdSyncSendMsg( const MsgHeader *pMsg, pmdEDUEvent &recvEvent,
                         ossSocket *sock, pmdEDUCB *cb,
                         INT32 timeout, INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_PMDSYNCSENDMSG ) ;
      UINT32 msgLen = 0 ;
      CHAR *pRecvBuf = NULL ;
      UINT32 buffSize = 0 ;
      UINT16 reserveSize = 0 ;   // For possible message conversion.
      msgConvertorImpl *msgConvertor = NULL ;

   reSend:
      rc = pmdSend( (const CHAR *)pMsg, pMsg->messageLength, sock,
                    cb, timeout ) ;
      if ( rc )
      {
         goto error ;
      }
      // recieve msg length
      rc = pmdRecv( (CHAR*)&msgLen, sizeof(INT32), sock, cb, timeout,
                    forceTimeout ) ;
      if ( rc )
      {
         goto error ;
      }

      // As we do not know the protocol version of the peer node, so we check
      // the length with sizeof(MsgHeaderV1), as sizeof(MsgOpReplyV1) is less
      // then sizeof(MsgHeader). If peer node is protocol version 2, need to
      // check with sizeof(MsgHeader) below.
      if ( msgLen < sizeof( MsgHeaderV1 ) || msgLen > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Recieve msg size[%u] less than msg header or more "
                 "than max size", msgLen ) ;
         rc = SDB_SYS ;
         sock->close() ;
         goto error ;
      }

      if ( buffSize < msgLen + reserveSize )
      {
         CHAR *newBuff =
               (CHAR *) SDB_THREAD_REALLOC( pRecvBuf, msgLen + reserveSize ) ;
         if ( !newBuff )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Alloc memory[size: %u] failed[%d]",
                    msgLen + reserveSize, rc ) ;
            goto error ;
         }
         pRecvBuf = newBuff ;
         buffSize = msgLen + reserveSize ;
      }

      ossMemcpy( pRecvBuf, ( CHAR* )&msgLen, sizeof( INT32 ) ) ;
      // recieve last msg
      rc = pmdRecv( pRecvBuf + sizeof( INT32 ), msgLen - sizeof( INT32 ),
                    sock, cb, timeout ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( MSG_COMM_EYE_DEFAULT != ((MsgHeader *)pRecvBuf)->eye )
      {
         // The eye is not as expected, so the peer node is most likely to be
         // using old protocol. The first reply should report unknown message
         // as the original request has not been converted. So convert the
         // request and send again. And when the reply is received, it also
         // needs to be converted.
         MsgOpReplyV1 *reply = (MsgOpReplyV1 *)pRecvBuf ;
         CHAR *convertedMsg = NULL ;
         UINT32 finalSize = 0 ;

         if ( !msgConvertor )
         {
            msgConvertor = SDB_OSS_NEW msgConvertorImpl ;
            if ( !msgConvertor )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Allocate memory for message "
                       "convertor[size: %d] failed[%d]",
                       sizeof(msgConvertorImpl), rc ) ;
               goto error ;
            }

            // Convert the request.
            rc = _pmdConvertMsg( msgConvertor, pMsg, convertedMsg, finalSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Convert request message[opCode: %d] "
                         "failed[%d]", pMsg->opCode, rc ) ;

            pMsg = (MsgHeader *)convertedMsg ;
            reserveSize = sizeof(MsgOpReply) - sizeof(MsgOpReplyV1) ;
            goto reSend ;
         }
         else
         {
            // Convert the reply.
            rc = _pmdConvertMsg( msgConvertor, (const MsgHeader *)reply,
                                 convertedMsg, finalSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Convert reply of message[opCode: %d] "
                         "failed[%d]", pMsg->opCode, rc ) ;

            // Copy the message to out buffer.
            if ( finalSize > buffSize )
            {
               CHAR *newBuff = (CHAR *)SDB_THREAD_REALLOC( pRecvBuf, finalSize ) ;
               if ( !newBuff )
               {
                  rc = SDB_OOM ;
                  PD_LOG( PDERROR, "Allocate memory[size: %u] for converted "
                                   "message failed[%d]", finalSize, rc ) ;
                  goto error ;
               }
               pRecvBuf = newBuff ;
               buffSize = finalSize ;
            }
            ossMemcpy( pRecvBuf, convertedMsg, finalSize ) ;
         }
      }
      else if ( msgLen < sizeof(MsgHeader) )
      {
         PD_LOG( PDERROR, "Recieve msg size[%u] less than msg header size",
                 msgLen ) ;
         rc = SDB_SYS ;
         sock->close() ;
         goto error ;
      }

      recvEvent._eventType = PMD_EDU_EVENT_MSG ;
      recvEvent._Data = (void*)pRecvBuf ;
      recvEvent._dataMemType = PMD_EDU_MEM_THREAD ;
      recvEvent._userData = 0 ;

   done:
      if ( msgConvertor )
      {
         SDB_OSS_DEL msgConvertor ;
      }
      PD_TRACE_EXITRC( SDB_PMDSYNCSENDMSG, rc ) ;
      return rc ;
   error:
      if ( pRecvBuf )
      {
         SDB_THREAD_FREE( pRecvBuf ) ;
      }
      goto done ;
   }

   INT32 pmdSendAndRecv2Que( const MsgHeader *pMsg, ossSocket *sock,
                             pmdEDUCB *cb, INT32 timeout,
                             INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      pmdEDUEvent event ;
      rc = pmdSyncSendMsg( pMsg, event, sock, cb, timeout, forceTimeout ) ;
      if ( rc )
      {
         goto error ;
      }
      cb->postEvent( event ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void pmdEduEventRelease( pmdEDUEvent &event, pmdEDUCB *cb )
   {
      if ( event._Data && event._dataMemType != PMD_EDU_MEM_NONE )
      {
         if ( PMD_EDU_MEM_ALLOC == event._dataMemType )
         {
            SDB_OSS_FREE( event._Data ) ;
         }
         else if ( PMD_EDU_MEM_SELF == event._dataMemType )
         {
            SDB_ASSERT( cb, "cb can't be NULL" ) ;
            if ( cb )
            {
               cb->releaseBuff( (CHAR *)event._Data ) ;
            }
         }
         else if ( PMD_EDU_MEM_THREAD == event._dataMemType )
         {
            SDB_THREAD_FREE( event._Data ) ;
         }
         event._Data = NULL ;
      }
   }

}

