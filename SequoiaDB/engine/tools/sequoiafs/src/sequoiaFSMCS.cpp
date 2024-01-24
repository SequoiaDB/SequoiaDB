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

   Source File Name = sequoiaFSMCS.cpp

   Descriptive Name = sequoiafs meta cache service.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        01/07/2021  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSMCS.hpp"
#include "sequoiaFSCommon.hpp"


#include "pmd.hpp"

#include "ossVer.h"
#include "ossMem.hpp"
#include "ossSocket.hpp"

#include <map>

using namespace engine;
using namespace sdbclient;  
using namespace sequoiafs;

sequoiaFSMCS::sequoiaFSMCS()
:_mcsSesMgr(),
 _msgHandler(&_mcsSesMgr, this),
 _regService(this)
{
   _agent = NULL;
   ossMemset(_hostName, 0, OSS_MAX_HOSTNAME+1);
   _thRegisterDB = NULL;
}

sequoiaFSMCS::~sequoiaFSMCS()
{
   _agent->shutdownListen();
   if(_thRegisterDB)
   {
      _thRegisterDB->join();
   }

   map<INT32, mountNode*>::iterator iter;
   for(iter = _fsIdMap.begin(); iter != _fsIdMap.end(); iter++)
   {
      mountNode* node = iter->second;
      mountNode* next;
      while(node)
      {
         next = node->next;
         SAFE_OSS_DELETE(node);
         node = next;
      }
   }
}

INT32 sequoiaFSMCS::_mcsInitDataSource(sequoiafsMcsOptionMgr *optionMgr)
{
   INT32 rc = SDB_OK;
   vector<string> coordHostPort;
   sdbConnectionPoolConf conf;
   string hosts;
   size_t index = 0;

   PD_LOG(PDDEBUG, "Called: mcsInitDataSource");

   conf.setAuthInfo(optionMgr->getUserName(), optionMgr->getPasswd());
   conf.setConnCntInfo(50, 10, 20, optionMgr->getConnNum());
   conf.setCheckIntervalInfo( 60*1000, 0 );
   conf.setSyncCoordInterval( 60*1000 );
   conf.setConnectStrategy( SDB_CONN_STY_BALANCE );
   conf.setValidateConnection( TRUE );
   conf.setUseSSL( FALSE );

   hosts = optionMgr->getHosts();
   for(;;)
   {
      index = hosts.find(',');
      if(index != std::string::npos)
      {
          coordHostPort.push_back(hosts.substr(0, index));
          hosts = hosts.substr(index + 1);
      }
      else
      {
          coordHostPort.push_back(hosts);
          break;
      }
   }
   
   rc = _ds.init( coordHostPort, conf);
   if (SDB_OK != rc)
   {
      ossPrintf("Fail to init sdbDataSouce, rc=%d" OSS_NEWLINE, rc);
      goto error;
   }
   
done:
   return rc;

error:
   goto done;
}

