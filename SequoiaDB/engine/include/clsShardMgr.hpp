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
#include "clsGTSAgent.hpp"
#include "sdbInterface.hpp"
#include "clsDCMgr.hpp"
#include "monDMS.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{
   // the shard mgr update some info from catalog timeout
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
      utilCLUniqueID clUniqueID ;

      _clsEventItem ()
      {
         send = FALSE ;
         waitNum = 0 ;
         requestID = 0 ;
         groupID = 0 ;
         netHandle = NET_INVALID_HANDLE ;
         clUniqueID = UTIL_UNIQUEID_NULL ;
      }
   } ;
   typedef class _clsEventItem clsEventItem ;

   /*
      _clsCSEventItem define
   */
   class _clsCSEventItem : public SDBObject
   {
      public:
         std::string       csName ;
         ossEvent          event ;
         UINT32            pageSize ;
         UINT32            lobPageSize ;
         DMS_STORAGE_TYPE  type ;
         NET_HANDLE        netHandle ;
         utilCSUniqueID    csUniqueID ;
         BSONObj           clInfo ;

         _clsCSEventItem()
         {
            pageSize = 0 ;
            lobPageSize = 0 ;
            type = DMS_STORAGE_NORMAL ;
            netHandle = NET_INVALID_HANDLE ;
            csUniqueID = UTIL_UNIQUEID_NULL ;
         }
   } ;
   typedef _clsCSEventItem clsCSEventItem ;

   /*
      _clsFreezingWindow define
   */
   class _clsFreezingWindow : public SDBObject
   {
      typedef ossPoolSet< UINT64 >                 OP_SET ;
      typedef ossPoolMap< ossPoolString, OP_SET >  MAP_WINDOW ;

      typedef struct _csKeyName
      {
         ossPoolString        _name ;

         _csKeyName( const ossPoolString &name )
         {
            _name = name ;
         }

         _csKeyName()
         {
         }

         bool operator< ( const _csKeyName &rhs ) const
         {
            UINT32 llen = _name.length() ;
            UINT32 rlen = rhs._name.length() ;
            UINT32 len = llen <= rlen ? llen : rlen ;

            INT32 cmp = ossStrncmp( _name.c_str(), rhs._name.c_str(), len ) ;
            if ( 0 == cmp && llen == len && rlen != len &&
                 '.' != rhs._name[len] )
            {
               cmp = -1 ;
            }

            return cmp < 0 ? true : false ;
         }

         bool operator== ( const _csKeyName &rhs ) const
         {
            UINT32 llen = _name.length() ;
            UINT32 rlen = rhs._name.length() ;
            UINT32 len = llen <= rlen ? llen : rlen ;

            INT32 cmp = ossStrncmp( _name.c_str(), rhs._name.c_str(), len ) ;
            if ( 0 == cmp && ( llen == len || '.' == _name[len] ) &&
                 ( rlen == len || '.' == rhs._name[len] ) )
            {
               return true ;
            }
            return false ;
         }

      } csKeyName ;

      typedef ossPoolMap< csKeyName, OP_SET >   MAP_CS_WINDOW ;

      public:
         _clsFreezingWindow() ;
         ~_clsFreezingWindow() ;

         INT32 registerCL ( const CHAR *pName, UINT64 &opID ) ;
         void  unregisterCL ( const CHAR *pName, UINT64 opID ) ;

         INT32 registerCS ( const CHAR *pName, UINT64 &opID ) ;
         void  unregisterCS ( const CHAR *pName, UINT64 opID ) ;

         INT32 registerWhole( UINT64 &opID ) ;
         void  unregisterWhole( UINT64 opID ) ;

         void  unregisterAll() ;

         INT32 waitForOpr( const CHAR *pName,
                           _pmdEDUCB *cb,
                           BOOLEAN isWrite ) ;

         BOOLEAN needBlockOpr( const ossPoolString &name,
                               UINT64 testOpID,
                               UINT64 testTransOpID,
                               _pmdEDUCB *cb ) ;

      private :
         INT32 _registerCLInternal ( const ossPoolString &name, UINT64 opID ) ;
         void  _unregisterCLInternal ( const ossPoolString &name, UINT64 opID ) ;
         INT32 _registerCSInternal( const ossPoolString &name, UINT64 opID ) ;
         void  _unregisterCSInternal( const ossPoolString &name, UINT64 opID ) ;
         INT32 _regWholeInternal( UINT64 opID ) ;
         void  _unregWholeInternal( UINT64 opID ) ;

         void  _blockCheck( const CHAR *pName,
                            const OP_SET &setID,
                            UINT64 testOPID,
                            UINT64 testTransOPID,
                            _pmdEDUCB *cb,
                            BOOLEAN &result,
                            BOOLEAN &forceEnd ) ;

         void  _unregisterCLByIter ( const CHAR * pName, UINT64 opID ) ;
         void  _unregisterCSByIter ( const CHAR * pName, UINT64 opID ) ;

      private:
         volatile UINT32   _clCount ;
         MAP_WINDOW        _mapWindow ;
         MAP_CS_WINDOW     _mapCSWindow ;
         OP_SET            _setWholeID ;
         ossSpinXLatch     _latch ;
         ossEvent          _event ;

   } ;
   typedef _clsFreezingWindow clsFreezingWindow ;

   /*
      _clsShardMgr define
   */
   class _clsShardMgr :  public _pmdObjBase
   {
      typedef ossPoolMap<std::string, clsEventItem*>     MAP_CAT_EVENT ;
      typedef MAP_CAT_EVENT::iterator                    MAP_CAT_EVENT_IT ;

      typedef ossPoolMap<utilCLUniqueID, clsEventItem*>  MAP_CLID_EVENT ;
      typedef MAP_CLID_EVENT::iterator                   MAP_CLID_EVENT_IT ;

      typedef ossPoolMap<UINT32, clsEventItem*>          MAP_NM_EVENT ;
      typedef MAP_NM_EVENT::iterator                     MAP_NM_EVENT_IT ;

      typedef ossPoolMap<UINT64, clsCSEventItem*>        MAP_CS_EVENT ;
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
         clsGTSAgent* getGTSAgent() ;

         INT32 getAndLockCataSet( const CHAR *name,
                                  clsCatalogSet **ppSet,
                                  BOOLEAN noWithUpdate = TRUE,
                                  INT64 waitMillSec = CLS_SHARD_TIMEOUT,
                                  BOOLEAN *pUpdated = NULL ) ;
         INT32 unlockCataSet( clsCatalogSet *catSet ) ;

         INT32 getAndLockGroupItem( UINT32 id, clsGroupItem **ppItem,
                                    BOOLEAN noWithUpdate = TRUE,
                                    INT64 waitMillSec = CLS_SHARD_TIMEOUT,
                                    BOOLEAN *pUpdated = NULL ) ;
         INT32 unlockGroupItem( clsGroupItem *item ) ;

         INT32 rGetCSInfo( const CHAR *csName,
                           utilCSUniqueID &csUniqueID,
                           UINT32 *pageSize = NULL,
                           UINT32 *lobPageSize = NULL,
                           DMS_STORAGE_TYPE *type = NULL,
                           BSONObj *clInfo = NULL,
                           string *newCSName = NULL,
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
         INT32  updatePrimaryByReply( MsgHeader *pMsg,
                                      UINT32 groupID = CATALOG_GROUPID ) ;

         INT32 syncUpdateCatalog ( const CHAR *pCollectionName,
                                   INT64 millsec = CLS_SHARD_TIMEOUT ) ;
         INT32 syncUpdateCatalog ( utilCLUniqueID clUniqueID,
                                   const CHAR *pCollectionName,
                                   INT64 millsec = CLS_SHARD_TIMEOUT ) ;

         INT32 syncUpdateGroupInfo ( UINT32 groupID,
                                     INT64 millsec = CLS_SHARD_TIMEOUT ) ;
         NodeID nodeID () const ;
         INT32 clearAllData () ;

         INT64 netIn() ;
         INT64 netOut() ;

         INT32 replyToRemoteEndpoint ( NET_HANDLE handle,
                                       MsgHeader * request,
                                       const BSONObj & replyObject ) ;

      protected:

         INT32 _sendCataQueryReq( INT32 queryType, const BSONObj &query,
                                  UINT64 requestID,
                                  NET_HANDLE *pHandle = NULL,
                                  INT64 millsec = 0 ) ;

         INT32 _sendCatalogReq ( const CHAR *pCollectionName,
                                 utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                                 UINT64 requestID = 0,
                                 NET_HANDLE *pHandle = NULL,
                                 INT64 millsec = 0
                               ) ;

         INT32 _sendGroupReq ( UINT32 groupID, UINT64 requestID = 0,
                               NET_HANDLE *pHandle = NULL,
                               INT64 millsec = 0 ) ;

         INT32 _sendCSInfoReq ( const CHAR *pCSName,
                                utilCSUniqueID csUniqueID,
                                UINT64 requestID = 0,
                                NET_HANDLE *pHandle = NULL,
                                INT64 millsec = 0 ) ;

         clsEventItem *_findCatSyncEvent ( const CHAR *pCollectionName,
                                           utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                                           BOOLEAN bCreate = FALSE ) ;
         clsEventItem *_findCatSyncEvent ( UINT64 requestID ) ;

         clsEventItem *_findNMSyncEvent ( UINT32 groupID,
                                          BOOLEAN bCreate = FALSE ) ;
         clsEventItem *_findNMSyncEvent ( UINT64 requestID ) ;

         INT32 _findCatNodeID ( MAP_ROUTE_NODE &catNodes,
                                const CHAR *hostName,
                                const std::string &service,
                                NodeID &id ) ;
         INT32 _sendToRemoteEndpoint( NET_HANDLE handle, MsgHeader *msg ) ;

      //msg functions
      protected:
         INT32 _onCatCatGroupRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onCatalogReqMsg ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onCatGroupRes ( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onQueryCSInfoRsp( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onHandleClose( NET_HANDLE handle, MsgHeader* msg ) ;
         INT32 _onAuthReqMsg( NET_HANDLE handle, MsgHeader * msg ) ;
         INT32 _onTextIdxInfoReqMsg( NET_HANDLE handle, MsgHeader * msg ) ;
         INT32 _onTransCheckReqMsg( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _updateRemoteEndpointInfo( NET_HANDLE handle,
                                          const BSONObj &regInfo ) ;
         INT32 _genAuthReplyInfo( BSONObj &replyInfo ) ;
         BSONObj _buildCataGroupInfo() ;

      private:
         _netRouteAgent                *_pNetRtAgent ;
         _clsCatalogAgent              *_pCatAgent ;
         _clsNodeMgrAgent              *_pNodeMgrAgent ;
         clsFreezingWindow             *_pFreezingWindow ;
         clsDCMgr                      *_pDCMgr ;
         clsGTSAgent                   *_pGTSAgent ;
         ossSpinXLatch                 _catLatch ;
         MAP_CAT_EVENT                 _mapSyncCatEvent ;
         MAP_CLID_EVENT                _mapSyncCLIDEvent ;
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

         // Currently remote endpoint refers to the search engine adapter.
         // It works as a client for indexing and server for searching.
         NET_HANDLE                    _remoteEndpointHandle ;
   } ;

   typedef _clsShardMgr shardCB ;
}

#endif //CLS_SHD_MGR_HPP_

