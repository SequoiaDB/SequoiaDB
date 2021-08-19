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

   Source File Name = netFrame.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef NETFRAME_HPP_
#define NETFRAME_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "monLatch.hpp"
#include "netMsgHandler.hpp"
#include "netEventSuit.hpp"
#include "netEventHandler.hpp"
#include "netUDPEventSuit.hpp"
#include "netUDPEventHandler.hpp"
#include "netInnerTimer.hpp"
#include "netTimer.hpp"
#include "ossAtomic.hpp"
#include "sdbInterface.hpp"
#include "ossMemPool.hpp"

#include <map>
#include <vector>

using namespace std ;
namespace engine
{

   typedef INT32 (*NET_START_THREAD_FUNC)( _netEventSuit *pSuit ) ;

   #define NET_HEARTBEAT_INTERVAL            ( 5000 )
   #define NET_MAKE_STAT_INTERVAL            ( 5000 )

   #define NET_FRAME_MASK_EMPTY              ( 0x00000000 )
   #define NET_FRAME_MASK_TCP                ( 0x00000001 )
   #define NET_FRAME_MASK_UDP                ( 0x00000002 )
   #define NET_FRAME_MASK_ALL                ( 0xffffffff )

   /*
     _netEHSegment define
     This class is only used by netFrame as the container/manager of
     netEventHandler
   */
   class _netEHSegment ;
   typedef class _netEHSegment netEHSegment ;
   typedef utilSharePtr<netEHSegment> netEHSegPtr ;

   class _netEHSegment : public SDBObject
   {
      typedef vector<NET_EH> VEC_EH ;
      typedef vector<NET_EH>::iterator VEC_EH_IT ;

      friend class _netFrame ;

      public:
         _netEHSegment( _netFrame *pFrame,
                        UINT32 capacity,
                        const _MsgRouteID &id ) ;
         ~_netEHSegment() ;

         static netEHSegPtr createShared( netFrame *pFrame,
                                          UINT32 capacity,
                                          const MsgRouteID &id ) ;

      public:
         INT32 getEH( NET_EH &eh ) ;

         void close() ;

         void addEH( NET_EH eh ) ;

         void delEH( const NET_HANDLE& handle ) ;

         OSS_INLINE BOOLEAN isEmpty()
         {
            _mtx.get_shared() ;
            BOOLEAN isEmpty = (_vecEH.size() == 0) ? TRUE : FALSE ;
            _mtx.release_shared() ;
            return isEmpty ;
         }

      private:
         BOOLEAN _createEH( NET_EH &eh) ;

      private:
         _netFrame                        *_pFrame ;
         _MsgRouteID                      _id ;
         //total allowed items in the container, is based on config parameter
         UINT32                           _capacity ;
         ossAtomic32                      _index ; // point to a proper item
         monSpinSLatch                    _mtx ;
         VEC_EH                           _vecEH ; // EV handle list
   } ;

   /*
      _netFrame define
   */
   class _netFrame : public IIOService
   {
      typedef map<UINT32, NET_TH>         MAP_TIMMER ;
      typedef MAP_TIMMER::iterator        MAP_TIMMER_IT ;

      typedef vector<netEvSuitPtr>        VEC_EVSUIT ;
      typedef VEC_EVSUIT::iterator        VEC_EVSUIT_IT ;

      typedef map<NET_HANDLE, NET_EH>     MAP_EVENT ;
      typedef MAP_EVENT::iterator         MAP_EVENT_IT ;

      typedef map<UINT64, netEHSegPtr>    MAP_ROUTE ;
      typedef MAP_ROUTE::iterator         MAP_ROUTE_IT ;
      typedef pair<MAP_ROUTE_IT, MAP_ROUTE_IT>  MULMAP_ROUTE_IT_PAIR ;

      typedef vector<NET_EH> VEC_EH ;
      typedef vector<NET_EH>::iterator VEC_EH_IT ;

      friend class _netRestartTimer ;
      friend class _netUDPRestartTimer ;
      friend class _netEventHandler ;
      friend class _netEHSegment ;
      friend class _netUDPEventHandler ;
      friend class _netUDPEventSuit ;

