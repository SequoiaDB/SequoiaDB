#   Copyright (C) 2011-2017 SequoiaDB Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

# This Header File is automatically generated, you MUST NOT modify this file anyway!
# On the contrary, you can modify the xml file "sequoiadb/misc/autogen/rclist.xml" if necessary!


class Errcode(object):
    """Errcode of SequoiaDB.
    """

    def __init__(self, name, code, desc):
        self.__name = name
        self.__code = code
        self.__desc = desc

    @property
    def name(self):
        """Name of this error code.
        """
        return self.__name

    @property
    def code(self):
        """Code of this error code.
        """
        return self.__code

    @property
    def desc(self):
        """Description of this error code.
        """
        return self.__desc

    def __eq__(self, other):
        """Errcode can equals to Errcode and int.
        """
        if isinstance(other, Errcode):
            return self.code == other.code
        elif isinstance(other, int):
            return self.code == other
        else:
            return False

    def __ne__(self, other):
        """Errcode can not equals to Errcode and int.
        """
        return not self.__eq__(other)

    def __repr__(self):
        return "Errcode('%s', %d, '%s')" % (self.name, self.code, self.desc)

    def __str__(self):
        return "Errcode('%s', %d, '%s')" % (self.name, self.code, self.desc)


SDB_OK = Errcode("SDB_OK", 0, "OK")
SDB_IO = Errcode("SDB_IO", -1, "IO Exception")
SDB_OOM = Errcode("SDB_OOM", -2, "Out of Memory")
SDB_PERM = Errcode("SDB_PERM", -3, "Permission Error")
SDB_FNE = Errcode("SDB_FNE", -4, "File Not Exist")
SDB_FE = Errcode("SDB_FE", -5, "File Exist")
SDB_INVALIDARG = Errcode("SDB_INVALIDARG", -6, "Invalid Argument")
SDB_INVALIDSIZE = Errcode("SDB_INVALIDSIZE", -7, "Invalid size")
SDB_INTERRUPT = Errcode("SDB_INTERRUPT", -8, "Interrupt")
SDB_EOF = Errcode("SDB_EOF", -9, "Hit end of file")
SDB_SYS = Errcode("SDB_SYS", -10, "System error")
SDB_NOSPC = Errcode("SDB_NOSPC", -11, "No space is left on disk")
SDB_EDU_INVAL_STATUS = Errcode("SDB_EDU_INVAL_STATUS", -12, "EDU status is not valid")
SDB_TIMEOUT = Errcode("SDB_TIMEOUT", -13, "Timeout error")
SDB_QUIESCED = Errcode("SDB_QUIESCED", -14, "Database is quiesced")
SDB_NETWORK = Errcode("SDB_NETWORK", -15, "Network error")
SDB_NETWORK_CLOSE = Errcode("SDB_NETWORK_CLOSE", -16, "Network is closed from remote")
SDB_DATABASE_DOWN = Errcode("SDB_DATABASE_DOWN", -17, "Database is in shutdown status")
SDB_APP_FORCED = Errcode("SDB_APP_FORCED", -18, "Application is forced")
SDB_INVALIDPATH = Errcode("SDB_INVALIDPATH", -19, "Given path is not valid")
SDB_INVALID_FILE_TYPE = Errcode("SDB_INVALID_FILE_TYPE", -20, "Unexpected file type specified")
SDB_DMS_NOSPC = Errcode("SDB_DMS_NOSPC", -21, "There's no space for DMS")
SDB_DMS_EXIST = Errcode("SDB_DMS_EXIST", -22, "Collection already exists")
SDB_DMS_NOTEXIST = Errcode("SDB_DMS_NOTEXIST", -23, "Collection does not exist")
SDB_DMS_RECORD_TOO_BIG = Errcode("SDB_DMS_RECORD_TOO_BIG", -24, "User record is too large")
SDB_DMS_RECORD_NOTEXIST = Errcode("SDB_DMS_RECORD_NOTEXIST", -25, "Record does not exist")
SDB_DMS_OVF_EXIST = Errcode("SDB_DMS_OVF_EXIST", -26, "Remote overflow record exists")
SDB_DMS_RECORD_INVALID = Errcode("SDB_DMS_RECORD_INVALID", -27, "Invalid record")
SDB_DMS_SU_NEED_REORG = Errcode("SDB_DMS_SU_NEED_REORG", -28, "Storage unit need reorg")
SDB_DMS_EOC = Errcode("SDB_DMS_EOC", -29, "End of collection")
SDB_DMS_CONTEXT_IS_OPEN = Errcode("SDB_DMS_CONTEXT_IS_OPEN", -30, "Context is already opened")
SDB_DMS_CONTEXT_IS_CLOSE = Errcode("SDB_DMS_CONTEXT_IS_CLOSE", -31, "Context is closed")
SDB_OPTION_NOT_SUPPORT = Errcode("SDB_OPTION_NOT_SUPPORT", -32, "Option is not supported yet")
SDB_DMS_CS_EXIST = Errcode("SDB_DMS_CS_EXIST", -33, "Collection space already exists")
SDB_DMS_CS_NOTEXIST = Errcode("SDB_DMS_CS_NOTEXIST", -34, "Collection space does not exist")
SDB_DMS_INVALID_SU = Errcode("SDB_DMS_INVALID_SU", -35, "Storage unit file is invalid")
SDB_RTN_CONTEXT_NOTEXIST = Errcode("SDB_RTN_CONTEXT_NOTEXIST", -36, "Context does not exist")
SDB_IXM_MULTIPLE_ARRAY = Errcode("SDB_IXM_MULTIPLE_ARRAY", -37, "More than one fields are array type")
SDB_IXM_DUP_KEY = Errcode("SDB_IXM_DUP_KEY", -38, "Duplicate key exist")
SDB_IXM_KEY_TOO_LARGE = Errcode("SDB_IXM_KEY_TOO_LARGE", -39, "Index key is too large")
SDB_IXM_NOSPC = Errcode("SDB_IXM_NOSPC", -40, "No space can be found for index extent")
SDB_IXM_KEY_NOTEXIST = Errcode("SDB_IXM_KEY_NOTEXIST", -41, "Index key does not exist")
SDB_DMS_MAX_INDEX = Errcode("SDB_DMS_MAX_INDEX", -42, "Hit max number of index")
SDB_DMS_INIT_INDEX = Errcode("SDB_DMS_INIT_INDEX", -43, "Failed to initialize index")
SDB_DMS_COL_DROPPED = Errcode("SDB_DMS_COL_DROPPED", -44, "Collection is dropped")
SDB_IXM_IDENTICAL_KEY = Errcode("SDB_IXM_IDENTICAL_KEY", -45, "Two records get same key and rid")
SDB_IXM_EXIST = Errcode("SDB_IXM_EXIST", -46, "Duplicate index name")
SDB_IXM_NOTEXIST = Errcode("SDB_IXM_NOTEXIST", -47, "Index name does not exist")
SDB_IXM_UNEXPECTED_STATUS = Errcode("SDB_IXM_UNEXPECTED_STATUS", -48, "Unexpected index flag")
SDB_IXM_EOC = Errcode("SDB_IXM_EOC", -49, "Hit end of index")
SDB_IXM_DEDUP_BUF_MAX = Errcode("SDB_IXM_DEDUP_BUF_MAX", -50, "Hit the max of dedup buffer")
SDB_RTN_INVALID_PREDICATES = Errcode("SDB_RTN_INVALID_PREDICATES", -51, "Invalid predicates")
SDB_RTN_INDEX_NOTEXIST = Errcode("SDB_RTN_INDEX_NOTEXIST", -52, "Index does not exist")
SDB_RTN_INVALID_HINT = Errcode("SDB_RTN_INVALID_HINT", -53, "Invalid hint")
SDB_DMS_NO_MORE_TEMP = Errcode("SDB_DMS_NO_MORE_TEMP", -54, "No more temp collections are avaliable")
SDB_DMS_SU_OUTRANGE = Errcode("SDB_DMS_SU_OUTRANGE", -55, "Exceed max number of storage unit")
SDB_IXM_DROP_ID = Errcode("SDB_IXM_DROP_ID", -56, "$id index can't be dropped")
SDB_DPS_LOG_NOT_IN_BUF = Errcode("SDB_DPS_LOG_NOT_IN_BUF", -57, "Log was not found in log buf")
SDB_DPS_LOG_NOT_IN_FILE = Errcode("SDB_DPS_LOG_NOT_IN_FILE", -58, "Log was not found in log file")
SDB_PMD_RG_NOT_EXIST = Errcode("SDB_PMD_RG_NOT_EXIST", -59, "Replication group does not exist")
SDB_PMD_RG_EXIST = Errcode("SDB_PMD_RG_EXIST", -60, "Replication group exists")
SDB_INVALID_REQID = Errcode("SDB_INVALID_REQID", -61, "Invalid request id is received")
SDB_PMD_SESSION_NOT_EXIST = Errcode("SDB_PMD_SESSION_NOT_EXIST", -62, "Session ID does not exist")
SDB_PMD_FORCE_SYSTEM_EDU = Errcode("SDB_PMD_FORCE_SYSTEM_EDU", -63, "System EDU cannot be forced")
SDB_NOT_CONNECTED = Errcode("SDB_NOT_CONNECTED", -64, "Database is not connected")
SDB_UNEXPECTED_RESULT = Errcode("SDB_UNEXPECTED_RESULT", -65, "Unexpected result received")
SDB_CORRUPTED_RECORD = Errcode("SDB_CORRUPTED_RECORD", -66, "Corrupted record")
SDB_BACKUP_HAS_ALREADY_START = Errcode("SDB_BACKUP_HAS_ALREADY_START", -67, "Backup has already been started")
SDB_BACKUP_NOT_COMPLETE = Errcode("SDB_BACKUP_NOT_COMPLETE", -68, "Backup is not completed")
SDB_RTN_IN_BACKUP = Errcode("SDB_RTN_IN_BACKUP", -69, "Backup is in progress")
SDB_BAR_DAMAGED_BK_FILE = Errcode("SDB_BAR_DAMAGED_BK_FILE", -70, "Backup is corrupted")
SDB_RTN_NO_PRIMARY_FOUND = Errcode("SDB_RTN_NO_PRIMARY_FOUND", -71, "No primary node was found")
SDB_ERROR_RESERVED_1 = Errcode("SDB_ERROR_RESERVED_1", -72, "Reserved")
SDB_PMD_HELP_ONLY = Errcode("SDB_PMD_HELP_ONLY", -73, "Engine help argument is specified")
SDB_PMD_CON_INVALID_STATE = Errcode("SDB_PMD_CON_INVALID_STATE", -74, "Invalid connection state")
SDB_CLT_INVALID_HANDLE = Errcode("SDB_CLT_INVALID_HANDLE", -75, "Invalid handle")
SDB_CLT_OBJ_NOT_EXIST = Errcode("SDB_CLT_OBJ_NOT_EXIST", -76, "Object does not exist")
SDB_NET_ALREADY_LISTENED = Errcode("SDB_NET_ALREADY_LISTENED", -77, "Listening port is already occupied")
SDB_NET_CANNOT_LISTEN = Errcode("SDB_NET_CANNOT_LISTEN", -78, "Unable to listen the specified address")
SDB_NET_CANNOT_CONNECT = Errcode("SDB_NET_CANNOT_CONNECT", -79, "Unable to connect to the specified address")
SDB_NET_NOT_CONNECT = Errcode("SDB_NET_NOT_CONNECT", -80, "Connection does not exist")
SDB_NET_SEND_ERR = Errcode("SDB_NET_SEND_ERR", -81, "Failed to send")
SDB_NET_TIMER_ID_NOT_FOUND = Errcode("SDB_NET_TIMER_ID_NOT_FOUND", -82, "Timer does not exist")
SDB_NET_ROUTE_NOT_FOUND = Errcode("SDB_NET_ROUTE_NOT_FOUND", -83, "Route info does not exist")
SDB_NET_BROKEN_MSG = Errcode("SDB_NET_BROKEN_MSG", -84, "Broken msg")
SDB_NET_INVALID_HANDLE = Errcode("SDB_NET_INVALID_HANDLE", -85, "Invalid net handle")
SDB_DMS_INVALID_REORG_FILE = Errcode("SDB_DMS_INVALID_REORG_FILE", -86, "Invalid reorg file")
SDB_DMS_REORG_FILE_READONLY = Errcode("SDB_DMS_REORG_FILE_READONLY", -87, "Reorg file is in read only mode")
SDB_DMS_INVALID_COLLECTION_S = Errcode("SDB_DMS_INVALID_COLLECTION_S", -88, "Collection status is not valid")
SDB_DMS_NOT_IN_REORG = Errcode("SDB_DMS_NOT_IN_REORG", -89, "Collection is not in reorg state")
SDB_REPL_GROUP_NOT_ACTIVE = Errcode("SDB_REPL_GROUP_NOT_ACTIVE", -90, "Replication group is not activated")
SDB_REPL_INVALID_GROUP_MEMBER = Errcode("SDB_REPL_INVALID_GROUP_MEMBER", -91, "Node does not belong to the group")
SDB_DMS_INCOMPATIBLE_MODE = Errcode("SDB_DMS_INCOMPATIBLE_MODE", -92, "Collection status is not compatible")
SDB_DMS_INCOMPATIBLE_VERSION = Errcode("SDB_DMS_INCOMPATIBLE_VERSION", -93, "Incompatible version for storage unit")
SDB_REPL_LOCAL_G_V_EXPIRED = Errcode("SDB_REPL_LOCAL_G_V_EXPIRED", -94, "Version is expired for local group")
SDB_DMS_INVALID_PAGESIZE = Errcode("SDB_DMS_INVALID_PAGESIZE", -95, "Invalid page size")
SDB_REPL_REMOTE_G_V_EXPIRED = Errcode("SDB_REPL_REMOTE_G_V_EXPIRED", -96, "Version is expired for remote group")
SDB_CLS_VOTE_FAILED = Errcode("SDB_CLS_VOTE_FAILED", -97, "Failed to vote for primary")
SDB_DPS_CORRUPTED_LOG = Errcode("SDB_DPS_CORRUPTED_LOG", -98, "Log record is corrupted")
SDB_DPS_LSN_OUTOFRANGE = Errcode("SDB_DPS_LSN_OUTOFRANGE", -99, "LSN is out of boundary")
SDB_UNKNOWN_MESSAGE = Errcode("SDB_UNKNOWN_MESSAGE", -100, "Unknown mesage is received")
SDB_NET_UPDATE_EXISTING_NODE = Errcode("SDB_NET_UPDATE_EXISTING_NODE", -101, "Updated information is same as old one")
SDB_CLS_UNKNOW_MSG = Errcode("SDB_CLS_UNKNOW_MSG", -102, "Unknown message")
SDB_CLS_EMPTY_HEAP = Errcode("SDB_CLS_EMPTY_HEAP", -103, "Empty heap")
SDB_CLS_NOT_PRIMARY = Errcode("SDB_CLS_NOT_PRIMARY", -104, "Node is not primary")
SDB_CLS_NODE_NOT_ENOUGH = Errcode("SDB_CLS_NODE_NOT_ENOUGH", -105, "Not enough number of data nodes")
SDB_CLS_NO_CATALOG_INFO = Errcode("SDB_CLS_NO_CATALOG_INFO", -106, "Catalog information does not exist on data node")
SDB_CLS_DATA_NODE_CAT_VER_OLD = Errcode("SDB_CLS_DATA_NODE_CAT_VER_OLD", -107, "Catalog version is expired on data node")
SDB_CLS_COORD_NODE_CAT_VER_OLD = Errcode("SDB_CLS_COORD_NODE_CAT_VER_OLD", -108, "Catalog version is expired on coordinator node")
SDB_CLS_INVALID_GROUP_NUM = Errcode("SDB_CLS_INVALID_GROUP_NUM", -109, "Exceeds the max group size")
SDB_CLS_SYNC_FAILED = Errcode("SDB_CLS_SYNC_FAILED", -110, "Failed to sync log")
SDB_CLS_REPLAY_LOG_FAILED = Errcode("SDB_CLS_REPLAY_LOG_FAILED", -111, "Failed to replay log")
SDB_REST_EHS = Errcode("SDB_REST_EHS", -112, "Invalid HTTP header")
SDB_CLS_CONSULT_FAILED = Errcode("SDB_CLS_CONSULT_FAILED", -113, "Failed to negotiate")
SDB_DPS_MOVE_FAILED = Errcode("SDB_DPS_MOVE_FAILED", -114, "Failed to change DPS metadata")
SDB_DMS_CORRUPTED_SME = Errcode("SDB_DMS_CORRUPTED_SME", -115, "SME is corrupted")
SDB_APP_INTERRUPT = Errcode("SDB_APP_INTERRUPT", -116, "Application is interrupted")
SDB_APP_DISCONNECT = Errcode("SDB_APP_DISCONNECT", -117, "Application is disconnected")
SDB_OSS_CCE = Errcode("SDB_OSS_CCE", -118, "Character encoding errors")
SDB_COORD_QUERY_FAILED = Errcode("SDB_COORD_QUERY_FAILED", -119, "Failed to query on coord node")
SDB_CLS_BUFFER_FULL = Errcode("SDB_CLS_BUFFER_FULL", -120, "Buffer array is full")
SDB_RTN_SUBCONTEXT_CONFLICT = Errcode("SDB_RTN_SUBCONTEXT_CONFLICT", -121, "Sub context is conflict")
SDB_COORD_QUERY_EOC = Errcode("SDB_COORD_QUERY_EOC", -122, "EOC message is received by coordinator node")
SDB_DPS_FILE_SIZE_NOT_SAME = Errcode("SDB_DPS_FILE_SIZE_NOT_SAME", -123, "Size of DPS files are not the same")
SDB_DPS_FILE_NOT_RECOGNISE = Errcode("SDB_DPS_FILE_NOT_RECOGNISE", -124, "Invalid DPS log file")
SDB_OSS_NORES = Errcode("SDB_OSS_NORES", -125, "No resource is avaliable")
SDB_DPS_INVALID_LSN = Errcode("SDB_DPS_INVALID_LSN", -126, "Invalid LSN")
SDB_OSS_NPIPE_DATA_TOO_BIG = Errcode("SDB_OSS_NPIPE_DATA_TOO_BIG", -127, "Pipe buffer size is too small")
SDB_CAT_AUTH_FAILED = Errcode("SDB_CAT_AUTH_FAILED", -128, "Catalog authentication failed")
SDB_CLS_FULL_SYNC = Errcode("SDB_CLS_FULL_SYNC", -129, "Full sync is in progress")
SDB_CAT_ASSIGN_NODE_FAILED = Errcode("SDB_CAT_ASSIGN_NODE_FAILED", -130, "Failed to assign data node from coordinator node")
SDB_PHP_DRIVER_INTERNAL_ERROR = Errcode("SDB_PHP_DRIVER_INTERNAL_ERROR", -131, "PHP driver internal error")
SDB_COORD_SEND_MSG_FAILED = Errcode("SDB_COORD_SEND_MSG_FAILED", -132, "Failed to send the message")
SDB_CAT_NO_NODEGROUP_INFO = Errcode("SDB_CAT_NO_NODEGROUP_INFO", -133, "No activated group information on catalog")
SDB_COORD_REMOTE_DISC = Errcode("SDB_COORD_REMOTE_DISC", -134, "Remote-node is disconnected")
SDB_CAT_NO_MATCH_CATALOG = Errcode("SDB_CAT_NO_MATCH_CATALOG", -135, "Unable to find the matched catalog information")
SDB_CLS_UPDATE_CAT_FAILED = Errcode("SDB_CLS_UPDATE_CAT_FAILED", -136, "Failed to update catalog")
SDB_COORD_UNKNOWN_OP_REQ = Errcode("SDB_COORD_UNKNOWN_OP_REQ", -137, "Unknown request operation code")
SDB_COOR_NO_NODEGROUP_INFO = Errcode("SDB_COOR_NO_NODEGROUP_INFO", -138, "Group information cannot be found on coordinator node")
SDB_DMS_CORRUPTED_EXTENT = Errcode("SDB_DMS_CORRUPTED_EXTENT", -139, "DMS extent is corrupted")
SDBCM_FAIL = Errcode("SDBCM_FAIL", -140, "Remote cluster manager failed")
SDBCM_STOP_PART = Errcode("SDBCM_STOP_PART", -141, "Remote database services have been stopped")
SDBCM_SVC_STARTING = Errcode("SDBCM_SVC_STARTING", -142, "Service is starting")
SDBCM_SVC_STARTED = Errcode("SDBCM_SVC_STARTED", -143, "Service has been started")
SDBCM_SVC_RESTARTING = Errcode("SDBCM_SVC_RESTARTING", -144, "Service is restarting")
SDBCM_NODE_EXISTED = Errcode("SDBCM_NODE_EXISTED", -145, "Node already exists")
SDBCM_NODE_NOTEXISTED = Errcode("SDBCM_NODE_NOTEXISTED", -146, "Node does not exist")
SDB_LOCK_FAILED = Errcode("SDB_LOCK_FAILED", -147, "Unable to lock")
SDB_DMS_STATE_NOT_COMPATIBLE = Errcode("SDB_DMS_STATE_NOT_COMPATIBLE", -148, "DMS state is not compatible with current command")
SDB_REBUILD_HAS_ALREADY_START = Errcode("SDB_REBUILD_HAS_ALREADY_START", -149, "Database rebuild is already started")
SDB_RTN_IN_REBUILD = Errcode("SDB_RTN_IN_REBUILD", -150, "Database rebuild is in progress")
SDB_RTN_COORD_CACHE_EMPTY = Errcode("SDB_RTN_COORD_CACHE_EMPTY", -151, "Cache is empty on coordinator node")
SDB_SPT_EVAL_FAIL = Errcode("SDB_SPT_EVAL_FAIL", -152, "Evalution failed with error")
SDB_CAT_GRP_EXIST = Errcode("SDB_CAT_GRP_EXIST", -153, "Group already exist")
SDB_CLS_GRP_NOT_EXIST = Errcode("SDB_CLS_GRP_NOT_EXIST", -154, "Group does not exist")
SDB_CLS_NODE_NOT_EXIST = Errcode("SDB_CLS_NODE_NOT_EXIST", -155, "Node does not exist")
SDB_CM_RUN_NODE_FAILED = Errcode("SDB_CM_RUN_NODE_FAILED", -156, "Failed to start the node")
SDB_CM_CONFIG_CONFLICTS = Errcode("SDB_CM_CONFIG_CONFLICTS", -157, "Invalid node configuration")
SDB_CLS_EMPTY_GROUP = Errcode("SDB_CLS_EMPTY_GROUP", -158, "Group is empty")
SDB_RTN_COORD_ONLY = Errcode("SDB_RTN_COORD_ONLY", -159, "The operation is for coord node only")
SDB_CM_OP_NODE_FAILED = Errcode("SDB_CM_OP_NODE_FAILED", -160, "Failed to operate on node")
SDB_RTN_MUTEX_JOB_EXIST = Errcode("SDB_RTN_MUTEX_JOB_EXIST", -161, "The mutex job already exist")
SDB_RTN_JOB_NOT_EXIST = Errcode("SDB_RTN_JOB_NOT_EXIST", -162, "The specified job does not exist")
SDB_CAT_CORRUPTION = Errcode("SDB_CAT_CORRUPTION", -163, "The catalog information is corrupted")
SDB_IXM_DROP_SHARD = Errcode("SDB_IXM_DROP_SHARD", -164, "$shard index can't be dropped")
SDB_RTN_CMD_NO_NODE_AUTH = Errcode("SDB_RTN_CMD_NO_NODE_AUTH", -165, "The command can't be run in the node")
SDB_RTN_CMD_NO_SERVICE_AUTH = Errcode("SDB_RTN_CMD_NO_SERVICE_AUTH", -166, "The command can't be run in the service plane")
SDB_CLS_NO_GROUP_INFO = Errcode("SDB_CLS_NO_GROUP_INFO", -167, "The group info not exist")
SDB_CLS_GROUP_NAME_CONFLICT = Errcode("SDB_CLS_GROUP_NAME_CONFLICT", -168, "Group name is conflict")
SDB_COLLECTION_NOTSHARD = Errcode("SDB_COLLECTION_NOTSHARD", -169, "The collection is not sharded")
SDB_INVALID_SHARDINGKEY = Errcode("SDB_INVALID_SHARDINGKEY", -170, "The record does not contains valid sharding key")
SDB_TASK_EXIST = Errcode("SDB_TASK_EXIST", -171, "A task that already exists does not compatible with the new task")
SDB_CL_NOT_EXIST_ON_GROUP = Errcode("SDB_CL_NOT_EXIST_ON_GROUP", -172, "The collection does not exists on the specified group")
SDB_CAT_TASK_NOTFOUND = Errcode("SDB_CAT_TASK_NOTFOUND", -173, "The specified task does not exist")
SDB_MULTI_SHARDING_KEY = Errcode("SDB_MULTI_SHARDING_KEY", -174, "The record contains more than one sharding key")
SDB_CLS_MUTEX_TASK_EXIST = Errcode("SDB_CLS_MUTEX_TASK_EXIST", -175, "The mutex task already exist")
SDB_CLS_BAD_SPLIT_KEY = Errcode("SDB_CLS_BAD_SPLIT_KEY", -176, "The split key is not valid or not in the source group")
SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY = Errcode("SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY", -177, "The unique index must include all fields in sharding key")
SDB_UPDATE_SHARD_KEY = Errcode("SDB_UPDATE_SHARD_KEY", -178, "Sharding key cannot be updated")
SDB_AUTH_AUTHORITY_FORBIDDEN = Errcode("SDB_AUTH_AUTHORITY_FORBIDDEN", -179, "Authority is forbidden")
SDB_CAT_NO_ADDR_LIST = Errcode("SDB_CAT_NO_ADDR_LIST", -180, "There is no catalog address specified by user")
SDB_CURRENT_RECORD_DELETED = Errcode("SDB_CURRENT_RECORD_DELETED", -181, "Current record has been removed")
SDB_QGM_MATCH_NONE = Errcode("SDB_QGM_MATCH_NONE", -182, "No records can be matched for the search condition")
SDB_IXM_REORG_DONE = Errcode("SDB_IXM_REORG_DONE", -183, "Index page is reorged and the pos got different lchild")
SDB_RTN_DUPLICATE_FIELDNAME = Errcode("SDB_RTN_DUPLICATE_FIELDNAME", -184, "Duplicate field name exists in the record")
SDB_QGM_MAX_NUM_RECORD = Errcode("SDB_QGM_MAX_NUM_RECORD", -185, "Too many records to be inserted at once")
SDB_QGM_MERGE_JOIN_EQONLY = Errcode("SDB_QGM_MERGE_JOIN_EQONLY", -186, "Sort-Merge Join only supports equal predicates")
SDB_PD_TRACE_IS_STARTED = Errcode("SDB_PD_TRACE_IS_STARTED", -187, "Trace is already started")
SDB_PD_TRACE_HAS_NO_BUFFER = Errcode("SDB_PD_TRACE_HAS_NO_BUFFER", -188, "Trace buffer does not exist")
SDB_PD_TRACE_FILE_INVALID = Errcode("SDB_PD_TRACE_FILE_INVALID", -189, "Trace file is not valid")
SDB_DPS_TRANS_LOCK_INCOMPATIBLE = Errcode("SDB_DPS_TRANS_LOCK_INCOMPATIBLE", -190, "Incompatible lock")
SDB_DPS_TRANS_DOING_ROLLBACK = Errcode("SDB_DPS_TRANS_DOING_ROLLBACK", -191, "Rollback operation is in progress")
SDB_MIG_IMP_BAD_RECORD = Errcode("SDB_MIG_IMP_BAD_RECORD", -192, "Invalid record is found during import")
SDB_QGM_REPEAT_VAR_NAME = Errcode("SDB_QGM_REPEAT_VAR_NAME", -193, "Repeated variable name")
SDB_QGM_AMBIGUOUS_FIELD = Errcode("SDB_QGM_AMBIGUOUS_FIELD", -194, "Column name is ambiguous")
SDB_SQL_SYNTAX_ERROR = Errcode("SDB_SQL_SYNTAX_ERROR", -195, "SQL syntax error")
SDB_DPS_TRANS_NO_TRANS = Errcode("SDB_DPS_TRANS_NO_TRANS", -196, "Invalid transactional operation")
SDB_DPS_TRANS_APPEND_TO_WAIT = Errcode("SDB_DPS_TRANS_APPEND_TO_WAIT", -197, "Append to lock-wait-queue")
SDB_DMS_DELETING = Errcode("SDB_DMS_DELETING", -198, "Record is deleted")
SDB_DMS_INVALID_INDEXCB = Errcode("SDB_DMS_INVALID_INDEXCB", -199, "Index is dropped or invalid")
SDB_COORD_RECREATE_CATALOG = Errcode("SDB_COORD_RECREATE_CATALOG", -200, "Unable to create new catalog when there's already one exists")
SDB_UTIL_PARSE_JSON_INVALID = Errcode("SDB_UTIL_PARSE_JSON_INVALID", -201, "Failed to parse JSON file")
SDB_UTIL_PARSE_CSV_INVALID = Errcode("SDB_UTIL_PARSE_CSV_INVALID", -202, "Failed to parse CSV file")
SDB_DPS_LOG_FILE_OUT_OF_SIZE = Errcode("SDB_DPS_LOG_FILE_OUT_OF_SIZE", -203, "Log file size is too large")
SDB_CATA_RM_NODE_FORBIDDEN = Errcode("SDB_CATA_RM_NODE_FORBIDDEN", -204, "Unable to remove the last node or primary in a group")
SDB_CATA_FAILED_TO_CLEANUP = Errcode("SDB_CATA_FAILED_TO_CLEANUP", -205, "Unable to clean up catalog, manual cleanup may be required")
SDB_CATA_RM_CATA_FORBIDDEN = Errcode("SDB_CATA_RM_CATA_FORBIDDEN", -206, "Unable to remove primary catalog or catalog group for non-empty database")
SDB_ERROR_RESERVED_2 = Errcode("SDB_ERROR_RESERVED_2", -207, "Reserved")
SDB_CAT_RM_GRP_FORBIDDEN = Errcode("SDB_CAT_RM_GRP_FORBIDDEN", -208, "Unable to remove non-empty group")
SDB_MIG_END_OF_QUEUE = Errcode("SDB_MIG_END_OF_QUEUE", -209, "End of queue")
SDB_COORD_SPLIT_NO_SHDIDX = Errcode("SDB_COORD_SPLIT_NO_SHDIDX", -210, "Unable to split because of no sharding index exists")
SDB_FIELD_NOT_EXIST = Errcode("SDB_FIELD_NOT_EXIST", -211, "The parameter field does not exist")
SDB_TOO_MANY_TRACE_BP = Errcode("SDB_TOO_MANY_TRACE_BP", -212, "Too many break points are specified")
SDB_BUSY_PREFETCHER = Errcode("SDB_BUSY_PREFETCHER", -213, "All prefetchers are busy")
SDB_CAT_DOMAIN_NOT_EXIST = Errcode("SDB_CAT_DOMAIN_NOT_EXIST", -214, "Domain does not exist")
SDB_CAT_DOMAIN_EXIST = Errcode("SDB_CAT_DOMAIN_EXIST", -215, "Domain already exists")
SDB_CAT_GROUP_NOT_IN_DOMAIN = Errcode("SDB_CAT_GROUP_NOT_IN_DOMAIN", -216, "Group is not in domain")
SDB_CLS_SHARDING_NOT_HASH = Errcode("SDB_CLS_SHARDING_NOT_HASH", -217, "Sharding type is not hash")
SDB_CLS_SPLIT_PERCENT_LOWER = Errcode("SDB_CLS_SPLIT_PERCENT_LOWER", -218, "split percentage is lower then expected")
SDB_TASK_ALREADY_FINISHED = Errcode("SDB_TASK_ALREADY_FINISHED", -219, "Task is already finished")
SDB_COLLECTION_LOAD = Errcode("SDB_COLLECTION_LOAD", -220, "Collection is in loading status")
SDB_LOAD_ROLLBACK = Errcode("SDB_LOAD_ROLLBACK", -221, "Rolling back load operation")
SDB_INVALID_ROUTEID = Errcode("SDB_INVALID_ROUTEID", -222, "RouteID is different from the local")
SDB_DUPLICATED_SERVICE = Errcode("SDB_DUPLICATED_SERVICE", -223, "Service already exists")
SDB_UTIL_NOT_FIND_FIELD = Errcode("SDB_UTIL_NOT_FIND_FIELD", -224, "Field is not found")
SDB_UTIL_CSV_FIELD_END = Errcode("SDB_UTIL_CSV_FIELD_END", -225, "csv field line end")
SDB_MIG_UNKNOW_FILE_TYPE = Errcode("SDB_MIG_UNKNOW_FILE_TYPE", -226, "Unknown file type")
SDB_RTN_EXPORTCONF_NOT_COMPLETE = Errcode("SDB_RTN_EXPORTCONF_NOT_COMPLETE", -227, "Exporting configuration does not complete in all nodes")
SDB_CLS_NOTP_AND_NODATA = Errcode("SDB_CLS_NOTP_AND_NODATA", -228, "Empty non-primary node")
SDB_DMS_SECRETVALUE_NOT_SAME = Errcode("SDB_DMS_SECRETVALUE_NOT_SAME", -229, "Secret value for index file does not match with data file")
SDB_PMD_VERSION_ONLY = Errcode("SDB_PMD_VERSION_ONLY", -230, "Engine version argument is specified")
SDB_SDB_HELP_ONLY = Errcode("SDB_SDB_HELP_ONLY", -231, "Help argument is specified")
SDB_SDB_VERSION_ONLY = Errcode("SDB_SDB_VERSION_ONLY", -232, "Version argument is specified")
SDB_FMP_FUNC_NOT_EXIST = Errcode("SDB_FMP_FUNC_NOT_EXIST", -233, "Stored procedure does not exist")
SDB_ILL_RM_SUB_CL = Errcode("SDB_ILL_RM_SUB_CL", -234, "Unable to remove collection partition")
SDB_RELINK_SUB_CL = Errcode("SDB_RELINK_SUB_CL", -235, "Duplicated attach collection partition")
SDB_INVALID_MAIN_CL = Errcode("SDB_INVALID_MAIN_CL", -236, "Invalid partitioned-collection")
SDB_BOUND_CONFLICT = Errcode("SDB_BOUND_CONFLICT", -237, "New boundary is conflict with the existing boundary")
SDB_BOUND_INVALID = Errcode("SDB_BOUND_INVALID", -238, "Invalid boundary for the shard")
SDB_HIT_HIGH_WATERMARK = Errcode("SDB_HIT_HIGH_WATERMARK", -239, "Hit the high water mark")
SDB_BAR_BACKUP_EXIST = Errcode("SDB_BAR_BACKUP_EXIST", -240, "Backup already exists")
SDB_BAR_BACKUP_NOTEXIST = Errcode("SDB_BAR_BACKUP_NOTEXIST", -241, "Backup does not exist")
SDB_INVALID_SUB_CL = Errcode("SDB_INVALID_SUB_CL", -242, "Invalid collection partition")
SDB_TASK_HAS_CANCELED = Errcode("SDB_TASK_HAS_CANCELED", -243, "Task is canceled")
SDB_INVALID_MAIN_CL_TYPE = Errcode("SDB_INVALID_MAIN_CL_TYPE", -244, "Sharding type must be ranged partition for partitioned-collection")
SDB_NO_SHARDINGKEY = Errcode("SDB_NO_SHARDINGKEY", -245, "There is no valid sharding-key defined")
SDB_MAIN_CL_OP_ERR = Errcode("SDB_MAIN_CL_OP_ERR", -246, "Operation is not supported on partitioned-collection")
SDB_IXM_REDEF = Errcode("SDB_IXM_REDEF", -247, "Redefine index")
SDB_DMS_CS_DELETING = Errcode("SDB_DMS_CS_DELETING", -248, "Dropping the collection space is in progress")
SDB_DMS_REACHED_MAX_NODES = Errcode("SDB_DMS_REACHED_MAX_NODES", -249, "Hit the limit of maximum number of nodes in the cluster")
SDB_CLS_NODE_BSFAULT = Errcode("SDB_CLS_NODE_BSFAULT", -250, "The node is not in normal status")
SDB_CLS_NODE_INFO_EXPIRED = Errcode("SDB_CLS_NODE_INFO_EXPIRED", -251, "Node information is expired")
SDB_CLS_WAIT_SYNC_FAILED = Errcode("SDB_CLS_WAIT_SYNC_FAILED", -252, "Failed to wait for the sync operation from secondary nodes")
SDB_DPS_TRANS_DIABLED = Errcode("SDB_DPS_TRANS_DIABLED", -253, "Transaction is disabled")
SDB_DRIVER_DS_RUNOUT = Errcode("SDB_DRIVER_DS_RUNOUT", -254, "Data source is running out of connection pool")
SDB_TOO_MANY_OPEN_FD = Errcode("SDB_TOO_MANY_OPEN_FD", -255, "Too many opened file description")
SDB_DOMAIN_IS_OCCUPIED = Errcode("SDB_DOMAIN_IS_OCCUPIED", -256, "Domain is not empty")
SDB_REST_RECV_SIZE = Errcode("SDB_REST_RECV_SIZE", -257, "The data received by REST is larger than the max size")
SDB_DRIVER_BSON_ERROR = Errcode("SDB_DRIVER_BSON_ERROR", -258, "Failed to build bson object")
SDB_OUT_OF_BOUND = Errcode("SDB_OUT_OF_BOUND", -259, "Stored procedure arguments are out of bound")
SDB_REST_COMMON_UNKNOWN = Errcode("SDB_REST_COMMON_UNKNOWN", -260, "Unknown REST command")
SDB_BUT_FAILED_ON_DATA = Errcode("SDB_BUT_FAILED_ON_DATA", -261, "Failed to execute command on data node")
SDB_CAT_NO_GROUP_IN_DOMAIN = Errcode("SDB_CAT_NO_GROUP_IN_DOMAIN", -262, "The domain is empty")
SDB_OM_PASSWD_CHANGE_SUGGUEST = Errcode("SDB_OM_PASSWD_CHANGE_SUGGUEST", -263, "Changing password is required")
SDB_COORD_NOT_ALL_DONE = Errcode("SDB_COORD_NOT_ALL_DONE", -264, "One or more nodes did not complete successfully")
SDB_OMA_DIFF_VER_AGT_IS_RUNNING = Errcode("SDB_OMA_DIFF_VER_AGT_IS_RUNNING", -265, "There is another OM Agent running with different version")
SDB_OM_TASK_NOT_EXIST = Errcode("SDB_OM_TASK_NOT_EXIST", -266, "Task does not exist")
SDB_OM_TASK_ROLLBACK = Errcode("SDB_OM_TASK_ROLLBACK", -267, "Task is rolling back")
SDB_LOB_SEQUENCE_NOT_EXIST = Errcode("SDB_LOB_SEQUENCE_NOT_EXIST", -268, "LOB sequence does not exist")
SDB_LOB_IS_NOT_AVAILABLE = Errcode("SDB_LOB_IS_NOT_AVAILABLE", -269, "LOB is not useable")
SDB_MIG_DATA_NON_UTF = Errcode("SDB_MIG_DATA_NON_UTF", -270, "Data is not in UTF-8 format")
SDB_OMA_TASK_FAIL = Errcode("SDB_OMA_TASK_FAIL", -271, "Task failed")
SDB_LOB_NOT_OPEN = Errcode("SDB_LOB_NOT_OPEN", -272, "Lob does not open")
SDB_LOB_HAS_OPEN = Errcode("SDB_LOB_HAS_OPEN", -273, "Lob has been open")
SDBCM_NODE_IS_IN_RESTORING = Errcode("SDBCM_NODE_IS_IN_RESTORING", -274, "Node is in restoring")
SDB_DMS_CS_NOT_EMPTY = Errcode("SDB_DMS_CS_NOT_EMPTY", -275, "There are some collections in the collection space")
SDB_CAT_LOCALHOST_CONFLICT = Errcode("SDB_CAT_LOCALHOST_CONFLICT", -276, "'localhost' and '127.0.0.1' cannot be used mixed with other hostname and IP address")
SDB_CAT_NOT_LOCALCONN = Errcode("SDB_CAT_NOT_LOCALCONN", -277, "If use 'localhost' and '127.0.0.1' for hostname, coord and catalog must in the same host ")
SDB_CAT_IS_NOT_DATAGROUP = Errcode("SDB_CAT_IS_NOT_DATAGROUP", -278, "The special group is not data group")
SDB_RTN_AUTOINDEXID_IS_FALSE = Errcode("SDB_RTN_AUTOINDEXID_IS_FALSE", -279, "can not update/delete records when $id index does not exist")
SDB_CLS_CAN_NOT_STEP_UP = Errcode("SDB_CLS_CAN_NOT_STEP_UP", -280, "can not step up when primary node exists or LSN is not the biggest")
SDB_CAT_IMAGE_ADDR_CONFLICT = Errcode("SDB_CAT_IMAGE_ADDR_CONFLICT", -281, "Image address is conflict with the self cluster")
SDB_CAT_GROUP_HASNOT_IMAGE = Errcode("SDB_CAT_GROUP_HASNOT_IMAGE", -282, "The data group does not have image group")
SDB_CAT_GROUP_HAS_IMAGE = Errcode("SDB_CAT_GROUP_HAS_IMAGE", -283, "The data group has image group")
SDB_CAT_IMAGE_IS_ENABLED = Errcode("SDB_CAT_IMAGE_IS_ENABLED", -284, "The image is in enabled status")
SDB_CAT_IMAGE_NOT_CONFIG = Errcode("SDB_CAT_IMAGE_NOT_CONFIG", -285, "The cluster's image does not configured")
SDB_CAT_DUAL_WRITABLE = Errcode("SDB_CAT_DUAL_WRITABLE", -286, "This cluster and image cluster is both writable")
SDB_CAT_CLUSTER_IS_READONLY = Errcode("SDB_CAT_CLUSTER_IS_READONLY", -287, "This cluster is readonly")
SDB_RTN_QUERYMODIFY_SORT_NO_IDX = Errcode("SDB_RTN_QUERYMODIFY_SORT_NO_IDX", -288, "Sorting of 'query and modify' must use index")
SDB_RTN_QUERYMODIFY_MULTI_NODES = Errcode("SDB_RTN_QUERYMODIFY_MULTI_NODES", -289, "'query and modify' can't use skip and limit in multiple nodes or sub-collections")
SDB_DIR_NOT_EMPTY = Errcode("SDB_DIR_NOT_EMPTY", -290, "Given path is not empty")
SDB_IXM_EXIST_COVERD_ONE = Errcode("SDB_IXM_EXIST_COVERD_ONE", -291, "Exist one index which can cover this scene")
SDB_CAT_IMAGE_IS_CONFIGURED = Errcode("SDB_CAT_IMAGE_IS_CONFIGURED", -292, "The cluster's image has already configured")
SDB_RTN_CMD_IN_LOCAL_MODE = Errcode("SDB_RTN_CMD_IN_LOCAL_MODE", -293, "The command is in local mode")
SDB_SPT_NOT_SPECIAL_JSON = Errcode("SDB_SPT_NOT_SPECIAL_JSON", -294, "The object is not a special object in sdb shell")
SDB_AUTH_USER_ALREADY_EXIST = Errcode("SDB_AUTH_USER_ALREADY_EXIST", -295, "The specified user already exist")
SDB_DMS_EMPTY_COLLECTION = Errcode("SDB_DMS_EMPTY_COLLECTION", -296, "The collection is empty")
SDB_LOB_SEQUENCE_EXISTS = Errcode("SDB_LOB_SEQUENCE_EXISTS", -297, "LOB sequence exists")
SDB_OM_CLUSTER_NOT_EXIST = Errcode("SDB_OM_CLUSTER_NOT_EXIST", -298, "cluster do not exist")
SDB_OM_BUSINESS_NOT_EXIST = Errcode("SDB_OM_BUSINESS_NOT_EXIST", -299, "business do not exist")
SDB_AUTH_USER_NOT_EXIST = Errcode("SDB_AUTH_USER_NOT_EXIST", -300, "user specified is not exist or password is invalid")
SDB_UTIL_COMPRESS_INIT_FAIL = Errcode("SDB_UTIL_COMPRESS_INIT_FAIL", -301, "Compression initialization failed")
SDB_UTIL_COMPRESS_FAIL = Errcode("SDB_UTIL_COMPRESS_FAIL", -302, "Compression failed")
SDB_UTIL_DECOMPRESS_FAIL = Errcode("SDB_UTIL_DECOMPRESS_FAIL", -303, "Decompression failed")
SDB_UTIL_COMPRESS_ABORT = Errcode("SDB_UTIL_COMPRESS_ABORT", -304, "Compression abort")
SDB_UTIL_COMPRESS_BUFF_SMALL = Errcode("SDB_UTIL_COMPRESS_BUFF_SMALL", -305, "Buffer for compression is too small")
SDB_UTIL_DECOMPRESS_BUFF_SMALL = Errcode("SDB_UTIL_DECOMPRESS_BUFF_SMALL", -306, "Buffer for decompression is too small")
SDB_OSS_UP_TO_LIMIT = Errcode("SDB_OSS_UP_TO_LIMIT", -307, "Up to the limit")
SDB_DS_NOT_ENABLE = Errcode("SDB_DS_NOT_ENABLE", -308, "data source is not enabled yet")
SDB_DS_NO_REACHABLE_COORD = Errcode("SDB_DS_NO_REACHABLE_COORD", -309, "No reachable coord notes")
SDB_RULE_ID_IS_NOT_EXIST = Errcode("SDB_RULE_ID_IS_NOT_EXIST", -310, "the record which exclusive ruleID is not exist")
SDB_STRTGY_TASK_NAME_CONFLICT = Errcode("SDB_STRTGY_TASK_NAME_CONFLICT", -311, "Task name conflict")
SDB_STRTGY_TASK_NOT_EXISTED = Errcode("SDB_STRTGY_TASK_NOT_EXISTED", -312, "The task is not existed")
SDB_DPS_LOG_NOT_ARCHIVED = Errcode("SDB_DPS_LOG_NOT_ARCHIVED", -313, "Replica log is not archived")
SDB_DS_NOT_INIT = Errcode("SDB_DS_NOT_INIT", -314, "Data source has not been initialized")
SDB_OPERATION_INCOMPATIBLE = Errcode("SDB_OPERATION_INCOMPATIBLE", -315, "Operation is incompatible with the object")
SDB_CAT_CLUSTER_IS_DEACTIVED = Errcode("SDB_CAT_CLUSTER_IS_DEACTIVED", -316, "This cluster is deactived")
SDB_LOB_IS_IN_USE = Errcode("SDB_LOB_IS_IN_USE", -317, "LOB is in use")
SDB_VALUE_OVERFLOW = Errcode("SDB_VALUE_OVERFLOW", -318, "Data operation is overflowed")
SDB_LOB_PIECESINFO_OVERFLOW = Errcode("SDB_LOB_PIECESINFO_OVERFLOW", -319, "LOB's pieces info is overflowed")
SDB_LOB_LOCK_CONFLICTED = Errcode("SDB_LOB_LOCK_CONFLICTED", -320, "LOB lock is conflicted")

