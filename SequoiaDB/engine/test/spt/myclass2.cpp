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

   Source File Name = myclass2.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "myclass2.hpp"
#include <iostream>

using namespace std ;

JS_MEMBER_FUNC_DEFINE( myclass2, func)
JS_CONSTRUCT_FUNC_DEFINE( myclass2, construct)
JS_DESTRUCT_FUNC_DEFINE(myclass2, destruct)

JS_BEGIN_MAPPING(myclass2, "myjsclass2")
  JS_ADD_MEMBER_FUNC("func", func)
  JS_ADD_CONSTRUCT_FUNC(construct)
  JS_ADD_DESTRUCT_FUNC(destruct)
JS_MAPPING_END()

myclass2::myclass2()
{
   cout << "myclass2::myclass2" << endl ;
}

myclass2::~myclass2()
{
   cout << "myclass2""~myclass2" << endl ;
}

INT32 myclass2::func( const _sptParamContainer &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail )
{
   cout << "myclass2::fun" << endl ;
   return SDB_OK ;
}

INT32 myclass2::construct( const _sptParamContainer &arg,
                          _sptReturnVal &rval,
                           bson::BSONObj &detail)
{
   cout << "myclass2::construct" << endl;
   return SDB_OK ;
}

INT32 myclass2::destruct()
{
   cout << "myclass2::destruct" << endl ;
   return SDB_OK ;
}
