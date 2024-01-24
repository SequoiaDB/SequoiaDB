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

   Source File Name = msgDef.h

   Descriptive Name = Message Defines Header

   When/how to use: this program may be used on binary and text-formatted
   versions of msg component. This file contains definition for global keywords
   that used in client/server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MSGDEF_H__
#define MSGDEF_H__

#define SYS_PREFIX                           "SYS"

#define FIELD_NAME_ROLE                      "Role"
#define FIELD_NAME_HOST                      "HostName"
#define FIELD_NAME_SERVICE                   "Service"
#define FIELD_NAME_NODE_NAME                 "NodeName"
#define FIELD_NAME_SERVICE_TYPE              "Type"
#define FIELD_NAME_SERVICE_NAME              "ServiceName"
#define FIELD_NAME_NAME                      "Name"
#define FIELD_NAME_GROUPID                   "GroupID"
#define FIELD_NAME_GROUPNAME                 "GroupName"
#define FIELD_NAME_DOMAIN                    "Domain"
#define FIELD_NAME_NODEID                    "NodeID"
#define FIELD_NAME_INSTANCEID                "InstanceID"
#define FIELD_NAME_IS_PRIMARY                "IsPrimary"
#define FIELD_NAME_CURRENT_LSN               "CurrentLSN"
#define FIELD_NAME_BEGIN_LSN                 "BeginLSN"
#define FIELD_NAME_COMMIT_LSN                "CommittedLSN"
#define FIELD_NAME_COMPLETE_LSN              "CompleteLSN"
#define FIELD_NAME_LSN_QUE_SIZE              "LSNQueSize"
#define FIELD_NAME_LSN_OFFSET                "Offset"
#define FIELD_NAME_LSN_VERSION               "Version"
#define FIELD_NAME_TRANS_INFO                "TransInfo"
#define FIELD_NAME_TOTAL_COUNT               "TotalCount"
#define FIELD_NAME_SERVICE_STATUS            "ServiceStatus"
#define FIELD_NAME_GROUP                     "Group"
#define FIELD_NAME_GROUPS                    "Groups"
#define FIELD_NAME_VERSION                   "Version"
#define FIELD_NAME_SECRETID                  "SecretID"
#define FIELD_NAME_EDITION                   "Edition"
#define FIELD_NAME_W                         "ReplSize"
#define FIELD_NAME_PRIMARY                   "PrimaryNode"
#define FIELD_NAME_GROUP_STATUS              "Status"
#define FIELD_NAME_FT_STATUS                 "FTStatus"
#define FIELD_NAME_DATA_STATUS               "DataStatus"
#define FIELD_NAME_SYNC_CONTROL              "SyncControl"
#define FIELD_NAME_PAGE_SIZE                 "PageSize"
#define FIELD_NAME_LOB_PAGE_SIZE             "LobPageSize"
#define FIELD_NAME_MAX_CAPACITY_SIZE         "MaxCapacitySize"
#define FIELD_NAME_MAX_DATA_CAP_SIZE         "MaxDataCapSize"
#define FIELD_NAME_MAX_INDEX_CAP_SIZE        "MaxIndexCapSize"
#define FIELD_NAME_QUERY_ID                  "QueryID"
// Deprecated
#define FIELD_NAME_MAX_LOB_CAP_SIZE          "MaxLobCapSize"
// Use MaxLobCapacity instead of MaxLobCapSize
#define FIELD_NAME_MAX_LOB_CAPACITY          "MaxLobCapacity"
#define FIELD_NAME_LOB_CAPACITY              "LobCapacity"
#define FIELD_NAME_LOB_META_CAPACITY         "LobMetaCapacity"
#define FIELD_NAME_TOTAL_SIZE                "TotalSize"
#define FIELD_NAME_FREE_SIZE                 "FreeSize"
#define FIELD_NAME_TOTAL_DATA_SIZE           "TotalDataSize"
#define FIELD_NAME_TOTAL_IDX_SIZE            "TotalIndexSize"
#define FIELD_NAME_TOTAL_VALID_LOB_SIZE      "TotalValidLobSize"
#define FIELD_NAME_TOTAL_LOB_SIZE            "TotalLobSize"
#define FIELD_NAME_FREE_DATA_SIZE            "FreeDataSize"
#define FIELD_NAME_FREE_IDX_SIZE             "FreeIndexSize"
#define FIELD_NAME_RECYCLE_DATA_SIZE         "RecycleDataSize"
#define FIELD_NAME_RECYCLE_IDX_SIZE          "RecycleIndexSize"
#define FIELD_NAME_RECYCLE_LOB_SIZE          "RecycleLobSize"
// Deprecated
#define FIELD_NAME_FREE_LOB_SIZE             "FreeLobSize"
// Use FreeLobSpace instead of FreeLobSize
#define FIELD_NAME_FREE_LOB_SPACE            "FreeLobSpace"
#define FIELD_NAME_TOTAL_USED_LOB_SPACE      "TotalUsedLobSpace"
#define FIELD_NAME_USED_LOB_SPACE_RATIO      "UsedLobSpaceRatio"
#define FIELD_NAME_LOB_USAGE_RATE            "LobUsageRate"
#define FIELD_NAME_AVG_LOB_SIZE              "AvgLobSize"
#define FIELD_NAME_COLLECTION                "Collection"
#define FIELD_NAME_COLLECTIONSPACE           "CollectionSpace"
#define FIELD_NAME_CATALOGINFO               "CataInfo"
#define FIELD_NAME_SHARDINGKEY               "ShardingKey"
#define FIELD_NAME_LOBSHARDINGKEY_FORMAT     "LobShardingKeyFormat"
#define FIELD_NAME_COMPRESSED                "Compressed"
#define FIELD_NAME_COMPRESSIONTYPE           "CompressionType"
#define FIELD_NAME_STRICTDATAMODE            "StrictDataMode"
#define FIELD_NAME_NOTRANS                   "NoTrans"
#define FIELD_NAME_COMPRESSIONTYPE_DESC      "CompressionTypeDesc"
#define FIELD_NAME_REPARECHECK               "RepairCheck"
#define VALUE_NAME_SNAPPY                    "snappy"
#define VALUE_NAME_LZW                       "lzw"
#define VALUE_NAME_LZ4                       "lz4"
#define VALUE_NAME_ZLIB                      "zlib"
#define VALUE_NAME_DATABASE                  "database"
#define VALUE_NAME_SESSIONS                  "sessions"
#define VALUE_NAME_SESSIONS_CURRENT          "sessions current"
#define VALUE_NAME_HEALTH                    "health"
#define VALUE_NAME_SVCTASKS                  "svctasks"
#define VALIE_NAME_COLLECTIONS               "collections"
#define VALUE_NAME_ALL                       "all"
#define VALUE_NAME_NONE                      "none"
#define VALUE_NAME_CATALOG                   "catalog"
#define VALUE_NAME_GROUP                     "group"
#define VALUE_NAME_STRATEGY                  "strategy"
#define VALUE_NAME_DATASOURCE                "datasource"
#define FIELD_NAME_ISMAINCL                  "IsMainCL"
#define FIELD_NAME_MAINCLNAME                "MainCLName"
#define FIELD_NAME_SUBCLNAME                 "SubCLName"
#define FIELD_NAME_SUBOBJSNUM                "SubObjsNum"
#define FIELD_NAME_SUBOBJSSIZE               "SubObjsSize"
#define FIELD_NAME_ENSURE_SHDINDEX           "EnsureShardingIndex"
#define FIELD_NAME_GLOBAL_INDEX              "GlobalIndex"
#define FIELD_NAME_CL_UNIQUEID               "CLUniqueID"
#define FIELD_NAME_SHARDTYPE                 "ShardingType"
#define FIELD_NAME_SHARDTYPE_RANGE           "range"
#define FIELD_NAME_SHARDTYPE_HASH            "hash"
#define FIELD_NAME_PARTITION                 "Partition"
#define FIELD_NAME_AUTOINCREMENT             "AutoIncrement"
#define FIELD_NAME_AUTOINC_FIELD             "Field"
#define FIELD_NAME_AUTOINC_SEQ               "SequenceName"
#define FIELD_NAME_AUTOINC_SEQ_ID            "SequenceID"
#define FIELD_NAME_GENERATED                 "Generated"
#define VALUE_NAME_ALWAYS                    "always"
#define VALUE_NAME_STRICT                    "strict"
#define VALUE_NAME_DEFAULT                   "default"
#define FIELD_NAME_CURRENT_VALUE             "CurrentValue"
#define FIELD_NAME_INCREMENT                 "Increment"
#define FIELD_NAME_START_VALUE               "StartValue"
#define FIELD_NAME_MIN_VALUE                 "MinValue"
#define FIELD_NAME_MAX_VALUE                 "MaxValue"
#define FIELD_NAME_CACHE_SIZE                "CacheSize"
#define FIELD_NAME_ACQUIRE_SIZE              "AcquireSize"
#define FIELD_NAME_CYCLED                    "Cycled"
#define FIELD_NAME_CYCLED_COUNT              "CycledCount"
#define FIELD_NAME_INTERNAL                  "Internal"
#define FIELD_NAME_INITIAL                   "Initial"
#define FIELD_NAME_NEXT_VALUE                "NextValue"
#define FIELD_NAME_EXPECT_VALUE              "ExpectValue"
#define FIELD_NAME_FETCH_NUM                 "FetchNum"
#define FIELD_NAME_MAJOR                     "Major"
#define FIELD_NAME_MINOR                     "Minor"
#define FIELD_NAME_FIX                       "Fix"
#define FIELD_NAME_RELEASE                   "Release"
#define FIELD_NAME_GITVERSION                "GitVersion"
#define FIELD_NAME_BUILD                     "Build"
#define FIELD_NAME_SESSIONID                 "SessionID"
#define FIELD_NAME_TID                       "TID"
#define FIELD_NAME_WAITER_TID                "WaiterTID"
#define FIELD_NAME_CLIENTINFO                "ClientInfo"
#define FIELD_NAME_CLIENTTID                 "ClientTID"
#define FIELD_NAME_CLIENTHOST                "ClientHost"
#define FIELD_NAME_CONTEXTS                  "Contexts"
#define FIELD_NAME_CONTEXTID                 "ContextID"
#define FIELD_NAME_ACCESSPLAN_ID             "AccessPlanID"
#define FIELD_NAME_DATAREAD                  "DataRead"
#define FIELD_NAME_DATAWRITE                 "DataWrite"
#define FIELD_NAME_INDEXREAD                 "IndexRead"
#define FIELD_NAME_INDEXWRITE                "IndexWrite"
#define FIELD_NAME_LOBREAD                   "LobRead"
#define FIELD_NAME_LOBWRITE                  "LobWrite"
#define FIELD_NAME_LOBTRUNCATE               "LobTruncate"
#define FIELD_NAME_LOBADDRESSING             "LobAddressing"
#define FIELD_NAME_QUERYTIMESPENT            "QueryTimeSpent"
#define FIELD_NAME_NODEWAITTIME              "RemoteNodeWaitTime"
#define FIELD_NAME_STARTTIMESTAMP            "StartTimestamp"
#define FIELD_NAME_ENDTIMESTAMP              "EndTimestamp"
#define VALUE_NAME_EMPTYENDTIMESTAMP         "--"
#define FIELD_NAME_TOTALNUMCONNECTS          "TotalNumConnects"
#define FIELD_NAME_TOTALDATAREAD             "TotalDataRead"
#define FIELD_NAME_TOTALINDEXREAD            "TotalIndexRead"
#define FIELD_NAME_TOTALDATAWRITE            "TotalDataWrite"
#define FIELD_NAME_TOTALINDEXWRITE           "TotalIndexWrite"
#define FIELD_NAME_TOTALUPDATE               "TotalUpdate"
#define FIELD_NAME_TOTALDELETE               "TotalDelete"
#define FIELD_NAME_TOTALINSERT               "TotalInsert"
#define FIELD_NAME_TOTALSELECT               "TotalSelect"
#define FIELD_NAME_TOTALREAD                 "TotalRead"
#define FIELD_NAME_TOTALWRITE                "TotalWrite"
#define FIELD_NAME_TOTALTBSCAN               "TotalTbScan"
#define FIELD_NAME_TOTALIXSCAN               "TotalIxScan"
#define FIELD_NAME_TOTALLOBGET               "TotalLobGet"
#define FIELD_NAME_TOTALLOBPUT               "TotalLobPut"
#define FIELD_NAME_TOTALLOBDELETE            "TotalLobDelete"
#define FIELD_NAME_TOTALLOBLIST              "TotalLobList"
#define FIELD_NAME_TOTALLOBREADSIZE          "TotalLobReadSize"
#define FIELD_NAME_TOTALLOBWRITESIZE         "TotalLobWriteSize"
#define FIELD_NAME_TOTALLOBREAD              "TotalLobRead"
#define FIELD_NAME_TOTALLOBWRITE             "TotalLobWrite"
#define FIELD_NAME_TOTALLOBTRUNCATE          "TotalLobTruncate"
#define FIELD_NAME_TOTALLOBADDRESSING        "TotalLobAddressing"

