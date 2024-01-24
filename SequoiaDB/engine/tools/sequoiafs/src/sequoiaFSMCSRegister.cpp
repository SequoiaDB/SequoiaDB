/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   ( at your option ) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sequoiaFSMCSRegister.cpp

   Descriptive Name = sequoiafs meta cache service.

   When/how to use:  This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossSocket.hpp"

#include "sequoiaFSMCSRegister.hpp"
#include "sequoiaFSMCS.hpp"

using namespace bson; 
using namespace sdbclient;  
using namespace sequoiafs;

mcsRegService::~mcsRegService()
{
}

INT32 mcsRegService::init(sdbConnectionPool* ds, CHAR* hostname, CHAR* port)
{
   INT32 rc = SDB_OK;

   _hostname = hostname;
   _port = port;
   _ds = ds;

   //初始注册
   rc = _registerToDB(FALSE);  //TODO:根据启动参数设置isforce参数
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to registerToDB. rc=%d", rc);
      goto error;
   }

   //_isRegistered = TRUE;
   _running = TRUE;
   _isWorking = TRUE;

done:
   return rc;
   
error :
   goto done ;
}

void mcsRegService::hold()
{
   INT32 rc = SDB_OK;
   
   while(_running)
   {
      rc = _registerToDB(FALSE);
      if(SDB_OK != rc)
      {
         _isRegistered = FALSE;
         _isWorking = FALSE;
      }
      else 
      {
         _isRegistered = TRUE;
      }

      ossSleepsecs(MCS_REGISTER_DB_SECOND);
   }
}

INT32 mcsRegService::_registerToDB(BOOLEAN isForce)
{
   INT32 rc = SDB_OK;
   sdbCursor cursor;
   BSONObj condition;
   BSONObj record;
   CHAR* fullCl = SEQUOIADB_SERVICE_FULLCL;
   fsConnectionDao db(_ds);
   BOOLEAN  isFind = TRUE;
   BOOLEAN  isMatch = TRUE;

   try 
   {
      condition = BSON(SERVICE_NAME<<"MCS");
   }
   catch (std::exception &e)
   {
      rc = SDB_SYS;
      PD_LOG(PDERROR, "Exception[%s] occurs. rc=", e.what(), rc);
      goto error;
   }

   rc = db.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to transBegin. rc=%d", rc);
      goto error;
   }

   rc = db.queryForUpdate(fullCl, cursor, condition);
   if(SDB_OK != rc)
   {      
      if(SDB_DMS_NOTEXIST == rc 
         || SDB_DMS_CS_NOTEXIST == rc)
      {
         isFind = FALSE;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to queryForUpdate, cl:%s. rc=%d", 
                 SEQUOIADB_SERVICE_FULLCL, rc);
         goto error;
      }
   }
   else 
   {
      rc = cursor.current(record);
      if(SDB_OK == rc)
      {
         if(SDB_OK != _checkHostNamePort(&cursor)) 
         {
            isMatch = FALSE;
         }
      }
      else if(SDB_DMS_EOC == rc)
      {
         isFind = FALSE;
      }
      else if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "get mcs service failed, error=%d", rc);
         goto error;
      }
   }

   if(!isFind)
   {
      rc = _insertMCSService(&db);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to insertMCSService. rc=%d", rc);
         goto error;
      }
   }
   else if(!isMatch)
   {
      if(isForce)
      {
         rc = _updateMCSService(&db);
         if(SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to updateMCSService. rc=%d", rc);
            goto error;
         }
      }
      else
      {
         rc = SDB_OPERATION_CONFLICT;
         PD_LOG( PDERROR, "The MCS has been started up. rc=%d", rc);
         goto error;
      }
   }

   rc = db.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to transCommit. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   db.transRollback();
   goto done;
}

