/******************************************************************************
*@Description : test NumberLong function with Boundary Value
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试NumberLong函数参数为边界值的情况
   // NumberLong( 100 ) NumberLong( "100" )指定64位整数
   var minv = -9007199254740992;
   var maxv = 9007199254740992;

   assert.equal( NumberLong( minv ), minv );

   assert.equal( NumberLong( maxv ), maxv );

}
