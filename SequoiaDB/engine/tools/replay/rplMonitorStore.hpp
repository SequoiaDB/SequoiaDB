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

   Source File Name = rplMonitorStore.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef REPLAY_MONITOR_STORE_HPP_
#define REPLAY_MONITOR_STORE_HPP_

#include "oss.hpp"
#include "ossFile.hpp"
#include "rplMonitor.hpp"
#include "rplOutputter.hpp"
#include <string>

using namespace std ;
using namespace engine ;

namespace replay
{
   class rplMonitorStore : public SDBObject {
   public:
      rplMonitorStore( Monitor *monitor ) ;
      ~rplMonitorStore() ;

   public:
      INT32 init( const CHAR *storeFile ) ;
      INT32 init() ;
      INT32 flushAndSave() ;
      INT32 submitAndSave() ;
      INT32 save() ;
      void captureOutputter( rplOutputter *outputter ) ;

   private:
      INT32 _initMonitor( ossFile& statusFile ) ;
      INT32 _saveMonitor() ;

   private:
      rplOutputter *_outputter ;
      Monitor *_monitor ;

      BOOLEAN _isNeedWriteStatusFile ;
      // full file name
      string _tmpStatusFileName ;
      // full file name
      string _statusFileName ;
   } ;
}

#endif  /* REPLAY_MONITOR_STORE_HPP_ */

