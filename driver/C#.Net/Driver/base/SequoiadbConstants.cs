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

using System;

namespace SequoiaDB
{
    internal class SequoiadbConstants
    {
        public const int OP_MAXNAMELENGTH = 255;

        public const int COLLECTION_SPACE_MAX_SIZE = 127;
        public const int COLLECTION_MAX_SZ = 127;
        public const int MAX_CS_SIZE = 127;

        public const Int32 MSG_SYSTEM_INFO_LEN = ~0;
        public const UInt32 MSG_SYSTEM_INFO_EYECATCHER = 0xFFFEFDFC;
        public const UInt32 MSG_SYSTEM_INFO_EYECATCHER_REVERT = 0xFCFDFEFF;

        public const string UNKNOWN_TYPE = "UNKNOWN";
        public const string UNKONWN_DESC = "Unknown Error";
        public const int UNKNOWN_CODE = 1;

        public const string SYS_PREFIX = "SYS";
        public const string CATALOG_GROUP = SYS_PREFIX + "CatalogGroup";
        public const int CATALOG_GROUPID = 1;
        public const string NODE_NAME_SERVICE_SEP = ":";

        public const string ADMIN_PROMPT = "$";
        public const string LIST_CMD = "list";
        public const string TEST_CMD = "test";
        public const string ADD_CMD = "add";
        public const string ALTER_CMD = "alter";
        public const string CREATE_CMD = "create";
	    public const string REMOVE_CMD = "remove";
        public const string RENAME_CMD = "rename";
        public const string SPLIT_CMD = "split";
        public const string DROP_CMD = "drop";
        public const string STARTUP_CMD = "startup";
        public const string SHUTDOWN_CMD = "shutdown";
        public const string ACTIVE_CMD = "active";
        public const string SNAP_CMD = "snapshot";
        public const string COLSPACES = "collectionspaces";
        public const string COLSPACE = "collectionspace";
        public const string COLLECTIONS = "collections";
        public const string GROUPS = "groups";
        public const string GROUP = "group";
        public const string CATALOG = "catalog";
        public const string NODE = "node";
        public const string CONTEXTS = "contexts";
        public const string CONTEXTS_CUR = "contexts current";
        public const string SESSIONS = "sessions";
        public const string SESSIONS_CUR = "sessions current";
        public const string STOREUNITS = "storageunits";
        public const string PROCEDURES = "procedures";
        public const string DOMAIN = "domain";
        public const string DOMAINS = "domains";
        public const string TASKS = "tasks";
        public const string TRANSACTIONS = "transactions";
        public const string TRANSACTIONS_CURRENT = "transactions current";
        public const string ACCESSPLANS = "accessplans";
        public const string HEALTH = "health";
        public const string CONFIGS = "configs";
        public const string SVCTASKS = "service tasks";
        public const string SEQUENCES = "sequences";
        public const string QUERIES = "queries";
        public const string LATCHWAITS = "latchwaits";
        public const string LOCKWAITS = "lockwaits";
        public const string INDEXSTATS = "index statistics";
        public const string TRANSWAITS = "waiting transactions";
        public const string TRANSDEADLOCK = "transaction deadlocks";
        public const string USERS = "users";
        public const string BACKUPS = "backups";
        public const string CS_IN_DOMAIN = "collectionspaces in domain";
        public const string CL_IN_DOMAIN = "collections in domain";
        public const string ALTER_CS = "alter collectionspace";

        public const string COLLECTION = "collection";
        public const string CREATE_INX = "create index";
        public const string DROP_INX = "drop index";
        public const string GET_INXES = "get indexes";
        public const string GET_INDEX_STAT = "get index statistic";
        public const string GET_COUNT = "get count";
        public const string DATABASE = "database";
        public const string SYSTEM = "system";
        public const string CATA = "catalog";
        public const string RESET = "reset";
        public const string GET_QUERYMETA = "get querymeta";
        public const string LINK_CL = "link collection"; 
        public const string UNLINK_CL = "unlink collection";
        public const string SETSESS_ATTR = "set session attribute";
        public const string GETSESS_ATTR = "get session attribute";
        public const string LIST_TASK_CMD = "list tasks";
	    public const string WAIT_TASK_CMD = "wait task";
        public const string CANCEL_TASK_CMD = "cancel task";
        public const string BACKUP_OFFLINE_CMD = "backup offline";
        public const string LIST_BACKUP_CMD = "list backups";
        public const string REMOVE_BACKUP_CMD = "remove backup";
        public const string CRT_PROCEDURES_CMD = "create procedures";
        public const string RM_PROCEDURES_CMD = "remove procedures";
        public const string ALTER_COLLECTION = "alter collection";
        public const string LIST_LOBS_CMD = "list lobs";
        public const string TRUNCATE = "truncate";
        public const string SDB_ALTER_CL_CRT_AUTOINC_FLD = "create autoincrement";
        public const string SDB_ALTER_CL_DROP_AUTOINC_FLD = "drop autoincrement";

