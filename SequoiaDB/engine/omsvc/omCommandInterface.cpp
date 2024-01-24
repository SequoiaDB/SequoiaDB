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

   Source File Name = omCommandInterface.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/12/2014  LYB Initial Draft

   Last Changed =

*******************************************************************************/

#include "omCommandInterface.hpp"
#include "omDef.hpp"
#include "rtn.hpp"
#include "msgMessage.hpp"


using namespace bson;

namespace engine
{
   omCommandInterafce::omCommandInterafce()
   {
   }

   omCommandInterafce::~omCommandInterafce()
   {
   }

   omRestCommandBase::omRestCommandBase( pmdRestSession *pRestSession,
                                         restAdaptor *pRestAdaptor, 
                                         restRequest *pRequest,
                                         restResponse *pResponse,
                                         const string &localAgentHost,
                                         const string &localAgentService,
                                         const string &rootPath )
   {
      _cb     = NULL ;

      _restSession = pRestSession ;
      _restAdaptor = pRestAdaptor ;
      _request     = pRequest ;
      _response    = pResponse ;

      _localAgentHost    = localAgentHost ;
      _localAgentService = localAgentService ;
      _rootPath          = rootPath ;
   }

   omRestCommandBase::~omRestCommandBase()
   {
   }

   INT32 omRestCommandBase::init( pmdEDUCB * cb )
   {
      _cb = cb ;

      _pKRCB  = pmdGetKRCB() ;
      _pDMDCB = _pKRCB->getDMSCB() ;
      _pRTNCB = _pKRCB->getRTNCB() ;
      _pDMSCB = _pKRCB->getDMSCB() ;

      return SDB_OK ;
   }

   bool omRestCommandBase::isFetchAgentResponse( UINT64 requestID )
   {
      return false ;
   }

   INT32 omRestCommandBase::doAgentResponse ( MsgHeader* pAgentResponse )
   {
      return SDB_OK ;
   }

