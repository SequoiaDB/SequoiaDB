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

   Source File Name = catCatalogManager.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "pmdCB.hpp"
#include "pd.hpp"
#include "rtn.hpp"
#include "catDef.hpp"
#include "catCatalogManager.hpp"
#include "rtnPredicate.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "catCommon.hpp"
#include "clsCatalogAgent.hpp"
#include "rtnAlterJob.hpp"
#include "catTask.hpp"
#include "catCommand.hpp"
#include "authDef.hpp"

using namespace bson;

namespace engine
{
   /*
      catCatalogueManager implement
   */
   catCatalogueManager::catCatalogueManager()
   {
      _pEduCB     = NULL ;
      _pDpsCB     = NULL ;
      _pCatCB     = NULL ;
      _pDmsCB     = NULL ;
   }

   INT32 catCatalogueManager::active()
   {
      if ( !_pCatCB->isDCReadonly() )
      {
         // WARNING: should not write DPS logs if DC is readonly
      }
      return SDB_OK ;
   }

   INT32 catCatalogueManager::deactive()
   {
      return SDB_OK ;
   }

   INT32 catCatalogueManager::init()
   {
      pmdKRCB *krcb  = pmdGetKRCB();
      _pDmsCB        = krcb->getDMSCB();
      _pDpsCB        = krcb->getDPSCB();
      _pCatCB        = krcb->getCATLOGUECB();

      _pCatCB->regEventHandler( this ) ;

      return SDB_OK ;
   }

   INT32 catCatalogueManager::fini()
   {
      if ( NULL != _pCatCB )
      {
         _pCatCB->unregEventHandler( this ) ;
      }

      return SDB_OK ;
   }

   void catCatalogueManager::attachCB( pmdEDUCB * cb )
   {
      _pEduCB = cb ;
   }

