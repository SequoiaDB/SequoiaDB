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

   Source File Name = rtnCommandSnapshot.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_COMMAND_SNAPSHOT_HPP_
#define RTN_COMMAND_SNAPSHOT_HPP_

#include "rtnCommandMon.hpp"
#include "monDump.hpp"
#include "rtnFetchBase.hpp"
#include "rtnDetectDeadlock.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;

   class _rtnSnapshotInner : public _rtnMonInnerBase
   {
      protected:
         _rtnSnapshotInner(CHAR* name,
                           RTN_COMMAND_TYPE type,
                           INT32 fetchType,
                           UINT32 infoMask )
           : _rtnMonInnerBase(name, type, fetchType, infoMask) {}

         virtual ~_rtnSnapshotInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const = 0;
         virtual BOOLEAN _isDetail() const { return TRUE ; }
   } ;

   class _rtnSnapshot : public _rtnMonBase
   {
      protected:
         _rtnSnapshot(const CHAR* name,
                      const CHAR* intrName,
                      RTN_COMMAND_TYPE type,
                      INT32 fetchType,
                      UINT32 infoMask )
           : _rtnMonBase(name, intrName, type, fetchType, infoMask) {}
         virtual ~_rtnSnapshot () {}

      protected:
         virtual BOOLEAN _isCurrent() const = 0 ;
         virtual BOOLEAN _isDetail() const { return TRUE ; }
   } ;

   class _rtnSnapshotSystem : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotSystem()
            : _rtnSnapshot( NAME_SNAPSHOT_SYSTEM,
                            CMD_NAME_SNAPSHOT_SYSTEM_INTR,
                            CMD_SNAPSHOT_SYSTEM,
                            RTN_FETCH_SYSTEM,
                            MON_MASK_ALL)
         {}
         virtual ~_rtnSnapshotSystem () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotSystemInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSystemInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_SYSTEM_INTR,
                                 CMD_SNAPSHOT_SYSTEM,
                                 RTN_FETCH_SYSTEM,
                                 MON_MASK_ALL)
         {}
         virtual ~_rtnSnapshotSystemInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotHealth : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotHealth()
            : _rtnSnapshot( NAME_SNAPSHOT_HEALTH,
                            CMD_NAME_SNAPSHOT_HEALTH_INTR,
                            CMD_SNAPSHOT_HEALTH,
                            RTN_FETCH_HEALTH,
                            MON_MASK_NODE_NAME |
                            MON_MASK_IS_PRIMARY |
                            MON_MASK_SERVICE_STATUS |
                            MON_MASK_LSN_INFO |
                            MON_MASK_NODEID )
         {}
         virtual ~_rtnSnapshotHealth () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotHealthInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotHealthInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_HEALTH_INTR,
                                 CMD_SNAPSHOT_HEALTH,
                                 RTN_FETCH_HEALTH,
                                 MON_MASK_NODE_NAME |
                                 MON_MASK_IS_PRIMARY |
                                 MON_MASK_SERVICE_STATUS |
                                 MON_MASK_LSN_INFO |
                                 MON_MASK_NODEID )
         {}
         virtual ~_rtnSnapshotHealthInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotTasks : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotTasks()
            : _rtnSnapshot( NAME_SNAPSHOT_TASKS,
                            CMD_NAME_SNAPSHOT_TASKS_INTR,
                            CMD_SNAPSHOT_TASKS,
                            RTN_FETCH_TASKS,
                            MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnSnapshotTasks () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotTasksInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTasksInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_TASKS_INTR,
                                 CMD_SNAPSHOT_TASKS,
                                 RTN_FETCH_TASKS,
                                 MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnSnapshotTasksInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotIndexes : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotIndexes()
            : _rtnSnapshot( NAME_SNAPSHOT_INDEXES,
                            CMD_NAME_SNAPSHOT_INDEXES_INTR,
                            CMD_SNAPSHOT_INDEXES,
                            RTN_FETCH_INDEX,
                            MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ),
              _collection( NULL ),
              _clInited( FALSE )
         {}
         virtual ~_rtnSnapshotIndexes () {}

         virtual const CHAR* collectionFullName() ;

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;

      protected:
         const CHAR* _collection ;
         BOOLEAN     _clInited ;
   } ;

   class _rtnSnapshotIndexesInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotIndexesInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_INDEXES_INTR,
                                 CMD_SNAPSHOT_INDEXES,
                                 RTN_FETCH_INDEX,
                                 MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ),
              _collection( NULL ),
              _clInited( FALSE )
         {}
         virtual ~_rtnSnapshotIndexesInner() {}

         virtual const CHAR* collectionFullName() ;

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;

      protected:
         const CHAR* _collection ;
         BOOLEAN     _clInited ;
   } ;

   class _rtnSnapshotContexts : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContexts()
            : _rtnSnapshot( NAME_SNAPSHOT_CONTEXTS,
                            CMD_NAME_SNAPSHOT_CONTEXT_INTR,
                            CMD_SNAPSHOT_CONTEXTS,
                            RTN_FETCH_CONTEXT,
                            MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotContexts () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotContextsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_CONTEXT_INTR,
                                 CMD_SNAPSHOT_CONTEXTS,
                                 RTN_FETCH_CONTEXT,
                                 MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotContextsInner() {}


      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotContextsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsCurrent()
            : _rtnSnapshot( NAME_SNAPSHOT_CONTEXTS_CURRENT,
                            CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR,
                            CMD_SNAPSHOT_CONTEXTS_CURRENT,
                            RTN_FETCH_CONTEXT,
                            MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotContextsCurrent () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotContextsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsCurrentInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR,
                                 CMD_SNAPSHOT_CONTEXTS_CURRENT,
                                 RTN_FETCH_CONTEXT,
                                 MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotContextsCurrentInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotDatabase : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotDatabase()
            : _rtnSnapshot( NAME_SNAPSHOT_DATABASE,
                            CMD_NAME_SNAPSHOT_DATABASE_INTR,
                            CMD_SNAPSHOT_DATABASE,
                            RTN_FETCH_DATABASE,
                            MON_MASK_ALL)
         {}
         virtual ~_rtnSnapshotDatabase () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotDatabaseInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotDatabaseInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_DATABASE_INTR,
                                 CMD_SNAPSHOT_DATABASE,
                                 RTN_FETCH_DATABASE,
                                 MON_MASK_ALL)
         {}
         virtual ~_rtnSnapshotDatabaseInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotCollections : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollections()
            : _rtnSnapshot( NAME_SNAPSHOT_COLLECTIONS,
                            CMD_NAME_SNAPSHOT_COLLECTION_INTR,
                            CMD_SNAPSHOT_COLLECTIONS,
                            RTN_FETCH_COLLECTION,
                            MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnSnapshotCollections () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotCollectionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_COLLECTION_INTR,
                                 CMD_SNAPSHOT_COLLECTIONS,
                                 RTN_FETCH_COLLECTION,
                                 MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnSnapshotCollectionsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotCollectionSpaces : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionSpaces()
            : _rtnSnapshot( NAME_SNAPSHOT_COLLECTIONSPACES,
                            CMD_NAME_SNAPSHOT_SPACE_INTR,
                            CMD_SNAPSHOT_COLLECTIONSPACES,
                            RTN_FETCH_COLLECTIONSPACE,
                            MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnSnapshotCollectionSpaces () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotCollectionSpacesInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionSpacesInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_SPACE_INTR,
                                 CMD_SNAPSHOT_COLLECTIONSPACES,
                                 RTN_FETCH_COLLECTIONSPACE,
                                 MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )

         {}
         virtual ~_rtnSnapshotCollectionSpacesInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotReset : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotReset () ;
         virtual ~_rtnSnapshotReset () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

         virtual const CHAR * collectionFullName ()
         {
            return _collection ;
         }

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      private:
         RTN_COMMAND_TYPE _type ;
         EDUID _sessionID ;
         BOOLEAN _resetAllSession ;
         const CHAR * _collectionSpace ;
         const CHAR * _collection ;

   };

   class _rtnSnapshotSessions : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessions()
            : _rtnSnapshot( NAME_SNAPSHOT_SESSIONS,
                            CMD_NAME_SNAPSHOT_SESSION_INTR,
                            CMD_SNAPSHOT_SESSIONS,
                            RTN_FETCH_SESSION,
                            MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotSessions () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotSessionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_SESSION_INTR,
                                 CMD_SNAPSHOT_SESSIONS,
                                 RTN_FETCH_SESSION,
                                 MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotSessionsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotSessionsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsCurrent()
            : _rtnSnapshot( NAME_SNAPSHOT_SESSIONS_CURRENT,
                            CMD_NAME_SNAPSHOT_SESSIONCUR_INTR,
                            CMD_SNAPSHOT_SESSIONS_CURRENT,
                            RTN_FETCH_SESSION,
                            MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotSessionsCurrent () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotSessionsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsCurrentInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_SESSIONCUR_INTR,
                                 CMD_SNAPSHOT_SESSIONS,
                                 RTN_FETCH_SESSION,
                                 MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotSessionsCurrentInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   };

   class _rtnSnapshotTransactionsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsCurrent()
            : _rtnSnapshot( NAME_SNAPSHOT_TRANSACTIONS_CUR,
                            CMD_NAME_SNAPSHOT_TRANSCUR_INTR,
                            CMD_SNAPSHOT_TRANSACTIONS_CUR,
                            RTN_FETCH_TRANS,
                            MON_MASK_NODE_NAME)

         {}
         virtual ~_rtnSnapshotTransactionsCurrent () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotTransactionsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsCurrentInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_TRANSCUR_INTR,
                                 CMD_SNAPSHOT_TRANSACTIONS_CUR,
                                 RTN_FETCH_TRANS,
                                 MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnSnapshotTransactionsCurrentInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotTransactions : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactions()
            : _rtnSnapshot( NAME_SNAPSHOT_TRANSACTIONS,
                            CMD_NAME_SNAPSHOT_TRANS_INTR,
                            CMD_SNAPSHOT_TRANSACTIONS,
                            RTN_FETCH_TRANS,
                            MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnSnapshotTransactions () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotTransactionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_TRANS_INTR,
                                 CMD_SNAPSHOT_TRANSACTIONS,
                                 RTN_FETCH_TRANS,
                                 MON_MASK_NODE_NAME )
         {}
         virtual ~_rtnSnapshotTransactionsInner () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnSnapshotAccessPlans define
    */
   class _rtnSnapshotAccessPlans : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public :
         _rtnSnapshotAccessPlans()
            : _rtnSnapshot( NAME_SNAPSHOT_ACCESSPLANS,
                            CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR,
                            CMD_SNAPSHOT_ACCESSPLANS,
                            RTN_FETCH_ACCESSPLANS,
                            MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}
         virtual ~_rtnSnapshotAccessPlans () {}

      protected :
         virtual BOOLEAN _isCurrent() const ;
   } ;

   /*
      _rtnSnapshotAccessPlansInner define
    */
   class _rtnSnapshotAccessPlansInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotAccessPlansInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR,
                                 CMD_SNAPSHOT_ACCESSPLANS,
                                 RTN_FETCH_ACCESSPLANS,
                                 MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME)
         {}

         virtual ~_rtnSnapshotAccessPlansInner () {}

      protected:
         virtual BOOLEAN _isCurrent () const ;
   } ;

   class _rtnSnapshotConfigs : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotConfigs()
            : _rtnSnapshot( NAME_SNAPSHOT_CONFIGS,
                            CMD_NAME_SNAPSHOT_CONFIGS_INTR,
                            CMD_SNAPSHOT_CONFIGS,
                            RTN_FETCH_CONFIGS,
                            MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotConfigs () {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   };

   class _rtnSnapshotConfigsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotConfigsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_CONFIGS_INTR,
                                 CMD_SNAPSHOT_CONFIGS,
                                 RTN_FETCH_CONFIGS,
                                 MON_MASK_NODE_NAME)
         {}
         virtual ~_rtnSnapshotConfigsInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotVCLSessionInfoInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotVCLSessionInfoInner()
            : _rtnSnapshotInner( SYS_CL_SESSION_INFO,
                                 CMD_SNAPSHOT_VCL_SESSIONINFO,
                                 RTN_FETCH_VCL_SESSIONINFO,
                                 0)
         {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotSvcTasks : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSvcTasks()
            : _rtnSnapshot( NAME_SNAPSHOT_SVCTASKS,
                            CMD_NAME_SNAPSHOT_SVCTASKS_INTR,
                            CMD_SNAPSHOT_SVCTASKS,
                            RTN_FETCH_SVCTASKS,
                            MON_MASK_NODE_NAME)
         {}

         virtual ~_rtnSnapshotSvcTasks() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotSvcTasksInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSvcTasksInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_SVCTASKS_INTR,
                                 CMD_SNAPSHOT_SVCTASKS,
                                 RTN_FETCH_SVCTASKS,
                                 MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotSvcTasksInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotQueries : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotQueries()
            : _rtnSnapshot( NAME_SNAPSHOT_QUERIES,
                            CMD_NAME_SNAPSHOT_QUERIES_INTR,
                            CMD_SNAPSHOT_QUERIES,
                            RTN_FETCH_QUERIES,
                            MON_MASK_NODE_NAME | MON_MASK_NODEID )
         {}

         virtual ~_rtnSnapshotQueries() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;
   class _rtnSnapshotQueriesInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotQueriesInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_QUERIES_INTR,
                                 CMD_SNAPSHOT_QUERIES,
                                 RTN_FETCH_QUERIES,
                                 MON_MASK_NODE_NAME | MON_MASK_NODEID )
         {}

         virtual ~_rtnSnapshotQueriesInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotLatchWaits : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotLatchWaits()
            : _rtnSnapshot( NAME_SNAPSHOT_LATCHWAITS,
                            CMD_NAME_SNAPSHOT_LATCHWAITS_INTR,
                            CMD_SNAPSHOT_LATCHWAITS,
                            RTN_FETCH_LATCHWAITS,
                            MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotLatchWaits() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotLatchWaitsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotLatchWaitsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_LATCHWAITS_INTR,
                                 CMD_SNAPSHOT_LATCHWAITS,
                                 RTN_FETCH_LATCHWAITS,
                                 MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotLatchWaitsInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotLockWaits : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotLockWaits()
            : _rtnSnapshot( NAME_SNAPSHOT_LOCKWAITS,
                            CMD_NAME_SNAPSHOT_LOCKWAITS_INTR,
                            CMD_SNAPSHOT_LOCKWAITS,
                            RTN_FETCH_LOCKWAITS,
                            MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotLockWaits() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotLockWaitsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotLockWaitsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_LOCKWAITS_INTR,
                                 CMD_SNAPSHOT_LOCKWAITS,
                                 RTN_FETCH_LOCKWAITS,
                                 MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotLockWaitsInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotIndexStats : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotIndexStats()
            : _rtnSnapshot( NAME_SNAPSHOT_INDEXSTATS,
                            CMD_NAME_SNAPSHOT_INDEXSTATS_INTR,
                            CMD_SNAPSHOT_INDEXSTATS,
                            RTN_FETCH_INDEXSTATS,
                            MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}

         virtual ~_rtnSnapshotIndexStats() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotIndexStatsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotIndexStatsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_INDEXSTATS_INTR,
                                 CMD_SNAPSHOT_INDEXSTATS,
                                 RTN_FETCH_INDEXSTATS,
                                 MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
         {}

         virtual ~_rtnSnapshotIndexStatsInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual BSONObj _getOptObj() const ;
   } ;

   class _rtnSnapshotTransWaits : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransWaits()
            : _rtnSnapshot( NAME_SNAPSHOT_TRANSWAITS,
                            CMD_NAME_SNAPSHOT_TRANSWAITS_INTR,
                            CMD_SNAPSHOT_TRANSWAITS,
                            RTN_FETCH_TRANSWAITS,
                            MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotTransWaits() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;

   class _rtnSnapshotTransWaitsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransWaitsInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_TRANSWAITS_INTR,
                                 CMD_SNAPSHOT_TRANSWAITS,
                                 RTN_FETCH_TRANSWAITS,
                                 MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotTransWaitsInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
   } ;


   class _rtnSnapshotTransDeadlock : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransDeadlock()
            : _rtnSnapshot( NAME_SNAPSHOT_TRANSDEADLOCK,
                            CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR,
                            CMD_SNAPSHOT_TRANSDEADLOCK,
                            RTN_FETCH_TRANSWAITS,
                            MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotTransDeadlock() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual INT32   _getMonProcessor( IRtnMonProcessorPtr & ptr ) ;
   } ;

   class _rtnSnapshotTransDeadlockInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransDeadlockInner()
            : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR,
                                 CMD_SNAPSHOT_TRANSDEADLOCK,
                                 RTN_FETCH_TRANSWAITS,
                                 MON_MASK_NODE_NAME )
         {}

         virtual ~_rtnSnapshotTransDeadlockInner() {}

      protected:
         virtual BOOLEAN _isCurrent() const ;
         virtual INT32   _getMonProcessor( IRtnMonProcessorPtr & ptr ) ;
   } ;

   /*
      _rtnSnapshotRecycleBin define
    */
   class _rtnSnapshotRecycleBin : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnSnapshotRecycleBin()
      : _rtnSnapshot( NAME_SNAPSHOT_RECYCLEBIN,
                      CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR,
                      CMD_SNAPSHOT_RECYCLEBIN,
                      RTN_FETCH_RECYCLEBIN,
                      MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
      {
      }

      virtual ~_rtnSnapshotRecycleBin()
      {
      }

   protected:
      virtual BOOLEAN _isCurrent() const
      {
         return FALSE ;
      }
   } ;

   typedef class _rtnSnapshotRecycleBin rtnSnapshotRecycleBin ;

   /*
      _rtnSnapshotRecycleBinInner define
    */
   class _rtnSnapshotRecycleBinInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

   public:
      _rtnSnapshotRecycleBinInner()
      : _rtnSnapshotInner( CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR,
                           CMD_SNAPSHOT_RECYCLEBIN,
                           RTN_FETCH_RECYCLEBIN,
                           MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME )
      {
      }

      virtual ~_rtnSnapshotRecycleBinInner()
      {
      }

   protected:
      virtual BOOLEAN _isCurrent() const
      {
         return FALSE ;
      }
   } ;

   typedef class _rtnSnapshotRecycleBinInner rtnSnapshotRecycleBinInner ;

}

#endif //RTN_COMMAND_SNAPSHOT_HPP_
