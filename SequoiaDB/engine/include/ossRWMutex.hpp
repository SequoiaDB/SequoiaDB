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

   Source File Name = ossRWMutex.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSS_RWMUTEX_HPP_
#define OSS_RWMUTEX_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"

namespace engine
{

#define RW_EXCLUSIVEWRITE        0X0000
#define RW_SHARDWRITE            0x0001

   class _ossRWMutex : public SDBObject
   {
      public:
         _ossRWMutex ( UINT32 type = RW_EXCLUSIVEWRITE ) ;
         ~_ossRWMutex () ;

      public:
         INT32 lock_r ( INT32 millisec = -1 ) ;
         INT32 lock_w ( INT32 millisec = -1 ) ;
         INT32 release_r () ;
         INT32 release_w () ;

      protected:
         UINT32   _makeTimeout( INT32 &millisec, UINT32 timeout ) ;

      private:
         ossAtomic32        _r ;
         ossAtomic32        _w ;
         ossAutoEvent       _event ;
         UINT32             _type ;

   };

   typedef _ossRWMutex ossRWMutex ;

   class _ossScopedRWLock
   {
      public:
         _ossScopedRWLock ( ossRWMutex *pMutex, OSS_LATCH_MODE mode ) ;
         ~_ossScopedRWLock () ;

      private:
         ossRWMutex *_pMutex ;
         OSS_LATCH_MODE _mode ;
   };

   typedef _ossScopedRWLock ossScopedRWLock ;
}

#endif //OSS_RWMUTEX_HPP_

