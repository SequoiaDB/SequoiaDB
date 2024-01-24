/******************************************************************************
 * @Description   : seqDB-22838:创建数据源，设置访问权限为NONE
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22838d";
   var csName = "cs_22838d";
   var srcCSName = "datasrcCS_22838d";
   var clName = "cl_22838d";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var scs = datasrcDB.createCS( srcCSName );
   var sdbcl = scs.createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2" }];
   sdbcl.insert( docs );

   var filePath = WORKDIR + "/lob22838b/";
   var fileName = "filelob_22838b";
   var fileSize = 1024 * 2;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = sdbcl.putLob( filePath + fileName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "NONE" } );
   //集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   crudAndCheckResult( dbcl, sdbcl, docs );
   lobAndCheckResult( dbcl, sdbcl, lobID, filePath, fileName );

   //集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   crudAndCheckResult( dbcl, sdbcl, docs );
   lobAndCheckResult( dbcl, sdbcl, lobID, filePath, fileName );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   deleteTmpFile( filePath );
}

function crudAndCheckResult ( dbcl, sdbcl, docs )
{
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.insert( { a: 3 } );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.find().toArray();
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.update( { $set: { c: "testcccc" } } );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.remove();
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   } );

   //源集群端查询数据操作结果
   var cursor = sdbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
}

function lobAndCheckResult ( dbcl, sdbcl, lobID, filePath, fileName )
{
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.putLob( filePath + fileName );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      println( "---lobID=" + lobID )
      dbcl.getLob( lobID, filePath + "checkputlob22838d" );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      var size = 1;
      dbcl.truncateLob( lobID, size );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.deleteLob( lobID );
   } );

   //源集群端查询数据操作结果
   var rc = sdbcl.listLobs();
   var lobnum = 0;
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      lobnum++;
   }
   rc.close();
   assert.equal( 1, lobnum );

}
