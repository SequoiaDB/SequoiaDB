/******************************************************************************
 * @Description   : seqDB-23970:主表取消创建索引任务   
 * @Author        : wu yan
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2022.01.24
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true };
testConf.clName = COMMCLNAME + "_index23970a";

main( test );
function test ( testPara )
{
   var indexName = "Index_23970";
   var mainCLName = COMMCLNAME + "_maincl_index23970";
   var subCLName2 = COMMCLNAME + "_subcl_index23970b";
   var recordNum = 40000;

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "range" } );
   maincl.attachCL( COMMCSNAME + "." + testConf.clName, { LowBound: { a: 0 }, UpBound: { a: 20000 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 20000 }, UpBound: { a: 40000 } } );

   var expRecs = insertBulkData( testPara.testCL, recordNum );
   var taskID = maincl.createIndexAsync( indexName, { a: 1, b: 1 } );

   //随机等待500ms再取消任务，覆盖任务ready和running阶段
   var waitTime = Math.floor( Math.random() * 500 );
   sleep( waitTime );
   try
   {
      db.cancelTask( taskID );
   }
   catch( e )
   {
      if( e == SDB_TASK_ALREADY_FINISHED )
      {  //如果任务已执行完成，则取消失败，校验任务执行完成结果
         checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName );
         checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName );
         checkIndexTask( "Create index", COMMCSNAME, mainCLName, indexName );
         println( "----task already finished!" );
         return;
      }
      else
      {
         throw new Error( e );
      }
   }

   waitTaskFinish( COMMCSNAME, mainCLName, "Create index" );
   var errorNo = -243;
   checkIndexTask( "Create index", COMMCSNAME, mainCLName, indexName, errorNo );
   var subcl1ErrorNo = getCreteTaskErrorNo( db, COMMCSNAME, testConf.clName, indexName );
   var subcl2ErrorNo = getCreteTaskErrorNo( db, COMMCSNAME, subCLName2, indexName );
   //子任务1执行成功未取消，子任务2取消成功
   if( subcl1ErrorNo !== errorNo && subcl2ErrorNo == errorNo )
   {
      println( "---subcl1 task has not be cancel! errorno=" + subcl1ErrorNo + ",subcl2 task has be canceled! errorno=" + subcl2ErrorNo );
      checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, 0 );
      checkIndexTask( "Create index", COMMCSNAME, subCLName2, indexName, errorNo );
      commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );
      commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName, false );
   }
   else if( subcl1ErrorNo == errorNo && subcl2ErrorNo !== errorNo )
   {
      //子任务2执行成功未取消，子任务1取消成功
      println( "---subcl2 task has not be cancel! errorno=" + subcl2ErrorNo + ",subcl1 task has be canceled! errorno=" + subcl1ErrorNo );
      checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, errorNo );
      checkIndexTask( "Create index", COMMCSNAME, subCLName2, indexName, 0 );
      commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );
      commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName, true );
   }
   else if( subcl1ErrorNo == errorNo && subcl2ErrorNo == errorNo )
   {
      //子任务1和子任务2都取消成功
      println( "---subcl1 task has be canceled! errorno=" + subcl1ErrorNo + ",subcl2 task has be canceled! errorno=" + subcl2ErrorNo );
      checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, errorNo );
      checkIndexTask( "Create index", COMMCSNAME, subCLName2, indexName, errorNo );
      commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );
      commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName, false );
   }
   else
   {
      //非预期结果
      throw new Error( "---subcl1 task errorno=" + subcl1ErrorNo + ",subcl2 task errorno=" + subcl2ErrorNo );
   }

   var cursor = testPara.testCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}

function getCreteTaskErrorNo ( db, csName, clName, indexName )
{
   var cursor = db.listTasks( { "Name": csName + '.' + clName, TaskTypeDesc: "Create index", "IndexName": indexName } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      var resultCode = taskInfo.ResultCode;
      println( "---" + csName + "." + clName + " task resultcode---" + resultCode )
   }
   cursor.close();
   return resultCode;
}