INT32 mcsRegService::_checkHostNamePort(sdbCursor* cursor)
{
   INT32 rc = SDB_OK;
   BSONObj record;
   BSONObjIterator itr ;
   BOOLEAN isSameHostName = FALSE;
   BOOLEAN isSamePort = FALSE;

   rc = cursor->current(record);
   if(SDB_OK != rc)
   {
      goto error;
   }

   itr = BSONObjIterator(record);
   while (itr.more())
   {
      BSONElement ele = itr.next();
      if ( 0 == ossStrcmp(SERVICE_HOSTNAME, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_SYMLINK);
         if(0 == ossStrncmp((CHAR*)ele.valuestrsafe(), _hostname, OSS_MAX_HOSTNAME))
         {
            isSameHostName = TRUE;
         }
      }
      if ( 0 == ossStrcmp(SERVICE_PORT, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_SYMLINK);
         if(0 == ossStrncmp((CHAR*)ele.valuestrsafe(), _port, OSS_MAX_HOSTNAME))
         {
            isSamePort = TRUE;
         }
      }
   }

   if(TRUE == isSameHostName && TRUE == isSamePort)
   {
      goto done;
   }
   else 
   {
      rc = SDB_OPERATION_CONFLICT;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 mcsRegService::_insertMCSService(fsConnectionDao* db)
{
   INT32 rc = SDB_OK;
   sdbCollectionSpace cs;
   sdbCollection cl;
   BSONObj idxDefObj;
   BSONObj mcsObj;
   BSONObj options;
   SINT64 timeoutTimeStamp = ossGetCurrentMilliseconds() + MCS_REGISTER_DB_SECOND * 2 * 1000;

   rc = db->getCL(SEQUOIADB_SERVICE_FULLCL, cl);
   if(SDB_DMS_CS_NOTEXIST == rc)
   {
      rc = db->createCollectionSpace(SEQUOIADB_SERVICE_CS, options, cs);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to create collectionspace:%s. rc=%d", SEQUOIADB_SERVICE_CS, rc);
         goto error;
      }

      rc = cs.createCollection(SEQUOIADB_SERVICE_CL, cl);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to create collection:%s. rc=%d", SEQUOIADB_SERVICE_CL, rc);
         goto error;
      }
   }
   else if(SDB_DMS_NOTEXIST == rc)
   {
      rc = db->getCS(SEQUOIADB_SERVICE_CS, cs);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to get collectionspace(%s). rc=%d", SEQUOIADB_SERVICE_CS, rc);
         goto error;
      }
      rc = cs.createCollection(SEQUOIADB_SERVICE_CL, cl);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to create collection(%s). rc=%d", SEQUOIADB_SERVICE_CL, rc);
         goto error;
      }
   }
   else if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collection(%s). rc=%d", SEQUOIADB_SERVICE_CL, rc);
      goto error;
   }

   rc = cl.getIndex(SEQUOIADB_SERVICE_INDEX, idxDefObj);
   if(SDB_IXM_NOTEXIST == rc)
   {
      try 
      {
         idxDefObj = BSON(SERVICE_NAME << 1 );
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error; 
      }
      rc = cl.createIndex(idxDefObj, SEQUOIADB_SERVICE_INDEX, TRUE, TRUE);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to create index, idx=%s, rc=%d",
                 SEQUOIADB_SERVICE_INDEX, rc);
          goto error;
      }
   }
   else if(SDB_OK != rc)
   {
       PD_LOG(PDERROR, "Failed to get index, idx=%s, rc=%d",
              SEQUOIADB_SERVICE_INDEX, rc);
       goto error;
   }

   try 
   {
      mcsObj = BSON(SERVICE_NAME<<"MCS"<<\
                    SERVICE_HOSTNAME<<_hostname<<\
                    SERVICE_PORT<<_port<<\
                    SERVICE_TIMEOUT<<timeoutTimeStamp);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   
   rc = cl.insert(mcsObj);
   if(SDB_OK != rc)
   {
       PD_LOG(PDERROR, "Failed to insert mcs service, rc=%d", rc);
       goto error;
   }

   PD_LOG(PDERROR, "insert mcsinfo port:%s. curtime:%ld, timeoutTimeStamp:%ld." OSS_NEWLINE, _port, ossGetCurrentMilliseconds(), timeoutTimeStamp);

done:
   return rc;
   
error:
   goto done;  
}

INT32 mcsRegService::_updateMCSService(fsConnectionDao* db)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   BSONObj condition;
   BSONObj rule;
   SINT64 timeoutTimeStamp = ossGetCurrentMilliseconds() + MCS_REGISTER_DB_SECOND * 2 * 1000;

   rc = db->getCL(SEQUOIADB_SERVICE_FULLCL, cl);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collection(%s). rc=%d", SEQUOIADB_SERVICE_FULLCL, rc);
      goto error;
   }

   try 
   {
      condition = BSON(SERVICE_NAME<<"MCS");
      rule = BSON( "$set" <<BSON(SERVICE_HOSTNAME<<_hostname<<\
                                 SERVICE_PORT<<_port<<\
                                 SERVICE_TIMEOUT<<timeoutTimeStamp));
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   
   rc = cl.update(rule, condition);
   if(SDB_OK != rc)
   {
       PD_LOG(PDERROR, "Failed to update mcs service, rc=%d", rc);
       goto error;
   }

   PD_LOG(PDERROR, "insert mcsinfo port:%s. curtime:%ld, timeoutTimeStamp:%ld." OSS_NEWLINE, _port, ossGetCurrentMilliseconds(), timeoutTimeStamp);

done:
   return rc;
   
error:
   goto done; 
}


