/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = msgCatalogDef.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MSGCATALOGDEF_HPP__
#define MSGCATALOGDEF_HPP__

#include "msgDef.h"

#define CAT_TYPE_FIELD_NAME              FIELD_NAME_ROLE
#define CAT_HOST_FIELD_NAME              FIELD_NAME_HOST
#define CAT_SERVICE_FIELD_NAME           FIELD_NAME_SERVICE
#define CAT_SERVICE_TYPE_FIELD_NAME      FIELD_NAME_SERVICE_TYPE
#define CAT_SERVICE_NAME_FIELD_NAME      FIELD_NAME_NAME
#define CAT_IP_FIELD_NAME                "IP"

#define CAT_GROUPID_NAME                  FIELD_NAME_GROUPID
#define CAT_GROUPNAME_NAME                FIELD_NAME_GROUPNAME
#define CAT_NODEID_NAME                   FIELD_NAME_NODEID
#define CAT_ROLE_NAME                     FIELD_NAME_ROLE
#define CAT_GROUP_NAME                    FIELD_NAME_GROUP
#define CAT_GROUPS_NAME                   FIELD_NAME_GROUPS
#define CAT_VERSION_NAME                  FIELD_NAME_VERSION
#define CAT_PRIMARY_NAME                  FIELD_NAME_PRIMARY
#define CAT_GROUP_STATUS                  FIELD_NAME_GROUP_STATUS

#define CAT_DOMAIN_NAME                   FIELD_NAME_DOMAIN
#define CAT_DOMAINNAME_NAME               FIELD_NAME_NAME

#define CAT_COLLECTION_SPACE_NAME         FIELD_NAME_NAME
#define CAT_COLLECTION_NAME               FIELD_NAME_NAME
#define CAT_CS_UNIQUEID                   FIELD_NAME_UNIQUEID
#define CAT_CL_UNIQUEID                   FIELD_NAME_UNIQUEID
#define CAT_CS_CLUNIQUEHWM                "CLUniqueHWM"
#define CAT_PAGE_SIZE_NAME                FIELD_NAME_PAGE_SIZE
#define CAT_COLLECTION                    FIELD_NAME_COLLECTION
#define CAT_COLLECTIONSPACE               FIELD_NAME_COLLECTIONSPACE

#define CAT_COLLECTION_SPACE_OLDNAME      FIELD_NAME_OLDNAME
#define CAT_COLLECTION_SPACE_NEWNAME      FIELD_NAME_NEWNAME
#define CAT_COLLECTION_OLDNAME            FIELD_NAME_OLDNAME
#define CAT_COLLECTION_NEWNAME            FIELD_NAME_NEWNAME

#define CAT_CATALOGNAME_NAME              FIELD_NAME_NAME
#define CAT_CATALOG_W_NAME                FIELD_NAME_W
#define CAT_CATALOGINFO_NAME              FIELD_NAME_CATALOGINFO
#define CAT_CATALOGGROUPID_NAME           FIELD_NAME_GROUPID
#define CAT_CATALOGVERSION_NAME           FIELD_NAME_VERSION
#define CAT_SHARDINGKEY_NAME              FIELD_NAME_SHARDINGKEY
#define CAT_LOBSHARDINGKEYFORMAT_NAME     FIELD_NAME_LOBSHARDINGKEY_FORMAT
#define CAT_ENSURE_SHDINDEX               FIELD_NAME_ENSURE_SHDINDEX
#define CAT_COMPRESSED                    FIELD_NAME_COMPRESSED
#define CAT_COMPRESSIONTYPE               FIELD_NAME_COMPRESSIONTYPE
#define CAT_STRICTDATAMODE                FIELD_NAME_STRICTDATAMODE
#define CAT_COMPRESSOR_SNAPPY             VALUE_NAME_SNAPPY
#define CAT_COMPRESSOR_LZW                VALUE_NAME_LZW
#define CAT_COMPRESSOR_ZLIB               VALUE_NAME_ZLIB
#define CAT_COMPRESSOR_LZ4                VALUE_NAME_LZ4
#define CAT_IS_MAINCL                     FIELD_NAME_ISMAINCL
#define CAT_MAINCL_NAME                   FIELD_NAME_MAINCLNAME
#define CAT_SUBCL_NAME                    FIELD_NAME_SUBCLNAME
#define CAT_LOWBOUND_NAME                 FIELD_NAME_LOWBOUND
#define CAT_UPBOUND_NAME                  FIELD_NAME_UPBOUND
#define CAT_OPTIONS_NAME                  FIELD_NAME_OPTIONS
#define CAT_GROUP_SPECIFIED               FIELD_NAME_GROUP
#define CAT_SHARDING_TYPE                 FIELD_NAME_SHARDTYPE
#define CAT_SHARDING_TYPE_HASH            FIELD_NAME_SHARDTYPE_HASH
#define CAT_SHARDING_TYPE_RANGE           FIELD_NAME_SHARDTYPE_RANGE
#define CAT_SHARDING_PARTITION            FIELD_NAME_PARTITION
#define CAT_AUTOINCREMENT                 FIELD_NAME_AUTOINCREMENT
#define CAT_AUTOINC_FIELD                 FIELD_NAME_AUTOINC_FIELD
#define CAT_AUTOINC_SEQ                   FIELD_NAME_AUTOINC_SEQ
#define CAT_AUTOINC_SEQ_ID                FIELD_NAME_AUTOINC_SEQ_ID
#define CAT_AUTOINC_GENERATED             FIELD_NAME_GENERATED
#define CAT_GENERATED_ALWAYS              VALUE_NAME_ALWAYS
#define CAT_GENERATED_STRICT              VALUE_NAME_STRICT
#define CAT_GENERATED_DEFAULT             VALUE_NAME_DEFAULT

