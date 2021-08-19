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

   Source File Name = rtnRemoteExec.hpp

   Descriptive Name = Remote Excuting Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for process op.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2/27/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNREMOTEEXEC_HPP__
#define RTNREMOTEEXEC_HPP__

#include "core.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   INT32 rtnRemoteExec ( SINT32 remoCode,
                         const CHAR * hostname,
                         SINT32 *retCode,
                         const BSONObj *arg1 = NULL,
                         const BSONObj *arg2 = NULL,
                         const BSONObj *arg3 = NULL,
                         const BSONObj *arg4 = NULL,
                         std::vector<BSONObj> *retObjs = NULL ) ;

}

#endif /* RTNREMOTEEXEC_HPP__ */

