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

   Source File Name = seAdptDef.hpp

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_DEF_HPP_
#define SEADPT_DEF_HPP_

namespace seadapter
{
   // Search engine definitions.
   #define SEADPT_PROCESS_NAME            "sdbseadapter"
   #define SEADPT_ROLE_SHORT_STR          "A"

   // Related with adapter configurations.
   #define SEADPT_DNODE_HOST              "datanodehost"
   #define SEADPT_DNODE_PORT              "datasvcname"
   #define SEADPT_DIAGLEVEL               "diaglevel"
   #define SEADPT_SE_HOST                 "searchenginehost"
   #define SEADPT_SE_PORT                 "searchengineport"
   #define SEADPT_SE_IDXPREFIX            "idxprefix"
   #define SEADPT_BULK_BUFF_SIZE          "bulkbuffsize"
   #define SEADPT_STR_MAP_TYPE            "stringmaptype"
   #define SEADPT_CONN_LIMIT              "connlimit"
   #define SEADPT_CONN_TIMEOUT            "conntimeout"
   #define SEADPT_SCROLL_SIZE             "scrollsize"

   #define SEADPT_DFT_TIMEOUT             10000
   #define SEADPT_DFT_BULKBUFF_SZ         10
   #define SEADPT_DFT_STR_MAP_TYPE        1
   #define SEADPT_SE_DFT_SERVICE          "9200"
   #define SEADPT_SE_SVCADDR_MAX_SZ       32
   #define SEADPT_DFT_CONN_LIMIT          50
   #define SEADPT_DFT_CONN_TIMEOUT        1800
   #define SEADPT_DFT_SCROLL_SIZE         1000

   // Route id for the adapter, just use one that will not conflict with sdb
   // nodes.
   #define SEADPT_GRP_ID                  65536
   #define SEADPT_NODE_ID                 0
   #define SEADPT_SVC_ID                  0

   #define SEADPT_MAX_IDX_NUM             64
   #define SEADPT_MAX_IDXNAME_SZ          255
   #define SEADPT_MAX_IDXPREFIX_SZ        16
   #define SEADPT_MAX_TYPE_SZ             127
   #define SEADPT_MAX_ID_SZ               512
   #define SEADPT_COMMIT_ID               "SDBCOMMIT"

   #define SEADPT_FIELD_NAME_ID          "_id"
   #define SEADPT_FIELD_NAME_RID         "_rid"
   #define SEADPT_FIELD_NAME_RID_NEW     "_ridNew"
   #define SEADPT_FIELD_NAME_CLUID       "_cluid"
   #define SEADPT_FIELD_NAME_LID         "_lid"
   #define SEADPT_FIELD_NAME_CLLID       "_cllid"
   #define SEADPT_FIELD_NAME_IDXLID      "_idxlid"
   #define SEADPT_FIELD_NAME_HASH        "_hash"
   #define SEADPT_FIELD_NAME_CAPPEDCL    "CappedCL"

   #define SEADPT_OPERATOR_STR_OR        "$or"
   #define SEADPT_OPERATOR_STR_EXIST     "$exists"
   #define SEADPT_OPERATOR_STR_INCLUDE   "$include"

   #define SEADPT_FETCH_MAX_SIZE         100000
   #define SEADPT_ES_ID_FILTER_PATH      "filter_path=_scroll_id,hits.hits._id"
   #define SEADPT_INVALID_LID            -1

   #define SEADPT_EXE_FILE_NAME          "sdbseadapter"
   #define SEADPT_CFG_FILE_NAME          SEADPT_EXE_FILE_NAME".conf"
   #define SEADPT_LOG_DIR                "sdbseadapterlog"
   #define SEADPT_LOG_FILE_NAME          SEADPT_EXE_FILE_NAME".log"
   #define SEADPT_LOCK_FILE_NAME         ".sdbseadapter.lock"
}

#endif /* SEADPT_DEF_HPP_ */

