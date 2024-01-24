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

final class SdbConstants {
    private SdbConstants() {
    }

    final static String FIELD_NAME_NAME = "Name";
    final static String FIELD_NAME_OLDNAME = "OldName";
    final static String FIELD_NAME_NEWNAME = "NewName";
    final static String FIELD_NAME_HOST = "HostName";
    final static String FIELD_NAME_GROUPNAME = "GroupName";
    final static String FIELD_NAME_GROUPSERVICE = "Service";
    final static String FIELD_NAME_GROUP = "Group";
    final static String FIELD_NAME_NODEID = "NodeID";
    final static String FIELD_NAME_NODENAME = "NodeName";
    final static String FIELD_NAME_GROUPID = "GroupID";
    final static String FIELD_NAME_PRIMARY = "PrimaryNode";
    final static String FIELD_NAME_SERVICENAME = "Name";
    final static String FIELD_NAME_SERVICETYPE = "Type";
    final static String FIELD_NAME_SOURCE = "Source";
    final static String FIELD_NAME_TARGET = "Target";
    final static String FIELD_NAME_SPLITQUERY = "SplitQuery";
    final static String FIELD_NAME_SPLITENDQUERY = "SplitEndQuery";
    final static String FIELD_NAME_SPLITPERCENT = "SplitPercent";
    final static String FIELD_NAME_PATH = "Path";
    final static String FIELD_NAME_FUNC = "func";
    final static String FIELD_NAME_SUBCLNAME = "SubCLName";
    final static String FIELD_NAME_HINT = "Hint";
    final static String FIELD_NAME_ASYNC = "Async";
    final static String FIELD_NAME_TASKID = "TaskID";
    final static String FIELD_NAME_OPTIONS = "Options";
    final static String FIELD_NAME_DOMAIN = "Domain";
    final static String FIELD_NAME_CELLECTIONSPACE = "CollectionSpace";
    final static String FIELD_NAME_AUTOINCREMENT = "AutoIncrement";
    final static String FIELD_NAME_AUTOINC_FIELD = "Field";

    final static String FIELD_NAME_SESSION_ID = "SessionID";

    final static String FIELD_NAME_ARGS = "Args";
    final static String FIELD_NAME_ALTER = "Alter";
    final static String FIELD_NAME_ALTER_TYPE = "AlterType";
    final static String FIELD_NAME_VERSION = "Version";

    final static int SDB_ALTER_VERSION = 1;
    final static String SDB_ALTER_CL = "collection";
    final static String SDB_ALTER_CS = "collection space";
    final static String SDB_ALTER_DOMAIN = "domain";
    final static String SDB_ALTER_DATASOURCE = "data source";
    final static String SDB_ALTER_CRT_ID_INDEX = "create id index";
    final static String SDB_ALTER_DROP_ID_INDEX = "drop id index";
    final static String SDB_ALTER_ENABLE_SHARDING = "enable sharding";
    final static String SDB_ALTER_DISABLE_SHARDING = "disable sharding";
    final static String SDB_ALTER_ENABLE_COMPRESSION = "enable compression";
    final static String SDB_ALTER_DISABLE_COMPRESSION = "disable compression";
    final static String SDB_ALTER_SET_ATTRIBUTES = "set attributes";
    final static String SDB_ALTER_SET_DOMAIN = "set domain";
    final static String SDB_ALTER_REMOVE_DOMAIN = "remove domain";
    final static String SDB_ALTER_ENABLE_CAPPED = "enable capped";
    final static String SDB_ALTER_SET_LOCATION = "set location";
    final static String SDB_ALTER_DISABLE_CAPPED = "disable capped";
    final static String SDB_ALTER_ADD_GROUPS = "add groups";
    final static String SDB_ALTER_SET_GROUPS = "set groups";
    final static String SDB_ALTER_REMOVE_GROUPS = "remove groups";
    final static String SDB_ALTER_CL_CRT_AUTOINC_FLD = "create autoincrement";
    final static String SDB_ALTER_CL_DROP_AUTOINC_FLD = "drop autoincrement";

    final static String SDB_ALTER_ENABLE_RECYCLEBIN = "enable";
    final static String SDB_ALTER_DISABLE_RECYCLEBIN = "disable";
    final static String SDB_ALTER_GROUP_SET_ACTIVE_LOCATION = "set active location";
    final static String SDB_ALTER_GROUP_START_CRITICAL_MODE = "start critical mode";
    final static String SDB_ALTER_GROUP_STOP_CRITICAL_MODE = "stop critical mode";
    final static String SDB_ALTER_GROUP_START_MAINTENANCE_MODE = "start maintenance mode";
    final static String SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE = "stop maintenance mode";

