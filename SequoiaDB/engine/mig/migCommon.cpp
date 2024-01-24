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

   Source File Name = migImport.cpp

   Descriptive Name = Migration Import Implementation

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains implementation for import
   operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/30/2014   JW Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCommon.hpp"
#include "migCommon.hpp"

namespace engine
{
   UINT32 migRC2ShellRC( INT32 rc )
   {
      return utilRC2ShellRC( rc ) ;
   }
}