INT32 sequoiaFSMCS::_openPdLog()
{
   INT32 rc = SDB_OK;
   CHAR diaglogPath[OSS_MAX_PATHSIZE + 1] = {0};
   CHAR verText[OSS_MAX_PATHSIZE + 1] = {0};

   rc = buildDialogPath(diaglogPath, _optionMgr.getDiaglogPath(), OSS_MAX_PATHSIZE + 1);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to build dialog path(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   rc = engine::utilCatPath(diaglogPath, OSS_MAX_PATHSIZE,
                            SDB_SEQUOIAFS_MCS_LOG_FILE_NAME);
   if(rc != SDB_OK)
   {
      ossPrintf("Failed to build dialog path(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }
   
   sdbEnablePD(diaglogPath, _optionMgr.getDiagMaxNUm());
   setPDLevel((PDLEVEL(_optionMgr.getDiaglogLevel())));
   
   ossSprintVersion("Version", verText, OSS_MAX_PATHSIZE, FALSE);
   PD_LOG(((getPDLevel()>PDEVENT)?PDEVENT:getPDLevel()),
        "Start sequoiaMCS[%s]...", verText);

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFSMCS::_activeEDU()
{
   INT32 rc = SDB_OK;
   pmdEDUMgr *pEDUMgr = pmdGetKRCB()->getEDUMgr();
   EDUID eduID = PMD_INVALID_EDUID ;
   EDU_STATUS waitStatus = PMD_EDU_UNKNOW;

   rc = pEDUMgr->startEDU(EDU_TYPE_FS_MCS_NET_SERVICE, (void *)_agent, &eduID) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Start agent failed, rc=%d", rc ) ;
      goto error ;
   }

   rc = pEDUMgr->waitUntil(eduID, PMD_EDU_RUNNING);
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to wait EDU[type:%d(%s)] to "
              "status[%d(%s)], rc=%d", EDU_TYPE_FS_MCS_NET_SERVICE,
              getEDUName((EDU_TYPES)EDU_TYPE_FS_MCS_NET_SERVICE), waitStatus,
              getEDUStatusDesp(waitStatus ), rc) ;
      goto error ;
   }

done:
   return rc;
error :
   goto done ;
}

INT32 sequoiaFSMCS::addMountNode(INT32 mountId, CHAR * mountIp, 
                                 CHAR * mountPath, NET_HANDLE handle)
{
   INT32 rc = SDB_OK;
   mountNode* cur = NULL;
   map<int, mountNode*>::iterator iter;
   
   mountNode* newNode = new(std::nothrow) mountNode(mountIp, mountPath, handle);
   if(NULL == newNode)
   {  
      rc = SDB_OOM;
      PD_LOG(PDERROR, "Fail to new mountNode, rc=%d", rc); 
      goto error;
   }

   _mapMutex.get();
   try 
   {
      _handleMap[handle] = mountId; 
   }
   catch(std::exception &e)
   {
      _mapMutex.release();
      PD_LOG(PDERROR, "add map failed, error=%s", e.what());
      rc = SDB_OOM;
      goto error;
   }  
   
   iter = _fsIdMap.find(mountId);
   if(iter != _fsIdMap.end())
   {
      cur = iter->second; 
      while(cur->next)
      {
         cur = cur->next;
      }

      cur->next = newNode;
      newNode->pre = cur;
   }
   else
   {
      try 
      {
         _fsIdMap[mountId] = newNode; 
      }
      catch(std::exception &e)
      {
         _handleMap.erase(handle);
         _mapMutex.release();
         PD_LOG(PDERROR, "add map failed, error=%s", e.what());
         rc = SDB_OOM;
         goto error;
      }
   }
   _mapMutex.release();

done:
   return rc;

error:
   SAFE_OSS_DELETE(newNode);
   goto done;
}

void sequoiaFSMCS::delMountNode(UINT32 handle)
{
   ossPoolMap<UINT32, INT32>::iterator iterhandle;
   ossPoolMap<INT32, mountNode*>::iterator iterMountId;

   _mapMutex.get();
   iterhandle = _handleMap.find(handle);
   if(iterhandle != _handleMap.end())
   {
      INT32 mountId = iterhandle->second; 
      mountNode* cur = NULL;
      iterMountId = _fsIdMap.find(mountId);
      if(_fsIdMap.end() != iterMountId)
      {
         cur = iterMountId->second;
         while(cur->next)
         {
            if(handle == cur->_handle)
            {
               break;
            }
            cur = cur->next;
         }

         if(cur)
         {
            if(cur->pre == NULL && cur->next == NULL)
            {
               _fsIdMap.erase(mountId);
            }
            else
            {
               if(cur->next)
               {
                  cur->next->pre = cur->pre;
               }

               if(cur->pre)
               {
                  cur->pre->next = cur->next;
               }
               else
               {
                  _fsIdMap[mountId] = cur->next;  
               }
            }
            
            SAFE_OSS_DELETE(cur);
         }
      }
      _handleMap.erase(handle);
   }
   _mapMutex.release();
}

mountNode* sequoiaFSMCS::getFsList(INT32 mountId)
{
   ossPoolMap<INT32, mountNode*>::iterator iter;

   iter = _fsIdMap.find(mountId);
   if(iter != _fsIdMap.end())
   {
      return iter->second;
   }
   else
   {
      return NULL;
   }
}

void sequoiaFSMCS::cleanAllFS()
{
   ossPoolMap<UINT32, INT32>::iterator iterhandle;
   ossPoolMap<INT32, mountNode*>::iterator iterMountId;
   mountNode* cur = NULL;
   mountNode* next = NULL;

   _mapMutex.get();
   iterhandle = _handleMap.begin();
   while(iterhandle != _handleMap.end())
   {
      _agent->close(iterhandle->second);
      _handleMap.erase(iterhandle->first);
      iterhandle++;
   }

   iterMountId = _fsIdMap.begin();
   while(iterMountId != _fsIdMap.end())
   {
      cur = iterMountId->second;
      while(cur)
      {
         PD_LOG( PDERROR, "SAFE_OSS_DELETE, cur:%d cur->next:%d,", cur, cur->next) ;
         next = cur->next;
         SAFE_OSS_DELETE(cur);
         cur = next;
      }
      _fsIdMap.erase(iterMountId->first);
   }
   _mapMutex.release();
}

INT32 sequoiaFSMCS::mcsThreadMain( INT32 argc, CHAR** argv )
{
   INT32 rc = SDB_OK;
   MsgRouteID routeID ;
   routeID.value = MSG_INVALID_ROUTEID ;

   rc = ossGetHostName( _hostName, OSS_MAX_HOSTNAME ) ;
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to get hostName. rc=%d, exit." OSS_NEWLINE, rc);
      goto error;
   }
   
   //1. read command line args and config args
   rc = _optionMgr.init(argc, argv);
   if(SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc)
   {
      rc = SDB_OK;
      goto done;
   }
   else if(SDB_OK != rc)
   {
      ossPrintf("Failed to resolving arguments(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   //2. open pd log
   rc = _openPdLog();
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to openPdLog, rc=%d, exit." OSS_NEWLINE, rc);
      goto error;
   }

   //3. init datasource
   rc = _mcsInitDataSource(&_optionMgr); //TODO:连接池改为connectpool
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to init connection pool, rc=%d", rc ) ;
      ossPrintf("Failed to init connection pool(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   //4. listen
   _port = _optionMgr.getPort();
   _agent = SDB_OSS_NEW _netRouteAgent(&_msgHandler);
   if(NULL == _agent)
   {
      PD_LOG( PDERROR, "Allocate route agent failed" ) ;
      ossPrintf("Allocate route agent failed. exit." OSS_NEWLINE);
      rc = SDB_OOM ;
      goto error ;
   }
   
   rc = _agent->updateRoute(routeID, _hostName, _port); 
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to updateRoute, rc=%d", rc ) ;
      ossPrintf("Failed to updateRoute(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   rc = _agent->listen(routeID, NET_FRAME_MASK_TCP);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to listen on port[%s], rc=%d", _port, rc ) ;
      ossPrintf("Failed to listen on port[%s](rc=%d), exit." OSS_NEWLINE, _port, rc);
      goto error;
   }

   //init();
   _mcsTimer = SDB_OSS_NEW _mcsTimerHandler(&_mcsSesMgr);
   if ( !_mcsTimer ) //TODO:这个实现的作用
   {
      PD_LOG( PDERROR, "Allocate shard timer handler failed" ) ;
      ossPrintf("Allocate shard timer handler failed. exit." OSS_NEWLINE);
      rc = SDB_OOM ;
      goto error ;
   }

   rc = _mcsSesMgr.init( _agent, _mcsTimer, 60 * OSS_ONE_SEC ) ;
   if(SDB_OK != rc) //TODO:这个实现的作用
   {
      PD_LOG( PDERROR, "Failed to init shard session manager, rc=%d", rc ) ;
      ossPrintf("Failed to init shard session manager(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   rc = _activeEDU();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to activeEDU, rc=%d", rc ) ;
      ossPrintf("Failed to activeEDU(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   //6. register to sequoiaDB
   rc = _regService.init(&_ds, _hostName, _port);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to init register, rc=%d", rc ) ;
      ossPrintf("Failed to init register(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }
   
   try
   {
      _thRegisterDB = new boost::thread( boost::bind( &mcsRegService::hold, &_regService) );
   }
   catch ( std::exception &e)
   {
      PD_LOG( PDERROR, "Failed to hold register, rc=%s", e.what() ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   while(_regService.isWorking())
   {
      ossSleepsecs(3);
   }

   _regService.stop();
   
done:
   return rc;
error :
   goto done ;
}


int main(INT32 argc, CHAR *argv[])
{
   INT32 rc = SDB_OK ;
   sequoiaFSMCS mcs;
   rc = mcs.mcsThreadMain(argc, argv);
   return rc ;
}


