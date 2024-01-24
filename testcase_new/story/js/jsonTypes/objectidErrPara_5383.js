/******************************************************************************
*@Description : wrong parameter test ObjectId function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试ObjectId函数参数不正确的错误
   // ObjectId()生成一个OID
   // ObjectId( 24字节16进制字符串 )生成一个指定值的OID
   var ErrPara = [["55713f7953e6769804000001", 1],
   ["55713f7953e676980400000123"]];
   for( var i = 0; i < ErrPara.length; ++i )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         ObjectId( ErrPara[i] );
      } );
   }

   // ObjectId( 55713f7953e6769804000001 )报语法错误，跳过测试
}
