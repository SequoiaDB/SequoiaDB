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

   Source File Name = dmsPersistGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsPersistGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

   /*
      _dmsPersistGuard implement
    */
   _dmsPersistGuard::_dmsPersistGuard()
   : _service( nullptr ),
     _persistUnit( nullptr ),
     _su( nullptr ),
     _mbStat( nullptr ),
     _eduCB( nullptr ),
     _isEnabled( FALSE ),
     _hasBegin( FALSE )
   {
   }

   _dmsPersistGuard::_dmsPersistGuard( IStorageService *service,
                                       dmsStorageDataCommon *su,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB *cb,
                                       BOOLEAN isEnabled )
   : _service( service ),
     _persistUnit( nullptr ),
     _su( su ),
     _mbStat( mbContext ? mbContext->mbStat() : nullptr ),
     _clUniqueID( mbContext ? mbContext->getCLUniqueID() : UTIL_UNIQUEID_NULL ),
     _eduCB( cb ),
     _isEnabled( isEnabled ),
     _hasBegin( FALSE )
   {
   }

   _dmsPersistGuard::~_dmsPersistGuard()
   {
      if ( _hasBegin )
      {
         abort() ;
      }
      else
      {
         fini() ;
      }
      if ( _dummySession.eduCB() )
      {
         _dummySession.detachCB() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_INIT, "_dmsPersistGuard::init" )
   INT32 _dmsPersistGuard::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_INIT ) ;

      PD_CHECK( !_persistUnit, SDB_SYS, error, PDERROR,
                "Failed to begin persist guard, already in guard" ) ;

      if ( !_isEnabled || !_service )
      {
         goto done ;
      }

      if ( !_eduCB->getSession() )
      {
         _dummySession.attachCB( _eduCB ) ;
      }
      rc = _service->getPersistUnit( _eduCB, _persistUnit ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_FINI, "_dmsPersistGuard::fini" )
   INT32 _dmsPersistGuard::fini()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_FINI ) ;

      _persistUnit = nullptr ;

      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_FINI, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_BEGIN, "_dmsPersistGuard::begin" )
   INT32 _dmsPersistGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_BEGIN ) ;

      if ( !_isEnabled || !_service )
      {
         goto done ;
      }

      if ( !_persistUnit )
      {
         if ( !_eduCB->getSession() )
         {
            _dummySession.attachCB( _eduCB ) ;
         }
         rc = _service->getPersistUnit( _eduCB, _persistUnit ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get persist unit, rc: %d", rc ) ;
      }

      if ( _persistUnit )
      {
         rc = _persistUnit->beginUnit( _eduCB, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to begin persist unit, rc: %d", rc ) ;

         if ( NULL != _su && NULL != _mbStat )
         {
            utilThreadLocalPtr<IStatPersistUnit> statUnitPtr ;
            _statUnitPtr = dmsStatPersistUnit::makeThreadLocalPtr( _clUniqueID, _su, _mbStat ) ;
            PD_CHECK( _statUnitPtr, SDB_OOM, error, PDERROR,
                      "Failed to make statistics persist unit" ) ;
            statUnitPtr = _statUnitPtr ;
            PD_CHECK( _statUnitPtr, SDB_SYS, error, PDERROR,
                      "Failed to convert statistics persist unit" ) ;
            rc = _persistUnit->registerStatUnit( statUnitPtr ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to register stat persist unit, rc: %d", rc ) ;
         }

         _hasBegin = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_BEGIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_COMMIT, "_dmsPersistGuard::commit" )
   INT32 _dmsPersistGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_COMMIT ) ;

      if ( _isEnabled && _persistUnit && _hasBegin )
      {
         rc = _persistUnit->commitUnit( _eduCB, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to commit persist unit, rc: %d", rc ) ;

         _hasBegin = FALSE ;
         _persistUnit = nullptr ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_COMMIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSPERSISTGUARD_ABORT, "_dmsPersistGuard::abort" )
   INT32 _dmsPersistGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSPERSISTGUARD_ABORT ) ;

      if ( _isEnabled && _persistUnit && _hasBegin )
      {
         rc = _persistUnit->abortUnit( _eduCB, FALSE, isForced ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to abort persist unit, rc: %d", rc ) ;

         _hasBegin = FALSE ;
         _persistUnit = nullptr ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSPERSISTGUARD_ABORT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _dmsPersistGuard::incRecordCount( UINT64 count )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incRecordCount( count ) ;
      }
      else if ( UTIL_UNIQUEID_NULL != _clUniqueID && _su && _mbStat )
      {
         _su->increaseMBStat( _clUniqueID, _mbStat, count, _eduCB ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalRecords.add( count ) ;
         _mbStat->_rcTotalRecords.add( count ) ;
      }
   }

   void _dmsPersistGuard::decRecordCount( UINT64 count )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->decRecordCount( count ) ;
      }
      else if ( UTIL_UNIQUEID_NULL != _clUniqueID && _su && _mbStat )
      {
         _su->decreaseMBStat( _clUniqueID, _mbStat, count, _eduCB ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalRecords.sub( count ) ;
         _mbStat->_rcTotalRecords.sub( count ) ;
      }
   }

   void _dmsPersistGuard::incDataLen( UINT64 dataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incDataLen( dataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalDataLen.add( dataLen ) ;
      }
   }

   void _dmsPersistGuard::decDataLen( UINT64 dataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incDataLen( dataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalDataLen.sub( dataLen ) ;
      }
   }

   void _dmsPersistGuard::incOrgDataLen( UINT64 orgDataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incOrgDataLen( orgDataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalOrgDataLen.add( orgDataLen ) ;
      }
   }

   void _dmsPersistGuard::decOrgDataLen( UINT64 orgDataLen )
   {
      if ( _statUnitPtr )
      {
         _statUnitPtr->incOrgDataLen( orgDataLen ) ;
      }
      else if ( _mbStat )
      {
         _mbStat->_totalOrgDataLen.sub( orgDataLen ) ;
      }
   }

}
