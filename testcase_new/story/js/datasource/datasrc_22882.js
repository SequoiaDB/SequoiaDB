/******************************************************************************
 * @Description   : seqDB-22882:本地子表关联不同数据源，挂载到主表
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
//需要两个不同数据源，目前CI环境不支持，用例暂时屏蔽
testConf.skipStandAlone = true;
//main( test );
function test ()
{
   var dataSrcName = "datasrc22882";
   var dataSrcName1 = "datasrc22882_b";
   var csName = "cs_22882";
   var clName = "cl_22882";
   var srcCSName = "datasrcCS_22882";
   var mainCLName = "mainCL_22882";
   var subCLName = "subCL_22882";
   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );
   var datasrcDB1 = new Sdb( datasrcIp1, datasrcPort, userName, passwd );
   commDropCS( datasrcDB, srcCSName );
   commDropCS( datasrcDB1, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   commCreateCS( datasrcDB1, srcCSName );
   commCreateCL( datasrcDB1, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName1, datasrcUrl1, userName, passwd );

   var cs = db.createCS( csName );
   cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { DataSource: dataSrcName1, Mapping: srcCSName + "." + clName } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var recordNum = 1000;
   var expRecs = insertBulkData( mainCL, recordNum, 0, 1000 );
   var cursor = mainCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   var count = mainCL.count();
   println( "--count=" + count );
   if( Number( count ) != Number( recordNum ) ) 
   {
      throw "act records num is " + count + "  ecp record num is " + recordNum;
   }

   db.dropCS( csName );
   datasrcDB.dropCS( srcCSName );
   db.dropDataSource( dataSrcName );
   datasrcDB1.dropCS( srcCSName );
   db.dropDataSource( dataSrcName1 );
   datasrcDB.close();
   datasrcDB1.close();
}