#define FIELD_NAME_TOTALREADTIME             "TotalReadTime"
#define FIELD_NAME_TOTALWRITETIME            "TotalWriteTime"
#define FIELD_NAME_TOTALQUERY                "TotalQuery"
#define FIELD_NAME_TOTALSLOWQUERY            "TotalSlowQuery"
#define FIELD_NAME_TOTALTRANSCOMMIT          "TotalTransCommit"
#define FIELD_NAME_TOTALTRANSROLLBACK        "TotalTransRollback"
#define FIELD_NAME_TOTALTIME                 "Time"
#define FIELD_NAME_TOTALCONTEXTS             "TotalContexts"
#define FIELD_NAME_READTIMESPENT             "ReadTimeSpent"
#define FIELD_NAME_WRITETIMESPENT            "WriteTimeSpent"
#define FIELD_NAME_LASTOPBEGIN               "LastOpBegin"
#define FIELD_NAME_LASTOPEND                 "LastOpEnd"
#define FIELD_NAME_LASTOPTYPE                "LastOpType"
#define FIELD_NAME_LASTOPINFO                "LastOpInfo"
#define FIELD_NAME_OPTYPE                    "OpType"
#define FIELD_NAME_TOTALMAPPED               "TotalMapped"
#define FIELD_NAME_REPLINSERT                "ReplInsert"
#define FIELD_NAME_REPLUPDATE                "ReplUpdate"
#define FIELD_NAME_REPLDELETE                "ReplDelete"
#define FIELD_NAME_ACTIVETIMESTAMP           "ActivateTimestamp"
#define FIELD_NAME_RESETTIMESTAMP            "ResetTimestamp"
#define FIELD_NAME_USERCPU                   "UserCPU"
#define FIELD_NAME_SYSCPU                    "SysCPU"
#define FIELD_NAME_CONNECTTIMESTAMP          "ConnectTimestamp"
#define FIELD_NAME_USER                      "User"
#define FIELD_NAME_PASSWD                    "Password"
#define FIELD_NAME_SYS                       "Sys"
#define FIELD_NAME_IDLE                      "Idle"
#define FIELD_NAME_IOWAIT                    "IOWait"
#define FIELD_NAME_OTHER                     "Other"
#define FIELD_NAME_CPU                       "CPU"
#define FIELD_NAME_LOADPERCENT               "LoadPercent"
#define FIELD_NAME_TOTALRAM                  "TotalRAM"
#define FIELD_NAME_FREERAM                   "FreeRAM"
#define FIELD_NAME_AVAILABLERAM              "AvailableRAM"
#define FIELD_NAME_TOTALSWAP                 "TotalSwap"
#define FIELD_NAME_FREESWAP                  "FreeSwap"
#define FIELD_NAME_TOTALVIRTUAL              "TotalVirtual"
#define FIELD_NAME_FREEVIRTUAL               "FreeVirtual"
#define FIELD_NAME_MEMORY                    "Memory"
#define FIELD_NAME_RSSSIZE                   "RssSize"
#define FIELD_NAME_LOADPERCENTVM             "LoadPercentVM"
#define FIELD_NAME_VMLIMIT                   "VMLimit"
#define FIELD_NAME_VMSIZE                    "VMSize"
#define FIELD_NAME_CORESZ                    "CoreFileSize"
#define FIELD_NAME_VM                        "VirtualMemory"
#define FIELD_NAME_OPENFL                    "OpenFiles"
#define FIELD_NAME_NPROC                     "NumProc"
#define FIELD_NAME_FILESZ                    "FileSize"
#define FIELD_NAME_STACKSZ                   "StackSize"
#define FIELD_NAME_ULIMIT                    "Ulimit"
#define FIELD_NAME_OOM                       "SDB_OOM"
#define FIELD_NAME_NOSPC                     "SDB_NOSPC"
#define FIELD_NAME_TOOMANY_OF                "SDB_TOO_MANY_OPEN_FD"
#define FIELD_NAME_ERRNUM                    "ErrNum"
#define FIELD_NAME_TOTALNUM                  "TotalNum"
#define FIELD_NAME_FREENUM                   "FreeNum"
#define FIELD_NAME_FILEDESP                  "FileDesp"
#define FIELD_NAME_ABNORMALHST               "AbnormalHistory"
#define FIELD_NAME_STARTHST                  "StartHistory"
#define FIELD_NAME_DIFFLSNPRIMARY            "DiffLSNWithPrimary"
#define FIELD_NAME_DATABASEPATH              "DatabasePath"
#define FIELD_NAME_TOTALSPACE                "TotalSpace"
#define FIELD_NAME_FREESPACE                 "FreeSpace"
#define FIELD_NAME_DISK                      "Disk"
#define FIELD_NAME_CURRENTACTIVESESSIONS     "CurrentActiveSessions"
#define FIELD_NAME_CURRENTIDLESESSIONS       "CurrentIdleSessions"
#define FIELD_NAME_CURRENTSYSTEMSESSIONS     "CurrentSystemSessions"
#define FIELD_NAME_CURRENTTASKSESSIONS       "CurrentTaskSessions"
#define FIELD_NAME_CURRENTCONTEXTS           "CurrentContexts"
#define FIELD_NAME_SESSIONS                  "Sessions"
#define FIELD_NAME_STATUS                    "Status"
#define FIELD_NAME_ISBLOCKED                 "IsBlocked"
#define FIELD_NAME_NUM_MSG_SENT              "TotalMsgSent"
#define FIELD_NAME_STATUSDESC                "StatusDesc"
#define VALUE_NAME_READY                     "Ready"
#define VALUE_NAME_RUNNING                   "Running"
#define VALUE_NAME_PAUSE                     "Pause"
#define VALUE_NAME_CANCELED                  "Canceled"
#define VALUE_NAME_CHGMETA                   "Change meta"
#define VALUE_NAME_CLEANUP                   "Clean up"
#define VALUE_NAME_ROLLBACK                  "Roll back"
#define VALUE_NAME_FINISH                    "Finish"
#define VALUE_NAME_END                       "End"
#define FIELD_NAME_DICT_CREATED              "DictionaryCreated"
#define FIELD_NAME_DICT_VERSION              "DictionaryVersion"
#define FIELD_NAME_DICT_CREATE_TIME          "DictionaryCreateTime"
#define FIELD_NAME_TYPE                      "Type"
#define FIELD_NAME_EXT_DATA_NAME             "ExtDataName"
#define FIELD_NAME_TOTAL_RECORDS             "TotalRecords"
#define FIELD_NAME_TOTAL_LOBS                "TotalLobs"
#define FIELD_NAME_TOTAL_DATA_PAGES          "TotalDataPages"
#define FIELD_NAME_TOTAL_INDEX_PAGES         "TotalIndexPages"
#define FIELD_NAME_TOTAL_LOB_PAGES           "TotalLobPages"
#define FIELD_NAME_TOTAL_DATA_FREESPACE      "TotalDataFreeSpace"
#define FIELD_NAME_TOTAL_INDEX_FREESPACE     "TotalIndexFreeSpace"
#define FIELD_NAME_CURR_COMPRESS_RATIO       "CurrentCompressionRatio"
#define FIELD_NAME_EDUNAME                   "Name"
#define FIELD_NAME_DOING                     "Doing"
#define FIELD_NAME_QUEUE_SIZE                "QueueSize"
#define FIELD_NAME_PROCESS_EVENT_COUNT       "ProcessEventCount"
#define FIELD_NAME_RELATED_ID                "RelatedID"
#define FIELD_NAME_RELATED_NODE              "RelatedNode"
#define FIELD_NAME_RELATED_NID               "RelatedNID"
#define FIELD_NAME_RELATED_TID               "RelatedTID"
#define FIELD_NAME_ID                        "ID"
#define FIELD_NAME_UNIQUEID                  "UniqueID"
#define FIELD_NAME_LOGICAL_ID                "LogicalID"
#define FIELD_NAME_SEQUENCE                  "Sequence"
#define FIELD_NAME_INDEXES                   "Indexes"
#define FIELD_NAME_DETAILS                   "Details"
#define FIELD_NAME_NUMCOLLECTIONS            "NumCollections"
#define FIELD_NAME_COLLECTIONHWM             "CollectionHWM"
#define FIELD_NAME_SIZE                      "Size"
#define FIELD_NAME_MAX                       "Max"
#define FIELD_NAME_TRACE                     "trace"
#define FIELD_NAME_TO                        "To"
#define FIELD_NAME_OLDNAME                   "OldName"
#define FIELD_NAME_NEWNAME                   "NewName"
#define FIELD_NAME_INDEX                     "Index"
#define FIELD_NAME_TOTAL                     "Total"
#define FIELD_NAME_LOWBOUND                  "LowBound"
#define FIELD_NAME_UPBOUND                   "UpBound"
#define FIELD_NAME_SOURCE                    "Source"
#define FIELD_NAME_TARGET                    "Target"
#define FIELD_NAME_SPLITQUERY                "SplitQuery"
#define FIELD_NAME_SPLITENDQUERY             "SplitEndQuery"
#define FIELD_NAME_SPLITPERCENT              "SplitPercent"
#define FIELD_NAME_SPLITVALUE                "SplitValue"
#define FIELD_NAME_SPLITENDVALUE             "SplitEndValue"
#define FIELD_NAME_RECEIVECOUNT              "ReceivedEvents"
#define FIELD_NAME_TASKTYPE                  "TaskType"
#define FIELD_NAME_TASKTYPEDESC              "TaskTypeDesc"
#define VALUE_NAME_SPLIT                     "Split"
#define VALUE_NAME_ALTERSEQUENCE             "Alter sequence"
#define VALUE_NAME_CREATEIDX                 "Create index"
#define VALUE_NAME_DROPIDX                   "Drop index"
#define VALUE_NAME_COPYIDX                   "Copy index"
#define FIELD_NAME_IS_MAINTASK               "IsMainTask"
#define FIELD_NAME_MAIN_TASKID               "MainTaskID"
#define FIELD_NAME_TASKID                    "TaskID"
#define FIELD_NAME_RULEID                    "RuleID"
#define FIELD_NAME_SOURCEID                  "SourceID"
#define FIELD_NAME_TARGETID                  "TargetID"
#define FIELD_NAME_ASYNC                     "Async"
#define FIELD_NAME_STANDALONE                "Standalone"
#define FIELD_NAME_RESULTCODE                "ResultCode"
#define FIELD_NAME_RESULTCODEDESC            "ResultCodeDesc"
#define FIELD_NAME_RESULTINFO                "ResultInfo"
#define FIELD_NAME_INTERNAL_RESULTCODE       "InternalResultCode"
#define FIELD_NAME_OPINFO                    "OpInfo"
#define FIELD_NAME_TOTALGROUP                "TotalGroups"
#define FIELD_NAME_SUCCEEDGROUP              "SucceededGroups"
#define FIELD_NAME_FAILGROUP                 "FailedGroups"
#define FIELD_NAME_TOTALSUBTASK              "TotalSubTasks"
#define FIELD_NAME_SUCCEEDSUBTASK            "SucceededSubTasks"
#define FIELD_NAME_FAILSUBTASK               "FailedSubTasks"
#define FIELD_NAME_SUBTASKS                  "SubTasks"
#define FIELD_NAME_PROGRESS                  "Progress"
#define FIELD_NAME_SPEED                     "Speed"
#define FIELD_NAME_TIMESPENT                 "TimeSpent"
#define FIELD_NAME_TIMELEFT                  "TimeLeft"
#define FIELD_NAME_CREATETIMESTAMP           "CreateTimestamp"
#define FIELD_NAME_BEGINTIMESTAMP            "BeginTimestamp"
#define FIELD_NAME_RETRY_COUNT               "RetryCount"
#define FIELD_NAME_PROCESSED_RECORDS         "ProcessedRecords"
#define FIELD_NAME_COPYTO                    "CopyTo"
#define FIELD_NAME_INDEXNAMES                "IndexNames"
#define FIELD_NAME_OPTIONS                   "Options"
#define FIELD_NAME_CONDITION                 "Condition"
#define FIELD_NAME_RULE                      "Rule"
#define FIELD_NAME_SORT                      "Sort"
#define FIELD_NAME_HINT                      "Hint"
#define FIELD_NAME_SELECTOR                  "Selector"
#define FIELD_NAME_SKIP                      "Skip"
#define FIELD_NAME_RETURN                    "Return"
#define FIELD_NAME_COMPONENTS                "Components"
#define FIELD_NAME_BREAKPOINTS               "BreakPoint"
#define FIELD_NAME_THREADS                   "Threads"
#define FIELD_NAME_FUNCTIONNAMES             "FunctionNames"
#define FIELD_NAME_THREADTYPES               "ThreadTypes"
#define FIELD_NAME_FILENAME                  "FileName"
#define FIELD_NAME_TRACESTARTED              "TraceStarted"
#define FIELD_NAME_WRAPPED                   "Wrapped"
#define FIELD_NAME_MASK                      "Mask"
#define FIELD_NAME_AGGR                      "Aggr"
#define FIELD_NAME_CMD                       "CMD"
#define FIELD_NAME_DATABLOCKS                "Datablocks"
#define FIELD_NAME_SCANTYPE                  "ScanType"
#define VALUE_NAME_TBSCAN                    "tbscan"
#define VALUE_NAME_IXSCAN                    "ixscan"
#define FIELD_NAME_INDEXNAME                 "IndexName"
#define FIELD_NAME_INDEXLID                  "IndexLID"
#define FIELD_NAME_DIRECTION                 "Direction"
#define FIELD_NAME_INDEXBLOCKS               "Indexblocks"
#define FIELD_NAME_STARTKEY                  "StartKey"
#define FIELD_NAME_ENDKEY                    "EndKey"
#define FIELD_NAME_STARTRID                  "StartRID"
#define FIELD_NAME_ENDRID                    "EndRID"
#define FIELD_NAME_META                      "$Meta"
#define FIELD_NAME_POSITION                  "$Position"
#define FIELD_NAME_RANGE                     "$Range"
#define FIELD_NAME_SET_ON_INSERT             "$SetOnInsert"
#define FIELD_NAME_PATH                      "Path"
#define FIELD_NAME_DESP                      "Description"
#define FIELD_NAME_ENSURE_INC                "EnsureInc"
#define FIELD_NAME_OVERWRITE                 "OverWrite"
#define FIELD_NAME_DETAIL                    "Detail"
#define FIELD_NAME_ESTIMATE                  "Estimate"
#define FIELD_NAME_ABBREV                    "Abbrev"
#define FIELD_NAME_SEARCH                    "Search"
#define FIELD_NAME_EVALUATE                  "Evaluate"
#define FIELD_NAME_EXPAND                    "Expand"
#define FIELD_NAME_LOCATION                  "Location"
#define FIELD_NAME_CMD_LOCATION              "CMDLocation"
#define FIELD_NAME_FLATTEN                   "Flatten"
#define FIELD_NAME_ISSUBDIR                  "IsSubDir"
#define FIELD_NAME_ENABLE_DATEDIR            "EnableDateDir"
#define FIELD_NAME_PREFIX                    "Prefix"
#define FIELD_NAME_MAX_DATAFILE_SIZE         "MaxDataFileSize"
#define FIELD_NAME_BACKUP_LOG                "BackupLog"
#define FIELD_NAME_USE_EXT_SORT              "UseExtSort"
#define FIELD_NAME_SUB_COLLECTIONS           "SubCollections"
#define FIELD_NAME_ELAPSED_TIME              "ElapsedTime"
#define FIELD_NAME_RETURN_NUM                "ReturnNum"
#define FIELD_NAME_RUN                       "Run"
#define FIELD_NAME_WAIT                      "Wait"
#define FIELD_NAME_CLUSTERNAME               "ClusterName"
#define FIELD_NAME_BUSINESSNAME              "BusinessName"
#define FIELD_NAME_DATACENTER                "DataCenter"
#define FIELD_NAME_ADDRESS                   "Address"
#define FIELD_NAME_IMAGE                     "Image"
#define FIELD_NAME_ACTIVATED                 "Activated"
#define FIELD_NAME_READONLY                  "Readonly"
#define FIELD_NAME_CSUNIQUEHWM               "CSUniqueHWM"
#define FIELD_NAME_TASKHWM                   "TaskHWM"
#define FIELD_NAME_IDXUNIQUEHWM              "IdxUniqueHWM"
#define FIELD_NAME_CAT_VERSION               "CATVersion"
#define FIELD_NAME_GLOBALID                  "GlobalID"
#define FIELD_NAME_ENABLE                    "Enable"
#define FIELD_NAME_ACTION                    "Action"
#define FIELD_NAME_DATA                      "Data"
#define FIELD_NAME_DATALEN                   "DataLen"
#define FIELD_NAME_ORG_LSNOFFSET             "OrgOffset"
#define FIELD_NAME_TRANSACTION_ID            "TransactionID"
#define FIELD_NAME_TRANSACTION_ID_SN         "TransactionIDSN"
#define FIELD_NAME_TRANS_LSN_CUR             "CurrentTransLSN"
#define FIELD_NAME_TRANS_LSN_BEGIN           "BeginTransLSN"
#define FIELD_NAME_IS_ROLLBACK               "IsRollback"
#define FIELD_NAME_TRANS_LOCKS_NUM           "TransactionLocksNum"
#define FIELD_NAME_TRANS_IS_LOCK_ESCALATED   "IsLockEscalated"
#define FIELD_NAME_USED_LOG_SPACE            "UsedLogSpace"
#define FIELD_NAME_RESERVED_LOG_SPACE        "ReservedLogSpace"
#define FIELD_NAME_TRANS_LOCKS               "GotLocks"
#define FIELD_NAME_TRANS_WAIT_LOCK           "WaitLock"
#define FIELD_NAME_SLICE                     "Slice"
#define FIELD_NAME_REMOTE_IP                 "RemoteIP"
#define FIELD_NAME_REMOTE_PORT               "RemotePort"
#define FIELD_NAME_MODE                      "Mode"
#define FIELD_NAME_REQUIRED_MODE             "RequiredMode"
#define VALUE_NAME_LOCAL                     "local"
#define FIELD_NAME_SHOW_MAIN_CL_MODE         "ShowMainCLMode"
#define VALUE_NAME_MAIN                      "main"
#define VALUE_NAME_SUB                       "sub"
#define VALUE_NAME_BOTH                      "both"
#define VALUE_NAME_SEQUOIADB                 "sequoiadb"
#define VALUE_NAME_READ                      "read"
#define VALUE_NAME_WRITE                     "write"
#define VALUE_NAME_LOW                       "low"
#define VALUE_NAME_HIGH                      "high"
#define VALUE_NAME_NEVER                     "never"
#define VALUE_NAME_NOT_SUPPORT               "notsupport"
#define VALUE_NAME_ADMIN                     "admin"
#define VALUE_NAME_MONITOR                   "monitor"

