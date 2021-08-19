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

   Source File Name = rtnCommandSnapshot.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCommandSnapshot.hpp"
#include "rtn.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSystem)

   BOOLEAN _rtnSnapshotSystem::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSystemInner)

   BOOLEAN _rtnSnapshotSystemInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotHealth)

   BOOLEAN _rtnSnapshotHealth::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotHealthInner )

   BOOLEAN _rtnSnapshotHealthInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContexts)

   BOOLEAN _rtnSnapshotContexts::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContextsInner)

   BOOLEAN _rtnSnapshotContextsInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContextsCurrent)

   BOOLEAN _rtnSnapshotContextsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContextsCurrentInner)

   BOOLEAN _rtnSnapshotContextsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotDatabase)

   BOOLEAN _rtnSnapshotDatabase::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotDatabaseInner )

   BOOLEAN _rtnSnapshotDatabaseInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotReset)

   _rtnSnapshotReset::_rtnSnapshotReset ()
   :_type( CMD_SNAPSHOT_ALL ),
    _sessionID( PMD_INVALID_EDUID ),
    _resetAllSession( FALSE ),
    _collectionSpace( NULL ),
    _collection( NULL )
   {
   }

   _rtnSnapshotReset::~_rtnSnapshotReset ()
   {
   }

   const CHAR *_rtnSnapshotReset::name ()
   {
      return NAME_SNAPSHOT_RESET ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotReset::type ()
   {
      return CMD_SNAPSHOT_RESET ;
   }

   INT32 _rtnSnapshotReset::init( INT32 flags, INT64 numToSkip,
                                  INT64 numToReturn,
                                  const CHAR *pMatcherBuff,
                                  const CHAR *pSelectBuff,
                                  const CHAR *pOrderByBuff,
                                  const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj obj( pMatcherBuff ) ;
         BOOLEAN hasSessionID = TRUE ;
         string type ;

         // check Type
         rc = rtnGetSTDStringElement( obj, FIELD_NAME_TYPE, type ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_TYPE, rc ) ;

         if ( VALUE_NAME_DATABASE == type )
         {
            _type = CMD_SNAPSHOT_DATABASE ;
         }
         else if ( VALUE_NAME_SESSIONS == type )
         {
            _type = CMD_SNAPSHOT_SESSIONS ;
         }
         else if ( VALUE_NAME_SESSIONS_CURRENT == type )
         {
            _type = CMD_SNAPSHOT_SESSIONS_CURRENT ;
         }
         else if ( VALUE_NAME_HEALTH == type )
         {
            _type = CMD_SNAPSHOT_HEALTH ;
         }
         else if ( VALUE_NAME_SVCTASKS == type )
         {
            _type = CMD_SNAPSHOT_SVCTASKS ;
         }
         else if ( VALIE_NAME_COLLECTIONS == type )
         {
            _type = CMD_SNAPSHOT_COLLECTIONS ;
         }
         else if ( VALUE_NAME_ALL == type )
         {
            _type = CMD_SNAPSHOT_ALL ; ;
         }

         // check SessionID
         rc = rtnGetNumberLongElement( obj, FIELD_NAME_SESSIONID,
                                       (INT64 &)_sessionID ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            hasSessionID = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_SESSIONID, rc ) ;

         // check collection space
         rc = rtnGetStringElement( obj, FIELD_NAME_COLLECTIONSPACE,
                                   &_collectionSpace ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_COLLECTIONSPACE, rc ) ;

         // check collection
         rc = rtnGetStringElement( obj, FIELD_NAME_COLLECTION,
                                   &_collection ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_COLLECTION, rc ) ;

         // check conflict
         if ( hasSessionID && _type != CMD_SNAPSHOT_SESSIONS )
         {
            PD_LOG( PDERROR, "Field[%s] take effect only when reset snapshot "
                    "session", FIELD_NAME_SESSIONID ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( NULL != _collectionSpace && CMD_SNAPSHOT_COLLECTIONS != _type )
         {
            PD_LOG( PDERROR, "Field[%s] take effect only when reset snapshot "
                    "collections", FIELD_NAME_COLLECTIONSPACE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( NULL != _collection && CMD_SNAPSHOT_COLLECTIONS != _type )
         {
            PD_LOG( PDERROR, "Field[%s] take effect only when reset snapshot "
                    "collections", FIELD_NAME_COLLECTION ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( NULL != _collection && NULL != _collectionSpace )
         {
            PD_LOG( PDERROR, "Field [%s] and Field [%s] conflict",
                    FIELD_NAME_COLLECTIONSPACE, FIELD_NAME_COLLECTION ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         // reset all session
         if ( _type == CMD_SNAPSHOT_SESSIONS && !hasSessionID )
         {
            _resetAllSession = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _rtnSnapshotReset::doit( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                  _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                                  INT16 w, INT64 *pContextID )
   {
      return monResetMon( _type, _resetAllSession, _sessionID,
                          _collectionSpace, _collection ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSessions)

   BOOLEAN _rtnSnapshotSessions::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotSessionsInner )

   BOOLEAN _rtnSnapshotSessionsInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSessionsCurrent)

   BOOLEAN _rtnSnapshotSessionsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotSessionsCurrentInner )

   BOOLEAN _rtnSnapshotSessionsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotTransactionsCurrent)

   BOOLEAN _rtnSnapshotTransactionsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotTransactionsCurrentInner )

   BOOLEAN _rtnSnapshotTransactionsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotTransactions)

   BOOLEAN _rtnSnapshotTransactions::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotTransactionsInner )

   BOOLEAN _rtnSnapshotTransactionsInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCollections)

   BOOLEAN _rtnSnapshotCollections::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotCollectionsInner )

   BOOLEAN _rtnSnapshotCollectionsInner::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotCollectionSpaces )

   BOOLEAN _rtnSnapshotCollectionSpaces::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotCollectionSpacesInner )

   BOOLEAN _rtnSnapshotCollectionSpacesInner::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotAccessPlans )

   BOOLEAN _rtnSnapshotAccessPlans::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotAccessPlansInner )

   BOOLEAN _rtnSnapshotAccessPlansInner::_isCurrent() const
   {
      /// for include system
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotConfigs)

   BOOLEAN _rtnSnapshotConfigs::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotConfigs::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotConfigsInner)

   BOOLEAN _rtnSnapshotConfigsInner::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotConfigsInner::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotVCLSessionInfoInner )

   BOOLEAN _rtnSnapshotVCLSessionInfoInner::_isCurrent() const
   {
      return TRUE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSvcTasks)

   BOOLEAN _rtnSnapshotSvcTasks::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotSvcTasksInner )

   BOOLEAN _rtnSnapshotSvcTasksInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotQueries )

   BOOLEAN _rtnSnapshotQueries::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotQueries::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotQueriesInner )

   BOOLEAN _rtnSnapshotQueriesInner::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotQueriesInner::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotLatchWaits )

   BOOLEAN _rtnSnapshotLatchWaits::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotLatchWaits::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotLatchWaitsInner )

   BSONObj _rtnSnapshotLatchWaitsInner::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   BOOLEAN _rtnSnapshotLatchWaitsInner::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotLockWaits )

   BSONObj _rtnSnapshotLockWaits::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }
   BOOLEAN _rtnSnapshotLockWaits::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotLockWaitsInner )

   BOOLEAN _rtnSnapshotLockWaitsInner::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotLockWaitsInner::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotIndexStats )

   BSONObj _rtnSnapshotIndexStats::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }
   BOOLEAN _rtnSnapshotIndexStats::_isCurrent() const
   {
      return FALSE ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotIndexStatsInner )

   BOOLEAN _rtnSnapshotIndexStatsInner::_isCurrent() const
   {
      return FALSE ;
   }

   BSONObj _rtnSnapshotIndexStatsInner::_getOptObj() const
   {
      return _getObjectFromHint( "$"FIELD_NAME_OPTIONS ) ;
   }
}
