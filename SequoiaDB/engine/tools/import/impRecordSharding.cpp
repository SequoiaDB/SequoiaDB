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
#include <iostream>

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
      INT32 cataCount = 0;

      SDB_ASSERT(!_inited, "Alreay inited");

      _hosts    = &hosts ;
      _user     = user ;
      _password = password ;
      _csname   = csname ;
      _clname   = clname ;
      _useSSL   = useSSL ;
      _collectionName = _csname + "." + _clname ;

      //get collection info
      rc = _getCatalogInfo( FALSE, cataCount ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get catalog info, rc=%d", rc ) ;
         goto error ;
      }

      // no catalog info
      if ( 0 == cataCount )
      {
         _inited = TRUE ;
         goto done ;
      }

      rc = _cataAgent.getCataInfo( _collectionName, _cataInfo ) ;
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to get catalog info, rc=%d", rc ) ;
         goto error;
      }

      _isMainCL = _cataInfo.isMainCL();

      if ( _isMainCL )
      {
         INT32 subGroupNum = 0 ;
         vector<string> subCLList ;

         //get sub collection info
         rc = _getCatalogInfo( TRUE, cataCount ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get catalog info, rc=%d", rc ) ;
            goto error ;
         }

         rc = _cataInfo.getSubCLList(subCLList);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get group by record, rc=%d", rc);
            goto error;
         }

         for (vector<string>::iterator it = subCLList.begin();
              it != subCLList.end(); it++)
         {
            CataInfo cataInfo;
            rc = _cataAgent.getCataInfo(*it, cataInfo);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to get catalog info, rc=%d", rc);
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
      return rc;
   error:
      goto done;
   }

   INT32 RecordSharding::_getCatalogInfo( BOOLEAN getSubCL, INT32 &cataCount )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pField = NULL ;
      sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
      sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
      bson cataObj ;
      bson condition ;

      bson_init( &cataObj ) ;
      bson_init( &condition ) ;

      if( getSubCL )
      {
         pField = FIELD_NAME_MAINCLNAME ;
      }
      else
      {
         pField = FIELD_NAME_NAME ;
      }

      if( BSON_ERROR == bson_append_string( &condition, pField,
                                            _collectionName.c_str() ) )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         PD_LOG( PDERROR, "Failed to build catalog condition bson, rc=%d",
                 rc ) ;
         goto error ;
      }

      if( BSON_ERROR == bson_finish( &condition ) )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         PD_LOG( PDERROR, "Failed to build catalog condition bson, rc=%d",
                 rc ) ;
         goto error;
      }

      for ( vector<Host>::const_iterator it = _hosts->begin();
            it != _hosts->end(); ++it )
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

         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to connect to server %s:%s, usessl: %d, "
                    "rc: %d", host.hostname.c_str(), host.svcname.c_str(),
                    _useSSL, rc );
            rc = SDB_OK;
            continue;
         }

         rc = sdbGetSnapshot( conn, SDB_SNAP_CATALOG, &condition,
                              NULL, NULL, &cursor ) ;
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
               // may be standalone node or data node in replica,
               // so we just set _hostname to it
               PD_LOG(PDWARNING, "%s:%s is not coordinator",
                      host.hostname.c_str(), host.svcname.c_str());
               rc = SDB_OK;
               continue;
            }

            PD_LOG(PDWARNING, "Failed to get coordinator group from %s:%s, rc = %d",
                   host.hostname.c_str(), host.svcname.c_str(), rc);
            rc = SDB_OK;
            continue;
         }

         break;
      }

      if ( SDB_INVALID_HANDLE == cursor )
      {
         rc = SDB_OK ;
         PD_LOG( PDWARNING, "Failed to get coordinator group" ) ;
         goto done ;
      }

      for(;;)
      {
         rc = sdbNext( cursor, &cataObj ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }

            PD_LOG( PDERROR, "Failed to get cataObj from cursor, rc=%d", rc ) ;
            goto error ;
         }

         rc = _cataAgent.updateCatalog( bson_data( &cataObj ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to update catalog agent, rc=%d", rc ) ;
            goto error ;
         }

         ++cataCount ;
      }

   done:
      bson_destroy( &condition ) ;
      bson_destroy( &cataObj ) ;
      if ( SDB_INVALID_HANDLE != cursor )
      {
         sdbCloseCursor( cursor ) ;
         sdbReleaseCursor( cursor ) ;
         cursor = SDB_INVALID_HANDLE ;
      }
      if ( SDB_INVALID_HANDLE != conn )
      {
         sdbDisconnect( conn ) ;
         sdbReleaseConnection( conn ) ;
         conn = SDB_INVALID_HANDLE ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordSharding::getGroupByRecord(bson* record,
                                          string& collection,
                                          UINT32& groupId)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_inited, "Must be inited");
      SDB_ASSERT(NULL != record, "Record can't be NULL");

      if ( _isMainCL )
      {
         map<string, CataInfo>::iterator it;

         rc = _cataInfo.getSubCLNameByRecord(bson_data(record), collection);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get subCL by record, rc=%d", rc);
            goto error;
         }

         it = _subCataInfo.find(collection);
         if (it == _subCataInfo.end())
         {
            rc = SDB_SYS;
            PD_LOG(PDERROR, "Failed to get CataInfo by subCL, subCL=%s, rc=%d",
                   collection.c_str(), rc);
            goto error;
         }

         rc = (it->second).getGroupByRecord(bson_data(record), groupId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get group of subCL[%s] by record, rc=%d",
                   collection.c_str(), rc);
            goto error;
         }
      }
      else
      {
         rc = _cataInfo.getGroupByRecord(bson_data(record), groupId);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get group by record, rc=%d", rc);
            goto error;
         }
         collection = _collectionName;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 RecordSharding::getAllGroupID( vector<UINT32>& list )
   {
      INT32 rc = SDB_OK ;

      if ( _isMainCL )
      {
         map<string, CataInfo>::iterator it ;

         for ( it = _subCataInfo.begin(); it != _subCataInfo.end(); ++it )
         {
            rc = it->second.getAllGroupID( list ) ;
            if ( rc )
            {
               goto error ;
            }
         }
      }
      else
      {
         rc = _cataInfo.getAllGroupID( list ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