#define FIELD_NAME_MODIFY                    "$Modify"
#define FIELD_NAME_OP                        "OP"
#define FIELD_NAME_OP_UPDATE                 "Update"
#define FIELD_NAME_OP_REMOVE                 "Remove"
#define FIELD_NAME_RETURNNEW                 "ReturnNew"
#define FIELD_NAME_KEEP_SHARDING_KEY         "KeepShardingKey"
#define FIELD_NAME_JUSTONE                   "JustOne"
#define FIELD_NAME_UPDATE_SHARDING_KEY       "UpdateShardingKey"

#define FIELD_NAME_INSERT                    "Insert"
#define FIELD_NAME_UPDATE                    "Update"
#define FIELD_NAME_DELETE                    "Delete"
#define FIELD_NAME_NLJOIN                    "NLJoin"
#define FIELD_NAME_HASHJOIN                  "HashJoin"
#define FIELD_NAME_SCAN                      "Scan"
#define FIELD_NAME_FILTER                    "Filter"
#define FIELD_NAME_SPLITBY                   "SPLITBY"
#define FIELD_NAME_MAX_GTID                  "MaxGlobTransID"
#define FIELD_NAME_DATA_COMMIT_LSN           "DataCommitLSN"
#define FIELD_NAME_IDX_COMMIT_LSN            "IndexCommitLSN"
#define FIELD_NAME_LOB_COMMIT_LSN            "LobCommitLSN"
#define FIELD_NAME_DATA_COMMITTED            "DataCommitted"
#define FIELD_NAME_IDX_COMMITTED             "IndexCommitted"
#define FIELD_NAME_LOB_COMMITTED             "LobCommitted"
#define FIELD_NAME_DIRTY_PAGE                "DirtyPage"

