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

   Source File Name = clsReplicateSet.hpp

   Descriptive Name = Replication Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSREPLICATESET_HPP_
#define CLSREPLICATESET_HPP_

#include "netRouteAgent.hpp"
#include "msgReplicator.hpp"
#include "msgCatalog.hpp"
#include "clsVoteMachine.hpp"
#include "msg.hpp"
#include "pmdObjBase.hpp"
#include "clsCatalogCaller.hpp"
#include "clsSyncManager.hpp"
#include "dms.hpp"
#include "clsReplBucket.hpp"
#include "dpsDef.hpp"
#include "ossQueue.hpp"
#include "clsReelection.hpp"
#include "utilReplSizePlan.hpp"
#include "utilBitmap.hpp"
#include <vector>

using namespace std ;

namespace engine
{
   class _netRouteAgent ;
   class _clsMgr ;
   class _clsDataSrcBaseSession ;
   class _pmdEDUCB ;
   class _pmdFTMgr ;

   #define CLS_SYNCCTRL_THRESHOLD_SIZE          (10)
   #define CLS_SYNC_DFT_TIMEOUT                 ( 600 * OSS_ONE_SEC )

   #define CLS_SYNCWAIT_FIX_TIME_SLICE          ( 10 * OSS_ONE_SEC )
   #define CLS_DISABLE_SRC_INTERVAL             ( 3600 * OSS_ONE_SEC )

   /*
      _clsReplicateSet define
   */
   class _clsReplicateSet : public _pmdObjBase, public _dpsEventHandler,
                            public _ICluster, public _clsReplayEventHandler
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsReplicateSet( _netRouteAgent *agent ) ;
         virtual ~_clsReplicateSet() ;

      public:
         /*
            When is SDB_DPS_INVALID_LSN_OFFSET means keep the same with
            ExpectLSN(in dps)
         */
         virtual UINT64    completeLsn( BOOLEAN doFast = TRUE,
                                        UINT32 *pVer = NULL ) ;
         virtual UINT32    lsnQueSize() ;
         /*
            When is SDB_DPS_INVALID_LSN_OFFSET means keep the same with
            ExpectLSN(in dps)
         */
         virtual BOOLEAN   primaryLsn( UINT64 &lsn, UINT32 *pVer = NULL ) ;

      public:
         OSS_INLINE BOOLEAN primaryIsMe()
         {
            return _vote.primaryIsMe() ;
         }

         OSS_INLINE BOOLEAN locationPrimaryIsMe()
         {
            return _locationVote.primaryIsMe() ;
         }

         OSS_INLINE BOOLEAN isActiveLocation()
         {
            return _vote.hasElectionWeight( CLS_ELECTION_WEIGHT_ACTIVE_LOCATION ) ;
         }

         OSS_INLINE clsBucket* getBucket ()
         {
            return &_replBucket ;
         }

         OSS_INLINE void setLocalID( const MsgRouteID &id )
         {
            _info.local = id ;
            _locationInfo.local = id ;
            /// _agent was set by clsMgr.
         }

         OSS_INLINE const UINT32 ailves( BOOLEAN isLocation = FALSE )
         {
            _clsGroupInfo &info = isLocation ? _locationInfo : _info ;
            ossScopedRWLock lock( &info.mtx, SHARED ) ;
            return info.aliveSize() ;
         }

         OSS_INLINE UINT32 groupSize( BOOLEAN isLocation = FALSE )
         {
            _clsGroupInfo &info = isLocation ? _locationInfo : _info ;
            ossScopedRWLock lock( &info.mtx, SHARED ) ;
            return info.groupSize() ;
         }

