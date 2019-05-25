/*    Copyright (C) 2011-2017 SequoiaDB Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


package com.sequoiadb.exception;

public enum SDBError {
    SDB_IO(-1, "IO Exception"),
    SDB_OOM(-2, "Out of Memory"),
    SDB_PERM(-3, "Permission Error"),
    SDB_FNE(-4, "File Not Exist"),
    SDB_FE(-5, "File Exist"),
    SDB_INVALIDARG(-6, "Invalid Argument"),
    SDB_INVALIDSIZE(-7, "Invalid size"),
    SDB_INTERRUPT(-8, "Interrupt"),
    SDB_EOF(-9, "Hit end of file"),
    SDB_SYS(-10, "System error"),
    SDB_NOSPC(-11, "No space is left on disk"),
    SDB_EDU_INVAL_STATUS(-12, "EDU status is not valid"),
    SDB_TIMEOUT(-13, "Timeout error"),
    SDB_QUIESCED(-14, "Database is quiesced"),
    SDB_NETWORK(-15, "Network error"),
    SDB_NETWORK_CLOSE(-16, "Network is closed from remote"),
    SDB_DATABASE_DOWN(-17, "Database is in shutdown status"),
    SDB_APP_FORCED(-18, "Application is forced"),
    SDB_INVALIDPATH(-19, "Given path is not valid"),
    SDB_INVALID_FILE_TYPE(-20, "Unexpected file type specified"),
    SDB_DMS_NOSPC(-21, "There's no space for DMS"),
    SDB_DMS_EXIST(-22, "Collection already exists"),
    SDB_DMS_NOTEXIST(-23, "Collection does not exist"),
    SDB_DMS_RECORD_TOO_BIG(-24, "User record is too large"),
    SDB_DMS_RECORD_NOTEXIST(-25, "Record does not exist"),
    SDB_DMS_OVF_EXIST(-26, "Remote overflow record exists"),
    SDB_DMS_RECORD_INVALID(-27, "Invalid record"),
    SDB_DMS_SU_NEED_REORG(-28, "Storage unit need reorg"),
    SDB_DMS_EOC(-29, "End of collection"),
    SDB_DMS_CONTEXT_IS_OPEN(-30, "Context is already opened"),
    SDB_DMS_CONTEXT_IS_CLOSE(-31, "Context is closed"),
    SDB_OPTION_NOT_SUPPORT(-32, "Option is not supported yet"),
    SDB_DMS_CS_EXIST(-33, "Collection space already exists"),
    SDB_DMS_CS_NOTEXIST(-34, "Collection space does not exist"),
    SDB_DMS_INVALID_SU(-35, "Storage unit file is invalid"),
    SDB_RTN_CONTEXT_NOTEXIST(-36, "Context does not exist"),
    SDB_IXM_MULTIPLE_ARRAY(-37, "More than one fields are array type"),
    SDB_IXM_DUP_KEY(-38, "Duplicate key exist"),
    SDB_IXM_KEY_TOO_LARGE(-39, "Index key is too large"),
    SDB_IXM_NOSPC(-40, "No space can be found for index extent"),
    SDB_IXM_KEY_NOTEXIST(-41, "Index key does not exist"),
    SDB_DMS_MAX_INDEX(-42, "Hit max number of index"),
    SDB_DMS_INIT_INDEX(-43, "Failed to initialize index"),
    SDB_DMS_COL_DROPPED(-44, "Collection is dropped"),
    SDB_IXM_IDENTICAL_KEY(-45, "Two records get same key and rid"),
    SDB_IXM_EXIST(-46, "Duplicate index name"),
    SDB_IXM_NOTEXIST(-47, "Index name does not exist"),
    SDB_IXM_UNEXPECTED_STATUS(-48, "Unexpected index flag"),
    SDB_IXM_EOC(-49, "Hit end of index"),
    SDB_IXM_DEDUP_BUF_MAX(-50, "Hit the max of dedup buffer"),
    SDB_RTN_INVALID_PREDICATES(-51, "Invalid predicates"),
    SDB_RTN_INDEX_NOTEXIST(-52, "Index does not exist"),
    SDB_RTN_INVALID_HINT(-53, "Invalid hint"),
    SDB_DMS_NO_MORE_TEMP(-54, "No more temp collections are avaliable"),
    SDB_DMS_SU_OUTRANGE(-55, "Exceed max number of storage unit"),
    SDB_IXM_DROP_ID(-56, "$id index can't be dropped"),
    SDB_DPS_LOG_NOT_IN_BUF(-57, "Log was not found in log buf"),
    SDB_DPS_LOG_NOT_IN_FILE(-58, "Log was not found in log file"),
    SDB_PMD_RG_NOT_EXIST(-59, "Replication group does not exist"),
    SDB_PMD_RG_EXIST(-60, "Replication group exists"),
    SDB_INVALID_REQID(-61, "Invalid request id is received"),
    SDB_PMD_SESSION_NOT_EXIST(-62, "Session ID does not exist"),
    SDB_PMD_FORCE_SYSTEM_EDU(-63, "System EDU cannot be forced"),
    SDB_NOT_CONNECTED(-64, "Database is not connected"),
    SDB_UNEXPECTED_RESULT(-65, "Unexpected result received"),
    SDB_CORRUPTED_RECORD(-66, "Corrupted record"),
    SDB_BACKUP_HAS_ALREADY_START(-67, "Backup has already been started"),
    SDB_BACKUP_NOT_COMPLETE(-68, "Backup is not completed"),
    SDB_RTN_IN_BACKUP(-69, "Backup is in progress"),
    SDB_BAR_DAMAGED_BK_FILE(-70, "Backup is corrupted"),
    SDB_RTN_NO_PRIMARY_FOUND(-71, "No primary node was found"),
    SDB_ERROR_RESERVED_1(-72, "Reserved"),
    SDB_PMD_HELP_ONLY(-73, "Engine help argument is specified"),
    SDB_PMD_CON_INVALID_STATE(-74, "Invalid connection state"),
    SDB_CLT_INVALID_HANDLE(-75, "Invalid handle"),
    SDB_CLT_OBJ_NOT_EXIST(-76, "Object does not exist"),
    SDB_NET_ALREADY_LISTENED(-77, "Listening port is already occupied"),
    SDB_NET_CANNOT_LISTEN(-78, "Unable to listen the specified address"),
    SDB_NET_CANNOT_CONNECT(-79, "Unable to connect to the specified address"),
    SDB_NET_NOT_CONNECT(-80, "Connection does not exist"),
    SDB_NET_SEND_ERR(-81, "Failed to send"),
    SDB_NET_TIMER_ID_NOT_FOUND(-82, "Timer does not exist"),
    SDB_NET_ROUTE_NOT_FOUND(-83, "Route info does not exist"),
    SDB_NET_BROKEN_MSG(-84, "Broken msg"),
    SDB_NET_INVALID_HANDLE(-85, "Invalid net handle"),
    SDB_DMS_INVALID_REORG_FILE(-86, "Invalid reorg file"),
    SDB_DMS_REORG_FILE_READONLY(-87, "Reorg file is in read only mode"),
    SDB_DMS_INVALID_COLLECTION_S(-88, "Collection status is not valid"),
    SDB_DMS_NOT_IN_REORG(-89, "Collection is not in reorg state"),
    SDB_REPL_GROUP_NOT_ACTIVE(-90, "Replication group is not activated"),
    SDB_REPL_INVALID_GROUP_MEMBER(-91, "Node does not belong to the group"),
    SDB_DMS_INCOMPATIBLE_MODE(-92, "Collection status is not compatible"),
    SDB_DMS_INCOMPATIBLE_VERSION(-93, "Incompatible version for storage unit"),
    SDB_REPL_LOCAL_G_V_EXPIRED(-94, "Version is expired for local group"),
    SDB_DMS_INVALID_PAGESIZE(-95, "Invalid page size"),
    SDB_REPL_REMOTE_G_V_EXPIRED(-96, "Version is expired for remote group"),
    SDB_CLS_VOTE_FAILED(-97, "Failed to vote for primary"),
    SDB_DPS_CORRUPTED_LOG(-98, "Log record is corrupted"),
    SDB_DPS_LSN_OUTOFRANGE(-99, "LSN is out of boundary"),
    SDB_UNKNOWN_MESSAGE(-100, "Unknown mesage is received"),
    SDB_NET_UPDATE_EXISTING_NODE(-101, "Updated information is same as old one"),
    SDB_CLS_UNKNOW_MSG(-102, "Unknown message"),
    SDB_CLS_EMPTY_HEAP(-103, "Empty heap"),
    SDB_CLS_NOT_PRIMARY(-104, "Node is not primary"),
    SDB_CLS_NODE_NOT_ENOUGH(-105, "Not enough number of data nodes"),
    SDB_CLS_NO_CATALOG_INFO(-106, "Catalog information does not exist on data node"),
    SDB_CLS_DATA_NODE_CAT_VER_OLD(-107, "Catalog version is expired on data node"),
    SDB_CLS_COORD_NODE_CAT_VER_OLD(-108, "Catalog version is expired on coordinator node"),
    SDB_CLS_INVALID_GROUP_NUM(-109, "Exceeds the max group size"),
    SDB_CLS_SYNC_FAILED(-110, "Failed to sync log"),
    SDB_CLS_REPLAY_LOG_FAILED(-111, "Failed to replay log"),
    SDB_REST_EHS(-112, "Invalid HTTP header"),
    SDB_CLS_CONSULT_FAILED(-113, "Failed to negotiate"),
    SDB_DPS_MOVE_FAILED(-114, "Failed to change DPS metadata"),
    SDB_DMS_CORRUPTED_SME(-115, "SME is corrupted"),
    SDB_APP_INTERRUPT(-116, "Application is interrupted"),
    SDB_APP_DISCONNECT(-117, "Application is disconnected"),
    SDB_OSS_CCE(-118, "Character encoding errors"),
    SDB_COORD_QUERY_FAILED(-119, "Failed to query on coord node"),
    SDB_CLS_BUFFER_FULL(-120, "Buffer array is full"),
    SDB_RTN_SUBCONTEXT_CONFLICT(-121, "Sub context is conflict"),
    SDB_COORD_QUERY_EOC(-122, "EOC message is received by coordinator node"),
    SDB_DPS_FILE_SIZE_NOT_SAME(-123, "Size of DPS files are not the same"),
    SDB_DPS_FILE_NOT_RECOGNISE(-124, "Invalid DPS log file"),
    SDB_OSS_NORES(-125, "No resource is avaliable"),
    SDB_DPS_INVALID_LSN(-126, "Invalid LSN"),
    SDB_OSS_NPIPE_DATA_TOO_BIG(-127, "Pipe buffer size is too small"),
    SDB_CAT_AUTH_FAILED(-128, "Catalog authentication failed"),
    SDB_CLS_FULL_SYNC(-129, "Full sync is in progress"),
    SDB_CAT_ASSIGN_NODE_FAILED(-130, "Failed to assign data node from coordinator node"),
    SDB_PHP_DRIVER_INTERNAL_ERROR(-131, "PHP driver internal error"),
    SDB_COORD_SEND_MSG_FAILED(-132, "Failed to send the message"),
    SDB_CAT_NO_NODEGROUP_INFO(-133, "No activated group information on catalog"),
    SDB_COORD_REMOTE_DISC(-134, "Remote-node is disconnected"),
    SDB_CAT_NO_MATCH_CATALOG(-135, "Unable to find the matched catalog information"),
    SDB_CLS_UPDATE_CAT_FAILED(-136, "Failed to update catalog"),
    SDB_COORD_UNKNOWN_OP_REQ(-137, "Unknown request operation code"),
    SDB_COOR_NO_NODEGROUP_INFO(-138, "Group information cannot be found on coordinator node"),
    SDB_DMS_CORRUPTED_EXTENT(-139, "DMS extent is corrupted"),
    SDBCM_FAIL(-140, "Remote cluster manager failed"),
    SDBCM_STOP_PART(-141, "Remote database services have been stopped"),
    SDBCM_SVC_STARTING(-142, "Service is starting"),
    SDBCM_SVC_STARTED(-143, "Service has been started"),
    SDBCM_SVC_RESTARTING(-144, "Service is restarting"),
    SDBCM_NODE_EXISTED(-145, "Node already exists"),
    SDBCM_NODE_NOTEXISTED(-146, "Node does not exist"),
    SDB_LOCK_FAILED(-147, "Unable to lock"),
    SDB_DMS_STATE_NOT_COMPATIBLE(-148, "DMS state is not compatible with current command"),
    SDB_REBUILD_HAS_ALREADY_START(-149, "Database rebuild is already started"),
    SDB_RTN_IN_REBUILD(-150, "Database rebuild is in progress"),
    SDB_RTN_COORD_CACHE_EMPTY(-151, "Cache is empty on coordinator node"),
    SDB_SPT_EVAL_FAIL(-152, "Evalution failed with error"),
    SDB_CAT_GRP_EXIST(-153, "Group already exist"),
    SDB_CLS_GRP_NOT_EXIST(-154, "Group does not exist"),
    SDB_CLS_NODE_NOT_EXIST(-155, "Node does not exist"),
    SDB_CM_RUN_NODE_FAILED(-156, "Failed to start the node"),
    SDB_CM_CONFIG_CONFLICTS(-157, "Invalid node configuration"),
    SDB_CLS_EMPTY_GROUP(-158, "Group is empty"),
    SDB_RTN_COORD_ONLY(-159, "The operation is for coord node only"),
    SDB_CM_OP_NODE_FAILED(-160, "Failed to operate on node"),
    SDB_RTN_MUTEX_JOB_EXIST(-161, "The mutex job already exist"),
    SDB_RTN_JOB_NOT_EXIST(-162, "The specified job does not exist"),
    SDB_CAT_CORRUPTION(-163, "The catalog information is corrupted"),
    SDB_IXM_DROP_SHARD(-164, "$shard index can't be dropped"),
    SDB_RTN_CMD_NO_NODE_AUTH(-165, "The command can't be run in the node"),
    SDB_RTN_CMD_NO_SERVICE_AUTH(-166, "The command can't be run in the service plane"),
    SDB_CLS_NO_GROUP_INFO(-167, "The group info not exist"),
    SDB_CLS_GROUP_NAME_CONFLICT(-168, "Group name is conflict"),
    SDB_COLLECTION_NOTSHARD(-169, "The collection is not sharded"),
    SDB_INVALID_SHARDINGKEY(-170, "The record does not contains valid sharding key"),
    SDB_TASK_EXIST(-171, "A task that already exists does not compatible with the new task"),
    SDB_CL_NOT_EXIST_ON_GROUP(-172, "The collection does not exists on the specified group"),
    SDB_CAT_TASK_NOTFOUND(-173, "The specified task does not exist"),
    SDB_MULTI_SHARDING_KEY(-174, "The record contains more than one sharding key"),
    SDB_CLS_MUTEX_TASK_EXIST(-175, "The mutex task already exist"),
    SDB_CLS_BAD_SPLIT_KEY(-176, "The split key is not valid or not in the source group"),
    SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY(-177, "The unique index must include all fields in sharding key"),
    SDB_UPDATE_SHARD_KEY(-178, "Sharding key cannot be updated"),
    SDB_AUTH_AUTHORITY_FORBIDDEN(-179, "Authority is forbidden"),
    SDB_CAT_NO_ADDR_LIST(-180, "There is no catalog address specified by user"),
    SDB_CURRENT_RECORD_DELETED(-181, "Current record has been removed"),
    SDB_QGM_MATCH_NONE(-182, "No records can be matched for the search condition"),
    SDB_IXM_REORG_DONE(-183, "Index page is reorged and the pos got different lchild"),
    SDB_RTN_DUPLICATE_FIELDNAME(-184, "Duplicate field name exists in the record"),
    SDB_QGM_MAX_NUM_RECORD(-185, "Too many records to be inserted at once"),
    SDB_QGM_MERGE_JOIN_EQONLY(-186, "Sort-Merge Join only supports equal predicates"),
    SDB_PD_TRACE_IS_STARTED(-187, "Trace is already started"),
    SDB_PD_TRACE_HAS_NO_BUFFER(-188, "Trace buffer does not exist"),
    SDB_PD_TRACE_FILE_INVALID(-189, "Trace file is not valid"),
    SDB_DPS_TRANS_LOCK_INCOMPATIBLE(-190, "Incompatible lock"),
    SDB_DPS_TRANS_DOING_ROLLBACK(-191, "Rollback operation is in progress"),
    SDB_MIG_IMP_BAD_RECORD(-192, "Invalid record is found during import"),
    SDB_QGM_REPEAT_VAR_NAME(-193, "Repeated variable name"),
    SDB_QGM_AMBIGUOUS_FIELD(-194, "Column name is ambiguous"),
    SDB_SQL_SYNTAX_ERROR(-195, "SQL syntax error"),
    SDB_DPS_TRANS_NO_TRANS(-196, "Invalid transactional operation"),
    SDB_DPS_TRANS_APPEND_TO_WAIT(-197, "Append to lock-wait-queue"),
    SDB_DMS_DELETING(-198, "Record is deleted"),
    SDB_DMS_INVALID_INDEXCB(-199, "Index is dropped or invalid"),
    SDB_COORD_RECREATE_CATALOG(-200, "Unable to create new catalog when there's already one exists"),
    SDB_UTIL_PARSE_JSON_INVALID(-201, "Failed to parse JSON file"),
    SDB_UTIL_PARSE_CSV_INVALID(-202, "Failed to parse CSV file"),
    SDB_DPS_LOG_FILE_OUT_OF_SIZE(-203, "Log file size is too large"),
    SDB_CATA_RM_NODE_FORBIDDEN(-204, "Unable to remove the last node or primary in a group"),
    SDB_CATA_FAILED_TO_CLEANUP(-205, "Unable to clean up catalog, manual cleanup may be required"),
    SDB_CATA_RM_CATA_FORBIDDEN(-206, "Unable to remove primary catalog or catalog group for non-empty database"),
    SDB_ERROR_RESERVED_2(-207, "Reserved"),
    SDB_CAT_RM_GRP_FORBIDDEN(-208, "Unable to remove non-empty group"),
    SDB_MIG_END_OF_QUEUE(-209, "End of queue"),
    SDB_COORD_SPLIT_NO_SHDIDX(-210, "Unable to split because of no sharding index exists"),
    SDB_FIELD_NOT_EXIST(-211, "The parameter field does not exist"),
    SDB_TOO_MANY_TRACE_BP(-212, "Too many break points are specified"),
    SDB_BUSY_PREFETCHER(-213, "All prefetchers are busy"),
    SDB_CAT_DOMAIN_NOT_EXIST(-214, "Domain does not exist"),
    SDB_CAT_DOMAIN_EXIST(-215, "Domain already exists"),
    SDB_CAT_GROUP_NOT_IN_DOMAIN(-216, "Group is not in domain"),
    SDB_CLS_SHARDING_NOT_HASH(-217, "Sharding type is not hash"),
    SDB_CLS_SPLIT_PERCENT_LOWER(-218, "split percentage is lower then expected"),
    SDB_TASK_ALREADY_FINISHED(-219, "Task is already finished"),
    SDB_COLLECTION_LOAD(-220, "Collection is in loading status"),
    SDB_LOAD_ROLLBACK(-221, "Rolling back load operation"),
    SDB_INVALID_ROUTEID(-222, "RouteID is different from the local"),
    SDB_DUPLICATED_SERVICE(-223, "Service already exists"),
    SDB_UTIL_NOT_FIND_FIELD(-224, "Field is not found"),
    SDB_UTIL_CSV_FIELD_END(-225, "csv field line end"),
    SDB_MIG_UNKNOW_FILE_TYPE(-226, "Unknown file type"),
    SDB_RTN_EXPORTCONF_NOT_COMPLETE(-227, "Exporting configuration does not complete in all nodes"),
    SDB_CLS_NOTP_AND_NODATA(-228, "Empty non-primary node"),
    SDB_DMS_SECRETVALUE_NOT_SAME(-229, "Secret value for index file does not match with data file"),
    SDB_PMD_VERSION_ONLY(-230, "Engine version argument is specified"),
    SDB_SDB_HELP_ONLY(-231, "Help argument is specified"),
    SDB_SDB_VERSION_ONLY(-232, "Version argument is specified"),
    SDB_FMP_FUNC_NOT_EXIST(-233, "Stored procedure does not exist"),
    SDB_ILL_RM_SUB_CL(-234, "Unable to remove collection partition"),
    SDB_RELINK_SUB_CL(-235, "Duplicated attach collection partition"),
    SDB_INVALID_MAIN_CL(-236, "Invalid partitioned-collection"),
    SDB_BOUND_CONFLICT(-237, "New boundary is conflict with the existing boundary"),
    SDB_BOUND_INVALID(-238, "Invalid boundary for the shard"),
    SDB_HIT_HIGH_WATERMARK(-239, "Hit the high water mark"),
    SDB_BAR_BACKUP_EXIST(-240, "Backup already exists"),
    SDB_BAR_BACKUP_NOTEXIST(-241, "Backup does not exist"),
    SDB_INVALID_SUB_CL(-242, "Invalid collection partition"),
    SDB_TASK_HAS_CANCELED(-243, "Task is canceled"),
    SDB_INVALID_MAIN_CL_TYPE(-244, "Sharding type must be ranged partition for partitioned-collection"),
    SDB_NO_SHARDINGKEY(-245, "There is no valid sharding-key defined"),
    SDB_MAIN_CL_OP_ERR(-246, "Operation is not supported on partitioned-collection"),
    SDB_IXM_REDEF(-247, "Redefine index"),
    SDB_DMS_CS_DELETING(-248, "Dropping the collection space is in progress"),
    SDB_DMS_REACHED_MAX_NODES(-249, "Hit the limit of maximum number of nodes in the cluster"),
    SDB_CLS_NODE_BSFAULT(-250, "The node is not in normal status"),
    SDB_CLS_NODE_INFO_EXPIRED(-251, "Node information is expired"),
    SDB_CLS_WAIT_SYNC_FAILED(-252, "Failed to wait for the sync operation from secondary nodes"),
    SDB_DPS_TRANS_DIABLED(-253, "Transaction is disabled"),
    SDB_DRIVER_DS_RUNOUT(-254, "Data source is running out of connection pool"),
    SDB_TOO_MANY_OPEN_FD(-255, "Too many opened file description"),
    SDB_DOMAIN_IS_OCCUPIED(-256, "Domain is not empty"),
    SDB_REST_RECV_SIZE(-257, "The data received by REST is larger than the max size"),
    SDB_DRIVER_BSON_ERROR(-258, "Failed to build bson object"),
    SDB_OUT_OF_BOUND(-259, "Stored procedure arguments are out of bound"),
    SDB_REST_COMMON_UNKNOWN(-260, "Unknown REST command"),
    SDB_BUT_FAILED_ON_DATA(-261, "Failed to execute command on data node"),
    SDB_CAT_NO_GROUP_IN_DOMAIN(-262, "The domain is empty"),
    SDB_OM_PASSWD_CHANGE_SUGGUEST(-263, "Changing password is required"),
    SDB_COORD_NOT_ALL_DONE(-264, "One or more nodes did not complete successfully"),
    SDB_OMA_DIFF_VER_AGT_IS_RUNNING(-265, "There is another OM Agent running with different version"),
    SDB_OM_TASK_NOT_EXIST(-266, "Task does not exist"),
    SDB_OM_TASK_ROLLBACK(-267, "Task is rolling back"),
    SDB_LOB_SEQUENCE_NOT_EXIST(-268, "LOB sequence does not exist"),
    SDB_LOB_IS_NOT_AVAILABLE(-269, "LOB is not useable"),
    SDB_MIG_DATA_NON_UTF(-270, "Data is not in UTF-8 format"),
    SDB_OMA_TASK_FAIL(-271, "Task failed"),
    SDB_LOB_NOT_OPEN(-272, "Lob does not open"),
    SDB_LOB_HAS_OPEN(-273, "Lob has been open"),
    SDBCM_NODE_IS_IN_RESTORING(-274, "Node is in restoring"),
    SDB_DMS_CS_NOT_EMPTY(-275, "There are some collections in the collection space"),
    SDB_CAT_LOCALHOST_CONFLICT(-276, "'localhost' and '127.0.0.1' cannot be used mixed with other hostname and IP address"),
    SDB_CAT_NOT_LOCALCONN(-277, "If use 'localhost' and '127.0.0.1' for hostname, coord and catalog must in the same host "),
    SDB_CAT_IS_NOT_DATAGROUP(-278, "The special group is not data group"),
    SDB_RTN_AUTOINDEXID_IS_FALSE(-279, "can not update/delete records when $id index does not exist"),
    SDB_CLS_CAN_NOT_STEP_UP(-280, "can not step up when primary node exists or LSN is not the biggest"),
    SDB_CAT_IMAGE_ADDR_CONFLICT(-281, "Image address is conflict with the self cluster"),
    SDB_CAT_GROUP_HASNOT_IMAGE(-282, "The data group does not have image group"),
    SDB_CAT_GROUP_HAS_IMAGE(-283, "The data group has image group"),
    SDB_CAT_IMAGE_IS_ENABLED(-284, "The image is in enabled status"),
    SDB_CAT_IMAGE_NOT_CONFIG(-285, "The cluster's image does not configured"),
    SDB_CAT_DUAL_WRITABLE(-286, "This cluster and image cluster is both writable"),
    SDB_CAT_CLUSTER_IS_READONLY(-287, "This cluster is readonly"),
    SDB_RTN_QUERYMODIFY_SORT_NO_IDX(-288, "Sorting of 'query and modify' must use index"),
    SDB_RTN_QUERYMODIFY_MULTI_NODES(-289, "'query and modify' can't use skip and limit in multiple nodes or sub-collections"),
    SDB_DIR_NOT_EMPTY(-290, "Given path is not empty"),
    SDB_IXM_EXIST_COVERD_ONE(-291, "Exist one index which can cover this scene"),
    SDB_CAT_IMAGE_IS_CONFIGURED(-292, "The cluster's image has already configured"),
    SDB_RTN_CMD_IN_LOCAL_MODE(-293, "The command is in local mode"),
    SDB_SPT_NOT_SPECIAL_JSON(-294, "The object is not a special object in sdb shell"),
    SDB_AUTH_USER_ALREADY_EXIST(-295, "The specified user already exist"),
    SDB_DMS_EMPTY_COLLECTION(-296, "The collection is empty"),
    SDB_LOB_SEQUENCE_EXISTS(-297, "LOB sequence exists"),
    SDB_OM_CLUSTER_NOT_EXIST(-298, "cluster do not exist"),
    SDB_OM_BUSINESS_NOT_EXIST(-299, "business do not exist"),
    SDB_AUTH_USER_NOT_EXIST(-300, "user specified is not exist or password is invalid"),
    SDB_UTIL_COMPRESS_INIT_FAIL(-301, "Compression initialization failed"),
    SDB_UTIL_COMPRESS_FAIL(-302, "Compression failed"),
    SDB_UTIL_DECOMPRESS_FAIL(-303, "Decompression failed"),
    SDB_UTIL_COMPRESS_ABORT(-304, "Compression abort"),
    SDB_UTIL_COMPRESS_BUFF_SMALL(-305, "Buffer for compression is too small"),
    SDB_UTIL_DECOMPRESS_BUFF_SMALL(-306, "Buffer for decompression is too small"),
    SDB_OSS_UP_TO_LIMIT(-307, "Up to the limit"),
    SDB_DS_NOT_ENABLE(-308, "data source is not enabled yet"),
    SDB_DS_NO_REACHABLE_COORD(-309, "No reachable coord notes"),
    SDB_RULE_ID_IS_NOT_EXIST(-310, "the record which exclusive ruleID is not exist"),
    SDB_STRTGY_TASK_NAME_CONFLICT(-311, "Task name conflict"),
    SDB_STRTGY_TASK_NOT_EXISTED(-312, "The task is not existed"),
    SDB_DPS_LOG_NOT_ARCHIVED(-313, "Replica log is not archived"),
    SDB_DS_NOT_INIT(-314, "Data source has not been initialized"),
    SDB_OPERATION_INCOMPATIBLE(-315, "Operation is incompatible with the object"),
    SDB_CAT_CLUSTER_IS_DEACTIVED(-316, "This cluster is deactived"),
    SDB_LOB_IS_IN_USE(-317, "LOB is in use"),
    SDB_VALUE_OVERFLOW(-318, "Data operation is overflowed"),
    SDB_LOB_PIECESINFO_OVERFLOW(-319, "LOB's pieces info is overflowed"),
    SDB_LOB_LOCK_CONFLICTED(-320, "LOB lock is conflicted"),
    SDB_DMS_TRUNCATED(-321, "Collection is truncated");

    private int code;
    private String desc;

    private SDBError(int code, String desc) {
        this.code = code;
        this.desc = desc;
    }

    @Override
    public String toString() {
        return this.name() + "(" + this.code + ")" + ": " + this.desc;
    }

    public int getErrorCode() {
        return this.code;
    }

    public String getErrorDescription() {
        return this.desc;
    }

    public String getErrorType() {
        return this.name();
    }

    public static SDBError getSDBError(int errorCode) {
        switch (errorCode) {
            case -1:
                return SDB_IO;
            case -2:
                return SDB_OOM;
            case -3:
                return SDB_PERM;
            case -4:
                return SDB_FNE;
            case -5:
                return SDB_FE;
            case -6:
                return SDB_INVALIDARG;
            case -7:
                return SDB_INVALIDSIZE;
            case -8:
                return SDB_INTERRUPT;
            case -9:
                return SDB_EOF;
            case -10:
                return SDB_SYS;
            case -11:
                return SDB_NOSPC;
            case -12:
                return SDB_EDU_INVAL_STATUS;
            case -13:
                return SDB_TIMEOUT;
            case -14:
                return SDB_QUIESCED;
            case -15:
                return SDB_NETWORK;
            case -16:
                return SDB_NETWORK_CLOSE;
            case -17:
                return SDB_DATABASE_DOWN;
            case -18:
                return SDB_APP_FORCED;
            case -19:
                return SDB_INVALIDPATH;
            case -20:
                return SDB_INVALID_FILE_TYPE;
            case -21:
                return SDB_DMS_NOSPC;
            case -22:
                return SDB_DMS_EXIST;
            case -23:
                return SDB_DMS_NOTEXIST;
            case -24:
                return SDB_DMS_RECORD_TOO_BIG;
            case -25:
                return SDB_DMS_RECORD_NOTEXIST;
            case -26:
                return SDB_DMS_OVF_EXIST;
            case -27:
                return SDB_DMS_RECORD_INVALID;
            case -28:
                return SDB_DMS_SU_NEED_REORG;
            case -29:
                return SDB_DMS_EOC;
            case -30:
                return SDB_DMS_CONTEXT_IS_OPEN;
            case -31:
                return SDB_DMS_CONTEXT_IS_CLOSE;
            case -32:
                return SDB_OPTION_NOT_SUPPORT;
            case -33:
                return SDB_DMS_CS_EXIST;
            case -34:
                return SDB_DMS_CS_NOTEXIST;
            case -35:
                return SDB_DMS_INVALID_SU;
            case -36:
                return SDB_RTN_CONTEXT_NOTEXIST;
            case -37:
                return SDB_IXM_MULTIPLE_ARRAY;
            case -38:
                return SDB_IXM_DUP_KEY;
            case -39:
                return SDB_IXM_KEY_TOO_LARGE;
            case -40:
                return SDB_IXM_NOSPC;
            case -41:
                return SDB_IXM_KEY_NOTEXIST;
            case -42:
                return SDB_DMS_MAX_INDEX;
            case -43:
                return SDB_DMS_INIT_INDEX;
            case -44:
                return SDB_DMS_COL_DROPPED;
            case -45:
                return SDB_IXM_IDENTICAL_KEY;
            case -46:
                return SDB_IXM_EXIST;
            case -47:
                return SDB_IXM_NOTEXIST;
            case -48:
                return SDB_IXM_UNEXPECTED_STATUS;
            case -49:
                return SDB_IXM_EOC;
            case -50:
                return SDB_IXM_DEDUP_BUF_MAX;
            case -51:
                return SDB_RTN_INVALID_PREDICATES;
            case -52:
                return SDB_RTN_INDEX_NOTEXIST;
            case -53:
                return SDB_RTN_INVALID_HINT;
            case -54:
                return SDB_DMS_NO_MORE_TEMP;
            case -55:
                return SDB_DMS_SU_OUTRANGE;
            case -56:
                return SDB_IXM_DROP_ID;
            case -57:
                return SDB_DPS_LOG_NOT_IN_BUF;
            case -58:
                return SDB_DPS_LOG_NOT_IN_FILE;
            case -59:
                return SDB_PMD_RG_NOT_EXIST;
            case -60:
                return SDB_PMD_RG_EXIST;
            case -61:
                return SDB_INVALID_REQID;
            case -62:
                return SDB_PMD_SESSION_NOT_EXIST;
            case -63:
                return SDB_PMD_FORCE_SYSTEM_EDU;
            case -64:
                return SDB_NOT_CONNECTED;
            case -65:
                return SDB_UNEXPECTED_RESULT;
            case -66:
                return SDB_CORRUPTED_RECORD;
            case -67:
                return SDB_BACKUP_HAS_ALREADY_START;
            case -68:
                return SDB_BACKUP_NOT_COMPLETE;
            case -69:
                return SDB_RTN_IN_BACKUP;
            case -70:
                return SDB_BAR_DAMAGED_BK_FILE;
            case -71:
                return SDB_RTN_NO_PRIMARY_FOUND;
            case -72:
                return SDB_ERROR_RESERVED_1;
            case -73:
                return SDB_PMD_HELP_ONLY;
            case -74:
                return SDB_PMD_CON_INVALID_STATE;
            case -75:
                return SDB_CLT_INVALID_HANDLE;
            case -76:
                return SDB_CLT_OBJ_NOT_EXIST;
            case -77:
                return SDB_NET_ALREADY_LISTENED;
            case -78:
                return SDB_NET_CANNOT_LISTEN;
            case -79:
                return SDB_NET_CANNOT_CONNECT;
            case -80:
                return SDB_NET_NOT_CONNECT;
            case -81:
                return SDB_NET_SEND_ERR;
            case -82:
                return SDB_NET_TIMER_ID_NOT_FOUND;
            case -83:
                return SDB_NET_ROUTE_NOT_FOUND;
            case -84:
                return SDB_NET_BROKEN_MSG;
            case -85:
                return SDB_NET_INVALID_HANDLE;
            case -86:
                return SDB_DMS_INVALID_REORG_FILE;
            case -87:
                return SDB_DMS_REORG_FILE_READONLY;
            case -88:
                return SDB_DMS_INVALID_COLLECTION_S;
            case -89:
                return SDB_DMS_NOT_IN_REORG;
            case -90:
                return SDB_REPL_GROUP_NOT_ACTIVE;
            case -91:
                return SDB_REPL_INVALID_GROUP_MEMBER;
            case -92:
                return SDB_DMS_INCOMPATIBLE_MODE;
            case -93:
                return SDB_DMS_INCOMPATIBLE_VERSION;
            case -94:
                return SDB_REPL_LOCAL_G_V_EXPIRED;
            case -95:
                return SDB_DMS_INVALID_PAGESIZE;
            case -96:
                return SDB_REPL_REMOTE_G_V_EXPIRED;
            case -97:
                return SDB_CLS_VOTE_FAILED;
            case -98:
                return SDB_DPS_CORRUPTED_LOG;
            case -99:
                return SDB_DPS_LSN_OUTOFRANGE;
            case -100:
                return SDB_UNKNOWN_MESSAGE;
            case -101:
                return SDB_NET_UPDATE_EXISTING_NODE;
            case -102:
                return SDB_CLS_UNKNOW_MSG;
            case -103:
                return SDB_CLS_EMPTY_HEAP;
            case -104:
                return SDB_CLS_NOT_PRIMARY;
            case -105:
                return SDB_CLS_NODE_NOT_ENOUGH;
            case -106:
                return SDB_CLS_NO_CATALOG_INFO;
            case -107:
                return SDB_CLS_DATA_NODE_CAT_VER_OLD;
            case -108:
                return SDB_CLS_COORD_NODE_CAT_VER_OLD;
            case -109:
                return SDB_CLS_INVALID_GROUP_NUM;
            case -110:
                return SDB_CLS_SYNC_FAILED;
            case -111:
                return SDB_CLS_REPLAY_LOG_FAILED;
            case -112:
                return SDB_REST_EHS;
            case -113:
                return SDB_CLS_CONSULT_FAILED;
            case -114:
                return SDB_DPS_MOVE_FAILED;
            case -115:
                return SDB_DMS_CORRUPTED_SME;
            case -116:
                return SDB_APP_INTERRUPT;
            case -117:
                return SDB_APP_DISCONNECT;
            case -118:
                return SDB_OSS_CCE;
            case -119:
                return SDB_COORD_QUERY_FAILED;
            case -120:
                return SDB_CLS_BUFFER_FULL;
            case -121:
                return SDB_RTN_SUBCONTEXT_CONFLICT;
            case -122:
                return SDB_COORD_QUERY_EOC;
            case -123:
                return SDB_DPS_FILE_SIZE_NOT_SAME;
            case -124:
                return SDB_DPS_FILE_NOT_RECOGNISE;
            case -125:
                return SDB_OSS_NORES;
            case -126:
                return SDB_DPS_INVALID_LSN;
            case -127:
                return SDB_OSS_NPIPE_DATA_TOO_BIG;
            case -128:
                return SDB_CAT_AUTH_FAILED;
            case -129:
                return SDB_CLS_FULL_SYNC;
            case -130:
                return SDB_CAT_ASSIGN_NODE_FAILED;
            case -131:
                return SDB_PHP_DRIVER_INTERNAL_ERROR;
            case -132:
                return SDB_COORD_SEND_MSG_FAILED;
            case -133:
                return SDB_CAT_NO_NODEGROUP_INFO;
            case -134:
                return SDB_COORD_REMOTE_DISC;
            case -135:
                return SDB_CAT_NO_MATCH_CATALOG;
            case -136:
                return SDB_CLS_UPDATE_CAT_FAILED;
            case -137:
                return SDB_COORD_UNKNOWN_OP_REQ;
            case -138:
                return SDB_COOR_NO_NODEGROUP_INFO;
            case -139:
                return SDB_DMS_CORRUPTED_EXTENT;
            case -140:
                return SDBCM_FAIL;
            case -141:
                return SDBCM_STOP_PART;
            case -142:
                return SDBCM_SVC_STARTING;
            case -143:
                return SDBCM_SVC_STARTED;
            case -144:
                return SDBCM_SVC_RESTARTING;
            case -145:
                return SDBCM_NODE_EXISTED;
            case -146:
                return SDBCM_NODE_NOTEXISTED;
            case -147:
                return SDB_LOCK_FAILED;
            case -148:
                return SDB_DMS_STATE_NOT_COMPATIBLE;
            case -149:
                return SDB_REBUILD_HAS_ALREADY_START;
            case -150:
                return SDB_RTN_IN_REBUILD;
            case -151:
                return SDB_RTN_COORD_CACHE_EMPTY;
            case -152:
                return SDB_SPT_EVAL_FAIL;
            case -153:
                return SDB_CAT_GRP_EXIST;
            case -154:
                return SDB_CLS_GRP_NOT_EXIST;
            case -155:
                return SDB_CLS_NODE_NOT_EXIST;
            case -156:
                return SDB_CM_RUN_NODE_FAILED;
            case -157:
                return SDB_CM_CONFIG_CONFLICTS;
            case -158:
                return SDB_CLS_EMPTY_GROUP;
            case -159:
                return SDB_RTN_COORD_ONLY;
            case -160:
                return SDB_CM_OP_NODE_FAILED;
            case -161:
                return SDB_RTN_MUTEX_JOB_EXIST;
            case -162:
                return SDB_RTN_JOB_NOT_EXIST;
            case -163:
                return SDB_CAT_CORRUPTION;
            case -164:
                return SDB_IXM_DROP_SHARD;
            case -165:
                return SDB_RTN_CMD_NO_NODE_AUTH;
            case -166:
                return SDB_RTN_CMD_NO_SERVICE_AUTH;
            case -167:
                return SDB_CLS_NO_GROUP_INFO;
            case -168:
                return SDB_CLS_GROUP_NAME_CONFLICT;
            case -169:
                return SDB_COLLECTION_NOTSHARD;
            case -170:
                return SDB_INVALID_SHARDINGKEY;
            case -171:
                return SDB_TASK_EXIST;
            case -172:
                return SDB_CL_NOT_EXIST_ON_GROUP;
            case -173:
                return SDB_CAT_TASK_NOTFOUND;
            case -174:
                return SDB_MULTI_SHARDING_KEY;
            case -175:
                return SDB_CLS_MUTEX_TASK_EXIST;
            case -176:
                return SDB_CLS_BAD_SPLIT_KEY;
            case -177:
                return SDB_SHARD_KEY_NOT_IN_UNIQUE_KEY;
            case -178:
                return SDB_UPDATE_SHARD_KEY;
            case -179:
                return SDB_AUTH_AUTHORITY_FORBIDDEN;
            case -180:
                return SDB_CAT_NO_ADDR_LIST;
            case -181:
                return SDB_CURRENT_RECORD_DELETED;
            case -182:
                return SDB_QGM_MATCH_NONE;
            case -183:
                return SDB_IXM_REORG_DONE;
            case -184:
                return SDB_RTN_DUPLICATE_FIELDNAME;
            case -185:
                return SDB_QGM_MAX_NUM_RECORD;
            case -186:
                return SDB_QGM_MERGE_JOIN_EQONLY;
            case -187:
                return SDB_PD_TRACE_IS_STARTED;
            case -188:
                return SDB_PD_TRACE_HAS_NO_BUFFER;
            case -189:
                return SDB_PD_TRACE_FILE_INVALID;
            case -190:
                return SDB_DPS_TRANS_LOCK_INCOMPATIBLE;
            case -191:
                return SDB_DPS_TRANS_DOING_ROLLBACK;
            case -192:
                return SDB_MIG_IMP_BAD_RECORD;
            case -193:
                return SDB_QGM_REPEAT_VAR_NAME;
            case -194:
                return SDB_QGM_AMBIGUOUS_FIELD;
            case -195:
                return SDB_SQL_SYNTAX_ERROR;
            case -196:
                return SDB_DPS_TRANS_NO_TRANS;
            case -197:
                return SDB_DPS_TRANS_APPEND_TO_WAIT;
            case -198:
                return SDB_DMS_DELETING;
            case -199:
                return SDB_DMS_INVALID_INDEXCB;
            case -200:
                return SDB_COORD_RECREATE_CATALOG;
            case -201:
                return SDB_UTIL_PARSE_JSON_INVALID;
            case -202:
                return SDB_UTIL_PARSE_CSV_INVALID;
            case -203:
                return SDB_DPS_LOG_FILE_OUT_OF_SIZE;
            case -204:
                return SDB_CATA_RM_NODE_FORBIDDEN;
            case -205:
                return SDB_CATA_FAILED_TO_CLEANUP;
            case -206:
                return SDB_CATA_RM_CATA_FORBIDDEN;
            case -207:
                return SDB_ERROR_RESERVED_2;
            case -208:
                return SDB_CAT_RM_GRP_FORBIDDEN;
            case -209:
                return SDB_MIG_END_OF_QUEUE;
            case -210:
                return SDB_COORD_SPLIT_NO_SHDIDX;
            case -211:
                return SDB_FIELD_NOT_EXIST;
            case -212:
                return SDB_TOO_MANY_TRACE_BP;
            case -213:
                return SDB_BUSY_PREFETCHER;
            case -214:
                return SDB_CAT_DOMAIN_NOT_EXIST;
            case -215:
                return SDB_CAT_DOMAIN_EXIST;
            case -216:
                return SDB_CAT_GROUP_NOT_IN_DOMAIN;
            case -217:
                return SDB_CLS_SHARDING_NOT_HASH;
            case -218:
                return SDB_CLS_SPLIT_PERCENT_LOWER;
            case -219:
                return SDB_TASK_ALREADY_FINISHED;
            case -220:
                return SDB_COLLECTION_LOAD;
            case -221:
                return SDB_LOAD_ROLLBACK;
            case -222:
                return SDB_INVALID_ROUTEID;
            case -223:
                return SDB_DUPLICATED_SERVICE;
            case -224:
                return SDB_UTIL_NOT_FIND_FIELD;
            case -225:
                return SDB_UTIL_CSV_FIELD_END;
            case -226:
                return SDB_MIG_UNKNOW_FILE_TYPE;
            case -227:
                return SDB_RTN_EXPORTCONF_NOT_COMPLETE;
            case -228:
                return SDB_CLS_NOTP_AND_NODATA;
            case -229:
                return SDB_DMS_SECRETVALUE_NOT_SAME;
            case -230:
                return SDB_PMD_VERSION_ONLY;
            case -231:
                return SDB_SDB_HELP_ONLY;
            case -232:
                return SDB_SDB_VERSION_ONLY;
            case -233:
                return SDB_FMP_FUNC_NOT_EXIST;
            case -234:
                return SDB_ILL_RM_SUB_CL;
            case -235:
                return SDB_RELINK_SUB_CL;
            case -236:
                return SDB_INVALID_MAIN_CL;
            case -237:
                return SDB_BOUND_CONFLICT;
            case -238:
                return SDB_BOUND_INVALID;
            case -239:
                return SDB_HIT_HIGH_WATERMARK;
            case -240:
                return SDB_BAR_BACKUP_EXIST;
            case -241:
                return SDB_BAR_BACKUP_NOTEXIST;
            case -242:
                return SDB_INVALID_SUB_CL;
            case -243:
                return SDB_TASK_HAS_CANCELED;
            case -244:
                return SDB_INVALID_MAIN_CL_TYPE;
            case -245:
                return SDB_NO_SHARDINGKEY;
            case -246:
                return SDB_MAIN_CL_OP_ERR;
            case -247:
                return SDB_IXM_REDEF;
            case -248:
                return SDB_DMS_CS_DELETING;
            case -249:
                return SDB_DMS_REACHED_MAX_NODES;
            case -250:
                return SDB_CLS_NODE_BSFAULT;
            case -251:
                return SDB_CLS_NODE_INFO_EXPIRED;
            case -252:
                return SDB_CLS_WAIT_SYNC_FAILED;
            case -253:
                return SDB_DPS_TRANS_DIABLED;
            case -254:
                return SDB_DRIVER_DS_RUNOUT;
            case -255:
                return SDB_TOO_MANY_OPEN_FD;
            case -256:
                return SDB_DOMAIN_IS_OCCUPIED;
            case -257:
                return SDB_REST_RECV_SIZE;
            case -258:
                return SDB_DRIVER_BSON_ERROR;
            case -259:
                return SDB_OUT_OF_BOUND;
            case -260:
                return SDB_REST_COMMON_UNKNOWN;
            case -261:
                return SDB_BUT_FAILED_ON_DATA;
            case -262:
                return SDB_CAT_NO_GROUP_IN_DOMAIN;
            case -263:
                return SDB_OM_PASSWD_CHANGE_SUGGUEST;
            case -264:
                return SDB_COORD_NOT_ALL_DONE;
            case -265:
                return SDB_OMA_DIFF_VER_AGT_IS_RUNNING;
            case -266:
                return SDB_OM_TASK_NOT_EXIST;
            case -267:
                return SDB_OM_TASK_ROLLBACK;
            case -268:
                return SDB_LOB_SEQUENCE_NOT_EXIST;
            case -269:
                return SDB_LOB_IS_NOT_AVAILABLE;
            case -270:
                return SDB_MIG_DATA_NON_UTF;
            case -271:
                return SDB_OMA_TASK_FAIL;
            case -272:
                return SDB_LOB_NOT_OPEN;
            case -273:
                return SDB_LOB_HAS_OPEN;
            case -274:
                return SDBCM_NODE_IS_IN_RESTORING;
            case -275:
                return SDB_DMS_CS_NOT_EMPTY;
            case -276:
                return SDB_CAT_LOCALHOST_CONFLICT;
            case -277:
                return SDB_CAT_NOT_LOCALCONN;
            case -278:
                return SDB_CAT_IS_NOT_DATAGROUP;
            case -279:
                return SDB_RTN_AUTOINDEXID_IS_FALSE;
            case -280:
                return SDB_CLS_CAN_NOT_STEP_UP;
            case -281:
                return SDB_CAT_IMAGE_ADDR_CONFLICT;
            case -282:
                return SDB_CAT_GROUP_HASNOT_IMAGE;
            case -283:
                return SDB_CAT_GROUP_HAS_IMAGE;
            case -284:
                return SDB_CAT_IMAGE_IS_ENABLED;
            case -285:
                return SDB_CAT_IMAGE_NOT_CONFIG;
            case -286:
                return SDB_CAT_DUAL_WRITABLE;
            case -287:
                return SDB_CAT_CLUSTER_IS_READONLY;
            case -288:
                return SDB_RTN_QUERYMODIFY_SORT_NO_IDX;
            case -289:
                return SDB_RTN_QUERYMODIFY_MULTI_NODES;
            case -290:
                return SDB_DIR_NOT_EMPTY;
            case -291:
                return SDB_IXM_EXIST_COVERD_ONE;
            case -292:
                return SDB_CAT_IMAGE_IS_CONFIGURED;
            case -293:
                return SDB_RTN_CMD_IN_LOCAL_MODE;
            case -294:
                return SDB_SPT_NOT_SPECIAL_JSON;
            case -295:
                return SDB_AUTH_USER_ALREADY_EXIST;
            case -296:
                return SDB_DMS_EMPTY_COLLECTION;
            case -297:
                return SDB_LOB_SEQUENCE_EXISTS;
            case -298:
                return SDB_OM_CLUSTER_NOT_EXIST;
            case -299:
                return SDB_OM_BUSINESS_NOT_EXIST;
            case -300:
                return SDB_AUTH_USER_NOT_EXIST;
            case -301:
                return SDB_UTIL_COMPRESS_INIT_FAIL;
            case -302:
                return SDB_UTIL_COMPRESS_FAIL;
            case -303:
                return SDB_UTIL_DECOMPRESS_FAIL;
            case -304:
                return SDB_UTIL_COMPRESS_ABORT;
            case -305:
                return SDB_UTIL_COMPRESS_BUFF_SMALL;
            case -306:
                return SDB_UTIL_DECOMPRESS_BUFF_SMALL;
            case -307:
                return SDB_OSS_UP_TO_LIMIT;
            case -308:
                return SDB_DS_NOT_ENABLE;
            case -309:
                return SDB_DS_NO_REACHABLE_COORD;
            case -310:
                return SDB_RULE_ID_IS_NOT_EXIST;
            case -311:
                return SDB_STRTGY_TASK_NAME_CONFLICT;
            case -312:
                return SDB_STRTGY_TASK_NOT_EXISTED;
            case -313:
                return SDB_DPS_LOG_NOT_ARCHIVED;
            case -314:
                return SDB_DS_NOT_INIT;
            case -315:
                return SDB_OPERATION_INCOMPATIBLE;
            case -316:
                return SDB_CAT_CLUSTER_IS_DEACTIVED;
            case -317:
                return SDB_LOB_IS_IN_USE;
            case -318:
                return SDB_VALUE_OVERFLOW;
            case -319:
                return SDB_LOB_PIECESINFO_OVERFLOW;
            case -320:
                return SDB_LOB_LOCK_CONFLICTED;
            case -321:
                return SDB_DMS_TRUNCATED;
            default:
                return null;
        }
    }
}