#define FIELD_NAME_ENSURE_EMPTY              "EnsureEmpty"

#define FIELD_NAME_PDLEVEL                   "PDLevel"
#define FIELD_NAME_ASYNCHRONOUS              "asynchronous"
#define FIELD_NAME_THREADNUM                 "threadNum"
#define FIELD_NAME_BUCKETNUM                 "bucketNum"
#define FIELD_NAME_PARSEBUFFERSIZE           "parseBufferSize"
#define FIELD_NAME_ATTRIBUTE                 "Attribute"
#define FIELD_NAME_ATTRIBUTE_DESC            "AttributeDesc"
#define FIELD_NAME_RCFLAG                    "Flag"
#define FIELD_NAME_GROUPBY_ID                "_id"
#define FIELD_NAME_FIELDS                    "fields"
#define FIELD_NAME_HEADERLINE                "headerline"
#define FIELD_NAME_LTYPE                     "type"
#define FIELD_NAME_CHARACTER                 "character"
#define FIELD_NAME_GLOBAL                    "Global"
#define FIELD_NAME_ERROR_NODES               "ErrNodes"
#define FIELD_NAME_ERROR_INFO                "ErrInfo"
#define FIELD_NAME_FUNC                      "func"
#define FIELD_NAME_FUNCTYPE                  "funcType"
#define FIELD_NAME_PREFERRED_INSTANCE_LEGACY "PreferedInstance"
#define FIELD_NAME_PREFERRED_INSTANCE        "PreferredInstance"
#define FIELD_NAME_PREFERED_INSTANCE_V1      "PreferedInstanceV1"
#define FIELD_NAME_PREFERRED_INSTANCE_MODE_LEGACY "PreferedInstanceMode"
#define FIELD_NAME_PREFERRED_INSTANCE_MODE   "PreferredInstanceMode"
#define FIELD_NAME_PREFERRED_STRICT_LEGACY   "PreferedStrict"
#define FIELD_NAME_PREFERRED_STRICT          "PreferredStrict"
#define FIELD_NAME_PREFERRED_PERIOD_LEGACY   "PreferedPeriod"
#define FIELD_NAME_PREFERRED_PERIOD          "PreferredPeriod"
#define FIELD_NAME_PREFERRED_CONSTRAINT      "PreferredConstraint"
#define FIELD_NAME_TIMEOUT                   "Timeout"
#define FIELD_NAME_NODE_SELECT               "NodeSelect"
#define FIELD_NAME_RAWDATA                   "RawData"
#define FIELD_NAME_SYS_AGGR                  "$Aggr"
#define FIELD_NAME_NODE_LOCATION             "Location"
#define FIELD_NAME_NODE_LOCATIONS            "Locations"
#define FIELD_NAME_NODE_LOCATIONID           "LocationID"
#define FIELD_NAME_IS_LOC_PRIMARY            "IsLocationPrimary"
#define FIELD_NAME_CONSISTENCY_STRATEGY      "ConsistencyStrategy"
#define FIELD_NAME_GROUP_ACTIVE_LOCATION     "ActiveLocation"

#define FIELD_NAME_FREELOGSPACE              "freeLogSpace"
#define FIELD_NAME_VSIZE                     "vsize"
#define FIELD_NAME_RSS                       "rss"
#define FIELD_NAME_MEM_SHARED                "MemShared"
#define FIELD_NAME_FAULT                     "fault"
#define FIELD_NAME_SVC_NETIN                 "svcNetIn"
#define FIELD_NAME_SVC_NETOUT                "svcNetOut"
#define FIELD_NAME_REPL_NETIN                "replNetIn"
#define FIELD_NAME_REPL_NETOUT               "replNetOut"
#define FIELD_NAME_SHARD_NETIN               "shardNetIn"
#define FIELD_NAME_SHARD_NETOUT              "shardNetOut"
#define FIELD_NAME_DOMAIN_AUTO_SPLIT         "AutoSplit"
#define FIELD_NAME_DOMAIN_AUTO_REBALANCE     "AutoRebalance"
#define FIELD_NAME_LOB_OID                   "Oid"
#define FIELD_NAME_LOB_OPEN_MODE             "Mode"
#define FIELD_NAME_LOB_SIZE                  "Size"
#define FIELD_NAME_LOB_CREATETIME            "CreateTime"
#define FIELD_NAME_LOB_MODIFICATION_TIME     "ModificationTime"
#define FIELD_NAME_LOB_FLAG                  "Flag"
#define FIELD_NAME_LOB_PIECESINFONUM         "PiecesInfoNum"
#define FIELD_NAME_LOB_PIECESINFO            "PiecesInfo"
#define FIELD_NAME_LOB_IS_MAIN_SHD           "IsMainShard"
#define FIELD_NAME_LOB_REOPENED              "Reopened"
#define FIELD_NAME_LOB_LOCK_SECTIONS         "LockSections"
#define FIELD_NAME_LOB_META_DATA             "MetaData"
#define FIELD_NAME_LOB_LIST_PIECES_MODE      "ListPieces"
#define FIELD_NAME_LOB_AVAILABLE             "Available"
#define FIELD_NAME_LOB_HAS_PIECESINFO        "HasPiecesInfo"
#define FIELD_NAME_LOB_PAGE_SZ               "LobPageSize"
#define FIELD_NAME_LOB_OFFSET                "Offset"
#define FIELD_NAME_LOB_LENGTH                "Length"
#define FIELD_NAME_LOB_ACCESSID              "AccessId"
#define FIELD_NAME_LOB_REFCOUNT              "RefCount"
#define FIELD_NAME_LOB_READCOUNT             "ReadCount"
#define FIELD_NAME_LOB_WRITECOUNT            "WriteCount"
#define FIELD_NAME_LOB_SHAREREADCOUNT        "ShareReadCount"
#define FIELD_NAME_LOB_SECTION_BEGIN         "Begin"
#define FIELD_NAME_LOB_SECTION_END           "End"
#define FIELD_NAME_LOB_LOCK_TYPE             "LockType"
#define FIELD_NAME_LOB_ACCESSINFO            "AccessInfo"
#define FIELD_NAME_AUTO_INDEX_ID             "AutoIndexId"
#define FIELD_NAME_REELECTION_TIMEOUT        "Seconds"
#define FIELD_NAME_REELECTION_LEVEL          "Level"
#define FIELD_NAME_FORCE_STEP_UP_TIME        FIELD_NAME_REELECTION_TIMEOUT
#define FIELD_NAME_INTERNAL_VERSION          "InternalV"
#define FIELD_NAME_RTYPE                     "ReturnType"
#define FIELD_NAME_IX_BOUND                  "IXBound"
#define FIELD_NAME_QUERY                     "Query"
#define FIELD_NAME_NEED_MATCH                "NeedMatch"
#define FIELD_NAME_INDEX_COVER               "IndexCover"
#define FIELD_NAME_RTYPE                     "ReturnType"
#define FIELD_NAME_ONLY_DETACH               "OnlyDetach"
#define FIELD_NAME_ONLY_ATTACH               "OnlyAttach"
#define FIELD_NAME_ALTER_TYPE                "AlterType"
#define FIELD_NAME_ALTER_INFO                "AlterInfo"
#define FIELD_NAME_ARGS                      "Args"
#define FIELD_NAME_ALTER                     "Alter"
#define FIELD_NAME_IGNORE_EXCEPTION          "IgnoreException"
#define FIELD_NAME_KEEP_DATA                 "KeepData"
#define FIELD_NAME_ENFORCED                  "enforced"
#define FIELD_NAME_ENFORCED1                 "Enforced"
#define FIELD_NAME_RECURSIVE                 "Recursive"
#define FIELD_NAME_DEEP                      "Deep"
#define FIELD_NAME_BLOCK                     "Block"
#define FIELD_NAME_CAPPED                    "Capped"
#define FIELD_NAME_TEXT                      "$Text"
#define FIELD_NAME_CONFIGS                   "Configs"
#define FIELD_NAME_SEQUENCE_NAME             "Name"
#define FIELD_NAME_SEQUENCE_OID              "_id"
#define FIELD_NAME_SEQUENCE_ID               "ID"
#define FIELD_NAME_CONTONDUP                 "ContOnDup"
#define FIELD_NAME_CONTONDUP_ID              "ContOnDupID"
#define FIELD_NAME_REPLACEONDUP              "ReplaceOnDup"
#define FIELD_NAME_REPLACEONDUP_ID           "ReplaceOnDupID"
#define FIELD_NAME_UPDATEONDUP               "UpdateOnDup"
#define FIELD_NAME_RETURN_OID                "ReturnOID"
#define FIELD_NAME_AUDIT_MASK                "AuditMask"
#define FIELD_NAME_AUDIT_CONFIG_MASK         "AuditConfigMask"
#define FIELD_NAME_ROLLBACK                  "Rollback"
#define FIELD_NAME_TRANS_RC                  "TransRC"
#define FIELD_NAME_TRANSISOLATION            "TransIsolation"
#define FIELD_NAME_TRANS_TIMEOUT             "TransTimeout"
#define FIELD_NAME_TRANS_WAITLOCK            "TransLockWait"
#define FIELD_NAME_TRANS_WAITLOCKTIME        "TransLockWaitTime"
#define FIELD_NAME_TRANS_USE_RBS             "TransUseRBS"
#define FIELD_NAME_TRANS_AUTOCOMMIT          "TransAutoCommit"
#define FIELD_NAME_TRANS_AUTOROLLBACK        "TransAutoRollback"
#define FIELD_NAME_TRANS_RCCOUNT             "TransRCCount"
#define FIELD_NAME_TRANS_ALLOWLOCKESCALATION "TransAllowLockEscalation"
#define FIELD_NAME_TRANS_MAXLOCKNUM          "TransMaxLockNum"
#define FIELD_NAME_TRANS_MAXLOGSPACERATIO    "TransMaxLogSpaceRatio"
#define FIELD_NAME_LAST_GENERATE_ID          "LastGenerateID"
#define FIELD_NAME_INSERT_NUM                "InsertedNum"
#define FIELD_NAME_DUPLICATE_NUM             "DuplicatedNum"
#define FIELD_NAME_UPDATE_NUM                "UpdatedNum"
#define FIELD_NAME_MODIFIED_NUM              "ModifiedNum"
#define FIELD_NAME_DELETE_NUM                "DeletedNum"
#define FIELD_NAME_MEMPOOL_SIZE              "MemPoolSize"
#define FIELD_NAME_MEMPOOL_FREE              "MemPoolFree"
#define FIELD_NAME_MEMPOOL_USED              "MemPoolUsed"
#define FIELD_NAME_MEMPOOL_MAX_OOLSZ         "MemPoolMaxOOLSize"
#define FIELD_NAME_CUR_RBS_CL                "CurRBSCL"
#define FIELD_NAME_LAST_FREE_RBS_CL          "LastFreeRBSCL"
#define FIELD_NAME_LATCH_WAIT_TIME           "LatchWaitTime"
#define FIELD_NAME_MSG_SENT_TIME             "MsgSentTime"
#define FIELD_NAME_XOWNER_TID                "XOwnerTID"
#define FIELD_NAME_LATEST_OWNER              "LatestOwner"
#define FIELD_NAME_LATEST_OWNER_MODE         "LatestOwnerMode"
#define FIELD_NAME_NUM_OWNER                 "NumOwner"
#define FIELD_NAME_LATCH_NAME                "LatchName"
#define FIELD_NAME_LATCH_DESC                "LatchDesc"
#define FIELD_NAME_VIEW_HISTORY              "ViewHistory"
#define FIELD_NAME_INDEXVALUE                "IndexValue"
#define FIELD_NAME_PREFIX_NUM                "PrefixNum"
#define FIELD_NAME_IS_ALL_EQUAL              "IsAllEqual"
#define FIELD_NAME_INDEXVALUE_INCLUDED       "IndexValueIncluded"
#define FIELD_NAME_CURRENTID                 "CurrentID"
#define FIELD_NAME_PEERID                    "PeerID"
#define FIELD_NAME_CURRENT_FIELD             "CurrentField"
#define FIELD_NAME_AVGNUM_FIELDS             "AvgNumFields"
#define FIELD_NAME_IS_DEFAULT                "IsDefault"
#define FIELD_NAME_IS_EXPIRED                "IsExpired"
#define FIELD_NAME_DISTINCT_VAL_NUM          "DistinctValNum"
#define FIELD_NAME_VALUES                    "Values"
#define FIELD_NAME_FRAC                      "Frac"
#define FIELD_NAME_NULL_FRAC                 "NullFrac"
#define FIELD_NAME_UNDEF_FRAC                "UndefFrac"
#define FIELD_NAME_MCV                       "MCV"
#define FIELD_NAME_STAT_TIMESTAMP            "StatTimestamp"
#define FIELD_NAME_KEY_PATTERN               "KeyPattern"
#define FIELD_NAME_TOTAL_IDX_LEVELS          "TotalIndexLevels"
#define FIELD_NAME_SAMPLE_RECORDS            "SampleRecords"
#define FIELD_NAME_CHECK_CLIENT_CATA_VERSION "CheckClientCataVersion"
#define FIELD_NAME_SDB_VERSION               "Version"
#define FIELD_NAME_WAITER_TRANSID            "WaiterTransID"
#define FIELD_NAME_HOLDER_TRANSID            "HolderTransID"
#define FIELD_NAME_WAITER_TRANS_COST         "WaiterTransCost"
#define FIELD_NAME_HOLDER_TRANS_COST         "HolderTransCost"
#define FIELD_NAME_WAITTIME                  "WaitTime"
#define FIELD_NAME_COST                      "Cost"
#define FIELD_NAME_DEGREE                    "Degree"
#define FIELD_NAME_DEADLOCKID                "DeadlockID"
#define FIELD_NAME_WAITER_RELATED_ID         "WaiterRelatedID"
#define FIELD_NAME_HOLDER_RELATED_ID         "HolderRelatedID"
#define FIELD_NAME_WAITER_RELATED_SESSIONID  "WaiterRelatedSessionID"
#define FIELD_NAME_HOLDER_RELATED_SESSIONID  "HolderRelatedSessionID"
#define FIELD_NAME_WAITER_RELATED_NODEID     "WaiterRelatedNodeID"
#define FIELD_NAME_HOLDER_RELATED_NODEID     "HolderRelatedNodeID"
#define FIELD_NAME_WAITER_RELATED_GROUPID    "WaiterRelatedGroupID"
#define FIELD_NAME_HOLDER_RELATED_GROUPID    "HolderRelatedGroupID"
#define FIELD_NAME_WAITER_SESSIONID          "WaiterSessionID"
#define FIELD_NAME_HOLDER_SESSIONID          "HolderSessionID"
#define FIELD_NAME_FORCE                     "Force"
#define FIELD_NAME_ISDESTINATION             "IsDestination"