        public const string CMD_NAME_ALTER_DC = "alter dc";
        public const string CMD_NAME_GET_DCINFO = "get dcinfo";
        public const string CMD_NAME_UPDATE_CONFIG = "update config";
        public const string CMD_NAME_DELETE_CONFIG = "delete config";
        public const string CMD_NAME_INVALIDATE_CACHE = "invalidate cache";
        public const string CMD_VALUE_NAME_CREATE = "create image";
        public const string CMD_VALUE_NAME_REMOVE = "remove image";
        public const string CMD_VALUE_NAME_ATTACH = "attach groups";
        public const string CMD_VALUE_NAME_DETACH = "detach groups";
        public const string CMD_VALUE_NAME_ENABLE = "enable image";
        public const string CMD_VALUE_NAME_DISABLE = "disable image";
        public const string CMD_VALUE_NAME_ACTIVATE = "activate";
        public const string CMD_VALUE_NAME_DEACTIVATE = "deactivate";
        public const string CMD_VALUE_NAME_ENABLE_READONLY = "enable readonly";
        public const string CMD_VALUE_NAME_DISABLE_READONLY = "disable readonly";
        public const string CMD_VALUE_NAME_SYNC_DB = "sync db";
        public const string CMD_VALUE_NAME_ANALYZE = "analyze";
        public const string CMD_NAME_RENAME_COLLECTION = "rename collection";
        public const string CMD_NAME_RENAME_COLLECTIONSPACE = "rename collectionspace";

        public const string CREATE_SEQUENCE = "create sequence";
        public const string DROP_SEQUENCE = "drop sequence";
        public const string ALTER_SEQUENCE = "alter sequence";
        public const string GET_SEQ_CURR_VAL = "get sequence current value";

        public const string OID = "_id";
        public const string CLIENT_RECORD_ID_INDEX = "$id";
        public const string SVCNAME = "svcname";
        public const string DBPATH = "dbpath";

