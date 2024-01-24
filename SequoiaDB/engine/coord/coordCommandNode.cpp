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

   Source File Name = coordCommandNode.cpp

   Descriptive Name = Runtime Coord Commands for Node Management

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandNode.hpp"
#include "rtn.hpp"
#include "msgCatalog.hpp"
#include "pmdOptionsMgr.hpp"
#include "pmd.hpp"
#include "coordUtil.hpp"
#include "msgMessage.hpp"
#include "rtnRemoteExec.hpp"
#include "omagentDef.hpp"
#include "msgReplicator.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "utilCommon.hpp"
#include "coordCMDEventHandler.hpp"
#include "coordRemoteSession.hpp"
#include "coordCacheAssist.hpp"

using namespace bson;

namespace engine
{
   /*
      _coordNodeCMD2Phase implement
   */
   _coordNodeCMD2Phase::_coordNodeCMD2Phase()
   {
   }

   _coordNodeCMD2Phase::~_coordNodeCMD2Phase()
   {
   }

   INT32 _coordNodeCMD2Phase::_generateDataMsg ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 const vector<BSONObj> &cataObjs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;
      return SDB_OK ;
   }

   INT32 _coordNodeCMD2Phase::_generateRollbackDataMsg ( MsgHeader *pMsg,
                                                         pmdEDUCB *cb,
                                                         coordCMDArguments *pArgs,
                                                         CHAR **ppMsgBuf,
                                                         INT32 *pBufSize )
   {
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;
      return SDB_OK ;
   }

   void _coordNodeCMD2Phase::_releaseRollbackDataMsg( CHAR *pMsgBuf,
                                                      INT32 bufSize,
                                                      pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODECMD2PHASE_DOAUDIT, "_coordNodeCMD2Phase::_doAudit" )
   INT32 _coordNodeCMD2Phase::_doAudit ( coordCMDArguments *pArgs, INT32 rc )
   {
      PD_TRACE_ENTRY ( COORD_NODECMD2PHASE_DOAUDIT ) ;

      if ( !pArgs->_targetName.empty() )
      {
         PD_AUDIT_COMMAND( AUDIT_CLUSTER, getName(),
                           _getAuditObjectType(),
                           _getAuditObjectName( pArgs ).c_str(),
                           rc, _getAuditDesp( pArgs ).c_str() ) ;
      }

      PD_TRACE_EXIT ( COORD_NODECMD2PHASE_DOAUDIT );

      return SDB_OK ;
   }

   AUDIT_OBJ_TYPE _coordNodeCMD2Phase::_getAuditObjectType() const
   {
      return AUDIT_OBJ_GROUP ;
   }

   string _coordNodeCMD2Phase::_getAuditObjectName( coordCMDArguments *pArgs ) const
   {
      return pArgs->_targetName ;
   }

   string _coordNodeCMD2Phase::_getAuditDesp( coordCMDArguments *pArgs ) const
   {
      return "" ;
   }

   /*
    * _coordNodeCMD3Phase implement
    */
    _coordNodeCMD3Phase::_coordNodeCMD3Phase()
   {
   }

   _coordNodeCMD3Phase::~_coordNodeCMD3Phase()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_NODE3PHASE_DOONCATAP2, "_coordNodeCMD3Phase::_doOnCataGroupP2" )
   INT32 _coordNodeCMD3Phase::_doOnCataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord::sharePtr *ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &pGroupLst,
                                                 vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_NODE3PHASE_DOONCATAP2 ) ;

      rtnContextBuf buffObj ;

      rc = _processContext( cb, ppContext, 1, buffObj ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to process context, rc: %d", rc ) ;

      try
      {
         while ( !buffObj.eof() )
         {
            BSONObj reply ;
            rc = buffObj.nextObj( reply ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get obj from obj buf, rc: %d",
                         rc ) ;
            cataObjs.push_back( reply.getOwned() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get reply object, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_NODE3PHASE_DOONCATAP2, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDOperateOnNode implement
   */
   _coordCMDOperateOnNode::_coordCMDOperateOnNode()
   {
   }

   _coordCMDOperateOnNode::~_coordCMDOperateOnNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_OPRONNODE_EXE, "_coordCMDOperateOnNode::execute" )
   INT32 _coordCMDOperateOnNode::execute( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          INT64 &contextID,
                                          rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMD_OPRONNODE_EXE ) ;

      rtnQueryOptions queryOption ;
      const CHAR *strHostName = NULL ;
      const CHAR *svcname = NULL ;

      BSONObj boNodeConf ;
      BSONObjBuilder bobNodeConf ;

      SINT32 opType = getOpType() ;
      SINT32 retCode = SDB_OK ;

      // fill default-reply(active group success)
      contextID                        = -1 ;

      rc = queryOption.fromQueryMsg( (CHAR *)pMsg ) ;
      PD_RC_CHECK( rc, PDERROR, "Extract msg failed, rc: %d", rc ) ;

      try
      {
         BSONObj objQuery = queryOption.getQuery() ;
         BSONElement ele = objQuery.getField( FIELD_NAME_HOST ) ;
         if ( ele.eoo() || ele.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Field[%s] is invalid[%s]",
                     FIELD_NAME_HOST, objQuery.toString().c_str() ) ;
            goto error ;
         }
         strHostName = ele.valuestrsafe () ;

         ele = objQuery.getField( PMD_OPTION_SVCNAME ) ;
         if ( ele.eoo() || ele.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG ( PDERROR, "Field[%s] is invalid[%s]",
                     PMD_OPTION_SVCNAME, objQuery.toString().c_str() ) ;
            goto error ;
         }
         svcname = ele.valuestrsafe() ;

         bobNodeConf.append( ele ) ;
         boNodeConf = bobNodeConf.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "occured unexpected error:%s",
                  e.what() );
         goto error ;
      }

      /// execute on node
      rc = rtnRemoteExec ( opType, strHostName,
                           &retCode, &boNodeConf ) ;
      rc = rc ? rc : retCode ;
      if ( rc != SDB_OK )
      {
         PD_LOG( PDERROR, "Do remote execute[%d] on node[%s:%s] failed, "
                 "rc: %d", opType, strHostName, svcname, rc ) ;
         goto error ;
      }

   done:
      if ( strHostName && svcname )
      {
         PD_AUDIT_COMMAND( AUDIT_SYSTEM, queryOption.getCLFullName() + 1,
                           AUDIT_OBJ_NODE, "", rc,
                           "HostName:%s, ServiceName:%s", strHostName,
                           svcname ) ;
      }
      PD_TRACE_EXITRC ( COORD_CMD_OPRONNODE_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDStartupNode implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDStartupNode,
                                      CMD_NAME_STARTUP_NODE,
                                      TRUE ) ;
   _coordCMDStartupNode::_coordCMDStartupNode()
   {
   }

   _coordCMDStartupNode::~_coordCMDStartupNode()
   {
   }

   SINT32 _coordCMDStartupNode::getOpType() const
   {
      return SDBSTART ;
   }

   /*
      _coordCMDShutdownNode implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDShutdownNode,
                                      CMD_NAME_SHUTDOWN_NODE,
                                      TRUE ) ;
   _coordCMDShutdownNode::_coordCMDShutdownNode()
   {
   }

   _coordCMDShutdownNode::~_coordCMDShutdownNode()
   {
   }

   SINT32 _coordCMDShutdownNode::getOpType() const
   {
      return SDBSTOP ;
   }

   /*
      _coordCMDOpOnGroup implement
   */
   _coordCMDOpOnGroup::_coordCMDOpOnGroup()
   {
   }

   _coordCMDOpOnGroup::~_coordCMDOpOnGroup()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMDOPONGROUP_OPON1NODE, "_coordCMDOpOnGroup::_opOnOneNode" )
   INT32 _coordCMDOpOnGroup::_opOnOneNode ( const vector<INT32> &opList,
                                            string hostName,
                                            string svcName,
                                            vector<BSONObj> &dataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMDOPONGROUP_OPON1NODE ) ;

      try
      {
         BSONObj boExecArg = BSON( FIELD_NAME_HOST << hostName <<
                                   PMD_OPTION_SVCNAME << svcName ) ;

         for ( vector<INT32>::const_iterator iter = opList.begin() ;
               iter != opList.end() ;
               ++iter )
         {
            INT32 retCode = SDB_OK ;
            CM_REMOTE_OP_CODE opCode = (CM_REMOTE_OP_CODE)(*iter) ;

            rc = rtnRemoteExec( opCode, hostName.c_str(),
                                &retCode, &boExecArg ) ;
            if ( SDB_OK == rc && SDB_OK == retCode )
            {
               continue ;
            }
            else
            {
               PD_LOG( PDERROR, "Do remote execute[code:%d] on the node[%s:%s] "
                       "failed, rc: %d, remoteRC: %d",opCode, hostName.c_str(),
                       svcName.c_str(), rc, retCode ) ;
               dataObjs.push_back( BSON( FIELD_NAME_HOST << hostName <<
                                         PMD_OPTION_SVCNAME << svcName <<
                                         OP_ERRNOFIELD <<
                                         ( ( SDB_OK == retCode ) ? rc : retCode )  ) ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occurred unexpected error:%s", e.what() ) ;
      }

      PD_TRACE_EXITRC ( COORD_CMDOPONGROUP_OPON1NODE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMDOPONGROUP_OPONNODES, "_coordCMDOpOnGroup::_opOnNodes" )
   INT32 _coordCMDOpOnGroup::_opOnNodes ( const vector<INT32> &opList,
                                          const BSONObj &boGroupInfo,
                                          vector<BSONObj> &dataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMDOPONGROUP_OPONNODES ) ;

      BOOLEAN skipSelf = FALSE ;
      const CHAR* selfHostName = pmdGetKRCB()->getHostName() ;
      const CHAR* selfSvcName  = pmdGetKRCB()->getSvcname() ;

      try
      {
         BSONObj boNodeList ;
         rc = rtnGetArrayElement( boGroupInfo, FIELD_NAME_GROUP,
                                  boNodeList ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get feild[%s] failed, rc: %d",
                    FIELD_NAME_GROUP, rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         BSONObjIterator i( boNodeList ) ;
         while ( i.more() )
         {
            BSONElement beNode = i.next() ;
            BSONObj boNode = beNode.embeddedObject() ;
            string hostName, svcName ;
            BSONElement beService ;

            /// get hostname and svcname of the node
            rc = rtnGetSTDStringElement( boNode, FIELD_NAME_HOST, hostName ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Get field[%s] failed, rc: %d",
                       FIELD_NAME_HOST, rc ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            beService = boNode.getField( FIELD_NAME_SERVICE ) ;
            if ( Array != beService.type() )
            {
               PD_LOG( PDERROR, "Failed to get the field[%s] from obj[%s]",
                       FIELD_NAME_SERVICE, boNode.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            svcName = getServiceName( beService, MSG_ROUTE_LOCAL_SERVICE );
            if ( svcName.empty() )
            {
               PD_LOG( PDERROR, "Failed to get service name from obj[%s]",
                       beService.toString().c_str() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            /// skip stopping myself
            /// if want to stop itself, we must stop all other first.
            vector<INT32>::const_iterator it = std::find( opList.begin(),
                                                          opList.end(),
                                                          SDBSTOP ) ;
            if( it != opList.end() )
            {
               if (  0 == ossStrcmp( hostName.c_str(), selfHostName ) &&
                     0 == ossStrcmp( svcName.c_str(),  selfSvcName ) )
               {
                  skipSelf = TRUE ;
                  continue ;
               }
            }

            /// do operation
            rc = _opOnOneNode( opList, hostName, svcName, dataObjs ) ;
         }

         /// stop itself after all other node stoped
         if ( skipSelf )
         {
            rc = _opOnOneNode( opList, selfHostName, selfSvcName, dataObjs ) ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occurred unexpected error:%s", e.what() ) ;
         goto error ;
      }

      if ( dataObjs.size() > 0 )
      {
         rc = _flagIsStartNodes() ? SDB_CM_RUN_NODE_FAILED :
                                    SDB_CM_OP_NODE_FAILED ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CMDOPONGROUP_OPONNODES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMDOPONGROUP_OPONCATANODES, "_coordCMDOpOnGroup::_opOnCataNodes" )
   INT32 _coordCMDOpOnGroup::_opOnCataNodes ( const vector<INT32> &opList,
                                              clsGroupItem *pItem,
                                              vector<BSONObj> &dataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMDOPONGROUP_OPONCATANODES ) ;

      MsgRouteID id ;
      string hostName ;
      string svcName ;
      UINT32 pos = 0 ;

      while ( SDB_OK == pItem->getNodeInfo( pos, id, hostName, svcName,
                                            MSG_ROUTE_LOCAL_SERVICE ) )
      {
         ++pos ;
         rc = _opOnOneNode( opList, hostName, svcName, dataObjs ) ;
      }

      if ( dataObjs.size() > 0 )
      {
         rc = _flagIsStartNodes() ? SDB_CM_RUN_NODE_FAILED :
                                    SDB_CM_OP_NODE_FAILED ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CMDOPONGROUP_OPONCATANODES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMDOPONGROUP_OPONVECNODES, "_coordCMDOpOnGroup::_opOnVecNodes" )
   INT32 _coordCMDOpOnGroup::_opOnVecNodes( const vector< INT32 > &opList,
                                            const CoordVecNodeInfo &vecNodes,
                                            vector< BSONObj > &dataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CMDOPONGROUP_OPONVECNODES ) ;

      string svcName ;
      UINT32 pos = 0 ;

      while ( pos < vecNodes.size() )
      {
         const clsNodeItem &item = vecNodes[ pos ] ;
         ++pos ;

         // get local service
         UINT16 cataPort = 0 ;
         UINT16 svcPort = 0 ;
         char svcChar[10] = { 0 } ;
         string cataName = item._service[ MSG_ROUTE_CAT_SERVICE ] ;
         rc = ossGetPort( cataName.c_str(), cataPort ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Convert service[%s] to port failed, rc: %d",
                    cataName.c_str(), rc ) ;
            goto error ;
         }
         svcPort = cataPort - MSG_ROUTE_CAT_SERVICE ;
         ossSnprintf( svcChar, sizeof( svcChar ), "%u", svcPort ) ;
         svcName = svcChar ;

         // do operation
         rc = _opOnOneNode( opList, item._host, svcName, dataObjs ) ;
      }

      if ( dataObjs.size() > 0 )
      {
         rc = _flagIsStartNodes() ? SDB_CM_RUN_NODE_FAILED :
                                    SDB_CM_OP_NODE_FAILED ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CMDOPONGROUP_OPONVECNODES, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   void _coordCMDOpOnGroup::_logErrorForNodes ( const CHAR *pCmdName,
                                                const string &targetName,
                                                const vector<BSONObj> &dataObjs )
   {
      UINT32 i = 0;
      string strNodeList ;

      for ( ; i < dataObjs.size(); i++ )
      {
         strNodeList += dataObjs[i].toString() ;
      }

      PD_LOG_MSG( PDERROR, "Do command[%s, targe:%s] failed, detail: %s",
                  pCmdName, targetName.c_str(), strNodeList.c_str() ) ;
   }

   BOOLEAN _coordCMDOpOnGroup::_flagIsStartNodes() const
   {
      return FALSE ;
   }

   /*
      _coordCMDConfigNode implement
   */
   _coordCMDConfigNode::_coordCMDConfigNode()
   {
   }

   _coordCMDConfigNode::~_coordCMDConfigNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMDCONFIGNODE_GETNODECFG, "_coordCMDConfigNode::getNodeConf" )
   INT32 _coordCMDConfigNode::getNodeConf( coordResource *pResource,
                                           const CHAR *pQuery,
                                           BSONObj &nodeConf )
   {
      INT32 rc             = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CMDCONFIGNODE_GETNODECFG ) ;

      pmdOptionsCB *optCB = pmdGetKRCB()->getOptionCB() ;
      const CHAR *roleStr  = NULL ;
      SDB_ROLE role        = SDB_ROLE_DATA ;

      if ( !pResource || !pQuery )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         // role and catalogaddr will be added if user doesn't provide
         BSONObj boInput( pQuery ) ;
         BSONObjBuilder bobNodeConf( boInput.objsize() + 128 ) ;
         BSONObjIterator iter( boInput ) ;

         BOOLEAN hasCatalogAddrKey = FALSE ;
         BOOLEAN hasClusterName = FALSE ;
         BOOLEAN hasBusinessName = FALSE ;

         const CHAR *pFieldName = NULL ;

         // loop through each input parameter
         while ( iter.more() )
         {
            BSONElement beField = iter.next() ;
            pFieldName = beField.fieldName() ;

            BOOLEAN ignored = FALSE ;
            rc = _onParseConfig( beField, pFieldName, ignored ) ;
            if ( rc )
            {
               goto error ;
            }
            else if ( ignored )
            {
               continue ;
            }

            // make sure to skip hostname and group name
            if ( 0 == ossStrcmp( pFieldName, FIELD_NAME_HOST ) ||
                 0 == ossStrcmp( pFieldName, PMD_OPTION_ROLE ) )
            {
               continue ;
            }
            else if ( 0 == ossStrcmp( pFieldName, PMD_OPTION_CLUSTER_NAME ) )
            {
               PD_CHECK( String == beField.type(), SDB_INVALIDARG, error,
                         PDERROR, "Field[%s] is invalid in obj[%s]",
                         PMD_OPTION_CLUSTER_NAME,
                         boInput.toString().c_str() ) ;
               hasClusterName = TRUE ;
            }
            else if ( 0 == ossStrcmp( pFieldName, PMD_OPTION_BUSINESS_NAME ) )
            {
               PD_CHECK( String == beField.type(), SDB_INVALIDARG, error,
                         PDERROR, "Field[%s] is invalid in obj[%s]",
                         PMD_OPTION_BUSINESS_NAME,
                         boInput.toString().c_str() ) ;
               hasBusinessName = TRUE ;
            }
            else if ( 0 == ossStrcmp( pFieldName, PMD_OPTION_CATALOG_ADDR ) )
            {
               PD_CHECK( String == beField.type(), SDB_INVALIDARG, error,
                         PDERROR, "Field[%s] is invalid in obj[%s]",
                         PMD_OPTION_CATALOG_ADDR,
                         boInput.toString().c_str() ) ;
               hasCatalogAddrKey = TRUE ;
            }
            else if ( 0 == ossStrcmp( pFieldName, FIELD_NAME_GROUPNAME ) )
            {
               PD_CHECK( String == beField.type(), SDB_INVALIDARG, error,
                         PDERROR, "Field[%s] is invalid in obj[%s]",
                         FIELD_NAME_GROUPNAME, boInput.toString().c_str() ) ;

               if ( 0 == ossStrcmp( CATALOG_GROUPNAME,
                                    beField.valuestr() ) )
               {
                  role = SDB_ROLE_CATALOG ;
               }
               else if ( 0 == ossStrcmp( COORD_GROUPNAME,
                                         beField.valuestr() ) )
               {
                  role = SDB_ROLE_COORD ;
               }
               continue ;
            }

            // append into beField
            bobNodeConf.append( beField );
         } /// end while

         // assign role if it doesn't include
         roleStr = utilDBRoleStr( role ) ;
         if ( *roleStr == 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         /// cluster name
         if ( !hasClusterName )
         {
            string strName ;
            optCB->getFieldStr( PMD_OPTION_CLUSTER_NAME, strName, "" ) ;
            if ( !strName.empty() )
            {
               bobNodeConf.append( PMD_OPTION_CLUSTER_NAME, strName ) ;
            }
         }

         /// business name
         if ( !hasBusinessName )
         {
            string strName ;
            optCB->getFieldStr( PMD_OPTION_BUSINESS_NAME, strName, "" ) ;
            if ( !strName.empty() )
            {
               bobNodeConf.append( PMD_OPTION_BUSINESS_NAME, strName ) ;
            }
         }

         rc = _onPostBuildConfig( pResource, bobNodeConf, roleStr,
                                  hasCatalogAddrKey ) ;
         if ( rc )
         {
            goto error ;
         }
         nodeConf = bobNodeConf.obj() ;
      } /// end try
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() );
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CMDCONFIGNODE_GETNODECFG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDConfigNode::_onParseConfig( const BSONElement &ele,
                                              const CHAR *pFieldName,
                                              BOOLEAN &ignored )
   {
      ignored = FALSE ;
      return SDB_OK ;
   }

   INT32 _coordCMDConfigNode::_onPostBuildConfig( coordResource *pResource,
                                                  BSONObjBuilder &builder,
                                                  const CHAR *roleStr,
                                                  BOOLEAN hasCataAddr )
   {
      INT32 rc = SDB_OK ;

      /// role string
      builder.append ( PMD_OPTION_ROLE, roleStr ) ;

      // assign catalog address, make sure to include all catalog nodes
      // that configured in the system ( for HA ), each system should be
      // separated by "," and sit in a single key: PMD_OPTION_CATALOG_ADDR
      if ( !hasCataAddr )
      {
         CoordGroupInfoPtr cataGroupPtr = pResource->getCataGroupInfo() ;
         MsgRouteID routeID ;
         string cataNodeLst ;
         string host ;
         string service ;
         UINT32 i = 0 ;

         if ( cataGroupPtr->nodeCount() == 0 )
         {
            rc = SDB_CLS_EMPTY_GROUP ;
            PD_LOG ( PDERROR, "Get catalog group info failed, rc: %d",
                     rc ) ;
            goto error ;
         }

         routeID.value = MSG_INVALID_ROUTEID ;
         while ( SDB_OK == cataGroupPtr->getNodeInfo( i, routeID, host,
                                                      service,
                                                      MSG_ROUTE_CAT_SERVICE ) )
         {
            if ( i > 0 )
            {
               cataNodeLst += "," ;
            }
            cataNodeLst += host + ":" + service ;
            ++i ;
         }
         builder.append( PMD_OPTION_CATALOG_ADDR, cataNodeLst ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_CFGNODE_GETNODEINFO, "_coordCMDConfigNode::getNodeInfo" )
   INT32 _coordCMDConfigNode::getNodeInfo( const CHAR *pQuery,
                                           BSONObj &nodeInfo )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMD_CFGNODE_GETNODEINFO ) ;

      try
      {
         BSONObj boConfig( pQuery ) ;
         BSONObjBuilder bobNodeInfo( boConfig.objsize() ) ;
         BSONElement ele ;

         if ( !_ignoreGroupNameWhenNodeInfo() )
         {
            ele = boConfig.getField( FIELD_NAME_GROUPNAME ) ;
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG ( PDERROR, "Failed to get the field[%s]",
                        FIELD_NAME_GROUPNAME ) ;
               goto error ;
            }
            bobNodeInfo.append( ele ) ;
         }

         ele = boConfig.getField( FIELD_NAME_HOST ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Failed to get the field[%s]",
                     FIELD_NAME_HOST );
            goto error ;
         }
         bobNodeInfo.append( ele ) ;

         ele = boConfig.getField( PMD_OPTION_SVCNAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Failed to get the field[%s]",
                     PMD_OPTION_SVCNAME ) ;
            goto error ;
         }
         bobNodeInfo.append( ele ) ;

         ele = boConfig.getField( PMD_OPTION_REPLNAME ) ;
         if ( String == ele.type() )
         {
            bobNodeInfo.append( ele ) ;
         }

         ele = boConfig.getField( PMD_OPTION_SHARDNAME ) ;
         if ( String == ele.type() )
         {
            bobNodeInfo.append( ele ) ;
         }

         ele = boConfig.getField( PMD_OPTION_CATANAME ) ;
         if ( String == ele.type() )
         {
            bobNodeInfo.append( ele ) ;
         }

         ele = boConfig.getField( PMD_OPTION_DBPATH ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG;
            PD_LOG ( PDERROR, "Failed to get the field[%s]",
                     PMD_OPTION_DBPATH ) ;
            goto error ;
         }
         bobNodeInfo.append( ele ) ;

         nodeInfo = bobNodeInfo.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR, "Occured unexpected error:%s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_CMD_CFGNODE_GETNODEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordCMDConfigNode::_ignoreGroupNameWhenNodeInfo() const
   {
      return FALSE ;
   }

   /*
      _coordCMDUpdateNode implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDUpdateNode,
                                      CMD_NAME_UPDATE_NODE,
                                      FALSE ) ;
   _coordCMDUpdateNode::_coordCMDUpdateNode()
   {
   }

   _coordCMDUpdateNode::~_coordCMDUpdateNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CMD_UPDATENODE_EXE, "_coordCMDUpdateNode::execute" )
   INT32 _coordCMDUpdateNode::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CMD_UPDATENODE_EXE ) ;

      // fill default-reply(create group success)
      contextID = -1 ;

      // TODO:
      // 1. first modify by the host's cm
      // 2. then send to catalog to update dbpath or other info

      rc = SDB_COORD_UNKNOWN_OP_REQ ;

      PD_TRACE_EXITRC ( COORD_CMD_UPDATENODE_EXE, rc ) ;
      return rc ;
   }

   /*
      _coordCMDCreateCataGroup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateCataGroup,
                                     CMD_NAME_CREATE_CATA_GROUP,
                                     FALSE ) ;
   _coordCMDCreateCataGroup::_coordCMDCreateCataGroup()
   {
      _pHostName = NULL ;
      _pSvcName = NULL ;
   }

   _coordCMDCreateCataGroup::~_coordCMDCreateCataGroup()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CREATECATAGRP_EXE, "_coordCMDCreateCataGroup::execute" )
   INT32 _coordCMDCreateCataGroup::execute( MsgHeader *pMsg,
                                            pmdEDUCB *cb,
                                            INT64 &contextID,
                                            rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( COORD_CREATECATAGRP_EXE ) ;

      contextID = -1 ;

      const CHAR *pQuery = NULL ;
      BSONObj boNodeConfig ;
      BSONObj boNodeInfo ;

      SINT32 retCode = 0 ;
      BSONObj boBackup = BSON( "Backup" << true ) ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL,
                            NULL, &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message, rc: %d", rc ) ;

      rc = getNodeConf( _pResource, pQuery, boNodeConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node config, rc: %d", rc ) ;

      rc = getNodeInfo( pQuery, boNodeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node info, rc: %d", rc ) ;

      /// make sure is no catalog addr
      if ( !_pResource->addCataNodeAddrWhenEmpty( _pHostName,
                                                  _cataSvcName.c_str() ) )
      {
         rc = SDB_COORD_RECREATE_CATALOG ;
         goto error ;
      }

      // Create
      rc = rtnRemoteExec( SDBADD, _pHostName, &retCode,
                          &boNodeConfig, &boNodeInfo ) ;
      rc = rc ? rc : retCode ;
      PD_RC_CHECK( rc, PDERROR, "Do remote execute on node[%s:%s] "
                   "failed, rc: %d", _pHostName, _pSvcName, rc ) ;

      // Start
      rc = rtnRemoteExec( SDBSTART, _pHostName, &retCode, &boNodeConfig ) ;
      rc = rc ? rc : retCode ;
      // if start catalog failed, need to remove config
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Do remote execute on node[%s:%s] "
                 "failed, rc: %d", _pHostName, _pSvcName, rc ) ;
         // boBackup for remove node to backup node info
         rtnRemoteExec( SDBRM, _pHostName, &retCode,
                        &boNodeConfig, &boBackup ) ;
         goto error ;
      }

      /// update the config file
      _pResource->syncAddress2Options( TRUE, FALSE ) ;

   done:
      if ( _pHostName && _pSvcName )
      {
         PD_AUDIT_COMMAND( AUDIT_CLUSTER, getName(),
                           AUDIT_OBJ_GROUP, CATALOG_GROUPNAME, rc,
                           "HostName:%s, ServiceName:%s", _pHostName,
                           _pSvcName ) ;
      }
      PD_TRACE_EXITRC ( COORD_CREATECATAGRP_EXE, rc ) ;
      return rc ;
   error:
      // clear the catalog-group info
      if ( rc != SDB_COORD_RECREATE_CATALOG )
      {
         _pResource->clearCataNodeAddrList() ;
      }
      goto done ;
   }

   INT32 _coordCMDCreateCataGroup::_onParseConfig( const BSONElement &ele,
                                                   const CHAR *pFieldName,
                                                   BOOLEAN &ignored )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( pFieldName, FIELD_NAME_HOST ) )
      {
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error,
                   PDERROR, "Field[%s] is invalid",
                   ele.toString( true, true ).c_str() ) ;
         _pHostName = ele.valuestr() ;
      }
      else if ( 0 == ossStrcmp( pFieldName, PMD_OPTION_SVCNAME ) )
      {
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error,
                   PDERROR, "Field[%s] is invalid",
                   ele.toString( true, true ).c_str() ) ;
         _pSvcName = ele.valuestr() ;
      }
      else if ( 0 == ossStrcmp( pFieldName, PMD_OPTION_CATANAME ) )
      {
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error,
                   PDERROR, "Field[%s] is invalid",
                   ele.toString( true, true ).c_str() ) ;
         _cataSvcName = ele.valuestr() ;
      }
      else if ( 0 == ossStrcmp( pFieldName, PMD_OPTION_CATALOG_ADDR ) )
      {
         ignored = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDCreateCataGroup::_onPostBuildConfig( coordResource *pResource,
                                                       BSONObjBuilder &builder,
                                                       const CHAR *roleStr,
                                                       BOOLEAN hasCataAddr )
   {
      INT32 rc = SDB_OK ;
      string cataAddr ;

      /// role
      builder.append( PMD_OPTION_ROLE, SDB_ROLE_CATALOG_STR ) ;

      if ( !_pHostName || !(*_pHostName) )
      {
         PD_LOG( PDERROR, "Host name is invalid in parameter" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( !_pSvcName || !(*_pSvcName) )
      {
         PD_LOG( PDERROR, "svcname is invalid in parameter" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _cataSvcName.empty() )
      {
         CHAR szPort[ 10 ] = { 0 } ;
         UINT16 svcPort = 0 ;
         rc = ossGetPort( _pSvcName, svcPort ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Invalid svcname[%s], rc: %d", _pSvcName, rc ) ;
            goto error ;
         }
         ossItoa( svcPort + MSG_ROUTE_CAT_SERVICE , szPort, 10 ) ;
         _cataSvcName = szPort ;
      }

      cataAddr = _pHostName ;
      cataAddr += ":" ;
      cataAddr += _cataSvcName ;
      /// catalog address list
      builder.append( PMD_OPTION_CATALOG_ADDR, cataAddr ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordCMDCreateCataGroup::_ignoreGroupNameWhenNodeInfo() const
   {
      return TRUE ;
   }

   /*
      _coordCMDCreateGroup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateGroup,
                                      CMD_NAME_CREATE_GROUP,
                                      FALSE ) ;
   _coordCMDCreateGroup::_coordCMDCreateGroup()
   {
   }

   _coordCMDCreateGroup::~_coordCMDCreateGroup()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( COORD_CREATEGROUP_EXE, "_coordCMDCreateGroup::execute" )
   INT32 _coordCMDCreateGroup::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc                         = SDB_OK ;
      const CHAR *pGroupName           = NULL ;
      const CHAR *pQuery               = NULL ;
      MsgOpQuery *pCreateReq           = (MsgOpQuery *)pMsg ;
      vector<BSONObj> replyObjs;

      PD_TRACE_ENTRY ( COORD_CREATEGROUP_EXE ) ;

      // fill default-reply(create group success)
      contextID                        = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL,
                            NULL, &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message, rc: %d", rc ) ;

      try
      {
         BSONObj obj( pQuery ) ;
         BSONElement ele = obj.getField( FIELD_NAME_GROUPNAME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "Get field[%s] failed from obj[%s]",
                    FIELD_NAME_GROUPNAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pGroupName = ele.valuestr() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// execute on catalog
      pCreateReq->header.opCode = MSG_CAT_CREATE_GROUP_REQ ;
      rc = executeOnCataGroup( pMsg, cb, NULL, &replyObjs, TRUE, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute on catalog, rc: %d", rc ) ;
         goto error ;
      }

      if ( replyObjs.empty() )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Failed to get group info from reply, rc: %d", rc);
         goto error;
      }

      *buf = rtnContextBuf( replyObjs[0] );

   done :
      if ( pGroupName )
      {
         PD_AUDIT_COMMAND( AUDIT_CLUSTER, getName(),
                           AUDIT_OBJ_GROUP, pGroupName, rc,
                           "" ) ;
      }
      PD_TRACE_EXITRC ( COORD_CREATEGROUP_EXE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   /*
      _coordCMDActiveGroup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDActiveGroup,
                                      CMD_NAME_ACTIVE_GROUP,
                                      TRUE ) ;
   _coordCMDActiveGroup::_coordCMDActiveGroup()
   {
   }

   _coordCMDActiveGroup::~_coordCMDActiveGroup()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ACTIVEGRP_PARSEMSG, "_coordCMDActiveGroup::_parseMsg" )
   INT32 _coordCMDActiveGroup::_parseMsg ( MsgHeader *pMsg,
                                           coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ACTIVEGRP_PARSEMSG ) ;

      try
      {
         string groupName ;
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_GROUPNAME_NAME,
                                      groupName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_GROUPNAME_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pArgs->_targetName = groupName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_ACTIVEGRP_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDActiveGroup::_generateCataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_ACTIVE_GROUP_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ACTIVEGRP_DOONCATAGROUP, "_coordCMDActiveGroup::_doOnCataGroup" )
   INT32 _coordCMDActiveGroup::_doOnCataGroup( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord::sharePtr *ppContext,
                                               coordCMDArguments *pArgs,
                                               CoordGroupList *pGroupLst,
                                               vector<BSONObj> *pReplyObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_ACTIVEGRP_DOONCATAGROUP ) ;

      rc = _coordCMD2Phase::_doOnCataGroup( pMsg, cb, ppContext, pArgs,
                                            NULL, pReplyObjs ) ;
      if ( rc )
      {
         _pResource->getCataNodeAddrList( _vecNodes ) ;

         // For catalog group only
         if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) &&
              !_vecNodes.empty() )
         {
            rc = SDB_OK ;
         }
         else
         {
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( COORD_ACTIVEGRP_DOONCATAGROUP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ACTIVEGRP_DOONDATAGROUP, "_coordCMDActiveGroup::_doOnDataGroup" )
   INT32 _coordCMDActiveGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                rtnContextCoord::sharePtr *ppContext,
                                                coordCMDArguments *pArgs,
                                                const CoordGroupList &groupLst,
                                                const vector<BSONObj> &cataObjs,
                                                CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_ACTIVEGRP_DOONDATAGROUP ) ;

      vector<BSONObj> dataObjs ;
      vector<INT32> opList ;

      opList.push_back( SDBSTART ) ;

      if ( cataObjs.size() > 0 )
      {
         rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;
      }
      else
      {
         CoordGroupInfoPtr groupPtr ;
         rc = _pResource->getOrUpdateGroupInfo( pArgs->_targetName.c_str(),
                                                groupPtr,
                                                cb ) ;
         if ( SDB_OK == rc )
         {
            // For catalog group
            rc = _opOnCataNodes( opList, groupPtr.get(), dataObjs ) ;
         }
         else if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) &&
                   !_vecNodes.empty() )
         {
            rc = _opOnVecNodes( opList, _vecNodes, dataObjs ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Get group info failed on command[%s, targe:%s], "
                    "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         }
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Start nodes failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         _logErrorForNodes( getName(), pArgs->_targetName, dataObjs ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_ACTIVEGRP_DOONDATAGROUP, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN _coordCMDActiveGroup::_flagIsStartNodes() const
   {
      return TRUE ;
   }

   /*
      _coordCMDShutdownGroup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDShutdownGroup,
                                      CMD_NAME_SHUTDOWN_GROUP,
                                      TRUE ) ;
   _coordCMDShutdownGroup::_coordCMDShutdownGroup()
   {
   }

   _coordCMDShutdownGroup::~_coordCMDShutdownGroup()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_SHUTDOWNGRP_PARSEMSG, "_coordCMDShutdownGroup::_parseMsg" )
   INT32 _coordCMDShutdownGroup::_parseMsg ( MsgHeader *pMsg,
                                             coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_SHUTDOWNGRP_PARSEMSG ) ;

      try
      {
         string groupName ;
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_GROUPNAME_NAME,
                                      groupName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[s], rc: %d",
                    CAT_GROUPNAME_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         pArgs->_targetName = groupName ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_SHUTDOWNGRP_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDShutdownGroup::_generateCataMsg ( MsgHeader *pMsg,
                                                    pmdEDUCB *cb,
                                                    coordCMDArguments *pArgs,
                                                    CHAR **ppMsgBuf,
                                                    INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_SHUTDOWN_GROUP_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_SHUTDOWNGRP_DOONDATA, "_coordCMDShutdownGroup::_doOnDataGroup" )
   INT32 _coordCMDShutdownGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  rtnContextCoord::sharePtr *ppContext,
                                                  coordCMDArguments *pArgs,
                                                  const CoordGroupList &groupLst,
                                                  const vector<BSONObj> &cataObjs,
                                                  CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_SHUTDOWNGRP_DOONDATA ) ;

      vector<BSONObj> dataObjs ;
      vector<INT32> opList ;

      PD_CHECK( 1 == cataObjs.size(), SDB_SYS, error, PDERROR,
                "Could not find group in catalog on command[%s, targe:%s]",
                getName(), pArgs->_targetName.c_str() ) ;

      opList.push_back( SDBSTOP ) ;
      rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;

      if ( rc )
      {
         PD_LOG( PDERROR, "Stop nodes failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         _logErrorForNodes( getName(), pArgs->_targetName, dataObjs ) ;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_SHUTDOWNGRP_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_SHUTDOWNGRP_DOCOMMIT, "_coordCMDShutdownGroup::_doCommit" )
   INT32 _coordCMDShutdownGroup::_doCommit ( MsgHeader *pMsg,
                                             pmdEDUCB * cb,
                                             rtnContextCoord::sharePtr *ppContext,
                                             coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_SHUTDOWNGRP_DOCOMMIT ) ;

      rc = _coordNodeCMD2Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      if ( rc )
      {
         if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
         {
            // Should have problem (e.g. -79) when shutdown Catalog group
            PD_LOG( PDWARNING, "Ignored error[%d] when do "
                    "command[%s, targe:%s]", rc, getName(),
                    pArgs->_targetName.c_str() ) ;
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXITRC ( COORD_SHUTDOWNGRP_DOCOMMIT, rc ) ;
      return rc ;
   }

   /*
      _coordCMDRemoveGroup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRemoveGroup,
                                      CMD_NAME_REMOVE_GROUP,
                                      FALSE ) ;
   _coordCMDRemoveGroup::_coordCMDRemoveGroup()
   {
      _enforce = FALSE ;
   }

   _coordCMDRemoveGroup::~_coordCMDRemoveGroup()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOGEGRP_PARSEMSG, "_coordCMDRemoveGroup::_parseMsg" )
   INT32 _coordCMDRemoveGroup::_parseMsg ( MsgHeader *pMsg,
                                           coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_REMOGEGRP_PARSEMSG ) ;

      try
      {
         string groupName ;
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_GROUPNAME_NAME,
                                      groupName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_GROUPNAME_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         pArgs->_targetName = groupName ;

         /// get enforce
         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ENFORCED1,
                                    _enforce ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", FIELD_NAME_ENFORCED1, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_REMOGEGRP_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDRemoveGroup::_generateCataMsg ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  coordCMDArguments *pArgs,
                                                  CHAR **ppMsgBuf,
                                                  INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_RM_GROUP_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOGEGRP_DOONDATA, "_coordCMDRemoveGroup::_doOnDataGroup" )
   INT32 _coordCMDRemoveGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                rtnContextCoord::sharePtr *ppContext,
                                                coordCMDArguments *pArgs,
                                                const CoordGroupList &groupLst,
                                                const vector<BSONObj> &cataObjs,
                                                CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_REMOGEGRP_DOONDATA ) ;

      vector<INT32> opList ;
      vector<BSONObj> dataObjs ;

      PD_CHECK( 1 == cataObjs.size(), SDB_SYS, error, PDERROR,
                "Could not find group in catalog on command[%s, targe:%s]",
                getName(), pArgs->_targetName.c_str() ) ;

      opList.push_back( SDBTEST ) ;
      rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;
      if ( rc )
      {
         PD_LOG( ( _enforce ? PDWARNING : PDERROR ),
                 "Test nodes failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         if ( _enforce )
         {
            rc = SDB_OK ;
         }
         else
         {
            _logErrorForNodes( getName(), pArgs->_targetName, dataObjs ) ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( COORD_REMOGEGRP_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOGEGRP_DOONDATA2, "_coordCMDRemoveGroup::_doOnDataGroupP2" )
   INT32 _coordCMDRemoveGroup::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  rtnContextCoord::sharePtr *ppContext,
                                                  coordCMDArguments *pArgs,
                                                  const CoordGroupList &groupLst,
                                                  const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_REMOGEGRP_DOONDATA2 ) ;

      vector<INT32> opList ;
      vector<BSONObj> dataObjs ;

      opList.push_back( SDBSTOP ) ;
      opList.push_back( SDBRM ) ;
      rc = _opOnNodes( opList, cataObjs[0], dataObjs ) ;
      if ( rc )
      {
         PD_LOG( ( _enforce ? PDWARNING : PDERROR ),
                 "Remove nodes failed on command[%s, targe:%s], "
                 "rc: %d", getName(), pArgs->_targetName.c_str(), rc ) ;
         if ( _enforce )
         {
            rc = SDB_OK ;
         }
         else
         {
            _logErrorForNodes( getName(), pArgs->_targetName, dataObjs ) ;
            goto error ;
         }
      }

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         _pResource->clearCataNodeAddrList() ;
         _pResource->syncAddress2Options( TRUE, FALSE ) ;
         _pResource->invalidateCataInfo() ;
         _pResource->invalidateGroupInfo() ;
      }
      else
      {
         _pResource->removeGroupInfo( pArgs->_targetName.c_str() ) ;
      }

      /// remove last node
      if ( cb->getRemoteSite() )
      {
         pmdRemoteSessionSite *pSite = NULL ;
         coordSessionPropSite *pSiteProp = NULL ;
         pSite = ( pmdRemoteSessionSite* )cb->getRemoteSite() ;
         pSiteProp = ( coordSessionPropSite* )pSite->getUserData() ;
         if ( pSiteProp )
         {
            pSiteProp->clear() ;
         }
      }

   done :
      PD_TRACE_EXITRC ( COORD_REMOGEGRP_DOONDATA2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOGEGRP_DOCOMMIT, "_coordCMDRemoveGroup::_doCommit" )
   INT32 _coordCMDRemoveGroup::_doCommit ( MsgHeader *pMsg,
                                           pmdEDUCB * cb,
                                           rtnContextCoord::sharePtr *ppContext,
                                           coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_REMOGEGRP_DOCOMMIT ) ;

      rc = _coordNodeCMD3Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      if ( rc )
      {
         if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
         {
            // Should have problem (e.g. -79) when removing Catalog group
            PD_LOG( PDWARNING, "Ignored error[%d] when do "
                    "command[%s, targe:%s]", rc, getName(),
                    pArgs->_targetName.c_str() ) ;
            rc = SDB_OK ;
         }
      }

      PD_TRACE_EXITRC ( COORD_REMOGEGRP_DOCOMMIT, rc ) ;

      return rc ;
   }

   /*
      _coordCMDCreateNode implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDCreateNode,
                                      CMD_NAME_CREATE_NODE,
                                      FALSE ) ;
   _coordCMDCreateNode::_coordCMDCreateNode()
   {
      _pHostName = NULL ;
      _onlyAttach = FALSE ;
      _keepData = FALSE ;
      _pSvcName = NULL ;
   }

   _coordCMDCreateNode::~_coordCMDCreateNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_PARSEMSG, "_coordCMDCreateNode::_parseMsg" )
   INT32 _coordCMDCreateNode::_parseMsg ( MsgHeader *pMsg,
                                          coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_CREATENODE_PARSEMSG ) ;

      try
      {
         UINT32 validCount = 0 ;
         BSONElement ele ;
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_GROUPNAME_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_GROUPNAME_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         ++validCount ;

         ele = pArgs->_boQuery.getField( CAT_HOST_FIELD_NAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_HOST_FIELD_NAME, getName(), rc ) ;
            goto error ;
         }
         _pHostName = ele.valuestr() ;
         ++validCount ;

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ONLY_ATTACH,
                                    _onlyAttach ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", FIELD_NAME_ONLY_ATTACH, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            ++validCount ;
         }

         /// check svcname
         ele = pArgs->_boQuery.getField( PMD_OPTION_SVCNAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", PMD_OPTION_SVCNAME, getName(), rc ) ;
            goto error ;
         }
         _pSvcName = ele.valuestr() ;
         ++validCount ;

         if ( _onlyAttach )
         {
            if ( 0 == pArgs->_targetName.compare( COORD_GROUPNAME ) )
            {
               PD_LOG( PDERROR, "Attach node only support for data "
                       "group or catalog group now" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_KEEP_DATA,
                                       _keepData ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Not specify the field[%s] or it has"
                           " an invalid value in options"
                           " when attaching node.", FIELD_NAME_KEEP_DATA ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else
            {
               ++validCount ;
            }

            if ( (UINT32)pArgs->_boQuery.nFields() > validCount )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Unknown parameters in command's args[%s]",
                       pArgs->_boQuery.toString().c_str() ) ;
               goto error ;
            }
         }
         else
         {
            /// check dbpath
            ele = pArgs->_boQuery.getField( PMD_OPTION_DBPATH ) ;
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                       "rc: %d", PMD_OPTION_DBPATH, getName(), rc ) ;
               goto error ;
            }

            /// check instance ID
            ele = pArgs->_boQuery.getField( PMD_OPTION_INSTANCE_ID ) ;
            if ( ele.eoo() || ele.isNumber() || ele.type() == bson::String )
            {
               UINT32 instanceID = NODE_INSTANCE_ID_UNKNOWN ;
               if ( ele.isNumber() )
               {
                  instanceID = ele.numberInt() ;
               }
               else if ( ele.type() == bson::String )
               {
                  rc = utilStr2Num( ele.str().c_str(), (INT32 &)instanceID,
                                    UTIL_STR2NUM_DEC ) ;
                  PD_RC_CHECK( rc, PDERROR,
                               "Fail to convert field[%s] %s to int, rc: %d",
                               PMD_OPTION_INSTANCE_ID, ele.str().c_str(), rc ) ;
               }
               PD_CHECK( utilCheckInstanceID( instanceID, TRUE ),
                         SDB_INVALIDARG, error, PDERROR,
                         "Failed to check field [%s], "
                         "should be %d, or between %d to %d",
                         PMD_OPTION_INSTANCE_ID, NODE_INSTANCE_ID_UNKNOWN,
                         NODE_INSTANCE_ID_MIN + 1, NODE_INSTANCE_ID_MAX - 1 ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                       "rc: %d", PMD_OPTION_INSTANCE_ID, getName(), rc ) ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATENODE_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_GENCATAMSG, "_coordCMDCreateNode::_generateCataMsg" )
   INT32 _coordCMDCreateNode::_generateCataMsg ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATENODE_GENCATAMSG ) ;

      CHAR *pBuf = NULL ;
      INT32 bufSize = 0 ;

      if ( _onlyAttach )
      {
         INT32 retCode = SDB_OK ;
         BSONObjBuilder builder ;
         std::vector<BSONObj> objs ;
         BSONObj obj ;

         rc = rtnRemoteExec( SDBGETCONF, _pHostName,
                             &retCode, &(pArgs->_boQuery),
                             NULL, NULL, NULL, &objs ) ;
         rc = SDB_OK == rc ? retCode : rc ;
         PD_RC_CHECK( rc, PDERROR, "Do remote getconf on node[%s:%s] "
                      "failed, rc: %d", _pHostName, _pSvcName,
                      rc ) ;

         PD_CHECK( 1 == objs.size(), SDB_SYS, error, PDERROR,
                   "Get config failed on host[%s:%s], command[%s, targe:%s]",
                   _pHostName, _pSvcName, getName(),
                   pArgs->_targetName.c_str() ) ;

         builder.append( CAT_GROUPNAME_NAME, pArgs->_targetName ) ;
         builder.append( FIELD_NAME_HOST, _pHostName ) ;
         builder.appendElements( objs[0] ) ;
         obj = builder.obj() ;

         rc = msgBuildQueryMsg( &pBuf, &bufSize,
                                CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE,
                                0, 0, 0, -1,
                                &obj, NULL, NULL, NULL,
                                cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Build catalog message failed on "
                      "command[%s], rc: %d", getName(), rc ) ;

         *ppMsgBuf = (CHAR*)pBuf ;
         *pBufSize = bufSize ;

         ((MsgHeader*)pBuf)->opCode = MSG_CAT_CREATE_NODE_REQ ;
      }
      else
      {
         pMsg->opCode = MSG_CAT_CREATE_NODE_REQ ;
         *ppMsgBuf = (CHAR*)pMsg ;
         *pBufSize = pMsg->messageLength ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATENODE_GENCATAMSG, rc ) ;
      return rc ;
   error :
      if ( pBuf )
      {
         msgReleaseBuffer( pBuf, cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_DOONDATA, "_coordCMDCreateNode::_doOnDataGroup" )
   INT32 _coordCMDCreateNode::_doOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord::sharePtr *ppContext,
                                               coordCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs,
                                               CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATENODE_DOONDATA ) ;

      if ( !_onlyAttach )
      {
         // We need to test the sdbcm
         INT32 retCode = SDB_OK ;
         rc = rtnRemoteExec( SDBTEST, _pHostName, &retCode ) ;
         PD_RC_CHECK( rc, PDERROR, "Do remote test on node[%s:%s] failed, "
                      "command[%s, targe:%s], rc: %d", _pHostName,
                      _pSvcName, getName(), pArgs->_targetName.c_str(),
                      rc) ;
      }
      else
      {
         // When attaching, sdbcm is tested by getting node configure in
         // previous phase, so do nothing here.
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATENODE_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_DOONDATA2, "_coordCMDCreateNode::_doOnDataGroupP2" )
   INT32 _coordCMDCreateNode::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord::sharePtr *ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATENODE_DOONDATA2 ) ;

      INT32 retCode = SDB_OK ;

      if ( _onlyAttach )
      {
         if ( !_keepData )
         {
            rc = rtnRemoteExec( SDBCLEARDATA, _pHostName,
                                &retCode, &(pArgs->_boQuery) ) ;
            rc = rc ? rc : retCode ;
            PD_RC_CHECK( rc, PDERROR, "Do remote cleardata on node[%s:%s] "
                         "failed, command[%s, targe:%s], rc: %d",
                         _pHostName, _pSvcName, getName(),
                         pArgs->_targetName.c_str(), rc ) ;
         }

         rc = rtnRemoteExec ( SDBSTART, _pHostName, &retCode,
                              &(pArgs->_boQuery) ) ;
         rc = rc ? rc : retCode ;
         PD_RC_CHECK( rc, PDERROR, "Do remote start on node[%s:%s] "
                      "failed, command[%s, targe:%s], rc: %d",
                      _pHostName, _pSvcName, getName(),
                      pArgs->_targetName.c_str(), rc ) ;
      }
      else
      {
         BSONObj boNodeConfig ;
         SINT32 retCode;

         rc = getNodeConf( _pResource, pArgs->_boQuery.objdata(),
                           boNodeConfig ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Build node config failed on "
                    "command[%s, targe:%s], rc: %d", getName(),
                    pArgs->_targetName.c_str(), rc ) ;
            goto error ;
         }

         rc = rtnRemoteExec( SDBADD, _pHostName, &retCode, &boNodeConfig ) ;
         rc = rc ? rc : retCode ;
         PD_RC_CHECK( rc, PDERROR, "Do remote add on node[%s:%s] "
                      "failed, command[%s, targe:%s], rc: %d",
                      _pHostName, _pSvcName, getName(),
                      pArgs->_targetName.c_str(), rc ) ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_CREATENODE_DOONDATA2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_COMPLETE, "_coordCMDCreateNode::_doComplete" )
   INT32 _coordCMDCreateNode::_doComplete ( MsgHeader *pMsg,
                                            pmdEDUCB * cb,
                                            coordCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( COORD_CREATENODE_COMPLETE ) ;

      CoordGroupInfoPtr groupPtr ;
      coordNodeCMDHelper helper ;

      // update group info on local node, in case that it isn't registered
      // in the cluster. ignored error.
      _pResource->updateGroupInfo( pArgs->_targetName.c_str(),
                                   groupPtr, cb ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         // notify all nodes, except local node
         helper.notify2AllNodes( _pResource, TRUE, cb ) ;
      }

      PD_TRACE_EXIT ( COORD_CREATENODE_COMPLETE ) ;
      return SDB_OK ;
   }

   INT32 _coordCMDCreateNode::_doOutput( rtnContextBuf *buf )
   {
      *buf = rtnContextBuf( _nodeInfo );
      return SDB_OK;
   }

   AUDIT_OBJ_TYPE _coordCMDCreateNode::_getAuditObjectType() const
   {
      return AUDIT_OBJ_NODE ;
   }

   string _coordCMDCreateNode::_getAuditDesp( coordCMDArguments *pArgs ) const
   {
      return "Group: " + pArgs->_targetName ;
   }

   string _coordCMDCreateNode::_getAuditObjectName( coordCMDArguments *pArgs ) const
   {
      string hostname ;
      string svcname ;
      if ( _pHostName )
      {
         hostname = _pHostName ;
      }
      if ( _pSvcName )
      {
         svcname = _pSvcName ;
      }
      return hostname + ":" + svcname ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_PARSECATP2RETURN, "_coordCMDCreateNode::parseCatP2Return" )
   INT32 _coordCMDCreateNode::parseCatP2Return( coordCMDArguments *pArgs,
                                                const std::vector< bson::BSONObj > &cataObjs )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY( COORD_CREATENODE_PARSECATP2RETURN );
      if ( cataObjs.empty() )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Failed to get node info from catalog on phase 2" );
         goto error;
      }
      _nodeInfo = cataObjs.at(0).getOwned();

   done:
      PD_TRACE_EXITRC( COORD_CREATENODE_PARSECATP2RETURN, rc );
      return rc;
   error:
      goto done;
   }

   /*
      _coordCMDRemoveNode implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDRemoveNode,
                                      CMD_NAME_REMOVE_NODE,
                                      FALSE ) ;
   _coordCMDRemoveNode::_coordCMDRemoveNode()
   {
      _pHostName = NULL ;
      _pSvcName = NULL ;
      _onlyDetach = FALSE ;
      _keepData = FALSE ;
      _enforce = FALSE ;
   }

   _coordCMDRemoveNode::~_coordCMDRemoveNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOVENODE_PARSEMSG, "_coordCMDRemoveNode::_parseMsg" )
   INT32 _coordCMDRemoveNode::_parseMsg ( MsgHeader *pMsg,
                                          coordCMDArguments *pArgs)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_REMOVENODE_PARSEMSG ) ;

      UINT32 validCount = 0 ;

      try
      {
         BSONElement ele ;
         rc = rtnGetSTDStringElement( pArgs->_boQuery, CAT_GROUPNAME_NAME,
                                      pArgs->_targetName ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], rc: %d",
                    CAT_GROUPNAME_NAME, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         ++validCount ;

         ele = pArgs->_boQuery.getField( CAT_HOST_FIELD_NAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_HOST_FIELD_NAME, getName(), rc ) ;
            goto error ;
         }
         _pHostName = ele.valuestr() ;
         ++validCount ;

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ONLY_DETACH,
                                    _onlyDetach ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", FIELD_NAME_ONLY_DETACH, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            ++validCount ;
         }

         /// get enforced and Enforced
         /// if there are both Enforced and enforced, just use Enforced.
         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ENFORCED,
                                    _enforce ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", FIELD_NAME_ENFORCED, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            ++validCount ;
         }

         rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_ENFORCED1,
                                    _enforce ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", FIELD_NAME_ENFORCED1, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            ++validCount ;
         }

         /// check svcname
         ele = pArgs->_boQuery.getField( PMD_OPTION_SVCNAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", PMD_OPTION_SVCNAME, getName(), rc ) ;
            goto error ;
         }
         _pSvcName = ele.valuestr() ;
         ++validCount ;

         if ( _onlyDetach )
         {
            if ( 0 == pArgs->_targetName.compare( COORD_GROUPNAME ) )
            {
               PD_LOG( PDERROR, "Detach node only support for data "
                       "group or catalog group now" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            rc = rtnGetBooleanElement( pArgs->_boQuery, FIELD_NAME_KEEP_DATA,
                                       _keepData ) ;
            if ( rc )
            {
               PD_LOG_MSG( PDERROR, "Not specify the field[%s] or it has"
                           " an invalid value in options"
                           " when detaching node.", FIELD_NAME_KEEP_DATA ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else
            {
               ++validCount ;
            }
         }

         if ( (UINT32)pArgs->_boQuery.nFields() > validCount )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Unknown parameters in command's args[%s]",
                    pArgs->_boQuery.toString().c_str() ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG;
         goto error ;
      }

   done :
      PD_TRACE_EXITRC ( COORD_REMOVENODE_PARSEMSG, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDRemoveNode::_generateCataMsg ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs,
                                                 CHAR **ppMsgBuf,
                                                 INT32 *pBufSize )
   {
      pMsg->opCode = MSG_CAT_DEL_NODE_REQ ;
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOVENODE_DOONDATA, "_coordCMDRemoveNode::_doOnDataGroup" )
   INT32 _coordCMDRemoveNode::_doOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord::sharePtr *ppContext,
                                               coordCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs,
                                               CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_REMOVENODE_DOONDATA ) ;

      // We need to test the sdbcm
      INT32 retCode = SDB_OK ;
      rc = rtnRemoteExec ( SDBTEST, _pHostName, &retCode ) ;
      if ( rc )
      {
         PD_LOG( ( _enforce ? PDWARNING : PDERROR ),
                 "Do remote test on host[%s:%s] failed, command[%s, targe:%s], "
                 "rc: %d", _pHostName, _pSvcName, getName(),
                 pArgs->_targetName.c_str(), rc ) ;
         if ( _enforce )
         {
            rc = SDB_OK ;
         }
         else
         {
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( COORD_REMOVENODE_DOONDATA, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOVENODE_DOONDATA2, "_coordCMDRemoveNode::_doOnDataGroupP2" )
   INT32 _coordCMDRemoveNode::_doOnDataGroupP2 ( MsgHeader *pMsg,
                                                 pmdEDUCB *cb,
                                                 rtnContextCoord::sharePtr *ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_REMOVENODE_DOONDATA2 ) ;

      INT32 retCode = SDB_OK ;

      /// notify the other nodes to update groupinfo.
      /// here we do not care whether they succeed.
      coordNodeCMDHelper helper ;
      helper.notify2GroupNodes( _pResource, pArgs->_targetName.c_str(), cb ) ;

      /// ignore the stop result, because remove or clear will
      /// retry to stop in sdbcm
      rc = rtnRemoteExec ( SDBSTOP, _pHostName, &retCode,
                           &(pArgs->_boQuery) ) ;
      rc = rc ? rc : retCode ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Do remote stop on node[%s:%s] failed in "
                 "command[%s, targe:%s], rc: %d", _pHostName, _pSvcName,
                 getName(), pArgs->_targetName.c_str(), rc ) ;
         if ( _enforce )
         {
            rc = SDB_OK ;
         }
      }

      if ( !_onlyDetach )
      {
         rc = rtnRemoteExec ( SDBRM, _pHostName, &retCode,
                              &(pArgs->_boQuery) ) ;
         rc = rc ? rc : retCode ;
         if ( rc )
         {
            PD_LOG( ( _enforce ? PDWARNING : PDERROR ),
                    "Do remote remove on node[%s:%s] failed in "
                    "command[%s, targe:%s], rc: %d",
                    _pHostName, _pSvcName, getName(),
                    pArgs->_targetName.c_str(), rc ) ;
            if ( _enforce )
            {
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
      }
      else if ( !_keepData )
      {
         rc = rtnRemoteExec( SDBCLEARDATA, _pHostName, &retCode,
                             &(pArgs->_boQuery) ) ;
         rc = rc ? rc : retCode ;
         if ( rc )
         {
            PD_LOG( ( _enforce ? PDWARNING : PDERROR ),
                    "Do remote cleardata on node[%s:%s] failed in "
                    "command[%s, targe:%s], rc: %d",
                    _pHostName, _pSvcName, getName(),
                    pArgs->_targetName.c_str(), rc ) ;
            if ( _enforce )
            {
               rc = SDB_OK ;
            }
            else
            {
               goto error ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC ( COORD_REMOVENODE_DOONDATA2, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOVENODE_DOCOMPLETE, "_coordCMDRemoveNode::_doComplete" )
   INT32 _coordCMDRemoveNode::_doComplete ( MsgHeader *pMsg,
                                            pmdEDUCB * cb,
                                            coordCMDArguments *pArgs )
   {
      PD_TRACE_ENTRY ( COORD_REMOVENODE_DOCOMPLETE ) ;

      CoordGroupInfoPtr groupPtr ;
      coordNodeCMDHelper helper ;

      // update group info on local node, in case that it isn't registered
      // in the cluster. ignored error.
      _pResource->updateGroupInfo( pArgs->_targetName.c_str(),
                                   groupPtr, cb ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         // notify all nodes, except local node
         helper.notify2AllNodes( _pResource, TRUE, cb ) ;
      }

      PD_TRACE_EXIT ( COORD_REMOVENODE_DOCOMPLETE ) ;
      return SDB_OK ;
   }

   AUDIT_OBJ_TYPE _coordCMDRemoveNode::_getAuditObjectType() const
   {
      return AUDIT_OBJ_NODE ;
   }

   string _coordCMDRemoveNode::_getAuditDesp( coordCMDArguments *pArgs ) const
   {
      return "Group: " + pArgs->_targetName ;
   }

   string _coordCMDRemoveNode::_getAuditObjectName( coordCMDArguments *pArgs ) const
   {
      string hostname ;
      string svcname ;
      if ( _pHostName )
      {
         hostname = _pHostName ;
      }
      if ( _pSvcName )
      {
         svcname = _pSvcName ;
      }
      return hostname + ":" + svcname ;
   }

   /* 
      _coordCMDAlterNode implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterNode,
                                      CMD_NAME_ALTER_NODE,
                                      FALSE ) ;
   _coordCMDAlterNode::_coordCMDAlterNode()
   {
   }

   _coordCMDAlterNode::~_coordCMDAlterNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERNODE_EXE, "_coordCMDAlterNode::execute" )
   INT32 _coordCMDAlterNode::execute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      INT64 &contextID,
                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERNODE_EXE ) ;

      BSONObj hintObj ;
      BSONElement ele ;
      UINT32 groupID = INVALID_GROUPID ;
      coordNodeCMDHelper helper ;

      // Get groupID
      const CHAR *pHint = NULL ;
      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL, &pHint ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message, rc: %d", rc ) ;

      try
      {
         hintObj.init( pHint ) ;
         ele = hintObj.getField( FIELD_NAME_GROUPID ) ;
         groupID = ele.numberInt() ;
      }
      catch ( const std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse hint object: %s", pHint ) ;
         goto error ;
      }

      // Send to cataGroup
      rc = executeOnCataGroup( pMsg, cb, NULL, NULL, TRUE, NULL, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute command [%s] on catalog, "
                   "rc: %d", getName(), rc ) ;

      if ( COORD_GROUPID == groupID ||
           CATALOG_GROUPID ==  groupID ||
           ( DATA_GROUP_ID_BEGIN <= groupID &&
             DATA_GROUP_ID_END >= groupID ) )
      {
         // Notify all group nodes to update group info in clsReplicateSet
         helper.notify2GroupNodes( _pResource, groupID, cb ) ;
      }
      else
      {
         // Only coord, cata and data groupID are valid
         PD_LOG( PDWARNING, "Failed to notify to group, got invalid groupID: %d",
                 groupID ) ;
      }

      if ( CATALOG_GROUPID ==  groupID )
      {
         // Notify all group nodes to update group info in _clsShardMgr
         helper.notify2AllNodes( _pResource, TRUE, cb ) ;
      }
   done:
      PD_TRACE_EXITRC ( COORD_ALTERNODE_EXE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _coordCMDReelectBase implement
   */
   _coordCMDReelectBase::_coordCMDReelectBase():
   _updatedGrpInfo( FALSE ),
   _groupName( NULL )
   {
   }

   _coordCMDReelectBase::~_coordCMDReelectBase()
   {
   }

   INT32 _coordCMDReelectBase::_parseNodeInfo( const BSONObj &optionsObj,
                                               MsgRouteID &nodeID,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      UINT16 tmpNodeID = 0 ;
      const CHAR *pHostName = "" ;
      const CHAR *pSvcName = "" ;

      nodeID.value = 0 ;

      BSONElement e ;
      BSONObjIterator itr( optionsObj ) ;
      while( itr.more() )
      {
         e = itr.next() ;

         if ( 0 == ossStrcasecmp( FIELD_NAME_NODEID, e.fieldName() ) )
         {
            if ( !e.isNumber() || 0 == e.numberInt() )
            {
               rc = SDB_INVALIDARG ;
               break ;
            }
            tmpNodeID = e.numberInt() ;
         }
         else if ( 0 == ossStrcasecmp( FIELD_NAME_HOST, e.fieldName() ) )
         {
            if ( String != e.type() || !*e.valuestr() )
            {
               rc = SDB_INVALIDARG ;
               break ;
            }
            pHostName = e.valuestr() ;
         }
         else if ( 0 == ossStrcasecmp( FIELD_NAME_SERVICE_NAME, e.fieldName() ) ||
                   0 == ossStrcasecmp( PMD_OPTION_SVCNAME, e.fieldName() ) )
         {
            if ( String != e.type() || !*e.valuestr() )
            {
               rc = SDB_INVALIDARG ;
               break ;
            }
            pSvcName = e.valuestr() ;
         }
         else if ( 0 == ossStrcmp( FIELD_NAME_GROUPNAME, e.fieldName() ) )
         {
            if ( String != e.type() || !*e.valuestr() )
            {
               rc = SDB_INVALIDARG ;
               break ;
            }
            _groupName = e.valuestr() ;
         }
         else if ( 0 == ossStrcmp( FIELD_NAME_REELECTION_TIMEOUT,
                                   e.fieldName() ) ||
                   0 == ossStrcmp( FIELD_NAME_REELECTION_LEVEL,
                                   e.fieldName() ) )
         {
            /// ignore
         }
         else
         {
            rc = _parseExtraArgs( e ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse extra args" ) ;
         }
      }

      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Param[%s] is invalid", e.fieldName() ) ;
         goto error ;
      }
      else if ( !_groupName || !*_groupName )
      {
         PD_LOG_MSG( PDERROR, "Param[%s] is not configured",
                     FIELD_NAME_GROUPNAME ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _checkExtraArgs( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check node info, rc: %d", rc ) ;

      if ( 0 != tmpNodeID || *pHostName || *pSvcName )
      {
         /// update group info
         if ( !_updatedGrpInfo )
         {
            rc = _pResource->updateGroupInfo( _groupName, _groupInfoPtr, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Update group[%s] info failed, rc: %d",
                       _groupName, rc ) ;
               goto error ;
            }
         }

         rc = _getNotifyNode( nodeID, tmpNodeID, pHostName, pSvcName ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get target primary nodeID" ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDReelectBase::_parseExtraArgs( const BSONElement &e )
   {
      PD_LOG_MSG( PDERROR, "Param[%s] is unknown", e.fieldName() ) ;
      return SDB_INVALIDARG ;
   }

   INT32 _coordCMDReelectBase::_checkExtraArgs( pmdEDUCB *cb )
   {
      return SDB_OK ;
   }


   INT32 _coordCMDReelectBase::_buildReelectMsg( CHAR **ppBuffer,
                                                 INT32 *bufferSize,
                                                 const BSONObj &optionsObj,
                                                 const MsgRouteID &nodeID,
                                                 const CHAR* cmdName,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      BSONObj newOptionsObj ;
      BSONObjBuilder optionBuilder ;
      BSONObjIterator itr( optionsObj ) ;

      BSONObjBuilder hintBuilder ;
      BSONObj hintObj ;

      // Build query obj
      optionBuilder.append( FIELD_NAME_NODEID, (INT32)nodeID.columns.nodeID ) ;
      while ( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_NODEID ) )
         {
            continue ;
         }
         optionBuilder.append( e ) ;
      }
      newOptionsObj = optionBuilder.obj() ;

      // Build hint obj
      hintBuilder.appendBool( FIELD_NAME_ISDESTINATION, FALSE ) ;
      hintObj = hintBuilder.obj() ;

      rc = msgBuildQueryMsg( ppBuffer, bufferSize, cmdName, 0, 0, 0, -1,
                             &newOptionsObj, NULL, NULL, &hintObj, cb ) ;
      return rc ;
   }

   void _coordCMDReelectBase::_notifyReelect2DestNode( MsgHeader *pMsg,
                                                       const MsgRouteID &nodeID,
                                                       pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      pmdRemoteSession *pRemote = _groupSession.getSession() ;
      pmdSubSession *pSub = NULL ;

      _groupSession.clear() ;
      pSub = pRemote->addSubSession( nodeID.value ) ;
      pSub->setReqMsg( pMsg, PMD_EDU_MEM_NONE ) ;

      rc = pRemote->sendMsg( pSub ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Send message to node[%s] failed, rc: %d",
                 routeID2String( pSub->getNodeID() ).c_str(), rc ) ;
         goto done ;
      }

      rc = pRemote->waitReply1( TRUE ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Recieve reply message from node[%s] failed, "
                 "rc: %d", routeID2String( pSub->getNodeID() ).c_str(), rc ) ;
         goto done ;
      }

   done:
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REELECT_EXE, "_coordCMDReelectBase::execute" )
   INT32 _coordCMDReelectBase::execute( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        INT64 &contextID,
                                        rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_REELECT_EXE ) ;

      const CHAR *cmdName = NULL ;
      const CHAR *pQuery = NULL ;

      CHAR *pReelectBuffer = NULL ;
      INT32 reelctBuffSize = 0 ;
      CHAR *pNotifyBuffer = NULL ;
      INT32 notifyBuffSize = 0 ;

      MsgHeader *pNotifyMsg = NULL ;
      MsgHeader *pReelectMsg = pMsg ;
      MsgRouteID nodeID ;

      contextID = -1 ;

      rc = msgExtractQuery( (const CHAR*)pMsg, NULL, &cmdName,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the message, rc: %d", rc ) ;

      // Parse option and build reelect msg
      try
      {
         BSONObj optionsObj( pQuery ) ;
         rc = _parseNodeInfo( optionsObj, nodeID, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse node info, rc: %d", rc ) ;

         if ( MSG_INVALID_ROUTEID != nodeID.value )
         {
            rc = _buildNotifyMsg( &pNotifyBuffer, &notifyBuffSize, nodeID, cb ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Build message failed, rc: %d", rc ) ;
               goto done ;
            }
            pNotifyMsg = ( MsgHeader* )pNotifyBuffer ;

            rc = _buildReelectMsg( &pReelectBuffer, &reelctBuffSize, optionsObj, nodeID, cmdName, cb ) ;
            if ( rc )
            {
               PD_LOG( PDWARNING, "Build message failed, rc: %d", rc ) ;
               goto done ;
            }
            pReelectMsg = ( MsgHeader* )pReelectBuffer ;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected error happened:%s", e.what() ) ;
         goto error ;
      }

      // Notify to destination node
      if ( MSG_INVALID_ROUTEID != nodeID.value )
      {
         nodeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
         _notifyReelect2DestNode( pNotifyMsg, nodeID, cb ) ;
      }

      // Reelect command
      rc = _reelect( pReelectMsg, cb, buf ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to execute %s command, rc: %d", &cmdName[1], rc ) ;

   done:
      if ( pNotifyBuffer )
      {
         msgReleaseBuffer( pNotifyBuffer, cb ) ;
         pNotifyBuffer = NULL ;
         notifyBuffSize = 0 ;
      }
      if ( pReelectBuffer )
      {
         msgReleaseBuffer( pReelectBuffer, cb ) ;
         pReelectBuffer = NULL ;
         reelctBuffSize = 0 ;
      }
      PD_TRACE_EXITRC( COORD_REELECT_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDReelectGroup implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDReelectGroup,
                                      CMD_NAME_REELECT,
                                      TRUE ) ;
   _coordCMDReelectGroup::_coordCMDReelectGroup()
   {
   }

   _coordCMDReelectGroup::~_coordCMDReelectGroup()
   {
   }

   INT32 _coordCMDReelectGroup::_getNotifyNode( MsgRouteID& nodeID,
                                                const UINT16 tmpNodeID,
                                                const CHAR* hostName,
                                                const CHAR* svcName )
   {
      INT32 rc = SDB_OK ;

      clsNodeItem *pItem = NULL ;
      UINT32 pos = 0 ;

      while ( NULL != ( pItem = _groupInfoPtr->nodeItemByPos( pos++ ) ) )
      {
         if ( 0 != tmpNodeID )
         {
            if ( pItem->_id.columns.nodeID == tmpNodeID )
            {
               nodeID.value = pItem->_id.value ;
               break ;
            }
         }
         else if ( *hostName )
         {
            if ( 0 == ossStrcmp( hostName, pItem->_host ) &&
                  ( !*svcName || 0 == ossStrcmp( svcName,
                     pItem->_service[ MSG_ROUTE_LOCAL_SERVICE ].c_str() ) ) )
            {
               nodeID.value = pItem->_id.value ;
               break ;
            }
         }
         else if ( *svcName && 0 == ossStrcmp( svcName,
                     pItem->_service[ MSG_ROUTE_LOCAL_SERVICE ].c_str() ) )
         {
            nodeID.value = pItem->_id.value ;
            break ;
         }
      }

      if ( 0 == nodeID.value )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
      }

      return rc ;
   }

   INT32 _coordCMDReelectGroup::_buildNotifyMsg( CHAR **ppBuffer,
                                                 INT32 *bufferSize,
                                                 const MsgRouteID &nodeID,
                                                 pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      rc = msgBuildQueryMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_REELECT,
                             0, 0, 0, -1,
                             NULL, NULL, NULL, NULL,
                             cb ) ;

      return rc ;
   }

   INT32 _coordCMDReelectGroup::_reelect( MsgHeader *pMsg,
                                          pmdEDUCB *cb,
                                          rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      CoordGroupList groupLst ;

       if ( !_updatedGrpInfo )
      {
         rc = _pResource->getOrUpdateGroupInfo( _groupName, _groupInfoPtr, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update group[%s] info failed, rc: %d",
                    _groupName, rc ) ;
            goto error ;
         }
      }

      groupLst[_groupInfoPtr->groupID()] = _groupInfoPtr->groupID() ;
      rc = executeOnDataGroup( pMsg, cb, groupLst, TRUE, NULL, NULL, NULL, buf ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDReelectLocation implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDReelectLocation,
                                      CMD_NAME_REELECT_LOCATION,
                                      TRUE ) ;
   _coordCMDReelectLocation::_coordCMDReelectLocation():
   _location( NULL ),
   _locationID( MSG_INVALID_LOCATIONID )
   {
   }

   _coordCMDReelectLocation::~_coordCMDReelectLocation()
   {
   }

   INT32 _coordCMDReelectLocation::_parseExtraArgs( const BSONElement &e )
   {
      INT32 rc = SDB_OK ;

      if ( 0 == ossStrcmp( FIELD_NAME_LOCATION, e.fieldName() ) )
      {
         if ( e.eoo() || !*e.valuestr() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Location name can't be null string" ) ;
            goto error ;
         }
         if ( String != e.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Location type[%d] is not String", e.type() ) ;
            goto error ;
         }
         if ( MSG_LOCATION_NAMESZ < e.valuestrsize() - 1 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
            goto error ;
         }
         _location = e.valuestr() ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Param[%s] is unknown", e.fieldName() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDReelectLocation::_checkExtraArgs( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      // Check and make sure location is not null or ""
      if ( NULL == _location || '\0' == _location[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Failed to get location of reelect" ) ;
         goto error ;
      }

      /// Update group info
      if ( !_updatedGrpInfo )
      {
         _updatedGrpInfo = TRUE ;
         rc = _pResource->updateGroupInfo( _groupName, _groupInfoPtr, cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update group[%s] info failed, rc: %d",
                    _groupName, rc ) ;
            goto error ;
         }
      }

      // Check and make sure LocationID is valid
      _locationID = _groupInfoPtr->getLocationID( _location ) ;
      if ( MSG_INVALID_LOCATIONID == _locationID )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Location:[%s] doesn't exist", _location ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDReelectLocation::_getNotifyNode( MsgRouteID& nodeID,
                                                   const UINT16 tmpNodeID,
                                                   const CHAR* hostName,
                                                   const CHAR* svcName )
   {
      INT32 rc = SDB_OK ;

      clsNodeItem *pItem = NULL ;
      UINT32 pos = 0 ;

      // The selected node's location should also be matched
      while ( NULL != ( pItem = _groupInfoPtr->nodeItemByPos( pos++ ) ) )
      {
         if ( 0 != tmpNodeID )
         {
            if ( pItem->_id.columns.nodeID == tmpNodeID )
            {
               if ( pItem->_locationID == _locationID )
               {
                  nodeID.value = pItem->_id.value ;
                  break ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Node:[%u] is not in location[%s]",
                              nodeID.columns.nodeID, _location ) ;
                  goto error ;
               }
            }
         }
         else if ( *hostName )
         {
            if ( pItem->_locationID == _locationID &&
                 0 == ossStrcmp( hostName, pItem->_host ) &&
                 ( !*svcName || 0 == ossStrcmp( svcName,
                   pItem->_service[ MSG_ROUTE_LOCAL_SERVICE ].c_str() ) ) )
            {
               nodeID.value = pItem->_id.value ;
               break ;
            }
         }
         else if ( pItem->_locationID == _locationID && *svcName &&
                   0 == ossStrcmp( svcName, pItem->_service[ MSG_ROUTE_LOCAL_SERVICE ].c_str() ) )
         {
            nodeID.value = pItem->_id.value ;
            break ;
         }
      }

      if ( 0 == nodeID.value )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDReelectLocation::_buildNotifyMsg( CHAR **ppBuffer,
                                                    INT32 *bufferSize,
                                                    const MsgRouteID &nodeID,
                                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      BSONObj locationObj ;
      BSONObj hintObj ;
      BSONObjBuilder hintBuilder ;

      // Build query and hint obj
      hintBuilder.appendBool( FIELD_NAME_ISDESTINATION, TRUE ) ;
      hintObj = hintBuilder.obj() ;
      locationObj = BSON( FIELD_NAME_LOCATION << _location ) ;

      rc = msgBuildQueryMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_REELECT_LOCATION,
                             0, 0, 0, -1,
                             &locationObj, NULL, NULL, &hintObj,
                             cb ) ;

      return rc ;
   }

   INT32 _coordCMDReelectLocation::_reelect( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             rtnContextBuf *buf )
   {
      INT32          rc = SDB_OK ;
      INT32          rcTmp = SDB_OK ;
      INT32          rcInner = SDB_OK ;

      SET_ROUTEID    sendNodes ;
      ROUTE_RC_MAP   faileds ;
      MsgRouteID     nodeID ;
      UINT32         retryTimes = 0 ;
      UINT32         pos = 0 ;
      BOOLEAN        needRetry = FALSE ;

   retry:
      needRetry = FALSE ;

      // Get primary nodeID
      nodeID = _groupInfoPtr->primary( MSG_ROUTE_SHARD_SERVCIE, _locationID ) ;
      if ( MSG_INVALID_ROUTEID == nodeID.value && !_updatedGrpInfo )
      {
         _updatedGrpInfo = TRUE ;
         _pResource->updateGroupInfo( _groupName, _groupInfoPtr, cb ) ;
         nodeID = _groupInfoPtr->primary( MSG_ROUTE_SHARD_SERVCIE, _locationID ) ;
      }

      // The location primary node info may be updated by other slave nodes,
      // if the updated location primary node has already stopped, there is no need to execute on it.
      if ( MSG_INVALID_ROUTEID != nodeID.value )
      {
         INT32 primaryPos = _groupInfoPtr->nodePos( nodeID.columns.nodeID ) ;
         if ( primaryPos == SDB_CLS_NODE_NOT_EXIST || 
               _groupInfoPtr->isNodeInStatus( primaryPos, NET_NODE_STAT_OFFLINE ) )
         {
            nodeID.value = MSG_INVALID_ROUTEID ;
         }
      }

      // If location has no primary node, match other location slave node
      if ( MSG_INVALID_ROUTEID == nodeID.value )
      {
         const VEC_NODE_INFO* pNodes = _groupInfoPtr->getNodes() ;
         UINT32 nodeSize = pNodes->size() ;

         for ( UINT32 i = 0 ; i < nodeSize ; ++i, ++pos )
         {
            pos %= nodeSize ;
            if ( _locationID == _groupInfoPtr->nodeLocationID( pos ) &&
                 _groupInfoPtr->isNodeInStatus( pos, NET_NODE_STAT_NORMAL ) )
            {
               nodeID = (*pNodes)[ pos ]._id ;
               nodeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
               break ;
            }
         }
      }

      // LocationID is valid, but there is no node in the same location,
      // which means the SYSCAT.SYSNODES has some error, or these node are not in normal status
      if ( MSG_INVALID_ROUTEID == nodeID.value )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
         PD_LOG( PDWARNING, "There is no normal status node in location[%s]", _location ) ;
         goto error ;
      }
      // Execute on node
      else
      {
         sendNodes.clear() ;
         faileds.clear() ;

         sendNodes.insert( nodeID.value ) ;
         rc = executeOnNodes( pMsg, cb, sendNodes, faileds, NULL, NULL, NULL ) ;

         if ( faileds.size() > 0 )
         {
            rcTmp = faileds[nodeID.value]._rc ;
         }
      }

      // Handle rc and rcTmp
      if ( SDB_OK != rc )
      {
         // Only waitReply1() in executeOnNodes() can change rc in this case
         PD_LOG( PDERROR, "Failed to recieve replys from node[%s], rc: %d",
                 routeID2String( nodeID.value ).c_str(), rcTmp ) ;
         needRetry = TRUE ;
      }
      else if ( SDB_CLS_NOT_LOCATION_PRIMARY == rcTmp )
      {
         UINT32 newPrimaryID = faileds[nodeID.value]._startFrom ;
         if ( INVALID_NODEID != newPrimaryID )
         {
            MsgRouteID primaryNodeID ;
            primaryNodeID.value = nodeID.value ;
            primaryNodeID.columns.nodeID = newPrimaryID ;
            rcInner = _groupInfoPtr->updateLocationPrimary( primaryNodeID, _locationID ) ;
            if ( SDB_OK != rcInner )
            {
               rcTmp = rcInner ;
               PD_LOG( PDWARNING, "Failed to update location primary node, rc: %d",
                       nodeID.columns.groupID, rcTmp ) ;
            }
            needRetry = TRUE ;
         }
      }
      else if ( SDB_INVALID_ROUTEID == rcTmp )
      {
         rcInner = _pResource->updateGroupInfo( _groupName, _groupInfoPtr, cb ) ;
         if ( SDB_OK != rcInner )
         {
            rcTmp = rcInner ;
            PD_LOG( PDWARNING, "Update group[%u] info from remote failed, "
                    "rc: %d", nodeID.columns.groupID, rcTmp ) ;
         }
         else
         {
            needRetry = TRUE ;
         }
      }
      else if ( SDB_NET_CANNOT_CONNECT == rcTmp )
      {
         needRetry = TRUE ;
      }

      // Check and increase retryTimes
      if ( needRetry )
      {
         if ( COORD_OPR_MAX_RETRY_TIMES_DFT >= ++retryTimes )
         {
            goto retry ;
         }
      }
      rc = rc ? rc : rcTmp ;

   done:
      if ( ( rc || faileds.size() > 0 ) && buf )
      {
         *buf = _rtnContextBuf( coordBuildErrorObj( _pResource, rc,
                                                    cb, &faileds,
                                                    sendNodes.size() ) ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /* 
      _coordCMDAlterRG implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAlterRG,
                                      CMD_NAME_ALTER_GROUP,
                                      FALSE ) ;
   _coordCMDAlterRG::_coordCMDAlterRG()
   : _groupID( INVALID_GROUPID ),
     _pActionName( NULL )
   {
   }

   _coordCMDAlterRG::~_coordCMDAlterRG()
   {
   }


   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_PARSE_MSG, "_coordCMDAlterRG::_parseMsg" )
   INT32 _coordCMDAlterRG::_parseMsg( MsgHeader *pMsg,
                                      coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_ALTERRG_PARSE_MSG ) ;

      try
      {
         const BSONObj &queryObj = pArgs->_boQuery ;
         BSONElement ele ;

         // Get action name
         ele = queryObj.getField( FIELD_NAME_ACTION ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_ACTION, queryObj.toPoolString().c_str() ) ;
            goto error ;
         }
         _pActionName = ele.valuestrsafe() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_ALTERRG_PARSE_MSG, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordCMDAlterRG::_generateCataMsg( MsgHeader *pMsg,
                                             pmdEDUCB *cb,
                                             coordCMDArguments *pArgs,
                                             CHAR **ppMsgBuf,
                                             INT32 *pBufSize )
   {
      *ppMsgBuf = (CHAR*)pMsg ;
      *pBufSize = pMsg->messageLength ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_DO_ON_DATA, "_coordCMDAlterRG::_doOnDataGroup" )
   INT32 _coordCMDAlterRG::_doOnDataGroup( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           rtnContextCoord::sharePtr *ppContext,
                                           coordCMDArguments *pArgs,
                                           const CoordGroupList &groupLst,
                                           const vector<BSONObj> &cataObjs,
                                           CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERRG_DO_ON_DATA ) ;

      try
      {
         // ReplyObj inculde: { GroupID: 1001 }
         const BSONObj &replyObj = cataObjs[0] ;
         BSONElement groupIDEle ;

         // Get groupID, used to notify group nodes to update catalog info
         groupIDEle = replyObj.getField( FIELD_NAME_GROUPID ) ;
         if ( groupIDEle.eoo() || ! groupIDEle.isNumber() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get field[%s] from cata reply: %s",
                    FIELD_NAME_GROUPID, replyObj.toPoolString().c_str() ) ;
            goto error ;
         }
         _groupID = groupIDEle.numberInt() ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

      if ( 0 == ossStrcmp( CMD_VALUE_NAME_SET_ACTIVE_LOCATION, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_STOP_CRITICAL_MODE, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_SET_ATTRIBUTES, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_START_MAINTENANCE_MODE, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_STOP_MAINTENANCE_MODE, _pActionName ) )
      {
         // do nothing
      }
      else if ( 0 == ossStrcmp( CMD_VALUE_NAME_START_CRITICAL_MODE, _pActionName ) )
      {
         rc = _startCriticalModeOnData( pMsg, cb, cataObjs ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to do action[%s] on data nodes, rc: %d",
                      _pActionName, rc ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to alter node, received unknown action[%s]", _pActionName ) ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_ALTERRG_DO_ON_DATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_DO_COMPLETE, "_coordCMDAlterRG::_doComplete" )
   INT32 _coordCMDAlterRG::_doComplete( MsgHeader *pMsg,
                                        pmdEDUCB * cb,
                                        coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERRG_DO_COMPLETE ) ;

      if ( 0 == ossStrcmp( CMD_VALUE_NAME_SET_ACTIVE_LOCATION, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_STOP_CRITICAL_MODE, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_START_CRITICAL_MODE, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_SET_ATTRIBUTES, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_START_MAINTENANCE_MODE, _pActionName ) ||
           0 == ossStrcmp( CMD_VALUE_NAME_STOP_MAINTENANCE_MODE, _pActionName ) )
      {
         coordNodeCMDHelper helper ;

         if ( CATALOG_GROUPID == _groupID ||
              ( DATA_GROUP_ID_BEGIN <= _groupID && DATA_GROUP_ID_END >= _groupID ) )
         {
            // Notify all group nodes to update group info in clsReplicateSet
            helper.notify2GroupNodes( _pResource, _groupID, cb ) ;
         }
         else
         {
            // Only cata and data groupID are valid
            PD_LOG( PDWARNING, "Failed to notify to group in [%s] command ,"
                    "got invalid groupID: %d", _pActionName, _groupID ) ;
         }

         if ( CATALOG_GROUPID == _groupID )
         {
            // Notify all group nodes to update group info in _clsShardMgr
            helper.notify2AllNodes( _pResource, TRUE, cb ) ;
         }
         
         {
            std::string groupName ;
            rc = _pResource->groupID2Name( _groupID, groupName ) ;
            if ( SDB_OK == rc && !groupName.empty() )
            {
               coordCacheInvalidator cacheInvalidator( _pResource ) ;
               rc = cacheInvalidator.notify( COORD_CACHE_GROUP,
                                             groupName.c_str(),
                                             cb ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDWARNING, "Failed to invalidate coord group cache, rc:%d", rc ) ;
                  rc = SDB_OK ;
               }
            }
            else
            {
               // Failure to fetch the group name means the coord cache has been invalidated.
               rc = SDB_OK ;
            }
         }
      }

      PD_TRACE_EXITRC ( COORD_ALTERRG_DO_COMPLETE, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_EXE_BY_LOCATION, "_coordCMDAlterRG::_executeByLocation" )
   INT32 _coordCMDAlterRG::_executeByLocation( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               UINT32 locationID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERRG_EXE_BY_LOCATION ) ;

      SET_ROUTEID sendNodes ;
      ROUTE_RC_MAP faileds ;

      try
      {
         // Get nodes in location
         UINT32 nodeCount = _groupInfoPtr->nodeCount() ;
         for ( UINT32 pos = 0 ; pos < nodeCount ; ++pos )
         {
            MsgRouteID nodeID ;

            if ( locationID == _groupInfoPtr->nodeLocationID( pos ) &&
                 _groupInfoPtr->isNodeInStatus( pos, NET_NODE_STAT_NORMAL ) )
            {
               _groupInfoPtr->getNodeID( pos, nodeID ) ;
               sendNodes.insert( nodeID.value ) ;
            }
         }

         rc = executeOnNodes( pMsg, cb, sendNodes, faileds ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to send msg to data node, occured exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_ALTERRG_EXE_BY_LOCATION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_EXE_BY_NODE, "_coordCMDAlterRG::_executeByNode" )
   INT32 _coordCMDAlterRG::_executeByNode( MsgHeader *pMsg,
                                           pmdEDUCB *cb,
                                           UINT16 nodeID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERRG_EXE_BY_NODE ) ;

      MsgRouteID node ;
      SET_ROUTEID sendNodes ;
      ROUTE_RC_MAP faileds ;

      try
      {
         node.columns.groupID = _groupID ;
         node.columns.nodeID = nodeID ;
         node.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
         sendNodes.insert( node.value ) ;

         rc = executeOnNodes( pMsg, cb, sendNodes, faileds ) ;
         PD_RC_CHECK( rc, PDERROR, "Execute on nodes failed, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to send msg to data node, occured exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_ALTERRG_EXE_BY_NODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_REELECT_GROUP, "_coordCMDAlterRG::_reelectGroup" )
   INT32 _coordCMDAlterRG::_reelectGroup( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERRG_REELECT_GROUP ) ;

      coordCMDReelectGroup cmd ;

      CHAR *msgBuff = NULL ;
      INT32 msgSize = 0 ;
      BSONObj query ;
      INT64 contextID = -1 ;

      try
      {
         query = BSON( FIELD_NAME_GROUPNAME << _groupInfoPtr->groupName().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to build reelect command query, occured exception: %s", e.what() ) ;
         goto error ;
      }

      rc = msgBuildQueryMsg( &msgBuff, &msgSize, CMD_ADMIN_PREFIX CMD_NAME_REELECT,
                             0, 0, 0, -1, &query, NULL, NULL, NULL, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build reelect command message, rc: %d", rc ) ;

      rc = cmd.init( _pResource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init reelect command, rc: %d", rc ) ;

      rc = cmd.execute( (MsgHeader *)msgBuff, cb, contextID,  NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reelect group[%u], rc: %d", _groupID, rc ) ;
      SDB_ASSERT( -1 == contextID, "contextID must be -1" ) ;

   done:
      if ( NULL != msgBuff )
      {
         msgReleaseBuffer( msgBuff, cb ) ;
      }
      PD_TRACE_EXITRC ( COORD_ALTERRG_REELECT_GROUP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CHECK_CRITICAL_MODE, "_coordCMDAlterRG::_checkCriticalMode" )
   INT32 _coordCMDAlterRG::_checkCriticalMode( pmdEDUCB *cb,
                                               const UINT16 &nodeID,
                                               const UINT32 &locationID,
                                               const UINT32 &waitTime )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CHECK_CRITICAL_MODE ) ;

      UINT32 timePassed = 0 ;
      UINT32 checkInterval = 2 ; // Seconds

      while ( timePassed < waitTime )
      {
         BOOLEAN isSuccessful = FALSE ;

         rc = _pResource->updateGroupInfo( _groupID, _groupInfoPtr, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Update group[%u] info failed, rc: %d", _groupID, rc ) ;

         // Check if targetNode is replica group's primary
         if ( INVALID_NODEID != nodeID )
         {
            if ( nodeID == _groupInfoPtr->primary().columns.nodeID )
            {
               isSuccessful = TRUE ;
            }
         }
         // Check if replica group's primary is in target location
         else if ( MSG_INVALID_LOCATIONID != locationID )
         {
            UINT32 nodeCount = _groupInfoPtr->nodeCount() ;
            MsgRouteID primary = _groupInfoPtr->primary() ;

            if (  MSG_INVALID_ROUTEID != primary.value )
            {
               for ( UINT32 pos = 0 ; pos < nodeCount ; ++pos )
               {
                  if ( locationID == _groupInfoPtr->nodeLocationID( pos ) )
                  {
                     MsgRouteID node ;
                     _groupInfoPtr->getNodeID( pos, node ) ;

                     if ( node.value == primary.value )
                     {
                        isSuccessful = TRUE ;
                        break ;
                     }
                  }
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get parameter [%s] or [%s] from cata node",
                    FIELD_NAME_NODEID, FIELD_NAME_LOCATION ) ;
            goto error ;
         }

         if ( isSuccessful )
         {
            break ;
         }

         // Sleep
         ossSleepsecs( checkInterval ) ;
         timePassed += checkInterval ;
      }

      if ( waitTime <= timePassed )
      {
         rc = SDB_TIMEOUT ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_CHECK_CRITICAL_MODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ALTERRG_START_CRITICAL_MODE_ON_DATA, "_coordCMDAlterRG::_startCriticalModeOnData" )
   INT32 _coordCMDAlterRG::_startCriticalModeOnData( MsgHeader *pMsg,
                                                     pmdEDUCB *cb,
                                                     const vector<BSONObj> &cataObjs )
   {
      /* 
         This function do the following steps:
         1. If starting critical mode in cata, do nothing
         2. Check and get the nodes from cata which will be effective in critical mode
         2. Send start critical mode msg to these nodes
         4. Execute a reelect group command, and check rc
         5. Check if critical mode has started successfully
       */

      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_ALTERRG_START_CRITICAL_MODE_ON_DATA ) ;

      UINT16 tarNodeID = INVALID_NODEID ;
      UINT32 tarLocationID = MSG_INVALID_LOCATIONID ;

      if ( CATALOG_GROUPID == _groupID )
      {
         // If start critical mode in catalog group, the effective nodes must in cata primary,
         // we don't need to do on data slave nodes.
         goto done ;
      }

      rc = _pResource->updateGroupInfo( _groupID, _groupInfoPtr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Update group[%u] info failed, rc: %d", _groupID, rc ) ;

      try
      {
         // Get effective node/location, we don't need to check if the nodeID or locationID is valid,
         // because these parameters has been validated in catalog node
         if ( ! cataObjs.empty() )
         {
            // replyObj: { LocationID: 1 } | { NodeID: 1000 }
            const BSONObj &replyObj = cataObjs[0] ;
            BSONElement replyEle ;

            if ( replyObj.hasField( FIELD_NAME_NODEID ) )
            {
               // Get effective nodeID
               replyEle = replyObj.getField( FIELD_NAME_NODEID ) ;
               if ( ! replyEle.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Failed to get [%s] from cata reply, type[%d] is not Number",
                          FIELD_NAME_NODEID, replyEle.type() ) ;
                  goto error ;
               }
               tarNodeID = replyEle.numberInt() ;
            }
            else if ( replyObj.hasField( FIELD_NAME_NODE_LOCATIONID ) )
            {
               // Get effective locationID
               replyEle = replyObj.getField( FIELD_NAME_NODE_LOCATIONID ) ;
               if ( ! replyEle.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Failed to get [%s] from cata reply, type[%d] is not Number",
                          FIELD_NAME_NODE_LOCATIONID, replyEle.type() ) ;
                  goto error ;
               }
               tarLocationID = replyEle.numberInt() ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Failed to get parameter [%s] or [%s] from cata node",
                       FIELD_NAME_NODEID, FIELD_NAME_LOCATION ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

      // Send start critical mode msg to data nodes
      if ( INVALID_NODEID != tarNodeID )
      {
         rc = _executeByNode( pMsg, cb, tarNodeID ) ;
      }
      else if ( MSG_INVALID_LOCATIONID != tarLocationID )
      {
         rc = _executeByLocation( pMsg, cb, tarLocationID ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to start critical mode on data nodes, rc: %d", rc ) ;

      // Execute a reelect command, and ignore the rc: SDB_CLS_NOT_PRIMARY,
      // because some data group may not have primary
      rc = _reelectGroup( cb ) ;
      if ( SDB_CLS_NOT_PRIMARY == rc )
      {
         // If data group don't have primary, need to wait at most 30s
         rc = _checkCriticalMode( cb, tarNodeID, tarLocationID, CLS_REELECT_COMMAND_TIMEOUT_DFT ) ;
      }
      else
      {
         // Wait at most 10s
         rc = _checkCriticalMode( cb, tarNodeID, tarLocationID, 10 ) ;
      }

      // If start critical mode successfully, we need to notify cata to
      // update the properties of critical mode to SYSCAT table, and notify
      // data nodes to update group info from cata
      if ( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to %s, rc: %d", CMD_VALUE_NAME_START_CRITICAL_MODE, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( COORD_ALTERRG_START_CRITICAL_MODE_ON_DATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

