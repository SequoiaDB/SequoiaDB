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

#include "ossTypes.hpp"
#include "authRBACGen.hpp"
#include "authActionSet.hpp"
#include "msgDef.h"
#include <cstring>

namespace engine
{
   const char ACTION_NAME__invalid[] = "_invalid";
   const char ACTION_NAME_alterCL[] = "alterCL";
   const char ACTION_NAME_attachCL[] = "attachCL";
   const char ACTION_NAME_copyIndex[] = "copyIndex";
   const char ACTION_NAME_find[] = "find";
   const char ACTION_NAME_insert[] = "insert";
   const char ACTION_NAME_update[] = "update";
   const char ACTION_NAME_remove[] = "remove";
   const char ACTION_NAME_getDetail[] = "getDetail";
   const char ACTION_NAME_createIndex[] = "createIndex";
   const char ACTION_NAME_dropIndex[] = "dropIndex";
   const char ACTION_NAME_detachCL[] = "detachCL";
   const char ACTION_NAME_truncate[] = "truncate";
   const char ACTION_NAME_split[] = "split";
   const char ACTION_NAME_alterCS[] = "alterCS";
   const char ACTION_NAME_createCL[] = "createCL";
   const char ACTION_NAME_dropCL[] = "dropCL";
   const char ACTION_NAME_renameCL[] = "renameCL";
   const char ACTION_NAME_listCollections[] = "listCollections";
   const char ACTION_NAME_testCL[] = "testCL";
   const char ACTION_NAME_alterBin[] = "alterBin";
   const char ACTION_NAME_countBin[] = "countBin";
   const char ACTION_NAME_dropAllBin[] = "dropAllBin";
   const char ACTION_NAME_dropItemBin[] = "dropItemBin";
   const char ACTION_NAME_getDetailBin[] = "getDetailBin";
   const char ACTION_NAME_listBin[] = "listBin";
   const char ACTION_NAME_returnItemBin[] = "returnItemBin";
   const char ACTION_NAME_snapshotBin[] = "snapshotBin";
   const char ACTION_NAME_analyze[] = "analyze";
   const char ACTION_NAME_backup[] = "backup";
   const char ACTION_NAME_createCS[] = "createCS";
   const char ACTION_NAME_dropCS[] = "dropCS";
   const char ACTION_NAME_loadCS[] = "loadCS";
   const char ACTION_NAME_unloadCS[] = "unloadCS";
   const char ACTION_NAME_cancelTask[] = "cancelTask";
   const char ACTION_NAME_createRole[] = "createRole";
   const char ACTION_NAME_dropRole[] = "dropRole";
   const char ACTION_NAME_getRole[] = "getRole";
   const char ACTION_NAME_listRoles[] = "listRoles";
   const char ACTION_NAME_updateRole[] = "updateRole";
   const char ACTION_NAME_grantPrivilegesToRole[] = "grantPrivilegesToRole";
   const char ACTION_NAME_revokePrivilegesFromRole[] = "revokePrivilegesFromRole";
   const char ACTION_NAME_grantRolesToRole[] = "grantRolesToRole";
   const char ACTION_NAME_revokeRolesFromRole[] = "revokeRolesFromRole";
   const char ACTION_NAME_createUsr[] = "createUsr";
   const char ACTION_NAME_dropUsr[] = "dropUsr";
   const char ACTION_NAME_getUser[] = "getUser";
   const char ACTION_NAME_grantRolesToUser[] = "grantRolesToUser";
   const char ACTION_NAME_revokeRolesFromUser[] = "revokeRolesFromUser";
   const char ACTION_NAME_createDataSource[] = "createDataSource";
   const char ACTION_NAME_createDomain[] = "createDomain";
   const char ACTION_NAME_createProcedure[] = "createProcedure";
   const char ACTION_NAME_createRG[] = "createRG";
   const char ACTION_NAME_createSequence[] = "createSequence";
   const char ACTION_NAME_dropDataSource[] = "dropDataSource";
   const char ACTION_NAME_dropDomain[] = "dropDomain";
   const char ACTION_NAME_dropSequence[] = "dropSequence";
   const char ACTION_NAME_eval[] = "eval";
   const char ACTION_NAME_flushConfigure[] = "flushConfigure";
   const char ACTION_NAME_forceSession[] = "forceSession";
   const char ACTION_NAME_forceStepUp[] = "forceStepUp";
   const char ACTION_NAME_getDataSource[] = "getDataSource";
   const char ACTION_NAME_getDomain[] = "getDomain";
   const char ACTION_NAME_getRG[] = "getRG";
   const char ACTION_NAME_getSequence[] = "getSequence";
   const char ACTION_NAME_getTask[] = "getTask";
   const char ACTION_NAME_invalidateCache[] = "invalidateCache";
   const char ACTION_NAME_invalidateUserCache[] = "invalidateUserCache";
   const char ACTION_NAME_list[] = "list";
   const char ACTION_NAME_listCollectionSpaces[] = "listCollectionSpaces";
   const char ACTION_NAME_removeBackup[] = "removeBackup";
   const char ACTION_NAME_removeRG[] = "removeRG";
   const char ACTION_NAME_removeProcedure[] = "removeProcedure";
   const char ACTION_NAME_renameCS[] = "renameCS";
   const char ACTION_NAME_resetSnapshot[] = "resetSnapshot";
   const char ACTION_NAME_reloadConf[] = "reloadConf";
   const char ACTION_NAME_deleteConf[] = "deleteConf";
   const char ACTION_NAME_snapshot[] = "snapshot";
   const char ACTION_NAME_sync[] = "sync";
   const char ACTION_NAME_setPDLevel[] = "setPDLevel";
   const char ACTION_NAME_trace[] = "trace";
   const char ACTION_NAME_traceStatus[] = "traceStatus";
   const char ACTION_NAME_trans[] = "trans";
   const char ACTION_NAME_updateConf[] = "updateConf";
   const char ACTION_NAME_testCS[] = "testCS";
   const char ACTION_NAME_waitTasks[] = "waitTasks";
   const char ACTION_NAME_listProcedures[] = "listProcedures";
   const char ACTION_NAME_alterUser[] = "alterUser";
   const char ACTION_NAME_listBackup[] = "listBackup";
   const char ACTION_NAME_alterDataSource[] = "alterDataSource";
   const char ACTION_NAME_createNode[] = "createNode";
   const char ACTION_NAME_getNode[] = "getNode";
   const char ACTION_NAME_reelect[] = "reelect";
   const char ACTION_NAME_removeNode[] = "removeNode";
   const char ACTION_NAME_startRG[] = "startRG";
   const char ACTION_NAME_stopRG[] = "stopRG";
   const char ACTION_NAME_alterRG[] = "alterRG";
   const char ACTION_NAME_startNode[] = "startNode";
   const char ACTION_NAME_stopNode[] = "stopNode";
   const char ACTION_NAME_alterNode[] = "alterNode";
   const char ACTION_NAME_fetchSequence[] = "fetchSequence";
   const char ACTION_NAME_getSequenceCurrentValue[] = "getSequenceCurrentValue";
   const char ACTION_NAME_alterSequence[] = "alterSequence";
   const char ACTION_NAME_alterDomain[] = "alterDomain";
   const char ACTION_NAME_getDCInfo[] = "getDCInfo";
   const char ACTION_NAME_alterDC[] = "alterDC";

