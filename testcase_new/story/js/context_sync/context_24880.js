/******************************************************************************
 * @Description   : seqDB-24880:上下文持续访问达到过期超时清理时间 
 * @Author        : liuli
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2021.12.31
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24880";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   try
   {
      db.updateConf( { "contexttimeout": 1 } );
      insertData( dbcl, 20000 );
      var cursor = dbcl.find();

      // 循环变量游标持续2分钟
      for( var i = 0; i < 6000; i++ )
      {
         cursor.next();
         sleep( 20 );
      }
      while( cursor.next() ) { }
   } finally
   {
      db.deleteConf( { "contexttimeout": 1 } );
      cursor.close();
   }
}