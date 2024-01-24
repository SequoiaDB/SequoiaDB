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

   Source File Name = sequoiaFSRegister.cpp

   Descriptive Name = sequoiafs meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =
*******************************************************************************/

#include "sequoiaFSRegister.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSDao.hpp"
#include "sequoiaFSMetaCache.hpp"

#include "pmd.hpp"

using namespace engine;
using namespace bson;
using namespace sdbclient;

namespace sequoiafs
{
   fsRegister::fsRegister(fsMetaCache* metaCache)
   :_metaCache(metaCache),
    _agent(metaCache->getMsgHandler()),
    _isRegistered(FALSE),
    _running(FALSE)
   {
   }

   fsRegister::~fsRegister()
   {
   }

   void fsRegister::fini()
   {
      _running = FALSE;
      _agent.stop();
   }

   INT32 fsRegister::start(sdbConnectionPool* ds, CHAR* mountCL, CHAR* mountPath)
   {
      INT32 rc = SDB_OK;
      string hostname;
      string port;
      _routeID.value = MSG_INVALID_ROUTEID;

      ossIPInfo ipInfo;
      ossIP *ip = ipInfo.getIPs();
      for(INT32 i = ipInfo.getIPNum(); i > 0; i--)
      {
         if(0 != ossStrncmp(ip->ipAddr, OSS_LOOPBACK_IP,
                             ossStrlen(OSS_LOOPBACK_IP)))
         {
            _mountIP = ip->ipAddr;
            break;
         }
         ip++;
      }

      _dbDao = new fsConnectionDao(ds);
      if(NULL == _dbDao)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "Fail to get connection, rc=%d", rc);
         goto error;
      }

      _mountPath = mountPath;
      _mountCL = mountCL;

      rc = _queryMountId(_mountCL, _mountPath, &_mountCLID);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to queryMountId. _mountCL:%s, _mountPath:%s, rc=%d",
                 _mountCL, _mountPath, rc);
         goto error;
      }

      _fsTimer = SDB_OSS_NEW _fsTimerHandler(&_fsSesMgr);
      if ( !_fsTimer )
      {
         PD_LOG( PDERROR, "Allocate shard timer handler failed" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = _fsSesMgr.init(&_agent, _fsTimer, 60 * OSS_ONE_SEC ) ;
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to init shard session manager, rc=%d", rc ) ;
         ossPrintf("Failed to init shard session manager(rc=%d), exit." OSS_NEWLINE, rc);
         goto error;
      }

      rc = activeEDU();
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to activeEDU, rc=%d", rc ) ;
         ossPrintf("Failed to activeEDU(rc=%d), exit." OSS_NEWLINE, rc);
         goto error;
      }

      _running = TRUE;

      rc = registerToMCS();
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to register to mcs, rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done ;
   }

   void fsRegister::hold()
   {
      INT32 rc = SDB_OK;
      string hostname;
      string port;
      INT32 count = 0;

      while(_running)
      {
         ossSleepsecs(1);  //TODO: mcs disconnect，retry immediately
         ++count;
         if(count < FS_REGISTER_MCS_SECOND)
         {
            continue;
         }
         count = 0;

         if(!_isRegistered)
         {
            rc = registerToMCS();
            if(SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to register to mcs, rc=%d", rc);
               continue;
            }
         }
      }
   }

   INT32 fsRegister::registerToMCS()
   {
      INT32 rc = SDB_OK;
      string hostname;
      string port;

      rc = _queryMcsInfo(SEQUOIADB_SERVICE_FULLCL, &hostname, &port);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to queryMcsInfo, cl:%s. rc=%d",
                 SEQUOIADB_SERVICE_FULLCL, rc);
         goto error;
      }

      rc = _agent.updateRoute(_routeID, hostname.c_str(), port.c_str());
      if(SDB_OK != rc)
      {
         if(SDB_NET_UPDATE_EXISTING_NODE != rc)
         {
            PD_LOG( PDERROR, "Failed to updateRoute. hostname:%s, port:%s, rc=%d",
                          hostname.c_str(), port.c_str(), rc);
            goto error;
         }
      }

      rc = _agent.syncConnect(_routeID, &_pHandle);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to syncConnect. rc=%d", rc);
         goto error;
      }

      rc = sendRegReq(_mountPath, _mountCLID, _mountIP);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to updateRoute. rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 fsRegister::sendRegReq(CHAR* mountPath, INT32 mountId, CHAR* mountIp)
   {
      INT32 rc = SDB_OK;

      INT32 pathLen = strnlen(mountPath, OSS_MAX_PATHSIZE);
      INT32 messageLength = sizeof(_mcsRegReq) + pathLen + 1;
      _mcsRegReq *regReq = (_mcsRegReq *)(void *)SDB_OSS_MALLOC(messageLength);
      if(!regReq)
      {
         PD_LOG( PDERROR, "Failed to alloc memory, size: %d", messageLength ) ;
         return SDB_OOM ;
      }
      regReq->header.messageLength = messageLength;
      regReq->header.opCode = FS_REGISTER_REQ;
      regReq->header.requestID = 0;
      regReq->header.routeID = _routeID;
      regReq->header.TID = 0;
      regReq->mountId = mountId; //mountId
      ossStrncpy(regReq->mountIp, mountIp, OSS_MAX_IP_ADDR); //mountIp
      regReq->mountIp[OSS_MAX_IP_ADDR] = '\0';
      ossStrncpy(regReq->mountPath, mountPath, pathLen); //mountpath
      regReq->pathLen = pathLen; //pathlen
      regReq->mountPath[pathLen] = '\0';
      rc = _agent.syncSend(_pHandle, (MsgHeader *)regReq);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to syncSend. rc=%d", rc);
         goto error;
      }

   done:
      SAFE_OSS_FREE(regReq);
      return rc;
   error :
      goto done ;
   }

   INT32 fsRegister::activeEDU()
   {
      INT32 rc = SDB_OK;
      pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr();
      EDUID eduID = PMD_INVALID_EDUID ;

      rc = pEDUMgr->startEDU(EDU_TYPE_FS_MCS_NET_AGENT, (void *)&_agent, &eduID) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Start agent failed, rc= %d", rc ) ;
         goto error ;
      }

      rc = pEDUMgr->waitUntil(eduID, PMD_EDU_RUNNING);
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to wait EDU[type:%d(%s)] to "
                 "status[%d(%s)], rc= %d", EDU_TYPE_FS_MCS_NET_AGENT,
                 getEDUName((EDU_TYPES)EDU_TYPE_FS_MCS_NET_AGENT), PMD_EDU_RUNNING,
                 getEDUStatusDesp(PMD_EDU_RUNNING ), rc) ;
         goto error ;
      }

   done:
      return rc;
   error :
      goto done ;
   }

   INT32 fsRegister::sendNotify(INT64 parentId, const CHAR* dirName)
   {
      INT32 rc = SDB_OK;
      _mcsNotifyReq *notifyReq = NULL;
      INT32 nameLen = strnlen(dirName, FS_MAX_NAMESIZE);
      INT32 messageLength = sizeof(_mcsNotifyReq) + nameLen + 1;

      if(!_isRegistered)
      {
         goto done;
      }

      notifyReq = (_mcsNotifyReq *)(void *)SDB_OSS_MALLOC(messageLength);
      if(!notifyReq)
      {
         PD_LOG( PDERROR, "Failed to alloc memory, size: %d", messageLength ) ;
         rc = SDB_OOM;
         goto error;
      }
      notifyReq->header.messageLength = messageLength;
      notifyReq->header.opCode = FS_RELEASELOCK;
      notifyReq->header.requestID = 0;
      notifyReq->header.routeID = _routeID;
      notifyReq->header.TID = 0;
      notifyReq->mountId = _mountCLID; //mountId
      notifyReq->parentId = parentId;
      ossStrncpy(notifyReq->dirName, dirName, nameLen); //dirName
      notifyReq->nameLen = nameLen; //nameLen
      notifyReq->dirName[nameLen] = '\0';

      rc = _agent.syncSend(_pHandle, (MsgHeader *)notifyReq);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to syncSend. rc=%d", rc);
         goto error;
      }

   done:
      SAFE_OSS_FREE(notifyReq);
      return rc;
   error :
      goto done ;
   }

   void fsRegister::setRegister(BOOLEAN isRegistered)
   {
      _isRegistered = isRegistered;
      if(isRegistered)
      {
         _metaCache->setCacheMeta();
      }
      else
      {
         _metaCache->cancleCacheMeta();
      }
   }

   INT32 fsRegister::_queryMcsInfo(const CHAR *pCLFullName,
                                  string* hostName, string* port)
   {
      INT32 rc = SDB_OK;
      //sdb *db = NULL;
      sdbCollection cl;
      sdbCursor cursor;
      BSONObj record;
      BSONObj condition;

      try
      {
         condition = BSON(SERVICE_NAME<<SERVICE_NAME_MCS);
      }
      catch(std::exception &e)
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error;
      }

      rc = _dbDao->getCL(SEQUOIADB_SERVICE_FULLCL, cl);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
         goto error;
      }

      rc = cl.query(cursor, condition);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
         goto error;
      }

      rc = cursor.next(record);
      if(SDB_DMS_EOC == rc)
      {
         PD_LOG(PDERROR, "The mcs does not exist, "
                "rc=%d", rc);
         goto error;
      }
      else if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "query mcs failed, "
                "rc=%d", rc);
         goto error;
      }

      {
         BSONObjIterator itr( record ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( SERVICE_HOSTNAME, ele.fieldName() ) )
            {
               PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                        error, PDERROR, "The type of field:%s is not string",
                        SERVICE_HOSTNAME);
               *hostName = (CHAR*)ele.valuestrsafe();
            }
            if ( 0 == ossStrcmp( SERVICE_PORT, ele.fieldName() ) )
            {
               PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                        error, PDERROR, "The type of field:%s is not string",
                        SERVICE_PORT);
               *port = (CHAR*)ele.valuestrsafe();
            }
         }
      }

   done:
      cursor.close();
      return rc;
   error:
      goto done;
   }

   INT32 fsRegister::_queryMountId(CHAR* mountcl, CHAR* mountpoint, INT64 *mountId)
   {
      INT32 rc = SDB_OK;
      //sdb *db = NULL;
      sdbCollection cl;
      sdbCursor cursor;
      BSONObj record;
      BSONObj condition;

      try
      {
         condition= BSON(FS_MOUNT_CL<<mountcl<<FS_MOUNT_PATH<<mountpoint);
      }
      catch(std::exception &e)
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error;
      }

      rc = _dbDao->getCL(SEQUOIAFS_MOUNTID_FULLCL, cl);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
         goto error;
      }

      rc = cl.query(cursor, condition);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
         goto error;
      }

      rc = cursor.next(record);
      if(SDB_DMS_EOC == rc)
      {
         PD_LOG(PDERROR, "The mountcl does not exist, cl:%s,"
                "rc=%d", SEQUOIAFS_MOUNTID_FULLCL, rc);
         goto error;
      }
      else if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "query mountcl failed, rc=%d", rc);
         goto error;
      }

      {
         BSONObjIterator itr(record);
         while (itr.more())
         {
            BSONElement ele = itr.next();
            if ( 0 == ossStrcmp(FS_MOUNT_ID, ele.fieldName()))
            {
               PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                        error, PDERROR, "The type of field:%s is not number",
                        FS_MOUNT_ID);
               *mountId = ele.numberLong();
            }
         }
      }

   done:
      cursor.close();
      return rc;
   error:
      goto done;
   }

}
