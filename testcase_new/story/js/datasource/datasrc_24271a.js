/******************************************************************************
 * @Description   : seqDB-24271:删除使用数据源的子表后，从主表对子表进行访问，覆盖场景1
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

   var dataSrcName = "datasrc24271a";
   var csName = "cs_24271a";
   var clName = "cl_24271a";
   var mainCLName = "maincl_24271a";
   var srcCSName = "datasrcCS_24271a";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db1, csName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var mainCL = dbcs.createCL( mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var mainCL2 = db2.getCS( csName ).getCL( mainCLName );
   var mainCL3 = db3.getCS( csName ).getCL( mainCLName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.insert( { a: 1 } );

   db2.getCS( csName ).dropCL( clName );

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