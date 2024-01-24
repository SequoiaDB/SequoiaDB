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

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

static void _testGetKeys( const BSONObj &obj,
                          const BSONObj &keyDef,
                          const BSONObjSet &keySetExp,
                          const BSONElement &arrExp,
                          BOOLEAN keepKeyName,
                          BOOLEAN ignoreUndefined = FALSE )
{
   // generator
   ixmIndexKeyGen gen( keyDef ) ;
   // key set
   BSONObjSet keySet( keyDef ) ;
   // array element
   BSONElement arr ;

   // get key set
   INT32 rc = SDB_OK ;
   rc = gen.getKeys( obj, keySet, &arr, keepKeyName, ignoreUndefined ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;

   // check key set
   for ( BSONObjSet::const_iterator itr = keySet.begin() ;
         itr != keySet.end() ;
         itr++ )
   {
      cout << itr->toString() << endl ;
   }
   ASSERT_TRUE( keySetExp == keySet ) ;

   // check array element
   cout << "arr:" << arr.toString( true, true )  << endl ;
   ASSERT_TRUE( arr == arrExp ) ;

   // get one keys
   BSONObj keys ;
   rc = gen.getKeys( obj, keys, &arr, keepKeyName, ignoreUndefined ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;

   // check keys
   cout << keys.toString() << endl ;
   ASSERT_TRUE( ( keys.isEmpty() && keySet.empty() ) ||
                ( *( keySet.begin() ) == keys ) ) ;

   // check array element
   cout << "arr:" << arr.toString( true, true )  << endl ;
   ASSERT_TRUE( arr == arrExp ) ;
}

static void _testGetKeysErr( const BSONObj &obj,
                             const BSONObj &keyDef,
                             INT32 rcExp )
{
   // generator
   ixmIndexKeyGen gen( keyDef ) ;
   // key set
   BSONObjSet keySet( keyDef ) ;

   INT32 rc = SDB_OK ;
   rc = gen.getKeys( obj, keySet ) ;
   cout << "rc: " << rc << endl ;
   ASSERT_TRUE( rcExp == rc ) ;

   // one keys
   BSONObj keys ;

   rc = gen.getKeys( obj, keys ) ;
   cout << "rc: " << rc << endl ;
   ASSERT_TRUE( rcExp == rc ) ;
}

TEST( generator, test_embeded )
{
   // { no : 1, name : "A", age : 2,
   //   array1 : [
   //      { array2 : [
   //         { array3 : [
   //            { array4 : [ "array5", "temp4" ] },
   //            "temp3" ] },
   //         "temp2" ] },
   //     "temp1" ] }
   BSONObj obj( BSON( "no" << 1 << "name" << "A" << "age" << 2 <<
                      "array1" << BSON_ARRAY(
                            BSON( "array2" << BSON_ARRAY(
                                  BSON( "array3" << BSON_ARRAY(
                                        BSON( "array4" << BSON_ARRAY(
                                              "array5" << "temp4" ) ) <<
                                        "temp3" ) ) <<
                                  "temp2" ) ) <<
                            "temp1" ) ) ) ;
   {
      BSONObj keyDef( BSON( "array1.array2.array3.array4.1" << 1 ) ) ;
      BSONObj keyExp( ixmGetUndefineKeyObj( 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "array1" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      BSONObj keyDef( BSON( "array1.array2.array3.array4.1" << 1 ) ) ;
      BSONObj keyExp( BSON( "array1.array2.array3.array4.1" <<
                            ixmGetUndefineKeyObj( 1 ).firstElement() ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "array1" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }

   {
      BSONObj keyDef( BSON( "array1.array2.array3.array4" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = ixmGetUndefineKeyObj( 1 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << "array5" ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << "temp4" ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "array1" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      BSONObj keyDef( BSON( "array1.array2.array3.array4" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "array1.array2.array3.array4" <<
                     ixmGetUndefineKeyObj( 1 ).firstElement() ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "array1.array2.array3.array4" << "array5" ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "array1.array2.array3.array4" << "temp4" ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "array1" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }

   {
      BSONObj keyDef( BSON( "array1.array2.array3.array4" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = ixmGetUndefineKeyObj( 1 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << "array5" ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << "temp4" ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "array1" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      BSONObj keyDef( BSON( "array1.array2.array3.array4" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "array1.array2.array3.array4" <<
                     ixmGetUndefineKeyObj( 1 ).firstElement() ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "array1.array2.array3.array4" << "array5" ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "array1.array2.array3.array4" << "temp4" ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "array1" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }
}

TEST( generator, test_array )
{
   // { a : [ 1, 2, 3, [ 4, 5 ] ] }
   BSONObj obj( BSON( "a" <<
                      BSON_ARRAY( 1 <<
                                  2 <<
                                  3 <<
                                  BSON_ARRAY( 4 << 5 ) ) ) ) ;

   {
      BSONObj keyDef( BSON( "a" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "" << 1 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << 2 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << 3 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << BSON_ARRAY( 4 << 5 ) ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      BSONObj keyDef( BSON( "a" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "a" << 1 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "a" << 2 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "a" << 3 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "a" << BSON_ARRAY( 4 << 5 ) ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }

   {
      BSONObj keyDef( BSON( "a" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "" << 1 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << 2 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << 3 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "" << BSON_ARRAY( 4 << 5 ) ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      BSONObj keyDef( BSON( "a" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "a" << 1 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "a" << 2 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "a" << 3 ) ;
      keySetExp.insert( keyExp ) ;
      keyExp = BSON( "a" << BSON_ARRAY( 4 << 5 ) ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }
}

TEST( generator, test_udf )
{
   // { a : [ 1, 2, 3, [ 4, 5 ] ] }
   BSONObj obj( BSON( "a" <<
                      BSON_ARRAY( 1 <<
                                  2 <<
                                  3 <<
                                  BSON_ARRAY( 4 << 5 ) ) ) ) ;
   {
      BSONObj keyDef( BSON( "b" << 1 << "c" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = ixmGetUndefineKeyObj( 2 ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      // get undefined with key name
      // NOTE: this should return with key names, but for
      // forward compatibility, we keep without key names
      BSONObj keyDef( BSON( "b" << 1 << "c" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = ixmGetUndefineKeyObj( 2 ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }
}

TEST( generator, test_empty_arr )
{
   // { a : { b : [] }, c : [], d : [ e : [] ] }
   BSONObj obj( BSON( "a" << BSON( "b" << BSONArray() ) <<
                      "c" << BSONArray() <<
                      "d" << BSON_ARRAY( BSON( "e" << BSONArray() ) ) ) ) ;

   {
      // { a : { b : [] } }
      BSONObj keyDef( BSON( "a.b" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp =
            obj.getField( "a" ).embeddedObject().getField( "b" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      // { a : { b : [] }
      BSONObj keyDef( BSON( "a.b" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "a.b" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp =
            obj.getField( "a" ).embeddedObject().getField( "b" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }

   {
      // { c : [] }
      BSONObj keyDef( BSON( "c.c1" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "c" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      // { c : [] }
      BSONObj keyDef( BSON( "c.c1" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "c.c1" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "c" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }

   {
      // { d : [ e : [] ] }
      BSONObj keyDef( BSON( "d.e" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "d" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, FALSE ) ;
   }

   {
      // { d : [ e : [] ] }
      BSONObj keyDef( BSON( "d.e" << -1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "d.e" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "d" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE ) ;
   }
}

TEST( generator, test_ignore_undefined )
{
   {
      BSONObj obj( BSON( "a" << 1 << "c" << 1 << "d" << BSONArray() ) ) ;
      BSONObj keyDef( BSON( "a" << 1 << "b" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "a" << 1 ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }

   {
      BSONObj obj( BSON( "a" << 1 << "c" << 1 << "d" << BSONArray() ) ) ;
      BSONObj keyDef( BSON( "a" << 1 << "d.e" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "a" << 1 << "d.e" << BSONArray() ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp = obj.getField( "d" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }

   {
      BSONObj obj( BSON( "a" << 1 ) ) ;
      BSONObj keyDef( BSON( "a.1" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      BSONElement arrExp ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }

   {
      BSONObj obj( BSON( "a" <<
                         BSON_ARRAY( "update" <<
                                     "update" <<
                                     "update" ) ) ) ;
      BSONObj keyDef( BSON( "a.1" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }

   {
      BSONObj obj( BSON( "a" <<
                         BSON_ARRAY( BSON( "0" << "update" ) <<
                                     BSON( "1" << "update" ) <<
                                     BSON( "2" << "update" ) ) ) ) ;
      BSONObj keyDef( BSON( "a.1" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp( BSON( "a.1" << "update" ) ) ;
      keySetExp.insert( keyExp ) ;
      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }

   {
      BSONObj obj( BSON( "a" <<
                         BSON_ARRAY( BSON( "1" << "update" ) ) ) ) ;
      BSONObj keyDef( BSON( "a.1" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp( BSON( "a.1" << "update" ) ) ;
      keySetExp.insert( keyExp ) ;
      BSONElement arrExp = obj.getField( "a" ) ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }
}

TEST( generator, test_multikeys )
{
   BSONObj obj( BSON( "a" << 1 << "b" << 1 << "c" << 1 << "d" << 1 <<
                      "e" << 1 << "f" << 1 << "g" << 1 << "h" << 1 ) ) ;

   {
      BSONObj keyDef( BSON( "a" << 1 << "b" << 1 << "c" << 1 << "d" << 1 <<
                            "e" << 1 << "f" << 1 << "g" << 1 << "h" << 1 ) ) ;
      BSONObjSet keySetExp( keyDef ) ;
      BSONObj keyExp ;
      keyExp = BSON( "a" << 1 << "b" << 1 << "c" << 1 << "d" << 1 <<
                     "e" << 1 << "f" << 1 << "g" << 1 << "h" << 1 ) ;
      keySetExp.insert( keyExp ) ;

      BSONElement arrExp ;

      _testGetKeys( obj, keyDef, keySetExp, arrExp, TRUE, TRUE ) ;
   }
}

TEST( generator, test_error )
{
   {
      // empty key
      BSONObj keyDef ;
      BSONObj obj ;

      _testGetKeysErr( obj, keyDef, SDB_SYS ) ;
   }

   {
      // multiple array
      BSONObj keyDef( BSON( "a" << 1 << "b" << 1 ) ) ;
      BSONObj obj( BSON( "a" << BSON_ARRAY( 1 << 2 ) <<
                         "b" << BSON_ARRAY( 3 << 4 ) ) ) ;
      _testGetKeysErr( obj, keyDef, SDB_IXM_MULTIPLE_ARRAY ) ;
   }
}
