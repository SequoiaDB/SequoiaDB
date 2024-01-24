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

   Source File Name = catCommon.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmd.hpp"
#include "pmdCB.hpp"
#include "catCommon.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"
#include "rtn.hpp"
#include "fmpDef.hpp"
#include "clsCatalogAgent.hpp"
#include "utilCommon.hpp"
#include "utilArguments.hpp"
#include "catGTSDef.hpp"
#include "ossMemPool.hpp"
#include "ixmUtil.hpp"
#include "../bson/lib/md5.hpp"
#include "catCMDBase.hpp"
#include "pdSecure.hpp"
#include "mthModifier.hpp"
#include "catLocation.hpp"

using namespace bson ;

namespace engine
{

   // initial size of BSONObj builder for CATALOG object
   #define CAT_BUILDER_INIT_SIZE ( 128 )

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGROUPNAMEVALIDATE, "catGroupNameValidate" )
   INT32 catGroupNameValidate ( const CHAR *pName, BOOLEAN isSys )
   {
      INT32 rc = SDB_INVALIDARG ;
      PD_TRACE_ENTRY ( SDB_CATGROUPNAMEVALIDATE ) ;

      if ( !pName || pName[0] == '\0' )
      {
         PD_LOG ( PDWARNING, "group name can't be empty" ) ;
         goto error ;
      }
      PD_TRACE2 ( SDB_CATGROUPNAMEVALIDATE, PD_PACK_STRING ( pName ),
                  PD_PACK_UINT ( isSys ) ) ;
      // name is within valid length
      if ( ossStrlen ( pName ) > OSS_MAX_GROUPNAME_SIZE )
      {
         PD_LOG ( PDWARNING, "group name %s is too long",
                  pName ) ;
         goto error ;
      }
      // group name should not start from SYS nor $ if it's not SYSTEM created
      if ( !isSys &&
           ( ossStrncmp ( pName, "SYS", ossStrlen ( "SYS" ) ) == 0 ||
             ossStrncmp ( pName, "$", ossStrlen ( "$" ) ) == 0 ) )
      {
         PD_LOG ( PDWARNING, "group name should not start with SYS nor $: %s",
                  pName ) ;
         goto error ;
      }
      // there shouldn't be any dot in the name
      if ( ossStrchr ( pName, '.' ) != NULL )
      {
         PD_LOG ( PDWARNING, "group name should not contain dot(.): %s",
                  pName ) ;
         goto error ;
      }

      rc = SDB_OK ;

   done :
      PD_TRACE_EXITRC ( SDB_CATGROUPNAMEVALIDATE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 catDomainNameValidate( const CHAR * pName )
   {
      return catGroupNameValidate( pName, FALSE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDOMAINOPTIONSEXTRACT, "catDomainOptionsExtract" )
   INT32 catDomainOptionsExtract( const BSONObj &options,
                                  pmdEDUCB *cb,
                                  BSONObjBuilder *builder,
                                  vector< string > *pVecGroups )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATDOMAINOPTIONSEXTRACT ) ;

      // we use std string here because
      // 1) compare if a group name already exist, and we don't need to write
      // comparitor
      // 2) it's not performance sensitive code
      std::set <std::string> groupNameList ;
      INT32 expectedOptSize = 0 ;
      BSONElement beGroupList = options.getField ( CAT_GROUPS_NAME ) ;
      if ( !beGroupList.eoo() && beGroupList.type() != Array )
      {
         PD_LOG ( PDERROR, "group list must be array" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // iterate each element for group, validate each group must be exist
      if ( beGroupList.type() == Array )
      {
         BSONArrayBuilder gpInfoBuilder ;
         BSONObjIterator it ( beGroupList.embeddedObject() ) ;
         while ( it.more() )
         {
            // for each element in group, first we need to check if it's string,
            // and we need to make sure it's in group list
            BSONObj groupInfo ;
            BSONObjBuilder oneGroup ;
            BSONElement gpID ;
            BSONElement gpName ;
            BSONElement beGroupElement = it.next () ;
            if ( beGroupElement.type() != String )
            {
               PD_LOG ( PDERROR, "Each element in group list must be string" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            if ( pVecGroups )
            {
               pVecGroups->push_back( string( beGroupElement.valuestr() ) ) ;
            }

            rc = catGetGroupObj( beGroupElement.valuestr(),
                                 TRUE, groupInfo, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to get group info of [%s]",
                       beGroupElement.valuestr() ) ;
               goto error ;
            }

            if ( !groupNameList.insert ( beGroupElement.valuestr() ).second )
            {
               PD_LOG ( PDERROR, "Duplicate group name: %s",
                        beGroupElement.valuestr() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            gpName = groupInfo.getField( CAT_GROUPNAME_NAME ) ;
            SDB_ASSERT( !gpName.eoo(), "can not be eoo" ) ;
            if ( !gpName.eoo() && NULL != builder )
            {
               oneGroup.append( gpName ) ;
            }

            gpID = groupInfo.getField( CAT_GROUPID_NAME ) ;
            SDB_ASSERT( !gpID.eoo(), "can not be eoo" ) ;
            if ( !gpID.eoo() && NULL != builder )
            {
               oneGroup.append( gpID ) ;
            }

            if ( NULL != builder )
            {
               gpInfoBuilder << oneGroup.obj() ;
            }
         }

         if ( NULL != builder )
         {
            builder->append( CAT_GROUPS_NAME, gpInfoBuilder.arr() ) ;
         }
         ++ expectedOptSize ;
      }

      /// check option auto split
      {
      BSONElement autoSplit = options.getField( CAT_DOMAIN_AUTO_SPLIT ) ;
      if ( !autoSplit.eoo() && autoSplit.isBoolean() )
      {
         if ( NULL != builder )
         {
            builder->append( autoSplit ) ;
         }
         ++expectedOptSize ;
      }
      }

      /// check option auto rebalance
      {
      BSONElement autoRebalance = options.getField( CAT_DOMAIN_AUTO_REBALANCE ) ;
      if ( !autoRebalance.eoo() && autoRebalance.isBoolean() )
      {
         if ( NULL != builder )
         {
            builder->append( autoRebalance ) ;
         }
         ++expectedOptSize ;
      }
      }

      if ( options.nFields() != expectedOptSize )
      {
         PD_LOG ( PDERROR, "Actual input doesn't match expected opt size, "
                  "there could be one or more invalid arguments in options" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_CATDOMAINOPTIONSEXTRACT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATQUERYANDGETMORE, "catQueryAndGetMore" )
   INT32 catQueryAndGetMore ( MsgOpReply **ppReply,
                              const CHAR *collectionName,
                              BSONObj &selector,
                              BSONObj &matcher,
                              BSONObj &orderBy,
                              BSONObj &hint,
                              SINT32 flags,
                              pmdEDUCB *cb,
                              SINT64 numToSkip,
                              SINT64 numToReturn )
   {
      INT32 rc               = SDB_OK ;
      SINT64 contextID       = -1 ;
      pmdKRCB *pKRCB          = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB        = pKRCB->getDMSCB() ;
      SDB_RTNCB *rtnCB        = pKRCB->getRTNCB() ;
      SINT32 replySize       = sizeof(MsgOpReply) ;
      SINT32 replyBufferSize = 0 ;

      PD_TRACE_ENTRY ( SDB_CATQUERYANDGETMORE ) ;
      // first initialize reply buffer, note the caller is responsible to free
      // the memory
      rc = rtnReallocBuffer ( (CHAR**)ppReply, &replyBufferSize, replySize,
                              SDB_PAGE_SIZE ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to realloc buffer, rc = %d", rc ) ;
      ossMemset ( *ppReply, 0, replySize ) ;

      // perform query
      rc = rtnQuery ( collectionName, selector, matcher, orderBy, hint, flags,
                      cb, numToSkip, numToReturn, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc = %d", rc ) ;

      // extract all results
      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, -1, buffObj, cb, rtnCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               contextID = -1 ;
               rc = SDB_OK ;
               break ;
            }
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto error ;
         }

         // reply is always 4 bytes aligned like in context
         replySize = ossRoundUpToMultipleX ( replySize, 4 ) ;
         rc = rtnReallocBuffer ( (CHAR**)ppReply, &replyBufferSize,
                                 replySize + buffObj.size(), SDB_PAGE_SIZE ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to realloc buffer, rc = %d", rc ) ;

         // copy the new records from context buffer to reply buffer
         ossMemcpy ( &((CHAR*)(*ppReply))[replySize], buffObj.data(),
                     buffObj.size() ) ;
         (*ppReply)->numReturned += buffObj.recordNum() ;
         // update the current offset of reply
         replySize               += buffObj.size() ;
      }
      // finally update reply header
      (*ppReply)->header.messageLength = replySize ;
      (*ppReply)->flags                = SDB_OK ;
      (*ppReply)->contextID            = -1 ;

   done :
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATQUERYANDGETMORE, rc ) ;
      return rc ;
   error :
      if ( *ppReply )
      {
         SDB_OSS_FREE( (CHAR*)(*ppReply) ) ;
         *ppReply = NULL ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETONEOBJ, "catGetOneObj" )
   INT32 catGetOneObj( const CHAR * collectionName,
                       const BSONObj & selector,
                       const BSONObj & matcher,
                       const BSONObj & hint,
                       pmdEDUCB * cb,
                       BSONObj & obj )
   {
      INT32 rc                = SDB_OK ;
      SINT64 contextID        = -1 ;
      pmdKRCB *pKRCB          = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB        = pKRCB->getDMSCB() ;
      SDB_RTNCB *rtnCB        = pKRCB->getRTNCB() ;
      BSONObj dummyObj ;

      rtnContextBuf buffObj ;

      PD_TRACE_ENTRY ( SDB_CATGETONEOBJ ) ;
      // query
      rc = rtnQuery( collectionName, selector, matcher, dummyObj, hint,
                     0, cb, 0, 1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query from %s, rc: %d",
                   collectionName, rc ) ;

      // get more
      rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
         }
         goto error ;
      }

      // copy obj
      try
      {
         BSONObj resultObj( buffObj.data() ) ;
         obj = resultObj.copy() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         buffObj.release() ;
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATGETONEOBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETONEOBJBYORDER, "catGetOneObjByOrder" )
   INT32 catGetOneObjByOrder( const CHAR * collectionName,
                              const BSONObj & selector,
                              const BSONObj & matcher,
                              const BSONObj & order,
                              const BSONObj & hint,
                              pmdEDUCB * cb,
                              BSONObj & obj )
   {
      INT32 rc                = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATGETONEOBJBYORDER ) ;

      SINT64 contextID        = -1 ;
      pmdKRCB *pKRCB          = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB        = pKRCB->getDMSCB() ;
      SDB_RTNCB *rtnCB        = pKRCB->getRTNCB() ;

      rtnContextBuf buffObj ;

      // query
      rc = rtnQuery( collectionName, selector, matcher, order, hint,
                     0, cb, 0, 1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query from %s, rc: %d",
                   collectionName, rc ) ;

      // get more
      rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
         }
         goto error ;
      }

      // copy obj
      try
      {
         BSONObj resultObj( buffObj.data() ) ;
         obj = resultObj.copy() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         buffObj.release() ;
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_CATGETONEOBJBYORDER, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETOBJCOUNT, "catGetObjectCount" )
   INT32 catGetObjectCount ( const CHAR * collectionName,
                             const BSONObj & selector,
                             const BSONObj & matcher,
                             const BSONObj & hint,
                             pmdEDUCB * cb,
                             INT64 & count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETOBJCOUNT ) ;

      pmdKRCB * krcb = pmdGetKRCB() ;
      SDB_DMSCB * dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB * rtnCB = krcb->getRTNCB() ;

      rtnQueryOptions options ;

      options.setCLFullName( collectionName ) ;
      options.setSelector( selector ) ;
      options.setQuery( matcher ) ;
      options.setHint( hint ) ;

      rc = rtnGetCount( options, dmsCB, cb, rtnCB, &count ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get count on collection [%s], "
                    "rc: %d", collectionName, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATGETOBJCOUNT, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETGROUPOBJ, "catGetGroupObj" )
   INT32 catGetGroupObj( const CHAR * groupName, BOOLEAN dataGroupOnly,
                         BSONObj & obj, pmdEDUCB *cb  )
   {
      INT32 rc           = SDB_OK;

      PD_TRACE_ENTRY ( SDB_CATGETGROUPOBJ ) ;
      BSONObj dummyObj ;
      BSONObj boMatcher ;
      BSONObjBuilder builder ;

      boMatcher = BSON( CAT_GROUPNAME_NAME << groupName ) ;

      rc = catGetOneObj( CAT_NODE_INFO_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 boMatcher.toString().c_str(), CAT_NODE_INFO_COLLECTION, rc ) ;
         goto error ;
      }
      else if ( dataGroupOnly )
      {
         BSONElement e = obj.getField( CAT_ROLE_NAME ) ;
         if ( !e.isNumber() || SDB_ROLE_DATA != e.numberInt() )
         {
            rc = SDB_CAT_IS_NOT_DATAGROUP ;
            goto error ;
         }

         e = obj.getField( FIELD_NAME_GROUPID ) ;
         if ( !e.isNumber() || DATA_GROUP_ID_BEGIN > e.numberInt() )
         {
            rc = SDB_CAT_IS_NOT_DATAGROUP ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETGROUPOBJ, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETGROUPOBJ1, "catGetGroupObj" )
   INT32 catGetGroupObj( UINT32 groupID, BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc           = SDB_OK;

      PD_TRACE_ENTRY ( SDB_CATGETGROUPOBJ1 ) ;
      BSONObj dummyObj ;
      BSONObj boMatcher = BSON( CAT_GROUPID_NAME << groupID );

      rc = catGetOneObj( CAT_NODE_INFO_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 boMatcher.toString().c_str(), CAT_NODE_INFO_COLLECTION, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETGROUPOBJ1, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETGROUPOBJ2, "catGetGroupObj" )
   INT32 catGetGroupObj( UINT16 nodeID, BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc           = SDB_OK;

      PD_TRACE_ENTRY ( SDB_CATGETGROUPOBJ2 ) ;
      BSONObj dummyObj ;
      BSONObj boMatcher = BSON(
            FIELD_NAME_GROUP "." FIELD_NAME_NODEID << nodeID );

      rc = catGetOneObj( CAT_NODE_INFO_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 boMatcher.toString().c_str(), CAT_NODE_INFO_COLLECTION, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETGROUPOBJ2, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGROUPCHECK, "catGroupCheck" )
   INT32 catGroupCheck( const CHAR * groupName, BOOLEAN & exist, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGROUPCHECK ) ;
      BSONObj boGroupInfo ;
      rc = catGetGroupObj( groupName, FALSE, boGroupInfo, cb ) ;
      if ( SDB_OK == rc )
      {
         exist = TRUE ;
      }
      else if ( SDB_CLS_GRP_NOT_EXIST == rc )
      {
         rc = SDB_OK;
         exist = FALSE;
      }
      PD_TRACE_EXITRC ( SDB_CATGROUPCHECK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSERVICECHECK, "catServiceCheck" )
   INT32 catServiceCheck( const CHAR * hostName, const CHAR * serviceName,
                          BOOLEAN & exist, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATSERVICECHECK ) ;
      BSONObj groupInfo ;
      BSONObj dummyObj ;
      BSONObj match = BSON( FIELD_NAME_GROUP << BSON( "$elemMatch" <<
                            BSON( FIELD_NAME_SERVICE "." FIELD_NAME_NAME <<
                                  serviceName << FIELD_NAME_HOST <<
                                  hostName )) ) ;

      rc = catGetOneObj( CAT_NODE_INFO_COLLECTION, dummyObj, match,
                         dummyObj, cb, groupInfo ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         exist = FALSE ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 match.toString().c_str(), CAT_NODE_INFO_COLLECTION, rc ) ;
      }
      else
      {
         exist = TRUE ;
      }
      PD_TRACE_EXITRC ( SDB_CATSERVICECHECK, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGROUPID2NAME, "catGroupID2Name" )
   INT32 catGroupID2Name( UINT32 groupID, string & groupName, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj groupObj ;
      const CHAR *name = NULL ;

      PD_TRACE_ENTRY ( SDB_CATGROUPID2NAME ) ;
      rc = catGetGroupObj( groupID, groupObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group obj by id[%d], rc: %d",
                   groupID, rc ) ;

      rc = rtnGetStringElement( groupObj, CAT_GROUPNAME_NAME, &name ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                   CAT_GROUPNAME_NAME, rc ) ;

      groupName = name ;

   done:
      PD_TRACE_EXITRC ( SDB_CATGROUPID2NAME, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGROUPNAME2ID, "catGroupName2ID" )
   INT32 catGroupName2ID( const CHAR * groupName, UINT32 &groupID,
                          BOOLEAN dataGroupOnly, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj groupObj ;
      INT32 tmpGrpID = CAT_INVALID_GROUPID ;

      PD_TRACE_ENTRY ( SDB_CATGROUPNAME2ID ) ;

      // Report SDB_CLS_GRP_NOT_EXIST first
      // Get back group object anyway, filter data group later
      rc = catGetGroupObj( groupName, dataGroupOnly, groupObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group obj by name[%s], rc: %d",
                   groupName, rc ) ;

      // Get group ID
      rc = rtnGetIntElement( groupObj, CAT_GROUPID_NAME, tmpGrpID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                   CAT_GROUPID_NAME, rc ) ;

      groupID = (UINT32)tmpGrpID ;

   done :
      PD_TRACE_EXITRC ( SDB_CATGROUPNAME2ID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 catGroupCount( INT64 & count, pmdEDUCB * cb )
   {
      INT32 rc                = SDB_OK ;
      SDB_DMSCB *dmsCB        = pmdGetKRCB()->getDMSCB() ;
      dmsStorageUnit *su      = NULL ;
      dmsStorageUnitID suID   = DMS_INVALID_CS ;
      const CHAR *pCollectionShortName = NULL ;

      rc = rtnResolveCollectionNameAndLock ( CAT_NODE_INFO_COLLECTION,
                                             dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s, rc: %d",
                  CAT_NODE_INFO_COLLECTION, rc ) ;
         goto error ;
      }

      rc = su->countCollection( pCollectionShortName, count, cb, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to count collection: %s, rc: %d",
                 pCollectionShortName, rc ) ;
         goto error ;
      }

   done:
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock( suID ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETNODEID_BY_NODENAME, "catGetNodeIDByNodeName" )
   INT32 catGetNodeIDByNodeName( const BSONObj &groupObj,
                                 const string &nodeName,
                                 UINT16 &nodeID,
                                 BOOLEAN &isExists )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATGETNODEID_BY_NODENAME ) ;

      UINT16 tmpNodeID = CAT_INVALID_NODEID ;
      isExists = FALSE ;

      try
      {
         vector< string > splitStrVec ;

         rc = utilSplitStr( nodeName, splitStrVec, ":" ) ;
         if( SDB_OK != rc || splitStrVec.size() != 2 ||
             splitStrVec.at( 0 ).empty() || splitStrVec.at( 1 ).empty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "NodeName: %s is invalid", nodeName.c_str() ) ;
            goto error ;
         }

         {
            BSONObj boNodeList ;
            ossPoolString tmpHostName ;
            ossPoolString tmpSvcName ;
            pmdAddrPair node( splitStrVec[0], splitStrVec[1] ) ;

            rc = rtnGetArrayElement( groupObj, CAT_GROUP_NAME, boNodeList ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d", CAT_GROUP_NAME, rc ) ;

            BSONObjIterator itr( boNodeList ) ;
            while ( itr.more() )
            {
               BSONObj boNode = itr.next().embeddedObject() ;

               BSONElement beHost = boNode.getField( FIELD_NAME_HOST ) ;
               if ( beHost.eoo() || String != beHost.type() )
               {
                  rc = SDB_CAT_CORRUPTION ;
                  PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                          FIELD_NAME_HOST, beHost.type() ) ;
                  goto error ;
               }
               tmpHostName = beHost.valuestrsafe() ;

               BSONElement beService = boNode.getField( FIELD_NAME_SERVICE ) ;
               if ( beService.eoo() || Array != beService.type() )
               {
                  rc = SDB_CAT_CORRUPTION ;
                  PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                          FIELD_NAME_SERVICE, beService.type() ) ;
                  goto error ;
               }
               tmpSvcName = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE ) ;

               if ( 0 == ossStrcmp( tmpHostName.c_str(), node._host ) &&
                    0 == ossStrcmp( tmpSvcName.c_str(), node._service ) )
               {
                  rc = rtnGetIntElement( boNode, FIELD_NAME_NODEID, ( INT32 & )tmpNodeID ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                               FIELD_NAME_NODEID, rc ) ;
                  isExists = TRUE ;
                  break ;
               }
            }

            nodeID = isExists ? tmpNodeID : nodeID ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATGETNODEID_BY_NODENAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETDOMAINOBJ, "catGetDomainObj" )
   INT32 catGetDomainObj( const CHAR * domainName, BSONObj & obj, pmdEDUCB * cb )
   {
      INT32 rc           = SDB_OK;

      PD_TRACE_ENTRY ( SDB_CATGETDOMAINOBJ ) ;
      BSONObj dummyObj ;
      BSONObj boMatcher = BSON( CAT_DOMAINNAME_NAME << domainName );

      rc = catGetOneObj( CAT_DOMAIN_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CAT_DOMAIN_NOT_EXIST ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 boMatcher.toString().c_str(), CAT_DOMAIN_COLLECTION, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETDOMAINOBJ, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKDOMAINEXIST, "catCheckDomainExist" )
   INT32 catCheckDomainExist( const CHAR *pDomainName,
                              BOOLEAN &isExist,
                              BSONObj &obj,
                              pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      isExist = FALSE ;

      PD_TRACE_ENTRY ( SDB_CATCHECKDOMAINEXIST ) ;
      BSONObj dummyObj ;
      BSONObj boMatcher = BSON( CAT_DOMAINNAME_NAME << pDomainName );

      rc = catGetOneObj( CAT_DOMAIN_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;

      if ( SDB_DMS_EOC == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      PD_TRACE_EXITRC ( SDB_CATCHECKDOMAINEXIST, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETDOMAINGROUPS, "catGetDomainGroups" )
   INT32 catGetDomainGroups( const BSONObj & domain,
                             map < string, UINT32 > & groups )
   {
      INT32 rc = SDB_OK ;
      const CHAR *groupName = NULL ;
      INT32 groupID = CAT_INVALID_GROUPID ;

      PD_TRACE_ENTRY ( SDB_CATGETDOMAINGROUPS ) ;
      BSONElement beGroups = domain.getField( CAT_GROUPS_NAME ) ;
      if ( beGroups.eoo() )
      {
         rc = SDB_CAT_NO_GROUP_IN_DOMAIN ;
         goto error ;
      }

      if ( Array != beGroups.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Domin group type error, Domain info: %s",
                 domain.toString().c_str() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( beGroups.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error, PDERROR,
                      "Domain group ele type is not object, Domain info: %s",
                      domain.toString().c_str() ) ;

            BSONObj boGroup = ele.embeddedObject() ;
            rc = rtnGetStringElement( boGroup, CAT_GROUPNAME_NAME,
                                      &groupName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                         CAT_GROUPNAME_NAME, rc ) ;
            rc = rtnGetIntElement( boGroup, CAT_GROUPID_NAME, groupID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                         CAT_GROUPID_NAME, rc ) ;

            groups[ groupName ] = groupID ;
         }
      }

      if ( groups.empty() )
      {
         rc = SDB_CAT_NO_GROUP_IN_DOMAIN ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATGETDOMAINGROUPS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETDOMAINGROUPS1, "catGetDomainGroups" )
   INT32 catGetDomainGroups( const BSONObj &domain,
                             vector< UINT32 > &groupIDs )
   {
      INT32 rc = SDB_OK ;
      INT32 groupID = CAT_INVALID_GROUPID ;

      PD_TRACE_ENTRY ( SDB_CATGETDOMAINGROUPS1 ) ;
      BSONElement beGroups = domain.getField( CAT_GROUPS_NAME ) ;
      if ( beGroups.eoo() )
      {
         rc = SDB_CAT_NO_GROUP_IN_DOMAIN ;
         goto error ;
      }

      if ( Array != beGroups.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Domin group type error, Domain info: %s",
                 domain.toString().c_str() ) ;
         goto error ;
      }

      {
         BSONObjIterator i( beGroups.embeddedObject() ) ;
         while ( i.more() )
         {
            BSONElement ele = i.next() ;
            PD_CHECK( ele.isABSONObj(), SDB_INVALIDARG, error, PDERROR,
                      "Domain group ele type is not object, Domain info: %s",
                      domain.toString().c_str() ) ;

            BSONObj boGroup = ele.embeddedObject() ;
            rc = rtnGetIntElement( boGroup, CAT_GROUPID_NAME, groupID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                         CAT_GROUPID_NAME, rc ) ;

            groupIDs.push_back( groupID ) ;
         }
      }

      if ( groupIDs.empty() )
      {
         rc = SDB_CAT_NO_GROUP_IN_DOMAIN ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATGETDOMAINGROUPS1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATADDGRP2DOMAIN, "catAddGroup2Domain" )
   INT32 catAddGroup2Domain( const CHAR *domainName, const CHAR *groupName,
                             INT32 groupID, pmdEDUCB *cb,
                             _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB,
                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATADDGRP2DOMAIN ) ;
      BSONObj boMatcher = BSON( CAT_DOMAINNAME_NAME << domainName ) ;

      BSONObjBuilder updateBuild ;
      BSONObjBuilder sub( updateBuild.subobjStart("$addtoset") ) ;

      BSONObj newGroupObj = BSON( CAT_GROUPNAME_NAME << groupName <<
                                  CAT_GROUPID_NAME << groupID ) ;
      BSONObjBuilder sub2( sub.subarrayStart( CAT_GROUPS_NAME ) ) ;
      sub2.append( "0", newGroupObj ) ;
      sub2.done() ;

      sub.done() ;
      BSONObj updator = updateBuild.obj() ;
      BSONObj hint ;

      rc = rtnUpdate( CAT_DOMAIN_COLLECTION, boMatcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w ) ;

      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_DOMAIN_COLLECTION,
                   boMatcher.toString().c_str(),
                   updator.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATADDGRP2DOMAIN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETSPLITGROUPS, "catGetSplitCandidateGroups" )
   INT32 catGetSplitCandidateGroups ( const CHAR * collection,
                                      map< string, UINT32 > & groups,
                                      pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETSPLITGROUPS ) ;

      CHAR szSpace [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ]  = {0} ;
      CHAR szCollection [ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      BSONObj csObj ;
      BOOLEAN csExist = FALSE ;

      const CHAR *domainName = NULL ;

      rc = rtnResolveCollectionName( collection, ossStrlen( collection ),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Resolve collection name[%s] failed, rc: %d",
                   collection, rc ) ;

      rc = catCheckSpaceExist( szSpace, csExist, csObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Check collection space[%s] exist failed, "
                   "rc: %d", szSpace, rc ) ;
      PD_CHECK( csExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space[%s] is not exist", szSpace ) ;

      // get domain name
      rc = rtnGetStringElement( csObj, CAT_DOMAIN_NAME, &domainName ) ;

      // SYSTEM DOMAIN
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         sdbGetCatalogueCB()->getGroupNameMap( groups, TRUE ) ;
         rc = SDB_OK ;
      }
      // USER DOMAIN
      else if ( SDB_OK == rc )
      {
         // Check domain exist
         BSONObj domainObj ;

         rc = catGetDomainObj( domainName, domainObj, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Get domain[%s] failed, rc: %d",
                      domainName, rc ) ;

         rc = catGetDomainGroups( domainObj,  groups ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups from domain info[%s], "
                      "rc: %d", domainObj.toString().c_str(), rc ) ;
      }
      else
      {
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s] from collection "
                      "space [%s], rc: %d", CAT_DOMAIN_NAME, szSpace, rc ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATGETSPLITGROUPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDELGRPFROMDOMAIN, "catDelGroupFromDomain" )
   INT32 catDelGroupFromDomain( const CHAR *domainName, const CHAR *groupName,
                                UINT32 groupID, pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _dpsLogWrapper *dpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATDELGRPFROMDOMAIN ) ;
      BSONObj modifier = BSON( "$pull" << BSON( CAT_GROUPS_NAME <<
                               BSON( CAT_GROUPNAME_NAME << groupName <<
                                     CAT_GROUPID_NAME << (INT32)groupID ) ) ) ;
      BSONObj modifier2 = BSON( "$pull" << BSON( CAT_GROUPS_NAME <<
                                BSON( CAT_GROUPID_NAME << (INT32)groupID <<
                                      CAT_GROUPNAME_NAME << groupName ) ) ) ;
      BSONObj matcher ;
      BSONObj dummy ;

      if ( domainName && domainName[0] != 0 )
      {
         matcher = BSON( CAT_DOMAINNAME_NAME << domainName ) ;
      }
      // remove from all domain
      else
      {
         matcher = BSON( CAT_GROUPS_NAME "." CAT_GROUPID_NAME <<
                         (INT32)groupID ) ;
      }

      rc = rtnUpdate( CAT_DOMAIN_COLLECTION, matcher, modifier,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_DOMAIN_COLLECTION,
                   matcher.toString().c_str(), modifier.toString().c_str(),
                   rc ) ;

      // because pull operation must need obj full same, include element order
      // ex a:[{a:1,b:1}], if $pull:{b:1,a:1} will not match, so we need to
      // modify {a:1,b:1} and {b:1,a:1}
      rc = rtnUpdate( CAT_DOMAIN_COLLECTION, matcher, modifier2,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_DOMAIN_COLLECTION,
                   matcher.toString().c_str(), modifier2.toString().c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATDELGRPFROMDOMAIN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATEDOMAIN, "catUpdateDomain" )
   INT32 catUpdateDomain ( const CHAR * domainName,
                           const BSONObj & domainObject,
                           pmdEDUCB * cb,
                           _SDB_DMSCB * dmsCB,
                           _dpsLogWrapper * dpsCB,
                           INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATUPDATEDOMAIN ) ;

      rtnQueryOptions queryOptions ;
      queryOptions.setCLFullName( CAT_DOMAIN_COLLECTION ) ;
      queryOptions.setQuery( BSON( CAT_DOMAINNAME_NAME << domainName ) ) ;

      BSONObj updator = BSON( "$set" << domainObject ) ;

      rc = rtnUpdate( queryOptions, updator, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection [%s], rc: %d",
                   CAT_DOMAIN_COLLECTION, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATUPDATEDOMAIN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAATADDCL2CS, "catAddCL2CS" )
   INT32 catAddCL2CS( const CHAR * csName,
                      const CHAR * clName, utilCLUniqueID clUniqueID,
                      pmdEDUCB * cb, SDB_DMSCB * dmsCB,
                      SDB_DPSCB * dpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CAATADDCL2CS ) ;

      SDB_ASSERT( NULL != csName, "collection space name is invalid" ) ;
      SDB_ASSERT( NULL != clName, "collection name is invalid" ) ;
      SDB_ASSERT( UTIL_UNIQUEID_NULL != clUniqueID, "unique ID is invalid" ) ;

      BSONObj matcher, updator, dummy, boSpace ;
      BSONObjBuilder updateBuilder ;

      PD_CHECK( UTIL_UNIQUEID_NULL != clUniqueID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to add collection [%s] into collection space [%s], "
                "no unique ID is given", clName, csName ) ;

      try
      {
         matcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      // fetch latest object of collection space
      rc = catGetOneObj( CAT_COLLECTION_SPACE_COLLECTION, dummy, matcher,
                         dummy, cb, boSpace ) ;
      if ( SDB_DMS_EOC == rc )
      {
         // not found, modify the return code
         rc = SDB_DMS_CS_NOTEXIST ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection space [%s] from "
                   "collection [%s], rc: %d", csName,
                   CAT_COLLECTION_SPACE_COLLECTION, rc ) ;

      try
      {
         // if unique ID is given, we need to move forward the high water
         // mark of collection unique ID
         utilCLUniqueID curCLUniqueID = UTIL_UNIQUEID_NULL ;
         BSONElement element = boSpace.getField( CAT_CS_CLUNIQUEHWM ) ;
         if ( element.isNumber() )
         {
            curCLUniqueID = (utilCLUniqueID)( element.numberLong() ) ;
         }

         // update array of collections
         BSONObjBuilder sub( updateBuilder.subobjStart( "$addtoset" ) ) ;
         BSONArrayBuilder sub1( sub.subarrayStart( CAT_COLLECTION ) ) ;
         BSONObjBuilder clBuilder( sub1.subobjStart() ) ;
         clBuilder.append( CAT_COLLECTION_NAME, clName ) ;
         clBuilder.append( CAT_CL_UNIQUEID, (INT64)clUniqueID ) ;
         clBuilder.doneFast() ;
         sub1.doneFast() ;
         sub.doneFast() ;

         BSONObjBuilder setBuilder( updateBuilder.subobjStart( "$set" ) ) ;
         if ( UTIL_UNIQUEID_NULL == curCLUniqueID ||
              curCLUniqueID < clUniqueID )
         {
            setBuilder.append( CAT_CS_CLUNIQUEHWM, (INT64)clUniqueID ) ;
         }

         UINT64 currentTime = ossGetCurrentMilliseconds() ;
         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( currentTime, timestamp ) ;
         setBuilder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

         setBuilder.doneFast() ;

         updator = updateBuilder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build updator, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, matcher, updator,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), updator.toString().c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CAATADDCL2CS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDELCLFROMCS, "catDelCLFromCS" )
   INT32 catDelCLFromCS( const string &clFullName,
                         pmdEDUCB * cb, SDB_DMSCB * dmsCB, SDB_DPSCB * dpsCB,
                         INT16 w )
   {
      INT32 rc = SDB_OK ;
      CHAR szCLName[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;
      CHAR szCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;

      PD_TRACE_ENTRY ( SDB_CATDELCLFROMCS ) ;

      BSONObj modifier, matcher, dummy ;

      rc = rtnResolveCollectionName( clFullName.c_str(), clFullName.size(),
                                     szCSName, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCLName, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDWARNING, "Resolve collection name[%s] failed, rc: %d",
                   clFullName.c_str(), rc ) ;

      try
      {
         BSONObjBuilder builder( CAT_BUILDER_INIT_SIZE ) ;
         BSONObjBuilder pullBuilder( builder.subobjStart( "$pull_by" ) ) ;
         BSONObjBuilder clBuilder( pullBuilder.subobjStart( CAT_COLLECTION ) ) ;
         clBuilder.append( CAT_COLLECTION_NAME, szCLName ) ;
         clBuilder.doneFast() ;
         pullBuilder.doneFast() ;

         BSONObjBuilder setBuilder( builder.subobjStart( "$set" ) ) ;

         UINT64 currentTime = ossGetCurrentMilliseconds() ;
         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( currentTime, timestamp ) ;
         setBuilder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

         setBuilder.doneFast() ;
         modifier = builder.obj() ;

         matcher = BSON( CAT_COLLECTION_SPACE_NAME << szCSName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build updator, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, matcher, modifier,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), modifier.toString().c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATDELCLFROMCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRENAMECLFROMCS, "catRenameCLFromCS" )
   INT32 catRenameCLFromCS( const string &csName,
                            const string &clShortName,
                            const string &newCLShortName,
                            pmdEDUCB * cb, SDB_DMSCB * dmsCB,
                            SDB_DPSCB * dpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATRENAMECLFROMCS ) ;

      BSONObj boSpace, boCollections ;
      vector< PAIR_CLNAME_ID > clVec ;
      BOOLEAN isExist = FALSE ;

      rc = catCheckSpaceExist( csName.c_str(), isExist, boSpace, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get info of collection space [%s], rc: %d",
                   csName.c_str(), rc ) ;
      PD_CHECK( isExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space [%s] does not exist!",
                csName.c_str() ) ;

      rc = rtnGetArrayElement( boSpace, CAT_COLLECTION, boCollections ) ;
      PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                "Failed to get the field [%s] from [%s], rc: %d",
                CAT_COLLECTION, boSpace.toString().c_str(), rc ) ;

      {
         BSONObjIterator iter( boCollections ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            string clName ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

            rc = rtnGetSTDStringElement( ele.embeddedObject(),
                                         CAT_COLLECTION_NAME,
                                         clName ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                      "Failed to get the field [%s], rc: %d",
                      CAT_COLLECTION_NAME, ele.toString().c_str(), rc ) ;

            rc = rtnGetNumberLongElement( ele.embeddedObject(),
                                          CAT_CL_UNIQUEID,
                                          (INT64&)clUniqueID ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                      "Failed to get the field [%s], rc: %d",
                      CAT_CL_UNIQUEID, ele.toString().c_str(), rc ) ;

            if ( clName == clShortName )
            {
               clName = newCLShortName ;
            }
            PAIR_CLNAME_ID clPair( clName, clUniqueID ) ;
            clVec.push_back( clPair ) ;
         }
      }

      rc = catUpdateCSCLs( csName, clVec, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update collections from collection space [%s], "
                   "rc: %d", csName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATRENAMECLFROMCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDELCLSFROMCS, "catDelCLsFromCS" )
   INT32 catDelCLsFromCS( const string &csName,
                          const vector<string> &deleteCLLst,
                          pmdEDUCB * cb, SDB_DMSCB * dmsCB, SDB_DPSCB * dpsCB,
                          INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATDELCLSFROMCS ) ;

      BSONObj boSpace, boCollections ;
      vector< PAIR_CLNAME_ID > remainCLs ;
      BOOLEAN isExist = FALSE ;

      rc = catCheckSpaceExist( csName.c_str(), isExist, boSpace, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get info of collection space [%s], rc: %d",
                   csName.c_str(), rc ) ;
      PD_CHECK( isExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space [%s] does not exist!",
                csName.c_str() ) ;

      rc = rtnGetArrayElement( boSpace, CAT_COLLECTION, boCollections ) ;
      PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                "Failed to get the field [%s] from [%s], rc: %d",
                CAT_COLLECTION, boSpace.toString().c_str(), rc ) ;

      {
         BSONObjIterator iter( boCollections ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            string collection ;
            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;
            rc = rtnGetSTDStringElement( ele.embeddedObject(),
                                         CAT_COLLECTION_NAME,
                                         collection ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                      "Failed to get the field [%s], rc: %d",
                      CAT_COLLECTION_NAME, ele.toString().c_str(), rc ) ;
            rc = rtnGetNumberLongElement( ele.embeddedObject(),
                                          CAT_CL_UNIQUEID,
                                          (INT64&)clUniqueID ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                      "Failed to get the field [%s], rc: %d",
                      CAT_CL_UNIQUEID, ele.toString().c_str(), rc ) ;
            if ( find( deleteCLLst.begin(), deleteCLLst.end(), collection ) ==
                 deleteCLLst.end() )
            {
               PAIR_CLNAME_ID clPair( collection, clUniqueID ) ;
               remainCLs.push_back( clPair ) ;
            }
         }
      }

      rc = catUpdateCSCLs( csName, remainCLs, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update collections from collection space [%s], "
                   "rc: %d", csName.c_str(), rc ) ;
   done :
      PD_TRACE_EXITRC( SDB_CATDELCLSFROMCS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATECSCL, "catUpdateCSCLs" )
   INT32 catUpdateCSCLs( const string &csName,
                         vector< PAIR_CLNAME_ID > &collections,
                         pmdEDUCB *cb, _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB,
                         INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATUPDATECSCL ) ;

      BSONObj matcher, modifier, dummy ;

      try
      {
         BSONObjBuilder builder( CAT_BUILDER_INIT_SIZE ) ;
         BSONObjBuilder subBuilder( builder.subobjStart( "$set" ) ) ;
         BSONArrayBuilder arrBuilder( subBuilder.subarrayStart( CAT_COLLECTION ) ) ;
         for ( vector<PAIR_CLNAME_ID>::iterator iter = collections.begin() ;
               iter != collections.end() ;
               ++ iter )
         {
            arrBuilder << BSON( CAT_COLLECTION_NAME << iter->first
                             << CAT_CL_UNIQUEID << (INT64)iter->second ) ;
         }
         arrBuilder.done() ;

         UINT64 currentTime = ossGetCurrentMilliseconds() ;
         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( currentTime, timestamp ) ;
         subBuilder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

         subBuilder.done() ;
         modifier = builder.obj() ;

         matcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build updator, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, matcher, modifier,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), modifier.toString().c_str(),
                   rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATUPDATECSCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATECS, "catUpdateCS" )
   INT32 catUpdateCS ( const CHAR * csName,
                       const BSONObj & setObject,
                       const BSONObj & unsetObject,
                       pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                       _dpsLogWrapper * dpsCB,
                       INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATUPDATECS ) ;

      BSONObj boMatcher, updator, hint ;


      try
      {
         BSONObjBuilder updateBuilder( CAT_BUILDER_INIT_SIZE ) ;

         BOOLEAN hasUpdateTime =
               ( setObject.hasField( FIELD_NAME_UPDATE_TIME ) ) ||
               ( unsetObject.hasField( FIELD_NAME_UPDATE_TIME ) ) ;

         if ( hasUpdateTime )
         {
            if ( !setObject.isEmpty() )
            {
               updateBuilder.append( "$set", setObject ) ;
            }
         }
         else
         {
            BSONObjBuilder tmpBuilder( updateBuilder.subobjStart( "$set" ) ) ;
            if ( !setObject.isEmpty() )
            {
               tmpBuilder.appendElements( setObject ) ;
            }

            UINT64 currentTime = ossGetCurrentMilliseconds() ;
            CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
            ossMillisecondsToString( currentTime, timestamp ) ;
            tmpBuilder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

            tmpBuilder.doneFast() ;
         }

         if ( !unsetObject.isEmpty() )
         {
            updateBuilder.append( "$unset", unsetObject ) ;
         }

         updator = updateBuilder.obj() ;

         boMatcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build updator, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, boMatcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   boMatcher.toString().c_str(),
                   updator.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATUPDATECS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   INT32 catCheckSpaceExist( const char *pSpaceName, BOOLEAN &isExist,
                             BSONObj &obj, pmdEDUCB *cb )
   {
      return catCheckSpaceExist( pSpaceName, UTIL_UNIQUEID_NULL,
                                 isExist, obj, cb ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKSPACEEXIST, "catCheckSpaceExist" )
   INT32 catCheckSpaceExist( const char *pSpaceName, utilCSUniqueID csUniqueID,
                             BOOLEAN &isExist, BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc           = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATCHECKSPACEEXIST ) ;

      isExist            = FALSE ;
      BSONObj matcher, dummyObj ;

      if ( ! UTIL_IS_VALID_CSUNIQUEID( csUniqueID ) )
      {
         matcher = BSON( CAT_COLLECTION_SPACE_NAME << pSpaceName ) ;
      }
      else
      {
         matcher = BSON( CAT_CS_UNIQUEID << csUniqueID ) ;
      }

      rc = catGetOneObj( CAT_COLLECTION_SPACE_COLLECTION, dummyObj, matcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 matcher.toString().c_str(), CAT_COLLECTION_SPACE_COLLECTION,
                 rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKSPACEEXIST, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSDOMAIN, "catGetCSDomain" )
   INT32 catGetCSDomain( const CHAR * pCollectionSpace, pmdEDUCB * cb,
                         string &domain )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETCSDOMAIN ) ;
      BSONObj matcher ;
      BSONObj dummyObj ;
      SDB_DMSCB * dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB * rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;

      // Query
      matcher = BSON( CAT_COLLECTION_SPACE_NAME << pCollectionSpace ) ;

      rc = rtnQuery( CAT_COLLECTION_SPACE_COLLECTION, dummyObj, matcher,
                     dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed:matcher=%s, "
                   "rc=%d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), rc ) ;

      // Get more
      try
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get cs info:cs=%s,rc=%d",
                      pCollectionSpace, rc ) ;

         obj = BSONObj( contextBuf.data() ) ;
         rc = rtnGetSTDStringElement( obj, CAT_DOMAIN_NAME, domain ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get cs's domain:cs=%s,rc=%d",
                      pCollectionSpace, rc ) ;
      }
      catch( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get cs's domain:cs=%s,exception=%s",
                 pCollectionSpace, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      if ( -1 != contextID )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }

      PD_TRACE_EXITRC ( SDB_CATGETCSDOMAIN, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGETDOMAINCSS, "catGetDomainCSs" )
   INT32 _catGetDomainCSs( const CHAR *sysCL,
                           const CHAR *domain,
                           pmdEDUCB *cb,
                           ossPoolList< utilCSUniqueID > &collectionSpaces )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB__CATGETDOMAINCSS ) ;

      SDB_ASSERT( NULL != sysCL, "collection is invalid" ) ;
      SDB_ASSERT( NULL != domain, "domain name is invalid" ) ;

      BSONObj matcher ;
      BSONObj dummyObj ;
      SDB_DMSCB * dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB * rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;

      try
      {
         // Query
         matcher = BSON( CAT_DOMAIN_NAME << domain ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = rtnQuery( sysCL, dummyObj, matcher,
                     dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, matcher: %s, "
                   "rc: %d", sysCL, matcher.toString().c_str(), rc ) ;

      // Get more
      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement element = obj.getField( CAT_CS_UNIQUEID ) ;
            if ( !element.isNumber() )
            {
               PD_LOG( PDWARNING, "Failed to parse collection space "
                       "for unique ID [%s]", obj.toPoolString().c_str() ) ;
               SDB_ASSERT( FALSE, "should have unique ID" ) ;
               continue ;
            }
            collectionSpaces.push_back(
                        (utilCSUniqueID)( element.numberInt() ) ) ;
         }
         catch( exception & e )
         {
            PD_LOG( PDERROR,
                    "Get collection space name from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC( SDB__CATGETDOMAINCSS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETDOMAINCSS, "catGetDomainCSs" )
   INT32 catGetDomainCSs ( const CHAR * domain, pmdEDUCB * cb,
                           ossPoolList< utilCSUniqueID > & collectionSpaces )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETDOMAINCSS ) ;

      rc = _catGetDomainCSs( CAT_COLLECTION_SPACE_COLLECTION, domain, cb,
                             collectionSpaces ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection space for "
                   "domain [%s], rc: %d", domain, rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATGETDOMAINCSS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATREMOVECL, "catRemoveCL" )
   INT32 catRemoveCL( const CHAR * clFullName, pmdEDUCB * cb,
                      SDB_DMSCB * dmsCB, SDB_DPSCB * dpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATREMOVECL ) ;
      BSONObj boMatcher = BSON( CAT_CATALOGNAME_NAME << clFullName ) ;
      BSONObj dummyObj ;

      rc = rtnDelete( CAT_COLLECTION_INFO_COLLECTION, boMatcher, dummyObj,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to del record from collection: %s, "
                   "match: %s, rc: %d", CAT_COLLECTION_INFO_COLLECTION,
                   boMatcher.toString().c_str(), rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATREMOVECL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catCheckCSExist( const CHAR* collection, pmdEDUCB* cb,
                          BOOLEAN& csExist )
   {
      INT32 rc = SDB_OK ;
      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      INT64 count = 0 ;
      csExist = FALSE ;

      rc = rtnResolveCollectionSpaceName( collection,
                                          ossStrlen( collection ),
                                          csName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get cs name from cl[%s], rc: %d",
                   collection, rc ) ;

      try
      {
         rc = catGetObjectCount( CAT_COLLECTION_SPACE_COLLECTION, BSONObj(),
                                 BSON( FIELD_NAME_NAME << csName ),
                                 BSONObj(), cb, count ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get count of collection[%s], rc: %d",
                      CAT_COLLECTION_SPACE_COLLECTION, rc ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      if ( count > 0 )
      {
         csExist = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKCOLLECTIONEXIST, "catCheckCollectionExist" )
   INT32 catCheckCollectionExist( const char *pCollectionName,
                                  BOOLEAN &isExist,
                                  BSONObj &obj,
                                  pmdEDUCB *cb )
   {
      INT32 rc           = SDB_OK ;
      isExist            = FALSE ;

      PD_TRACE_ENTRY ( SDB_CATCHECKCOLLECTIONEXIST ) ;
      BSONObj matcher = BSON( CAT_CATALOGNAME_NAME << pCollectionName ) ;
      BSONObj dummyObj ;

      rc = catGetOneObj( CAT_COLLECTION_INFO_COLLECTION, dummyObj, matcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 matcher.toString().c_str(), CAT_COLLECTION_INFO_COLLECTION,
                 rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKCOLLECTIONEXIST, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATECATALOG, "catUpdateCatalog" )
   INT32 catUpdateCatalog ( const CHAR * clFullName, const BSONObj & setInfo,
                            const BSONObj & unsetInfo, pmdEDUCB * cb, INT16 w,
                            BOOLEAN incVersion )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATUPDATECATALOG ) ;
      BSONObj hint, updator, match ;

      try
      {
         BSONObjBuilder updateBuilder( CAT_BUILDER_INIT_SIZE ) ;

         BOOLEAN hasUpdateTime =
                  ( setInfo.hasField( FIELD_NAME_UPDATE_TIME ) ) ||
                  ( unsetInfo.hasField( FIELD_NAME_UPDATE_TIME ) ) ;

         if ( incVersion )
         {
            updateBuilder.append( "$inc", BSON( CAT_VERSION_NAME << 1 ) ) ;
         }

         if ( hasUpdateTime )
         {
            if ( !setInfo.isEmpty() )
            {
               updateBuilder.append( "$set", setInfo ) ;
            }
         }
         else
         {
            BSONObjBuilder tmpBuilder( updateBuilder.subobjStart( "$set" ) ) ;
            if ( !setInfo.isEmpty() )
            {
               tmpBuilder.appendElements( setInfo ) ;
            }

            UINT64 currentTime = ossGetCurrentMilliseconds() ;
            CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
            ossMillisecondsToString( currentTime, timestamp ) ;
            tmpBuilder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;

            tmpBuilder.doneFast() ;
         }

         if ( !unsetInfo.isEmpty() )
         {
            updateBuilder.append( "$unset", unsetInfo ) ;
         }

         updator = updateBuilder.obj() ;

         match = BSON( CAT_CATALOGNAME_NAME << clFullName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build updator and matcher, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = rtnUpdate( CAT_COLLECTION_INFO_COLLECTION,
                      match, updator, hint,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDSEVERE, "Failed to update collection[%s] catalog info"
                   "[%s], rc: %d", clFullName, setInfo.toString().c_str(),
                   rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATUPDATECATALOG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATECATALOGBYPUSH, "catUpdateCatalogByPush" )
   INT32 catUpdateCatalogByPush ( const CHAR * clFullName,
                                  const CHAR *field, const BSONObj &boObj,
                                  pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATUPDATECATALOGBYPUSH ) ;
      BSONObj hint ;
      BSONObj match = BSON( CAT_CATALOGNAME_NAME << clFullName ) ;
      BSONObj updator = BSON( "$inc" << BSON( CAT_VERSION_NAME << 1 ) <<
                              "$push" << BSON( field << boObj ) ) ;

      rc = rtnUpdate( CAT_COLLECTION_INFO_COLLECTION,
                      match, updator, hint,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDSEVERE, "Failed to update collection[%s] catalog info"
                   "by push [%s:%s], rc: %d", clFullName, field,
                   boObj.toString().c_str(), rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATUPDATECATALOGBYPUSH, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATECATALOGBYUNSET, "catUpdateCatalogByUnset" )
   INT32 catUpdateCatalogByUnset( const CHAR * clFullName, const CHAR * field,
                                  pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATUPDATECATALOGBYUNSET ) ;
      BSONObj dummy ;
      BSONObj match = BSON( CAT_CATALOGNAME_NAME << clFullName ) ;
      BSONObj updator = BSON( "$inc" << BSON( CAT_VERSION_NAME << 1 ) <<
                              "$unset" << BSON( field << "" ) );

      rc = rtnUpdate( CAT_COLLECTION_INFO_COLLECTION, match, updator, dummy, 0,
                      cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDSEVERE,
                   "Failed to update collection[%s] by unset[%s], rc: %d",
                   clFullName, field, rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATUPDATECATALOGBYUNSET, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   static INT32 _catBuildCLMatcher( const CHAR *fieldName,
                                    const CHAR *csName,
                                    BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      CHAR lowBound[ DMS_COLLECTION_SPACE_NAME_SZ + 1 + 1 ] = { 0 } ;
      CHAR upBound[  DMS_COLLECTION_SPACE_NAME_SZ + 1 + 1 ] = { 0 } ;

      // eg: csName is "test", { Name: { $regex: "^test\\." } } is equal to
      // { Name: { $gt: "test.", $lt: "test/" } }. So if csName has
      // metacharacter(eg: "^"), we do not need to escape it.
      ossStrncpy( lowBound, csName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( lowBound, ".", 1 ) ;
      ossStrncpy( upBound, csName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( upBound, "/", 1 ) ;

      try
      {
         BSONObjBuilder subBuilder( builder.subobjStart( fieldName ) ) ;
         subBuilder.append( "$gt", lowBound ) ;
         subBuilder.append( "$lt", upBound ) ;
         subBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   static INT32 _catBuildCLMatcher( const CHAR *fieldName,
                                    const CHAR *csName,
                                    BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;

         rc = _catBuildCLMatcher( fieldName, csName, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build matcher, rc: %d", rc ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher for collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSSUBCLGRPS, "catGetCSSubCLGroups" )
   INT32 catGetCSSubCLGroups ( const CHAR * csName,
                               pmdEDUCB * cb,
                               ossPoolSet< UINT32 > & groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCSSUBCLGRPS ) ;

      SDB_DMSCB * dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB * rtnCB = pmdGetKRCB()->getRTNCB() ;

      INT64 contextID = -1 ;
      rtnQueryOptions queryOptions ;

      queryOptions.setCLFullName( CAT_COLLECTION_INFO_COLLECTION ) ;
      BSONObj matcherMaincl ;

      rc = _catBuildCLMatcher( CAT_MAINCL_NAME, csName, matcherMaincl ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] matcher for "
                   "collection space [%s], rc: %d", CAT_MAINCL_NAME,
                   csName, rc ) ;
      queryOptions.setQuery( matcherMaincl ) ;

      // query
      rc = rtnQuery( queryOptions, cb, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, "
                   "rc: %d", CAT_COLLECTION_INFO_COLLECTION, rc ) ;

      // get more
      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement eleCataInfo = obj.getField( CAT_CATALOGINFO_NAME ) ;
            if ( Array != eleCataInfo.type() )
            {
               continue ;
            }
            BSONObjIterator itr( eleCataInfo.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement e = itr.next() ;
               if ( Object != e.type() )
               {
                  continue ;
               }
               BSONObj cataItemObj = e.embeddedObject() ;
               BSONElement eleGID = cataItemObj.getField( CAT_GROUPID_NAME ) ;
               if ( eleGID.isNumber() )
               {
                  groups.insert( eleGID.numberInt() ) ;
               }
            }
         }
         catch( exception & e )
         {
            PD_LOG( PDERROR,
                    "Get collection name from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }// end of get more

   done :
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC( SDB_CATGETCSSUBCLGRPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGETCSGRPS_UID, "_catGetCSGroups" )
   INT32 _catGetCSGroups( const CHAR *sysCL,
                          utilCSUniqueID csUniqueID,
                          pmdEDUCB *cb,
                          ossPoolSet< UINT32 > &groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGETCSGRPS_UID ) ;

      SDB_DMSCB * dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB * rtnCB = pmdGetKRCB()->getRTNCB() ;

      rtnQueryOptions queryOptions ;
      BSONObj matcher ;
      INT64 contextID = -1 ;

      rc = utilGetCSBounds( CAT_CL_UNIQUEID, csUniqueID, matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] matcher for "
                   "collection space [%u], rc: %d", CAT_CS_UNIQUEID,
                   csUniqueID, rc ) ;

      queryOptions.setCLFullName( sysCL ) ;
      queryOptions.setQuery( matcher ) ;

      // query
      rc = rtnQuery( queryOptions, cb, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, "
                   "rc: %d", sysCL, rc ) ;

      // get more
      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement eleCataInfo = obj.getField( CAT_CATALOGINFO_NAME ) ;
            if ( Array != eleCataInfo.type() )
            {
               continue ;
            }
            BSONObjIterator itr( eleCataInfo.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement e = itr.next() ;
               if ( Object != e.type() )
               {
                  continue ;
               }
               BSONObj cataItemObj = e.embeddedObject() ;
               BSONElement eleGID = cataItemObj.getField( CAT_GROUPID_NAME ) ;
               if ( eleGID.isNumber() )
               {
                  groups.insert( eleGID.numberInt() ) ;
               }
            }
         }
         catch( exception & e )
         {
            PD_LOG( PDERROR,
                    "Get collection name from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }// end of get more

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CATGETCSGRPS_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSGRPS_VEC, "catGetCSGroups" )
   INT32 catGetCSGroups( utilCSUniqueID csUniqueID,
                         pmdEDUCB *cb,
                         BOOLEAN includeRecycleBin,
                         BOOLEAN includeRunningTask,
                         vector< UINT32 > &groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCSGRPS_VEC ) ;

      ossPoolSet< UINT32 > groupSet ;

      rc = catGetCSGroups( csUniqueID, cb, includeRecycleBin,
                           includeRunningTask, groupSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get groups for collection "
                   "space [%u], rc: %d", csUniqueID, rc ) ;

      rc = catSaveToGroupIDList( groupSet, groups ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to save group ID list, rc: %d",
                   rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETCSGRPS_VEC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSGRPS_UID, "catGetCSGroups" )
   INT32 catGetCSGroups( utilCSUniqueID csUniqueID,
                         pmdEDUCB *cb,
                         BOOLEAN includeRecycleBin,
                         BOOLEAN includeRunningTask,
                         ossPoolSet< UINT32 > &groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCSGRPS_UID ) ;

      rc = _catGetCSGroups( CAT_COLLECTION_INFO_COLLECTION, csUniqueID,
                            cb, groups ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get groups for collection "
                   "space [%u], rc: %d", csUniqueID, rc ) ;

      if ( includeRecycleBin )
      {
         rc = _catGetCSGroups( CAT_SYSRECYCLEBIN_CL_COLLECTION, csUniqueID,
                               cb, groups ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groups for collection "
                      "space [%u] in recycle bin, rc: %d", csUniqueID, rc ) ;
      }

      if ( includeRunningTask )
      {
         rc = catGetCSSplitTargetGroups( csUniqueID, cb, groups ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get splitting "
                      "group list of collection space [%u]: rc: %d",
                      csUniqueID, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATGETCSGRPS_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGETCLCNTFROMCS_UID, "_catGetCLCountFromCS" )
   static INT32 _catGetCLCountFromCS( utilCSUniqueID csUniqueID,
                                      pmdEDUCB *cb,
                                      UINT32 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGETCLCNTFROMCS_UID ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      INT64 curCount = 0, recycledCount = 0 ;
      rtnQueryOptions queryOptions ;
      BSONObj matcher ;

      rc = utilGetCSBounds( CAT_CL_UNIQUEID, csUniqueID, matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] matcher for "
                   "collection space [%u], rc: %d", CAT_CL_UNIQUEID,
                   csUniqueID, rc ) ;
      queryOptions.setQuery( matcher ) ;

      queryOptions.setCLFullName( CAT_COLLECTION_INFO_COLLECTION ) ;
      rc = rtnGetCount( queryOptions, dmsCB, cb, rtnCB, &curCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query collection [%s], "
                   "rc: %d", CAT_COLLECTION_INFO_COLLECTION, rc ) ;

      // check recycle bin
      queryOptions.setCLFullName( CAT_SYSRECYCLEBIN_CL_COLLECTION ) ;
      rc = rtnGetCount( queryOptions, dmsCB, cb, rtnCB, &curCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query collection [%s], "
                   "rc: %d", CAT_COLLECTION_INFO_COLLECTION, rc ) ;

      count = (UINT32)curCount + (UINT32)recycledCount ;

   done:
      PD_TRACE_EXITRC( SDB__CATGETCLCNTFROMCS_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHKCSCAP_UID, "catCheckCSCapacity" )
   INT32 catCheckCSCapacity( utilCSUniqueID csUniqueID,
                             UINT32 newAdding,
                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCHKCSCAP_UID ) ;

      UINT32 currentCount = 0 ;

      rc = _catGetCLCountFromCS( csUniqueID, cb, currentCount ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection count from "
                   "collection space [%u], rc: %d", csUniqueID, rc ) ;

      PD_CHECK( currentCount + newAdding <= DMS_MME_SLOTS,
                SDB_DMS_NOSPC, error, PDERROR,
                "Failed to check capacity of collection space, current "
                "[%u], adding [%u]", currentCount, newAdding ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCHKCSCAP_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETINCIDXUID, "catGetAndIncIdxUniqID" )
   INT32 catGetAndIncIdxUniqID( const CHAR* collection, pmdEDUCB *cb, INT16 w,
                                UINT64& idxUniqID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATGETINCIDXUID ) ;

      SDB_ASSERT( collection, "collection shouldn't be null" ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      try
      {
         BSONObj dummy, result, matcher, updator ;
         UINT64 idxUniqHWM = 0 ;
         CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

         // get cs name
         rc = rtnResolveCollectionSpaceName( collection,
                                             ossStrlen( collection ),
                                             csName,
                                             DMS_COLLECTION_SPACE_NAME_SZ ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get cs name from cl name[%s],rc: %d",
                      collection, rc ) ;

         // get index uniqueID hwm
         matcher = BSON( FIELD_NAME_NAME << csName ) ;

         rc = catGetOneObj( CAT_COLLECTION_SPACE_COLLECTION, dummy, matcher,
                            dummy, cb, result ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to match obj[%s] from collection[%s], rc: %d",
                      matcher.toString().c_str(),
                      CAT_COLLECTION_SPACE_COLLECTION, rc ) ;

         rc = rtnGetNumberLongElement( result, FIELD_NAME_IDXUNIQUEHWM,
                                       (INT64&)idxUniqHWM ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
            UINT32 csUniqID = 0 ;
            rc = rtnGetIntElement( result, FIELD_NAME_UNIQUEID,
                                   (INT32&)csUniqID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s]",
                         FIELD_NAME_UNIQUEID, result.toString().c_str(), rc ) ;
            idxUniqHWM = utilBuildIdxUniqueID( csUniqID, 1 ) ;
         }
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s]",
                      FIELD_NAME_IDXUNIQUEHWM, result.toString().c_str(), rc ) ;

         // check index unique id
         if ( utilGetIdxInnerID( idxUniqHWM ) > UTIL_IDXINNERID_MAX )
         {
            rc = SDB_IDX_UNIQUEID_EXCEEDED ;
            PD_LOG( PDERROR, "Index "
                    "inner id can't exceed %u, idx unique id: %llu, rc: %d",
                    UTIL_IDXINNERID_MAX, idxUniqID, rc ) ;
            goto error ;
         }
         else if ( utilGetIdxInnerID( idxUniqHWM ) == UTIL_UNIQUEID_NULL )
         {
            ++ idxUniqHWM ;
         }
         idxUniqID = idxUniqHWM ;

         // set index unique hwm
         updator = BSON( "$set" << BSON( FIELD_NAME_IDXUNIQUEHWM <<
                                         (INT64)(++idxUniqHWM) ) );

         rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, matcher, updator,
                         dummy, 0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Fail to update obj[%s] in collection[%s], rc: %d",
                      updator.toString().c_str(),
                      CAT_COLLECTION_SPACE_COLLECTION, rc ) ;

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATGETINCIDXUID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATADDIDX, "catAddIndex" )
   INT32 catAddIndex( const CHAR *collectionName, const BSONObj &indexDef,
                      pmdEDUCB *cb, INT16 w,
                      BOOLEAN *pAddNewIdx, BSONObj *pIndexObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATADDIDX ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;
      BSONObj obj, boCollection ;
      utilCLUniqueID clUniqID = UTIL_UNIQUEID_NULL ;

      if ( pAddNewIdx )
      {
         *pAddNewIdx = FALSE ;
      }

      // get collection unique id
      rc = catGetCollection( collectionName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection[%s], rc: %d",
                   collectionName, rc ) ;

      rc = rtnGetNumberLongElement( boCollection, FIELD_NAME_UNIQUEID,
                                    (INT64&)clUniqID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection[%s]'s unique id, rc: %d",
                   collectionName, rc ) ;

      // format index info
      rc = catFormatIndexInfo( collectionName, clUniqID, indexDef, cb, obj ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to convert[%s] to index obj, rc: %d",
                   indexDef.toString().c_str(), rc ) ;

      rc = rtnInsert( CAT_INDEX_INFO_COLLECTION, obj, 1, 0, cb, dmsCB,
                      dpsCB, w ) ;
      if ( SDB_OK == rc )
      {
         if ( pAddNewIdx )
         {
            *pAddNewIdx = TRUE ;
         }
      }
      else if ( SDB_IXM_DUP_KEY == rc )
      {
         BOOLEAN isExist = FALSE ;
         BOOLEAN isSameDef = FALSE ;
         INT32 rcTmp = catCheckIndexExist( collectionName, indexDef, cb,
                                           isExist, isSameDef ) ;
         if ( SDB_OK == rcTmp && isSameDef )
         {
            rc = SDB_OK ;
         }
         else
         {
            rc = SDB_IXM_EXIST ;
         }
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed insert obj[%s] to collection[%s]",
                   obj.toString().c_str(), CAT_INDEX_INFO_COLLECTION ) ;

      if ( pIndexObj )
      {
         *pIndexObj = obj.getOwned() ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATADDIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRMIDX, "catRemoveIndex" )
   INT32 catRemoveIndex( const CHAR *collection, const CHAR *indexName,
                         pmdEDUCB *cb, INT16 w, BOOLEAN *pRemoveOldIdx )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRMIDX ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      utilDeleteResult result ;
      BSONObj dummyObj ;
      BSONObj boMatcher = BSON( FIELD_NAME_COLLECTION << collection <<
                                FIELD_NAME_NAME << indexName ) ;

      if ( pRemoveOldIdx )
      {
         *pRemoveOldIdx = FALSE ;
      }

      rc = rtnDelete( CAT_INDEX_INFO_COLLECTION, boMatcher, dummyObj,
                      0, cb, dmsCB, dpsCB, w, &result ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove index, collection: %s, index: %s, rc: %d",
                   collection, indexName, rc ) ;

      if ( pRemoveOldIdx && result.deletedNum() > 0 )
      {
         *pRemoveOldIdx = TRUE ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATRMIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRMCLIDX, "catRemoveCLIndexes" )
   INT32 catRemoveCLIndexes( const CHAR *collection, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRMCLIDX ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      BSONObj dummyObj ;
      BSONObj boMatcher ;

      try
      {
         boMatcher = BSON( FIELD_NAME_COLLECTION << collection ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = rtnDelete( CAT_INDEX_INFO_COLLECTION, boMatcher, dummyObj,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove collection[%s]'s indexes, rc: %d",
                   collection, rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATRMCLIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRMCSIDX, "catRemoveCSIndexes" )
   INT32 catRemoveCSIndexes( const CHAR *csName, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRMCSIDX ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      BSONObj dummyObj ;
      BSONObj matcher ;

      rc = _catBuildCLMatcher( FIELD_NAME_COLLECTION, csName, matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build matcher, rc: %d", rc ) ;

      rc = rtnDelete( CAT_INDEX_INFO_COLLECTION, matcher, dummyObj,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to remove collection space[%s]'s indexes, rc: %d",
                   csName, rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATRMCSIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHKIDXEXIST, "catCheckIndexExist" )
   INT32 catCheckIndexExist( const CHAR *collection, const BSONObj &indexDef,
                             pmdEDUCB *cb,
                             BOOLEAN &isExist, BOOLEAN &isSameDef )
   {
      PD_TRACE_ENTRY( SDB_CATCHKIDXEXIST ) ;

      INT32 rc = SDB_OK ;
      BSONObj obj ;
      const CHAR* indexName = indexDef.getStringField( IXM_FIELD_NAME_NAME ) ;

      isExist = FALSE ;
      isSameDef = FALSE ;

      rc = catGetIndex( collection, indexName, cb, obj ) ;
      if ( SDB_IXM_NOTEXIST == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection[%s]'s index[%s], rc: %d",
                   collection, indexName, rc ) ;

      isExist = TRUE ;

      if ( ixmIsSameDef( obj.getObjectField( IXM_FIELD_NAME_INDEX_DEF ),
                         indexDef,
                         TRUE ) )
      {
         isSameDef = TRUE ;
      }
      else
      {
         isSameDef = FALSE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCHKIDXEXIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CHKIDXEX2, "catCheckIndexExist" )
   INT32 catCheckIndexExist( const CHAR* collection, const CHAR* indexName,
                             const BSONObj& indexDef, _pmdEDUCB* cb,
                             BOOLEAN skipShardIdx,
                             BSONObj* pIndexObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CHKIDXEX2 ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      BSONObj dummyObj ;
      BSONObj matcher = BSON( FIELD_NAME_COLLECTION << collection ) ;

      rc = rtnQuery( CAT_INDEX_INFO_COLLECTION,
                     dummyObj, matcher, dummyObj, dummyObj,
                     0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc: %d", rc ) ;

      while ( TRUE )
      {
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to retreive record, rc: %d", rc ) ;

         BSONObj obj( contextBuf.data() ) ;
         BSONObj def = obj.getObjectField( IXM_FIELD_NAME_INDEX_DEF ) ;
         const CHAR* name = def.getStringField( IXM_NAME_FIELD ) ;

         if ( skipShardIdx && 0 == ossStrcmp( name, IXM_SHARD_KEY_NAME ) )
         {
            // skip $shard index
            continue ;
         }

         if ( 0 == ossStrcmp( indexName, name ) )
         {
            if ( ixmIsSameDef( def, indexDef, TRUE ) )
            {
               rc = SDB_IXM_REDEF ;
               PD_LOG_MSG( PDERROR,
                           "The same index '%s' has been defined already",
                           indexName ) ;
               if ( pIndexObj )
               {
                  *pIndexObj = obj.getOwned() ;
               }
               goto error ;
            }
            else
            {
               rc = SDB_IXM_EXIST ;
               PD_LOG_MSG( PDERROR,
                           "The existing index '%s' has the same name "
                           "but with a different definition", indexName ) ;
               goto error ;
            }
         }
         else
         {
            if ( ixmIsSameDef( def, indexDef ) )
            {
               rc = SDB_IXM_EXIST_COVERD_ONE ;
               PD_LOG_MSG( PDERROR,
                           "The scene of index '%s' is covered by "
                           "the existing index '%s'", indexName, name );
               goto error ;
            }
            else
            {
               continue ;
            }
         }
      }

   done:
      if ( contextID != -1 )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC( SDB_CHKIDXEX2, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETIDX, "catGetIndex" )
   INT32 catGetIndex( const CHAR *collection, const CHAR *indexName,
                      pmdEDUCB *cb, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATGETIDX ) ;

      BSONObj dummyObj ;
      BSONObj boMatcher = BSON( FIELD_NAME_COLLECTION << collection <<
                                FIELD_NAME_NAME << indexName ) ;

      rc = catGetOneObj( CAT_INDEX_INFO_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR,
                 "Failed to get obj(%s) from %s, rc: %d",
                 boMatcher.toString().c_str(), CAT_INDEX_INFO_COLLECTION, rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCLIDX, "catGetCLIndexes" )
   INT32 catGetCLIndexes( const CHAR *collection,
                          BOOLEAN onlyGlobaIndex,
                          pmdEDUCB *cb,
                          ossPoolVector<BSONObj>& indexes )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATGETCLIDX ) ;

      BSONObj matcher ;
      BSONObj dummyObj ;
      SDB_DMSCB * dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB * rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;

      try
      {

      // only query collection's global indexes or all indexes
      if ( onlyGlobaIndex )
      {
         matcher = BSON( FIELD_NAME_COLLECTION <<
                         collection <<
                         IXM_FIELD_NAME_INDEX_DEF "." IXM_FIELD_NAME_GLOBAL <<
                         true ) ;
      }
      else
      {
         matcher = BSON( FIELD_NAME_COLLECTION << collection ) ;
      }

      rc = rtnQuery( CAT_INDEX_INFO_COLLECTION,
                     dummyObj, matcher, dummyObj, dummyObj,
                     0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Query collection[%s] failed, matcher: %s, rc: %d",
                   CAT_INDEX_INFO_COLLECTION, matcher.toString().c_str(), rc ) ;

      // Get more
      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         indexes.push_back( BSONObj( contextBuf.data() ).getOwned() ) ;
      }

      }
      catch( std::bad_alloc )
      {
         PD_RC_CHECK( SDB_OOM, PDERROR, "Exception occurred: out of memory" ) ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done :
      if ( contextID != -1 )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATGETCLIDX, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETGIDXINFO, "catGetGlobalIndexInfo" )
   INT32 catGetGlobalIndexInfo( const CHAR *collection, const CHAR *indexName,
                                pmdEDUCB *cb, BOOLEAN &isGlobalIndex,
                                string &indexCLName,
                                utilCLUniqueID &indexCLUID )
   {
      PD_TRACE_ENTRY ( SDB_CATGETGIDXINFO ) ;

      INT32 rc = SDB_OK ;
      BSONObj obj, indexDef, globalOpt ;

      try
      {

      rc = catGetIndex( collection, indexName, cb, obj ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get index[%s] of collection[%s], rc: %d",
                   indexName, collection, rc ) ;

      rc = rtnGetObjElement( obj, IXM_FIELD_NAME_INDEX_DEF, indexDef ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   IXM_FIELD_NAME_INDEX_DEF, obj.toString().c_str(), rc ) ;

      rc = rtnGetObjElement( indexDef, IXM_FIELD_NAME_GLOBAL_OPTION,
                             globalOpt ) ;
      if ( SDB_OK == rc )
      {
         isGlobalIndex = TRUE ;
      }
      else if ( SDB_FIELD_NOT_EXIST == rc )
      {
         isGlobalIndex = FALSE ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   IXM_FIELD_NAME_GLOBAL_OPTION,
                   indexDef.toString().c_str(), rc ) ;

      if ( !isGlobalIndex )
      {
         goto done ;
      }

      rc = rtnGetSTDStringElement( globalOpt, CAT_COLLECTION, indexCLName ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   CAT_COLLECTION, globalOpt.toString().c_str(), rc ) ;

      rc = rtnGetNumberLongElement( globalOpt, CAT_GIDX_CL_UNIQUEID,
                                    (INT64&)indexCLUID ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get field[%s] from obj[%s], rc: %d",
                   CAT_GIDX_CL_UNIQUEID, globalOpt.toString().c_str(), rc ) ;

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATGETGIDXINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catGetCLGlobalIndexesInfo( const CHAR *collection, pmdEDUCB *cb,
                                    CAT_PAIR_CLNAME_ID_LIST& indexCLList )
   {
      INT32 rc = SDB_OK ;
      ossPoolVector<BSONObj> indexObjList ;

      try
      {

      rc = catGetCLIndexes( collection, TRUE, cb, indexObjList ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get collection[%s]'s indexes, rc: %d",
                   collection, rc ) ;

      for( ossPoolVector<BSONObj>::iterator it = indexObjList.begin() ;
           it != indexObjList.end() ; it++ )
      {
         BSONObj indexDef, globalOpt ;
         const CHAR* indexCLName = NULL ;
         utilCLUniqueID indexCLUID = UTIL_UNIQUEID_NULL ;
         PAIR_CLNAME_ID indexCLPair ;

         rc = rtnGetObjElement( *it, IXM_FIELD_NAME_INDEX_DEF, indexDef ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s], rc: %d",
                      IXM_FIELD_NAME_INDEX_DEF, it->toString().c_str(), rc ) ;

         rc = rtnGetObjElement( indexDef, IXM_FIELD_NAME_GLOBAL_OPTION,
                                globalOpt ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s], rc: %d",
                      IXM_FIELD_NAME_GLOBAL_OPTION,
                      indexDef.toString().c_str(), rc ) ;

         rc = rtnGetStringElement( globalOpt, CAT_COLLECTION, &indexCLName ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s], rc: %d",
                      CAT_COLLECTION, globalOpt.toString().c_str(), rc ) ;

         rc = rtnGetNumberLongElement( globalOpt, CAT_GIDX_CL_UNIQUEID,
                                       (INT64&)indexCLUID ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get field[%s] from obj[%s], rc: %d",
                      CAT_GIDX_CL_UNIQUEID, globalOpt.toString().c_str(), rc ) ;

         indexCLPair = std::make_pair( indexCLName, indexCLUID ) ;
         indexCLList.push_back( indexCLPair ) ;
      }

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRENAMECLIDX, "catRenameCLInIndexes" )
   INT32 catRenameCLInIndexes( const CHAR *clFullName,
                               const CHAR *newCLFullName,
                               pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRENAMECLIDX ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      BSONObj dummy ;
      BSONObj match = BSON( FIELD_NAME_COLLECTION << clFullName ) ;
      BSONObj updator = BSON( "$set" <<
                              BSON( FIELD_NAME_COLLECTION << newCLFullName ) ) ;

      rc = rtnUpdate( CAT_INDEX_INFO_COLLECTION,
                      match, updator, dummy,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update index of collection[%s], updator[%s], "
                   "rc: %d", clFullName, updator.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATRENAMECLIDX, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATADDTASK, "catAddTask" )
   INT32 catAddTask( const BSONObj & taskObj, pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATADDTASK ) ;
      rc = rtnInsert( CAT_TASK_INFO_COLLECTION, taskObj, 1, 0, cb, dmsCB,
                      dpsCB, w ) ;

      if ( rc )
      {
         if ( SDB_IXM_DUP_KEY == rc )
         {
            rc = SDB_TASK_EXIST ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed insert obj[%s] to collection[%s]",
                    taskObj.toString().c_str(), CAT_TASK_INFO_COLLECTION ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATADDTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETTASK, "catGetTask" )
   INT32 catGetTask( UINT64 taskID, BSONObj & obj, pmdEDUCB * cb )
   {
      INT32 rc           = SDB_OK;

      PD_TRACE_ENTRY ( SDB_CATGETTASK ) ;
      BSONObj dummyObj ;
      BSONObj boMatcher = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;

      rc = catGetOneObj( CAT_TASK_INFO_COLLECTION, dummyObj, boMatcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CAT_TASK_NOTFOUND ;
         goto error ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 boMatcher.toString().c_str(), CAT_TASK_INFO_COLLECTION, rc ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATGETTASK, rc ) ;
      return rc;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSTASKCNTBYTYPE, "catGetCSTaskCountByType" )
   INT32 catGetCSTaskCountByType( const CHAR *csName, CLS_TASK_TYPE type,
                                  pmdEDUCB *cb, INT64 &count )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATGETCSTASKCNTBYTYPE ) ;

      SDB_ASSERT( NULL != csName, "cs is invalid" ) ;

      BSONObj dummyObj, matcher ;

      try
      {
         BSONObjBuilder builder ;

         rc = _catBuildCLMatcher( CAT_COLLECTION_NAME, csName, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build matcher, rc: %d", rc ) ;

         builder.append( CAT_TASKTYPE_NAME, type ) ;
         builder.append( CAT_STATUS_NAME,
                         BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = catGetObjectCount( CAT_TASK_INFO_COLLECTION, dummyObj, matcher,
                              dummyObj, cb, count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get task count of cs[%s], "
                   "rc: %d", csName, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATGETCSTASKCNTBYTYPE, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETTASKCOUNTBYTYPE, "catGetCLTaskCountByType" )
   INT32 catGetCLTaskCountByType( const CHAR *collection, CLS_TASK_TYPE type,
                                  pmdEDUCB *cb, INT64 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETTASKCOUNTBYTYPE ) ;
      SDB_ASSERT( NULL != collection, "Collection is invalid" ) ;
      SDB_ASSERT( CLS_TASK_UNKNOWN != type, "Task type is invalid" ) ;

      BSONObj dummy ;
      BSONObj matcher = BSON( CAT_COLLECTION_NAME <<
                              collection <<
                              CAT_TASKTYPE_NAME <<
                              type <<
                              CAT_STATUS_NAME <<
                              BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;

      rc = catGetObjectCount( CAT_TASK_INFO_COLLECTION, dummy, matcher, dummy,
                              cb, count ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get task count for collection [%s]",
                   "rc: %d", collection, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETTASKCOUNTBYTYPE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSSPLITTARGETGRPS, "catGetCSSplitTargetGroups" )
   INT32 catGetCSSplitTargetGroups( utilCSUniqueID csUniqueID,
                                    pmdEDUCB * cb,
                                    ossPoolSet< UINT32 > & groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCSSPLITTARGETGRPS ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      BSONObj matcher ;
      rtnQueryOptions queryOptions ;

      try
      {
         BSONObjBuilder builder ;

         // status
         BSONObjBuilder statusBuilder(
                                 builder.subobjStart( CAT_STATUS_NAME ) ) ;
         statusBuilder.append( "$ne", (INT32)CLS_TASK_STATUS_FINISH ) ;
         statusBuilder.doneFast() ;

         // cs unique ID bound
         rc = utilGetCSBounds( CAT_CL_UNIQUEID, csUniqueID, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] matcher for "
                      "collection space [%u], rc: %d", CAT_CS_UNIQUEID,
                      csUniqueID, rc ) ;

         // split task
         builder.append( CAT_TASKTYPE_NAME, (INT32)CLS_TASK_SPLIT ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      queryOptions.setCLFullName( CAT_TASK_INFO_COLLECTION ) ;
      queryOptions.setQuery( matcher ) ;

      rc = rtnQuery( queryOptions, cb, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, "
                   "rc: %d", CAT_TASK_INFO_COLLECTION, rc ) ;

      // get more
      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement ele = obj.getField( CAT_TARGETID_NAME ) ;
            groups.insert( ele.numberInt() ) ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Get group id from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

   done :
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB_CATGETCSSPLITTARGETGRPS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGETTASKS, "_catGetTasks" )
   static INT32 _catGetTasks( const BSONObj &matcher,
                              pmdEDUCB *cb,
                              ossPoolSet< UINT64 > &tasks )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGETTASKS ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      rtnQueryOptions queryOptions ;

      queryOptions.setCLFullName( CAT_TASK_INFO_COLLECTION ) ;
      queryOptions.setQuery( matcher ) ;

      rc = rtnQuery( queryOptions, cb, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, "
                   "rc: %d", CAT_TASK_INFO_COLLECTION, rc ) ;

      // get more
      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement ele = obj.getField( CAT_TASKID_NAME ) ;
            tasks.insert( ele.numberInt() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Get task ID, occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      if ( -1 != contextID )
      {
         rtnKillContexts( 1, &contextID, cb, rtnCB ) ;
         contextID = -1 ;
      }
      PD_TRACE_EXITRC( SDB__CATGETTASKS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCLTASKSBYTYPE, "catGetCLTaskByType" )
   INT32 catGetCLTaskByType( const CHAR *clName,
                             CLS_TASK_TYPE type,
                             pmdEDUCB *cb,
                             ossPoolSet< UINT64 > &tasks )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCLTASKSBYTYPE ) ;

      BSONObj matcher ;

      try
      {
         BSONObjBuilder builder ;

         builder.append( CAT_COLLECTION_NAME, clName ) ;
         builder.append( CAT_TASKTYPE_NAME, type ) ;
         builder.append( CAT_STATUS_NAME,
                         BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _catGetTasks( matcher, cb, tasks ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get tasks for collection "
                   "[%s], rc: %d", clName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETCLTASKSBYTYPE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSTASKSBYTYPE, "catGetCSTaskByType" )
   INT32 catGetCSTaskByType( const CHAR *csName,
                             CLS_TASK_TYPE type,
                             pmdEDUCB *cb,
                             ossPoolSet< UINT64 > &tasks )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCSTASKSBYTYPE ) ;

      BSONObj matcher ;

      try
      {
         BSONObjBuilder builder ;

         rc = _catBuildCLMatcher( CAT_COLLECTION_NAME, csName, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] matcher for "
                      "collection space [%s], rc: %d", CAT_CS_UNIQUEID,
                      csName, rc ) ;

         builder.append( CAT_TASKTYPE_NAME, type ) ;
         builder.append( CAT_STATUS_NAME,
                         BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _catGetTasks( matcher, cb, tasks ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get tasks for collection space "
                   "[%s], rc: %d", csName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETCSTASKSBYTYPE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCLTASKS, "catGetCLTasks" )
   INT32 catGetCLTasks( const CHAR *clName,
                        pmdEDUCB *cb,
                        ossPoolSet< UINT64 > &tasks )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCLTASKS ) ;

      // get all running tasks of given collection

      BSONObj matcher ;

      try
      {
         BSONObjBuilder builder ;

         builder.append( CAT_COLLECTION_NAME, clName ) ;
         builder.append( CAT_STATUS_NAME,
                         BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _catGetTasks( matcher, cb, tasks ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get tasks for collection "
                   "[%s], rc: %d", clName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETCLTASKS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCSTASKS, "catGetCSTasks" )
   INT32 catGetCSTasks( const CHAR *csName,
                        pmdEDUCB *cb,
                        ossPoolSet< UINT64 > &tasks )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCSTASKS ) ;

      // get all running tasks of given collection space

      BSONObj matcher ;

      try
      {
         BSONObjBuilder builder ;

         rc = _catBuildCLMatcher( CAT_COLLECTION_NAME, csName, builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build [%s] matcher for "
                      "collection space [%s], rc: %d", CAT_CS_UNIQUEID,
                      csName, rc ) ;

         builder.append( CAT_STATUS_NAME,
                         BSON( "$ne" << CLS_TASK_STATUS_FINISH ) ) ;

         matcher = builder.obj() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _catGetTasks( matcher, cb, tasks ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get tasks for collection space "
                   "[%s], rc: %d", csName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETCSTASKS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETTASKSTATUS, "catGetTaskStatus" )
   INT32 catGetTaskStatus( UINT64 taskID, INT32 & status, pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj taskObj ;
      PD_TRACE_ENTRY ( SDB_CATGETTASKSTATUS ) ;
      rc = catGetTask( taskID, taskObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Get task[%lld] failed, rc: %d", taskID, rc ) ;

      rc = rtnGetIntElement( taskObj, CAT_STATUS_NAME, status ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   CAT_STATUS_NAME, rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATGETTASKSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETMAXTASKID, "catGetCurrentMaxTaskID" )
   UINT64 catGetCurrentMaxTaskID( pmdEDUCB * cb )
   {
      UINT64 taskID           = CLS_INVALID_TASKID ;
      INT32 rc                = SDB_OK ;
      SINT64 contextID        = -1 ;
      pmdKRCB *pKRCB          = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB        = pKRCB->getDMSCB() ;
      SDB_RTNCB *rtnCB        = pKRCB->getRTNCB() ;

      PD_TRACE_ENTRY ( SDB_CATGETMAXTASKID ) ;
      BSONObj dummyObj ;
      BSONObj orderby = BSON( CAT_TASKID_NAME << -1 ) ;

      rtnContextBuf buffObj ;

      // query
      rc = rtnQuery( CAT_TASK_INFO_COLLECTION, dummyObj, dummyObj, orderby,
                     dummyObj, 0, cb, 0, 1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to query from %s, rc: %d",
                   CAT_TASK_INFO_COLLECTION, rc ) ;

      // get more
      rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
         }
         goto error ;
      }

      // copy obj
      try
      {
         BSONObj resultObj( buffObj.data() ) ;
         BSONElement ele = resultObj.getField( CAT_TASKID_NAME ) ;
         if ( !ele.isNumber() )
         {
            PD_LOG( PDWARNING, "Failed to get field[%s], type: %d",
                    CAT_TASKID_NAME, ele.type() ) ;
            goto error ;
         }
         taskID = (UINT64)ele.numberLong() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         buffObj.release() ;
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATGETMAXTASKID, rc ) ;
      return taskID ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATETASK, "catUpdateTask" )
   INT32 catUpdateTask( UINT64 taskID, const BSONObj *pSetInfo,
                        const BSONObj *pUnsetInfo, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATUPDATETASK ) ;

      BSONObj match = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
      BSONObjBuilder builder ;
      if ( pSetInfo )
      {
         builder.append( "$set", *pSetInfo ) ;
      }
      if ( pUnsetInfo )
      {
         builder.append( "$unset", *pUnsetInfo ) ;
      }

      rc = catUpdateTask( match, builder.done(), cb, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update task[%llu], rc: %d",
                   taskID, rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATUPDATETASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATETASK1, "catUpdateTask" )
   INT32 catUpdateTask( const BSONObj &matcher, const BSONObj &updator,
                        pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATUPDATETASK1 ) ;

      if ( !updator.isEmpty() )
      {
         pmdKRCB *krcb = pmdGetKRCB() ;
         SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
         SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
         BSONObj dummy;

         rc = rtnUpdate( CAT_TASK_INFO_COLLECTION, matcher, updator, dummy, 0,
                         cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task, matcher: %s, updator: %s, rc: %d",
                      matcher.toString().c_str(), updator.toString().c_str(),
                      rc ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATUPDATETASK1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catUpdateTaskStatus( UINT64 taskID, CLS_TASK_STATUS status,
                              pmdEDUCB * cb, INT16 w )
   {
      BSONObj setInfo = BSON( FIELD_NAME_STATUS << status <<
                              FIELD_NAME_STATUSDESC <<
                              clsTaskStatusStr( status ) ) ;
      return catUpdateTask( taskID, &setInfo, NULL, cb, w ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPTASKFNS, "catUpdateTask2Finish" )
   INT32 catUpdateTask2Finish( UINT64 taskID, INT32 resultCode,
                               pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATUPTASKFNS ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      ossTimestamp ts ;
      ossGetCurrentTime( ts ) ;
      CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
      ossTimestampToString( ts, timeStr ) ;

      BSONObj match = BSON( FIELD_NAME_TASKID << (INT64)taskID ) ;

      BSONObjBuilder builder ;
      builder.append( FIELD_NAME_STATUS, CLS_TASK_STATUS_FINISH ) ;
      builder.append( FIELD_NAME_STATUSDESC, VALUE_NAME_FINISH ) ;
      builder.append( FIELD_NAME_RESULTCODE, resultCode ) ;
      builder.append( FIELD_NAME_RESULTCODEDESC, getErrDesp( resultCode ) ) ;
      builder.append( FIELD_NAME_ENDTIMESTAMP, timeStr ) ;
      BSONObj updator = BSON( "$set" << builder.done() ) ;

      rc = rtnUpdate( CAT_TASK_INFO_COLLECTION, match, updator, BSONObj(), 0,
                      cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to update task[%llu], match[%s] updator[%s], rc: %d",
                   taskID, match.toString().c_str(),
                   updator.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATUPTASKFNS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATREMOVETASK, "catRemoveTask" )
   INT32 catRemoveTask( BSONObj & match, BOOLEAN checkExist, pmdEDUCB * cb,
                        INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATREMOVETASK ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      BSONObj taskObj ;
      BSONObj dummyObj ;
      utilDeleteResult delResult ;

      if ( checkExist )
      {
         rc = catGetOneObj( CAT_TASK_INFO_COLLECTION, dummyObj, match,
                            dummyObj, cb, taskObj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CAT_TASK_NOTFOUND ;
            goto error ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                    match.toString().c_str(), CAT_TASK_INFO_COLLECTION, rc ) ;
            goto error ;
         }
      }

      rc = rtnDelete( CAT_TASK_INFO_COLLECTION, match, dummyObj, 0, cb,
                      dmsCB, dpsCB, w, &delResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove task from collection[%s], "
                   "rc: %d, del cond: %s", CAT_TASK_INFO_COLLECTION, rc,
                   match.toString().c_str() ) ;
      PD_LOG( PDINFO, "Removed %llu tasks for [%s]", delResult.deletedNum(),
              match.toString().c_str() ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATREMOVETASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catRemoveTask( UINT64 taskID, BOOLEAN checkExist, pmdEDUCB *cb,
                        INT16 w )
   {
      BSONObj matcher = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
      return catRemoveTask( matcher, checkExist, cb, w ) ;
   }

   static INT32 _queryTask( const BSONObj &matcher, pmdEDUCB *cb,
                            BOOLEAN &hasTask,
                            ossPoolSet<UINT64> &mainTaskSet,
                            ossPoolMap<UINT64,UINT64> &subTaskMap )
   {
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      BSONObj dummyObj ;

      hasTask = FALSE ;
      mainTaskSet.clear() ;
      subTaskMap.clear() ;

      try
      {
         rc = rtnQuery( CAT_TASK_INFO_COLLECTION, dummyObj, matcher, dummyObj,
                        dummyObj, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to perform query, rc: %d",
                       rc ) ;

         while ( TRUE )
         {
            rtnContextBuf contextBuf ;
            BOOLEAN isMainTask = FALSE ;
            BOOLEAN isSubTask = FALSE ;
            UINT64 taskID = CLS_INVALID_TASKID ;
            UINT64 mainTaskID = CLS_INVALID_TASKID ;

            rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               contextID = -1 ;
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get more, rc: %d", rc ) ;

            BSONObj obj( contextBuf.data() ) ;
            hasTask = TRUE ;

            rc = rtnGetBooleanElement( obj, FIELD_NAME_IS_MAINTASK,
                                       isMainTask ) ;

            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_IS_MAINTASK, obj.toString().c_str(), rc ) ;

            if ( !isMainTask )
            {
               rc = rtnGetNumberLongElement( obj, FIELD_NAME_MAIN_TASKID,
                                             (INT64&)mainTaskID ) ;
               if ( SDB_FIELD_NOT_EXIST == rc )
               {
                  rc = SDB_OK ;
               }
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to get field[%s] from obj[%s], rc: %d",
                            FIELD_NAME_MAIN_TASKID, obj.toString().c_str(), rc ) ;
               if ( mainTaskID != CLS_INVALID_TASKID )
               {
                  isSubTask = TRUE ;
               }
            }

            if ( !isMainTask && !isSubTask )
            {
               continue ;
            }

            rc = rtnGetNumberLongElement( obj, FIELD_NAME_TASKID,
                                          (INT64&)taskID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s], rc: %d",
                         FIELD_NAME_TASKID, obj.toString().c_str(), rc ) ;

            if ( isMainTask )
            {
               mainTaskSet.insert( taskID ) ;
            }
            if ( isSubTask )
            {
               subTaskMap[ taskID ] = mainTaskID ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      if ( contextID != -1 )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   static INT32 _updateMainTaskByRemoveTask( UINT64 mainTaskID,
                                             UINT64 subTaskID, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj taskObj, matcher1, matcher2, updator, selector, dummyObj ;
      clsTask *pMainTask = NULL ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      ossPoolVector<BSONObj> subTaskVec ;
      catCMDBase *pCommand = NULL ;

      try
      {
         /// 1. get main-task object from SYSTASKS
         rc = catGetTask( mainTaskID, taskObj, cb ) ;
         if ( SDB_CAT_TASK_NOTFOUND == rc )
         {
            // main-collection and sub-collection belongs to the same cs, so
            // main task has been deleted
            rc = SDB_OK ;
            goto done ;
         }
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to get task[%llu], rc: %d",
                       mainTaskID, rc ) ;

         /// 2. new and init main-task
         rc = clsNewTask( taskObj, pMainTask ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to new task, rc: %d",
                      rc ) ;

         /// 3. update main-task progress
         if ( CLS_TASK_STATUS_FINISH == pMainTask->status() )
         {
            PD_LOG( PDDEBUG, "Task[%llu] is already finished", mainTaskID ) ;
            goto done ;
         }

         rc = pMainTask->buildQuerySubTasks( BSONObj(), matcher1, selector ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to get subtask's matcher and selector, rc: %d",
                      rc ) ;

         // query sub-task info
         if ( !matcher1.isEmpty() )
         {
            rc = rtnQuery( CAT_TASK_INFO_COLLECTION, selector, matcher1,
                           dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                           contextID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Query collection[%s] failed: matcher=%s, rc=%d",
                         CAT_TASK_INFO_COLLECTION, matcher1.toString().c_str(),
                         rc ) ;
         }

         // get more
         while ( TRUE )
         {
            rtnContextBuf contextBuf ;
            rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

            subTaskVec.push_back( BSONObj( contextBuf.data() ).getOwned() ) ;
         }

         rc = pMainTask->buildRemoveTaskBy( subTaskID, subTaskVec,
                                            updator, matcher2 ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to remove sub task[%llu], rc: %d",
                      subTaskID, rc ) ;

         if ( updator.isEmpty() )
         {
            // nothing changed, just goto done
            goto done ;
         }

         rc = catUpdateTask( matcher2, updator, cb, 1 ) ;
         PD_RC_CHECK ( rc, PDERROR,
                       "Failed to update task, rc: %d",
                       rc ) ;

         /// 4. if task finish, then we may update metadata
         if ( CLS_TASK_STATUS_FINISH == pMainTask->status() )
         {
            const CHAR* commandName = pMainTask->commandName() ;
            if ( commandName )
            {
               rc = getCatCmdBuilder()->create( commandName, pCommand ) ;
               PD_RC_CHECK ( rc, PDERROR,
                             "Failed to create command[%s], rc: %d",
                             commandName, rc ) ;

               rc = pCommand->postDoit( pMainTask, cb ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Failed to post doit for command[%s], rc: %d",
                            commandName, rc ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      if ( contextID != -1 )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      if ( pCommand )
      {
         getCatCmdBuilder()->release( pCommand ) ;
      }
      if ( pMainTask )
      {
         clsReleaseTask( pMainTask ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 catRemoveCLTasks( const CHAR *clName, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj dummyObj, matcher ;
      BOOLEAN hasTask = FALSE ;
      ossPoolSet<UINT64> mainTaskSet ;
      ossPoolMap<UINT64,UINT64> subTaskMap ; // <taskID, mainTaskID>

      try
      {
         matcher = BSON( CAT_COLLECTION_NAME << clName ) ;

         rc = _queryTask( matcher, cb, hasTask, mainTaskSet, subTaskMap ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to query task by matcher, rc: %d",
                      rc ) ;

         if ( hasTask )
         {
            rc = catRemoveTask( matcher, FALSE, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to remove task by matcher[%s], rc: %d",
                         matcher.toString().c_str(), rc ) ;
         }

         // if the task to be deleted is main task, remove their sub tasks
         for ( ossPoolSet<UINT64>::iterator it = mainTaskSet.begin() ;
               it != mainTaskSet.end() ; it++ )
         {
            BSONObj matcher1 = BSON( FIELD_NAME_MAIN_TASKID << (INT64)(*it) ) ;
            rc = catRemoveTask( matcher1, FALSE, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to remove task by matcher[%s], rc: %d",
                         matcher1.toString().c_str(), rc ) ;
         }

         // if the task to be deleted is sub task, update their main task's info
         for ( ossPoolMap<UINT64,UINT64>::iterator it = subTaskMap.begin() ;
               it != subTaskMap.end() ; it++ )
         {
            rc = _updateMainTaskByRemoveTask( it->second, it->first, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update main task[%llu], rc: %d",
                         it->second, rc ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catRemoveCSTasks( const CHAR *csName, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj dummyObj, matcher ;
      BOOLEAN hasTask = FALSE ;
      ossPoolSet<UINT64> mainTaskSet ;
      ossPoolMap<UINT64,UINT64> subTaskMap ; // <taskID, mainTaskID>
      BSONObjBuilder matcherBob ;
      BSONObj updator ;

      try
      {
         rc = _catBuildCLMatcher( CAT_COLLECTION_NAME, csName, matcher ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build matcher, rc: %d", rc ) ;

         rc = _queryTask( matcher, cb, hasTask, mainTaskSet, subTaskMap ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to query task by matcher, rc: %d",
                      rc ) ;

         if ( hasTask )
         {
            rc = catRemoveTask( matcher, FALSE, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to remove task by matcher[%s], rc: %d",
                         matcher.toString().c_str(), rc ) ;
         }

         updator = BSON( MTH_MODIFIER_UNSET << BSON( FIELD_NAME_MAIN_TASKID << "" ) ) ;

         // if the task to be deleted is main task, remove their sub tasks
         for ( ossPoolSet<UINT64>::iterator it = mainTaskSet.begin() ;
               it != mainTaskSet.end() ; it++ )
         {
            {
               matcherBob.reset() ;
               BSONArrayBuilder andBab( matcherBob.subarrayStart( MTH_OPERATOR_STR_AND ) ) ;
               BSONObjBuilder mainTaskIDBob( andBab.subobjStart() ) ;
               mainTaskIDBob.append( FIELD_NAME_MAIN_TASKID, (INT64)(*it) ) ;
               mainTaskIDBob.done() ;
               BSONObjBuilder statusBob( andBab.subobjStart() ) ;
               BSONObjBuilder neBob( statusBob.subobjStart( FIELD_NAME_STATUS ) ) ;
               neBob.append( MTH_OPERATOR_STR_NE, CLS_TASK_STATUS_FINISH ) ;
               neBob.done() ;
               statusBob.done() ;
               andBab.done() ;
               matcher = matcherBob.done() ;
            }

            // matcher = { "$and": [ { "MainTaskID": xxx }, { "Status": { "$ne": 9 } } ] }
            // updator = { "$unset": { "MainTaskID": "" } }
            rc = catUpdateTask( matcher, updator, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task by matcher[%s] and updator[%s], rc: %d",
                         matcher.toString().c_str(), updator.toString().c_str(), rc ) ;

            matcherBob.reset() ;
            matcherBob.append( FIELD_NAME_MAIN_TASKID, (INT64)(*it) ) ;
            matcher = matcherBob.done() ;

            // matcher = { "MainTaskID": xxx }
            rc = catRemoveTask( matcher, FALSE, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to remove task by matcher[%s], rc: %d",
                         matcher.toString().c_str(), rc ) ;
         }

         // if the task to be deleted is sub task, update their main task's info
         for ( ossPoolMap<UINT64,UINT64>::iterator it = subTaskMap.begin() ;
               it != subTaskMap.end() ; it++ )
         {
            rc = _updateMainTaskByRemoveTask( it->second, it->first, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update main task[%llu], rc: %d",
                         it->second, rc ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catRemoveSequenceTasks ( const CHAR * sequenceName, pmdEDUCB * cb,
                                  INT16 w )
   {
      SDB_ASSERT( NULL != sequenceName, "sequence name is invalid" ) ;
      BSONObj matcher = BSON( FIELD_NAME_AUTOINC_SEQ << sequenceName ) ;
      return catRemoveTask( matcher, FALSE, cb, w ) ;
   }

   INT32 catRemoveTasksByType ( CLS_TASK_TYPE type, pmdEDUCB * cb, INT16 w )
   {
      SDB_ASSERT( CLS_TASK_UNKNOWN != type, "Task type is invalid" ) ;
      BSONObj matcher = BSON( CAT_TASKTYPE_NAME << type ) ;
      return catRemoveTask( matcher, FALSE, cb, w ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRMEXPTASK, "catRemoveExpiredTasks" )
   INT32 catRemoveExpiredTasks ( pmdEDUCB* cb, INT16 w, INT32 expirationTimeS )
   {
      PD_TRACE_ENTRY ( SDB_CATRMEXPTASK ) ;
      INT32 rc = SDB_OK ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;

      try
      {

      // query tasks
      BSONObj dummyObj ;
      BSONObj matcher = BSON( CAT_STATUS_NAME << CLS_TASK_STATUS_FINISH ) ;
      rc = rtnQuery( CAT_TASK_INFO_COLLECTION,
                     dummyObj, matcher, dummyObj, dummyObj,
                     0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc: %d", rc ) ;

      // loop every task
      while ( TRUE )
      {
         rtnContextBuf contextBuf ;
         const CHAR* timeStr = NULL ;
         UINT64 taskID = CLS_INVALID_TASKID ;

         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to retreive record, rc: %d", rc ) ;

         BSONObj obj( contextBuf.data() ) ;
         rc = rtnGetStringElement( obj, FIELD_NAME_ENDTIMESTAMP, &timeStr ) ;
         if ( rc )
         {
            // Before 3.4.1 version (include), split task hasn't EndTimestamp
            if ( rc != SDB_FIELD_NOT_EXIST )
            {
               PD_LOG( PDWARNING,
                       "Failed to get field[%s] from obj[%s], rc: %d",
                       FIELD_NAME_ENDTIMESTAMP, obj.toString().c_str(), rc ) ;
            }
            continue ;
         }
         if ( 0 == timeStr[0] )
         {
            continue ;
         }

         rc = rtnGetNumberLongElement( obj, CAT_TASKID_NAME, (INT64&)taskID ) ;
         if ( rc )
         {
            PD_LOG( PDWARNING,
                    "Failed to get field[%s] from obj[%s], rc: %d",
                     CAT_TASKID_NAME, obj.toString().c_str() , rc ) ;
            continue ;
         }

         ossTimestamp endTS, curTS ;
         ossStringToTimestamp( timeStr, endTS ) ;
         ossGetCurrentTime( curTS ) ;
         if ( curTS.time - endTS.time > expirationTimeS )
         {
            rc = catRemoveTask( taskID, FALSE, cb, w ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING,
                       "Failed to remove expired task[%llu], rc: %d",
                       taskID, rc ) ;
               continue ;
            }
            PD_LOG( PDINFO, "Remove expired task[%llu]", taskID ) ;
         }
      }

      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() );
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete ( contextID, cb ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATRMEXPTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRENAMECLTASK, "catRenameCLInTasks" )
   INT32 catRenameCLInTasks( const CHAR *clFullName, const CHAR *newCLFullName,
                             pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRENAMECLTASK ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      INT64 contextID = -1 ;
      ossPoolSet<UINT64> mainTaskSet ;
      BSONObj dummyObj, match, updator ;

      try
      {
         // find all related sub-task
         match = BSON( FIELD_NAME_NAME << clFullName <<
                       FIELD_NAME_MAIN_TASKID << BSON( "$exists" << 1 ) ) ;

         rc = rtnQuery( CAT_TASK_INFO_COLLECTION,
                        dummyObj, match, dummyObj, dummyObj,
                        0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc: %d", rc ) ;

         // get all sub-task's main-task
         while ( TRUE )
         {
            UINT64 mainTaskID = 0 ;
            rtnContextBuf contextBuf ;
            rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

            BSONObj obj( contextBuf.data() ) ;
            rc = rtnGetNumberLongElement( obj, FIELD_NAME_MAIN_TASKID,
                                          (INT64&)mainTaskID ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s]",
                         FIELD_NAME_MAIN_TASKID, obj.toString().c_str() ) ;
            mainTaskSet.insert( mainTaskID ) ;
         }

         // loop every main-task, if it is copy-index task, change 'CopyTo' field
         for( ossPoolSet<UINT64>::iterator it = mainTaskSet.begin() ;
              it != mainTaskSet.end() ; it++ )
         {
            UINT64 taskID = *it ;
            BSONObj taskObj, copyToArray ;
            ossPoolVector<const CHAR*> copyToList ;

            // get 'CopyTo' array
            match = BSON( FIELD_NAME_TASKID << (INT64)taskID <<
                          FIELD_NAME_TASKTYPE << CLS_TASK_COPY_IDX ) ;
            rc = catGetOneObj( CAT_TASK_INFO_COLLECTION,
                               dummyObj, match, dummyObj, cb, taskObj ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               continue ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get task[%llu]", taskID ) ;

            rc = rtnGetArrayElement( taskObj, FIELD_NAME_COPYTO, copyToArray ) ;
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               continue ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get field[%s] from obj[%s]",
                         FIELD_NAME_MAIN_TASKID, taskObj.toString().c_str() ) ;

            BSONObjIterator iter( copyToArray ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               const CHAR* copyToName = ele.valuestrsafe() ;
               if ( 0 == ossStrcmp( copyToName, clFullName ) )
               {
                  copyToList.push_back( newCLFullName ) ;
               }
               else
               {
                  copyToList.push_back( copyToName ) ;
               }
            }

            // set 'CopyTo' array
            BSONObjBuilder builder ;
            BSONArrayBuilder ab( builder.subarrayStart( FIELD_NAME_COPYTO ) ) ;
            for ( ossPoolVector<const CHAR*>::iterator it = copyToList.begin() ;
                  it != copyToList.end() ; ++it )
            {
               ab.append( *it ) ;
            }
            ab.done() ;

            updator = BSON( "$set" << builder.done() ) ;
            rc = rtnUpdate( CAT_TASK_INFO_COLLECTION,
                            match, updator, dummyObj,
                            0, cb, dmsCB, dpsCB, w ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to update task, matcher[%s] updator[%s], "
                         "rc: %d", match.toString().c_str(),
                         updator.toString().c_str(), rc ) ;

         }

         // rename collection name
         match = BSON( FIELD_NAME_NAME << clFullName ) ;
         updator = BSON( "$set" << BSON( FIELD_NAME_NAME << newCLFullName ) ) ;
         rc = rtnUpdate( CAT_TASK_INFO_COLLECTION,
                         match, updator, dummyObj,
                         0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to update task, matcher[%s] updator[%s], rc: %d",
                      match.toString().c_str(),
                      updator.toString().c_str(), rc ) ;

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

   done:
      if ( contextID != -1 )
      {
         rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
      }
      PD_TRACE_EXITRC ( SDB_CATRENAMECLTASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catGetBucketVersion( const CHAR *pCLName, pmdEDUCB *cb )
   {
      INT32 version = CAT_VERSION_BEGIN ;
      UINT32 bucketID = catCalcBucketID( pCLName, ossStrlen( pCLName ) ) ;
      BSONObj dummy ;
      BSONObj mather = BSON( FIELD_NAME_BUCKETID << bucketID ) ;
      BSONObj result ;

      INT32 rc = catGetOneObj( CAT_HISTORY_COLLECTION, dummy, mather,
                               dummy, cb, result ) ;
      if ( SDB_OK == rc )
      {
         version = (INT32)result.getField( FIELD_NAME_VERSION ).numberInt() ;
         ++version ;
      }
      return version ;
   }

   INT32 catSaveBucketVersion( const CHAR *pCLName, INT32 version,
                               pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      const INT32 reverseVer = 0x00FFFFFF ;
      UINT32 bucketID = catCalcBucketID( pCLName, ossStrlen( pCLName ) ) ;
      BSONObj dummy ;
      BSONObj mather = BSON( FIELD_NAME_BUCKETID << bucketID ) ;
      BSONObj result ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      rc = catGetOneObj( CAT_HISTORY_COLLECTION, dummy, mather,
                         dummy, cb, result ) ;
      // not exist
      if ( SDB_DMS_EOC == rc )
      {
#if defined ( _DEBUG )
         BSONObj obj = BSON( FIELD_NAME_BUCKETID << bucketID <<
                             FIELD_NAME_NAME << pCLName <<
                             FIELD_NAME_VERSION << version ) ;
#else
         BSONObj obj = BSON( FIELD_NAME_BUCKETID << bucketID <<
                             FIELD_NAME_VERSION << version ) ;
#endif // _DEBUG

         rc = rtnInsert( CAT_HISTORY_COLLECTION, obj, 1, 0, cb, dmsCB,
                         dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to insert record[%s] to "
                      "collection[%s], rc: %d", obj.toString().c_str(),
                      CAT_HISTORY_COLLECTION, rc ) ;
      }
      else if ( SDB_OK == rc )
      {
         INT32 verTmp = (INT32)result.getField( FIELD_NAME_VERSION
                                               ).numberInt() ;
         if ( version < verTmp && verTmp - version < reverseVer )
         {
            goto done ;
         }
         // update
         else
         {
#if defined ( _DEBUG )
            BSONObj updator = BSON( "$set" << BSON( FIELD_NAME_VERSION <<
                                                    version <<
                                                    FIELD_NAME_NAME <<
                                                    pCLName ) ) ;
#else
            BSONObj updator = BSON( "$set" << BSON( FIELD_NAME_VERSION <<
                                                    version ) ) ;
#endif // _DEBUG
            rc = rtnUpdate( CAT_HISTORY_COLLECTION, mather, updator,
                            BSONObj(), 0, cb, dmsCB, dpsCB, w, NULL ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to update record[%s] to "
                         "collection[%s], rc: %d", updator.toString().c_str(),
                         CAT_HISTORY_COLLECTION, rc ) ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get record from collection[%s], rc: %d",
                 CAT_HISTORY_COLLECTION, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catCheckBaseInfoExist( const char *pTypeStr, BOOLEAN &isExist,
                                BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc           = SDB_OK ;
      isExist            = FALSE ;

      BSONObj matcher = BSON( FIELD_NAME_TYPE << pTypeStr ) ;
      BSONObj dummyObj ;

      rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummyObj, matcher,
                         dummyObj, cb, obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         isExist = FALSE ;
         rc = SDB_OK ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                 matcher.toString().c_str(), CAT_SYSDCBASE_COLLECTION_NAME,
                 rc ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 catUpdateBaseInfoAddr( const CHAR *pAddr, BOOLEAN self,
                                pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj updator ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      if ( self )
      {
         updator = BSON( "$set" << BSON( FIELD_NAME_DATACENTER"."
                                         FIELD_NAME_ADDRESS << pAddr )
                       ) ;
      }
      else
      {
         updator = BSON( "$set" << BSON( FIELD_NAME_IMAGE"."
                                         FIELD_NAME_ADDRESS << pAddr )
                        ) ;
      }

      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      BSONObj(), 0, cb, dmsCB, dpsCB, w, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Update collection[%s] obj[%s] failed, rc: %d",
                   CAT_SYSDCBASE_COLLECTION_NAME, updator.toString().c_str(),
                   rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catEnableImage( BOOLEAN enable, pmdEDUCB *cb, INT16 w,
                         _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj updator ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                              CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj hint ;
      utilUpdateResult result ;

      if ( enable )
      {
         updator = BSON( "$set" << BSON( FIELD_NAME_IMAGE "." FIELD_NAME_ENABLE
                                         << true ) ) ;
      }
      else
      {
         updator = BSON( "$set" << BSON( FIELD_NAME_IMAGE "." FIELD_NAME_ENABLE
                                         << false ) ) ;
      }
      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w, &result ) ;
      PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                   "rc: %d", updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      if ( 0 == result.updateNum() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "No found obj[%s] in collection[%s]",
                 matcher.toString().c_str(), CAT_SYSDCBASE_COLLECTION_NAME ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catUpdateDCStatus( const CHAR *pField, BOOLEAN status,
                            pmdEDUCB *cb, INT16 w,
                            _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      BSONObj updator ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE <<
                              CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj hint ;
      utilUpdateResult upResult ;

      if ( status )
      {
         updator = BSON( "$set" << BSON( pField << true ) ) ;
      }
      else
      {
         updator = BSON( "$set" << BSON( pField << false ) ) ;
      }
      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w, &upResult ) ;
      PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                   "rc: %d", updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      if ( 0 == upResult.updateNum() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "No found obj[%s] in collection[%s]",
                 matcher.toString().c_str(), CAT_SYSDCBASE_COLLECTION_NAME ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSETUHWM, "catSetCSUniqueHWM" )
   INT32 catSetCSUniqueHWM( pmdEDUCB *cb, INT16 w, UINT32 csUniqueHWM )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSETUHWM ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj updator = BSON( "$set" <<
                              BSON( FIELD_NAME_CSUNIQUEHWM << csUniqueHWM ) );

      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      BSONObj(), 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Fail to update obj[%s] in collection[%s], rc: %d",
                   updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
   done :
      PD_TRACE_EXITRC( SDB_CATSETUHWM, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPCSUID, "catUpdateCSUniqueID" )
   INT32 catUpdateCSUniqueID( pmdEDUCB *cb, INT16 w,
                              utilCSUniqueID& csUniqueID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATUPCSUID ) ;

      BSONObj dummy, result ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj updator ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummy, matcher,
                         dummy, cb, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to match obj[%s] from collection[%s], "
                   "rc: %d", matcher.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      try
      {
         BSONElement ele = result.getField( FIELD_NAME_CSUNIQUEHWM ) ;
         PD_CHECK( ele.isNumber() || ele.eoo(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field[%s], type: %d",
                   FIELD_NAME_CSUNIQUEHWM, ele.type() );
         if ( ele.eoo() )
         {
            csUniqueID = 1 ;
         }
         else
         {
            csUniqueID = ( utilCSUniqueID )ele.numberInt() + 1 ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      if ( csUniqueID > ( utilCSUniqueID )UTIL_CSUNIQUEID_MAX )
      {
         rc = SDB_CAT_CS_UNIQUEID_EXCEEDED ;
         PD_LOG( PDERROR,
                 "CS unique id[%u] can't exceed %u, rc: %d",
                 csUniqueID, UTIL_CSUNIQUEID_MAX, rc ) ;
         goto error ;
      }

      updator = BSON( "$set" << BSON( FIELD_NAME_CSUNIQUEHWM << csUniqueID ) ) ;
      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Fail to update obj[%s] to collection[%s], "
                   "rc: %d", updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATUPCSUID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSETTASKHWM, "catSetTaskHWM" )
   INT32 catSetTaskHWM( pmdEDUCB *cb, INT16 w, UINT64 taskHWM )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATSETTASKHWM ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      try
      {

      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj updator = BSON( "$set" <<
                              BSON( FIELD_NAME_TASKHWM << (INT64)taskHWM ) );

      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      BSONObj(), 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Fail to update obj[%s] in collection[%s], rc: %d",
                   updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }
   done :
      PD_TRACE_EXITRC( SDB_CATSETTASKHWM, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   #define CAT_TASKID_MAX DMS_DUMMY_CATTASKID_MIN

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETTASKID, "catGetAndIncTaskID" )
   INT32 catGetAndIncTaskID( pmdEDUCB *cb, INT16 w, UINT64& taskID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATGETTASKID ) ;

      try
      {

      BSONObj dummy, result ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj updator ;

      rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummy, matcher,
                         dummy, cb, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to match obj[%s] from collection[%s], "
                   "rc: %d", matcher.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      BSONElement ele = result.getField( FIELD_NAME_TASKHWM ) ;
      PD_CHECK( ele.isNumber(), SDB_INVALIDARG, error, PDERROR,
                "Failed to get field[%s], type: %d",
                FIELD_NAME_TASKHWM, ele.type() );
      taskID = ( UINT64 )ele.numberLong() + 1 ;

      if ( taskID > CAT_TASKID_MAX )
      {
         taskID = 1 ;
         PD_LOG( PDINFO,
                 "TaskID reached maximun[%llu], start from %llu",
                 CAT_TASKID_MAX, taskID ) ;
      }

      rc = catSetTaskHWM( cb, w, taskID ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to set task hwm, rc: %d",
                   rc ) ;

      }
      catch ( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATGETTASKID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPGLOBALID, "catUpdateGlobalID" )
   INT32 catUpdateGlobalID( pmdEDUCB *cb, INT16 w,
                            utilGlobalID& globalID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATUPGLOBALID ) ;

      BSONObj dummy, result ;
      BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
      BSONObj updator ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummy, matcher,
                         dummy, cb, result ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to match obj[%s] from collection[%s], "
                   "rc: %d", matcher.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

      try
      {
         BSONElement ele = result.getField( FIELD_NAME_GLOBALID ) ;
         PD_CHECK( ele.isNumber() || ele.eoo(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field[%s], type: %d",
                   FIELD_NAME_GLOBALID, ele.type() );
         if ( ele.eoo() )
         {
            globalID = 1 ;
         }
         else
         {
            globalID = ( utilGlobalID )ele.numberLong() ;
            if ( UTIL_GLOGALID_MAX - 1 < globalID )
            {
               rc = SDB_CAT_GLOBALID_EXCEEDED ;
               PD_LOG( PDERROR,
                       "Global id[%llu] can't exceed %llu, rc: %d",
                       globalID, UTIL_GLOGALID_MAX, rc ) ;
               goto error ;
            }
            globalID += 1 ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      updator = BSON( "$set" << BSON( FIELD_NAME_GLOBALID << (INT64)globalID ) ) ;
      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Fail to update obj[%s] to collection[%s], "
                   "rc: %d", updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

   done :
      PD_TRACE_EXITRC( SDB_CATUPGLOBALID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 catFormatIndexInfo( const CHAR* collection, utilCLUniqueID clUniqID,
                             const BSONObj& indexDef,
                             _pmdEDUCB *cb, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      const CHAR* indexName = NULL ;
      clsCatalogSet *pCataSet = NULL ;

      try
      {
         builder.append( FIELD_NAME_COLLECTION, collection ) ;
         builder.append( FIELD_NAME_CL_UNIQUEID, (INT64)clUniqID ) ;

         indexName = indexDef.getStringField( IXM_NAME_FIELD ) ;
         builder.append( FIELD_NAME_NAME, indexName) ;

         BSONObjBuilder ob ( builder.subobjStart( IXM_FIELD_NAME_INDEX_DEF ) );
         ob.append( IXM_NAME_FIELD, indexName ) ;
         if ( indexDef.hasField( DMS_ID_KEY_NAME ) )
         {
            ob.append( DMS_ID_KEY_NAME,
                       indexDef.getField( DMS_ID_KEY_NAME ).OID() ) ;
         }
         else
         {
            ob.append( DMS_ID_KEY_NAME, OID::gen() ) ;
         }
         if ( indexDef.hasField( FIELD_NAME_UNIQUEID ) )
         {
            ob.append( FIELD_NAME_UNIQUEID,
                       indexDef.getField( FIELD_NAME_UNIQUEID ).numberLong() ) ;
         }
         ob.append( IXM_KEY_FIELD,
                    indexDef.getObjectField( IXM_KEY_FIELD ) ) ;
         ob.append( IXM_V_FIELD, 0 ) ;
         ob.append( IXM_UNIQUE_FIELD,
                    indexDef.getBoolField( IXM_UNIQUE_FIELD ) ) ;
         ob.append( IXM_DROPDUP_FIELD,
                    indexDef.getBoolField( IXM_DROPDUP_FIELD ) ) ;
         ob.append( IXM_ENFORCED_FIELD,
                    indexDef.getBoolField( IXM_ENFORCED_FIELD ) ) ;
         ob.append( IXM_NOTNULL_FIELD,
                    indexDef.getBoolField( IXM_NOTNULL_FIELD ) ) ;

         if( 0 == ossStrcmp( indexDef.getStringField( IXM_NAME_FIELD ),
                             IXM_ID_KEY_NAME ) )
         {
            ob.append ( IXM_NOTARRAY_FIELD, true ) ;
         }
         else
         {
            ob.append ( IXM_NOTARRAY_FIELD,
                        indexDef.getBoolField( IXM_NOTARRAY_FIELD ) ) ;
         }

         BOOLEAN isGlobalIdx = indexDef.getBoolField( IXM_FIELD_NAME_GLOBAL ) ;
         ob.appendBool( IXM_FIELD_NAME_GLOBAL, isGlobalIdx ) ;
         if ( isGlobalIdx )
         {
            ob.append( IXM_FIELD_NAME_GLOBAL_OPTION,
                       indexDef.getObjectField(IXM_FIELD_NAME_GLOBAL_OPTION) ) ;
         }

         ob.append( IXM_FIELD_NAME_STANDALONE, false ) ;

         BSONObj range = indexDef.getObjectField( IXM_2DRANGE_FIELD ) ;
         if ( !range.isEmpty() )
         {
            ob.append( IXM_2DRANGE_FIELD, range ) ;
         }
         ob.done () ;

         builder.append( IXM_FIELD_NAME_INDEX_FLAG,
                         ixmGetIndexFlagDesp( IXM_INDEX_FLAG_NORMAL ) ) ;

         UINT16 idxType = 0 ;
         rc = ixmGetIndexType( indexDef, idxType ) ;
         if ( rc )
         {
            goto error ;
         }
         builder.append( FIELD_NAME_TYPE,
                         ixmGetIndexTypeDesp( idxType ).c_str() ) ;

         if ( IXM_EXTENT_HAS_TYPE( idxType, IXM_EXTENT_TYPE_TEXT ) )
         {
            BSONObj boCollection ;
            CHAR extDataName[ DMS_MAX_EXT_NAME_SIZE + 1 ] = { 0 } ;

            // get collection's unique id
            pCataSet = SDB_OSS_NEW clsCatalogSet( collection ) ;
            PD_CHECK( pCataSet, SDB_OOM, error, PDERROR, "Allocate failed" ) ;

            rc = catGetCollection( collection, boCollection, cb ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to get the collection[%s], rc: %d",
                         collection, rc ) ;

            rc = pCataSet->updateCatSet( boCollection ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to parse cata info[%s], rc: %d",
                         boCollection.toString().c_str(), rc );

            if ( !pCataSet->isMainCL() )
            {
               // generate ext data name, except main-collection
               rc = ixmBuildExtDataName( pCataSet->clUniqueID(), indexName,
                                         extDataName,
                                         DMS_MAX_EXT_NAME_SIZE + 1 ) ;
               PD_RC_CHECK( rc, PDERROR,
                            "Get external data name failed[%d]",
                            rc ) ;
               builder.append( FIELD_NAME_EXT_DATA_NAME, extDataName ) ;
            }
         }

         obj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_RC_CHECK( SDB_SYS, PDERROR, "Exception occurred: %s", e.what() ) ;
      }

   done:
      if ( pCataSet )
      {
         SDB_OSS_DEL pCataSet ;
         pCataSet = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGET_GRPMODE_OBJ, "catGetGrpModeObj" )
   INT32 catGetGrpModeObj( const UINT32 &groupID, BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATGET_GRPMODE_OBJ ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj boMatcher ;

         boMatcher = BSON( CAT_GROUPID_NAME << groupID ) ;

         rc = catGetOneObj( CAT_GROUP_MODE_COLLECTION, dummyObj, boMatcher, dummyObj, cb, obj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CLS_GRP_NOT_EXIST ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                    boMatcher.toString().c_str(), CAT_GROUP_MODE_COLLECTION, rc ) ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATGET_GRPMODE_OBJ, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_PARSE_GRPMODE_OBJ, "catParseGrpModeObj" )
   INT32 catParseGrpModeObj( const BSONObj &grpModeObj,
                             clsGroupMode &grpMode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_PARSE_GRPMODE_OBJ ) ;

      try
      {
         BSONElement ele ;
         BSONObjIterator propItr ;
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossTimestamp curTime ;
         ossGetCurrentTime( curTime ) ;

         // Get old GroupMode field
         ele = grpModeObj.getField( CAT_GROUP_MODE_NAME ) ;
         if ( ! ele.eoo() && String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDWARNING, "Failed to get field[%s] from GroupMode obj[%s]",
                    CAT_GROUP_MODE_NAME, grpModeObj.toPoolString().c_str() ) ;
            goto error ;
         }

         if ( ele.eoo() )
         {
            // Do nothing, because if this field is empty, this function shouldn't be called
            goto done ;
         }
         else if ( 0 == ossStrcmp( CAT_CRITICAL_MODE_NAME, ele.valuestrsafe() ) )
         {
            grpMode.mode = CLS_GROUP_MODE_CRITICAL ;
         }
         else if ( 0 == ossStrcmp( CAT_MAINTENANCE_MODE_NAME, ele.valuestrsafe() ) )
         {
            grpMode.mode = CLS_GROUP_MODE_MAINTENANCE ;
         }
         else
         {
            grpMode.mode = CLS_GROUP_MODE_NONE ;
         }

         // Get GroupID
         ele = grpModeObj.getField( CAT_GROUPID_NAME ) ;
         if ( ele.eoo() || ! ele.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDWARNING, "Failed to get field[%s] from GroupMode obj[%s]",
                    CAT_GROUPID_NAME, grpModeObj.toPoolString().c_str() ) ;
            goto error ;
         }
         grpMode.groupID = ele.numberInt() ;

         // Get properties field
         ele = grpModeObj.getField( CAT_PROPERTIES_NAME ) ;
         if ( ele.eoo() || Array != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDWARNING, "Failed to get field[%s] from GroupMode obj[%s]",
                    CAT_PROPERTIES_NAME, grpModeObj.toPoolString().c_str() ) ;
            goto error ;
         }
         propItr = BSONObjIterator( ele.embeddedObject() ) ;

         // Parse properties array
         while ( propItr.more() )
         {
            clsGrpModeItem tmpGrpModeItem ;
            BSONObj propObj = propItr.next().embeddedObject() ;
            BSONElement propEle ;
            ossTimestamp tmpTime ;

            // Get nodeID field
            if ( propObj.hasField( CAT_NODEID_NAME ) )
            {
               propEle = propObj.getField( CAT_NODEID_NAME ) ;
               if ( ! propEle.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                          CAT_NODEID_NAME, propObj.toPoolString().c_str() ) ;
                  goto error ;
               }
               tmpGrpModeItem.nodeID = propEle.numberInt() ;

               propEle = propObj.getField( FIELD_NAME_NODE_NAME ) ;
               if ( String != propEle.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                          FIELD_NAME_NODE_NAME, propObj.toPoolString().c_str() ) ;
                  goto error ;
               }
               tmpGrpModeItem.nodeName = propEle.valuestrsafe() ;
            }
            // Get location field
            else if ( propObj.hasField( CAT_LOCATION_NAME ) )
            {
               propEle = propObj.getField( CAT_LOCATION_NAME ) ;
               if ( String != propEle.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                          CAT_LOCATION_NAME, propObj.toPoolString().c_str() ) ;
                  goto error ;
               }
               tmpGrpModeItem.location = propEle.valuestrsafe() ;

               propEle = propObj.getField( CAT_LOCATIONID_NAME ) ;
               if ( ! propEle.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                          CAT_LOCATIONID_NAME, propObj.toPoolString().c_str() ) ;
                  goto error ;
               }
               tmpGrpModeItem.locationID = propEle.numberInt() ;
            }

            // Get MinKeepTime
            propEle = propObj.getField( CAT_MIN_KEEP_TIME_NAME ) ;
            if ( propEle.eoo() || String != propEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                       CAT_MIN_KEEP_TIME_NAME, propObj.toPoolString().c_str() ) ;
               goto error ;
            }
            ossStrcpy( timeStr, propEle.valuestrsafe() ) ;
            ossStringToTimestamp( timeStr, tmpTime ) ;
            tmpGrpModeItem.minKeepTime.time =
               OSS_MIN( tmpTime.time, curTime.time + CLS_GROUP_MODE_KEEP_TIME_MAX * 60 ) ;
            tmpGrpModeItem.minKeepTime.microtm = tmpTime.microtm ;

            // Get MaxKeepTime
            propEle = propObj.getField( CAT_MAX_KEEP_TIME_NAME ) ;
            if ( propEle.eoo() || String != propEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                       CAT_MAX_KEEP_TIME_NAME, propObj.toPoolString().c_str() ) ;
               goto error ;
            }
            ossStrcpy( timeStr, propEle.valuestrsafe() ) ;
            ossStringToTimestamp( timeStr, tmpTime ) ;
            tmpGrpModeItem.maxKeepTime.time =
               OSS_MIN( tmpTime.time, curTime.time + CLS_GROUP_MODE_KEEP_TIME_MAX * 60 ) ;
            tmpGrpModeItem.maxKeepTime.microtm = tmpTime.microtm ;

            // Get UpdateTime
            propEle = propObj.getField( CAT_UPDATETIME_NAME ) ;
            if ( propEle.eoo() || String != propEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDWARNING, "Failed to get field[%s] from Properties obj[%s]",
                       CAT_UPDATETIME_NAME, propObj.toPoolString().c_str() ) ;
               goto error ;
            }
            ossStrcpy( timeStr, propEle.valuestrsafe() ) ;
            ossStringToTimestamp( timeStr, tmpGrpModeItem.updateTime ) ;

            grpMode.grpModeInfo.push_back( tmpGrpModeItem ) ;
         }

      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_PARSE_GRPMODE_OBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_DELGRP_FROM_GRPMODE, "catDelGroupFromGrpMode" )
   INT32 catDelGroupFromGrpMode( const UINT32 &groupID,
                                 const INT16 &w,
                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_DELGRP_FROM_GRPMODE ) ;

      SDB_DMSCB *dmsCB = sdbGetDMSCB() ;
      SDB_DPSCB *dpsCB = sdbGetDPSCB() ;

      try
      {
         BSONObj matcher, dummyObj, updator ;

         // Append groupID to matcher
         matcher = BSON( CAT_GROUPID_NAME << groupID ) ;

         // Remove in CAT_NODE_INFO_COLLECTION
         updator = BSON( "$unset" << BSON( CAT_GROUP_MODE_NAME << "" ) ) ;

         rc = rtnUpdate( CAT_NODE_INFO_COLLECTION, matcher, updator, dummyObj,
                         0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update group[%u], matcher: %s, "
                      "updator: %s, rc: %d", groupID, matcher.toPoolString().c_str(),
                      updator.toPoolString().c_str(), rc ) ;

         // Remove in CAT_GROUP_MODE_COLLECTION
         rc = rtnDelete( CAT_GROUP_MODE_COLLECTION, matcher,
                         dummyObj, 0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to delete group info in %s, "
                      "groupID: %u, matcher: %s, rc: %d", CAT_GROUP_MODE_COLLECTION,
                      groupID, matcher.toPoolString().c_str(), rc ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_DELGRP_FROM_GRPMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_INSERT_GRPMODE, "catInsertGrpMode" )
   INT32 catInsertGrpMode( const clsGroupMode &grpMode,
                           const string &groupName,
                           BSONObjBuilder &insertBuilder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_INSERT_GRPMODE ) ;

      try
      {
         // Store MinKeepTime, MaxKeepTime and UpdateTime in string format
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;

         insertBuilder.append( CAT_GROUPID_NAME, grpMode.groupID ) ;
         insertBuilder.append( CAT_GROUPNAME_NAME, groupName ) ;

         if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
         {
            insertBuilder.append( CAT_GROUP_MODE_NAME, CAT_CRITICAL_MODE_NAME ) ;
         }
         else if ( CLS_GROUP_MODE_MAINTENANCE == grpMode.mode )
         {
            insertBuilder.append( CAT_GROUP_MODE_NAME, CAT_MAINTENANCE_MODE_NAME ) ;
         }

         // Properties builder
         BSONArrayBuilder propBuilder( insertBuilder.subarrayStart( CAT_PROPERTIES_NAME ) ) ;

         VEC_GRPMODE_ITEM::const_iterator itr = grpMode.grpModeInfo.begin() ;
         while ( grpMode.grpModeInfo.end() != itr )
         {
            BSONObjBuilder sub( propBuilder.subobjStart() ) ;
            const clsGrpModeItem& item = *itr++ ;

            // Append nodeID or Location
            if ( CAT_INVALID_NODEID != item.nodeID )
            {
               sub.append( CAT_NODEID_NAME, item.nodeID ) ;
               sub.append( FIELD_NAME_NODE_NAME, item.nodeName.c_str() ) ;
            }
            else if ( CAT_INVALID_LOCATIONID != item.locationID )
            {
               sub.append( CAT_LOCATIONID_NAME, item.locationID ) ;
               sub.append( CAT_LOCATION_NAME, item.location.c_str() ) ;
            }

            // Append MinKeepTime and MaxKeepTime
            ossTimestampToString( const_cast< ossTimestamp& >( item.minKeepTime ), timeStr ) ;
            sub.append( CAT_MIN_KEEP_TIME_NAME, timeStr ) ;
            ossTimestampToString( const_cast< ossTimestamp& >( item.maxKeepTime ), timeStr ) ;
            sub.append( CAT_MAX_KEEP_TIME_NAME, timeStr ) ;

            // Append UpdateTime
            ossTimestampToString( const_cast< ossTimestamp& >( item.updateTime ), timeStr ) ;
            sub.append( CAT_UPDATETIME_NAME, timeStr ) ;

            sub.doneFast() ;
         }
         propBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_INSERT_GRPMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_UPDATE_GRPMODE, "catUpdateGrpMode" )
   INT32 catUpdateGrpMode( const clsGroupMode &grpMode,
                           const BSONObj &groupModeObj,
                           BSONObjBuilder &updatorBuilder,
                           BSONObjBuilder &matcherBuilder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_UPDATE_GRPMODE ) ;

      try
      {
         VEC_GRPMODE_ITEM_PTR setItemVec ;
         VEC_GRPMODE_ITEM_PTR pushItemVec ;

         // Update group mode
         BSONObjBuilder setBuilder( updatorBuilder.subobjStart( "$set" ) ) ;
         if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
         {
            setBuilder.append( CAT_GROUP_MODE_NAME, CAT_CRITICAL_MODE_NAME ) ;
         }
         else if ( CLS_GROUP_MODE_MAINTENANCE == grpMode.mode )
         {
            setBuilder.append( CAT_GROUP_MODE_NAME, CAT_MAINTENANCE_MODE_NAME ) ;
         }
         setBuilder.doneFast() ;

         // Get Properties field
         BSONElement propEle ;
         BSONObjIterator propItr ;
         propEle = groupModeObj.getField( CAT_PROPERTIES_NAME ) ;
         if ( propEle.eoo() || Array != propEle.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDERROR, "Failed to get field[%s] from GroupMode obj[%s]",
                    CAT_PROPERTIES_NAME, groupModeObj.toPoolString().c_str() ) ;
            goto error ;
         }
         propItr = BSONObjIterator( propEle.embeddedObject() ) ;

         // Parse properties
         while ( propItr.more() )
         {
            BSONObj propObj = propItr.next().embeddedObject() ;
            UINT16 nodeID = CAT_INVALID_NODEID ;
            UINT32 locationID = CAT_INVALID_LOCATIONID ;

            // Get nodeID field
            if ( propObj.hasField( CAT_NODEID_NAME ) )
            {
               propEle = propObj.getField( CAT_NODEID_NAME ) ;
               if ( ! propEle.isNumber() )
               {
                  rc = SDB_CAT_CORRUPTION ;
                  PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s]", CAT_NODEID_NAME,
                          propObj.toPoolString().c_str() ) ;
                  goto error ;
               }
               nodeID = propEle.numberInt() ;
            }
            else if ( propObj.hasField( CAT_LOCATIONID_NAME ) )
            {
               propEle = propObj.getField( CAT_LOCATIONID_NAME ) ;
               if ( ! propEle.isNumber() )
               {
                  rc = SDB_CAT_CORRUPTION ;
                  PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s]", CAT_LOCATIONID_NAME,
                          propObj.toPoolString().c_str() ) ;
                  goto error ;
               }
               locationID = propEle.numberInt() ;
            }
            else
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDERROR, "Failed to get field [%s] or [%s] from obj[%s]",
                       CAT_NODEID_NAME, CAT_LOCATION_NAME, propObj.toPoolString().c_str() ) ;
               goto error ;
            }

            // Add to setItemVec
            VEC_GRPMODE_ITEM::const_iterator itr = grpMode.grpModeInfo.begin() ;
            while ( grpMode.grpModeInfo.end() != itr )
            {
               if ( nodeID == itr->nodeID && locationID == itr->locationID )
               {
                  // The new property match the old, update it
                  setItemVec.push_back( &( *itr ) ) ;
                  break ;
               }
               // For critical mode: Properties.location -> Properties.node | Properties.node -> Properties.location
               else if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
               {
                  setItemVec.push_back( &( *itr ) ) ;
                  if ( CAT_INVALID_NODEID != nodeID )
                  {
                     matcherBuilder.append( CAT_PROPERTIES_NAME ".$0." CAT_NODEID_NAME, nodeID ) ;
                  }
                  else if ( CAT_INVALID_LOCATIONID != locationID )
                  {
                     matcherBuilder.append( CAT_PROPERTIES_NAME ".$0." CAT_LOCATIONID_NAME,
                                            locationID ) ;
                  }
                  break ;
               }
               ++itr ;
            }
         }

         // Add to pushItemVec
         VEC_GRPMODE_ITEM::const_iterator grpModeItr = grpMode.grpModeInfo.begin() ;
         while ( grpMode.grpModeInfo.end() != grpModeItr )
         {
            VEC_GRPMODE_ITEM_PTR::const_iterator setModeItr = setItemVec.begin() ;
            while ( setItemVec.end() != setModeItr )
            {
               if ( grpModeItr->nodeID == ( *setModeItr )->nodeID &&
                    grpModeItr->locationID == ( *setModeItr )->locationID )
               {
                  break ;
               }
               ++setModeItr ;
            }
            if ( setItemVec.end() == setModeItr )
            {
               pushItemVec.push_back( &( *grpModeItr ) ) ;
            }
            ++grpModeItr ;
         }

         rc = catSetGrpModePropty( setItemVec, updatorBuilder,
                                   matcherBuilder.isEmpty() ? &matcherBuilder : NULL ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build set groupMode's properties builder, rc: %d", rc ) ;

         rc = catPushGrpModePropty( pushItemVec, updatorBuilder, matcherBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build push groupMode's properties builder, rc: %d", rc ) ;

         // Append groupID to matcher
         matcherBuilder.append( CAT_GROUPID_NAME, grpMode.groupID ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_UPDATE_GRPMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_REMOVE_GRPMODE, "catRemoveGrpMode" )
   INT32 catRemoveGrpMode( const clsGroupMode &grpMode,
                           const BOOLEAN &needStop,
                           BSONObjBuilder &updatorBuilder,
                           BSONObjBuilder &matcherBuilder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_REMOVE_GRPMODE ) ;

      try
      {
         VEC_GRPMODE_ITEM_PTR pullItemVec ;

         // Append groupID to matcher
         matcherBuilder.append( CAT_GROUPID_NAME, grpMode.groupID ) ;

         // Build unset updator
         if ( needStop )
         {
            // Unset group mode
            BSONObjBuilder unsetBuilder( updatorBuilder.subobjStart( "$unset" ) ) ;
            unsetBuilder.append( CAT_GROUP_MODE_NAME, "" ) ;
            unsetBuilder.doneFast() ;

            // Set properties to empty
            BSONObjBuilder setBuilder( updatorBuilder.subobjStart( "$set" ) ) ;
            setBuilder.append( CAT_PROPERTIES_NAME, BSONArray() ) ;
            setBuilder.doneFast() ;
         }
         else if ( CLS_GROUP_MODE_MAINTENANCE == grpMode.mode )
         {
            // Build pull_by updator
            BSONObjBuilder pullBuilder( updatorBuilder.subobjStart( "$pull_all_by" ) ) ;
            BSONArrayBuilder propBuilder( pullBuilder.subarrayStart( CAT_PROPERTIES_NAME ) ) ;

            VEC_GRPMODE_ITEM::const_iterator itr = grpMode.grpModeInfo.begin() ;
            while ( grpMode.grpModeInfo.end() != itr )
            {
               BSONObjBuilder sub( propBuilder.subobjStart() ) ;
               const clsGrpModeItem& item = *itr++ ;
               if ( CAT_INVALID_NODEID != item.nodeID )
               {
                  sub.append( CAT_NODEID_NAME, item.nodeID ) ;
               }
               else if ( CAT_INVALID_LOCATIONID != item.locationID )
               {
                  sub.append( CAT_LOCATIONID_NAME, item.locationID ) ;
               }
            }
            propBuilder.doneFast() ;
            pullBuilder.doneFast() ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_REMOVE_GRPMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_SET_GRPMODE_PROPERTY, "catSetGrpModePropty" )
   INT32 catSetGrpModePropty( const VEC_GRPMODE_ITEM_PTR &setItemVec,
                              BSONObjBuilder &updatorBuilder,
                              BSONObjBuilder *matcherBuilder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_SET_GRPMODE_PROPERTY ) ;

      if ( setItemVec.empty() )
      {
         goto done ;
      }

      try
      {
         // Store MinKeepTime, MaxKeepTime and UpdateTime in string format
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         BSONObjBuilder setBuilder( updatorBuilder.subobjStart( "$set" ) ) ;
         BSONObjBuilder unsetBuilder ;
         ossPoolStringStream ssCnt ;

         // Parse groupMode item
         for ( UINT16 cnt = 0 ; cnt < setItemVec.size() ; ++cnt, ssCnt.str( "" ) )
         {
            ssCnt << cnt ;
            ossPoolString propPrefixStr = CAT_PROPERTIES_NAME ".$" + ssCnt.str() + "." ;
            const clsGrpModeItem* item = setItemVec[cnt] ;

            // Append nodeID or Location
            if ( CAT_INVALID_NODEID != item->nodeID )
            {
               setBuilder.append( propPrefixStr + CAT_NODEID_NAME, item->nodeID ) ;
               setBuilder.append( propPrefixStr + FIELD_NAME_NODE_NAME, item->nodeName.c_str() ) ;
               unsetBuilder.append( propPrefixStr + CAT_LOCATIONID_NAME, "" ) ;
               unsetBuilder.append( propPrefixStr + CAT_LOCATION_NAME, "" ) ;
               if ( NULL != matcherBuilder )
               {
                  matcherBuilder->append( propPrefixStr + CAT_NODEID_NAME, item->nodeID ) ;
               }
            }
            else if ( CAT_INVALID_LOCATIONID != item->locationID )
            {
               setBuilder.append( propPrefixStr + CAT_LOCATIONID_NAME, item->locationID ) ;
               setBuilder.append( propPrefixStr + CAT_LOCATION_NAME, item->location.c_str() ) ;
               unsetBuilder.append( propPrefixStr + CAT_NODEID_NAME, "" ) ;
               unsetBuilder.append( propPrefixStr + FIELD_NAME_NODE_NAME, "" ) ;
               if ( NULL != matcherBuilder )
               {
                  matcherBuilder->append( propPrefixStr + CAT_LOCATIONID_NAME, item->locationID ) ;
               }
            }

            // Append MinKeepTime and MaxKeepTime
            ossTimestampToString( const_cast< ossTimestamp& >( item->minKeepTime ), timeStr ) ;
            setBuilder.append( propPrefixStr + CAT_MIN_KEEP_TIME_NAME, timeStr ) ;
            ossTimestampToString( const_cast< ossTimestamp& >( item->maxKeepTime ), timeStr ) ;
            setBuilder.append( propPrefixStr + CAT_MAX_KEEP_TIME_NAME, timeStr ) ;

            // Append UpdateTime
            ossTimestampToString( const_cast< ossTimestamp& >( item->updateTime ), timeStr ) ;
            setBuilder.append( propPrefixStr + CAT_UPDATETIME_NAME, timeStr ) ;
         }

         setBuilder.doneFast() ;
         unsetBuilder.doneFast() ;
         updatorBuilder.append( "$unset", unsetBuilder.done() ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_SET_GRPMODE_PROPERTY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_PUSH_GRPMODE_PROPERTY, "catPushGrpModePropty" )
   INT32 catPushGrpModePropty( const VEC_GRPMODE_ITEM_PTR &pushItemVec,
                               BSONObjBuilder &updatorBuilder,
                               BSONObjBuilder &matcherBuilder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_PUSH_GRPMODE_PROPERTY ) ;

      if ( pushItemVec.empty() )
      {
         goto done ;
      }

      try
      {
         // Store MinKeepTime, MaxKeepTime and UpdateTime in string format
         CHAR timeStr[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         BSONObjBuilder pushBuilder( updatorBuilder.subobjStart( "$push_all" ) ) ;
         BSONArrayBuilder propBuilder( pushBuilder.subarrayStart( CAT_PROPERTIES_NAME ) ) ;

         // Parse groupMode item
         VEC_GRPMODE_ITEM_PTR::const_iterator itr = pushItemVec.begin() ;
         while ( pushItemVec.end() != itr )
         {
            BSONObjBuilder sub( propBuilder.subobjStart() ) ;
            const clsGrpModeItem *item = *itr++ ;

            // Append nodeID or Location
            if ( CAT_INVALID_NODEID != item->nodeID )
            {
               sub.append( CAT_NODEID_NAME, item->nodeID ) ;
               sub.append( FIELD_NAME_NODE_NAME, item->nodeName.c_str() ) ;
            }
            else if ( CAT_INVALID_LOCATIONID != item->locationID )
            {
               sub.append( CAT_LOCATIONID_NAME, item->locationID ) ;
               sub.append( CAT_LOCATION_NAME, item->location.c_str() ) ;
            }

            // Append MinKeepTime and MaxKeepTime
            ossTimestampToString( const_cast< ossTimestamp& >( item->minKeepTime ), timeStr ) ;
            sub.append( CAT_MIN_KEEP_TIME_NAME, timeStr ) ;
            ossTimestampToString( const_cast< ossTimestamp& >( item->maxKeepTime ), timeStr ) ;
            sub.append( CAT_MAX_KEEP_TIME_NAME, timeStr ) ;

            // Append UpdateTime
            ossTimestampToString( const_cast< ossTimestamp& >( item->updateTime ), timeStr ) ;
            sub.append( CAT_UPDATETIME_NAME, timeStr ) ;

            sub.doneFast() ;
         }
         propBuilder.doneFast() ;
         pushBuilder.doneFast() ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_PUSH_GRPMODE_PROPERTY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAT_PARSE_GRPMODE_INFO, "catParseGroupModeInfo" )
   INT32 catParseGroupModeInfo( const BSONObj &option,
                                const BSONObj &groupObj,
                                const UINT32 &groupID,
                                const BOOLEAN &parseTime,
                                clsGroupMode &groupMode,
                                ossPoolString *pHostName )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CAT_PARSE_GRPMODE_INFO ) ;

      try
      {
         BSONElement     optionEle ;
         BSONObjIterator itr ;
         string          tmpNodeName ;
         string          tmpLocation ;
         string          tmpHostName ;
         INT32           tmpMinKeepTime = 0 ;
         INT32           tmpMaxKeepTime = 0 ;
         clsGrpModeItem  tmpGrpModeItem ;
         BOOLEAN         hasNodeName = FALSE ;
         BOOLEAN         hasLocation = FALSE ;
         BOOLEAN         hasHostName = FALSE ;
         BOOLEAN         hasMinKeepTime = FALSE ;
         BOOLEAN         hasMaxKeepTime = FALSE ;
         ossTimestamp    curTime ;

         // Get properties from _option object
         itr = BSONObjIterator( option ) ;
         while ( itr.more() )
         {
            optionEle = itr.next() ;

            // Get nodeName
            if ( 0 == ossStrcmp( FIELD_NAME_NODE_NAME, optionEle.fieldName() ) )
            {
               if ( String != optionEle.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Parameter [%s]'s value is not string", FIELD_NAME_NODE_NAME ) ;
                  goto error ;
               }
               hasNodeName = TRUE ;
               tmpNodeName = optionEle.valuestrsafe() ;
            }
            // Get location
            else if ( 0 == ossStrcmp( FIELD_NAME_NODE_LOCATION, optionEle.fieldName() ) )
            {
               if ( String != optionEle.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Parameter [%s]'s value is not string",
                              FIELD_NAME_NODE_LOCATION ) ;
                  goto error ;
               }
               hasLocation = TRUE ;
               tmpLocation = optionEle.valuestrsafe() ;
            }
            // Get hostname
            else if ( 0 == ossStrcmp( FIELD_NAME_HOST, optionEle.fieldName() ) )
            {
               if ( String != optionEle.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Parameter [%s]'s value is not string",
                              FIELD_NAME_HOST ) ;
                  goto error ;
               }
               hasHostName = TRUE ;
               if ( NULL != pHostName )
               {
                  *pHostName = optionEle.valuestrsafe() ;
               }
            }
            // Check Enforced field
            else if ( 0 == ossStrcmp( FIELD_NAME_ENFORCED1, optionEle.fieldName() ) )
            {
               if ( Bool != optionEle.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Patrmeter [%s]'s value is not bool", FIELD_NAME_ENFORCED1 ) ;
                  goto error ;
               }
            }
            // Get MinKeepTime field
            else if ( 0 == ossStrcmp( CAT_MIN_KEEP_TIME_NAME, optionEle.fieldName() ) )
            {
               if ( ! optionEle.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Parameter [%s]'s value is not number",
                              CAT_MIN_KEEP_TIME_NAME ) ;
                  goto error ;
               }
               hasMinKeepTime = TRUE ;
               tmpMinKeepTime = optionEle.numberInt() ;
            }
            // Get MaxKeepTime field
            else if ( 0 == ossStrcmp( CAT_MAX_KEEP_TIME_NAME, optionEle.fieldName() ) )
            {
               if ( ! optionEle.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Parameter [%s]'s value is not number",
                              CAT_MAX_KEEP_TIME_NAME ) ;
                  goto error ;
               }
               hasMaxKeepTime = TRUE ;
               tmpMaxKeepTime = optionEle.numberInt() ;
            }
            // Get unknown input
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Parameter [%s] is unknown", optionEle.fieldName() ) ;
               goto error ;
            }
         }

         // Check if any required parameters are missing
         if ( ( ! hasNodeName ) && ( ! hasLocation ) && ( NULL == pHostName ) )
         {
            rc = SDB_OUT_OF_BOUND ;
            PD_LOG_MSG( PDERROR, "Parameter [%s] or [%s] is missing",
                        FIELD_NAME_NODE_NAME, FIELD_NAME_LOCATION ) ;
            goto error ;
         }
         else if ( ( ! hasLocation ) && ( ! hasHostName ) && ( NULL != pHostName ) )
         {
            rc = SDB_OUT_OF_BOUND ;
            PD_LOG_MSG( PDERROR, "Parameter [%s] or [%s] is missing",
                        FIELD_NAME_LOCATION, FIELD_NAME_HOST ) ;
            goto error ;
         }
         else if ( parseTime && ! ( hasMinKeepTime && hasMaxKeepTime ) )
         {
            rc = SDB_OUT_OF_BOUND ;
            PD_LOG_MSG( PDERROR, "Parameter [%s] or [%s] is missing",
                        CAT_MIN_KEEP_TIME_NAME, CAT_MAX_KEEP_TIME_NAME ) ;
            goto error ;
         }

         // Validate the parameters

         // Validate node or location
         if ( hasNodeName && NULL == pHostName )
         {
            BOOLEAN isExists = FALSE ;
            UINT16 tmpNodeID = CAT_INVALID_NODEID ;

            rc = catGetNodeIDByNodeName( groupObj, tmpNodeName, tmpNodeID, isExists ) ;
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG_MSG( PDERROR, "NodeName: [%s] is invalid", tmpNodeName.c_str() ) ;
               goto error ;
            }
            else if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to get nodeID by nodeName[%s] from group[%s]",
                       tmpNodeName.c_str(), groupObj.toPoolString().c_str() ) ;
               goto error ;
            }
            else if ( ! isExists )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "NodeName: [%s] is not exist", tmpNodeName.c_str() ) ;
               goto error ;
            }
            tmpGrpModeItem.nodeName = tmpNodeName.c_str() ;
            tmpGrpModeItem.nodeID = tmpNodeID ;
         }
         else if ( hasLocation )
         {
            UINT32 tmpLocationID = CAT_INVALID_LOCATIONID ;

            if ( tmpLocation.empty() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Location: [%s] can't be null", tmpLocation.c_str() ) ;
               goto error ;
            }
            else if ( MSG_LOCATION_NAMESZ < tmpLocation.size() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
               goto error ;
            }

            rc = catGetLocationID( groupObj, tmpLocation.c_str(), tmpLocationID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check location[%s] info in group[%u]",
                         tmpLocation.c_str(), groupID ) ;

            if ( CAT_INVALID_LOCATIONID == tmpLocationID )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Location:[%s] doesn't exist in group[%u]",
                           tmpLocation.c_str(), groupID ) ;
               goto error ;
            }
            tmpGrpModeItem.location = tmpLocation.c_str() ;
            tmpGrpModeItem.locationID = tmpLocationID ;
         }

         // If the command is stop group mode, we don't need to validate time parameter
         if ( parseTime )
         {
            // Validate MinKeepTime and MaxKeepTime
            if ( CLS_GROUP_MODE_KEEP_TIME_MIN > tmpMinKeepTime ||
                 CLS_GROUP_MODE_KEEP_TIME_MAX < tmpMinKeepTime ||
                 CLS_GROUP_MODE_KEEP_TIME_MIN > tmpMaxKeepTime ||
                 CLS_GROUP_MODE_KEEP_TIME_MAX < tmpMaxKeepTime )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Parameter [%s: %d] or [%s: %d] is not in range: [%d, %d]",
                           CAT_MIN_KEEP_TIME_NAME, tmpMinKeepTime,
                           CAT_MAX_KEEP_TIME_NAME, tmpMaxKeepTime,
                           CLS_GROUP_MODE_KEEP_TIME_MIN,
                           CLS_GROUP_MODE_KEEP_TIME_MAX ) ;
               goto error ;
            }
            else if ( tmpMinKeepTime > tmpMaxKeepTime )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Parameter [%s: %d] shoule be less or equal than [%s: %d]",
                           CAT_MIN_KEEP_TIME_NAME, tmpMinKeepTime,
                           CAT_MAX_KEEP_TIME_NAME, tmpMaxKeepTime ) ;
               goto error ;
            }
            ossGetCurrentTime( curTime ) ;

            // The unit of input: tmpMinKeepTime and tmpMaxKeepTime is minute,
            // so we need to convert it to second
            tmpGrpModeItem.minKeepTime = curTime ;
            tmpGrpModeItem.minKeepTime.time += tmpMinKeepTime * 60 ;

            tmpGrpModeItem.maxKeepTime = curTime ;
            tmpGrpModeItem.maxKeepTime.time += tmpMaxKeepTime * 60 ;

            // Set current time to update time
            tmpGrpModeItem.updateTime = curTime ;
         }

         groupMode.grpModeInfo.push_back( tmpGrpModeItem ) ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CAT_PARSE_GRPMODE_INFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATPRASEFUNC, "catPraseFunc" )
   INT32 catPraseFunc( const BSONObj &func, BSONObj &parsed )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATPRASEFUNC ) ;
      BSONElement fValue = func.getField( FMP_FUNC_VALUE ) ;
      BSONElement fType = func.getField( FMP_FUNC_TYPE ) ;
      if ( fValue.eoo() || fType.eoo() )
      {
         PD_LOG( PDERROR, "failed to find specific element from func:%s",
                 func.toString().c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( Code != fValue.type() || NumberInt != fType.type())
      {
         PD_LOG( PDERROR, "invalid type of func element:%d, %d",
                 fValue.type(), fType.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         BSONObjBuilder builder ;
         const CHAR *nameBegin = NULL ;
         BOOLEAN appendBegun = FALSE ;
         std::string name ;
         const CHAR *fStr = ossStrstr(fValue.valuestr(),
                                      FMP_FUNCTION_DEF) ;
         if ( NULL == fStr )
         {
            PD_LOG( PDERROR, "can not find \"function\" in funcelement" ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         nameBegin = fStr + ossStrlen( FMP_FUNCTION_DEF ) ;
         while ( '\0' != *nameBegin )
         {
            if ( '(' == *nameBegin )
            {
               break ;
            }
            else if ( ' ' == *nameBegin && appendBegun )
            {
               break ;
            }
            else if ( ' ' != *nameBegin )
            {
               name.append( 1, *nameBegin ) ;
               appendBegun = TRUE ;
               ++nameBegin ;
            }
            else
            {
               ++nameBegin ;
            }
         }

         if ( name.empty() )
         {
            PD_LOG( PDERROR, "can not find func name" ) ;
            rc = SDB_INVALIDARG ;
         }

         builder.append( FMP_FUNC_NAME, name ) ;
         builder.append( fValue ) ;
         builder.append( fType ) ;
         parsed = builder.obj() ;
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATPRASEFUNC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   UINT32 catCalcBucketID( const CHAR *pData, UINT32 length,
                           UINT32 bucketSize )
   {
      md5::md5digest digest ;
      md5::md5( pData, length, digest ) ;
      UINT32 hashValue = 0 ;
      UINT32 i = 0 ;
      while ( i++ < 4 )
      {
         hashValue |= ( (UINT32)digest[i-1] << ( 32 - 8 * i ) ) ;
      }
      return ( hashValue % bucketSize ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCREATECONTEXT, "catCreateContext" )
   INT32 catCreateContext ( MSG_TYPE cmdType,
                            catContextPtr &context,
                            SINT64 &contextID,
                            _pmdEDUCB *pEDUCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCREATECONTEXT ) ;

      pmdKRCB *krcb = pmdGetKRCB();
      _SDB_RTNCB *pRtnCB = krcb->getRTNCB();

      RTN_CONTEXT_TYPE contextType ;
      /// Map command to context
      switch ( cmdType )
      {
      case MSG_CAT_CREATE_COLLECTION_REQ :
         contextType = RTN_CONTEXT_CAT_CREATE_CL ;
         break ;
      case MSG_CAT_DROP_COLLECTION_REQ :
         contextType = RTN_CONTEXT_CAT_DROP_CL ;
         break ;
      case MSG_CAT_DROP_SPACE_REQ :
         contextType = RTN_CONTEXT_CAT_DROP_CS ;
         break ;
      case MSG_CAT_ALTER_CS_REQ :
         contextType = RTN_CONTEXT_CAT_ALTER_CS ;
         break ;
      case MSG_CAT_RENAME_CS_REQ :
         contextType = RTN_CONTEXT_CAT_RENAME_CS ;
         break ;
      case MSG_CAT_RENAME_CL_REQ :
         contextType = RTN_CONTEXT_CAT_RENAME_CL ;
         break ;
      case MSG_CAT_ALTER_COLLECTION_REQ :
         contextType = RTN_CONTEXT_CAT_ALTER_CL ;
         break ;
      case MSG_CAT_LINK_CL_REQ :
         contextType = RTN_CONTEXT_CAT_LINK_CL ;
         break ;
      case MSG_CAT_UNLINK_CL_REQ :
         contextType = RTN_CONTEXT_CAT_UNLINK_CL ;
         break ;
      case MSG_CAT_CREATE_IDX_REQ :
         contextType = RTN_CONTEXT_CAT_CREATE_IDX ;
         break ;
      case MSG_CAT_DROP_IDX_REQ :
         contextType = RTN_CONTEXT_CAT_DROP_IDX ;
         break ;
      case MSG_CAT_TRUNCATE_REQ :
         contextType = RTN_CONTEXT_CAT_TRUNCATE_CL ;
         break ;
      case MSG_CAT_CREATE_NODE_REQ :
         contextType = RTN_CONTEXT_CAT_CREATE_NODE ;
         break ;
      case MSG_CAT_DEL_NODE_REQ :
         contextType = RTN_CONTEXT_CAT_REMOVE_NODE ;
         break ;
      case MSG_CAT_RM_GROUP_REQ :
         contextType = RTN_CONTEXT_CAT_REMOVE_GROUP ;
         break ;
      case MSG_CAT_ACTIVE_GROUP_REQ :
         contextType = RTN_CONTEXT_CAT_ACTIVE_GROUP ;
         break ;
      case MSG_CAT_SHUTDOWN_GROUP_REQ :
         contextType = RTN_CONTEXT_CAT_SHUTDOWN_GROUP ;
         break ;
      default :
         rc = SDB_INVALIDARG ;
         break ;
      }
      if ( SDB_OK == rc )
      {
         rc = pRtnCB->contextNew( contextType,
                                  context,
                                  contextID,
                                  pEDUCB ) ;
      }
      if ( SDB_OK == rc &&
           pEDUCB->getMonConfigCB()->timestampON )
      {
         context->getMonCB()->recordStartTimestamp() ;
      }

      PD_TRACE_EXITRC ( SDB_CATCREATECONTEXT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDELETECONTEXT, "catDeleteContext" )
   INT32 catDeleteContext ( SINT64 contextID,
                            _pmdEDUCB *pEDUCB )
   {
      PD_TRACE_ENTRY ( SDB_CATDELETECONTEXT ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      _SDB_RTNCB *pRtnCB = krcb->getRTNCB() ;
      if ( -1 != contextID  )
      {
         pRtnCB->contextDelete( contextID, pEDUCB ) ;
      }

      PD_TRACE_EXIT ( SDB_CATDELETECONTEXT ) ;

      return SDB_OK ;
   }

   INT32 catGetCollectionNameByUID( utilCLUniqueID clUID,
                                    string &clName,
                                    BSONObj &clInfo,
                                    _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher = BSON( CAT_CL_UNIQUEID << (INT64) clUID ) ;
      BSONObj dummyObj ;

      PD_CHECK( UTIL_IS_VALID_CLUNIQUEID(clUID), SDB_INVALIDARG, error,
                PDERROR, "Invalid cl uid(%llu)", clUID ) ;

      rc = catGetOneObj( CAT_COLLECTION_INFO_COLLECTION, dummyObj, matcher,
                         dummyObj, cb, clInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get obj(%s) from %s, rc: %d",
                   matcher.toString().c_str(), CAT_COLLECTION_INFO_COLLECTION,
                   rc ) ;

      rc = rtnGetSTDStringElement( clInfo, CAT_CATALOGNAME_NAME, clName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field(%s):cl=%s,rc=%d",
                   CAT_CATALOGNAME_NAME, clInfo.toString().c_str(), rc ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCOLLETION, "catGetCollection" )
   INT32 catGetCollection ( const string &clName, BSONObj &boCollection,
                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETCOLLETION ) ;

      BOOLEAN isExist = FALSE ;

      rc = catCheckCollectionExist( clName.c_str(), isExist, boCollection, cb );
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get info of collection [%s], rc: %d",
                   clName.c_str(), rc ) ;
      PD_CHECK( isExist,
                SDB_DMS_NOTEXIST, error, PDDEBUG,
                "Collection [%s] does not exist!",
                clName.c_str() ) ;
   done :
      PD_TRACE_EXITRC ( SDB_CATGETCOLLETION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCHKCL, "catGetAndCheckCollection" )
   INT32 catGetAndCheckCollection ( const string &clName, BSONObj &boCollection,
                                    _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATGETCHKCL ) ;

      BOOLEAN isExist = FALSE ;

      try
      {
         rc = catCheckCollectionExist( clName.c_str(), isExist, boCollection, cb );
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get info of collection [%s], rc: %d",
                      clName.c_str(), rc ) ;

         if ( !isExist )
         {
            rc = catCheckCSExist( clName.c_str(), cb, isExist ) ;
            if ( isExist )
            {
               // If the cs exists, and the collection is not found, check if
               // the cs is using data source.
               BOOLEAN inMappingCS = FALSE ;
               BSONObj csMetaRecord ;
               rc = catCheckCLInPureMappingCS( clName.c_str(), cb,
                                               inMappingCS, &csMetaRecord ) ;
               PD_RC_CHECK( rc, PDERROR, "Checking if using mapping cs for "
                            "collection[%s] failed[%d]", clName.c_str(), rc ) ;
               if ( !inMappingCS )
               {
                  rc = SDB_DMS_NOTEXIST ;
                  PD_LOG( PDWARNING, "Collection[%s] does not exist, rc: %d",
                          clName.c_str(), rc ) ;
                  goto error ;
               }
               // It's a pure mapping cs, let's build a catalog record for the
               // collection.
               rc = catBuildCatalogByPureMappingCS( clName.c_str(),
                                                    csMetaRecord,
                                                    boCollection, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Build catalog information for "
                            "collection[%s] failed[%d]", clName.c_str(), rc ) ;
            }
            else if ( SDB_OK == rc )
            {
               rc = SDB_DMS_CS_NOTEXIST ;
               PD_LOG( PDWARNING,
                       "Collection[%s]'s space does not exist, rc: %d",
                       clName.c_str(), rc ) ;
               goto error ;
            }
            else
            {
               rc = SDB_DMS_NOTEXIST ;
               PD_LOG( PDWARNING, "Collection[%s] does not exist, rc: %d",
                       clName.c_str(), rc ) ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e );
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_CATGETCHKCL, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKMAINCOLLECTION, "catCheckMainCollection" )
   INT32 catCheckMainCollection ( const BSONObj &boCollection,
                                  BOOLEAN expectMain )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKMAINCOLLECTION ) ;

      // sub-collection could not be a main-collection
      BSONElement beIsMainCL = boCollection.getField( CAT_IS_MAINCL ) ;
      if ( expectMain && !beIsMainCL.booleanSafe() )
      {
         rc = SDB_INVALID_MAIN_CL ;
         goto error ;
      }
      else if ( !expectMain && beIsMainCL.booleanSafe() )
      {
         rc = SDB_INVALID_SUB_CL ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKMAINCOLLECTION, rc ) ;

      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKRELINKCOLLECTION, "catCheckRelinkCollection" )
   INT32 catCheckRelinkCollection ( const BSONObj &boCollection, string &mainCLName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKRELINKCOLLECTION ) ;

      BSONElement beMainCLName = boCollection.getField( CAT_MAINCL_NAME );
      if ( beMainCLName.type() == String )
      {
         mainCLName = beMainCLName.str();
         // TODO: May need to check mainCLName
         if ( !mainCLName.empty() ) {
            rc = SDB_RELINK_SUB_CL ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKRELINKCOLLECTION, rc ) ;

      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKLINKMULTIDSCOLLECTION, "catCheckLinkMultiDSCollection" )
   INT32 catCheckLinkMultiDSCollection( const BSONObj &boMainCL,
                                        const BSONObj &boSubCL,
                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCHECKLINKMULTIDSCOLLECTION ) ;

      try
      {
         BSONObj subObj ;
         BSONObj subCLObj ;
         BSONElement cataEle ;
         BSONElement myIDEle = boSubCL.getField( FIELD_NAME_DATASOURCE_ID ) ;
         if ( myIDEle.eoo() )
         {
            // If no data source id field in the metadata, it's a local
            // collection.
            goto done ;
         }

         cataEle = boMainCL.getField( FIELD_NAME_CATALOGINFO ) ;
         {
            const CHAR *subCLName = NULL ;
            BSONElement idEle ;
            BSONObjIterator itr( cataEle.embeddedObject() ) ;
            // Check the field 'DataSourceID' in each sub cl metadata record.
            // If it exists and its value is the same with the current cl,
            // return error.
            while ( itr.more() )
            {
               subObj = itr.next().Obj() ;
               subCLName = subObj.getStringField( FIELD_NAME_SUBCLNAME ) ;
               rc = catGetCollection( subCLName, subCLObj, cb ) ;
               PD_RC_CHECK( rc, PDERROR, "Get collection[%s] metadata "
                            "failed[%d]", subCLName, rc ) ;
               idEle = subCLObj.getField( FIELD_NAME_DATASOURCE_ID ) ;
               if ( !idEle.eoo() && idEle.valuesEqual( myIDEle ) )
               {
                  rc = SDB_OPTION_NOT_SUPPORT ;
                  PD_LOG_MSG( PDERROR, "Main collection can not attach more "
                              "then one sub collection mapping to the same "
                              "data source[%d]", rc ) ;
                  goto error ;
               }
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCHECKLINKMULTIDSCOLLECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCOLLECTIONGROUPS, "catGetCollectionGroups" )
   INT32 catGetCollectionGroups ( const BSONObj &boCollection,
                                  vector<UINT32> &groupIDList,
                                  vector<string> &groupNameList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETCOLLECTIONGROUPS ) ;

      BSONElement beCataInfo = boCollection.getField( CAT_CATALOGINFO_NAME ) ;
      if ( Array != beCataInfo.type() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
         BSONObj boCataInfo = beCataInfo.embeddedObject() ;
         BSONObjIterator iterArr( boCataInfo ) ;
         while ( iterArr.more() )
         {
            BSONElement beTmp = iterArr.next();
            if ( Object != beTmp.type() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            BSONObj boTmp = beTmp.embeddedObject();

            BSONElement beGroupId = boTmp.getField( CAT_GROUPID_NAME ) ;
            if ( !beGroupId.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            BSONElement beGroupName = boTmp.getField( CAT_GROUPNAME_NAME ) ;
            if ( String != beGroupName.type() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            groupIDList.push_back( beGroupId.numberInt() ) ;
            groupNameList.push_back( beGroupName.valuestr() ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETCOLLECTIONGROUPS, rc ) ;

      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETANDLOCKDOMAIN, "catGetAndLockDomain" )
   INT32 catGetAndLockDomain ( const std::string &domainName, BSONObj &boDomain,
                               _pmdEDUCB *cb,
                               catCtxLockMgr *pLockMgr, OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETANDLOCKDOMAIN ) ;

      try
      {
         BOOLEAN isExist = FALSE ;
         rc = catCheckDomainExist( domainName.c_str(), isExist, boDomain, cb ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get info of domain [%s], rc: %d",
                      domainName.c_str(), rc ) ;
         PD_CHECK( isExist, SDB_CAT_DOMAIN_NOT_EXIST, error, PDWARNING,
                   "Domain [%s] does not exist!",
                   domainName.c_str() ) ;

         // Lock domain
         if ( pLockMgr &&
              !pLockMgr->tryLockDomain( domainName, mode ) )
         {
            rc = SDB_LOCK_FAILED ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETANDLOCKDOMAIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETANDLOCKCOLLECTIONSPACE, "catGetAndLockCollectionSpace" )
   INT32 catGetAndLockCollectionSpace ( const string &csName, BSONObj &boSpace,
                                        _pmdEDUCB *cb,
                                        catCtxLockMgr *pLockMgr,
                                        OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETANDLOCKCOLLECTIONSPACE ) ;

      try
      {
         BOOLEAN isExist = FALSE ;
         rc = catCheckSpaceExist( csName.c_str(), isExist, boSpace, cb ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get info of collection space [%s], rc: %d",
                      csName.c_str(), rc ) ;
         PD_CHECK( isExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                   "Collection space [%s] does not exist!",
                   csName.c_str() ) ;

         // Lock collection space
         if ( pLockMgr &&
              !pLockMgr->tryLockCollectionSpace( csName, mode ) )
         {
            rc = SDB_LOCK_FAILED ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETANDLOCKCOLLECTIONSPACE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETANDLOCKCOLLECTIONSPACE_UID, "catGetAndLockCollectionSpace" )
   INT32 catGetAndLockCollectionSpace( utilCSUniqueID csUniqueID,
                                       BSONObj &boSpace,
                                       const CHAR *&csName,
                                       pmdEDUCB *cb,
                                       catCtxLockMgr *pLockMgr,
                                       OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETANDLOCKCOLLECTIONSPACE_UID ) ;

      try
      {
         BOOLEAN isExist = FALSE ;
         BSONElement ele ;

         rc = catCheckSpaceExist( NULL, csUniqueID, isExist, boSpace, cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get info of collection "
                      "space [%u], rc: %d", csUniqueID, rc ) ;
         PD_CHECK( isExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                   "Collection space [%u] does not exist!", csUniqueID ) ;

         ele = boSpace.getField( CAT_COLLECTION_SPACE_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_CAT_CORRUPTION, error, PDWARNING,
                   "Failed to get field [%s], it is not string",
                   CAT_COLLECTION_SPACE_NAME ) ;
         csName = ele.valuestr() ;

         // Lock collection space
         if ( pLockMgr &&
              !pLockMgr->tryLockCollectionSpace( csName, mode ) )
         {
            rc = SDB_LOCK_FAILED ;
            goto error ;
         }
      }
      catch( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to check collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATGETANDLOCKCOLLECTIONSPACE_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETANDLOCKCOLLECTION, "catGetAndLockCollection" )
   INT32 catGetAndLockCollection ( const string &clName, BSONObj &boCollection,
                                   _pmdEDUCB *cb,
                                   catCtxLockMgr *pLockMgr,
                                   OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATGETANDLOCKCOLLECTION ) ;

      rc = catGetAndCheckCollection( clName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get collection[%s], rc: %d",
                   clName.c_str(), rc ) ;

      // Lock sub-collection
      if ( pLockMgr &&
           !pLockMgr->tryLockCollection( clName, mode ) )
      {
         rc = SDB_LOCK_FAILED ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETANDLOCKCOLLECTION, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETANDLOCKCLGRP, "catGetAndLockCollectionGroups" )
   INT32 catGetAndLockCollectionGroups ( const BSONObj &boCollection,
                                         vector<UINT32> &groupIDList,
                                         catCtxLockMgr &lockMgr,
                                         OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETANDLOCKCLGRP ) ;

      if ( boCollection.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         vector<string> groupNameList ;
         rc = catGetCollectionGroups ( boCollection, groupIDList, groupNameList ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get group info from collection record [%s], "
                      "rc: %d",
                      boCollection.toString().c_str(), rc ) ;
         // Lock groups
         for ( UINT32 idx = 0 ; idx < groupNameList.size() ; ++idx )
         {
            if ( !lockMgr.tryLockGroup( groupNameList[idx], mode ) )
            {
               rc = SDB_LOCK_FAILED ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETANDLOCKCLGRP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCLGRPSET, "catGetCollectionGroupSet" )
   INT32 catGetCollectionGroupSet ( const BSONObj &boCollection,
                                    vector<UINT32> &groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETCLGRPSET ) ;

      std::set< UINT32 > groupIDSet ;
      std::set< UINT32 >::iterator iterGroupID ;

      if ( boCollection.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         vector<string> groupNameList ;
         rc = catGetCollectionGroups ( boCollection, groupIDList, groupNameList ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get group info from collection record [%s], "
                      "rc: %d",
                      boCollection.toString().c_str(), rc ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// Remove duplicate groups
      for ( UINT32 i = 0 ; i < groupIDList.size() ; ++i )
      {
         groupIDSet.insert( groupIDList[i] ) ;
      }
      groupIDList.clear() ;
      iterGroupID = groupIDSet.begin() ;
      while ( iterGroupID != groupIDSet.end() )
      {
         groupIDList.push_back( *iterGroupID ) ;
         ++iterGroupID ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETCLGRPSET, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCLGRPSET_SET, "catGetCollectionGroupSet" )
   INT32 catGetCollectionGroupSet ( const BSONObj &boCollection,
                                    SET_UINT32 &groupIDSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETCLGRPSET_SET ) ;

      try
      {
         BSONElement beCataInfo ;

         if ( boCollection.isEmpty() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         beCataInfo = boCollection.getField( CAT_CATALOGINFO_NAME ) ;
         if ( Array != beCataInfo.type() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
            BSONObj boCataInfo = beCataInfo.embeddedObject() ;
            BSONObjIterator iterArr( boCataInfo ) ;
            while ( iterArr.more() )
            {
               BSONElement beTmp = iterArr.next() ;
               if ( Object != beTmp.type() )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj boTmp = beTmp.embeddedObject();
                  BSONElement beGroupId = boTmp.getField( CAT_GROUPID_NAME ) ;
                  if ( !beGroupId.isNumber() )
                  {
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  groupIDSet.insert( beGroupId.numberInt() ) ;
               }
            }
         }
      }
      catch( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to get group ID set from collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATGETCLGRPSET_SET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATLOCKGROUPS, "catLockGroups" )
   INT32 catLockGroups ( vector<UINT32> &groupIDList,
                         _pmdEDUCB *cb,
                         catCtxLockMgr &lockMgr,
                         OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATLOCKGROUPS ) ;

      // Lock groups
      for ( UINT32 idx = 0 ; idx < groupIDList.size() ; ++idx )
      {
         string groupName ;
         UINT32 groupID = groupIDList[idx] ;

         if ( SDB_IS_DSID( groupID ) )
         {
            // Do nothing for data source groups.
            continue ;
         }

         rc = catGroupID2Name( groupID, groupName, cb ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to convert group id [%d] to group name, rc: %d",
                      groupID, rc ) ;

         PD_CHECK( lockMgr.tryLockGroup( groupName, mode ),
                   SDB_LOCK_FAILED, error, PDWARNING,
                   "Failed to lock group [%s]",
                   groupName.c_str() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATLOCKGROUPS, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATLOCKGROUPS_SET, "catLockGroups" )
   INT32 catLockGroups ( const CAT_GROUP_SET &groupIDSet,
                         _pmdEDUCB *cb,
                         catCtxLockMgr &lockMgr,
                         OSS_LATCH_MODE mode,
                         BOOLEAN ignoreNonExist )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATLOCKGROUPS_SET ) ;

      // Lock groups
      for ( CAT_GROUP_SET_IT iter = groupIDSet.begin() ;
            iter != groupIDSet.end() ;
            ++ iter )
      {
         string groupName ;
         UINT32 groupID = *iter ;

         if ( SDB_IS_DSID( groupID ) )
         {
            // Do nothing for data source groups.
            continue ;
         }

         rc = catGroupID2Name( groupID, groupName, cb ) ;
         if ( SDB_CLS_GRP_NOT_EXIST == rc && ignoreNonExist )
         {
            rc = SDB_OK ;
            continue ;
         }
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to convert group id [%d] to group name, rc: %d",
                      groupID, rc ) ;

         PD_CHECK( lockMgr.tryLockGroup( groupName, mode ),
                   SDB_LOCK_FAILED, error, PDWARNING,
                   "Failed to lock group [%s]",
                   groupName.c_str() ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATLOCKGROUPS_SET, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKGROUPS_ID, "catCheckGroupsByID" )
   INT32 catCheckGroupsByID ( std::vector<UINT32> &groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKGROUPS_ID ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      sdbCatalogueCB *pCatCB = krcb->getCATLOGUECB() ;

      if ( 0 == groupIDList.size() )
      {
         rc = SDB_CAT_NO_NODEGROUP_INFO ;
         goto error ;
      }

      for ( std::vector<UINT32>::iterator iterGrp = groupIDList.begin();
            iterGrp != groupIDList.end();
            ++iterGrp )
     {
         UINT32 grpID = (*iterGrp) ;
         BOOLEAN isExist = FALSE ;
         if ( !pCatCB->checkGroupActived( pCatCB->groupID2Name( grpID ),
                                          isExist ) )
         {
            rc = SDB_REPL_GROUP_NOT_ACTIVE ;
            if ( !isExist )
            {
               rc = SDB_CLS_GRP_NOT_EXIST ;
            }
            PD_RC_CHECK( rc, PDWARNING,
                         "The group [%s] is not active, rc: %d",
                         pCatCB->groupID2Name( grpID ), rc ) ;
         }
         if ( pCatCB->isImageEnabled() &&
              !pCatCB->getCatDCMgr()->groupInImage( grpID ) )
         {
            // the group that has no image can't be as the collection location
            PD_LOG( PDWARNING,
                    "The group [%s] that has no image can't "
                    "be as the collection's location when image is enabled",
                    pCatCB->groupID2Name( grpID ) ) ;
            rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKGROUPS_ID, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKGROUPS_NAME, "catCheckGroupsByName" )
   INT32 catCheckGroupsByName ( std::vector<std::string> &groupNameList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKGROUPS_NAME ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      sdbCatalogueCB *pCatCB = krcb->getCATLOGUECB() ;

      if ( 0 == groupNameList.size() )
      {
         rc = SDB_CAT_NO_NODEGROUP_INFO ;
         goto error ;
      }

      for ( std::vector<std::string>::iterator iterGrp = groupNameList.begin();
            iterGrp != groupNameList.end();
            ++iterGrp )
     {
         std::string groupName = (*iterGrp) ;
         BOOLEAN isExist = FALSE ;
         if ( !pCatCB->checkGroupActived( groupName.c_str(), isExist ) )
         {
            rc = SDB_REPL_GROUP_NOT_ACTIVE ;
            if ( !isExist )
            {
               rc = SDB_CLS_GRP_NOT_EXIST ;
            }
            PD_RC_CHECK( rc, PDWARNING,
                         "The group [%s] is not active, rc: %d",
                         groupName.c_str(), rc ) ;
         }
         if ( pCatCB->isImageEnabled() &&
              !pCatCB->getCatDCMgr()->groupInImage( groupName ) )
         {
            // the group that has no image can't be as the collection location
            PD_LOG( PDWARNING,
                    "The group [%s] that has no image can't "
                    "be as the collection's location when image is enabled",
                    groupName.c_str() ) ;
            rc = SDB_CAT_GROUP_HASNOT_IMAGE ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKGROUPS_NAME, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATMIANCLRENAME, "catMainCLRename" )
   INT32 catMainCLRename( const string &mainCLName, const string &newMainCLName,
                          clsCatalogSet &mainclCata,
                          _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATMIANCLRENAME ) ;

      CLS_SUBCL_LIST subCLLst ;
      CLS_SUBCL_LIST_IT iterSubCL ;

      rc = mainclCata.getSubCLList( subCLLst );
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get subcl list of collection[%s], rc: %d",
                   mainCLName.c_str(), rc ) ;

      iterSubCL = subCLLst.begin() ;
      while( iterSubCL != subCLLst.end() )
      {
         const std::string& subCLName = (*iterSubCL) ;

         BSONObj setObj = BSON( CAT_MAINCL_NAME << newMainCLName ) ;
         rc = catUpdateCatalog( subCLName.c_str(), setObj, BSONObj(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update collection space[%s], rc: %d",
                      subCLName.c_str(), rc ) ;

         iterSubCL++ ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATMIANCLRENAME, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSUBCLRENAME, "catSubCLRename" )
   INT32 catSubCLRename( const string &subCLName, const string &newSubCLName,
                         clsCatalogSet &subclCata,
                         _pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATSUBCLRENAME ) ;

      std::string mainCLName = subclCata.getMainCLName() ;
      SDB_ASSERT( !mainCLName.empty(), "main-collection must be not empty!" ) ;

      clsCatalogSet mainclCata( mainCLName.c_str() );

      BSONObj mainclInfo ;
      rc = catGetCollection( mainCLName, mainclInfo, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get catalog-info of collection[%s], rc: %d",
                   mainCLName.c_str(), rc ) ;

      rc = mainclCata.updateCatSet( mainclInfo ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to parse catalog info[%s], rc: %d",
                   mainCLName.c_str(), rc ) ;

      rc = mainclCata.renameSubCL( subCLName.c_str(), newSubCLName.c_str() ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rename the subcl[%s] from maincl [%s], rc: %d",
                   subCLName.c_str(), mainCLName.c_str(), rc ) ;

      {
         // Update the catalog
         BSONObj subListObj = mainclCata.toCataInfoBson() ;
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj,
                                BSONObj(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update the catalog of maincl [%s], rc: %d",
                      mainCLName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATSUBCLRENAME, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPCSSTEP, "catDropCSStep" )
   INT32 catDropCSStep ( const string &csName,
                         _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                         INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATDROPCSSTEP ) ;

      BSONObj boMatcher, dummyObj ;

      // 1) Remove tasks with the collection space
      rc = catRemoveCSTasks( csName.c_str(), cb, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to remove tasks with the collection [%s], rc: %d",
                   csName.c_str(), rc ) ;

      // 2) Remove indexes with the collection space
      rc = catRemoveCSIndexes( csName.c_str(), cb, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to remove indexes with the collection [%s], rc: %d",
                   csName.c_str(), rc ) ;

      // 3) Remove collections with the collection space
      try
      {
         boMatcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
      }

      rc = rtnDelete( CAT_COLLECTION_SPACE_COLLECTION, boMatcher, dummyObj,
                      0, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to delete record from collection [%s] with match [%s], "
                   "rc: %d",
                   CAT_COLLECTION_SPACE_COLLECTION,
                   boMatcher.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATDROPCSSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRENAMECSSTEP, "catRenameCSStep" )
   INT32 catRenameCSStep ( const string &oldCSName, const string &newCSName,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRENAMECSSTEP ) ;

      BSONObj setObj = BSON( CAT_COLLECTION_SPACE_NAME << newCSName ) ;

      rc = catUpdateCS( oldCSName.c_str(), setObj, BSONObj(), cb,
                        pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update collection space[%s], rc: %d",
                   oldCSName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATRENAMECSSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRENAMECLSTEP, "catRenameCLStep" )
   INT32 catRenameCLStep ( const string &oldCLName, const string &newCLName,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATRENAMECLSTEP ) ;

      clsCatalogSet cataSet( oldCLName.c_str() );
      BSONObj setObj, boCollection ;
      INT32 curVersion = 0 ;
      INT32 newCLVersion = 0 ;
      string csName, newCSName, clShortName, newCLShortName ;

      /// get cl info
      rc = catGetCollection( oldCLName, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get catalog-info of collection[%s], rc: %d",
                   oldCLName.c_str(), rc ) ;

      rc = rtnGetIntElement( boCollection, CAT_VERSION_NAME, curVersion ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get version of collection[%s], rc: %d",
                   oldCLName.c_str(), rc ) ;

      rc = cataSet.updateCatSet( boCollection ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to parse catalog info[%s], rc: %d",
                   oldCLName.c_str(), rc ) ;

      /// process this cl name
      newCLVersion = catGetBucketVersion( newCLName.c_str(), cb ) ;
      if ( ( curVersion + 1 ) > newCLVersion )
      {
         newCLVersion = curVersion + 1 ;
      }
      setObj = BSON( CAT_COLLECTION_NAME << newCLName <<
                     CAT_VERSION_NAME << newCLVersion ) ;
      rc = catUpdateCatalog( oldCLName.c_str(), setObj, BSONObj(),
                             cb, w, FALSE ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update collection space[%s], rc: %d",
                   oldCLName.c_str(), rc ) ;

      rc = catSaveBucketVersion( oldCLName.c_str(), curVersion, cb, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING,
                 "Failed to save history version of collection [%s], rc: %d",
                 oldCLName.c_str(), rc ) ;
      }

      /// process cs
      csName         = dmsGetCSNameFromFullName( oldCLName ) ;
      newCSName      = dmsGetCSNameFromFullName( newCLName ) ;
      clShortName    = dmsGetCLShortNameFromFullName( oldCLName ) ;
      newCLShortName = dmsGetCLShortNameFromFullName( newCLName ) ;
      if ( csName == newCSName && clShortName != newCLShortName )
      {
         rc = catRenameCLFromCS( csName, clShortName, newCLShortName,
                                 cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to rename collection[%s] from cs[%s], rc: %d",
                      clShortName.c_str(), csName.c_str(), rc ) ;
      }

      /// process its subcl or maincl
      if ( cataSet.isMainCL() )
      {
         rc = catMainCLRename( oldCLName, newCLName, cataSet, cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to rename maincl[%s], rc: %d",
                      oldCLName.c_str(), rc ) ;
      }
      else
      {
         const std::string& mainCLName = cataSet.getMainCLName() ;
         if ( !mainCLName.empty() )
         {
            rc = catSubCLRename( oldCLName, newCLName, cataSet, cb, w ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to rename subcl[%s], rc: %d",
                         oldCLName.c_str(), rc ) ;
         }
      }

      rc = catRenameCLInTasks( oldCLName.c_str(), newCLName.c_str(), cb, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rename cl[%s] in tasks, rc: %d",
                   oldCLName.c_str(), rc ) ;

      rc = catRenameCLInIndexes( oldCLName.c_str(), newCLName.c_str(),
                                 cb, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rename cl[%s] in indexes, rc: %d",
                   oldCLName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATRENAMECLSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCREATECLSTEP, "catCreateCLStep" )
   INT32 catCreateCLStep ( const string &clName, utilCLUniqueID clUniqueID,
                           BSONObj &boCollection, _pmdEDUCB *cb,
                           SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCREATECLSTEP ) ;

      CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR szCollection[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      // split collection full name to csname and clname
      rc = rtnResolveCollectionName( clName.c_str(),
                                     clName.size(),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to resolve collection name: %s",
                    clName.c_str() ) ;

      // insert to system collection of meta data.
      rc = rtnInsert( CAT_COLLECTION_INFO_COLLECTION, boCollection,
                      1, 0, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed insert record [%s] to collection [%s], rc: %d",
                   boCollection.toString().c_str(),
                   CAT_COLLECTION_INFO_COLLECTION, rc ) ;

      // update collection space info
      rc = catAddCL2CS( szSpace, szCollection, clUniqueID,
                        cb, pDmsCB, pDpsCB, w ) ;
      if ( SDB_OK != rc )
      {
         /// Rollback immediately instead of waiting for a kill context signal
         PD_LOG( PDWARNING,
                 "Failed to add collection [%s] into space [%s], rc: %d",
                 clName.c_str(), szSpace, rc ) ;
         goto rollback ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCREATECLSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   rollback :
      INT32 tmpRC = catRemoveCL( clName.c_str(), cb, pDmsCB, pDpsCB, w ) ;
      if ( SDB_OK != tmpRC )
      {
         PD_LOG( PDWARNING,
                 "Failed to rollback insert collection [%s], rc: %d",
                 clName.c_str(), tmpRC ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPCLSTEP, "catDropCLStep" )
   INT32 catDropCLStep ( const string &clName, INT32 version, BOOLEAN delFromCS,
                         _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                         INT16 w, BOOLEAN rmTaskAndIdx )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATDROPCLSTEP ) ;

      BSONObj boCollection ;

      if ( rmTaskAndIdx )
      {
         // 1) Remove tasks with the collection
         rc = catRemoveCLTasks( clName.c_str(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to remove tasks with the collection [%s], rc: %d",
                      clName.c_str(), rc ) ;
         // 2) Remove indexes with the collection
         rc = catRemoveCLIndexes( clName.c_str(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to remove indexes with the collection [%s], rc: %d",
                      clName.c_str(), rc ) ;
      }

      rc = catGetCollection( clName, boCollection, cb ) ;
      if ( SDB_OK == rc )
      {
         // get current version for later process
         INT32 curVersion = -1 ;
         rc = rtnGetIntElement( boCollection, CAT_VERSION_NAME, curVersion ) ;
         if ( SDB_OK == rc )
         {
            version = curVersion ;
         }

         // 3) remove sequences
         rc = catDropAutoIncSequences( boCollection, cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to remove system sequences of "
                      "collection [%s], rc: %d", clName.c_str(), rc ) ;
      }
      else
      {
         PD_LOG( PDWARNING,
                 "Failed to get catalog of collection [%s], rc: %d",
                 clName.c_str(), rc ) ;
      }

      // 4) Remove the collection info
      rc = catRemoveCL( clName.c_str(), cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to remove collection [%s], rc: %d",
                   clName.c_str(), rc ) ;

      // update the latest version
      rc = catSaveBucketVersion( clName.c_str(), version, cb, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING,
                 "Failed to save history version of collection [%s], rc: %d",
                 clName.c_str(), rc ) ;
      }

      if ( delFromCS )
      {
         // 5) Pull collection from collection space info
         rc = catDelCLFromCS( clName, cb, pDmsCB, pDpsCB, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to remove collection [%s] from space, rc: %d",
                      clName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATDROPCLSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATALTERCLSTEP, "catAlterCLStep" )
   INT32 catAlterCLStep ( const std::string &clName, const BSONObj &boNewData,
                          const BSONObj & boUnsetData, _pmdEDUCB *cb,
                          SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATALTERCLSTEP ) ;

      rc = catUpdateCatalog( clName.c_str(), boNewData, boUnsetData, cb, w ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to alter collection [%s] with new info [%s], rc: %d",
                    clName.c_str(), boNewData.toString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATALTERCLSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATLINKMAINCLSTEP, "catLinkMainCLStep" )
   INT32 catLinkMainCLStep ( const string &mainCLName, const string &subCLName,
                             const BSONObj &lowBound, const BSONObj &upBound,
                             _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATLINKMAINCLSTEP ) ;

      BSONObj mainCLObj ;
      clsCatalogSet mainCLSet( mainCLName.c_str() ) ;
      clsCatalogItem *pItem = NULL ;

      rc = catGetCollection( mainCLName, mainCLObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get catalog-info of main-collection [%s], rc: %d",
                   mainCLName.c_str(), rc ) ;

      rc = mainCLSet.updateCatSet( mainCLObj ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to parse catalog-info of main-collection [%s], rc: %d",
                   mainCLName.c_str(), rc ) ;
      SDB_ASSERT( mainCLSet.isRangeSharding(),
                  "main-collection must be range-sharding!" ) ;

      if ( SDB_TIME_INVALID != mainCLSet.getLobShardingKeyFormat() )
      {
         BSONObj subCLObj ;
         clsCatalogSet subCLSet( subCLName.c_str() ) ;
         rc = catGetCollection( subCLName, subCLObj, cb ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get catalog-info of "
                      "sub-collection [%s], rc: %d", subCLName.c_str(), rc ) ;

         rc = subCLSet.updateCatSet( subCLObj ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to parse catalog-info of "
                      "sub-collection [%s], rc: %d", subCLName.c_str(), rc ) ;

         if ( subCLSet.isRangeSharding() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "SubCL[%s] can't be range sharding in lob's "
                    "MainCL[%s]", subCLName.c_str(), mainCLName.c_str() ) ;
            goto error ;
         }
      }

      rc = mainCLSet.addSubCL( subCLName.c_str(), lowBound, upBound, &pItem ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to add sub-collection [%s] into main-collection [%s], "
                   "lowBound: [%s], upBound: [%s], rc: %d",
                   subCLName.c_str(), mainCLName.c_str(),
                   lowBound.toString().c_str(), upBound.toString().c_str(), rc ) ;

      if ( pItem )
      {
         BSONObj newSubCLObj = pItem->toBson() ;
         rc = catUpdateCatalogByPush( mainCLName.c_str(), CAT_CATALOGINFO_NAME,
                                      newSubCLObj, cb, w ) ;
      }
      else
      {
         BSONObj subListObj = mainCLSet.toCataInfoBson() ;
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj, BSONObj(), cb, w ) ;
      }
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update the catalog of main-collection [%s], rc: %d",
                   mainCLName.c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATLINKMAINCLSTEP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATLINKSUBCLSTEP, "catLinkSubCLStep" )
   INT32 catLinkSubCLStep ( const string &mainCLName, const string &subCLName,
                            _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATLINKSUBCLSTEP ) ;

      BSONObj subCLObj ;
      BSONObjBuilder subClBuilder;

      rc = catGetCollection( subCLName, subCLObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get catalog-info of sub-collection [%s], rc: %d",
                   subCLName.c_str(), rc ) ;

      subClBuilder.appendElements( subCLObj ) ;
      subClBuilder.append( CAT_MAINCL_NAME, mainCLName ) ;

      {
         BSONObj newSubCLObj = subClBuilder.done() ;
         rc = catUpdateCatalog( subCLName.c_str(), newSubCLObj, BSONObj(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update the catalog of sub-collection [%s], rc: %d",
                      subCLName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATLINKSUBCLSTEP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUNLINKMAINCLSTEP, "catUnlinkMainCLStep" )
   INT32 catUnlinkMainCLStep ( const string &mainCLName,
                               const string &subCLName,
                               BOOLEAN needBounds,
                               BSONObj &lowBound, BSONObj &upBound,
                               _pmdEDUCB *cb,
                               SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                               INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATUNLINKMAINCLSTEP ) ;

      BSONObj mainCLObj ;
      clsCatalogSet mainCLSet( mainCLName.c_str() ) ;

      // Update the object first
      rc = catGetCollection( mainCLName, mainCLObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "failed to get catalog-info of main-collection(%s)",
                   mainCLName.c_str() ) ;

      // Parse the object to get the sub-collection list
      rc = mainCLSet.updateCatSet( mainCLObj ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to parse catalog-info of main-collection(%s)",
                   mainCLName.c_str() ) ;
      PD_CHECK( mainCLSet.isRangeSharding() && mainCLSet.isMainCL(),
                SDB_INVALID_MAIN_CL, error, PDWARNING,
                "main-collection must be range-sharding!" ) ;

      if ( needBounds )
      {
         INT32 tmprc = SDB_OK ;
         BSONObj tmpLowBound, tmpUpBound ;

         // Copy bounds for rollback
         tmprc = mainCLSet.getSubCLBounds( subCLName, tmpLowBound, tmpUpBound) ;
         // No need to delete, post a warning
         if ( SDB_OK != tmprc )
         {
            PD_LOG( PDWARNING,
                    "Failed to get sub-collection [%s] info from "
                    "main-collection [%s], rc: %d",
                    subCLName.c_str(), mainCLName.c_str(), tmprc ) ;
         }
         else
         {
            // Make sure the objects are owned for later process
            lowBound = tmpLowBound.getOwned() ;
            upBound = tmpUpBound.getOwned() ;
         }
      }

      // Delete the sub-collection from list
      rc = mainCLSet.delSubCL( subCLName.c_str() ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to delete the sub-collection [%s] from "
                   "main-collection [%s], rc: %d",
                   subCLName.c_str(), mainCLName.c_str(), rc ) ;

      {
         // Update the catalog
         BSONObj subListObj = mainCLSet.toCataInfoBson() ;
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj, BSONObj(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update the catalog of "
                      "main-collection [%s], rc: %d",
                      mainCLName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATUNLINKMAINCLSTEP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUNLINKCSSTEP, "catUnlinkCSStep" )
   INT32 catUnlinkCSStep ( const string &mainCLName, const string &csName,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATUNLINKMAINCLSTEP ) ;

      BSONObj mainCLObj ;
      clsCatalogSet mainCLSet( mainCLName.c_str() ) ;
      CLS_SUBCL_LIST subCLLst ;
      CHAR szCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR szCLName[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      // Update the object first
      rc = catGetCollection( mainCLName, mainCLObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "failed to get catalog-info of main-collection(%s)",
                   mainCLName.c_str() ) ;

      // Parse the object to get the sub-collection list
      rc = mainCLSet.updateCatSet( mainCLObj ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to parse catalog-info of main-collection(%s)",
                   mainCLName.c_str() ) ;
      PD_CHECK( mainCLSet.isRangeSharding() && mainCLSet.isMainCL(),
                SDB_INVALID_MAIN_CL, error, PDWARNING,
                "main-collection must be range-sharding!" ) ;

      rc = mainCLSet.getSubCLList( subCLLst ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get sub-collection list of main-collection(%s)",
                   mainCLName.c_str() ) ;

      for ( CLS_SUBCL_LIST_IT iter = subCLLst.begin() ;
            iter != subCLLst.end() ;
            ++ iter )
      {
         string &subCLName = (*iter) ;
         rc = rtnResolveCollectionName( subCLName.c_str(), subCLName.size(),
                                        szCSName, DMS_COLLECTION_SPACE_NAME_SZ,
                                        szCLName, DMS_COLLECTION_NAME_SZ ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to resolve collection name [%s] failed, rc: %d",
                      subCLName.c_str(), rc ) ;

         if ( 0 == csName.compare( szCSName ) )
         {
            // Delete the sub-collection from list
            rc = mainCLSet.delSubCL( subCLName.c_str() ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to delete the sub-collection [%s] from "
                         "main-collection [%s], rc: %d",
                         subCLName.c_str(), mainCLName.c_str(), rc ) ;
         }
      }

      {
         // Update the catalog
         BSONObj subListObj = mainCLSet.toCataInfoBson() ;
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj, BSONObj(), cb, w ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to update the catalog of "
                      "main-collection [%s], rc: %d",
                      mainCLName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATUNLINKMAINCLSTEP, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUNLINKSUBCLSTEP, "catUnlinkSubCLStep" )
   INT32 catUnlinkSubCLStep ( const string &subCLName,
                              _pmdEDUCB *cb,
                              SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                              INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATUNLINKSUBCLSTEP ) ;

      rc = catUpdateCatalogByUnset( subCLName.c_str(), CAT_MAINCL_NAME, cb, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update the catalog of sub-collection [%s], rc: %d",
                   subCLName.c_str(), rc ) ;
   done :
      PD_TRACE_EXITRC ( SDB_CATUNLINKSUBCLSTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKANDBUILDCATARECORD, "catCheckAndBuildCataRecord" )
   INT32 catCheckAndBuildCataRecord( const BSONObj &boCollection,
                                     UINT32 &fieldMask,
                                     catCollectionInfo &clInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKANDBUILDCATARECORD ) ;
      const CHAR *origMapping = NULL ;

      clInfo.reset() ;

      fieldMask = 0 ;

      BSONObjIterator it( boCollection ) ;
      while ( it.more() )
      {
         BSONElement eleTmp = it.next() ;

         // collection name
         if ( ossStrcmp( eleTmp.fieldName(), CAT_COLLECTION_NAME ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_COLLECTION_NAME,
                      eleTmp.type() ) ;
            clInfo._pCLName = eleTmp.valuestr() ;
            fieldMask |= UTIL_CL_NAME_FIELD ;
         }
         // sharding key
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_SHARDINGKEY_NAME ) == 0 )
         {
            PD_CHECK( Object == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_SHARDINGKEY_NAME,
                      eleTmp.type() ) ;
            clInfo._shardingKey = eleTmp.embeddedObject() ;
            PD_CHECK( _ixmIndexKeyGen::validateKeyDef( clInfo._shardingKey ),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Sharding key [%s] definition is invalid",
                      clInfo._shardingKey.toString().c_str() ) ;
            fieldMask |= UTIL_CL_SHDKEY_FIELD ;
            clInfo._isSharding = TRUE ;
         }
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_LOBSHARDINGKEYFORMAT_NAME ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_LOBSHARDINGKEYFORMAT_NAME, eleTmp.type() ) ;
            PD_CHECK( clsCheckAndParseLobKeyFormat( eleTmp.valuestr() ),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Lob sharding key format [%s] definition is invalid",
                      eleTmp.valuestr() ) ;

            clInfo._lobShardingKeyFormat = eleTmp.valuestr() ;
            fieldMask |= UTIL_CL_LOBKEYFORMAT_FIELD ;
         }
         // repl size
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_CATALOG_W_NAME ) == 0 )
         {
            PD_CHECK( NumberInt == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CATALOG_W_NAME, eleTmp.type() ) ;
            clInfo._replSize = eleTmp.numberInt() ;
            if ( 1 <= clInfo._replSize &&
                 clInfo._replSize <= CLS_REPLSET_MAX_NODE_SIZE )
            {
               /// do nothing.
            }
            else if ( clInfo._replSize == 0 )
            {
               clInfo._replSize = CLS_REPLSET_MAX_NODE_SIZE ;
            }
            else if ( -1 == clInfo._replSize )
            {
               /// do nothing
            }
            else
            {
               PD_LOG( PDWARNING,
                       "Invalid repl size: %d",
                       clInfo._replSize ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            fieldMask |= UTIL_CL_REPLSIZE_FIELD ;
         }
         // consistency strategy
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_CATALOG_CONSISTENCYSTRATEGY ) == 0 )
         {
            PD_CHECK( NumberInt == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CATALOG_CONSISTENCYSTRATEGY, eleTmp.type() ) ;
            UINT32 consistencyStrategy = eleTmp.numberInt() ;
            if ( SDB_CONSISTENCY_NODE <= consistencyStrategy &&
                 SDB_CONSISTENCY_PRY_LOC_MAJOR >=
                 consistencyStrategy )
            {
               clInfo._consistencyStrategy =
                  (SDB_CONSISTENCY_STRATEGY) consistencyStrategy ;
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Invalid consistency strategy: %d",
                           consistencyStrategy ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            fieldMask |= UTIL_CL_CONSISTENCYSTRATEGY_FIELD ;
         }
         // ensure sharding index
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_ENSURE_SHDINDEX ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_ENSURE_SHDINDEX,
                      eleTmp.type() ) ;
            clInfo._enSureShardIndex = eleTmp.Bool() ;
            fieldMask |= UTIL_CL_ENSURESHDIDX_FIELD ;
         }
         // sharding type
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_SHARDING_TYPE ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_SHARDING_TYPE,
                      eleTmp.type() ) ;

            // check string value
            clInfo._pShardingType = eleTmp.valuestr() ;
            PD_CHECK( 0 == ossStrcmp( clInfo._pShardingType,
                                      CAT_SHARDING_TYPE_HASH ) ||
                      0 == ossStrcmp( clInfo._pShardingType,
                                      CAT_SHARDING_TYPE_RANGE ),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] value [%s] should be [%s/%s]",
                      CAT_SHARDING_TYPE, clInfo._pShardingType,
                      CAT_SHARDING_TYPE_HASH, CAT_SHARDING_TYPE_RANGE ) ;
            fieldMask |= UTIL_CL_SHDTYPE_FIELD ;

            clInfo._isHash = ( 0 == ossStrcmp( clInfo._pShardingType,
                                               CAT_SHARDING_TYPE_HASH ) ) ;
         }
         // sharding partition
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_SHARDING_PARTITION ) == 0 )
         {
            PD_CHECK( NumberInt == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_SHARDING_PARTITION, eleTmp.type() ) ;
            clInfo._shardPartition = eleTmp.numberInt() ;
            // must be the power of 2
            PD_CHECK( ossIsPowerOf2( (UINT32)clInfo._shardPartition ),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] value must be power of 2",
                      CAT_SHARDING_PARTITION ) ;
            PD_CHECK( clInfo._shardPartition >= CAT_SHARDING_PARTITION_MIN &&
                      clInfo._shardPartition <= CAT_SHARDING_PARTITION_MAX,
                      SDB_INVALIDARG, error, PDWARNING, "Field[%s] value[%d] "
                      "should between in[%d, %d]", CAT_SHARDING_PARTITION,
                      clInfo._shardPartition, CAT_SHARDING_PARTITION_MIN,
                      CAT_SHARDING_PARTITION_MAX ) ;
            fieldMask |= UTIL_CL_PARTITION_FIELD ;
         }
         // compression flag
         else if ( ossStrcmp ( eleTmp.fieldName(),
                               CAT_COMPRESSED ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_COMPRESSED, eleTmp.type() ) ;
            clInfo._isCompressed = eleTmp.boolean() ;
            fieldMask |= UTIL_CL_COMPRESSED_FIELD ;
         }
         // main-collection flag
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_IS_MAINCL ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_IS_MAINCL, eleTmp.type() ) ;
            clInfo._isMainCL = eleTmp.boolean() ;
            fieldMask |= UTIL_CL_ISMAINCL_FIELD ;
            if ( !( fieldMask & UTIL_CL_SHDTYPE_FIELD ) )
            {
               clInfo._pShardingType = CAT_SHARDING_TYPE_RANGE ;
               clInfo._isHash = FALSE ;
            }
         }
         // strictDataMode flag
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_STRICTDATAMODE ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_STRICTDATAMODE, eleTmp.type() ) ;
            clInfo._strictDataMode = eleTmp.boolean() ;
            fieldMask |= UTIL_CL_STRICTDATAMODE_FIELD ;
         }
         // no trans flag
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_NOTRANS ) == 0 )
         {
            clInfo._noTrans = eleTmp.boolean() ;
            fieldMask |= UTIL_CL_NOTRANS_FIELD ;
         }
         // group specified
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_GROUP_NAME ) )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_GROUP_NAME, eleTmp.type() ) ;
            if ( 0 == ossStrcasecmp( eleTmp.valuestr(),
                                     CAT_ASSIGNGROUP_FOLLOW ) )
            {
               clInfo._assignType = ASSIGN_FOLLOW ;
            }
            else if ( 0 == ossStrcasecmp( eleTmp.valuestr(),
                                          CAT_ASSIGNGROUP_RANDOM ) )
            {
               clInfo._assignType = ASSIGN_RANDOM ;
            }
            else
            {
               clInfo._gpSpecified = eleTmp.valuestr() ;
            }
         }
         // auto split
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_DOMAIN_AUTO_SPLIT ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_DOMAIN_AUTO_SPLIT, eleTmp.type() ) ;
            clInfo._autoSplit = eleTmp.Bool() ;
            fieldMask |= UTIL_CL_AUTOSPLIT_FIELD ;
         }
         // auto rebalance
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_DOMAIN_AUTO_REBALANCE ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_DOMAIN_AUTO_REBALANCE, eleTmp.type() ) ;
            clInfo._autoRebalance = eleTmp.Bool() ;
            fieldMask |= UTIL_CL_AUTOREBALANCE_FIELD ;
         }
         // auto index id
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_AUTO_INDEX_ID ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_AUTO_INDEX_ID, eleTmp.type() ) ;
            clInfo._autoIndexId = eleTmp.Bool() ;
            fieldMask |= UTIL_CL_AUTOIDXID_FIELD ;
         }
         // compression type
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_COMPRESSIONTYPE ) )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_COMPRESSIONTYPE, eleTmp.type() ) ;
            if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_LZW ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_LZW ;
            }
            else if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_SNAPPY ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_SNAPPY ;
            }
            else if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_LZ4 ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_LZ4 ;
            }
            else if ( 0 == ossStrcmp( eleTmp.valuestr(), CAT_COMPRESSOR_ZLIB ) )
            {
               clInfo._compressorType = UTIL_COMPRESSOR_ZLIB ;
            }
            else
            {
               PD_LOG( PDWARNING,
                       "Invalid Compression Type. Field[%s] value[%s] should "
                       "be [%s|%s] or leave empty",
                       CAT_COMPRESSIONTYPE, eleTmp.valuestr(),
                       CAT_COMPRESSOR_LZW, CAT_COMPRESSOR_SNAPPY );
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            fieldMask |= UTIL_CL_COMPRESSTYPE_FIELD ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CAPPED_NAME ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CAPPED_NAME, eleTmp.type() ) ;
            clInfo._capped = eleTmp.boolean() ;
            fieldMask |= UTIL_CL_CAPPED_FIELD ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CL_MAX_RECNUM ) )
         {
            PD_CHECK( NumberLong == eleTmp.type()
                      || NumberInt == eleTmp.type()
                      || NumberDouble == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CL_MAX_RECNUM, eleTmp.type() ) ;
            PD_CHECK( eleTmp.numberLong() >= 0,
                      SDB_INVALIDARG, error, PDWARNING,
                      "Invalid Max[ %lld ] when creating capped collection",
                      eleTmp.numberLong() ) ;
            clInfo._maxRecNum = eleTmp.numberLong() ;
            fieldMask |= UTIL_CL_MAXREC_FIELD ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CL_MAX_SIZE ) )
         {
            PD_CHECK( NumberLong == eleTmp.type()
                      || NumberInt == eleTmp.type()
                      || NumberDouble == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CL_MAX_SIZE, eleTmp.type() ) ;
            PD_CHECK( eleTmp.numberLong() > 0 &&
                      eleTmp.numberLong() <= DMS_CAP_CL_SIZE,
                      SDB_INVALIDARG, error, PDWARNING,
                      "Invalid Size[ %lld ] when creating capped collection",
                      eleTmp.numberLong() ) ;
            // Always align the size upper to 32MB.
            clInfo._maxSize = ossRoundUpToMultipleX( eleTmp.numberLong() << 20,
                                                     DMS_MAX_CL_SIZE_ALIGN_SIZE ) ;
            fieldMask |= UTIL_CL_MAXSIZE_FIELD ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CL_OVERWRITE ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CL_OVERWRITE, eleTmp.type() ) ;
            clInfo._overwrite = eleTmp.Bool() ;
            fieldMask |= UTIL_CL_OVERWRITE_FIELD ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_AUTOINCREMENT ) )
         {
            PD_CHECK( Object == eleTmp.type() || Array == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_AUTOINCREMENT, eleTmp.type() ) ;
            if ( Object == eleTmp.type() )
            {
               BSONObj options ;
               options = eleTmp.Obj() ;
               rc = clsAutoIncItem::validAutoIncOption( options ) ;
               PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
               rc = catValidSequenceOption( options ) ;
               PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
            }
            else if( Array == eleTmp.type() )
            {
               BSONObjIterator it( eleTmp.embeddedObject() ) ;
               while ( it.more() )
               {
                  BSONElement ele ;
                  BSONObj options ;
                  ele = it.next() ;
                  PD_CHECK( Object == ele.type(), SDB_INVALIDARG, error,
                            PDWARNING, "AutoIncrement[%s] definition is invalid",
                            eleTmp.String().c_str() ) ;
                  options = ele.Obj() ;
                  rc = clsAutoIncItem::validAutoIncOption( options ) ;
                  PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
                  rc = catValidSequenceOption( options ) ;
                  PD_RC_CHECK( rc, PDWARNING, "Invalid autoIncrement options" ) ;
               }
            }
            clInfo._autoIncFields = BSON( CAT_AUTOINCREMENT <<
                                          boCollection.getField( CAT_AUTOINCREMENT ) ) ;
            fieldMask |= UTIL_CL_AUTOINC_FIELD ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), FIELD_NAME_DATASOURCE ) )
         {
            PD_CHECK( String == eleTmp.type(), SDB_INVALIDARG, error,
                      PDWARNING, "Field [%s] type [%d] error",
                      FIELD_NAME_DATASOURCE, eleTmp.type() ) ;
            BOOLEAN exist = FALSE ;
            BSONObj obj ;
            const CHAR *dsName = eleTmp.valuestrsafe() ;
            rc = catCheckDataSourceExist( dsName, exist, obj,
                                          pmdGetThreadEDUCB() ) ;
            PD_RC_CHECK( rc, PDERROR, "Check data source[%s] existence "
                         "failed[%d]", dsName, rc ) ;
            PD_CHECK( exist, SDB_CAT_DATASOURCE_NOTEXIST, error, PDERROR,
                      "Data source[%s] dose not exist", dsName ) ;
            clInfo._dsUID = obj.getIntField( FIELD_NAME_ID ) ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), FIELD_NAME_MAPPING ) )
         {
            PD_CHECK( String == eleTmp.type(), SDB_INVALIDARG, error,
                      PDWARNING, "Field [%s] type [%d] error",
                      FIELD_NAME_MAPPING, eleTmp.type() ) ;
            origMapping = eleTmp.valuestr() ;
            PD_CHECK( ossStrlen(origMapping) > 0, SDB_INVALIDARG, error,
                      PDERROR, "Mapping value is invalid" ) ;
         }
         else
         {
            PD_RC_CHECK ( SDB_INVALIDARG, PDWARNING,
                          "Unexpected field[%s] in create collection command",
                          eleTmp.toString().c_str() ) ;
         }
      }

      if ( clInfo._isMainCL )
      {
         PD_CHECK ( clInfo._isSharding,
                    SDB_NO_SHARDINGKEY, error, PDWARNING,
                    "main-collection must have ShardingKey!" );
         PD_CHECK ( !clInfo._isHash,
                    SDB_INVALID_MAIN_CL_TYPE, error, PDWARNING,
                    "the sharding-type of main-collection must be range!" );

         PD_CHECK( !( UTIL_CL_AUTOIDXID_FIELD & fieldMask ),
                   SDB_INVALIDARG, error, PDWARNING,
                   "can not set auto-index-id on main collection" ) ;
         PD_CHECK( !( ( UTIL_CL_CAPPED_FIELD & fieldMask ) ||
                      ( UTIL_CL_MAXREC_FIELD & fieldMask ) ||
                      ( UTIL_CL_MAXSIZE_FIELD & fieldMask ) ||
                      ( UTIL_CL_OVERWRITE_FIELD & fieldMask ) ),
                   SDB_INVALIDARG, error, PDWARNING,
                   "can not set Capped|Max|Size on main collection" ) ;
         // no-trans is not supported yet
         PD_CHECK( !clInfo._noTrans,
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "can not set no-trans on main collection" ) ;
      }

      if ( clInfo._autoSplit || clInfo._autoRebalance )
      {
         PD_CHECK ( clInfo._isSharding,
                    SDB_NO_SHARDINGKEY, error, PDWARNING,
                    "can not do split or rebalance with out ShardingKey!" );

         PD_CHECK ( NULL == clInfo._gpSpecified,
                    SDB_INVALIDARG, error, PDWARNING,
                    "can not do split or rebalance with out more than one group" );

         PD_CHECK( clInfo._isHash,
                   SDB_INVALIDARG, error, PDWARNING,
                   "auto options only can be set when shard type is hash" ) ;
      }

      if ( fieldMask & UTIL_CL_ENSURESHDIDX_FIELD ||
           fieldMask & UTIL_CL_SHDTYPE_FIELD ||
           fieldMask & UTIL_CL_PARTITION_FIELD )
      {
         PD_CHECK( fieldMask & UTIL_CL_SHDKEY_FIELD,
                   SDB_INVALIDARG, error, PDWARNING,
                   "these arguments are legal only when sharding key is specified." ) ;
      }

      PD_CHECK( clInfo._pCLName, SDB_INVALIDARG, error, PDWARNING,
                "Collection name not set" ) ;

      if ( clInfo._isCompressed &&
           !( fieldMask & UTIL_CL_COMPRESSTYPE_FIELD ) )
      {
         clInfo._compressorType = UTIL_COMPRESSOR_SNAPPY ;
      }

      if ( !( fieldMask & UTIL_CL_COMPRESSED_FIELD ) &&
           ( fieldMask & UTIL_CL_COMPRESSTYPE_FIELD ) )
      {
         clInfo._isCompressed = TRUE ;
         fieldMask |= UTIL_CL_COMPRESSED_FIELD ;
      }

      if ( !clInfo._isCompressed &&
           ( fieldMask & UTIL_CL_COMPRESSTYPE_FIELD ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "CompressionType can't be set when "
                     "Compressed is false" ) ;
         goto error ;
      }

      if ( clInfo._capped &&
           !( fieldMask & UTIL_CL_ENSURESHDIDX_FIELD ) )
      {
         clInfo._enSureShardIndex = FALSE ;
      }

      if ( clInfo._capped &&
           !( fieldMask & UTIL_CL_AUTOIDXID_FIELD ) )
      {
         clInfo._autoIndexId = FALSE ;
         fieldMask |= UTIL_CL_AUTOIDXID_FIELD ;
      }

      if ( !clInfo._capped &&
           !( fieldMask & UTIL_CL_COMPRESSED_FIELD ) &&
           !( fieldMask & UTIL_CL_COMPRESSTYPE_FIELD ) )
      {
         clInfo._isCompressed = TRUE ;
         fieldMask |= UTIL_CL_COMPRESSED_FIELD ;
         clInfo._compressorType = UTIL_COMPRESSOR_SNAPPY ;
      }

      if ( clInfo._capped )
      {
         if ( clInfo._isCompressed )
         {
            PD_LOG( PDWARNING,
                    "Compression is not allowed on capped collection." ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( ( clInfo._isSharding && clInfo._enSureShardIndex ) ||
              clInfo._autoIndexId )
         {
            PD_LOG( PDWARNING,
                    "Index is not allowed to be created on capped collection, "
                    "including $id index and $shard index.") ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( 0 == clInfo._maxSize )
         {
            PD_LOG( PDWARNING,
                    "Field[%s] must always be used when Capped is true",
                    CAT_CL_MAX_SIZE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // no-trans is not supported yet
         PD_CHECK( !clInfo._noTrans,
                   SDB_OPTION_NOT_SUPPORT, error, PDERROR,
                   "can not set no-trans on capped collection" ) ;
      }

      if ( clInfo._lobShardingKeyFormat != NULL )
      {
         if ( !clInfo._isMainCL )
         {
            PD_LOG( PDWARNING,
                    "Field[%s] can't be set when collection is not MainCL",
                    CAT_LOBSHARDINGKEYFORMAT_NAME ) ;
            rc = SDB_INVALIDARG ;
         }

         if ( clInfo._shardingKey.nFields() != 1 )
         {
            PD_LOG( PDWARNING,
                    "Field[%s] can't be more than one key when support lob in"
                    "MainCL",
                    CAT_SHARDINGKEY_NAME ) ;
            rc = SDB_INVALIDARG ;
         }
      }

      if ( UTIL_INVALID_DS_UID != clInfo._dsUID )
      {
         if ( origMapping )
         {
            const CHAR *dotPtr = ossStrchr( origMapping, '.' ) ;
            if ( dotPtr )
            {
               // Full name mapping
               ossStrncpy( clInfo._fullMapping, origMapping,
                           DMS_COLLECTION_FULL_NAME_SZ ) ;
            }
            else
            {
               // Short name mapping, need to append the collection space name.
               dotPtr = ossStrchr( clInfo._pCLName, '.' ) ;
               SDB_ASSERT( dotPtr, "No dot found in colleciton name" ) ;
               UINT16 csNameLen = dotPtr - clInfo._pCLName ;
               ossStrncpy( clInfo._fullMapping, clInfo._pCLName, csNameLen ) ;
               clInfo._fullMapping[ csNameLen ] = '.' ;
               ossStrncpy( clInfo._fullMapping + csNameLen + 1, origMapping,
                           DMS_COLLECTION_FULL_NAME_SZ - csNameLen - 1 ) ;
            }
         }
         else
         {
            // Mapping is not specified explicitly. Use name of the collection
            // as the mapping name.
            ossStrncpy( clInfo._fullMapping, clInfo._pCLName,
                        DMS_COLLECTION_FULL_NAME_SZ ) ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKANDBUILDCATARECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 catBuildInitRangeBound ( const BSONObj & shardingKey,
                                  const Ordering & order,
                                  BSONObj & lowBound, BSONObj & upBound )
   {
      INT32 index = 0 ;
      BSONObjBuilder lowBoundBD ;
      BSONObjBuilder upBoundBD ;

      BSONObjIterator iter( shardingKey ) ;
      while ( iter.more() )
      {
         BSONElement ele        = iter.next() ;
         const CHAR * fieldName = ele.fieldName() ;
         if ( order.get( index ) == 1 )
         {
            lowBoundBD.appendMinKey ( fieldName ) ;
            upBoundBD.appendMaxKey ( fieldName ) ;
         }
         else
         {
            lowBoundBD.appendMaxKey ( fieldName ) ;
            upBoundBD.appendMinKey ( fieldName ) ;
         }

         ++index ;
      }

      lowBound = lowBoundBD.obj () ;
      upBound = upBoundBD.obj () ;

      return SDB_OK ;
   }

   INT32 catBuildInitHashBound ( BSONObj & lowBound, BSONObj & upBound,
                                 INT32 paritition )
   {
      INT32 rc = SDB_OK ;

      lowBound = BSON("" << CAT_HASH_LOW_BOUND ) ;
      upBound = BSON("" << paritition )  ;

      return rc ;
   }

   INT32 catBuildHashBound ( BSONObj & lowBound, BSONObj & upBound,
                             INT32 beginBound, INT32 endBound )
   {
      INT32 rc = SDB_OK ;

      lowBound = BSON("" << beginBound ) ;
      upBound = BSON("" << endBound )  ;

      return rc ;
   }

   INT32 catBuildHashSplitTask ( const CHAR * collection,
                                 utilCLUniqueID clUniqueID,
                                 const CHAR * srcGroup,
                                 const CHAR * dstGroup,
                                 UINT32 beginBound,
                                 UINT32 endBound,
                                 BSONObj & splitTask )
   {
      INT32 rc = SDB_OK ;

      splitTask = BSON( CAT_COLLECTION_NAME << collection <<
                        CAT_CL_UNIQUEID << (INT64)clUniqueID <<
                        CAT_SOURCE_NAME << srcGroup <<
                        CAT_TARGET_NAME << dstGroup <<
                        CAT_SPLITVALUE_NAME << BSON( "" << beginBound ) <<
                        CAT_SPLITENDVALUE_NAME << BSON( "" << endBound ) ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATBUILDCATAAUTOINCFLD, "catBuildCatalogAutoIncField" )
   INT32 catBuildCatalogAutoIncField( _pmdEDUCB *cb,
                                      catCollectionInfo &clInfo,
                                      const BSONObj &obj,
                                      utilCLUniqueID clUniqueID ,
                                      INT16 w )
   {
      PD_TRACE_ENTRY ( SDB_CATBUILDCATAAUTOINCFLD ) ;
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      const CHAR *fieldName = NULL ;
      utilSequenceID seqID = UTIL_SEQUENCEID_NULL ;

      fieldName = obj.getField( CAT_AUTOINC_FIELD ).valuestr() ;

      builder.append( CAT_AUTOINC_FIELD, fieldName ) ;
      builder.append( CAT_AUTOINC_SEQ,
                      catGetSeqName4AutoIncFld( clUniqueID, fieldName ) ) ;
      rc = catUpdateGlobalID( cb, w, seqID );
      PD_RC_CHECK( rc, PDERROR, "Failed to get global ID for field[%s], "
                   "rc: %d", fieldName, rc ) ;
      builder.append( CAT_AUTOINC_SEQ_ID, (INT64)seqID ) ;
      if ( obj.hasField( CAT_AUTOINC_GENERATED ) )
      {
         builder.append( obj.getField( CAT_AUTOINC_GENERATED ) ) ;
      }
      else
      {
         builder.append( CAT_AUTOINC_GENERATED, CAT_GENERATED_DEFAULT ) ;
      }

      rc = clInfo._autoIncSet.insert( builder.obj() ) ;
      if(  SDB_AUTOINCREMENT_FIELD_CONFLICT == rc )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Failed to insert autoinc field "
                 "options[%s], rc: %d", PD_SECURE_OBJ( obj ), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATBUILDCATAAUTOINCFLD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // build catalogue-info record:
   // {  Name: "SpaceName.CollectionName", Version: 1,
   //    ShardingKey: { Key1: 1, Key2: -1 },
   //    CataInfo:
   //       [ { GroupID: 1000, LowBound:{ "":MinKey,"":MaxKey }, UpBound:{"":MaxKey,"":MinKey} } ] }
   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATBUILDCATARECORD, "catBuildCatalogRecord" )
   INT32 catBuildCatalogRecord( _pmdEDUCB *cb,
                                catCollectionInfo &clInfo,
                                UINT32 mask, UINT32 attribute,
                                const CAT_GROUP_SET &grpIDSet,
                                const std::map<std::string, UINT32> &splitLst,
                                BSONObj &catRecord,
                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATBUILDCATARECORD ) ;

      BSONObjBuilder builder ;
      CHAR szAttr[ 100 ] = { 0 } ;

      if ( ( mask & UTIL_CL_COMPRESSED_FIELD ) && clInfo._isCompressed )
      {
         attribute |= DMS_MB_ATTR_COMPRESSED ;
      }
      if ( ( mask & UTIL_CL_AUTOIDXID_FIELD ) && !clInfo._autoIndexId )
      {
         attribute |= DMS_MB_ATTR_NOIDINDEX ;
      }
      if ( ( mask & UTIL_CL_CAPPED_FIELD ) && clInfo._capped )
      {
         attribute |= DMS_MB_ATTR_CAPPED ;
      }
      if ( ( mask & UTIL_CL_STRICTDATAMODE_FIELD ) && clInfo._strictDataMode )
      {
         attribute |= DMS_MB_ATTR_STRICTDATAMODE ;
      }
      if ( ( mask & UTIL_CL_NOTRANS_FIELD ) && clInfo._noTrans )
      {
         attribute |= DMS_MB_ATTR_NOTRANS ;
      }
      mbAttr2String( attribute, szAttr, sizeof( szAttr ) - 1 ) ;

      if ( mask & UTIL_CL_NAME_FIELD )
      {
         builder.append( CAT_CATALOGNAME_NAME, clInfo._pCLName ) ;
      }

      builder.append( CAT_CL_UNIQUEID, (INT64)clInfo._clUniqueID ) ;


      /// this is not specified by user.
      builder.append( CAT_CATALOGVERSION_NAME,
                      0 == clInfo._version ?
                      CAT_VERSION_BEGIN :
                      clInfo._version ) ;

      if ( mask & UTIL_CL_REPLSIZE_FIELD )
      {
         builder.append( CAT_CATALOG_W_NAME, clInfo._replSize ) ;
      }

      if ( mask & UTIL_CL_CONSISTENCYSTRATEGY_FIELD )
      {
         builder.append( CAT_CATALOG_CONSISTENCYSTRATEGY,
                         clInfo._consistencyStrategy ) ;
      }

      builder.append( CAT_ATTRIBUTE_NAME, attribute ) ;
      builder.append( FIELD_NAME_ATTRIBUTE_DESC, szAttr ) ;

      /// only record the options specified by user.
      if ( ( mask & UTIL_CL_COMPRESSED_FIELD ) && clInfo._isCompressed )
      {
         builder.append( CAT_COMPRESSIONTYPE, clInfo._compressorType ) ;
         builder.append( FIELD_NAME_COMPRESSIONTYPE_DESC,
                         utilCompressType2String( clInfo._compressorType ) ) ;
      }
      if ( mask & UTIL_CL_SHDKEY_FIELD )
      {
         builder.append( CAT_SHARDINGKEY_NAME, clInfo._shardingKey ) ;
         builder.appendBool( CAT_ENSURE_SHDINDEX, clInfo._enSureShardIndex ) ;
         builder.append( CAT_SHARDING_TYPE, clInfo._pShardingType ) ;
         if( clInfo._isHash )
         {
            builder.append( CAT_SHARDING_PARTITION, clInfo._shardPartition ) ;

            /// optimize query on hash-sharding only sdb's version >= 1.12
            /// update version since 1.12.4
            builder.append( CAT_INTERNAL_VERSION, CAT_INTERNAL_CURRENT_VERSION ) ;
         }
      }
      /// add catainfo to record even not specified by user.
      if ( clInfo._isMainCL )
      {
         builder.appendBool( CAT_IS_MAINCL, clInfo._isMainCL );
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         sub.done() ;
      }
      else if ( clInfo._autoSplit && !splitLst.empty() )
      {
         // cata info build
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         INT32 itemID = 0 ;
         UINT32 totalBound = (UINT32) clInfo._shardPartition ;
         UINT32 grpSize = splitLst.size() ;
         UINT32 avgBound = totalBound / grpSize ;
         UINT32 modMark = grpSize - ( totalBound % grpSize ) ;
         UINT32 beginBound = CAT_HASH_LOW_BOUND ;
         UINT32 endBound = beginBound + avgBound ;

         PD_CHECK ( clInfo._isSharding && clInfo._isHash,
                    SDB_INVALIDARG, error, PDWARNING,
                    "AutoSplit only on hash sharding" ) ;

         if (totalBound < grpSize )
         {
            rc = SDB_INVALIDARG;
            PD_LOG_MSG(PDERROR, "Partition can not less than group number of domain."
                   "partition = %d, group number = %d", totalBound, grpSize);
            goto error;
         }

         for ( std::map<std::string, UINT32>::const_iterator iterGrp = splitLst.begin();
               iterGrp != splitLst.end();
               ++iterGrp )
         {
            UINT32 grpID = iterGrp->second ;
            const std::string &grpName = iterGrp->first ;
            BSONObj lowBound, upBound ;

            if ( (UINT32)itemID >= modMark )
            {
               ++endBound ;
            }

            BSONObjBuilder cataItemBd ( sub.subobjStart ( sub.numStr(itemID) ) ) ;
            cataItemBd.append ( FIELD_NAME_ID, itemID ) ;
            cataItemBd.append ( CAT_CATALOGGROUPID_NAME, (INT32)grpID ) ;
            cataItemBd.append ( CAT_GROUPNAME_NAME, grpName ) ;

            rc = catBuildHashBound( lowBound, upBound, beginBound, endBound ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to build cata info bound, rc: %d",
                         rc ) ;

            cataItemBd.append ( CAT_LOWBOUND_NAME, lowBound ) ;
            cataItemBd.append ( CAT_UPBOUND_NAME, upBound ) ;

            cataItemBd.done () ;

            beginBound = endBound ;
            endBound = endBound + avgBound ;
            ++ itemID ;
         }
         sub.done () ;
      }
      else
      {
         // cata info build
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         BSONObjBuilder cataItemBd ( sub.subobjStart ( sub.numStr(0) ) ) ;
         pmdKRCB *krcb = pmdGetKRCB() ;
         sdbCatalogueCB *pCatCB = krcb->getCATLOGUECB() ;
         UINT32 grpID = CAT_INVALID_GROUPID ;
         std::string grpName ;

         if ( UTIL_INVALID_DS_UID == clInfo._dsUID )
         {
            PD_CHECK( grpIDSet.size() == 1,
                      SDB_INVALIDARG, error, PDWARNING,
                      "Must has only one group specified" ) ;
            grpID = *( grpIDSet.begin() ) ;
         }
         else
         {
            grpID = SDB_DSID_2_GROUPID( clInfo._dsUID ) ;
         }

         cataItemBd.append ( CAT_CATALOGGROUPID_NAME, (INT32)grpID ) ;

         /// Get SYS groups for SYS collections
         grpName = pCatCB->groupID2Name( grpID ) ;
         cataItemBd.append ( CAT_GROUPNAME_NAME, grpName ) ;

         if ( clInfo._isSharding )
         {
            // add LowBound and UpBound
            BSONObj lowBound, upBound ;

            if ( !clInfo._isHash )
            {
               Ordering order = Ordering::make( clInfo._shardingKey ) ;
               rc = catBuildInitRangeBound ( clInfo._shardingKey, order ,
                                             lowBound, upBound ) ;
            }
            else
            {
               rc = catBuildInitHashBound( lowBound, upBound,
                                           clInfo._shardPartition ) ;
            }
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to build cata info bound, rc: %d",
                         rc ) ;

            cataItemBd.append ( CAT_LOWBOUND_NAME, lowBound ) ;
            cataItemBd.append ( CAT_UPBOUND_NAME, upBound ) ;
         }
         cataItemBd.done () ;
         sub.done () ;
      }

      if ( mask & UTIL_CL_AUTOSPLIT_FIELD )
      {
         builder.appendBool ( CAT_DOMAIN_AUTO_SPLIT, clInfo._autoSplit ) ;
      }

      if ( mask & UTIL_CL_AUTOREBALANCE_FIELD )
      {
         builder.appendBool ( CAT_DOMAIN_AUTO_REBALANCE, clInfo._autoRebalance ) ;
      }

      if ( mask & UTIL_CL_MAXREC_FIELD )
      {
         builder.append( CAT_CL_MAX_RECNUM, (INT64)clInfo._maxRecNum ) ;
      }

      if ( mask & UTIL_CL_MAXSIZE_FIELD )
      {
         builder.append( CAT_CL_MAX_SIZE, (INT64)clInfo._maxSize ) ;
      }

      if ( mask & UTIL_CL_OVERWRITE_FIELD )
      {
         builder.appendBool( CAT_CL_OVERWRITE, clInfo._overwrite ) ;
      }

      if ( mask & UTIL_CL_AUTOINC_FIELD )
      {
         BSONElement ele ;
         utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

         clUniqueID = clInfo._clUniqueID ;
         ele = clInfo._autoIncFields.getField( CAT_AUTOINCREMENT ) ;
         if ( Object == ele.type() )
         {
            rc = catBuildCatalogAutoIncField( cb, clInfo, ele.Obj(),
                                              clUniqueID, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build autoinc field, rc: %d",
                         rc ) ;
         }
         else if( Array == ele.type() )
         {
            BSONObjIterator it( ele.embeddedObject() ) ;
            while ( it.more() )
            {
               rc = catBuildCatalogAutoIncField( cb, clInfo, it.next().Obj(),
                                                 clUniqueID, w ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get global ID for field, "
                            "rc: %d", rc ) ;
            }
         }
         builder.appendElements( clInfo._autoIncSet.toBson() ) ;
      }

      if ( mask & UTIL_CL_LOBKEYFORMAT_FIELD )
      {
         builder.append( CAT_LOBSHARDINGKEYFORMAT_NAME,
                         clInfo._lobShardingKeyFormat ) ;
      }

      if ( UTIL_INVALID_DS_UID != clInfo._dsUID )
      {
         builder.append( FIELD_NAME_DATASOURCE_ID, clInfo._dsUID ) ;
         if ( ossStrlen( clInfo._fullMapping ) > 0 )
         {
            builder.append( FIELD_NAME_MAPPING, clInfo._fullMapping ) ;
         }
      }

      {
         UINT64 currentTime = ossGetCurrentMilliseconds() ;
         CHAR timestamp[ OSS_TIMESTAMP_STRING_LEN + 1 ] = { 0 } ;
         ossMillisecondsToString( currentTime, timestamp ) ;
         builder.append( FIELD_NAME_CREATE_TIME, timestamp ) ;
         builder.append( FIELD_NAME_UPDATE_TIME, timestamp ) ;
      }

      catRecord = builder.obj () ;

   done :
      PD_TRACE_EXITRC ( SDB_CATBUILDCATARECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   string catGetSeqName4AutoIncFld( const utilCLUniqueID id,
                                    const CHAR* fldName )
   {
      stringstream seqNameStream ;
      seqNameStream.str("") ;
      seqNameStream << "SYS_" << id << "_" << fldName << "_SEQ" ;
      return seqNameStream.str() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSETLOCATION, "catSetLocation" )
   INT32 catSetLocation ( UINT32 locID, const ossPoolString &location,
                          UINT16 nodeID, const BSONObj &groupObj,
                          ossPoolString &oldLocation,
                          BSONObjBuilder &updatorBuilder,
                          BSONObjBuilder &matcherBuilder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATSETLOCATION ) ;

      BSONObj nodeListObj ;
      ossPoolString activeLocation ;
      ossPoolSet<ossPoolString> locSet ;

      rc = rtnGetArrayElement( groupObj, CAT_GROUP_NAME, nodeListObj ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      try
      {
         // Parse nodeList array
         BSONObjIterator nodeItr( nodeListObj ) ;
         while ( nodeItr.more() )
         {
            BSONElement nodeEle = nodeItr.next() ;
            BSONObj nodeObj = nodeEle.embeddedObject() ;

            BSONElement nodeIDEle = nodeObj.getField( CAT_NODEID_NAME ) ;
            BSONElement locationEle = nodeObj.getField( CAT_LOCATION_NAME ) ;

            if ( nodeIDEle.eoo() || ! nodeIDEle.isNumber() )
            {
               PD_LOG( PDWARNING, "Failed to get the field(%s)",
                       CAT_NODEID_NAME );
               rc = SDB_INVALIDARG;
               goto error ;
            }

            if ( ! locationEle.eoo() )
            {
               if ( nodeIDEle.numberInt() == nodeID )
               {
                  oldLocation = locationEle.valuestrsafe() ;
               }
               else
               {
                  locSet.insert( locationEle.valuestrsafe() ) ;
               }
            }
         }

         {
            // Get and check active location
            BSONElement ele = groupObj.getField( CAT_ACTIVE_LOCATION_NAME ) ;
            if ( ! ele.eoo() )
            {
               if ( String != ele.type() )
               {
                  rc = SDB_CAT_CORRUPTION ;
                  PD_LOG( PDWARNING, "Failed to get the field[%s]", CAT_ACTIVE_LOCATION_NAME ) ;
                  goto error ;
               }
               else
               {
                  activeLocation = ele.valuestrsafe() ;
               }
            }
         }

         // Build updator and matcher
         // Set location in Group
         BSONObjBuilder setBuilder( updatorBuilder.subobjStart( "$set" ) ) ;
         setBuilder.append( CAT_GROUP_NAME ".$1." CAT_LOCATION_NAME,
                            location.c_str() ) ;
         setBuilder.doneFast() ;

         // Match nodeList update
         matcherBuilder.append( CAT_GROUP_NAME ".$1." CAT_NODEID_NAME, nodeID ) ;

         ossPoolSet<ossPoolString>::const_iterator itr = locSet.find( location ) ;
         BOOLEAN existNewLoc = itr == locSet.end() ? FALSE : TRUE ;

         if ( oldLocation.empty() )
         {
            if ( ! existNewLoc )
            {
               BSONObjBuilder pushBuilder( updatorBuilder.subobjStart( "$push" ) ) ;
               BSONObjBuilder locBuilder(
                              pushBuilder.subobjStart( CAT_LOCATIONS_NAME ) ) ;

               locBuilder.append( CAT_LOCATION_NAME, location.c_str() ) ;
               locBuilder.append( CAT_LOCATIONID_NAME, locID ) ;

               locBuilder.doneFast() ;
               pushBuilder.doneFast() ;
            }
         }
         else
         {
            itr = locSet.find( oldLocation ) ;
            BOOLEAN existOldLoc = itr == locSet.end() ? FALSE : TRUE ;

            if ( ! existNewLoc && existOldLoc )
            {
               BSONObjBuilder pushBuilder( updatorBuilder.subobjStart( "$push" ) ) ;
               BSONObjBuilder locBuilder(
                              pushBuilder.subobjStart( CAT_LOCATIONS_NAME ) ) ;

               locBuilder.append( CAT_LOCATION_NAME, location.c_str() ) ;
               locBuilder.append( CAT_LOCATIONID_NAME, locID ) ;

               locBuilder.doneFast() ;
               pushBuilder.doneFast() ;
            }
            if ( existNewLoc && ! existOldLoc )
            {
               BSONObjBuilder pullBuilder(
                              updatorBuilder.subobjStart( "$pull_by" ) ) ;
               BSONObjBuilder locBuilder(
                              pullBuilder.subobjStart( CAT_LOCATIONS_NAME ) ) ;
               locBuilder.append( CAT_LOCATION_NAME, oldLocation.c_str() ) ;
               locBuilder.doneFast() ;
               pullBuilder.doneFast() ;
            }
            if ( ! existNewLoc && ! existOldLoc )
            {
               BSONObjBuilder setBuilder(
                              updatorBuilder.subobjStart( "$set" ) ) ;
               setBuilder.append( CAT_LOCATIONS_NAME ".$2." CAT_LOCATION_NAME,
                                  location.c_str() ) ;
               setBuilder.append( CAT_LOCATIONS_NAME ".$2." CAT_LOCATIONID_NAME,
                                  locID ) ;
               setBuilder.doneFast() ;

               matcherBuilder.append(
               CAT_LOCATIONS_NAME ".$2." CAT_LOCATION_NAME, oldLocation.c_str() ) ;
            }

            // Remove activeLocation in group if the last location in node was remove
            if ( activeLocation == oldLocation && ! existOldLoc )
            {
               BSONObjBuilder unsetBuilder( updatorBuilder.subobjStart( "$unset" ) ) ;
               unsetBuilder.append( CAT_ACTIVE_LOCATION_NAME, "" ) ;
               unsetBuilder.doneFast() ;
            }
         }
      }
      catch( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATSETLOCATION, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATREMOVELOCATION, "catRemoveLocation" )
   INT32 catRemoveLocation ( UINT16         nodeID,
                             const BSONObj  &groupObj,
                             BSONObjBuilder &updatorBuilder,
                             BSONObjBuilder &matcherBuilder,
                             BOOLEAN        isRemoveNode,
                             ossPoolString  *oldLocation )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATREMOVELOCATION ) ;

      BSONObj nodeListObj ;
      ossPoolString tmpOldLoc ;
      ossPoolString activeLocation ;
      ossPoolSet<ossPoolString> locationSet ;

      rc = rtnGetArrayElement( groupObj, CAT_GROUP_NAME, nodeListObj ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      try
      {
         // Parse nodeList array and prepare locationSet
         BSONObjIterator nodeItr( nodeListObj ) ;
         while ( nodeItr.more() )
         {
            BSONElement nodeEle = nodeItr.next() ;
            BSONObj nodeObj = nodeEle.embeddedObject() ;

            BSONElement nodeIDEle = nodeObj.getField( CAT_NODEID_NAME ) ;
            BSONElement locationEle = nodeObj.getField( CAT_LOCATION_NAME ) ;

            if ( nodeIDEle.eoo() || ! nodeIDEle.isNumber() )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDWARNING, "Failed to get the field(%s)",
                       CAT_NODEID_NAME );
               goto error ;
            }

            if ( ! locationEle.eoo() )
            {
               if ( nodeIDEle.numberInt() == nodeID )
               {
                  tmpOldLoc = locationEle.valuestrsafe() ;
               }
               else
               {
                  locationSet.insert( locationEle.valuestrsafe() ) ;
               }
            }
         }

         {
            // Get and check active location
            BSONElement ele = groupObj.getField( CAT_ACTIVE_LOCATION_NAME ) ;
            if ( ! ele.eoo() )
            {
               if ( String != ele.type() )
               {
                  rc = SDB_CAT_CORRUPTION ;
                  PD_LOG( PDWARNING, "Failed to get the field[%s]", CAT_ACTIVE_LOCATION_NAME ) ;
                  goto error ;
               }
               else
               {
                  activeLocation = ele.valuestrsafe() ;
               }
            }
         }

         // Build updator and matcher
         if ( ! isRemoveNode && ! tmpOldLoc.empty() )
         {
            // Set location in Group
            BSONObjBuilder unsetBuilder( updatorBuilder.subobjStart( "$unset" ) ) ;
            unsetBuilder.append( CAT_GROUP_NAME ".$1." CAT_LOCATION_NAME, "" ) ;
            unsetBuilder.doneFast() ;

            // Match nodeList update
            matcherBuilder.append( CAT_GROUP_NAME ".$1." CAT_NODEID_NAME, nodeID ) ;

         }

         if ( ! tmpOldLoc.empty() )
         {
            // Set location in Locations
            if ( locationSet.end() == locationSet.find( tmpOldLoc ) )
            {
               BSONObjBuilder pullBuilder(
                              updatorBuilder.subobjStart( "$pull_by" ) ) ;
               BSONObjBuilder locBuilder(
                              pullBuilder.subobjStart( CAT_LOCATIONS_NAME ) ) ;
               locBuilder.append( CAT_LOCATION_NAME, tmpOldLoc.c_str() ) ;
               locBuilder.doneFast() ;
               pullBuilder.doneFast() ;

               // Remove activeLocation in group if the last location in node was remove
               if ( activeLocation == tmpOldLoc )
               {
                  BSONObjBuilder unsetBuilder( updatorBuilder.subobjStart( "$unset" ) ) ;
                  unsetBuilder.append( CAT_ACTIVE_LOCATION_NAME, "" ) ;
                  unsetBuilder.doneFast() ;
               }
            }
         }

         if ( ! isRemoveNode )
         {
            *oldLocation = tmpOldLoc ;
         }
      }
      catch( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATREMOVELOCATION, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSETGRPMODE, "catSetGrpMode" )
   INT32 catSetGrpMode ( const clsGroupMode &grpMode,
                         const BOOLEAN incGrpVer,
                         _pmdEDUCB *cb, SDB_DMSCB *pDmsCB,
                         SDB_DPSCB *pDpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_CATSETGRPMODE ) ;

      BSONObj updator, matcher ;
      BSONObjBuilder updatorBuilder, matcherBuilder ;

      // Append groupID to matcher
      matcherBuilder.append( CAT_GROUPID_NAME, grpMode.groupID ) ;

      // Build version increase updator
      if ( incGrpVer )
      {
         BSONObjBuilder incBuilder( updatorBuilder.subobjStart( "$inc" ) ) ;
         incBuilder.append( CAT_VERSION_NAME, 1 ) ;
         incBuilder.doneFast() ;
      }

      // Build set/unset updator
      if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
      {
         BSONObjBuilder setBuilder( updatorBuilder.subobjStart( "$set" ) ) ;
         setBuilder.append( CAT_GROUP_MODE_NAME, CAT_CRITICAL_MODE_NAME ) ;
         setBuilder.doneFast() ;
      }
      else if ( CLS_GROUP_MODE_MAINTENANCE == grpMode.mode )
      {
         BSONObjBuilder setBuilder( updatorBuilder.subobjStart( "$set" ) ) ;
         setBuilder.append( CAT_GROUP_MODE_NAME, CAT_MAINTENANCE_MODE_NAME ) ;
         setBuilder.doneFast() ;
      }
      else
      {
         BSONObjBuilder unsetBuilder( updatorBuilder.subobjStart( "$unset" ) ) ;
         unsetBuilder.append( CAT_GROUP_MODE_NAME, "" ) ;
         unsetBuilder.doneFast() ;
      }

      // Update group
      updator = updatorBuilder.obj() ;
      matcher = matcherBuilder.obj() ;

      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION, matcher, updator, BSONObj(),
                      0, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update group[%u], matcher: %s, "
                   "updator: %s, rc: %d", grpMode.groupID, matcher.toPoolString().c_str(),
                   updator.toPoolString().c_str(), rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATSETGRPMODE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCREATENODESTEP, "catCreateNodeStep" )
   INT32 catCreateNodeStep ( const string &groupName, const string &hostName,
                             const string &dbPath, UINT32 instanceID,
                             const string &localSvc, const string &replSvc,
                             const string &shardSvc, const string &cataSvc,
                             INT32 nodeRole, UINT16 nodeID, INT32 nodeStatus,
                             _pmdEDUCB *cb, SDB_DMSCB *pDmsCB,
                             SDB_DPSCB *pDpsCB, INT16 w  )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCREATENODESTEP ) ;

      BSONObjBuilder newNodeBuilder ;
      BSONObjBuilder updateBuilder ;
      BSONObj updator, matcher ;
      BSONObj dummyObj ;

      catBuildNewNode( hostName, dbPath, instanceID, localSvc, replSvc, shardSvc, cataSvc, nodeRole,
                       nodeID, nodeStatus, newNodeBuilder ) ;
      BSONObj newInfoObj = newNodeBuilder.obj() ;

      // update group info
      updateBuilder.append("$inc", BSON( CAT_VERSION_NAME << 1 ) ) ;
      updateBuilder.append("$push", BSON( CAT_GROUP_NAME << newInfoObj ) ) ;
      updator = updateBuilder.obj() ;

      matcher = BSON( FIELD_NAME_GROUPNAME << groupName ) ;

      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION, matcher, updator, dummyObj,
                      0, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to update node info [%s] to group [%s],"
                   "matcher: %s, updator: %s, rc: %d",
                   newInfoObj.toString().c_str(), groupName.c_str(),
                   matcher.toString().c_str(), updator.toString().c_str(),
                   rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATCREATENODESTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATBUILDNEWNODE "catBuildNewNode" )
   void catBuildNewNode( const string &hostName,
                          const string &dbPath,
                          UINT32 instanceID,
                          const string &localSvc,
                          const string &replSvc,
                          const string &shardSvc,
                          const string &cataSvc,
                          INT32 nodeRole,
                          UINT16 nodeID,
                          INT32 nodeStatus,
                          BSONObjBuilder &builder )
   {
      PD_TRACE_ENTRY(SDB_CATBUILDNEWNODE);
      builder.append( CAT_HOST_FIELD_NAME, hostName ) ;
      builder.append( PMD_OPTION_DBPATH, dbPath ) ;

      if ( utilCheckInstanceID( instanceID, FALSE ) )
      {
         builder.append( PMD_OPTION_INSTANCE_ID, (INT32)instanceID ) ;
      }

      // service
      BSONObjBuilder sub( builder.subarrayStart( CAT_SERVICE_FIELD_NAME ) ) ;
      // local
      BSONObjBuilder sub1( sub.subobjStart("0") ) ;
      sub1.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_LOCAL_SERVICE ) ;
      sub1.append( CAT_SERVICE_NAME_FIELD_NAME, localSvc ) ;
      sub1.done() ;

      // repl
      BSONObjBuilder sub2( sub.subobjStart("1") ) ;
      sub2.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_REPL_SERVICE ) ;
      sub2.append( CAT_SERVICE_NAME_FIELD_NAME, replSvc ) ;
      sub2.done() ;
      // shard
      BSONObjBuilder sub3( sub.subobjStart("2") ) ;
      sub3.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_SHARD_SERVCIE ) ;
      sub3.append( CAT_SERVICE_NAME_FIELD_NAME, shardSvc ) ;
      sub3.done() ;
      // cata
      if ( SDB_ROLE_CATALOG == nodeRole )
      {
         BSONObjBuilder sub4( sub.subobjStart("3") ) ;
         sub4.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_CAT_SERVICE ) ;
         sub4.append( CAT_SERVICE_NAME_FIELD_NAME, cataSvc ) ;
         sub4.done() ;
      }

      sub.done() ;

      builder.append( CAT_NODEID_NAME, nodeID ) ;
      builder.append( CAT_GROUP_STATUS, nodeStatus ) ;
      PD_TRACE_EXIT(SDB_CATBUILDNEWNODE);
   }

   static INT32 _catRemoveNode ( const BSONObj &boGroup,
                                 UINT16 nodeID,
                                 BSONArray &baNewNodeList )
   {
      INT32 rc = SDB_OK ;

      BSONObj boNodeList ;
      BSONArrayBuilder newListBuilder ;

      rc = rtnGetArrayElement( boGroup, CAT_GROUP_NAME, boNodeList ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get field [%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      {
         BSONObjIterator i( boNodeList ) ;
         while ( i.more() )
         {
            BSONElement beNode = i.next() ;
            BSONObj boNode = beNode.embeddedObject() ;

            UINT16 tmpNodeID = INVALID_NODEID ;
            rc = rtnGetIntElement( boNode, FIELD_NAME_NODEID,
                                   (INT32 &)tmpNodeID ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to get field [%s], rc: %d",
                         FIELD_NAME_NODEID, rc ) ;
            if ( tmpNodeID != nodeID )
            {
               newListBuilder.append( boNode ) ;
            }
         }
      }

      baNewNodeList = newListBuilder.arr() ;

   done :
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATREMOVENODESTEP, "catRemoveNodeStep" )
   INT32 catRemoveNodeStep ( const string &groupName, UINT16 nodeID,
                             _pmdEDUCB *cb,
                             SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                             INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATREMOVENODESTEP ) ;

      BSONObjBuilder updatorBuilder, matcherBuilder ;
      BSONObj updator, matcher, dummyObj ;
      BSONArray baNewNodeList ;
      BSONObj boGroup ;

      // refresh group
      rc = catGetGroupObj( groupName.c_str(), FALSE, boGroup, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get group [%s], rc: %d",
                   groupName.c_str(), rc ) ;

      rc = _catRemoveNode( boGroup, nodeID, baNewNodeList ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get new node list" ) ;

      rc = catRemoveLocation( nodeID, boGroup, updatorBuilder,
                              matcherBuilder, TRUE, NULL ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to build removeLocation builder" ) ;

      if ( 0 == baNewNodeList.nFields() )
      {
         UINT32 groupID = CAT_INVALID_GROUPID ;
         rc = catGroupName2ID( groupName.c_str(), groupID, FALSE, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get groupID, groupName[%s], rc: %d",
                      groupName.c_str(), rc ) ;

         rc = catDelGroupFromGrpMode( groupID, w, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove group [%s] from group mode, rc: %d",
                      groupName.c_str(), rc ) ;
      }

      try
      {
         // update group info
         updatorBuilder.append( "$inc", BSON( CAT_VERSION_NAME << 1 ) ) ;
         updatorBuilder.append( "$set", BSON( FIELD_NAME_GROUP << baNewNodeList ) ) ;

         updator = updatorBuilder.obj() ;

         matcherBuilder.append( FIELD_NAME_GROUPNAME, groupName ) ;
         matcher = matcherBuilder.obj() ;
      }
      catch( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d",
               e.what(), rc ) ;
         goto error ;
      }

      rc = rtnUpdate( CAT_NODE_INFO_COLLECTION, matcher, updator, dummyObj,
                      0, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to rollback node [%d] from group [%s],"
                   "matcher: %s, updator: %s, rc: %d",
                   nodeID, groupName.c_str(),
                   matcher.toString().c_str(), updator.toString().c_str(),
                   rc ) ;

   done :
      PD_TRACE_EXITRC ( SDB_CATREMOVENODESTEP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATTRANSBEGIN, "catTransBegin" )
   INT32 catTransBegin ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATTRANSBEGIN ) ;

      pmdKRCB *pKrcb = pmdGetKRCB() ;
      dpsTransCB *pTransCB = pKrcb->getTransCB() ;

      // Ignored if transaction is not enabled
      if ( pTransCB && pTransCB->isTransOn() )
      {
         rc = rtnTransBegin( cb ) ;
         PD_RC_CHECK( rc, PDERROR,
                      "Failed to begin transaction, rc: %d", rc ) ;
         PD_LOG( PDDEBUG, "Begin transaction" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB_CATTRANSBEGIN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATTRANSEND, "catTransEnd" )
   INT32 catTransEnd ( INT32 result, _pmdEDUCB *cb, SDB_DPSCB *pDpsCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATTRANSEND ) ;

      if ( DPS_INVALID_TRANS_ID != cb->getTransID() )
      {
         if ( SDB_OK == result ||
              SDB_DMS_EOC == result )
         {
            rc = rtnTransCommit( cb, pDpsCB ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to commit transaction, rc: %d", rc ) ;
            PD_LOG( PDDEBUG, "Commit transaction" ) ;
         }
         else
         {
            rc = rtnTransRollback( cb, pDpsCB ) ;
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to rollback transaction, rc: %d", rc ) ;
            PD_LOG( PDDEBUG, "Rollback transaction" ) ;
         }
      }

   done :
      PD_TRACE_EXITRC( SDB_CATTRANSEND, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   static OSS_THREAD_LOCAL INT16 _catSyncW ;

   void catSetSyncW ( INT16 w )
   {
      _catSyncW = w ;
   }

   INT16 catGetSyncW ()
   {
      return _catSyncW ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATVALIDSEQOPTION, "catValidSequenceOption" )
   INT32 catValidSequenceOption( const BSONObj &option )
   {
      PD_TRACE_ENTRY( SDB_CATVALIDSEQOPTION ) ;
      INT32 rc = SDB_OK ;
      BSONObj       seqOpt ;
      catSequence sequence( "SYS_UNKOWN_SEQ" ) ;

      try
      {
         seqOpt = catBuildSequenceOptions( option ) ;
         rc = catSequence::validateFieldNames( seqOpt ) ;
         PD_RC_CHECK( rc, PDERROR, "AutoIncrement sequence options is invalid" ) ;
         rc = sequence.setOptions( seqOpt, TRUE, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "AutoIncrement sequence options is invalid" ) ;
         rc = sequence.validate() ;
         PD_RC_CHECK( rc, PDERROR, "AutoIncrement sequence options is invalid" ) ;
      }
      catch( std::exception & )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to check autoincrement option[%s]",
                 option.toString( false, false ).c_str()) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATVALIDSEQOPTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCREATEAUTOINCSEQUENCE, "catCreateAutoIncSequence" )
   INT32 catCreateAutoIncSequence( const BSONObj &option,
                                   const clsAutoIncSet &autoIncSet,
                                   utilCLUniqueID clUniqueID,
                                   _pmdEDUCB *cb,
                                   INT16 w )
   {
      PD_TRACE_ENTRY( SDB_CATCREATEAUTOINCSEQUENCE ) ;
      INT32 rc = SDB_OK ;
      BSONObj seqOpt ;
      const CHAR *fieldName = NULL ;
      const CHAR *seqName = NULL ;
      const clsAutoIncItem *field = NULL ;
      utilSequenceID seqID = UTIL_SEQUENCEID_NULL ;
      catSequenceManager   *pSeqMgr = NULL ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;

      fieldName = option.getField( CAT_AUTOINC_FIELD ).valuestrsafe() ;
      field = autoIncSet.find( fieldName ) ;
      PD_CHECK( NULL != field, SDB_SYS,
               error, PDERROR, "Field[%s] does not exist on "
                "collection", fieldName ) ;
      seqName = field->sequenceName() ;
      seqID = field->sequenceID() ;
      seqOpt = catBuildSequenceOptions( option, seqID ) ;
      rc = pSeqMgr->createSequence( seqName, clUniqueID, seqOpt, cb, w ) ;
      if ( SDB_SEQUENCE_EXIST == rc )
      {
         // Got sequence with the same name, remove previous one
         catRemoveSequenceTasks( seqName, cb, w ) ;
         pSeqMgr->dropSequence( seqName, cb, w ) ;
         rc = pSeqMgr->createSequence( seqName, clUniqueID, seqOpt, cb, w ) ;
      }
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to create sequence on field [%s] with ""options "
                    "[%s], rc: %d", fieldName,
                    option.toString().c_str(), rc ) ;
   done:
      PD_TRACE_EXITRC( SDB_CATCREATEAUTOINCSEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCREATEAUTOINCSEQUENCES, "catCreateAutoIncSequences" )
   INT32 catCreateAutoIncSequences( const catCollectionInfo &clInfo,
                                    _pmdEDUCB *cb, INT16 w )
   {
      PD_TRACE_ENTRY( SDB_CATCREATEAUTOINCSEQUENCES ) ;
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj autoIncObj = clInfo._autoIncFields ;
      const clsAutoIncSet &autoIncSet = clInfo._autoIncSet ;
      utilCLUniqueID clUniqueID = clInfo._clUniqueID ;

      ele = autoIncObj.getField( CAT_AUTOINCREMENT ) ;
      PD_CHECK( Object == ele.type() || Array == ele.type(),
                SDB_INVALIDARG, error, PDWARNING,
                "Field [%s] type [%d] error",
                CAT_AUTOINCREMENT, ele.type() ) ;
      if ( Object == ele.type() )
      {
         rc = catCreateAutoIncSequence( ele.Obj(), autoIncSet, clUniqueID, cb,
                                        w ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else if( Array == ele.type() )
      {
         BSONObjIterator it( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            rc = catCreateAutoIncSequence( it.next().Obj(), autoIncSet,
                                           clUniqueID, cb, w ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCREATEAUTOINCSEQUENCES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPAUTOINCSEQUENCE, "catDropAutoIncSequence" )
   INT32 catDropAutoIncSequence( const BSONObj &boAutoInc, _pmdEDUCB *cb, INT16 w )
   {
      PD_TRACE_ENTRY( SDB_CATDROPAUTOINCSEQUENCE ) ;
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      const CHAR *seqName = NULL ;
      catSequenceManager   *pSeqMgr = NULL ;

      pSeqMgr = sdbGetCatalogueCB()->getCatGTSMgr()->getSequenceMgr() ;
      seqName = boAutoInc.getField( FIELD_NAME_AUTOINC_SEQ ).valuestr() ;
      rc = pSeqMgr->dropSequence( seqName, cb, w ) ;
      if ( SDB_SEQUENCE_NOT_EXIST == rc )
      {
         PD_LOG( PDWARNING, "Sequence[%s] not exist, no need to drop", seqName ) ;
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING, "Failed to remove sequence [%s], rc: %d",
                   seqName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATDROPAUTOINCSEQUENCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPAUTOINCSEQUENCES, "catDropAutoIncSequences" )
   INT32 catDropAutoIncSequences( const BSONObj &boCollection, _pmdEDUCB *cb, INT16 w )
   {
      PD_TRACE_ENTRY( SDB_CATDROPAUTOINCSEQUENCES ) ;

      INT32                rc = SDB_OK ;
      BSONElement          ele ;

      if ( boCollection.hasField( CAT_AUTOINCREMENT ) )
      {
         ele = boCollection.getField( CAT_AUTOINCREMENT ) ;
         if ( Object == ele.type() )
         {
            rc = catDropAutoIncSequence(ele.Obj(), cb, w ) ;
            if( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else if( Array == ele.type() )
         {
            BSONObjIterator it( ele.embeddedObject() ) ;
            while ( it.more() )
            {
               rc = catDropAutoIncSequence( it.next().Obj(), cb, w ) ;
               if( SDB_OK != rc )
               {
                  goto error ;
               }
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATDROPAUTOINCSEQUENCES, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj catBuildSequenceOptions( const BSONObj &autoIncOpt,
                                    utilSequenceID ID,
                                    UINT32 fieldMask )
   {
      BSONObjBuilder seqOptBuilder ;
      BSONObjIterator iter( autoIncOpt ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         const CHAR *fieldName = ele.fieldName() ;
         if ( ossStrcmp( fieldName, CAT_AUTOINC_FIELD ) == 0 ||
              ossStrcmp( fieldName, CAT_AUTOINC_GENERATED ) == 0 ||
              ossStrcmp( fieldName, CAT_AUTOINC_SEQ ) == 0 )
         {
            // ignored system fields
            continue ;
         }
         else if ( UTIL_ARG_FIELD_ALL == fieldMask )
         {
            // all fields are needed
            seqOptBuilder.append( ele ) ;
         }
         else if ( ( ossStrcmp( fieldName, CAT_SEQUENCE_INCREMENT ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_INCREMENT_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_CURRENT_VALUE ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_CURVALUE_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_START_VALUE ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_STARTVALUE_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_MIN_VALUE ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_MINVALUE_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_MAX_VALUE ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_MAXVALUE_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_CACHE_SIZE ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_CACHESIZE_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_ACQUIRE_SIZE ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_ACQUIRESIZE_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_CYCLED ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_CYCLED_FIELD ) ) ||
                   ( ossStrcmp( fieldName, CAT_SEQUENCE_INITIAL ) == 0 &&
                     OSS_BIT_TEST( fieldMask, UTIL_CL_AUTOINC_INITIAL_FIELD ) ) )
         {
            // fields are specified
            seqOptBuilder.append( ele ) ;
         }
      }

      seqOptBuilder.append( CAT_SEQUENCE_INTERNAL, true ) ;

      if( ID != UTIL_SEQUENCEID_NULL )
      {
         seqOptBuilder.append( CAT_SEQUENCE_ID, (INT64)ID ) ;
      }

      return seqOptBuilder.obj() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKDATASRUOCEEXIST, "catCheckDataSourceExist" )
   INT32 catCheckDataSourceExist( const CHAR *dsName, BOOLEAN &exist,
                                  BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCHECKDATASRUOCEEXIST ) ;
      SDB_ASSERT( dsName, "Data source name is NULL" ) ;

      exist = FALSE ;

      try
      {
         BSONObj matcher ;
         BSONObj dummyObj ;
         matcher = BSON( FIELD_NAME_NAME << dsName ) ;
         rc = catGetOneObj( CAT_DATASOURCE_COLLECTION, dummyObj, matcher,
                            dummyObj, cb, obj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            exist = FALSE ;
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            exist = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Get metadata of data source[%s] failed[%d]",
                    dsName, rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCHECKDATASRUOCEEXIST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKDATASRUOCEEXIST_UID, "catCheckDataSourceExist" )
   INT32 catCheckDataSourceExist( UTIL_DS_UID dsUID,
                                  BOOLEAN &exist,
                                  BSONObj &obj,
                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCHECKDATASRUOCEEXIST_UID ) ;

      exist = FALSE ;

      try
      {
         BSONObj matcher, dummy ;
         matcher = BSON( FIELD_NAME_ID << (INT32)dsUID ) ;
         rc = catGetOneObj( CAT_DATASOURCE_COLLECTION, dummy, matcher,
                            dummy, cb, obj ) ;
         if ( SDB_DMS_EOC == rc )
         {
            exist = FALSE ;
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            exist = TRUE ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to get metadata of data source[%llu], "
                    "rc: %d", dsUID, rc ) ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get metadata of data source, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCHECKDATASRUOCEEXIST_UID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKPUREMAPPINGCS, "catCheckPureMappingCS" )
   INT32 catCheckPureMappingCS( const CHAR *csName,
                                pmdEDUCB *cb,
                                BOOLEAN &isMappingCS,
                                BSONObj *csMeta )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCHECKPUREMAPPINGCS ) ;

      try
      {
         BSONObj dummyObj ;
         BSONObj csMetaRecord ;
         BSONObj cataInfo ;
         BSONObj matcher = BSON( FIELD_NAME_NAME << csName ) ;

         rc = catGetOneObj( CAT_COLLECTION_SPACE_COLLECTION, dummyObj, matcher,
                            dummyObj, cb, csMetaRecord ) ;
         if ( SDB_DMS_EOC == rc )
         {
            isMappingCS = FALSE ;
            rc = SDB_OK ;
            goto done ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get collection space[%s] metadata from "
                             "SYSCOLLECTIONSPACES failed[%d]", csName, rc ) ;
            goto error ;
         }

         isMappingCS = csMetaRecord.hasField( FIELD_NAME_DATASOURCE_ID ) ;
         if ( csMeta )
         {
            *csMeta = csMetaRecord.getOwned() ;
         }
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to check mapping collection space, "
                 "occur exception %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCHECKPUREMAPPINGCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKCLINPUREMAPPINGCS, "catCheckCLInPureMappingCS" )
   INT32 catCheckCLInPureMappingCS( const CHAR *clFullName,
                                    pmdEDUCB *cb, BOOLEAN &inMappingCS,
                                    BSONObj *csMeta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATCHECKCLINPUREMAPPINGCS ) ;

      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;

      rc = rtnResolveCollectionSpaceName( clFullName, ossStrlen( clFullName ),
                                          csName,
                                          DMS_COLLECTION_SPACE_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection space name "
                   "from collection name [%s], rc: %d", clFullName, rc ) ;

      rc = catCheckPureMappingCS( csName, cb, inMappingCS, csMeta ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check pure mapping collection "
                   "space [%s], rc: %d", csName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCHECKCLINPUREMAPPINGCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATBUILDCATALOGBYPUREMAPPINGCS, "catBuildCatalogByPureMappingCS" )
   INT32 catBuildCatalogByPureMappingCS( const CHAR *clFullName,
                                         const BSONObj &csMetaData,
                                         BSONObj &catalog, pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_CATBUILDCATALOGBYPUREMAPPINGCS ) ;
      const CHAR *clShortName = ossStrchr( clFullName, '.' ) + 1 ;

      try
      {
         UTIL_DS_UID dsID = UTIL_INVALID_DS_UID ;
         BSONElement ele = csMetaData.getField( FIELD_NAME_DATASOURCE_ID ) ;

         dsID = ele.numberInt() ;
         SDB_ASSERT( UTIL_INVALID_DS_UID != dsID,
                     "Data source id is invalid" ) ;
         if ( UTIL_INVALID_DS_UID != dsID )
         {
            UINT32 mask = UTIL_CL_NAME_FIELD ;
            const CHAR *csMapping =
                  csMetaData.getStringField( FIELD_NAME_MAPPING ) ;
            CAT_GROUP_SET grpIDset ;
            map<string, UINT32> splitList ;

            SDB_ASSERT( csMapping, "cs mapping is NULL" ) ;
            catCollectionInfo clInfo ;
            CHAR mappingName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] = { 0 } ;
            ossSnprintf( mappingName, DMS_COLLECTION_FULL_NAME_SZ,
                         "%s.%s", csMapping, clShortName ) ;
            clInfo._pCLName = clFullName ;
            ossStrncpy( clInfo._fullMapping, mappingName,
                        DMS_COLLECTION_FULL_NAME_SZ ) ;
            grpIDset.insert( SDB_DSID_2_GROUPID( dsID ) ) ;
            clInfo._dsUID = dsID ;
            clInfo._version = CATALOG_INVALID_VERSION ;

            rc = catBuildCatalogRecord( cb, clInfo, mask, 0, grpIDset,
                                        splitList, catalog, 1 ) ;
            PD_RC_CHECK( rc, PDERROR, "Build catalogue record for collection"
                                      "[%s] failed[%d]", clFullName, rc ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATBUILDCATALOGBYPUREMAPPINGCS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHKDSUID, "catCheckDataSourceID" )
   INT32 catCheckDataSourceID( const BSONObj &boObject,
                               UTIL_DS_UID &dsUID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCHKDSUID ) ;

      dsUID = UTIL_INVALID_DS_UID ;

      try
      {
         BSONElement ele = boObject.getField( FIELD_NAME_DATASOURCE_ID ) ;
         if ( EOO == ele.type() )
         {
            // not from data source
            goto done ;
         }
         PD_CHECK( NumberInt == ele.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not integer",
                   FIELD_NAME_DATASOURCE_ID ) ;
         dsUID = (UTIL_DS_UID)( ele.numberInt() ) ;
         SDB_ASSERT( UTIL_INVALID_DS_UID != dsUID,
                     "Data source ID is invalid" ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get data source ID, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCHKDSUID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATERECYCLEBINCONF, "catUpdateRecycleBinConf" )
   INT32 catUpdateRecycleBinConf( const utilRecycleBinConf &newConf,
                                  pmdEDUCB *cb,
                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATUPDATERECYCLEBINCONF ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      try
      {
         BOOLEAN rewriteAll = TRUE ;

         utilRecycleBinConf oldConf ;

         BSONElement ele ;
         BSONObj dummy, result ;
         BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
         BSONObjBuilder builder ;
         BSONObjBuilder subBuilder( builder.subobjStart( "$set" ) ) ;
         BSONObj updator ;

         rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummy, matcher,
                            dummy, cb, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get DC info from "
                      "collection [%s], rc: %d",
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

         ele = result.getField( FIELD_NAME_RECYCLEBIN ) ;
         if ( EOO == ele.type() )
         {
            PD_LOG( PDINFO, "Field [%s] is empty, need initialize",
                    FIELD_NAME_RECYCLEBIN ) ;
            rewriteAll = TRUE ;
         }
         else if ( Object != ele.type() )
         {
            PD_LOG( PDWARNING, "Failed to get field [%s], "
                    "it is not an object, ele [%s]",
                    FIELD_NAME_RECYCLEBIN, ele.toPoolString().c_str() ) ;
            rewriteAll = TRUE ;
         }
         else
         {
            rc = oldConf.fromBSON( result ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get recycle bin info "
                         "from BSON, rc: %d", rc ) ;
         }
         if ( rewriteAll ||
              oldConf.getEnable() != newConf.getEnable() )
         {
            subBuilder.appendBool(
                  FIELD_NAME_RECYCLEBIN "." FIELD_NAME_ENABLE,
                  newConf.getEnable() ) ;
         }
         if ( rewriteAll ||
              oldConf.getExpireTime() != newConf.getExpireTime() )
         {
            subBuilder.append(
                  FIELD_NAME_RECYCLEBIN "." FIELD_NAME_EXPIRETIME,
                  newConf.getExpireTime() ) ;
         }
         if ( rewriteAll ||
              oldConf.getMaxItemNum() != newConf.getMaxItemNum() )
         {
            subBuilder.append(
                  FIELD_NAME_RECYCLEBIN "." FIELD_NAME_MAXITEMNUM,
                  newConf.getMaxItemNum() ) ;
         }
         if ( rewriteAll ||
              oldConf.getMaxVersionNum() != newConf.getMaxVersionNum() )
         {
            subBuilder.append(
                  FIELD_NAME_RECYCLEBIN "." FIELD_NAME_MAXVERNUM,
                  newConf.getMaxVersionNum() ) ;
         }
         if ( rewriteAll ||
              oldConf.getAutoDrop() != newConf.getAutoDrop() )
         {
            subBuilder.appendBool(
                  FIELD_NAME_RECYCLEBIN "." FIELD_NAME_AUTODROP,
                  newConf.getAutoDrop() ) ;
         }
         subBuilder.doneFast() ;
         updator = builder.done() ;

         rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                         dummy, 0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Fail to update obj [%s] to "
                      "collection [%s], rc: %d", updator.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to update recycle bin, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATUPDATERECYCLEBINCONF, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // collections of catalog meta data to recycle from
   // e.g. collections in SYSCAT collection space
   const CHAR *catGetRecycleBinMetaCL( UTIL_RECYCLE_TYPE type )
   {
      switch ( type )
      {
         case UTIL_RECYCLE_CS :
         {
            return CAT_COLLECTION_SPACE_COLLECTION ;
         }
         case UTIL_RECYCLE_CL :
         {
            return CAT_COLLECTION_INFO_COLLECTION ;
         }
         case UTIL_RECYCLE_SEQ :
         {
            return GTS_SEQUENCE_COLLECTION_NAME ;
         }
         case UTIL_RECYCLE_IDX :
         {
            return CAT_INDEX_INFO_COLLECTION ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "invalid recycle object type" ) ;
            break ;
         }
      }
      return NULL ;
   }

   // collections of catalog meta data to recycle to
   // e.g. collections in SYSRECYCLEBIN collection space
   const CHAR *catGetRecycleBinRecyCL( UTIL_RECYCLE_TYPE type )
   {
      switch ( type )
      {
         case UTIL_RECYCLE_CS :
         {
            return CAT_SYSRECYCLEBIN_CS_COLLECTION ;
         }
         case UTIL_RECYCLE_CL :
         {
            return CAT_SYSRECYCLEBIN_CL_COLLECTION ;
         }
         case UTIL_RECYCLE_SEQ :
         {
            return CAT_SYSRECYCLEBIN_SEQ_COLLECTION ;
         }
         case UTIL_RECYCLE_IDX :
         {
            return CAT_SYSRECYCLEBIN_IDX_COLLECTION ;
         }
         default :
         {
            SDB_ASSERT( FALSE, "invalid recycle object type" ) ;
            break ;
         }
      }
      return NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATGETGRPSFORRECYCLE, "_catGetGroupsForRecycle" )
   static INT32 _catGetGroupsForRecycle( const BSONObj &matcher,
                                         pmdEDUCB *cb,
                                         CAT_GROUP_SET &groupIDSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATGETGRPSFORRECYCLE ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      INT64 contextID = -1 ;

      rtnQueryOptions options ;
      options.setCLFullName( CAT_SYSRECYCLEBIN_CL_COLLECTION ) ;
      options.setQuery( matcher ) ;

      rc = rtnQuery( options, cb, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to execute query on collection [%s], "
                    "rc: %d", CAT_SYSRECYCLEBIN_CL_COLLECTION, rc ) ;

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         BSONObj boCollection ;

         rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               contextID = -1 ;
               rc = SDB_OK ;
               break ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d",
                         rc ) ;
         }

         try
         {
            BSONElement element ;

            boCollection = BSONObj( buffObj.data() ) ;

            element = boCollection.getField( CAT_IS_MAINCL ) ;
            if ( EOO != element.type() && element.booleanSafe() )
            {
               // skip main-collection
               continue ;
            }

            element = boCollection.getField( FIELD_NAME_DATASOURCE_ID ) ;
            if ( EOO != element.type() )
            {
               // skip data source
               continue ;
            }
         }
         catch ( exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get result for collection, "
                    "occur exception: %s", e.what() ) ;
            goto error ;
         }

         rc = catGetCollectionGroupSet( boCollection, groupIDSet ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get group ID set from "
                      "collection object, rc: %d", rc ) ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      PD_TRACE_EXITRC( SDB__CATGETGRPSFORRECYCLE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETGRPSFORRECYCLECS, "catGetGroupsForRecycleCS" )
   INT32 catGetGroupsForRecycleCS( utilCSUniqueID csUniqueID,
                                   pmdEDUCB *cb,
                                   CAT_GROUP_SET &groupIDSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETGRPSFORRECYCLECS ) ;

      BSONObj matcher ;

      rc = utilGetCSBounds( FIELD_NAME_UNIQUEID, csUniqueID, matcher ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build collection space matcher by "
                   "unique ID [%u], rc: %d", csUniqueID, rc ) ;

      rc = _catGetGroupsForRecycle( matcher, cb, groupIDSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group list for "
                   "collection space[%u] from recycle collection, rc: %d",
                   csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETGRPSFORRECYCLECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETGRPSFORRECYITEM, "catGetGroupsForRecycleItem" )
   INT32 catGetGroupsForRecycleItem( utilRecycleID recycleID,
                                     pmdEDUCB *cb,
                                     CAT_GROUP_SET &groupIDSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETGRPSFORRECYITEM ) ;

      BSONObj matcher ;

      try
      {
         matcher = BSON( FIELD_NAME_RECYCLE_ID << (INT64)recycleID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build matcher, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      rc = _catGetGroupsForRecycle( matcher, cb, groupIDSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group list for "
                   "recycle item [%llu], rc: %d", recycleID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETGRPSFORRECYITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATSAVETOGROUPIDLIST, "catSaveToGroupIDList" )
   INT32 catSaveToGroupIDList( SET_UINT32 &groupIDSet,
                               vector<UINT32> &groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATSAVETOGROUPIDLIST ) ;

      try
      {
         /// Remove duplicate groups
         for ( UINT32 i = 0 ; i < groupIDList.size() ; ++i )
         {
            groupIDSet.insert( groupIDList[ i ] ) ;
         }
         groupIDList.clear() ;
         SET_UINT32::iterator iterGroupID = groupIDSet.begin() ;
         while ( iterGroupID != groupIDSet.end() )
         {
            groupIDList.push_back( *iterGroupID ) ;
            ++ iterGroupID ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save group ID, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATSAVETOGROUPIDLIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATPARSECLUNIQUEID, "catParseCLUniqueID" )
   INT32 catParseCLUniqueID( const bson::BSONObj &object,
                             utilCLUniqueID &uniqueID,
                             const CHAR *fieldName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATPARSECLUNIQUEID ) ;

      try
      {
         BSONElement element = object.getFieldDotted( fieldName ) ;
         PD_CHECK( element.isNumber(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s] from object [%s], "
                   "it is not a number", fieldName,
                   object.toPoolString().c_str() ) ;
         uniqueID = (utilCLUniqueID)( element.numberLong() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get unique ID from object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATPARSECLUNIQUEID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATPARSECSUNIQUEID, "catParseCSUniqueID" )
   INT32 catParseCSUniqueID( const bson::BSONObj &object,
                             utilCSUniqueID &uniqueID,
                             const CHAR *fieldName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATPARSECSUNIQUEID ) ;

      try
      {
         BSONElement element = object.getFieldDotted( fieldName ) ;
         PD_CHECK( element.isNumber(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s] from object [%s], "
                   "it is not a number", fieldName,
                   object.toPoolString().c_str() ) ;
         uniqueID = (utilCSUniqueID)( element.numberInt() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get unique ID from object, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATPARSECSUNIQUEID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATINCANDFETCHRECYID, "catIncAndFetchRecycleID" )
   INT32 catIncAndFetchRecycleID( utilRecycleID &recycleID,
                                  pmdEDUCB *cb,
                                  INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATINCANDFETCHRECYID ) ;

      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      try
      {
         BSONElement ele ;
         BSONObj dummy, result ;
         BSONObj matcher = BSON( FIELD_NAME_TYPE << CAT_BASE_TYPE_GLOBAL_STR ) ;
         BSONObj updator ;

         rc = catGetOneObj( CAT_SYSDCBASE_COLLECTION_NAME, dummy, matcher,
                            dummy, cb, result ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get DC info from "
                      "collection [%s], rc: %d",
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;

         ele = result.getField( FIELD_NAME_RECYCLEBIN ) ;
         PD_CHECK( Object == ele.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_RECYCLEBIN ) ;
         ele = ele.embeddedObject().getField( FIELD_NAME_RECYCLEIDHWM ) ;
         PD_CHECK( NumberLong == ele.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s]", FIELD_NAME_RECYCLEIDHWM ) ;
         recycleID = ( utilRecycleID )ele.numberLong() ;
         PD_CHECK( recycleID < UTIL_GLOGALID_MAX, SDB_CAT_GLOBALID_EXCEEDED,
                   error, PDERROR, "Failed to get recycle ID, it can't "
                   "exceed %llu" ) ;
         recycleID += 1 ;

         updator = BSON( "$set" <<
                         BSON( FIELD_NAME_RECYCLEBIN "." FIELD_NAME_RECYCLEIDHWM <<
                               (INT64)recycleID ) ) ;
         rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                         dummy, 0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Fail to update obj [%s] to "
                      "collection [%s], rc: %d", updator.toString().c_str(),
                      CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATINCANDFETCHRECYID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETDOMAINRECYCS, "catGetDomainRecycleCSs" )
   INT32 catGetDomainRecycleCSs( const CHAR *domain,
                                 pmdEDUCB *cb,
                                 ossPoolList< utilCSUniqueID > &collectionSpaces )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETDOMAINRECYCS ) ;

      rc = _catGetDomainCSs( CAT_SYSRECYCLEBIN_CS_COLLECTION, domain, cb,
                             collectionSpaces ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get collection space for "
                   "domain [%s] in recycle bin, rc: %d", domain, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETDOMAINRECYCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETRECYCSGRPS, "catGetRecyCSGroups" )
   INT32 catGetRecyCSGroups( utilCSUniqueID csUniqueID,
                             pmdEDUCB *cb,
                             SET_UINT32 &groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETRECYCSGRPS ) ;

      rc = _catGetCSGroups( CAT_SYSRECYCLEBIN_CL_COLLECTION, csUniqueID,
                            cb, groups ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get groups for collection "
                   "space [%u], rc: %d", csUniqueID, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETRECYCSGRPS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATACQCLUID, "_catAcquireCLUniqueID" )
   INT32 _catAcquireCLUniqueID( utilCSUniqueID csUniqueID,
                                utilCLUniqueID &clUniqueID,
                                pmdEDUCB *cb,
                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__CATACQCLUID ) ;

      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_DPSCB *dpsCB = pmdGetKRCB()->getDPSCB() ;

      BOOLEAN isExist = FALSE ;
      BSONObj boSpace ;

      rc = catCheckSpaceExist( NULL, csUniqueID, isExist, boSpace, cb ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get info of collection "
                   "space [%u], rc: %d", csUniqueID, rc ) ;
      PD_CHECK( isExist, SDB_DMS_CS_NOTEXIST, error, PDWARNING,
                "Collection space [%u] does not exist", csUniqueID ) ;

      try
      {
         BSONObj matcher, updator, dummy ;

         BSONElement ele = boSpace.getField( CAT_CS_CLUNIQUEHWM ) ;
         PD_CHECK( ele.isNumber(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], type: %d",
                   CAT_CS_CLUNIQUEHWM, ele.type() );

         clUniqueID = (UINT64)( ele.numberLong() ) + 1 ;

         PD_CHECK( utilGetCLInnerID( clUniqueID ) <=
                                     (utilCLInnerID)UTIL_CLINNERID_MAX,
                   SDB_CAT_CL_UNIQUEID_EXCEEDED, error, PDERROR,
                   "Failed to get collection unique ID, inner ID "
                    "can not exceed %u, current collection unique id: %llu",
                    UTIL_CLINNERID_MAX, utilGetCLInnerID( clUniqueID ) ) ;

         matcher = BSON( CAT_CS_UNIQUEID << (INT32)csUniqueID ) ;
         updator = BSON( "$set" <<
                         BSON( CAT_CS_CLUNIQUEHWM << (INT64)clUniqueID ) ) ;

         rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, matcher, updator,
                         dummy, 0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, "
                      "match: %s, updator: %s, rc: %d",
                      CAT_COLLECTION_SPACE_COLLECTION,
                      matcher.toPoolString().c_str(),
                      updator.toPoolString().c_str(), rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to acquire collection unique ID, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATACQCLUID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETRTRNCLUID, "catGetReturnCLUID" )
   INT32 catGetReturnCLUID( utilCLUniqueID clUniqueID,
                            utilCLUniqueID &returnCLUniqueID,
                            pmdEDUCB *cb,
                            INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATGETRTRNCLUID ) ;

      utilCSUniqueID csUniqueID = utilGetCSUniqueID( clUniqueID ) ;

      rc = _catAcquireCLUniqueID( csUniqueID, returnCLUniqueID, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to acquire collection unique ID, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATGETRTRNCLUID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