         OSS_INLINE UINT32 criticalSize()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.criticalSize() ;
         }

         OSS_INLINE UINT32 maintenanceSize()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.maintenanceSize() ;
         }

         OSS_INLINE UINT32 criticalAliveSize()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.criticalAliveSize() ;
         }

         OSS_INLINE INT16 majoritySize()
         {
            INT16 w = 0 ;

            ossScopedRWLock lock( &_info.mtx, SHARED ) ;

            if ( CLS_GROUP_MODE_CRITICAL == _info.localGrpMode )
            {
               // If group is in critical mode, use critical size to calculate majority size
               w = (INT16)( _info.criticalSize() / 2 + 1 ) ;
            }
            else
            {
               w = (INT16)( _info.groupSize() / 2 + 1 ) ;
            }

            return w ;
         }

         OSS_INLINE CLS_GROUP_MODE getGrpMode()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;
            return _info.grpMode.mode ;
         }

         OSS_INLINE void getDetailInfo( UINT32 &nodeCnt, UINT32 &aliveCnt,
                                        UINT32 &falutCnt, UINT32 &ssCnt,
                                        INT32 &indoubtErr,
                                        UINT16 &indoubtNodeID,
                                        utilLocationInfo *locationInfo = NULL,
                                        const SDB_CONSISTENCY_STRATEGY strategy =
                                        SDB_CONSISTENCY_NODE )
         {
            map<UINT64, _clsSharingStatus *>::iterator it ;
            _clsSharingStatus *pStatus = NULL ;

            nodeCnt = 0 ;
            aliveCnt = 0 ;
            falutCnt = 0 ;
            ssCnt = 0 ;
            indoubtErr = SDB_OK ;
            UINT32 selfLocationID = pmdGetLocationID() ;
            UINT32 locationID = MSG_INVALID_LOCATIONID ;
            UINT8 primaryLocationNodes = 0 ;
            UINT8 affinitiveLocations = 0 ;
            UINT8 locations = 0 ;
            _utilStackBitmap< CLS_REPLSET_MAX_NODE_SIZE > isMarked ;
            BOOLEAN needLocInfo = SDB_CONSISTENCY_NODE != strategy ;
            UINT32 remoteAliveNodeCnt = 0 ;

            ossScopedRWLock lock( &_info.mtx, SHARED ) ;

            if ( CLS_GROUP_MODE_CRITICAL == _info.grpMode.mode )
            {
               const clsGrpModeItem &grpModeItem = _info.grpMode.grpModeInfo[0] ;

               // This is critical node mode, use alive node count
               if ( INVALID_NODEID != grpModeItem.nodeID )
               {
                  nodeCnt = _info.aliveSize() ;
                  aliveCnt = _info.aliveSize() ;
               }
               // This is critical location mode, use location node count
               else if ( ! grpModeItem.location.empty() )
               {
                  nodeCnt = _info.criticalSize() ;
                  aliveCnt = _info.criticalAliveSize() ;
               }
            }
            else
            {
               nodeCnt = _info.groupSize() ;
               aliveCnt = _info.aliveSize() ;
            }

            it = _info.alives.begin() ;
            while( it != _info.alives.end() )
            {
               pStatus = it->second ;
               ++it ;

               if ( CLS_GROUP_MODE_MAINTENANCE == pStatus->grpMode )
               {
                  continue ;
               }

               // If the remoteLocationConsistency is false,
               // don't care whether the remote node failed.
               locationID = pStatus->beat.locationID ;
               if ( MSG_INVALID_LOCATIONID != selfLocationID &&
                    isActiveLocation() &&
                    !pStatus->isAffinitiveLocation &&
                    !_remoteLocationConsistency )
               {
                  ++remoteAliveNodeCnt ;
                  continue ;
               }

               if ( CLS_NODE_STOP == pStatus->beat.nodeRunStat )
               {
                  --aliveCnt ;
                  continue ;
               }
               else if ( 0 != pStatus->beat.ftConfirmStat )
               {
                  ++falutCnt ;
                  if ( SDB_OK == indoubtErr )
                  {
                     indoubtErr = pStatus->beat.indoubtErr ;
                     indoubtNodeID = pStatus->beat.identity.columns.nodeID ;
                  }
                  continue ;
               }
               else if ( CLS_NODE_RUNNING != pStatus->beat.nodeRunStat )
               {
                  ++ssCnt ;
                  continue ;
               }

               if ( needLocInfo )
               {
                  if ( MSG_INVALID_LOCATIONID != selfLocationID &&
                       MSG_INVALID_LOCATIONID != locationID )
                  {
                     if ( selfLocationID == locationID )
                     {
                        primaryLocationNodes++ ;
                     }
                     else if ( !isMarked.testBit( pStatus->locationIndex ) &&
                               pStatus->isAffinitiveLocation )
                     {
                        locations++ ;
                        affinitiveLocations++ ;
                        isMarked.setBit( pStatus->locationIndex ) ;
                     }
                     // If the remoteLocationConsistency is false, we ignore the impact
                     // of remote nodes on synchronization consistency.
                     else if ( !isMarked.testBit( pStatus->locationIndex ) &&
                               _remoteLocationConsistency )
                     {
                        locations++ ;
                        isMarked.setBit( pStatus->locationIndex ) ;
                     }
                  }
               }
            }
            if ( needLocInfo && NULL != locationInfo )
            {
               locationInfo->primaryLocationNodes = primaryLocationNodes ;
               locationInfo->locations = locations ;
               locationInfo->affinitiveLocations = affinitiveLocations ;
            }

            if ( !_remoteLocationConsistency && isActiveLocation() )
            {
               if ( CLS_GROUP_MODE_CRITICAL == _info.grpMode.mode )
               {
                  const clsGrpModeItem &grpModeItem = _info.grpMode.grpModeInfo[0] ;

                  // This is critical node mode, use alive node count
                  if ( INVALID_NODEID != grpModeItem.nodeID )
                  {
                     nodeCnt -= remoteAliveNodeCnt ;
                     aliveCnt -= remoteAliveNodeCnt ;
                  }
               }
               else
               {
                  nodeCnt -= _info.remoteLocationNodeSize ;
                  aliveCnt -= remoteAliveNodeCnt ;
               }
               if ( NULL != locationInfo )
               {
                  locationInfo->affinitiveNodes = nodeCnt ;
               }
            }
         }

         OSS_INLINE BOOLEAN isAlive ( NodeID node )
         {
            BOOLEAN bAlive = FALSE ;
            _info.mtx.lock_r() ;
            map<UINT64, _clsSharingStatus *>::iterator it =
               _info.alives.find ( node.value ) ;
            if ( it != _info.alives.end() )
            {
               bAlive = TRUE ;
            }
            _info.mtx.release_r() ;

            return bAlive ;
         }

         OSS_INLINE _clsSyncManager *syncMgr()
         {
            return &_sync ;
         }

         OSS_INLINE _clsVoteMachine* voteMachine( BOOLEAN isLocation = FALSE )
         {
            return isLocation ? &_locationVote : &_vote ;
         }

         OSS_INLINE INT32 sync( const DPS_LSN_OFFSET &offset,
                                _pmdEDUCB *eduCB,
                                UINT32 w = 1,
                                INT64 timeout = CLS_SYNC_DFT_TIMEOUT )
         {
            if ( DPS_INVALID_LSN_OFFSET == offset || 1 >= w )
            {
               return SDB_OK ;
            }

            _clsSyncSession session ;
            session.eduCB = eduCB ;
            eduCB->getEvent().reset() ;

            if ( w > CLS_REPLSET_MAX_NODE_SIZE )
            {
               w = CLS_REPLSET_MAX_NODE_SIZE ;
            }

            session.waitPlan = eduCB->getOperator()->getWaitplan() ;
            session.waitPlan.offset = offset ;
            return _sync.sync( session, w, timeout,
                               FT_LEVEL_WHOLE == _pFTMgr->getFTLevel() ?
                                                             TRUE : FALSE ) ;
         }

         OSS_INLINE UINT32 getNtySessionNum ()
         {
            return _srcSessionNum ;
         }

         OSS_INLINE BOOLEAN isInStepUp() const
         {
            return _vote.isInStepUp() ;
         }

         OSS_INLINE BOOLEAN isInCriticalMode()
         {
            return CLS_GROUP_MODE_CRITICAL == _info.localGrpMode ;
         }

         OSS_INLINE BOOLEAN isInMaintenanceMode()
         {
            return CLS_GROUP_MODE_MAINTENANCE == _info.localGrpMode ;
         }

         OSS_INLINE BOOLEAN isInEnforcedGrpMode()
         {
            return _info.enforcedGrpMode ;
         }

         OSS_INLINE BOOLEAN isReadyForSrc( UINT64 curTick )
         {
            UINT64 lastLogMoveTick = _lastLogMoveTick.fetch() ;
            UINT64 curTimeSpan = pmdDBTickSpan2Time( curTick - lastLogMoveTick ) ;
            /// source is ready in the following two cases
            /// 1. During this full sync, the secondary replay log has not failed
            /// 2. The downtime of the last full sync has passed(curTimeSpan > 1h)
            return 0 == lastLogMoveTick || curTimeSpan > CLS_DISABLE_SRC_INTERVAL ;
         }

         OSS_INLINE void onReplayLogError()
         {
            _lastLogMoveTick.swap( pmdGetDBTick() ) ;
         }

         ossQueue< clsLSNNtyInfo >* getNtyQue() { return &_ntyQue ; }
         DPS_LSN_OFFSET getNtyLastOffset() const { return _ntyLastOffset ; }
         DPS_LSN_OFFSET getNtyProcessedOffset() const { return _ntyProcessedOffset ; }

         DPS_LSN_OFFSET getNtyReplayOffset() const { return _ntyReplayOffset ; }
         void updateNtyReplayOffset( DPS_LSN_OFFSET offset )
         {
            if ( offset > _ntyReplayOffset )
            {
               _ntyReplayOffset = offset ;
            }
         }
         void resetNtyReplayOffset( DPS_LSN_OFFSET offset )
         {
            _ntyReplayOffset = offset ;
         }

         void notify2Session( UINT32 suLID, UINT32 clLID,
                              dmsExtentID extID, dmsOffset extOffset,
                              const OID &lobOid, UINT32 lobSequence,
                              const DPS_LSN_OFFSET &offset ) ;

         virtual void onWriteLog( DPS_LSN_OFFSET offset ) ;

         virtual void onPrepareLog( UINT32 csLID, UINT32 clLID,
                                    UINT32 extID, UINT32 extOffset,
                                    const OID &lobOid, UINT32 lobSequence,
                                    DPS_LSN_OFFSET offset ) ;

         virtual void onMoveLog( DPS_LSN_OFFSET moveToOffset,
                                 DPS_LSN_VER moveToVersion,
                                 DPS_LSN_OFFSET expectOffset,
                                 DPS_LSN_VER expectVersion,
                                 DPS_MOMENT moment,
                                 INT32 errcode ) ;

         virtual void onReplayLog( UINT32 csLID, UINT32 clLID,
                                   UINT32 extID, UINT32 extOffset,
                                   const OID &lobOid, UINT32 lobSequence,
                                   DPS_LSN_OFFSET offset ) ;

         virtual INT32 canAssignLogPage( UINT32 reqLen, pmdEDUCB *cb ) ;

         virtual INT32 onCompleteOpr( _pmdEDUCB *cb, INT32 w )
         {
            INT32 rc = SDB_OK ;
            UINT32 timeout = 0 ;
            UINT32 onceTimeout = 0 ;
            BOOLEAN replCheckRC = SDB_OK ;

            while ( TRUE )
            {
               UINT32 tmpSyncWaitTimeout = SDB_OK == replCheckRC ?
                                           _syncwaitTimeout :
                                           _fusingTimeout ;

               if ( tmpSyncWaitTimeout <= timeout )
               {
                  onceTimeout = 1 ;
               }
               else if ( tmpSyncWaitTimeout < timeout +
                                              CLS_SYNCWAIT_FIX_TIME_SLICE )
               {
                  onceTimeout = tmpSyncWaitTimeout - timeout ;
               }
               else
               {
                  onceTimeout = CLS_SYNCWAIT_FIX_TIME_SLICE ;
               }

               rc = sync( cb->getEndLsn(), cb, w, onceTimeout ) ;
               if ( SDB_TIMEOUT == rc || SDB_DATABASE_DOWN == rc )
               {
                  if ( SDB_DATABASE_DOWN == rc ||
                       SDB_CLS_WAIT_SYNC_FAILED == replCheckRC )
                  {
                     rc = SDB_CLS_WAIT_SYNC_FAILED ;
                  }
                  timeout += onceTimeout ;
                  if ( timeout >= tmpSyncWaitTimeout )
                  {
                     break ;
                  }

                  INT16 tmpW = 0 ;
                  replCheckRC = replSizeCheck( cb->getOrgReplSize(),
                                               tmpW, cb, TRUE ) ;
                  /// check replsize again
                  if ( SDB_OK == replCheckRC )
                  {
                     w = tmpW ;
                  }
                  continue ;
               }

               break ;
            }

            /// clean saved org repl size
            cb->setOrgReplSize( 1 ) ;
            return rc ;
         }

         virtual void  onSwitchLogFile( UINT32 preLogicalFileId,
                                        UINT32 preFileId,
                                        UINT32 curLogicalFileId,
                                        UINT32 curFileId )
         {
            return ;
         }

         BOOLEAN isMajorityAlive()
         {
            ossScopedRWLock lock( &_info.mtx, SHARED ) ;

            return CLS_IS_MAJORITY( _info.getAlivesByTimeout( CLS_NODE_KEEPALIVE_TIMEOUT ),
                                    _info.groupSize() ) ||
                   ( CLS_GROUP_MODE_CRITICAL == _info.localGrpMode &&
                     CLS_IS_MAJORITY( _info.getCriticalAlivesByTimeout( CLS_NODE_KEEPALIVE_TIMEOUT ),
                                      _info.criticalSize() ) ) ;
         }

         virtual INT32 canAssignLogPageOnSecondary( UINT32 reqLen, _pmdEDUCB *cb )
         {
            return SDB_OK ;
         }

      public:
         void  regSession ( _clsDataSrcBaseSession *pSession ) ;
         void  unregSession ( _clsDataSrcBaseSession *pSession ) ;

      public:
         INT32 initialize() ;
         INT32 active() ;
         INT32 deactive () ;
         INT32 final() ;
         void  onConfigChange() ;
         void  ntyPrimaryChange( BOOLEAN primary,
                                 SDB_EVENT_OCCUR_TYPE type ) ;

         virtual void  onTimer ( UINT64 timerID, UINT32 interval ) ;

         INT32 handleMsg( NET_HANDLE handle, MsgHeader* msg ) ;

         INT32 handleEvent( pmdEDUEvent *event ) ;

         INT32 callCatalog( MsgHeader *header, UINT32 times = 1 ) ;

         const _clsCataCallerMeta* getCataCallerMeta( UINT32 key ) ;

         BOOLEAN getPrimaryInfo( _clsSharingStatus &primaryInfo ) ;

         void getGroupInfo( _MsgRouteID &primary,
                            vector<_netRouteNode > &group ) ;

         const CLS_LOC_INFO_MAP& getLocInfoMap() const
         {
            return _info.locationInfoMap ;
         }

         MsgRouteID     getPrimary () ;
         MsgRouteID     getLocationPrimary () ;
         BOOLEAN        isSendNormal( UINT64 nodeID ) ;

         ossEvent*      getFaultEvent() ;
         ossEvent*      getSyncEmptyEvent() ;

         INT64 netIn() ;
         INT64 netOut() ;

         INT32 reelect( CLS_REELECTION_LEVEL lvl,
                        INT32 seconds,
                        pmdEDUCB *cb,
                        UINT16 destID = 0 ) ;

         void reelectionDone() ;

         INT32 locationReelect( CLS_REELECTION_LEVEL lvl,
                                INT32 seconds,
                                pmdEDUCB *cb,
                                UINT16 destID = 0 ) ;

         void locationReelectionDone() ;

         /// this func is used to support command "forceStepUp".
         INT32 stepUp( UINT32 seconds,
                       pmdEDUCB *cb ) ;

         INT32 primaryCheck( pmdEDUCB *cb ) ;
         INT32 replSizeCheck( INT16 w, INT16 &finalW, _pmdEDUCB *cb,
                              BOOLEAN isAfterData = FALSE ) ;

         INT32 aliveNode( const MsgRouteID &id ) ;

         UINT64   getLastConsultTick() const ;
         void     setLastConsultTick( UINT64 tick ) ;

      private:
         INT32 _checkGroupInfo( const CLS_GROUP_VERSION &version,
                                const map<UINT64, _netRouteNode> &nodes ) ;

         INT32 _checkGrpModeInfo( CLS_GROUP_MODE grpMode ) ;

         INT32 _setGroupSet( const CLS_GROUP_VERSION &version,
                             const CLS_LOC_INFO_MAP &locationInfoMap,
                             map<UINT64, _netRouteNode> &nodes,
                             BOOLEAN &changeStatus ) ;

         INT32 _setLocationSet( const map<UINT64, _netRouteNode> &nodes ) ;
         INT32 _setLocationInfo( const map<UINT64, _netRouteNode> &nodes,
                                 CLS_LOC_INFO_MAP &locationInfoMap,
                                 const ossPoolString &activeLocation ) ;

         void _setElectionWeight( const ossPoolString &activeLocation ) ;

         BOOLEAN _isUDPHandle( NET_HANDLE handle ) ;

         INT32 _alive( const _MsgRouteID &id,
                       BOOLEAN fromUDP,
                       BOOLEAN isLocation = FALSE ) ;

         INT32 _handleSharingBeat( NET_HANDLE handle, const _MsgClsBeat *msg ) ;

         INT32 _handleSharingBeatRes( NET_HANDLE handle,
                                      const _MsgClsBeatRes *msg ) ;

         INT32 _handleGroupRes( const MsgCatGroupRes *msg ) ;

         void _sharingBeat() ;

         INT32 _sendSharingBeat( _clsSharingStatus &status,
                                 MsgClsBeat *message ) ;

         void _checkBreak( const UINT32 &millisec ) ;

         UINT32 _getThresholdTime( UINT64 diffSize ) ;

         INT32 _handleStepDown( BOOLEAN isLocation ) ;

         INT32 _handleStepUp( UINT32 seconds ) ;

         INT32 _handleGroupModeUpdate( const clsGroupMode *pGrpMode ) ;

         void _notifySrcSessions( UINT32 csLID, UINT32 clLID,
                                  UINT32 extID, UINT32 extOffset,
                                  OID lobOid, UINT32 lobSequence,
                                  DPS_LSN_OFFSET offset ) ;

         void _forceSrcSessions() ;

         void _calLocationAffinity( const map<UINT64, _netRouteNode> &nodes,
                                    CLS_LOC_INFO_MAP &locationInfoMap ) ;

         INT32 _handleBallot( const MsgHeader *header ) ;

         INT32 _handleBallotRes( const MsgHeader *header ) ;

      private:
         _netRouteAgent          *_agent ;
         _clsGroupInfo           _info ;
         _clsGroupInfo           _locationInfo ;
         _clsVoteMachine         _vote ;
         _clsVoteMachine         _locationVote ;
         _dpsLogWrapper          *_logger ;
         _pmdFTMgr               *_pFTMgr ;
         _clsSyncManager         _sync ;
         _clsCatalogCaller       _cata ;
         _clsReelection          _reelection ;
         _clsReelection          _locationReelection ;
         clsBucket               _replBucket ;
         _clsMgr                 *_clsCB ;
         UINT64                  _timerID ;
         UINT32                  _beatTime ;
         BOOLEAN                 _active ;
         BOOLEAN                 _locationActive ;
         UINT64                  _lastTimerTick ;

         UINT64                  _lastConsultTick ;

         UINT32                  _srcSessionNum ;
         ossRWMutex              _vecLatch ;
         std::vector<_clsDataSrcBaseSession*> _vecSrcSessions ;

         // notify queue
         ossQueue< clsLSNNtyInfo >  _ntyQue ;
         DPS_LSN_OFFSET             _ntyLastOffset ;
         DPS_LSN_OFFSET             _ntyProcessedOffset ;
         DPS_LSN_OFFSET             _ntyReplayOffset ;

         // sync control param
         UINT64                  _totalLogSize ;
         UINT64                  _sizethreshold[ CLS_SYNCCTRL_THRESHOLD_SIZE ] ;
         UINT32                  _timeThreshold[ CLS_SYNCCTRL_THRESHOLD_SIZE ] ;
         BOOLEAN                 _inSyncCtrl ;

         ossEvent                _faultEvent ;
         ossEvent                _syncEmptyEvent ;

         UINT32                  _syncwaitTimeout ;
         UINT32                  _shutdownWaitTimeout ;
         UINT32                  _fusingTimeout ;

         BOOLEAN                 _isAllNodeFatal ;
         ossEvent                _heartbeatEvent ;

         ossAtomic64             _lastLogMoveTick ;
         BOOLEAN                 _remoteLocationConsistency ;
   } ;

   typedef class _clsReplicateSet clsReplicateSet ;
   typedef _clsReplicateSet replCB ;
}

#endif

