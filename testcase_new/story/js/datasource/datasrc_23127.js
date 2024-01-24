/******************************************************************************
 * @Description   : seqDB-23127:使用数据源的集合执行count操作 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23127";
   var csName = "cs_23127";
   var clName = "cl_23127";
   var srcCSName = "datasrcCS_23127";
   var mainCLName = "mainCL_23127";
   var subCLName = "subCL_23127";
   var recordNum = 2000;

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 普通集合
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   insertBulkData( dbcl, recordNum, 0, 4000 );
   var count = dbcl.count( { a: { $gte: 1000 } } );
   if( Number( count ) !== Number( 1000 ) ) 
   {
      throw "act records num is " + count + "  ecp record num is " + 1000;
   }

   commDropCS( db, csName );
   datasrcDB.getCS( srcCSName ).getCL( clName ).remove();

   // 主子表
   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   insertBulkData( mainCL, recordNum, 0, 4000 );
   var count = mainCL.count( { a: { $gte: 500 } } );
   if( Number( count ) !== Number( 1500 ) ) 
   {
      throw "act records num is " + count + "  ecp record num is " + 1500;
   }

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}