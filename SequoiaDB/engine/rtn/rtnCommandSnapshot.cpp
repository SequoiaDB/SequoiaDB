/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "pmd.hpp"
#include "monDump.hpp"
#include "aggrDef.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "msgDef.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSystem)
   _rtnSnapshotSystem::_rtnSnapshotSystem ()
   {
   }

   _rtnSnapshotSystem::~_rtnSnapshotSystem ()
   {
   }

   const CHAR *_rtnSnapshotSystem::name ()
   {
      return NAME_SNAPSHOT_SYSTEM ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotSystem::type ()
   {
      return CMD_SNAPSHOT_SYSTEM ;
   }

   INT32 _rtnSnapshotSystem::_getFetchType() const
   {
      return RTN_FETCH_SYSTEM ;
   }

   BOOLEAN _rtnSnapshotSystem::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotSystem::_addInfoMask() const
   {
      return MON_MASK_ALL ;
   }

   const CHAR* _rtnSnapshotSystem::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_SYSTEM_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSystemInner)
   const CHAR *_rtnSnapshotSystemInner::name ()
   {
      return CMD_NAME_SNAPSHOT_SYSTEM_INTR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotSystemInner::type ()
   {
      return CMD_SNAPSHOT_SYSTEM ;
   }

   INT32 _rtnSnapshotSystemInner::_getFetchType() const
   {
      return RTN_FETCH_SYSTEM ;
   }

   BOOLEAN _rtnSnapshotSystemInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotSystemInner::_addInfoMask() const
   {
      return MON_MASK_ALL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotHealth)
   _rtnSnapshotHealth::_rtnSnapshotHealth ()
   {
   }

   _rtnSnapshotHealth::~_rtnSnapshotHealth ()
   {
   }

   const CHAR *_rtnSnapshotHealth::name ()
   {
      return NAME_SNAPSHOT_HEALTH ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotHealth::type ()
   {
      return CMD_SNAPSHOT_HEALTH ;
   }

   INT32 _rtnSnapshotHealth::_getFetchType() const
   {
      return RTN_FETCH_HEALTH ;
   }

   BOOLEAN _rtnSnapshotHealth::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotHealth::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME |
             MON_MASK_IS_PRIMARY |
             MON_MASK_SERVICE_STATUS |
             MON_MASK_LSN_INFO |
             MON_MASK_NODEID ;
   }

   const CHAR* _rtnSnapshotHealth::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_HEALTH_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotHealthInner)
   const CHAR *_rtnSnapshotHealthInner::name ()
   {
      return CMD_NAME_SNAPSHOT_HEALTH_INTR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotHealthInner::type ()
   {
      return CMD_SNAPSHOT_HEALTH ;
   }

   INT32 _rtnSnapshotHealthInner::_getFetchType() const
   {
      return RTN_FETCH_HEALTH ;
   }

   BOOLEAN _rtnSnapshotHealthInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotHealthInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME |
             MON_MASK_IS_PRIMARY |
             MON_MASK_SERVICE_STATUS |
             MON_MASK_LSN_INFO |
             MON_MASK_NODEID ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContexts)
   _rtnSnapshotContexts::_rtnSnapshotContexts ()
   {
   }

   _rtnSnapshotContexts::~_rtnSnapshotContexts ()
   {
   }

   const CHAR *_rtnSnapshotContexts::name ()
   {
      return NAME_SNAPSHOT_CONTEXTS ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotContexts::type ()
   {
      return CMD_SNAPSHOT_CONTEXTS ;
   }

   INT32 _rtnSnapshotContexts::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnSnapshotContexts::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotContexts::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnSnapshotContexts::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_CONTEXT_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContextsInner)
   const CHAR *_rtnSnapshotContextsInner::name ()
   {
      return CMD_NAME_SNAPSHOT_CONTEXT_INTR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotContextsInner::type ()
   {
      return CMD_SNAPSHOT_CONTEXTS ;
   }

   INT32 _rtnSnapshotContextsInner::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnSnapshotContextsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotContextsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContextsCurrent)
   _rtnSnapshotContextsCurrent::_rtnSnapshotContextsCurrent ()
   {
   }

   _rtnSnapshotContextsCurrent::~_rtnSnapshotContextsCurrent ()
   {
   }

   const CHAR *_rtnSnapshotContextsCurrent::name ()
   {
      return NAME_SNAPSHOT_CONTEXTS_CURRENT ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotContextsCurrent::type ()
   {
      return CMD_SNAPSHOT_CONTEXTS_CURRENT ;
   }

   INT32 _rtnSnapshotContextsCurrent::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnSnapshotContextsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnSnapshotContextsCurrent::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnSnapshotContextsCurrent::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotContextsCurrentInner)
   const CHAR *_rtnSnapshotContextsCurrentInner::name ()
   {
      return CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotContextsCurrentInner::type ()
   {
      return CMD_SNAPSHOT_CONTEXTS_CURRENT ;
   }

   INT32 _rtnSnapshotContextsCurrentInner::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnSnapshotContextsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnSnapshotContextsCurrentInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotDatabase)
   _rtnSnapshotDatabase::_rtnSnapshotDatabase ()
   {
   }

   _rtnSnapshotDatabase::~_rtnSnapshotDatabase ()
   {
   }

   const CHAR *_rtnSnapshotDatabase::name ()
   {
      return NAME_SNAPSHOT_DATABASE ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotDatabase::type ()
   {
      return CMD_SNAPSHOT_DATABASE ;
   }

   const CHAR* _rtnSnapshotDatabase::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_DATABASE_INTR ;
   }

   INT32 _rtnSnapshotDatabase::_getFetchType() const
   {
      return RTN_FETCH_DATABASE ;
   }

   BOOLEAN _rtnSnapshotDatabase::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotDatabase::_addInfoMask() const
   {
      return MON_MASK_ALL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotDatabaseInner)
   const CHAR *_rtnSnapshotDatabaseInner::name ()
   {
      return CMD_NAME_SNAPSHOT_DATABASE_INTR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotDatabaseInner::type ()
   {
      return CMD_SNAPSHOT_DATABASE ;
   }

   INT32 _rtnSnapshotDatabaseInner::_getFetchType() const
   {
      return RTN_FETCH_DATABASE ;
   }

   BOOLEAN _rtnSnapshotDatabaseInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotDatabaseInner::_addInfoMask() const
   {
      return MON_MASK_ALL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotReset)
   _rtnSnapshotReset::_rtnSnapshotReset ()
   :_type( CMD_SNAPSHOT_ALL ),
    _sessionID( PMD_INVALID_EDUID ),
    _resetAllSession( FALSE )
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
         else if ( VALUE_NAME_ALL == type )
         {
            _type = CMD_SNAPSHOT_ALL ; ;
         }

         rc = rtnGetNumberLongElement( obj, FIELD_NAME_SESSIONID,
                                       (INT64 &)_sessionID ) ;
         if ( SDB_FIELD_NOT_EXIST == rc )
         {
            hasSessionID = FALSE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get field[%s], rc: %d",
                      FIELD_NAME_SESSIONID, rc ) ;

         if ( hasSessionID && _type != CMD_SNAPSHOT_SESSIONS )
         {
            PD_LOG( PDERROR, "Field[%s] take effect only when reset snapshot "
                    "session", FIELD_NAME_SESSIONID ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

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
      monResetMon( _type, _resetAllSession, _sessionID ) ;
      return SDB_OK ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSessions)
   _rtnSnapshotSessions::_rtnSnapshotSessions ()
   {
   }

   _rtnSnapshotSessions::~_rtnSnapshotSessions ()
   {
   }

   const CHAR *_rtnSnapshotSessions::name ()
   {
      return NAME_SNAPSHOT_SESSIONS ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotSessions::type ()
   {
      return CMD_SNAPSHOT_SESSIONS ;
   }

   INT32 _rtnSnapshotSessions::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnSnapshotSessions::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotSessions::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnSnapshotSessions::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_SESSION_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSessionsInner)
   const CHAR *_rtnSnapshotSessionsInner::name ()
   {
      return CMD_NAME_SNAPSHOT_SESSION_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotSessionsInner::type ()
   {
      return CMD_SNAPSHOT_SESSIONS ;
   }

   INT32 _rtnSnapshotSessionsInner::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnSnapshotSessionsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotSessionsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSessionsCurrent)
   _rtnSnapshotSessionsCurrent::_rtnSnapshotSessionsCurrent ()
   {
   }

   _rtnSnapshotSessionsCurrent::~_rtnSnapshotSessionsCurrent ()
   {
   }

   const CHAR *_rtnSnapshotSessionsCurrent::name ()
   {
      return NAME_SNAPSHOT_SESSIONS_CURRENT ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotSessionsCurrent::type ()
   {
      return CMD_SNAPSHOT_SESSIONS_CURRENT ;
   }

   INT32 _rtnSnapshotSessionsCurrent::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnSnapshotSessionsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnSnapshotSessionsCurrent::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnSnapshotSessionsCurrent::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_SESSIONCUR_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotSessionsCurrentInner)
   const CHAR *_rtnSnapshotSessionsCurrentInner::name ()
   {
      return CMD_NAME_SNAPSHOT_SESSIONCUR_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotSessionsCurrentInner::type ()
   {
      return CMD_SNAPSHOT_SESSIONS_CURRENT ;
   }

   INT32 _rtnSnapshotSessionsCurrentInner::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnSnapshotSessionsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnSnapshotSessionsCurrentInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotTransactionsCurrent)
   _rtnSnapshotTransactionsCurrent::_rtnSnapshotTransactionsCurrent()
   {
   }

   _rtnSnapshotTransactionsCurrent::~_rtnSnapshotTransactionsCurrent()
   {
   }

   const CHAR *_rtnSnapshotTransactionsCurrent::name ()
   {
      return NAME_SNAPSHOT_TRANSACTIONS_CUR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotTransactionsCurrent::type ()
   {
      return CMD_SNAPSHOT_TRANSACTIONS_CUR ;
   }

   INT32 _rtnSnapshotTransactionsCurrent::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnSnapshotTransactionsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnSnapshotTransactionsCurrent::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnSnapshotTransactionsCurrent::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_TRANSCUR_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotTransactionsCurrentInner)
   const CHAR *_rtnSnapshotTransactionsCurrentInner::name ()
   {
      return CMD_NAME_SNAPSHOT_TRANSCUR_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotTransactionsCurrentInner::type ()
   {
      return CMD_SNAPSHOT_TRANSACTIONS_CUR ;
   }

   INT32 _rtnSnapshotTransactionsCurrentInner::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnSnapshotTransactionsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnSnapshotTransactionsCurrentInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotTransactions)
   _rtnSnapshotTransactions::_rtnSnapshotTransactions()
   {
   }

   _rtnSnapshotTransactions::~_rtnSnapshotTransactions()
   {
   }

   const CHAR *_rtnSnapshotTransactions::name ()
   {
      return NAME_SNAPSHOT_TRANSACTIONS ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotTransactions::type ()
   {
      return CMD_SNAPSHOT_TRANSACTIONS ;
   }

   INT32 _rtnSnapshotTransactions::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnSnapshotTransactions::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotTransactions::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnSnapshotTransactions::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_TRANS_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotTransactionsInner)
   const CHAR *_rtnSnapshotTransactionsInner::name ()
   {
      return CMD_NAME_SNAPSHOT_TRANS_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotTransactionsInner::type ()
   {
      return CMD_SNAPSHOT_TRANSACTIONS ;
   }

   INT32 _rtnSnapshotTransactionsInner::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnSnapshotTransactionsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotTransactionsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCollections)
   _rtnSnapshotCollections::_rtnSnapshotCollections ()
   {
   }

   _rtnSnapshotCollections::~_rtnSnapshotCollections ()
   {
   }

   const CHAR *_rtnSnapshotCollections::name ()
   {
      return NAME_SNAPSHOT_COLLECTIONS ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotCollections::type ()
   {
      return CMD_SNAPSHOT_COLLECTIONS ;
   }

   INT32 _rtnSnapshotCollections::_getFetchType() const
   {
      return RTN_FETCH_COLLECTION ;
   }

   BOOLEAN _rtnSnapshotCollections::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnSnapshotCollections::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   const CHAR* _rtnSnapshotCollections::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_COLLECTION_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCollectionsInner)
   const CHAR *_rtnSnapshotCollectionsInner::name ()
   {
      return CMD_NAME_SNAPSHOT_COLLECTION_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotCollectionsInner::type ()
   {
      return CMD_SNAPSHOT_COLLECTIONS ;
   }

   INT32 _rtnSnapshotCollectionsInner::_getFetchType() const
   {
      return RTN_FETCH_COLLECTION ;
   }

   BOOLEAN _rtnSnapshotCollectionsInner::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnSnapshotCollectionsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCollectionSpaces)
   _rtnSnapshotCollectionSpaces::_rtnSnapshotCollectionSpaces ()
   {
   }

   _rtnSnapshotCollectionSpaces::~_rtnSnapshotCollectionSpaces ()
   {
   }

   const CHAR *_rtnSnapshotCollectionSpaces::name ()
   {
      return NAME_SNAPSHOT_COLLECTIONSPACES ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotCollectionSpaces::type ()
   {
      return CMD_SNAPSHOT_COLLECTIONSPACES ;
   }

   INT32 _rtnSnapshotCollectionSpaces::_getFetchType() const
   {
      return RTN_FETCH_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnSnapshotCollectionSpaces::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnSnapshotCollectionSpaces::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   const CHAR* _rtnSnapshotCollectionSpaces::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_SPACE_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotCollectionSpacesInner)
   const CHAR *_rtnSnapshotCollectionSpacesInner::name ()
   {
      return CMD_NAME_SNAPSHOT_SPACE_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotCollectionSpacesInner::type ()
   {
      return CMD_SNAPSHOT_COLLECTIONSPACES ;
   }

   INT32 _rtnSnapshotCollectionSpacesInner::_getFetchType() const
   {
      return RTN_FETCH_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnSnapshotCollectionSpacesInner::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnSnapshotCollectionSpacesInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotAccessPlans )
   _rtnSnapshotAccessPlans::_rtnSnapshotAccessPlans ()
   {
   }

   _rtnSnapshotAccessPlans::~_rtnSnapshotAccessPlans ()
   {
   }

   const CHAR *_rtnSnapshotAccessPlans::name ()
   {
      return NAME_SNAPSHOT_ACCESSPLANS ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotAccessPlans::type ()
   {
      return CMD_SNAPSHOT_ACCESSPLANS ;
   }

   INT32 _rtnSnapshotAccessPlans::_getFetchType() const
   {
      return RTN_FETCH_ACCESSPLANS ;
   }

   BOOLEAN _rtnSnapshotAccessPlans::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnSnapshotAccessPlans::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   const CHAR* _rtnSnapshotAccessPlans::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER( _rtnSnapshotAccessPlansInner )
   _rtnSnapshotAccessPlansInner::_rtnSnapshotAccessPlansInner ()
   {
   }

   _rtnSnapshotAccessPlansInner::~_rtnSnapshotAccessPlansInner ()
   {
   }

   const CHAR *_rtnSnapshotAccessPlansInner::name ()
   {
      return CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR ;
   }
   RTN_COMMAND_TYPE _rtnSnapshotAccessPlansInner::type ()
   {
      return CMD_SNAPSHOT_ACCESSPLANS ;
   }

   INT32 _rtnSnapshotAccessPlansInner::_getFetchType() const
   {
      return RTN_FETCH_ACCESSPLANS ;
   }

   BOOLEAN _rtnSnapshotAccessPlansInner::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnSnapshotAccessPlansInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotConfigs)
   _rtnSnapshotConfigs::_rtnSnapshotConfigs ()
   {
   }

   _rtnSnapshotConfigs::~_rtnSnapshotConfigs ()
   {
   }

   const CHAR *_rtnSnapshotConfigs::name ()
   {
      return NAME_SNAPSHOT_CONFIGS ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotConfigs::type ()
   {
      return CMD_SNAPSHOT_CONFIGS ;
   }

   INT32 _rtnSnapshotConfigs::_getFetchType() const
   {
      return RTN_FETCH_CONFIGS ;
   }

   BOOLEAN _rtnSnapshotConfigs::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotConfigs::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME |
             MON_MASK_IS_PRIMARY |
             MON_MASK_SERVICE_STATUS |
             MON_MASK_LSN_INFO |
             MON_MASK_NODEID ;
   }

   const CHAR* _rtnSnapshotConfigs::getIntrCMDName()
   {
      return CMD_NAME_SNAPSHOT_CONFIGS_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnSnapshotConfigsInner)
   const CHAR *_rtnSnapshotConfigsInner::name ()
   {
      return CMD_NAME_SNAPSHOT_CONFIGS_INTR ;
   }

   RTN_COMMAND_TYPE _rtnSnapshotConfigsInner::type ()
   {
      return CMD_SNAPSHOT_CONFIGS ;
   }

   INT32 _rtnSnapshotConfigsInner::_getFetchType() const
   {
      return RTN_FETCH_CONFIGS ;
   }

   BOOLEAN _rtnSnapshotConfigsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnSnapshotConfigsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME |
             MON_MASK_IS_PRIMARY |
             MON_MASK_SERVICE_STATUS |
             MON_MASK_LSN_INFO |
             MON_MASK_NODEID ;
   }

}
