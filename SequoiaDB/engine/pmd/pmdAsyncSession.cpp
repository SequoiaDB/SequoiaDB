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

   Source File Name = pmdAsyncSession.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdAsyncSession.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "rtnCommand.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      _pmdSessionMeta implement
   */
   _pmdSessionMeta::_pmdSessionMeta( const NET_HANDLE handle )
   :_basedHandleNum( 0 )
   {
      _netHandle = handle ;
   }

   _pmdSessionMeta::~_pmdSessionMeta()
   {
   }

   /*
      _pmdAsyncSession implement
   */
   BEGIN_OBJ_MSG_MAP( _pmdAsyncSession, _pmdObjBase )
   END_OBJ_MSG_MAP()

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN, "_pmdAsyncSession::_pmdAsyncSession" )
   _pmdAsyncSession::_pmdAsyncSession( UINT64 sessionID )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN ) ;
      _lockFlag    = FALSE ;
      _startType   = PMD_SESSION_PASSIVE ;
      _pSessionMgr = NULL ;

      _reset() ;

      _sessionID   = sessionID ;

      _evtIn.reset() ;
      _evtOut.signal() ;

      PD_TRACE_EXIT ( SDB__PMDSN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_DESC, "_pmdAsyncSession::~_pmdAsyncSession" )
   _pmdAsyncSession::~_pmdAsyncSession()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_DESC ) ;
      clear() ;
      PD_TRACE_EXIT ( SDB__PMDSN_DESC ) ;
   }

   UINT64 _pmdAsyncSession::identifyID()
   {
      return _identifyID ;
   }

   UINT32 _pmdAsyncSession::identifyTID()
   {
      return _identifyTID ;
   }

   UINT64 _pmdAsyncSession::identifyEDUID()
   {
      return _identifyEDUID ;
   }

   void _pmdAsyncSession::setIdentifyInfo( UINT32 ip, UINT16 port,
                                           UINT32 tid, UINT64 eduID )
   {
      _identifyID = ossPack32To64( ip, port ) ;
      _identifyTID = tid ;
      _identifyEDUID = eduID ;
   }

   INT32 _pmdAsyncSession::getServiceType() const
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_ATHIN, "_pmdAsyncSession::attachIn" )
   INT32 _pmdAsyncSession::attachIn ( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_ATHIN );

      SDB_ASSERT( cb, "cb can't be NULL" ) ;

      PD_LOG( PDINFO, "Session[%s] attach edu[%d]", sessionName(),
              cb->getID() ) ;

      _pEDUCB = cb ;
      _eduID  = cb->getID() ;
      _pEDUCB->setName( sessionName() ) ;
      _pEDUCB->attachSession( this ) ;
      _client.attachCB( cb ) ;

      _identifyTID = cb->getTID() ;
      _identifyEDUID = cb->getID() ;

      _evtOut.reset() ;
      _evtIn.signal() ;
      _detachEvent.reset() ;

      _onAttach () ;

      PD_TRACE_EXIT ( SDB__PMDSN_ATHIN );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_ATHOUT, "_pmdAsyncSession::attachOut" )
   INT32 _pmdAsyncSession::attachOut ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_ATHOUT );

      PD_LOG( PDINFO, "Session[%s] detach edu[%d]", sessionName(),
              eduID() ) ;

      if ( SDB_OK != _detachEvent.wait( 0 ) &&
           _pSessionMgr->forceNotify( sessionID(), eduCB() ) )
      {
         _detachEvent.wait( 300 * OSS_ONE_SEC ) ;
      }

      _onDetach () ;

      _client.detachCB() ;
      _pEDUCB->detachSession() ;
      _evtOut.signal() ;
      _pEDUCB = NULL ;
      PD_TRACE_EXIT ( SDB__PMDSN_ATHOUT );
      return SDB_OK ;
   }

   void _pmdAsyncSession::forceBack()
   {
      _detachEvent.signalAll() ;
   }

   BOOLEAN _pmdAsyncSession::isDetached () const
   {
      return _pEDUCB ? FALSE : TRUE ;
   }

   BOOLEAN _pmdAsyncSession::isAttached () const
   {
      return _pEDUCB ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_CLEAR, "_pmdAsyncSession::clear" )
   void _pmdAsyncSession::clear()
   {
      _reset() ;

      _evtIn.reset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_RESET, "_pmdAsyncSession::_reset" )
   void _pmdAsyncSession::_reset()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_RESET );
      if ( _lockFlag )
      {
         _unlock () ;
      }

      _sessionID = INVLIAD_SESSION_ID;
      _pEDUCB    = NULL ;
      _eduID     = PMD_INVALID_EDUID ;
      _netHandle = NET_INVALID_HANDLE ;
      _name [0]  = 0 ;
      _pMeta     = NULL ;

      _identifyID= ossPack32To64( _netFrame::getLocalAddress(),
                                  pmdGetLocalPort() ) ;
      _identifyTID=0 ;
      _identifyEDUID = 0 ;

      for ( UINT32 index = 0 ; index < MAX_BUFFER_ARRAY_SIZE; ++index )
      {
         _buffArray[index].pBuffer = NULL ;
         _buffArray[index].size    = 0 ;
         _buffArray[index].useFlag = PMD_BUFF_INVALID ;
         _buffArray[index].addTime = 0 ;
      }
      _buffBegin = 0 ;
      _buffEnd   = 0 ;
      _buffCount = 0 ;
      PD_TRACE_EXIT ( SDB__PMDSN_RESET );
   }

   void _pmdAsyncSession::onRecieve ( const NET_HANDLE netHandle,
                                      MsgHeader * msg )
   {
   }

   BOOLEAN _pmdAsyncSession::timeout ( UINT32 interval )
   {
      return FALSE ;
   }

   void _pmdAsyncSession::_onAttach ()
   {
   }

   void _pmdAsyncSession::_onDetach ()
   {
   }

   UINT64 _pmdAsyncSession::sessionID () const
   {
      return _sessionID ;
   }

   void _pmdAsyncSession::sessionID ( UINT64 sessionID )
   {
      _sessionID = sessionID ;
      _makeName () ;
   }

   EDUID _pmdAsyncSession::eduID () const
   {
      return _eduID ;
   }

   pmdEDUCB *_pmdAsyncSession::eduCB () const
   {
      return _pEDUCB ;
   }

   NET_HANDLE _pmdAsyncSession::netHandle () const
   {
      return _netHandle ;
   }

   void _pmdAsyncSession::meta ( pmdSessionMeta * pMeta )
   {
      _pMeta = pMeta ;
      if ( _pMeta )
      {
         _netHandle = _pMeta->getHandle() ;
         _client.setClientInfo( _pSessionMgr->getRouteAgent(),
                                _netHandle ) ;
      }
      else
      {
         _netHandle = NET_INVALID_HANDLE ;
      }
   }

   void _pmdAsyncSession::setSessionMgr( _pmdAsycSessionMgr *pSessionMgr )
   {
      _pSessionMgr = pSessionMgr ;
   }

   netRouteAgent* _pmdAsyncSession::routeAgent()
   {
      return _pSessionMgr->getRouteAgent() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN__MKNAME, "_pmdAsyncSession::_makeName" )
   void _pmdAsyncSession::_makeName ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN__MKNAME );
      UINT32 nodeID = 0 ;
      UINT32 TID    = 0 ;
      ossUnpack32From64 ( _sessionID, nodeID, TID ) ;
      if ( nodeID > PMD_BASE_HANDLE_ID )
      {
         ossSnprintf( _name , SESSION_NAME_LEN, "Type:%s,NetID:%u,R-TID:%u",
                      className(), nodeID - PMD_BASE_HANDLE_ID, TID ) ;
      }
      else
      {
         ossSnprintf( _name , SESSION_NAME_LEN, "Type:%s,NodeID:%u,TID:%u",
                      className(), nodeID, TID ) ;
      }
      _name [SESSION_NAME_LEN] = 0 ;
      PD_TRACE_EXIT ( SDB__PMDSN__MKNAME );
   }

   BOOLEAN _pmdAsyncSession::isStartActive ()
   {
      return _startType == PMD_SESSION_ACTIVE ? TRUE : FALSE ;
   }

   void _pmdAsyncSession::startType ( INT32 startType )
   {
      _startType = startType ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN__LOCK, "_pmdAsyncSession::_lock" )
   INT32 _pmdAsyncSession::_lock ()
   {
      INT32 rc = SDB_SYS ;
      PD_TRACE_ENTRY ( SDB__PMDSN__LOCK );
      if ( _pMeta && !_lockFlag )
      {
         _pMeta->getLatch()->get() ;
         _lockFlag = TRUE ;
         rc = SDB_OK ;
      }
      PD_TRACE_EXITRC ( SDB__PMDSN__LOCK, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN__UNLOCK, "_pmdAsyncSession::_unlock" )
   INT32 _pmdAsyncSession::_unlock ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN__UNLOCK );
      if ( _pMeta && _lockFlag )
      {
         _pMeta->getLatch()->release () ;
         _lockFlag = FALSE ;
      }
      PD_TRACE_EXIT ( SDB__PMDSN__UNLOCK );
      return SDB_OK ;
   }

   const CHAR *_pmdAsyncSession::sessionName () const
   {
      return _name ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_WTATH, "_pmdAsyncSession::waitAttach" )
   INT32 _pmdAsyncSession::waitAttach ( INT64 millisec )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_WTATH );
      INT32 rc = _evtIn.wait( millisec ) ;
      PD_TRACE_EXIT ( SDB__PMDSN_WTATH );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_WTDTH, "_pmdAsyncSession::waitDetach" )
   INT32 _pmdAsyncSession::waitDetach ( INT64 millisec )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_WTDTH );
      INT32 rc = _evtOut.wait( millisec ) ;
      PD_TRACE_EXIT ( SDB__PMDSN_WTDTH );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_CPMSG, "_pmdAsyncSession::copyMsg" )
   void * _pmdAsyncSession::copyMsg( const CHAR *msg, UINT32 length )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_CPMSG );
      void *p        = NULL ;
      UINT32 buffPos = _decBuffPos ( _buffEnd ) ;
      if ( _buffArray[buffPos].isAlloc() &&
           _buffArray[buffPos].size >= length )
      {
         ossMemcpy( _buffArray[buffPos].pBuffer, msg, length ) ;
         _buffArray[buffPos].useFlag = PMD_BUFF_USING ;
         p = (void*)&_buffArray[buffPos] ;
         goto done ;
      }
      PD_LOG ( PDERROR, "Session[%s] copy msg failed[buffindex:%d, size:%d, "
               "flag:%d, message length:%d", sessionName(), buffPos,
               _buffArray[buffPos].size, _buffArray[buffPos].useFlag, length ) ;

   done :
      PD_TRACE_EXIT ( SDB__PMDSN_CPMSG );
      return p ;
   }

   BOOLEAN _pmdAsyncSession::isBufferFull() const
   {
      return _buffCount >= MAX_BUFFER_ARRAY_SIZE ? TRUE : FALSE ;
   }

   BOOLEAN _pmdAsyncSession::isBufferEmpty() const
   {
      return _buffCount == 0 ? TRUE : FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_FRNBUF, "_pmdAsyncSession::frontBuffer" )
   pmdBuffInfo *_pmdAsyncSession::frontBuffer ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_FRNBUF );
      pmdBuffInfo *p = NULL ;
      if ( _buffArray[_buffBegin].isInvalid() )
      {
         goto done ;
      }
      SDB_ASSERT ( _buffCount > 0 , "_buffCount must be greater than 0" ) ;

      p = &_buffArray[_buffBegin] ;
   done :
      PD_TRACE_EXIT ( SDB__PMDSN_FRNBUF );
      return p ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_POPBUF, "_pmdAsyncSession::popBuffer" )
   void _pmdAsyncSession::popBuffer ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_POPBUF );
      SDB_ASSERT ( _buffCount > 0 , "_buffCount must be greater than 0" ) ;

      _buffArray[_buffBegin].pBuffer = NULL ;
      _buffArray[_buffBegin].size    = 0 ;
      _buffArray[_buffBegin].useFlag = PMD_BUFF_INVALID ;
      _buffArray[_buffBegin].addTime = 0 ;

      --_buffCount ;
      _buffBegin = _incBuffPos( _buffBegin ) ;
      PD_TRACE_EXIT ( SDB__PMDSN_POPBUF );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_PSHBUF, "_pmdAsyncSession::pushBuffer" )
   INT32 _pmdAsyncSession::pushBuffer ( CHAR * pBuffer, UINT32 size )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDSN_PSHBUF );
      if ( _buffCount >= MAX_BUFFER_ARRAY_SIZE )
      {
         rc = SDB_CLS_BUFFER_FULL ;
         PD_LOG ( PDWARNING, "cls buffer is full" ) ;
         goto done ;
      }

      SDB_ASSERT ( _buffArray[_buffEnd].isInvalid (),
                   "end buffer can't be invalid" ) ;

      ++_buffCount ;
      _buffArray[_buffEnd].pBuffer = pBuffer ;
      _buffArray[_buffEnd].size    = size ;
      _buffArray[_buffEnd].useFlag = PMD_BUFF_ALLOC ;
      _buffArray[_buffEnd].addTime = time( NULL ) ;

      _buffEnd = _incBuffPos( _buffEnd ) ;

   done :
      PD_TRACE_EXITRC ( SDB__PMDSN_PSHBUF, rc );
      return rc ;
   }

   UINT32 _pmdAsyncSession::_incBuffPos ( UINT32 pos )
   {
      ++pos ;
      if ( pos < MAX_BUFFER_ARRAY_SIZE )
      {
         return pos ;
      }

      return 0 ;
   }

   UINT32 _pmdAsyncSession::_decBuffPos ( UINT32 pos )
   {
      return pos ? pos - 1 : MAX_BUFFER_ARRAY_SIZE - 1 ;
   }

   /*
      _pmdAsycSessionMgr implement
   */
   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR, "_pmdAsycSessionMgr::_pmdAsycSessionMgr" )
   _pmdAsycSessionMgr::_pmdAsycSessionMgr()
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR ) ;
      _quit                   = FALSE ;
      _isStop                 = FALSE ;
      _pRTAgent               = NULL ;
      _pTimerHandle           = NULL ;
      _handleCloseTimerID     = NET_INVALID_TIMER_ID ;
      _sessionTimerID         = NET_INVALID_TIMER_ID ;
      _forceChecktimer        = NET_INVALID_TIMER_ID ;
      _timerInterval          = OSS_ONE_SEC ;
      _cacheSessionNum        = 0 ;
      PD_TRACE_EXIT ( PMD_SESSMGR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_DESC, " _pmdAsycSessionMgr::~_pmdAsycSessionMgr" )
   _pmdAsycSessionMgr::~_pmdAsycSessionMgr()
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_DESC ) ;
      _pRTAgent               = NULL ;
      _pTimerHandle           = NULL ;
      PD_TRACE_EXIT ( PMD_SESSMGR_DESC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_INIT, "_pmdAsycSessionMgr::init" )
   INT32 _pmdAsycSessionMgr::init( netRouteAgent *pRTAgent,
                                   _netTimeoutHandler *pTimerHandle,
                                   UINT32 timerInterval )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_INIT ) ;

      if ( !pRTAgent || !pTimerHandle )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Invalid argument to init async session mgr" ) ;
         goto error ;
      }
      _pRTAgent      = pRTAgent ;
      _pTimerHandle  = pTimerHandle ;
      _timerInterval = timerInterval ;

      rc = _memPool.initialize() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init mem pool, rc: %d", rc ) ;
         goto error ;
      }

      rc = _pRTAgent->addTimer( _timerInterval, _pTimerHandle,
                                _sessionTimerID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Add session timer failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( PMD_SESSMGR_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_FINI, "_pmdAsycSessionMgr::fini" )
   INT32 _pmdAsycSessionMgr::fini()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_FINI ) ;

      _quit = TRUE ;

      if ( _pRTAgent )
      {
         if ( NET_INVALID_TIMER_ID != _sessionTimerID )
         {
            _pRTAgent->removeTimer( _sessionTimerID ) ;
            _sessionTimerID = NET_INVALID_TIMER_ID ;
         }
         if ( NET_INVALID_TIMER_ID != _handleCloseTimerID )
         {
            _pRTAgent->removeTimer( _handleCloseTimerID ) ;
            _handleCloseTimerID = NET_INVALID_TIMER_ID ;
         }
      }

      MAPSESSION_IT it = _mapSession.begin () ;
      while ( it != _mapSession.end() )
      {
         _releaseSession_i( it->second, FALSE, FALSE ) ;
         ++it ;
      }
      _mapSession.clear () ;

      while ( _deqCacheSessions.size () > 0 )
      {
         _releaseSession_i( _deqCacheSessions.front (), FALSE, FALSE ) ;
         _deqCacheSessions.pop_front () ;
      }
      _cacheSessionNum = 0 ;

      while ( _deqDeletingSessions.size() > 0 )
      {
         _releaseSession_i ( _deqDeletingSessions.front(), FALSE, FALSE ) ;
         _deqDeletingSessions.pop_front() ;
      }

      MAPMETA_IT itMeta = _mapMeta.begin() ;
      while ( itMeta != _mapMeta.end() )
      {
         SDB_OSS_DEL itMeta->second ;
         ++itMeta ;
      }
      _mapMeta.clear() ;

      rc = _memPool.final() ;
      if ( rc )
      {
         PD_LOG ( PDWARNING, "Failed to finalize mempool, rc = %d", rc ) ;
      }
      PD_TRACE_EXIT ( PMD_SESSMGR_FINI ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_FORCENTY, "_pmdAsycSessionMgr::forceNotify" )
   BOOLEAN _pmdAsycSessionMgr::forceNotify( UINT64 sessionID,
                                            _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( PMD_SESSMGR_FORCENTY ) ;

      BOOLEAN ret = TRUE ;
      ossScopedLock lock( &_forceLatch ) ;

      if ( _isStop )
      {
         ret = FALSE ;
      }
      else
      {
         _forceSessions.push_back( sessionID ) ;
         if ( NET_INVALID_TIMER_ID == _forceChecktimer )
         {
            _pRTAgent->addTimer( 1, _pTimerHandle, _forceChecktimer ) ;
         }
      }

      PD_TRACE_EXITRC ( PMD_SESSMGR_FORCENTY, ret ) ;
      return ret ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_ONTIMER, "_pmdAsycSessionMgr::onTimer" )
   void _pmdAsycSessionMgr::onTimer( UINT32 interval )
   {
      PD_TRACE_ENTRY( PMD_SESSMGR_ONTIMER ) ;

      pmdAsyncSession *pSession = NULL ;
      DEQSESSION tmpDeletingSessions ;
      DEQSESSION::iterator it ;

      _deqDeletingMutex.get() ;
      it = _deqDeletingSessions.begin() ;
      while ( it != _deqDeletingSessions.end() )
      {
         pSession = *it ;
         if ( !pSession->isDetached() )
         {
            ++it ;
            continue ;
         }
         tmpDeletingSessions.push_back( pSession ) ;
         it = _deqDeletingSessions.erase( it ) ;
      }
      _deqDeletingMutex.release() ;

      it = tmpDeletingSessions.begin() ;
      while( it != tmpDeletingSessions.end() )
      {
         pSession = *it ;
         ++it ;

         _releaseSession( pSession ) ;
      }
      tmpDeletingSessions.clear() ;

      PD_TRACE_EXIT( PMD_SESSMGR_ONTIMER ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_PUSHMSG, "_pmdAsycSessionMgr::pushMessage" )
   INT32 _pmdAsycSessionMgr::pushMessage( pmdAsyncSession *pSession,
                                          const MsgHeader *header,
                                          const NET_HANDLE &handle )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_PUSHMSG ) ;
      CHAR *pNewBuff          = NULL ;
      UINT32 buffSize         = 0 ;
      UINT64 userData         = 0 ; // 0: memPool, 1: alloc
      pmdEDUMemTypes memType  = PMD_EDU_MEM_NONE ;
      pmdBuffInfo * pBuffInfo = pSession->frontBuffer () ;
      while ( pBuffInfo && pBuffInfo->isFree() )
      {
         if ( !pNewBuff && pBuffInfo->size >= (UINT32)header->messageLength )
         {
            pNewBuff = pBuffInfo->pBuffer ;
            buffSize = pBuffInfo->size ;
         }
         else //release memory to pool
         {
            _memPool.release( pBuffInfo->pBuffer, pBuffInfo->size ) ;
         }
         pSession->popBuffer () ;
         pBuffInfo = pSession->frontBuffer () ;
      }
      if ( !pNewBuff && !pSession->isBufferFull() )
      {
         pNewBuff = _memPool.alloc ( header->messageLength, buffSize ) ;
         if ( !pNewBuff )
         {
            PD_LOG ( PDWARNING, "Memory pool assign memory failed[size:%d]",
                     header->messageLength ) ;
         }
      }
      if ( pNewBuff )
      {
         rc = pSession->pushBuffer ( pNewBuff, buffSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "push buffer failed in session[%s, rc:%d]", 
                     pSession->sessionName(), rc ) ;
            _memPool.release ( pNewBuff, buffSize ) ;
            SDB_ASSERT ( 0, "why the buffer is full??? check" ) ;
            goto error ;
         }

         pNewBuff = (CHAR*)pSession->copyMsg( (const CHAR*)header,
                                              header->messageLength ) ;
         if ( NULL == pNewBuff )
         {
            PD_LOG ( PDERROR, "Unable to find a previous valid memory" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         pNewBuff = ( CHAR* )SDB_OSS_MALLOC( header->messageLength ) ;
         if ( !pNewBuff )
         {
            PD_LOG( PDERROR, "Failed to alloc msg[size: %d] in session[%s]",
                    header->messageLength, pSession->sessionName() ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         ossMemcpy( pNewBuff, (void*)header, header->messageLength ) ;
         userData = 1 ;
         memType  = PMD_EDU_MEM_ALLOC ;
      }

      pSession->eduCB()->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                                 memType, pNewBuff,
                                                 userData ) ) ;
   done:
      PD_TRACE_EXITRC ( PMD_SESSMGR_PUSHMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_GETSESSION, "_pmdAsycSessionMgr::getSession" )
   INT32 _pmdAsycSessionMgr::getSession( UINT64 sessionID,
                                         INT32 startType,
                                         const NET_HANDLE handle,
                                         BOOLEAN bCreate, INT32 opCode,
                                         void *data,
                                         pmdAsyncSession **ppSession )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_GETSESSION );
      pmdAsyncSession *pSession    = NULL ;
      SDB_SESSION_TYPE sessionType = SDB_SESSION_MAX ;
      NET_EH eh ;

      MAPSESSION_IT it = _mapSession.find( sessionID ) ;
      if ( it != _mapSession.end() )
      {
         pSession = it->second ;

         if ( !pSession->getMeta() && pSession->canAttachMeta() &&
              NET_INVALID_HANDLE != handle )
         {
            _attachSessionMeta( pSession, handle ) ;
         }
         goto done ;
      }

      if ( !bCreate )
      {
         rc = SDB_EOF ;
         goto done ;
      }

      sessionType = _prepareCreate( sessionID, startType, opCode ) ;
      if ( SDB_SESSION_MAX == sessionType )
      {
         PD_LOG( PDERROR, "Failed to parse session type by info[sessionID: "
                 "%lld, startType: %d, opCode: (%d)%d ]", sessionID,
                 startType, IS_REPLY_TYPE(opCode), GET_REQUEST_TYPE(opCode) ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( _canReuse( sessionType ) && _cacheSessionNum > 0 )
      {
         ossScopedLock lock( &_deqDeletingMutex ) ;

         DEQSESSION::iterator itDeq = _deqCacheSessions.begin() ;
         while ( itDeq != _deqCacheSessions.end() )
         {
            if ( (*itDeq)->sessionType() == sessionType )
            {
               pSession = *itDeq ;
               _deqCacheSessions.erase( itDeq ) ;
               --_cacheSessionNum ;
               break ;
            }
            ++itDeq ;
         }
      }

      if ( !pSession )
      {
         pSession = _createSession( sessionType, startType, sessionID, data ) ;
         if ( !pSession )
         {
            PD_LOG( PDERROR, "Failed to create session[sessionType: %d, "
                    "startType: %d, sessionID: %lld ]", sessionType,
                    startType, sessionID ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         pSession->setSessionMgr( this ) ;
      }

      _mapSession[ sessionID ] = pSession ;
      pSession->startType( startType ) ;
      pSession->sessionID( sessionID ) ;

      PD_LOG ( PDEVENT, "Create session[%s,StartType:%d]",
               pSession->sessionName(), startType ) ;

      _onSessionNew( pSession ) ;

      if ( !pSession->getMeta() && pSession->canAttachMeta() &&
           NET_INVALID_HANDLE != handle )
      {
         rc = _attachSessionMeta( pSession, handle ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Unable to attach metadata, rc = %d", rc ) ;
            goto error ;
         }
      }

      rc = _startSessionEDU( pSession ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to start session EDU, rc = %d", rc ) ;
         goto error ;
      }

   done:
      if ( ppSession )
      {
         *ppSession = pSession ;
      }
      PD_TRACE_EXIT ( PMD_SESSMGR_GETSESSION );
      return rc ;
   error:
      if ( pSession )
      {
         releaseSession ( pSession ) ;
         pSession = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( CLS_PMDSMGR_ATCHMETA, "_pmdAsycSessionMgr::_attachSessionMeta" )
   INT32 _pmdAsycSessionMgr::_attachSessionMeta( pmdAsyncSession *pSession,
                                                 const NET_HANDLE handle )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( CLS_PMDSMGR_ATCHMETA ) ;
      pmdSessionMeta * pMeta = NULL ;
      MAPMETA_IT itMeta = _mapMeta.find ( handle ) ;
      if ( itMeta == _mapMeta.end() )
      {
         pMeta = SDB_OSS_NEW pmdSessionMeta ( handle ) ;
         if ( NULL == pMeta )
         {
            PD_LOG ( PDERROR, "Failed to allocate memory for meta" ) ;
            rc = SDB_OOM ;
            goto error ;
         }
         _mapMeta[handle] = pMeta ;
      }
      else
      {
         pMeta = itMeta->second ;
      }
      pMeta->incBaseHandleNum() ;
      pSession->meta ( pMeta ) ;

   done:
      PD_TRACE_EXITRC ( CLS_PMDSMGR_ATCHMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_STARTEDU, "_pmdAsycSessionMgr::_startSessionEDU" )
   INT32 _pmdAsycSessionMgr::_startSessionEDU( pmdAsyncSession *pSession )
   {
      INT32 rc           = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_STARTEDU ) ;
      pmdKRCB *pKRCB     = pmdGetKRCB() ;
      pmdEDUMgr *pEDUMgr = pKRCB->getEDUMgr() ;
      EDUID eduID        = PMD_INVALID_EDUID ;

      rc = pEDUMgr->startEDU( pSession->eduType(), (void *)pSession, &eduID ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_QUIESCED == rc )
         {
            PD_LOG ( PDWARNING, "Reject new connection due to quiesced "
                     "database" ) ;
         }
         else
         {
            PD_LOG ( PDERROR, "Failed to create subagent thread, rc: %d",
                     rc ) ;
         }
         goto error ;
      }

      while( SDB_OK != pSession->waitAttach ( OSS_ONE_SEC ) )
      {
         if ( !pEDUMgr->getEDUByID( eduID ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC ( PMD_SESSMGR_STARTEDU, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_RLSSS, "_pmdAsycSessionMgr::releaseSession" )
   INT32 _pmdAsycSessionMgr::releaseSession( pmdAsyncSession * pSession,
                                             BOOLEAN delay )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_RLSSS ) ;
      MAPSESSION_IT it = _mapSession.find( pSession->sessionID() ) ;
      if ( it != _mapSession.end() )
      {
         _mapSession.erase( it ) ;
      }

      rc = _releaseSession_i( pSession, TRUE, delay ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to release session, rc = %d", rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( PMD_SESSMGR_RLSSS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_RLSSS_I, "_pmdAsycSessionMgr::_releaseSession_i" )
   INT32 _pmdAsycSessionMgr::_releaseSession_i ( pmdAsyncSession *pSession,
                                                 BOOLEAN postQuit,
                                                 BOOLEAN delay )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_RLSSS_I ) ;

      SDB_ASSERT ( pSession, "pSession can't be NULL" ) ;

      if ( !_quit && postQuit && pSession->eduCB() )
      {
         pmdEDUMgr *pMgr = pmdGetKRCB()->getEDUMgr() ;
         pMgr->disconnectUserEDU( pSession->eduID() ) ;
      }

      pSession->forceBack() ;

      onSessionDestoryed( pSession ) ;

      if ( delay || !pSession->isDetached() )
      {
         ossScopedLock lock ( &_deqDeletingMutex ) ;
         _deqDeletingSessions.push_back ( pSession ) ;
         goto done ;
      }

      _releaseSession( pSession ) ;

   done:
      PD_TRACE_EXIT ( PMD_SESSMGR_RLSSS_I );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR__RELEASESESSION, "_pmdAsycSessionMgr::_releaseSession" )
   INT32 _pmdAsycSessionMgr::_releaseSession( pmdAsyncSession *pSession )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR__RELEASESESSION ) ;
      pmdBuffInfo *pBuffInfo = NULL ;

      SDB_ASSERT ( pSession, "pSession can't be NULL" ) ;

      pSession->waitDetach () ;

      if ( pSession->getMeta() )
      {
         pSession->getMeta()->decBaseHandleNum() ;
      }

      pBuffInfo = pSession->frontBuffer() ;
      while ( pBuffInfo )
      {
         _memPool.release ( pBuffInfo->pBuffer, pBuffInfo->size ) ;
         pSession->popBuffer () ;
         pBuffInfo = pSession->frontBuffer() ;
      }
      pSession->clear() ;

      if ( !_quit && _canReuse( pSession->sessionType() ) &&
           _cacheSessionNum < _maxCacheSize() )
      {
         ossScopedLock lock ( &_deqDeletingMutex ) ;
         _deqCacheSessions.push_back( pSession ) ;
         ++_cacheSessionNum ;
         goto done ;
      }
      SDB_OSS_DEL pSession ;

   done:
      PD_TRACE_EXIT ( PMD_SESSMGR__RELEASESESSION );
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_REPLY, "_pmdAsycSessionMgr::_reply" )
   INT32 _pmdAsycSessionMgr::_reply( const NET_HANDLE &handle, INT32 rc,
                                     const MsgHeader *pReqMsg )
   {
      INT32 ret = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_REPLY ) ;

      MsgOpReply reply ;
      BSONObj obj = utilGetErrorBson( rc, "can't create session" ) ;

      if ( !_pRTAgent )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      reply.header.opCode = MAKE_REPLY_TYPE( pReqMsg->opCode ) ;
      reply.header.requestID = pReqMsg->requestID ;
      reply.header.routeID.value = 0 ;
      reply.header.TID  = pReqMsg->TID ;
      reply.header.messageLength = sizeof ( MsgOpReply ) ;
      reply.flags = rc ;
      reply.contextID = -1 ;
      reply.numReturned = 1 ;
      reply.startFrom = 0 ;

      reply.header.messageLength += obj.objsize() ;

      ret = _pRTAgent->syncSend ( handle, ( MsgHeader*)&reply,
                                  (void*)obj.objdata(),
                                  obj.objsize() ) ;

   done:
      PD_TRACE_EXITRC ( PMD_SESSMGR_REPLY, rc );
      return ret ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_HDLSNCLOSE, "_pmdAsycSessionMgr::handleSessionClose" )
   INT32 _pmdAsycSessionMgr::handleSessionClose( const NET_HANDLE handle )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_HDLSNCLOSE ) ;
      pmdAsyncSession *pSession = NULL ;
      MAPSESSION_IT it = _mapSession.begin() ;
      while ( it != _mapSession.end() )
      {
         pSession = it->second ;
         if ( pSession->netHandle() == handle )
         {
            PD_LOG ( PDEVENT, "Session[%s, handle:%d] closed",
                     pSession->sessionName(), pSession->netHandle() ) ;
            onSessionHandleClose( pSession ) ;
            _releaseSession_i( pSession, TRUE, TRUE ) ;
            _mapSession.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }
      if ( NET_INVALID_TIMER_ID == _handleCloseTimerID )
      {
         _pRTAgent->addTimer( 30 * OSS_ONE_SEC, _pTimerHandle,
                              _handleCloseTimerID ) ;
      }

      PD_TRACE_EXIT ( PMD_SESSMGR_HDLSNCLOSE ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_HDLSTOP, "_pmdAsycSessionMgr::handleStop" )
   void _pmdAsycSessionMgr::handleStop()
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_HDLSTOP ) ;
      _forceLatch.get() ;
      _isStop = TRUE ;
      _forceLatch.release() ;

      _checkForceSession( 0 ) ;
      PD_TRACE_EXIT ( PMD_SESSMGR_HDLSTOP ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_HDLSNTM, "_pmdAsycSessionMgr::handleSessionTimeout" )
   INT32 _pmdAsycSessionMgr::handleSessionTimeout( UINT32 timerID,
                                                   UINT32 interval )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_HDLSNTM ) ;

      if ( _sessionTimerID == timerID )
      {
         _checkSession( interval ) ;
      }
      else if ( _handleCloseTimerID == timerID )
      {
         _checkSessionMeta( interval ) ;
         _pRTAgent->removeTimer( _handleCloseTimerID ) ;
         _handleCloseTimerID = NET_INVALID_TIMER_ID ;
         goto done ;
      }
      else if ( _forceChecktimer == timerID )
      {
         _checkForceSession( interval ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }

   done :
      PD_TRACE_EXIT ( PMD_SESSMGR_HDLSNTM ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_CHKSNMETA, "_pmdAsycSessionMgr::_checkSessionMeta" )
   void _pmdAsycSessionMgr::_checkSessionMeta( UINT32 interval )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_CHKSNMETA ) ;

      MAPMETA_IT it = _mapMeta.begin() ;
      while ( it != _mapMeta.end() )
      {
         pmdSessionMeta *pMeta = it->second ;
         if ( 0 == pMeta->getBasedHandleNum() )
         {
            SDB_OSS_DEL pMeta ;
            _mapMeta.erase( it++ ) ;
            continue ;
         }
         ++it ;
      }

      PD_TRACE_EXIT ( PMD_SESSMGR_CHKSNMETA ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_CHKFORCESN, "_pmdAsycSessionMgr::_checkForceSession" )
   void _pmdAsycSessionMgr::_checkForceSession( UINT32 interval )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_CHKFORCESN ) ;

      pmdAsyncSession *pSession = NULL ;
      UINT64 sessionID = 0 ;

      ossScopedLock lock( &_forceLatch ) ;

      while ( !_forceSessions.empty() )
      {
         sessionID = _forceSessions.front() ;
         _forceSessions.pop_front() ;

         MAPSESSION_IT itSession = _mapSession.find( sessionID ) ;
         if ( itSession == _mapSession.end() )
         {
            continue ;
         }
         pSession = itSession->second ;
         _releaseSession_i( pSession, FALSE, TRUE ) ;
         _mapSession.erase( itSession ) ;
      }

      if ( NET_INVALID_TIMER_ID != _forceChecktimer )
      {
         _pRTAgent->removeTimer( _forceChecktimer ) ;
         _forceChecktimer = NET_INVALID_TIMER_ID ;
      }

      PD_TRACE_EXIT ( PMD_SESSMGR_CHKFORCESN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_CHKSN, "_pmdAsycSessionMgr::_checkSession" )
   void _pmdAsycSessionMgr::_checkSession( UINT32 interval )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_CHKSN ) ;

      pmdAsyncSession *pSession = NULL ;
      MAPSESSION_IT it = _mapSession.begin() ;
      while ( it != _mapSession.end() )
      {
         pSession = it->second ;

         if ( !pSession->isProcess() && pSession->timeout( interval ) )
         {
            PD_LOG ( PDEVENT, "Session[%s] timeout", pSession->sessionName() ) ;
            _releaseSession_i ( pSession, TRUE, TRUE ) ;
            _mapSession.erase ( it++ ) ;
            continue ;
         }
         ++it ;
      }

      PD_TRACE_EXIT ( PMD_SESSMGR_CHKSN ) ;
   }

}


