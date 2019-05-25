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

   Source File Name = ossASIO.cpp

   Descriptive Name = Operating System Services ASync IO

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for creating a ASIO
   listening object. Users must pass-in callback functions for data receiving.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossASIO.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
_ossAsioMsgProcessor::~_ossAsioMsgProcessor ()
{
   if ( _startRun && _onDisconnect )
      _onDisconnect ( NULL, NULL, &_socket ) ;
   if ( _reply && _reply != _message )
   {
      SDB_OSS_FREE ( _reply ) ;
      _reply = NULL ;
   }
   if ( _message )
   {
      SDB_OSS_FREE ( _message ) ;
      _message = NULL ;
   }
   _socket.close() ;
}
PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIOMSGPROC__PROC, "_ossAsioMsgProcessor::_process" )
void _ossAsioMsgProcessor::_process()
{
   PD_TRACE_ENTRY ( SDB__OSSAIOMSGPROC__PROC );
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "_process" ) ;
   if ( _startRun && _onReceive )
      _onReceive ( _message, &_reply, &_socket ) ;
   if ( _reply )
   {
      async_write ( _socket,
                    buffer ( _reply,
                             ((MsgReplHeader*)_reply)->messageLength ),
                    boost::bind ( &_ossAsioMsgProcessor::_handleWritePacket,
                                  shared_from_this(),
                                  boost::asio::placeholders::error )
                  ) ;
   }
   _readPacketHead() ;
   PD_TRACE_EXIT ( SDB__OSSAIOMSGPROC__PROC );
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIOMSGPROC__HNDWP, "_ossAsioMsgProcessor::_handleWritePacket" )
void _ossAsioMsgProcessor::_handleWritePacket (
      const boost::system::error_code &error )
{
   PD_TRACE_ENTRY ( SDB__OSSAIOMSGPROC__HNDWP );
   pdLog ( PDEVENT, __FUNC__, __FILE__, __LINE__,
           "Finished writing output" ) ;
   if ( error )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Error received when handling accept" ) ;
      goto done ;
   }
   if ( _reply && _reply != _message )
   {
      SDB_OSS_FREE ( _reply ) ;
      _reply = NULL ;
   }
   if ( _message )
   {
      SDB_OSS_FREE ( _message ) ;
      _message = NULL ;
   }
done :
   PD_TRACE_EXIT ( SDB__OSSAIOMSGPROC__HNDWP );
}

void _ossAsioMsgProcessor::_handleReadPacketBody (
      const boost::system::error_code &error )
{
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "Data received" ) ;
   _process() ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIOMSGPROC__HNDRPH, "_ossAsioMsgProcessor::_handleReadPacketHead" )
void _ossAsioMsgProcessor::_handleReadPacketHead (
      const boost::system::error_code &error )
{
   PD_TRACE_ENTRY ( SDB__OSSAIOMSGPROC__HNDRPH );
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "_handleReadPacketHead" ) ;
   if ( error )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Error received when handling packet head" ) ;
      goto done ;
   }
   if ( 0 == _header.messageLength )
      goto done ;
   if ( sizeof(CHAR)*_header.messageLength > (UINT32)_bufferLen )
   {
      if ( _message )
      {
         SDB_OSS_FREE ( _message ) ;
         _message = NULL ;
         _bufferLen = 0 ;
      }
   }
   if ( !_message )
   {
      INT32 bufSize = ossRoundUpToMultipleX ( _header.messageLength,
                                              SDB_PAGE_SIZE ) ;
      _message = (CHAR*)SDB_OSS_MALLOC ( bufSize ) ;
      if ( !_message )
      {
         pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                 "Failed to allocate memory" ) ;
         goto done ;
      }
      _bufferLen = bufSize ;
   }

   ossMemcpy ( _message, (CHAR*)&_header, sizeof(_header) ) ;
   async_read ( _socket,
                buffer ( _message + sizeof(_header), _header.messageLength -
                         sizeof(_header) ),
                boost::bind ( &_ossAsioMsgProcessor::_handleReadPacketBody,
                              shared_from_this(),
                              boost::asio::placeholders::error ) ) ;