#define FIELD_NAME_CREATE_TIME               "CreateTime"
#define FIELD_NAME_UPDATE_TIME               "UpdateTime"

/// strategy field begin
#define FIELD_NAME_NICE                      "Nice"
#define FIELD_NAME_TASK_NAME                 "TaskName"
#define FIELD_NAME_CONTAINER_NAME            "ContainerName"
#define FIELD_NAME_IP                        "IP"
#define FIELD_NAME_TASK_ID                   "TaskID"
#define FIELD_NAME_SCHDLR_TYPE               "SchdlrType"
#define FIELD_NAME_SCHDLR_TYPE_DESP          "SchdlrTypeDesp"
#define FIELD_NAME_SCHDLR_TIMES              "SchdlrTimes"
#define FIELD_NAME_SCHDLR_MGR_EVT_NUM        "SchdlrMgrEvtNum"
/// strategy field end

#define FIELD_NAME_ANALYZE_MODE              "Mode"
#define FIELD_NAME_ANALYZE_NUM               "SampleNum"
#define FIELD_NAME_ANALYZE_PERCENT           "SamplePercent"

#define FIELD_NAME_DATASOURCE                "DataSource"
#define FIELD_NAME_DATASOURCE_ID             "DataSourceID"
#define FIELD_NAME_MAPPING                   "Mapping"

#define FIELD_OP_VALUE_UPDATE                "update"
#define FIELD_OP_VALUE_REMOVE                "remove"

#define FIELD_OP_VALUE_KEEP                  "keep"
#define FIELD_OP_VALUE_REPLACE               "replace"
#define FIELD_OP_VALUE_SET                   "set"

// For parameters
// Used internal: { $param : paramIndex, $ctype : canonicalType }
#define FIELD_NAME_PARAM                     "$param"
#define FIELD_NAME_CTYPE                     "$ctype"
#define FIELD_NAME_PARAMETERS                "Parameters"

#define FIELD_NAME_DSVERSION                 "DSVersion"
#define FIELD_NAME_ACCESSMODE                "AccessMode"
#define FIELD_NAME_ACCESSMODE_DESC           "AccessModeDesc"
#define FIELD_NAME_ERRORCTLLEVEL             "ErrorControlLevel"
#define FIELD_NAME_ERRORFILTERMASK           "ErrorFilterMask"
#define FIELD_NAME_ERRORFILTERMASK_DESC      "ErrorFilterMaskDesc"
#define FIELD_NAME_TRANS_PROPAGATE_MODE      "TransPropagateMode"
#define FIELD_NAME_INHERIT_SESSION_ATTR      "InheritSessionAttr"

// for recycle-bin
#define FIELD_NAME_RECYCLEBIN                "RecycleBin"
#define FIELD_NAME_RECYCLEIDHWM              "RecycleIDHWM"
#define FIELD_NAME_EXPIRETIME                "ExpireTime"
#define FIELD_NAME_MAXITEMNUM                "MaxItemNum"
#define FIELD_NAME_MAXVERNUM                 "MaxVersionNum"
#define FIELD_NAME_AUTODROP                  "AutoDrop"
#define FIELD_NAME_RECYCLE_NAME              "RecycleName"
#define FIELD_NAME_RETURN_NAME               "ReturnName"
#define FIELD_NAME_ORIGIN_NAME               "OriginName"
#define FIELD_NAME_RECYCLE_ID                "RecycleID"
#define FIELD_NAME_ORIGIN_ID                 "OriginID"
#define FIELD_NAME_RECYCLE_TIME              "RecycleTime"
#define FIELD_NAME_IGNORE_LOCK               "IgnoreLock"
#define FIELD_NAME_SKIPRECYCLEBIN            "SkipRecycleBin"
#define FIELD_NAME_RECYCLE_ISCSRECY          "IsCSRecycled"
#define FIELD_NAME_RECYCLE_ITEM              "RecycleItem"
#define FIELD_NAME_DROP_RECYCLE_ITEM         "DropRecycleItem"
#define FIELD_NAME_REPLACE_CS                "ReplaceCS"
#define FIELD_NAME_REPLACE_CL                "ReplaceCL"
#define FIELD_NAME_RENAME_CS                 "RenameCS"
#define FIELD_NAME_RENAME_CL                 "RenameCL"
#define FIELD_NAME_CHANGEUID_CL              "ChangeUIDCL"


// for role
#define FIELD_NAME_PRIVILEGES                "Privileges"
#define FIELD_NAME_ROLES                     "Roles"
#define FIELD_NAME_RESOURCE                  "Resource"
#define FIELD_NAME_ACTIONS                   "Actions"

#define IXM_FIELD_NAME_KEY                   "key"
#define IXM_FIELD_NAME_NAME                  "name"
#define IXM_FIELD_NAME_UNIQUE                "unique"
#define IXM_FIELD_NAME_UNIQUE1               "Unique"
#define IXM_FIELD_NAME_V                     "v"
#define IXM_FIELD_NAME_ENFORCED              "enforced"
#define IXM_FIELD_NAME_ENFORCED1             "Enforced"
#define IXM_FIELD_NAME_DROPDUPS              "dropDups"
#define IXM_FIELD_NAME_2DRANGE               "2drange"
#define IXM_FIELD_NAME_INDEX_DEF             "IndexDef"
#define IXM_FIELD_NAME_INDEX_FLAG            "IndexFlag"
#define IXM_FIELD_NAME_SCAN_EXTLID           "ScanExtentLID"
#define IXM_FIELD_NAME_SCAN_OFFSET           "ScanOffset"
#define IXM_FIELD_NAME_SORT_BUFFER_SIZE      "SortBufferSize"
#define IXM_FIELD_NAME_NOTNULL               "NotNull"
#define IXM_FIELD_NAME_NOTARRAY              "NotArray"
#define IXM_FIELD_NAME_GLOBAL                "Global"
#define IXM_FIELD_NAME_GLOBAL_OPTION         "GlobalOption"
#define IXM_FIELD_NAME_STANDALONE            "Standalone"
#define IXM_FIELD_NAME_UNIQUEID              "UniqueID"
// global logical time to create index ( add meta data to collection )
#define IXM_FIELD_NAME_CREATETIME            "CreateTime"
// global logical time to finish rebuild index ( scan all data to build index )
#define IXM_FIELD_NAME_REBUILDTIME           "RebuildTime"

