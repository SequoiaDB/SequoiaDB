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

   Source File Name = dmsWTStoreHolder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTStoreHolder.hpp"
#include "wiredtiger/dmsWTStorageService.hpp"
#include "wiredtiger/dmsWTDataCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

namespace engine
{
namespace wiredtiger
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTOREHOLDER_GETCOUNT, "_dmsWTStoreHolder::getCount" )
   INT32 _dmsWTStoreHolder::getCount( UINT64 &count,
                                      IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTOREHOLDER_GETCOUNT ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         rc = cursor.getCount( count ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to count from store, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTOREHOLDER_GETCOUNT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTOREHOLDER_GESTATS, "_dmsWTStoreHolder::getStats" )
   INT32 _dmsWTStoreHolder::getStats( INT32 statsKey,
                                      dmsWTStatsCatalog statsCatalog,
                                      INT64 &statsValue,
                                      IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTOREHOLDER_GESTATS ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getStatsURI(), dmsWTGetStatsConfig( statsCatalog ) ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to open cursor, rc: %d", rc ) ;

         rc = cursor.search( statsKey ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to count from store statistics, rc: %d", rc ) ;

         rc = cursor.getValue( statsValue ) ;
         PD_RC_CHECK( rc, PDWARNING, "Failed to get value from store statistics, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTOREHOLDER_GESTATS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTOREHOLDER_GESSTORETOTALSIZE, "_dmsWTStoreHolder::getStoreTotalSize" )
   INT32 _dmsWTStoreHolder::getStoreTotalSize( UINT64 &totalSize, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTOREHOLDER_GESSTORETOTALSIZE ) ;

      rc = getStats( WT_STAT_DSRC_BLOCK_SIZE, dmsWTStatsCatalog::STATS_SIZE,
                     (INT64 &)totalSize, executor ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get store total size, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTOREHOLDER_GESSTORETOTALSIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTOREHOLDER_GESSTOREFREESIZE, "_dmsWTStoreHolder::getStoreFreeSize" )
   INT32 _dmsWTStoreHolder::getStoreFreeSize( UINT64 &freeSize, IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTOREHOLDER_GESSTOREFREESIZE ) ;

      rc = getStats( WT_STAT_DSRC_BLOCK_REUSE_BYTES, dmsWTStatsCatalog::STATS_FAST,
                     (INT64 &)freeSize, executor ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to get store free size, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTOREHOLDER_GESSTOREFREESIZE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTOREHOLDER__VALIDATESTORE, "_dmsWTStoreHolder::_validateStore" )
   INT32 _dmsWTStoreHolder::_validateStore( _dmsWTStoreValidator &validator,
                                            IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTOREHOLDER__VALIDATESTORE ) ;

      dmsWTSessionHolder sessionHolder ;
      rc = _engine.getPersistSession( executor, sessionHolder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist session, rc: %d", rc ) ;

      {
         dmsWTCursor cursor( sessionHolder.getSession() ) ;

         rc = cursor.open( _store.getURI(), "" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

         while ( TRUE )
         {
            dmsWTItem keyItem, valueItem ;
            rc = cursor.next() ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            rc = cursor.getKey( keyItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

            rc = cursor.getValue( valueItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

            rc = validator.validate( keyItem, valueItem ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to validate store, rc: %d", rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTOREHOLDER__VALIDATESTORE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
