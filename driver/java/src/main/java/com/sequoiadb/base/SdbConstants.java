/*
 * Copyright 2017 SequoiaDB Inc.
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

    final static String FIELD_NAME_ARGS = "Args";
    final static String FIELD_NAME_ALTER = "Alter";
    final static String FIELD_NAME_ALTER_TYPE = "AlterType";
    final static String FIELD_NAME_VERSION = "Version";

    final static int SDB_ALTER_VERSION = 1;
    final static String SDB_ALTER_CL = "collection";
    final static String SDB_ALTER_CRT_ID_INDEX = "create id index";
    final static String SDB_ALTER_DROP_ID_INDEX = "drop id index";

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
    final static String FIELD_NAME_PREFERED_INSTANCE = "PreferedInstance";
    final static String FIELD_NAME_PREFERED_INSTANCE_V1 = "PreferedInstanceV1";
    final static String FIELD_NAME_RETYE = "ReturnType";

    final static String FIELD_NAME_ONLY_DETACH = "OnlyDetach";
    final static String FIELD_NAME_ONLY_ATTACH = "OnlyAttach";

    final static String FIELD_NAME_LOGICALID = "LogicalID";
    final static String FIELD_NAME_DIRECTION = "Direction";

    final static String IXM_NAME = "name";
    final static String IXM_KEY = "key";
    final static String IXM_UNIQUE = "unique";
    final static String IXM_ENFORCED = "enforced";
    final static String IXM_INDEXDEF = "IndexDef";
    final static String IXM_FIELD_NAME_SORT_BUFFER_SIZE = "SortBufferSize";
    final static int IXM_SORT_BUFFER_DEFAULT_SIZE = 64;

    final static String PMD_OPTION_SVCNAME = "svcname";
    final static String PMD_OPTION_DBPATH = "dbpath";

    final static String OID = "_id";

    final static int FLG_UPDATE_UPSERT = 0x00000001;
}
