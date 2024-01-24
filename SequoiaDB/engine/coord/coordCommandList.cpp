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

   Source File Name = coordCommandList.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05-02-2017  XJH Init
   Last Changed =

*******************************************************************************/
#include "coordCommandList.hpp"
#include "msgMessage.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtnContextDump.hpp"
#include "catDef.hpp"
#include "authDef.hpp"
#include "coordQueryOperator.hpp"
#include "coordQueryLobOperator.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "catGTSDef.hpp"
#include "coordUtil.hpp"
#include "coordCommandStat.hpp"

using namespace bson ;

namespace engine
{

   #define COORD_EMPTY_AGGR_CONTEXT       NULL

   /*
      _coordCMDListIntrBase implement
   */
   _coordCMDListIntrBase::_coordCMDListIntrBase()
   {
   }

   _coordCMDListIntrBase::~_coordCMDListIntrBase()
   {
   }

   /*
      _coordCMDListCurIntrBase implement
   */
   _coordCMDListCurIntrBase::_coordCMDListCurIntrBase()
   {
   }

   _coordCMDListCurIntrBase::~_coordCMDListCurIntrBase()
   {
   }

   /*
      _coordListTransCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordListTransCurIntr,
                                      CMD_NAME_LIST_TRANSCUR_INTR,
                                      TRUE ) ;
   _coordListTransCurIntr::_coordListTransCurIntr()
   {
   }

   _coordListTransCurIntr::~_coordListTransCurIntr()
   {
   }

   void _coordListTransCurIntr::_preSet( pmdEDUCB *cb,
                                         coordCtrlParam &ctrlParam )
   {
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      ctrlParam._useSpecialNode = TRUE ;
      _groupSession.getPropSite()->dumpTransNode( ctrlParam._specialNodes ) ;
   }

   /*
      _coordListTransIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordListTransIntr,
                                      CMD_NAME_LIST_TRANS_INTR,
                                      TRUE ) ;
   _coordListTransIntr::_coordListTransIntr()
   {
   }

   _coordListTransIntr::~_coordListTransIntr()
   {
   }

   void _coordListTransIntr::_preSet( pmdEDUCB *cb,
                                        coordCtrlParam &ctrlParam )
   {
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;
   }

   /*
      _coordListTransCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordListTransCur,
                                      CMD_NAME_LIST_TRANSACTIONS_CUR,
                                      TRUE ) ;
   _coordListTransCur::_coordListTransCur()
   {
   }

   _coordListTransCur::~_coordListTransCur()
   {
   }

   const CHAR* _coordListTransCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_TRANSCUR_INTR ;
   }

   const CHAR* _coordListTransCur::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordListTrans implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordListTrans,
                                      CMD_NAME_LIST_TRANSACTIONS,
                                      TRUE ) ;
   _coordListTrans::_coordListTrans()
   {
   }

   _coordListTrans::~_coordListTrans()
   {
   }

   const CHAR* _coordListTrans::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_TRANS_INTR ;
   }

   const CHAR* _coordListTrans::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordListBackupIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordListBackupIntr,
                                      CMD_NAME_LIST_BACKUP_INTR,
                                      TRUE ) ;
   _coordListBackupIntr::_coordListBackupIntr()
   {
   }

   _coordListBackupIntr::~_coordListBackupIntr()
   {
   }

   void _coordListBackupIntr::_preSet( pmdEDUCB *cb,
                                       coordCtrlParam &ctrlParam )
   {
      ctrlParam._filterID = FILTER_ID_HINT ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
   }

   /*
      _coordListBackup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordListBackup,
                                      CMD_NAME_LIST_BACKUPS,
                                      TRUE ) ;
   _coordListBackup::_coordListBackup()
   {
   }

   _coordListBackup::~_coordListBackup()
   {
   }

   const CHAR* _coordListBackup::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_BACKUP_INTR ;
   }

   const CHAR* _coordListBackup::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordCMDListGroups implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListGroups,
                                      CMD_NAME_LIST_GROUPS,
                                      TRUE ) ;
   _coordCMDListGroups::_coordCMDListGroups()
   {
   }

   _coordCMDListGroups::~_coordCMDListGroups()
   {
   }

   INT32 _coordCMDListGroups::_preProcess( rtnQueryOptions &queryOpt,
                                           string &clName,
                                           BSONObj &outSelector )
   {
      clName = CAT_NODE_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCmdListGroupIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListGroupIntr,
                                      CMD_NAME_LIST_GROUP_INTR,
                                      TRUE ) ;
   _coordCmdListGroupIntr::_coordCmdListGroupIntr()
   {
   }

   _coordCmdListGroupIntr::~_coordCmdListGroupIntr()
   {
   }

   /*
      _coordCMDListCollectionSpace implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListCollectionSpace,
                                      CMD_NAME_LIST_COLLECTIONSPACES,
                                      TRUE ) ;
   _coordCMDListCollectionSpace::_coordCMDListCollectionSpace()
   {
   }

   _coordCMDListCollectionSpace::~_coordCMDListCollectionSpace()
   {
   }

   INT32 _coordCMDListCollectionSpace::_preProcess( rtnQueryOptions &queryOpt,
                                                    string &clName,
                                                    BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      builder.appendNull( CAT_COLLECTION_SPACE_NAME ) ;
      outSelector = queryOpt.getSelector() ;
      queryOpt.setSelector( builder.obj() ) ;
      return SDB_OK ;
   }

   /*
      _coordCMDListCollectionSpaceIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListCollectionSpaceIntr,
                                      CMD_NAME_LIST_SPACE_INTR,
                                      TRUE ) ;
   _coordCMDListCollectionSpaceIntr::_coordCMDListCollectionSpaceIntr()
   {
   }

   _coordCMDListCollectionSpaceIntr::~_coordCMDListCollectionSpaceIntr()
   {
   }

   INT32 _coordCMDListCollectionSpaceIntr::_preProcess( rtnQueryOptions &queryOpt,
                                                        string &clName,
                                                        BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDListCollection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListCollection,
                                      CMD_NAME_LIST_COLLECTIONS,
                                      TRUE ) ;
   _coordCMDListCollection::_coordCMDListCollection()
   {
   }

   _coordCMDListCollection::~_coordCMDListCollection()
   {
   }

   INT32 checkPrivilegesForListCollections(const BSONObj &query)
   {
      INT32 rc = SDB_OK;
      ISession *session = sdbGetThreadExecutor()->getSession();
      if ( !session )
      {
         PD_LOG( PDERROR, "Failed to get session" );
         rc = SDB_SYS;
         goto error;
      }
      if ( !session->privilegeCheckEnabled() )
      {
         goto done;
      }

      try
      {
         if ( query.isEmpty() )
         {
            authActionSet actions;
            actions.addAction( ACTION_TYPE_list );
            rc = session->checkPrivilegesForActionsOnCluster( actions );
            PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );
         }
         else
         {
            BSONElement e = query.getField( FIELD_NAME_NAME );
            if ( Object != e.type() )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "Invalid argument" );
               goto error;
            }
            BSONObj obj = e.Obj();
            BSONElement gtEle = obj.getField( "$gt" );
            BSONElement ltEle = obj.getField( "$lt" );
            if ( String != gtEle.type() || String != ltEle.type() )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "Invalid argument" );
               goto error;
            }
            UINT32 len = ossStrlen( gtEle.valuestr() );
            if ( len < 2 )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "Invalid argument" );
               goto error;
            }
            if ( 0 == ossStrncmp( gtEle.valuestr(), ltEle.valuestr(), len - 1 ) )
            {
               boost::shared_ptr< const authAccessControlList > acl;
               rc = session->getACL( acl );
               PD_RC_CHECK( rc, PDERROR, "Failed to get acl" );
               ossPoolString csName( gtEle.valuestr(), len - 1 );
               boost::shared_ptr< authResource > res = authResource::forCS( csName );
               if ( !res )
               {
                  rc = SDB_OOM;
                  PD_LOG( PDERROR, "Failed to allocate memory" );
                  goto error;
               }
               authActionSet actions1;
               actions1.addAction( ACTION_TYPE_find );
               authActionSet actions2;
               actions2.addAction( ACTION_TYPE_getDetail );
               authActionSet actions3;
               actions3.addAction( ACTION_TYPE_listCollections );
               authActionSet actions4;
               actions4.addAction( ACTION_TYPE_list );

               if ( !acl->isAuthorizedForActionsOnResource( *res, actions1 ) &&
                    !acl->isAuthorizedForActionsOnResource( *res, actions2 ) &&
                    !acl->isAuthorizedForActionsOnResource( *res, actions3 ) &&
                    !acl->isAuthorizedForActionsOnResource( *authResource::forCluster(),
                                                            actions4 ) )
               {
                  rc = SDB_NO_PRIVILEGES;
                  PD_LOG_MSG( PDERROR,
                              "No privilege to execute command: %s, need at least one in actions "
                              "[%s, %s, %s] on collectionspace [%s] or action [%s] on cluster",
                              CMD_NAME_LIST_COLLECTIONS,
                              authActionTypeSerializer( ACTION_TYPE_find ),
                              authActionTypeSerializer( ACTION_TYPE_getDetail ),
                              authActionTypeSerializer( ACTION_TYPE_listCollections ),
                              csName.c_str(), authActionTypeSerializer( ACTION_TYPE_list ) );
                  goto error;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Occur exception, %s, rc: %d", e.what() );
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _coordCMDListCollection::_preProcess( rtnQueryOptions &queryOpt,
                                               string & clName,
                                               BSONObj &outSelector )
   {
      INT32 rc = SDB_OK;
      BSONObjBuilder builder ;
      BSONElement ele ;

      rc = checkPrivilegesForListCollections( queryOpt.getQuery() );
      PD_RC_CHECK( rc, PDERROR, "Failed to check privileges" );

      clName = CAT_COLLECTION_INFO_COLLECTION ;
      outSelector = queryOpt.getSelector() ;

      builder.appendNull( CAT_COLLECTION_NAME ) ;
      ele = queryOpt.getSelector().getField( FIELD_NAME_VERSION ) ;
      if( EOO != ele.type() )
      {
         builder.appendNull( FIELD_NAME_VERSION ) ;
      }
      queryOpt.setSelector( builder.obj() ) ;
      
   done:
      return rc;
   error:
      goto done;
   }

   /*
      _coordCMDListCollectionIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListCollectionIntr,
                                      CMD_NAME_LIST_COLLECTION_INTR,
                                      TRUE ) ;
   _coordCMDListCollectionIntr::_coordCMDListCollectionIntr()
   {
   }

   _coordCMDListCollectionIntr::~_coordCMDListCollectionIntr()
   {
   }

   INT32 _coordCMDListCollectionIntr::_preProcess( rtnQueryOptions &queryOpt,
                                                   string & clName,
                                                   BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDListSequences implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListSequences,
                                      CMD_NAME_LIST_SEQUENCES,
                                      TRUE ) ;
   _coordCMDListSequences::_coordCMDListSequences()
   {
   }

   _coordCMDListSequences::~_coordCMDListSequences()
   {
   }

   INT32 _coordCMDListSequences::_preProcess( rtnQueryOptions &queryOpt,
                                               string & clName,
                                               BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = GTS_SEQUENCE_COLLECTION_NAME ;
      builder.appendNull( CAT_SEQUENCE_NAME ) ;
      outSelector = queryOpt.getSelector() ;
      queryOpt.setSelector( builder.obj() ) ;
      return SDB_OK ;
   }

   /*
      _coordCMDListSequencesIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListSequencesIntr,
                                      CMD_NAME_LIST_SEQUENCES_INTR,
                                      TRUE ) ;
   _coordCMDListSequencesIntr::_coordCMDListSequencesIntr()
   {
   }

   _coordCMDListSequencesIntr::~_coordCMDListSequencesIntr()
   {
   }

   /*
      _coordCMDListContexts implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListContexts,
                                      CMD_NAME_LIST_CONTEXTS,
                                      TRUE ) ;
   _coordCMDListContexts::_coordCMDListContexts()
   {
   }

   _coordCMDListContexts::~_coordCMDListContexts()
   {
   }

   const CHAR* _coordCMDListContexts::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_CONTEXT_INTR ;
   }

   const CHAR* _coordCMDListContexts::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordCmdListContextIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListContextIntr,
                                      CMD_NAME_LIST_CONTEXT_INTR,
                                      TRUE ) ;
   _coordCmdListContextIntr::_coordCmdListContextIntr()
   {
   }

   _coordCmdListContextIntr::~_coordCmdListContextIntr()
   {
   }

   /*
      _coordCMDListContextsCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListContextsCur,
                                      CMD_NAME_LIST_CONTEXTS_CURRENT,
                                      TRUE ) ;
   _coordCMDListContextsCur::_coordCMDListContextsCur()
   {
   }

   _coordCMDListContextsCur::~_coordCMDListContextsCur()
   {
   }

   const CHAR* _coordCMDListContextsCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_CONTEXTCUR_INTR ;
   }

   const CHAR* _coordCMDListContextsCur::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordCmdListContextCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListContextCurIntr,
                                      CMD_NAME_LIST_CONTEXTCUR_INTR,
                                      TRUE ) ;
   _coordCmdListContextCurIntr::_coordCmdListContextCurIntr()
   {
   }

   _coordCmdListContextCurIntr::~_coordCmdListContextCurIntr()
   {
   }

   /*
      _coordCMDListSessions implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListSessions,
                                      CMD_NAME_LIST_SESSIONS,
                                      TRUE ) ;
   _coordCMDListSessions::_coordCMDListSessions()
   {
   }

   _coordCMDListSessions::~_coordCMDListSessions()
   {
   }

   const CHAR* _coordCMDListSessions::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_SESSION_INTR ;
   }

   const CHAR* _coordCMDListSessions::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordCmdListSessionIntr impelment
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListSessionIntr,
                                      CMD_NAME_LIST_SESSION_INTR,
                                      TRUE ) ;
   _coordCmdListSessionIntr::_coordCmdListSessionIntr()
   {
   }

   _coordCmdListSessionIntr::~_coordCmdListSessionIntr()
   {
   }

   /*
      _coordCMDListSessionsCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListSessionsCur,
                                      CMD_NAME_LIST_SESSIONS_CURRENT,
                                      TRUE ) ;
   _coordCMDListSessionsCur::_coordCMDListSessionsCur()
   {
   }

   _coordCMDListSessionsCur::~_coordCMDListSessionsCur()
   {
   }

   const CHAR* _coordCMDListSessionsCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_SESSIONCUR_INTR ;
   }

   const CHAR* _coordCMDListSessionsCur::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordCmdListSessionCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListSessionCurIntr,
                                      CMD_NAME_LIST_SESSIONCUR_INTR,
                                      TRUE ) ;
   _coordCmdListSessionCurIntr::_coordCmdListSessionCurIntr()
   {
   }

   _coordCmdListSessionCurIntr::~_coordCmdListSessionCurIntr()
   {
   }

   /*
      _coordCMDListUser implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListUser,
                                      CMD_NAME_LIST_USERS,
                                      TRUE ) ;
   _coordCMDListUser::_coordCMDListUser()
   {
   }

   _coordCMDListUser::~_coordCMDListUser()
   {
   }

   INT32 _coordCMDListUser::_preProcess( rtnQueryOptions &queryOpt,
                                         string & clName,
                                         BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = AUTH_USR_COLLECTION ;

      BSONObjBuilder passBuilder( builder.subobjStart( SDB_AUTH_PASSWD ) ) ;
      passBuilder.append( "$include", 0 ) ;
      passBuilder.done() ;

      BSONObjBuilder idBuilder( builder.subobjStart( DMS_ID_KEY_NAME ) ) ;
      idBuilder.append( "$include", 0 ) ;
      idBuilder.done() ;

      BSONObjBuilder scramBuilder( builder.subobjStart( SDB_AUTH_SCRAMSHA256 ) ) ;
      scramBuilder.append( "$include", 0 ) ;
      scramBuilder.done() ;

      BSONObjBuilder scram1Builder( builder.subobjStart( SDB_AUTH_SCRAMSHA1 ) ) ;
      scram1Builder.append( "$include", 0 ) ;
      scram1Builder.done() ;

      outSelector = queryOpt.getSelector() ;
      queryOpt.setSelector( builder.obj() ) ;
      return SDB_OK ;
   }

   /*
      _coordCmdListUserIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListUserIntr,
                                      CMD_NAME_LIST_USER_INTR,
                                      TRUE ) ;
   _coordCmdListUserIntr::_coordCmdListUserIntr()
   {
   }

   _coordCmdListUserIntr::~_coordCmdListUserIntr()
   {
   }

   /*
      _coordCmdListTask implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListTask,
                                      CMD_NAME_LIST_TASKS,
                                      TRUE ) ;
   _coordCmdListTask::_coordCmdListTask()
   {
   }

   _coordCmdListTask::~_coordCmdListTask()
   {
   }

   INT32 _coordCmdListTask::_preProcess( rtnQueryOptions &queryOpt,
                                         string &clName,
                                         BSONObj &outSelector )
   {
      clName = CAT_TASK_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCmdListTaskIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListTaskIntr,
                                      CMD_NAME_LIST_TASK_INTR,
                                      TRUE ) ;
   _coordCmdListTaskIntr::_coordCmdListTaskIntr()
   {
   }

   _coordCmdListTaskIntr::~_coordCmdListTaskIntr()
   {
   }

   /*
      _coordCmdListIndexes implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListIndexes,
                                      CMD_NAME_LIST_INDEXES,
                                      TRUE ) ;
   _coordCmdListIndexes::_coordCmdListIndexes()
   {
   }

   _coordCmdListIndexes::~_coordCmdListIndexes()
   {
   }

   INT32 _coordCmdListIndexes::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;

      rc = _coordCMDQueryBase::execute( pMsg, cb, contextID, buf ) ;
      if ( SDB_DMS_NOTEXIST == rc )
      {
         // If catalog is old version, it doesn't has SYSINDEXES collection,
         // it will return -23, we should try to list index by old command.
         if ( buf )
         {
            buf->release() ;
         }

         coordCMDGetIndexesOldVersion listIndexOld ;
         rc = listIndexOld.init( _pResource, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init get index command, rc: %d",
                    rc ) ;
            goto error ;
         }

         rc = listIndexOld.execute( pMsg, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to list indexes by old command, rc: %d",
                    rc ) ;
            goto error ;
         }
      }
      else if ( rc )
      {

         PD_LOG( PDERROR, "Failed to list indexes, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORDLISTIDX_PREPCS, "_coordCmdListIndexes::_preProcess" )
   INT32 _coordCmdListIndexes::_preProcess( rtnQueryOptions &queryOpt,
                                            string &clName,
                                            BSONObj &outSelector )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORDLISTIDX_PREPCS ) ;
      const CHAR* collection = NULL ;
      BOOLEAN isDataSourceCL = FALSE ;
      BOOLEAN isHighErrLevel = FALSE ;
      clName = CAT_INDEX_INFO_COLLECTION ;

      try
      {
         /**
          * Client msg: Matcher: { IndexDef.name: 'a' }
          *             Hint:    { Collection: 'cs.cl' } ==>
          * Coord msg : Matcher: { IndexDef.name: 'a', Collection: 'cs.cl' }
          *             Hint:    {}
          */
         const BSONObj& matcher = queryOpt.getQuery() ;
         const BSONObj& hint    = queryOpt.getHint() ;
         BSONElement ele ;