    final static String FIELD_NAME_MODIFY = "$Modify";
    final static String FIELD_NAME_OP = "OP";
    final static String FIELD_NAME_OP_UPDATE = "Update";
    final static String FIELD_NAME_OP_REMOVE = "Remove";
    final static String FIELD_NAME_RETURNNEW = "ReturnNew";
    final static String FIELD_OP_VALUE_UPDATE = "update";
    final static String FIELD_OP_VALUE_REMOVE = "remove";

    final static String FIELD_NAME_SET_ON_INSERT = "$SetOnInsert";

    final static String FMP_FUNC_TYPE = "funcType";
    final static String FIELD_COLLECTION = "Collection";
    final static String FIELD_TOTAL = "Total";
    final static String FIELD_INDEX = "Index";
    final static String FIELD_NAME_PREFERRED_INSTANCE_LEGACY = "PreferedInstance";
    final static String FIELD_NAME_PREFERRED_INSTANCE_V1_LEGACY = "PreferedInstanceV1";
    final static String FIELD_NAME_PREFERRED_INSTANCE = "PreferredInstance";
    final static String FIELD_NAME_RETYE = "ReturnType";

    final static String FIELD_NAME_ONLY_DETACH = "OnlyDetach";
    final static String FIELD_NAME_ONLY_ATTACH = "OnlyAttach";

    final static String FIELD_NAME_LOGICALID = "LogicalID";
    final static String FIELD_NAME_DIRECTION = "Direction";

    final static String FIELD_NAME_ACTION = "Action";
    final static String FIELD_NAME_FETCH_NUM = "FetchNum";
    final static String FIELD_NAME_START_VALUE = "StartValue";
    final static String FIELD_NAME_EXPECT_VALUE = "ExpectValue";
    final static String FIELD_NAME_CURRENT_VALUE = "CurrentValue";
    final static String FIELD_NAME_INDEXNAME = "IndexName";

    final static String FIELD_NAME_ADDRESS = "Address";
    final static String FIELD_NAME_USER = "User";
    final static String FIELD_NAME_PASSWD = "Password";
    final static String FIELD_NAME_TYPE = "Type";

    final static String FIELD_NAME_ENABLE = "Enable";
    final static String FIELD_NAME_RECYCLE_NAME = "RecycleName";
    final static String FIELD_NAME_RETURN_NAME = "ReturnName";

    final static String IXM_NAME = "name";
    final static String IXM_KEY = "key";
    final static String IXM_UNIQUE = "Unique";
    final static String IXM_ENFORCED = "Enforced";
    final static String IXM_UNIQUE_LEGACY = "unique";
    final static String IXM_ENFORCED_LEGACY = "enforced";
    final static String IXM_NOTNULL = "NotNull";
    final static String IXM_INDEXDEF = "IndexDef";
    final static String IXM_FIELD_NAME_SORT_BUFFER_SIZE = "SortBufferSize";
    final static int IXM_SORT_BUFFER_DEFAULT_SIZE = 64;
    final static boolean IXM_UNIQUE_DEFAULT = false;
    final static boolean IXM_ENFORCED_DEFAULT = false;
    final static boolean IXM_NOTNULL_DEFAULT = false;


    final static String PMD_OPTION_SVCNAME = "svcname";
    final static String PMD_OPTION_DBPATH = "dbpath";

    final static String OID = "_id";

    final static int FLG_UPDATE_UPSERT = 0x00000001;
    final static int FLG_INSERT_RETURNNUM = 0x00000002;
    final static int FLG_UPDATE_RETURNNUM = 0x00000004;
    final static int FLG_DELETE_RETURNNUM = 0x00000004;

    final static String SEQ_OPT_SETATTR = "set attributes";
    final static String SEQ_OPT_SET_CURR_VALUE = "set current value";
    final static String SEQ_OPT_RENAME = "rename";
    final static String SEQ_OPT_RESTART = "restart";

    final static String NODE_LOCATION = "Location";
    final static String NODE_SET_LOCATION = "set location";

    final static String FIELD_NAME_GROUP_ACTIVE_LOCATION = "ActiveLocation";

    // for role
    final static String FIELD_NAME_ROLE = "Role";
    final static String FIELD_NAME_PRIVILEGES = "Privileges";
    final static String FIELD_NAME_ROLES = "Roles";
}