   void catCatalogueManager::detachCB( pmdEDUCB * cb )
   {
      _pEduCB = NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_ONUPGRADE, "catCatalogueManager::onUpgrade")
   INT32 catCatalogueManager::onUpgrade( UINT32 version )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATALOGMGR_ONUPGRADE ) ;

      if ( CATALOG_VERSION_V1 == version )
      {
         rc = _checkAllCSCLUniqueID() ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to check cs/cl unique id, rc: %d", rc ) ;

         rc = _checkTaskHWM() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check task hwm, rc: %d", rc ) ;

         rc = _checkAndUpgradeDSCLInfo() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check and update data source and "
                      "collection information, rc: %d", rc ) ;
      }
      else if ( CATALOG_VERSION_V3 == version )
      {
         rc = _checkAndUpgradeUserRole() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check and upgrade user role, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_ONUPGRADE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CRT_PROCEDURES, "catCatalogueManager::processCmdCrtProcedures")
   INT32 catCatalogueManager::processCmdCrtProcedures( const CHAR *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR_CRT_PROCEDURES ) ;
      try
      {
         BSONObj func( pMsg ) ;
         BSONObj parsed ;
         rc = catPraseFunc( func, parsed ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to parse store procedures:%s",
                    func.toString().c_str() ) ;
            goto error ;
         }

         rc = rtnInsert( CAT_PROCEDURES_COLLECTION,
                         parsed, 1, 0,
                         _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         if ( SDB_OK != rc )
         {
            if( SDB_IXM_DUP_KEY == rc )
            {
               rc = SDB_FMP_FUNC_EXIST ;
               PD_LOG( PDERROR, "The procedure with the same name exists, failed"
                       " to add func: %s, rc: %d",
                       parsed.toString().c_str(), rc ) ;
               goto error ;
            }
            PD_LOG( PDERROR, "failed to add func: %s",
                    parsed.toString().c_str() ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened: %s",e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_CRT_PROCEDURES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_RM_PROCEDURES, "catCatalogueManager::processCmdRmProcedures")
   INT32 catCatalogueManager::processCmdRmProcedures( const CHAR *pMsg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR_RM_PROCEDURES ) ;
      try
      {
         BSONObj obj( pMsg ) ;
         BSONElement name = obj.getField( FIELD_NAME_FUNC ) ;
         if ( name.eoo() || String != name.type() )
         {
            PD_LOG( PDERROR, "invalid type of func name[%s:%d]",
                    name.toString().c_str(), name.type()  ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
         BSONObjBuilder builder ;
         BSONObj deletor ;
         BSONObj dummy ;
         BSONObj func ;
         builder.appendAs( name, FMP_FUNC_NAME ) ;
         deletor = builder.obj() ;

         rc = catGetOneObj( CAT_PROCEDURES_COLLECTION,
                            dummy, deletor, dummy,
                            _pEduCB, func ) ;

         if ( SDB_DMS_EOC == rc )
         {
            PD_LOG( PDERROR, "func %s is not exist",
                    deletor.toString().c_str() ) ;
            rc = SDB_FMP_FUNC_NOT_EXIST ;
            goto error ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get func:%s, rc=%d",
                    deletor.toString().c_str(), rc ) ;
            goto error ;
         }

         rc = rtnDelete( CAT_PROCEDURES_COLLECTION,
                         deletor, BSONObj(),
                         0, _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to rm func:%s",
                    deletor.toString().c_str() ) ;
            goto error ;
         }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR_RM_PROCEDURES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_QUERYSPACEINFO, "catCatalogueManager::processCmdQuerySpaceInfo" )
   INT32 catCatalogueManager::processCmdQuerySpaceInfo( const CHAR * pQuery,
                                                        rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_QUERYSPACEINFO ) ;
      const CHAR *csName = NULL ;
      utilCSUniqueID csUniqueID = UTIL_UNIQUEID_NULL ;
      BOOLEAN includeSubCLGroup = FALSE ;
      BSONObj boSpace ;
      BOOLEAN isExist = FALSE ;
      vector< UINT32 > groups ;
      ossPoolVector< BSONObj > indexVec ;
      BSONObj boQuery ;

      try
      {
         boQuery = BSONObj( pQuery ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = rtnGetIntElement( boQuery, CAT_CS_UNIQUEID,
                             (INT32&)csUniqueID ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = rtnGetStringElement( boQuery, CAT_COLLECTION_SPACE_NAME,
                                   &csName ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s] or field[%s], "
                   "rc: %d", CAT_COLLECTION_SPACE_NAME,
                    CAT_CS_UNIQUEID, rc ) ;

      rc = rtnGetBooleanElement( boQuery, CAT_INCLUDE_SUBCL,
                                 includeSubCLGroup ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         includeSubCLGroup = TRUE ; // default is true
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   CAT_INCLUDE_SUBCL, rc ) ;

      PD_TRACE1 ( SDB_CATALOGMGR_QUERYSPACEINFO, PD_PACK_STRING ( csName ) ) ;

      // check cs exist or not
      rc = catCheckSpaceExist( csName, csUniqueID, isExist, boSpace, _pEduCB ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Check collection space[name: %s, id: %u] exist failed, "
                   "rc: %d", csName, csUniqueID, rc ) ;

      PD_TRACE1 ( SDB_CATALOGMGR_QUERYSPACEINFO,PD_PACK_INT ( isExist ) ) ;
      if ( !isExist )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
         goto error ;
      }

      // get cs name
      if ( NULL == csName )
      {
         rc = rtnGetStringElement( boSpace, CAT_COLLECTION_SPACE_NAME, &csName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_COLLECTION_SPACE_NAME, rc ) ;
      }
      else if ( !UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         rc = rtnGetIntElement( boSpace, CAT_CS_UNIQUEID,
                                (INT32 &)( csUniqueID ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      CAT_CS_UNIQUEID, rc ) ;
      }

      rc = catGetCSGroups( csUniqueID, _pEduCB, TRUE, FALSE, groups ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection space[%s] all groups failed, "
                   "rc: %d", csName, rc ) ;

      // get groups in sub-collections
      if ( includeSubCLGroup )
      {
         ossPoolSet< UINT32 > groupSet ;
         rc = catGetCSSubCLGroups( csName, _pEduCB, groupSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups for sub-collections "
                      "in collection space [%s], rc: %d", csName, rc ) ;

         rc = catSaveToGroupIDList( groupSet, groups ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save group list, rc: %d", rc ) ;
      }

      try
      {
         BSONObjBuilder builder ;
         builder.appendElements( boSpace ) ;
         _pCatCB->makeGroupsObj( builder, groups, TRUE, TRUE ) ;
         ctxBuf = rtnContextBuf( builder.obj() ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_QUERYSPACEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHECKPUREMAPPINGCS, "catCatalogueManager::_checkPureMappingCS" )
   INT32 catCatalogueManager::_checkPureMappingCS( const CHAR *clFullName,
                                                   MsgOpReply *&reply )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__CHECKPUREMAPPINGCS ) ;
      BOOLEAN inMappingCS = FALSE ;
      INT32 buffSize = 0 ;

      try
      {
         BSONObj cataInfo ;
         BSONObj csMetaRecord ;

         rc = catCheckCLInPureMappingCS( clFullName, _pEduCB, inMappingCS,
                                         &csMetaRecord ) ;
         PD_RC_CHECK( rc, PDERROR, "Checking if using mapping cs for "
                      "collection[%s] failed[%d]", clFullName, rc ) ;
         if ( !inMappingCS )
         {
            // Not in a pure mapping cs, it's a normal collection space.
            goto done ;
         }

         rc = catBuildCatalogByPureMappingCS( clFullName, csMetaRecord,
                                              cataInfo, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Build catalog information for "
                                   "collection[%s] failed[%d]", clFullName, rc ) ;

         rc = rtnReallocBuffer( (CHAR **)&reply, &buffSize,
                                sizeof(MsgOpReply) + cataInfo.objsize(),
                                SDB_PAGE_SIZE ) ;
         PD_RC_CHECK( rc, PDERROR, "Reallocate reply buffer failed[%d]",
                      rc ) ;
         ossMemcpy( (CHAR *)reply + sizeof(MsgOpReply),
                    cataInfo.objdata(), cataInfo.objsize() ) ;
         reply->header.messageLength += cataInfo.objsize() ;
         reply->numReturned = 1 ;
         reply->flags = SDB_OK ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__CHECKPUREMAPPINGCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // this function is for catalog collection check
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_QUERYCATALOG, "catCatalogueManager::processQueryCatalogue" )
   INT32 catCatalogueManager::processQueryCatalogue ( const NET_HANDLE &handle,
                                                      MsgHeader *pMsg )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_QUERYCATALOG ) ;
      MsgCatQueryCatReq *pCatReq       = (MsgCatQueryCatReq*)pMsg ;
      MsgOpReply *pReply               = NULL;
      BOOLEAN isDelay                  = FALSE ;

      // primary check
      rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
      if ( isDelay )
      {
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG ( PDWARNING, "service deactive but received query "
                  "catalogue request, rc: %d", rc ) ;
         goto error ;
      }

      // sanity check, header can't be too small
      PD_CHECK ( pCatReq->header.messageLength >=
                 (INT32)sizeof(MsgCatQueryCatReq),
                 SDB_INVALIDARG, error, PDERROR,
                 "received unexpected query catalogue request, "
                 "message length(%d) is invalid",
                 pCatReq->header.messageLength ) ;
      // extract and query
      try
      {
         const CHAR *pCollectionName   = NULL ;
         SINT32 flag                   = 0 ;
         SINT64 numToSkip              = 0 ;
         SINT64 numToReturn            = -1 ;
         const CHAR *pQuery            = NULL ;
         const CHAR *pFieldSelector    = NULL ;
         const CHAR *pOrderBy          = NULL ;
         const CHAR *pHint             = NULL ;
         rc = msgExtractQuery  ( (const CHAR *)pCatReq, &flag, &pCollectionName,
                                 &numToSkip, &numToReturn, &pQuery,
                                 &pFieldSelector, &pOrderBy, &pHint ) ;
         BSONObj matcher(pQuery);
         BSONObj selector(pFieldSelector);
         BSONObj orderBy(pOrderBy);
         BSONObj hint(pHint);
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to extract message, rc = %d", rc ) ;
         // perform catalog query, result buffer will be placed in pReply, and
         // we are responsible to free it by end of the function
         rc = catQueryAndGetMore ( &pReply, CAT_COLLECTION_INFO_COLLECTION,
                                   selector, matcher, orderBy, hint, flag,
                                   _pEduCB, numToSkip, numToReturn ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to query from catalog, rc = %d", rc ) ;

         if ( 0 == ossStrcmp( matcher.firstElementFieldName(),
                              CAT_CATALOGNAME_NAME ) )
         {
            if ( 0 == pReply->numReturned )
            {
               // if returned = 0, it means collection does not exist
               const CHAR* collection = matcher.firstElement().valuestrsafe() ;
               BOOLEAN csExist = FALSE ;

               rc = catCheckCSExist( collection, _pEduCB, csExist ) ;
               if ( csExist )
               {
                  // If the cs exists, and the collection is not found, check if
                  // the cs is using data source.
                  const CHAR *clName = matcher.firstElement().valuestr() ;
                  rc = _checkPureMappingCS( clName, pReply ) ;
                  PD_RC_CHECK( rc, PDERROR, "Check mapping info of "
                               "collection[%s] failed[%d]", clName, rc ) ;
                  if ( 0 == pReply->numReturned )
                  {
                     rc = SDB_DMS_NOTEXIST ;
                     PD_LOG( PDWARNING, "Collection[%s] does not exist, rc: %d",
                             collection, rc ) ;
                     goto error ;
                  }
               }
               else if ( SDB_OK == rc )
               {
                  rc = SDB_DMS_CS_NOTEXIST ;
                  PD_LOG( PDWARNING,
                          "Collection[%s]'s space does not exist, rc: %d",
                          collection, rc ) ;
                  goto error ;
               }
               else
               {
                  rc = SDB_DMS_NOTEXIST ;
                  PD_LOG( PDWARNING, "Collection[%s] does not exist, rc: %d",
                          collection, rc ) ;
                  goto error ;
               }
            }
            else if ( pReply->numReturned > 1 )
            {
               // if returned > 1, it means possible catalog corruption
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDSEVERE, "More than one records returned for query, "
                       "possible catalog corruption, rc: %d", rc ) ;
               goto error ;
            }
         }
         else
         {
            PD_CHECK ( pReply->numReturned >= 1, SDB_DMS_NOTEXIST, error,
                       PDWARNING, "Collection does not exist:%s, rc: %d",
                       matcher.toString().c_str(), rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception during query catalogue request:%s",
                       e.what() ) ;
      }
      pReply->header.opCode        = MSG_CAT_QUERY_CATALOG_RSP ;
      pReply->header.TID           = pCatReq->header.TID ;
      pReply->header.requestID     = pCatReq->header.requestID ;
      pReply->header.globalID      = pCatReq->header.globalID ;
      pReply->header.routeID.value = 0 ;
   done :
      if ( !_pCatCB->isDelayed() )
      {
         if ( SDB_OK == rc && NULL != pReply )
         {
            rc = _pCatCB->sendReply( handle, pReply, rc ) ;
         }
         else
         {
            MsgOpReply replyMsg;
            replyMsg.header.messageLength = sizeof( MsgOpReply );
            replyMsg.header.opCode        = MSG_CAT_QUERY_CATALOG_RSP;
            replyMsg.header.TID           = pCatReq->header.TID;
            replyMsg.header.routeID.value = 0;
            replyMsg.header.requestID     = pCatReq->header.requestID;
            replyMsg.header.globalID      = pCatReq->header.globalID ;
            replyMsg.numReturned          = 0;
            replyMsg.flags                = rc;
            replyMsg.contextID            = -1 ;
            PD_TRACE1 ( SDB_CATALOGMGR_QUERYCATALOG,
                        PD_PACK_INT ( rc ) ) ;
            if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               replyMsg.startFrom = _pCatCB->getPrimaryNode() ;
            }
            rc = _pCatCB->sendReply( handle, &replyMsg, rc ) ;
         }
      }
      if( pReply )
      {
         SDB_OSS_FREE ( pReply );
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_QUERYCATALOG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_QUERYTASK, "catCatalogueManager::processQueryTask" )
   INT32 catCatalogueManager::processQueryTask ( const NET_HANDLE &handle,
                                                 MsgHeader *pMsg )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_QUERYTASK ) ;
      MsgCatQueryTaskReq *pTaskRequest = (MsgCatQueryTaskReq*)pMsg ;
      MsgCatQueryTaskRes *pReply       = NULL ;
      INT32 flag                       = 0 ;
      SINT64 numToSkip                 = 0 ;
      SINT64 numToReturn               = 0 ;
      const CHAR *pQuery               = NULL ;
      const CHAR *pFieldSelector       = NULL ;
      const CHAR *pOrderBy             = NULL ;
      const CHAR *pHint                = NULL ;
      const CHAR *pCollectionName      = NULL ;
      BOOLEAN isDelay                  = FALSE ;

      // primary check
      rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
      if ( isDelay )
      {
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG ( PDWARNING, "service deactive but received query "
                  "catalogue request, rc: %d", rc ) ;
         goto error ;
      }

      // sanity check, the query length should be at least header size
      PD_CHECK ( pTaskRequest->header.messageLength >=
                 (INT32)sizeof(MsgCatQueryTaskReq),
                 SDB_INVALIDARG, error, PDERROR,
                 "received unexpected query task request, "
                 "message length(%d) is invalid",
                 pTaskRequest->header.messageLength ) ;

      try
      {
         // extract the request message
         rc = msgExtractQuery ( (const CHAR*)pTaskRequest, &flag,
                                &pCollectionName,
                                &numToSkip, &numToReturn, &pQuery,
                                &pFieldSelector, &pOrderBy, &pHint ) ;
         BSONObj matcher ( pQuery ) ;
         BSONObj selector ( pFieldSelector ) ;
         BSONObj orderBy ( pOrderBy );
         BSONObj hint ( pHint ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to extract message, rc = %d", rc ) ;
         // pReply will be allocated by catQueryAndGetMore, we are
         // responsible to free the memory
         rc = catQueryAndGetMore ( &pReply, CAT_TASK_INFO_COLLECTION,
                                   selector, matcher, orderBy, hint, flag,
                                   _pEduCB, numToSkip, numToReturn ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to perform query from catalog, rc = %d", rc ) ;

         // If there's no task satisfy the request, let's return SDB_CAT_TASK_NOTFOUND,
         // otherwise return all tasks satisfy the request
         PD_CHECK ( pReply->numReturned >= 1, SDB_CAT_TASK_NOTFOUND, error,
                    PDINFO, "Task[%s] does not exist",
                    matcher.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK ( SDB_SYS, PDERROR,
                       "Exception when extracting query task: %s",
                       e.what() ) ;
      }
      // construct reply header to match the request
      pReply->header.opCode        = MSG_CAT_QUERY_TASK_RSP ;
      pReply->header.TID           = pTaskRequest->header.TID ;
      pReply->header.requestID     = pTaskRequest->header.requestID ;
      pReply->header.routeID.value = 0 ;
      pReply->header.globalID      = pTaskRequest->header.globalID ;

   done :
      if ( !_pCatCB->isDelayed() )
      {
         if ( SDB_OK == rc && pReply )
         {
            rc = _pCatCB->sendReply( handle, pReply, rc ) ;
         }
         else
         {
            // if something wrong happened, return a reply with rc
            MsgOpReply replyMsg;
            replyMsg.header.messageLength = sizeof( MsgOpReply );
            replyMsg.header.opCode        = MSG_CAT_QUERY_TASK_RSP ;
            replyMsg.header.TID           = pTaskRequest->header.TID;
            replyMsg.header.routeID.value = 0;
            replyMsg.header.requestID     = pTaskRequest->header.requestID;
            replyMsg.header.globalID      = pTaskRequest->header.globalID ;
            replyMsg.numReturned          = 0;
            replyMsg.flags                = rc;
            replyMsg.contextID            = -1 ;
            PD_TRACE1 ( SDB_CATALOGMGR_QUERYTASK, PD_PACK_INT ( rc ) ) ;

            if ( SDB_CLS_NOT_PRIMARY == rc )
            {
               replyMsg.startFrom = _pCatCB->getPrimaryNode() ;
            }
            rc = _pCatCB->sendReply( handle, &replyMsg, rc ) ;
         }
      }
      if ( pReply )
      {
         SDB_OSS_FREE ( pReply ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_QUERYTASK, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CREATECS, "catCatalogueManager::processCmdCreateCS" )
   INT32 catCatalogueManager::processCmdCreateCS( const CHAR * pQuery,
                                                  rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CREATECS ) ;
      INT64 contextID = -1 ;

      catCMDCreateCS cmd ;
      rc = cmd.init( pQuery ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = cmd.doit( _pEduCB, ctxBuf, contextID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CREATECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catCatalogueManager::processCmdCreateIndex( const CHAR *pQuery,
                                                     const CHAR *pHint,
                                                     rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      INT64 contextID = -1 ;

      catCMDCreateIndex cmd ;
      rc = cmd.init( pQuery, NULL, NULL, pHint ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = cmd.doit( _pEduCB, ctxBuf, contextID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catCatalogueManager::processCmdDropIndex( const CHAR *pQuery,
                                                   const CHAR *pHint,
                                                   rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      INT64 contextID = -1 ;

      catCMDDropIndex cmd ;
      rc = cmd.init( pQuery, NULL, NULL, pHint ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = cmd.doit( _pEduCB, ctxBuf, contextID ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CMDSPLIT, "catCatalogueManager::processCmdTask" )
   INT32 catCatalogueManager::processCmdTask( const CHAR * pQuery,
                                              INT32 opCode,
                                              rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;
      INT16 w = _majoritySize() ;

      UINT32 returnGroupID = CAT_INVALID_GROUPID ;
      UINT64 taskID = CLS_INVALID_TASKID ;
      INT32 returnVersion = -1 ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CMDSPLIT ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;

         if ( MSG_CAT_SPLIT_CHGMETA_REQ == opCode ||
              MSG_CAT_SPLIT_CLEANUP_REQ == opCode ||
              MSG_CAT_SPLIT_FINISH_REQ == opCode )
         {
            // Get the task ID
            rc = rtnGetNumberLongElement( boQuery, CAT_TASKID_NAME,
                                          (INT64 &)taskID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to process task [%d]: "
                         "Failed to get the field [%s] from query",
                         opCode, CAT_TASKID_NAME ) ;
         }

         // dispatch
         switch ( opCode )
         {
            case MSG_CAT_SPLIT_PREPARE_REQ :
               rc = catSplitPrepare( boQuery, _pEduCB, returnGroupID,
                                     returnVersion ) ;
               break ;
            case MSG_CAT_SPLIT_READY_REQ :
               // Generate task ID
               taskID = assignTaskID() ;
               rc = catSplitReady( boQuery, taskID, TRUE, _pEduCB, w,
                                   returnGroupID, returnVersion ) ;
               break ;
            case MSG_CAT_SPLIT_CHGMETA_REQ :
               rc = catSplitChgMeta( boQuery, taskID, _pEduCB, w ) ;
               break ;
            case MSG_CAT_TASK_START_REQ :
               rc = catTaskStart( boQuery, _pEduCB, w ) ;
               break ;
            case MSG_CAT_SPLIT_CLEANUP_REQ :
               rc = catSplitCleanup( taskID, _pEduCB, w ) ;
               break ;
            case MSG_CAT_SPLIT_FINISH_REQ :
               rc = catSplitFinish( taskID, _pEduCB, w ) ;
               break ;
            case MSG_CAT_TASK_CANCEL_REQ :
               rc = catTaskCancel( boQuery, _pEduCB, w, returnGroupID ) ;
               break ;
            default :
               rc = SDB_INVALIDARG ;
               break ;
         }

         PD_RC_CHECK( rc, PDERROR,
                      "Failed to process task, opCode: %d, rc: %d",
                      opCode, rc ) ;

         // Generate reply message
         if ( CAT_INVALID_GROUPID != returnGroupID )
         {
            BSONObjBuilder replyBuild ;
            vector< UINT32 > vecGroups ;

            vecGroups.push_back( returnGroupID ) ;

            if ( MSG_CAT_SPLIT_PREPARE_REQ == opCode ||
                 MSG_CAT_SPLIT_READY_REQ == opCode )
            {
               replyBuild.append( CAT_CATALOGVERSION_NAME, returnVersion ) ;
            }
            if ( CLS_INVALID_TASKID != taskID )
            {
               replyBuild.append( CAT_TASKID_NAME, (long long)taskID ) ;
            }
            _pCatCB->makeGroupsObj( replyBuild, vecGroups, TRUE ) ;
            ctxBuf = rtnContextBuf( replyBuild.obj() ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occurred exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CMDSPLIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__SETUID, "catCatalogueManager::_setCSCLUniqueID" )
   INT32 catCatalogueManager::_setCSCLUniqueID( string csName,
                                                const BSONObj& boCollections,
                                                UINT32 csUniqueID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__SETUID ) ;

      vector< PAIR_CLNAME_ID > clInfoList ;
      vector< PAIR_CLNAME_ID >::iterator clIt ;
      BSONObj dummyObj ;
      UINT64 clUniqueHWM = ossPack32To64( csUniqueID, 0 ) ;

      // generator cl unique id
      BSONObjIterator iter( boCollections ) ;
      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         string collection ;
         PAIR_CLNAME_ID clPair ;

         rc = rtnGetSTDStringElement( ele.embeddedObject(),
                                      CAT_COLLECTION_NAME,
                                      collection ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s] from [%s], rc: %d",
                      CAT_COLLECTION_NAME, ele.toString().c_str(), rc ) ;

         clPair = std::make_pair( collection, ++clUniqueHWM ) ;
         clInfoList.push_back( clPair ) ;
      }

      // set cl unique id
      for ( clIt = clInfoList.begin() ; clIt != clInfoList.end() ; clIt++ )
      {
         string clFullName = csName + '.' + clIt->first ;
         BSONObj setInfo = BSON( CAT_CL_UNIQUEID << (INT64)clIt->second ) ;
         rc = catUpdateCatalog( clFullName.c_str(), setInfo,
                                dummyObj, _pEduCB, _majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update cl catalog info, rc: %d", rc ) ;
      }

      // set cl list in cs
      rc = catUpdateCSCLs( csName, clInfoList,
                           _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
      PD_RC_CHECK( rc, PDERROR,
                      "Failed to update cl list in cs, rc: %d", rc ) ;

      // set cs unique id
      {
         BSONObj setObject = BSON( CAT_CS_UNIQUEID << csUniqueID <<
                                   CAT_CS_CLUNIQUEHWM << (INT64)clUniqueHWM ) ;
         rc = catUpdateCS( csName.c_str(), setObject, dummyObj,
                           _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update cs catalog info, rc: %d", rc ) ;
      }

   done:
      if ( SDB_OK == rc )
      {
         PD_LOG( PDEVENT,
                 "Set unique id, cs name: %s, cs id: %u, cl info: %s",
                 csName.c_str(), csUniqueID,
                 utilClNameId2Str( clInfoList ).c_str() ) ;
      }
      PD_TRACE_EXITRC( SDB_CATALOGMGR__SETUID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catCatalogueManager::_checkTaskHWM()
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj dummyObj, resultObj ;
      UINT64 curTaskHWM = 0 ;

      PD_LOG( PDDEBUG, "Begin checkTaskHWM" );

      // If [TaskHWM] field doesn't exist, set it.
      rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummyObj, matcher,
                         dummyObj, _pEduCB, resultObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query from %s, rc: %d",
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      rc = rtnGetNumberLongElement( resultObj, FIELD_NAME_TASKHWM,
                                    (INT64&)curTaskHWM ) ;
      if ( SDB_OK == rc )
      {
         goto done ;
      }
      else if ( SDB_FIELD_NOT_EXIST == rc )
      {
         // set task hwm
         UINT64 taskHWM = catGetCurrentMaxTaskID( _pEduCB ) ;

         rc = catSetTaskHWM( _pEduCB, _majoritySize(), taskHWM ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to set task hwm, rc: %d",
                      rc ) ;
         PD_LOG( PDEVENT,
                 "Set taskID high water mark to %llu",
                 taskHWM ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s], rc: %d",
                 FIELD_NAME_TASKHWM, resultObj.toString().c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHKUID, "catCatalogueManager::_checkAllCSCLUniqueID" )
   INT32 catCatalogueManager::_checkAllCSCLUniqueID()
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__CHKUID ) ;

      SINT64 contextID        = -1 ;
      SDB_RTNCB *rtnCB        = pmdGetKRCB()->getRTNCB() ;
      UINT32 csUniqueHWM      = 0 ;
      INT32 curCSHWM          = 0 ;
      INT64 count             = 0 ;
      UINT32 iRec             = 0 ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj orderby = BSON( CAT_CS_UNIQUEID << -1 ) ;
      BSONObj dummyObj, resultObj ;
      rtnContextBuf buffObj ;

      PD_LOG( PDDEBUG, "Begin checkAllCSCLUniqueID" );

      // Check [CSUniqueHWM] field exists or not.
      // If the field exists, check uniqueid task has been done.
      rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummyObj, matcher,
                         dummyObj, _pEduCB, resultObj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to query from %s, rc: %d",
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      rc = rtnGetIntElement( resultObj, FIELD_NAME_CSUNIQUEHWM, curCSHWM ) ;
      if ( SDB_OK == rc )
      {
         goto done ;
      }
      if ( rc && rc != SDB_FIELD_NOT_EXIST )
      {
         goto error ;
      }

      // check cs count
      rc = catGetObjectCount( CAT_COLLECTION_SPACE_COLLECTION, dummyObj,
                              dummyObj, dummyObj, _pEduCB, count ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get count of collection[%s], rc: %d",
                   CAT_COLLECTION_SPACE_COLLECTION, rc ) ;
      if ( count > ( utilCSUniqueID )UTIL_CSUNIQUEID_MAX )
      {
         rc = SDB_CAT_CS_UNIQUEID_EXCEEDED ;
         PD_LOG( PDERROR,
                 "CS unique id can't exceed %u, cs count: %lld, rc: %d",
                 UTIL_CSUNIQUEID_MAX, count, rc ) ;
         goto error ;
      }

      // get all cs
      rc = rtnQuery( CAT_COLLECTION_SPACE_COLLECTION,
                     dummyObj, dummyObj, orderby, dummyObj,
                     0, _pEduCB, 0, -1, _pDmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to query from %s, rc: %d",
                   CAT_COLLECTION_SPACE_COLLECTION, rc ) ;

      while( TRUE )
      {
         iRec++ ;

         rc = rtnGetMore( contextID, 1, buffObj, _pEduCB, rtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               contextID = -1 ;
               break ;
            }
            goto error ;
         }

         const CHAR *csName = NULL ;
         utilCSUniqueID maxUniqueID = UTIL_UNIQUEID_NULL ;
         BSONObj boCollections ;
         BSONObj boSpace( buffObj.data() ) ;
         INT32 result = SDB_OK ;

         PD_LOG( PDDEBUG, "Begin to check cs[%s]", boSpace.toString().c_str() );

         // sort by { UniqueID: -1 }, the first one is the cs with max unique id
         rc = rtnGetIntElement( boSpace, CAT_CS_UNIQUEID, (INT32&)maxUniqueID );
         if ( 1 == iRec )
         {
            csUniqueHWM = maxUniqueID ;
         }
         if ( rc == SDB_OK )
         {
            continue ;
         }
         else if ( rc == SDB_FIELD_NOT_EXIST )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from [%s], rc: %d",
                      CAT_CS_UNIQUEID, boSpace.toString().c_str(), rc ) ;

         // get cs name and its cl list
         rc = rtnGetStringElement( boSpace, CAT_COLLECTION_SPACE_NAME,
                                   &csName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from [%s], rc: %d",
                      CAT_COLLECTION_SPACE_NAME, boSpace.toString().c_str(), rc ) ;

         rc = rtnGetArrayElement( boSpace, CAT_COLLECTION, boCollections ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get the field [%s] from [%s], rc: %d",
                      CAT_COLLECTION, boSpace.toString().c_str(), rc ) ;

         // set cs and cl unqiue id
         rc = catTransBegin( _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to begin trans, rc: %d", rc ) ;

         result = _setCSCLUniqueID( csName, boCollections, ++csUniqueHWM ) ;
         if ( result )
         {
            PD_LOG( PDERROR,
                    "Failed to set cs and its cl unique id, rc: %d",
                    result ) ;
         }

         rc = catTransEnd( result, _pEduCB, _pDpsCB ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to end trans, rc: %d", rc ) ;
         if ( result )
         {
            rc = result ;
            goto error ;
         }

      }

      // set cs uniqueid hwm in finally
      rc = catSetCSUniqueHWM( _pEduCB, _majoritySize(), csUniqueHWM ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to set cs unqiue id high water mask, rc: %d",
                   rc ) ;

   done:
      if ( -1 != contextID )
      {
         buffObj.release() ;
         rtnCB->contextDelete( contextID, _pEduCB ) ;
      }
      PD_TRACE_EXITRC( SDB_CATALOGMGR__CHKUID, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /**
    * Compatibility handling on data source with old version. The feture of data
    * source is originally developed in v3.2.8. In that version, the value of
    * the field 'GroupName' in collection metadata is always 'DataSource'. That
    * is a bad design, as 'DataSource' is just a very ordinary name which user
    * may use. Now we change it to '$null'.
    * So if there is any data source which was created when using sequoiadb
    * 3.2.8, we need to upgrade the related information.
    */
   PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHECKCLDATASOURCEINFO, "catCatalogueManager::_checkAndUpgradeDSCLInfo" )
   INT32 catCatalogueManager::_checkAndUpgradeDSCLInfo()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__CHECKCLDATASOURCEINFO ) ;
      INT64 count = 0 ;

      try
      {
         BSONObj dummyObj ;
         BSONObjBuilder builder ;
         BSONObj updator ;
         BSONObj matcher ;

         // Check whether need to upgrade or not.
         BSONObj dsMatcher = BSON( FIELD_NAME_TRANS_PROPAGATE_MODE <<
                                 BSON( MTH_OPERATOR_STR_EXISTS << 0 ) ) ;
         rc = catGetObjectCount( CAT_DATASOURCE_COLLECTION, dummyObj, dsMatcher,
                                 dummyObj, _pEduCB, count ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to query from %s, rc: %d",
                      CAT_DATASOURCE_COLLECTION, rc ) ;
         if ( 0 == count )
         {
            goto done ;
         }

         /**
          * Update collection SYSCAT.SYSCOLLECTIONS.
          * Matcher:
          * { "$and": [ { "DataSourceID": { "$exists": 1 } },
          *             { "CataInfo.GroupName": "DataSource" } ] } ;
          * Updator:
          * { "$set" : "CataInfo.0.GroupName": "$null" } }
          */
         BSONArrayBuilder subBuilder(
            builder.subarrayStart( MTH_OPERATOR_STR_AND ) ) ;
         subBuilder.append( BSON( FIELD_NAME_DATASOURCE_ID <<
                                  BSON( MTH_OPERATOR_STR_EXISTS << 1 ) ) ) ;
         subBuilder.append( BSON( FIELD_NAME_CATALOGINFO "." FIELD_NAME_GROUPNAME
                                  << "DataSource" ) ) ;
         subBuilder.done() ;
         matcher = builder.done() ;
         updator = BSON( "$set" <<
                         BSON( FIELD_NAME_CATALOGINFO ".0." FIELD_NAME_GROUPNAME
                               << CAT_DATASOURCE_GROUPNAME ) ) ;

         PD_LOG( PDDEBUG, "Update collection SYSCOLLECTIONS. Matcher: %s. "
                 "Updator: %s", matcher.toString().c_str(),
                 updator.toString().c_str() ) ;

         rc = rtnUpdate( CAT_COLLECTION_INFO_COLLECTION, matcher, updator,
                         dummyObj, 0, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Update collection %s failed, rc: %d",
                      CAT_COLLECTION_INFO_COLLECTION, rc ) ;

         /**
          * Update collection SYSCAT.SYSDATASOURCES.
          * Matcher:
          * { "TransPropagateMode": { "$exists": 0 } }
          * Updator:
          * { "$set": { "TransPropagateMode": "never" } }
          */
         updator = BSON( "$set" << BSON( FIELD_NAME_TRANS_PROPAGATE_MODE <<
                                         VALUE_NAME_NEVER <<
                                         FIELD_NAME_INHERIT_SESSION_ATTR <<
                                         true ) ) ;
         rc = rtnUpdate( CAT_DATASOURCE_COLLECTION, dsMatcher, updator,
                         dummyObj, 0, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Update collection %s failed, rc: %d",
                      CAT_DATASOURCE_COLLECTION, rc ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__CHECKCLDATASOURCEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR__CHECKANDUPGRADEUSERROLE, "catCatalogueManager::_checkAndUpgradeUserRole" )
   INT32 catCatalogueManager::_checkAndUpgradeUserRole()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATALOGMGR__CHECKANDUPGRADEUSERROLE ) ;

      // Set the role of user who do not have one to admin.
      try
      {
         BSONObj dummyObj ;
         BSONObj query = BSON( FIELD_NAME_OPTIONS "." FIELD_NAME_ROLE <<
                               BSON( "$exists" << 0 ) ) ;
         BSONObj updator = BSON( "$set" <<
            BSON( FIELD_NAME_OPTIONS "." FIELD_NAME_ROLE << VALUE_NAME_ADMIN ) ) ;
         rc = rtnUpdate( AUTH_USR_COLLECTION, query, updator,
                         dummyObj, 0, _pEduCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Update collection %s failed, rc: %d",
                      AUTH_USR_COLLECTION, rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATALOGMGR__CHECKANDUPGRADEUSERROLE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_PROCESSMSG, "catCatalogueManager::processMsg" )
   INT32 catCatalogueManager::processMsg( const NET_HANDLE &handle,
                                          MsgHeader *pMsg )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_PROCESSMSG ) ;
      PD_TRACE1 ( SDB_CATALOGMGR_PROCESSMSG,
                  PD_PACK_INT ( pMsg->opCode ) ) ;

      switch ( pMsg->opCode )
      {
      // command dispatch, need the second dispath in the function
      case MSG_CAT_CREATE_COLLECTION_REQ :
      case MSG_CAT_DROP_COLLECTION_REQ :
      case MSG_CAT_CREATE_COLLECTION_SPACE_REQ :
      case MSG_CAT_DROP_SPACE_REQ :
      case MSG_CAT_ALTER_CS_REQ :
      case MSG_CAT_ALTER_COLLECTION_REQ :
      case MSG_CAT_LINK_CL_REQ :
      case MSG_CAT_UNLINK_CL_REQ :
      case MSG_CAT_SPLIT_PREPARE_REQ :
      case MSG_CAT_SPLIT_READY_REQ :
      case MSG_CAT_TASK_CANCEL_REQ :
      case MSG_CAT_TASK_START_REQ :
      case MSG_CAT_SPLIT_CHGMETA_REQ :
      case MSG_CAT_SPLIT_CLEANUP_REQ :
      case MSG_CAT_SPLIT_FINISH_REQ :
      case MSG_CAT_CRT_PROCEDURES_REQ :
      case MSG_CAT_RM_PROCEDURES_REQ :
      case MSG_CAT_CREATE_DOMAIN_REQ :
      case MSG_CAT_DROP_DOMAIN_REQ :
      case MSG_CAT_ALTER_DOMAIN_REQ :
      case MSG_CAT_CREATE_IDX_REQ :
      case MSG_CAT_DROP_IDX_REQ :
      case MSG_CAT_TRUNCATE_REQ :
      case MSG_CAT_RENAME_CS_REQ :
      case MSG_CAT_RENAME_CL_REQ :
         {
            // up commands is run in cluster acitve status
            _pCatCB->getCatDCMgr()->setWritedCommand( TRUE ) ;
            rc = processCommandMsg( handle, pMsg, TRUE ) ;
            break;
         }
      case MSG_CAT_QUERY_SPACEINFO_REQ :
         {
            rc = processCommandMsg( handle, pMsg, TRUE ) ;
            break;
         }
      case MSG_CAT_QUERY_CATALOG_REQ:
         {
            rc = processQueryCatalogue( handle, pMsg ) ;
            break;
         }
      case MSG_CAT_QUERY_TASK_REQ:
         {
            rc = processQueryTask ( handle, pMsg ) ;
            break ;
         }
      default:
         {
            rc = SDB_UNKNOWN_MESSAGE;
            PD_LOG( PDWARNING, "received unknown message (opCode: [%d]%u)",
                    IS_REPLY_TYPE(pMsg->opCode),
                    GET_REQUEST_TYPE(pMsg->opCode) ) ;
            break;
         }
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_PROCESSMSG, rc ) ;
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_PROCESSCOMMANDMSG, "catCatalogueManager::processCommandMsg" )
   INT32 catCatalogueManager::processCommandMsg( const NET_HANDLE &handle,
                                                 MsgHeader *pMsg,
                                                 BOOLEAN writable )
   {
      INT32 rc = SDB_OK ;
      MsgOpQuery *pQueryReq = (MsgOpQuery *)pMsg ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR_PROCESSCOMMANDMSG ) ;
      MsgOpReply replyHeader ;
      rtnContextBuf ctxBuff ;

      INT32      opCode = pQueryReq->header.opCode ;
      BOOLEAN    fillPeerRouteID = FALSE ;

      INT32 flag = 0 ;
      const CHAR *pCMDName = NULL ;
      INT64 numToSkip = 0 ;
      INT64 numToReturn = 0 ;
      const CHAR *pQuery = NULL ;
      const CHAR *pFieldSelector = NULL ;
      const CHAR *pOrderBy = NULL ;
      const CHAR *pHint = NULL ;

      BOOLEAN delayLockFailed = TRUE ;

      // init reply msg
      replyHeader.header.messageLength = sizeof( MsgOpReply ) ;
      replyHeader.contextID = -1 ;
      replyHeader.flags = SDB_OK ;
      replyHeader.numReturned = 0 ;
      replyHeader.startFrom = 0 ;
      _fillRspHeader( &(replyHeader.header), &(pQueryReq->header) ) ;

      if ( MSG_CAT_TASK_START_REQ == opCode ||
           MSG_CAT_SPLIT_CHGMETA_REQ == opCode ||
           MSG_CAT_SPLIT_CLEANUP_REQ == opCode ||
           MSG_CAT_SPLIT_FINISH_REQ == opCode )
      {
         fillPeerRouteID = TRUE ;
         _pCatCB->getCatDCMgr()->setWritedCommand( FALSE ) ;
      }

      // extract msg
      rc = msgExtractQuery( (const CHAR*)pMsg, &flag, &pCMDName, &numToSkip,
                            &numToReturn, &pQuery, &pFieldSelector,
                            &pOrderBy, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to extract query msg, rc: %d", rc ) ;

      if ( writable )
      {
         // primary check
         BOOLEAN isDelay = FALSE ;
         rc = _pCatCB->primaryCheck( _pEduCB, TRUE, isDelay ) ;
         if ( isDelay )
         {
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG ( PDWARNING, "Service deactive but received command: %s,"
                     "opCode: %d, rc: %d", pCMDName,
                     pQueryReq->header.opCode, rc ) ;
            goto error ;
         }
      }

      if ( _pCatCB->getCatDCMgr()->isWritedCommand() &&
           _pCatCB->isDCReadonly() )
      {
         rc = SDB_CAT_CLUSTER_IS_READONLY ;
         goto error ;
      }

      // the second dispatch msg
      switch ( pQueryReq->header.opCode )
      {
         case MSG_CAT_CREATE_COLLECTION_SPACE_REQ :
            rc = processCmdCreateCS( pQuery, ctxBuff ) ;
            break ;
         case MSG_CAT_SPLIT_PREPARE_REQ :
         case MSG_CAT_SPLIT_READY_REQ :
         case MSG_CAT_TASK_CANCEL_REQ :
         case MSG_CAT_TASK_START_REQ :
         case MSG_CAT_SPLIT_CHGMETA_REQ :
         case MSG_CAT_SPLIT_CLEANUP_REQ :
         case MSG_CAT_SPLIT_FINISH_REQ :
            // No delay for lock failed, since split task has lower priority to
            // process data, if lock failed the collection might be being dropped
            delayLockFailed = FALSE ;
            rc = processCmdTask( pQuery, pQueryReq->header.opCode, ctxBuff ) ;
            break ;
         case MSG_CAT_QUERY_SPACEINFO_REQ :
            rc = processCmdQuerySpaceInfo( pQuery, ctxBuff ) ;
            break ;
         case MSG_CAT_CRT_PROCEDURES_REQ :
            rc = processCmdCrtProcedures( pQuery ) ;
            break ;
         case MSG_CAT_RM_PROCEDURES_REQ :
            rc = processCmdRmProcedures( pQuery ) ;
            break ;
         case MSG_CAT_DROP_SPACE_REQ :
         case MSG_CAT_ALTER_CS_REQ :
         case MSG_CAT_CREATE_COLLECTION_REQ :
         case MSG_CAT_DROP_COLLECTION_REQ :
         case MSG_CAT_ALTER_COLLECTION_REQ :
         case MSG_CAT_LINK_CL_REQ :
         case MSG_CAT_UNLINK_CL_REQ :
         case MSG_CAT_RENAME_CS_REQ :
         case MSG_CAT_RENAME_CL_REQ :
         case MSG_CAT_TRUNCATE_REQ :
         {
            SINT64 contextID = -1;
            catContextPtr pCatCtx ;
            rc = catCreateContext ( (MSG_TYPE)pQueryReq->header.opCode,
                                    pCatCtx, contextID,
                                    _pEduCB ) ;
            if ( SDB_OK == rc )
            {
               rc = pCatCtx->open( handle, pMsg, pQuery, pHint, ctxBuff,
                                   _pEduCB ) ;
               if ( SDB_OK != rc )
               {
                  catDeleteContext( contextID, _pEduCB ) ;
                  contextID = -1 ;
               }
               else
               {
                  replyHeader.contextID = contextID ;
                  _pCatCB->addContext( handle, replyHeader.header.TID,
                                       contextID ) ;
               }
            }
            break;
         }
         case MSG_CAT_CREATE_IDX_REQ :
            rc = processCmdCreateIndex( pQuery, pHint, ctxBuff ) ;
            break ;
         case MSG_CAT_DROP_IDX_REQ :
            rc = processCmdDropIndex( pQuery, pHint, ctxBuff ) ;
            break ;
         case MSG_CAT_CREATE_DOMAIN_REQ :
            rc = processCmdCreateDomain ( pQuery ) ;
            break ;
         case MSG_CAT_DROP_DOMAIN_REQ :
            rc = processCmdDropDomain ( pQuery ) ;
            break ;
         case MSG_CAT_ALTER_DOMAIN_REQ :
            rc = processCmdAlterDomain ( pQuery, ctxBuff ) ;
            break ;
         default :
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Received unknown command: %s, opCode: %d",
                    pCMDName, pQueryReq->header.opCode ) ;
            break ;
      }

      // Process lock failed error first
      if ( SDB_LOCK_FAILED == rc && delayLockFailed )
      {
         goto lock_failed ;
      }

      PD_RC_CHECK( rc, PDERROR, "Process command[%s] failed, opCode: %d, "
                   "rc: %d", pCMDName, pQueryReq->header.opCode, rc ) ;

   done:
      if ( fillPeerRouteID )
      {
         replyHeader.header.routeID.value = pQueryReq->header.routeID.value ;
      }

      if ( !_pCatCB->isDelayed() )
      {
         // send reply
         if ( 0 == ctxBuff.size() )
         {
            rc = _pCatCB->sendReply( handle, &replyHeader, rc ) ;
         }
         else
         {
            replyHeader.header.messageLength += ctxBuff.size() ;
            replyHeader.numReturned = ctxBuff.recordNum() ;
            rc = _pCatCB->sendReply( handle, &replyHeader, rc,
                                     (void *)ctxBuff.data(), ctxBuff.size() ) ;
         }
      }
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_PROCESSCOMMANDMSG, rc ) ;
      return rc ;

   lock_failed:
      // Lock failed, then try to delay operation
      if ( !_pCatCB->delayCurOperation() )
      {
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }
      else
      {
         // Ignore the lock error
         rc = SDB_OK ;
      }
      goto done ;

   error:
      replyHeader.flags = rc ;
      if( SDB_CLS_NOT_PRIMARY == rc )
      {
         replyHeader.startFrom = _pCatCB->getPrimaryNode() ;
      }
      goto done ;
   }

   void catCatalogueManager::_fillRspHeader( MsgHeader * rspMsg,
                                             const MsgHeader * reqMsg )
   {
      rspMsg->opCode = MAKE_REPLY_TYPE( reqMsg->opCode ) ;
      rspMsg->requestID = reqMsg->requestID ;
      rspMsg->routeID.value = 0 ;
      rspMsg->TID = reqMsg->TID ;
      rspMsg->globalID = reqMsg->globalID ;
   }

   INT16 catCatalogueManager::_majoritySize()
   {
      return _pCatCB->majoritySize() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_CREATEDOMAIN, "catCatalogueManager::processCmdCreateDomain" )
   INT32 catCatalogueManager::processCmdCreateDomain ( const CHAR *pQuery )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_CREATEDOMAIN ) ;

      catCtxLockMgr lockMgr ;

      // first extract pQuery and find the options
      try
      {
         BSONObj insertObj ;
         BSONObj boQuery( pQuery );
         BSONObjBuilder ob ;
         BSONElement beDomainOptions ;
         const CHAR *pDomainName = NULL ;
         INT32 expectedObjSize   = 0 ;

         // find out the domain name
         BSONElement beDomainName = boQuery.getField ( CAT_DOMAINNAME_NAME ) ;
         PD_CHECK( beDomainName.type() == String,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to create domain: "
                   "failed to get the field [%s] from query",
                   CAT_DOMAINNAME_NAME );
         pDomainName = beDomainName.valuestr() ;

         PD_TRACE1 ( SDB_CATALOGMGR_CREATEDOMAIN, PD_PACK_STRING(pDomainName) ) ;

         // domain name validation
         rc = catDomainNameValidate ( pDomainName ) ;
         PD_CHECK ( SDB_OK == rc,
                    rc, error, PDERROR,
                    "Invalid domain name: %s, rc = %d",
                    pDomainName, rc ) ;
         ob.append ( CAT_DOMAINNAME_NAME, pDomainName ) ;
         expectedObjSize ++ ;

         // Lock domain
         PD_CHECK( lockMgr.tryLockDomain( pDomainName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock domain[%s]",
                   pDomainName ) ;

         // options validation
         beDomainOptions = boQuery.getField ( CAT_OPTIONS_NAME ) ;
         if ( !beDomainOptions.eoo() && beDomainOptions.type() != Object )
         {
            PD_LOG ( PDERROR,
                     "Invalid options type, expected eoo or object" ) ;
            rc = SDB_INVALIDARG ;
         }
         // if we provide options, let's extract each option
         if ( beDomainOptions.type() == Object )
         {
            vector< string > vecGroups ;
            rc = catDomainOptionsExtract ( beDomainOptions.embeddedObject(),
                                           _pEduCB, &ob, &vecGroups ) ;
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to validate domain options, rc = %d",
                        rc ) ;
               goto error ;
            }
            expectedObjSize ++ ;

            if ( !vecGroups.empty() )
            {
               // check group is active or not
               rc = catCheckGroupsByName( vecGroups ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to check groups for domain [%s], rc: %d",
                            pDomainName, rc ) ;
            }

            // Lock data group which will be assigned to this domain
            for ( UINT32 i = 0 ; i < vecGroups.size() ; ++i )
            {
               PD_CHECK( lockMgr.tryLockGroup( vecGroups[i], SHARED ),
                         SDB_LOCK_FAILED, error, PDERROR,
                         "Failed to lock group [%s]",
                         vecGroups[i].c_str() );
            }
         }

         // sanity check for garbage fields
         if ( boQuery.nFields() != expectedObjSize )
         {
            PD_LOG ( PDERROR,
                     "Actual input doesn't match expected opt size, "
                     "there could be one or more invalid arguments" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // checks are done, let's insert into collection
         insertObj = ob.obj () ;
         rc = rtnInsert ( CAT_DOMAIN_COLLECTION, insertObj, 1,
                          0, _pEduCB ) ;
         if ( rc )
         {
            // if there's duplicate key exception, that means the domain is
            // already exist
            if ( SDB_IXM_DUP_KEY == rc )
            {
               PD_LOG ( PDERROR,
                        "Domain [%s] is already exist",
                        pDomainName ) ;
               rc = SDB_CAT_DOMAIN_EXIST ;
               goto error ;
            }
            else
            {
               PD_LOG ( PDERROR,
                        "Failed to insert domain object into [%s], rc = %d",
                        CAT_DOMAIN_COLLECTION, rc ) ;
               goto error ;
            }
         }
         PD_LOG( PDDEBUG, "Created domain [%s]", pDomainName ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_CREATEDOMAIN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_DROPDOMAIN, "catCatalogueManager::processCmdDropDomain" )
   INT32 catCatalogueManager::processCmdDropDomain ( const CHAR *pQuery )
   {
      INT32 rc                = SDB_OK ;
      const CHAR *pDomainName = NULL ;
      utilDeleteResult delResult ;
      PD_TRACE_ENTRY ( SDB_CATALOGMGR_DROPDOMAIN ) ;

      catCtxLockMgr lockMgr ;

      // first extract pQuery and find the options
      try
      {
         BSONObj tempObj ;
         BSONObj queryObj ;
         BSONObj resultObj ;
         BSONObj boQuery( pQuery );

         // find out the domain name
         BSONElement beDomainName = boQuery.getField( CAT_DOMAINNAME_NAME );
         PD_CHECK( beDomainName.type() == String,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to drop domain, "
                   "failed to get the field [%s] from query",
                   CAT_DOMAINNAME_NAME );
         pDomainName = beDomainName.valuestr() ;

         PD_TRACE1 ( SDB_CATALOGMGR_DROPDOMAIN, PD_PACK_STRING(pDomainName) ) ;

         // validate the domain is not empty by searching SYSCOLLECTIONSPACES
         // for {Domain} field matches pDomainName
         queryObj = BSON ( CAT_DOMAIN_NAME << pDomainName ) ;
         // context will be closed when rc == 0, otherwise it should already be
         // closed in the function
         rc = catGetOneObj ( CAT_COLLECTION_SPACE_COLLECTION, tempObj,
                             queryObj, tempObj, _pEduCB, resultObj ) ;
         if ( SDB_DMS_EOC != rc )
         {
            if ( rc )
            {
               PD_LOG ( PDERROR,
                        "Failed to get object from %s, rc: %d",
                        CAT_COLLECTION_SPACE_COLLECTION, rc ) ;
               goto error ;
            }
            else
            {
               rc = SDB_DOMAIN_IS_OCCUPIED ;
               PD_LOG_MSG( PDERROR,
                           "There are one or more collection spaces, "
                           "e.g. [%s], are using the domain [%s], rc: %d",
                           resultObj.getField(
                                 CAT_COLLECTION_SPACE_NAME ).valuestrsafe(),
                           pDomainName, rc ) ;
               goto error ;
            }
         }
         rc = SDB_OK ;

         // Lock domain
         PD_CHECK( lockMgr.tryLockDomain( pDomainName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock domain [%s]",
                   pDomainName ) ;

         // if we cannot find any record with given domain name, that's expected
         // attempt to delete from the SYSDOMAINS
         queryObj = BSON ( CAT_DOMAINNAME_NAME << pDomainName ) ;
         rc = rtnDelete ( CAT_DOMAIN_COLLECTION, queryObj,
                          tempObj, 0, _pEduCB, &delResult ) ;
         // if something wrong happened
         if ( rc )
         {
            PD_LOG ( PDERROR,
                  "Failed to drop domain %s, rc = %d",
                     pDomainName, rc ) ;
            goto error ;
         }
         // if delete is fine but we didn't find anything
         if ( 0 == delResult.deletedNum() )
         {
            PD_LOG ( PDERROR,
                     "Domain [%s] does not exist",
                     pDomainName ) ;
            rc = SDB_CAT_DOMAIN_NOT_EXIST ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "dropped domain [%s]", pDomainName ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATALOGMGR_DROPDOMAIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALOGMGR_ALTERDOMAIN, "catCatalogueManager::processCmdAlterDomain" )
   INT32 catCatalogueManager::processCmdAlterDomain ( const CHAR *pQuery, rtnContextBuf &ctxBuf )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATALOGMGR_ALTERDOMAIN ) ;

      BSONObj alterObject( pQuery ) ;
      BSONObj alterOptions ;

      BSONElement argElement ;
      const CHAR * domain = NULL ;

      rtnAlterJob alterJob ;

      /// 1. be sure that the request is legal.
      /// 2. update the record of this domain.

      argElement = alterObject.getField( CAT_DOMAINNAME_NAME ) ;
      PD_CHECK( String == argElement.type(), SDB_INVALIDARG, error, PDERROR,
                "Failed to alter domain: failed to get the field [%s] from query [%s]",
                CAT_DOMAINNAME_NAME, alterObject.toString().c_str() ) ;
      domain = argElement.valuestr() ;

      PD_TRACE1( SDB_CATALOGMGR_ALTERDOMAIN, PD_PACK_STRING( domain ) ) ;

      rc = alterJob.initialize( domain, RTN_ALTER_DOMAIN, alterObject ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize alter job on "
                   "domain [%s], rc: %d", domain, rc ) ;

      if ( alterJob.isEmpty() )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to alter domain [%s]: failed to initialize alter job",
                   domain ) ;
      }
      else
      {
         const RTN_ALTER_TASK_LIST & tasks = alterJob.getAlterTasks() ;
         for ( RTN_ALTER_TASK_LIST::const_iterator iterTask = tasks.begin() ;
               iterTask != tasks.end() ;
               iterTask ++ )
         {
            const rtnAlterTask * task = ( *iterTask ) ;
            catCtxAlterDomainTask catTask( domain, task ) ;
            catCtxLockMgr lockMgr ;

            rc = catTask.checkTask( _pEduCB, lockMgr ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check alter task [%s] on "
                         "domain [%s], rc: %d", task->getActionName(), domain,
                         rc ) ;

            rc = catTask.execute( _pEduCB, _pDmsCB, _pDpsCB, _majoritySize() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to execute alter task [%s] on "
                         "domain [%s], rc: %d", task->getActionName(), domain,
                         rc ) ;

            try
            {
               // Build return error group obj
               if ( RTN_ALTER_DOMAIN_SET_ACTIVE_LOCATION == task->getActionType() ||
                    RTN_ALTER_DOMAIN_SET_LOCATION == task->getActionType() )
               {
                  BSONObjBuilder retBuilder ;
                  rc = catTask.buildDomainGroups( retBuilder, CAT_GROUP_NAME ) ;
                  PD_RC_CHECK( rc, PDERROR, "Make return groups object failed, rc: %d", rc ) ;

                  rc = _pCatCB->makeFailedGroupsObj( retBuilder, catTask.getFailedGroupList() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Make return failed groups object failed, rc: %d", rc ) ;

                  ctxBuf = rtnContextBuf( retBuilder.obj() ) ;
               }
            }
            catch ( exception &e )
            {
               rc = ossException2RC( &e ) ;
               PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
               goto error ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATALOGMGR_ALTERDOMAIN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   UINT64 catCatalogueManager::assignTaskID ()
   {
      INT32 rc = SDB_OK ;
      UINT64 taskID = 0 ;

      rc = catGetAndIncTaskID( _pEduCB, _majoritySize(), taskID ) ;
      if ( rc )
      {
         return CLS_INVALID_TASKID ;
      }
      else
      {
         return taskID ;
      }
   }
}