done :
   PD_TRACE_EXIT ( SDB__OSSAIOMSGPROC__HNDRPH );
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIOMSGPROC__RDPH, "_ossAsioMsgProcessor::_readPacketHead" )
void _ossAsioMsgProcessor::_readPacketHead ()
{
   PD_TRACE_ENTRY ( SDB__OSSAIOMSGPROC__RDPH );
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "_readPacketHead" ) ;
   _header.messageLength = 0 ;
   async_read ( _socket,
                buffer ( (CHAR*)&_header, sizeof(_header) ),
                boost::bind ( &_ossAsioMsgProcessor::_handleReadPacketHead,
                              shared_from_this(),
                              boost::asio::placeholders::error ) ) ;
   PD_TRACE_EXIT ( SDB__OSSAIOMSGPROC__RDPH );
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIOMSGPROC_CONNECT, "_ossAsioMsgProcessor::connect" )
INT32 _ossAsioMsgProcessor::connect ( CHAR *pHostName, CHAR *pServiceName )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSAIOMSGPROC_CONNECT );
   boost::system::error_code ec = boost::asio::error::host_not_found ;
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "Connect to %s: %s", pHostName, pServiceName ) ;
   if ( _socket.is_open() )
   {
      _socket.close() ;
   }
   try
   {
      tcp::resolver resolver ( *_ioservice ) ;
      tcp::resolver::query query ( pHostName, pServiceName ) ;
      tcp::resolver::iterator iter = resolver.resolve ( query ) ;
      tcp::resolver::iterator end ;
      while ( ec && iter != end )
      {
         ip::tcp::endpoint endpoint = *iter++ ;
         _socket.close() ;
         _socket.connect ( endpoint, ec ) ;
         if ( !ec )
            break ;
      }
   }
   catch ( std::exception &e )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Failed to connect to %s: %s: %s", pHostName, pServiceName,
              e.what() ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }
   if ( ec )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Failed to connect to %s: %s", pHostName, pServiceName ) ;
      rc = SDB_NETWORK ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB__OSSAIOMSGPROC_CONNECT, rc );
   return rc ;
error :
   goto done ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB__TMPAIR_CHK_DLINE, "_timerPair::check_deadline" )
void _timerPair::check_deadline()
{
   PD_TRACE_ENTRY ( SDB__TMPAIR_CHK_DLINE );
   if ( _timer.expires_at() <= boost::asio::steady_timer::clock_type::now() )
   {
      if ( _onTimer )
         _onTimer ( NULL, NULL, NULL ) ;
   }
   _timer.expires_from_now ( boost::chrono::milliseconds (_timeoutMS)) ;
   _timer.async_wait ( boost::bind ( &_timerPair::check_deadline,
                       this ) ) ;
   PD_TRACE_EXIT ( SDB__TMPAIR_CHK_DLINE );
}

PD_TRACE_DECLARE_FUNCTION ( SDB__TMPAIR_RUN, "_timerPair::run" )
void _timerPair::run ()
{
   PD_TRACE_ENTRY ( SDB__TMPAIR_RUN );
   _timer.expires_from_now ( boost::chrono::milliseconds (_timeoutMS)) ;
   if ( _onTimer )
         _onTimer ( NULL, NULL, NULL ) ;
   _timer.async_wait ( boost::bind (&_timerPair::check_deadline,
                       this ) ) ;
   PD_TRACE_EXIT ( SDB__TMPAIR_RUN );
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIO__HNDAPT, "_ossASIO::_handleAccept" )
void _ossASIO::_handleAccept ( boost::shared_ptr<ossAsioMsgProcessor> processor,
                               const boost::system::error_code &error )
{
   PD_TRACE_ENTRY ( SDB__OSSAIO__HNDAPT );
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "_handleAccept" ) ;
   if ( error )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Error received when handling accept" ) ;
      goto done ;
   }
   processor->run() ;
   _accept() ;
done :
   PD_TRACE_EXIT ( SDB__OSSAIO__HNDAPT );
   return ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIO__ACCEPT, "_ossASIO::_accept" )
