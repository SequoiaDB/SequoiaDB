/******************************************************************************
 * @Description   : seqDB-25344:分区表上下文过期超时清理
 * @Author        : 钟子明
 * @CreateTime    : 2022.01.29
 * @LastEditTime  : 2022.04.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_25344";
testConf.clName = COMMCLNAME + "_25344";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: "hash", AutoSplit: true };
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var cursor;
   try
   {
      db.updateConf( { "contexttimeout": 1 } );

      insertData( cl, 50000 );

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