/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = impRecordSharding.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordSharding.hpp"
#include "msgDef.h"
#include "../client/client.h"
#include "pd.hpp"

namespace import
{
   RecordSharding::RecordSharding()
   {
      _inited = FALSE;
      _groupNum = 0;
      _isMainCL = FALSE;
   }

   RecordSharding::~RecordSharding()
   {
   }

   INT32 RecordSharding::init(const vector<Host>& hosts,
                              const string& user,
                              const string& password,
                              const string& csname,
                              const string& clname,
                              BOOLEAN useSSL)
   {
      INT32 rc = SDB_OK;
      sdbConnectionHandle conn = SDB_INVALID_HANDLE;
      sdbCursorHandle cursor = SDB_INVALID_HANDLE;
      bson cataObj;
      INT32 cataCount = 0;

      SDB_ASSERT(!_inited, "alreay inited");

      _hosts = &hosts;
      _user = user;
      _password = password;
      _csname = csname;
      _clname = clname;
      _useSSL = useSSL;
      bson_init(&cataObj);

      for (vector<Host>::const_iterator it = hosts.begin();
           it != hosts.end(); it++)
      {
         const Host& host = *it;

         if (SDB_INVALID_HANDLE != cursor)
         {
            sdbCloseCursor(cursor);
            sdbReleaseCursor(cursor);
            cursor = SDB_INVALID_HANDLE;
         }

         if (SDB_INVALID_HANDLE != conn)
         {
            sdbDisconnect(conn);
            sdbReleaseConnection(conn);
            conn = SDB_INVALID_HANDLE;
         }

         if (_useSSL)
         {
            rc = sdbSecureConnect(host.hostname.c_str(), host.svcname.c_str(),
                                  _user.c_str(), _password.c_str(), &conn);
         }
         else
         {
            rc = sdbConnect(host.hostname.c_str(), host.svcname.c_str(),
                            _user.c_str(), _password.c_str(), &conn);
         }

         if (SDB_OK != rc)
         {
            PD_LOG(PDWARNING, "failed to connect to server %s:%s, rc=%d, usessl=%d",
                   host.hostname.c_str(), host.svcname.c_str(), rc, _useSSL);
            rc = SDB_OK;
            continue;
         }

         rc = sdbGetSnapshot(conn, SDB_SNAP_CATALOG, NULL, NULL, NULL, &cursor);
         if (SDB_OK != rc)
         {
            if (SDB_INVALID_HANDLE != cursor)
            {
               sdbCloseCursor(cursor);
               sdbReleaseCursor(cursor);
               cursor = SDB_INVALID_HANDLE;
            }

            if (SDB_RTN_COORD_ONLY == rc)
            {
               PD_LOG(PDWARNING, "%s:%s is not coordinator",
                      host.hostname.c_str(), host.svcname.c_str());
               rc = SDB_OK;
               continue;
            }

            PD_LOG(PDWARNING, "failed to get coordinator group from %s:%s, rc = %d",
                   host.hostname.c_str(), host.svcname.c_str(), rc);
            rc = SDB_OK;
            continue;
         }

         break;
      }

      if (SDB_INVALID_HANDLE == cursor)
      {
         rc = SDB_OK;
         PD_LOG(PDWARNING, "failed to get coordinator group");
         goto done;
      }

      for(;;)
      {
         rc = sdbNext(cursor, &cataObj);
         if (SDB_OK != rc)
         {
            if (SDB_DMS_EOC == rc)
            {
               rc = SDB_OK;
               break;
            }

            PD_LOG(PDERROR, "failed to get cataObj from cursor, rc=%d", rc);
            goto error;
         }

         rc = _cataAgent.updateCatalog(bson_data(&cataObj));
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update catalog agent, rc=%d", rc);
            goto error;
         }

         cataCount++;
      }

      if (0 == cataCount)
      {
         _inited = TRUE;
         goto done;
      }

      _collectionName = _csname + "." + _clname;

      rc = _cataAgent.getCataInfo(_collectionName, _cataInfo);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get catalog info, rc=%d", rc);
         goto error;
      }

      _isMainCL = _cataInfo.isMainCL();

      if (_cataInfo.isMainCL())
      {
         vector<string> subCLList;
         INT32 subGroupNum = 0;

         rc = _cataInfo.getSubCLList(subCLList);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get group by record, rc=%d", rc);
            goto error;
         }

         for (vector<string>::iterator it = subCLList.begin();
              it != subCLList.end(); it++)
         {
            CataInfo cataInfo;
            rc = _cataAgent.getCataInfo(*it, cataInfo);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get catalog info, rc=%d", rc);
               goto error;
            }

            subGroupNum += cataInfo.getGroupNum();
            _subCataInfo[*it] = cataInfo;
         }

         _groupNum = subGroupNum;
      }
      else
      {
         _groupNum = _cataInfo.getGroupNum();
      }

      _inited = TRUE;

   done:
      bson_destroy(&cataObj);
      if (SDB_INVALID_HANDLE != cursor)
      {
         sdbCloseCursor(cursor);
         sdbReleaseCursor(cursor);
         cursor = SDB_INVALID_HANDLE;
      }
      if (SDB_INVALID_HANDLE != conn)
      {
         sdbDisconnect(conn);
         sdbReleaseConnection(conn);
         conn = SDB_INVALID_HANDLE;
      }
      return rc;
   error:
      goto done;
   }

   INT32 RecordSharding::getGroupByRecord(bson* record,
                                          string& collection,
                                          UINT32& groupId)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_inited, "must be inited");
      SDB_ASSERT(NULL != record, "record can't be NULL");

      if (_groupNum > 1)
      {
         if (_isMainCL)
         {
            map<string, CataInfo>::iterator it;

            rc = _cataInfo.getSubCLNameByRecord(bson_data(record), collection);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get subCL by record, rc=%d", rc);
               goto error;
            }

            it = _subCataInfo.find(collection);
            if (it == _subCataInfo.end())
            {
               rc = SDB_SYS;
               PD_LOG(PDERROR, "failed to get CataInfo by subCL, subCL=%s, rc=%d",
                      collection.c_str(), rc);
               goto error;
            }

            rc = (it->second).getGroupByRecord(bson_data(record), groupId);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get group of subCL[%s] by record, rc=%d",
                      collection.c_str(), rc);
               goto error;
            }
         }
         else
         {
            rc = _cataInfo.getGroupByRecord(bson_data(record), groupId);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get group by record, rc=%d", rc);
               goto error;
            }
            collection = _collectionName;
         }
      }
      else
      {
         groupId = 0;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