        public const string FIELD_SOURCE = "Source";
        public const string FIELD_TARGET = "Target";
        public const string FIELD_SPLITQUERY = "SplitQuery";
        public const string FIELD_SPLITENDQUERY = "SplitEndQuery";
        public const string FIELD_SPLITPERCENT = "SplitPercent";
        public const string FIELD_NAME = "Name";
        public const string FIELD_OLDNAME = "OldName";
        public const string FIELD_NEWNAME = "NewName";
        public const string FIELD_PAGESIZE = "PageSize";
        public const string FIELD_INDEX = "Index";
        public const string FIELD_TOTAL = "Total";
        public const string FIELD_HINT = "Hint";
        public const string FIELD_COLLECTION = "Collection";
        public const string FIELD_COLLECTIONSPACE = "CollectionSpace";
        public const string FIELD_GROUP = "Group";
        public const string FIELD_GROUPS = "Groups";
        public const string FIELD_GROUPNAME = "GroupName";
        public const string FIELD_HOSTNAME = "HostName";
        public const string FIELD_SERVICE = "Service";
        public const string FIELD_NODEID = "NodeID";
        public const string FIELD_GROUPID = "GroupID";
        public const string FIELD_SERVICE_TYPE = "Type";
        public const string FIELD_STATUS = "Status";
        public const string FIELD_PRIMARYNODE = "PrimaryNode";
        public const string FIELD_SHARDINGKEY = "ShardingKey";
        public const string FIELD_SUBCLNAME = "SubCLName";
        public const string FIELD_PREFERED_INSTANCE = "PreferedInstance";
        public const string FIELD_PREFERED_INSTANCE_V1 = "PreferedInstanceV1";
        public const string FIELD_ASYNC = "Async";
        public const string FIELD_TASKTYPE = "TaskType";
        public const string FIELD_TASKID = "TaskID";
        public const string FIELD_PATH = "Path";
	    public const string FIELD_DESP = "Description";
	    public const string FIELD_ENSURE_INC = "EnsureInc";
        public const string FIELD_OVERWRITE = "OverWrite";
        public const string FIELD_OPTIONS = "Options";
        public const string FIELD_DOMAIN = "Domain";
        public const string FIELD_LOB_OID = "Oid";
        public const string FIELD_LOB_OPEN_MODE = "Mode";
        public const string FIELD_LOB_SIZE = "Size";
        public const string FIELD_LOB_CREATE_TIME = "CreateTime";
        public const string FIELD_LOB_MODIFICATION_TIME = "ModificationTime";
        public const string FIELD_LOB_PAGESIZE = "LobPageSize";
        public const string FIELD_LOB_OFFSET = "Offset";
        public const string FIELD_LOB_LENGTH = "Length";
        public const string FIELD_NAME_ONLY_DETACH = "OnlyDetach";
        public const string FIELD_NAME_ONLY_ATTACH = "OnlyAttach";
        public const string FIELD_NAME_ALTER = "Alter";
        public const string FIELD_NAME_ALTER_TYPE = "AlterType";
        public const string FIELD_NAME_ARGS = "Args";
        public const string FIELD_NAME_VERSION = "Version";
        public const string FIELD_NAME_ACTION = "Action";
        public const string FIELD_NAME_ADDRESS = "Address";
        public const string FIELD_NAME_CLUSTERNAME = "ClusterName";
        public const string FIELD_NAME_BUSINESSNAME = "BusinessName";
        public const string FIELD_NAME_DATACENTER = "DataCenter";
        public const string FIELD_NAME_CONFIGS = "Configs";
        public const string FIELD_NAME_OLDNAME = "OldName";
        public const string FIELD_NAME_NEWNAME = "NewName";
        public const string FIELD_NAME_CELLECTIONSPACE = "CollectionSpace";
	    public const string FIELD_NAME_AUTOINCREMENT = "AutoIncrement";
        public const string FIELD_NAME_AUTOINC_FIELD = "Field";
        public const string FIELD_NAME_FETCH_NUM = "FetchNum";
        public const string FIELD_NAME_START_VALUE = "StartValue";
        public const string FIELD_NAME_EXPECT_VALUE = "ExpectValue";
        public const string FIELD_NAME_CURRENT_VALUE = "CurrentValue";
        public const string FIELD_MODIFY = "$Modify";
        public const string FIELD_OP = "OP";
        public const string FIELD_OP_UPDATE = "Update";
        public const string FIELD_OP_REMOVE = "Remove";
		public const string FIELD_RETURNNEW = "ReturnNew";
        public const string FIELD_OP_VALUE_UPDATE = "update";
        public const string FIELD_OP_VALUE_REMOVE = "remove";

        public const int SDB_ALTER_VERSION = 1;
        public const string SDB_ALTER_DB = "db";
        public const string SDB_ALTER_CL = "collection";
        public const string SDB_ALTER_CS = "collection space";
        public const string SDB_ALTER_DOMAIN = "domain";
        public const string SDB_ALTER_GROUP = "group";
        public const string SDB_ALTER_NODE = "node";

        public const string SDB_ALTER_CRT_ID_INDEX = "create id index";
        public const string SDB_ALTER_DROP_ID_INDEX = "drop id index";
        public const string SDB_ALTER_ENABLE_SHARDING = "enable sharding";
        public const string SDB_ALTER_DISABLE_SHARDING = "disable sharding";
        public const string SDB_ALTER_ENABLE_COMPRESSION = "enable compression";
        public const string SDB_ALTER_DISABLE_COMPRESSION = "disable compression";
        public const string SDB_ALTER_SET_ATTRIBUTES = "set attributes";
        public const string SDB_ALTER_SET_DOMAIN = "set domain";
        public const string SDB_ALTER_REMOVE_DOMAIN = "remove domain";
        public const string SDB_ALTER_ENABLE_CAPPED = "enable capped";
        public const string SDB_ALTER_DISABLE_CAPPED = "disable capped";
        public const string SDB_ALTER_ADD_GROUPS = "add groups";
        public const string SDB_ALTER_SET_GROUPS = "set groups";
        public const string SDB_ALTER_REMOVE_GROUPS = "remove groups";

        public const string SEQ_OPT_SET_CURR_VALUE = "set current value";
        public const string SEQ_OPT_RESTART = "restart";


        public const string FIELD_SET_ON_INSERT = "$SetOnInsert";

