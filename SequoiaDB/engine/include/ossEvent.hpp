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

   Source File Name = ossEvent.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OSS_EVENT_HPP_
#define OSS_EVENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <boost/thread.hpp>
#include <boost/thread/cv_status.hpp>

namespace engine
{

   class _ossEvent : public SDBObject
   {
      public:
         _ossEvent () ;
         virtual ~_ossEvent () ;

      public:
         INT32 wait ( INT64 millisec = -1, INT32 *pData = NULL ) ;
         INT32 signal ( INT32 data = 0 ) ;
         INT32 signalAll ( INT32 data = 0 ) ;
         INT32 reset () ;
         UINT32 waitNum () ;

      protected:
         virtual void _onWait () ;

      protected:
         mutable boost::mutex       _mutex ;
         boost::condition_variable  _cond ;
         UINT32                     _signal ;
         UINT32                     _waitNum ;
         INT32                      _useData ;

   };

   typedef _ossEvent ossEvent ;

   class _ossAutoEvent : public _ossEvent
   {
      public:
         _ossAutoEvent () ;
         virtual ~_ossAutoEvent () ;

      protected:
         virtual void _onWait () ;

   };

   typedef _ossAutoEvent ossAutoEvent ;

}

#endif //OSS_EVENT_HPP_
