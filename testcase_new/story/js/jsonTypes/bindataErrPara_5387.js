/******************************************************************************
*@Description : wrong parameter test BinData function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试Bindata函数参数不正确的错误
   // BinData( "base64加密后的内容", "类型" )指定二进制对象
   var ErrPara = [["aGVsbG8gd29ybGQ=", "1", "1"], ["aGVsbG8gd29ybGQ="]];
   for( var i = 0; i < ErrPara.length; ++i )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         BinData( ErrPara[i] );
      } );
   }

   var bd = BinData( "aGVsbG8gd29ybGQ=", 1 );
   assert.equal( bd.toString(), BinData( "aGVsbG8gd29ybGQ=", "1" ).toString() );

   // BinData( aGVsbG8gd29ybGQ =, "1" )报语法错误，跳过测试
}
