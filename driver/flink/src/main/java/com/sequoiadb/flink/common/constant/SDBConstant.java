/*
 * Copyright 2022 SequoiaDB Inc.
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

package com.sequoiadb.flink.common.constant;

public class SDBConstant {

    public static final String GROUP_NAME = "GroupName";
    public static final String GROUP = "Group";

    public static final String COORD_GROUP = "SYSCoord";
    public static final String CATALOG_GROUP = "SYSCatalogGroup";

    public static final String PRIMARY_NODE = "PrimaryNode";

    public static final String NODE_ID = "NodeID";
    public static final String NODE_NAME = "NodeName";
    public static final String NODE_STATUS = "Status";
    public static final String NODE_FLAG = "Flag";
    public static final String HOSTNAME = "HostName";
    public static final String SERVICE = "Service";
    public static final String SERVICE_TYPE = "Type";
    public static final String NAME = "Name";
    public static final String SERVICE_NAME = "ServiceName";
    public static final String SERVICE_STATUS = "ServiceStatus";
    public static final String INSTANCE_ID = "instanceid";

    public static final String SUB_COLLECTIONS = "SubCollections";
    public static final String CL_FULL_NAME = "Name";
    public static final String SCAN_TYPE = "ScanType";

    public static final String ERR_NODES = "ErrNodes";

    public static final String DATA_BLOCKS = "Datablocks";

    public static final String TRANS_MAX_LOCK_NUM = "TransMaxLockNum";
    public static final String PAGE_SIZE = "PageSize";
    public static final String DOMAIN = "Domain";

    public static final String SHARDING_KEY = "ShardingKey";
    public static final String SHARDING_TYPE = "ShardingType";
    public static final String HASH_SHARDING_TYPE = "hash";

    public static final String REPL_SIZE = "ReplSize";
    public static final String COMPRESSION_TYPE = "CompressionType";
    public static final String AUTO_SPLIT = "AutoSplit";
    public static final String ENSURE_SHARDING_INDEX = "EnsureShardingIndex";

    public static final String INDEX_DEF = "IndexDef";
    public static final String UNIQUE = "unique";
    public static final String KEY = "key";

    public static final String INDEX_UNIQUE = "Unique";
    public static final String INDEX_NOT_NULL = "NotNull";

    // BSON Matcher Type
    public static final String AND = "$and";
    public static final String OR = "$or";
    public static final String IN = "$in";
    public static final String NIN = "$nin";
    public static final String NOT = "$not";
    public static final String GT = "$gt";
    public static final String GTE = "$gte";
    public static final String LT = "$lt";
    public static final String LTE = "$lte";
    public static final String ET = "$et";
    public static final String NE = "$ne";
    public static final String IS_NULL = "$isnull";

    public static final String IN_MERGE = "in";
    public static final String NIN_MERGE = "nin";


    //Lookup
    public static final String INDEX_DEF_KEY_NAME = "name";

    public static final String SDB_ID_INDEX = "$id";
}
