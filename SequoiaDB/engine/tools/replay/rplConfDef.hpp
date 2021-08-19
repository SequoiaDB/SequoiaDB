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

   // table
   #define RPL_CONF_NAME_SOURCE             "source"
   #define RPL_CONF_NAME_TARGET             "target"
   #define RPL_CONF_NAME_FIELDS             "fields"

   // field
   #define RPL_CONF_NAME_FIELD_TYPE         "fieldType"
   #define RPL_CONF_NAME_FIELD_DEFAULTVALUE "defaultValue"
   #define RPL_CONF_NAME_FIELD_CONSTVALUE   "constValue"
   #define RPL_CONF_NMAE_FIELD_DOUBLEQUOTE  "doubleQuote"

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
   #define RPL_FIELD_TYPE_CONST_STRING      "CONST_STRING"
   #define RPL_FIELD_TYPE_OUTPUT_TIME       "OUTPUT_TIME"
   #define RPL_FIELD_TYPE_AUTO_OP           "AUTO_OP"
   #define RPL_FIELD_TYPE_ORIGINAL_TIME     "ORIGINAL_TIME"
}

#endif //REPLAY_CONF_DEF_HPP_

