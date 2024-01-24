/******************************************************************************


   Copyright (C) 2011-2023 SequoiaDB Ltd.

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


******************************************************************************/

/* This list file is automatically generated, you shoud NOT modify this file anyway!*/

#ifndef AUTH_ACTION_TYPE_GEN_HPP__
#define AUTH_ACTION_TYPE_GEN_HPP__
#include <vector>
namespace engine
{
   enum ACTION_TYPE_ENUM
   {
      ACTION_TYPE__invalid = -1,
      ACTION_TYPE_alterCL,
      ACTION_TYPE_attachCL,
      ACTION_TYPE_copyIndex,
      ACTION_TYPE_find,
      ACTION_TYPE_insert,
      ACTION_TYPE_update,
      ACTION_TYPE_remove,
      ACTION_TYPE_getDetail,
      ACTION_TYPE_createIndex,
      ACTION_TYPE_dropIndex,
      ACTION_TYPE_detachCL,
      ACTION_TYPE_truncate,
      ACTION_TYPE_split,
      ACTION_TYPE_alterCS,
      ACTION_TYPE_createCL,
      ACTION_TYPE_dropCL,
      ACTION_TYPE_renameCL,
      ACTION_TYPE_listCollections,
      ACTION_TYPE_testCL,
      ACTION_TYPE_alterBin,
      ACTION_TYPE_countBin,
      ACTION_TYPE_dropAllBin,
      ACTION_TYPE_dropItemBin,
      ACTION_TYPE_getDetailBin,
      ACTION_TYPE_listBin,
      ACTION_TYPE_returnItemBin,
      ACTION_TYPE_snapshotBin,
      ACTION_TYPE_analyze,
      ACTION_TYPE_backup,
      ACTION_TYPE_createCS,
      ACTION_TYPE_dropCS,
      ACTION_TYPE_loadCS,
      ACTION_TYPE_unloadCS,
      ACTION_TYPE_cancelTask,
      ACTION_TYPE_createRole,
      ACTION_TYPE_dropRole,
      ACTION_TYPE_getRole,
      ACTION_TYPE_listRoles,
      ACTION_TYPE_updateRole,
      ACTION_TYPE_grantPrivilegesToRole,
      ACTION_TYPE_revokePrivilegesFromRole,
      ACTION_TYPE_grantRolesToRole,
      ACTION_TYPE_revokeRolesFromRole,
      ACTION_TYPE_createUsr,
      ACTION_TYPE_dropUsr,
      ACTION_TYPE_getUser,
      ACTION_TYPE_grantRolesToUser,
      ACTION_TYPE_revokeRolesFromUser,
      ACTION_TYPE_createDataSource,
      ACTION_TYPE_createDomain,
      ACTION_TYPE_createProcedure,
      ACTION_TYPE_createRG,
      ACTION_TYPE_createSequence,
      ACTION_TYPE_dropDataSource,
      ACTION_TYPE_dropDomain,
      ACTION_TYPE_dropSequence,
      ACTION_TYPE_eval,
      ACTION_TYPE_flushConfigure,
      ACTION_TYPE_forceSession,
      ACTION_TYPE_forceStepUp,
      ACTION_TYPE_getDataSource,
      ACTION_TYPE_getDomain,
      ACTION_TYPE_getRG,
      ACTION_TYPE_getSequence,
      ACTION_TYPE_getTask,
      ACTION_TYPE_invalidateCache,
      ACTION_TYPE_invalidateUserCache,
      ACTION_TYPE_list,
      ACTION_TYPE_listCollectionSpaces,
      ACTION_TYPE_removeBackup,
      ACTION_TYPE_removeRG,
      ACTION_TYPE_removeProcedure,
      ACTION_TYPE_renameCS,
      ACTION_TYPE_resetSnapshot,
      ACTION_TYPE_reloadConf,
      ACTION_TYPE_deleteConf,
      ACTION_TYPE_snapshot,
      ACTION_TYPE_sync,
      ACTION_TYPE_setPDLevel,
      ACTION_TYPE_trace,
      ACTION_TYPE_traceStatus,
      ACTION_TYPE_trans,
      ACTION_TYPE_updateConf,
      ACTION_TYPE_testCS,
      ACTION_TYPE_waitTasks,
      ACTION_TYPE_listProcedures,
      ACTION_TYPE_alterUser,
      ACTION_TYPE_listBackup,
      ACTION_TYPE_alterDataSource,
      ACTION_TYPE_createNode,
      ACTION_TYPE_getNode,
      ACTION_TYPE_reelect,
      ACTION_TYPE_removeNode,
      ACTION_TYPE_startRG,
      ACTION_TYPE_stopRG,
      ACTION_TYPE_alterRG,
      ACTION_TYPE_startNode,
      ACTION_TYPE_stopNode,
      ACTION_TYPE_alterNode,
      ACTION_TYPE_fetchSequence,
      ACTION_TYPE_getSequenceCurrentValue,
      ACTION_TYPE_alterSequence,
      ACTION_TYPE_alterDomain,
      ACTION_TYPE_getDCInfo,
      ACTION_TYPE_alterDC,
   };

