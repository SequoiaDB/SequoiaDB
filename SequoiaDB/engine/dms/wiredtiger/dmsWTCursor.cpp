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

   Source File Name = dmsWTCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTCursor.hpp"
#include "wiredtiger/dmsWTSession.hpp"
#include "wiredtiger/dmsWTUtil.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pd.hpp"

using namespace std ;

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTCursor implement
    */
   _dmsWTCursor::_dmsWTCursor( dmsWTSession &session )
   : _session( session ),
     _cursor( nullptr )
   {
   }

   _dmsWTCursor::~_dmsWTCursor()
   {
      close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_OPEN, "_dmsWTCursor::open" )
   INT32 _dmsWTCursor::open( const ossPoolString &uri,
                             const ossPoolString &config )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_OPEN ) ;

      WT_SESSION *sess = nullptr ;

      PD_CHECK( _session.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to open WiredTiger cursor, session is not opened" ) ;
      PD_CHECK( nullptr == _cursor, SDB_SYS, error, PDERROR,
                "Failed to open WiredTiger cursor, already opened" ) ;

      sess = _session.getSession() ;
      rc = WT_CALL( sess->open_cursor( sess,
                                       uri.c_str(),
                                       nullptr,
                                       config.c_str(),
                                       &_cursor ),
                    sess ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open WiredTiger cursor [%s], "
                   "rc: %d", uri.c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_OPEN, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_CLOSE, "_dmsWTCursor::close" )
   INT32 _dmsWTCursor::close()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_CLOSE ) ;

      if ( nullptr != _cursor )
      {
         rc = WT_CALL( _cursor->close( _cursor ), _session.getSession() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDWARNING, "Failed to close WiredTiger session, "
                  "rc: %d", rc ) ;
         }
         _cursor = nullptr ;
      }

      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_CLOSE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_NEXT, "_dmsWTCursor::next" )
   INT32 _dmsWTCursor::next()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_NEXT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to move cursor to next, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->next( _cursor ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to next, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_NEXT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_PREV, "_dmsWTCursor::prev" )
   INT32 _dmsWTCursor::prev()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_PREV ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to move cursor to prev, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->prev( _cursor ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to prev, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_PREV, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_GETKEY_INT, "_dmsWTCursor::getKey" )
   INT32 _dmsWTCursor::getKey( UINT64 &key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_GETKEY_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->get_key( _cursor, &key ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_GETKEY_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_GETKEY_STRING, "_dmsWTCursor::getKey" )
   INT32 _dmsWTCursor::getKey( const CHAR *&key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_GETKEY_STRING ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->get_key( _cursor, &key ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_GETKEY_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_GETKEY_ITEM, "_dmsWTCursor::getKey" )
   INT32 _dmsWTCursor::getKey( const dmsWTItem &key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_GETKEY_ITEM ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->get_key( _cursor, key.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_GETKEY_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_GETVALUE, "_dmsWTCursor::getValue" )
   INT32 _dmsWTCursor::getValue( dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_GETVALUE ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get value from cursor, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->get_value( _cursor,  value.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_GETVALUE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_GETVALUE_INT, "_dmsWTCursor::getValue" )
   INT32 _dmsWTCursor::getValue( INT64 &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_GETVALUE_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get value from cursor, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->get_value( _cursor, nullptr, nullptr, &value ),
                    _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_GETVALUE_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_INSERT_INT, "_dmsWTCursor::insert" )
   INT32 _dmsWTCursor::insert( UINT64 key, const dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_INSERT_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to insert key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;
      _cursor->set_value( _cursor, value.get() ) ;

      rc = WT_CALL( _cursor->insert( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_INSERT_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_UPDATE_INT, "_dmsWTCursor::update" )
   INT32 _dmsWTCursor::update( UINT64 key, const dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_UPDATE_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to update key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;
      _cursor->set_value( _cursor, value.get() ) ;

      rc = WT_CALL( _cursor->update( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_UPDATE_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_REMOVE_INT, "_dmsWTCursor::remove" )
   INT32 _dmsWTCursor::remove( UINT64 key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_REMOVE_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to remove key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->remove( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_REMOVE_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_INSERT_STRING, "_dmsWTCursor::insert" )
   INT32 _dmsWTCursor::insert( const CHAR *key, const dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_INSERT_STRING ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to insert key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;
      _cursor->set_value( _cursor, value.get() ) ;

      rc = WT_CONFLICT_CALL( _cursor->insert( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_INSERT_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_UPDATE_STRING, "_dmsWTCursor::update" )
   INT32 _dmsWTCursor::update( const CHAR *key, const dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_UPDATE_STRING ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to update key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;
      _cursor->set_value( _cursor, value.get() ) ;

      rc = WT_CALL( _cursor->update( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_UPDATE_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_REMOVE_STRING, "_dmsWTCursor::remove" )
   INT32 _dmsWTCursor::remove( const CHAR *key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_REMOVE_STRING ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to remove key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->remove( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_REMOVE_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_INSERT_ITEM, "_dmsWTCursor::insert" )
   INT32 _dmsWTCursor::insert( const dmsWTItem &key, const dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_INSERT_ITEM ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to insert key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key.get() ) ;
      _cursor->set_value( _cursor, value.get() ) ;

      rc = WT_CONFLICT_CALL( _cursor->insert( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to insert key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_INSERT_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_UPDATE_ITEM, "_dmsWTCursor::update" )
   INT32 _dmsWTCursor::update( const dmsWTItem &key, const dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_UPDATE_ITEM ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to update key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key.get() ) ;
      _cursor->set_value( _cursor, value.get() ) ;

      rc = WT_CALL( _cursor->update( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to update key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_UPDATE_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_REMOVE_ITEM, "_dmsWTCursor::remove" )
   INT32 _dmsWTCursor::remove( const dmsWTItem &key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_REMOVE_ITEM ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to remove key by cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key.get() ) ;

      rc = WT_CALL( _cursor->remove( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove key by cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_REMOVE_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCH_INT, "_dmsWTCursor::search" )
   INT32 _dmsWTCursor::search( UINT64 key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCH_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->search( _cursor ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCH_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCH_STRING, "_dmsWTCursor::search" )
   INT32 _dmsWTCursor::search( const CHAR *key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCH_STRING ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->search( _cursor ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCH_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCH_ITEM, "_dmsWTCursor::search" )
   INT32 _dmsWTCursor::search( const dmsWTItem &key )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCH_ITEM ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key.get() ) ;

      rc = WT_CALL( _cursor->search( _cursor ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCH_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHNEXT_INT, "_dmsWTCursor::searchNext" )
   INT32 _dmsWTCursor::searchNext( UINT64 key, BOOLEAN isAfter, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHNEXT_INT ) ;

      INT32 exact = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search near key from cursor, rc: %d", rc ) ;

      isFound = ( 0 == exact ) ;
      if ( ( 0 > exact ) ||
           ( isAfter && 0 == exact ) )
      {
         rc = WT_CALL( _cursor->next( _cursor ), _session.getSession() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to next, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHNEXT_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHNEXT_STRING, "_dmsWTCursor::searchNext" )
   INT32 _dmsWTCursor::searchNext( const CHAR *key, BOOLEAN isAfter, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHNEXT_STRING ) ;

      INT32 exact = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search near key from cursor, rc: %d", rc ) ;

      isFound = ( 0 == exact ) ;
      if ( ( 0 > exact ) ||
           ( isAfter && 0 == exact ) )
      {
         rc = WT_CALL( _cursor->next( _cursor ), _session.getSession() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to next, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHNEXT_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHNEXT_ITEM, "_dmsWTCursor::searchNext" )
   INT32 _dmsWTCursor::searchNext( const dmsWTItem &key, BOOLEAN isAfter, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHNEXT_ITEM ) ;

      INT32 exact = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key.get() ) ;

      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search near key from cursor, rc: %d", rc ) ;

#if defined(_DEBUG)
      {
         utilSlice keySlice( key.get()->size, key.get()->data ) ;
         dmsWTItem curItem ;
         _cursor->get_key( _cursor, curItem.get() ) ;
         utilSlice curSlice( curItem.get()->size, curItem.get()->data ) ;
         PD_LOG( PDDEBUG, "search [%s], current [%s] exect %d",
                 keySlice.toPoolString().c_str(),
                 curSlice.toPoolString().c_str(),
                 exact ) ;
      }
#endif

      isFound = ( 0 == exact ) ;
      if ( ( 0 > exact ) ||
           ( isAfter && 0 == exact ) )
      {
         rc = WT_CALL( _cursor->next( _cursor ), _session.getSession() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to next, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHNEXT_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHPREV_INT, "_dmsWTCursor::searchPrev" )
   INT32 _dmsWTCursor::searchPrev( UINT64 key, BOOLEAN isBefore, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHPREV_INT ) ;

      INT32 exact = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search near key from cursor, rc: %d", rc ) ;

      isFound = ( 0 == exact ) ;
      if ( ( 0 < exact ) ||
           ( isBefore && 0 == exact ) )
      {
         rc = WT_CALL( _cursor->prev( _cursor ), _session.getSession() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to prev, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHPREV_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHPREV_STRING, "_dmsWTCursor::searchPrev" )
   INT32 _dmsWTCursor::searchPrev( const CHAR *key, BOOLEAN isBefore, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHPREV_STRING ) ;

      INT32 exact = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key ) ;

      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search near key from cursor, rc: %d", rc ) ;

      isFound = ( 0 == exact ) ;
      if ( ( 0 < exact ) ||
           ( isBefore && 0 == exact ) )
      {
         rc = WT_CALL( _cursor->prev( _cursor ), _session.getSession() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to prev, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHPREV_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHPREV_ITEM, "_dmsWTCursor::searchPrev" )
   INT32 _dmsWTCursor::searchPrev( const dmsWTItem &key, BOOLEAN isBefore, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHPREV_ITEM ) ;

      INT32 exact = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      _cursor->set_key( _cursor, key.get() ) ;

      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search near key from cursor, rc: %d", rc ) ;

#if defined(_DEBUG)
      {
         utilSlice keySlice( key.get()->size, key.get()->data ) ;
         dmsWTItem curItem ;
         _cursor->get_key( _cursor, curItem.get() ) ;
         utilSlice curSlice( curItem.get()->size, curItem.get()->data ) ;
         PD_LOG( PDDEBUG, "search [%s], current [%s] exect %d",
                 keySlice.toPoolString().c_str(),
                 curSlice.toPoolString().c_str(),
                 exact ) ;
      }
#endif

      isFound = ( 0 == exact ) ;
      if ( ( 0 < exact ) ||
           ( isBefore && 0 == exact ) )
      {
         rc = WT_CALL( _cursor->prev( _cursor ), _session.getSession() ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to prev, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHPREV_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_INT, "_dmsWTCursor::searchAndGetValue" )
   INT32 _dmsWTCursor::searchAndGetValue( UINT64 key, dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_INT ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from cursor, cursor is not opened" ) ;

      rc = search( key ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

      rc = WT_CALL( _cursor->get_value( _cursor, value.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_INT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_STRING, "_dmsWTCursor::searchAndGetValue" )
   INT32 _dmsWTCursor::searchAndGetValue( const CHAR *key, dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_STRING ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from WiredTiger cursor, cursor is not opened" ) ;

      rc = search( key ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

      rc = WT_CALL( _cursor->get_value( _cursor, value.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from WiredTiger cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_STRING, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_ITEM, "_dmsWTCursor::searchAndGetValue" )
   INT32 _dmsWTCursor::searchAndGetValue( const dmsWTItem &key, dmsWTItem &value )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_ITEM ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from WiredTiger cursor, cursor is not opened" ) ;

      rc = search( key ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

      rc = WT_CALL( _cursor->get_value( _cursor, value.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from WiredTiger cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHANDGETVALUE_ITEM, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_SEARCHPREFIX, "_dmsWTCursor::searchPrefix" )
   INT32 _dmsWTCursor::searchPrefix( const dmsWTItem &key,
                                     dmsWTItem &existsKey,
                                     dmsWTItem &existsValue,
                                     BOOLEAN &isFound,
                                     BOOLEAN &isExactMatch )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_SEARCHPREFIX ) ;

      BOOLEAN setSearchMode = FALSE ;
      INT32 exact = 0 ;

      isFound = FALSE ;
      isExactMatch = FALSE ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get key from WiredTiger cursor, cursor is not opened" ) ;

      // use prefix search
      rc = WT_CALL( _cursor->reconfigure( _cursor, "prefix_search=true" ),
                    _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to set prefx search mode, rc: %d", rc ) ;
      setSearchMode = TRUE ;

      _cursor->set_key( _cursor, key.get() ) ;
      rc = WT_CALL( _cursor->search_near( _cursor, &exact ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to search key from cursor, rc: %d", rc ) ;

      rc = WT_CALL( _cursor->get_key( _cursor, existsKey.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;

      if ( 0 == exact )
      {
         isExactMatch = TRUE ;
      }
      else if ( 0 == ossMemcmp( key.getData(), existsKey.getData(), key.getSize() ) )
      {
         exact = 0 ;
      }
      if ( 0 != exact )
      {
         existsKey.reset() ;
         if ( exact < 0 )
         {
            rc = WT_CALL( _cursor->next( _cursor ), _session.getSession() ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to move cursor, rc: %d", rc ) ;
         }
         else if ( exact > 0 )
         {
            rc = WT_CALL( _cursor->prev( _cursor ), _session.getSession() ) ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               goto done ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to move cursor, rc: %d", rc ) ;
         }
         rc = WT_CALL( _cursor->get_key( _cursor, existsKey.get() ), _session.getSession() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get key from cursor, rc: %d", rc ) ;
         if ( 0 != ossMemcmp( key.getData(), existsKey.getData(), key.getSize() ) )
         {
            rc = SDB_OK ;
            goto done ;
         }
      }

      rc = WT_CALL( _cursor->get_value( _cursor, existsValue.get() ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get value from cursor, rc: %d", rc ) ;

      isFound = TRUE ;

   done:
      if ( nullptr != _cursor && setSearchMode )
      {
         // disable prefix search
         INT32 tmpRC = WT_CALL( _cursor->reconfigure( _cursor, "prefix_search=false" ),
                                _session.getSession() ) ;
         if ( SDB_OK != tmpRC )
         {
            PD_LOG( PDERROR, "Failed to set prefx search mode, rc: %d", tmpRC ) ;
            if ( SDB_OK == rc )
            {
               rc = tmpRC ;
            }
         }
      }
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_SEARCHPREFIX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_MOVETOHEAD, "_dmsWTCursor::moveToHead" )
   INT32 _dmsWTCursor::moveToHead()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_MOVETOHEAD ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to move cursor to tail, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->reset( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reset cursor, rc: %d", rc ) ;

      rc = next() ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move cursor, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_MOVETOHEAD, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_MOVETOTAIL, "_dmsWTCursor::moveToTail" )
   INT32 _dmsWTCursor::moveToTail()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_MOVETOTAIL ) ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to move cursor to tail, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->largest_key( _cursor ), _session.getSession() ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to move cursor to end, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_MOVETOTAIL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_GETCOUNT, "_dmsWTCursor::getCount" )
   INT32 _dmsWTCursor::getCount( UINT64 &count )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_GETCOUNT ) ;

      count = 0 ;

      PD_CHECK( nullptr != _cursor, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get count from WiredTiger cursor, cursor is not opened" ) ;

      rc = WT_CALL( _cursor->reset( _cursor ), _session.getSession() ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reset cursor, rc: %d", rc ) ;

      while ( TRUE )
      {
         rc = next() ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
            break ;
         }
         ++ count ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_GETCOUNT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTCURSOR_PAUSE, "_dmsWTCursor::pause" )
   INT32 _dmsWTCursor::pause()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTCURSOR_PAUSE ) ;

      if ( nullptr != _cursor )
      {
         rc = WT_CALL( _cursor->reset( _cursor ), _session.getSession() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to reset cursor, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTCURSOR_PAUSE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
