/******************************************************************************
 * @Description   : seqDB-24312 :: 主表执行创建/删除/复制索引，renameCS/CL名   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.12
 * @LastEditTime  : 2021.11.26
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var indexName = "Index_24312";
   var csName = "cs_24312";
   var newCSName = "newcs_24312";
   var mainCLName = "maincl_index24312";
   var newMainCLName = "newmaincl_index24312";
   var subCLName1 = "subcl_index24312a";
   var subCLName2 = "subcl_index24312b";
   var recordNum = 20000;

   commDropCS( db, csName, true, "clean cs in the beginning." );
   commDropCS( db, newCSName, true, "clean cs in the beginning." );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var createTaskId = maincl.createIndexAsync( indexName, { a: 1, b: 1 } );
   waitTaskFinish( csName, mainCLName, "Create index" );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   //主表上复制索引到指定子表
   var expRecs = insertBulkData( maincl, recordNum );
   var copyTaskId = maincl.copyIndexAsync( csName + "." + subCLName1, indexName );
   waitTaskFinish( csName, mainCLName, "Copy index" );

   //主表上删除索引
   var dropTaskId = maincl.dropIndexAsync( indexName );
   waitTaskFinish( csName, mainCLName, "Drop index" );

   db.renameCS( csName, newCSName );
   checkListTask( createTaskId, newCSName, mainCLName, indexName, "Create index" );
   checkListTask( dropTaskId, newCSName, mainCLName, indexName, "Drop index" );
   checkListTask( copyTaskId, newCSName, mainCLName, indexName, "Copy index", subCLName1 );
   checkSnapTask( createTaskId, newCSName, mainCLName, indexName, "Create index" );
   checkSnapTask( dropTaskId, newCSName, mainCLName, indexName, "Drop index" );

   var cs = db.getCS( newCSName );
   cs.renameCL( mainCLName, newMainCLName );
   checkListTask( createTaskId, newCSName, newMainCLName, indexName, "Create index" );
   checkListTask( dropTaskId, newCSName, newMainCLName, indexName, "Drop index" );
   checkListTask( copyTaskId, newCSName, newMainCLName, indexName, "Copy index", subCLName1 );
   checkIndexTask( "Create index", newCSName, subCLName1, indexName );
   checkIndexTask( "Drop index", newCSName, subCLName1, indexName );
   checkSnapTask( createTaskId, newCSName, mainCLName, indexName, "Create index" );
   checkSnapTask( dropTaskId, newCSName, mainCLName, indexName, "Drop index" );

   checkNoTask( csName, mainCLName, "Create index" );
   checkNoTask( csName, mainCLName, "Drop index" );
   checkNoTask( csName, mainCLName, "Copy index" );
   checkNoTask( csName, subCLName1, "Create index" );
   checkNoTask( csName, subCLName2, "Create index" );
   checkNoTask( csName, subCLName1, "Drop index" );
   checkNoTask( csName, subCLName2, "Drop index" );
   commDropCS( db, newCSName, true, "clean cs in the ending." );
}

function checkListTask ( taskId, csName, clName, indexName, taskTypeDesc, subCLName1 )
{
   if( undefined == subCLName1 ) { subCLName1 = "subcl_index24312a" }
   var cursor = db.listTasks( { "TaskID": taskId } );
   var taskInfo;
   var count = 0;
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      assert.equal( taskInfo.Name, csName + "." + clName, "check name error! \ntask=" + JSON.stringify( taskInfo ) );
      if( taskInfo.TaskTypeDesc == "Copy index" )
      {
         assert.equal( taskInfo.IndexNames, [indexName], "check indexName error!\ntask=" + JSON.stringify( taskInfo ) );
         assert.equal( taskInfo.CopyTo, [csName + "." + subCLName1], "check copy subclName error!\ntask=" + JSON.stringify( taskInfo ) );
      }
      else
      {
         assert.equal( taskInfo.IndexName, indexName, "check indexName error!\ntask=" + JSON.stringify( taskInfo ) );
      }

      assert.equal( taskInfo.TaskTypeDesc, taskTypeDesc, "check taskType error!\ntask=" + JSON.stringify( taskInfo ) );
      count++;
   }
   cursor.close();
   assert.equal( count, 1, "check task num must be 1!" );
}

function checkSnapTask ( taskId, csName, clName, indexName, taskTypeDesc )
{
   var cursor = db.snapshot( SDB_SNAP_TASKS, { "TaskID": taskId } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      assert.equal( taskInfo.Name, csName + "." + clName, "check name error! \ntask=" + JSON.stringify( taskInfo ) );
      assert.equal( taskInfo.IndexName, indexName, "check indexName error!\ntask=" + JSON.stringify( taskInfo ) );
      assert.equal( taskInfo.TaskTypeDesc, taskTypeDesc, "check taskType error!\ntask=" + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}