   const int ACTION_TYPE_NUM_GEN = 106;
   const int ACTION_TYPE_VALID_NUM_GEN = 105;


   ACTION_TYPE_ENUM authActionTypeParse( const char *actionName );
   const char *authActionTypeSerializer( ACTION_TYPE_ENUM actionType );

   enum RESOURCE_TYPE_ENUM
   {
      RESOURCE_TYPE__INVALID,
      RESOURCE_TYPE_CLUSTER,
      RESOURCE_TYPE_COLLECTION_NAME,
      RESOURCE_TYPE_COLLECTION_SPACE,
      RESOURCE_TYPE_EXACT_COLLECTION,
      RESOURCE_TYPE_NON_SYSTEM,
      RESOURCE_TYPE_ANY,
   };

   const int ACTION_SET_NUMBERS = 2 ;
   struct ACTION_SET_NUMBER_ARRAY
   {
      unsigned long long numbers[ACTION_SET_NUMBERS];
      ACTION_SET_NUMBER_ARRAY(
         unsigned long long n0,
         unsigned long long n1
      )
      {
         numbers[0] = n0;
         numbers[1] = n1;
      }
   };

   // _INVALID: []
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE__INVALID_BITSET_NUMBERS(
      0ULL,
      0ULL
   );

   // CLUSTER: ['listCollectionSpaces', 'updateConf', 'backup', 'createCS', 'createSequence', 'dropSequence', 'alterSequence', 'getSequenceCurrentValue', 'cancelTask', 'dropCS', 'loadCS', 'unloadCS', 'getDCInfo', 'list', 'snapshot', 'listBin', 'renameCS', 'removeBackup', 'createRG', 'removeRG', 'startRG', 'stopRG', 'createNode', 'removeNode', 'startNode', 'stopNode', 'setPDLevel', 'waitTasks', 'trace', 'traceStatus', 'createDomain', 'dropDomain', 'createProcedure', 'forceStepUp', 'removeProcedure', 'listProcedures', 'eval', 'invalidateCache', 'invalidateUserCache', 'forceSession', 'alterDC', 'alterUser', 'reelect', 'sync', 'reloadConf', 'deleteConf', 'createDataSource', 'dropDataSource', 'alterDataSource', 'alterBin', 'countBin', 'dropAllBin', 'dropItemBin', 'getDetailBin', 'listBin', 'returnItemBin', 'snapshotBin', 'alterDomain', 'createRole', 'dropRole', 'getRole', 'listRoles', 'updateRole', 'grantPrivilegesToRole', 'revokePrivilegesFromRole', 'grantRolesToRole', 'revokeRolesFromRole', 'createUsr', 'dropUsr', 'getUser', 'grantRolesToUser', 'revokeRolesFromUser', 'fetchSequence', 'flushConfigure', 'forceSession', 'getDataSource', 'getDomain', 'getRG', 'getSequence', 'trans', 'getTask', 'getNode', 'resetSnapshot', 'listBackup', 'alterNode', 'alterRG']
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_CLUSTER_BITSET_NUMBERS(
      18446744073574809600ULL,
      2199022731263ULL
   );

   // COLLECTION_NAME: ['find', 'insert', 'update', 'remove', 'getDetail', 'createIndex', 'dropIndex', 'copyIndex', 'split', 'attachCL', 'detachCL', 'truncate', 'alterCL', 'testCL', 'analyze']
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_COLLECTION_NAME_BITSET_NUMBERS(
      134488063ULL,
      0ULL
   );

