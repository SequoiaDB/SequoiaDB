/******************************************************************************
 * @Description   : seqDB-26370:整组data节点故障后创建/删除索引  
 * @Author        : Wu Yan
 * @CreateTime    : 2022.04.13
 * @LastEditTime  : 2022.04.15
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 } };
testConf.clName = COMMCLNAME + "_indexConsistent_26370";

main( test );
function test ()
{
   var srcGroupName = testPara.srcGroupName;
   var indexName1 = "testcreateindex26370";
   var indexName2 = "testdeleteindex26370";
   var recordNum = 1000;
   insertBulkData( testPara.testCL, recordNum );
   testPara.testCL.createIndex( indexName2, { b: 1 } );

   var rg = db.getRG( srcGroupName );
   try
   {
      rg.stop();
      assert.tryThrow( SDB_CLS_NODE_BSFAULT, function()
      {
         testPara.testCL.createIndex( indexName1, { no: 1 } );
      } );
      assert.tryThrow( SDB_CLS_NODE_BSFAULT, function()
      {
         testPara.testCL.dropIndex( indexName2 );
      } );

      var resultCode = SDB_CLS_NODE_BSFAULT;
      checkOneIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexName1, resultCode );
      checkOneIndexTaskResult( "Drop index", COMMCSNAME, testConf.clName, indexName1, resultCode );

      //再次创建索引、删除索引仍然报错
      assert.tryThrow( SDB_CLS_NODE_BSFAULT, function()
      {
         testPara.testCL.createIndex( indexName1, { no: 1 } );
      } );
      assert.tryThrow( SDB_CLS_NODE_BSFAULT, function()
      {
         testPara.testCL.dropIndex( indexName2 );
      } );

      //恢复节点故障后，创建索引/删除索引操作正常
      rg.start();
      commCheckBusinessStatus( db );

      testPara.testCL.createIndex( indexName1, { no: 1 } );
      checkIndexExist( db, COMMCSNAME, testConf.clName, indexName1, true );
      checkOneIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexName1, 0 );
      testPara.testCL.dropIndex( indexName2 );
      checkIndexExist( db, COMMCSNAME, testConf.clName, indexName2, false );
      checkOneIndexTaskResult( "Drop index", COMMCSNAME, testConf.clName, indexName2, 0 );
   }
   finally
   {
      db.getRG( srcGroupName ).start();
      commCheckBusinessStatus( db );
   }
}

