/******************************************************************************
 * @Description   : seqDB-23975:快照查看主子表创建/复制/删除索引任务
 * @Author        : liuli
 * @CreateTime    : 2021.08.05
 * @LastEditTime  : 2022.01.19
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_maincl_23975";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };

main( test );
function test ( args )
{
   var indexName1 = "index_23975_1";
   var indexName2 = "index_23975_2";
   var subCLName1 = "subcl_23975_1";
   var subCLName2 = "subcl_23975_2";
   var maincl = args.testCL;

   var subcl = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5000 } } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 5000 }, UpBound: { a: 10000 } } );

   // 创建索引
   maincl.createIndex( indexName1, { b: 1 } );
   maincl.createIndex( indexName2, { c: 1 } );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName2, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName2, true );

   // 插入大量数据
   insertBulkData( maincl, 10000 );

   // 子表删除索引
   subcl.dropIndex( indexName1 );
   // 校验任务和索引一致性
   checkIndexTask( "Drop index", COMMCSNAME, subCLName1, indexName1, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName1, false );

   // 复制索引
   maincl.copyIndex( COMMCSNAME + "." + subCLName1, indexName1 );
   checkCopyTask( COMMCSNAME, COMMCLNAME + "_maincl_23975", indexName1, COMMCSNAME + "." + subCLName1, 0 );
   checkIndexTask( "Create index", COMMCSNAME, subCLName1, [indexName1, indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName1, true );

   // 主表删除索引，只有一个子表有索引
   maincl.dropIndex( indexName1 );
   checkIndexTask( "Drop index", COMMCSNAME, subCLName1, [indexName1, indexName1], 0 );
   checkIndexTask( "Drop index", COMMCSNAME, subCLName2, indexName1, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName1, false );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName1, false );

   // 主表删除索引，两个子表都有索引
   maincl.dropIndex( indexName2 );
   checkIndexTask( "Drop index", COMMCSNAME, subCLName1, [indexName1, indexName1, indexName2], 0 );
   checkIndexTask( "Drop index", COMMCSNAME, subCLName2, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName2, false );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName2, false );

   var taskIds = getTasksId( COMMCSNAME + "." + subCLName1 );
   snapAndList( taskIds );

   var taskIds = getTasksId( COMMCSNAME + "." + subCLName2 );
   snapAndList( taskIds );
}

function getTasksId ( fullName )
{
   var taskIds = [];
   var cur = db.listTasks( { Name: fullName } );
   while( cur.next() )
   {
      var taskId = cur.current().toObj().TaskID;
      taskIds.push( taskId );
   }
   return taskIds;
}

function snapAndList ( taskIds )
{
   for( var i = 0; i < taskIds.length; i++ )
   {
      var listCur = db.listTasks( { TaskID: taskIds[i] } );
      listCur.next();
      var snapCur = db.snapshot( SDB_SNAP_TASKS, { TaskID: taskIds[i] } );
      while( snapCur.next() )
      {
         assert.equal( listCur.current().toObj().Status, snapCur.current().toObj().Status );
         assert.equal( listCur.current().toObj().StatusDesc, snapCur.current().toObj().StatusDesc );
         assert.equal( listCur.current().toObj().TaskType, snapCur.current().toObj().TaskType );
         assert.equal( listCur.current().toObj().TaskTypeDesc, snapCur.current().toObj().TaskTypeDesc );
         assert.equal( listCur.current().toObj().ResultCode, snapCur.current().toObj().ResultCode );
         assert.equal( listCur.current().toObj().ResultCodeDesc, snapCur.current().toObj().ResultCodeDesc );
         assert.equal( listCur.current().toObj().Name, snapCur.current().toObj().Name );
         assert.equal( listCur.current().toObj().IndexName, snapCur.current().toObj().IndexName );
      }
   }
}