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

   Source File Name = fmpDef.h

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef FMPDEF_H_
#define FMPDEF_H_

#include "msgDef.h"
#include "spd.h"

#if defined( _WINDOWS )
#define SPD_PROCESS_NAME      "sdbfmp.exe"
#else
#define SPD_PROCESS_NAME      "sdbfmp"
#endif // _WINDOWS

#define PD_FMP_DIAGLOG_PREFIX       "fmpdiag"
#define PD_FMP_DIAGLOG_SUBFIX       "log"

/// WARNING: do not modify this define.
/// spdFMP.cpp::SPD_NEXT is depend on this define.
#define FMP_MSG_MAGIC               {(CHAR)0xFF, (CHAR)0xFE, (CHAR)0xFD, (CHAR)0xFB, 0}

#define FMP_FUNC_VALUE        FIELD_NAME_FUNC
#define FMP_FUNC_NAME         "name"
#define FMP_FUNC_TYPE         FIELD_NAME_FUNCTYPE
#define FMP_ERR_MSG           "errmsg"
#define FMP_RES_TYPE          "resType"
#define FMP_RES_VALUE         "value"
#define FMP_RES_CODE          "retCode"
#define FMP_RES_CLASSNAME     "className"
#define FMP_CONTROL_FIELD     "step"
#define FMP_DIAG_PATH         "diag"
#define FMP_SEQ_ID            "seqid"
#define FMP_FUNCTION_DEF      "function"
#define FMP_LOCAL_SERVICE     "service"
#define FMP_LOCAL_USERNAME    "username"
#define FMP_LOCAL_PASSWORD    "password"

#define FMP_CONTROL_STEP_INVALID    -3
#define FMP_CONTROL_STEP_QUIT       -2
#define FMP_CONTROL_STEP_RESET      -1
#define FMP_CONTROL_STEP_BEGIN      0
#define FMP_CONTROL_STEP_DOWNLOAD   1
#define FMP_CONTROL_STEP_EVAL       2
#define FMP_CONTROL_STEP_FETCH      3
/// when add step, max must be changed.
#define FMP_CONTROL_SETP_MAX        4


#define FMP_RES_TYPE_VOID        SDB_SPD_RES_TYPE_VOID
#define FMP_RES_TYPE_STR         SDB_SPD_RES_TYPE_STR
#define FMP_RES_TYPE_NUMBER      SDB_SPD_RES_TYPE_NUMBER
#define FMP_RES_TYPE_OBJ         SDB_SPD_RES_TYPE_OBJ
#define FMP_RES_TYPE_BOOL        SDB_SPD_RES_TYPE_BOOL
#define FMP_RES_TYPE_RECORDSET   SDB_SPD_RES_TYPE_RECORDSET
#define FMP_RES_TYPE_SPECIALOBJ  SDB_SPD_RES_TYPE_SPECIALOBJ
#define FMP_RES_TYPE_NULL        SDB_SPD_RES_TYPE_NULL

#define FMP_FUNC_TYPE_INVALID    -1
#define FMP_FUNC_TYPE_JS         0
#define FMP_FUNC_TYPE_C          1
#define FMP_FUNC_TYPE_JAVA       2

#endif

