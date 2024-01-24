/******************************************************************************
 * @Description   : seqDB-25343:分区表查询达到上下文超时清理时间
 * @Author        : 钟子明
 * @CreateTime    : 2022.01.29
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_25343";
testConf.clName = COMMCLNAME + "_25343";
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

      insertData( cl, 60000 );

      cursor = cl.find();
      for( var i = 0; i < 12; i++ )
      {
         for( var j = 0; j < 1000; j++ )
         {
            cursor.next();
         }
         sleep( 1000 * 10 );
      }
      while( cursor.next() ) { }
   } finally
   {
      //恢复默认配置
      db.deleteConf( { "contexttimeout": 1 } );
      cursor.close();
   }
}