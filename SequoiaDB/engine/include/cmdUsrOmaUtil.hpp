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

   Source File Name = cmdUsrOmaUtil.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/12/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

namespace engine
{
   #define SPT_OMA_REL_PATH            SDBCM_CONF_DIR_NAME OSS_FILE_SEP
   #define SPT_OMA_REL_PATH_FILE       SPT_OMA_REL_PATH SDBCM_CFG_FILE_NAME

   /*
      define config
   */
   #define MAP_CONFIG_DESC( desc ) \
      desc.add_options() \
      ( SDBCM_RESTART_COUNT, po::value<INT32>(), "" ) \
      ( SDBCM_RESTART_INTERVAL, po::value<INT32>(), "" ) \
      ( SDBCM_AUTO_START, po::value<string>(), "" ) \
      ( SDBCM_DIALOG_LEVEL, po::value<INT32>(), "" ) \
      ( SDBCM_ENABLE_WATCH, po::value<string>(), "" ) \
      ( "*", po::value<string>(), "" )

   /*
      define normal config
   */
   #define MAP_NORMAL_CONFIG_DESC( desc ) \
      desc.add_options() \
      ( "*", po::value<string>(), "" )

}