/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = utilDataSource.cpp

   Descriptive Name = Data source utilities

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/13/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilDataSource.hpp"

namespace engine
{
   const CHAR *sdbDSTransModeDesc( SDB_DS_TRANS_PROPAGATE_MODE mode )
   {
      const CHAR *desc = NULL ;
      switch ( mode )
      {
         case DS_TRANS_PROPAGATE_NEVER:
            desc = VALUE_NAME_NEVER ;
            break ;
         case DS_TRANS_PROPAGATE_NOT_SUPPORT:
            desc = VALUE_NAME_NOT_SUPPORT ;
            break ;
         default:
            desc = "" ;
            break ;
      }
      return desc ;
   }

   SDB_DS_TRANS_PROPAGATE_MODE sdbDSTransModeFromDesc( const CHAR *descStr )
   {
      SDB_ASSERT( descStr, "Propagate mode string is NULL" ) ;
      if ( 0 == ossStrcasecmp( descStr, VALUE_NAME_NEVER ) )
      {
         return DS_TRANS_PROPAGATE_NEVER ;
      }
      else if ( 0 == ossStrcasecmp( descStr, VALUE_NAME_NOT_SUPPORT ) )
      {
         return DS_TRANS_PROPAGATE_NOT_SUPPORT ;
      }
      else
      {
         return DS_TRANS_PROPAGATE_INVALID ;
      }
   }
}
