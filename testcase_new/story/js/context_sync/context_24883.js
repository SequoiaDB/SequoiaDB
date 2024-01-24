/******************************************************************************
 * @Description   : seqDB-24883:查询指定sort()上下文超时清理 
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.30
 * @LastEditTime  : 2022.04.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24883";
main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var cursor;
   try
   {
      db.updateConf( { "contexttimeout": 1 } );
      insertData( cl, 50000 );
      cursor = cl.find().sort( { no: 1 } );
      cursor.next();
      sleep( 1000 * 60 * 3 );
      assert.tryThrow( SDB_RTN_CONTEXT_NOTEXIST, function()
      {
         while( cursor.next() ) { }
      } );
   } finally
   {
      //恢复默认配置
      db.deleteConf( { "contexttimeout": 1 } );
      cursor.close();
   }
}