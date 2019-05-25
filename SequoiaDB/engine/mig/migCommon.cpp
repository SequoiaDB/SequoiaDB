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