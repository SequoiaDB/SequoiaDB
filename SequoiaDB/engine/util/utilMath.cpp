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

   Source File Name = utilMath.cpp

   Descriptive Name = math utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  HAS Initial Draft
          05/04/2018  HGM Add license

   Last Changed =

*******************************************************************************/

#include "utilMath.hpp"

namespace engine
{

   BOOLEAN utilAddIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if( l > 0 && r > 0 && result < 0 )
      {
         ret = TRUE ;
      }
      else if( l < 0 && r < 0 && result >= 0 )
      {
         ret = TRUE ;
      }

      return ret ;
   }

   BOOLEAN utilSubIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if( l >= 0 && r < 0 && result < 0 )
      {
         ret = TRUE ;
      }
      else if( l < 0 && r > 0 && result > 0 )
      {
         ret = TRUE ;
      }

      return ret ;
   }

   BOOLEAN utilMulIsOverflow( INT64 l, INT64 r, INT64 result )
   {
      BOOLEAN ret = FALSE ;
      if ( l != (INT64) ((INT32) l) || r != (INT64) ((INT32) r) )
      {
         if ( r != 0 &&
              ( ( r == -1 && l < 0 && result < 0 ) ||
                result / r != l ) )
         {
            ret = TRUE ;
         }
      }
      return ret ;
   }

   BOOLEAN utilDivIsOverflow( INT64 l, INT64 r )
   {
      BOOLEAN ret = FALSE ;
      if ( OSS_SINT64_MIN == l && -1 == r )
      {
         ret = TRUE ;
      }
      return ret ;
   }

}
