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

   Source File Name = rplSdbOutputter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rplSdbOutputter.hpp"
#include "msgDef.h"
#include "dms.hpp"

using namespace sdbclient ;
using namespace bson ;

namespace replay
{
   const CHAR RPL_OUTPUT_SEQUOIADB[] = "SEQUOIADB" ;

   rplSdbOutputter::rplSdbOutputter( BOOLEAN updateWithShardingKey )
   {
      _updateWithShardingKey = updateWithShardingKey ;
      _sdb = NULL ;
   }

   rplSdbOutputter::~rplSdbOutputter()
   {
      if ( NULL != _sdb )
      {
         _sdb->disconnect() ;
         SAFE_OSS_DELETE( _sdb ) ;
      }
   }

   INT32 rplSdbOutputter::init( const CHAR *hostName, const CHAR *svcName,
                                const CHAR *user, const CHAR *passwd,
                                BOOLEAN useSSL )
   {
      INT32 rc = SDB_OK ;
      _sdb = new(std::nothrow) sdb( useSSL ) ;
      if (NULL == _sdb)
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to new sdbclient::sdb, rc=%d", rc ) ;
         goto error ;
      }

      rc = _sdb->connect( hostName, svcName, user, passwd ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to connect to sdb[%s:%s], rc=%d", hostName,
                 svcName, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _sdb ) ;
      goto done ;
   }

   string rplSdbOutputter::getType()
   {
      return RPL_OUTPUT_SEQUOIADB ;
   }

   INT32 rplSdbOutputter::insertRecord( const CHAR *clFullName, UINT64 lsn,
                                        const BSONObj &obj,
                                        const UINT64 &opTimeMicroSecond )
   {
      INT32 rc = SDB_OK ;
      sdbCollection cl ;

      rc = _sdb->getCollection( clFullName, cl ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection:%s, rc=%d",
                 clFullName, rc ) ;
         goto error ;
      }

      rc = cl.insert( obj ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_IXM_DUP_KEY == rc )
         {
            /* skip duplicate key error */
            rc = SDB_OK ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to insert record(%s), rc=%d",
                    obj.toString(FALSE, TRUE).c_str(), rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplSdbOutputter::deleteRecord( const CHAR *clFullName, UINT64 lsn,
                                        const BSONObj &oldObj,
                                        const UINT64 &opTimeMicroSecond )
   {
      INT32 rc = SDB_OK ;
      sdbCollection cl ;
      BSONObj condition ;
      BSONObj hint = BSON( "" << "$id" ) ;

      {
         BSONObjBuilder conditionBuilder ;
         BSONElement idEle = oldObj.getField( DMS_ID_KEY_NAME ) ;
         if ( idEle.eoo() )
         {
            PD_LOG( PDWARNING, "Failed to parse oid from bson:[%s]",
                    oldObj.toString().c_str() ) ;
            rc = SDB_INVALIDARG;
            goto error;
         }

         conditionBuilder.append( idEle ) ;
         condition = conditionBuilder.obj() ;
      }

      rc = _sdb->getCollection( clFullName, cl ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection: %s, rc=%d",
                 clFullName, rc ) ;
         goto error ;
      }

      rc = cl.del( condition, hint ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to delete record[%s], rc=%d",
                 condition.toString(FALSE, TRUE).c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplSdbOutputter::updateRecord( const CHAR *clFullName, UINT64 lsn,
                                        const BSONObj &matcher,
                                        const BSONObj &newModifier,
                                        const BSONObj &shardingKey,
                                        const BSONObj &oldModifier,
                                        const UINT64 &opTimeMicroSecond,
                                        const UINT32 &logWriteMod )
   {
      INT32 rc = SDB_OK ;
      sdbCollection cl ;
      BSONObj match;
      BSONObj modifier;
      BSONObj hint = BSON( "" << "$id" );

      if ( DMS_LOG_WRITE_MOD_FULL == logWriteMod )
      {
         modifier = BSON( "$replace" << newModifier ) ;
      }
      else
      {
         modifier = newModifier ;
      }

      if ( !shardingKey.isEmpty() && _updateWithShardingKey )
      {
         BSONObjBuilder builder ;
         builder.appendElements( matcher ) ;
         builder.appendElements( shardingKey ) ;
         match = builder.obj() ;
      }
      else
      {
         match = matcher ;
      }

      rc = _sdb->getCollection( clFullName, cl ) ;
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to get collection:%s, rc=%d",
                 clFullName, rc ) ;
         goto error ;
      }

      rc = cl.update( modifier, match, hint, UPDATE_KEEP_SHARDINGKEY ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to update record, match[%s], modifier[%s], "
                 "rc=%d", match.toString(FALSE, TRUE).c_str(),
                 modifier.toString(FALSE, TRUE).c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplSdbOutputter::truncateCL( const CHAR *clFullName, UINT64 lsn )
   {
      INT32 rc = SDB_OK ;
      sdbCollection cl ;

      rc = _sdb->getCollection( clFullName, cl ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection: %s, rc=%d",
                 clFullName, rc) ;
         goto error ;
      }

      rc = cl.truncate() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to truncate collection[%s], rc=%d",
                 clFullName, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplSdbOutputter::pop( const CHAR *clFullName, UINT64 lsn,
                               INT64 logicalID, INT8 direction )
   {
      INT32 rc = SDB_OK ;
      sdbCollection cl ;

      rc  = _sdb->getCollection( clFullName, cl ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get collection: %s, rc=%d",
                 clFullName, rc ) ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_LOGICAL_ID, logicalID ) ;
         builder.append( FIELD_NAME_DIRECTION, direction ) ;
         BSONObj option = builder.obj() ;

         rc = cl.pop( option ) ;
         if ( SDB_OK != rc )
         {
            if ( SDB_INVALIDARG == rc )
            {
               rc = SDB_OK ;
            }
            else
            {
               PD_LOG( PDERROR, "Failed to do pop[option: %s] on "
                       "collection[%s], rc=%d", option.toString().c_str(),
                       clFullName, rc) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

