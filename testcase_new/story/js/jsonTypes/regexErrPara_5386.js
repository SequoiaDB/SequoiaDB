/******************************************************************************
*@Description : wrong parameter test Regex function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试Regex函数参数不正确的错误
   // Regex( "表达式", "类型" )指定正则表达式
   var ErrPara = [["^W"]];
   for( var i = 0; i < ErrPara.length; ++i )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         Regex( ErrPara[i] );
      } );
   }

   var rg = Regex( "^W", "i", "i" );

   assert.equal( rg.toString(), Regex( "^W", "i" ).toString() );

   // Regex( "^W", i ) Regex( W, "i" )报语法错误，跳过测试
}
