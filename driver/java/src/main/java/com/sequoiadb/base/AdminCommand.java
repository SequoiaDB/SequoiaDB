/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sequoiadb.base;

final class AdminCommand {
    private AdminCommand() {
    }

    private final static String PREFIX = "$";

    final static String CREATE_CS = PREFIX + "create collectionspace";
    final static String DROP_CS = PREFIX + "drop collectionspace";
    final static String LOAD_CS = PREFIX + "load collectionspace";
    final static String UNLOAD_CS = PREFIX + "unload collectionspace";
    final static String RENAME_CS = PREFIX + "rename collectionspace";
    final static String TEST_CS = PREFIX + "test collectionspace";
    final static String ALTER_CS = PREFIX + "alter collectionspace";

    final static String TEST_CL = PREFIX + "test collection";
    final static String CREATE_CL = PREFIX + "create collection";
    final static String DROP_CL = PREFIX + "drop collection";
    final static String RENAME_CL = PREFIX + "rename collection";


    final static String GET_INDEXES = PREFIX + "get indexes";
    final static String CREATE_INDEX = PREFIX + "create index";
    final static String DROP_INDEX = PREFIX + "drop index";
    final static String COPY_INDEX = PREFIX + "copy index";
    final static String SNAPSHOT_INDEXES = PREFIX + "snapshot indexes";

    final static String GET_COUNT = PREFIX + "get count";
    final static String GET_QUERYMETA = PREFIX + "get querymeta";
    final static String SPLIT = PREFIX + "split";

    final static String ATTACH_CL = PREFIX + "link collection";
    final static String DETACH_CL = PREFIX + "unlink collection";
    final static String ALTER_COLLECTION = PREFIX + "alter collection";
    final static String TRUNCATE = PREFIX + "truncate";
    final static String POP = PREFIX + "pop";

    final static String LIST_LOBS = PREFIX + "list lobs";

    final static String SET_SESSION_ATTRIBUTE = PREFIX + "set session attribute";
    final static String GET_SESSION_ATTRIBUTE = PREFIX + "get session attribute";
    final static String FORCE_SESSION = PREFIX + "force session";
    final static String SYNC_DB = PREFIX + "sync db";
    final static String EXPORT_CONFIG = PREFIX + "export configuration";
    final static String UPDATE_CONFIG = PREFIX + "update config";
    final static String DELETE_CONFIG = PREFIX + "delete config";
    final static String ANALYZE = PREFIX + "analyze";
    final static String GET_INDEX_STAT = PREFIX + "get index statistic";

    final static String LIST_CONTEXTS = PREFIX + "list contexts";
    final static String LIST_CONTEXTS_CURRENT = PREFIX + "list contexts current";
    final static String LIST_SESSIONS = PREFIX + "list sessions";
    final static String LIST_SESSIONS_CURRENT = PREFIX + "list sessions current";
    final static String LIST_COLLECTIONS = PREFIX + "list collections";
    final static String LIST_COLLECTIONSPACES = PREFIX + "list collectionspaces";
    final static String LIST_STORAGEUNITS = PREFIX + "list storageunits";
    final static String LIST_GROUPS = PREFIX + "list groups";
    final static String LIST_PROCEDURES = PREFIX + "list procedures";
    final static String LIST_DOMAINS = PREFIX + "list domains";
    final static String LIST_TASKS = PREFIX + "list tasks";
    final static String LIST_TRANSACTIONS = PREFIX + "list transactions";
    final static String LIST_TRANSACTIONS_CURRENT = PREFIX + "list transactions current";
    final static String LIST_SVCTASKS = PREFIX + "list service tasks";
    final static String LIST_SEQUENCES = PREFIX + "list sequences";
    final static String LIST_USERS = PREFIX + "list users";
    final static String LIST_BACKUPS = PREFIX + "list backups";
    final static String LIST_DATASOURCES = PREFIX + "list datasources";
    final static String LIST_CL_IN_DOMAIN = PREFIX + "list collections in domain";
    final static String LIST_CS_IN_DOMAIN = PREFIX + "list collectionspaces in domain";
    final static String LIST_RECYCLEBIN = PREFIX + "list recyclebin";
    final static String LIST_GROUPMODES = PREFIX + "list group modes";

