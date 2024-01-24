/******************************************************************************
 * @Description   : seqDB-22878:源集群上创建/删除使用数据源的cl
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22878";
   var csName = "cs_22878";
   var clName = "cl_22878";
   var srcCSName = "datasrcCS_22878";
   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd )

   //映射集合全名
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var expRecs = insertBulkData( dbcl, 2000, 0, 2000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   dbcl.remove();
   cs.dropCL( clName );

   //映射集合短名
   var cs = db.createCS( srcCSName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: clName } );
   var expRecs = insertBulkData( dbcl, 1000, 0, 2000 );
   expRecs.sort( sortBy( 'a' ) );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, expRecs );
   cs.dropCL( clName );

   db.dropCS( csName );
   db.dropCS( srcCSName );
   datasrcDB.dropCS( srcCSName );
   db.dropDataSource( dataSrcName );
   datasrcDB.close();
}