      public:
         /// handler will not be freed by frame
         _netFrame( _netMsgHandler *handler, _netRoute *pRoute ) ;

         ~_netFrame() ;

      public:
         virtual INT32     run() ;
         virtual void      stop() ;
         virtual void      resetMon() ;

      public:
         OSS_INLINE void setLocal( const MsgRouteID &id )
         {
            _local = id ;
         }

         OSS_INLINE const MsgRouteID& getLocal() const
         {
            return _local ;
         }

         OSS_INLINE io_service &getIOService()
         {
            return _mainSuitPtr->getIOService() ;
         }

         void     setMaxSockPerNode( UINT32 maxSockPerNode ) ;
         void     setMaxSockPerThread( UINT32 maxSockPerThread ) ;
         void     setMaxThreadNum( UINT32 maxThreadNum ) ;

         UINT32   getSockNumByNode( const _MsgRouteID &nodeID ) ;
         UINT32   getSockNum() ;
         UINT32   getNodeNum() ;
         UINT32   getThreadNum() ;

      public:
         void     onRunSuitStart( netEvSuitPtr evSuitPtr ) ;
         void     onRunSuitStop( netEvSuitPtr evSuitPtr ) ;
         void     onSuitTimer( netEvSuitPtr evSuitPtr ) ;
         UINT32   getEvSuitSize() ;
         OSS_INLINE netEvSuitPtr getMainSuit()
         {
            return _mainSuitPtr ;
         }

      public:
         // return 0 if error happened
         static UINT32 getCurrentLocalAddress() ;
         static UINT32 getLocalAddress() ;

      public:
         void     heartbeat( UINT32 interval, INT32 serviceType = -1 ) ;

         void     setBeatInfo( UINT32 beatTimeout,
                               UINT32 beatInteval = 0 ) ;

         NET_EH   getEventHandle( const NET_HANDLE &handle ) ;
         NET_EVENT_HANDLER_TYPE getEventHandleType( const NET_HANDLE &handle ) ;

         INT32    listen( const CHAR *hostName,
                          const CHAR *serviceName,
                          UINT32 protocolMask = NET_FRAME_MASK_TCP,
                          INetUDPMsgHandler *udpHandler = NULL,
                          UINT32 udpBufferSize = NET_UDP_DEFAULT_BUFFER_SIZE ) ;

         /// if call this func with same params for twice,
         /// will create two connections.
         /// the connection will be maintained until the
         /// disconnect happens.
         INT32 syncConnect( const CHAR *hostName,
                            const CHAR *serviceName,
                            const _MsgRouteID &id,
                            NET_HANDLE *pHandle = NULL ) ;

         INT32 syncConnect( const _MsgRouteID &id,
                            NET_HANDLE *pHandle = NULL ) ;

         INT32 syncConnect( NET_EH &eh ) ;

         INT32 syncSend( const NET_HANDLE &handle,
                         void *header ) ;

         INT32 syncSendRaw( const NET_HANDLE &handle,
                            const CHAR *pBuff,
                            UINT32 buffSize ) ;

