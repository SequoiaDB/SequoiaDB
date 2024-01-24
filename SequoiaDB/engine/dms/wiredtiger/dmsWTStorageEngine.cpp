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

   Source File Name = dmsWTStorageEngine.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTStorageEngine.hpp"
#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTPersistUnit.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "pmdOptionsMgr.hpp"
#include "dmsReadUnit.hpp"

#include <boost/filesystem/operations.hpp>

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStorageEngine implement
    */
   _dmsWTStorageEngine::_dmsWTStorageEngine( _dmsWTStorageService &service,
                                             dmsWTEngineOptions &options )
   : _service( service ),
     _options( options )
   {
   }

   _dmsWTStorageEngine::~_dmsWTStorageEngine()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_OPEN, "_dmsWTStorageEngine::open" )
   INT32 _dmsWTStorageEngine::open( const boost::filesystem::path &dbPath,
                                    const CHAR *config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_OPEN ) ;

      boost::filesystem::path enginePath ;

      PD_CHECK( nullptr == _conn, SDB_SYS, error, PDERROR,
                "Failed to open WiredTiger engine, already opened" ) ;

      rc = _checkDBPath( dbPath, enginePath ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check db path, rc: %d", rc ) ;

      rc = WT_CALL( wiredtiger_open( enginePath.c_str(),
                                     _handler.getHandler(),
                                     config,
                                     &_conn ),
                    nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_CLOSE, "_dmsWTStorageEngine::close" )
   INT32 _dmsWTStorageEngine::close( const CHAR *config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_CLOSE ) ;

      if ( nullptr == _conn )
      {
         goto done ;
      }

      rc = WT_CALL( _conn->close( _conn, config ), nullptr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close WiredTiger engine, rc: %d", rc ) ;
         ossPanic() ;
      }

      _conn = nullptr ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_CLOSE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_RECONFIG, "_dmsWTStorageEngine::reconfig" )
   INT32 _dmsWTStorageEngine::reconfig( const CHAR *config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_RECONFIG ) ;

      boost::filesystem::path enginePath ;

      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to open WiredTiger engine, it is not openned" ) ;

      rc = WT_CALL( _conn->reconfigure( _conn, config ), nullptr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reconfig WiredTiger engine, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_RECONFIG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_OPENSESSION, "_dmsWTStorageEngine::openSession" )
   INT32 _dmsWTStorageEngine::openSession( dmsWTSession &session,
                                           dmsWTSessIsolation isolation )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_OPENSESSION ) ;

      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to open session, engine is not opened" ) ;

      rc = session.open( _conn, isolation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_OPENSESSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_CREATESTORE, "_dmsWTStorageEngine::createStore" )
   INT32 _dmsWTStorageEngine::createStore( const CHAR *uri,
                                           const CHAR *config,
                                           dmsWTStore &store )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_CREATESTORE ) ;

      dmsWTSession sess ;
      WT_SESSION *s = nullptr ;

      SDB_ASSERT( nullptr != uri, "uri should be valid" ) ;
      PD_CHECK( nullptr != uri, SDB_INVALIDARG, error, PDERROR,
                "Failed to create WiredTiger storage object, "
                "uri is null" ) ;
      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to create WiredTiger storage object, "
                "engine is not opened" ) ;

      rc = sess.open( _conn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger session, rc: %d", rc ) ;

      s = sess.getSession() ;
      SDB_ASSERT( nullptr != s, "session should not be null" ) ;

      rc = WT_CALL( s->create( s, uri, config ), s ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create WiredTiger storage, "
                   "rc: %d", rc ) ;

      store.setURI( uri ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_CREATESTORE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_DROPSTORE, "_dmsWTStorageEngine::dropStore" )
   INT32 _dmsWTStorageEngine::dropStore( const CHAR *uri,
                                         const CHAR *config,
                                         IContext *context )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_DROPSTORE ) ;

      dmsWTSession sess ;
      WT_SESSION *s = nullptr ;

      SDB_ASSERT( nullptr != uri, "uri should be valid" ) ;
      PD_CHECK( nullptr != uri, SDB_INVALIDARG, error, PDERROR,
                "Failed to drop WiredTiger store, uri is null" ) ;
      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to drop WiredTiger store, engine is not opened" ) ;

      rc = sess.open( _conn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger session, rc: %d", rc ) ;

      s = sess.getSession() ;
      SDB_ASSERT( nullptr != s, "session should not be null" ) ;

      while ( TRUE )
      {
         rc = WT_CALL( s->drop( s, uri, config ), s ) ;
         if ( SDB_OK != rc && EBUSY == dmsWTGetLastErrorCode() )
         {
            if ( context )
            {
               rc = context->pause() ;
               PD_RC_CHECK( rc, PDERROR, "Failed to pause context, rc: %d", rc ) ;
            }
            ossSleep( 100 ) ;
            if ( context )
            {
               rc = context->resume() ;
               PD_RC_CHECK( rc, PDERROR, "Failed to resume context, rc: %d", rc ) ;
            }
            continue ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to drop WiredTiger store, rc: %d", rc ) ;
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_DROPSTORE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_DROPSTORES, "_dmsWTStorageEngine::dropStores" )
   INT32 _dmsWTStorageEngine::dropStores( const ossPoolList<ossPoolString> &uriList,
                                          const CHAR *config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_DROPSTORES ) ;

      dmsWTSession sess ;
      WT_SESSION *s = nullptr ;

      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to drop WiredTiger store, engine is not opened" ) ;

      rc = sess.open( _conn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger session, rc: %d", rc ) ;

      s = sess.getSession() ;
      SDB_ASSERT( nullptr != s, "session should not be null" ) ;

      for ( auto &uri : uriList )
      {
         while ( TRUE )
         {
            rc = WT_CALL( s->drop( s, uri.c_str(), config ), s ) ;
            if ( SDB_OK != rc && EBUSY == dmsWTGetLastErrorCode() )
            {
               ossSleep( 100 ) ;
               continue ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to drop WiredTiger store, rc: %d", rc ) ;
            break ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_DROPSTORES, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_TRUNCSTORE_SESS, "_dmsWTStorageEngine::truncateStore" )
   INT32 _dmsWTStorageEngine::truncateStore( dmsWTSession &session,
                                             const CHAR *uri,
                                             const CHAR *config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_TRUNCSTORE_SESS ) ;

      dmsWTCursor cursor( session ) ;
      WT_SESSION *s = nullptr ;

      SDB_ASSERT( nullptr != uri, "uri should be valid" ) ;
      PD_CHECK( nullptr != uri, SDB_INVALIDARG, error, PDERROR,
                "Failed to truncate table, uri is null" ) ;
      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to truncate table, engine is not opened" ) ;

      rc = cursor.open( uri, "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

      // check if collection is empty
      rc = cursor.next() ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to next, rc: %d", rc ) ;

      s = session.getSession() ;
      SDB_ASSERT( nullptr != s, "session should not be null" ) ;

      rc = WT_CALL( s->truncate( s, nullptr, cursor.getCursor(), nullptr, config ), s ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_TRUNCSTORE_SESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_COMPACTSTORE_SESS, "_dmsWTStorageEngine::compactStore" )
   INT32 _dmsWTStorageEngine::compactStore( dmsWTSession &session,
                                            const CHAR *uri,
                                            const CHAR *config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_COMPACTSTORE_SESS ) ;

      WT_SESSION *s = nullptr ;

      SDB_ASSERT( nullptr != uri, "uri should be valid" ) ;
      PD_CHECK( nullptr != uri, SDB_INVALIDARG, error, PDWARNING,
                "Failed to truncate table, uri is null" ) ;
      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDWARNING,
                "Failed to truncate table, engine is not opened" ) ;

      s = session.getSession() ;
      SDB_ASSERT( nullptr != s, "session should not be null" ) ;

      rc = WT_CALL( s->compact( s, uri, "timeout=0" ), s ) ;
      PD_RC_CHECK( rc, PDWARNING, "Failed to compact store, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_COMPACTSTORE_SESS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_LOADSTORE, "_dmsWTStorageEngine::loadStore" )
   INT32 _dmsWTStorageEngine::loadStore( const CHAR *uri,
                                         dmsWTStore &store )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_LOADSTORE ) ;

      dmsWTSession sess ;
      dmsWTCursor cursor( sess ) ;
      dmsWTItem value ;

      SDB_ASSERT( nullptr != uri, "uri should be valid" ) ;
      PD_CHECK( nullptr != uri, SDB_INVALIDARG, error, PDERROR,
                "Failed to truncate table, uri is null" ) ;
      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to truncate table, engine is not opened" ) ;

      rc = sess.open( _conn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      // just check if cursor can open
      rc = cursor.open( "metadata:", "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor, rc: %d", rc ) ;

      rc = cursor.searchAndGetValue( uri, value ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to search metadata for [%s], rc: %d", uri, rc ) ;

      store.setURI( uri ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_LOADSTORE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_CREATESTORECURSOR, "_dmsWTStorageEngine::openStoreCursor" )
   INT32 _dmsWTStorageEngine::openStoreCursor( const CHAR *uri,
                                               const CHAR *config,
                                               dmsWTCursor &cursor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_CREATESTORECURSOR ) ;

      SDB_ASSERT( nullptr != uri, "uri should be valid" ) ;
      PD_CHECK( nullptr != uri, SDB_INVALIDARG, error, PDERROR,
                "Failed to open store cursor, uri is null" ) ;
      PD_CHECK( nullptr != _conn, SDB_SYS, error, PDERROR,
                "Failed to open store cursor, engine is not opened" ) ;

      rc = cursor.open( uri, "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open store cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_CREATESTORECURSOR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE__DUMPURILISTBYPREFIX, "_dmsWTStorageEngine::dumpURIListByPrefix" )
   INT32 _dmsWTStorageEngine::dumpURIListByPrefix( const CHAR *prefix,
                                                   ossPoolList< ossPoolString > &uriList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE__DUMPURILISTBYPREFIX ) ;

      dmsWTSession sess ;
      dmsWTCursor cursor( sess ) ;

      UINT32 prefixLength = ossStrlen( prefix ) ;

      rc = sess.open( _conn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, "
                   "rc: %d", rc ) ;

      rc = cursor.open( "metadata:", "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      while ( TRUE )
      {
         rc = cursor.next() ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move metadata cursor to next, rc: %d", rc ) ;

         const CHAR *key = NULL ;
         rc = cursor.getKey( key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get metadata cursor key, rc: %d", rc ) ;

         if ( 0 == ossStrncmp( key, prefix, prefixLength ) )
         {
            try
            {
               uriList.push_back( key ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to push uri to list, occur exception: %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE__DUMPURILISTBYPREFIX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE__DUMPURILIST, "_dmsWTStorageEngine::dumpURIList" )
   INT32 _dmsWTStorageEngine::dumpURIList( ossPoolList< ossPoolString > &uriList )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE__DUMPURILIST ) ;

      dmsWTSession sess ;
      dmsWTCursor cursor( sess ) ;

      rc = sess.open( _conn ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, "
                   "rc: %d", rc ) ;

      rc = cursor.open( "metadata:", "" ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      while ( TRUE )
      {
         rc = cursor.next() ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move metadata cursor to next, rc: %d", rc ) ;

         const CHAR *key = NULL ;
         rc = cursor.getKey( key ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get metadata cursor key, rc: %d", rc ) ;

         try
         {
            uriList.push_back( key ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to push uri to list, occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE__DUMPURILIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_GETPERSISTSESSION, "_dmsWTStorageEngine::getPersistSession" )
   INT32 _dmsWTStorageEngine::getPersistSession( IExecutor *executor,
                                                 dmsWTSessionHolder &sessionHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_GETPERSISTSESSION ) ;

      dmsWTPersistUnit *pu = nullptr ;

      if ( executor &&
           executor->getOperationContext() &&
           executor->getOperationContext()->getPersistUnit() )
      {
         pu = dynamic_cast<dmsWTPersistUnit *>(
               executor->getSession()->getOperationContext()->getPersistUnit() ) ;

         if ( pu )
         {
            sessionHolder.setSession( &( pu->getWriteSession() ) ) ;
         }
      }

      if ( !sessionHolder.getSession().isOpened() )
      {
         rc = openSession( sessionHolder.getSession() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_GETPERSISTSESSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_GETREADSESSION, "_dmsWTStorageEngine::getReadSession" )
   INT32 _dmsWTStorageEngine::getReadSession( IExecutor *executor,
                                              dmsWTSessionHolder &sessionHolder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_GETREADSESSION ) ;

      dmsWTPersistUnit *pu = nullptr ;

      if ( executor &&
           executor->getOperationContext() )
      {
         BOOLEAN got = FALSE ;
         if ( executor->getOperationContext()->getReadUnit() )
         {
            dmsReadUnit *ru = dynamic_cast<dmsReadUnit *>(
                  executor->getOperationContext()->getReadUnit() ) ;
            if ( ru )
            {
               IStorageSession *sess = ru->getSession() ;
               dmsWTSession *wtSess = dynamic_cast<dmsWTSession *>( sess ) ;
               if ( wtSess )
               {
                  sessionHolder.setSession( wtSess ) ;
                  got = TRUE ;
               }
            }
         }
         if ( !got &&
              executor->getOperationContext()->getPersistUnit() )
         {
            pu = dynamic_cast<dmsWTPersistUnit *>(
                  executor->getSession()->getOperationContext()->getPersistUnit() ) ;

            if ( pu )
            {
               sessionHolder.setSession( &( pu->getReadSession() ) ) ;
            }
         }
      }

      if ( !sessionHolder.getSession().isOpened() )
      {
         rc = openSession( sessionHolder.getSession() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_GETREADSESSION, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE__CHKDBPATH, "_dmsWTStorageEngine::_checkDBPath" )
   INT32 _dmsWTStorageEngine::_checkDBPath( const boost::filesystem::path &dbPath,
                                            boost::filesystem::path &enginePath )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE__CHKDBPATH ) ;

      enginePath = dbPath / "wiredtiger" ;
      boost::filesystem::path journalPath = enginePath / "journal" ;
      if ( !boost::filesystem::exists( enginePath ) )
      {
         try
         {
               boost::filesystem::create_directory( enginePath ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to create engine directory, "
                    "occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }
      if ( !boost::filesystem::exists( journalPath ) )
      {
         try
         {
               boost::filesystem::create_directory( journalPath ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to create journal directory, "
                    "occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE__CHKDBPATH, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTSTORAGEENGINE_CHECKPOINT, "_dmsWTStorageEngine::checkPoint" )
   INT32 _dmsWTStorageEngine::checkPoint( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTSTORAGEENGINE_CHECKPOINT ) ;

      // checkpoint
      dmsWTSession sess ;
      WT_SESSION *s = nullptr ;

      rc = openSession( sess ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;

      s = sess.getSession() ;
      rc = WT_CALL( s->checkpoint( s, nullptr ), s ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to checkpoint, rc: %d", rc ) ;

      PD_LOG( PDEVENT, "Checkpoint done" ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTSTORAGEENGINE_CHECKPOINT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
