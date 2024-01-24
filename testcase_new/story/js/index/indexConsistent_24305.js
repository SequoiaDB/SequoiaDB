/******************************************************************************
 * @Description   : seqDB-24305 :: 多个子表在相同组上，主表创建索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.02
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var indexName = "Index_24305";
   var mainCLName = COMMCLNAME + "_maincl_index24305";
   var subCLName = COMMCLNAME + "_subcl_index24305";
   var recordNum = 100000;
   var srcGroupName = testPara.srcGroupName;

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ReplSize: -1, ShardingType: "range", IsMainCL: true } );

   var subclNum = 10;
   createAndAttachSubCL( maincl, subCLName, subclNum, srcGroupName );
   var expRecs = insertBulkData( maincl, recordNum );
   maincl.createIndex( indexName, { a: 1, b: 1 } );

   checkIndexConsistentWithSubcls( subCLName, subclNum, indexName, true );
   checkTasks( mainCLName, subCLName, subclNum, indexName, "Create index" );
   checkExplainByMaincl( maincl, { a: 1, b: 1 }, "ixscan", indexName );
   checkExplainByMaincl( maincl, { a: 10000, b: 10000 }, "ixscan", indexName );

   maincl.dropIndex( indexName );

   checkIndexConsistentWithSubcls( subCLName, subclNum, indexName, false );
   checkTasks( mainCLName, subCLName, subclNum, indexName, "Drop index" );
   checkExplainByMaincl( maincl, { a: 1, b: 1 }, "ixscan", "$shard" );
   checkExplainByMaincl( maincl, { a: 10000, b: 10000 }, "ixscan", "$shard" );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}

function checkTasks ( mainCLName, subclName, subclNum, indexName, taskTypeDesc )
{
   checkMainTask( COMMCSNAME, mainCLName, indexName, subclNum );

   for( var i = 0; i < subclNum; i++ )
   {
      var name = subclName + "_" + i;
      checkIndexTask( taskTypeDesc, COMMCSNAME, name, [indexName] );
      checkSnapTask( COMMCSNAME, name, indexName, taskTypeDesc );
   }
}

function checkIndexConsistentWithSubcls ( subclName, subclNum, indexName, isExistIndex )
{
   for( var i = 0; i < subclNum; i++ )
   {
      var name = subclName + "_" + i;
      commCheckIndexConsistent( db, COMMCSNAME, name, indexName, isExistIndex );
   }
}

function createAndAttachSubCL ( maincl, subclName, subclNum, groupName )
{
   var lowValue = 0;
   var bound = 10000;
   var upValue = 10000;
   for( var i = 0; i < subclNum; i++ )
   {
      var name = subclName + "_" + i;
      commCreateCL( db, COMMCSNAME, name, { ShardingKey: { a: 1 }, Group: groupName } );
      maincl.attachCL( COMMCSNAME + "." + name, { LowBound: { a: lowValue }, UpBound: { a: upValue } } );
      lowValue = lowValue + bound;
      upValue = lowValue + bound;
   }
}

function checkMainTask ( csName, mainCLName, indexName, subclNum )
{
   var cursor = db.listTasks( { "Name": csName + '.' + mainCLName, TaskTypeDesc: "Create index" } );
   var taskInfo;
   while( cursor.next() )
   {
      taskInfo = cursor.current().toObj();
   }
   cursor.close();

   var status = 9;
   assert.equal( taskInfo.Status, status, "check status error! \ntask=" + JSON.stringify( taskInfo ) );

   var expCode = 0;
   assert.equal( taskInfo.ResultCode, expCode, "check resultcode error! \ntask=" + JSON.stringify( taskInfo ) );

   assert.equal( taskInfo.IndexName, indexName, "check indexname error! \ntask=" + JSON.stringify( taskInfo ) );

   var subTasks = taskInfo.SubTasks;
   assert.equal( subTasks.length, subclNum, "subTasks=" + JSON.stringify( subTasks ) );
   for( var i = 0; i < subTasks.length; i++ )
   {
      var subtask = subTasks[i];
      assert.equal( subtask.ResultCode, expCode, "check subtask code error! \n subtask=" + JSON.stringify( subtask ) );
      assert.equal( subtask.Status, status, "check subtask status error! \n subtask=" + JSON.stringify( subtask ) );
   }

   var mainTaskId = taskInfo.TaskID;
   return mainTaskId;
}

function checkSnapTask ( csName, clName, indexName, taskTypeDesc )
{
   var cursor = db.snapshot( SDB_SNAP_TASKS, { "Name": csName + '.' + clName, "TaskTypeDesc": taskTypeDesc } );
   var expCode = 0;
   var status = 9;
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      assert.equal( taskInfo.ResultCode, expCode, "check task code error! \ntask=" + JSON.stringify( taskInfo ) );
      assert.equal( taskInfo.Status, status, "check task status error! \ntask=" + JSON.stringify( taskInfo ) );
      assert.equal( taskInfo.IndexName, indexName, "check indexname error! \ntask=" + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}