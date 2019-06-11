/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = seAdptMgr.hpp

   Descriptive Name = Search engine adapter manager.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SE_ADPTMGR_HPP_
#define SE_ADPTMGR_HPP_

#include "sdbInterface.hpp"
#include "seAdptOptionsMgr.hpp"
#include "pmdAsyncSession.hpp"
#include "pmdAsyncHandler.hpp"
#include "utilESCltMgr.hpp"
#include "seAdptMsgHandler.hpp"
#include "seAdptIdxMetaMgr.hpp"

using namespace engine ;

namespace seadapter
{
   class _seAdptCB ;

   enum SEADPT_SESSION_TYPE
   {
      SEADPT_SESSION_INDEX = 1      // Indexer session for one text index.
   } ;

   struct _seAdptSessionInfo
   {
      SEADPT_SESSION_TYPE  type ;
      INT32                startType ;
      UINT64               sessionID ;
      void*                data ;
   } ;
   typedef _seAdptSessionInfo seAdptSessionInfo ;

   /*
    * Service sessions manager. It manages the sessions which started by client
    * connections, or connections for text search from SDB data node.
    * One listener is created during startup, and one seperate session will be
    * created for each client connection.
    */
   class _seSvcSessionMgr : public pmdAsycSessionMgr
   {
   public:
      _seSvcSessionMgr() {}
      virtual ~_seSvcSessionMgr() {}

      virtual UINT64 makeSessionID( const NET_HANDLE &handle,
                                    const MsgHeader *header ) ;

      virtual INT32  onErrorHanding( INT32 rc,
                                     const MsgHeader *pReq,
                                     const NET_HANDLE &handle,
                                     UINT64 sessionID,
                                     pmdAsyncSession *pSession ) ;

   protected:
      virtual SDB_SESSION_TYPE _prepareCreate( UINT64 sessionID,
                                               INT32 startType,
                                               INT32 opCode ) ;

      virtual BOOLEAN _canReuse( SDB_SESSION_TYPE sessionType )
      {
         return FALSE ;
      }
      virtual UINT32 _maxCacheSize() const
      {
         return 0 ;
      }

      virtual pmdAsyncSession* _createSession( SDB_SESSION_TYPE sessionType,
                                               INT32 startType,
                                               UINT64 sessionID,
                                               void *data = NULL ) ;
   } ;
   typedef _seSvcSessionMgr seSvcSessionMgr ;

   typedef pair<const UINT64, seIndexMeta>      TASK_SESSION_ITEM ;
   class _seIndexSessionMgr : public pmdAsycSessionMgr
   {
      typedef map<UINT64, seIndexMeta>          TASK_SESSION_MAP ;
      typedef TASK_SESSION_MAP::iterator        TASK_SESSION_MAP_ITR ;
   public:
      _seIndexSessionMgr( _seAdptCB *pAdptCB ) ;
      virtual ~_seIndexSessionMgr() ;

      virtual UINT64 makeSessionID( const NET_HANDLE &handle,
                                    const MsgHeader *header ) ;

      virtual INT32  onErrorHanding( INT32 rc,
                                     const MsgHeader *pReq,
                                     const NET_HANDLE &handle,
                                     UINT64 sessionID,
                                     pmdAsyncSession *pSession ) ;

      virtual void onSessionDestoryed( pmdAsyncSession *pSession ) ;

      INT32 refreshTasks( BSONObj &obj ) ;
      void  stopAllIndexer() ;

   protected:
      virtual SDB_SESSION_TYPE _prepareCreate( UINT64 sessionID,
                                               INT32 startType,
                                               INT32 opCode ) ;
      virtual BOOLEAN _canReuse( SDB_SESSION_TYPE sessionType ) ;
      virtual UINT32 _maxCacheSize() const ;

      virtual pmdAsyncSession* _createSession( SDB_SESSION_TYPE sessionType,
                                               INT32 startType,
                                               UINT64 sessionID,
                                               void *data = NULL ) ;
      UINT64 _newSessionID()
      {
         ++_innerSessionID ;
         return ossPack32To64( PMD_BASE_HANDLE_ID, _innerSessionID ) ;
      }

      TASK_SESSION_ITEM* _findTask( const seIndexMeta *idxMeta ) ;

   private:
      _seAdptCB            *_pAdptCB ;
      TASK_SESSION_MAP     _taskSessionMap ;
      UINT32               _indexSessionTimer ;
      UINT32               _innerSessionID ;
   } ;
   typedef _seIndexSessionMgr seIndexSessionMgr ;

   /*
    * Key control block of search engine adapter.
    * It takes care of the following things:
    * (1) Register the adapter on SDB data node, both at startup and when a
    *     network break happens.
    * (2) Check for text index udpates.
    * (2) Use the seIndexSessionMgr to manage all the indexer sessions. The
    *     index session manager does not have a seperated thread.
    */
   class _seAdptCB : public _pmdObjBase, public _IControlBlock
   {
      friend class _seIndexSessionMgr ;
      DECLARE_OBJ_MSG_MAP()
      typedef std::vector<_seAdptSessionInfo>    VECINNERPARAM ;

