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

   Source File Name = catDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CATDEF_HPP__
#define CATDEF_HPP__

#include "msgCatalog.hpp"
#include "fmpDef.hpp"
#include "utilUniqueID.hpp"

/*
   SYSCAT CollectionSpace define
*/
#define CAT_SYS_SPACE_NAME                "SYSCAT"
#define CAT_NODE_INFO_COLLECTION          CAT_SYS_SPACE_NAME".SYSNODES"
#define CAT_COLLECTION_SPACE_COLLECTION   CAT_SYS_SPACE_NAME".SYSCOLLECTIONSPACES"
#define CAT_COLLECTION_INFO_COLLECTION    CAT_SYS_SPACE_NAME".SYSCOLLECTIONS"
#define CAT_TASK_INFO_COLLECTION          CAT_SYS_SPACE_NAME".SYSTASKS"
#define CAT_INDEX_INFO_COLLECTION         CAT_SYS_SPACE_NAME".SYSINDEXES"
#define CAT_DOMAIN_COLLECTION             CAT_SYS_SPACE_NAME".SYSDOMAINS"
#define CAT_HISTORY_COLLECTION            CAT_SYS_SPACE_NAME".SYSHISTORY"
#define CAT_DATASOURCE_COLLECTION         CAT_SYS_SPACE_NAME".SYSDATASOURCES"
#define CAT_GROUP_MODE_COLLECTION         CAT_SYS_SPACE_NAME".SYSGROUPMODES"

#define CAT_NODEINFO_GROUPNAMEIDX         "{name:\"SYSIDX1\",key: {"\
                                          CAT_GROUPNAME_NAME":1}, unique: true, enforced: true } "
#define CAT_NODEINFO_GROUPIDIDX           "{name:\"SYSIDX2\",key: {"\
                                          CAT_GROUPID_NAME":1}, unique: true, enforced: true } "
#define CAT_COLLECTION_SPACE_NAMEIDX      "{name:\"SYSIDX1\",key: {"\
                                          CAT_COLLECTION_SPACE_NAME":1}, unique: true, enforced: true } "
#define CAT_COLLECTION_SPACE_IDIDX        "{name:\"SYSIDX2\",key: {"\
                                          CAT_CS_UNIQUEID":1}, unique: true, enforced: false } "
#define CAT_COLLECTION_NAMEIDX            "{name:\"SYSIDX1\",key: {"\
                                          CAT_COLLECTION_NAME":1}, unique: true, enforced: true } "
#define CAT_COLLECTION_IDIDX              "{name:\"SYSIDX2\",key: {"\
                                          CAT_CL_UNIQUEID":1}, unique: true, enforced: false } "
#define CAT_COLLECTION_MAINCLIDX          "{name:\"SYSIDX3\",key: {"\
                                          CAT_MAINCL_NAME":1}} "
#define CAT_TASK_INFO_TASKIDIDX           "{name:\"SYSIDX1\",key: {"\
                                          CAT_TASKID_NAME":1}, unique: true }"
#define CAT_TASK_INFO_MAINTASKIDIDX       "{name:\"SYSIDX2\",key: {"\
                                          FIELD_NAME_MAIN_TASKID":1} }"
#define CAT_TASK_INFO_NAMEIDX             "{name:\"SYSIDX3\",key: {"\
                                          FIELD_NAME_NAME":1} }"
#define CAT_INDEX_INFO_NAMEIDX            "{name:\"SYSIDX1\",key: {"\
                                          FIELD_NAME_COLLECTION":1, " FIELD_NAME_NAME":1 }, unique: true }"
#define CAT_DOMAIN_NAMEIDX                "{name:\"SYSIDX1\",key: {"\
                                          CAT_DOMAINNAME_NAME":1}, unique: true, enforced: true } "
#define CAT_HISTORY_BUCKETID_IDX          "{name:\"SYSIDX1\",key: {"\
                                          FIELD_NAME_BUCKETID":1}, unique: true, enforced: true } "
#define CAT_GROUP_MODE_GROUPID_IDX        "{name:\"SYSIDX1\",key: {"\
                                          CAT_GROUPID_NAME":1}, unique: true, enforced: true } "

#define CAT_MATCHER_HOST_NAME             CAT_HOST_FIELD_NAME
#define CAT_MATCHER_NODEID_NAME           CAT_GROUP_NAME "." CAT_NODEID_NAME
#define CAT_MATCHER_SERVICE_NAME          CAT_SERVICE_FIELD_NAME "." CAT_SERVICE_NAME_FIELD_NAME

#define READ_BUFFER_SIZE                  8192

#define CAT_VERSION_BEGIN                 1

#define CAT_HASH_LOW_BOUND                0

