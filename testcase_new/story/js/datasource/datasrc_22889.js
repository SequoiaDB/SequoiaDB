/******************************************************************************
 * @Description   : seqDB-22889:获取使用数据源的cs/cl快照 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22889";
   var csName = "cs_22889";
   var clName = "cl_22889";
   var srcCSName = "datasrcCS_22889";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 集合空间级别映射
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName } );
   commCompareResults( cursor, [] );

   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   commCompareResults( cursor, [] );

   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var expectResult = [];
   commCompareResults( cursor, expectResult );

   // 集合级别映射
   commDropCS( db, csName );
   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: csName + "." + clName } );
   commCompareResults( cursor, [] );

   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } );
   commCompareResults( cursor, [] );

   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName }, { Mapping: "" } );
   var expectResult = [{ Mapping: srcCSName + "." + clName }]
   commCompareResults( cursor, expectResult );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}