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

   Source File Name = sptApi.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_API_HPP_
#define SPT_API_HPP_

#include "sptInjection.hpp"
#include "sptObjDesc.hpp"
#include "sptInvoker.hpp"
#include "sptContainer.hpp"
#include "sptScope.hpp"
#include "sptArguments.hpp"
#include "sptReturnVal.hpp"

#define SPT_CLASS_DEF( c )\
        ( (c)->__desc.getClassDef() )

#endif

