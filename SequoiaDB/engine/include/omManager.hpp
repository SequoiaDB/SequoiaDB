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

   Source File Name = omManager.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_MANAGER_HPP__
#define OM_MANAGER_HPP__

#include "omDef.hpp"
#include "ossLatch.hpp"
#include "pmdObjBase.hpp"
#include "sdbInterface.hpp"
#include "pmdEDU.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "rtnCB.hpp"
#include "netRouteAgent.hpp"
#include "pmdRemoteSession.hpp"
#include "pmdRemoteMsgEventHandler.hpp"
#include "ossMemPool.hpp"
#include "msg.hpp"

#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std ;
using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
      omAgentInfo define
   */
   struct omAgentInfo
   {
      UINT64   _id ;
      string   _host ;
      string   _service ;
   } ;

   class omHostVersion ;
   class omTaskManager ;
   /*
      _omManager define
   */
   class _omManager : public _pmdObjBase, public _IControlBlock,
                      public IEventHander
   {
      DECLARE_OBJ_MSG_MAP()

      typedef map< UINT64, omAgentInfo* >    MAP_ID2HOSTPTR ;
      typedef MAP_ID2HOSTPTR::iterator       MAP_ID2HOSTPTR_IT ;

      typedef map< string, omAgentInfo >     MAP_HOST2ID ;
      typedef MAP_HOST2ID::iterator          MAP_HOST2ID_IT ;

      typedef ossPoolMap< SINT64, UINT64>    CONTEXT_LIST ;

      public:
         _omManager() ;
         virtual ~_omManager() ;

         virtual SDB_CB_TYPE cbType() const { return SDB_CB_OMSVC ; }
         virtual const CHAR* cbName() const { return "OMSVC" ; }

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() {}

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         UINT32      setTimer( UINT32 milliSec ) ;
         void        killTimer( UINT32 timerID ) ;

         // comm interface
         netRouteAgent* getRouteAgent() ;
         MsgRouteID     updateAgentInfo( const string &host,
                                         const string &service ) ;
         MsgRouteID     getAgentIDByHost( const string &host ) ;
         INT32          getHostInfoByID( MsgRouteID routeID,
                                         string &host,
                                         string &service ) ;
         void           delAgent( MsgRouteID routeID ) ;
         void           delAgent( const string &host ) ;

         pmdRemoteSessionMgr* getRSManager() { return &_rsManager ; }

         INT32             md5Authenticate( const BSONObj &obj,
                                            _pmdEDUCB *cb,
                                            BSONObj &outUserObj ) ;
         INT32             SCRAMSHAAuthenticate( const BSONObj &obj,
                                                 _pmdEDUCB *cb,
                                                 BSONObj &outUserObj ) ;

         INT32             authUpdatePasswd( string user, string oldPasswd,
                                             string newPasswd, pmdEDUCB *cb ) ;

         INT32             getBizHostInfo( const string &businessName,
                                           list <string> &hostsList ) ;
         INT32             appendBizHostInfo( const string &businessName,
                                              list <string> &hostsList ) ;

         string            getLocalAgentPort() ;

         INT32             refreshVersions() ;
         void              updateClusterVersion( string cluster ) ;
         void              removeClusterVersion( string cluster ) ;
         void              updateClusterHostFilePrivilege( string clusterName,
                                                           BOOLEAN privilege ) ;
         void              getPluginPasswd( string &passwd ) ;

         void              getUpdatePluginPasswdTimeDiffer( INT64 &differ ) ;

         omTaskManager     *getTaskManager() ;

      public:
         virtual void   onRegistered( const MsgRouteID &nodeID ) ;
         virtual void   onPrimaryChange( BOOLEAN primary,
                                         SDB_EVENT_OCCUR_TYPE occurType ) ;

      protected:
         virtual void      onTimer ( UINT64 timerID, UINT32 interval ) ;
         virtual INT32     _defaultMsgFunc( NET_HANDLE handle,
                                            MsgHeader *pMsg ) ;
         void              _onMsgBegin( MsgHeader *pMsg ) ;
         void              _onMsgEnd() ;
         INT32             _reply ( const NET_HANDLE &handle,
                                    MsgOpReply *pReply,
                                    const CHAR *pReplyData = NULL,
                                    UINT32 replyDataLen = 0 ) ;

         MsgRouteID        _incNodeID() ;

         INT32             _initOmTables() ;

         void              _getOMVersion( string &version ) ;

         INT32             _appendBusinessInfo( const string &businessName,
                                                const string &businessType,
                                                const string &clusterName,
                                                const string &deployMode ) ;

         INT32             _getBussinessInfo( const string &businessName,
                                              string &businessType,
                                              string &clusterName,
                                              string &deployMode ) ;

         INT32             _appendHostPackage( const string &hostName,
                                               const BSONObj &packageInfo ) ;

         INT32             _updateConfTable() ;
         INT32             _updateBusinessTable() ;
         INT32             _updateClusterTable() ;
         INT32             _updateHostTable() ;
         INT32             _updatePluginIndex() ;
         INT32             _updateAuthTable() ;
         INT32             _updateTable() ;

         INT32             _createJobs() ;

         void              _readAgentPort() ;

         BOOLEAN           _isCommand( const CHAR *pCheckName ) ;

         void              _sendResVector2Agent( NET_HANDLE handle,
                                                 MsgHeader *pSrcMsg,
                                                 INT32 flag,
                                                 vector < BSONObj > &objs,
                                                 INT64 contextID = -1 ) ;

         void              _sendRes2Agent( NET_HANDLE handle,
                                           MsgHeader *pSrcMsg,
                                           INT32 flag,
                                           const BSONObj &obj,
                                           INT64 contextID = -1 ) ;

         void              _sendRes2Agent( NET_HANDLE handle,
                                           MsgHeader *pSrcMsg,
                                           INT32 flag,
                                           const rtnContextBuf &buffObj,
                                           INT64 contextID = -1 ) ;

         void              _checkTaskTimeout( const BSONObj &task ) ;

         INT32             _updatePluginPasswd() ;

         void              _createVersionFile() ;

      private:
         INT32             _appendClusterGrant( const string& clusertName,
                                                const string& grantName,
                                                BOOLEAN privilege ) ;

         void              _addContext( const UINT32 &handle,
                                        UINT32 tid,
                                        INT64 contextID ) ;

         void              _delContextByHandle( const UINT32 &handle ) ;

         void              _delContext( const UINT32 &handle,
                                        UINT32 tid ) ;

         void              _delContextByID( INT64 contextID, BOOLEAN rtnDel ) ;

      // Msg functions
      protected:
         INT32             _processMsg( const NET_HANDLE &handle,
                                        MsgHeader *pMsg ) ;

         INT32             _processQueryMsg( MsgHeader *pMsg,
                                             rtnContextBuf &buf,
                                             INT64 &contextID ) ;

         INT32             _processGetMoreMsg( MsgHeader *pMsg,
                                               rtnContextBuf &buf,
                                               INT64 &contextID ) ;

         INT32             _processAdvanceMsg( MsgHeader *pMsg,
                                               rtnContextBuf &buf,
                                               INT64 &contextID ) ;

         INT32             _processKillContext( MsgHeader *pMsg ) ;

         INT32             _processSessionInit( MsgHeader *pMsg ) ;

         INT32             _processDisconnectMsg( NET_HANDLE handle,
                                                  MsgHeader *pMsg ) ;

         INT32             _processInterruptMsg( NET_HANDLE handle,
                                                 MsgHeader *pMsg ) ;

         INT32             _processRemoteDisc( NET_HANDLE handle,
                                               MsgHeader *pMsg ) ;

         INT32             _processPacketMsg( const NET_HANDLE &handle,
                                              MsgHeader *pMsg,
                                              INT64 &contextID,
                                              rtnContextBuf &buf ) ;

         INT32             _onAgentUpdateTaskReq( NET_HANDLE handle,
                                                  MsgHeader *pMsg ) ;

      private:

         MAP_ID2HOSTPTR                         _mapID2Host ;
         MAP_HOST2ID                            _mapHost2ID ;
         MsgRouteID                             _hwRouteID ;

         ossSpinSLatch                          _omLatch ;
         ossEvent                               _attachEvent ;

         pmdRemoteSessionMgr                    _rsManager ;

         pmdRemoteMsgHandler                    _msgHandler ;
         pmdRemoteTimerHandler                  _timerHandler ;
         netRouteAgent                          _netAgent ;
         MsgRouteID                             _myNodeID ;

         SDB_DMSCB*                             _pDmsCB ;
         SDB_RTNCB*                             _pRtnCB ;
         pmdEDUCB                               *_pEDUCB ;

         string                                 _wwwRootPath ;

         string                                 _localAgentPort ;
         omHostVersion                          *_hostVersion ;

         omTaskManager                          *_taskManager ;

         CONTEXT_LIST                           _contextLst;
         ossSpinXLatch                          _contextLatch ;
         MsgOpReply                             _replyHeader ;
         BOOLEAN                                _needReply ;
         BSONObj                                _errorInfo ;
         UINT32                                 _inPacketLevel ;
         INT64                                  _pendingContextID ;
         rtnContextBuf                          _pendingBuff ;

         INT64                                  _updateTimestamp ;
         UINT64                                 _updatePluinUsrTimer ;

         BOOLEAN                                _isInitTable ;

         string                                 _usrPluginPasswd ;
   } ;

   typedef _omManager omManager ;
   /*
      get the global om manager object point
   */
   omManager *sdbGetOMManager() ;

}

#endif // OM_MANAGER_HPP__

