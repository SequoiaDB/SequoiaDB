/******************************************************************************
 * @Description   : seqDB-23135:创建使用数据源的集合空间，连接不同coord删除cs再创建本地同名cs 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
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

   var dataSrcName = "datasrc23135";
   var csName = "cs_23135";
   var clName = "cl_23135";
   var srcCSName = "datasrcCS_23135";
   var docs = [{ a: 1 }];

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );

   dbcl.insert( docs );
   db2.dropCS( csName );
   db2.createCS( csName );

   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db2.getCS( csName ).getCL( clName ).insert( docs );
   } );

   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcl.insert( docs );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
   db2.close();
}