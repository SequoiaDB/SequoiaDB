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

#include "../bson/lib/md5.hpp"

using namespace bson ;

#define MAX_CL_SIZE_ALIGN_SIZE      (32 * 1024 * 1024)

namespace engine
{

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
      if ( ossStrlen ( pName ) > OSS_MAX_GROUPNAME_SIZE )
      {
         PD_LOG ( PDWARNING, "group name %s is too long",
                  pName ) ;
         goto error ;
      }
      if ( !isSys &&
           ( ossStrncmp ( pName, "SYS", ossStrlen ( "SYS" ) ) == 0 ||
             ossStrncmp ( pName, "$", ossStrlen ( "$" ) ) == 0 ) )
      {
         PD_LOG ( PDWARNING, "group name should not start with SYS nor $: %s",
                  pName ) ;
         goto error ;
      }
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

      std::set <std::string> groupNameList ;
      INT32 expectedOptSize = 0 ;
      BSONElement beGroupList = options.getField ( CAT_GROUPS_NAME ) ;
      if ( !beGroupList.eoo() && beGroupList.type() != Array )
      {
         PD_LOG ( PDERROR, "group list must be array" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( beGroupList.type() == Array )
      {
         BSONArrayBuilder gpInfoBuilder ;
         BSONObjIterator it ( beGroupList.embeddedObject() ) ;
         while ( it.more() )
         {
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
      rc = rtnReallocBuffer ( (CHAR**)ppReply, &replyBufferSize, replySize,
                              SDB_PAGE_SIZE ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to realloc buffer, rc = %d", rc ) ;
      ossMemset ( *ppReply, 0, replySize ) ;

      rc = rtnQuery ( collectionName, selector, matcher, orderBy, hint, flags,
                      cb, numToSkip, numToReturn, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to perform query, rc = %d", rc ) ;

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

         replySize = ossRoundUpToMultipleX ( replySize, 4 ) ;
         rc = rtnReallocBuffer ( (CHAR**)ppReply, &replyBufferSize,
                                 replySize + buffObj.size(), SDB_PAGE_SIZE ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to realloc buffer, rc = %d", rc ) ;

         ossMemcpy ( &((CHAR*)(*ppReply))[replySize], buffObj.data(),
                     buffObj.size() ) ;
         (*ppReply)->numReturned += buffObj.recordNum() ;
         replySize               += buffObj.size() ;
      }
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
      rc = rtnQuery( collectionName, selector, matcher, dummyObj, hint,
                     0, cb, 0, 1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to query from %s, rc: %d",
                   collectionName, rc ) ;

      rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
         }
         goto error ;
      }

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
            FIELD_NAME_GROUP"."FIELD_NAME_NODEID << nodeID );

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
                            BSON( FIELD_NAME_SERVICE"."FIELD_NAME_NAME <<
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

      rc = catGetGroupObj( groupName, dataGroupOnly, groupObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get group obj by name[%s], rc: %d",
                   groupName, rc ) ;

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
      else
      {
         matcher = BSON( CAT_GROUPS_NAME"."CAT_GROUPID_NAME <<
                         (INT32)groupID ) ;
      }

      rc = rtnUpdate( CAT_DOMAIN_COLLECTION, matcher, modifier,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), modifier.toString().c_str(),
                   rc ) ;

      rc = rtnUpdate( CAT_DOMAIN_COLLECTION, matcher, modifier2,
                      dummy, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), modifier2.toString().c_str(),
                   rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATDELGRPFROMDOMAIN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CAATADDCL2CS, "catAddCL2CS" )
   INT32 catAddCL2CS( const CHAR * csName, const CHAR * clName,
                      pmdEDUCB * cb, SDB_DMSCB * dmsCB, SDB_DPSCB * dpsCB,
                      INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CAATADDCL2CS ) ;
      BSONObj boMatcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;

      BSONObjBuilder updateBuild ;
      BSONObjBuilder sub( updateBuild.subobjStart("$addtoset") ) ;

      BSONObj newCLObj = BSON( CAT_COLLECTION_NAME << clName ) ;
      BSONObjBuilder sub1( sub.subarrayStart( CAT_COLLECTION ) ) ;
      sub1.append( "0", newCLObj ) ;
      sub1.done() ;

      sub.done() ;
      BSONObj updator = updateBuild.obj() ;
      BSONObj hint ;

      rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, boMatcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w ) ;

      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   boMatcher.toString().c_str(),
                   updator.toString().c_str(), rc ) ;

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

