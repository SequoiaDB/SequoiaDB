/******************************************************************************
 * @Description   : seqDB-24882:主子表访问一个子表达到上下文超时清理时间 
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.30
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_24882";
testConf.skipStandAlone = true;
// 问题单：http://jira.web:8080/browse/SEQUOIADBMAINSTREAM-7888
main( test );
function test ( testPara )
{
   var cs = testPara.testCS;
   var csName = testConf.csName;
   var clName = "maincl_24882";
   var cl1Name = "subcl_24882_1";
   var cl2Name = "subcl_24882_2";
   var cl3Name = "subcl_24882_3";
   var cursor;
   try
   {
      db.updateConf( { "contexttimeout": 1 } );
      var cl = cs.createCL( clName, { IsMainCL: true, ShardingKey: { no: 1 }, ShardingType: "range" } );
      cs.createCL( cl1Name, { ShardingKey: { no: 1 } } );
      cs.createCL( cl2Name, { ShardingKey: { no: 1 } } );
      cs.createCL( cl3Name, { ShardingKey: { no: 1 } } );

      cl.attachCL( csName + "." + cl1Name, { LowBound: { no: 0 }, UpBound: { no: 20000 } } );
      cl.attachCL( csName + "." + cl2Name, { LowBound: { no: 20000 }, UpBound: { no: 40000 } } );
      cl.attachCL( csName + "." + cl3Name, { LowBound: { no: 40000 }, UpBound: { no: 60000 } } );

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