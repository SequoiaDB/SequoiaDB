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

   Source File Name = monInterface.hpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   application and snapshot.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/10/2022  TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MON_INTERFACE_HPP__
#define MON_INTERFACE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "monCB.hpp"

namespace engine
{

   /*
      _IMonSubmitEvent define
   */
   class _IMonSubmitEvent
   {
      public:
         _IMonSubmitEvent() {}
         virtual ~_IMonSubmitEvent() {}

      public:
         virtual void onSubmit( const monAppCB &delta ) = 0 ;
   } ;
   typedef _IMonSubmitEvent IMonSubmitEvent ;


}

#endif // MON_INTERFACE_HPP__

