/******************************************************************************
 * @Description   : seqDB-24304:快照查看切分表创建/删除索引任务
 * @Author        : liuli
 * @CreateTime    : 2021.08.09
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24304";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( args )
{
   var indexName1 = "index_24304_1";
   var indexName2 = "index_24304_2";
   var dbcl = args.testCL;

   // 插入大量数据
   insertBulkData( dbcl, 10000 );

   // 创建索引
   dbcl.createIndex( indexName1, { "no": 1 } );
   dbcl.createIndex( indexName2, { "c": 1 } );
   checkIndexTask( "Create index", COMMCSNAME, COMMCLNAME + "_24304", [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24304", indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24304", indexName2, true );

   // 删除索引
   dbcl.dropIndex( indexName1 );
   dbcl.dropIndex( indexName2 );
   checkIndexTask( "Drop index", COMMCSNAME, COMMCLNAME + "_24304", [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24304", indexName1, false );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24304", indexName2, false );

   var taskIds = getTasksId( COMMCSNAME + "." + COMMCLNAME + "_24304" );
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
      }
      listCur.close();
      snapCur.close();
   }
}