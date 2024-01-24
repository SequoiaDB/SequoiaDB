/******************************************************************************
 * @Description   : seqDB-24316 :: 所有子表使用数据源，主表上创建索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.11
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : Wu Yan
 ******************************************************************************/
//TODO:需要多个数据源的环境，目前CI只有一个数据源，暂时屏蔽
testConf.skipStandAlone = true;
//main( test );
function test ( testPara )
{
   var indexName = "Index_24316";
   var dataSrcName1 = "datasrc24316a";
   var dataSrcName2 = "datasrc24316b";
   var csName = "cs_24136";
   var srcCSName = "srccs_24316";
   var clName = "srccl_24316";
   var mainCLName = COMMCLNAME + "_maincl_index24316";
   var subCLName1 = COMMCLNAME + "_subcl_index24316a";
   var subCLName2 = COMMCLNAME + "_subcl_index24316b";
   var recordNum = 20000;

   var datasrcDB2 = new Sdb( other_datasrcIp1, datasrcPort, userName, passwd );
   commDropCS( datasrcDB, srcCSName );
   commDropCS( datasrcDB2, srcCSName );
   clearDataSource( csName, dataSrcName1 );
   clearDataSource( csName, dataSrcName2 );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   commCreateCS( datasrcDB2, srcCSName );
   commCreateCL( datasrcDB2, srcCSName, clName );
   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName2, otherDSUrl1, userName, passwd );

   var cs = db.createCS( csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   cs.createCL( subCLName1, { DataSource: dataSrcName1, Mapping: srcCSName + "." + clName } );
   cs.createCL( subCLName2, { DataSource: dataSrcName2, Mapping: srcCSName + "." + clName } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   var expRecs = insertBulkData( maincl, recordNum );
   maincl.createIndex( indexName, { a: 1, b: 1 } );
   checkTask( csName, mainCLName, indexName );

   commCheckIndexConsistent( datasrcDB, srcCSName, clName, indexName, false );
   commCheckIndexConsistent( datasrcDB2, srcCSName, clName, indexName, false );

   var cursor = maincl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   checkExplain( maincl, { a: 1, b: 1 }, "tbscan", "" );
   checkExplain( maincl, { a: 10000, b: 10000 }, "tbscan", "" );

   db.dropCS( csName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB2.dropCS( srcCSName );
   db.dropDataSource( dataSrcName1 );
   db.dropDataSource( dataSrcName2 );
   datasrcDB.close();
   datasrcDB2.close();
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

   assert.equal( taskInfo.IndexName, indexName, "check indexName error!\ntask=" + JSON.stringify( taskInfo ) );
}
