/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = clsRegAssit.cpp

   Descriptive Name = node register assistant

   When/how to use: this program may be used on binary and text-formatted
   versions of clsication component. This file contains structure for
   clsication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          10/10/2017  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsRegAssit.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"
#include "msgCatalog.hpp"
#include "pmdOptions.hpp"
#include "ossUtil.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "utilCommon.hpp"

using namespace bson ;

namespace engine
{
   _clsRegAssit::_clsRegAssit()
   :_groupID( INVALID_GROUPID ),
    _nodeID( INVALID_NODEID )
   {
      _hostName[ 0 ] = '\0' ;
   }

   _clsRegAssit::~_clsRegAssit()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREGASSIT_BUILDOBJ, "_clsRegAssit::buildRequestObj" )
   BSONObj _clsRegAssit::buildRequestObj ()
   {
      PD_TRACE_ENTRY ( SDB__CLSREGASSIT_BUILDOBJ );
      pmdKRCB *pKRCB = pmdGetKRCB () ;
      const CHAR* hostName = pKRCB->getHostName() ;
      BSONObj obj ;

      BSONObjBuilder bsonBuilder ;
      bsonBuilder.append ( CAT_TYPE_FIELD_NAME, (INT32)(pKRCB->getDBRole()) ) ;
      bsonBuilder.append ( CAT_HOST_FIELD_NAME, hostName ) ;
      bsonBuilder.append ( PMD_OPTION_DBPATH, pKRCB->getDBPath() ) ;

      if ( utilCheckInstanceID( pKRCB->getOptionCB()->getInstanceID(), FALSE ) )
      {
         bsonBuilder.append ( PMD_OPTION_INSTANCE_ID,
                              pKRCB->getOptionCB()->getInstanceID() ) ;
      }

      BSONArrayBuilder subServiceBuild( bsonBuilder.subarrayStart(
         CAT_SERVICE_FIELD_NAME ) ) ;

      /// local
      BSONObjBuilder subLocalBuild( subServiceBuild.subobjStart() ) ;
      subLocalBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                            (INT32)MSG_ROUTE_LOCAL_SERVICE ) ;
      subLocalBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                            pKRCB->getSvcname() ) ;
      subLocalBuild.done() ;

      /// repl
      BSONObjBuilder subReplBuild( subServiceBuild.subobjStart() ) ;
      subReplBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                            (INT32)MSG_ROUTE_REPL_SERVICE ) ;
      subReplBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                            pKRCB->getOptionCB()->replService() ) ;
      subReplBuild.done() ;

      /// shard
      BSONObjBuilder subShdBuild( subServiceBuild.subobjStart() ) ;
      subShdBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                           (INT32)MSG_ROUTE_SHARD_SERVCIE) ;
      subShdBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                           pKRCB->getOptionCB()->shardService() ) ;
      subShdBuild.done() ;

      /// cata
      BSONObjBuilder subCataBuild( subServiceBuild.subobjStart() ) ;
      subCataBuild.append ( CAT_SERVICE_TYPE_FIELD_NAME ,
                            (INT32)MSG_ROUTE_CAT_SERVICE ) ;
      subCataBuild.append ( CAT_SERVICE_NAME_FIELD_NAME,
                            pKRCB->getOptionCB()->catService() ) ;
      subCataBuild.done() ;

      subServiceBuild.done() ;

      // append IP address
      ossIPInfo ipInfo ;
      if ( ipInfo.getIPNum() > 0 )
      {
         BSONArrayBuilder subIPBuild( bsonBuilder.subarrayStart(
            CAT_IP_FIELD_NAME ) ) ;

         ossIP* ip = ipInfo.getIPs() ;
         for ( INT32 i = ipInfo.getIPNum(); i > 0; i-- )
         {
            // skip loopback IP
            if (0 != ossStrncmp( ip->ipAddr, OSS_LOOPBACK_IP,
                                 ossStrlen(OSS_LOOPBACK_IP)) )
            {
               subIPBuild.append( ip->ipAddr ) ;
            }
            ip++ ;
         }

         // support 'localhost' and '127.0.0.1' for node's hostname
         subIPBuild.append( OSS_LOOPBACK_IP ) ;
         subIPBuild.append( OSS_LOCALHOST ) ;
         subIPBuild.done() ;

         obj = bsonBuilder.obj () ;
      }

      PD_TRACE_EXIT ( SDB__CLSREGASSIT_BUILDOBJ );
      return obj ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSREGASSIT_EXTRACTRES, "_clsRegAssit::extractResponseMsg" )
   INT32 _clsRegAssit::extractResponseMsg ( MsgHeader *pMsg )
   {
      PD_TRACE_ENTRY ( SDB__CLSREGASSIT_EXTRACTRES );
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj object ( MSG_GET_INNER_REPLY_DATA( pMsg ) ) ;
         BSONElement gidEle  = object.getField ( CAT_GROUPID_NAME ) ;
         BSONElement nidEle  = object.getField ( CAT_NODEID_NAME ) ;
         BSONElement hostEle = object.getField ( CAT_HOST_FIELD_NAME ) ;

         if ( gidEle.type() != NumberInt )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Fail to extract field[%s] from register "
                     "response msg, rc: %d", CAT_GROUPID_NAME, rc ) ;
            goto error ;
         }
         if ( nidEle.type() != NumberInt )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Fail to extract field[%s] from register "
                     "response msg, rc: %d", CAT_NODEID_NAME, rc ) ;
            goto error ;
         }
         if ( hostEle.type() != String )
         {
            rc = SDB_SYS ;
            PD_LOG ( PDERROR, "Fail to extract field[%s] from register "
                     "response msg, rc: %d", CAT_HOST_FIELD_NAME, rc ) ;
            goto error ;
         }
         _groupID = (UINT32)gidEle.Int () ;
         _nodeID = (UINT16)nidEle.Int () ;
         ossStrncpy( _hostName, hostEle.valuestrsafe(),
                     OSS_MAX_HOSTNAME ) ;
         _hostName[ OSS_MAX_HOSTNAME ] = '\0' ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__CLSREGASSIT_EXTRACTRES, rc );
      return rc ;
   error :
      goto done ;
   }

   UINT32 _clsRegAssit::getGroupID ()
   {
      return _groupID ;
   }

   UINT16 _clsRegAssit::getNodeID ()
   {
      return _nodeID ;
   }

   const CHAR* _clsRegAssit::getHostname ()
   {
      return _hostName ;
   }
}