   // COLLECTION_SPACE: [['find', 'insert', 'update', 'remove', 'getDetail', 'createIndex', 'dropIndex', 'copyIndex', 'split', 'attachCL', 'detachCL', 'truncate', 'alterCL', 'testCL', 'analyze'], 'alterCS', 'createCL', 'dropCL', 'renameCL', 'listCollections', 'testCS']
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_COLLECTION_SPACE_BITSET_NUMBERS(
      134742015ULL,
      524288ULL
   );

   // EXACT_COLLECTION: ['find', 'insert', 'update', 'remove', 'getDetail', 'createIndex', 'dropIndex', 'copyIndex', 'split', 'attachCL', 'detachCL', 'truncate', 'alterCL', 'testCL', 'analyze']
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_EXACT_COLLECTION_BITSET_NUMBERS(
      134488063ULL,
      0ULL
   );

   // NON_SYSTEM: [['find', 'insert', 'update', 'remove', 'getDetail', 'createIndex', 'dropIndex', 'copyIndex', 'split', 'attachCL', 'detachCL', 'truncate', 'alterCL', 'testCL', 'analyze'], 'alterCS', 'createCL', 'dropCL', 'renameCL', 'listCollections', 'testCS']
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_NON_SYSTEM_BITSET_NUMBERS(
      134742015ULL,
      524288ULL
   );

   // ANY: __all__
   const ACTION_SET_NUMBER_ARRAY RESOURCE_TYPE_ANY_BITSET_NUMBERS(
      18446744073709551615ULL,
      18446744073709551615ULL
   );


   // Built-in roles
   // _cs_read
   const int BUILTIN_ROLE_DATA_SIZE_cs_read = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_cs_read[];

   // _cs_readWrite
   const int BUILTIN_ROLE_DATA_SIZE_cs_readWrite = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_cs_readWrite[];

   // _cs_admin
   const int BUILTIN_ROLE_DATA_SIZE_cs_admin = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_cs_admin[];

   // _clusterAdmin
   const int BUILTIN_ROLE_DATA_SIZE_clusterAdmin = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_clusterAdmin[];

   // _clusterMonitor
   const int BUILTIN_ROLE_DATA_SIZE_clusterMonitor = 2;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_clusterMonitor[];

   // _backup
   const int BUILTIN_ROLE_DATA_SIZE_backup = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_backup[];

   // _dbAdmin
   const int BUILTIN_ROLE_DATA_SIZE_dbAdmin = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_dbAdmin[];

   // _userAdmin
   const int BUILTIN_ROLE_DATA_SIZE_userAdmin = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_userAdmin[];

   // _root
   const int BUILTIN_ROLE_DATA_SIZE_root = 1;
   extern const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_root[];


