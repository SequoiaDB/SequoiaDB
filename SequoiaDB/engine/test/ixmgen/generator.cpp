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

   Source File Name = generator.cpp

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

#include "ixmIndexKey.hpp"
#include "../bson/ordering.h"
#include "sptConvertor2.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

extern void getBSONRaw( const CHAR *js, CHAR **raw ) ;

TEST(generator, test1)
{
   {
   const CHAR *js = "{no:1,name:\"A\",age:2,array1:[{array2:[{array3:[{array4:[\"array5\",\"temp4\"]},\"temp3\"]},\"temp2\"]},\"temp1\"]}"; 
   CHAR *raw = NULL ;
   getBSONRaw( js, &raw ) ;
   ASSERT_TRUE( NULL != raw ) ;
   BSONObj obj( raw ) ;
   BSONObj keyDef = BSON("array1.array2.array3.array4.1" << 1 ) ;
   _ixmIndexKeyGen gen( keyDef ) ;
   Ordering order(Ordering::make(keyDef)) ;
   BSONObjSet keySet( keyDef ) ;
   BSONElement arr ;
   INT32 rc = SDB_OK ;
   rc = gen.getKeys( obj, keySet, &arr ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   for ( BSONObjSet::const_iterator itr = keySet.begin() ;
         itr != keySet.end() ;
         itr++ )
   {
      cout << itr->toString() << endl ;
   }

   cout << "arr:" << arr.toString( true, true )  << endl ;
   }
}
