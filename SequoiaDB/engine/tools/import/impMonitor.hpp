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

   Source File Name = impMonitor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          4/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_MONITOR_HPP_
#define IMP_MONITOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"

namespace import
{
   class Monitor: public SDBObject
   {
   public:
      Monitor();
      ~Monitor();

      inline INT64 recordsMem() { return _recordsMem.fetch(); }
      inline void recordsMemInc(INT64 size) { _recordsMem.add(size); }
      inline void recordsMemDec(INT64 size) { _recordsMem.sub(size); }

      inline INT64 recordsNum() { return _recordsNum.fetch(); }
      inline void recordsNumInc(INT64 size) { _recordsNum.add(size); }
      inline void recordsNumDec(INT64 size) { _recordsNum.sub(size); }

   private:
      ossAtomicSigned64  _recordsMem;
      ossAtomicSigned64  _recordsNum;
   };

   Monitor* impGetMonitor();
}

#endif /* IMP_MONITOR_HPP_ */
