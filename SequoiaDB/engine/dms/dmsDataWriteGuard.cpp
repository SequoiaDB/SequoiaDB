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

   Source File Name = dmsDataWriteGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsDataWriteGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

   /*
      _dmsDataWriteGuard imeplement
    */
   _dmsDataWriteGuard::_dmsDataWriteGuard()
   : _su( nullptr ),
     _mbID( DMS_INVALID_MBID ),
     _eduCB( nullptr ),
     _isEnabled( FALSE ),
     _isInWrite( FALSE )
   {
   }

   _dmsDataWriteGuard::_dmsDataWriteGuard( dmsStorageDataCommon *su,
                                           dmsMBContext *mbContext,
                                           pmdEDUCB *cb,
                                           BOOLEAN isEnabled )
   : _su( su ),
     _mbStat( mbContext->mbStat() ),
     _mbID( mbContext->mbID() ),
     _eduCB( cb ),
     _isEnabled( isEnabled ),
     _isInWrite( FALSE )
   {
   }

   _dmsDataWriteGuard::~_dmsDataWriteGuard()
   {
      abort() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_BEFOREWRITE, "_dmsDataWriteGuard::beforeWrite" )
   void _dmsDataWriteGuard::beforeWrite()
   {
      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_BEFOREWRITE ) ;

      if ( _isEnabled && !_isInWrite )
      {
         _su->markDirty( _mbID, DMS_CHG_BEFORE ) ;
         _su->incWritePtrCount( _mbID ) ;
         _isInWrite = TRUE ;
      }

      PD_TRACE_EXIT( SDB__DMSDATAWRITEGUARD_BEFOREWRITE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_AFTERWRITE, "_dmsDataWriteGuard::afterWrite" )
   void _dmsDataWriteGuard::afterWrite()
   {
      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_AFTERWRITE ) ;

      if ( _isEnabled && _isInWrite )
      {
         _mbStat->_snapshotID.inc() ;
         _su->markDirty( _mbID, DMS_CHG_AFTER ) ;
         _su->decWritePtrCount( _mbID ) ;
         _isInWrite = FALSE ;
      }

      PD_TRACE_EXIT( SDB__DMSDATAWRITEGUARD_AFTERWRITE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_BEGIN, "_dmsDataWriteGuard::begin" )
   INT32 _dmsDataWriteGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_BEGIN ) ;

      beforeWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_BEGIN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_COMMIT, "_dmsDataWriteGuard::commit" )
   INT32 _dmsDataWriteGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_COMMIT ) ;

      afterWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSDATAWRITEGUARD_ABORT, "_dmsDataWriteGuard::abort" )
   INT32 _dmsDataWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSDATAWRITEGUARD_ABORT ) ;

      afterWrite() ;

      PD_TRACE_EXITRC( SDB__DMSDATAWRITEGUARD_ABORT, rc ) ;

      return rc ;
   }

}
