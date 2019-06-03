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

   Source File Name = pd.hpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "msgCatalog.hpp"
#include "msgMessage.hpp"
#include "pd.hpp"
#include "../bson/bsonobj.h"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "msgTrace.hpp"
#include "utilCommon.hpp"

using namespace bson ;
namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB_MSGPASCATGRPRES, "msgParseCatGroupRes" )
   INT32 msgParseCatGroupRes( const MsgCatGroupRes *msg,
                              CLS_GROUP_VERSION &version,
                              string &groupName,
                              map<UINT64, _netRouteNode> &group,
                              UINT32 *pPrimary,
                              UINT32 *pSecID )
   {
      SDB_ASSERT( NULL != msg, "data should not be NULL" ) ;
      INT32 rc = SDB_OK ;
      MsgHeader *pHeader = (MsgHeader*)msg ;
      PD_TRACE_ENTRY ( SDB_MSGPASCATGRPRES );

      if ( SDB_OK != MSG_GET_INNER_REPLY_RC( pHeader ) )
      {
         rc = MSG_GET_INNER_REPLY_RC( pHeader ) ;
         goto done ;
      }
      {
         UINT32 groupID = 0 ;
         rc = msgParseCatGroupObj( MSG_GET_INNER_REPLY_DATA( pHeader ),
                                   version, groupID, groupName,
                                   group, pPrimary, pSecID ) ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_MSGPASCATGRPRES, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGPASNODESERV, "_msgParseNodeService" )
   static INT32 _msgParseNodeService ( const BSONObj & service,
                                       _netRouteNode & route )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MSGPASNODESERV ) ;

      UINT16 servID = 0 ;
      BSONElement beServID, beServName ;

      beServID = service.getField( CAT_SERVICE_TYPE_FIELD_NAME ) ;
      PD_CHECK( NumberInt == beServID.type(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse [%s]", CAT_SERVICE_TYPE_FIELD_NAME ) ;

      if ( MSG_ROUTE_REPL_SERVICE == beServID.Int() ||
           MSG_ROUTE_SHARD_SERVCIE == beServID.Int() ||
           MSG_ROUTE_CAT_SERVICE == beServID.Int() ||
           MSG_ROUTE_LOCAL_SERVICE == beServID.Int() ||
           MSG_ROUTE_OM_SERVICE == beServID.Int() )
      {
         servID = (UINT16)beServID.Int() ;
      }
      else
      {
         PD_LOG( PDWARNING, "Unknown service type: %d", beServID.Int() ) ;
         goto done ;
      }

      beServName = service.getField( CAT_SERVICE_NAME_FIELD_NAME ) ;
      PD_CHECK( String == beServName.type(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse [%s]", CAT_SERVICE_NAME_FIELD_NAME ) ;

      route._service[ servID ] = beServName.String() ;

   done :
      PD_TRACE_EXITRC( SDB__MSGPASNODESERV, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGPASNODESERVS, "_msgParseNodeServices" )
   static INT32 _msgParseNodeServices ( const BSONObj & boServices,
                                        _netRouteNode & route )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MSGPASNODESERVS ) ;

      BSONObjIterator iterServ( boServices ) ;
      while ( iterServ.more() )
      {
         BSONElement beService = iterServ.next() ;
         PD_CHECK( Object == beService.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse node service" ) ;

         rc = _msgParseNodeService( beService.embeddedObject(), route ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to parse service for node" ) ;
      }

   done :
      PD_TRACE_EXITRC( SDB__MSGPASNODESERVS, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MSGPASCATNODEOBJ, "_msgParseCatNodeObj" )
   static INT32 _msgParseCatNodeObj ( const BSONObj & nodeObj,
                                      _netRouteNode & route )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__MSGPASCATNODEOBJ ) ;

      UINT16 nodeID = 0 ;

      BSONElement beField = nodeObj.getField( CAT_NODEID_NAME ) ;
      PD_CHECK( NumberInt == beField.type(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse [%s]", CAT_NODEID_NAME ) ;
      nodeID = beField.Int() ;

      beField = nodeObj.getField( CAT_HOST_FIELD_NAME ) ;
      PD_CHECK( String == beField.type(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse [%s]", CAT_HOST_FIELD_NAME ) ;
      ossStrncpy( route._host, beField.valuestrsafe(), OSS_MAX_HOSTNAME ) ;
      route._host[ OSS_MAX_HOSTNAME ] = '\0' ;

      beField = nodeObj.getField( CAT_STATUS_NAME ) ;
      if ( beField.eoo() || SDB_CAT_GRP_ACTIVE == beField.numberInt() )
      {
         route._isActive = TRUE ;
      }
      else
      {
         route._isActive = FALSE ;
      }

      beField = nodeObj.getField( CAT_SERVICE_FIELD_NAME ) ;
      PD_CHECK( Array == beField.type(), SDB_INVALIDARG, error, PDWARNING,
                "Failed to parse [%s]", CAT_SERVICE_FIELD_NAME ) ;
      rc = _msgParseNodeServices( beField.embeddedObject(), route ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to parse node services, rc: %d",
                   rc ) ;

      beField = nodeObj.getField( PMD_OPTION_INSTANCE_ID ) ;
      PD_CHECK( beField.eoo() || NumberInt == beField.type(), SDB_INVALIDARG,
                error, PDWARNING, "Failed to parse [%s]",
                PMD_OPTION_INSTANCE_ID ) ;
      if ( NumberInt == beField.type() )
      {
         INT32 instanceID = beField.Int() ;
         if ( utilCheckInstanceID( instanceID, FALSE ) )
         {
            route._instanceID = (UINT8)instanceID ;
         }
      }

      route._id.columns.nodeID = nodeID ;
      route._id.columns.serviceID = MSG_ROUTE_REPL_SERVICE ;

   done :
      PD_TRACE_EXITRC( SDB__MSGPASCATNODEOBJ, rc ) ;
      return rc ;

   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_MSGPASCATGRPOBJ, "msgParseCatGroupObj" )
   INT32 msgParseCatGroupObj( const CHAR* objdata,
                              CLS_GROUP_VERSION &version,
                              UINT32 &groupID,
                              string &groupName,
                              map<UINT64, _netRouteNode> &group,
                              UINT32 *pPrimary,
                              UINT32 *pSecID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_MSGPASCATGRPOBJ ) ;

      SDB_ASSERT( NULL != objdata, "data should not be NULL" ) ;

      try
      {
         BSONObj obj( objdata ) ;
         PD_LOG( PDDEBUG, "Parsing group bson: %s", obj.toString().c_str() ) ;

         BSONElement ele = obj.getField( CAT_GROUPID_NAME ) ;
         PD_CHECK( NumberInt == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse [%s]", CAT_GROUPID_NAME ) ;
         groupID = ele.Int() ;

         ele = obj.getField( CAT_GROUPNAME_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse [%s]", CAT_GROUPNAME_NAME ) ;
         groupName = ele.str() ;

         ele = obj.getField( FIELD_NAME_SECRETID ) ;
         PD_CHECK( ele.eoo() || NumberInt == ele.type(), SDB_INVALIDARG, error,
                   PDWARNING, "Failed to parse [%s]", FIELD_NAME_SECRETID ) ;
         if ( NULL != pSecID )
         {
            if ( ele.eoo() )
            {
               *pSecID = 0 ;
            }
            else
            {
               *pSecID = ele.Int() ;
            }
         }

         ele = obj.getField( CAT_ROLE_NAME ) ;
         PD_CHECK( NumberInt == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse [%s]", CAT_ROLE_NAME ) ;

         ele = obj.getField( CAT_VERSION_NAME ) ;
         PD_CHECK( NumberInt == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse [%s]", CAT_VERSION_NAME ) ;
         version = ele.Int() ;

         if ( pPrimary )
         {
            ele = obj.getField ( CAT_PRIMARY_NAME ) ;
            PD_CHECK( ele.eoo() || NumberInt == ele.type(), SDB_INVALIDARG,
                      error, PDWARNING, "Failed to parse [%s]",
                      CAT_PRIMARY_NAME ) ;
            if ( ele.eoo() )
            {
               *pPrimary = 0 ;
            }
            else
            {
               *pPrimary = ele.Int() ;
            }
         }

         ele = obj.getField( CAT_GROUP_NAME ) ;
         PD_CHECK( ele.eoo() || Array == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse [%s]", CAT_GROUP_NAME ) ;

         if ( Array == ele.type() )
         {
            BSONObjIterator i( ele.embeddedObject() ) ;
            while ( i.more() )
            {
               _netRouteNode route ;

               BSONElement beNode = i.next() ;
               PD_CHECK( Object == beNode.type(), SDB_INVALIDARG, error,
                         PDWARNING, "Failed to parse [%s]", CAT_GROUP_NAME ) ;

               rc = _msgParseCatNodeObj( beNode.embeddedObject(), route ) ;
               PD_RC_CHECK( rc, PDWARNING, "Failed to parse node, rc: %d",
                            rc ) ;

               route._id.columns.groupID = groupID ;
               group.insert( make_pair( route._id.value,  route ) ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "unexpected exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC( SDB_MSGPASCATGRPOBJ, rc ) ;
      return rc ;

   error :
      rc = SDB_INVALIDARG ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GETSVCNAME, "getServiceName" )
   const CHAR* getServiceName ( const bson::BSONElement &beService,
                                INT32 serviceType )
   {
      PD_TRACE_ENTRY ( SDB_GETSVCNAME );
      const CHAR *pSvcName = "" ;

      try
      {
         if ( beService.type() != Array )
         {
            goto done ;
         }

         BSONObjIterator i( beService.embeddedObject() );
         while ( i.more() )
         {
            BSONElement beTmp = i.next();
            BSONObj boTmp = beTmp.embeddedObject();
            BSONElement beServiceType = boTmp.getField(
               CAT_SERVICE_TYPE_FIELD_NAME );
            if ( beServiceType.eoo() || !beServiceType.isNumber() )
            {
               continue ;
            }
            if ( beServiceType.numberInt() == serviceType )
            {
               BSONElement beServiceName = boTmp.getField(
                  CAT_SERVICE_NAME_FIELD_NAME );
               if ( beServiceName.type() != String )
               {
                  goto done ;
               }
               pSvcName = beServiceName.valuestr() ;
               goto done ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "unexpected exception: %s", e.what() ) ;
      }

   done :
      PD_TRACE1 ( SDB_GETSVCNAME, PD_PACK_STRING( pSvcName ) );
      PD_TRACE_EXIT ( SDB_GETSVCNAME );
      return pSvcName ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_GETSHDSVCNAME, "getShardServiceName" )
   const CHAR *getShardServiceName ( const BSONElement &beService )
   {
      PD_TRACE_ENTRY ( SDB_GETSHDSVCNAME ) ;

      const CHAR* pRet = getServiceName( beService,
                                         MSG_ROUTE_SHARD_SERVCIE ) ;
      if ( pRet && !*pRet )
      {
         pRet = NULL ;
      }

      PD_TRACE_EXIT ( SDB_GETSHDSVCNAME );
      return pRet ;
   }

}

