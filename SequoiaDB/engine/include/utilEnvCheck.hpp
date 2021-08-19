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

   Source File Name = utilEnvCheck.hpp

   Descriptive Name =

   When/how to use: linux environment check util

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== =========== ==============================================
          20/04/2016  Chen Chucai Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTILENVCHECK_HPP__
#define UTILENVCHECK_HPP__

#include "ossTypes.h"


namespace engine
{

   BOOLEAN utilCheckIs64BitSys() ;
   BOOLEAN utilCheckIsOpenVZ() ;
   BOOLEAN utilCheckNumaStatus() ;
   BOOLEAN utilCheckVmStatus() ;
   BOOLEAN utilCheckThpStatus() ; //thp : transparent_hugepage
  
   BOOLEAN utilCheckEnv() ;
}

#endif //UTILENVCHECK_HPP_