#define CMD_ADMIN_PREFIX                     "$"
#define CMD_NAME_BACKUP_OFFLINE              "backup offline"
#define CMD_NAME_CREATE_COLLECTION           "create collection"
#define CMD_NAME_CREATE_COLLECTIONSPACE      "create collectionspace"
#define CMD_NAME_CREATE_SEQUENCE             "create sequence"
#define CMD_NAME_DROP_SEQUENCE               "drop sequence"
#define CMD_NAME_ALTER_SEQUENCE              "alter sequence"
#define CMD_NAME_GET_SEQ_CURR_VAL            "get sequence current value"
#define CMD_NAME_CREATE_INDEX                "create index"
#define CMD_NAME_CANCEL_TASK                 "cancel task"
#define CMD_NAME_DROP_COLLECTION             "drop collection"
#define CMD_NAME_DROP_COLLECTIONSPACE        "drop collectionspace"
#define CMD_NAME_LOAD_COLLECTIONSPACE        "load collectionspace"
#define CMD_NAME_UNLOAD_COLLECTIONSPACE      "unload collectionspace"
#define CMD_NAME_DROP_INDEX                  "drop index"
#define CMD_NAME_COPY_INDEX                  "copy index"
#define CMD_NAME_REPORT_TASK_PROGRESS        "report task progess"
#define CMD_NAME_GET_COUNT                   "get count"
#define CMD_NAME_GET_INDEXES                 "get indexes"
#define CMD_NAME_GET_DATABLOCKS              "get datablocks"
#define CMD_NAME_GET_QUERYMETA               "get querymeta"
#define CMD_NAME_GET_CONFIG                  "get config"
#define CMD_NAME_GET_DCINFO                  "get dcinfo"
#define CMD_NAME_GET_DOMAIN_NAME             "get domain name"
#define CMD_NAME_LIST_COLLECTIONS            "list collections"
#define CMD_NAME_LIST_COLLECTIONSPACES       "list collectionspaces"
#define CMD_NAME_LIST_CONTEXTS               "list contexts"
#define CMD_NAME_LIST_CONTEXTS_CURRENT       "list contexts current"
#define CMD_NAME_LIST_SESSIONS               "list sessions"
#define CMD_NAME_LIST_SESSIONS_CURRENT       "list sessions current"
#define CMD_NAME_LIST_STORAGEUNITS           "list storageunits"
#define CMD_NAME_LIST_GROUPS                 "list groups"
#define CMD_NAME_LIST_DOMAINS                "list domains"
#define CMD_NAME_LIST_CS_IN_DOMAIN           "list collectionspaces in domain"
#define CMD_NAME_LIST_CL_IN_DOMAIN           "list collections in domain"
#define CMD_NAME_LIST_CL_IN_COLLECTIONSPACE  "list collections in collectionspace"
#define CMD_NAME_LIST_USERS                  "list users"
#define CMD_NAME_LIST_BACKUPS                "list backups"
#define CMD_NAME_LIST_TASKS                  "list tasks"
#define CMD_NAME_LIST_INDEXES                CMD_NAME_GET_INDEXES
#define CMD_NAME_LIST_TRANSACTIONS           "list transactions"
#define CMD_NAME_LIST_TRANSACTIONS_CUR       "list transactions current"
#define CMD_NAME_LIST_SVCTASKS               "list service tasks"
#define CMD_NAME_LIST_SEQUENCES              "list sequences"
#define CMD_NAME_LIST_DATASOURCES            "list datasources"
#define CMD_NAME_LIST_RECYCLEBIN             "list recyclebin"
#define CMD_NAME_LIST_GROUPMODES             "list group modes"
#define CMD_NAME_RENAME_COLLECTION           "rename collection"
#define CMD_NAME_RENAME_COLLECTIONSPACE      "rename collectionspace"
#define CMD_NAME_REORG_OFFLINE               "reorg offline"
#define CMD_NAME_REORG_ONLINE                "reorg online"
#define CMD_NAME_REORG_RECOVER               "reorg recover"
#define CMD_NAME_SHUTDOWN                    "shutdown"
#define CMD_NAME_SNAPSHOT_CONTEXTS           "snapshot contexts"
#define CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT   "snapshot contexts current"
#define CMD_NAME_SNAPSHOT_DATABASE           "snapshot database"
#define CMD_NAME_SNAPSHOT_RESET              "snapshot reset"
#define CMD_NAME_SNAPSHOT_SESSIONS           "snapshot sessions"
#define CMD_NAME_SNAPSHOT_SESSIONS_CURRENT   "snapshot sessions current"
#define CMD_NAME_SNAPSHOT_SYSTEM             "snapshot system"
#define CMD_NAME_SNAPSHOT_COLLECTIONS        "snapshot collections"
#define CMD_NAME_SNAPSHOT_COLLECTIONSPACES   "snapshot collectionspaces"
#define CMD_NAME_SNAPSHOT_CATA               "snapshot catalog"
#define CMD_NAME_SNAPSHOT_TRANSACTIONS       "snapshot transactions"
#define CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR   "snapshot transactions current"
#define CMD_NAME_SNAPSHOT_ACCESSPLANS        "snapshot accessplans"
#define CMD_NAME_SNAPSHOT_HEALTH             "snapshot health"
#define CMD_NAME_SNAPSHOT_CONFIGS            "snapshot configs"
#define CMD_NAME_SNAPSHOT_SVCTASKS           "snapshot service tasks"
#define CMD_NAME_SNAPSHOT_SEQUENCES          "snapshot sequences"
#define CMD_NAME_SNAPSHOT_QUERIES            "snapshot queries"
#define CMD_NAME_SNAPSHOT_LATCHWAITS         "snapshot latchwaits"
#define CMD_NAME_SNAPSHOT_LOCKWAITS          "snapshot lockwaits"
#define CMD_NAME_SNAPSHOT_INDEXSTATS         "snapshot index statistics"
#define CMD_NAME_SNAPSHOT_TASKS              "snapshot tasks"
#define CMD_NAME_SNAPSHOT_INDEXES            "snapshot indexes"
#define CMD_NAME_SNAPSHOT_TRANSWAITS         "snapshot waiting transactions"
#define CMD_NAME_SNAPSHOT_TRANSDEADLOCK      "snapshot transaction deadlocks"
#define CMD_NAME_SNAPSHOT_RECYCLEBIN         "snapshot recyclebin"
#define CMD_NAME_TEST_COLLECTION             "test collection"
#define CMD_NAME_TEST_COLLECTIONSPACE        "test collectionspace"
#define CMD_NAME_CREATE_GROUP                "create group"
#define CMD_NAME_REMOVE_GROUP                "remove group"
#define CMD_NAME_CREATE_NODE                 "create node"
#define CMD_NAME_REMOVE_NODE                 "remove node"
#define CMD_NAME_REMOVE_BACKUP               "remove backup"
#define CMD_NAME_UPDATE_NODE                 "update node"
#define CMD_NAME_ACTIVE_GROUP                "active group"
#define CMD_NAME_STARTUP_NODE                "startup node"
#define CMD_NAME_SHUTDOWN_NODE               "shutdown node"
#define CMD_NAME_SHUTDOWN_GROUP              "shutdown group"
#define CMD_NAME_SET_PDLEVEL                 "set pdlevel"
#define CMD_NAME_SPLIT                       "split"
#define CMD_NAME_WAITTASK                    "wait task"
#define CMD_NAME_CREATE_CATA_GROUP           "create catalog group"
#define CMD_NAME_TRACE_START                 "trace start"
#define CMD_NAME_TRACE_RESUME                "trace resume"
#define CMD_NAME_TRACE_STOP                  "trace stop"
#define CMD_NAME_TRACE_STATUS                "trace status"
#define CMD_NAME_CREATE_DOMAIN               "create domain"
#define CMD_NAME_DROP_DOMAIN                 "drop domain"
#define CMD_NAME_ADD_DOMAIN_GROUP            "add domain group"
#define CMD_NAME_REMOVE_DOMAIN_GROUP         "remove domain group"
#define CMD_NAME_EXPORT_CONFIG               "export configuration"
#define CMD_NAME_CRT_PROCEDURE               "create procedure"
#define CMD_NAME_RM_PROCEDURE                "remove procedure"
#define CMD_NAME_LIST_PROCEDURES             "list procedures"
#define CMD_NAME_EVAL                        "eval"
#define CMD_NAME_LINK_CL                     "link collection"
#define CMD_NAME_UNLINK_CL                   "unlink collection"
#define CMD_NAME_SETSESS_ATTR                "set session attribute"
#define CMD_NAME_GETSESS_ATTR                "get session attribute"
#define CMD_NAME_INVALIDATE_CACHE            "invalidate cache"
#define CMD_NAME_INVALIDATE_SEQUENCE_CACHE   "invalidate sequence cache"
#define CMD_NAME_INVALIDATE_DATASOURCE_CACHE "invalidate datasource cache"
#define CMD_NAME_FORCE_SESSION               "force session"
#define CMD_NAME_LIST_LOBS                   "list lobs"
#define CMD_NAME_ALTER_DC                    "alter dc"
#define CMD_NAME_ALTER_USR                   "alter user"
#define CMD_NAME_REELECT                     "reelect"
#define CMD_NAME_REELECT_LOCATION            "reelect location"
#define CMD_NAME_FORCE_STEP_UP               "force step up"
#define CMD_NAME_JSON_LOAD                   "json load"
#define CMD_NAME_TRUNCATE                    "truncate"
#define CMD_NAME_SYNC_DB                     "sync db"
#define CMD_NAME_POP                         "pop"
#define CMD_NAME_RELOAD_CONFIG               "reload config"
#define CMD_NAME_UPDATE_CONFIG               "update config"
#define CMD_NAME_DELETE_CONFIG               "delete config"
#define CMD_NAME_ANALYZE                     "analyze"
#define CMD_NAME_MEM_TRIM                    "mem trim"
#define CMD_NAME_GET_CL_DETAIL               "get collection detail"
#define CMD_NAME_GET_CL_STAT                 "get collection statistic"
#define CMD_NAME_GET_INDEX_STAT              "get index statistic"
#define CMD_NAME_CREATE_DATASOURCE           "create datasource"
#define CMD_NAME_DROP_DATASOURCE             "drop datasource"
#define CMD_NAME_ALTER_DATASOURCE            "alter datasource"

#define CMD_NAME_GET_RECYCLEBIN_DETAIL       "get recyclebin detail"
#define CMD_NAME_GET_RECYCLEBIN_COUNT        "get recyclebin count"
#define CMD_NAME_ALTER_RECYCLEBIN            "alter recyclebin"
#define CMD_NAME_DROP_RECYCLEBIN_ITEM        "drop recyclebin item"
#define CMD_NAME_DROP_RECYCLEBIN_ALL         "drop recyclebin all"
#define CMD_NAME_RETURN_RECYCLEBIN_ITEM      "return recyclebin item"
#define CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME "return recyclebin item to name"

#define CMD_VALUE_NAME_RECYCLEBIN_ENABLE     "enable"
#define CMD_VALUE_NAME_RECYCLEBIN_DISABLE    "disable"
#define CMD_VALUE_NAME_RECYCLEBIN_SETATTR    "set attributes"

/**
 * NOTE:
 * As the following names are used as table names in build-in SQL, so they
 * should be singular. For any new added names, DON'T add 's' to them.
 */
