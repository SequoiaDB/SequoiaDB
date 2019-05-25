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

   Source File Name = migExport.hpp

   Descriptive Name = Migration Export Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for export operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/30/2014   JW Initial Draft
   Last Changed =

*******************************************************************************/
#ifndef MIGCOMMON_HPP__
#define MIGCOMMON_HPP__

#include "core.hpp"
#include "msgDef.h"
#include "pmdDef.hpp"
#include "utilStr.hpp"
#include "msg.h"

namespace engine
{
   UINT32 migRC2ShellRC( INT32 rc ) ;
}

#endif