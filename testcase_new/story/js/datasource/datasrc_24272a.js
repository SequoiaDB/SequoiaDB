/******************************************************************************
 * @Description   : seqDB-24272:删除使用数据源的子表后重新挂载一个同名的使用数据源的子表，从主表对子表进行访问，覆盖场景1
 * @Author        : liuli
 * @CreateTime    : 2021.06.21
 * @LastEditTime  : 2021.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var coordArr = getCoordUrl( db );
   if( coordArr.length < 3 )
   {
      return;
   }
   var db1 = new Sdb( coordArr[0] );
   var db2 = new Sdb( coordArr[1] );
   var db3 = new Sdb( coordArr[2] );

   var dataSrcName = "datasrc24272a";
   var csName = "cs_24272a";
   var clName = "cl_24272a";
   var clName2 = "cl_24272a_1";
   var mainCLName = "maincl_24272a";
   var srcCSName = "datasrcCS_24272a";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCL( datasrcDB, srcCSName, clName );
   commCreateCL( datasrcDB, srcCSName, clName2 );
   var dbcs = commCreateCS( db1, csName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var mainCL = dbcs.createCL( mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var mainCL2 = db2.getCS( csName ).getCL( mainCLName );
   var mainCL3 = db3.getCS( csName ).getCL( mainCLName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.insert( { a: 1 } );

   db2.getCS( csName ).dropCL( clName );
   db2.getCS( csName ).createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName2 } );
   db2.getCS( csName ).getCL( mainCLName ).attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   var cursor = mainCL.find();
   commCompareResults( cursor, [] );

   var cursor = mainCL2.find();
   commCompareResults( cursor, [] );

   var cursor = mainCL3.find();
   commCompareResults( cursor, [] );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
   db2.close();
   db3.close();
}