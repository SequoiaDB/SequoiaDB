/******************************************************************************
 * @Description   : seqDB-24268:删除主表后访问使用数据源的子表，覆盖场景1
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

   var dataSrcName = "datasrc24268a";
   var csName = "cs_24268a";
   var clName = "cl_24268a";
   var mainCLName = "maincl_24268a";
   var srcCSName = "datasrcCS_24268a";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db1, csName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var mainCL = dbcs.createCL( mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dbcl2 = db2.getCS( csName ).getCL( clName );
   var dbcl3 = db3.getCS( csName ).getCL( clName );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 100 } } );

   db2.getCS( csName ).dropCL( mainCLName );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.insert( { a: 1 } );
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl2.insert( { a: 1 } );
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl3.insert( { a: 1 } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
   db2.close();
   db3.close();
}