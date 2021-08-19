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

   Source File Name = omToolCmdHost.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/09/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMTOOL_CMD_HOST_HPP_
#define OMTOOL_CMD_HOST_HPP_

#include "omToolCmdBase.hpp"

#define OM_TOOL_MODE_ADD_HOST  "addhost"
#define OM_TOOL_MODE_DEL_HOST  "delhost"

namespace omTool
{
   class omToolAddHost : public omToolCmdBase
   {
   DECLARE_OMTOOL_CMD_AUTO_REGISTER() ;

   public:
      omToolAddHost() ;

      ~omToolAddHost() ;

      INT32 doCommand() ;

      const CHAR *name(){ return OM_TOOL_MODE_ADD_HOST ; }
   } ;

   class omToolDelHost : public omToolCmdBase
   {
   DECLARE_OMTOOL_CMD_AUTO_REGISTER();

   public:
      omToolDelHost() ;
      ~omToolDelHost() ;

      INT32 doCommand() ;

      const CHAR *name(){ return OM_TOOL_MODE_DEL_HOST ; }
   } ;
}


#endif /* OMT_CMD_HOST_HPP_ */