#define CMD_NAME_SNAPSHOT_DATABASE_INTR      "SNAPSHOT_DB"
#define CMD_NAME_SNAPSHOT_SYSTEM_INTR        "SNAPSHOT_SYSTEM"
#define CMD_NAME_SNAPSHOT_COLLECTION_INTR    "SNAPSHOT_CL"
#define CMD_NAME_SNAPSHOT_SPACE_INTR         "SNAPSHOT_CS"
#define CMD_NAME_SNAPSHOT_CONTEXT_INTR       "SNAPSHOT_CONTEXT"
#define CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR    "SNAPSHOT_CONTEXT_CUR"
#define CMD_NAME_SNAPSHOT_SESSION_INTR       "SNAPSHOT_SESSION"
#define CMD_NAME_SNAPSHOT_SESSIONCUR_INTR    "SNAPSHOT_SESSION_CUR"
#define CMD_NAME_SNAPSHOT_CATA_INTR          "SNAPSHOT_CATA"
#define CMD_NAME_SNAPSHOT_TRANS_INTR         "SNAPSHOT_TRANS"
#define CMD_NAME_SNAPSHOT_TRANSCUR_INTR      "SNAPSHOT_TRANS_CUR"
#define CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR   "SNAPSHOT_ACCESSPLANS"
#define CMD_NAME_SNAPSHOT_HEALTH_INTR        "SNAPSHOT_HEALTH"
#define CMD_NAME_SNAPSHOT_CONFIGS_INTR       "SNAPSHOT_CONFIGS"
#define CMD_NAME_SNAPSHOT_SVCTASKS_INTR      "SNAPSHOT_SVCTASKS"
#define CMD_NAME_SNAPSHOT_SEQUENCES_INTR     "SNAPSHOT_SEQUENCES"
#define CMD_NAME_SNAPSHOT_QUERIES_INTR       "SNAPSHOT_QUERIES"
#define CMD_NAME_SNAPSHOT_LATCHWAITS_INTR    "SNAPSHOT_LATCHWAITS"
#define CMD_NAME_SNAPSHOT_LOCKWAITS_INTR     "SNAPSHOT_LOCKWAITS"
#define CMD_NAME_SNAPSHOT_INDEXSTATS_INTR    "SNAPSHOT_INDEXSTATS"
#define CMD_NAME_SNAPSHOT_TASKS_INTR         "SNAPSHOT_TASKS"
#define CMD_NAME_SNAPSHOT_INDEXES_INTR       "SNAPSHOT_INDEXES"
#define CMD_NAME_SNAPSHOT_TRANSWAITS_INTR    "SNAPSHOT_TRANSWAIT"
#define CMD_NAME_SNAPSHOT_TRANSDEADLOCK_INTR "SNAPSHOT_TRANSDEADLOCK"
#define CMD_NAME_SNAPSHOT_RECYCLEBIN_INTR    "SNAPSHOT_RECYCLEBIN"

#define CMD_NAME_LIST_COLLECTION_INTR        "LIST_CL"
#define CMD_NAME_LIST_SPACE_INTR             "LIST_CS"
#define CMD_NAME_LIST_CONTEXT_INTR           "LIST_CONTEXT"
#define CMD_NAME_LIST_CONTEXTCUR_INTR        "LIST_CONTEXT_CUR"
#define CMD_NAME_LIST_SESSION_INTR           "LIST_SESSION"
#define CMD_NAME_LIST_SESSIONCUR_INTR        "LIST_SESSION_CUR"
#define CMD_NAME_LIST_STORAGEUNIT_INTR       "LIST_SU"
#define CMD_NAME_LIST_BACKUP_INTR            "LIST_BACKUP"
#define CMD_NAME_LIST_TRANS_INTR             "LIST_TRANS"
#define CMD_NAME_LIST_TRANSCUR_INTR          "LIST_TRANS_CUR"
#define CMD_NAME_LIST_GROUP_INTR             "LIST_GROUP"
#define CMD_NAME_LIST_GROUPMODES_INTR        "LIST_GROUPMODES"
#define CMD_NAME_LIST_USER_INTR              "LIST_USER"
#define CMD_NAME_LIST_TASK_INTR              "LIST_TASK"
#define CMD_NAME_LIST_INDEXES_INTR           "LIST_INDEXES"
#define CMD_NAME_LIST_DOMAIN_INTR            "LIST_DOMAIN"
#define CMD_NAME_LIST_SVCTASKS_INTR          "LIST_SVCTASKS"
#define CMD_NAME_LIST_SEQUENCES_INTR         "LIST_SEQUENCES"
#define CMD_NAME_LIST_DATASOURCE_INTR        "LIST_DATASOURCE"
#define CMD_NAME_LIST_RECYCLEBIN_INTR        "LIST_RECYCLEBIN"

#define SYS_VIRTUAL_CS                       "SYS_VCS"
#define SYS_VIRTUAL_CS_LEN                   sizeof( SYS_VIRTUAL_CS )
#define SYS_CL_SESSION_INFO                  SYS_VIRTUAL_CS".SYS_SESSION_INFO"

#define SYS_INEXISTENCE_CS                   "SYS_INEXISTENCE_CS"
#define SYS_INEXISTENCE_CL                   "SYS_INEXISTENCE_CL"

#define CMD_VALUE_NAME_CREATE                "create image"
#define CMD_VALUE_NAME_REMOVE                "remove image"
#define CMD_VALUE_NAME_ATTACH                "attach groups"
#define CMD_VALUE_NAME_DETACH                "detach groups"
#define CMD_VALUE_NAME_ENABLE                "enable image"
#define CMD_VALUE_NAME_DISABLE               "disable image"
#define CMD_VALUE_NAME_ACTIVATE              "activate"
#define CMD_VALUE_NAME_DEACTIVATE            "deactivate"
#define CMD_VALUE_NAME_ENABLE_READONLY       "enable readonly"
#define CMD_VALUE_NAME_DISABLE_READONLY      "disable readonly"
#define CMD_VALUE_NAME_SET_ATTRIBUTES        "set attributes"
#define CMD_VALUE_NAME_SET_ACTIVE_LOCATION   "set active location"
#define CMD_VALUE_NAME_SET_LOCATION          "set location"
#define CMD_VALUE_NAME_START_CRITICAL_MODE   "start critical mode"
#define CMD_VALUE_NAME_STOP_CRITICAL_MODE    "stop critical mode"
#define CMD_VALUE_NAME_START_MAINTENANCE_MODE "start maintenance mode"
#define CMD_VALUE_NAME_STOP_MAINTENANCE_MODE  "stop maintenance mode"

/*
   alter user
*/
#define CMD_VALUE_NAME_CHANGEPASSWD          "change passwd"
#define CMD_VALUE_NAME_SETATTR               "set attributes"

#define CMD_VALUE_NAME_SET_CURR_VALUE        "set current value"
#define CMD_VALUE_NAME_RENAME                "rename"
#define CMD_VALUE_NAME_RESTART               "restart"

#define CLS_REPLSET_MAX_NODE_SIZE            7
#define SDB_MAX_MSG_LENGTH                   ( 512 * 1024 * 1024 )

#define SDB_MAX_USERNAME_LENGTH              256
#define SDB_MAX_PASSWORD_LENGTH              256
#define SDB_MAX_TOKEN_LENGTH                 256

#define INVALID_GROUPID                      0
#define CATALOG_GROUPID                      1
#define COORD_GROUPID                        2
#define OM_GROUPID                           3
#define OMAGENT_GROUPID                      4
#define SPARE_GROUPID                        5
#define CATALOG_GROUPNAME                    SYS_PREFIX"CatalogGroup"
#define COORD_GROUPNAME                      SYS_PREFIX"Coord"
#define SPARE_GROUPNAME                      SYS_PREFIX"Spare"
#define OM_GROUPNAME                         SYS_PREFIX"OM"
#define NODE_NAME_SERVICE_SEP                ":"
#define NODE_NAME_SERVICE_SEPCHAR            (((CHAR*)NODE_NAME_SERVICE_SEP)[0])
#define INVALID_NODEID                       0
#define CURRENT_NODEID                       -1
#define SYS_NODE_ID_BEGIN                    1
#define SYS_NODE_ID_END                      ( OM_NODE_ID_BEGIN - 1 )
#define OM_NODE_ID_BEGIN                     800
#define OM_NODE_ID_END                       ( RESERVED_NODE_ID_BEGIN - 1 )
#define RESERVED_NODE_ID_BEGIN               810
#define RESERVED_NODE_ID_END                 ( DATA_NODE_ID_BEGIN - 1 )
#define DATA_NODE_ID_BEGIN                   1000
#define DATA_NODE_ID_END                     ( 60000 + DATA_NODE_ID_BEGIN )
#define DATA_GROUP_ID_BEGIN                  1000
#define DATA_GROUP_ID_END                    ( 60000 + DATA_GROUP_ID_BEGIN )
#define CATA_NODE_MAX_NUM                    CLS_REPLSET_MAX_NODE_SIZE

// Use for location attributes
#define MSG_INVALID_LOCATIONID              0
#define MSG_LOCATION_ID_BEGIN               1
#define MSG_LOCATION_ID_END                 100000
#define MSG_LOCATION_NAMESZ                 256

#define SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE   64

#define MSG_HINT_MARK_LEN                    4

#define SDB_ROLE_DATA_STR                    "data"
#define SDB_ROLE_COORD_STR                   "coord"
#define SDB_ROLE_CATALOG_STR                 "catalog"
#define SDB_ROLE_STANDALONE_STR              "standalone"
#define SDB_ROLE_OM_STR                      "om"
#define SDB_ROLE_OMA_STR                     "cm"

#define SDB_AUTH_USER                        "User"
#define SDB_AUTH_PASSWD                      "Passwd"
#define SDB_AUTH_OLDPASSWD                   "OldPasswd"
#define SDB_AUTH_TEXTPASSWD                  "TextPasswd"
#define SDB_AUTH_EXTENDPASSWD                "ExtendPasswd"
#define SDB_AUTH_SALT                        "Salt"
#define SDB_AUTH_ITERATIONCOUNT              "IterationCount"
#define SDB_AUTH_STOREDKEY                   "StoredKey"
#define SDB_AUTH_STOREDKEYTEXT               "StoredKeyText"
#define SDB_AUTH_STOREDKEYEXTEND             "StoredKeyExtend"
#define SDB_AUTH_SERVERKEY                   "ServerKey"
#define SDB_AUTH_SERVERKEYTEXT               "ServerKeyText"
#define SDB_AUTH_SERVERKEYEXTEND             "ServerKeyExtend"
#define SDB_AUTH_SCRAMSHA256                 "SCRAM-SHA256"
#define SDB_AUTH_SCRAMSHA1                   "SCRAM-SHA1"
#define SDB_AUTH_STEP                        "Step"
#define SDB_AUTH_NONCE                       "Nonce"
#define SDB_AUTH_IDENTIFY                    "Identify"
#define SDB_AUTH_PROOF                       "Proof"
#define SDB_AUTH_TYPE                        "Type"
#define SDB_AUTH_HASHCODE                    "HashCode"

#define SDB_AUTH_STEP_1                      1
#define SDB_AUTH_STEP_2                      2

