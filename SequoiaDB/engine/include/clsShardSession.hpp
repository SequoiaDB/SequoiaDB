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

   struct _clsIdentifyInfo
   {
      UINT64   _id ;
      UINT32   _tid ;
      UINT64   _eduid ;

      string   _username ;
      string   _passwd ;

      _clsIdentifyInfo()
      {
         _id = 0 ;
         _tid = 0 ;
         _eduid = 0 ;
      }
   } ;
   typedef _clsIdentifyInfo clsIdentifyInfo ;

   class _clsShdSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()

      public:
         _clsShdSession ( UINT64 sessionID ) ;
         virtual ~_clsShdSession ( ) ;

         virtual const CHAR*      sessionName() const ;
         virtual SDB_SESSION_TYPE sessionType() const ;
         virtual const CHAR*      className() const { return "Shard" ; }
         virtual EDU_TYPES eduType () const ;
         virtual void clear() ;

         virtual void    onRecieve ( const NET_HANDLE netHandle,
                                     MsgHeader * msg ) ;
         virtual BOOLEAN timeout ( UINT32 interval ) ;

         BOOLEAN isSetLogout() const ;
         BOOLEAN isDelayLogin() const ;
         void    setLogout() ;
         void    setDelayLogin( const clsIdentifyInfo &info ) ;

      protected:
         INT32 _checkWriteStatus() ;
         INT32 _checkPrimaryWhenRead( INT32 flag, INT32 reqFlag ) ;

         INT32 _checkCLStatusAndGetSth( const CHAR *name,
                                        INT32 version,
                                        BOOLEAN *isMainCL = NULL,
                                        INT16 *w = NULL,
                                        CHAR *mainCLName = NULL ) ;

         INT32 _calculateW( const INT16 *replSize,
                            const INT16 *clientW,
                            INT16 &w ) ;

         INT32   _reply ( MsgOpReply *header, const CHAR *buff, UINT32 size ) ;

         virtual void   _onDetach () ;
         virtual INT32  _defaultMsgFunc ( NET_HANDLE handle, MsgHeader* msg ) ;

         INT32   _createCSByCatalog( const CHAR *clFullName ) ;
         INT32   _createCLByCatalog( const CHAR *clFullName,
                                     const CHAR *pParent = NULL,
                                     BOOLEAN mustOnSelf = TRUE ) ;
         INT32   _processSubCLResult( INT32 result,
                                      const CHAR *clFullName,
                                      const CHAR *pParent ) ;

      protected:
         INT32 _onOPMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onUpdateReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                 INT64 &updateNum ) ;
         INT32 _onInsertReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                 INT32 &insertedNum, INT32 &ignoredNum ) ;
         INT32 _onDeleteReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                 INT64 &delNum ) ;
         INT32 _onQueryReqMsg ( NET_HANDLE handle, MsgHeader *msg, 
                                rtnContextBuf &buffObj, INT32 &startingPos,
                                INT64 &contextID ) ;
         INT32 _onGetMoreReqMsg ( MsgHeader *msg, rtnContextBuf &buffObj,
                                  INT32 &startingPos, INT64 &contextID ) ;
         INT32 _onKillContextsReqMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onMsgReq ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onInterruptMsg ( NET_HANDLE handle, MsgHeader *msg ) ;
         INT32 _onTransBeginMsg ();
         INT32 _onTransCommitMsg ();
         INT32 _onTransRollbackMsg ();
         INT32 _onTransCommitPreMsg( MsgHeader *msg );
         INT32 _onTransUpdateReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                      INT64 &updateNum ) ;
         INT32 _onTransInsertReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                      INT32 &insertedNum,
                                      INT32 &ignoredNum ) ;
         INT32 _onTransDeleteReqMsg ( NET_HANDLE handle, MsgHeader *msg,
                                      INT64 &delNum ) ;
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

         INT32 _onCloseLobReq( MsgHeader *msg ) ;

         INT32 _onRemoveLobReq( MsgHeader *msg ) ;

      private:
         INT32 _getShardingKey( const CHAR* clName,
                                BSONObj &shardingKey ) ;

         INT32 _includeShardingOrder( const CHAR *pCollectionName,
                                      const BSONObj &orderBy,
                                      BOOLEAN &result ) ;
         INT32 _insertToMainCL( BSONObj &objs, INT32 objNum, INT32 flags,
                                INT16 w, INT32 &insertedNum,
                                INT32 &ignoredNum ) ;

         INT32 _queryToMainCL( rtnQueryOptions &options,
                               pmdEDUCB *cb,
                               SINT64 &contextID,
                               _rtnContextBase **ppContext = NULL,
                               INT16 w = 1 ) ;
         INT32 _updateToMainCL( rtnQueryOptions &options,
                                const BSONObj &updator,
                                pmdEDUCB *cb,
                                SDB_DMSCB *pDmsCB,
                                SDB_DPSCB *pDpsCB,
                                INT16 w,
                                INT64 *pUpdateNum = NULL );
         INT32 _deleteToMainCL ( rtnQueryOptions &options,
                                 pmdEDUCB *cb,
                                 SDB_DMSCB *dmsCB,
                                 SDB_DPSCB *dpsCB,
                                 INT16 w,
                                 INT64 *pDelNum = NULL );
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
                             INT32 version,
                             SINT64 &contextID );

         INT32 _getOnMainCL( const CHAR *pCommand,
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

         INT32 _createIndexOnMainCL( const CHAR *pCommand,
                                     const CHAR *pCollection,
                                     const CHAR *pQuery,
                                     const CHAR *pHint,
                                     INT16 w,
                                     SINT64 &contextID,
                                     BOOLEAN syscall = FALSE );

         INT32 _dropIndexOnMainCL( const CHAR *pCommand,
                                   const CHAR *pCollection,
                                   const CHAR *pQuery,
                                   INT16 w,
                                   SINT64 &contextID,
                                   BOOLEAN syscall = FALSE );

         INT32 _dropMainCL( const CHAR *pCollection,
                           INT16 w,
                           INT32 version,
                           SINT64 &contextID );

         INT32 _getSubCLList( const BSONObj &matcher,
                              const CHAR *pCollectionName,
                              BSONObj &boNewMatcher,
                              vector< string > &strSubCLList ) ;

         INT32 _getSubCLList( const CHAR *pCollectionName,
                              vector< string > &subCLList ) ;

         INT32 _sortSubCLListByBound( const CHAR *pCollectionName,
                                      std::vector< std::string > &strSubCLList ) ;

         INT32 _truncateMainCL( const CHAR *fullName ) ;

         INT32 _testMainCollection( const CHAR *fullName ) ;

         INT32 _alterMainCL( _rtnCommand *command,
                             pmdEDUCB *cb,
                             SDB_DPSCB *dpsCB ) ;

         INT32 _analyzeMainCL( _rtnCommand *command ) ;

         INT32 _checkPrimaryStatus() ;

         INT32 _checkRollbackStatus() ;

         INT32 _checkReplStatus() ;

         INT32 _checkClusterActive( MsgHeader *msg ) ;

         void  _login() ;

      protected:
         _clsReplicateSet       *_pReplSet ;
         _clsShardMgr           *_pShdMgr ;
         _clsCatalogAgent       *_pCatAgent ;
         _clsFreezingWindow     *_pFreezingWindow ;
         _SDB_DMSCB             *_pDmsCB ;
         _SDB_RTNCB             *_pRtnCB ;
         _dpsLogWrapper         *_pDpsCB ;

         MsgOpReply             _replyHeader ;
         MsgRouteID             _primaryID ;
         BSONObj                _errorInfo ;
         const CHAR             *_pCollectionName ;
         std::string             _cmdCollectionName ;

         BOOLEAN                _isMainCL ;
         BOOLEAN                _hasUpdateCataInfo ;

         ossTimestamp           _lastRecvTime ;

         CHAR                   _detailName[SESSION_NAME_LEN+1] ;
         BOOLEAN                _logout ;
         BOOLEAN                _delayLogin ;
         string                 _username ;
         string                 _passwd ;
   };

}

#endif //CLS_SHARD_SESSION_HPP_