   ACTION_TYPE_ENUM authActionTypeParse( const char *actionName )
   {
      if (strcmp(actionName, ACTION_NAME__invalid) == 0)
      {
         return ACTION_TYPE__invalid;
      }
      if (strcmp(actionName, ACTION_NAME_alterCL) == 0)
      {
         return ACTION_TYPE_alterCL;
      }
      if (strcmp(actionName, ACTION_NAME_attachCL) == 0)
      {
         return ACTION_TYPE_attachCL;
      }
      if (strcmp(actionName, ACTION_NAME_copyIndex) == 0)
      {
         return ACTION_TYPE_copyIndex;
      }
      if (strcmp(actionName, ACTION_NAME_find) == 0)
      {
         return ACTION_TYPE_find;
      }
      if (strcmp(actionName, ACTION_NAME_insert) == 0)
      {
         return ACTION_TYPE_insert;
      }
      if (strcmp(actionName, ACTION_NAME_update) == 0)
      {
         return ACTION_TYPE_update;
      }
      if (strcmp(actionName, ACTION_NAME_remove) == 0)
      {
         return ACTION_TYPE_remove;
      }
      if (strcmp(actionName, ACTION_NAME_getDetail) == 0)
      {
         return ACTION_TYPE_getDetail;
      }
      if (strcmp(actionName, ACTION_NAME_createIndex) == 0)
      {
         return ACTION_TYPE_createIndex;
      }
      if (strcmp(actionName, ACTION_NAME_dropIndex) == 0)
      {
         return ACTION_TYPE_dropIndex;
      }
      if (strcmp(actionName, ACTION_NAME_detachCL) == 0)
      {
         return ACTION_TYPE_detachCL;
      }
      if (strcmp(actionName, ACTION_NAME_truncate) == 0)
      {
         return ACTION_TYPE_truncate;
      }
      if (strcmp(actionName, ACTION_NAME_split) == 0)
      {
         return ACTION_TYPE_split;
      }
      if (strcmp(actionName, ACTION_NAME_alterCS) == 0)
      {
         return ACTION_TYPE_alterCS;
      }
      if (strcmp(actionName, ACTION_NAME_createCL) == 0)
      {
         return ACTION_TYPE_createCL;
      }
      if (strcmp(actionName, ACTION_NAME_dropCL) == 0)
      {
         return ACTION_TYPE_dropCL;
      }
      if (strcmp(actionName, ACTION_NAME_renameCL) == 0)
      {
         return ACTION_TYPE_renameCL;
      }
      if (strcmp(actionName, ACTION_NAME_listCollections) == 0)
      {
         return ACTION_TYPE_listCollections;
      }
      if (strcmp(actionName, ACTION_NAME_testCL) == 0)
      {
         return ACTION_TYPE_testCL;
      }
      if (strcmp(actionName, ACTION_NAME_alterBin) == 0)
      {
         return ACTION_TYPE_alterBin;
      }
      if (strcmp(actionName, ACTION_NAME_countBin) == 0)
      {
         return ACTION_TYPE_countBin;
      }
      if (strcmp(actionName, ACTION_NAME_dropAllBin) == 0)
      {
         return ACTION_TYPE_dropAllBin;
      }
      if (strcmp(actionName, ACTION_NAME_dropItemBin) == 0)
      {
         return ACTION_TYPE_dropItemBin;
      }
      if (strcmp(actionName, ACTION_NAME_getDetailBin) == 0)
      {
         return ACTION_TYPE_getDetailBin;
      }
      if (strcmp(actionName, ACTION_NAME_listBin) == 0)
      {
         return ACTION_TYPE_listBin;
      }
      if (strcmp(actionName, ACTION_NAME_returnItemBin) == 0)
      {
         return ACTION_TYPE_returnItemBin;
      }
      if (strcmp(actionName, ACTION_NAME_snapshotBin) == 0)
      {
         return ACTION_TYPE_snapshotBin;
      }
      if (strcmp(actionName, ACTION_NAME_analyze) == 0)
      {
         return ACTION_TYPE_analyze;
      }
      if (strcmp(actionName, ACTION_NAME_backup) == 0)
      {
         return ACTION_TYPE_backup;
      }
      if (strcmp(actionName, ACTION_NAME_createCS) == 0)
      {
         return ACTION_TYPE_createCS;
      }
      if (strcmp(actionName, ACTION_NAME_dropCS) == 0)
      {
         return ACTION_TYPE_dropCS;
      }
      if (strcmp(actionName, ACTION_NAME_loadCS) == 0)
      {
         return ACTION_TYPE_loadCS;
      }
      if (strcmp(actionName, ACTION_NAME_unloadCS) == 0)
      {
         return ACTION_TYPE_unloadCS;
      }
      if (strcmp(actionName, ACTION_NAME_cancelTask) == 0)
      {
         return ACTION_TYPE_cancelTask;
      }
      if (strcmp(actionName, ACTION_NAME_createRole) == 0)
      {
         return ACTION_TYPE_createRole;
      }
      if (strcmp(actionName, ACTION_NAME_dropRole) == 0)
      {
         return ACTION_TYPE_dropRole;
      }
      if (strcmp(actionName, ACTION_NAME_getRole) == 0)
      {
         return ACTION_TYPE_getRole;
      }
      if (strcmp(actionName, ACTION_NAME_listRoles) == 0)
      {
         return ACTION_TYPE_listRoles;
      }
      if (strcmp(actionName, ACTION_NAME_updateRole) == 0)
      {
         return ACTION_TYPE_updateRole;
      }
      if (strcmp(actionName, ACTION_NAME_grantPrivilegesToRole) == 0)
      {
         return ACTION_TYPE_grantPrivilegesToRole;
      }
      if (strcmp(actionName, ACTION_NAME_revokePrivilegesFromRole) == 0)
      {
         return ACTION_TYPE_revokePrivilegesFromRole;
      }
      if (strcmp(actionName, ACTION_NAME_grantRolesToRole) == 0)
      {
         return ACTION_TYPE_grantRolesToRole;
      }
      if (strcmp(actionName, ACTION_NAME_revokeRolesFromRole) == 0)
      {
         return ACTION_TYPE_revokeRolesFromRole;
      }
      if (strcmp(actionName, ACTION_NAME_createUsr) == 0)
      {
         return ACTION_TYPE_createUsr;
      }
      if (strcmp(actionName, ACTION_NAME_dropUsr) == 0)
      {
         return ACTION_TYPE_dropUsr;
      }
      if (strcmp(actionName, ACTION_NAME_getUser) == 0)
      {
         return ACTION_TYPE_getUser;
      }
      if (strcmp(actionName, ACTION_NAME_grantRolesToUser) == 0)
      {
         return ACTION_TYPE_grantRolesToUser;
      }
      if (strcmp(actionName, ACTION_NAME_revokeRolesFromUser) == 0)
      {
         return ACTION_TYPE_revokeRolesFromUser;
      }
      if (strcmp(actionName, ACTION_NAME_createDataSource) == 0)
      {
         return ACTION_TYPE_createDataSource;
      }
      if (strcmp(actionName, ACTION_NAME_createDomain) == 0)
      {
         return ACTION_TYPE_createDomain;
      }
      if (strcmp(actionName, ACTION_NAME_createProcedure) == 0)
      {
         return ACTION_TYPE_createProcedure;
      }
      if (strcmp(actionName, ACTION_NAME_createRG) == 0)
      {
         return ACTION_TYPE_createRG;
      }
      if (strcmp(actionName, ACTION_NAME_createSequence) == 0)
      {
         return ACTION_TYPE_createSequence;
      }
      if (strcmp(actionName, ACTION_NAME_dropDataSource) == 0)
      {
         return ACTION_TYPE_dropDataSource;
      }
      if (strcmp(actionName, ACTION_NAME_dropDomain) == 0)
      {
         return ACTION_TYPE_dropDomain;
      }
      if (strcmp(actionName, ACTION_NAME_dropSequence) == 0)
      {
         return ACTION_TYPE_dropSequence;
      }
      if (strcmp(actionName, ACTION_NAME_eval) == 0)
      {
         return ACTION_TYPE_eval;
      }
      if (strcmp(actionName, ACTION_NAME_flushConfigure) == 0)
      {
         return ACTION_TYPE_flushConfigure;
      }
      if (strcmp(actionName, ACTION_NAME_forceSession) == 0)
      {
         return ACTION_TYPE_forceSession;
      }
      if (strcmp(actionName, ACTION_NAME_forceStepUp) == 0)
      {
         return ACTION_TYPE_forceStepUp;
      }
      if (strcmp(actionName, ACTION_NAME_getDataSource) == 0)
      {
         return ACTION_TYPE_getDataSource;
      }
      if (strcmp(actionName, ACTION_NAME_getDomain) == 0)
      {
         return ACTION_TYPE_getDomain;
      }
      if (strcmp(actionName, ACTION_NAME_getRG) == 0)
      {
         return ACTION_TYPE_getRG;
      }
      if (strcmp(actionName, ACTION_NAME_getSequence) == 0)
      {
         return ACTION_TYPE_getSequence;
      }
      if (strcmp(actionName, ACTION_NAME_getTask) == 0)
      {
         return ACTION_TYPE_getTask;
      }
      if (strcmp(actionName, ACTION_NAME_invalidateCache) == 0)
      {
         return ACTION_TYPE_invalidateCache;
      }
      if (strcmp(actionName, ACTION_NAME_invalidateUserCache) == 0)
      {
         return ACTION_TYPE_invalidateUserCache;
      }
      if (strcmp(actionName, ACTION_NAME_list) == 0)
      {
         return ACTION_TYPE_list;
      }
      if (strcmp(actionName, ACTION_NAME_listCollectionSpaces) == 0)
      {
         return ACTION_TYPE_listCollectionSpaces;
      }
      if (strcmp(actionName, ACTION_NAME_removeBackup) == 0)
      {
         return ACTION_TYPE_removeBackup;
      }
      if (strcmp(actionName, ACTION_NAME_removeRG) == 0)
      {
         return ACTION_TYPE_removeRG;
      }
      if (strcmp(actionName, ACTION_NAME_removeProcedure) == 0)
      {
         return ACTION_TYPE_removeProcedure;
      }
      if (strcmp(actionName, ACTION_NAME_renameCS) == 0)
      {
         return ACTION_TYPE_renameCS;
      }
      if (strcmp(actionName, ACTION_NAME_resetSnapshot) == 0)
      {
         return ACTION_TYPE_resetSnapshot;
      }
      if (strcmp(actionName, ACTION_NAME_reloadConf) == 0)
      {
         return ACTION_TYPE_reloadConf;
      }
      if (strcmp(actionName, ACTION_NAME_deleteConf) == 0)
      {
         return ACTION_TYPE_deleteConf;
      }
      if (strcmp(actionName, ACTION_NAME_snapshot) == 0)
      {
         return ACTION_TYPE_snapshot;
      }
      if (strcmp(actionName, ACTION_NAME_sync) == 0)
      {
         return ACTION_TYPE_sync;
      }
      if (strcmp(actionName, ACTION_NAME_setPDLevel) == 0)
      {
         return ACTION_TYPE_setPDLevel;
      }
      if (strcmp(actionName, ACTION_NAME_trace) == 0)
      {
         return ACTION_TYPE_trace;
      }
      if (strcmp(actionName, ACTION_NAME_traceStatus) == 0)
      {
         return ACTION_TYPE_traceStatus;
      }
      if (strcmp(actionName, ACTION_NAME_trans) == 0)
      {
         return ACTION_TYPE_trans;
      }
      if (strcmp(actionName, ACTION_NAME_updateConf) == 0)
      {
         return ACTION_TYPE_updateConf;
      }
      if (strcmp(actionName, ACTION_NAME_testCS) == 0)
      {
         return ACTION_TYPE_testCS;
      }
      if (strcmp(actionName, ACTION_NAME_waitTasks) == 0)
      {
         return ACTION_TYPE_waitTasks;
      }
      if (strcmp(actionName, ACTION_NAME_listProcedures) == 0)
      {
         return ACTION_TYPE_listProcedures;
      }
      if (strcmp(actionName, ACTION_NAME_alterUser) == 0)
      {
         return ACTION_TYPE_alterUser;
      }
      if (strcmp(actionName, ACTION_NAME_listBackup) == 0)
      {
         return ACTION_TYPE_listBackup;
      }
      if (strcmp(actionName, ACTION_NAME_alterDataSource) == 0)
      {
         return ACTION_TYPE_alterDataSource;
      }
      if (strcmp(actionName, ACTION_NAME_createNode) == 0)
      {
         return ACTION_TYPE_createNode;
      }
      if (strcmp(actionName, ACTION_NAME_getNode) == 0)
      {
         return ACTION_TYPE_getNode;
      }
      if (strcmp(actionName, ACTION_NAME_reelect) == 0)
      {
         return ACTION_TYPE_reelect;
      }
      if (strcmp(actionName, ACTION_NAME_removeNode) == 0)
      {
         return ACTION_TYPE_removeNode;
      }
      if (strcmp(actionName, ACTION_NAME_startRG) == 0)
      {
         return ACTION_TYPE_startRG;
      }
      if (strcmp(actionName, ACTION_NAME_stopRG) == 0)
      {
         return ACTION_TYPE_stopRG;
      }
      if (strcmp(actionName, ACTION_NAME_alterRG) == 0)
      {
         return ACTION_TYPE_alterRG;
      }
      if (strcmp(actionName, ACTION_NAME_startNode) == 0)
      {
         return ACTION_TYPE_startNode;
      }
      if (strcmp(actionName, ACTION_NAME_stopNode) == 0)
      {
         return ACTION_TYPE_stopNode;
      }
      if (strcmp(actionName, ACTION_NAME_alterNode) == 0)
      {
         return ACTION_TYPE_alterNode;
      }
      if (strcmp(actionName, ACTION_NAME_fetchSequence) == 0)
      {
         return ACTION_TYPE_fetchSequence;
      }
      if (strcmp(actionName, ACTION_NAME_getSequenceCurrentValue) == 0)
      {
         return ACTION_TYPE_getSequenceCurrentValue;
      }
      if (strcmp(actionName, ACTION_NAME_alterSequence) == 0)
      {
         return ACTION_TYPE_alterSequence;
      }
      if (strcmp(actionName, ACTION_NAME_alterDomain) == 0)
      {
         return ACTION_TYPE_alterDomain;
      }
      if (strcmp(actionName, ACTION_NAME_getDCInfo) == 0)
      {
         return ACTION_TYPE_getDCInfo;
      }
      if (strcmp(actionName, ACTION_NAME_alterDC) == 0)
      {
         return ACTION_TYPE_alterDC;
      }
      return ACTION_TYPE__invalid;
   }