_errcode_map = {
    -1: SDB_IO,
    -2: SDB_OOM,
    -3: SDB_PERM,
    -4: SDB_FNE,
    -5: SDB_FE,
    -6: SDB_INVALIDARG,
    -7: SDB_INVALIDSIZE,
    -8: SDB_INTERRUPT,
    -9: SDB_EOF,
    -10: SDB_SYS,
    -11: SDB_NOSPC,
    -12: SDB_EDU_INVAL_STATUS,
    -13: SDB_TIMEOUT,
    -14: SDB_QUIESCED,
    -15: SDB_NETWORK,
    -16: SDB_NETWORK_CLOSE,
    -17: SDB_DATABASE_DOWN,
    -18: SDB_APP_FORCED,
    -19: SDB_INVALIDPATH,
    -20: SDB_INVALID_FILE_TYPE,
    -21: SDB_DMS_NOSPC,
    -22: SDB_DMS_EXIST,
    -23: SDB_DMS_NOTEXIST,
    -24: SDB_DMS_RECORD_TOO_BIG,
    -25: SDB_DMS_RECORD_NOTEXIST,
    -26: SDB_DMS_OVF_EXIST,
    -27: SDB_DMS_RECORD_INVALID,
    -28: SDB_DMS_SU_NEED_REORG,
    -29: SDB_DMS_EOC,
    -30: SDB_DMS_CONTEXT_IS_OPEN,
    -31: SDB_DMS_CONTEXT_IS_CLOSE,
    -32: SDB_OPTION_NOT_SUPPORT,
    -33: SDB_DMS_CS_EXIST,
    -34: SDB_DMS_CS_NOTEXIST,
    -35: SDB_DMS_INVALID_SU,
    -36: SDB_RTN_CONTEXT_NOTEXIST,
    -37: SDB_IXM_MULTIPLE_ARRAY,
    -38: SDB_IXM_DUP_KEY,
    -39: SDB_IXM_KEY_TOO_LARGE,
    -40: SDB_IXM_NOSPC,
    -41: SDB_IXM_KEY_NOTEXIST,
    -42: SDB_DMS_MAX_INDEX,
    -43: SDB_DMS_INIT_INDEX,
    -44: SDB_DMS_COL_DROPPED,
    -45: SDB_IXM_IDENTICAL_KEY,
    -46: SDB_IXM_EXIST,
    -47: SDB_IXM_NOTEXIST,
    -48: SDB_IXM_UNEXPECTED_STATUS,
    -49: SDB_IXM_EOC,
    -50: SDB_IXM_DEDUP_BUF_MAX,
    -51: SDB_RTN_INVALID_PREDICATES,
    -52: SDB_RTN_INDEX_NOTEXIST,
    -53: SDB_RTN_INVALID_HINT,
    -54: SDB_DMS_NO_MORE_TEMP,
    -55: SDB_DMS_SU_OUTRANGE,
    -56: SDB_IXM_DROP_ID,
    -57: SDB_DPS_LOG_NOT_IN_BUF,
    -58: SDB_DPS_LOG_NOT_IN_FILE,
    -59: SDB_PMD_RG_NOT_EXIST,
    -60: SDB_PMD_RG_EXIST,
    -61: SDB_INVALID_REQID,
    -62: SDB_PMD_SESSION_NOT_EXIST,
    -63: SDB_PMD_FORCE_SYSTEM_EDU,
    -64: SDB_NOT_CONNECTED,
    -65: SDB_UNEXPECTED_RESULT,
    -66: SDB_CORRUPTED_RECORD,
    -67: SDB_BACKUP_HAS_ALREADY_START,
    -68: SDB_BACKUP_NOT_COMPLETE,
    -69: SDB_RTN_IN_BACKUP,
    -70: SDB_BAR_DAMAGED_BK_FILE,
    -71: SDB_RTN_NO_PRIMARY_FOUND,
    -72: SDB_ERROR_RESERVED_1,
    -73: SDB_PMD_HELP_ONLY,
    -74: SDB_PMD_CON_INVALID_STATE,
    -75: SDB_CLT_INVALID_HANDLE,
    -76: SDB_CLT_OBJ_NOT_EXIST,
    -77: SDB_NET_ALREADY_LISTENED,
    -78: SDB_NET_CANNOT_LISTEN,
    -79: SDB_NET_CANNOT_CONNECT,
    -80: SDB_NET_NOT_CONNECT,
    -81: SDB_NET_SEND_ERR,
    -82: SDB_NET_TIMER_ID_NOT_FOUND,
    -83: SDB_NET_ROUTE_NOT_FOUND,
    -84: SDB_NET_BROKEN_MSG,
    -85: SDB_NET_INVALID_HANDLE,
    -86: SDB_DMS_INVALID_REORG_FILE,
    -87: SDB_DMS_REORG_FILE_READONLY,
    -88: SDB_DMS_INVALID_COLLECTION_S,
    -89: SDB_DMS_NOT_IN_REORG,
    -90: SDB_REPL_GROUP_NOT_ACTIVE,
    -91: SDB_REPL_INVALID_GROUP_MEMBER,
    -92: SDB_DMS_INCOMPATIBLE_MODE,
    -93: SDB_DMS_INCOMPATIBLE_VERSION,
    -94: SDB_REPL_LOCAL_G_V_EXPIRED,
    -95: SDB_DMS_INVALID_PAGESIZE,
    -96: SDB_REPL_REMOTE_G_V_EXPIRED,
    -97: SDB_CLS_VOTE_FAILED,
    -98: SDB_DPS_CORRUPTED_LOG,
    -99: SDB_DPS_LSN_OUTOFRANGE,
    -100: SDB_UNKNOWN_MESSAGE,
    -101: SDB_NET_UPDATE_EXISTING_NODE,
    -102: SDB_CLS_UNKNOW_MSG,
    -103: SDB_CLS_EMPTY_HEAP,
    -104: SDB_CLS_NOT_PRIMARY,
    -105: SDB_CLS_NODE_NOT_ENOUGH,
    -106: SDB_CLS_NO_CATALOG_INFO,
    -107: SDB_CLS_DATA_NODE_CAT_VER_OLD,
    -108: SDB_CLS_COORD_NODE_CAT_VER_OLD,
    -109: SDB_CLS_INVALID_GROUP_NUM,
    -110: SDB_CLS_SYNC_FAILED,
    -111: SDB_CLS_REPLAY_LOG_FAILED,
    -112: SDB_REST_EHS,
    -113: SDB_CLS_CONSULT_FAILED,
    -114: SDB_DPS_MOVE_FAILED,
    -115: SDB_DMS_CORRUPTED_SME,
    -116: SDB_APP_INTERRUPT,
    -117: SDB_APP_DISCONNECT,
    -118: SDB_OSS_CCE,
    -119: SDB_COORD_QUERY_FAILED,
    -120: SDB_CLS_BUFFER_FULL,
    -121: SDB_RTN_SUBCONTEXT_CONFLICT,
    -122: SDB_COORD_QUERY_EOC,
    -123: SDB_DPS_FILE_SIZE_NOT_SAME,
    -124: SDB_DPS_FILE_NOT_RECOGNISE,
    -125: SDB_OSS_NORES,
    -126: SDB_DPS_INVALID_LSN,
    -127: SDB_OSS_NPIPE_DATA_TOO_BIG,
    -128: SDB_CAT_AUTH_FAILED,
    -129: SDB_CLS_FULL_SYNC,
    -130: SDB_CAT_ASSIGN_NODE_FAILED,
    -131: SDB_PHP_DRIVER_INTERNAL_ERROR,
    -132: SDB_COORD_SEND_MSG_FAILED,
    -133: SDB_CAT_NO_NODEGROUP_INFO,
    -134: SDB_COORD_REMOTE_DISC,
    -135: SDB_CAT_NO_MATCH_CATALOG,
    -136: SDB_CLS_UPDATE_CAT_FAILED,
    -137: SDB_COORD_UNKNOWN_OP_REQ,
    -138: SDB_COOR_NO_NODEGROUP_INFO,
    -139: SDB_DMS_CORRUPTED_EXTENT,
    -140: SDBCM_FAIL,
    -141: SDBCM_STOP_PART,
    -142: SDBCM_SVC_STARTING,
    -143: SDBCM_SVC_STARTED,
    -144: SDBCM_SVC_RESTARTING,
    -145: SDBCM_NODE_EXISTED,
    -146: SDBCM_NODE_NOTEXISTED,
    -147: SDB_LOCK_FAILED,
    -148: SDB_DMS_STATE_NOT_COMPATIBLE,
    -149: SDB_REBUILD_HAS_ALREADY_START,
    -150: SDB_RTN_IN_REBUILD,
    -151: SDB_RTN_COORD_CACHE_EMPTY,
    -152: SDB_SPT_EVAL_FAIL,
    -153: SDB_CAT_GRP_EXIST,
    -154: SDB_CLS_GRP_NOT_EXIST,
    -155: SDB_CLS_NODE_NOT_EXIST,
    -156: SDB_CM_RUN_NODE_FAILED,
    -157: SDB_CM_CONFIG_CONFLICTS,
    -158: SDB_CLS_EMPTY_GROUP,
    -159: SDB_RTN_COORD_ONLY,
    -160: SDB_CM_OP_NODE_FAILED,
    -161: SDB_RTN_MUTEX_JOB_EXIST,
    -162: SDB_RTN_JOB_NOT_EXIST,
    -163: SDB_CAT_CORRUPTION,
    -164: SDB_IXM_DROP_SHARD,
    -165: SDB_RTN_CMD_NO_NODE_AUTH,
    -166: SDB_RTN_CMD_NO_SERVICE_AUTH,
    -167: SDB_CLS_NO_GROUP_INFO,
    -168: SDB_CLS_GROUP_NAME_CONFLICT,
    -169: SDB_COLLECTION_NOTSHARD,
    -170: SDB_INVALID_SHARDINGKEY,
    -171: SDB_TASK_EXIST,
    -172: SDB_CL_NOT_EXIST_ON_GROUP,
    -173: SDB_CAT_TASK_NOTFOUND,
    -174: SDB_MULTI_SHARDING_KEY,
    -175: SDB_CLS_MUTEX_TASK_EXIST,
    -176: SDB_CLS_BAD_SPLIT_KEY,
    -177: SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY,
    -178: SDB_UPDATE_SHARD_KEY,
    -179: SDB_AUTH_AUTHORITY_FORBIDDEN,
    -180: SDB_CAT_NO_ADDR_LIST,
    -181: SDB_CURRENT_RECORD_DELETED,
    -182: SDB_QGM_MATCH_NONE,
    -183: SDB_IXM_REORG_DONE,
    -184: SDB_RTN_DUPLICATE_FIELDNAME,
    -185: SDB_QGM_MAX_NUM_RECORD,
    -186: SDB_QGM_MERGE_JOIN_EQONLY,
    -187: SDB_PD_TRACE_IS_STARTED,
    -188: SDB_PD_TRACE_HAS_NO_BUFFER,
    -189: SDB_PD_TRACE_FILE_INVALID,
    -190: SDB_DPS_TRANS_LOCK_INCOMPATIBLE,
    -191: SDB_DPS_TRANS_DOING_ROLLBACK,
    -192: SDB_MIG_IMP_BAD_RECORD,
    -193: SDB_QGM_REPEAT_VAR_NAME,
    -194: SDB_QGM_AMBIGUOUS_FIELD,
    -195: SDB_SQL_SYNTAX_ERROR,
    -196: SDB_DPS_TRANS_NO_TRANS,
    -197: SDB_DPS_TRANS_APPEND_TO_WAIT,
    -198: SDB_DMS_DELETING,
    -199: SDB_DMS_INVALID_INDEXCB,
    -200: SDB_COORD_RECREATE_CATALOG,
    -201: SDB_UTIL_PARSE_JSON_INVALID,
    -202: SDB_UTIL_PARSE_CSV_INVALID,
    -203: SDB_DPS_LOG_FILE_OUT_OF_SIZE,
    -204: SDB_CATA_RM_NODE_FORBIDDEN,
    -205: SDB_CATA_FAILED_TO_CLEANUP,
    -206: SDB_CATA_RM_CATA_FORBIDDEN,
    -207: SDB_ERROR_RESERVED_2,
    -208: SDB_CAT_RM_GRP_FORBIDDEN,
    -209: SDB_MIG_END_OF_QUEUE,
    -210: SDB_COORD_SPLIT_NO_SHDIDX,
    -211: SDB_FIELD_NOT_EXIST,
    -212: SDB_TOO_MANY_TRACE_BP,
    -213: SDB_BUSY_PREFETCHER,
    -214: SDB_CAT_DOMAIN_NOT_EXIST,
    -215: SDB_CAT_DOMAIN_EXIST,
    -216: SDB_CAT_GROUP_NOT_IN_DOMAIN,
    -217: SDB_CLS_SHARDING_NOT_HASH,
    -218: SDB_CLS_SPLIT_PERCENT_LOWER,
    -219: SDB_TASK_ALREADY_FINISHED,
    -220: SDB_COLLECTION_LOAD,
    -221: SDB_LOAD_ROLLBACK,
    -222: SDB_INVALID_ROUTEID,
    -223: SDB_DUPLICATED_SERVICE,
    -224: SDB_UTIL_NOT_FIND_FIELD,
    -225: SDB_UTIL_CSV_FIELD_END,
    -226: SDB_MIG_UNKNOW_FILE_TYPE,
    -227: SDB_RTN_EXPORTCONF_NOT_COMPLETE,
    -228: SDB_CLS_NOTP_AND_NODATA,
    -229: SDB_DMS_SECRETVALUE_NOT_SAME,
    -230: SDB_PMD_VERSION_ONLY,
    -231: SDB_SDB_HELP_ONLY,
    -232: SDB_SDB_VERSION_ONLY,
    -233: SDB_FMP_FUNC_NOT_EXIST,
    -234: SDB_ILL_RM_SUB_CL,
    -235: SDB_RELINK_SUB_CL,
    -236: SDB_INVALID_MAIN_CL,
    -237: SDB_BOUND_CONFLICT,
    -238: SDB_BOUND_INVALID,
    -239: SDB_HIT_HIGH_WATERMARK,
    -240: SDB_BAR_BACKUP_EXIST,
    -241: SDB_BAR_BACKUP_NOTEXIST,
    -242: SDB_INVALID_SUB_CL,
    -243: SDB_TASK_HAS_CANCELED,
    -244: SDB_INVALID_MAIN_CL_TYPE,
    -245: SDB_NO_SHARDINGKEY,
    -246: SDB_MAIN_CL_OP_ERR,
    -247: SDB_IXM_REDEF,
    -248: SDB_DMS_CS_DELETING,
    -249: SDB_DMS_REACHED_MAX_NODES,
    -250: SDB_CLS_NODE_BSFAULT,
    -251: SDB_CLS_NODE_INFO_EXPIRED,
    -252: SDB_CLS_WAIT_SYNC_FAILED,
    -253: SDB_DPS_TRANS_DIABLED,
    -254: SDB_DRIVER_DS_RUNOUT,
    -255: SDB_TOO_MANY_OPEN_FD,
    -256: SDB_DOMAIN_IS_OCCUPIED,
    -257: SDB_REST_RECV_SIZE,
    -258: SDB_DRIVER_BSON_ERROR,
    -259: SDB_OUT_OF_BOUND,
    -260: SDB_REST_COMMON_UNKNOWN,
    -261: SDB_BUT_FAILED_ON_DATA,
    -262: SDB_CAT_NO_GROUP_IN_DOMAIN,
    -263: SDB_OM_PASSWD_CHANGE_SUGGUEST,
    -264: SDB_COORD_NOT_ALL_DONE,
    -265: SDB_OMA_DIFF_VER_AGT_IS_RUNNING,
    -266: SDB_OM_TASK_NOT_EXIST,
    -267: SDB_OM_TASK_ROLLBACK,
    -268: SDB_LOB_SEQUENCE_NOT_EXIST,
    -269: SDB_LOB_IS_NOT_AVAILABLE,
    -270: SDB_MIG_DATA_NON_UTF,
    -271: SDB_OMA_TASK_FAIL,
    -272: SDB_LOB_NOT_OPEN,
    -273: SDB_LOB_HAS_OPEN,
    -274: SDBCM_NODE_IS_IN_RESTORING,
    -275: SDB_DMS_CS_NOT_EMPTY,
    -276: SDB_CAT_LOCALHOST_CONFLICT,
    -277: SDB_CAT_NOT_LOCALCONN,
    -278: SDB_CAT_IS_NOT_DATAGROUP,
    -279: SDB_RTN_AUTOINDEXID_IS_FALSE,
    -280: SDB_CLS_CAN_NOT_STEP_UP,
    -281: SDB_CAT_IMAGE_ADDR_CONFLICT,
    -282: SDB_CAT_GROUP_HASNOT_IMAGE,
    -283: SDB_CAT_GROUP_HAS_IMAGE,
    -284: SDB_CAT_IMAGE_IS_ENABLED,
    -285: SDB_CAT_IMAGE_NOT_CONFIG,
    -286: SDB_CAT_DUAL_WRITABLE,
    -287: SDB_CAT_CLUSTER_IS_READONLY,
    -288: SDB_RTN_QUERYMODIFY_SORT_NO_IDX,
    -289: SDB_RTN_QUERYMODIFY_MULTI_NODES,
    -290: SDB_DIR_NOT_EMPTY,
    -291: SDB_IXM_EXIST_COVERD_ONE,
    -292: SDB_CAT_IMAGE_IS_CONFIGURED,
    -293: SDB_RTN_CMD_IN_LOCAL_MODE,
    -294: SDB_SPT_NOT_SPECIAL_JSON,
    -295: SDB_AUTH_USER_ALREADY_EXIST,
    -296: SDB_DMS_EMPTY_COLLECTION,
    -297: SDB_LOB_SEQUENCE_EXISTS,
    -298: SDB_OM_CLUSTER_NOT_EXIST,
    -299: SDB_OM_BUSINESS_NOT_EXIST,
    -300: SDB_AUTH_USER_NOT_EXIST,
    -301: SDB_UTIL_COMPRESS_INIT_FAIL,
    -302: SDB_UTIL_COMPRESS_FAIL,
    -303: SDB_UTIL_DECOMPRESS_FAIL,
    -304: SDB_UTIL_COMPRESS_ABORT,
    -305: SDB_UTIL_COMPRESS_BUFF_SMALL,
    -306: SDB_UTIL_DECOMPRESS_BUFF_SMALL,
    -307: SDB_OSS_UP_TO_LIMIT,
    -308: SDB_DS_NOT_ENABLE,
    -309: SDB_DS_NO_REACHABLE_COORD,
    -310: SDB_RULE_ID_IS_NOT_EXIST,
    -311: SDB_STRTGY_TASK_NAME_CONFLICT,
    -312: SDB_STRTGY_TASK_NOT_EXISTED,
    -313: SDB_DPS_LOG_NOT_ARCHIVED,
    -314: SDB_DS_NOT_INIT,
    -315: SDB_OPERATION_INCOMPATIBLE,
    -316: SDB_CAT_CLUSTER_IS_DEACTIVED,
    -317: SDB_LOB_IS_IN_USE,
    -318: SDB_VALUE_OVERFLOW,
    -319: SDB_LOB_PIECESINFO_OVERFLOW,
    -320: SDB_LOB_LOCK_CONFLICTED
}


def get_errcode(code):
    return _errcode_map.get(code)
