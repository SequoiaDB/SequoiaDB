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

   Source File Name = catalogueCB.hpp

   Descriptive Name = Process MoDel Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure kernel control block,
   which is the most critical data structure in the engine process.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CATALOGUECB_HPP_
#define CATALOGUECB_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMemPool.hpp"
#include "pmdDef.hpp"
#include "msg.hpp"
#include "netRouteAgent.hpp"
#include "msgCatalog.hpp"
#include "catMainController.hpp"
#include "catCatalogManager.hpp"
#include "catNodeManager.hpp"
#include "catGTSManager.hpp"
#include "catDCManager.hpp"
#include "sdbInterface.hpp"
#include "catLevelLock.hpp"
#include "catRecycleBinManager.hpp"

using namespace bson ;

namespace engine
{

   /*
      sdbCatalogueCB define
   */
   class sdbCatalogueCB : public _IControlBlock, public _IEventHander
   {
   public:
      friend class catMainController ;

      typedef std::map<UINT32, string>    GRP_ID_MAP;
      typedef std::map<UINT16, UINT16>    NODE_ID_MAP;
      typedef std::vector<_catEventHandler *> VEC_EVENT_HANDLER ;

      struct catLocationInfo
      {
      public:
         catLocationInfo( UINT32 locationID, UINT32 count )
         {
            _locationID = locationID ;
            _count = count ;
         } ;

         UINT32 _locationID ;
         UINT32 _count ;
      } ;

      typedef ossPoolMap<ossPoolString, catLocationInfo>  CAT_LOC2ID_MAP ;
      typedef ossPoolMap<UINT32, ossPoolString> CAT_ID2LOC_MAP ;

      public:
         sdbCatalogueCB() ;
         virtual ~sdbCatalogueCB() ;

         virtual SDB_CB_TYPE cbType() const { return SDB_CB_CATALOGUE ; }
         virtual const CHAR* cbName() const { return "CATALOGUECB" ; }

         virtual INT32  init () ;
         virtual INT32  active () ;
         virtual INT32  deactive () ;
         virtual INT32  fini () ;
         virtual void   onConfigChange() ;
         virtual void   onConfigSave() ;
         void           loadNodeInfo () ;

         virtual void   onRegistered( const MsgRouteID &nodeID ) ;
         virtual void   onPrimaryChange( BOOLEAN primary,
                                         SDB_EVENT_OCCUR_TYPE occurType ) ;

         void     insertGroupID( UINT32 grpID, const string &name,
                                 BOOLEAN isActive = TRUE ) ;
         void     removeGroupID( UINT32 grpID ) ;
         void     insertNodeID( UINT16 nodeID ) ;
         void     activeGroup( UINT32 groupID ) ;
         void     deactiveGroup( UINT32 groupID ) ;
         UINT32   allocGroupID() ;
         INT32    getAGroupRand( UINT32 &groupID) ;
         UINT16   allocNodeID() ;
         void     releaseNodeID( UINT16 nodeID ) ;
         UINT16   allocSystemNodeID() ;
         BOOLEAN  checkGroupActived( const CHAR *gpName, BOOLEAN &gpExist  ) ;
         BOOLEAN  checkGroupActived( UINT32 groupID, BOOLEAN &gpExist ) ;

         INT32    insertLocID( const ossPoolString &locName,
                                      UINT32 locID,
                                      UINT16 refCount = 1 ) ;
         void     releaseLocID( const ossPoolString &locName ) ;
         UINT32   allocLocID( const ossPoolString &locName ) ;

         void     releaseNode( UINT16 nodeID, const ossPoolString &locName ) ;

         BOOLEAN getNodeInfoChanged()
         {
            return _nodeInfoChanged ;
         }

         void setNodeInfoChanged( BOOLEAN nodeInfoChanged )
         {
            _nodeInfoChanged = nodeInfoChanged ;
         }

         void        clearInfo() ;
         GRP_ID_MAP* getGroupMap( BOOLEAN isActive = TRUE ) ;
         const CHAR* groupID2Name( UINT32 groupID ) ;
         UINT32      groupName2ID( const string &groupName ) ;
         INT32       getGroupsName( vector< string > &vecNames ) ;
         INT32       getGroupsID( vector< UINT32 > &vecIDs, BOOLEAN isActiveOnly ) ;
         INT32       getGroupNameMap ( map<std::string, UINT32> & nameMap,
                                       BOOLEAN isActiveOnly ) ;

         INT32       makeGroupsObj( BSONObjBuilder &builder,
                                    vector< string > &groups,
                                    BOOLEAN ignoreErr = FALSE,
                                    BOOLEAN ignoreNonExist = FALSE ) ;
         INT32       makeGroupsObj( BSONObjBuilder &builder,
                                    vector< UINT32 > &groups,
                                    BOOLEAN ignoreErr = FALSE,
                                    BOOLEAN ignoreNonExist = FALSE ) ;
         INT32       makeGroupsObj( BSONObjBuilder &builder,
                                    const CAT_GROUP_SET &groups,
                                    BOOLEAN ignoreErr = FALSE,
                                    BOOLEAN ignoreNonExist = FALSE ) ;
         INT32       makeFailedGroupsObj( BSONObjBuilder &builder,
                                          const CAT_GROUP_LIST &groups,
                                          BOOLEAN ignoreErr = FALSE,
                                          BOOLEAN ignoreNonExist = FALSE ) ;

