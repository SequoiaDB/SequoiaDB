/******************************************************************************
 * @Description   : seqDB-24299 :: 等待执行成功的任务   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.02
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range" };
testConf.clName = COMMCLNAME + "_index24299";

main( test );
function test ( testPara )
{
   var indexName = "Index_24299";
   var recordNum = 100000;
   var expRecs = insertBulkData( testPara.testCL, recordNum );
   var taskID = testPara.testCL.createIndexAsync( indexName, { testa: 1, b: 1 } );

   db.waitTasks( taskID );
   checkTask( COMMCSNAME, testConf.clName, indexName );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );
}

function checkTask ( csName, clName, indexName )
{
   var cursor = db.listTasks( { "Name": csName + '.' + clName, TaskTypeDesc: "Create index" } );
   var taskInfo;
   while( cursor.next() )
   {
      taskInfo = cursor.current().toObj();
   }
   cursor.close();

   var status = 9;
   assert.equal( taskInfo.Status, status, "check status error!\ntask=" + JSON.stringify( taskInfo ) );

   var expCode = 0;
   assert.equal( taskInfo.ResultCode, expCode, "check resultcode error!\ntask=" + JSON.stringify( taskInfo ) );

   assert.equal( taskInfo.IndexName, indexName, "check indexname error!\ntask=" + JSON.stringify( taskInfo ) );
}