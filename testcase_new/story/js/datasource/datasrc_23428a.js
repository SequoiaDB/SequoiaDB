/******************************************************************************
 * @Description   : seqDB-23428 : 创建数据源，设置ErrorControlLevel忽略错误(不设置ErrorControlLevel)
 * @Author        : Wu Yan
 * @CreateTime    : 2021.03.09
 * @LastEditTime  : 2021.09.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc23428a";
   var csName = "cs_23428a";
   var srcCSName = "datasrcCS_23428a";
   var clName = "cl_23428a";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   var sdbcl = datasrcDB.getCS( srcCSName ).getCL( clName );
   sdbcl.createIndex( "testsrc", { no: -1 } );

   // 创建数据源ErrorControlLevel使用默认值，为low
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB" );
   // 集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   indexOprAndCheckResult( dbcl );

   // 集合空间级使用数据源 
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   indexOprAndCheckResult( dbcl );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function indexOprAndCheckResult ( dbcl )
{
   dbcl.createIndex( "testno", { no: 1 } );
   var cursor = dbcl.listIndexes();
   while( cursor.next() )
   {
      throw new Error( JSON.stringify( "expected list indexes to return 0 row" + cursor.current().toObj() ) );
   }
   cursor.close();

   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      dbcl.getIndex( "testsrc" );
   } );
   dbcl.dropIndex( "testno" );
   dbcl.getDetail();

   var recordNum = 10000;
   var expRecs = insertBulkData( dbcl, recordNum, 0, 40000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   dbcl.truncate();
}
