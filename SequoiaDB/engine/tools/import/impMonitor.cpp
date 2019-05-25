/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = impMonitor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          4/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impMonitor.hpp"

namespace import
{
   Monitor::Monitor()
   : _recordsMem(0),
     _recordsNum(0)
   {
   }

   Monitor::~Monitor()
   {
   }

   Monitor* impGetMonitor()
   {
      static Monitor monitor;

      return &monitor;
   }
}
