/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = clsFSDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSFSDEF_HPP_
#define CLSFSDEF_HPP_

#include "core.hpp"

namespace engine
{

   #define CLS_FS_VALIDCLS                "validcls"
   #define CLS_FS_COMMITFLAG              "commitflag"
   #define CLS_FS_COMMITLSN               "commitlsn"
   #define CLS_FS_CS_NAME                 "cs"
   #define CLS_FS_COLLECTION_NAME         "collection"
   #define CLS_FS_CS_META_NAME            "csmeta"
   #define CLS_FS_PAGE_SIZE               "pagesize"
   #define CLS_FS_KEYOBJ                  "keyobj"
   #define CLS_FS_END_KEYOBJ              "endkeyobj"
   #define CLS_FS_NOMORE                  "nomore"
   #define CLS_FS_INDEXES                 "indexes"
   #define CLS_FS_FULLNAME                "fullname"
   #define CLS_FS_FULLNAMES               "fullnames"
   #define CLS_FS_CSNAME                  "csname"
   #define CLS_FS_CSNAMES                 "csnames"
   #define CLS_FS_NEEDDATA                "needdata"
   #define CLS_FS_ATTRIBUTES              "attributes"
   #define CLS_FS_LOB_PAGE_SIZE           "lobpagesize"
   #define CLS_FS_CS_TYPE                 "cstype"
   #define CLS_FS_COMP_TYPE               "comptype"
   #define CLS_FS_EXT_OPTION              "extoption"
   #define CLS_FS_CL_MAX_SIZE             "maxsize"
   #define CLS_FS_CL_MAX_RECNUM           "maxrecnum"

   enum CLS_FS_STATUS
   {
      CLS_FS_STATUS_NONE = 0,
      CLS_FS_STATUS_BEGIN,
      CLS_FS_STATUS_META,
      CLS_FS_STATUS_INDEX,
      CLS_FS_STATUS_NOTIFY_DOC,
      CLS_FS_STATUS_NOTIFY_LOB,
      CLS_FS_STATUS_NOTIFY_LOG,
      CLS_FS_STATUS_END
   } ;
}

#endif