   enum AUTH_CMD_ACTION_SETS_TAG
   {
      AUTH_CMD_NAME_BACKUP_OFFLINE_default,
      AUTH_CMD_NAME_CREATE_COLLECTION_default,
      AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default,
      AUTH_CMD_NAME_CREATE_SEQUENCE_default,
      AUTH_CMD_NAME_DROP_SEQUENCE_default,
      AUTH_CMD_NAME_ALTER_SEQUENCE_default,
      AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default,
      AUTH_CMD_NAME_CREATE_INDEX_default,
      AUTH_CMD_NAME_CANCEL_TASK_default,
      AUTH_CMD_NAME_DROP_COLLECTION_default,
      AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default,
      AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default,
      AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default,
      AUTH_CMD_NAME_DROP_INDEX_default,
      AUTH_CMD_NAME_COPY_INDEX_maincl,
      AUTH_CMD_NAME_GET_COUNT_default,
      AUTH_CMD_NAME_GET_INDEXES_default,
      AUTH_CMD_NAME_GET_QUERYMETA_default,
      AUTH_CMD_NAME_GET_DCINFO_default,
      AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default,
      AUTH_CMD_NAME_LIST_CONTEXTS_default,
      AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default,
      AUTH_CMD_NAME_LIST_SESSIONS_default,
      AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default,
      AUTH_CMD_NAME_LIST_STORAGEUNITS_default,
      AUTH_CMD_NAME_LIST_GROUPS_default,
      AUTH_CMD_NAME_LIST_DOMAINS_default,
      AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default,
      AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default,
      AUTH_CMD_NAME_LIST_USERS_default,
      AUTH_CMD_NAME_LIST_BACKUPS_default,
      AUTH_CMD_NAME_LIST_TASKS_default,
      AUTH_CMD_NAME_LIST_TRANSACTIONS_default,
      AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default,
      AUTH_CMD_NAME_LIST_SVCTASKS_default,
      AUTH_CMD_NAME_LIST_SEQUENCES_default,
      AUTH_CMD_NAME_LIST_DATASOURCES_default,
      AUTH_CMD_NAME_LIST_RECYCLEBIN_default,
      AUTH_CMD_NAME_RENAME_COLLECTION_default,
      AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default,
      AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default,
      AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default,
      AUTH_CMD_NAME_SNAPSHOT_DATABASE_default,
      AUTH_CMD_NAME_SNAPSHOT_RESET_default,
      AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default,
      AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default,
      AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default,
      AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default,
      AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default,
      AUTH_CMD_NAME_SNAPSHOT_CATA_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default,
      AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default,
      AUTH_CMD_NAME_SNAPSHOT_HEALTH_default,
      AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default,
      AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default,
      AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default,
      AUTH_CMD_NAME_SNAPSHOT_QUERIES_default,
      AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default,
      AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default,
      AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default,
      AUTH_CMD_NAME_SNAPSHOT_TASKS_default,
      AUTH_CMD_NAME_SNAPSHOT_INDEXES_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default,
      AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default,
      AUTH_CMD_NAME_TEST_COLLECTION_default,
      AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default,
      AUTH_CMD_NAME_CREATE_GROUP_default,
      AUTH_CMD_NAME_REMOVE_GROUP_default,
      AUTH_CMD_NAME_CREATE_NODE_default,
      AUTH_CMD_NAME_REMOVE_NODE_default,
      AUTH_CMD_NAME_REMOVE_BACKUP_default,
      AUTH_CMD_NAME_ACTIVE_GROUP_default,
      AUTH_CMD_NAME_STARTUP_NODE_default,
      AUTH_CMD_NAME_SHUTDOWN_NODE_default,
      AUTH_CMD_NAME_SHUTDOWN_GROUP_default,
      AUTH_CMD_NAME_SET_PDLEVEL_default,
      AUTH_CMD_NAME_SPLIT_default,
      AUTH_CMD_NAME_WAITTASK_default,
      AUTH_CMD_NAME_CREATE_CATA_GROUP_default,
      AUTH_CMD_NAME_TRACE_START_default,
      AUTH_CMD_NAME_TRACE_RESUME_default,
      AUTH_CMD_NAME_TRACE_STOP_default,
      AUTH_CMD_NAME_TRACE_STATUS_default,
      AUTH_CMD_NAME_CREATE_DOMAIN_default,
      AUTH_CMD_NAME_DROP_DOMAIN_default,
      AUTH_CMD_NAME_EXPORT_CONFIG_default,
      AUTH_CMD_NAME_CRT_PROCEDURE_default,
      AUTH_CMD_NAME_RM_PROCEDURE_default,
      AUTH_CMD_NAME_LIST_PROCEDURES_default,
      AUTH_CMD_NAME_EVAL_default,
      AUTH_CMD_NAME_LINK_CL_maincl,
      AUTH_CMD_NAME_LINK_CL_subcl,
      AUTH_CMD_NAME_UNLINK_CL_default,
      AUTH_CMD_NAME_INVALIDATE_CACHE_default,
      AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default,
      AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default,
      AUTH_CMD_NAME_FORCE_SESSION_default,
      AUTH_CMD_NAME_LIST_LOBS_default,
      AUTH_CMD_NAME_ALTER_DC_default,
      AUTH_CMD_NAME_ALTER_USR_default,
      AUTH_CMD_NAME_REELECT_default,
      AUTH_CMD_NAME_FORCE_STEP_UP_default,
      AUTH_CMD_NAME_TRUNCATE_default,
      AUTH_CMD_NAME_SYNC_DB_default,
      AUTH_CMD_NAME_POP_default,
      AUTH_CMD_NAME_RELOAD_CONFIG_default,
      AUTH_CMD_NAME_UPDATE_CONFIG_default,
      AUTH_CMD_NAME_DELETE_CONFIG_default,
      AUTH_CMD_NAME_GET_CL_DETAIL_default,
      AUTH_CMD_NAME_GET_CL_STAT_default,
      AUTH_CMD_NAME_GET_INDEX_STAT_default,
      AUTH_CMD_NAME_CREATE_DATASOURCE_default,
      AUTH_CMD_NAME_DROP_DATASOURCE_default,
      AUTH_CMD_NAME_ALTER_DATASOURCE_default,
      AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default,
      AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default,
      AUTH_CMD_NAME_ALTER_RECYCLEBIN_default,
      AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default,
      AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default,
      AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default,
      AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default,
      AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default,
      AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default,
      AUTH_CMD_NAME_LIST_COLLECTION_INTR_default,
      AUTH_CMD_NAME_LIST_SPACE_INTR_default,
      AUTH_CMD_NAME_LIST_CONTEXT_INTR_default,
      AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default,
      AUTH_CMD_NAME_LIST_SESSION_INTR_default,
      AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default,
      AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default,
      AUTH_CMD_NAME_LIST_BACKUP_INTR_default,
      AUTH_CMD_NAME_LIST_TRANS_INTR_default,
      AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default,
      AUTH_CMD_NAME_LIST_GROUP_INTR_default,
      AUTH_CMD_NAME_LIST_USER_INTR_default,
      AUTH_CMD_NAME_LIST_TASK_INTR_default,
      AUTH_CMD_NAME_LIST_INDEXES_INTR_default,
      AUTH_CMD_NAME_LIST_DOMAIN_INTR_default,
      AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default,
      AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default,
      AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default,
      AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default,
      AUTH_CMD_NAME_ALTER_COLLECTION_default,
      AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default,
      AUTH_CMD_NAME_ALTER_DOMAIN_default,
      AUTH_CMD_NAME_ALTER_NODE_default,
      AUTH_CMD_NAME_ALTER_GROUP_default,
      AUTH_CMD_NAME_CREATE_ROLE_default,
      AUTH_CMD_NAME_DROP_ROLE_default,
      AUTH_CMD_NAME_GET_ROLE_default,
      AUTH_CMD_NAME_LIST_ROLES_default,
      AUTH_CMD_NAME_UPDATE_ROLE_default,
      AUTH_CMD_NAME_GRANT_PRIVILEGES_default,
      AUTH_CMD_NAME_REVOKE_PRIVILEGES_default,
      AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default,
      AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default,
      AUTH_CMD_NAME_GET_USER_default,
      AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default,
      AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default,
      AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default,
   };

