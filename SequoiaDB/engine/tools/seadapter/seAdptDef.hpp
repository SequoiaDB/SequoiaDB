/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
   #define SDB_SEADPT_PROCESS_NAME     "sdbseadapter"
   #define SDB_SEADPT_ROLE_SHORT_STR   "A"
   #define SDB_SEADPT_DNODE_HOST       "datanodehost"
   #define SDB_SEADPT_DNODE_PORT       "datasvcname"
   #define SDB_SEADPT_DIAGLEVEL        "diaglevel"
   #define SDB_SEADPT_SE_HOST          "searchenginehost"
   #define SDB_SEADPT_SE_PORT          "searchengineport"
   #define SDB_SEADPT_GRP_ID           65536
   #define SDB_SEADPT_NODE_ID          0
   #define SDB_SEADPT_SVC_ID           0
   #define SDB_SEADPT_MAX_IDXNAME_SZ   255
   #define SDB_SEADPT_COMMIT_ID        "SDBCOMMIT"

   #define SDB_SEADPT_EXE_FILE_NAME    "sdbseadapter"
   #define SDB_SEADPT_CFG_FILE_NAME    SDB_SEADPT_EXE_FILE_NAME".conf"
   #define SDB_SEADPT_LOG_DIR          "sdbseadapterlog"
   #define SDB_SEADPT_LOG_FILE_NAME    SDB_SEADPT_EXE_FILE_NAME".log"
}

#endif /* SEADPT_DEF_HPP_ */

