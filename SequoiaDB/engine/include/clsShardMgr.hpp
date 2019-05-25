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

   Source File Name = clsShardMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_SHD_MGR_HPP_
#define CLS_SHD_MGR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pmdObjBase.hpp"
#include "netRouteAgent.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"
#include "clsCatalogAgent.hpp"
#include "sdbInterface.hpp"
#include "clsDCMgr.hpp"
#include "utilMap.hpp"
#include "monDMS.hpp"

using namespace bson ;

namespace engine
{
   #define CLS_SHARD_TIMEOUT     (30*OSS_ONE_SEC)

   /*
      _clsEventItem define
   */
   class _clsEventItem : public SDBObject
   {
   public :
      BOOLEAN        send ;
      UINT32         waitNum ;
      ossEvent       event ;
      UINT64         requestID ;
      std::string    name ;
      UINT32         groupID ;
      NET_HANDLE     netHandle ;

      _clsEventItem ()
      {
         send = FALSE ;
         waitNum = 0 ;
         requestID = 0 ;
         groupID = 0 ;
         netHandle = NET_INVALID_HANDLE ;
      }
   } ;
   typedef class _clsEventItem clsEventItem ;

   /*
      _clsCSEventItem define
   */
   class _clsCSEventItem : public SDBObject
   {
      public:
         std::string    csName ;
         ossEvent       event ;
         UINT32         pageSize ;
         UINT32         lobPageSize ;
         DMS_STORAGE_TYPE    type ;
         NET_HANDLE     netHandle ;

         _clsCSEventItem()
         {
            pageSize = 0 ;
            lobPageSize = 0 ;
            type = DMS_STORAGE_NORMAL ;
            netHandle = NET_INVALID_HANDLE ;
         }
   } ;
   typedef _clsCSEventItem clsCSEventItem ;

   /*
      _clsFreezingWindow define
   */
   class _clsFreezingWindow : public SDBObject
   {
      typedef std::set< UINT64 > OP_SET ;
      typedef _utilMap< std::string, OP_SET > MAP_WINDOW ;

      public:
         _clsFreezingWindow() ;
         ~_clsFreezingWindow() ;

         void registerCL ( const CHAR * pName, const CHAR * pMainCLName,
                           UINT64 & opID ) ;
         void unregisterCL ( const CHAR * pName, const CHAR * pMainCLName,
                             UINT64 opID ) ;

         BOOLEAN needBlockOpr( const CHAR *pName, UINT64 testOpID ) ;

         INT32 waitForOpr( const CHAR *pName,
                           _pmdEDUCB *cb,
                           BOOLEAN isWrite ) ;

      private :
         void _registerCLInternal ( const CHAR * pName, UINT64 opID ) ;
         void _unregisterCLInternal ( const CHAR * pName, UINT64 opID ) ;

      private:
         UINT32            _clCount ;
         MAP_WINDOW        _mapWindow ;
         ossSpinXLatch     _latch ;
         ossEvent          _event ;

   } ;
   typedef _clsFreezingWindow clsFreezingWindow ;

   /*
      _clsShardMgr define
   */
   class _clsShardMgr :  public _pmdObjBase
   {
      typedef std::map<std::string, clsEventItem*>       MAP_CAT_EVENT ;
      typedef MAP_CAT_EVENT::iterator                    MAP_CAT_EVENT_IT ;

      typedef std::map<UINT32, clsEventItem*>            MAP_NM_EVENT ;
      typedef MAP_NM_EVENT::iterator                     MAP_NM_EVENT_IT ;

      typedef std::map<UINT64, clsCSEventItem*>          MAP_CS_EVENT ;
      typedef MAP_CS_EVENT::iterator                     MAP_CS_EVENT_IT ;

      typedef std::map<UINT64, _netRouteNode>            MAP_ROUTE_NODE ;
      typedef MAP_ROUTE_NODE::iterator                   MAP_ROUTE_NODE_IT ;

      DECLARE_OBJ_MSG_MAP()

      public:
         _clsShardMgr( _netRouteAgent *rtAgent );
         virtual ~_clsShardMgr();

         INT32    initialize() ;
         INT32    active () ;
         INT32    deactive () ;
         INT32    final() ;
         void     onConfigChange() ;
         void     ntyPrimaryChange( BOOLEAN primary,
                                    SDB_EVENT_OCCUR_TYPE type ) ;

         virtual void   attachCB( _pmdEDUCB *cb ) ;
         virtual void   detachCB( _pmdEDUCB *cb ) ;

         virtual void     onTimer ( UINT32 timerID, UINT32 interval ) ;

         void setCatlogInfo ( const NodeID &id, const std::string& host,
                              const std::string& service ) ;
         void setNodeID ( const MsgRouteID& nodeID ) ;

         catAgent* getCataAgent () ;
         nodeMgrAgent* getNodeMgrAgent () ;
         clsFreezingWindow *getFreezingWindow() ;
         clsDCMgr* getDCMgr() ;

         INT32 getAndLockCataSet( const CHAR *name, clsCatalogSet **ppSet,
                                  BOOLEAN noWithUpdate = TRUE,
                                  INT64 waitMillSec = CLS_SHARD_TIMEOUT,
                                  BOOLEAN *pUpdated = NULL ) ;
         INT32 unlockCataSet( clsCatalogSet *catSet ) ;