   typedef class _authActionSet authActionSet;
   typedef const authActionSet *ACTION_SETS_PTR;
   class authRequiredActionSets
   {
      public:
         enum SOURCE_OBJ
         {
            SOURCE_OBJ_NONE,
            SOURCE_OBJ_QUERY,
            SOURCE_OBJ_SELECTOR,
            SOURCE_OBJ_ORDERBY,
            SOURCE_OBJ_HINT
         };

         struct SOURCE
         {
            SOURCE( SOURCE_OBJ obj, const char *key ) : obj( obj ), key( key ) {}
            SOURCE_OBJ obj;
            const char *key;
         };
      public:
         authRequiredActionSets( RESOURCE_TYPE_ENUM t, ACTION_SETS_PTR sets, unsigned int size, SOURCE_OBJ obj, const char *key )
            : _t( t ), _sets( sets ), _size( size ), _source( obj, key ) {}
         RESOURCE_TYPE_ENUM getResourceType() const { return _t; }
         ACTION_SETS_PTR getActionSets() const { return _sets; }
         unsigned int getSize() const { return _size; }
         SOURCE getSource() const { return _source; }
      private:
         RESOURCE_TYPE_ENUM _t;
         ACTION_SETS_PTR _sets;
         unsigned int _size;
         SOURCE _source;
   };
   typedef const std::pair< const AUTH_CMD_ACTION_SETS_TAG*, unsigned int > CMD_TAGS_ARRAY;
   const CMD_TAGS_ARRAY* authGetCMDActionSetsTags( const char *cmd );
   const authRequiredActionSets* authGetCMDActionSetsByTag( AUTH_CMD_ACTION_SETS_TAG tag );
}

#endif