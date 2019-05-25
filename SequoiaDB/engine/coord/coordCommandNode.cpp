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

   void _coordNodeCMD2Phase::_releaseDataMsg( CHAR *pMsgBuf,
                                              INT32 bufSize,
                                              pmdEDUCB *cb )
   {
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

   INT32 _coordNodeCMD2Phase::notifyCatalogChange2AllNodes( pmdEDUCB *cb,
                                                            BOOLEAN exceptSelf )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      MsgHeader ntyMsg ;
      CoordGroupList grpLst ;
      SET_ROUTEID nodes ;
      pmdRemoteSessionSite *pSite = NULL ;
      pmdRemoteSession *pSession = NULL ;
      coordRemoteHandlerBase baseHander ;
      pmdSubSession *pSub           = NULL ;
      SET_ROUTEID::iterator it ;

      ntyMsg.messageLength = sizeof( MsgHeader ) ;
      ntyMsg.opCode = MSG_CAT_GRP_CHANGE_NTY ;

      pSite = (pmdRemoteSessionSite*)cb->getRemoteSite() ;
      if ( !pSite )
      {
         PD_LOG( PDERROR, "Remote session is NULL in cb" ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      pSession = pSite->addSession( getTimeout(), &baseHander ) ;
      if ( !pSession )
      {
         PD_LOG( PDERROR, "Create remote session failed in session[%s]",
                 cb->getName() ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _pResource->updateGroupList( grpLst, cb, NULL, FALSE, FALSE, TRUE ) ;

      rc = coordGetGroupNodes( _pResource, cb, BSONObj(), NODE_SEL_ALL,
                               grpLst, nodes ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get nodes, rc: %d", rc ) ;
      if ( nodes.size() == 0 )
      {
         PD_LOG( PDWARNING, "Not found any node" ) ;
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }
      if ( exceptSelf )
      {
         MsgRouteID routeID ;
         routeID = pmdGetNodeID() ;
         routeID.columns.serviceID = MSG_ROUTE_SHARD_SERVCIE ;
         nodes.erase( routeID.value ) ;
      }

      it = nodes.begin() ;
      while( it != nodes.end() )
      {
         pSub = pSession->addSubSession( *it ) ;
         pSub->setReqMsg( &ntyMsg, PMD_EDU_MEM_NONE ) ;

         rcTmp = pSession->sendMsg( pSub ) ;
         ++it ;

         if ( rcTmp && !rc )
         {
            rc = rcTmp ;
         }
      }

   done:
      if ( pSession )
      {
         pSite->removeSession( pSession->sessionID() ) ;
      }
      return rc ;
   error:
      goto done ;
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
                                                 rtnContextCoord **ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &pGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_NODE3PHASE_DOONCATAP2 ) ;

      rc = _processContext( cb, ppContext, 1 ) ;

      PD_TRACE_EXITRC ( COORD_NODE3PHASE_DOONCATAP2, rc ) ;

      return rc ;
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

   // PD_TRACE_DECLARE_FUNCTION( COORD_CMDOPONGROUP_OPON1NODE, "_coordCMDOpOnGroup::_opOnNodes" )
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
                                         OP_ERRNOFIELD << retCode ) ) ;
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

            rc = _opOnOneNode( opList, hostName, svcName, dataObjs ) ;
         }

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
         BSONObj boInput( pQuery ) ;
         BSONObjBuilder bobNodeConf( boInput.objsize() + 128 ) ;
         BSONObjIterator iter( boInput ) ;

         BOOLEAN hasCatalogAddrKey = FALSE ;
         BOOLEAN hasClusterName = FALSE ;
         BOOLEAN hasBusinessName = FALSE ;

         const CHAR *pFieldName = NULL ;

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

            bobNodeConf.append( beField );
         } /// end while

         roleStr = utilDBRoleStr( role ) ;
         if ( *roleStr == 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( !hasClusterName )
         {
            string strName ;
            optCB->getFieldStr( PMD_OPTION_CLUSTER_NAME, strName, "" ) ;
            if ( !strName.empty() )
            {
               bobNodeConf.append( PMD_OPTION_CLUSTER_NAME, strName ) ;
            }
         }

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

      builder.append ( PMD_OPTION_ROLE, roleStr ) ;

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

      contextID = -1 ;


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

      CHAR *pQuery = NULL ;
      BSONObj boNodeConfig ;
      BSONObj boNodeInfo ;

      SINT32 retCode = 0 ;
      BSONObj boBackup = BSON( "Backup" << true ) ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL,
                            NULL, &pQuery, NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse message, rc: %d", rc ) ;

      rc = getNodeConf( _pResource, pQuery, boNodeConfig ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node config, rc: %d", rc ) ;

      rc = getNodeInfo( pQuery, boNodeInfo ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get node info, rc: %d", rc ) ;

      if ( !_pResource->addCataNodeAddrWhenEmpty( _pHostName,
                                                  _cataSvcName.c_str() ) )
      {
         rc = SDB_COORD_RECREATE_CATALOG ;
         goto error ;
      }

      rc = rtnRemoteExec( SDBADD, _pHostName, &retCode,
                          &boNodeConfig, &boNodeInfo ) ;
      rc = rc ? rc : retCode ;
      PD_RC_CHECK( rc, PDERROR, "Do remote execute on node[%s:%s] "
                   "failed, rc: %d", _pHostName, _pSvcName, rc ) ;

      rc = rtnRemoteExec( SDBSTART, _pHostName, &retCode, &boNodeConfig ) ;
      rc = rc ? rc : retCode ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Do remote execute on node[%s:%s] "
                 "failed, rc: %d", _pHostName, _pSvcName, rc ) ;
         rtnRemoteExec( SDBRM, _pHostName, &retCode,
                        &boNodeConfig, &boBackup ) ;
         goto error ;
      }

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
      CHAR *pQuery                     = NULL ;
      MsgOpQuery *pCreateReq           = (MsgOpQuery *)pMsg ;

      PD_TRACE_ENTRY ( COORD_CREATEGROUP_EXE ) ;

      contextID                        = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL,
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

      pCreateReq->header.opCode = MSG_CAT_CREATE_GROUP_REQ ;
      rc = executeOnCataGroup ( pMsg, cb, TRUE, NULL, NULL, buf ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to execute on catalog, rc: %d", rc ) ;
         goto error ;
      }

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

   void _coordCMDActiveGroup::_releaseCataMsg( CHAR *pMsgBuf,
                                               INT32 bufSize,
                                               pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_ACTIVEGRP_DOONCATAGROUP, "_coordCMDActiveGroup::_doOnCataGroup" )
   INT32 _coordCMDActiveGroup::_doOnCataGroup( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord **ppContext,
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
                                                rtnContextCoord **ppContext,
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

   void _coordCMDShutdownGroup::_releaseCataMsg( CHAR *pMsgBuf,
                                                 INT32 bufSize,
                                                 pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_SHUTDOWNGRP_DOONDATA, "_coordCMDShutdownGroup::_doOnDataGroup" )
   INT32 _coordCMDShutdownGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                  pmdEDUCB *cb,
                                                  rtnContextCoord **ppContext,
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
                                             rtnContextCoord **ppContext,
                                             coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_SHUTDOWNGRP_DOCOMMIT ) ;

      rc = _coordNodeCMD2Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      if ( rc )
      {
         if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
         {
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

         rc = rtnGetBooleanElement( pArgs->_boQuery, CMD_NAME_ENFORCED,
                                    _enforce ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CMD_NAME_ENFORCED, getName(), rc ) ;
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

   void _coordCMDRemoveGroup::_releaseCataMsg( CHAR *pMsgBuf,
                                               INT32 bufSize,
                                               pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOGEGRP_DOONDATA, "_coordCMDRemoveGroup::_doOnDataGroup" )
   INT32 _coordCMDRemoveGroup::_doOnDataGroup ( MsgHeader *pMsg,
                                                pmdEDUCB *cb,
                                                rtnContextCoord **ppContext,
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
                                                  rtnContextCoord **ppContext,
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
                                           rtnContextCoord **ppContext,
                                           coordCMDArguments *pArgs )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY ( COORD_REMOGEGRP_DOCOMMIT ) ;

      rc = _coordNodeCMD3Phase::_doCommit( pMsg, cb, ppContext, pArgs ) ;
      if ( rc )
      {
         if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
         {
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

         ele = pArgs->_boQuery.getField( CAT_HOST_FIELD_NAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_HOST_FIELD_NAME, getName(), rc ) ;
            goto error ;
         }
         _pHostName = ele.valuestr() ;

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

         ele = pArgs->_boQuery.getField( PMD_OPTION_SVCNAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", PMD_OPTION_SVCNAME, getName(), rc ) ;
            goto error ;
         }
         _pSvcName = ele.valuestr() ;

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
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                       "rc: %d", FIELD_NAME_KEEP_DATA, getName(), rc ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            ele = pArgs->_boQuery.getField( PMD_OPTION_DBPATH ) ;
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                       "rc: %d", PMD_OPTION_DBPATH, getName(), rc ) ;
               goto error ;
            }

            ele = pArgs->_boQuery.getField( PMD_OPTION_INSTANCE_ID ) ;
            if ( ele.eoo() || ele.isNumber() )
            {
               UINT32 instanceID = ele.isNumber() ? ele.numberInt() :
                                                    NODE_INSTANCE_ID_UNKNOWN ;
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

   void _coordCMDCreateNode::_releaseCataMsg( CHAR *pMsgBuf,
                                              INT32 bufSize,
                                              pmdEDUCB *cb )
   {
      if ( pMsgBuf && _onlyAttach )
      {
         msgReleaseBuffer( pMsgBuf, cb ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_CREATENODE_DOONDATA, "_coordCMDCreateNode::_doOnDataGroup" )
   INT32 _coordCMDCreateNode::_doOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord **ppContext,
                                               coordCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs,
                                               CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_CREATENODE_DOONDATA ) ;

      if ( !_onlyAttach )
      {
         INT32 retCode = SDB_OK ;
         rc = rtnRemoteExec( SDBTEST, _pHostName, &retCode ) ;
         PD_RC_CHECK( rc, PDERROR, "Do remote test on node[%s:%s] failed, "
                      "command[%s, targe:%s], rc: %d", _pHostName,
                      _pSvcName, getName(), pArgs->_targetName.c_str(),
                      rc) ;
      }
      else
      {
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
                                                 rtnContextCoord **ppContext,
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

      _pResource->updateGroupInfo( pArgs->_targetName.c_str(),
                                   groupPtr, cb ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         notifyCatalogChange2AllNodes( cb, TRUE ) ;
      }

      PD_TRACE_EXIT ( COORD_CREATENODE_COMPLETE ) ;
      return SDB_OK ;
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

         ele = pArgs->_boQuery.getField( CAT_HOST_FIELD_NAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CAT_HOST_FIELD_NAME, getName(), rc ) ;
            goto error ;
         }
         _pHostName = ele.valuestr() ;

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

         rc = rtnGetBooleanElement( pArgs->_boQuery, CMD_NAME_ENFORCED,
                                    _enforce ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", CMD_NAME_ENFORCED, getName(), rc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ele = pArgs->_boQuery.getField( PMD_OPTION_SVCNAME ) ;
         if ( String != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                    "rc: %d", PMD_OPTION_SVCNAME, getName(), rc ) ;
            goto error ;
         }
         _pSvcName = ele.valuestr() ;

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
            if ( SDB_FIELD_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Get field[%s] failed on command[%s], "
                       "rc: %d", FIELD_NAME_KEEP_DATA, getName(), rc ) ;
               rc = SDB_INVALIDARG ;
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

   void _coordCMDRemoveNode::_releaseCataMsg( CHAR *pMsgBuf,
                                              INT32 bufSize,
                                              pmdEDUCB *cb )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REMOVENODE_DOONDATA, "_coordCMDRemoveNode::_doOnDataGroup" )
   INT32 _coordCMDRemoveNode::_doOnDataGroup ( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               rtnContextCoord **ppContext,
                                               coordCMDArguments *pArgs,
                                               const CoordGroupList &groupLst,
                                               const vector<BSONObj> &cataObjs,
                                               CoordGroupList &sucGroupLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_REMOVENODE_DOONDATA ) ;

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
                                                 rtnContextCoord **ppContext,
                                                 coordCMDArguments *pArgs,
                                                 const CoordGroupList &groupLst,
                                                 const vector<BSONObj> &cataObjs )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( COORD_REMOVENODE_DOONDATA2 ) ;

      INT32 retCode = SDB_OK ;

      _notify2GroupNodes( cb, pArgs ) ;

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

      _pResource->updateGroupInfo( pArgs->_targetName.c_str(),
                                   groupPtr, cb ) ;

      if ( 0 == pArgs->_targetName.compare( CATALOG_GROUPNAME ) )
      {
         notifyCatalogChange2AllNodes( cb, TRUE ) ;
      }

      PD_TRACE_EXIT ( COORD_REMOVENODE_DOCOMPLETE ) ;
      return SDB_OK ;
   }

   void _coordCMDRemoveNode::_notify2GroupNodes( pmdEDUCB *cb,
                                                 coordCMDArguments *pArgs )
   {
      CoordGroupInfoPtr groupPtr ;
      _netRouteAgent *pAgent = _pResource->getRouteAgent() ;

      if ( SDB_OK == _pResource->updateGroupInfo( pArgs->_targetName.c_str(),
                                                  groupPtr,
                                                  cb ) )
      {
         _MsgClsGInfoUpdated updated ;
         updated.groupID = groupPtr->groupID() ;

         MsgRouteID routeID ;
         UINT32 index = 0 ;

         while ( SDB_OK == groupPtr->getNodeID( index++, routeID,
                                                MSG_ROUTE_SHARD_SERVCIE ) )
         {
            pAgent->syncSend( routeID, (void*)&updated ) ;
         }
      }
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
      _coordCMDReelection implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDReelection,
                                      CMD_NAME_REELECT,
                                      TRUE ) ;
   _coordCMDReelection::_coordCMDReelection()
   {
   }

   _coordCMDReelection::~_coordCMDReelection()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( COORD_REELECT_EXE, "_coordCMDReelection::execute" )
   INT32 _coordCMDReelection::execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( COORD_REELECT_EXE ) ;
      CHAR *pQuery = NULL ;
      const CHAR *gpName = NULL ;
      CoordGroupInfoPtr gpInfo ;
      CoordGroupList gpLst ;

      contextID = -1 ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL,
                            NULL, NULL, &pQuery,
                            NULL, NULL, NULL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse the message, rc: %d", rc ) ;

      try
      {
         BSONObj query( pQuery ) ;
         BSONElement ele = query.getField( FIELD_NAME_GROUPNAME ) ;
         if ( String != ele.type() )
         {
            PD_LOG( PDERROR, "Invalid reelection msg:%s",
                    query.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         gpName = ele.valuestr() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected error happened:%s", e.what() ) ;
         goto error ;
      }

      rc = _pResource->getOrUpdateGroupInfo( gpName, gpInfo, cb ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Update group info by name[%s] failed, rc: %d",
                 gpName, rc ) ;
         goto error ;
      }

      gpLst[gpInfo->groupID()] = gpInfo->groupID() ;
      rc = executeOnDataGroup( pMsg, cb, gpLst, TRUE, NULL, NULL,
                               NULL, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to execute on group[%s], rc: %d",
                 gpName, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( COORD_REELECT_EXE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

