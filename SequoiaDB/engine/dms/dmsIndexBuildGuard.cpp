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

   Source File Name = dmsIndexBuildGuard.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsIndexBuildGuard.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

using namespace std ;

namespace engine
{

   #define DMS_INDEX_BUILD_STEP 10000

   /*
      _dmsIndexBuildGuard implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_INIT, "_dmsIndexBuildGuard::init" )
   INT32 _dmsIndexBuildGuard::init( const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_INIT ) ;

      SDB_ASSERT( 0 == _beyondMaxCount, "should not have beyond record" ) ;
      _minRID = rid ;
      if ( !_minRID.isValid() )
      {
         _minRID.resetMin() ;
      }
      if ( _minRID.toUINT64() + DMS_INDEX_BUILD_STEP >
                                       dmsRecordID::maxRID().toUINT64() )
      {
         _maxRID.resetMax() ;
      }
      else
      {
         _maxRID.fromUINT64( _minRID.toUINT64() + DMS_INDEX_BUILD_STEP ) ;
      }

      PD_TRACE_EXITRC( SDB__DMSINDEXBUILDGUARD_INIT, rc ) ;

      return rc ;
   }
   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_WRITECHECK, "_dmsIndexBuildGuard::writeCheck" )
   INT32 _dmsIndexBuildGuard::writeCheck( const dmsRecordID &rid,
                                          BOOLEAN &needProcess,
                                          BOOLEAN &hasRisk )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_WRITECHECK ) ;

      BOOLEAN needWaitEvent = FALSE ;

      needProcess = FALSE ;
      hasRisk = FALSE ;

      // quick path: index build is ended
      if ( _isEnded )
      {
         needProcess = TRUE ;
         goto done ;
      }

      while ( TRUE )
      {
         if ( needWaitEvent )
         {
            _buildingEvent.wait( OSS_ONE_SEC ) ;
            needWaitEvent = FALSE ;
         }
         try
         {
            ossScopedLock lock( &_ridLatch ) ;
            if ( _isEnded )
            {
               needProcess = TRUE ;
               goto done ;
            }
            if ( rid < _minRID )
            {
               // less than current processing record IDs
               // need to process
               needProcess = TRUE ;
            }
            else if ( _maxRID < rid )
            {
               // greater than current processing record IDs
               // no need to process
               needProcess = FALSE ;
               hasRisk = TRUE ;
               ++ _beyondMaxCount ;
            }
            else
            {
               // in current processing record IDs
               // check if builder already processed
               if ( _recordMap.insert( make_pair( rid, FALSE ) ).second )
               {
                  // not processed by builder, need to process by data writer
                  needProcess = TRUE ;
                  hasRisk = TRUE ;
               }
               else if ( _buildingMinRID <= rid && rid <= _buildingMaxRID )
               {
                  // just processing by builder, need wait
                  _buildingEvent.reset() ;
                  needWaitEvent = TRUE ;
                  continue ;
               }
               else
               {
                  // processed by builder, need to process by data writer either
                  needProcess = TRUE ;
               }
            }
#if defined(_DEBUG)
            PD_LOG( PDDEBUG, "Check write index build guard: "
                    "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
                    "building minRID: [%u, %u], building maxRID: [%u, %u], "
                    "need process [%s], has risk [%s]",
                    rid._extent, rid._offset,
                    _minRID._extent, _minRID._offset,
                    _maxRID._extent, _maxRID._offset,
                    _buildingMinRID._extent, _buildingMinRID._offset,
                    _buildingMaxRID._extent, _buildingMaxRID._offset,
                    needProcess ? "TRUE" : "FALSE",
                    hasRisk ? "TRUE" : "FALSE" ) ;
#endif
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to process write check, occur exception: %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
         break ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DMSINDEXBUILDGUARD_WRITECHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_WRITECOMMIT, "_dmsIndexBuildGuard::writeCommit" )
   void _dmsIndexBuildGuard::writeCommit( const dmsRecordID &rid,
                                          BOOLEAN hasRisk )
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_WRITECOMMIT ) ;

      if ( hasRisk )
      {
         ossScopedLock lock( &_ridLatch ) ;
         if ( _maxRID < rid )
         {
            -- _beyondMaxCount ;
         }
      }

#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "Check write commit index build guard: "
               "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
               "building minRID: [%u, %u], building maxRID: [%u, %u], "
               "has risk [%s]",
               rid._extent, rid._offset,
               _minRID._extent, _minRID._offset,
               _maxRID._extent, _maxRID._offset,
               _buildingMinRID._extent, _buildingMinRID._offset,
               _buildingMaxRID._extent, _buildingMaxRID._offset,
               hasRisk ? "TRUE" : "FALSE" ) ;
#endif

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_WRITECOMMIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_WRITEABORT, "_dmsIndexBuildGuard::writeAbort" )
   void _dmsIndexBuildGuard::writeAbort( const dmsRecordID &rid,
                                         BOOLEAN hasRisk )
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_WRITEABORT ) ;

      if ( hasRisk )
      {
         ossScopedLock lock( &_ridLatch ) ;
         if ( _maxRID < rid )
         {
            -- _beyondMaxCount ;
         }
         else
         {
            // aborted, remove the record processed by builder
            auto iter = _recordMap.find( rid ) ;
            if ( iter != _recordMap.end() &&
                 iter->second == FALSE )
            {
               _recordMap.erase( rid ) ;
            }
         }
      }

#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "Check write abort index build guard: "
               "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
               "building minRID: [%u, %u], building maxRID: [%u, %u], "
               "has risk [%s]",
               rid._extent, rid._offset,
               _minRID._extent, _minRID._offset,
               _maxRID._extent, _maxRID._offset,
               _buildingMinRID._extent, _buildingMinRID._offset,
               _buildingMaxRID._extent, _buildingMaxRID._offset,
               hasRisk ? "TRUE" : "FALSE" ) ;
#endif

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_WRITEABORT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDSETRANGE, "_dmsIndexBuildGuard::buildSetRange" )
   void _dmsIndexBuildGuard::buildSetRange( const dmsRecordID &minRID,
                                            const dmsRecordID &maxRID )
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDSETRANGE ) ;

      ossScopedLock lock( &_ridLatch ) ;
      _buildingMinRID = minRID ;
      _buildingMaxRID = maxRID ;

#if defined(_DEBUG)
            PD_LOG( PDDEBUG, "build set range index build guard: "
                    "minRID: [%u, %u], maxRID: [%u, %u], "
                    "building minRID: [%u, %u], building maxRID: [%u, %u]",
                    _minRID._extent, _minRID._offset,
                    _maxRID._extent, _maxRID._offset,
                    _buildingMinRID._extent, _buildingMinRID._offset,
                    _buildingMaxRID._extent, _buildingMaxRID._offset ) ;
#endif

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_BUILDSETRANGE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDCHECK, "_dmsIndexBuildGuard::buildCheck" )
   INT32 _dmsIndexBuildGuard::buildCheck( const dmsRecordID &rid,
                                          BOOLEAN isBuildByRange,
                                          BOOLEAN &needProcess,
                                          BOOLEAN &needWait,
                                          BOOLEAN &needMove )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDCHECK ) ;

      needProcess = FALSE ;
      needWait = FALSE ;

      try
      {
         ossScopedLock lock( &_ridLatch ) ;
         if ( rid < _minRID )
         {
            SDB_ASSERT( FALSE, "Should not be here" ) ;
         }
         else if ( _maxRID < rid )
         {
            // out of current processing range, need move to next
            // processing range
            SDB_ASSERT( !_isEnded, "should not be ended" ) ;
            needWait = TRUE ;
            needMove = TRUE ;
         }
         else
         {
            // save to processed record map
            auto res = _recordMap.insert( make_pair( rid, TRUE ) ) ;
            if ( res.second )
            {
               needProcess = TRUE ;
               if ( isBuildByRange )
               {
                  SDB_ASSERT( _buildingMinRID <= rid && rid <= _buildingMaxRID,
                              "Should be in building range" ) ;
               }
               else
               {
                  _buildingMinRID = rid ;
                  _buildingMaxRID = rid ;
               }
            }
            else if ( res.first->second )
            {
               // already processed by builder, should be an array key field
               needProcess = TRUE ;
               if ( isBuildByRange )
               {
                  SDB_ASSERT( _buildingMinRID <= rid && rid <= _buildingMaxRID,
                              "Should be in building range" ) ;
               }
               else
               {
                  _buildingMinRID = rid ;
                  _buildingMaxRID = rid ;
               }
            }
            else if ( !_isEnded )
            {
               needWait = TRUE ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to process write check, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

#if defined(_DEBUG)
            PD_LOG( PDDEBUG, "Check build index build guard: "
                    "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
                    "building minRID: [%u, %u], building maxRID: [%u, %u], "
                    "need process [%s], need wait [%s], need move [%s]",
                    rid._extent, rid._offset,
                    _minRID._extent, _minRID._offset,
                    _maxRID._extent, _maxRID._offset,
                    _buildingMinRID._extent, _buildingMinRID._offset,
                    _buildingMaxRID._extent, _buildingMaxRID._offset,
                    needProcess ? "TRUE" : "FALSE",
                    needWait ? "TRUE" : "FALSE",
                    needMove ? "TRUE" : "FALSE" ) ;
#endif

   done:
      PD_TRACE_EXITRC( SDB__DMSINDEXBUILDGUARD_BUILDCHECK, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDCHECKAFTERWAIT, "_dmsIndexBuildGuard::buildCheckAfterWait" )
   INT32 _dmsIndexBuildGuard::buildCheckAfterWait( const dmsRecordID &rid,
                                                   BOOLEAN &needProcess )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDCHECKAFTERWAIT ) ;

      needProcess = FALSE ;

      try
      {
         ossScopedLock lock( &_ridLatch ) ;
         if ( rid < _minRID || _maxRID < rid )
         {
            SDB_ASSERT( FALSE, "Should not be here" ) ;
         }
         else
         {
            auto res = _recordMap.insert( make_pair( rid, TRUE ) ) ;
            if ( res.second )
            {
               // have the lock exclusively, must need to process
               // if not in the processed map
               needProcess = TRUE ;
            }
            else if ( res.first->second )
            {
               // already processed by builder, should be an array key field
               // need to process by builder
               needProcess = TRUE ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to process build check, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

#if defined(_DEBUG)
            PD_LOG( PDDEBUG, "Check build after wait index build guard: "
                    "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
                    "building minRID: [%u, %u], building maxRID: [%u, %u], "
                    "need process [%s]",
                    rid._extent, rid._offset,
                    _minRID._extent, _minRID._offset,
                    _maxRID._extent, _maxRID._offset,
                    _buildingMinRID._extent, _buildingMinRID._offset,
                    _buildingMaxRID._extent, _buildingMaxRID._offset,
                    needProcess ? "TRUE" : "FALSE" ) ;
#endif

   done:
      PD_TRACE_EXITRC( SDB__DMSINDEXBUILDGUARD_BUILDCHECKAFTERWAIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDDONE, "_dmsIndexBuildGuard::buildDone" )
   void _dmsIndexBuildGuard::buildDone( BOOLEAN isBuildByRange,
                                        const dmsRecordID &rid )
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDDONE ) ;

      if ( !isBuildByRange )
      {
         buildExit() ;
      }

#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "build done index build guard: "
               "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
               "building minRID: [%u, %u], building maxRID: [%u, %u]",
               rid._extent, rid._offset,
               _minRID._extent, _minRID._offset,
               _maxRID._extent, _maxRID._offset,
               _buildingMinRID._extent, _buildingMinRID._offset,
               _buildingMaxRID._extent, _buildingMaxRID._offset ) ;
#endif

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_BUILDDONE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDEND, "_dmsIndexBuildGuard::buildEnd" )
   void _dmsIndexBuildGuard::buildEnd()
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDEND ) ;

      ossScopedLock lock( &_ridLatch ) ;
      _isEnded = TRUE ;

#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "build end index build guard" ) ;
#endif

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_BUILDEND ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDEXIT, "_dmsIndexBuildGuard::buildExit" )
   void _dmsIndexBuildGuard::buildExit()
   {
      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDEXIT ) ;

      ossScopedLock lock( &_ridLatch ) ;
      _buildingMinRID.reset() ;
      _buildingMaxRID.reset() ;
      _buildingEvent.signalAll() ;

#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "build exit index build guard" ) ;
#endif

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_BUILDEXIT ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDMOVE, "_dmsIndexBuildGuard::buildMove" )
   INT32 _dmsIndexBuildGuard::buildMove( const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDMOVE ) ;

      ossScopedLock lock( &_ridLatch ) ;
      rc = init( rid ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to reinit index build guard, rc: %d", rc ) ;

#if defined(_DEBUG)
      PD_LOG( PDDEBUG, "build move index build guard: "
               "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
               "building minRID: [%u, %u], building maxRID: [%u, %u]",
               rid._extent, rid._offset,
               _minRID._extent, _minRID._offset,
               _maxRID._extent, _maxRID._offset,
               _buildingMinRID._extent, _buildingMinRID._offset,
               _buildingMaxRID._extent, _buildingMaxRID._offset ) ;
#endif

   done:
      PD_TRACE_EXITRC( SDB__DMSINDEXBUILDGUARD_BUILDMOVE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_BUILDCHECKMOVE, "_dmsIndexBuildGuard::buildCheckMove" )
   INT32 _dmsIndexBuildGuard::buildCheckMove( const dmsRecordID &rid,
                                              BOOLEAN &needMove )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_BUILDCHECKMOVE ) ;

      ossScopedLock lock( &_ridLatch ) ;
      if ( _maxRID < rid )
      {
         needMove = TRUE ;
      }

#if defined(_DEBUG)
            PD_LOG( PDDEBUG, "Check build move index build guard: "
                    "rid: [%u, %u], minRID: [%u, %u], maxRID: [%u, %u], "
                    "building minRID: [%u, %u], building maxRID: [%u, %u], "
                    "need move [%s]",
                    rid._extent, rid._offset,
                    _minRID._extent, _minRID._offset,
                    _maxRID._extent, _maxRID._offset,
                    _buildingMinRID._extent, _buildingMinRID._offset,
                    _buildingMaxRID._extent, _buildingMaxRID._offset,
                    needMove ? "TRUE" : "FALSE" ) ;
#endif

      PD_TRACE_EXITRC( SDB__DMSINDEXBUILDGUARD_BUILDCHECKMOVE, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DMSINDEXBUILDGUARD_ISSETBYWRITE, "_dmsIndexBuildGuard::isSetByWrite" )
   BOOLEAN _dmsIndexBuildGuard::isSetByWrite( const dmsRecordID &rid )
   {
      BOOLEAN isSet = FALSE ;

      PD_TRACE_ENTRY( SDB__DMSINDEXBUILDGUARD_ISSETBYWRITE ) ;

      ossScopedLock lock( &_ridLatch ) ;
      auto iter = _recordMap.find( rid ) ;
      if ( iter != _recordMap.end() )
      {
         isSet = !iter->second ;
      }

      PD_TRACE_EXIT( SDB__DMSINDEXBUILDGUARD_ISSETBYWRITE ) ;

      return isSet ;
   }

}