#define SDB_AUTH_CPP_IDENTIFY                "C++_Session"
#define SDB_AUTH_C_IDENTIFY                  "C_Session"

#define SDB_AUTH_MECHANISM_SS256             "SCRAM-SHA-256"
#define SDB_AUTH_MECHANISM_SS                "SCRAM-SHA"
#define SDB_AUTH_MECHANISM_MD5               "MD5"

enum AUTH_TYPE
{
   SDB_AUTH_TYPE_MD5_PWD = 0,
   SDB_AUTH_TYPE_TEXT_PWD,
   SDB_AUTH_TYPE_EXTEND_PWD
} ;

#define SDB_LOB_OID_LEN                      16

#define SDB_SHARDING_PARTITION_DEFAULT    4096       // 2^12
#define SDB_SHARDING_PARTITION_MIN        8          // 2^3
#define SDB_SHARDING_PARTITION_MAX        1048576    // 2^20

enum SDB_ROLE
{
   SDB_ROLE_DATA = 0,
   SDB_ROLE_COORD,
   SDB_ROLE_CATALOG,
   SDB_ROLE_STANDALONE,
   SDB_ROLE_OM,
   SDB_ROLE_OMA,
   SDB_ROLE_MAX
} ;

enum SDB_LOB_MODE
{
   SDB_LOB_MODE_CREATEONLY = 0x00000001,
   SDB_LOB_MODE_READ       = 0x00000004,
   SDB_LOB_MODE_WRITE      = 0x00000008,
   SDB_LOB_MODE_REMOVE     = 0x00000010,
   SDB_LOB_MODE_TRUNCATE   = 0x00000020,
   SDB_LOB_MODE_SHAREREAD  = 0x00000040,
} ;

enum SDB_CONSISTENCY_STRATEGY
{
   // Write the number of different nodes in the replication group.
   SDB_CONSISTENCY_NODE = 1,
   // Wtite to most locations first, and then write to most nodes
   // of the location where the primary node is located.
   SDB_CONSISTENCY_LOC_MAJOR,
   // Write most nodes of the location where the primary node is
   // located first, and the wtite to most locations.
   SDB_CONSISTENCY_PRY_LOC_MAJOR,
} ;

#define SDB_ANALYZE_MODE_SAMPLE     ( 1 )
#define SDB_ANALYZE_MODE_FULL       ( 2 )
#define SDB_ANALYZE_MODE_GENDFT     ( 3 )
#define SDB_ANALYZE_MODE_RELOAD     ( 4 )
#define SDB_ANALYZE_MODE_CLEAR      ( 5 )

#define SDB_ANALYZE_SAMPLE_MIN      ( 100 )
#define SDB_ANALYZE_SAMPLE_DEF      ( 200 )
#define SDB_ANALYZE_SAMPLE_MAX      ( 10000 )

#define SDB_ALTER_VERSION 1

/// catalog objects
#define SDB_CATALOG_DB        "db"
#define SDB_CATALOG_CS        "collection space"
#define SDB_CATALOG_CL        "collection"
#define SDB_CATALOG_DOMAIN    "domain"
#define SDB_CATALOG_GROUP     "group"
#define SDB_CATALOG_NODE      "node"
#define SDB_CATALOG_SEQ       "sequence"
#define SDB_CATALOG_DATASOURCE "data source"
#define SDB_CATALOG_UNKNOWN   "unknown"

#define SDB_CATALOG_CL_ID_INDEX     "id index"
#define SDB_CATALOG_CL_SHARDING     "sharding"
#define SDB_CATALOG_CL_COMPRESS     "compression"
#define SDB_CATALOG_CL_AUTOINC_FLD  "autoincrement"

#define SDB_CATALOG_CS_DOMAIN    SDB_CATALOG_DOMAIN
#define SDB_CATALOG_CS_CAPPED    "capped"

#define SDB_CATALOG_DOMAIN_GROUPS "groups"
#define SDB_CATALOG_NODE_LOCATION "location"
#define SDB_CATALOG_GROUP_ACTIVE_LOCATION "active location"

#define SDB_ALTER_ACTION_CREATE     "create"
#define SDB_ALTER_ACTION_DROP       "drop"
#define SDB_ALTER_ACTION_ENABLE     "enable"
#define SDB_ALTER_ACTION_DISABLE    "disable"
#define SDB_ALTER_ACTION_ADD        "add"
#define SDB_ALTER_ACTION_SET        "set"
#define SDB_ALTER_ACTION_REMOVE     "remove"
#define SDB_ALTER_ACTION_SET_ATTR   "set attributes"
#define SDB_ALTER_ACTION_INC_VER    "increase version"

#define SDB_ALTER_DELIMITER         " "

/// alter collection
#define CMD_NAME_ALTER_COLLECTION      "alter collection"

/// create id index
#define SDB_ALTER_CL_CRT_ID_INDEX      SDB_ALTER_ACTION_CREATE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_ID_INDEX

/// drop id index
#define SDB_ALTER_CL_DROP_ID_INDEX     SDB_ALTER_ACTION_DROP \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_ID_INDEX

/// create autoincrement field
#define SDB_ALTER_CL_CRT_AUTOINC_FLD   SDB_ALTER_ACTION_CREATE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_AUTOINC_FLD

/// drop autoincrement field
#define SDB_ALTER_CL_DROP_AUTOINC_FLD  SDB_ALTER_ACTION_DROP \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_AUTOINC_FLD

/// enable sharding
#define SDB_ALTER_CL_ENABLE_SHARDING   SDB_ALTER_ACTION_ENABLE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_SHARDING

/// disable sharding
#define SDB_ALTER_CL_DISABLE_SHARDING  SDB_ALTER_ACTION_DISABLE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_SHARDING

/// enable compression
#define SDB_ALTER_CL_ENABLE_COMPRESS   SDB_ALTER_ACTION_ENABLE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_COMPRESS

/// disable compression
#define SDB_ALTER_CL_DISABLE_COMPRESS  SDB_ALTER_ACTION_DISABLE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_CL_COMPRESS

/// set attributes
#define SDB_ALTER_CL_SET_ATTR          SDB_ALTER_ACTION_SET_ATTR

/// increase version
#define SDB_ALTER_CL_INC_VER           SDB_ALTER_ACTION_INC_VER

/// alter collection space
#define CMD_NAME_ALTER_COLLECTION_SPACE   "alter collectionspace"

/// set domain
#define SDB_ALTER_CS_SET_DOMAIN           SDB_ALTER_ACTION_SET \
                                          SDB_ALTER_DELIMITER \
                                          SDB_CATALOG_CS_DOMAIN

/// remove domain
#define SDB_ALTER_CS_REMOVE_DOMAIN        SDB_ALTER_ACTION_REMOVE \
                                          SDB_ALTER_DELIMITER \
                                          SDB_CATALOG_CS_DOMAIN

/// enable capped
#define SDB_ALTER_CS_ENABLE_CAPPED        SDB_ALTER_ACTION_ENABLE \
                                          SDB_ALTER_DELIMITER \
                                          SDB_CATALOG_CS_CAPPED

/// disable capped
#define SDB_ALTER_CS_DISABLE_CAPPED       SDB_ALTER_ACTION_DISABLE \
                                          SDB_ALTER_DELIMITER \
                                          SDB_CATALOG_CS_CAPPED

/// set attributes
#define SDB_ALTER_CS_SET_ATTR             SDB_ALTER_ACTION_SET_ATTR

/// alter domain
#define CMD_NAME_ALTER_DOMAIN          "alter domain"

/// add groups
#define SDB_ALTER_DOMAIN_ADD_GROUPS    SDB_ALTER_ACTION_ADD \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_DOMAIN_GROUPS

/// set groups
#define SDB_ALTER_DOMAIN_SET_GROUPS    SDB_ALTER_ACTION_SET \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_DOMAIN_GROUPS

/// remove groups
#define SDB_ALTER_DOMAIN_REMOVE_GROUPS SDB_ALTER_ACTION_REMOVE \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_DOMAIN_GROUPS

/// set active location
#define SDB_ALTER_DOMAIN_SET_ACTIVE_LOCATION SDB_ALTER_ACTION_SET \
                                             SDB_ALTER_DELIMITER \
                                             SDB_CATALOG_GROUP_ACTIVE_LOCATION

/// set attributes
#define SDB_ALTER_DOMAIN_SET_ATTR      SDB_ALTER_ACTION_SET_ATTR

/// alter node
#define CMD_NAME_ALTER_NODE            "alter node"

/// set location
#define SDB_ALTER_NODE_SET_LOCATION    SDB_ALTER_ACTION_SET \
                                       SDB_ALTER_DELIMITER \
                                       SDB_CATALOG_NODE_LOCATION
/// set location
#define SDB_ALTER_DOMAIN_SET_LOCATION    SDB_ALTER_ACTION_SET \
                                         SDB_ALTER_DELIMITER \
                                         SDB_CATALOG_NODE_LOCATION

/// set attributes
#define SDB_ALTER_NODE_SET_ATTR        SDB_ALTER_ACTION_SET_ATTR

/// alter group
#define CMD_NAME_ALTER_GROUP                 "alter group"

/// set location
#define SDB_ALTER_GROUP_SET_ACTIVE_LOCATION  SDB_ALTER_ACTION_SET \
                                             SDB_ALTER_DELIMITER \
                                             SDB_CATALOG_GROUP_ACTIVE_LOCATION

/// critical mode
#define SDB_ALTER_GROUP_START_CRITICAL_MODE  CMD_VALUE_NAME_START_CRITICAL_MODE
#define SDB_ALTER_GROUP_STOP_CRITICAL_MODE   CMD_VALUE_NAME_STOP_CRITICAL_MODE

/// maintenance mode
#define SDB_ALTER_GROUP_START_MAINTENANCE_MODE  CMD_VALUE_NAME_START_MAINTENANCE_MODE
#define SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE   CMD_VALUE_NAME_STOP_MAINTENANCE_MODE

/// set attributes
#define SDB_ALTER_GROUP_SET_ATTR             SDB_ALTER_ACTION_SET_ATTR

// rbac
#define CMD_NAME_CREATE_ROLE "create role"
#define CMD_NAME_DROP_ROLE "drop role"
#define CMD_NAME_GET_ROLE "get role"
#define CMD_NAME_LIST_ROLES "list roles"
#define CMD_NAME_UPDATE_ROLE "update role"
#define CMD_NAME_GRANT_PRIVILEGES "grant privileges"
#define CMD_NAME_REVOKE_PRIVILEGES "revoke privileges"
#define CMD_NAME_GRANT_ROLES_TO_ROLE "grant roles to role"
#define CMD_NAME_REVOKE_ROLES_FROM_ROLE "revoke roles from role"
#define CMD_NAME_GET_USER "get user"
#define CMD_NAME_GRANT_ROLES_TO_USER "grant roles to user"
#define CMD_NAME_REVOKE_ROLES_FROM_USER "revoke roles from user"
#define CMD_NAME_INVALIDATE_USER_CACHE "invalidate user cache"

#endif // MSGDEF_H__
