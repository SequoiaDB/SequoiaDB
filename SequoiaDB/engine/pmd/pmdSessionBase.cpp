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

   Source File Name = pmdSessionBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/04/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdSessionBase.hpp"
#include "pmdEDU.hpp"
#include "pmdEnv.hpp"
#include "pmd.hpp"
#include "dpsLogWrapper.hpp"
#include "netFrame.hpp"

namespace engine
{
   const UINT32 SESSION_SOCKET_DFT_TIMEOUT = 10000 ;

   /*
      _pmdSession implement
   */
   _pmdSession::_pmdSession( SOCKET fd )
   :_socket( &fd, SESSION_SOCKET_DFT_TIMEOUT ),
    _client( &_socket ),
    _processor( NULL ),
    _pDPSCB( NULL )
   {
      _pEDUCB  = NULL ;
      _eduID   = PMD_INVALID_EDUID ;
      _pBuff   = NULL ;
      _buffLen = 0 ;
      _awaitingHandshake = TRUE ;

      _socket.disableNagle() ;
      _socket.setKeepAlive() ;

      if ( SOCKET_INVALIDSOCKET != fd )
      {
         CHAR tmpName [ 128 ] = {0} ;
         _socket.getPeerAddress( tmpName, sizeof( tmpName ) -1 ) ;
         _sessionName = tmpName ;
         _sessionName += ":" ;
         ossSnprintf( tmpName, sizeof( tmpName ) -1, "%d",
                      _socket.getPeerPort() ) ;
         _sessionName += tmpName ;
      }
   }

   _pmdSession::~_pmdSession()
   {
      clear() ;
   }

   _IProcessor* _pmdSession::getProcessor()
   {
      return _processor ;
   }

   void _pmdSession::clear ()
   {
      if ( _pBuff )
      {
         releaseBuff( _pBuff ) ;
         _pBuff = NULL ;
      }
      _buffLen = 0 ;
   }

   void _pmdSession::attach( _pmdEDUCB * cb )
   {
      SDB_ASSERT( cb, "cb can't be NULL" ) ;

      PD_LOG( PDINFO, "Session[%s] attach edu[%d]", sessionName(),
              cb->getID() ) ;

      _pDPSCB = pmdGetKRCB()->getDPSCB() ;
      if ( SDB_ROLE_COORD != pmdGetDBRole() &&
           _pDPSCB && !_pDPSCB->isLogLocal() )
      {
         _pDPSCB = NULL ;
      }

      _pEDUCB = cb ;
      _eduID  = cb->getID() ;
      _pEDUCB->attachSession( this ) ;
      _pEDUCB->setName( sessionName() ) ;
      _pEDUCB->setClientSock( socket() ) ;
      _client.attachCB( cb ) ;

      _onAttach() ;
   }

   void _pmdSession::detach ()
   {
      PD_LOG( PDINFO, "Session[%s] detach edu[%d]", sessionName(),
              eduID() ) ;

      _onDetach() ;
      clear() ;
      _client.logout() ;
      _client.detachCB() ;
      _pEDUCB->detachSession() ;
      _pEDUCB->setClientSock( NULL ) ;
      _pEDUCB = NULL ;
   }

   void _pmdSession::attachProcessor( pmdProcessor *pProcessor )
   {
      SDB_ASSERT( pProcessor, "Processor can't be NULL" ) ;
      _processor = pProcessor ;
      _processor->attachSession( this ) ;
   }

   void _pmdSession::detachProcessor()
   {
      SDB_ASSERT( _processor, "Processor can't be NULL" ) ;
      _processor->detachSession() ;
      _processor = NULL ;
   }

   const CHAR* _pmdSession::sessionName () const
   {
      return _sessionName.c_str() ;
   }

   UINT64 _pmdSession::identifyID()
   {
      return ossPack32To64( _netFrame::getLocalAddress(),
                            pmdGetLocalPort() ) ;
   }

   UINT32 _pmdSession::identifyTID()
   {
      if ( _pEDUCB )
      {
         return _pEDUCB->getTID() ;
      }
      return 0 ;
   }

   UINT64 _pmdSession::identifyEDUID()
   {
      if ( _pEDUCB )
      {
         return _pEDUCB->getID() ;
      }
      return 0 ;
   }

