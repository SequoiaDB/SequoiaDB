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

   Source File Name = omSdbAdaptor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "omSdbAdaptor.hpp"
#include "pmdOptions.h"
#include "omDef.hpp"
#include "msgDef.h"
#include "msgMessage.hpp"
#include "rtn.hpp"
#include "omSdbConnector.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _omSdbAdaptor implement
   */
   _omSdbAdaptor::_omSdbAdaptor()
   {
   }

   _omSdbAdaptor::~_omSdbAdaptor()
   {
   }

   INT32 _omSdbAdaptor::notifyStrategyChanged( const string &clsName,
                                               const string &bizName,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      omBizInfo bizInfo ;

      bizInfo._clsName = clsName ;
      bizInfo._bizName = bizName ;

      rc = getBizNodeInfo( bizInfo, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get business[%s] nodes failed, rc: %d",
                 bizName.c_str(), rc ) ;
         goto error ;
      }

      for ( UINT32 i = 0 ; i < bizInfo._vecNodes.size() ; ++i )
      {
         omSdbNodeInfo &nodeInfo = bizInfo._vecNodes[i] ;

         rcTmp = notifyStrategyChange2Node( &nodeInfo,
                                            bizInfo._userName,
                                            bizInfo._password,
                                            cb ) ;
         if ( rcTmp )
         {
            PD_LOG( PDWARNING, "Notify strategy change to node[%s:%s] "
                    "failed, rc: %d",
                    nodeInfo._host.c_str(), nodeInfo._svcname.c_str(),
                    rcTmp ) ;
            rc = rc ? rc : rcTmp ;
         }
      }

      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbAdaptor::getBizNodeInfo( omBizInfo &bizInfo,
                                        pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      BSONObj matcher ;
      BSONObj emptyObj ;
      INT64 contextID ;
      rtnContextBuf buffObj ;
      BSONObj record ;
      omSdbNodeInfo nodeInfo ;

      matcher = BSON( OM_BUSINESS_FIELD_NAME << bizInfo._bizName ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_CONFIGURE, emptyObj, matcher,
                     emptyObj, emptyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 OM_CS_DEPLOY_CL_CONFIGURE, rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         rc = rtnGetMore( contextID, -1, buffObj, cb, rtnCB ) ;
         if ( rc )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_LOG( PDERROR, "Getmore from context failed, rc: %d", rc ) ;
            goto error ;            
         }

         while( TRUE )
         {
            rc = buffObj.nextObj( record ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
               }
               break ;
            }

            rc = _parseCoordNodeInfo( record, nodeInfo ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Parse node info[%s] failed, rc: %d",
                       record.toString().c_str(), rc ) ;
               rc = SDB_OK ;
            }
            else
            {
               bizInfo._vecNodes.push_back( nodeInfo ) ;
            }
         }
      }

      rc = _fillAuthInfo( bizInfo, cb ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Fill node's auth info failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbAdaptor::_parseCoordNodeInfo( const BSONObj &obj,
                                             omSdbNodeInfo &nodeInfo )
   {
      INT32 rc = SDB_OK ;
      BSONElement beField ;

      try
      {
         beField = obj.getField( OM_HOST_FIELD_NAME ) ;
         if ( String != beField.type() )
         {
            PD_LOG( PDERROR, "Field[%s] must be string",
                    beField.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         nodeInfo._host = beField.str() ;

         beField = obj.getField( OM_CONFIGURE_FIELD_CONFIG ) ;
         if ( Array != beField.type() )
         {
            PD_LOG( PDERROR, "Field[%s] must be object array",
                    beField.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            BSONObj objSub ;
            BSONObjIterator iter( beField.embeddedObject() ) ;
            while( iter.more() )
            {
               BSONElement e = iter.next() ;
               if ( Object != e.type() )
               {
                  PD_LOG( PDERROR, "Field[%s] must be object array",
                          beField.toString( TRUE, TRUE ).c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               objSub = e.embeddedObject() ;

               e = objSub.getField( OM_CONF_DETAIL_ROLE ) ;
               if ( String != e.type() ||
                    0 != ossStrcasecmp( e.valuestr(), SDB_ROLE_COORD_STR ) )
               {
                  continue ;
               }
               e = objSub.getField( OM_CONF_DETAIL_SVCNAME ) ;
               if ( String != e.type() )
               {
                  PD_LOG( PDERROR, "Field[%s] must be string",
                          e.toString( TRUE, TRUE ).c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               nodeInfo._svcname = e.str() ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbAdaptor::_fillAuthInfo( omBizInfo &bizInfo,
                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      SDB_DMSCB *dmsCB = krcb->getDMSCB() ;
      SDB_RTNCB *rtnCB = krcb->getRTNCB() ;
      BSONObj matcher ;
      BSONObj emptyObj ;
      INT64 contextID ;
      rtnContextBuf buffObj ;
      BSONObj record ;

      matcher = BSON( OM_BUSINESS_FIELD_NAME << bizInfo._bizName ) ;

      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS_AUTH, emptyObj, matcher,
                     emptyObj, emptyObj, 0, cb, 0, -1, dmsCB, rtnCB,
                     contextID ) ;
      if ( rc )
      {
         if ( SDB_DMS_NOTEXIST == rc )
         {
            rc = SDB_OK ;
            goto done ;
         }
         PD_LOG( PDERROR, "Query on collection[%s] failed, rc: %d",
                 OM_CS_DEPLOY_CL_BUSINESS_AUTH, rc ) ;
         goto error ;
      }

      while( TRUE )
      {
         rc = rtnGetMore( contextID, -1, buffObj, cb, rtnCB ) ;
         if ( rc )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            PD_LOG( PDERROR, "Getmore from context failed, rc: %d", rc ) ;
            goto error ;            
         }

         while( TRUE )
         {
            rc = buffObj.nextObj( record ) ;
            if ( rc )
            {
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
               }
               break ;
            }

            rc = _parseAuthInfo( record, bizInfo._userName,
                                 bizInfo._password ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Parse auth info[%s] failed, rc: %d",
                       record.toString().c_str(), rc ) ;
               rc = SDB_OK ;
            }
            else
            {
               break ;
            }
         }
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbAdaptor::_parseAuthInfo( const BSONObj &obj,
                                        string &userName,
                                        string &password )
   {
      INT32 rc = SDB_OK ;
      BSONElement beField ;

      try
      {
         beField = obj.getField( OM_HOST_FIELD_USER ) ;
         if ( String != beField.type() )
         {
            PD_LOG( PDERROR, "Field[%s] must be string",
                    beField.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         userName = beField.str() ;

         beField = obj.getField( OM_HOST_FIELD_PASSWORD ) ;
         if ( String != beField.type() )
         {
            PD_LOG( PDERROR, "Field[%s] must be string",
                    beField.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         password = beField.str() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _omSdbAdaptor::notifyStrategyChange2Node( const omSdbNodeInfo *nodeInfo,
                                                   const string &userName,
                                                   const string &password,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT16 port = 0 ;
      omSdbConnector connector ;

      BSONObj emptyObj ;
      BSONObj options ;
      CHAR *pMsgBuff = NULL ;
      INT32 buffSize = 0 ;
      MsgHeader *pReply = NULL ;
      MsgOpReply *pReplyMsg = NULL ;

      BSONObjBuilder builder ;
      builder.appendBool( FIELD_NAME_GLOBAL, FALSE ) ;
      options = builder.obj() ;

      rc = msgBuildQueryMsg( &pMsgBuff, &buffSize,
                             CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_CACHE,
                             0, 0, 0, -1, &options, &emptyObj,
                             &emptyObj, &emptyObj, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Build query message failed, rc: %d", rc ) ;
         goto error ;
      }

      rc = ossGetPort( nodeInfo->_svcname.c_str(), port ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Get port from svcname[%s] failed, rc: %d",
                 nodeInfo->_svcname.c_str(), rc ) ;
         goto error ;
      }

      rc = connector.init( nodeInfo->_host, port, userName,
                           password, PREFER_REPL_ANYONE ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Connect to node[%s:%d] failed, rc: %d",
                 nodeInfo->_host.c_str(), port, rc ) ;
         goto error ;
      }

      rc = connector.sendMessage( (const MsgHeader*)pMsgBuff ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Send message to node[%s:%s] failed, rc: %d",
                 nodeInfo->_host.c_str(), nodeInfo->_svcname.c_str(),
                 rc ) ;
         goto error ;
      }

      rc = connector.recvMessage( &pReply ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Recieve response from node[%s:%s] failed, rc: %d",
                 nodeInfo->_host.c_str(), nodeInfo->_svcname.c_str(),
                 rc ) ;
         goto error ;
      }

      pReplyMsg = ( MsgOpReply* )pReply ;
      if ( SDB_OK != pReplyMsg->flags )
      {
         rc = pReplyMsg->flags ;
         PD_LOG( PDERROR, "Invalidate cache on node[%s:%s] failed, rc: %d",
                 nodeInfo->_host.c_str(), nodeInfo->_svcname.c_str(),
                 rc ) ;
         goto error ;
      }

   done:
      if ( pMsgBuff )
      {
         msgReleaseBuffer( pMsgBuff, cb ) ;
      }
      if ( pReply )
      {
         SDB_OSS_FREE( (CHAR*)pReply ) ;
      }
      return rc ;
   error:
      goto done ;
   }

}

