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

   Source File Name = coordRemoteSession.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_REMOTE_SESSION_HPP__
#define COORD_REMOTE_SESSION_HPP__

#include "coordResource.hpp"
#include "pmdRemoteSession.hpp"
#include "coordRemoteHandle.hpp"
#include "rtnSessionProperty.hpp"
#include "ossMemPool.hpp"
#include "msg.hpp"

using namespace bson ;

namespace engine
{

   class _coordSessionPropMgr ;
   class _coordGroupSel ;
   class _coordGroupSessionCtrl ;

   /*
      _IGroupSessionHandler define
   */
   class _IGroupSessionHandler
   {
      public:
         _IGroupSessionHandler() {}
         virtual ~_IGroupSessionHandler() {}

      public:
         virtual void   prepareForSend( UINT32 groupID,
                                        pmdSubSession *pSub,
                                        _coordGroupSel *pSel,
                                        _coordGroupSessionCtrl *pCtrl ) = 0 ;
   } ;
   typedef _IGroupSessionHandler IGroupSessionHandler ;

   /*
      _coordTransNodeStatus define
   */
   struct _coordTransNodeStatus
   {
      MsgRouteID        _nodeID ;
      BOOLEAN           _hasWritten ;

      _coordTransNodeStatus()
      {
         _nodeID.value = MSG_INVALID_ROUTEID ;
         _hasWritten = FALSE ;
      }

      _coordTransNodeStatus( const MsgRouteID &id,
                             BOOLEAN isWrite = FALSE )
      {
         _nodeID.value = id.value ;
         _hasWritten = isWrite ;
      }
   } ;
   typedef _coordTransNodeStatus coordTransNodeStatus ;

   /*
      _coordLastNodeStatus define
    */
   typedef struct _coordLastNodeStatus
   {
      _coordLastNodeStatus()
      {
         _nodeID.value = MSG_INVALID_ROUTEID ;
         _addTick = 0LL ;
      }

      // node ID of last selected node
      MsgRouteID  _nodeID ;
      // tick to add the last selected node
      UINT64      _addTick ;
   } coordLastNodeStatus ;

   /*
      _coordSessionPropSite define
   */
   class _coordSessionPropSite : public _rtnSessionProperty
   {
      public:
         typedef ossPoolMap< UINT32, coordLastNodeStatus > MAP_GROUP_2_NODE ;
         typedef MAP_GROUP_2_NODE::iterator        MAP_GROUP_2_NODE_IT ;

         typedef ossPoolMap< UINT32, coordTransNodeStatus > MAP_TRANS_NODES ;
         typedef MAP_TRANS_NODES::iterator                  MAP_TRANS_NODES_IT ;
         typedef MAP_TRANS_NODES::const_iterator            MAP_TRANS_NODES_CIT ;

         friend class _coordSessionPropMgr ;

      public:
         _coordSessionPropSite() ;
         virtual ~_coordSessionPropSite() ;

         void        clear() ;

         INT32       addLastNode( const MsgRouteID &nodeID,
                                  BOOLEAN primaryRequest ) ;
         UINT64      getLastNode( UINT32 groupID ) ;
         BOOLEAN     existNode( UINT32 groupID ) const ;
         void        delLastNode( UINT32 groupID ) ;
         void        delLastNode( const MsgRouteID &nodeID ) ;

         /*
            Trans info
         */
         BOOLEAN     isTransNode( const MsgRouteID &routeID ) const ;
         BOOLEAN     getTransNodeRouteID( UINT32 groupID,
                                          MsgRouteID &routeID ) const ;
         BOOLEAN     hasTransNode( UINT32 groupID ) const ;
         BOOLEAN     isTransNodeEmpty() const ;

         BOOLEAN     checkAndUpdateNode( const MsgRouteID &routeID,
                                         BOOLEAN doWrite ) ;

         BOOLEAN     delTransNode( const MsgRouteID &routeID ) ;
         void        addTransNode( const MsgRouteID &routeID,
                                   BOOLEAN isWrite = FALSE ) ;

