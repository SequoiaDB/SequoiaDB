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

   Source File Name = test.cpp

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
#include "myclass2.hpp"
#include "ossUtil.hpp"


#include <gtest/gtest.h>
#include <iostream>

using namespace std ;

TEST(sptTest, test1)
{
  INT32 rc = SDB_OK ;
  _sptContainer container ;
  _sptScope *scope = container.newScope( SPT_SCOPE_TYPE_SP ) ; 
  ASSERT_TRUE( NULL != scope );

  rc = scope->loadUsrDefObj( &(myclass::__desc) ) ;
  ASSERT_TRUE( SDB_OK == rc ) ;

  rc = scope->loadUsrDefObj( &(myclass2::__desc) ) ;
  ASSERT_TRUE( SDB_OK == rc ) ;

  {
//  const CHAR *code = "var a = new myjsclass(); var b = new myjsclass2();" ;
  const CHAR *code = "function sum(x,y){return x+y;} sum(1,2);"
  bson::BSONObj detail ;
  bson::BSONObj rval ;
  rc = scope->eval( code, ossStrlen(code), rval, detail ) ;
  ASSERT_TRUE( SDB_OK == rc ) ;
  cout << rval.toString() << endl ;
  }
/*
  {
  const CHAR *code = "a.func(); b.func()" ;
  bson::BSONObj detail ;
  rc = scope->eval( code, ossStrlen(code), detail) ;
  ASSERT_TRUE( SDB_OK == rc ) ;
  }
*/
  scope->shutdown() ;
  delete scope ;
}