         if ( !matcher.hasField( FIELD_NAME_COLLECTION ) )
         {
            // reset matcher by hint's collection name
            ele = hint.getField( FIELD_NAME_COLLECTION ) ;
            if ( EOO != ele.type() )
            {
               collection = ele.valuestrsafe() ;

               BSONObjBuilder builder ;
               builder.appendElements( matcher ) ;
               builder.appendAs( ele, FIELD_NAME_COLLECTION ) ;
               queryOpt.setQuery( builder.obj() ) ;
            }
         }

         queryOpt.setHint( BSONObj() ) ;

         outSelector = queryOpt.getSelector() ;
         queryOpt.setSelector( BSON( DMS_ID_KEY_NAME <<
                                     BSON( "$include" << 0 ) <<
                                     FIELD_NAME_COLLECTION <<
                                     BSON( "$include" << 0 ) <<
                                     FIELD_NAME_CL_UNIQUEID <<
                                     BSON( "$include" << 0 ) <<
                                     FIELD_NAME_NAME <<
                                     BSON( "$include" << 0 ) ) ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
      }

      // If it is data source collection, ignore it or report error
      if ( collection && collection[0] != 0 )
      {
         rc = coordGetCLDataSource( collection, pmdGetThreadEDUCB(), _pResource,
                                    isDataSourceCL, isHighErrLevel ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get collection[%s]'s data source, rc: %d",
                      collection, rc ) ;
         if ( isDataSourceCL & isHighErrLevel )
         {
            rc = SDB_OPERATION_INCOMPATIBLE ;
            PD_LOG_MSG( PDERROR, "The collection[%s] mapped to a data source "
                        "can't do %s", collection, getName() ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( COORDLISTIDX_PREPCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdListIndexesIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListIndexesIntr,
                                      CMD_NAME_LIST_INDEXES_INTR,
                                      TRUE ) ;
   _coordCmdListIndexesIntr::_coordCmdListIndexesIntr()
   {
   }

   _coordCmdListIndexesIntr::~_coordCmdListIndexesIntr()
   {
   }

   INT32 _coordCmdListIndexesIntr::_preProcess( rtnQueryOptions &queryOpt,
                                                string &clName,
                                                BSONObj &outSelector )
   {
      clName = CAT_INDEX_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDListProcedures implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListProcedures,
                                      CMD_NAME_LIST_PROCEDURES,
                                      TRUE ) ;
   _coordCMDListProcedures::_coordCMDListProcedures()
   {
   }

   _coordCMDListProcedures::~_coordCMDListProcedures()
   {
   }

   INT32 _coordCMDListProcedures::_preProcess( rtnQueryOptions &queryOpt,
                                               string &clName,
                                               BSONObj &outSelector )
   {
      clName = CAT_PROCEDURES_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDListDomains implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListDomains,
                                      CMD_NAME_LIST_DOMAINS,
                                      TRUE ) ;
   _coordCMDListDomains::_coordCMDListDomains()
   {
   }

   _coordCMDListDomains::~_coordCMDListDomains()
   {
   }

   INT32 _coordCMDListDomains::_preProcess( rtnQueryOptions &queryOpt,
                                            string &clName,
                                            BSONObj &outSelector )
   {
      clName = CAT_DOMAIN_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCmdListDomainIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdListDomainIntr,
                                      CMD_NAME_LIST_DOMAIN_INTR,
                                      TRUE ) ;
   _coordCmdListDomainIntr::_coordCmdListDomainIntr()
   {
   }

   _coordCmdListDomainIntr::~_coordCmdListDomainIntr()
   {
   }

   /*
      _coordCMDListCSInDomain implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListCSInDomain,
                                      CMD_NAME_LIST_CS_IN_DOMAIN,
                                      TRUE ) ;
   _coordCMDListCSInDomain::_coordCMDListCSInDomain()
   {
   }

   _coordCMDListCSInDomain::~_coordCMDListCSInDomain()
   {
   }

   INT32 _coordCMDListCSInDomain::_preProcess( rtnQueryOptions &queryOpt,
                                               string &clName,
                                               BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_SPACE_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDListCLInDomain implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListCLInDomain,
                                      CMD_NAME_LIST_CL_IN_DOMAIN,
                                      TRUE ) ;
   _coordCMDListCLInDomain::_coordCMDListCLInDomain()
   {
   }

   _coordCMDListCLInDomain::~_coordCMDListCLInDomain()
   {
   }

   INT32 _coordCMDListCLInDomain::execute( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           INT64 &contextID,
                                           rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BSONObj conObj ;
      BSONObj dummy ;
      const CHAR *query = NULL ;
      BSONElement domain ;
      rtnQueryOptions queryOptions ;
      vector<BSONObj> replyFromCata ;
      contextID = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &query,
                            NULL, NULL, NULL );
      if ( rc != SDB_OK )
      {
         PD_LOG ( PDERROR, "failed to parse query request(rc=%d)", rc ) ;
         goto error ;
      }

      try
      {
         conObj = BSONObj( query ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      domain = conObj.getField( FIELD_NAME_DOMAIN ) ;
      if ( String != domain.type() )
      {
         PD_LOG( PDERROR, "invalid domain field in object:%s",
                  conObj.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      queryOptions.setQuery( BSON( CAT_DOMAIN_NAME << domain.valuestr() ) ) ;
      queryOptions.setCLFullName( CAT_COLLECTION_SPACE_COLLECTION ) ;

      rc = queryOnCataAndPushToVec( queryOptions, cb, replyFromCata,
                                    buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to execute query on catalog:%d", rc ) ;
         goto error ;
      }

      /// build result
      rc = _rebuildListResult( replyFromCata, cb, contextID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to rebuild list result:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( contextID >= 0 )
      {
         pmdGetKRCB()->getRTNCB()->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   INT32 _coordCMDListCLInDomain::_rebuildListResult(
                                    const vector<BSONObj> &infoFromCata,
                                    pmdEDUCB *cb,
                                    SINT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      rtnContextDump::sharePtr context ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP,
                              context,
                              contextID,
                              cb ) ;
      if  ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create new context:%d", rc ) ;
         goto error ;
      }

      rc = context->open( BSONObj(), BSONObj() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open context:%d", rc ) ;
         goto error ;
      }

      for ( vector<BSONObj>::const_iterator itr = infoFromCata.begin();
            itr != infoFromCata.end();
            itr++ )
      {
         BSONElement cl ;
         BSONElement cs = itr->getField( FIELD_NAME_NAME ) ;
         if ( String != cs.type() )
         {
            PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         cl = itr->getField( FIELD_NAME_COLLECTION ) ;
         if ( Array != cl.type() )
         {
            PD_LOG( PDERROR, "invalid collection space info:%s",
                    itr->toString().c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            BSONObjIterator clItr( cl.embeddedObject() ) ;
            while ( clItr.more() )
            {
               stringstream ss ;
               BSONElement clName ;
               BSONElement oneCl = clItr.next() ;
               if ( Object != oneCl.type() )
               {
                  PD_LOG( PDERROR, "invalid collection space info:%s",
                          itr->toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               clName = oneCl.embeddedObject().getField( FIELD_NAME_NAME ) ;
               if ( String != clName.type() )
               {
                  PD_LOG( PDERROR, "invalid collection space info: %s",
                          itr->toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               ss << cs.valuestr() << "." << clName.valuestr() ;
               context->append( BSON( FIELD_NAME_NAME << ss.str() ) ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

   /*
      _coordCMDListLobs implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListLobs,
                                      CMD_NAME_LIST_LOBS,
                                      TRUE ) ;
   _coordCMDListLobs::_coordCMDListLobs()
   {
   }

   _coordCMDListLobs::~_coordCMDListLobs()
   {
   }

   INT32 _coordCMDListLobs::execute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     INT64 &contextID,
                                     rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      SDB_RTNCB *pRtncb = pmdGetKRCB()->getRTNCB() ;

      const CHAR *pQuery = NULL ;
      BSONObj query ;
      const CHAR *pHint = NULL ;
      BSONObj hint ;
      const CHAR *pCollectionName = NULL ;
      INT32 flag ;
      CHAR *pNewMsg = NULL ;
      INT32 bufferSize = 0 ;

      rtnContextCoord::sharePtr context ;
      coordQueryLobOperator queryOpr ;
      coordQueryConf queryConf ;
      coordSendOptions sendOpt ;
      monAppCB *monAppCB = cb ? cb->getMonAppCB() : NULL ;

      contextID = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCollectionName,
                            NULL, NULL, &pQuery,
                            NULL, NULL, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Parse message failed, rc: %d", rc ) ;

      try
      {
         query = BSONObj( pQuery ) ;
         hint = BSONObj( pHint ) ;
         BSONElement ele = hint.getField( FIELD_NAME_COLLECTION ) ;
         if ( String == ele.type() )
         {
            // new version message, collection is in hint field
            queryConf._realCLName = ele.valuestr() ;
         }
         else
         {
            BSONObj dummy ;
            BSONObj newQuery ;
            // old version message, collection is in query field
            ele = query.getField( FIELD_NAME_COLLECTION ) ;
            if ( String != ele.type() )
            {
               PD_LOG( PDERROR, "invalid obj of list lob:%s",
                       query.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            queryConf._realCLName = ele.valuestr() ;
            hint = query ;
            rc = msgBuildQueryMsg( &pNewMsg, &bufferSize, pCollectionName, flag,
                                   pMsg->requestID, 0, -1, &dummy, &dummy,
                                   &dummy, &hint, cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build new message:rc=%d",
                         rc ) ;
            pMsg = (MsgHeader *)pNewMsg ;
            queryConf._allCataGroups = TRUE ;
            queryConf._openEmptyContext = TRUE ;
         }

      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = queryOpr.init( _pResource, cb, getTimeout() ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init query operator failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = queryOpr.queryOrDoOnCL( pMsg, cb, &context,
                                   sendOpt, &queryConf, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "List lobs[%s] on groups failed, rc: %d",
                   queryConf._realCLName.c_str(), rc ) ;

      // set context id
      contextID = context->contextID() ;

      DMS_MON_OP_COUNT_INC( monAppCB, MON_LOB_LIST, 1 ) ;

   done:
      if ( NULL != pNewMsg )
      {
         msgReleaseBuffer( pNewMsg, cb ) ;
      }
      return rc ;
   error:
      if ( context )
      {
         pRtncb->contextDelete( context->contextID(), cb ) ;
      }
      goto done ;
   }

   /*
      _coordCMDListSvcTasks implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListSvcTasks,
                                      CMD_NAME_LIST_SVCTASKS,
                                      TRUE ) ;

   _coordCMDListSvcTasks::_coordCMDListSvcTasks()
   {
   }

   _coordCMDListSvcTasks::~_coordCMDListSvcTasks()
   {
   }

   const CHAR* _coordCMDListSvcTasks::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_LIST_SVCTASKS_INTR ;
   }

   const CHAR* _coordCMDListSvcTasks::getInnerAggrContent()
   {
      return COORD_EMPTY_AGGR_CONTEXT ;
   }

   /*
      _coordCMDListSvcTasksIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListSvcTasksIntr,
                                      CMD_NAME_LIST_SVCTASKS_INTR,
                                      TRUE ) ;

   _coordCMDListSvcTasksIntr::_coordCMDListSvcTasksIntr()
   {
   }

   _coordCMDListSvcTasksIntr::~_coordCMDListSvcTasksIntr()
   {
   }

   void _coordCMDListSvcTasksIntr::_preSet( pmdEDUCB *cb,
                                            coordCtrlParam &ctrlParam )
   {
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
   }

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListDataSources,
                                      CMD_NAME_LIST_DATASOURCES,
                                      TRUE ) ;
   _coordCMDListDataSources::_coordCMDListDataSources()
   {
   }

   _coordCMDListDataSources::~_coordCMDListDataSources()
   {
   }

   INT32 _coordCMDListDataSources::_preProcess( rtnQueryOptions &queryOpt,
                                                string &clName,
                                                BSONObj &outSelector )
   {
      clName = CAT_DATASOURCE_COLLECTION ;
      return SDB_OK ;
   }

   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListDataSourceIntr,
                                      CMD_NAME_LIST_DATASOURCE_INTR,
                                      TRUE ) ;
   _coordCMDListDataSourceIntr::_coordCMDListDataSourceIntr()
   {
   }

   _coordCMDListDataSourceIntr::~_coordCMDListDataSourceIntr()
   {
   }

   /*
      _coordCMDListRecycleBin implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListRecycleBin,
                                      CMD_NAME_LIST_RECYCLEBIN,
                                      TRUE ) ;
   _coordCMDListRecycleBin::_coordCMDListRecycleBin()
   {
   }

   _coordCMDListRecycleBin::~_coordCMDListRecycleBin()
   {
   }

   INT32 _coordCMDListRecycleBin::_preProcess( rtnQueryOptions &queryOpt,
                                               string &clName,
                                               BSONObj &outSelector )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         builder.appendNull( FIELD_NAME_RECYCLE_NAME ) ;
         builder.appendNull( FIELD_NAME_RECYCLE_ID ) ;
         builder.appendNull( FIELD_NAME_ORIGIN_NAME ) ;
         builder.appendNull( FIELD_NAME_ORIGIN_ID ) ;
         builder.appendNull( FIELD_NAME_TYPE ) ;
         builder.appendNull( FIELD_NAME_OPTYPE ) ;
         builder.appendNull( FIELD_NAME_RECYCLE_TIME ) ;

         outSelector = queryOpt.getSelector() ;
         queryOpt.setSelector( builder.obj() ) ;

         clName.assign( CAT_SYSRECYCLEBIN_ITEM_COLLECTION ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to pre-process list recycle bin command, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDListRecycleBinIntr implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListRecycleBinIntr,
                                      CMD_NAME_LIST_RECYCLEBIN_INTR,
                                      TRUE ) ;
   _coordCMDListRecycleBinIntr::_coordCMDListRecycleBinIntr()
   {
   }

   _coordCMDListRecycleBinIntr::~_coordCMDListRecycleBinIntr()
   {
   }

   /*
      _coordCMDListGrpModes implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListGrpModes,
                                      CMD_NAME_LIST_GROUPMODES,
                                      TRUE ) ;
   _coordCMDListGrpModes::_coordCMDListGrpModes()
   {
   }

   _coordCMDListGrpModes::~_coordCMDListGrpModes()
   {
   }

   INT32 _coordCMDListGrpModes::_preProcess( rtnQueryOptions &queryOpt,
                                             string &clName,
                                             BSONObj &outSelector )
   {
      clName = CAT_GROUP_MODE_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDListGroupModeIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDListGroupModeIntr,
                                      CMD_NAME_LIST_GROUPMODES_INTR,
                                      TRUE ) ;

   _coordCMDListGroupModeIntr::_coordCMDListGroupModeIntr()
   {
   }

   _coordCMDListGroupModeIntr::~_coordCMDListGroupModeIntr()
   {
   }

}
