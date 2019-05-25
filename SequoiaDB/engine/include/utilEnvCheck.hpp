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
