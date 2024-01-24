/******************************************************************************
 * @Description   : seqDB-24879:上下文过期超时清理 
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.29
 * @LastEditTime  : 2022.04.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24879";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var cursor;
   try
   {
      db.updateConf( { "contexttimeout": 1 } );
      insertData( cl, 30000 );
      cursor = cl.find();
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