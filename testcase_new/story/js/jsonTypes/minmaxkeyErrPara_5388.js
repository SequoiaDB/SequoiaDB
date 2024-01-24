/******************************************************************************
*@Description : wrong parameter test MinKey MaxKey function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试MinKey MaxKey函数参数不正确的错误
   // MinKey() MaxKey()生成MinKey, MaxKey对象
   var ErrPara = [[1], ["1", "1"]];
   var temp;

   for( var i = 0; i < ErrPara.length; ++i )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         temp = MinKey( ErrPara[i] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         temp = MaxKey( ErrPara[i] );
      } );
   }
}
