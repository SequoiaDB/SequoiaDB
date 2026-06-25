/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = rplConfDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_CONF_DEF_HPP_
#define REPLAY_CONF_DEF_HPP_

#include "oss.hpp"

namespace replay
{
   // global
   #define RPL_CONF_OUTPUT_TYPE             "outputType"
   #define RPL_CONF_NAME_OUTPUT_DIR         "outputDir"
   #define RPL_CONF_NAME_PREFIX             "filePrefix"
   #define RPL_CONF_NAME_SUFFIX             "fileSuffix"
   #define RPL_CONF_SUBMIT_TIME             "submitTime"
   #define RPL_CONF_SUBMIT_INTERVAL         "submitInterval"
   #define RPL_CONF_NAME_DELIMITER          "delimiter"
   #define RPL_CONF_NAME_TABLES             "tables"

   #define RPL_CONF_NAME_SOURCE             "source"
   #define RPL_CONF_NAME_TARGET             "target"
   #define RPL_CONF_NAME_FIELDS             "fields"

   // field
   #define RPL_CONF_NAME_FIELD_TYPE         "fieldType"
   #define RPL_CONF_NAME_FIELD_DEFAULTVALUE "defaultValue"
   #define RPL_CONF_NAME_FIELD_CONSTVALUE   "constValue"
   #define RPL_CONF_NMAE_FIELD_DOUBLEQUOTE  "doubleQuote"
   // omit floatFmt for default Ryu shortest (same as "full");
   // "full" explicit; "db2" for %+.14E; otherwise printf float format
   #define RPL_CONF_NAME_FIELD_FLOATFMT       "floatFmt"
   #define RPL_FLOATFMT_FULL                  "full"
   #define RPL_FLOATFMT_DB2                   "db2"
   #define RPL_FLOATFMT_DEFAULT               "%.16g"
   #define RPL_FLOATFMT_DB2_VALUE             "%+.14E"

   // RPL_CONF_NAME_DELIMITER defalut value
   #define RPL_CONF_DEFAULT_DELIMITER       ","
   // RPL_CONF_OUTPUT_TYPE value
   #define RPL_OUTPUT_DB2LOAD               "DB2LOAD"

   // field type values
   #define RPL_FIELD_TYPE_MAPPING_INT       "MAPPING_INT"
   #define RPL_FIELD_TYPE_MAPPING_DECIMAL   "MAPPING_DECIMAL"
   #define RPL_FIELD_TYPE_MAPPING_TIMESTAMP "MAPPING_TIMESTAMP"
   #define RPL_FIELD_TYPE_MAPPING_LONG      "MAPPING_LONG"
   #define RPL_FIELD_TYPE_MAPPING_STRING    "MAPPING_STRING"
   #define RPL_FIELD_TYPE_MAPPING_DOUBLE    "MAPPING_DOUBLE"
   #define RPL_FIELD_TYPE_MAPPING_DATE      "MAPPING_DATE"
   #define RPL_FIELD_TYPE_MAPPING_BINARY    "MAPPING_BINARY"
   #define RPL_FIELD_TYPE_CONST_STRING      "CONST_STRING"
   #define RPL_FIELD_TYPE_OUTPUT_TIME       "OUTPUT_TIME"
   #define RPL_FIELD_TYPE_AUTO_OP           "AUTO_OP"
   #define RPL_FIELD_TYPE_ORIGINAL_TIME     "ORIGINAL_TIME"
}

#endif //REPLAY_CONF_DEF_HPP_

