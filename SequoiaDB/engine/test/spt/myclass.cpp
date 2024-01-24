/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = myclass.cpp

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

#include "myclass.hpp"
#include <iostream>

using namespace std ;

JS_MEMBER_FUNC_DEFINE( myclass, func)
JS_CONSTRUCT_FUNC_DEFINE( myclass, construct)
JS_DESTRUCT_FUNC_DEFINE(myclass, destruct)

JS_BEGIN_MAPPING(myclass, "myjsclass")
  JS_ADD_MEMBER_FUNC("func", func)
  JS_ADD_CONSTRUCT_FUNC(construct)
  JS_ADD_DESTRUCT_FUNC(destruct)
JS_MAPPING_END()

myclass::myclass()
{
   cout << "myclass::myclass" << endl ;
}

myclass::~myclass()
{
   cout << "myclass""~myclass" << endl ;
}

INT32 myclass::func( const _sptParamContainer &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail )
{
   cout << "myclass::fun" << endl ;
   return SDB_OK ;
}

INT32 myclass::construct( const _sptParamContainer &arg,
                          _sptReturnVal &rval,
                           bson::BSONObj &detail)
{
   cout << "myclass::construct" << endl;
   return SDB_OK ;
}

INT32 myclass::destruct()
{
   cout << "myclass::destruct" << endl ;
   return SDB_OK ;
}
