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

   Source File Name = clsShardSession.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_SHARD_SESSION_HPP_
#define CLS_SHARD_SESSION_HPP_

#include "pmdAsyncSession.hpp"
#include "rtn.hpp"
#include "rtnInsertModifier.hpp"

using namespace bson ;

namespace engine
{

   class _clsReplicateSet ;
   class _clsShardMgr ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _clsCatalogAgent ;
   class _clsFreezingWindow ;
   class _rtnContextBase ;
   class _clsOprHandler ;

   struct _clsIdentifyInfo
   {
      UINT64   _id ;
      MsgRouteID _nid ;
      UINT32   _tid ;
      UINT64   _eduid ;

      string   _username ;
      string   _passwd ;
      string   _source ;

      UINT32   _auditMask ;
      UINT32   _auditConfigMask ;

      dpsTransConfItem  _transConf ;

      BSONObj  _objSchedInfo ;

      _clsIdentifyInfo()
      {
         _id = 0 ;
         _nid.value = 0 ;
         _tid = 0 ;
         _eduid = 0 ;

         _auditMask = 0 ;
         _auditConfigMask = 0 ;
      }
   } ;
   typedef _clsIdentifyInfo clsIdentifyInfo ;

   class _clsShdSession : public _pmdAsyncSession
   {
      friend class _clsOprHandler ;

      DECLARE_OBJ_MSG_MAP()

      public:
         _clsShdSession ( UINT64 sessionID, _schedTaskInfo *pTaskInfo ) ;
         virtual ~_clsShdSession ( ) ;

         virtual const CHAR*      sessionName() const ;
         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual BOOLEAN          isBusinessSession() const { return TRUE ; }
         virtual const CHAR*      className() const { return "Shard" ; }
         virtual EDU_TYPES eduType () const ;
         virtual void clear() ;

         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         virtual BOOLEAN timeout ( UINT32 interval ) ;
         virtual void    onTimer ( UINT64 timerID, UINT32 interval ) ;

         virtual void    onDispatchMsgBegin( const NET_HANDLE netHandle,
                                             const MsgHeader *pHeader ) ;
         virtual void    onDispatchMsgEnd( INT64 costUsecs ) ;

         BOOLEAN isSetLogout() const ;
         BOOLEAN isDelayLogin() const ;
         void    setLogout() ;
         void    setDelayLogin( const clsIdentifyInfo &info ) ;

         const dpsTransConfItem&    getTransConf() const ;
         const string&              getSource() const ;

      protected:
         enum CLS_CL_OP_TYPE {
            CLS_CL_OP_UNKNOWN = 0,
            CLS_CL_OP_WRITE,
            CLS_CL_OP_READ_ON_ANY,
            CLS_CL_OP_READ_ON_PRY,
            CLS_CL_OP_READ_ON_SND
         } ;

         INT32 _checkWriteStatus() ;
         INT32 _checkPrimaryWhenRead() ;
         INT32 _checkSecondaryWhenRead() ;

         /// do multi things to reduce times of getting lock
         INT32 _checkCLStatusAndGetSth( const CHAR *name,
                                        INT32 version,
                                        CLS_CL_OP_TYPE opType,
                                        INT16 *w = NULL,
                                        utilCLUniqueID *clUniqueID = NULL,
                                        BOOLEAN *repairCheck = NULL ) ;

         /// valid: replSize == NULL and clientW != NULL
         ///        replSize != NULL and clientW == NULL
         ///        replSize != NULL and clientW != NULL
         INT32 _calculateW( const INT16 *replSize,
                            const INT16 *clientW,
                            INT16 &w ) ;

         INT32 _checkCLVersion( const CHAR *name,
                                INT32 version,
                                BOOLEAN *isMainCL = NULL,
                                INT16 *w = NULL,
                                CHAR *mainCLName = NULL,
                                utilCLUniqueID *clUniqueID = NULL,
                                BOOLEAN *repairCheck = NULL,
                                BOOLEAN setReplStrategy = FALSE ) ;

         INT32   _reply ( MsgOpReply *header, const CHAR *buff, UINT32 size ) ;

         virtual void   _onDetach () ;
         virtual INT32  _defaultMsgFunc ( NET_HANDLE handle, MsgHeader* msg ) ;