         INT32 getAndLockGroupItem( UINT32 id, clsGroupItem **ppItem,
                                     BOOLEAN noWithUpdate = TRUE,
                                     INT64 waitMillSec = CLS_SHARD_TIMEOUT,
                                     BOOLEAN *pUpdated = NULL ) ;
         INT32 unlockGroupItem( clsGroupItem *item ) ;

         INT32 rGetCSInfo( const CHAR *csName, UINT32 &pageSize,
                           UINT32 &lobPageSize,
                           DMS_STORAGE_TYPE &type,
                           INT64 waitMillSec = CLS_SHARD_TIMEOUT ) ;

         INT32 updateDCBaseInfo() ;

      public:
         INT32  sendToCatlog ( MsgHeader * msg,
                               NET_HANDLE *pHandle = NULL,
                               INT64 upCataMillsec = 0,
                               BOOLEAN canUpCataGrp = TRUE ) ;
         INT32  syncSend( MsgHeader * msg, UINT32 groupID, BOOLEAN primary,
                          MsgHeader **ppRecvMsg,
                          INT64 millisec = CLS_SHARD_TIMEOUT ) ;
         INT32  updatePrimary ( const NodeID & id , BOOLEAN primary ) ;
         INT32  updateCatGroup ( INT64 millsec = 0 ) ;
         INT32  updatePrimaryByReply( MsgHeader *pMsg ) ;

         INT32 syncUpdateCatalog ( const CHAR *pCollectionName,
                                   INT64 millsec = CLS_SHARD_TIMEOUT ) ;
         INT32 syncUpdateGroupInfo ( UINT32 groupID,
                                     INT64 millsec = CLS_SHARD_TIMEOUT ) ;
         NodeID nodeID () const ;
         INT32 clearAllData () ;

         INT64 netIn() ;
         INT64 netOut() ;

      protected:

         INT32 _sendCataQueryReq( INT32 queryType, const BSONObj &query,
                                  UINT64 requestID,
                                  NET_HANDLE *pHandle = NULL,
                                  INT64 millsec = 0 ) ;

         INT32 _sendCatalogReq ( const CHAR *pCollectionName,
                                 UINT64 requestID = 0,
                                 NET_HANDLE *pHandle = NULL,
                                 INT64 millsec = 0 ) ;

         INT32 _sendGroupReq ( UINT32 groupID, UINT64 requestID = 0,
                               NET_HANDLE *pHandle = NULL,
                               INT64 millsec = 0 ) ;

         INT32 _sendCSInfoReq ( const CHAR *pCSName, UINT64 requestID = 0,
                                NET_HANDLE *pHandle = NULL,
                                INT64 millsec = 0 ) ;

         clsEventItem *_findCatSyncEvent ( const CHAR *pCollectionName,
                                           BOOLEAN bCreate = FALSE ) ;
         clsEventItem *_findCatSyncEvent ( UINT64 requestID ) ;

         clsEventItem *_findNMSyncEvent ( UINT32 groupID,
                                          BOOLEAN bCreate = FALSE ) ;
         clsEventItem *_findNMSyncEvent ( UINT64 requestID ) ;

         INT32 _findCatNodeID ( MAP_ROUTE_NODE &catNodes,
                                const CHAR *hostName,
                                const std::string &service,
                                NodeID &id ) ;
         INT32 _sendToSeAdpt( NET_HANDLE handle, MsgHeader *msg ) ;

      protected:
         INT32 _onCatCatGroupRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onCatalogReqMsg ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onCatGroupRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onQueryCSInfoRsp( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onHandleClose( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onAuthReqMsg( NET_HANDLE handle, MsgHeader * msg ) ;
         INT32 _onTextIdxInfoReqMsg( NET_HANDLE handle, MsgHeader * msg ) ;
         INT32 _buildTextIdxObj( const monCSSimple *csInfo,
                                 const monCLSimple *clInfo,
                                 const monIndex *idxInfo,
                                 BSONObjBuilder &builder ) ;
         INT32 _dumpTextIdxInfo( INT64 localVersion, BSONObj &obj,
                                 BOOLEAN onlyVersion = FALSE ) ;

      private:
         _netRouteAgent                *_pNetRtAgent ;
         _clsCatalogAgent              *_pCatAgent ;
         _clsNodeMgrAgent              *_pNodeMgrAgent ;
         clsFreezingWindow             *_pFreezingWindow ;
         clsDCMgr                      *_pDCMgr ;
         ossSpinXLatch                 _catLatch ;
         MAP_CAT_EVENT                 _mapSyncCatEvent ;
         MAP_NM_EVENT                  _mapSyncNMEvent ;
         MAP_CS_EVENT                  _mapSyncCSEvent ;
         UINT64                        _requestID ;

         clsGroupItem                  _cataGrpItem ;
         MAP_ROUTE_NODE                _mapNodes ;

         UINT32                        _catVerion ;
         ossEvent                      _upCatEvent ;
         NET_HANDLE                    _upCatHandle ;
         ossSpinSLatch                 _shardLatch ;

         MsgRouteID                    _nodeID ;
         MsgRouteID                    _seAdptID ;
   } ;

   typedef _clsShardMgr shardCB ;
}

#endif //CLS_SHD_MGR_HPP_