      rc = rtnResolveCollectionName( clFullName.c_str(), clFullName.size(),
                                     szCSName, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCLName, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK( rc, PDWARNING, "Resolve collection name[%s] failed, rc: %d",
                   clFullName.c_str(), rc ) ;

      {
         BSONObj modifier = BSON( "$pull" << BSON( CAT_COLLECTION <<
                                     BSON( CAT_COLLECTION_NAME << szCLName ) ) ) ;
         BSONObj matcher = BSON( CAT_COLLECTION_SPACE_NAME << szCSName ) ;
         BSONObj dummy ;

         rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, matcher, modifier,
                         dummy, 0, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                      "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                      matcher.toString().c_str(), modifier.toString().c_str(),
                      rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_CATDELCLFROMCS, rc ) ;
      return rc ;
   error:
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
      vector<string> remainCLs ;
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
            rc = rtnGetSTDStringElement( ele.embeddedObject(),
                                         CAT_COLLECTION_NAME,
                                         collection ) ;
            PD_CHECK( SDB_OK == rc, SDB_CAT_CORRUPTION, error, PDWARNING,
                      "Failed to get the field [%s], rc: %d",
                      CAT_COLLECTION_NAME, ele.toString().c_str(), rc ) ;
            if ( find( deleteCLLst.begin(), deleteCLLst.end(), collection ) ==
                 deleteCLLst.end() )
            {
               remainCLs.push_back( collection ) ;
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
   INT32 catUpdateCSCLs( const string &csName, vector<string> &collections,
                         pmdEDUCB *cb, _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB,
                         INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATUPDATECSCL ) ;

      BSONObj matcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      BSONObj dummy ;

      BSONObjBuilder builder ;
      BSONObjBuilder subBuilder( builder.subobjStart( "$set" ) ) ;
      BSONArrayBuilder arrBuilder( subBuilder.subarrayStart( CAT_COLLECTION ) ) ;
      for ( vector<string>::iterator iter = collections.begin() ;
            iter != collections.end() ;
            ++ iter )
      {
         arrBuilder << BSON( CAT_COLLECTION_NAME << (*iter) ) ;
      }
      arrBuilder.done() ;
      subBuilder.done() ;
      BSONObj modifier = builder.obj() ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRESTORECS, "catRestoreCS" )
   INT32 catRestoreCS( const CHAR * csName, const BSONObj & oldInfo,
                       pmdEDUCB * cb, SDB_DMSCB * dmsCB,
                       SDB_DPSCB * dpsCB, INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATRESTORECS ) ;
      BSONObj boMatcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      BSONObj updator   = BSON( "$set" << oldInfo ) ;
      BSONObj hint ;

      rc = rtnUpdate( CAT_COLLECTION_SPACE_COLLECTION, boMatcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update collection: %s, match: %s, "
                   "updator: %s, rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   boMatcher.toString().c_str(),
                   updator.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATRESTORECS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKSPACEEXIST, "catCheckSpaceExist" )
   INT32 catCheckSpaceExist( const char *pSpaceName, BOOLEAN &isExist,
                             BSONObj &obj, pmdEDUCB *cb )
   {
      INT32 rc           = SDB_OK ;
      isExist            = FALSE ;

      PD_TRACE_ENTRY ( SDB_CATCHECKSPACEEXIST ) ;
      BSONObj matcher = BSON( CAT_COLLECTION_SPACE_NAME << pSpaceName ) ;
      BSONObj dummyObj ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETDOMAINCSS, "catGetDomainCSs" )
   INT32 catGetDomainCSs( const BSONObj &domain, pmdEDUCB *cb,
                          vector< string > &collectionSpaces )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETDOMAINCSS ) ;
      BSONObj matcher ;
      BSONObj dummyObj ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;

      collectionSpaces.clear() ;

      matcher = BSON( CAT_DOMAIN_NAME <<
                      domain.getField( CAT_DOMAINNAME_NAME ).valuestrsafe() ) ;

