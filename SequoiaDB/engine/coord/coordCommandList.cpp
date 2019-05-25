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
#include "pdTrace.hpp"
#include "coordTrace.hpp"

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
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      ctrlParam._useSpecialNode = TRUE ;
      DpsTransNodeMap *pMap = cb->getTransNodeLst() ;
      if ( pMap )
      {
         DpsTransNodeMap::iterator it = pMap->begin() ;
         while( it != pMap->end() )
         {
            ctrlParam._specialNodes.insert( it->second.value ) ;
            ++it ;
         }
      }
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
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
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

   INT32 _coordCMDListCollection::_preProcess( rtnQueryOptions &queryOpt,
                                               string & clName,
                                               BSONObj &outSelector )
   {
      BSONObjBuilder builder ;
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      builder.appendNull( CAT_COLLECTION_NAME ) ;
      outSelector = queryOpt.getSelector() ;
      queryOpt.setSelector( builder.obj() ) ;
      return SDB_OK ;
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
      if ( queryOpt.isSelectorEmpty() )
      {
         builder.appendNull( FIELD_NAME_USER ) ;
      }
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
      CHAR *query = NULL ;
      BSONElement domain ;
      rtnQueryOptions queryOptions ;
      vector<BSONObj> replyFromCata ;
      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
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
      rtnContext *context = NULL ;
      SDB_RTNCB *rtnCB = sdbGetRTNCB() ;

      rc = rtnCB->contextNew( RTN_CONTEXT_DUMP,
                              &context,
                              contextID,
                              cb ) ;
      if  ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to create new context:%d", rc ) ;
         goto error ;
      }

      rc = (( rtnContextDump * )context)->open( BSONObj(), BSONObj() ) ;
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

      CHAR *pQuery = NULL ;
      BSONObj query ;

      rtnContextCoord *context = NULL ;
      coordQueryOperator queryOpr( TRUE ) ;
      coordQueryConf queryConf ;
      coordSendOptions sendOpt ;

      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL ) ;

      PD_RC_CHECK( rc, PDERROR, "Parse message failed, rc: %d", rc ) ;

      try
      {
         query = BSONObj( pQuery ) ;
         BSONElement ele = query.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "invalid obj of list lob:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         queryConf._realCLName = ele.valuestr() ;
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

      queryConf._openEmptyContext = TRUE ;
      queryConf._allCataGroups = TRUE ;
      rc = queryOpr.queryOrDoOnCL( pMsg, cb, &context,
                                   sendOpt, &queryConf, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "List lobs[%s] on groups failed, rc: %d",
                   queryConf._realCLName.c_str(), rc ) ;

      contextID = context->contextID() ;

   done:
      return rc ;
   error:
      if ( context )
      {
         pRtncb->contextDelete( context->contextID(), cb ) ;
      }
      goto done ;
   }

}