         UINT32      getTransNodeSize() const ;
         UINT32      getWriteTransNodeSize() const ;
         UINT32      dumpTransNode( SET_ROUTEID &setID ) const ;

         const MAP_TRANS_NODES* getTransNodeMap() const ;

         INT32       beginTrans( _pmdEDUCB *cb, BOOLEAN isAutoCommit = FALSE ) ;
         void        endTrans( _pmdEDUCB *cb ) ;

         _pmdEDUCB*  getEDUCB() { return _pEDUCB ; }
         pmdRemoteSessionSite* getSite() { return _pSite ; }

      protected :
         virtual void _onSetInstance () ;
         virtual void _toBson( BSONObjBuilder &builder ) const ;

         virtual INT32  _checkTransConf( const _dpsTransConfItem *pTransConf ) ;
         virtual void   _updateTransConf( const _dpsTransConfItem *pTransConf ) ;
         virtual void   _updateSource( const CHAR *pSource ) ;

      private:
         void        setEduCB( _pmdEDUCB *cb ) ;
         void        setSite( pmdRemoteSessionSite *pSite ) ;

      private:
         MAP_GROUP_2_NODE     _mapLastNodes ;
         _pmdEDUCB            *_pEDUCB ;
         pmdRemoteSessionSite *_pSite ;

         MAP_TRANS_NODES      _mapTransNodes ;
         UINT32               _writeTransNodeNum ;
   } ;

   typedef _coordSessionPropSite coordSessionPropSite ;

   /*
      _coordSessionPropMgr define
   */
   class _coordSessionPropMgr : public _IRemoteMgrHandle,
                                public _rtnSessionProperty
   {
      typedef ossPoolMap< UINT32, coordSessionPropSite >    MAP_TID_2_PROP ;
      typedef MAP_TID_2_PROP::iterator                      MAP_TID_2_PROP_IT ;

      public:
         _coordSessionPropMgr() ;
         _coordSessionPropMgr( BOOLEAN userOwnQueen ) ;
         ~_coordSessionPropMgr() ;

         coordSessionPropSite * getSite( _pmdEDUCB *cb ) ;

      protected:
         virtual void   onRegister( _pmdRemoteSessionSite *pSite,
                                    _pmdEDUCB *cb ) ;
         virtual void   onUnreg( _pmdRemoteSessionSite *pSite,
                                 _pmdEDUCB *cb ) ;

      private:
         MAP_TID_2_PROP _mapProps ;
         BOOLEAN _useOwnQueue ;
   } ;

   typedef _coordSessionPropMgr coordSessionPropMgr ;

   /*
      _coordGroupSel define
   */
   class _coordGroupSel : public SDBObject
   {
      public:
         _coordGroupSel() ;
         ~_coordGroupSel() ;

         void     init( coordResource *pResource,
                        coordSessionPropSite *pPropSite,
                        BOOLEAN primary = FALSE,
                        MSG_ROUTE_SERVICE_TYPE svcType =
                        MSG_ROUTE_SHARD_SERVCIE ) ;

         void     setPrimary( BOOLEAN primary ) ;
         void     setServiceType( MSG_ROUTE_SERVICE_TYPE svcType ) ;

         BOOLEAN  isPrimary() const ;
         BOOLEAN  isRequiredPrimary() const ;
         BOOLEAN  isRequiredSecondary() const ;
         BOOLEAN  isPreferredPrimary() const ;
         BOOLEAN  isPreferredSecondary() const ;
         BOOLEAN  existLastNode( UINT32 groupID ) const ;

         INT32    selBegin( UINT32 groupID, MsgRouteID &nodeID ) ;
         INT32    selNext( MsgRouteID &nodeID ) ;
         void     selDone() ;

         void     updateStat( const MsgRouteID &nodeID, INT32 rc ) ;

         BOOLEAN  getGroupPtrFromMap( UINT32 groupID,
                                      CoordGroupInfoPtr &groupPtr ) ;

         void     addGroupPtr2Map( CoordGroupInfoPtr &groupPtr ) ;