   const char *authActionTypeSerializer( ACTION_TYPE_ENUM actionType )
   {
      if (actionType == ACTION_TYPE__invalid)
      {
         return ACTION_NAME__invalid;
      }
      if (actionType == ACTION_TYPE_alterCL)
      {
         return ACTION_NAME_alterCL;
      }
      if (actionType == ACTION_TYPE_attachCL)
      {
         return ACTION_NAME_attachCL;
      }
      if (actionType == ACTION_TYPE_copyIndex)
      {
         return ACTION_NAME_copyIndex;
      }
      if (actionType == ACTION_TYPE_find)
      {
         return ACTION_NAME_find;
      }
      if (actionType == ACTION_TYPE_insert)
      {
         return ACTION_NAME_insert;
      }
      if (actionType == ACTION_TYPE_update)
      {
         return ACTION_NAME_update;
      }
      if (actionType == ACTION_TYPE_remove)
      {
         return ACTION_NAME_remove;
      }
      if (actionType == ACTION_TYPE_getDetail)
      {
         return ACTION_NAME_getDetail;
      }
      if (actionType == ACTION_TYPE_createIndex)
      {
         return ACTION_NAME_createIndex;
      }
      if (actionType == ACTION_TYPE_dropIndex)
      {
         return ACTION_NAME_dropIndex;
      }
      if (actionType == ACTION_TYPE_detachCL)
      {
         return ACTION_NAME_detachCL;
      }
      if (actionType == ACTION_TYPE_truncate)
      {
         return ACTION_NAME_truncate;
      }
      if (actionType == ACTION_TYPE_split)
      {
         return ACTION_NAME_split;
      }
      if (actionType == ACTION_TYPE_alterCS)
      {
         return ACTION_NAME_alterCS;
      }
      if (actionType == ACTION_TYPE_createCL)
      {
         return ACTION_NAME_createCL;
      }
      if (actionType == ACTION_TYPE_dropCL)
      {
         return ACTION_NAME_dropCL;
      }
      if (actionType == ACTION_TYPE_renameCL)
      {
         return ACTION_NAME_renameCL;
      }
      if (actionType == ACTION_TYPE_listCollections)
      {
         return ACTION_NAME_listCollections;
      }
      if (actionType == ACTION_TYPE_testCL)
      {
         return ACTION_NAME_testCL;
      }
      if (actionType == ACTION_TYPE_alterBin)
      {
         return ACTION_NAME_alterBin;
      }
      if (actionType == ACTION_TYPE_countBin)
      {
         return ACTION_NAME_countBin;
      }
      if (actionType == ACTION_TYPE_dropAllBin)
      {
         return ACTION_NAME_dropAllBin;
      }
      if (actionType == ACTION_TYPE_dropItemBin)
      {
         return ACTION_NAME_dropItemBin;
      }
      if (actionType == ACTION_TYPE_getDetailBin)
      {
         return ACTION_NAME_getDetailBin;
      }
      if (actionType == ACTION_TYPE_listBin)
      {
         return ACTION_NAME_listBin;
      }
      if (actionType == ACTION_TYPE_returnItemBin)
      {
         return ACTION_NAME_returnItemBin;
      }
      if (actionType == ACTION_TYPE_snapshotBin)
      {
         return ACTION_NAME_snapshotBin;
      }
      if (actionType == ACTION_TYPE_analyze)
      {
         return ACTION_NAME_analyze;
      }
      if (actionType == ACTION_TYPE_backup)
      {
         return ACTION_NAME_backup;
      }
      if (actionType == ACTION_TYPE_createCS)
      {
         return ACTION_NAME_createCS;
      }
      if (actionType == ACTION_TYPE_dropCS)
      {
         return ACTION_NAME_dropCS;
      }
      if (actionType == ACTION_TYPE_loadCS)
      {
         return ACTION_NAME_loadCS;
      }
      if (actionType == ACTION_TYPE_unloadCS)
      {
         return ACTION_NAME_unloadCS;
      }
      if (actionType == ACTION_TYPE_cancelTask)
      {
         return ACTION_NAME_cancelTask;
      }
      if (actionType == ACTION_TYPE_createRole)
      {
         return ACTION_NAME_createRole;
      }
      if (actionType == ACTION_TYPE_dropRole)
      {
         return ACTION_NAME_dropRole;
      }
      if (actionType == ACTION_TYPE_getRole)
      {
         return ACTION_NAME_getRole;
      }
      if (actionType == ACTION_TYPE_listRoles)
      {
         return ACTION_NAME_listRoles;
      }
      if (actionType == ACTION_TYPE_updateRole)
      {
         return ACTION_NAME_updateRole;
      }
      if (actionType == ACTION_TYPE_grantPrivilegesToRole)
      {
         return ACTION_NAME_grantPrivilegesToRole;
      }
      if (actionType == ACTION_TYPE_revokePrivilegesFromRole)
      {
         return ACTION_NAME_revokePrivilegesFromRole;
      }
      if (actionType == ACTION_TYPE_grantRolesToRole)
      {
         return ACTION_NAME_grantRolesToRole;
      }
      if (actionType == ACTION_TYPE_revokeRolesFromRole)
      {
         return ACTION_NAME_revokeRolesFromRole;
      }
      if (actionType == ACTION_TYPE_createUsr)
      {
         return ACTION_NAME_createUsr;
      }
      if (actionType == ACTION_TYPE_dropUsr)
      {
         return ACTION_NAME_dropUsr;
      }
      if (actionType == ACTION_TYPE_getUser)
      {
         return ACTION_NAME_getUser;
      }
      if (actionType == ACTION_TYPE_grantRolesToUser)
      {
         return ACTION_NAME_grantRolesToUser;
      }
      if (actionType == ACTION_TYPE_revokeRolesFromUser)
      {
         return ACTION_NAME_revokeRolesFromUser;
      }
      if (actionType == ACTION_TYPE_createDataSource)
      {
         return ACTION_NAME_createDataSource;
      }
      if (actionType == ACTION_TYPE_createDomain)
      {
         return ACTION_NAME_createDomain;
      }
      if (actionType == ACTION_TYPE_createProcedure)
      {
         return ACTION_NAME_createProcedure;
      }
      if (actionType == ACTION_TYPE_createRG)
      {
         return ACTION_NAME_createRG;
      }
      if (actionType == ACTION_TYPE_createSequence)
      {
         return ACTION_NAME_createSequence;
      }
      if (actionType == ACTION_TYPE_dropDataSource)
      {
         return ACTION_NAME_dropDataSource;
      }
      if (actionType == ACTION_TYPE_dropDomain)
      {
         return ACTION_NAME_dropDomain;
      }
      if (actionType == ACTION_TYPE_dropSequence)
      {
         return ACTION_NAME_dropSequence;
      }
      if (actionType == ACTION_TYPE_eval)
      {
         return ACTION_NAME_eval;
      }
      if (actionType == ACTION_TYPE_flushConfigure)
      {
         return ACTION_NAME_flushConfigure;
      }
      if (actionType == ACTION_TYPE_forceSession)
      {
         return ACTION_NAME_forceSession;
      }
      if (actionType == ACTION_TYPE_forceStepUp)
      {
         return ACTION_NAME_forceStepUp;
      }
      if (actionType == ACTION_TYPE_getDataSource)
      {
         return ACTION_NAME_getDataSource;
      }
      if (actionType == ACTION_TYPE_getDomain)
      {
         return ACTION_NAME_getDomain;
      }
      if (actionType == ACTION_TYPE_getRG)
      {
         return ACTION_NAME_getRG;
      }
      if (actionType == ACTION_TYPE_getSequence)
      {
         return ACTION_NAME_getSequence;
      }
      if (actionType == ACTION_TYPE_getTask)
      {
         return ACTION_NAME_getTask;
      }
      if (actionType == ACTION_TYPE_invalidateCache)
      {
         return ACTION_NAME_invalidateCache;
      }
      if (actionType == ACTION_TYPE_invalidateUserCache)
      {
         return ACTION_NAME_invalidateUserCache;
      }
      if (actionType == ACTION_TYPE_list)
      {
         return ACTION_NAME_list;
      }
      if (actionType == ACTION_TYPE_listCollectionSpaces)
      {
         return ACTION_NAME_listCollectionSpaces;
      }
      if (actionType == ACTION_TYPE_removeBackup)
      {
         return ACTION_NAME_removeBackup;
      }
      if (actionType == ACTION_TYPE_removeRG)
      {
         return ACTION_NAME_removeRG;
      }
      if (actionType == ACTION_TYPE_removeProcedure)
      {
         return ACTION_NAME_removeProcedure;
      }
      if (actionType == ACTION_TYPE_renameCS)
      {
         return ACTION_NAME_renameCS;
      }
      if (actionType == ACTION_TYPE_resetSnapshot)
      {
         return ACTION_NAME_resetSnapshot;
      }
      if (actionType == ACTION_TYPE_reloadConf)
      {
         return ACTION_NAME_reloadConf;
      }
      if (actionType == ACTION_TYPE_deleteConf)
      {
         return ACTION_NAME_deleteConf;
      }
      if (actionType == ACTION_TYPE_snapshot)
      {
         return ACTION_NAME_snapshot;
      }
      if (actionType == ACTION_TYPE_sync)
      {
         return ACTION_NAME_sync;
      }
      if (actionType == ACTION_TYPE_setPDLevel)
      {
         return ACTION_NAME_setPDLevel;
      }
      if (actionType == ACTION_TYPE_trace)
      {
         return ACTION_NAME_trace;
      }
      if (actionType == ACTION_TYPE_traceStatus)
      {
         return ACTION_NAME_traceStatus;
      }
      if (actionType == ACTION_TYPE_trans)
      {
         return ACTION_NAME_trans;
      }
      if (actionType == ACTION_TYPE_updateConf)
      {
         return ACTION_NAME_updateConf;
      }
      if (actionType == ACTION_TYPE_testCS)
      {
         return ACTION_NAME_testCS;
      }
      if (actionType == ACTION_TYPE_waitTasks)
      {
         return ACTION_NAME_waitTasks;
      }
      if (actionType == ACTION_TYPE_listProcedures)
      {
         return ACTION_NAME_listProcedures;
      }
      if (actionType == ACTION_TYPE_alterUser)
      {
         return ACTION_NAME_alterUser;
      }
      if (actionType == ACTION_TYPE_listBackup)
      {
         return ACTION_NAME_listBackup;
      }
      if (actionType == ACTION_TYPE_alterDataSource)
      {
         return ACTION_NAME_alterDataSource;
      }
      if (actionType == ACTION_TYPE_createNode)
      {
         return ACTION_NAME_createNode;
      }
      if (actionType == ACTION_TYPE_getNode)
      {
         return ACTION_NAME_getNode;
      }
      if (actionType == ACTION_TYPE_reelect)
      {
         return ACTION_NAME_reelect;
      }
      if (actionType == ACTION_TYPE_removeNode)
      {
         return ACTION_NAME_removeNode;
      }
      if (actionType == ACTION_TYPE_startRG)
      {
         return ACTION_NAME_startRG;
      }
      if (actionType == ACTION_TYPE_stopRG)
      {
         return ACTION_NAME_stopRG;
      }
      if (actionType == ACTION_TYPE_alterRG)
      {
         return ACTION_NAME_alterRG;
      }
      if (actionType == ACTION_TYPE_startNode)
      {
         return ACTION_NAME_startNode;
      }
      if (actionType == ACTION_TYPE_stopNode)
      {
         return ACTION_NAME_stopNode;
      }
      if (actionType == ACTION_TYPE_alterNode)
      {
         return ACTION_NAME_alterNode;
      }
      if (actionType == ACTION_TYPE_fetchSequence)
      {
         return ACTION_NAME_fetchSequence;
      }
      if (actionType == ACTION_TYPE_getSequenceCurrentValue)
      {
         return ACTION_NAME_getSequenceCurrentValue;
      }
      if (actionType == ACTION_TYPE_alterSequence)
      {
         return ACTION_NAME_alterSequence;
      }
      if (actionType == ACTION_TYPE_alterDomain)
      {
         return ACTION_NAME_alterDomain;
      }
      if (actionType == ACTION_TYPE_getDCInfo)
      {
         return ACTION_NAME_getDCInfo;
      }
      if (actionType == ACTION_TYPE_alterDC)
      {
         return ACTION_NAME_alterDC;
      }
      return ACTION_NAME__invalid;
   }

   const char RESOURCE_NAME__INVALID[] = "_Invalid";
   const char RESOURCE_NAME_CLUSTER[] = "Cluster";
   const char RESOURCE_NAME_COLLECTION_NAME[] = "CollectionName";
   const char RESOURCE_NAME_COLLECTION_SPACE[] = "ExactCollectionSpace";
   const char RESOURCE_NAME_EXACT_COLLECTION[] = "ExactCollection";
   const char RESOURCE_NAME_NON_SYSTEM[] = "NonSystem";
   const char RESOURCE_NAME_ANY[] = "AnyResource";