   public:
      _seAdptCB() ;
      virtual ~_seAdptCB() ;

      virtual SDB_CB_TYPE cbType() const ;
      virtual const CHAR* cbName() const ;

      virtual INT32 init() ;
      virtual INT32 active() ;
      virtual INT32 deactive() ;
      virtual INT32 fini() ;

      virtual void attachCB( _pmdEDUCB *cb ) ;
      virtual void detachCB( _pmdEDUCB *cb ) ;

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      seAdptOptionsMgr*    getOptions() ;
      utilESCltMgr*        getSeCltMgr() ;
      seSvcSessionMgr*     getSeAgentMgr() ;
      seIndexSessionMgr*   getIdxSessionMgr() ;
      netRouteAgent*       getIdxRouteAgent() ;
      seIdxMetaMgr*        getIdxMetaCache() { return &_idxMetaCache ; }

      INT32 startInnerSession( SEADPT_SESSION_TYPE type,
                               UINT64 sessionID, void *data = NULL ) ;
      void  cleanInnerSession( INT32 type ) ;
      INT32 sendToDataNode( MsgHeader *msg ) ;
      BOOLEAN isDataNodePrimary() { return _peerPrimary ; }
      void setDataNodePrimary( BOOLEAN isPrimary )
      {
         _peerPrimary = isPrimary ;
      }

      const CHAR *getDataNodeGrpName() { return _peerGroupName ; }

      INT32 syncUpdateCLVersion( const CHAR *collectionName, INT64 millsec,
                                 pmdEDUCB *cb, INT32 &version ) ;
   private:
      INT32 _startSvcListener() ;
      INT32 _initSdbAddr() ;
      INT32 _initSearchEngineAddr() ;
      INT32 _sendRegisterMsg() ;
      INT32 _resumeRegister() ;
      INT32 _startEDU( INT32 type, EDU_STATUS waitStatus,
                       void *args, BOOLEAN regSys ) ;

      INT32 _onRegisterRes( NET_HANDLE handle, MsgHeader *msg ) ;

   private:
      INT32 _startInnerSession( INT32 type, pmdAsycSessionMgr *pSessionMgr ) ;
      INT32 _sendIdxUpdateReq() ;
      INT32 _onIdxUpdateRes( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _onRemoteDisconnect( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _setTimers() ;
      void  _killTimer( UINT32 timerID ) ;
      INT32 _onCatalogResMsg( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _sendCataQueryReq( const BSONObj &query, const BSONObj &selector,
                               UINT64 requestID, _pmdEDUCB *cb ) ;
      INT32 _updateIndexInfo( BSONObj &obj, BOOLEAN &updated,
                              BOOLEAN &upgrade ) ;
      INT32 _parseIndexInfo( const BSONElement *ele, seIndexMeta &idxMeta ) ;

      void _genESIdxName( UINT32 csLID, UINT32 clLID, INT32 idxLID,
                          CHAR *esIdxName, UINT32 buffSize ) ;
      void _genESIdxName( seIndexMeta &idxMeta ) ;

   private:
      indexMsgHandler         _indexMsgHandler ;
      pmdAsyncMsgHandler      _svcMsgHandler ;
      pmdAsyncTimerHandler    _indexTimerHandler ;
      pmdAsyncTimerHandler    _svcTimerHandler ;
      netRouteAgent           _indexNetRtAgent ;  // net route agent for indexer
      netRouteAgent           _svcRtAgent ;
      seIndexSessionMgr       _idxSessionMgr ;
      seSvcSessionMgr         _svcSessionMgr ;
      seAdptOptionsMgr        _options ;
      CHAR                    _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;

      ossEvent                _attachEvent ;
      MsgRouteID              _dataNodeID ;
      MsgRouteID              _cataNodeID ;
      BOOLEAN                 _peerPrimary ;    // If the connected data node is
      CHAR                    _peerGroupName[ OSS_MAX_GROUPNAME_SIZE + 1 ] ;

      utilESCltMgr            _seCltMgr ;
      MsgRouteID              _selfRouteID ;
      ossSpinSLatch           _seLatch ;
      VECINNERPARAM           _vecInnerSessionParam ;
      UINT32                  _regTimerID ;        // For register adapter on data node.
      UINT32                  _idxUpdateTimerID ;  // For text index information update.
      UINT32                  _oneSecTimerID ;     // For session check by session managers.
      INT32                   _clVersion ;
      ossSpinSLatch           _verUpdateLock ;
      ossEvent                _cataEvent ;

      INT64                   _localIdxVer ;
      seIdxMetaMgr            _idxMetaCache ;
      MsgHeader              *_regMsgBuff ;
   } ;
   typedef _seAdptCB seAdptCB ;

   seAdptCB* sdbGetSeAdapterCB() ;
   seAdptOptionsMgr* sdbGetSeAdptOptions() ;
   seSvcSessionMgr* sdbGetSeAgentCB() ;
   utilESCltMgr* sdbGetSeCltMgr() ;
}

#endif /* SE_ADPTMGR_HPP_ */

