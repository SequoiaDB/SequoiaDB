/******************************************************************************
 * @Description   : seqDB-24881:主子表上下文过期超时清理
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.29
 * @LastEditTime  : 2022.04.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_24881";
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var cs = testPara.testCS;
   var csName = testConf.csName;
   var clName = "maincl_24881";
   var cl1Name = "subcl_24881_1";
   var cl2Name = "subcl_24881_2";
   var cl3Name = "subcl_24881_3";
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