         INT16    majoritySize( BOOLEAN needWaitSync = FALSE ) ;
         INT32    primaryCheck( _pmdEDUCB *cb, BOOLEAN canDelay,
                                BOOLEAN &isDelay ) ;
         UINT16   getPrimaryNode() const { return _primaryID.columns.nodeID ; }

         BOOLEAN  isDCActivated() const { return _catDCMgr.isDCActivated() ; }
         BOOLEAN  isImageEnabled() const { return _catDCMgr.isImageEnabled() ; }
         BOOLEAN  isDCReadonly() const { return _catDCMgr.isDCReadonly() ; }
         BOOLEAN  isActived() const { return _isActived ; }

         UINT32   setTimer( UINT32 milliSec ) ;
         void     killTimer( UINT32 timerID ) ;

         BOOLEAN  delayCurOperation() ;
         BOOLEAN  isDelayed() const { return _catMainCtrl.isDelayed() ; }
         void     addContext( const UINT32 &handle, UINT32 tid, INT64 contextID ) ;

         _netRouteAgent* netWork()
         {
            return _pNetWork;
         }
         catMainController* getMainController()
         {
            return &_catMainCtrl ;
         }
         catGTSManager* getCatGTSMgr()
         {
            return &_catGTSMgr ;
         }
         catCatalogueManager* getCatlogueMgr()
         {
            return &_catlogueMgr ;
         }
         catNodeManager* getCatNodeMgr()
         {
            return &_catNodeMgr ;
         }
         catDCManager* getCatDCMgr()
         {
            return &_catDCMgr ;
         }
         catLevelLockMgr* getLevelLockMgr()
         {
            return &_levelLockMgr ;
         }

         catRecycleBinManager *getRecycleBinMgr()
         {
            return &_recycleBinMgr ;
         }

         void regEventHandler ( _catEventHandler *pHandler ) ;
         void unregEventHandler ( _catEventHandler *pHandler ) ;

         INT32 onBeginCommand ( MsgHeader *pReqMsg ) ;
         INT32 onEndCommand ( MsgHeader *pReqMsg, INT32 result ) ;
         INT32 onSendReply ( MsgOpReply *pReply, INT32 result ) ;
         INT32 checkUpgrade() ;

         INT32 sendReply ( const NET_HANDLE &handle,
                           MsgOpReply *pReply,
                           INT32 result,
                           void *pReplyData = NULL,
                           UINT32 replyDataLen = 0,
                           BOOLEAN needSync = TRUE ) ;

         void     incPacketLevel() { ++_inPacketLevel ; }
         void     decPacketLevel() { --_inPacketLevel ; }
         UINT32   getPacketLevel() const { return _inPacketLevel ; }

         void fillErrReply ( const MsgOpReply *pReply, MsgOpReply *pErrReply,
                             INT32 rc ) ;

         void setNeedForceSecondary( BOOLEAN needForce )
         {
            _needForceSecondary = needForce ;
         }

      protected:
         INT32 _onUpgrade( UINT32 beginVersion ) ;
         INT32 _onDowngrade( UINT32 beginVersion ) ;

      private:
         _netRouteAgent       *_pNetWork ;
         _MsgRouteID          _routeID ;
         std::string          _strHostName ;
         std::string          _strCatServiceName ;
         NODE_ID_MAP          _nodeIdMap ;
         NODE_ID_MAP          _sysNodeIdMap ;
         GRP_ID_MAP           _grpIdMap ;
         GRP_ID_MAP           _deactiveGrpIdMap ;
         CAT_LOC2ID_MAP       _locIdMap ;
         CAT_ID2LOC_MAP       _idLocMap ;
         UINT16               _iCurNodeId ;
         UINT16               _curSysNodeId ;
         UINT32               _iCurGrpId ;
         UINT32               _nextLocId ;

         catMainController    _catMainCtrl ;
         catCatalogueManager  _catlogueMgr ;
         catNodeManager       _catNodeMgr ;
         catGTSManager        _catGTSMgr ;
         catDCManager         _catDCMgr ;
         catLevelLockMgr      _levelLockMgr ;
         catRecycleBinManager _recycleBinMgr ;

         MsgRouteID           _primaryID ;
         BOOLEAN              _isActived ;
         BOOLEAN              _needForceSecondary ;

         // Use to record whether the node info has changed
         BOOLEAN              _nodeInfoChanged ;

         VEC_EVENT_HANDLER    _vecEventHandler ;

         UINT32               _inPacketLevel ;
   } ;

   /*
      get global catalogue cb
   */
   sdbCatalogueCB* sdbGetCatalogueCB() ;

}

#endif // CATALOGUECB_HPP_

