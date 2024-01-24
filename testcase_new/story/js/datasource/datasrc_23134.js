/******************************************************************************
 * @Description   : seqDB-23134:创建使用数据源的集合，连接不同coord删除集合再创建本地同名集合执行数据操作
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
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

   var dataSrcName = "datasrc23134";
   var csName = "cs_23134";
   var clName = "cl_23134";
   var srcCSName = "datasrcCS_23134";
   var docs = [{ a: 1 }];

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db1, csName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   dbcl.insert( docs );
   db2.getCS( csName ).dropCL( clName );
   db2.getCS( csName ).createCL( clName );
   var expectedResult1 = [];
   var expectedResult2 = [{ "GroupID": -2147483647, "GroupName": "DataSource" }];
   var explainObj = db2.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName }, { CataInfo: "" } );
   var actualResult = explainObj.current().toObj()["CataInfo"];
   assert.notEqual( actualResult, expectedResult1 );
   assert.notEqual( actualResult, expectedResult2 );

   db1.getCS( csName ).getCL( clName ).insert( docs );
   var cursor = db1.getCS( csName ).getCL( clName ).find();
   commCompareResults( cursor, docs );

   var cursor = srccl.find();
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
   db2.close();
}