   INT32 omRestCommandBase::_queryTable( const string &tableName, 
                                         const BSONObj &selector, 
                                         const BSONObj &matcher,
                                         const BSONObj &order, 
                                         const BSONObj &hint, SINT32 flag,
                                         SINT64 numSkip, SINT64 numReturn, 
                                         list<BSONObj> &records )
   {
      INT32 rc         = SDB_OK ;
      SINT64 contextID = -1 ;
      rc = rtnQuery( tableName.c_str(), selector, matcher, order, hint, flag, 
                     _cb, numSkip, numReturn, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:name=%s,rc=%d", 
                     tableName.c_str(), rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            _pRTNCB->contextDelete( contextID, _cb ) ;
            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:name=%s,"
                        "rc=%d", tableName.c_str(), rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         records.push_back( result.copy() );
      }

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_getBusinessInfo( const string &business, 
                                              BSONObj &businessInfo )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      matcher = BSON( OM_BUSINESS_FIELD_NAME << business ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         BSONObjBuilder innerBuilder ;
         BSONObj tmp ;
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               break ;
            }
            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d", 
                        OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         businessInfo = result.copy() ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_getBusinessInfoOfCluster( const string &clusterName,
                                                       BSONObj &clusterBusinessInfo )
   {
      BSONObjBuilder bsonBuilder ;
      BSONArrayBuilder arrayBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      matcher = BSON( OM_BUSINESS_FIELD_CLUSTERNAME << clusterName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d", 
                        OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         arrayBuilder.append( result ) ;
      }

      bsonBuilder.append( OM_BSON_BUSINESS_INFO, arrayBuilder.arr() ) ;
      clusterBusinessInfo = bsonBuilder.obj() ;

   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_getHostInfo( string hostName, 
                                          BSONObj &hostInfo )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID = -1 ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint, 0, 
                     _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               break ;
            }
            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d", 
                        OM_CS_DEPLOY_CL_HOST, rc ) ;
            goto error ;
         }

         BSONObj record( buffObj.data() ) ;
         BSONObj filter = BSON( OM_HOST_FIELD_PASSWORD << "" ) ;
         BSONObj result = record.filterFieldsUndotted( filter, false ) ;
         hostInfo = result.copy() ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /**
   * get the host's disk info. the host is specific by the hostNameList
   * if hostNameList's size is 0, get all the cluster(clusterName )'s hosts 
     instead
   * @param clusterName the cluster's name
   * @param hostNameList the specific hosts
   * @param hostInfoList the result of  disk infos
   * @return SDB_OK if success; otherwise failure
   */
   INT32 omRestCommandBase::_fetchHostDiskInfo( const string &clusterName, 
                                            list<string> &hostNameList, 
                                            list<simpleHostDisk> &hostInfoList )
   {
      BSONObj matcher ;

      if ( hostNameList.size() > 0 )
      {
         BSONArrayBuilder arrayBuilder ;
         list<string>::iterator iterList = hostNameList.begin() ;
         while ( iterList != hostNameList.end() )
         {
            BSONObj tmp = BSON( OM_HOST_FIELD_NAME << *iterList ) ;
            arrayBuilder.append( tmp ) ;
            iterList++ ;
         }
         matcher = BSON( OM_HOST_FIELD_CLUSTERNAME << clusterName 
                         << "$or" << arrayBuilder.arr() ) ;
      }
      else
      {
         matcher = BSON( OM_HOST_FIELD_CLUSTERNAME << clusterName  ) ;
      }

      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:table=%s,rc=%d", 
                     OM_CS_DEPLOY_CL_HOST, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:table=%s,"
                        "rc=%d", OM_CS_DEPLOY_CL_HOST, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         BSONObj diskArray ;
         simpleHostDisk hostDisk ;
         hostDisk.hostName  = result.getStringField( OM_HOST_FIELD_NAME ) ;
         hostDisk.user      = result.getStringField( OM_HOST_FIELD_USER ) ;
         hostDisk.passwd    = result.getStringField( OM_HOST_FIELD_PASSWORD ) ;
         hostDisk.agentPort = result.getStringField( OM_HOST_FIELD_AGENT_PORT ) ;
         diskArray = result.getObjectField( OM_HOST_FIELD_DISK ) ;
         {
            BSONObjIterator iter( diskArray ) ;
            while ( iter.more() )
            {
               BSONElement ele = iter.next() ;
               BSONObj oneDisk = ele.embeddedObject() ;
               simpleDiskInfo diskInfo ;
               diskInfo.diskName  = oneDisk.getStringField( 
                                                    OM_HOST_FIELD_DISK_NAME ) ;
               diskInfo.mountPath = oneDisk.getStringField( 
                                                    OM_HOST_FIELD_DISK_MOUNT ) ;
               BSONElement eleNum ;
               eleNum = oneDisk.getField( OM_HOST_FIELD_DISK_SIZE ) ;
               diskInfo.totalSize = eleNum.numberLong() ;
               eleNum = oneDisk.getField( OM_HOST_FIELD_DISK_FREE_SIZE ) ;
               diskInfo.freeSize  = eleNum.numberLong() ;
               hostDisk.diskInfo.push_back( diskInfo ) ;
            }
         }

         hostInfoList.push_back( hostDisk ) ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_deleteHost( const string &hostName )
   {
      INT32 rc          = SDB_OK ;
      BSONObj condition = BSON( OM_HOST_FIELD_NAME << hostName ) ;
      BSONObj hint ;

      rc = rtnDelete( OM_CS_DEPLOY_CL_HOST, condition, hint, 0, _cb );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to delete record from table:%s,"
                     "%s=%s,rc=%d", OM_CS_DEPLOY_CL_HOST, 
                     OM_HOST_FIELD_NAME, hostName.c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_receiveFromAgent( pmdRemoteSession *remoteSession,
                                               SINT32 &flag,
                                               BSONObj &result )
   {
      VEC_SUB_SESSIONPTR subSessionVec ;
      INT32 rc           = SDB_OK ;
      MsgHeader *pRspMsg = NULL ;
      SINT64 contextID   = -1 ;
      SINT32 startFrom   = 0 ;
      SINT32 numReturned = 0 ;
      vector<BSONObj> objVec ;

      rc = remoteSession->waitReply( TRUE, &subSessionVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "wait reply failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( 1 != subSessionVec.size() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         PD_LOG( PDERROR, "unexpected session size:size=%d", 
                 subSessionVec.size() ) ;
         goto error ;
      }

      if ( subSessionVec[0]->isDisconnect() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         PD_LOG(PDERROR, "session disconnected:id=%s,rc=%d", 
                routeID2String(subSessionVec[0]->getNodeID()).c_str(), rc ) ;
         goto error ;
      }

      pRspMsg = subSessionVec[0]->getRspMsg() ;
      if ( NULL == pRspMsg )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         PD_LOG( PDERROR, "receive null response:rc=%d", rc ) ;
         goto error ;
      }

      rc = msgExtractReply( (CHAR *)pRspMsg, &flag, &contextID, &startFrom, 
                            &numReturned, objVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "extract reply failed:rc=%d", rc ) ;
         goto error ;
      }

      if ( objVec.size() > 0 )
      {
         result = objVec[0] ;
         result = result.getOwned() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_getAllReplay( pmdRemoteSession *remoteSession, 
                                           VEC_SUB_SESSIONPTR *subSessionVec )
   {
      SDB_ASSERT( NULL != remoteSession, "remoteSession can't be null" ) ;

      pmdSubSessionItr itr( NULL ) ;
      INT32 rc = SDB_OK ;
      VEC_SUB_SESSIONPTR tmpSessionVec ;
      rc = remoteSession->waitReply( TRUE, &tmpSessionVec ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "wait replay failed:rc=%d", rc ) ;
         goto error ;
      }

      itr = remoteSession->getSubSessionItr( PMD_SSITR_ALL ) ;
      while ( itr.more() )
      {
         subSessionVec->push_back( itr.next() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_getClusterInfo( const string &clusterName, 
                                             BSONObj &clusterInfo )
   {
      BSONObjBuilder bsonBuilder ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      BSONObj result ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;

      matcher = BSON( OM_CLUSTER_FIELD_NAME << clusterName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_CLUSTER, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "failed to get record from table:table=%s,rc=%d", 
                     OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               break ;
            }
            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:table=%s,"
                        "rc=%d", OM_CS_DEPLOY_CL_CLUSTER, rc ) ;
            goto error ;
         }

         BSONObj result( buffObj.data() ) ;
         clusterInfo = result.copy() ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 omRestCommandBase::_checkHostBasicContent( BSONObj &oneHost )
   {
      INT32 rc = SDB_INVALIDARG ;
      BSONElement ele ;

      ele = oneHost.getField( OM_BSON_FIELD_HOST_IP ) ;
      if ( ele.type() != String )
      {
         PD_LOG_MSG( PDERROR, "field is not String type:field=%s,type=%d", 
                     OM_BSON_FIELD_HOST_IP, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_HOST_NAME ) ;
      if ( ele.type() != String )
      {
         PD_LOG_MSG( PDERROR, "field is not String type:field=%s,type=%d", 
                     OM_BSON_FIELD_HOST_NAME, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_OS ) ;
      if ( ele.type() != Object )
      {
         PD_LOG_MSG( PDERROR, "field is not Object type:field=%s,type=%d", 
                     OM_BSON_FIELD_OS, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_OMA ) ;
      if ( ele.type() != Object )
      {
         PD_LOG_MSG( PDERROR, "field is not Object type:field=%s,type=%d", 
                     OM_BSON_FIELD_OMA, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_CPU ) ;
      if ( ele.type() != Array )
      {
         PD_LOG_MSG( PDERROR, "field is not Array type:field=%s,type=%d", 
                     OM_BSON_FIELD_CPU, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_MEMORY ) ;
      if ( ele.type() != Object )
      {
         PD_LOG_MSG( PDERROR, "field is not Object type:field=%s,type=%d", 
                     OM_BSON_FIELD_MEMORY, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_NET ) ;
      if ( ele.type() != Array )
      {
         PD_LOG_MSG( PDERROR, "field is not Array type:field=%s,type=%d", 
                     OM_BSON_FIELD_NET, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_PORT ) ;
      if ( ele.type() != Array )
      {
         PD_LOG_MSG( PDERROR, "field is not Array type:field=%s,type=%d", 
                     OM_BSON_FIELD_PORT, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_SAFETY ) ;
      if ( ele.type() != Object )
      {
         PD_LOG_MSG( PDERROR, "field is not Object type:field=%s,type=%d", 
                     OM_BSON_FIELD_SAFETY, ele.type() ) ;
         goto error ;
      }

      ele = oneHost.getField( OM_BSON_FIELD_DISK ) ;
      if ( ele.type() != Array )
      {
         PD_LOG_MSG( PDERROR, "field is not Array type:field=%s,type=%d", 
                     OM_BSON_FIELD_DISK, ele.type() ) ;
         goto error ;
      }

      rc = SDB_OK ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // if error happened when quering table, TRUE will be returned
   BOOLEAN omRestCommandBase::_isHostExistInTask( const string &hostName )
   {
      BSONObjBuilder bsonBuilder ;
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;
      rtnContextBuf buffObj ;

      // ResultInfo.$elemMatch.HostName == hostName 
      // && Status not in( OM_TASK_STATUS_FINISH, OM_TASK_STATUS_CANCEL )
      BSONObj hostNameObj = BSON( OM_HOST_FIELD_NAME << hostName ) ;
      BSONObj elemMatch   = BSON( "$elemMatch" << hostNameObj ) ;

      BSONArrayBuilder arrBuilder ;
      arrBuilder.append( OM_TASK_STATUS_FINISH ) ;
      arrBuilder.append( OM_TASK_STATUS_CANCEL ) ;
      BSONObj status = BSON( "$nin" << arrBuilder.arr() ) ;

      matcher = BSON( OM_TASKINFO_FIELD_RESULTINFO << elemMatch
                      << OM_TASKINFO_FIELD_STATUS << status ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to query host:rc=%d,host=%s", rc, 
                 matcher.toString().c_str() ) ;
         goto done ;
      }

      rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto done ;
         }

         // notice: if rc != SDB_OK, contextID is deleted in rtnGetMore
         isExist = FALSE ;
         goto done ;
      }
      {
         BSONObj result( buffObj.data() ) ;
         BSONElement eleID = result.getField( OM_TASKINFO_FIELD_TASKID ) ;
         PD_LOG( PDERROR, "host[%s] is exist in task[" OSS_LL_PRINT_FORMAT "]",
                 hostName.c_str(), eleID.numberLong() ) ;
      }

      _pRTNCB->contextDelete( contextID, _cb ) ;

   done:
      return isExist;
   }

   BOOLEAN omRestCommandBase::_isBusinessExistInTask( 
                                                   const string &businessName )
   {
      BOOLEAN isExist = TRUE ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID = -1 ;
      INT32 rc         = SDB_OK ;
      string businessKey ;
      rtnContextBuf buffObj ;

      //Status not in( OM_TASK_STATUS_FINISH, OM_TASK_STATUS_CANCEL )
      BSONArrayBuilder arrBuilder ;
      arrBuilder.append( OM_TASK_STATUS_FINISH ) ;
      arrBuilder.append( OM_TASK_STATUS_CANCEL ) ;
      BSONObj status = BSON( "$nin" << arrBuilder.arr() ) ;

      businessKey = OM_TASKINFO_FIELD_INFO "." OM_BSON_BUSINESS_NAME ;
      matcher = BSON( businessKey << businessName
                      << OM_TASKINFO_FIELD_STATUS << status ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_TASKINFO, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to query business:rc=%d,matcher=%s", rc, 
                     matcher.toString().c_str() ) ;
         goto done ;
      }

      rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG_MSG( PDERROR, "Failed to retreive record, rc = %d", rc ) ;
            goto done ;
         }

         // notice: if rc != SDB_OK, contextID is deleted in rtnGetMore
         isExist = FALSE ;
         goto done ;
      }
      {
         BSONObj result( buffObj.data() ) ;
         BSONElement eleID = result.getField( OM_TASKINFO_FIELD_TASKID ) ;
         PD_LOG_MSG( PDERROR, "business[%s] is exist in task["
                     OSS_LL_PRINT_FORMAT"]", businessName.c_str(), 
                     eleID.numberLong() ) ;
      }

      _pRTNCB->contextDelete( contextID, _cb ) ;

   done:
      return isExist;
   }

   INT32 omRestCommandBase::_getBusinessType( const string &businessName,
                                              string &businessType,
                                              string &deployMode ) 
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      SINT64 contextID = -1 ;

      selector = BSON( OM_BUSINESS_FIELD_TYPE << 1 
                    << OM_BUSINESS_FIELD_DEPLOYMOD << 1) ;
      matcher = BSON( OM_BUSINESS_FIELD_NAME << businessName ) ;
      rc = rtnQuery( OM_CS_DEPLOY_CL_BUSINESS, selector, matcher, order, hint, 
                     0, _cb, 0, -1, _pDMSCB, _pRTNCB, contextID );
      if ( rc )
      {
         PD_LOG_MSG( PDERROR, "fail to query table:%s,rc=%d",
                     OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rtnContextBuf buffObj ;
         rc = rtnGetMore ( contextID, 1, buffObj, _cb, _pRTNCB ) ;
         if ( rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               break ;
            }
            contextID = -1 ;
            PD_LOG_MSG( PDERROR, "failed to get record from table:%s,rc=%d", 
                        OM_CS_DEPLOY_CL_BUSINESS, rc ) ;
            goto error ;
         }

         BSONObj record( buffObj.data() ) ;
         businessType = record.getStringField( OM_BUSINESS_FIELD_TYPE ) ;
         deployMode = record.getStringField( OM_BUSINESS_FIELD_DEPLOYMOD ) ;
         break ;
      }
   done:
      if ( -1 != contextID )
      {
         _pRTNCB->contextDelete ( contextID, _cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN omRestCommandBase::_isClusterExist( const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BSONObj clusterInfo ;
      rc = _getClusterInfo( clusterName, clusterInfo ) ;
      if ( SDB_OK != rc )
      {
         return FALSE ;
      }

      return TRUE ;
   }

   BOOLEAN omRestCommandBase::_isBusinessExist( const string &clusterName, 
                                                const string &businessName )
   {
      INT32 rc = SDB_OK ;
      BSONObj businessInfo ;
      rc = _getBusinessInfo( businessName, businessInfo ) ;
      if ( SDB_OK != rc )
      {
         return FALSE ;
      }

      return TRUE ;
   }

   BOOLEAN omRestCommandBase::_isHostExistInCluster( const string &hostName,
                                                     const string &clusterName )
   {
      INT32 rc = SDB_OK ;
      BSONObj selector ;
      BSONObj matcher ;
      BSONObj order ;
      BSONObj hint ;
      list < BSONObj > records ;

      matcher = BSON( OM_HOST_FIELD_NAME << hostName 
                      << OM_HOST_FIELD_CLUSTERNAME << clusterName ) ;
      selector = BSON( OM_HOST_FIELD_NAME << "" ) ;
      rc = _queryTable( OM_CS_DEPLOY_CL_HOST, selector, matcher, order, hint, 
                        0, 0, -1, records ) ;
      if ( SDB_OK != rc )
      {
         return FALSE ;
      }

      if ( records.size() > 0 )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   void omRestCommandBase::_setOPResult( INT32 rc, const CHAR* detail )
   {
      BSONObj res = BSON( OM_REST_RES_RETCODE << rc 
                          << OM_REST_RES_DESP << getErrDesp( rc )
                          << OM_REST_RES_DETAIL << detail ) ;

      _response->setOPResult( rc, res ) ;
   }

   void omRestCommandBase::_sendOKRes2Web()
   {
      _sendErrorRes2Web( SDB_OK, "" ) ;
   }

   void omRestCommandBase::_sendErrorRes2Web( INT32 rc, const CHAR* detail )
   {
      _setOPResult( rc, detail ) ;
      _response->setResponse( HTTP_OK ) ;
      _restAdaptor->sendRest( _restSession->socket(), _response ) ;
   }

   void omRestCommandBase::_sendErrorRes2Web( INT32 rc, const string &detail )
   {
      _sendErrorRes2Web( rc, detail.c_str() ) ;
   }

   INT32 omRestCommandBase::_parseIPsField( const CHAR *input,
                                            set< string > &IPs )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCur = input ;
      const CHAR *pBegin = input ;
      while( TRUE )
      {
         if ( ',' == *pCur || 0 == *pCur )
         {
            INT32 nLen = pCur - pBegin ;
            if ( nLen > 0 )
            {
               if ( TRUE == ossNetIpIsValid( pBegin, nLen ) )
               {
                  string ip( pBegin, nLen ) ;
                  IPs.insert( ip ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }
            if ( 0 == *pCur )
            {
               break ;
            }
            pBegin = pCur + 1 ;
         }
         ++pCur ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _omRestCmdAssit
   */
   _omRestCmdAssit::_omRestCmdAssit( OMREST_NEW_FUNC pFunc )
   {
      if ( pFunc )
      {
         omRestCommandBase *pCommand = (*pFunc)( NULL, NULL, NULL, NULL,
                                                 "", "", "" ) ;
         if ( pCommand )
         {
            getOmRestCmdBuilder()->_register ( pCommand->name(), pFunc ) ;
            SDB_OSS_DEL pCommand ;
            pCommand = NULL ;
         }
      }
   }

   _omRestCmdAssit::~_omRestCmdAssit ()
   {
   }

   
   /*
      _omRestCmdBuilder
   */
   _omRestCmdBuilder::_omRestCmdBuilder ()
   {
   }

   _omRestCmdBuilder::~_omRestCmdBuilder ()
   {
   }

   omRestCommandBase* _omRestCmdBuilder::create( const CHAR *command,
                                                 OMREST_CLASS_PARAMETER )
   {
      OMREST_NEW_FUNC pFunc = _find ( command ) ;

      if ( pFunc )
      {
         SDB_ASSERT ( pRestSession, "pRestSession is NULL" ) ;
         SDB_ASSERT ( pRestAdaptor, "pRestAdaptor is NULL" ) ;
         SDB_ASSERT ( pRequest,     "pRequest is NULL" ) ;
         SDB_ASSERT ( pResponse,    "pResponse is NULL" ) ;

         return (*pFunc)( OMREST_CLASS_INPUT_PARAMETER ) ;
      }

      return NULL ;
   }

   void _omRestCmdBuilder::release ( omRestCommandBase *&pCommand )
   {
      if ( pCommand )
      {
         SDB_OSS_DEL pCommand ;
         pCommand = NULL ;
      }
   }

   INT32 _omRestCmdBuilder::_register ( const CHAR *name, OMREST_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;
      pair< MAP_OACMD_IT, BOOLEAN > ret ;

      if ( ossStrlen( name ) == 0 )
      {
         goto done ;
      }

      ret = _cmdMap.insert( pair<const CHAR*, OMREST_NEW_FUNC>(name, pFunc) ) ;
      if ( FALSE == ret.second )
      {
         PD_LOG_MSG ( PDERROR, "Failed to register om rest command[%s], "
                      "already exist", name ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OMREST_NEW_FUNC _omRestCmdBuilder::_find ( const CHAR *name )
   {
      if ( name )
      {
         MAP_OACMD_IT it = _cmdMap.find( name ) ;
         if ( it != _cmdMap.end() )
         {
            return it->second ;
         }
      }
      return NULL ;
   }

   /*
      get om rest command builder
   */
   _omRestCmdBuilder* getOmRestCmdBuilder()
   {
      static _omRestCmdBuilder cmdBuilder ;
      return &cmdBuilder ;
   }

   /*
      omAgentReqBase
   */
   omAgentReqBase::omAgentReqBase( BSONObj &request )
                  :_request( request.copy() ), _response( BSONObj() )
   {
   }

   omAgentReqBase::~omAgentReqBase()
   {
   }

   void omAgentReqBase::getResponse( BSONObj &response )
   {
      _response = response ;
   }

   const CHAR *omGetMyEDUInfoSafe( EDU_INFO_TYPE type )
   {
      return omGetEDUInfoSafe( pmdGetThreadEDUCB(), type ) ;
   }

   const CHAR *omGetEDUInfoSafe( _pmdEDUCB *cb, EDU_INFO_TYPE type )
   {
      SDB_ASSERT( NULL != cb, "cb can't be null" ) ;
      const CHAR *info = cb->getInfo( type ) ;
      if ( NULL == info )
      {
         return "" ;
      }

      return info ;
   }
}