void _ossASIO::_accept()
{
   PD_TRACE_ENTRY ( SDB__OSSAIO__ACCEPT );
   pdLog ( PDDEBUG, __FUNC__, __FILE__, __LINE__,
           "_accept" ) ;
   boost::shared_ptr<ossAsioMsgProcessor> processor ( SDB_OSS_NEW
         ossAsioMsgProcessor ( _onReceive, _onDisconnect, _ioservice ) ) ;
   _acceptor.async_accept ( processor->socket(),
                            boost::bind ( &ossASIO::_handleAccept,
                                          this,
                                          processor,
                                          boost::asio::placeholders::error )
                          ) ;
   PD_TRACE_EXIT ( SDB__OSSAIO__ACCEPT );
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIO_CONNECT, "_ossASIO::connect" )
INT32 _ossASIO::connect ( CHAR *pHostName, CHAR *pServiceName,
                         tcp::socket **sock )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSAIO_CONNECT );
   SDB_ASSERT ( pHostName, "hostname can't be NULL" ) ;
   SDB_ASSERT ( pServiceName, "service name can't be NULL" ) ;
   SDB_ASSERT ( sock, "sock can't be NULL" ) ;
   boost::shared_ptr<ossAsioMsgProcessor> processor ( SDB_OSS_NEW
        ossAsioMsgProcessor ( _onReceive, _onDisconnect, _ioservice ) ) ;
   rc = processor->connect ( pHostName, pServiceName ) ;
   if ( !rc )
   {
      *sock = &processor->socket() ;
      processor->run() ;
   }
   else
   {
      *sock = NULL ;
   }
   PD_TRACE_EXITRC ( SDB__OSSAIO_CONNECT, rc );
   return rc ;
}

_ossASIO::_ossASIO ( INT32 port, ossAsioProcessFunction onReceive,
                     ossAsioProcessFunction onDisconnect ) :
   _maxTimer ( 0 ),
   _endpoint ( tcp::v4(), port ),
   _acceptor ( _ioservice, _endpoint )
{
   _port = port ;
   _onReceive = onReceive ;
   _onDisconnect = onDisconnect ;
   if ( _port >= 0 )
      _accept() ;
}
_ossASIO::~_ossASIO ()
{
   std::map<UINT32, timerPair*>::iterator it ;
   for ( it = _timerList.begin(); it != _timerList.end(); ++it )
   {
      SDB_OSS_DEL (*it).second ;
   }
   _timerList.clear() ;
}
void _ossASIO::run ()
{
   _ioservice.run() ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIO_ADDTIMER, "_ossASIO::addTimer" )
INT32 _ossASIO::addTimer ( UINT32 timeoutMS,
                           ossAsioProcessFunction onTimer, UINT32 &timerID )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB__OSSAIO_ADDTIMER );
   timerPair *pair = SDB_OSS_NEW timerPair ( timeoutMS, onTimer, _ioservice ) ;
   if ( !pair )
   {
      pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
              "Failed to allocate timerPair" ) ;
      rc = SDB_OOM ;
      goto error ;
   }
   timerID = _maxTimer.inc() ;
   _mutex.get() ;
   _timerList [ timerID ] = pair ;
   _mutex.release() ;
   pair->run() ;
done :
   PD_TRACE_EXITRC ( SDB__OSSAIO_ADDTIMER, rc );
   return rc ;
error :
   goto done ;
}

PD_TRACE_DECLARE_FUNCTION ( SDB__OSSAIO_RMTIMER, "_ossASIO::removeTimer" )
void _ossASIO::removeTimer ( UINT32 timerID )
{
   PD_TRACE_ENTRY ( SDB__OSSAIO_RMTIMER );
   std::map<UINT32, timerPair*>::iterator it ;
   _mutex.get () ;
   if ( _timerList.end() != (it=_timerList.find ( timerID ) ) )
   {
      SDB_OSS_DEL (*it).second ;
   }
   _mutex.release() ;
   PD_TRACE_EXIT ( SDB__OSSAIO_RMTIMER );
}
