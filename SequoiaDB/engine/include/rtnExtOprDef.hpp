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

   Source File Name = rtnExtOprDef.hpp

   Descriptive Name = External Operation Definitions.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/11/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_EXTOPR_DEF_HPP__
#define RTN_EXTOPR_DEF_HPP__

namespace engine
{
   enum _rtnExtOprType
   {
      RTN_EXT_INVALID = 0,
      RTN_EXT_INSERT = 1,
      RTN_EXT_DELETE,
      RTN_EXT_UPDATE,
      RTN_EXT_UPDATE_WITH_ID,
      RTN_EXT_DUMMY = 10000
   } ;
}

#endif /* RTN_EXTOPR_DEF_HPP__ */

