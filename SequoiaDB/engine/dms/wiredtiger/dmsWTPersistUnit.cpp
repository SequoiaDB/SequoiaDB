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

   Source File Name = dmsWTPersistUnit.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "wiredtiger/dmsWTPersistUnit.hpp"
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
      _dmsWTPersistUnit implement
    */
   _dmsWTPersistUnit::_dmsWTPersistUnit( dmsWTStorageEngine &engine )
   : _dmsPersistUnit(),
     _engine( engine )
   {
   }

   _dmsWTPersistUnit::~_dmsWTPersistUnit()
   {
      if ( dmsPersistUnitState::INACTIVE != _state )
      {
         INT32 tmpRC = _abortUnit( sdbGetThreadExecutor() ) ;
         PD_LOG( PDWARNING, "Failed to abort persist unit, rc: %d", tmpRC ) ;
      }
      _writeSession.close() ;
      _readSession.close() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT_INITUNIT, "_dmsWTPersistUnit::initUnit" )
   INT32 _dmsWTPersistUnit::initUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT_INITUNIT ) ;

      if ( !_writeSession.isOpened() )
      {
         // only snapshot session can support write operations
         rc = _engine.openSession( _writeSession, dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

      if ( !_readSession.isOpened() )
      {
         rc = _engine.openSession( _readSession, dmsWTSessIsolation::SNAPSHOT ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open session, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT_INITUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__BEGINUNIT, "_dmsWTPersistUnit::_beginUnit" )
   INT32 _dmsWTPersistUnit::_beginUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__BEGINUNIT ) ;

      rc = _writeSession.beginTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to begin transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__BEGINUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__PREPAREUNIT, "_dmsWTPersistUnit::_prepareUnit" )
   INT32 _dmsWTPersistUnit::_prepareUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__PREPAREUNIT ) ;

      PD_CHECK( _writeSession.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to prepare transaction, session is not opened" ) ;

      rc = _writeSession.prepareTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__PREPAREUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__COMMITUNIT, "_dmsWTPersistUnit::_commitUnit" )
   INT32 _dmsWTPersistUnit::_commitUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__COMMITUNIT ) ;

      PD_CHECK( _writeSession.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to commit transaction, session is not opened" ) ;

      rc = _writeSession.commitTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to commit transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__COMMITUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSWTPERSISTUNIT__ABORTUNIT, "_dmsWTPersistUnit::_abortUnit" )
   INT32 _dmsWTPersistUnit::_abortUnit( IExecutor *executor )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSWTPERSISTUNIT__ABORTUNIT ) ;

      PD_CHECK( _writeSession.isOpened(), SDB_SYS, error, PDERROR,
                "Failed to abort transaction, session is not opened" ) ;

      rc = _writeSession.abortTrans() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to abort transaction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSWTPERSISTUNIT__ABORTUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
}
