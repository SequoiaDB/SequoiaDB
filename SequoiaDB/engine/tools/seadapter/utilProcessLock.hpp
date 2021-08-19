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

   Source File Name = utilProcessLock.hpp

   Descriptive Name = Util Processor Lock.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/25/2018  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_PROCESS_LOCK__
#define UTIL_PROCESS_LOCK__

#include "oss.hpp"
#include "ossIO.hpp"

namespace seadapter
{
   class _IProcessLock
   {
   public:
      _IProcessLock() {}
      virtual ~_IProcessLock() {}

      virtual INT32 init( const CHAR *processName, void *arg ) = 0 ;
      virtual INT32 lock() = 0 ;
      virtual INT32 tryLock() = 0 ;
      virtual void unlock() = 0 ;
      virtual void destroy() = 0 ;
   } ;
   typedef _IProcessLock IProcessLock ;

   class _utilProcFlockMutex : public IProcessLock
   {
   public:
      _utilProcFlockMutex() ;
      ~_utilProcFlockMutex() ;
      INT32 init( const CHAR *processName, void *arg ) ;
      INT32 lock() ;
      INT32 tryLock() ;
      void unlock() ;
      void destroy() ;
   private:
      std::string _lockFileName ;
      OSSFILE     _file ;
      BOOLEAN     _fileOpened ;
      BOOLEAN     _fileLocked ;
   } ;
   typedef _utilProcFlockMutex utilProcFlockMutex ;
}

#endif /* UTIL_PROCESS_LOCK__ */