         INT32   _createCSByCatalog( const CHAR *clFullName ) ;
         INT32   _createCLByCatalog( const CHAR *clFullName,
                                     const CHAR *pParent = NULL,
                                     BOOLEAN mustOnSelf = TRUE ) ;
         INT32   _renameCSByCatalog( const CHAR* csName,
                                     utilCSUniqueID csUniqueID ) ;
         INT32   _renameCLByCatalog( const CHAR* clFullName,
                                     utilCLUniqueID clUniqueID ) ;
         INT32   _processSubCLResult( INT32 result,
                                      const CHAR *clFullName,
                                      const CHAR *pParent ) ;

      //message functions
      protected:
         INT32 _onOPMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onUpdateReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                 utilUpdateResult &upResult ) ;
         INT32 _onInsertReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                 utilInsertResult &inResult ) ;
         INT32 _onDeleteReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                 utilDeleteResult &delResult ) ;
         INT32 _onQueryReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                rtnContextBuf &buffObj, INT32 &startingPos,
                                INT64 &contextID, BOOLEAN &needRollback,
                                BSONObjBuilder *pBuilder ) ;
         INT32 _onGetMoreReqMsg ( MsgHeader *msg, rtnContextBuf &buffObj,
                                  INT32 &startingPos, INT64 &contextID,
                                  BOOLEAN &needRollback ) ;
         INT32 _onAdvanceReqMsg ( MsgHeader *msg, rtnContextBuf &buffObj,
                                  INT32 &startingPos, INT64 &contextID,
                                  BOOLEAN &needRollback ) ;
         INT32 _onKillContextsReqMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onMsgReq ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onInterruptMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onTransBeginMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onTransCommitMsg (NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onTransRollbackMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onTransCommitPreMsg( NET_HANDLE handle, MsgHeader *msg );
         INT32 _onTransUpdateReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                      utilUpdateResult &upResult ) ;
         INT32 _onTransInsertReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                      utilInsertResult &inResult ) ;
         INT32 _onTransDeleteReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                      utilDeleteResult &delResult ) ;
         INT32 _onTransQueryReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                     rtnContextBuf &buffObj, INT32 &startingPos,
                                     INT64 &contextID, BOOLEAN &needRollback ) ;
         INT32 _onSessionInitReqMsg ( MsgHeader *msg ) ;

         INT32 _onCatalogChangeNtyMsg( MsgHeader *msg ) ;

         INT32 _onTransStopEvnt( pmdEDUEvent *event ) ;

         INT32 _onOpenLobReq( MsgHeader *msg,
                              SINT64 &contextID,
                              rtnContextBuf &buf ) ;

         INT32 _onWriteLobReq( MsgHeader *msg ) ;

         INT32 _onReadLobReq( MsgHeader *msg,
                              rtnContextBuf &buf ) ;

         INT32 _onUpdateLobReq( MsgHeader *msg ) ;

         INT32 _onLockLobReq( MsgHeader *msg ) ;

         INT32 _onGetLobRTDetailReq( MsgHeader *msg, rtnContextBuf &buffObj ) ;

         INT32 _onCloseLobReq( MsgHeader *msg ) ;

         INT32 _onRemoveLobReq( MsgHeader *msg ) ;

         INT32 _onPacketMsg( NET_HANDLE handle,
                             MsgHeader *msg,
                             INT64 &contextID,
                             rtnContextBuf &buf,
                             INT32 &startFrom,
                             INT32 &opCode ) ;

      private:
         INT32 _getShardingKey( const CHAR* clName,
                                BSONObj &shardingKey ) ;

         INT32 _includeShardingOrder( const CHAR *pCollectionName,
                                      const BSONObj &orderBy,
                                      INT32 &result ) ;
         INT32 _insertToMainCL( BSONObj &objs, INT32 objNum, INT32 flags,
                                INT16 w, BOOLEAN onlyCheck,
                                utilInsertResult &inResult,
                                const rtnInsertModifier *modifier = NULL ) ;

         INT32 _queryToMainCL( rtnQueryOptions &options,
                               pmdEDUCB *cb,
                               SINT64 &contextID,
                               rtnContextPtr *ppContext,
                               INT16 w = 1,
                               BOOLEAN isWrite = FALSE ) ;
         INT32 _updateToMainCL( rtnQueryOptions &options,
                                const BSONObj &updator,
                                pmdEDUCB *cb,
                                SDB_DMSCB *pDmsCB,
                                SDB_DPSCB *pDpsCB,
                                INT16 w,
                                utilUpdateResult &upResult );
         INT32 _deleteToMainCL ( rtnQueryOptions &options,
                                 pmdEDUCB *cb,
                                 SDB_DMSCB *dmsCB,
                                 SDB_DPSCB *dpsCB,
                                 INT16 w,
                                 utilDeleteResult &delResult );
         INT32 _runOnMainCL( const CHAR *pCommandName,
                             _rtnCommand *pCommand,
                             INT32 flags,
                             INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pQuery,
                             const CHAR *pField,
                             const CHAR *pOrderBy,
                             const CHAR *pHint,
                             INT16 w,
                             SINT64 &contextID,
                             BSONObjBuilder *pBuilder );

         INT32 _getOnMainCL( const CHAR *pCommandName,
                             const CHAR *pCollection,
                             INT32 flags,
                             INT64 numToSkip,
                             INT64 numToReturn,
                             const CHAR *pQuery,
                             const CHAR *pField,
                             const CHAR *pOrderBy,
                             const CHAR *pHint,
                             INT16 w,
                             SINT64 &contextID );

         INT32 _createIndexOnMainCL( const CHAR *pCommandName,
                                     const CHAR *pCollection,
                                     const CHAR *pQuery,
                                     const CHAR *pHint,
                                     INT16 w,
                                     BSONObjBuilder *pBuilder ) ;
         INT32 _createConsistentIndex( const BSONObj &boMatcher,
                                       const BSONObj &boHint ) ;
         INT32 _createStandaloneIndex( const CHAR *pCollection,
                                       const BSONObj &boMatcher,
                                       const BSONObj &boHint,
                                       BSONObjBuilder *pBuilder ) ;

         INT32 _dropIndexOnMainCL( const CHAR *pCommandName,
                                   const CHAR *pCollection,
                                   const CHAR *pQuery,
                                   const CHAR *pHint,
                                   INT16 w ) ;
         INT32 _dropConsistentIndex( const BSONObj &boMatcher,
                                     const BSONObj &boHint ) ;
         INT32 _dropStandaloneIndex( const CHAR *pCollection,
                                     const BSONObj &boMatcher,
                                     const BSONObj &boHint ) ;

         INT32 _copyIndexOnMainCL( const CHAR *pCommandName,
                                   const CHAR *pCollection,
                                   const CHAR *pQuery,
                                   const CHAR *pHint,
                                   INT16 w ) ;

         INT32 _dropMainCL( const CHAR *pCollection,
                            const CHAR *pQuery,
                            const CHAR *pHint,
                            INT16 w,
                            SINT64 &contextID ) ;

         INT32 _renameMainCL( const CHAR *pCollection,
                              INT16 w,
                              SINT64 &contextID ) ;

         INT32 _updateVCS( const CHAR *fullName, const BSONObj &updator ) ;

         // get and check all sub-collections
         INT32 _getAndChkAllSubCL( const CHAR *pCollectionName,
                                   BOOLEAN isWrite,
                                   CLS_SUBCL_LIST &subCLList,
                                   CLS_SUBCL_SORT_TYPE sortType = SUBCL_SORT_BY_ID ) ;

         // get and check sub-collections specified by matcher
         INT32 _getAndChkSubCL( const BSONObj &matcher,
                                const CHAR *pCollectionName,
                                BOOLEAN isWrite,
                                BOOLEAN isAllowEmptyList,
                                BSONObj &boNewMatcher,
                                CLS_SUBCL_LIST &strSubCLList,
                                INT32 *pIncludeShardingOrder ) ;

         // prepare sub-collection list from query matcher
         INT32 _prepareSubCLList( const BSONObj &matcher,
                                  BSONObj &boNewMatcher,
                                  CLS_SUBCL_LIST &subCLList ) ;
         // get sub-collection list by sharding order
         INT32 _getSubCLOrder( const CHAR *pCollectionName,
                               INT32 &includeShardingOrder,
                               CLS_SUBCL_LIST &subCLList ) ;

         // check sub-collections ( write operators, version, etc )
         INT32 _checkSubCLList( const CHAR *pCollectionName,
                                const CLS_SUBCL_LIST &subCLList,
                                BOOLEAN isWrite,
                                BOOLEAN isAllowEmptyList ) ;

         // check sub-collection ( write operators, version, etc )
         INT32 _checkSubCL( const CHAR *mainCLName,
                            const CHAR *subCLName ) ;

         // get sub-collection list
         INT32 _getSubCLList( const CHAR *pCollectionName,
                              CLS_SUBCL_LIST &subCLList,
                              CLS_SUBCL_SORT_TYPE sortType = SUBCL_SORT_BY_ID ) ;

         INT32 _truncateMainCL( const CHAR *fullName,
                                const CHAR *pQuery,
                                const CHAR *pHint,
                                INT16 w,
                                SINT64 &contextID ) ;

         INT32 _testMainCollection( const CHAR *fullName ) ;

         INT32 _alterMainCL( _rtnCommand *command,
                             pmdEDUCB *cb,
                             SDB_DPSCB *dpsCB,
                             BSONObjBuilder *pBuilder ) ;

         INT32 _analyzeMainCL( _rtnCommand *command ) ;
         INT32 _resetSnapshotMainCL ( _rtnCommand * command ) ;

         INT32 _checkPrimaryStatus() ;

         INT32 _checkRollbackStatus() ;

         INT32 _checkReplStatus() ;

         INT32 _checkClusterActive( MsgHeader *msg ) ;

         void  _login() ;

         INT32 _testCollectionBeforeCreate( const CHAR* clName,
                                            utilCLUniqueID clUniqueID ) ;

         INT32 _checkTransAutoCommit( const MsgHeader *msg ) ;

         // wait synchronized for offset
         // - offset: LSN offset to wait
         // - w: number of replica to wait
         // - timeout: timeout to wait,
         //            0 means only test,
         //            <0 means wait forever
         INT32 _waitSync( const DPS_LSN_OFFSET &offset,
                          INT32 w,
                          INT32 timeout ) ;
         // rollback current transaction
         // for wait commit status, we need to wait pre-commit log to be
         // replicated for at lease one another node
         // - checkStatusTimeout: timeout to check status with other groups
         //                       involved in transaction
         // - waitSyncTimeout: timeout to wait for pre-commit log to be
         //                    replicated,
         //                    0 means only test, <0 means wait forever
         // - ignoreWaitSyncError: indicate whether to ignore wait sync error
         //                        if so, it may cause inconsistent status in
         //                        different groups
         INT32 _rollbackTrans( BOOLEAN *pHasRollback = NULL,
                               INT32 checkStatusTimeout = OSS_ONE_SEC * 60,
                               INT32 waitSyncTimeout = OSS_ONE_SEC * 60,
                               BOOLEAN ignoreWaitSyncError = FALSE ) ;

         void _setCollectionName( const CHAR *collectionName )
         {
            _pEDUCB->setCurProcessName( collectionName ) ;
            _pCollectionName = _pEDUCB->getCurProcessName() ;
         }

         void _setCollectionSpaceName( const CHAR *collectionSpaceName )
         {
            _pEDUCB->setCurProcessName( collectionSpaceName ) ;
            _pCollectionName = NULL ;
         }

         void _clearCollectionName()
         {
            _pCollectionName = NULL ;
         }

         void _clearProcessInfo()
         {
            _pCollectionName = NULL ;
            _pEDUCB->clearProcessInfo() ;
         }

         INT32 _getCSInfoWhenLoadCS( _rtnLoadCollectionSpace* pCommand ) ;

         INT32 _getIndexInfoFromCatalog( utilCLUniqueID clUniqID,
                                         ossPoolVector<BSONObj> &indexInfo ) ;

      protected:
         _clsReplicateSet       *_pReplSet ;
         _clsShardMgr           *_pShdMgr ;
         _clsCatalogAgent       *_pCatAgent ;
         _clsFreezingWindow     *_pFreezingWindow ;
         _SDB_DMSCB             *_pDmsCB ;
         _SDB_RTNCB             *_pRtnCB ;
         _dpsLogWrapper         *_pDpsCB ;
         _schedTaskInfo         *_pTaskInfo ;

         MsgOpReply             _replyHeader ;
         MsgRouteID             _primaryID ;
         BSONObj                _errorInfo ;
         const CHAR             *_pCollectionName ;
         INT32                  _clVersion ;
         BOOLEAN                _isMainCL ;
         BOOLEAN                _hasUpdateCataInfo ;

         ossTimestamp           _lastRecvTime ;

         CHAR                   _detailName[SESSION_NAME_LEN+1] ;
         BOOLEAN                _logout ;
         BOOLEAN                _delayLogin ;
         string                 _username ;
         string                 _passwd ;
         string                 _source ;
         BSONObj                _objDelayInfo ;
         dpsTransConfItem       _transConf ;

         UINT32                 _inPacketLevel ;
         INT64                  _pendingContextID ;
         INT32                  _pendingStartFrom ;
         rtnContextBuf          _pendingBuff ;

         UINT32                 _transWaitTimeout ;
         DPS_TRANS_ID           _transWaitID ;

         BSONObjBuilder         _retBuilder ;
   } ;
}

#endif //CLS_SHARD_SESSION_HPP_

