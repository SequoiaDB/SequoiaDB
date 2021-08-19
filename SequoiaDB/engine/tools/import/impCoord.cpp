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

   Source File Name = impCoord.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impCoord.hpp"
#include "msgDef.h"
#include "../client/bson/bson.h"
#include "pd.hpp"
#include "ossUtil.hpp"
#include <sstream>

namespace import
{
   static INT32 _getService(bson_iterator* it, string& svcname)
   {
      INT32 rc = SDB_OK;
      bson obj;
      BOOLEAN find = FALSE;

      bson_init(&obj);

      SDB_ASSERT(NULL != it, "it can't be NULL");

      while (BSON_EOO != bson_iterator_next(it))
      {
         if (BSON_OBJECT != bson_iterator_type(it))
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "invalid service data");
            goto error;
         }

         bson_iterator_subobject(it, &obj);

         if (BSON_INT != bson_find(it, &obj, FIELD_NAME_SERVICE_TYPE))
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "invalid type of service type in bson");
            goto error;
         }

         if (0 != bson_iterator_int(it))
         {
            continue;
         }

         if (BSON_STRING != bson_find(it, &obj, FIELD_NAME_NAME))
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "invalid type of service name in bson");
            goto error;
         }

         svcname = bson_iterator_string(it);
         find = TRUE;
         break;
      }

      if (!find)
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to find service name in bson");
         goto error;
      }

   done:
      bson_destroy(&obj);
      return rc;
   error:
      goto done;
   }

   static INT32 _getCoord(bson* obj, string& hostname, string& svcname)
   {
      INT32 rc = SDB_OK;
      bson_iterator it;
      bson_iterator subit;

      SDB_ASSERT(NULL != obj, "obj can't be NULL");

      if (BSON_STRING != bson_find(&it, obj, FIELD_NAME_HOST))
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to find hostname in bson");
         goto error;
      }

      hostname = bson_iterator_string(&it);

      if (BSON_ARRAY != bson_find(&it, obj, FIELD_NAME_SERVICE))
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "invalid coord data");
         goto error;
      }

      bson_iterator_subiterator(&it, &subit);

      rc = _getService(&subit, svcname);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get service name");
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _getCoords(bson* obj, vector<Host>& coords)
   {
      INT32 rc = SDB_OK;
      bson_iterator it;
      bson_iterator subit;
      bson coord;
      bson service;

      bson_init(&coord);
      bson_init(&service);

      SDB_ASSERT(NULL != obj, "obj can't be NULL");

      if (BSON_ARRAY != bson_find(&it, obj, FIELD_NAME_GROUP))
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "invalid coord data");
         goto error;
      }

      bson_iterator_subiterator(&it, &subit);

      while (BSON_EOO != bson_iterator_next(&subit))
      {
         Host host;

         if (BSON_OBJECT != bson_iterator_type(&subit))
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "invalid coord data");
            goto error;
         }

         bson_iterator_subobject(&subit, &coord);

         rc = _getCoord(&coord, host.hostname, host.svcname);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "invalid coord data");
            continue;
         }

         coords.push_back(host);
      }

      if (coords.empty())
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to get coords");
         goto error;
      }

   done:
      bson_destroy(&coord);
      bson_destroy(&service);
      return rc;
   error:
      goto done;
   }

   Coords::Coords()
   {
      _inited = FALSE;
      _refCount = 0;
   }

   Coords::~Coords()
   {
      _coords.clear();
   }

   INT32 Coords::init(const vector<Host>& hosts,
                      const string& user,
                      const string& password,
                      BOOLEAN useSSL)
   {
      #define COORD_GROUP_NAME "SYSCoord"
      sdbConnectionHandle conn = SDB_INVALID_HANDLE;
      sdbCursorHandle cursor = SDB_INVALID_HANDLE;
      bson cond;
      bson result;

      INT32 rc = SDB_OK;

      SDB_ASSERT(!_inited, "alreay inited");

      _user = user;
      _password = password;
      _useSSL = useSSL;

      bson_init(&cond);
      bson_init(&result);

      if (BSON_OK != bson_append_string(&cond,
                                        FIELD_NAME_GROUPNAME,
                                        COORD_GROUP_NAME))
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to append string to bson");
         goto error;
      }

      if (BSON_OK != bson_finish(&cond))
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to finish bson");
         goto error;
      }

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
         _disconnect(conn);

         rc = _connect(host.hostname, host.svcname, conn);
         if (SDB_OK != rc)
         {
            PD_LOG(PDWARNING,
                   "failed to connect to server %s:%s, rc = %d, usessl=%d",
                   host.hostname.c_str(), host.svcname.c_str(), rc, _useSSL);
            continue;
         }

         rc = sdbGetList(conn, SDB_LIST_GROUPS, &cond, NULL, NULL, &cursor);
         if (SDB_OK != rc)
         {
            if (SDB_RTN_COORD_ONLY == rc)
            {
               // may be standalone node or data node in replica,
               // so we just set _hostname to it
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

         rc = sdbNext(cursor, &result);
         if (SDB_OK != rc)
         {
            if (SDB_INVALID_HANDLE != cursor)
            {
               sdbCloseCursor(cursor);
               sdbReleaseCursor(cursor);
               cursor = SDB_INVALID_HANDLE;
            }
            PD_LOG(PDWARNING, "failed to get result from cursor, rc=%d", rc);
            rc = SDB_OK;
            continue;
         }

         break;
      }

      if (SDB_INVALID_HANDLE != cursor && NULL != bson_data(&result))
      {
         rc = _getCoords(&result, _coords);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get coordinators, rc=%d", rc);
            goto error;
         }
      }

      for (vector<Host>::const_iterator it = hosts.begin();
           it != hosts.end(); it++)
      {
         Host host;
         host.hostname = (*it).hostname;
         host.svcname = (*it).svcname;
         _coords.push_back(host);
      }

      Hosts::removeDuplicate(_coords);

      for (vector<Host>::iterator i = _coords.begin(); i != _coords.end();)
      {
         Host& host = *i;

         rc = _checkCoord(host.hostname, host.svcname);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to connect to %s:%s, rc=%d",
                   host.hostname.c_str(), host.svcname.c_str(), rc);
            i = _coords.erase(i);
            rc = SDB_OK;
         }
         else
         {
            i++;
         }
      }

      if (_coords.empty())
      {
         rc = SDB_SYS;
         PD_LOG(PDERROR, "failed to get coordinators, rc=%d", rc);
         goto error;
      }
      else
      {
         stringstream ss;

         for (vector<Host>::iterator it = _coords.begin();
              it != _coords.end(); it++)
         {
            Host& host = *it;
            ss << host.hostname << ":" << host.svcname << ",";
         }

         PD_LOG(PDINFO, "%d hosts: %s", _coords.size(), ss.str().c_str());
      }

      _inited = TRUE;

   done:
      bson_destroy(&cond);
      bson_destroy(&result);
      if (SDB_INVALID_HANDLE != cursor)
      {
         sdbCloseCursor(cursor);
         sdbReleaseCursor(cursor);
         cursor = SDB_INVALID_HANDLE;
      }
      _disconnect(conn);
      return rc;
   error:
      goto done;
   }

   INT32 Coords::_connect(const string& hostname,
                          const string& svcname,
                          sdbConnectionHandle& conn)
   {
      INT32 rc = SDB_OK;

      if (_useSSL)
      {
         rc = sdbSecureConnect(hostname.c_str(), svcname.c_str(),
                               _user.c_str(), _password.c_str(), &conn);
      }
      else
      {
         rc = sdbConnect(hostname.c_str(), svcname.c_str(),
                         _user.c_str(), _password.c_str(), &conn);
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR,
                "Failed to connect to database %s:%s, rc = %d, usessl=%d",
                hostname.c_str(), svcname.c_str(), rc, _useSSL);
      }

      return rc;
   }

   void Coords::_disconnect(sdbConnectionHandle& conn)
   {
      if (SDB_INVALID_HANDLE != conn)
      {
         sdbDisconnect(conn);
         sdbReleaseConnection(conn);
         conn = SDB_INVALID_HANDLE;
      }
   }

   INT32 Coords::_checkCoord(const string& hostname, const string& svcname)
   {
      sdbConnectionHandle conn = SDB_INVALID_HANDLE;
      INT32 rc = SDB_OK;

      rc = _connect(hostname, svcname, conn);
      _disconnect(conn);

      return rc;
   }

   INT32 Coords::getRandomCoord(string& hostname, string& svcname)
   {
      return Coords::getRandomCoord(_coords, _refCount, hostname, svcname);
   }

   INT32 Coords::getRandomCoord(vector<Host>& hosts, UINT32& refCount, 
                                string& hostname, string& svcname)
   {
      INT32 rc = SDB_OK;
      INT32 i = 0;
      INT32 thres = 0;

      INT32 size = hosts.size();
      if (0 == size)
      {
         rc = SDB_SYS;
         goto error;
      }

      // choose a random coordinator, but it can't be 
      // used much more than other coordinator
      // try 100 times at most
      thres = refCount / size + 1;

      for (INT32 t = 0; t < 100; t++)
      {
         i = ossRand() % size;
         Host& host = hosts[i];
         if (host.refCount + 1 <= thres)
         {
            break;
         }
      }

      {
         Host& host = hosts[i];
         hostname = host.hostname;
         svcname = host.svcname;
         host.refCount++;
         refCount++;
      }

   done:
      return rc;
   error:
      goto done;
   }
}