      rc = rtnQuery( CAT_COLLECTION_SPACE_COLLECTION, dummyObj, matcher,
                     dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, matcher: %s, "
                   "rc: %d", CAT_COLLECTION_SPACE_COLLECTION,
                   matcher.toString().c_str(), rc ) ;

      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement csName = obj.getField( CAT_COLLECTION_SPACE_NAME ) ;
            if ( String != csName.type() )
            {
               continue ;
            }
            collectionSpaces.push_back ( csName.String() ) ;
         }
         catch( std::exception &e )
         {
            rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
            PD_LOG( PDERROR,
                    "Get collection space name from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC ( SDB_CATGETDOMAINCSS, rc ) ;
      return rc ;
   error:
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
   INT32 catUpdateCatalog( const CHAR * clFullName, const BSONObj & cataInfo,
                           pmdEDUCB * cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATUPDATECATALOG ) ;
      BSONObj hint ;
      BSONObj match = BSON( CAT_CATALOGNAME_NAME << clFullName ) ;
      BSONObj updator = BSON( "$inc" << BSON( CAT_VERSION_NAME << 1 ) <<
                              "$set" << cataInfo ) ;

      rc = rtnUpdate( CAT_COLLECTION_INFO_COLLECTION,
                      match, updator, hint,
                      0, cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDSEVERE, "Failed to update collection[%s] catalog info"
                   "[%s], rc: %d", clFullName, cataInfo.toString().c_str(),
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

   INT32 catGetCSGroupsFromCLs( const CHAR *csName, pmdEDUCB *cb,
                                vector< UINT32 > &groups,
                                BOOLEAN includeSubCLGroups )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher ;
      BSONObj dummyObj ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      std::set< UINT32 > groupSet ;
      std::set< UINT32 >::iterator itSet ;
      BSONObjBuilder builder ;
      std::stringstream ss ;

      ss << "^" << csName << "\\." ;

      if ( FALSE == includeSubCLGroups )
      {
         builder.appendRegex( CAT_COLLECTION_NAME, ss.str() ) ;
      }
      else
      {
         BSONArrayBuilder orBuilder( builder.subarrayStart( "$or" ) ) ;
         BSONObjBuilder nameObjBuilder( orBuilder.subobjStart() ) ;
         nameObjBuilder.appendRegex( CAT_COLLECTION_NAME, ss.str() ) ;
         nameObjBuilder.done() ;
         BSONObjBuilder subCLObjBuilder( orBuilder.subobjStart() ) ;
         subCLObjBuilder.appendRegex( CAT_MAINCL_NAME, ss.str() ) ;
         subCLObjBuilder.done() ;
         orBuilder.done() ;
      }
      matcher = builder.obj() ;

      rc = rtnQuery( CAT_COLLECTION_INFO_COLLECTION, dummyObj, matcher,
                     dummyObj, dummyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, matcher: %s, "
                   "rc: %d", CAT_COLLECTION_INFO_COLLECTION,
                   matcher.toString().c_str(), rc ) ;

      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
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
               BSONElement eleGrpID = cataItemObj.getField( CAT_GROUPID_NAME ) ;
               if ( eleGrpID.isNumber() )
               {
                  groupSet.insert( eleGrpID.numberInt() ) ;
               }
            }
         }
         catch( std::exception &e )
         {
            rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
            PD_LOG( PDERROR,
                    "Get collection name from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      for ( UINT32 i = 0 ; i < groups.size() ; ++i )
      {
         groupSet.insert( groups[ i ] ) ;
      }
      groups.clear() ;

      itSet = groupSet.begin() ;
      while ( itSet != groupSet.end() )
      {
         groups.push_back( *itSet ) ;
         ++itSet ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATADDTASK, "catAddTask" )
   INT32 catAddTask( BSONObj & taskObj, pmdEDUCB * cb, INT16 w )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETMAXTASKID, "catGetMaxTaskID" )
   INT64 catGetMaxTaskID( pmdEDUCB * cb )
   {
      INT64 taskID            = CLS_INVALID_TASKID ;
      INT32 rc                = SDB_OK ;
      SINT64 contextID        = -1 ;
      pmdKRCB *pKRCB          = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB        = pKRCB->getDMSCB() ;
      SDB_RTNCB *rtnCB        = pKRCB->getRTNCB() ;

      PD_TRACE_ENTRY ( SDB_CATGETMAXTASKID ) ;
      BSONObj dummyObj ;
      BSONObj orderby = BSON( CAT_TASKID_NAME << -1 ) ;

      rtnContextBuf buffObj ;

      rc = rtnQuery( CAT_TASK_INFO_COLLECTION, dummyObj, dummyObj, orderby,
                     dummyObj, 0, cb, 0, 1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to query from %s, rc: %d",
                   CAT_TASK_INFO_COLLECTION, rc ) ;

      rc = rtnGetMore( contextID, 1, buffObj, cb, rtnCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            contextID = -1 ;
         }
         goto error ;
      }

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
         taskID = (INT64)ele.numberLong() ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATUPDATETASKSTATUS, "catUpdateTaskStatus" )
   INT32 catUpdateTaskStatus( UINT64 taskID, INT32 status, pmdEDUCB * cb,
                              INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATUPDATETASKSTATUS ) ;
      BSONObj taskObj ;
      BSONObj dummy ;
      BSONObj match = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
      BSONObj updator = BSON( "$set" << BSON( CAT_STATUS_NAME << status ) ) ;

      rc = catGetTask( taskID, taskObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Get task[%lld] failed, rc: %d", taskID, rc ) ;

      rc = rtnUpdate( CAT_TASK_INFO_COLLECTION, match, updator, dummy, 0,
                      cb, dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Update task[%lld] status to [%d] failed, "
                   "rc: %d", taskID, status, rc ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATUPDATETASKSTATUS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATREMOVETASK, "catRemoveTask" )
   INT32 catRemoveTask( UINT64 taskID, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;

      PD_TRACE_ENTRY ( SDB_CATREMOVETASK ) ;
      BSONObj taskObj ;
      BSONObj match = BSON( CAT_TASKID_NAME << (INT64)taskID ) ;
      BSONObj dummy ;

      rc = catGetTask( taskID, taskObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Get task[%llu] failed, rc: %d",
                   taskID, rc ) ;

      rc = rtnDelete( CAT_TASK_INFO_COLLECTION, match, dummy, 0, cb, dmsCB,
                      dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Delete task[%s] failed, rc: %d",
                   taskObj.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB_CATREMOVETASK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATREMOVETASK1, "catRemoveTask" )
   INT32 catRemoveTask( BSONObj &match, pmdEDUCB *cb, INT16 w )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_DPSCB *dpsCB = krcb->getDPSCB() ;
      BSONObj taskObj ;
      BSONObj dummyObj ;

      PD_TRACE_ENTRY ( SDB_CATREMOVETASK1 ) ;

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

      rc = rtnDelete( CAT_TASK_INFO_COLLECTION, match, dummyObj, 0, cb,
                      dmsCB, dpsCB, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove task from collection[%s], "
                   "rc: %d, del cond: %s", CAT_TASK_INFO_COLLECTION, rc,
                   match.toString().c_str() ) ;
   done:
      PD_TRACE_EXITRC ( SDB_CATREMOVETASK1, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 catRemoveCLTasks ( const string &clName, pmdEDUCB *cb, INT16 w )
   {
      BSONObj matcher = BSON( CAT_COLLECTION_NAME << clName ) ;
      return catRemoveTask( matcher, cb, w ) ;
   }

   INT32 catGetCSGroupsFromTasks( const CHAR *csName, pmdEDUCB *cb,
                                  vector< UINT32 > &groups )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher ;
      BSONObj dummyObj ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      INT64 contextID = -1 ;
      std::set< UINT32 > groupSet ;
      std::set< UINT32 >::iterator itSet ;
      BSONObjBuilder builder ;
      std::stringstream ss ;

      ss << "^" << csName << "\\." ;
      builder.appendRegex( CAT_COLLECTION_NAME, ss.str() ) ;
      matcher = builder.obj() ;

      rc = rtnQuery( CAT_TASK_INFO_COLLECTION, dummyObj, matcher, dummyObj,
                     dummyObj, 0, cb, 0, -1, dmsCB, rtnCB, contextID ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collection[%s] failed, matcher: %s, "
                   "rc: %d", CAT_TASK_INFO_COLLECTION,
                   matcher.toString().c_str(), rc ) ;

      while ( TRUE )
      {
         BSONObj obj ;
         rtnContextBuf contextBuf ;
         rc = rtnGetMore( contextID, 1, contextBuf, cb, rtnCB ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Get more failed, rc: %d", rc ) ;

         try
         {
            obj = BSONObj( contextBuf.data() ) ;
            BSONElement ele = obj.getField( CAT_TARGETID_NAME ) ;
            groupSet.insert( ele.numberInt() ) ;
         }
         catch( std::exception &e )
         {
            rtnKillContexts( 1 , &contextID, cb, rtnCB ) ;
            PD_LOG( PDERROR, "Get group id from obj[%s] occur exception: %s",
                    obj.toString().c_str(), e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      for ( UINT32 i = 0 ; i < groups.size() ; ++i )
      {
         groupSet.insert( groups[ i ] ) ;
      }
      groups.clear() ;

      itSet = groupSet.begin() ;
      while ( itSet != groupSet.end() )
      {
         groups.push_back( *itSet ) ;
         ++itSet ;
      }

   done:
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
      INT64 updateNum = 0 ;
      BSONObj hint ;

      if ( enable )
      {
         updator = BSON( "$set" << BSON( FIELD_NAME_IMAGE"."FIELD_NAME_ENABLE
                                         << true ) ) ;
      }
      else
      {
         updator = BSON( "$set" << BSON( FIELD_NAME_IMAGE"."FIELD_NAME_ENABLE
                                         << false ) ) ;
      }
      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w, &updateNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                   "rc: %d", updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      if ( 0 == updateNum )
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
      INT64 updateNum = 0 ;
      BSONObj hint ;

      if ( status )
      {
         updator = BSON( "$set" << BSON( pField << true ) ) ;
      }
      else
      {
         updator = BSON( "$set" << BSON( pField << false ) ) ;
      }
      rc = rtnUpdate( CAT_SYSDCBASE_COLLECTION_NAME, matcher, updator,
                      hint, 0, cb, dmsCB, dpsCB, w, &updateNum ) ;
      PD_RC_CHECK( rc, PDERROR, "Update obj[%s] to collection[%s] failed, "
                   "rc: %d", updator.toString().c_str(),
                   CAT_SYSDCBASE_COLLECTION_NAME, rc ) ;
      if ( 0 == updateNum )
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
                            catContext **context,
                            SINT64 &contextID,
                            _pmdEDUCB *pEDUCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCREATECONTEXT ) ;

      pmdKRCB *krcb = pmdGetKRCB();
      _SDB_RTNCB *pRtnCB = krcb->getRTNCB();

      RTN_CONTEXT_TYPE contextType ;
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
                                  (rtnContext **)context,
                                  contextID,
                                  pEDUCB ) ;
      }
      if ( SDB_OK == rc &&
           pEDUCB->getMonConfigCB()->timestampON )
      {
         (*context)->getMonCB()->recordStartTimestamp() ;
      }

      PD_TRACE_EXITRC ( SDB_CATCREATECONTEXT, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATFINDCONTEXT, "catFindContext" )
   INT32 catFindContext ( SINT64 contextID,
                          catContext **pCtx,
                          _pmdEDUCB *pEDUCB )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATFINDCONTEXT ) ;

      rtnContext *pTmpCtx = NULL ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      _SDB_RTNCB *pRtnCB = krcb->getRTNCB() ;

      if ( -1 != contextID  )
      {
         pTmpCtx = pRtnCB->contextFind( contextID ) ;
         if ( !pTmpCtx )
         {
            rc = SDB_RTN_CONTEXT_NOTEXIST ;
            goto done ;
         }
         else if ( RTN_CONTEXT_CAT_BEGIN >= pTmpCtx->getType() ||
                   RTN_CONTEXT_CAT_END <= pTmpCtx->getType() )
         {
            rc = SDB_RTN_CONTEXT_NOTEXIST ;
            goto done ;
         }
      }
      if ( pTmpCtx && pCtx )
      {
         (*pCtx) = (catContext *)pTmpCtx ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATFINDCONTEXT, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCOLLETION, "catGetCollection" )
   INT32 catGetCollection ( const string &clName, BSONObj &boCollection,
                            _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETCOLLETION ) ;

      BOOLEAN isExist = FALSE ;

      rc = catCheckCollectionExist( clName.c_str(), isExist, boCollection, cb ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCHECKMAINCOLLECTION, "catCheckMainCollection" )
   INT32 catCheckMainCollection ( const BSONObj &boCollection,
                                  BOOLEAN expectMain )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKMAINCOLLECTION ) ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETANDLOCKCOLLECTION, "catGetAndLockCollection" )
   INT32 catGetAndLockCollection ( const string &clName, BSONObj &boCollection,
                                   _pmdEDUCB *cb,
                                   catCtxLockMgr *pLockMgr,
                                   OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETANDLOCKCOLLECTION ) ;

      try
      {
         BOOLEAN isExist = FALSE ;
         rc = catCheckCollectionExist( clName.c_str(), isExist, boCollection, cb ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to get info of collection [%s], rc: %d",
                      clName.c_str(), rc ) ;
         PD_CHECK( isExist, SDB_DMS_NOTEXIST, error, PDWARNING,
                   "Collection [%s] does not exist!",
                   clName.c_str() ) ;

         if ( pLockMgr &&
              !pLockMgr->tryLockCollection( clName, mode ) )
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATLOCKGROUPS, "catLockGroups" )
   INT32 catLockGroups ( vector<UINT32> &groupIDList,
                         _pmdEDUCB *cb,
                         catCtxLockMgr &lockMgr,
                         OSS_LATCH_MODE mode )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATLOCKGROUPS ) ;

      for ( UINT32 idx = 0 ; idx < groupIDList.size() ; ++idx )
      {
         string groupName ;
         UINT32 groupID = groupIDList[idx] ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATGETCLGRPCASCADE, "catGetCollectionGroupsCascade" )
   INT32 catGetCollectionGroupsCascade ( const std::string &clName,
                                         const BSONObj &boCollection,
                                         _pmdEDUCB *cb,
                                         std::vector<UINT32> &groupIDList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATGETCLGRPCASCADE ) ;

      if ( boCollection.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         clsCatalogSet cataSet( clName.c_str() );

         rc = cataSet.updateCatSet( boCollection ) ;
         PD_RC_CHECK( rc, PDWARNING,
                      "Failed to parse catalog info of collection [%s], rc: %d",
                      clName.c_str(), rc ) ;

         if ( cataSet.isMainCL() )
         {
            std::vector< std::string > subCLLst;
            std::vector< std::string >::iterator iterSubCL;
            rc = cataSet.getSubCLList( subCLLst );
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to get sub-collection list of collection [%s], "
                         "rc: %d",
                         clName.c_str(), rc ) ;
            iterSubCL = subCLLst.begin() ;
            while( iterSubCL != subCLLst.end() )
            {
               const std::string &subCLName = (*iterSubCL) ;
               BSONObj boSubCL ;

               rc = catGetCollection( subCLName, boSubCL, cb ) ;
               PD_RC_CHECK( rc, PDWARNING,
                            "Failed to get sub-collection [%s], rc: %d",
                            subCLName.c_str(), rc ) ;

               rc = catGetCollectionGroupSet( boSubCL, groupIDList ) ;
               PD_RC_CHECK( rc, PDWARNING,
                            "Failed to collect groups of sub-collection [%s], "
                            "rc: %d",
                            subCLName.c_str(), rc ) ;

               ++iterSubCL ;
            }
         }
         else
         {
            rc = catGetCollectionGroupSet( boCollection, groupIDList ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to collect groups for collection [%s], rc: %d",
                         boCollection.toString().c_str(), rc ) ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATGETCLGRPCASCADE, rc ) ;
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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATDROPCSSTEP, "catDropCSStep" )
   INT32 catDropCSStep ( const string &csName,
                         _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                         INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATDROPCSSTEP ) ;

      BSONObj boMatcher = BSON( CAT_COLLECTION_SPACE_NAME << csName ) ;
      BSONObj dummyObj ;

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

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCREATECLSTEP, "catCreateCLStep" )
   INT32 catCreateCLStep ( const string &clName, BSONObj &boCollection,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCREATECLSTEP ) ;

      CHAR szSpace[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR szCollection[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      rc = rtnResolveCollectionName( clName.c_str(),
                                     clName.size(),
                                     szSpace, DMS_COLLECTION_SPACE_NAME_SZ,
                                     szCollection, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK ( rc, PDWARNING,
                    "Failed to resolve collection name: %s",
                    clName.c_str() ) ;

      rc = rtnInsert( CAT_COLLECTION_INFO_COLLECTION, boCollection,
                      1, 0, cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed insert record [%s] to collection [%s], rc: %d",
                   boCollection.toString().c_str(),
                   CAT_COLLECTION_INFO_COLLECTION, rc ) ;

      rc = catAddCL2CS( szSpace, szCollection, cb, pDmsCB, pDpsCB, w ) ;
      if ( SDB_OK != rc )
      {
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
                         INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATDROPCLSTEP ) ;

      BSONObj boCollection ;

      rc = catRemoveCLTasks( clName, cb, w ) ;
      if ( SDB_CAT_TASK_NOTFOUND == rc )
      {
         rc = SDB_OK ;
      }
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to remove tasks with the collection [%s], rc: %d",
                   clName.c_str(), rc ) ;

      rc = catGetCollection( clName, boCollection, cb ) ;
      if ( SDB_OK == rc )
      {
         INT32 curVersion = -1 ;

         rc = rtnGetIntElement( boCollection, CAT_VERSION_NAME, curVersion ) ;
         if ( SDB_OK == rc )
         {
            version = curVersion ;
         }
      }
      else
      {
         PD_LOG( PDWARNING,
                 "Failed to get version of collection [%s], rc: %d",
                 clName.c_str(), rc ) ;
      }

      rc = catRemoveCL( clName.c_str(), cb, pDmsCB, pDpsCB, w ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to remove collection [%s], rc: %d",
                   clName.c_str(), rc ) ;

      rc = catSaveBucketVersion( clName.c_str(), version, cb, w ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDWARNING,
                 "Failed to save history version of collection [%s], rc: %d",
                 clName.c_str(), rc ) ;
      }

      if ( delFromCS )
      {
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
                          _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                          INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATALTERCLSTEP ) ;

      rc = catUpdateCatalog( clName.c_str(), boNewData, cb, w ) ;
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
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj, cb, w ) ;
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
         rc = catUpdateCatalog( subCLName.c_str(), newSubCLObj, cb, w ) ;
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

      rc = catGetCollection( mainCLName, mainCLObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "failed to get catalog-info of main-collection(%s)",
                   mainCLName.c_str() ) ;

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

         tmprc = mainCLSet.getSubCLBounds( subCLName, tmpLowBound, tmpUpBound) ;
         if ( SDB_OK != tmprc )
         {
            PD_LOG( PDWARNING,
                    "Failed to get sub-collection [%s] info from "
                    "main-collection [%s], rc: %d",
                    subCLName.c_str(), mainCLName.c_str(), tmprc ) ;
         }
         else
         {
            lowBound = tmpLowBound.getOwned() ;
            upBound = tmpUpBound.getOwned() ;
         }
      }

      rc = mainCLSet.delSubCL( subCLName.c_str() ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to delete the sub-collection [%s] from "
                   "main-collection [%s], rc: %d",
                   subCLName.c_str(), mainCLName.c_str(), rc ) ;

      {
         BSONObj subListObj = mainCLSet.toCataInfoBson() ;
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj, cb, w ) ;
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
      vector<string> subCLLst ;
      CHAR szCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = {0} ;
      CHAR szCLName[ DMS_COLLECTION_NAME_SZ + 1 ] = {0} ;

      rc = catGetCollection( mainCLName, mainCLObj, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "failed to get catalog-info of main-collection(%s)",
                   mainCLName.c_str() ) ;

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

      for ( vector<string>::iterator iter = subCLLst.begin() ;
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
            rc = mainCLSet.delSubCL( subCLName.c_str() ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to delete the sub-collection [%s] from "
                         "main-collection [%s], rc: %d",
                         subCLName.c_str(), mainCLName.c_str(), rc ) ;
         }
      }

      {
         BSONObj subListObj = mainCLSet.toCataInfoBson() ;
         rc = catUpdateCatalog( mainCLName.c_str(), subListObj, cb, w ) ;
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
                                     catCollectionInfo &clInfo,
                                     BOOLEAN needCLName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATCHECKANDBUILDCATARECORD ) ;

      clInfo._pCLName            = NULL ;
      clInfo._replSize           = 1 ;
      clInfo._enSureShardIndex   = true ;
      clInfo._pShardingType      = CAT_SHARDING_TYPE_HASH ;
      clInfo._shardPartition     = CAT_SHARDING_PARTITION_DEFAULT ;
      clInfo._isHash             = TRUE ;
      clInfo._isSharding         = FALSE ;
      clInfo._isMainCL           = false;
      clInfo._strictDataMode     = FALSE ;
      clInfo._assignType         = ASSIGN_RANDOM ;

      fieldMask = 0 ;

      BSONObjIterator it( boCollection ) ;
      while ( it.more() )
      {
         BSONElement eleTmp = it.next() ;

         if ( ossStrcmp( eleTmp.fieldName(), CAT_COLLECTION_NAME ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_COLLECTION_NAME,
                      eleTmp.type() ) ;
            clInfo._pCLName = eleTmp.valuestr() ;
            fieldMask |= CAT_MASK_CLNAME ;
         }
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
            fieldMask |= CAT_MASK_SHDKEY ;
            clInfo._isSharding = TRUE ;
         }
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
            }
            else if ( clInfo._replSize == 0 )
            {
               clInfo._replSize = CLS_REPLSET_MAX_NODE_SIZE ;
            }
            else if ( -1 == clInfo._replSize )
            {
            }
            else
            {
               PD_LOG( PDWARNING,
                       "Invalid repl size: %d",
                       clInfo._replSize ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            fieldMask |= CAT_MASK_REPLSIZE ;
         }
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_ENSURE_SHDINDEX ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_ENSURE_SHDINDEX,
                      eleTmp.type() ) ;
            clInfo._enSureShardIndex = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_SHDIDX ;
         }
         else if ( ossStrcmp( eleTmp.fieldName(), CAT_SHARDING_TYPE ) == 0 )
         {
            PD_CHECK( String == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error", CAT_SHARDING_TYPE,
                      eleTmp.type() ) ;

            clInfo._pShardingType = eleTmp.valuestr() ;
            PD_CHECK( 0 == ossStrcmp( clInfo._pShardingType,
                                      CAT_SHARDING_TYPE_HASH ) ||
                      0 == ossStrcmp( clInfo._pShardingType,
                                      CAT_SHARDING_TYPE_RANGE ),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] value [%s] should be [%s/%s]",
                      CAT_SHARDING_TYPE, clInfo._pShardingType,
                      CAT_SHARDING_TYPE_HASH, CAT_SHARDING_TYPE_RANGE ) ;
            fieldMask |= CAT_MASK_SHDTYPE ;

            clInfo._isHash = ( 0 == ossStrcmp( clInfo._pShardingType,
                                               CAT_SHARDING_TYPE_HASH ) ) ;
         }
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_SHARDING_PARTITION ) == 0 )
         {
            PD_CHECK( NumberInt == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_SHARDING_PARTITION, eleTmp.type() ) ;
            clInfo._shardPartition = eleTmp.numberInt() ;
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
            fieldMask |= CAT_MASK_SHDPARTITION ;
         }
         else if ( ossStrcmp ( eleTmp.fieldName(),
                               CAT_COMPRESSED ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_COMPRESSED, eleTmp.type() ) ;
            clInfo._isCompressed = eleTmp.boolean() ;
            fieldMask |= CAT_MASK_COMPRESSED ;
         }
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_IS_MAINCL ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_IS_MAINCL, eleTmp.type() ) ;
            clInfo._isMainCL = eleTmp.boolean() ;
            fieldMask |= CAT_MASK_ISMAINCL ;
            if ( !( fieldMask & CAT_MASK_SHDTYPE ) )
            {
               clInfo._pShardingType = CAT_SHARDING_TYPE_RANGE ;
               clInfo._isHash = FALSE ;
            }
         }
         else if ( ossStrcmp( eleTmp.fieldName(),
                              CAT_STRICTDATAMODE ) == 0 )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_STRICTDATAMODE, eleTmp.type() ) ;
            clInfo._strictDataMode = eleTmp.boolean() ;
            fieldMask |= CAT_MASK_STRICTDATAMODE ;
         }
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
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_DOMAIN_AUTO_SPLIT ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_DOMAIN_AUTO_SPLIT, eleTmp.type() ) ;
            clInfo._autoSplit = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_AUTOASPLIT ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_DOMAIN_AUTO_REBALANCE ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_DOMAIN_AUTO_REBALANCE, eleTmp.type() ) ;
            clInfo._autoRebalance = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_AUTOREBALAN ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(),
                                   CAT_AUTO_INDEX_ID ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_AUTO_INDEX_ID, eleTmp.type() ) ;
            clInfo._autoIndexId = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_AUTOINDEXID ;
         }
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

            fieldMask |= CAT_MASK_COMPRESSIONTYPE ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CAPPED_NAME ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CAPPED_NAME, eleTmp.type() ) ;
            clInfo._capped = eleTmp.boolean() ;
            fieldMask |= CAT_MASK_CAPPED ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CL_MAX_RECNUM ) )
         {
            PD_CHECK( NumberLong == eleTmp.type()
                      || NumberInt == eleTmp.type()
                      || NumberDouble == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CL_MAX_RECNUM, eleTmp.type() ) ;
            clInfo._maxRecNum = eleTmp.numberLong() ;
            fieldMask |= CAT_MASK_CLMAXRECNUM ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CL_MAX_SIZE ) )
         {
            PD_CHECK( NumberLong == eleTmp.type()
                      || NumberInt == eleTmp.type()
                      || NumberDouble == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CL_MAX_SIZE, eleTmp.type() ) ;
            clInfo._maxSize = ossRoundUpToMultipleX( eleTmp.numberLong() << 20,
                                                     MAX_CL_SIZE_ALIGN_SIZE ) ;
            fieldMask |= CAT_MASK_CLMAXSIZE ;
         }
         else if ( 0 == ossStrcmp( eleTmp.fieldName(), CAT_CL_OVERWRITE ) )
         {
            PD_CHECK( Bool == eleTmp.type(),
                      SDB_INVALIDARG, error, PDWARNING,
                      "Field [%s] type [%d] error",
                      CAT_CL_OVERWRITE, eleTmp.type() ) ;
            clInfo._overwrite = eleTmp.Bool() ;
            fieldMask |= CAT_MASK_CLOVERWRITE ;
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

         PD_CHECK( !( CAT_MASK_AUTOINDEXID & fieldMask ),
                   SDB_INVALIDARG, error, PDWARNING,
                   "can not set auto-index-id on main collection" ) ;
         PD_CHECK( !( ( CAT_MASK_CAPPED & fieldMask ) ||
                      ( CAT_MASK_CLMAXRECNUM & fieldMask ) ||
                      ( CAT_MASK_CLMAXSIZE & fieldMask ) ||
                      ( CAT_MASK_CLOVERWRITE & fieldMask ) ),
                   SDB_INVALIDARG, error, PDWARNING,
                   "can not set Capped|Max|Size on main collection" ) ;
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

      if ( fieldMask & CAT_MASK_SHDIDX ||
           fieldMask & CAT_MASK_SHDTYPE ||
           fieldMask & CAT_MASK_SHDPARTITION )
      {
         PD_CHECK( fieldMask & CAT_MASK_SHDKEY,
                   SDB_INVALIDARG, error, PDWARNING,
                   "these arguments are legal only when sharding key is specified." ) ;
      }

      if ( needCLName )
      {
         PD_CHECK( clInfo._pCLName, SDB_INVALIDARG, error, PDWARNING,
                   "Collection name not set" ) ;
      }

      if ( clInfo._isCompressed )
      {
         if ( 0 == ( fieldMask & CAT_MASK_COMPRESSIONTYPE ) )
         {
            clInfo._compressorType = UTIL_COMPRESSOR_SNAPPY ;
         }
      }
      else
      {
         PD_CHECK( UTIL_COMPRESSOR_INVALID == clInfo._compressorType,
                   SDB_INVALIDARG, error, PDWARNING,
                   "CompressionType can only be set when Compressed is true."
                   ) ;
      }

      if ( !clInfo._capped && clInfo._maxSize )
      {
         clInfo._capped = TRUE ;
         fieldMask |= CAT_MASK_CAPPED ;
      }

   done :
      PD_TRACE_EXITRC ( SDB_CATCHECKANDBUILDCATARECORD, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   static INT32 _catBuildInitBound ( const BSONObj &shardingKey,
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

   static INT32 _catBuildHashBound( BSONObj &lowBound, BSONObj &upBound,
                                    INT32 paritition )
   {
      INT32 rc = SDB_OK ;

      lowBound = BSON("" << CAT_HASH_LOW_BOUND ) ;
      upBound = BSON("" << paritition )  ;

      return rc ;
   }

   static INT32 _catBuildHashBound( BSONObj &lowBound, BSONObj &upBound,
                                    INT32 beginBound, INT32 endBound )
   {
      INT32 rc = SDB_OK ;

      lowBound = BSON("" << beginBound ) ;
      upBound = BSON("" << endBound )  ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATBUILDCATARECORD, "catBuildCatalogRecord" )
   INT32 catBuildCatalogRecord( const catCollectionInfo &clInfo,
                                UINT32 mask, UINT32 attribute,
                                const std::vector<UINT32> &grpIDLst,
                                const std::map<std::string, UINT32> &splitLst,
                                BSONObj &catRecord )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( SDB_CATBUILDCATARECORD ) ;

      BSONObjBuilder builder ;
      CHAR szAttr[ 100 ] = { 0 } ;

      if ( ( mask & CAT_MASK_COMPRESSED ) && clInfo._isCompressed )
      {
         attribute |= DMS_MB_ATTR_COMPRESSED ;
      }
      if ( ( mask & CAT_MASK_AUTOINDEXID ) && !clInfo._autoIndexId )
      {
         attribute |= DMS_MB_ATTR_NOIDINDEX ;
      }
      if ( ( mask & CAT_MASK_CAPPED ) && clInfo._capped )
      {
         attribute |= DMS_MB_ATTR_CAPPED ;
      }
      if ( ( mask & CAT_MASK_STRICTDATAMODE ) && clInfo._strictDataMode )
      {
         attribute |= DMS_MB_ATTR_STRICTDATAMODE ;
      }
      mbAttr2String( attribute, szAttr, sizeof( szAttr ) - 1 ) ;

      if ( mask & CAT_MASK_CLNAME )
      {
         builder.append( CAT_CATALOGNAME_NAME, clInfo._pCLName ) ;
      }

      builder.append( CAT_CATALOGVERSION_NAME,
                      0 == clInfo._version ?
                      CAT_VERSION_BEGIN :
                      clInfo._version ) ;

      if ( mask & CAT_MASK_REPLSIZE )
      {
         builder.append( CAT_CATALOG_W_NAME, clInfo._replSize ) ;
      }

      builder.append( CAT_ATTRIBUTE_NAME, attribute ) ;
      builder.append( FIELD_NAME_ATTRIBUTE_DESC, szAttr ) ;

      if ( ( mask & CAT_MASK_COMPRESSED ) && clInfo._isCompressed )
      {
         builder.append( CAT_COMPRESSIONTYPE, clInfo._compressorType ) ;
         builder.append( FIELD_NAME_COMPRESSIONTYPE_DESC,
                         utilCompressType2String( clInfo._compressorType ) ) ;
      }
      if ( mask & CAT_MASK_SHDKEY )
      {
         builder.append( CAT_SHARDINGKEY_NAME, clInfo._shardingKey ) ;
         builder.appendBool( CAT_ENSURE_SHDINDEX, clInfo._enSureShardIndex ) ;
         builder.append( CAT_SHARDING_TYPE, clInfo._pShardingType ) ;
         if( clInfo._isHash )
         {
            builder.append( CAT_SHARDING_PARTITION, clInfo._shardPartition ) ;

            builder.append( CAT_INTERNAL_VERSION, CAT_INTERNAL_VERSION_3 ) ;
         }
      }
      if ( clInfo._isMainCL )
      {
         builder.appendBool( CAT_IS_MAINCL, clInfo._isMainCL );
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         sub.done() ;
      }
      else if ( clInfo._autoSplit && !splitLst.empty() )
      {
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         INT32 itemID = 0 ;
         UINT32 totalBound = (UINT32) clInfo._shardPartition ;
         UINT32 grpSize = splitLst.size() ;
         UINT32 avgBound = totalBound / grpSize ;
         UINT32 beginBound = CAT_HASH_LOW_BOUND ;
         UINT32 endBound = beginBound + avgBound ;

         PD_CHECK ( clInfo._isSharding && clInfo._isHash,
                    SDB_INVALIDARG, error, PDWARNING,
                    "AutoSplit only on hash sharding" ) ;

         for ( std::map<std::string, UINT32>::const_iterator iterGrp = splitLst.begin();
               iterGrp != splitLst.end();
               ++iterGrp )
         {
            UINT32 grpID = iterGrp->second ;
            const std::string &grpName = iterGrp->first ;
            BSONObj lowBound, upBound ;

            BSONObjBuilder cataItemBd ( sub.subobjStart ( sub.numStr(itemID) ) ) ;
            cataItemBd.append ( FIELD_NAME_ID, itemID ) ;
            cataItemBd.append ( CAT_CATALOGGROUPID_NAME, (INT32)grpID ) ;
            cataItemBd.append ( CAT_GROUPNAME_NAME, grpName ) ;

            rc = _catBuildHashBound( lowBound, upBound, beginBound, endBound ) ;
            PD_RC_CHECK( rc, PDWARNING,
                         "Failed to build cata info bound, rc: %d",
                         rc ) ;

            cataItemBd.append ( CAT_LOWBOUND_NAME, lowBound ) ;
            cataItemBd.append ( CAT_UPBOUND_NAME, upBound ) ;

            cataItemBd.done () ;

            beginBound = endBound ;
            endBound = endBound + avgBound ;
            ++ itemID ;
            if ( grpSize - 1 == (UINT32)itemID )
            {
               endBound = totalBound ;
            }
         }
         sub.done () ;
      }
      else
      {
         BSONObjBuilder sub( builder.subarrayStart( CAT_CATALOGINFO_NAME ) ) ;
         BSONObjBuilder cataItemBd ( sub.subobjStart ( sub.numStr(0) ) ) ;
         pmdKRCB *krcb = pmdGetKRCB() ;
         sdbCatalogueCB *pCatCB = krcb->getCATLOGUECB() ;
         UINT32 grpID = CAT_INVALID_GROUPID ;
         std::string grpName ;

         PD_CHECK( grpIDLst.size() == 1,
                   SDB_INVALIDARG, error, PDWARNING,
                   "Must has only one group specified" ) ;

         grpID = grpIDLst[0] ;
         cataItemBd.append ( CAT_CATALOGGROUPID_NAME, (INT32)grpID ) ;

         grpName = pCatCB->groupID2Name( grpID ) ;
         cataItemBd.append ( CAT_GROUPNAME_NAME, grpName ) ;

         if ( clInfo._isSharding )
         {
            BSONObj lowBound, upBound ;

            if ( !clInfo._isHash )
            {
               Ordering order = Ordering::make( clInfo._shardingKey ) ;
               rc = _catBuildInitBound ( clInfo._shardingKey, order ,
                                         lowBound, upBound ) ;
            }
            else
            {
               rc = _catBuildHashBound( lowBound, upBound,
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

      if ( mask & CAT_MASK_AUTOASPLIT )
      {
         builder.appendBool ( CAT_DOMAIN_AUTO_SPLIT, clInfo._autoSplit ) ;
      }

      if ( mask & CAT_MASK_AUTOREBALAN )
      {
         builder.appendBool ( CAT_DOMAIN_AUTO_REBALANCE, clInfo._autoRebalance ) ;
      }

      if ( mask & CAT_MASK_AUTOINDEXID )
      {
         builder.appendBool( CAT_AUTO_INDEX_ID, clInfo._autoIndexId ) ;
      }

      if ( mask & CAT_MASK_CLMAXRECNUM )
      {
         builder.append( CAT_CL_MAX_RECNUM, (INT64)clInfo._maxRecNum ) ;
      }

      if ( mask & CAT_MASK_CLMAXSIZE )
      {
         builder.append( CAT_CL_MAX_SIZE, (INT64)clInfo._maxSize ) ;
      }

      if ( mask & CAT_MASK_CLOVERWRITE )
      {
         builder.appendBool( CAT_CL_OVERWRITE, clInfo._overwrite ) ;
      }

      catRecord = builder.obj () ;

   done :
      PD_TRACE_EXITRC ( SDB_CATBUILDCATARECORD, rc ) ;
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

      newNodeBuilder.append( CAT_HOST_FIELD_NAME, hostName ) ;
      newNodeBuilder.append( PMD_OPTION_DBPATH, dbPath ) ;

      if ( utilCheckInstanceID( instanceID, FALSE ) )
      {
         newNodeBuilder.append( PMD_OPTION_INSTANCE_ID, (INT32)instanceID ) ;
      }

      BSONObjBuilder sub( newNodeBuilder.subarrayStart( CAT_SERVICE_FIELD_NAME ) ) ;
      BSONObjBuilder sub1( sub.subobjStart("0") ) ;
      sub1.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_LOCAL_SERVICE ) ;
      sub1.append( CAT_SERVICE_NAME_FIELD_NAME, localSvc ) ;
      sub1.done() ;

      BSONObjBuilder sub2( sub.subobjStart("1") ) ;
      sub2.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_REPL_SERVICE ) ;
      sub2.append( CAT_SERVICE_NAME_FIELD_NAME, replSvc ) ;
      sub2.done() ;
      BSONObjBuilder sub3( sub.subobjStart("2") ) ;
      sub3.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_SHARD_SERVCIE ) ;
      sub3.append( CAT_SERVICE_NAME_FIELD_NAME, shardSvc ) ;
      sub3.done() ;
      if ( SDB_ROLE_CATALOG == nodeRole )
      {
         BSONObjBuilder sub4( sub.subobjStart("3") ) ;
         sub4.append( CAT_SERVICE_TYPE_FIELD_NAME, MSG_ROUTE_CAT_SERVICE ) ;
         sub4.append( CAT_SERVICE_NAME_FIELD_NAME, cataSvc ) ;
         sub4.done() ;
      }

      sub.done() ;

      newNodeBuilder.append( CAT_NODEID_NAME, nodeID ) ;
      newNodeBuilder.append( CAT_GROUP_STATUS, nodeStatus ) ;
      BSONObj newInfoObj = newNodeBuilder.obj() ;

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

      BSONObjBuilder updateBuilder ;
      BSONObj updator, matcher, dummyObj ;
      BSONArray baNewNodeList ;
      BSONObj boGroup ;

      rc = catGetGroupObj( groupName.c_str(), FALSE, boGroup, cb ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get group [%s], rc: %d",
                   groupName.c_str(), rc ) ;

      rc = _catRemoveNode( boGroup, nodeID, baNewNodeList ) ;
      PD_RC_CHECK( rc, PDWARNING,
                   "Failed to get new node list" ) ;

      updateBuilder.append( "$inc", BSON( CAT_VERSION_NAME << 1 ) ) ;
      updateBuilder.append( "$set", BSON( FIELD_NAME_GROUP << baNewNodeList ) ) ;

      updator = updateBuilder.obj() ;

      matcher = BSON( FIELD_NAME_GROUPNAME << groupName ) ;

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
}

