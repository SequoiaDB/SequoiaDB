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

   Source File Name = dmsIndexWriteGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsIndexWriteGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageDataCommon.hpp"

using namespace std ;

namespace engine
{

   /*
      _dmsIndexWriteGaurd imeplement
    */
   _dmsIndexWriteGuard::_dmsIndexWriteGuard()
   : _eduCB( nullptr ),
     _isEnabled( FALSE )
   {
   }

   _dmsIndexWriteGuard::_dmsIndexWriteGuard( pmdEDUCB *cb, BOOLEAN isEnabled )
   : _eduCB( cb ),
     _isEnabled( isEnabled )
   {
   }

   _dmsIndexWriteGuard::~_dmsIndexWriteGuard()
   {
      abort() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_LOCK, "_dmsIndexWriteGuard::lock" )
   INT32 _dmsIndexWriteGuard::lock( const dmsIdxMetadataKey &metadataKey,
                                    UINT32 indexID,
                                    const ixmIndexCB &indexCB,
                                    const dmsRecordID &rid,
                                    dmsIndexBuildGuardPtr &guardPtr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_LOCK ) ;

      BOOLEAN isLockedByThisRound = FALSE ;
      BOOLEAN hasRisk = FALSE ;
      BOOLEAN needProcess = FALSE ;

      _dmsRIDIdxBuildGuardMapIter iter ;

      if ( _rid.isValid() )
      {
         PD_CHECK( _rid == rid, SDB_SYS, error, PDERROR,
                   "Failed to check index bulder, already registered "
                   "for record ID [%u, %u]", _rid._extent, _rid._offset ) ;
      }
      else
      {
         _rid = rid ;
      }

      iter = _guards.find( metadataKey ) ;
      // already locked
      if ( iter != _guards.end() )
      {
         SDB_ASSERT( guardPtr == iter->second, "should be same guard" ) ;
         needProcess = TRUE ;
         goto done ;
      }
      else
      {
         // wait lock
         while ( TRUE )
         {
            if ( _eduCB->isInterrupted() )
            {
               PD_RC_CHECK( rc, PDERROR, "Failed to lock index write guard, "
                           "interrupted" ) ;
               rc = SDB_APP_INTERRUPT ;
               goto error ;
            }
            rc = guardPtr->getProcessMutex().lock_r( OSS_ONE_SEC ) ;
            if ( SDB_OK == rc )
            {
               isLockedByThisRound = TRUE ;
               break ;
            }
            else if ( SDB_TIMEOUT == rc )
            {
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to lock index write guard, rc: %d", rc ) ;
         }
      }

      needProcess = TRUE ;

      rc = guardPtr->writeCheck( rid, needProcess, hasRisk ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check write, rc: %d", rc ) ;

      if ( hasRisk )
      {
         try
         {
            _guards.insert( make_pair( metadataKey, guardPtr ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to save index build guard, "
                    "occur exception: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
         isLockedByThisRound = FALSE ;
      }

   done:
      if ( isLockedByThisRound )
      {
         guardPtr->getProcessMutex().release_r() ;
      }
      if ( SDB_OK == rc )
      {
         if ( needProcess )
         {
            _processMap.setBit( indexID ) ;
         }
      }
      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_LOCK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_BEGIN, "_dmsIndexWriteGuard::begin" )
   INT32 _dmsIndexWriteGuard::begin()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_BEGIN ) ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_BEGIN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_COMMIT, "_dmsIndexWriteGuard::commit" )
   INT32 _dmsIndexWriteGuard::commit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_COMMIT ) ;

      for ( _dmsRIDIdxBuildGuardMapIter iter = _guards.begin() ;
            iter != _guards.end() ;
            ++ iter )
      {
         dmsIndexBuildGuardPtr &guardPtr = iter->second ;
         guardPtr->writeCommit( _rid, TRUE ) ;
         guardPtr->getProcessMutex().release_r() ;
      }
      _guards.clear() ;
      _processMap.resetBitmap() ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_COMMIT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_ABORT, "_dmsIndexWriteGuard::abort" )
   INT32 _dmsIndexWriteGuard::abort( BOOLEAN isForced )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_ABORT ) ;

      for ( _dmsRIDIdxBuildGuardMapIter iter = _guards.begin() ;
            iter != _guards.end() ;
            ++ iter )
      {
         dmsIndexBuildGuardPtr &guardPtr = iter->second ;
         guardPtr->writeAbort( _rid, TRUE ) ;
         guardPtr->getProcessMutex().release_r() ;
      }
      _guards.clear() ;
      _processMap.resetBitmap() ;

      PD_TRACE_EXITRC( SDB__DMSINDEXWRITEGUARD_ABORT, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXWRITEGUARD_ISSET, "_dmsIndexWriteGuard::isSet" )
   BOOLEAN _dmsIndexWriteGuard::isSet( const dmsIdxMetadataKey &metadataKey,
                                       const dmsRecordID &rid )
   {
      BOOLEAN isSet = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSINDEXWRITEGUARD_ISSET ) ;

      if ( _isEnabled )
      {
         auto iter = _guards.find( metadataKey ) ;
         if ( iter != _guards.end() )
         {
            isSet = iter->second->isSetByWrite( rid ) ;
         }
      }

      PD_TRACE_EXIT( SDB__DMSINDEXWRITEGUARD_ISSET ) ;

      return isSet ;
   }

}