      protected:
         INT32    _selPrimaryBegin( MsgRouteID &nodeID ) ;
         INT32    _selOtherBegin( MsgRouteID &nodeID ) ;

      protected :
         typedef _utilArray< UINT8, CLS_REPLSET_MAX_NODE_SIZE >   COORD_POS_ARRAY ;
         typedef ossPoolList< UINT8>                              COORD_POS_LIST ;
      private:
         INT32    _calcBeginPos( clsGroupItem *pGroupItem,
                                 const rtnInstanceOption & instanceOption,
                                 UINT32 random,
                                 INT32 &pos ) ;
         INT32    _nextPos( CoordGroupInfoPtr &groupPtr,
                            const rtnInstanceOption & instanceOption,
                            UINT32 &selTimes,
                            INT32 &pos,
                            MsgRouteID &nodeID ) ;
         void     _resetStatus() ;
         INT32    _selectPositions ( const VEC_NODE_INFO & groupNodes,
                                     UINT32 primaryPos,
                                     const rtnInstanceOption & instanceOption,
                                     COORD_POS_LIST & selectedPositions ) ;
         INT32    _selectSlavePreferred ( const VEC_NODE_INFO & groupNodes,
                                          UINT32 primaryPos,
                                          COORD_POS_LIST & selectedPositions ) ;
         INT32    _savePositions ( COORD_POS_ARRAY & positionArray,
                                   COORD_POS_LIST & positionList ) ;
         INT32    _shufflePositions ( COORD_POS_ARRAY & positionArray,
                                      COORD_POS_LIST & positionList ) ;
         BOOLEAN  _meetPreferConstraint( const MsgRouteID &nodeID ) const ;

      private:
         coordResource           *_pResource ;
         coordSessionPropSite    *_pPropSite ;
         BOOLEAN                 _primary ;
         MSG_ROUTE_SERVICE_TYPE  _svcType ;

         CoordGroupInfoPtr       _groupPtr ;
         BOOLEAN                 _hasUpdate ;
         COORD_POS_LIST          _selectedPositions ;
         INT32                   _pos ;
         UINT32                  _selTimes ;
         UINT32                  _ignoredNum ;
         MsgRouteID              _lastNodeID ;

         CoordGroupMap           _mapGroupPtr ;
   } ;
   typedef _coordGroupSel coordGroupSel ;

   /*
      _coordCataSel define
   */
   class _coordCataSel : public SDBObject
   {
      public:
         _coordCataSel() ;
         ~_coordCataSel() ;

         INT32    bind( coordResource *pResource,
                        const CHAR *pCollectionName,
                        _pmdEDUCB *cb,
                        BOOLEAN forceUpdate = FALSE,
                        BOOLEAN isRoot = FALSE ) ;

         INT32    bind( coordResource *pResource,
                        const CoordCataInfoPtr &cataPtr,
                        BOOLEAN hasUpdated = FALSE ) ;

         void     clear() ;
         void     setUpdated( BOOLEAN updated ) ;
         BOOLEAN  hasUpdated() const ;

         CoordCataInfoPtr&  getCataPtr() ;

         INT32    getGroupLst( _pmdEDUCB *cb,
                               const CoordGroupList &exceptGrpLst,
                               CoordGroupList &groupLst,
                               const BSONObj *pQuery = NULL ) ;

         INT32    getLobGroupLst( _pmdEDUCB *cb,
                                  const CoordGroupList &exceptGrpLst,
                                  CoordGroupList &groupLst,
                                  const BSONObj *pQuery = NULL ) ;

         /*
            This function only used when the collection is table-partitioned,
            and valid when called after getGroupLst
         */
         CoordGroupSubCLMap& getGroup2SubsMap() ;

         INT32    updateCataInfo( const CHAR *pCollectionName,
                                  _pmdEDUCB *cb,
                                  BOOLEAN isRoot = FALSE ) ;

      private:
         coordResource        *_pResource ;
         BOOLEAN              _hasUpdate ;
         CoordCataInfoPtr     _cataPtr ;
         CoordGroupSubCLMap   _mapGrp2subs ;

   } ;
   typedef _coordCataSel coordCataSel ;