   static const authActionSet AUTH_CMD_NAME_BACKUP_OFFLINE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      268435456ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_BACKUP_OFFLINE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_BACKUP_OFFLINE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_COLLECTION_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      16384ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_COLLECTION_default_SETS( RESOURCE_TYPE_COLLECTION_SPACE , AUTH_CMD_NAME_CREATE_COLLECTION_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      536870912ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_SEQUENCE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4503599627370496ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_SEQUENCE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_SEQUENCE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_SEQUENCE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      36028797018963968ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_SEQUENCE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_SEQUENCE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_SEQUENCE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      137438953472ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_SEQUENCE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_SEQUENCE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      68719476736ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_INDEX_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      256ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_INDEX_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_CREATE_INDEX_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_CANCEL_TASK_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8589934592ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CANCEL_TASK_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CANCEL_TASK_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_COLLECTION_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      32768ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_COLLECTION_default_SETS( RESOURCE_TYPE_COLLECTION_SPACE , AUTH_CMD_NAME_DROP_COLLECTION_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1073741824ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2147483648ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4294967296ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_INDEX_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      512ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_INDEX_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_DROP_INDEX_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_COPY_INDEX_maincl_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_COPY_INDEX_maincl_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_COPY_INDEX_maincl_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_GET_COUNT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_COUNT_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_GET_COUNT_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_GET_INDEXES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_INDEXES_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_GET_INDEXES_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_GET_QUERYMETA_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_QUERYMETA_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_GET_QUERYMETA_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_GET_DCINFO_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      549755813888ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_DCINFO_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GET_DCINFO_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      16ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_CONTEXTS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_CONTEXTS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_CONTEXTS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SESSIONS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SESSIONS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SESSIONS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_STORAGEUNITS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_STORAGEUNITS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_STORAGEUNITS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_GROUPS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4611686018427387904ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      67108864ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_GROUPS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_GROUPS_default_ACTION_SETS_ARRAY, 3, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_DOMAINS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2305843009213693952ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_DOMAINS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_DOMAINS_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      16ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_USERS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_USERS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_USERS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_BACKUPS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8388608ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_BACKUPS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_BACKUPS_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_TASKS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      1ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_TASKS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_TASKS_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_TRANSACTIONS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_TRANSACTIONS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_TRANSACTIONS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SVCTASKS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SVCTASKS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SVCTASKS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SEQUENCES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      9223372036854775808ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SEQUENCES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SEQUENCES_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_DATASOURCES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1152921504606846976ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_DATASOURCES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_DATASOURCES_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_RECYCLEBIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      16777216ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_RECYCLEBIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_RECYCLEBIN_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_RENAME_COLLECTION_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      65536ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_RENAME_COLLECTION_default_SETS( RESOURCE_TYPE_COLLECTION_SPACE , AUTH_CMD_NAME_RENAME_COLLECTION_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_COLLECTIONSPACE
   );

   static const authActionSet AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      256ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_DATABASE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_DATABASE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_DATABASE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_RESET_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      512ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_RESET_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_RESET_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CATA_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CATA_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CATA_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_HEALTH_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_HEALTH_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_HEALTH_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_QUERIES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_QUERIES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_QUERIES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TASKS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TASKS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TASKS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_INDEXES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_INDEXES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_INDEXES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      67108864ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_TEST_COLLECTION_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      262144ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TEST_COLLECTION_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_TEST_COLLECTION_default_ACTION_SETS_ARRAY, 3, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      524288ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default_SETS( RESOURCE_TYPE_COLLECTION_SPACE , AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default_ACTION_SETS_ARRAY, 3, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_GROUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2251799813685248ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_GROUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_GROUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REMOVE_GROUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      64ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REMOVE_GROUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REMOVE_GROUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_NODE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      33554432ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_NODE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_NODE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REMOVE_NODE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      268435456ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REMOVE_NODE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REMOVE_NODE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REMOVE_BACKUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      32ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REMOVE_BACKUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REMOVE_BACKUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ACTIVE_GROUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      536870912ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ACTIVE_GROUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ACTIVE_GROUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_STARTUP_NODE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4294967296ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_STARTUP_NODE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_STARTUP_NODE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SHUTDOWN_NODE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8589934592ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SHUTDOWN_NODE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SHUTDOWN_NODE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SHUTDOWN_GROUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      1073741824ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SHUTDOWN_GROUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SHUTDOWN_GROUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SET_PDLEVEL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      16384ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SET_PDLEVEL_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SET_PDLEVEL_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SPLIT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4096ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SPLIT_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_SPLIT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_WAITTASK_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      1048576ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_WAITTASK_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_WAITTASK_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_CATA_GROUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2251799813685248ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_CATA_GROUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_CATA_GROUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_TRACE_START_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      32768ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TRACE_START_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_TRACE_START_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_TRACE_RESUME_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      32768ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TRACE_RESUME_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_TRACE_RESUME_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_TRACE_STOP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      32768ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TRACE_STOP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_TRACE_STOP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_TRACE_STATUS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      65536ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TRACE_STATUS_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_TRACE_STATUS_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_DOMAIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      562949953421312ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_DOMAIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_DOMAIN_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_DOMAIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      18014398509481984ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_DOMAIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_DOMAIN_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_EXPORT_CONFIG_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      144115188075855872ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_EXPORT_CONFIG_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_EXPORT_CONFIG_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CRT_PROCEDURE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1125899906842624ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CRT_PROCEDURE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CRT_PROCEDURE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_RM_PROCEDURE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      128ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_RM_PROCEDURE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_RM_PROCEDURE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_PROCEDURES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      2097152ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_PROCEDURES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_PROCEDURES_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_EVAL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      72057594037927936ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_EVAL_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_EVAL_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LINK_CL_maincl_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      10ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LINK_CL_maincl_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_LINK_CL_maincl_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_LINK_CL_subcl_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      120ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LINK_CL_subcl_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_LINK_CL_subcl_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_SUBCLNAME
   );

   static const authActionSet AUTH_CMD_NAME_UNLINK_CL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1024ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_UNLINK_CL_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_UNLINK_CL_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_INVALIDATE_CACHE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      2ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_INVALIDATE_CACHE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_INVALIDATE_CACHE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      2ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      2ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_FORCE_SESSION_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      288230376151711744ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_FORCE_SESSION_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_FORCE_SESSION_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_LOBS_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_LOBS_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_LIST_LOBS_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_DC_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      1099511627776ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_DC_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_DC_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_USR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4194304ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_USR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_USR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REELECT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      134217728ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REELECT_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REELECT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_FORCE_STEP_UP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      576460752303423488ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_FORCE_STEP_UP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_FORCE_STEP_UP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_TRUNCATE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2048ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_TRUNCATE_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_TRUNCATE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_SYNC_DB_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8192ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SYNC_DB_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SYNC_DB_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_POP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      64ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_POP_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_POP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_RELOAD_CONFIG_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      1024ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_RELOAD_CONFIG_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_RELOAD_CONFIG_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_UPDATE_CONFIG_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      262144ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_UPDATE_CONFIG_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_UPDATE_CONFIG_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DELETE_CONFIG_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      2048ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DELETE_CONFIG_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DELETE_CONFIG_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GET_CL_DETAIL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_CL_DETAIL_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_GET_CL_DETAIL_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_GET_CL_STAT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_CL_STAT_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_GET_CL_STAT_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_GET_INDEX_STAT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8ULL,
      0ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      128ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_INDEX_STAT_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_GET_INDEX_STAT_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_HINT, FIELD_NAME_COLLECTION
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_DATASOURCE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      281474976710656ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_DATASOURCE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_DATASOURCE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_DATASOURCE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      9007199254740992ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_DATASOURCE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_DATASOURCE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_DATASOURCE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      16777216ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_DATASOURCE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_DATASOURCE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8388608ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1048576ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_RECYCLEBIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      524288ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_RECYCLEBIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_RECYCLEBIN_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4194304ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2097152ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      33554432ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      33554432ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4096ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_COLLECTION_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_COLLECTION_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_COLLECTION_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SPACE_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      16ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SPACE_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SPACE_INTR_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_CONTEXT_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_CONTEXT_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_CONTEXT_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SESSION_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SESSION_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SESSION_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_BACKUP_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_BACKUP_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_BACKUP_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_TRANS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_TRANS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_TRANS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_GROUP_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_GROUP_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_GROUP_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_USER_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_USER_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_USER_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_TASK_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_TASK_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_TASK_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_INDEXES_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_INDEXES_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_INDEXES_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_DOMAIN_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2305843009213693952ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_DOMAIN_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_DOMAIN_INTR_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      9223372036854775808ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1152921504606846976ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default_ACTION_SETS_ARRAY, 2, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      8ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_COLLECTION_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_COLLECTION_default_SETS( RESOURCE_TYPE_EXACT_COLLECTION , AUTH_CMD_NAME_ALTER_COLLECTION_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      8192ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default_SETS( RESOURCE_TYPE_COLLECTION_SPACE , AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_QUERY, FIELD_NAME_NAME
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_DOMAIN_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      274877906944ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_DOMAIN_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_DOMAIN_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_NODE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      17179869184ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_NODE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_NODE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_ALTER_GROUP_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      2147483648ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_ALTER_GROUP_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_ALTER_GROUP_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_CREATE_ROLE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      17179869184ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_CREATE_ROLE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_CREATE_ROLE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_DROP_ROLE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      34359738368ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_DROP_ROLE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_DROP_ROLE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GET_ROLE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      68719476736ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_ROLE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GET_ROLE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_LIST_ROLES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      137438953472ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_LIST_ROLES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_LIST_ROLES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_UPDATE_ROLE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      274877906944ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_UPDATE_ROLE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_UPDATE_ROLE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GRANT_PRIVILEGES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      549755813888ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GRANT_PRIVILEGES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GRANT_PRIVILEGES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REVOKE_PRIVILEGES_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      1099511627776ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REVOKE_PRIVILEGES_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REVOKE_PRIVILEGES_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      2199023255552ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      4398046511104ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GET_USER_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      35184372088832ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GET_USER_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GET_USER_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      70368744177664ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      140737488355328ULL,
      0ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );

   static const authActionSet AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default_ACTION_SETS_ARRAY[] = {
      authActionSet( ACTION_SET_NUMBER_ARRAY(
      0ULL,
      4ULL
      )),
   };
   static const authRequiredActionSets AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default_SETS( RESOURCE_TYPE_CLUSTER , AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default_ACTION_SETS_ARRAY, 1, 
   authRequiredActionSets::SOURCE_OBJ_NONE, NULL
   );



   const authRequiredActionSets* authGetCMDActionSetsByTag( AUTH_CMD_ACTION_SETS_TAG tag )
   {
      if (tag == AUTH_CMD_NAME_BACKUP_OFFLINE_default)
      {
         return &AUTH_CMD_NAME_BACKUP_OFFLINE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_COLLECTION_default)
      {
         return &AUTH_CMD_NAME_CREATE_COLLECTION_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default)
      {
         return &AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_SEQUENCE_default)
      {
         return &AUTH_CMD_NAME_CREATE_SEQUENCE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_SEQUENCE_default)
      {
         return &AUTH_CMD_NAME_DROP_SEQUENCE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_SEQUENCE_default)
      {
         return &AUTH_CMD_NAME_ALTER_SEQUENCE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default)
      {
         return &AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_INDEX_default)
      {
         return &AUTH_CMD_NAME_CREATE_INDEX_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CANCEL_TASK_default)
      {
         return &AUTH_CMD_NAME_CANCEL_TASK_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_COLLECTION_default)
      {
         return &AUTH_CMD_NAME_DROP_COLLECTION_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default)
      {
         return &AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default)
      {
         return &AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default)
      {
         return &AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_INDEX_default)
      {
         return &AUTH_CMD_NAME_DROP_INDEX_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_COPY_INDEX_maincl)
      {
         return &AUTH_CMD_NAME_COPY_INDEX_maincl_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_COUNT_default)
      {
         return &AUTH_CMD_NAME_GET_COUNT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_INDEXES_default)
      {
         return &AUTH_CMD_NAME_GET_INDEXES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_QUERYMETA_default)
      {
         return &AUTH_CMD_NAME_GET_QUERYMETA_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_DCINFO_default)
      {
         return &AUTH_CMD_NAME_GET_DCINFO_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default)
      {
         return &AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_CONTEXTS_default)
      {
         return &AUTH_CMD_NAME_LIST_CONTEXTS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default)
      {
         return &AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SESSIONS_default)
      {
         return &AUTH_CMD_NAME_LIST_SESSIONS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default)
      {
         return &AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_STORAGEUNITS_default)
      {
         return &AUTH_CMD_NAME_LIST_STORAGEUNITS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_GROUPS_default)
      {
         return &AUTH_CMD_NAME_LIST_GROUPS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_DOMAINS_default)
      {
         return &AUTH_CMD_NAME_LIST_DOMAINS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default)
      {
         return &AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default)
      {
         return &AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_USERS_default)
      {
         return &AUTH_CMD_NAME_LIST_USERS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_BACKUPS_default)
      {
         return &AUTH_CMD_NAME_LIST_BACKUPS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_TASKS_default)
      {
         return &AUTH_CMD_NAME_LIST_TASKS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_TRANSACTIONS_default)
      {
         return &AUTH_CMD_NAME_LIST_TRANSACTIONS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default)
      {
         return &AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SVCTASKS_default)
      {
         return &AUTH_CMD_NAME_LIST_SVCTASKS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SEQUENCES_default)
      {
         return &AUTH_CMD_NAME_LIST_SEQUENCES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_DATASOURCES_default)
      {
         return &AUTH_CMD_NAME_LIST_DATASOURCES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_RECYCLEBIN_default)
      {
         return &AUTH_CMD_NAME_LIST_RECYCLEBIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_RENAME_COLLECTION_default)
      {
         return &AUTH_CMD_NAME_RENAME_COLLECTION_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default)
      {
         return &AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_DATABASE_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_DATABASE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_RESET_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_RESET_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CATA_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CATA_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_HEALTH_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_HEALTH_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_QUERIES_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_QUERIES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TASKS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TASKS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_INDEXES_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_INDEXES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TEST_COLLECTION_default)
      {
         return &AUTH_CMD_NAME_TEST_COLLECTION_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default)
      {
         return &AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_GROUP_default)
      {
         return &AUTH_CMD_NAME_CREATE_GROUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REMOVE_GROUP_default)
      {
         return &AUTH_CMD_NAME_REMOVE_GROUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_NODE_default)
      {
         return &AUTH_CMD_NAME_CREATE_NODE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REMOVE_NODE_default)
      {
         return &AUTH_CMD_NAME_REMOVE_NODE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REMOVE_BACKUP_default)
      {
         return &AUTH_CMD_NAME_REMOVE_BACKUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ACTIVE_GROUP_default)
      {
         return &AUTH_CMD_NAME_ACTIVE_GROUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_STARTUP_NODE_default)
      {
         return &AUTH_CMD_NAME_STARTUP_NODE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SHUTDOWN_NODE_default)
      {
         return &AUTH_CMD_NAME_SHUTDOWN_NODE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SHUTDOWN_GROUP_default)
      {
         return &AUTH_CMD_NAME_SHUTDOWN_GROUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SET_PDLEVEL_default)
      {
         return &AUTH_CMD_NAME_SET_PDLEVEL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SPLIT_default)
      {
         return &AUTH_CMD_NAME_SPLIT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_WAITTASK_default)
      {
         return &AUTH_CMD_NAME_WAITTASK_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_CATA_GROUP_default)
      {
         return &AUTH_CMD_NAME_CREATE_CATA_GROUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TRACE_START_default)
      {
         return &AUTH_CMD_NAME_TRACE_START_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TRACE_RESUME_default)
      {
         return &AUTH_CMD_NAME_TRACE_RESUME_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TRACE_STOP_default)
      {
         return &AUTH_CMD_NAME_TRACE_STOP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TRACE_STATUS_default)
      {
         return &AUTH_CMD_NAME_TRACE_STATUS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_DOMAIN_default)
      {
         return &AUTH_CMD_NAME_CREATE_DOMAIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_DOMAIN_default)
      {
         return &AUTH_CMD_NAME_DROP_DOMAIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_EXPORT_CONFIG_default)
      {
         return &AUTH_CMD_NAME_EXPORT_CONFIG_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CRT_PROCEDURE_default)
      {
         return &AUTH_CMD_NAME_CRT_PROCEDURE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_RM_PROCEDURE_default)
      {
         return &AUTH_CMD_NAME_RM_PROCEDURE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_PROCEDURES_default)
      {
         return &AUTH_CMD_NAME_LIST_PROCEDURES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_EVAL_default)
      {
         return &AUTH_CMD_NAME_EVAL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LINK_CL_maincl)
      {
         return &AUTH_CMD_NAME_LINK_CL_maincl_SETS;
      }
      if (tag == AUTH_CMD_NAME_LINK_CL_subcl)
      {
         return &AUTH_CMD_NAME_LINK_CL_subcl_SETS;
      }
      if (tag == AUTH_CMD_NAME_UNLINK_CL_default)
      {
         return &AUTH_CMD_NAME_UNLINK_CL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_INVALIDATE_CACHE_default)
      {
         return &AUTH_CMD_NAME_INVALIDATE_CACHE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default)
      {
         return &AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default)
      {
         return &AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_FORCE_SESSION_default)
      {
         return &AUTH_CMD_NAME_FORCE_SESSION_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_LOBS_default)
      {
         return &AUTH_CMD_NAME_LIST_LOBS_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_DC_default)
      {
         return &AUTH_CMD_NAME_ALTER_DC_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_USR_default)
      {
         return &AUTH_CMD_NAME_ALTER_USR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REELECT_default)
      {
         return &AUTH_CMD_NAME_REELECT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_FORCE_STEP_UP_default)
      {
         return &AUTH_CMD_NAME_FORCE_STEP_UP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_TRUNCATE_default)
      {
         return &AUTH_CMD_NAME_TRUNCATE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SYNC_DB_default)
      {
         return &AUTH_CMD_NAME_SYNC_DB_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_POP_default)
      {
         return &AUTH_CMD_NAME_POP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_RELOAD_CONFIG_default)
      {
         return &AUTH_CMD_NAME_RELOAD_CONFIG_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_UPDATE_CONFIG_default)
      {
         return &AUTH_CMD_NAME_UPDATE_CONFIG_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DELETE_CONFIG_default)
      {
         return &AUTH_CMD_NAME_DELETE_CONFIG_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_CL_DETAIL_default)
      {
         return &AUTH_CMD_NAME_GET_CL_DETAIL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_CL_STAT_default)
      {
         return &AUTH_CMD_NAME_GET_CL_STAT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_INDEX_STAT_default)
      {
         return &AUTH_CMD_NAME_GET_INDEX_STAT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_DATASOURCE_default)
      {
         return &AUTH_CMD_NAME_CREATE_DATASOURCE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_DATASOURCE_default)
      {
         return &AUTH_CMD_NAME_DROP_DATASOURCE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_DATASOURCE_default)
      {
         return &AUTH_CMD_NAME_ALTER_DATASOURCE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default)
      {
         return &AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default)
      {
         return &AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_RECYCLEBIN_default)
      {
         return &AUTH_CMD_NAME_ALTER_RECYCLEBIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default)
      {
         return &AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default)
      {
         return &AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default)
      {
         return &AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default)
      {
         return &AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default)
      {
         return &AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_COLLECTION_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_COLLECTION_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SPACE_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_SPACE_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_CONTEXT_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_CONTEXT_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SESSION_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_SESSION_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_BACKUP_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_BACKUP_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_TRANS_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_TRANS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_GROUP_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_GROUP_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_USER_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_USER_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_TASK_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_TASK_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_INDEXES_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_INDEXES_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_DOMAIN_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_DOMAIN_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default)
      {
         return &AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_COLLECTION_default)
      {
         return &AUTH_CMD_NAME_ALTER_COLLECTION_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default)
      {
         return &AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_DOMAIN_default)
      {
         return &AUTH_CMD_NAME_ALTER_DOMAIN_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_NODE_default)
      {
         return &AUTH_CMD_NAME_ALTER_NODE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_ALTER_GROUP_default)
      {
         return &AUTH_CMD_NAME_ALTER_GROUP_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_CREATE_ROLE_default)
      {
         return &AUTH_CMD_NAME_CREATE_ROLE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_DROP_ROLE_default)
      {
         return &AUTH_CMD_NAME_DROP_ROLE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_ROLE_default)
      {
         return &AUTH_CMD_NAME_GET_ROLE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_LIST_ROLES_default)
      {
         return &AUTH_CMD_NAME_LIST_ROLES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_UPDATE_ROLE_default)
      {
         return &AUTH_CMD_NAME_UPDATE_ROLE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GRANT_PRIVILEGES_default)
      {
         return &AUTH_CMD_NAME_GRANT_PRIVILEGES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REVOKE_PRIVILEGES_default)
      {
         return &AUTH_CMD_NAME_REVOKE_PRIVILEGES_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default)
      {
         return &AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default)
      {
         return &AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GET_USER_default)
      {
         return &AUTH_CMD_NAME_GET_USER_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default)
      {
         return &AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default)
      {
         return &AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default_SETS;
      }
      if (tag == AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default)
      {
         return &AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default_SETS;
      }
      return NULL;
   }

   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_BACKUP_OFFLINE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_BACKUP_OFFLINE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_BACKUP_OFFLINE_TAGS( CMD_NAME_BACKUP_OFFLINE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_COLLECTION_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_CREATE_COLLECTION_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_COLLECTION_TAGS( CMD_NAME_CREATE_COLLECTION_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_COLLECTIONSPACE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_COLLECTIONSPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_COLLECTIONSPACE_TAGS( CMD_NAME_CREATE_COLLECTIONSPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_SEQUENCE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_SEQUENCE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_SEQUENCE_TAGS( CMD_NAME_CREATE_SEQUENCE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_SEQUENCE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_SEQUENCE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_SEQUENCE_TAGS( CMD_NAME_DROP_SEQUENCE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_SEQUENCE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_SEQUENCE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_SEQUENCE_TAGS( CMD_NAME_ALTER_SEQUENCE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_SEQ_CURR_VAL_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GET_SEQ_CURR_VAL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_SEQ_CURR_VAL_TAGS( CMD_NAME_GET_SEQ_CURR_VAL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_INDEX_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_CREATE_INDEX_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_INDEX_TAGS( CMD_NAME_CREATE_INDEX_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CANCEL_TASK_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CANCEL_TASK_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CANCEL_TASK_TAGS( CMD_NAME_CANCEL_TASK_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_COLLECTION_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_DROP_COLLECTION_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_COLLECTION_TAGS( CMD_NAME_DROP_COLLECTION_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_COLLECTIONSPACE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_COLLECTIONSPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_COLLECTIONSPACE_TAGS( CMD_NAME_DROP_COLLECTIONSPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LOAD_COLLECTIONSPACE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LOAD_COLLECTIONSPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LOAD_COLLECTIONSPACE_TAGS( CMD_NAME_LOAD_COLLECTIONSPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_UNLOAD_COLLECTIONSPACE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_UNLOAD_COLLECTIONSPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_UNLOAD_COLLECTIONSPACE_TAGS( CMD_NAME_UNLOAD_COLLECTIONSPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_INDEX_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_DROP_INDEX_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_INDEX_TAGS( CMD_NAME_DROP_INDEX_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_COPY_INDEX_TAGS_ARRAY[] = {
            AUTH_CMD_NAME_COPY_INDEX_maincl,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_COPY_INDEX_TAGS( CMD_NAME_COPY_INDEX_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_COUNT_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_GET_COUNT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_COUNT_TAGS( CMD_NAME_GET_COUNT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_INDEXES_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_GET_INDEXES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_INDEXES_TAGS( CMD_NAME_GET_INDEXES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_QUERYMETA_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_GET_QUERYMETA_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_QUERYMETA_TAGS( CMD_NAME_GET_QUERYMETA_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_DCINFO_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GET_DCINFO_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_DCINFO_TAGS( CMD_NAME_GET_DCINFO_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_COLLECTIONSPACES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_COLLECTIONSPACES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_COLLECTIONSPACES_TAGS( CMD_NAME_LIST_COLLECTIONSPACES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_CONTEXTS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_CONTEXTS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_CONTEXTS_TAGS( CMD_NAME_LIST_CONTEXTS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_CONTEXTS_CURRENT_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_CONTEXTS_CURRENT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_CONTEXTS_CURRENT_TAGS( CMD_NAME_LIST_CONTEXTS_CURRENT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SESSIONS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SESSIONS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SESSIONS_TAGS( CMD_NAME_LIST_SESSIONS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SESSIONS_CURRENT_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SESSIONS_CURRENT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SESSIONS_CURRENT_TAGS( CMD_NAME_LIST_SESSIONS_CURRENT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_STORAGEUNITS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_STORAGEUNITS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_STORAGEUNITS_TAGS( CMD_NAME_LIST_STORAGEUNITS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_GROUPS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_GROUPS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_GROUPS_TAGS( CMD_NAME_LIST_GROUPS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_DOMAINS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_DOMAINS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_DOMAINS_TAGS( CMD_NAME_LIST_DOMAINS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_CS_IN_DOMAIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_CS_IN_DOMAIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_CS_IN_DOMAIN_TAGS( CMD_NAME_LIST_CS_IN_DOMAIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_CL_IN_DOMAIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_CL_IN_DOMAIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_CL_IN_DOMAIN_TAGS( CMD_NAME_LIST_CL_IN_DOMAIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_USERS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_USERS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_USERS_TAGS( CMD_NAME_LIST_USERS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_BACKUPS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_BACKUPS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_BACKUPS_TAGS( CMD_NAME_LIST_BACKUPS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_TASKS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_TASKS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_TASKS_TAGS( CMD_NAME_LIST_TASKS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_TRANSACTIONS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_TRANSACTIONS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_TRANSACTIONS_TAGS( CMD_NAME_LIST_TRANSACTIONS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_TRANSACTIONS_CUR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_TRANSACTIONS_CUR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_TRANSACTIONS_CUR_TAGS( CMD_NAME_LIST_TRANSACTIONS_CUR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SVCTASKS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SVCTASKS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SVCTASKS_TAGS( CMD_NAME_LIST_SVCTASKS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SEQUENCES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SEQUENCES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SEQUENCES_TAGS( CMD_NAME_LIST_SEQUENCES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_DATASOURCES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_DATASOURCES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_DATASOURCES_TAGS( CMD_NAME_LIST_DATASOURCES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_RECYCLEBIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_RECYCLEBIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_RECYCLEBIN_TAGS( CMD_NAME_LIST_RECYCLEBIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_RENAME_COLLECTION_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_RENAME_COLLECTION_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_RENAME_COLLECTION_TAGS( CMD_NAME_RENAME_COLLECTION_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_RENAME_COLLECTIONSPACE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_RENAME_COLLECTIONSPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_RENAME_COLLECTIONSPACE_TAGS( CMD_NAME_RENAME_COLLECTIONSPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CONTEXTS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CONTEXTS_TAGS( CMD_NAME_SNAPSHOT_CONTEXTS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_TAGS( CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_DATABASE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_DATABASE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_DATABASE_TAGS( CMD_NAME_SNAPSHOT_DATABASE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_RESET_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_RESET_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_RESET_TAGS( CMD_NAME_SNAPSHOT_RESET_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SESSIONS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SESSIONS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SESSIONS_TAGS( CMD_NAME_SNAPSHOT_SESSIONS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_TAGS( CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SYSTEM_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SYSTEM_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SYSTEM_TAGS( CMD_NAME_SNAPSHOT_SYSTEM_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_COLLECTIONS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_COLLECTIONS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_COLLECTIONS_TAGS( CMD_NAME_SNAPSHOT_COLLECTIONS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_COLLECTIONSPACES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_COLLECTIONSPACES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_COLLECTIONSPACES_TAGS( CMD_NAME_SNAPSHOT_COLLECTIONSPACES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CATA_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CATA_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CATA_TAGS( CMD_NAME_SNAPSHOT_CATA_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSACTIONS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSACTIONS_TAGS( CMD_NAME_SNAPSHOT_TRANSACTIONS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_TAGS( CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_ACCESSPLANS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_ACCESSPLANS_TAGS( CMD_NAME_SNAPSHOT_ACCESSPLANS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_HEALTH_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_HEALTH_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_HEALTH_TAGS( CMD_NAME_SNAPSHOT_HEALTH_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CONFIGS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CONFIGS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CONFIGS_TAGS( CMD_NAME_SNAPSHOT_CONFIGS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SVCTASKS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SVCTASKS_TAGS( CMD_NAME_SNAPSHOT_SVCTASKS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SEQUENCES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SEQUENCES_TAGS( CMD_NAME_SNAPSHOT_SEQUENCES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_QUERIES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_QUERIES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_QUERIES_TAGS( CMD_NAME_SNAPSHOT_QUERIES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_LATCHWAITS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_LATCHWAITS_TAGS( CMD_NAME_SNAPSHOT_LATCHWAITS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_LOCKWAITS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_LOCKWAITS_TAGS( CMD_NAME_SNAPSHOT_LOCKWAITS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_INDEXSTATS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_INDEXSTATS_TAGS( CMD_NAME_SNAPSHOT_INDEXSTATS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TASKS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TASKS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TASKS_TAGS( CMD_NAME_SNAPSHOT_TASKS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_INDEXES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_INDEXES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_INDEXES_TAGS( CMD_NAME_SNAPSHOT_INDEXES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSWAITS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSWAITS_TAGS( CMD_NAME_SNAPSHOT_TRANSWAITS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSDEADLOCK_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSDEADLOCK_TAGS( CMD_NAME_SNAPSHOT_TRANSDEADLOCK_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_RECYCLEBIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_RECYCLEBIN_TAGS( CMD_NAME_SNAPSHOT_RECYCLEBIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TEST_COLLECTION_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_TEST_COLLECTION_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TEST_COLLECTION_TAGS( CMD_NAME_TEST_COLLECTION_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TEST_COLLECTIONSPACE_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_TEST_COLLECTIONSPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TEST_COLLECTIONSPACE_TAGS( CMD_NAME_TEST_COLLECTIONSPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_GROUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_GROUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_GROUP_TAGS( CMD_NAME_CREATE_GROUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REMOVE_GROUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REMOVE_GROUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REMOVE_GROUP_TAGS( CMD_NAME_REMOVE_GROUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_NODE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_NODE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_NODE_TAGS( CMD_NAME_CREATE_NODE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REMOVE_NODE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REMOVE_NODE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REMOVE_NODE_TAGS( CMD_NAME_REMOVE_NODE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REMOVE_BACKUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REMOVE_BACKUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REMOVE_BACKUP_TAGS( CMD_NAME_REMOVE_BACKUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ACTIVE_GROUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ACTIVE_GROUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ACTIVE_GROUP_TAGS( CMD_NAME_ACTIVE_GROUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_STARTUP_NODE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_STARTUP_NODE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_STARTUP_NODE_TAGS( CMD_NAME_STARTUP_NODE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SHUTDOWN_NODE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SHUTDOWN_NODE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SHUTDOWN_NODE_TAGS( CMD_NAME_SHUTDOWN_NODE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SHUTDOWN_GROUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SHUTDOWN_GROUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SHUTDOWN_GROUP_TAGS( CMD_NAME_SHUTDOWN_GROUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SET_PDLEVEL_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SET_PDLEVEL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SET_PDLEVEL_TAGS( CMD_NAME_SET_PDLEVEL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SPLIT_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_SPLIT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SPLIT_TAGS( CMD_NAME_SPLIT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_WAITTASK_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_WAITTASK_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_WAITTASK_TAGS( CMD_NAME_WAITTASK_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_CATA_GROUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_CATA_GROUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_CATA_GROUP_TAGS( CMD_NAME_CREATE_CATA_GROUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TRACE_START_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_TRACE_START_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TRACE_START_TAGS( CMD_NAME_TRACE_START_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TRACE_RESUME_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_TRACE_RESUME_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TRACE_RESUME_TAGS( CMD_NAME_TRACE_RESUME_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TRACE_STOP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_TRACE_STOP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TRACE_STOP_TAGS( CMD_NAME_TRACE_STOP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TRACE_STATUS_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_TRACE_STATUS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TRACE_STATUS_TAGS( CMD_NAME_TRACE_STATUS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_DOMAIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_DOMAIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_DOMAIN_TAGS( CMD_NAME_CREATE_DOMAIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_DOMAIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_DOMAIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_DOMAIN_TAGS( CMD_NAME_DROP_DOMAIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_EXPORT_CONFIG_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_EXPORT_CONFIG_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_EXPORT_CONFIG_TAGS( CMD_NAME_EXPORT_CONFIG_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CRT_PROCEDURE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CRT_PROCEDURE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CRT_PROCEDURE_TAGS( CMD_NAME_CRT_PROCEDURE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_RM_PROCEDURE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_RM_PROCEDURE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_RM_PROCEDURE_TAGS( CMD_NAME_RM_PROCEDURE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_PROCEDURES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_PROCEDURES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_PROCEDURES_TAGS( CMD_NAME_LIST_PROCEDURES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_EVAL_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_EVAL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_EVAL_TAGS( CMD_NAME_EVAL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LINK_CL_TAGS_ARRAY[] = {
            AUTH_CMD_NAME_LINK_CL_maincl,
            AUTH_CMD_NAME_LINK_CL_subcl,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LINK_CL_TAGS( CMD_NAME_LINK_CL_TAGS_ARRAY, 2);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_UNLINK_CL_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_UNLINK_CL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_UNLINK_CL_TAGS( CMD_NAME_UNLINK_CL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_INVALIDATE_CACHE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_INVALIDATE_CACHE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_INVALIDATE_CACHE_TAGS( CMD_NAME_INVALIDATE_CACHE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_INVALIDATE_SEQUENCE_CACHE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_INVALIDATE_SEQUENCE_CACHE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_INVALIDATE_SEQUENCE_CACHE_TAGS( CMD_NAME_INVALIDATE_SEQUENCE_CACHE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_INVALIDATE_DATASOURCE_CACHE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_INVALIDATE_DATASOURCE_CACHE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_INVALIDATE_DATASOURCE_CACHE_TAGS( CMD_NAME_INVALIDATE_DATASOURCE_CACHE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_FORCE_SESSION_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_FORCE_SESSION_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_FORCE_SESSION_TAGS( CMD_NAME_FORCE_SESSION_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_LOBS_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_LIST_LOBS_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_LOBS_TAGS( CMD_NAME_LIST_LOBS_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_DC_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_DC_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_DC_TAGS( CMD_NAME_ALTER_DC_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_USR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_USR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_USR_TAGS( CMD_NAME_ALTER_USR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REELECT_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REELECT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REELECT_TAGS( CMD_NAME_REELECT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_FORCE_STEP_UP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_FORCE_STEP_UP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_FORCE_STEP_UP_TAGS( CMD_NAME_FORCE_STEP_UP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_TRUNCATE_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_TRUNCATE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_TRUNCATE_TAGS( CMD_NAME_TRUNCATE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SYNC_DB_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SYNC_DB_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SYNC_DB_TAGS( CMD_NAME_SYNC_DB_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_POP_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_POP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_POP_TAGS( CMD_NAME_POP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_RELOAD_CONFIG_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_RELOAD_CONFIG_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_RELOAD_CONFIG_TAGS( CMD_NAME_RELOAD_CONFIG_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_UPDATE_CONFIG_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_UPDATE_CONFIG_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_UPDATE_CONFIG_TAGS( CMD_NAME_UPDATE_CONFIG_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DELETE_CONFIG_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DELETE_CONFIG_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DELETE_CONFIG_TAGS( CMD_NAME_DELETE_CONFIG_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_CL_DETAIL_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_GET_CL_DETAIL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_CL_DETAIL_TAGS( CMD_NAME_GET_CL_DETAIL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_CL_STAT_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_GET_CL_STAT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_CL_STAT_TAGS( CMD_NAME_GET_CL_STAT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_INDEX_STAT_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_GET_INDEX_STAT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_INDEX_STAT_TAGS( CMD_NAME_GET_INDEX_STAT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_DATASOURCE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_DATASOURCE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_DATASOURCE_TAGS( CMD_NAME_CREATE_DATASOURCE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_DATASOURCE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_DATASOURCE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_DATASOURCE_TAGS( CMD_NAME_DROP_DATASOURCE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_DATASOURCE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_DATASOURCE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_DATASOURCE_TAGS( CMD_NAME_ALTER_DATASOURCE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_RECYCLEBIN_DETAIL_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GET_RECYCLEBIN_DETAIL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_RECYCLEBIN_DETAIL_TAGS( CMD_NAME_GET_RECYCLEBIN_DETAIL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_RECYCLEBIN_COUNT_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GET_RECYCLEBIN_COUNT_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_RECYCLEBIN_COUNT_TAGS( CMD_NAME_GET_RECYCLEBIN_COUNT_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_RECYCLEBIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_RECYCLEBIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_RECYCLEBIN_TAGS( CMD_NAME_ALTER_RECYCLEBIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_RECYCLEBIN_ITEM_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_RECYCLEBIN_ITEM_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_RECYCLEBIN_ITEM_TAGS( CMD_NAME_DROP_RECYCLEBIN_ITEM_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_RECYCLEBIN_ALL_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_RECYCLEBIN_ALL_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_RECYCLEBIN_ALL_TAGS( CMD_NAME_DROP_RECYCLEBIN_ALL_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_RETURN_RECYCLEBIN_ITEM_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_RETURN_RECYCLEBIN_ITEM_TAGS( CMD_NAME_RETURN_RECYCLEBIN_ITEM_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_TAGS( CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_DATABASE_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_DATABASE_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_DATABASE_INTR_TAGS( CMD_NAME_SNAPSHOT_DATABASE_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SYSTEM_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SYSTEM_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SYSTEM_INTR_TAGS( CMD_NAME_SNAPSHOT_SYSTEM_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_COLLECTION_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_COLLECTION_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_COLLECTION_INTR_TAGS( CMD_NAME_SNAPSHOT_COLLECTION_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SPACE_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SPACE_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SPACE_INTR_TAGS( CMD_NAME_SNAPSHOT_SPACE_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CONTEXT_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CONTEXT_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CONTEXT_INTR_TAGS( CMD_NAME_SNAPSHOT_CONTEXT_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_TAGS( CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SESSION_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SESSION_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SESSION_INTR_TAGS( CMD_NAME_SNAPSHOT_SESSION_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_TAGS( CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CATA_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CATA_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CATA_INTR_TAGS( CMD_NAME_SNAPSHOT_CATA_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANS_INTR_TAGS( CMD_NAME_SNAPSHOT_TRANS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSCUR_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSCUR_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSCUR_INTR_TAGS( CMD_NAME_SNAPSHOT_TRANSCUR_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_TAGS( CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_HEALTH_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_HEALTH_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_HEALTH_INTR_TAGS( CMD_NAME_SNAPSHOT_HEALTH_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_CONFIGS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_CONFIGS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_CONFIGS_INTR_TAGS( CMD_NAME_SNAPSHOT_CONFIGS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SVCTASKS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SVCTASKS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SVCTASKS_INTR_TAGS( CMD_NAME_SNAPSHOT_SVCTASKS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_SEQUENCES_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_SEQUENCES_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_SEQUENCES_INTR_TAGS( CMD_NAME_SNAPSHOT_SEQUENCES_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_QUERIES_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_QUERIES_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_QUERIES_INTR_TAGS( CMD_NAME_SNAPSHOT_QUERIES_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_TAGS( CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_TAGS( CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_TAGS( CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TASKS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TASKS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TASKS_INTR_TAGS( CMD_NAME_SNAPSHOT_TASKS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_INDEXES_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_INDEXES_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_INDEXES_INTR_TAGS( CMD_NAME_SNAPSHOT_INDEXES_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_TAGS( CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_TAGS( CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_TAGS( CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_COLLECTION_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_COLLECTION_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_COLLECTION_INTR_TAGS( CMD_NAME_LIST_COLLECTION_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SPACE_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SPACE_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SPACE_INTR_TAGS( CMD_NAME_LIST_SPACE_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_CONTEXT_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_CONTEXT_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_CONTEXT_INTR_TAGS( CMD_NAME_LIST_CONTEXT_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_CONTEXTCUR_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_CONTEXTCUR_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_CONTEXTCUR_INTR_TAGS( CMD_NAME_LIST_CONTEXTCUR_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SESSION_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SESSION_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SESSION_INTR_TAGS( CMD_NAME_LIST_SESSION_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SESSIONCUR_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SESSIONCUR_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SESSIONCUR_INTR_TAGS( CMD_NAME_LIST_SESSIONCUR_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_STORAGEUNIT_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_STORAGEUNIT_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_STORAGEUNIT_INTR_TAGS( CMD_NAME_LIST_STORAGEUNIT_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_BACKUP_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_BACKUP_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_BACKUP_INTR_TAGS( CMD_NAME_LIST_BACKUP_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_TRANS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_TRANS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_TRANS_INTR_TAGS( CMD_NAME_LIST_TRANS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_TRANSCUR_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_TRANSCUR_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_TRANSCUR_INTR_TAGS( CMD_NAME_LIST_TRANSCUR_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_GROUP_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_GROUP_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_GROUP_INTR_TAGS( CMD_NAME_LIST_GROUP_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_USER_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_USER_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_USER_INTR_TAGS( CMD_NAME_LIST_USER_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_TASK_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_TASK_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_TASK_INTR_TAGS( CMD_NAME_LIST_TASK_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_INDEXES_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_INDEXES_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_INDEXES_INTR_TAGS( CMD_NAME_LIST_INDEXES_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_DOMAIN_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_DOMAIN_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_DOMAIN_INTR_TAGS( CMD_NAME_LIST_DOMAIN_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SVCTASKS_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SVCTASKS_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SVCTASKS_INTR_TAGS( CMD_NAME_LIST_SVCTASKS_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_SEQUENCES_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_SEQUENCES_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_SEQUENCES_INTR_TAGS( CMD_NAME_LIST_SEQUENCES_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_DATASOURCE_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_DATASOURCE_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_DATASOURCE_INTR_TAGS( CMD_NAME_LIST_DATASOURCE_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_RECYCLEBIN_INTR_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_RECYCLEBIN_INTR_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_RECYCLEBIN_INTR_TAGS( CMD_NAME_LIST_RECYCLEBIN_INTR_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_COLLECTION_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_ALTER_COLLECTION_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_COLLECTION_TAGS( CMD_NAME_ALTER_COLLECTION_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_COLLECTION_SPACE_TAGS_ARRAY[] = {
             AUTH_CMD_NAME_ALTER_COLLECTION_SPACE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_COLLECTION_SPACE_TAGS( CMD_NAME_ALTER_COLLECTION_SPACE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_DOMAIN_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_DOMAIN_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_DOMAIN_TAGS( CMD_NAME_ALTER_DOMAIN_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_NODE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_NODE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_NODE_TAGS( CMD_NAME_ALTER_NODE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_ALTER_GROUP_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_ALTER_GROUP_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_ALTER_GROUP_TAGS( CMD_NAME_ALTER_GROUP_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_CREATE_ROLE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_CREATE_ROLE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_CREATE_ROLE_TAGS( CMD_NAME_CREATE_ROLE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_DROP_ROLE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_DROP_ROLE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_DROP_ROLE_TAGS( CMD_NAME_DROP_ROLE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_ROLE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GET_ROLE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_ROLE_TAGS( CMD_NAME_GET_ROLE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_LIST_ROLES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_LIST_ROLES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_LIST_ROLES_TAGS( CMD_NAME_LIST_ROLES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_UPDATE_ROLE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_UPDATE_ROLE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_UPDATE_ROLE_TAGS( CMD_NAME_UPDATE_ROLE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GRANT_PRIVILEGES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GRANT_PRIVILEGES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GRANT_PRIVILEGES_TAGS( CMD_NAME_GRANT_PRIVILEGES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REVOKE_PRIVILEGES_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REVOKE_PRIVILEGES_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REVOKE_PRIVILEGES_TAGS( CMD_NAME_REVOKE_PRIVILEGES_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GRANT_ROLES_TO_ROLE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GRANT_ROLES_TO_ROLE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GRANT_ROLES_TO_ROLE_TAGS( CMD_NAME_GRANT_ROLES_TO_ROLE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REVOKE_ROLES_FROM_ROLE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REVOKE_ROLES_FROM_ROLE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REVOKE_ROLES_FROM_ROLE_TAGS( CMD_NAME_REVOKE_ROLES_FROM_ROLE_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GET_USER_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GET_USER_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GET_USER_TAGS( CMD_NAME_GET_USER_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_GRANT_ROLES_TO_USER_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_GRANT_ROLES_TO_USER_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_GRANT_ROLES_TO_USER_TAGS( CMD_NAME_GRANT_ROLES_TO_USER_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_REVOKE_ROLES_FROM_USER_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_REVOKE_ROLES_FROM_USER_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_REVOKE_ROLES_FROM_USER_TAGS( CMD_NAME_REVOKE_ROLES_FROM_USER_TAGS_ARRAY, 1);
   static const AUTH_CMD_ACTION_SETS_TAG CMD_NAME_INVALIDATE_USER_CACHE_TAGS_ARRAY[] = {
         AUTH_CMD_NAME_INVALIDATE_USER_CACHE_default,
   };
   static const CMD_TAGS_ARRAY CMD_NAME_INVALIDATE_USER_CACHE_TAGS( CMD_NAME_INVALIDATE_USER_CACHE_TAGS_ARRAY, 1);

   const CMD_TAGS_ARRAY *authGetCMDActionSetsTags( const char *cmd )
   {
      if (strcmp(cmd, CMD_NAME_BACKUP_OFFLINE) == 0)
      {
         return &CMD_NAME_BACKUP_OFFLINE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_COLLECTION) == 0)
      {
         return &CMD_NAME_CREATE_COLLECTION_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_COLLECTIONSPACE) == 0)
      {
         return &CMD_NAME_CREATE_COLLECTIONSPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_SEQUENCE) == 0)
      {
         return &CMD_NAME_CREATE_SEQUENCE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_SEQUENCE) == 0)
      {
         return &CMD_NAME_DROP_SEQUENCE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_SEQUENCE) == 0)
      {
         return &CMD_NAME_ALTER_SEQUENCE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_SEQ_CURR_VAL) == 0)
      {
         return &CMD_NAME_GET_SEQ_CURR_VAL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_INDEX) == 0)
      {
         return &CMD_NAME_CREATE_INDEX_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CANCEL_TASK) == 0)
      {
         return &CMD_NAME_CANCEL_TASK_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_COLLECTION) == 0)
      {
         return &CMD_NAME_DROP_COLLECTION_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_COLLECTIONSPACE) == 0)
      {
         return &CMD_NAME_DROP_COLLECTIONSPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LOAD_COLLECTIONSPACE) == 0)
      {
         return &CMD_NAME_LOAD_COLLECTIONSPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_UNLOAD_COLLECTIONSPACE) == 0)
      {
         return &CMD_NAME_UNLOAD_COLLECTIONSPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_INDEX) == 0)
      {
         return &CMD_NAME_DROP_INDEX_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_COPY_INDEX) == 0)
      {
         return &CMD_NAME_COPY_INDEX_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_COUNT) == 0)
      {
         return &CMD_NAME_GET_COUNT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_INDEXES) == 0)
      {
         return &CMD_NAME_GET_INDEXES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_QUERYMETA) == 0)
      {
         return &CMD_NAME_GET_QUERYMETA_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_DCINFO) == 0)
      {
         return &CMD_NAME_GET_DCINFO_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_COLLECTIONSPACES) == 0)
      {
         return &CMD_NAME_LIST_COLLECTIONSPACES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_CONTEXTS) == 0)
      {
         return &CMD_NAME_LIST_CONTEXTS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_CONTEXTS_CURRENT) == 0)
      {
         return &CMD_NAME_LIST_CONTEXTS_CURRENT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SESSIONS) == 0)
      {
         return &CMD_NAME_LIST_SESSIONS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SESSIONS_CURRENT) == 0)
      {
         return &CMD_NAME_LIST_SESSIONS_CURRENT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_STORAGEUNITS) == 0)
      {
         return &CMD_NAME_LIST_STORAGEUNITS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_GROUPS) == 0)
      {
         return &CMD_NAME_LIST_GROUPS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_DOMAINS) == 0)
      {
         return &CMD_NAME_LIST_DOMAINS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_CS_IN_DOMAIN) == 0)
      {
         return &CMD_NAME_LIST_CS_IN_DOMAIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_CL_IN_DOMAIN) == 0)
      {
         return &CMD_NAME_LIST_CL_IN_DOMAIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_USERS) == 0)
      {
         return &CMD_NAME_LIST_USERS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_BACKUPS) == 0)
      {
         return &CMD_NAME_LIST_BACKUPS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_TASKS) == 0)
      {
         return &CMD_NAME_LIST_TASKS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_TRANSACTIONS) == 0)
      {
         return &CMD_NAME_LIST_TRANSACTIONS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_TRANSACTIONS_CUR) == 0)
      {
         return &CMD_NAME_LIST_TRANSACTIONS_CUR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SVCTASKS) == 0)
      {
         return &CMD_NAME_LIST_SVCTASKS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SEQUENCES) == 0)
      {
         return &CMD_NAME_LIST_SEQUENCES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_DATASOURCES) == 0)
      {
         return &CMD_NAME_LIST_DATASOURCES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_RECYCLEBIN) == 0)
      {
         return &CMD_NAME_LIST_RECYCLEBIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_RENAME_COLLECTION) == 0)
      {
         return &CMD_NAME_RENAME_COLLECTION_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_RENAME_COLLECTIONSPACE) == 0)
      {
         return &CMD_NAME_RENAME_COLLECTIONSPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CONTEXTS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CONTEXTS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_DATABASE) == 0)
      {
         return &CMD_NAME_SNAPSHOT_DATABASE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_RESET) == 0)
      {
         return &CMD_NAME_SNAPSHOT_RESET_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SESSIONS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SESSIONS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SESSIONS_CURRENT) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SESSIONS_CURRENT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SYSTEM) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SYSTEM_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_COLLECTIONS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_COLLECTIONS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_COLLECTIONSPACES) == 0)
      {
         return &CMD_NAME_SNAPSHOT_COLLECTIONSPACES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CATA) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CATA_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSACTIONS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSACTIONS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_ACCESSPLANS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_ACCESSPLANS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_HEALTH) == 0)
      {
         return &CMD_NAME_SNAPSHOT_HEALTH_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CONFIGS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CONFIGS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SVCTASKS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SVCTASKS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SEQUENCES) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SEQUENCES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_QUERIES) == 0)
      {
         return &CMD_NAME_SNAPSHOT_QUERIES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_LATCHWAITS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_LATCHWAITS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_LOCKWAITS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_LOCKWAITS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_INDEXSTATS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_INDEXSTATS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TASKS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TASKS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_INDEXES) == 0)
      {
         return &CMD_NAME_SNAPSHOT_INDEXES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSWAITS) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSWAITS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSDEADLOCK) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSDEADLOCK_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_RECYCLEBIN) == 0)
      {
         return &CMD_NAME_SNAPSHOT_RECYCLEBIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TEST_COLLECTION) == 0)
      {
         return &CMD_NAME_TEST_COLLECTION_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TEST_COLLECTIONSPACE) == 0)
      {
         return &CMD_NAME_TEST_COLLECTIONSPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_GROUP) == 0)
      {
         return &CMD_NAME_CREATE_GROUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REMOVE_GROUP) == 0)
      {
         return &CMD_NAME_REMOVE_GROUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_NODE) == 0)
      {
         return &CMD_NAME_CREATE_NODE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REMOVE_NODE) == 0)
      {
         return &CMD_NAME_REMOVE_NODE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REMOVE_BACKUP) == 0)
      {
         return &CMD_NAME_REMOVE_BACKUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ACTIVE_GROUP) == 0)
      {
         return &CMD_NAME_ACTIVE_GROUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_STARTUP_NODE) == 0)
      {
         return &CMD_NAME_STARTUP_NODE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SHUTDOWN_NODE) == 0)
      {
         return &CMD_NAME_SHUTDOWN_NODE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SHUTDOWN_GROUP) == 0)
      {
         return &CMD_NAME_SHUTDOWN_GROUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SET_PDLEVEL) == 0)
      {
         return &CMD_NAME_SET_PDLEVEL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SPLIT) == 0)
      {
         return &CMD_NAME_SPLIT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_WAITTASK) == 0)
      {
         return &CMD_NAME_WAITTASK_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_CATA_GROUP) == 0)
      {
         return &CMD_NAME_CREATE_CATA_GROUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TRACE_START) == 0)
      {
         return &CMD_NAME_TRACE_START_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TRACE_RESUME) == 0)
      {
         return &CMD_NAME_TRACE_RESUME_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TRACE_STOP) == 0)
      {
         return &CMD_NAME_TRACE_STOP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TRACE_STATUS) == 0)
      {
         return &CMD_NAME_TRACE_STATUS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_DOMAIN) == 0)
      {
         return &CMD_NAME_CREATE_DOMAIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_DOMAIN) == 0)
      {
         return &CMD_NAME_DROP_DOMAIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_EXPORT_CONFIG) == 0)
      {
         return &CMD_NAME_EXPORT_CONFIG_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CRT_PROCEDURE) == 0)
      {
         return &CMD_NAME_CRT_PROCEDURE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_RM_PROCEDURE) == 0)
      {
         return &CMD_NAME_RM_PROCEDURE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_PROCEDURES) == 0)
      {
         return &CMD_NAME_LIST_PROCEDURES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_EVAL) == 0)
      {
         return &CMD_NAME_EVAL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LINK_CL) == 0)
      {
         return &CMD_NAME_LINK_CL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_UNLINK_CL) == 0)
      {
         return &CMD_NAME_UNLINK_CL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_INVALIDATE_CACHE) == 0)
      {
         return &CMD_NAME_INVALIDATE_CACHE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_INVALIDATE_SEQUENCE_CACHE) == 0)
      {
         return &CMD_NAME_INVALIDATE_SEQUENCE_CACHE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_INVALIDATE_DATASOURCE_CACHE) == 0)
      {
         return &CMD_NAME_INVALIDATE_DATASOURCE_CACHE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_FORCE_SESSION) == 0)
      {
         return &CMD_NAME_FORCE_SESSION_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_LOBS) == 0)
      {
         return &CMD_NAME_LIST_LOBS_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_DC) == 0)
      {
         return &CMD_NAME_ALTER_DC_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_USR) == 0)
      {
         return &CMD_NAME_ALTER_USR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REELECT) == 0)
      {
         return &CMD_NAME_REELECT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_FORCE_STEP_UP) == 0)
      {
         return &CMD_NAME_FORCE_STEP_UP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_TRUNCATE) == 0)
      {
         return &CMD_NAME_TRUNCATE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SYNC_DB) == 0)
      {
         return &CMD_NAME_SYNC_DB_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_POP) == 0)
      {
         return &CMD_NAME_POP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_RELOAD_CONFIG) == 0)
      {
         return &CMD_NAME_RELOAD_CONFIG_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_UPDATE_CONFIG) == 0)
      {
         return &CMD_NAME_UPDATE_CONFIG_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DELETE_CONFIG) == 0)
      {
         return &CMD_NAME_DELETE_CONFIG_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_CL_DETAIL) == 0)
      {
         return &CMD_NAME_GET_CL_DETAIL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_CL_STAT) == 0)
      {
         return &CMD_NAME_GET_CL_STAT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_INDEX_STAT) == 0)
      {
         return &CMD_NAME_GET_INDEX_STAT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_DATASOURCE) == 0)
      {
         return &CMD_NAME_CREATE_DATASOURCE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_DATASOURCE) == 0)
      {
         return &CMD_NAME_DROP_DATASOURCE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_DATASOURCE) == 0)
      {
         return &CMD_NAME_ALTER_DATASOURCE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_RECYCLEBIN_DETAIL) == 0)
      {
         return &CMD_NAME_GET_RECYCLEBIN_DETAIL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_RECYCLEBIN_COUNT) == 0)
      {
         return &CMD_NAME_GET_RECYCLEBIN_COUNT_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_RECYCLEBIN) == 0)
      {
         return &CMD_NAME_ALTER_RECYCLEBIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_RECYCLEBIN_ITEM) == 0)
      {
         return &CMD_NAME_DROP_RECYCLEBIN_ITEM_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_RECYCLEBIN_ALL) == 0)
      {
         return &CMD_NAME_DROP_RECYCLEBIN_ALL_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_RETURN_RECYCLEBIN_ITEM) == 0)
      {
         return &CMD_NAME_RETURN_RECYCLEBIN_ITEM_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME) == 0)
      {
         return &CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_DATABASE_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_DATABASE_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SYSTEM_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SYSTEM_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_COLLECTION_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_COLLECTION_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SPACE_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SPACE_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CONTEXT_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CONTEXT_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SESSION_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SESSION_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SESSIONCUR_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SESSIONCUR_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CATA_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CATA_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSCUR_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSCUR_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_HEALTH_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_HEALTH_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_CONFIGS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_CONFIGS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SVCTASKS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SVCTASKS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_SEQUENCES_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_SEQUENCES_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_QUERIES_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_QUERIES_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_LATCHWAITS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_LATCHWAITS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_LOCKWAITS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_LOCKWAITS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_INDEXSTATS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_INDEXSTATS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TASKS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TASKS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_INDEXES_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_INDEXES_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSWAITS_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSWAITS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR) == 0)
      {
         return &CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_COLLECTION_INTR) == 0)
      {
         return &CMD_NAME_LIST_COLLECTION_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SPACE_INTR) == 0)
      {
         return &CMD_NAME_LIST_SPACE_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_CONTEXT_INTR) == 0)
      {
         return &CMD_NAME_LIST_CONTEXT_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_CONTEXTCUR_INTR) == 0)
      {
         return &CMD_NAME_LIST_CONTEXTCUR_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SESSION_INTR) == 0)
      {
         return &CMD_NAME_LIST_SESSION_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SESSIONCUR_INTR) == 0)
      {
         return &CMD_NAME_LIST_SESSIONCUR_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_STORAGEUNIT_INTR) == 0)
      {
         return &CMD_NAME_LIST_STORAGEUNIT_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_BACKUP_INTR) == 0)
      {
         return &CMD_NAME_LIST_BACKUP_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_TRANS_INTR) == 0)
      {
         return &CMD_NAME_LIST_TRANS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_TRANSCUR_INTR) == 0)
      {
         return &CMD_NAME_LIST_TRANSCUR_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_GROUP_INTR) == 0)
      {
         return &CMD_NAME_LIST_GROUP_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_USER_INTR) == 0)
      {
         return &CMD_NAME_LIST_USER_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_TASK_INTR) == 0)
      {
         return &CMD_NAME_LIST_TASK_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_INDEXES_INTR) == 0)
      {
         return &CMD_NAME_LIST_INDEXES_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_DOMAIN_INTR) == 0)
      {
         return &CMD_NAME_LIST_DOMAIN_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SVCTASKS_INTR) == 0)
      {
         return &CMD_NAME_LIST_SVCTASKS_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_SEQUENCES_INTR) == 0)
      {
         return &CMD_NAME_LIST_SEQUENCES_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_DATASOURCE_INTR) == 0)
      {
         return &CMD_NAME_LIST_DATASOURCE_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_RECYCLEBIN_INTR) == 0)
      {
         return &CMD_NAME_LIST_RECYCLEBIN_INTR_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_COLLECTION) == 0)
      {
         return &CMD_NAME_ALTER_COLLECTION_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_COLLECTION_SPACE) == 0)
      {
         return &CMD_NAME_ALTER_COLLECTION_SPACE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_DOMAIN) == 0)
      {
         return &CMD_NAME_ALTER_DOMAIN_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_NODE) == 0)
      {
         return &CMD_NAME_ALTER_NODE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_ALTER_GROUP) == 0)
      {
         return &CMD_NAME_ALTER_GROUP_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_CREATE_ROLE) == 0)
      {
         return &CMD_NAME_CREATE_ROLE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_DROP_ROLE) == 0)
      {
         return &CMD_NAME_DROP_ROLE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_ROLE) == 0)
      {
         return &CMD_NAME_GET_ROLE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_LIST_ROLES) == 0)
      {
         return &CMD_NAME_LIST_ROLES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_UPDATE_ROLE) == 0)
      {
         return &CMD_NAME_UPDATE_ROLE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GRANT_PRIVILEGES) == 0)
      {
         return &CMD_NAME_GRANT_PRIVILEGES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REVOKE_PRIVILEGES) == 0)
      {
         return &CMD_NAME_REVOKE_PRIVILEGES_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GRANT_ROLES_TO_ROLE) == 0)
      {
         return &CMD_NAME_GRANT_ROLES_TO_ROLE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REVOKE_ROLES_FROM_ROLE) == 0)
      {
         return &CMD_NAME_REVOKE_ROLES_FROM_ROLE_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GET_USER) == 0)
      {
         return &CMD_NAME_GET_USER_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_GRANT_ROLES_TO_USER) == 0)
      {
         return &CMD_NAME_GRANT_ROLES_TO_USER_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_REVOKE_ROLES_FROM_USER) == 0)
      {
         return &CMD_NAME_REVOKE_ROLES_FROM_USER_TAGS;
      }
      if (strcmp(cmd, CMD_NAME_INVALIDATE_USER_CACHE) == 0)
      {
         return &CMD_NAME_INVALIDATE_USER_CACHE_TAGS;
      }
      return NULL;
   }

   // Built-in roles
   // _cs_read
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_cs_read[BUILTIN_ROLE_DATA_SIZE_cs_read] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_COLLECTION_SPACE, ACTION_SET_NUMBER_ARRAY(
         136ULL,
         0ULL
      )),
   };

   // _cs_readWrite
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_cs_readWrite[BUILTIN_ROLE_DATA_SIZE_cs_readWrite] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_COLLECTION_SPACE, ACTION_SET_NUMBER_ARRAY(
         248ULL,
         0ULL
      )),
   };

   // _cs_admin
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_cs_admin[BUILTIN_ROLE_DATA_SIZE_cs_admin] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_COLLECTION_SPACE, ACTION_SET_NUMBER_ARRAY(
         134742015ULL,
         524288ULL
      )),
   };

   // _clusterAdmin
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_clusterAdmin[BUILTIN_ROLE_DATA_SIZE_clusterAdmin] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_CLUSTER, ACTION_SET_NUMBER_ARRAY(
         5190398570544496640ULL,
         14797508680ULL
      )),
   };

   // _clusterMonitor
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_clusterMonitor[BUILTIN_ROLE_DATA_SIZE_clusterMonitor] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_CLUSTER, ACTION_SET_NUMBER_ARRAY(
         72092984594792448ULL,
         618477457944ULL
      )),
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_NON_SYSTEM, ACTION_SET_NUMBER_ARRAY(
         262272ULL,
         524288ULL
      )),
   };

   // _backup
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_backup[BUILTIN_ROLE_DATA_SIZE_backup] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_CLUSTER, ACTION_SET_NUMBER_ARRAY(
         268435456ULL,
         8388640ULL
      )),
   };

   // _dbAdmin
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_dbAdmin[BUILTIN_ROLE_DATA_SIZE_dbAdmin] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_NON_SYSTEM, ACTION_SET_NUMBER_ARRAY(
         134742015ULL,
         524288ULL
      )),
   };

   // _userAdmin
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_userAdmin[BUILTIN_ROLE_DATA_SIZE_userAdmin] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_CLUSTER, ACTION_SET_NUMBER_ARRAY(
         281457796841472ULL,
         4ULL
      )),
   };

   // _root
   const std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY> BUILTIN_ROLE_DATA_root[BUILTIN_ROLE_DATA_SIZE_root] = {
      std::pair<RESOURCE_TYPE_ENUM, ACTION_SET_NUMBER_ARRAY>(RESOURCE_TYPE_ANY, ACTION_SET_NUMBER_ARRAY(
         18446744073709551615ULL,
         18446744073709551615ULL
      )),
   };

}