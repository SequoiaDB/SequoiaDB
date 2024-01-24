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

   Source File Name = mthUpdateTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "mthModifier.hpp"
#include "../bson/bson.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

TEST( generator, test_inc_long )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;
   BSONObjBuilder updateBuilder ;

   BSONObjBuilder incBuilder( updateBuilder.subobjStart( "$inc" ) ) ;
   incBuilder.append( "a", 1LL ) ;
   incBuilder.doneFast() ;

   BSONObj updator = updateBuilder.obj() ;
   rc = modifier.loadPattern( updator ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;

   {
      // result is number int
      BSONObj source = BSON( "a" << 1 ) ;
      BSONObj target ;
      rc = modifier.modify( source, target ) ;
      ASSERT_TRUE( SDB_OK == rc ) ;
      ASSERT_TRUE( NumberInt == target.firstElement().type() ) ;
   }

   {
      // result is overflow to number long
      BSONObj source = BSON( "a" << OSS_SINT32_MAX ) ;
      BSONObj target ;
      rc = modifier.modify( source, target ) ;
      ASSERT_TRUE( SDB_OK == rc ) ;
      ASSERT_TRUE( NumberLong == target.firstElement().type() ) ;
   }

   {
      // result is double
      BSONObj source = BSON( "a" << 1.0 ) ;
      BSONObj target ;
      rc = modifier.modify( source, target ) ;
      ASSERT_TRUE( SDB_OK == rc ) ;
      ASSERT_TRUE( NumberDouble == target.firstElement().type() ) ;
   }
}