#define CAT_SHARDING_PARTITION_DEFAULT    SDB_SHARDING_PARTITION_DEFAULT
#define CAT_SHARDING_PARTITION_MIN        SDB_SHARDING_PARTITION_MIN
#define CAT_SHARDING_PARTITION_MAX        SDB_SHARDING_PARTITION_MAX

#define CAT_BUCKET_SIZE                   ( 200000 )
#define FIELD_NAME_BUCKETID               "BucketID"

#define CAT_SYS_DOMAIN_NAME               SYS_PREFIX"DOMAIN"

#define CAT_DATASOURCE_IDIDX              "{name:\"SYSIDX1\",key: {"\
                                          FIELD_NAME_ID":1}, unique: true, enforced: true } "
#define CAT_DATASOURCE_NAMEIDX            "{name:\"SYSIDX2\",key: {"\
                                          FIELD_NAME_NAME":1}, unique: true, enforced: true } "

/*
   SYSPROCEDURES CollectionSpace define
*/
#define CAT_PROCEDURES_SPACE_NAME         "SYSPROCEDURES"
#define CAT_PROCEDURES_COLLECTION         CAT_PROCEDURES_SPACE_NAME".STOREPROCEDURES"

#define CAT_PROCEDURES_COLLECTION_INDEX   "{name:\"PROCEDUREIDX1\", key: {"\
                                          FMP_FUNC_NAME":1}, unique: true, enforced: true } "

/*
   SYSINFO CollectionSpace define
*/
#define CAT_SYSINFO_SPACE_NAME            "SYSINFO"
#define CAT_SYSDCBASE_COLLECTION_NAME     CAT_SYSINFO_SPACE_NAME".SYSDCBASE"

#define CAT_DCBASEINFO_TYPE_INDEX         "{name:\"SYSTYPE\",key: {"\
                                          FIELD_NAME_TYPE":1}, unique: true, enforced: true } "

#define CAT_BASE_TYPE_GLOBAL_STR          "GLOBAL"

#define CAT_SYSLOG_COLLECTION_NAME        CAT_SYSINFO_SPACE_NAME".SYSLOG"
#define CAT_SYSLOG_CL_NUM                 ( 5 )
#define CAT_SYSLOG_CL_MAX_COUNT           ( 100000 )

#define CAT_SYSLOG_TYPE_LSNVER            "{name:\"SYSLSNVER\",key: {"\
                                          FIELD_NAME_LSN_VERSION":1}, unique: false } "
#define CAT_SYSLOG_TYPE_LSNOFF            "{name:\"SYSLSNOFF\",key: {"\
                                          FIELD_NAME_LSN_OFFSET":1}, unique: true, enforced: true } "

// collection space for recycle bin
#define CAT_SYSRECYCLEBIN_SPACE_NAME      "SYSRECYCLEBIN"

// collection for recycle bin item
#define CAT_SYSRECYCLEBIN_ITEM_COLLECTION CAT_SYSRECYCLEBIN_SPACE_NAME".SYSRECYCLEITEMS"

// collection for recycle collection space
#define CAT_SYSRECYCLEBIN_CS_COLLECTION   CAT_SYSRECYCLEBIN_SPACE_NAME".SYSCOLLECTIONSPACES"

// collection for recycle collection
#define CAT_SYSRECYCLEBIN_CL_COLLECTION   CAT_SYSRECYCLEBIN_SPACE_NAME".SYSCOLLECTIONS"

// collection for recycle sequence ( auto-incremental field )
#define CAT_SYSRECYCLEBIN_SEQ_COLLECTION  CAT_SYSRECYCLEBIN_SPACE_NAME".SYSSEQUENCES"

// collection for recycle index
#define CAT_SYSRECYCLEBIN_IDX_COLLECTION  CAT_SYSRECYCLEBIN_SPACE_NAME".SYSINDEXES"

namespace engine
{
   typedef std::vector<UINT32> CAT_GROUP_LIST ;
   typedef CAT_GROUP_LIST::iterator CAT_GROUP_LIST_IT ;

   typedef SET_UINT32 CAT_GROUP_SET ;
   typedef CAT_GROUP_SET::iterator CAT_GROUP_SET_IT ;

   typedef ossPoolList<PAIR_CLNAME_ID> CAT_PAIR_CLNAME_ID_LIST ;
   typedef CAT_PAIR_CLNAME_ID_LIST::iterator CAT_PAIR_CLNAME_ID_LIST_IT ;
   typedef CAT_PAIR_CLNAME_ID_LIST::const_iterator CAT_PAIR_CLNAME_ID_LIST_CIT ;
}

#endif // CATDEF_HPP__