   #define COORD_OPR_MAX_RETRY_TIMES_DFT        ( 3 )
   #define COORD_OPR_MAX_RETRY_TIMES            ( 7 )

   /*
      _coordGroupSessionCtrl define
   */
   class _coordGroupSessionCtrl : public SDBObject
   {
      public:
         _coordGroupSessionCtrl() ;
         ~_coordGroupSessionCtrl() ;

         void        init( coordResource *pResource,
                           coordSessionPropSite *pPropSite,
                           coordGroupSel *pGroupSel,
                           IRemoteSessionHandler *pRemoteHandle ) ;

         BOOLEAN     canRetry( INT32 flag,
                               const MsgRouteID &nodeID,
                               UINT32 newPrimaryID,
                               BOOLEAN isReadCmd,
                               BOOLEAN canUpdate = TRUE ) ;

         BOOLEAN     canRetry( INT32 flag,
                               coordCataSel &cataSel,
                               BOOLEAN canUpdate = TRUE ) ;

         void        incRetry() ;
         void        resetRetry() ;
         UINT32      getRetryTimes() const ;

         void        setMaxRetryTimes( UINT32 maxRetryTimes ) ;

      private:
         BOOLEAN     _canRetry() const ;

      private:
         UINT32                  _retryTimes ;
         UINT32                  _maxRetryTimes ;
         coordResource           *_pResource ;
         coordSessionPropSite    *_pPropSite ;
         coordGroupSel           *_pGroupSel ;
         IRemoteSessionHandler   *_pRemoteHandle ;

   } ;
   typedef _coordGroupSessionCtrl coordGroupSessionCtrl ;

   /*
      _coordGroupSession define
   */
   class _coordGroupSession : public SDBObject
   {
      public:
         _coordGroupSession() ;
         ~_coordGroupSession() ;

         INT64             getTimeout() const ;

         /*
            timeout : 0 , will use the session's timeout attribute
                      otherwise, use the specified timeout value
         */
         INT32             init( coordResource *pResource,
                                 _pmdEDUCB *cb,
                                 INT64 timeout = 0,
                                 IRemoteSessionHandler *pHandle = NULL,
                                 IGroupSessionHandler *pGroupHandle = NULL ) ;

         void              release() ;

         void              clear() ;
         void              resetSubSession() ;

         INT32             sendMsg( MsgHeader *pSrcMsg,
                                    UINT32 groupID,
                                    const netIOVec *pIov = NULL,
                                    pmdSubSession **ppSub = NULL ) ;

         INT32             sendMsg( MsgHeader *pSrcMsg,
                                    CoordGroupList &grpLst,
                                    const netIOVec *pIov = NULL ) ;

         INT32             sendMsg( MsgHeader *pSrcMsg,
                                    CoordGroupList &grpLst,
                                    const GROUP_2_IOVEC &iov ) ;

         pmdRemoteSession*       getSession() ;
         coordGroupSel*          getGroupSel() ;
         coordSessionPropSite*   getPropSite() ;
         coordGroupSessionCtrl*  getGroupCtrl() ;
         coordRemoteHandlerBase* getBaseHandle() ;

      protected:

         INT32             _sendMsg( MsgHeader *pSrcMsg,
                                     UINT32 groupID,
                                     const netIOVec *pIov,
                                     pmdSubSession **ppSub = NULL ) ;

      private:
         pmdRemoteSessionSite          *_pSite ;
         coordSessionPropSite          *_pPropSite ;
         pmdRemoteSession              *_pSession ;
         coordGroupSel                 _groupSel ;
         coordGroupSessionCtrl         _groupCtrl ;
         coordRemoteHandlerBase        _baseHandle ;

         IGroupSessionHandler          *_pGroupHandle ;

         INT64                         _timeout ;

   } ;
   typedef _coordGroupSession coordGroupSession ;

}

#endif // COORD_REMOTE_SESSION_HPP__

