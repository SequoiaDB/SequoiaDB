/******************************************************************************
*@Description : wrong parameter test Timestamp function
*@Modify list :
*               2016-07-11  XueWang Liang  Init
******************************************************************************/

main( test );

function test ()
{
   // 测试Timestamp函数参数不正确的错误
   // Timestamp()当前时间戳； Timestamp( "YYYY-MM-DD-HH.mm.ss.ffffff" )指定时间的时间戳
   // Timestamp( 秒数，微秒数 )使用绝对秒数指定时间戳
   var ErrPara = [[1433492413, 0, 0], [1433492413],
   ["2015-06-05-16.10.33.000000", true],
   ["1433492413", 0], [1433492413, "0"]];
   var ErrCode = -6;
   for( var i = 0; i < ErrPara.length; ++i )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         Timestamp( ErrPara[i] );
      } );
   }

   // Timestamp( 2015-06-05-16.10.33.000000 )报语法错误，跳过测试
}