#define CAT_INVALID_GROUPID               INVALID_GROUPID
#define CAT_CATALOG_GROUPID               CATALOG_GROUPID
#define CAT_CATALOG_GROUPNAME             CATALOG_GROUPNAME
#define CAT_INVALID_NODEID                INVALID_NODEID
#define CAT_DATA_NODE_ID_BEGIN            DATA_NODE_ID_BEGIN
#define CAT_DATA_GROUP_ID_BEGIN           DATA_GROUP_ID_BEGIN

#define CAT_SOURCE_NAME                   FIELD_NAME_SOURCE
#define CAT_TARGET_NAME                   FIELD_NAME_TARGET
#define CAT_SPLITQUERY_NAME               FIELD_NAME_SPLITQUERY
#define CAT_SPLITENDQUERY_NAME            FIELD_NAME_SPLITENDQUERY
#define CAT_SPLITPERCENT_NAME             FIELD_NAME_SPLITPERCENT
#define CAT_SPLITVALUE_NAME               FIELD_NAME_SPLITVALUE
#define CAT_SPLITENDVALUE_NAME            FIELD_NAME_SPLITENDVALUE
#define CAT_TASKTYPE_NAME                 FIELD_NAME_TASKTYPE
#define CAT_TASKID_NAME                   FIELD_NAME_TASKID
#define CAT_SOURCEID_NAME                 FIELD_NAME_SOURCEID
#define CAT_TARGETID_NAME                 FIELD_NAME_TARGETID
#define CAT_STATUS_NAME                   FIELD_NAME_STATUS
#define CAT_ATTRIBUTE_NAME                FIELD_NAME_ATTRIBUTE
#define CAT_LOB_PAGE_SZ_NAME              FIELD_NAME_LOB_PAGE_SZ

#define CAT_PROCEDURES_NAME               FIELD_NAME_FUNC_NAME
#define CAT_PROCEDURES_VALUE              FIELD_NAME_FUNC_VALUE

#define CAT_DOMAIN_AUTO_SPLIT             FIELD_NAME_DOMAIN_AUTO_SPLIT
#define CAT_DOMAIN_AUTO_REBALANCE         FIELD_NAME_DOMAIN_AUTO_REBALANCE
#define CAT_AUTO_INDEX_ID                 FIELD_NAME_AUTO_INDEX_ID
#define CAT_INTERNAL_VERSION              FIELD_NAME_INTERNAL_VERSION

#define CAT_INCLUDE_SUBCL                 "IncludeSubCL"

#define CAT_INTERNAL_VERSION_2            (2)
#define CAT_INTERNAL_VERSION_3            (3)

#define CAT_ASSIGNGROUP_FOLLOW            "$follow"
#define CAT_ASSIGNGROUP_RANDOM            "$random"

#define CAT_DELAY_REPLY_TYPE_NAME         "DELAYEVENT"
#define CAT_DELAY_REPLY_MSG_NAME          "DELAYREPLY"
#define CAT_DELAY_SYNC_LSN_NAME           "DELAYSYNCLSN"
#define CAT_DELAY_SYNC_W_NAME             "DELAYSYNCW"

#define CAT_TYPE_NAME                     FIELD_NAME_TYPE
#define CAT_CAPPED_NAME                   FIELD_NAME_CAPPED
#define CAT_CL_MAX_SIZE                   FIELD_NAME_SIZE
#define CAT_CL_MAX_RECNUM                 FIELD_NAME_MAX
#define CAT_CL_OVERWRITE                  FIELD_NAME_OVERWRITE

#endif // MSGCATALOGDEF_HPP__
