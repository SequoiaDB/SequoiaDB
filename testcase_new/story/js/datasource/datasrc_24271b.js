/******************************************************************************
 * @Description   : seqDB-24271:删除使用数据源的子表后，从主表对子表进行访问，覆盖场景2
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
   if( coordArr.length < 2 )
   {
      return;
   }
   var db1 = new Sdb( coordArr[0] );
   var db2 = new Sdb( coordArr[1] );

   var dataSrcName = "datasrc24271b";
   var csName = "cs_24271b";
   var clName = "cl_24271b";
   var mainCLName = "maincl_24271b";
   var srcCSName = "datasrcCS_24271b";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db1, csName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var mainCL = dbcs.createCL( mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var mainCL2 = db2.getCS( csName ).getCL( mainCLName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.insert( { a: 1 } );

   db1.getCS( csName ).dropCL( clName );

   var cursor = mainCL.find();
   commCompareResults( cursor, [] );

   var cursor = mainCL2.find();
   commCompareResults( cursor, [] );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
   db2.close();
}