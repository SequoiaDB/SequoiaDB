/******************************************************************************
*@Description : wrong parameter test NumberLong function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试NumberLong函数参数不正确的错误
   // NumberLong( 100 ) NumberLong( "100" )指定64位整数
   var ErrPara = [[100, 101], ["100", 101], [],
   [true], ["abc"]];
   for( var i = 0; i < ErrPara.length; ++i )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         NumberLong( ErrPara[i] );
      } );
   }
}
