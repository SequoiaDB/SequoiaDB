/******************************************************************************
 * @Description   : seqDB-24884:查询指定sort()，持续访问游标达到上下文超时清理时间
 * @Author        : liuli
 * @CreateTime    : 2021.12.31
 * @LastEditTime  : 2022.01.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24884";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   try
   {
      db.updateConf( { "contexttimeout": 1 } );
      var docs = insertData( dbcl, 100000 );
      docs.splice( 0, 6000 );
      var cursor = dbcl.find();

      // 循环变量游标持续2分钟
      for( var i = 0; i < 6000; i++ )
      {
         cursor.next();
         sleep( 20 );
      }
      while( cursor.next() ) { }
   }
   finally
   {
      db.deleteConf( { "contexttimeout": 1 } );
      cursor.close();
   }
}