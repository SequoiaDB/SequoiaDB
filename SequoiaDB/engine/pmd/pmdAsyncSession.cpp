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

   #define PMD_SESSION_FORCE_TIMEOUT         ( 1800 * OSS_ONE_SEC )

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
      // ON_MSG
   END_OBJ_MSG_MAP()

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN, "_pmdAsyncSession::_pmdAsyncSession" )
   _pmdAsyncSession::_pmdAsyncSession( UINT64 sessionID )
   :_pendingMsgNum( 0 ), _holdCount( 0 )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN ) ;
      _lockFlag    = FALSE ;
      _startType   = PMD_SESSION_PASSIVE ;
      _pSessionMgr = NULL ;

      _reset() ;

      _sessionID   = sessionID ;

      _evtIn.reset() ;
      _evtOut.signal() ;
      _detachEvent.signal() ;

      _isClosed    = TRUE ;

      PD_TRACE_EXIT ( SDB__PMDSN ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_DESC, "_pmdAsyncSession::~_pmdAsyncSession" )
   _pmdAsyncSession::~_pmdAsyncSession()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_DESC ) ;
      clear() ;
      PD_TRACE_EXIT ( SDB__PMDSN_DESC ) ;
   }

   UINT32 _pmdAsyncSession::getPendingMsgNum()
   {
      return _pendingMsgNum.fetch() ;
   }

   UINT32 _pmdAsyncSession::incPendingMsgNum()
   {
      return _pendingMsgNum.inc() ;
   }

   UINT32 _pmdAsyncSession::decPendingmsgNum()
   {
      return _pendingMsgNum.dec() ;
   }

   BOOLEAN _pmdAsyncSession::hasHold()
   {
      return _holdCount.compare( 0 ) ? FALSE : TRUE ;
   }

   const schedInfo* _pmdAsyncSession::getSchedInfo() const
   {
      return &( _info._info ) ;
   }

   void* _pmdAsyncSession::getSchedItemPtr()
   {
      return (void*)&_info ;
   }

   void _pmdAsyncSession::setSchedItemVer( INT32 ver )
   {
      _info._info.setVersion( ver ) ;
   }

   void _pmdAsyncSession::_holdIn()
   {
      _holdCount.inc() ;
   }

   void _pmdAsyncSession::_holdOut()
   {
      _holdCount.dec() ;
   }

   UINT64 _pmdAsyncSession::identifyID()
   {
      return _identifyID ;
   }

   MsgRouteID _pmdAsyncSession::identifyNID()
   {
      return _identifyNID ;
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
                                           MsgRouteID nid,
                                           UINT32 tid, UINT64 eduID )
   {
      _identifyID = ossPack32To64( ip, port ) ;
      _identifyNID.value = nid.value ;
      _identifyTID = tid ;
      _identifyEDUID = eduID ;
   }

   INT32 _pmdAsyncSession::getServiceType() const
   {
      return CMD_SPACE_SERVICE_SHARD ;
   }

   // This function will be called by another thread to attach a CB into the
   // session thread
   // It will do bunch of assignments and initialization, and attempt to
   // latch out, then release latchIn, so that the session calling getSession
   // will move on
   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_ATHIN, "_pmdAsyncSession::attachIn" )
   INT32 _pmdAsyncSession::attachIn ( pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_ATHIN );

      SDB_ASSERT( cb, "cb can't be NULL" ) ;

      PD_LOG( PDINFO, "Session[%s] attach edu[%d]", sessionName(),
              cb->getID() ) ;

      _isClosed = FALSE ;
      _pEDUCB = cb ;
      _eduID  = cb->getID() ;
      _pEDUCB->setName( sessionName() ) ;
      _pEDUCB->attachSession( this ) ;
      _client.attachCB( cb ) ;

      /// set identify tid and eduid
      _identifyTID = cb->getTID() ;
      _identifyEDUID = cb->getID() ;

      _evtOut.reset() ;
      _evtIn.signal() ;
      _detachEvent.reset() ;

      _onAttach () ;

      PD_TRACE_EXIT ( SDB__PMDSN_ATHIN );
      return SDB_OK ;
   }

   // attachOut is called by the async agent once all logic are done, so that
   // the data structure can be released
   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_ATHOUT, "_pmdAsyncSession::attachOut" )
   INT32 _pmdAsyncSession::attachOut ()
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_ATHOUT );

      PD_LOG( PDINFO, "Session[%s] detach edu[%d]", sessionName(),
              eduID() ) ;

      if ( SDB_OK != _detachEvent.wait( 0 ) &&
           _pSessionMgr->forceNotify( sessionID(), eduCB() ) )
      {
         _detachEvent.wait( PMD_SESSION_FORCE_TIMEOUT ) ;
      }

      _detachEvent.signal() ;
      _isClosed = TRUE ;
      /// wait holdout
      while( hasHold() )
      {
         ossSleep( 100 ) ;
      }
      _pEDUCB->getMonAppCB()->setSvcTaskInfo( NULL ) ;

      _onDetach () ;

      _client.detachCB() ;
      _pEDUCB->detachSession() ;
      _evtOut.signal() ;
      /// set _pEDUCB must at end
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

   BOOLEAN _pmdAsyncSession::isClosed() const
   {
      return _isClosed ;
   }

   void _pmdAsyncSession::close()
   {
      _isClosed = TRUE ;
   }

   void _pmdAsyncSession::setAuditConfig( UINT32 auditMask,
                                          UINT32 auditConfigMask )
   {
      _auditMask = auditMask ;
      _auditConfigMask = auditConfigMask ;
   }

   void _pmdAsyncSession::getAuditConfig( UINT32 &auditMask,
                                          UINT32 &auditConfigMask )
   {
      auditMask = _auditMask ;
      auditConfigMask = _auditConfigMask ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_CLEAR, "_pmdAsyncSession::clear" )
   void _pmdAsyncSession::clear()
   {
      _reset() ;

      _evtIn.reset() ;
      _pendingMsgNum.swap( 0 ) ;
      _info.reset() ;
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

      _auditMask = 0 ;
      _auditConfigMask = 0 ;

      // release all buffer pointers
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
      // determine whether the session is from coord
      // we use this number to identify whether a node is coming from coord
      // We always increase nodeID with PMD_BASE_HANDLE_ID when
      // it's sent from coord, so we need to minus the number before
      // printing
      if ( nodeID > PMD_BASE_HANDLE_ID )
      {
         // if the session is coming from coord
         ossSnprintf( _name , SESSION_NAME_LEN, "Type:%s,NetID:%u,R-TID:%u",
                      className(), nodeID - PMD_BASE_HANDLE_ID, TID ) ;
      }
      else
      {
         // otherwise it's not session from coord
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
      // if the session is already locked, we return SDB_SYS
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

   // wait until someone calls attachIn, otherwise stay here since
   // latchIn is got in constructor
   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDSN_WTATH, "_pmdAsyncSession::waitAttach" )
   INT32 _pmdAsyncSession::waitAttach ( INT64 millisec )
   {
      PD_TRACE_ENTRY ( SDB__PMDSN_WTATH );
      INT32 rc = _evtIn.wait( millisec ) ;
      PD_TRACE_EXIT ( SDB__PMDSN_WTATH );
      return rc ;
   }

   // wait until the session is detached
   // latchOut will be released only when the thread finish doing the job
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
      // we shouldn't get here
      // if we hit here that means the memory we were trying to use was not
      // properly allocated or the length is not good
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
      _buffArray[_buffEnd].addTime = ossGetCurrentMicroseconds() ;

      _buffEnd = _incBuffPos( _buffEnd ) ;

   done :
      PD_TRACE_EXITRC ( SDB__PMDSN_PSHBUF, rc );
      return rc ;
   }

   // increase buffer position
   // no need to latch since it can only be touched by one thread
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

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_DESC, "_pmdAsycSessionMgr::~_pmdAsycSessionMgr" )
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

      // set timer
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
      PD_TRACE_ENTRY ( PMD_SESSMGR_FINI ) ;

      _quit = TRUE ;

      // kill timer
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

      // release session and meta
      MAPSESSION_IT it = _mapSession.begin () ;
      while ( it != _mapSession.end() )
      {
         _releaseSession_i( it->second, FALSE, FALSE ) ;
         ++it ;
      }
      _mapSession.clear () ;

      it = _mapPendingSession.begin() ;
      while( it != _mapPendingSession.end() )
      {
         _releaseSession( it->second ) ;
         ++it ;
      }
      _mapPendingSession.clear() ;

      while ( _deqCacheSessions.size () > 0 )
      {
         _releaseSession( _deqCacheSessions.front () ) ;
         _deqCacheSessions.pop_front () ;
      }
      _cacheSessionNum = 0 ;

      while ( _deqDeletingSessions.size() > 0 )
      {
         _releaseSession ( _deqDeletingSessions.front() ) ;
         _deqDeletingSessions.pop_front() ;
      }

      //Clear latch
      MAPMETA_IT itMeta = _mapMeta.begin() ;
      while ( itMeta != _mapMeta.end() )
      {
         SDB_OSS_DEL itMeta->second ;
         ++itMeta ;
      }
      _mapMeta.clear() ;

      PD_TRACE_EXIT ( PMD_SESSMGR_FINI ) ;
      // always return SDB_OK
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
         // push session into the force list
         _forceSessions.push_back( sessionID ) ;
         // create timer to clean up the session if there's no timer exist
         if ( NET_INVALID_TIMER_ID == _forceChecktimer )
         {
            // _checkForceSession must wait for _forceLatch before ierate force
            // session list, so there's no concurrent issue
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
      MAPSESSION_IT itPending ;

      /// check pending session and push to tmpDeletingSessions
      {
         ossScopedLock lock( &_metaLatch ) ;
         itPending = _mapPendingSession.begin() ;
         while( itPending != _mapPendingSession.end() )
         {
            pSession = itPending->second ;

            if ( 0 == pSession->getPendingMsgNum() &&
                 !pSession->hasHold() )
            {
               tmpDeletingSessions.push_back( pSession ) ;
               _mapPendingSession.erase( itPending++ ) ;
               continue ;
            }
            ++itPending ;
         }
      }

      /// check _deqShdDeletingSessions and push to tmpDeletingSessions
      {
         ossScopedLock lock( &_deqDeletingMutex ) ;

         it = tmpDeletingSessions.begin() ;
         while( it != tmpDeletingSessions.end() )
         {
            pSession = *it ;
            ++it ;
            /// push to deleting que
            _deqDeletingSessions.push_back( pSession ) ;
         }
         tmpDeletingSessions.clear() ;

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
      }

      /// release the session
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

   void _pmdAsycSessionMgr::holdOut( pmdAsyncSession *pSession )
   {
      pSession->_holdOut() ;
   }

   INT32 _pmdAsycSessionMgr::dispatchMsg( const NET_HANDLE &handle,
                                          const MsgHeader *pMsg,
                                          pmdEDUMemTypes memType,
                                          BOOLEAN decPending,
                                          BOOLEAN *hasDispatched )
   {
      INT32 rc        = SDB_OK ;
      _pmdAsyncSession *pSession = NULL ;
      BOOLEAN bCreate = TRUE ;
      UINT64 sessionID = 0 ;

      if ( hasDispatched )
      {
         *hasDispatched = FALSE ;
      }

      // if opcode is disconnect or interrupt, we don't expect to create
      // new session
      if ( MSG_BS_DISCONNECT == pMsg->opCode ||
           MSG_BS_INTERRUPTE == pMsg->opCode ||
           MSG_BS_INTERRUPTE_SELF == pMsg->opCode )
      {
         bCreate = FALSE ;
      }
      sessionID = makeSessionID( handle, pMsg ) ;

      // Find the associated session if exist
      // If the session doesn't exist, we'll check bCreate, if bCreate=TRUE it
      // will create one, otherwise will not
      rc = getSession( sessionID ,
                       TRUE,
                       PMD_SESSION_PASSIVE,
                       handle, bCreate, pMsg->opCode,
                       NULL, &pSession ) ;
      // Determine whether a session is created or retreived
      if ( rc )
      {
         // If session is not retreived
         if ( !bCreate )
         {
            if ( MSG_BS_DISCONNECT == pMsg->opCode )
            {
               _metaLatch.get() ;
               onNoneSessionDisconnect( sessionID ) ;
               _metaLatch.release() ;
            }
            // It's okay if we don't expect one
            rc = SDB_OK ;
            goto done ;
         }
         // Otherwise log the message
         PD_LOG ( PDERROR, "Failed to create session[ID:%lld], rc: %d",
                  sessionID, rc ) ;

         rc = onErrorHanding( rc, pMsg, handle, sessionID, NULL ) ;
         if ( rc )
         {
            goto error ;
         }
         else
         {
            goto done ;
         }
      }

      if ( decPending )
      {
         pSession->decPendingmsgNum() ;
      }

      /// When session is closed
      if ( pSession->isClosed() )
      {
         rc = SDB_APP_INTERRUPT ;
         PD_LOG( PDWARNING, "Session[%s] is closed, Pending msg num:%d, rc:%d",
                 pSession->sessionName(), pSession->getPendingMsgNum(), rc ) ;

         rc = onErrorHanding( rc, pMsg, handle,
                              sessionID, pSession ) ;
         if ( rc )
         {
            goto error ;
         }
         else
         {
            goto done ;
         }
      }

      // Check the received code
      if ( MSG_BS_DISCONNECT == pMsg->opCode )
      {
         PD_LOG ( PDEVENT, "Session[%s] recieved disconnect message",
                  pSession->sessionName() ) ;

         _metaLatch.get() ;
         onSessionDisconnect( pSession ) ;
         _metaLatch.release() ;

         // Session will be released and we don't need to push message
         holdOut( pSession ) ;
         rc = releaseSession( pSession, TRUE ) ;
         if ( rc )
         {
            PD_LOG ( PDWARNING, "Failed to release session, rc = %d", rc ) ;
            rc = SDB_OK ;
         }
         pSession = NULL ;
         goto done ;
      }
      else if ( MSG_BS_INTERRUPTE == pMsg->opCode )
      {
         PD_LOG ( PDINFO, "Session[%s] recieved interrupt message",
                  pSession->sessionName() ) ;
         pSession->eduCB()->interrupt() ;
         // For interrupt message, we have to continue in order to push the
         // message
      }
      else if ( MSG_BS_INTERRUPTE_SELF == pMsg->opCode )
      {
         PD_LOG( PDINFO, "Session[%s] recieved interrupt self message",
                 pSession->sessionName() ) ;
         pSession->eduCB()->interrupt() ;
         goto done ;
      }

      // On recieve
      pSession->onRecieve ( handle, (_MsgHeader*)pMsg ) ;

      // push the mssage into session manager
      rc = _pushMessage( pSession, pMsg, memType, handle ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG ( PDERROR, "Failed to push message[Len:%u, opCode:%d, "
                  "TID:%u, RequestID:%llu, RouteID:%d.%d], rc = %d",
                  pMsg->messageLength, pMsg->opCode, pMsg->TID,
                  pMsg->requestID, pMsg->routeID.columns.groupID,
                  pMsg->routeID.columns.nodeID, rc ) ;

         rc = onErrorHanding( rc, pMsg, handle, sessionID, pSession ) ;
         if ( rc )
         {
            goto error ;
         }
         else
         {
            goto done ;
         }
      }
      else if ( hasDispatched )
      {
         *hasDispatched = TRUE ;
      }

   done:
      if ( pSession )
      {
         holdOut( pSession ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_PUSHMSG, "_pmdAsycSessionMgr::_pushMessage" )
   INT32 _pmdAsycSessionMgr::_pushMessage( pmdAsyncSession *pSession,
                                           const MsgHeader *header,
                                           pmdEDUMemTypes memType,
                                           const NET_HANDLE &handle )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_PUSHMSG ) ;
      CHAR *pNewBuff          = NULL ;
      UINT64 userData         = PMD_MAKE_SESSION_USERDATA( handle,
                                           PMD_SESSION_MSG_INPOOL ) ;

      if ( pSession->isClosed() )
      {
         rc = SDB_APP_INTERRUPT ;
         PD_LOG( PDWARNING, "Session[%s] is closed, Pending msg num:%d, rc:%d",
                 pSession->sessionName(), pSession->getPendingMsgNum(), rc ) ;
         goto error ;
      }

      if ( PMD_EDU_MEM_NONE == memType )
      {
         UINT32 buffSize         = 0 ;
         pmdBuffInfo * pBuffInfo = pSession->frontBuffer () ;
         // loop through all free slots
         while ( pBuffInfo && pBuffInfo->isFree() )
         {
            if ( !pNewBuff && pBuffInfo->size >= (UINT32)header->messageLength )
            {
               pNewBuff = pBuffInfo->pBuffer ;
               buffSize = pBuffInfo->size ;
            }
            else //release memory to pool
            {
               SDB_THREAD_FREE( pBuffInfo->pBuffer ) ;
            }
            pSession->popBuffer () ;
            pBuffInfo = pSession->frontBuffer () ;
         }
         // if we cannot find any free slots
         if ( !pNewBuff && !pSession->isBufferFull() )
         {
            // let's allocate memory from pool
            pNewBuff = ( CHAR* )SDB_THREAD_ALLOC2( header->messageLength,
                                                   &buffSize ) ;
            // if unable to allocate from thread pool, let's dump warning message and
            // and keep calling oss malloc to get memory
            if ( !pNewBuff )
            {
               PD_LOG ( PDWARNING, "Thread pool assign memory failed[size:%d]",
                        header->messageLength ) ;
            }
         }
         // if memory is got from existing pool, let's assign to the session
         if ( pNewBuff )
         {
            rc = pSession->pushBuffer ( pNewBuff, buffSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG ( PDERROR, "push buffer failed in session[%s, rc:%d]", 
                        pSession->sessionName(), rc ) ;
               SDB_THREAD_FREE( pNewBuff ) ;
               SDB_ASSERT ( 0, "why the buffer is full??? check" ) ;
               goto error ;
            }

            // copyMsg will NOT allocate memory inside
            // so we don't need to set memType
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
            pNewBuff = ( CHAR* )SDB_THREAD_ALLOC( header->messageLength ) ;
            if ( !pNewBuff )
            {
               PD_LOG( PDERROR, "Failed to alloc msg[size: %d] in session[%s]",
                       header->messageLength, pSession->sessionName() ) ;
               rc = SDB_OOM ;
               goto error ;
            }
            ossMemcpy( pNewBuff, (void*)header, header->messageLength ) ;
            userData = PMD_MAKE_SESSION_USERDATA( handle,
                                                  PMD_SESSION_MSG_UNPOOL ) ;
            memType  = PMD_EDU_MEM_THREAD ;
         }
      }
      else
      {
         userData = PMD_MAKE_SESSION_USERDATA( handle,
                                               PMD_SESSION_MSG_UNPOOL ) ;
         pNewBuff = ( CHAR* )header ;
      }

      // post edu event
      pSession->eduCB()->postEvent( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                                 memType, pNewBuff,
                                                 userData ) ) ;
   done:
      PD_TRACE_EXITRC ( PMD_SESSMGR_PUSHMSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_GETSESSIONOBJ, "_pmdAsycSessionMgr::getSessionObj" )
   INT32 _pmdAsycSessionMgr::getSessionObj( UINT64 sessionID,
                                            BOOLEAN withHold,
                                            BOOLEAN bCreate,
                                            INT32 startType,
                                            const NET_HANDLE &handle,
                                            INT32 opCode,
                                            void *data,
                                            pmdAsyncSession **ppSession,
                                            BOOLEAN *pIsPending )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_GETSESSIONOBJ );
      pmdAsyncSession *pSession    = NULL ;
      SDB_SESSION_TYPE sessionType = SDB_SESSION_MAX ;
      MAPSESSION_IT it ;
      BOOLEAN isPending            = FALSE ;

      {
         ossScopedLock lock( &_metaLatch ) ;

         // check if there's already session for the sessionID
         if ( _mapSession.end() != ( it = _mapSession.find( sessionID ) ) )
         {
            pSession = it->second ;
         }
         else if ( _mapPendingSession.end() !=
                   ( it = _mapPendingSession.find( sessionID ) ) )
         {
            pSession = it->second ;

            if ( 0 == pSession->getPendingMsgNum() && !pSession->hasHold() )
            {
               /// release
               _mapPendingSession.erase( it ) ;

               /// push to deleting que
               ossScopedLock lock ( &_deqDeletingMutex ) ;
               _deqDeletingSessions.push_back ( pSession ) ;
               pSession = NULL ;
            }
            else
            {
               isPending = TRUE ;
            }
         }

         if ( pSession )
         {
            // need to attach meta
            if ( !pSession->getMeta() && pSession->canAttachMeta() &&
                 NET_INVALID_HANDLE != handle )
            {
               // we can safely ignore the return code from _attachSessionMeta
               // if for any reason we were not able to attach, we can do
               // it next time
               _attachSessionMeta( pSession, handle ) ;
            }

            if ( withHold )
            {
               pSession->_holdIn() ;
            }
            goto done ;
         }
      }

      // if we are not asked for create new session, let's simply return
      if ( !bCreate )
      {
         rc = SDB_EOF ;
         /// must goto done
         goto done ;
      }

      // parse session type
      sessionType = _prepareCreate( sessionID, startType, opCode ) ;
      // if we hit SESSION_MAX, that means we can't find a valid session type
      if ( SDB_SESSION_MAX == sessionType )
      {
         PD_LOG( PDERROR, "Failed to parse session type by info[sessionID: "
                 "%lld, startType: %d, opCode: (%d)%d ]", sessionID,
                 startType, IS_REPLY_TYPE(opCode), GET_REQUEST_TYPE(opCode) ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // can we get from cached session list?
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

      // if we still don't have a session, let's create one
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

      pSession->startType( startType ) ;
      pSession->sessionID( sessionID ) ;

      PD_LOG ( PDEVENT, "Create session[%s,StartType:%d]",
               pSession->sessionName(), startType ) ;

      // set session info
      {
         ossScopedLock lock( &_metaLatch ) ;

         _onSessionNew( pSession ) ;

         _mapSession[ sessionID ] = pSession ;

         // attach meta
         if ( !pSession->getMeta() && pSession->canAttachMeta() &&
              NET_INVALID_HANDLE != handle )
         {
            // if the session is not able to attached with metadata,
            // it cann't do much thing anyway
            // so we don't bother to start the session if we can't attach meta
            rc = _attachSessionMeta( pSession, handle ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR, "Unable to attach metadata, rc = %d", rc ) ;
               goto error ;
            }
         }

         if ( withHold )
         {
            pSession->_holdIn() ;
         }
      }

   done:
      if ( ppSession )
      {
         *ppSession = pSession ;
      }
      if ( pIsPending )
      {
         *pIsPending = isPending ;
      }
      PD_TRACE_EXITRC( PMD_SESSMGR_GETSESSIONOBJ, rc ) ;
      return rc ;
   error:
      if ( pSession )
      {
         releaseSession ( pSession ) ;
         pSession = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_GETSESSION, "_pmdAsycSessionMgr::getSession" )
   INT32 _pmdAsycSessionMgr::getSession( UINT64 sessionID,
                                         BOOLEAN withHold,
                                         INT32 startType,
                                         const NET_HANDLE &handle,
                                         BOOLEAN bCreate,
                                         INT32 opCode,
                                         void *data,
                                         pmdAsyncSession **ppSession )
   {
      INT32 rc                     = SDB_OK ;
      PD_TRACE_ENTRY ( PMD_SESSMGR_GETSESSION ) ;
      pmdAsyncSession *pSession = NULL ;
      BOOLEAN isPending = FALSE ;

      rc = getSessionObj( sessionID, withHold, bCreate,
                          startType, handle, opCode,
                          data, &pSession, &isPending ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( isPending )
      {
         goto done ;
      }
      else if ( !pSession->isAttached() )
      {
         //Start session EDU
         rc = _startSessionEDU( pSession ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to start session EDU, rc = %d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( ppSession )
      {
         *ppSession = pSession ;
      }
      PD_TRACE_EXITRC ( PMD_SESSMGR_GETSESSION, rc );
      return rc ;
   error:
      if ( pSession )
      {
         if ( withHold )
         {
            holdOut( pSession ) ;
         }
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
         // memory will be freed in _checkSessionMeta and fini
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
      // increase reference counter
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

      // Wait the EDUCB is in the session by the newly created thread
      while( SDB_OK != pSession->waitAttach ( OSS_ONE_SEC ) )
      {
         if ( !pEDUMgr->getEDUByID( eduID ) )
         {
            /// The edu has terminate due to some exception
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
      // clear session map
      MAPSESSION_IT it ;

      ossScopedLock lock( &_metaLatch ) ;

      it = _mapSession.find( pSession->sessionID() ) ;
      if ( it != _mapSession.end() )
      {
         _mapSession.erase( it ) ;
      }
      else
      {
         goto done ;
      }

      // release session
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
         // Notify the edu quit
         pmdEDUMgr *pMgr = pmdGetKRCB()->getEDUMgr() ;
         pMgr->disconnectUserEDU( pSession->eduID() ) ;
      }

      /// forceBack must after disconnectUserEDU
      pSession->forceBack() ;

      onSessionDestoryed( pSession ) ;

      if ( pSession->hasHold() || 0 != pSession->getPendingMsgNum() )
      {
         PD_LOG( PDEVENT, "Change session[%s] to pending queue, Its pending "
                 "msg number:%d", pSession->sessionName(),
                 pSession->getPendingMsgNum() ) ;
         _mapPendingSession[ pSession->sessionID() ] = pSession ;
         goto done ;
      }
      // if we don't need to relase it rightaway, we can push the request to
      // delete queue and return
      else if ( delay || !pSession->isDetached() )
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

      // Wait the working agent finish the job
      pSession->waitDetach () ;

      // dec based handle number
      if ( pSession->getMeta() )
      {
         pSession->getMeta()->decBaseHandleNum() ;
      }

      // Release Memory to thread pool
      pBuffInfo = pSession->frontBuffer() ;
      while ( pBuffInfo )
      {
         SDB_THREAD_FREE( pBuffInfo->pBuffer ) ;
         pSession->popBuffer () ;
         pBuffInfo = pSession->frontBuffer() ;
      }
      pSession->clear() ;

      // if the session can be reused, let's queue it
      if ( !_quit && _canReuse( pSession->sessionType() ) &&
           _cacheSessionNum < _maxCacheSize() )
      {
         ossScopedLock lock ( &_deqDeletingMutex ) ;
         _deqCacheSessions.push_back( pSession ) ;
         ++_cacheSessionNum ;
         goto done ;
      }
      // only free memory when it can't be queued
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
      else if ( MSG_BS_DISCONNECT == pReqMsg->opCode ||
                MSG_BS_INTERRUPTE == pReqMsg->opCode ||
                MSG_BS_INTERRUPTE_SELF == pReqMsg->opCode )
      {
         /// not reply
         goto done ;
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

      {
         ossScopedLock lock( &_metaLatch ) ;
         MAPSESSION_IT it = _mapSession.begin() ;
         // iterate all sessions
         while ( it != _mapSession.end() )
         {
            pSession = it->second ;
            // release the session
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
      }

      // create a timer if it doesn't exist
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
         //return not zero, the timer will dispath to main cb
         rc = SDB_INVALIDARG ;
      }

   done :
      // rc can be ignored in this function, so we don't need to bother
      // record it in trace
      PD_TRACE_EXIT ( PMD_SESSMGR_HDLSNTM ) ;
      return rc ;
   }

   // clean up all unused meta by checking the reference counter
   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_CHKSNMETA, "_pmdAsycSessionMgr::_checkSessionMeta" )
   void _pmdAsycSessionMgr::_checkSessionMeta( UINT32 interval )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_CHKSNMETA ) ;

      ossScopedLock lock( &_metaLatch ) ;

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

   // check if there's any session is forced
   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_CHKFORCESN, "_pmdAsycSessionMgr::_checkForceSession" )
   void _pmdAsycSessionMgr::_checkForceSession( UINT32 interval )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_CHKFORCESN ) ;

      pmdAsyncSession *pSession = NULL ;
      UINT64 sessionID = 0 ;

      ossScopedLock lock( &_forceLatch ) ;

      // iterate all sessions from force list
      while ( !_forceSessions.empty() )
      {
         sessionID = _forceSessions.front() ;
         _forceSessions.pop_front() ;

         {
            ossScopedLock metaLock( &_metaLatch ) ;
            MAPSESSION_IT itSession = _mapSession.find( sessionID ) ;
            if ( itSession == _mapSession.end() )
            {
               continue ;
            }
            pSession = itSession->second ;
            _releaseSession_i( pSession, FALSE, TRUE ) ;
            _mapSession.erase( itSession ) ;
         }
      }

      // remove the timer if it's already exist
      if ( NET_INVALID_TIMER_ID != _forceChecktimer )
      {
         _pRTAgent->removeTimer( _forceChecktimer ) ;
         _forceChecktimer = NET_INVALID_TIMER_ID ;
      }

      PD_TRACE_EXIT ( PMD_SESSMGR_CHKFORCESN ) ;
   }

   // check if there's any session timeout
   // PD_TRACE_DECLARE_FUNCTION ( PMD_SESSMGR_CHKSN, "_pmdAsycSessionMgr::_checkSession" )
   void _pmdAsycSessionMgr::_checkSession( UINT32 interval )
   {
      PD_TRACE_ENTRY ( PMD_SESSMGR_CHKSN ) ;

      pmdAsyncSession *pSession = NULL ;

      ossScopedLock lock( &_metaLatch ) ;

      MAPSESSION_IT it = _mapSession.begin() ;
      while ( it != _mapSession.end() )
      {
         pSession = it->second ;

         if ( 0 == pSession->getPendingMsgNum() &&
              !pSession->hasHold() &&
              !pSession->isProcess() &&
              pSession->isAttached() &&
              pSession->timeout( interval ) )
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


