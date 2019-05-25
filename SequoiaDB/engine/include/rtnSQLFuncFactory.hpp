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

   Source File Name = rtnSQLFuncFactory.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSQLFUNCFACTORY_HPP_
#define RTNSQLFUNCFACTORY_HPP_

#include "rtnSQLFunc.hpp"

namespace engine
{
   class _rtnSQLFunc ;

   #define RTN_SQL_FUNC_COUNT                "count"
   #define RTN_SQL_FUNC_SUM                  "sum"
   #define RTN_SQL_FUNC_MIN                  "min"
   #define RTN_SQL_FUNC_MAX                  "max"
   #define RTN_SQL_FUNC_AVG                  "avg"
   #define RTN_SQL_FUNC_FIRST                "first"
   #define RTN_SQL_FUNC_LAST                 "last"
   #define RTN_SQL_FUNC_PUSH                 "push"
   #define RTN_SQL_FUNC_ADDTOSET             "addtoset"
   #define RTN_SQL_FUNC_BUILDOBJ             "buildobj"
   #define RTN_SQL_FUNC_MERGEARRAYSET        "mergearrayset"

   class _rtnSQLFuncFactory : public SDBObject
   {
   public:
      INT32 create( const CHAR *name,
                    UINT32 paramNum,
                    _rtnSQLFunc *&func ) ;
   } ;

   typedef class _rtnSQLFuncFactory rtnSQLFuncFactory ;
}

#endif

