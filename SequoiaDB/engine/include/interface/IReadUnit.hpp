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

   Source File Name = IReadUnit.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_I_READ_UNIT_HPP_
#define SDB_I_READ_UNIT_HPP_

#include "sdbInterface.hpp"
#include "interface/IStorageSession.hpp"
#include "utilPooledAutoPtr.hpp"
#include "utilPooledObject.hpp"

namespace engine
{

   /*
      IReadUnit define
    */
   class IReadUnit : public _utilPooledObject
   {
   public:
      IReadUnit() = default ;
      virtual ~IReadUnit() = default ;
      IReadUnit( const IReadUnit &o ) = delete ;
      IReadUnit &operator =( const IReadUnit& ) = delete ;
   } ;

}

#endif // SDB_I_READ_UNIT_HPP_