/******************************************************************************
 * @Description   : seqDB-23293:创建使用数据源的集合，连接不同coord删除cs再创建本地同名cs 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var clName1 = "cl_23293";
   var clName2 = "datasrcCL_23293";
   // 不同名 cl 映射
   differentClName( clName1, clName2 );
   // 同名 cl 映射
   differentClName( clName1, clName1 );
}

function differentClName ( srcCLName, dbclName )
{
   var coordArr = getCoordUrl( db );
   if( coordArr.length < 2 )
   {
      return;
   }
   var db1 = new Sdb( coordArr[0] );
   var db2 = new Sdb( coordArr[1] );

   var dataSrcName = "datasrc23293";
   var csName = "cs_23293";
   var srcCSName = "datasrcCS_23293";
   var docs = [{ a: 1 }];

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( dbclName, { DataSource: dataSrcName, Mapping: srcCSName + "." + srcCLName } );

   dbcl.insert( docs );
   db2.dropCS( csName );
   db2.createCS( csName );

   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db2.getCS( csName ).getCL( dbclName ).insert( docs );
   } );

   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcl.insert( docs );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   db1.close();
   db2.close();
}