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

   Source File Name = schedDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/04/2016  Li Jianhua  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_DEF_HPP__
#define SCHED_DEF_HPP__

#include <string>
#include "oss.hpp"
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      COMMON DEFINE
   */
   #define SCHED_SYS_CONTAINER_NAME                   "SYSTEM"
   #define SCHED_TASK_NAME_DFT                        "Default"

   #define SCHED_NICE_DFT                             ( 0 )
   #define SCHED_NICE_MAX                             ( 19 )
   #define SCHED_NICE_MIN                             ( -20 )

   #define SCHED_TASK_ID_DFT                          ( 0 )

   /*
      _schedInfo define
   */
   class _schedInfo : public SDBObject
   {
      public:
         _schedInfo() ;
         ~_schedInfo() ;

         BSONObj  toBSON() const ;
         INT32    fromBSON( const BSONObj &obj ) ;

         void     reset() ;

      public:
         INT32                _nice ;
         INT64                _taskID ;

         string               _taskName ;
         string               _containerName ;
         string               _userName ;
         string               _ip ;
   } ;
   typedef _schedInfo schedInfo ;

}

#endif // SCHED_DEF_HPP__
