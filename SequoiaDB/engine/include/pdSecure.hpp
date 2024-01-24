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

   Source File Name = pdSecure.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/10/2022  Ting YU  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef PDSECURE_HPP_
#define PDSECURE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "utilSecure.hpp"

namespace engine
{
   // input: BSONObj
   #define PD_SECURE_OBJ( obj ) PD_SECURE_STR( (obj).toPoolString() )

   // input: ossPoolString or string
   #define PD_SECURE_STR( str ) \
      ( ( pdIsDiaglogSecureEnabled() && pdLocalIsDiaglogSecureEnabled() ) ? \
        utilSecureStr( str ).c_str() : str.c_str() )

   // input: const CHAR*
   #define PD_SECURE_STR1( data ) \
      ( ( pdIsDiaglogSecureEnabled() && pdLocalIsDiaglogSecureEnabled() ) ? \
        utilSecureStr( data, ossStrlen( data ) ).c_str() : data )
}

#endif