    final static String SNAP_CONTEXTS = PREFIX + "snapshot contexts";
    final static String SNAP_CONTEXTS_CURRENT = PREFIX + "snapshot contexts current";
    final static String SNAP_SESSIONS = PREFIX + "snapshot sessions";
    final static String SNAP_SESSIONS_CURRENT = PREFIX + "snapshot sessions current";
    final static String SNAP_COLLECTIONS = PREFIX + "snapshot collections";
    final static String SNAP_COLLECTIONSPACES = PREFIX + "snapshot collectionspaces";
    final static String SNAP_DATABASE = PREFIX + "snapshot database";
    final static String SNAP_SYSTEM = PREFIX + "snapshot system";
    final static String SNAP_CATALOG = PREFIX + "snapshot catalog";
    final static String SNAP_TRANSACTIONS = PREFIX + "snapshot transactions";
    final static String SNAP_TRANSACTIONS_CURRENT = PREFIX + "snapshot transactions current";
    final static String SNAP_ACCESSPLANS = PREFIX + "snapshot accessplans";
    final static String SNAP_HEALTH = PREFIX + "snapshot health";
    final static String SNAP_CONFIGS = PREFIX + "snapshot configs";
    final static String SNAP_SVCTASKS = PREFIX + "snapshot service tasks";
    final static String SNAP_SEQUENCES = PREFIX + "snapshot sequences";
    final static String SNAP_QUERIES  = PREFIX + "snapshot queries";
    final static String SNAP_LATCHWAITS  = PREFIX + "snapshot latchwaits";
    final static String SNAP_LOCKWAITS  = PREFIX + "snapshot lockwaits";
    final static String SNAP_INDEXSTATS = PREFIX + "snapshot index statistics";
    final static String SNAP_TASKS = PREFIX + "snapshot tasks";
    final static String SNAP_INDEXES = PREFIX + "snapshot indexes";
    final static String SNAP_TRANSWAITS = PREFIX + "snapshot waiting transactions";
    final static String SNAP_TRANSDEADLOCK = PREFIX + "snapshot transaction deadlocks";
    final static String SNAP_RECYCLEBIN = PREFIX + "snapshot recyclebin";

    final static String RESET_SNAPSHOT = PREFIX + "snapshot reset";

    final static String CREATE_PROCEDURE = PREFIX + "create procedure";
    final static String REMOVE_PROCEDURE = PREFIX + "remove procedure";
    final static String EVAL = PREFIX + "eval";

    final static String BACKUP_OFFLINE = PREFIX + "backup offline";
    final static String LIST_BACKUP = PREFIX + "list backups";
    final static String REMOVE_BACKUP = PREFIX + "remove backup";

    final static String WAIT_TASK = PREFIX + "wait task";
    final static String CANCEL_TASK = PREFIX + "cancel task";

    final static String CREATE_DOMAIN = PREFIX + "create domain";
    final static String DROP_DOMAIN = PREFIX + "drop domain";
    final static String ALTER_DOMAIN = PREFIX + "alter domain";

    final static String CREATE_GROUP = PREFIX + "create group";
    final static String REMOVE_GROUP = PREFIX + "remove group";
    final static String ACTIVE_GROUP = PREFIX + "active group";
    final static String SHUTDOWN_GROUP = PREFIX + "shutdown group";
    final static String CREATE_CATALOG_GROUP = PREFIX + "create catalog group";
    final static String ALTER_GROUP = PREFIX + "alter group";

    final static String REELECT = PREFIX + "reelect";

    final static String STARTUP_NODE = PREFIX + "startup node";
    final static String SHUTDOWN_NODE = PREFIX + "shutdown node";
    final static String CREATE_NODE = PREFIX + "create node";
    final static String REMOVE_NODE = PREFIX + "remove node";

    final static String CREATE_SEQUENCE = PREFIX + "create sequence";
    final static String DROP_SEQUENCE = PREFIX + "drop sequence";
    final static String ALTER_SEQUENCE = PREFIX + "alter sequence";
    final static String GET_SEQ_CURR_VAL = PREFIX + "get sequence current value";

    final static String CREATE_DATASOURCE = PREFIX + "create datasource";
    final static String DROP_DATASOURCE = PREFIX + "drop datasource";
    final static String ALTER_DATASOURCE = PREFIX + "alter datasource";

    final static String INVALIDATE_CACHE = PREFIX + "invalidate cache";

    final static String GET_RECYCLEBIN_DETAIL = PREFIX + "get recyclebin detail";
    final static String GET_RECYCLEBIN_COUNT = PREFIX + "get recyclebin count";
    final static String ALTER_RECYCLEBIN = PREFIX + "alter recyclebin";
    final static String RETURN_RECYCLEBIN_ITEM = PREFIX + "return recyclebin item";
    final static String RETURN_RECYCLEBIN_ITEM_TO_NAME = PREFIX + "return recyclebin item to name";
    final static String DROP_RECYCLEBIN_ITEM = PREFIX + "drop recyclebin item";
    final static String DROP_RECYCLEBIN_ALL = PREFIX + "drop recyclebin all";

    final static String ALTER_NODE = PREFIX + "alter node";

    // rbac
    final static String CREATE_ROLE = PREFIX + "create role";
    final static String DROP_ROLE = PREFIX + "drop role";
    final static String GET_ROLE = PREFIX + "get role";
    final static String LIST_ROLES = PREFIX + "list roles";
    final static String UPDATE_ROLE = PREFIX + "update role";
    final static String GRANT_PRIVILEGES = PREFIX + "grant privileges";
    final static String REVOKE_PRIVILEGES = PREFIX + "revoke privileges";
    final static String GRANT_ROLES_TO_ROLE = PREFIX + "grant roles to role";
    final static String REVOKE_ROLES_FROM_ROLE = PREFIX + "revoke roles from role";
    final static String GET_USER = PREFIX + "get user";
    final static String GRANT_ROLES_TO_USER = PREFIX + "grant roles to user";
    final static String REVOKE_ROLES_FROM_USER = PREFIX + "revoke roles from user";
    final static String INVALIDATE_USER_CACHE = PREFIX + "invalidate user cache";
}
