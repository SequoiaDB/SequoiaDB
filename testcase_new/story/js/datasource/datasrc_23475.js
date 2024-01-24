/******************************************************************************
 * @Description   : seqDB-23475:多个子表使用相同数据源，主表上执行CRUD操作
 * @Author        : Wu Yan
 * @CreateTime    : 2021.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23475";
   var csName = "cs_23475";
   var clName = "cl_23475";
   var clName1 = "cl_23475a";
   var mainCLName = "mainCL_23475";
   var subCLName = "subCL_23475";
   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );
   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, csName );
   commCreateCL( datasrcDB, csName, clName );
   commCreateCL( datasrcDB, csName, clName1 );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var cs = db.createCS( csName );
   cs.createCL( clName, { DataSource: dataSrcName } );
   cs.createCL( subCLName, { DataSource: dataSrcName, Mapping: clName1 } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );

   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      mainCL.attachCL( csName + "." + clName, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );
   } );

   var recordNum = 10000;
   var expRecs = insertBulkData( mainCL, recordNum, 0, 10000 );
   var cursor = mainCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( csName );
   datasrcDB.close();
}