        public const string IXM_NAME = "name";
        public const string IXM_KEY = "key";
        public const string IXM_UNIQUE = "unique";
        public const string IXM_ENFORCED = "enforced";
        public const string IXM_INDEXDEF = "IndexDef";
        public const string IXM_SORT_BUFFER_SIZE = "SortBufferSize";
        public const int IXM_SORT_BUFFER_DEFAULT_SIZE = 64;

        public const string SDB_AUTH_USER = "User";
        public const string SDB_AUTH_PASSWD = "Passwd";

        public const int FLG_UPDATE_UPSERT = 0x00000001;
        public const int FLG_REPLY_CONTEXTSORNOTFOUND = 0x00000001;
        public const int FLG_REPLY_SHARDCONFSTALE = 0x00000004;

        public const int SDB_DMS_EOC = (int) Errors.errors.SDB_DMS_EOC; 

        public static readonly byte[] ZERO_NODEID = new byte[12] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	    public const int DEFAULT_VERSION    = 1;
	    public const short DEFAULT_W        = 1;
	    public const int DEFAULT_FLAGS      = 0;
	    public const long DEFAULT_CONTEXTID = -1;
    }
    internal enum Operation : int
    {
        RES_FLAG                  = unchecked((int)0x80000000),

        OP_MSG                    = 1000,
        OP_UPDATE                 = 2001,
        OP_INSERT                 = 2002,
        OP_SQL                    = 2003,
        OP_QUERY                  = 2004,
        OP_GETMORE                = 2005,
        OP_DELETE                 = 2006,
        OP_KILL_CONTEXT           = 2007,
        OP_DISCONNECT             = 2008,

        OP_INTERRUPT              = 2009,
        OP_TRANS_BEGIN            = 2010,
        OP_TRANS_COMMIT           = 2011,
        OP_TRANS_ROLLBACK         = 2012,

	    OP_AGGREGATE              = 2019,
        OP_INTERRUPTE_SELF        = 2020,
        MSG_BS_SEQUENCE_FETCH_REQ = 2022, MSG_BS_SEQUENCE_FETCH_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_SEQUENCE_FETCH_REQ)),

        MSG_AUTH_VERIFY_REQ       = 7000,
        MSG_AUTH_CRTUSR_REQ       = 7001,
        MSG_AUTH_DELUSR_REQ       = 7002,

        MSG_BS_LOB_OPEN_REQ = 8001, MSG_BS_LOB_OPEN_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_OPEN_REQ)),
        MSG_BS_LOB_WRITE_REQ = 8002, MSG_BS_LOB_WRITE_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_WRITE_REQ)),
        MSG_BS_LOB_READ_REQ = 8003, MSG_BS_LOB_READ_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_READ_REQ)),
        MSG_BS_LOB_REMOVE_REQ = 8004, MSG_BS_LOB_REMOVE_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_REMOVE_REQ)),
        MSG_BS_LOB_UPDATE_REQ = 8005, MSG_BS_LOB_UPDATE_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_UPDATE_REQ)),
        MSG_BS_LOB_CLOSE_REQ = 8006, MSG_BS_LOB_CLOSE_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_CLOSE_REQ)),
        MSG_BS_LOB_LOCK_REQ = 8007, MSG_BS_LOB_LOCK_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_LOCK_REQ)),
        MSG_BS_LOB_TRUNCATE_REQ = 8008, MSG_BS_LOB_TRUNCATE_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_TRUNCATE_REQ)),
        MSG_BS_LOB_CREATELOBID_REQ = 8009, MSG_BS_LOB_CREATELOBID_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_CREATELOBID_REQ)),
        MSG_BS_LOB_GETRTDETAIL_REQ = 8010, MSG_BS_LOB_GETRTDETAIL_RES = unchecked((int)(RES_FLAG | (uint)MSG_BS_LOB_GETRTDETAIL_REQ)),
        MSG_NULL                  = 9999
    };

    internal enum PreferInstanceType : int
    { 
		INS_TYPE_MIN = 0,
		INS_NODE_1 = 1,
		INS_NODE_2 = 2,
		INS_NODE_3 = 3,
		INS_NODE_4 = 4,
		INS_NODE_5 = 5,
		INS_NODE_6 = 6,
		INS_NODE_7 = 7,
		INS_MASTER = 8,
		INS_SLAVE = 9,
		INS_ANYONE = 10,
		INS_TYPE_MAX = 11
    };
}