         INT32 syncSend( const _MsgRouteID &id,
                         void *header,
                         NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSend( const NET_HANDLE &handle,
                         MsgHeader *header,
                         const void *body,
                         UINT32 bodyLen ) ;

         INT32 syncSend( const _MsgRouteID &id,
                         MsgHeader *header,
                         const void *body,
                         UINT32 bodyLen,
                         NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSendv( const _MsgRouteID &id,
                          MsgHeader *header,
                          const netIOVec &iov,
                          NET_HANDLE *pHandle = NULL ) ;

         INT32 syncSendv( const NET_HANDLE &handle,
                          MsgHeader *header,
                          const netIOVec &iov ) ;

         INT32 syncSendUDP( const MsgRouteID &id,
                            void *header ) ;

         /// frame will not release handler for ever
         INT32 addTimer( UINT32 millsec, _netTimeoutHandler *handler,
                         UINT32 &timerid );

         INT32 removeTimer( UINT32 timerid ) ;

         void  close( const _MsgRouteID &id ) ;

         void  close( const NET_HANDLE &handle, MsgRouteID *pID = NULL ) ;

         void  close() ;

         INT32 closeListen ( UINT32 protocolMask = NET_FRAME_MASK_ALL ) ;

         void  handleMsg( NET_EH eh ) ;

         void  handleClose( NET_EH eh, _MsgRouteID id ) ;

         INT64 netIn() ;

         INT64 netOut() ;

         void  makeStat( UINT32 timeout ) ;
         void  setNetStartThreadFunc( NET_START_THREAD_FUNC pFunc ) ;

         OSS_INLINE UINT32 getProtocolMask() const
         {
            return _protocolMask ;
         }

         OSS_INLINE BOOLEAN listeningUDP() const
         {
            return OSS_BIT_TEST( _protocolMask, NET_FRAME_MASK_UDP ) ;
         }

         OSS_INLINE BOOLEAN listeningTCP() const
         {
            return OSS_BIT_TEST( _protocolMask, NET_FRAME_MASK_TCP ) ;
         }

         OSS_INLINE NET_HANDLE allocateHandler()
         {
            return (NET_HANDLE)( _handle.inc() ) ;
         }

      protected:
         netEvSuitPtr      _getEvSuit( BOOLEAN needLock ) ;
         void              _stopAllEvSuit() ;
         void              _eraseSuit_i( netEvSuitPtr &ptr ) ;

         INT32             _getHandle( const _MsgRouteID &id,
                                       NET_EH &eh ) ;

         NET_EH            _createEvHandler() ;

         void              _addOpposite( NET_EH eh ) ;

         INT32             _listenTCP( const CHAR *hostName,
                                       const CHAR *serviceName ) ;
         INT32             _listenUDP( const CHAR *hostName,
                                       const CHAR *serviceName,
                                       INetUDPMsgHandler *handler,
                                       UINT32 bufferSize ) ;

      private:
         INT32    _asyncAccept() ;
         void     _acceptCallback( NET_EH eh,
                                   const boost::system::error_code &error ) ;

         void     _erase( const NET_HANDLE &handle ) ;

         void     _addRoute( NET_EH eh ) ;

         void     _heartbeat( INT32 serviceType ) ;
         void     _handleHeartBeat( NET_EH eh, MsgHeader *message ) ;
         void     _handleHeartBeatRes( NET_EH eh, MsgHeader *message ) ;
         void     _checkBreak( UINT32 timeout, INT32 serviceType ) ;

      private:
         UINT32                           _protocolMask ;
         _netRoute                        *_pRoute ;
         netEvSuitPtr                     _mainSuitPtr ;
         NET_START_THREAD_FUNC            _pThreadFunc ;

         monSpinSLatch                    _suiteMtx ;
         VEC_EVSUIT                       _vecEvSuit ;

         MAP_ROUTE                        _route ;
         MAP_EVENT                        _opposite ;

         MAP_TIMMER                       _timers ;

         _netMsgHandler                   *_handler ;
         MsgRouteID                       _local ;
         monSpinSLatch                    _mtx ;
         boost::asio::ip::tcp::acceptor   _acceptor ;
         _ossAtomic32                     _handle ;
         UINT32                           _timerID;
         ossAtomicSigned64                _netOut;
         ossAtomicSigned64                _netIn;

         UINT32                           _beatInterval ;
         UINT32                           _beatTimeout ;
         UINT64                           _beatLastTick ;
         BOOLEAN                          _checkBeat ;

         netRestartTimer                  _restartTimer ;

         /// communicate shedule config info
         UINT32                           _statInterval ;
         UINT64                           _statLastTick ;
         UINT32                           _maxSockPerNode ;    /// 0 for unlimited
         UINT32                           _maxSockPerThread ;  /// 0 for unlimited
         UINT32                           _maxThreadNum ;

         monRWMutex                       _suiteExitMutex ;
         BOOLEAN                          _suiteStopFlag ;

         NET_UDP_EV_SUIT                  _udpMainSuit ;
   } ;

}

#endif