   INT32 _pmdSession::allocBuff( UINT32 len, CHAR **ppBuff, UINT32 *pRealSize )
   {
      INT32 rc = SDB_OK ;

      if ( !_pEDUCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      rc = _pEDUCB->allocBuff( len, ppBuff, pRealSize ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _pmdSession::releaseBuff( CHAR *pBuff )
   {
      SDB_ASSERT( _pEDUCB, "EDUCB can't be NULL" ) ;

      if ( _pEDUCB )
      {
         _pEDUCB->releaseBuff( pBuff ) ;
      }
   }

   INT32 _pmdSession::reallocBuff( UINT32 len, CHAR **ppBuff,
                                   UINT32 *pRealSize )
   {
      INT32 rc = SDB_OK ;

      if ( !_pEDUCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      rc = _pEDUCB->reallocBuff( len, ppBuff, pRealSize ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   CHAR* _pmdSession::getBuff( UINT32 len )
   {
      if ( _buffLen < len )
      {
         if ( _pBuff )
         {
            releaseBuff( _pBuff ) ;
            _pBuff = NULL ;
         }
         _buffLen = 0 ;

         allocBuff( len, &_pBuff, &_buffLen ) ;
      }

      return _pBuff ;
   }

   void _pmdSession::disconnect()
   {
      _socket.close() ;
   }

   INT32 _pmdSession::sendData( const CHAR * pData, INT32 size,
                                INT32 timeout, BOOLEAN block,
                                INT32 *pSentLen, INT32 flags )
   {
      INT32 rc = SDB_OK ;
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;
      INT32 realTimeout = timeout < 0 ? OSS_SOCKET_DFT_TIMEOUT : timeout ;

      while ( TRUE )
      {
         if ( _pEDUCB && _pEDUCB->isForced () )
         {
            rc = SDB_APP_FORCED ;
            goto done ;
         }
         rc = _socket.send ( &pData[totalSentSize], size-totalSentSize,
                             sentSize, realTimeout, flags, block ) ;
         totalSentSize += sentSize ;
         if ( timeout < 0 && SDB_TIMEOUT == rc )
         {
            continue ;
         }
         break ;
      }

   done :
#if defined ( SDB_ENGINE )
      if ( totalSentSize > 0 )
      {
         pmdGetKRCB()->getMonDBCB()->svcNetOutAdd( totalSentSize ) ;
      }
#endif // SDB_ENGINE
      if ( pSentLen )
      {
         *pSentLen = totalSentSize ;
      }
      return rc ;
   }

   INT32 _pmdSession::recvData( CHAR * pData, INT32 size, INT32 timeout,
                                BOOLEAN block, INT32 *pRecvLen, INT32 flags )
   {
      INT32 rc = SDB_OK ;
      INT32 receivedSize = 0 ;
      INT32 totalReceivedSize = 0 ;
      INT32 realTimeout = timeout < 0 ? OSS_SOCKET_DFT_TIMEOUT : timeout ;

      while ( TRUE )
      {
         if ( _pEDUCB )
         {
            if ( _pEDUCB->isForced () )
            {
               rc = SDB_APP_FORCED ;
               goto done ;
            }
            _pEDUCB->resetInterrupt() ;
         }
         rc = _socket.recv ( &pData[totalReceivedSize], size-totalReceivedSize,
                             receivedSize, realTimeout, flags, block ) ;
         totalReceivedSize += receivedSize ;
         if ( timeout < 0 && SDB_TIMEOUT == rc )
         {
            continue ;
         }
         break ;
      }

   done :
#if defined ( SDB_ENGINE )
      if ( totalReceivedSize > 0 )
      {
         pmdGetKRCB()->getMonDBCB()->svcNetInAdd( totalReceivedSize ) ;
      }
#endif // SDB_ENGINE
      if ( pRecvLen )
      {
         *pRecvLen = totalReceivedSize ;
      }
      return rc ;
   }

   INT32 _pmdSession::sniffData( INT32 timeout )
   {
      CHAR buff[ 4 ] = { 0 } ;
      INT32 recvLen  = 0 ;

      return _socket.recv( buff, sizeof( buff ), recvLen,
                           timeout, MSG_PEEK, TRUE, TRUE ) ;
   }

   /*
      _pmdProcessor implement
   */
   _pmdProcessor::_pmdProcessor()
   {
      _pSession = NULL ;
   }

   _pmdProcessor::~_pmdProcessor()
   {
      _pSession = NULL ;
   }

   void _pmdProcessor::attachSession( pmdSession *pSession )
   {
      SDB_ASSERT( pSession, "Session can't be NULL" ) ;
      _pSession = pSession ;
      _onAttach() ;
   }

   void _pmdProcessor::detachSession()
   {
      SDB_ASSERT( _pSession, "Session can't be NULL" ) ;
      _onDetach() ;
      _pSession = NULL ;
   }

   _dpsLogWrapper* _pmdProcessor::getDPSCB()
   {
      if ( _pSession )
      {
         return _pSession->getDPSCB() ;
      }
      return NULL ;
   }

   _IClient* _pmdProcessor::getClient()
   {
      if ( _pSession )
      {
         return _pSession->getClient() ;
      }
      return NULL ;
   }

   _pmdEDUCB* _pmdProcessor::eduCB()
   {
      if ( _pSession )
      {
         return _pSession->eduCB() ;
      }
      return NULL ;
   }

   EDUID _pmdProcessor::eduID() const
   {
      if ( _pSession )
      {
         return _pSession->eduID() ;
      }
      return PMD_INVALID_EDUID ;
   }

}


