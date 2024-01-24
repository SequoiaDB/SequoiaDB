/******************************************************************************
 * @Description   : seqDB-23474 : 多个子表使用不同数据源，主表上执行CRUD操作
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
//需要多个数据源的环境，目前CI只有一个数据源，暂时屏蔽
testConf.skipStandAlone = true;
//main( test );
function test ()
{
   var dataSrcName1 = "jsdatasrc22474_a";
   var dataSrcName2 = "jsdatasrc22474_b";
   var dataSrcName3 = "jsdatasrc22474_c";
   var csName = "jscs_22474test";
   var clName = "jscl_22474test";
   var mainCLName = "jsmainCL_22474test";
   var subCLName1 = "jssubCL_22474_a";
   var subCLName2 = "jssubCL_22474_b";
   var subCLName3 = "jssubCL_22474_c";
   var subCLName4 = "jssubCL_22474_d";
   var datasrcDB1 = new Sdb( datasrcIp, datasrcPort, userName, passwd );
   var datasrcDB2 = new Sdb( other_datasrcIp1, datasrcPort, userName, passwd );
   var datasrcDB3 = new Sdb( other_datasrcIp2, datasrcPort, userName, passwd );

   commDropCS( datasrcDB1, csName );
   commDropCS( datasrcDB2, csName );
   commDropCS( datasrcDB3, csName );
   clearDataSource( csName, dataSrcName1 );
   clearDataSource( csName, dataSrcName2 );
   clearDataSource( csName, dataSrcName3 );
   commCreateCS( datasrcDB1, csName );
   commCreateCL( datasrcDB1, csName, clName );
   commCreateCS( datasrcDB2, csName );
   commCreateCL( datasrcDB2, csName, clName );
   commCreateCS( datasrcDB3, csName );
   commCreateCL( datasrcDB3, csName, clName );
   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName2, otherDSUrl1, userName, passwd );
   db.createDataSource( dataSrcName3, otherDSUrl2, userName, passwd );

   var cs = db.createCS( csName );
   cs.createCL( clName, { DataSource: dataSrcName1 } );
   cs.createCL( subCLName2, { DataSource: dataSrcName2, Mapping: clName } );
   cs.createCL( subCLName3, { DataSource: dataSrcName3, Mapping: csName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 20000 }, UpBound: { a: 30000 } } );
   mainCL.attachCL( csName + "." + subCLName3, { LowBound: { a: 30000 }, UpBound: { a: 40000 } } );

   var recordNum = 40000;
   var expRecs = insertBulkData( mainCL, recordNum, 0, 40000 );
   var cursor = mainCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   var count = mainCL.count();
   assert.equal( count, recordNum );

   mainCL.remove( { a: { $gte: 500 } } );
   var count = mainCL.count();
   assert.equal( count, 500 );

   mainCL.truncate();
   var count = mainCL.count();
   assert.equal( count, 0 );

   db.dropCS( csName );
   datasrcDB1.dropCS( csName );
   datasrcDB2.dropCS( csName );
   datasrcDB3.dropCS( csName );
   db.dropDataSource( dataSrcName1 );
   db.dropDataSource( dataSrcName2 );
   db.dropDataSource( dataSrcName3 );
   datasrcDB1.close();
   datasrcDB2.close();
   datasrcDB3.close();
}


