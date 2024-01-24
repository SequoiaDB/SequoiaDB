#include "mthSelector.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace std ;
using namespace bson ;
using namespace engine ;

/// simple include
/// select b, a from {a:1,b:2,c:3} -> {a:1,b:2}
TEST( selector, simple_include_test_1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "b" << BSON( "$include" << 1 ) << "a" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 << "b" << 2 << "c" << 3 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 << "b" << 2 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select b, a from {c:1, b:2} -> {b:2}
TEST( selector, simple_include_test_2 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "b" << BSON( "$include" << 1 ) << "a" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "c" << 1 << "b" << 2 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "b" << 2 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select b, a from {c:1,d:1} -> {}
TEST( selector, simple_include_test_3 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "b" << BSON( "$include" << 1 ) << "a" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "c" << 1 << "d" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSONObj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select a.b from {a:{c:1, b:1}} -> {a:{b:1}} 
TEST( selector, simple_include_test_4 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON( "c" << 1 << "b" << 1 ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "b" << 1 ) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select a.b from {a:1} -> {}
TEST( selector, simple_include_test_5 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSONObj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select a.b from {a:[{c:1}, {d:1}, {b:1}, {b:2}]} -> {a:[{}, {}, {b:1},{b:2}]}
TEST( selector, simple_include_test_6 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON( "c" << 1) << BSON("d" << 1) << BSON("b"<< 1) << BSON("b" << 2)) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY(BSONObj() << BSONObj() <<BSON("b" << 1) << BSON("b" << 2))) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select a.b from {a:[{c:1}]} -> {a:[{}]}
TEST( selector, simple_include_test_7 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON( "c" << 1) ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( BSONObj() ) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// select a.b from {a:[1,2]} -> {a:[]}
TEST( selector, simple_include_test_8 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << 2 ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSONArrayBuilder().arr() ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// exclude b from {a:1,b:1} -> {a:1}
TEST( selector, simple_exclude_test_1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "b" << BSON( "$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// exclude a.b from { a:1, b:1} -> {a:1,b:1}
TEST( selector, simple_exclude_test_2 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 << "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// exclude a.b from { a:{b:1, c:1}, b:1} -> {a:{c:1},b:1}
TEST( selector, simple_exclude_test_3 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON( "b" << 1 << "c" << 1) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON("c" << 1) << "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// exclude a.b from {a:[1, {b:1, c:1}, {b:1}, {d:1}]} -> {a:[1, {c:1}, {}, {d:1}]}
TEST( selector, simple_exclude_test_4 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << BSON("b" << 1 << "c" <<1 ) << BSON("b" << 1 ) << BSON("d" << 1))) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( 1 << BSON("c" <<1 ) << BSONObj() << BSON("d" << 1)) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// exclude a from {b:1} -> {b:1}
TEST( selector, simple_exclude_test_5 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// exclude a.b from {b:1} -> {b:1}
TEST( selector, simple_exclude_test_6 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// include a, exclude b --> error
TEST( selector, simple_exclude_test_7 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$include" << 1 ) << "b" << BSON("$include" << 0 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_INVALIDARG , rc ) ;
}

/// default a:1 from {b:1 } -> {a:1}
TEST( selector, simple_default_test_1 )
{
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$default" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << 1 ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}


/// default a:1 from { a:2, b:1 } -> {a:2}
TEST( selector, simple_default_test_2 )
{
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$default" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 2 << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 2 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << 1 ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 2 << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 2 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// default a.b:1 from {a:{c:1, d:1}} -> {a:{b:1}}
TEST( selector, simple_default_test_3 )
{
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$default" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON( "c" << 1 << "d" << 1 ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "b" << 1) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << 1 ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON( "c" << 1 << "d" << 1 ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "b" << 1) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// default a.b:1  from {b:1} -> {a:{b:1}}
TEST( selector, simple_default_test_4 )
{
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$default" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 1) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "b" << 1 ) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << 1 ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 1) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "b" << 1 ) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// default a.b:1 from {a:[{c:1}, 1, {b:1}] } -> {a:[{b:1}, {b:1}]}
TEST( selector, simple_default_test_5 )
{
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$default" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON("c" << 1) << 1 << BSON("b" << 1) )) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( BSON("b" << 1) << BSON( "b" << 1 )) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << 1 ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON("c" << 1) << 1 << BSON("b" << 1 )) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( BSON("b" << 1) << BSON("b" << 1 )) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// default a.b:1 from {a:1, b:1 } -> {}
TEST( selector, simple_default_test_6 )
{
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$default" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 << "b" << 1) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSONObj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
   {
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << 1 ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSONObj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// slice a : 1 from {a:[1,2,3], b:1} ->{a:[1], b:1}
TEST( selector, simple_slice_test_1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$slice" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( 1 ) << "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a.b : 1 from {a:{a:1, b:[1,2,3]}, b:1} ->{a:{a:1, b:[1]}, b:1}
TEST( selector, simple_slice_test_2 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$slice" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON( "a" << 1 << "b" << BSON_ARRAY(1 << 2 << 3)) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "a" << 1 << "b" << BSON_ARRAY( 1 ))<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a.b : 1 from {a:{a:1, b:1}, b:1} ->{a:{a:1, b:[1]}, b:1}
TEST( selector, simple_slice_test_3 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$slice" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON( "a" << 1 << "b" << 1 ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "a" << 1 << "b" << 1 )<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a : -2 from {a:[1,2,3], b:1} ->{a:[2,3], b:1}
TEST( selector, simple_slice_test_4 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$slice" << -2 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( 2 << 3 )<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a : -2 from {a:[1], b:1} ->{a:[1], b:1}
TEST( selector, simple_slice_test_5 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$slice" << -2 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( 1 )<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a :[0,2] from {a:[1,2,3], b:1} ->{a:[1,2], b:1}
TEST( selector, simple_slice_test_6 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$slice" << BSON_ARRAY( 0 << 2 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( 1 << 2 )<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a :[2,2] from {a:[1,2,3], b:1} ->{a:[3], b:1}
TEST( selector, simple_slice_test_7 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$slice" << BSON_ARRAY( 2 << 2 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << 2 << 3 ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( 3 )<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// slice a.b:1 from {a:[{b:1}, {b:[1,2,3]}, {b:[4,5,6]}, {c:1}], b:1} ->{a:[{b:1}, {b:[1]}, {b:[4]}, {c:1}], b:1}
TEST( selector, simple_slice_test_8 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a.b" << BSON( "$slice" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON("b" << 1 ) << BSON( "b" << BSON_ARRAY( 1 << 2 << 3)) << BSON( "b" << BSON_ARRAY(4 << 5 << 6))<< BSON("b" << 1) ) << "b" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( BSON( "b" << 1) << BSON("b" << BSON_ARRAY( 1)) << BSON("b" << BSON_ARRAY(4)) << BSON("b" << 1) )<< "b" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// elemMatch a:{b:1} from {a:[{b:1}, {b:2}, {b:1}]} -> {a:[{b:1}, {b:1}]}
TEST( selector, simple_elemmatch_test_1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$elemMatch" << BSON( "b" << 1 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON( "b" << 1 ) << BSON( "b" << 2 ) << BSON( "b" << 1 ) ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( BSON("b" << 1 ) << BSON( "b" << 1 )) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// elemMatch a:{c:1} from {a:[{b:1}, {b:2}, {b:1}]} -> {a:[]}
TEST( selector, simple_elemmatch_test_2 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$elemMatch" << BSON( "c" << 1 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON( "b" << 1 ) << BSON( "b" << 2 ) << BSON( "b" << 1 ) ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSONArrayBuilder().arr() ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// elemMatch a:{b:1} from {a:1, b:2} -> {b:2}
TEST( selector, simple_elemmatch_test_3 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$elemMatch" << BSON( "b" << 1 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << 1 << "b" << 2 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "b" << 2 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// elemMatchOne a:{b:1} from {a:[{b:1}, {b:2}, {b:1}]} -> {a:[{b:1}]}
TEST( selector, simple_elemmatchone_test_1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$elemMatchOne" << BSON( "b" << 1 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "a" << BSON_ARRAY( BSON( "b" << 1 ) << BSON( "b" << 2 ) << BSON( "b" << 1 ) ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON_ARRAY( BSON("b" << 1 ) ) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

/// jira SEQUOIADBMAINSTREAM-915
TEST( selector, jira_915 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$default" << 1 << "$include" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   BSONObj record = BSON( "b" << 2 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST( selector, simple_abs_test1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$abs" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << 10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << 1.123 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1.123 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -1.123 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1.123 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "") ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// CEILING
TEST( selector, simple_celling_test1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$ceiling" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << 10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << 10.1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 11 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10.1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "") ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// floor
TEST( selector, simple_floor_test1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$floor" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << 10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << 10.123 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -10 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10.123 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -11 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "abc" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// mod
TEST( selector, simple_mod_test1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$mod" << 3 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << 10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }


/// 10.123 mod 3 -> 1.1229999999999999
/*
   {
   BSONObj record = BSON( "a" << 10.123 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1.123 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << -10.123 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -1.123 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
*/
   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "abc" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// substr {a:{$substr:3}}
TEST( selector, simple_substr_test1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$substr" << 3 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcde" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abc" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "a" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "a" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// substr {a:{$substr:[2,3]}}
TEST( selector, simple_substr_test2 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$substr" << BSON_ARRAY( 2 << 3 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "cde" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcde" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "cde" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "a" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// substr {a:{$substr:[-2,3]}}
TEST( selector, simple_substr_test3 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$substr" << BSON_ARRAY( -2 << 3 ) ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "ef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "a" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// substr {a:{$substr:-2 }}
TEST( selector, simple_substr_test4 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$substr" << -2 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "ef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "a" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// strlen
TEST( selector, simple_strlen_test1 )
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$strlen" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 6 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 0 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// lower
TEST( selector, simple_lower_test1)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$lower" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdeF" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

///upper
TEST( selector, simple_upper_test1)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$upper" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "ABCDEF" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "ABCDEF" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdeF" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "ABCDEF" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull( "a" ).obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// ltrim
TEST( selector, simple_ltrim_test1)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$ltrim" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << " abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "  \tabcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdef " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef " ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "  \tabcdef " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef " ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << " " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull("a").obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

}

/// rtrim
TEST( selector, simple_rtrim_test1)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$rtrim" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "abcdef " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdef  \t" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << " abcdef  \t" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << " abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << " abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << " abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << " " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull("a").obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// trim
TEST( selector, simple_trim_test1)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$trim" << 1 ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << " abcdef " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "\t abcdef \t" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "abcdef " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << " abcdef" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "abcdef" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "  " ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.appendNull("a").obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast string -> double
/// cast bool -> double
/// cast number -> double
TEST( selector, simple_cast_test_double)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "double" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObj record = BSON( "a" << "1.1" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1.1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "1.0" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1.0 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "-1.1" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << -1.1 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 0.0 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   
   {
   BSONObj record = BSON( "a" << true ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 1.0 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << false ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 0.0 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << 100 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 100.0 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

//// cast
///  int32 -> string
///  int64 -> string
///  double ->string
///  Date -> string
///  Timestamp ->string
///  oid -> string
///  Object -> string
///  Array -> string 
TEST( selector, simple_cast_test_string)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "string" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   
   {
   BSONObj record = BSON( "a" << 100 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "100" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   builder.appendNumber( "a", (long long)100 ) ;
   BSONObj record = builder.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "100" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << 1.1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "1.1" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   Date_t t = 1440133662000 ;
   builder.appendDate( "a", t ) ;
   BSONObj record = builder.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "2015-08-21" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   builder.appendTimestamp( "a", 1440133662000, 400 ) ;
   BSONObj record = builder.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "2015-08-21-13.07.42.000400" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   bson::OID o("55d69ba75fe0fa182d000002") ;
   BSONObjBuilder builder ;
   builder.appendOID( "a", &o ) ;
   BSONObj record = builder.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "55d69ba75fe0fa182d000002" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << BSON( "b" << 1 << "c" << 1 ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "{ \"b\": 1, \"c\": 1 }" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << BSON_ARRAY( 1 << 2 ) ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << "[ 1, 2 ]" ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast
/// string -> object
TEST( selector, simple_cast_test_obj)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "object" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   {
   BSONObj record = BSON( "a" << "{ b:1, c:1}" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << BSON( "b" << 1 << "c" << 1) ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "{ b:1, c:1" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder.appendNull( "a" ) ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast
/// string -> oid
TEST( selector, simple_cast_test_oid)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "oid" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   {
   BSONObjBuilder builder ;
   OID o( "55d6ad7a9e7d552e42000000" ) ;
   builder.appendOID( "a", &o ) ;
   BSONObj record = BSON( "a" << "55d6ad7a9e7d552e42000000" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   builder.appendNull( "a" ) ;
   BSONObj record = BSON( "a" << "abc" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast
/// int32 -> bool
TEST( selector, simple_cast_test4)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "bool" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   {
   BSONObj record = BSON( "a" << 1 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << true ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << 0 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << false ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast
/// int64 -> date
/// string -> date
/// timestamp -> date
TEST( selector, simple_cast_test_date)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "date" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << (long long)1440133662000 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder.appendDate( "a", 1440133662000 ) ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "2015-08-21" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;

   /// not 1440133662000. 
   builder.appendDate( "a", 1440086400000 ) ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder1, builder2 ;
   builder1.appendTimestamp( "a", 1440133662000, 400 ) ;
   BSONObj record = builder1.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder2.appendDate( "a", 1440133662000 ) ;
   BSONObj expect = builder2.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast
/// string -> int32
/// bool -> int32
/// Timestamp -> int32
/// Date -> int32
TEST( selector, simple_cast_test_int32)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "int32" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   {
   BSONObj record = BSON( "a" << "100" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << 100 ) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

}

/// cast
//  int64 -> timestamp
//  string -> timestamp
//  date -> timestamp
TEST( selector, simple_cast_test_timestamp)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "timestamp" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << ( long long )1440133662000 ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder.appendTimestamp( "a", 1440133662000, 0 ) ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder ;
   BSONObj record = BSON( "a" << "2015-08-21-13.07.42.000400" ) ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder.appendTimestamp( "a", 1440133662000, 400 ) ;
   BSONObj expect = builder.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder1, builder2 ;
   builder1.appendDate( "a", 1440133662000 ) ;
   BSONObj record = builder1.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder2.appendTimestamp( "a", 1440133662000, 0 ) ;
   BSONObj expect = builder2.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}

/// cast 
//   timestamp -> int64
//   date -> int64
//   string -> int64
TEST( selector, simple_cast_test_int64)
{
   INT32 rc = SDB_OK ;
   mthSelector selector ;
   BSONObj rule = BSON( "a" << BSON( "$cast" << "int64" ) ) ;
   rc = selector.loadPattern( rule ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   {
   BSONObjBuilder builder1, builder2 ;
   builder1.appendTimestamp( "a", 1440133662000, 0 ) ;
   BSONObj record = builder1.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder2.appendNumber( "a", (long long)1440133662000) ;
   BSONObj expect = builder2.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObjBuilder builder1, builder2 ;
   builder1.appendDate( "a", 1440133662000 ) ;
   BSONObj record = builder1.obj() ;
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   builder2.appendNumber( "a", (long long)1440133662000) ;
   BSONObj expect = builder2.obj() ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }

   {
   BSONObj record = BSON( "a" << "1440133662000" );
   BSONObj result ;
   rc = selector.select( record, result ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   cout << result.toString( FALSE, TRUE ) << endl ;
   BSONObj expect = BSON( "a" << (long long )1440133662000) ;
   rc = expect.woCompare( result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   }
}
