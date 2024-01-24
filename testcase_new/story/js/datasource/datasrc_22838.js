/******************************************************************************
 * @Description   : seqDB-22838:创建数据源，设置访问权限为READ
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/

testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22838";
   var csName = "cs_22838";
   var srcCSName = "datasrcCS_22838";
   var clName = "cl_22838";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var scs = datasrcDB.createCS( srcCSName );
   //数据源集合插入记录和lob
   var sdbcl = scs.createCL( clName );
   var filePath = WORKDIR + "/lob22838/";
   var fileName = "filelob_22838";
   var fileSize = 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = sdbcl.putLob( filePath + fileName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2" }];
   sdbcl.insert( docs );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "READ" } );

   //集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   crudAndCheckResult( dbcl, docs );
   lobAndCheckResult( dbcl, lobID, filePath, fileName, fileMD5 );

   //集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   crudAndCheckResult( dbcl, docs );
   lobAndCheckResult( dbcl, lobID, filePath, fileName, fileMD5 );

   deleteTmpFile( filePath );
   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function crudAndCheckResult ( dbcl, docs )
{
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.insert( { a: 3 } );
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

   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   docs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, docs );
}

function lobAndCheckResult ( dbcl, lobID, filePath, fileName, fileMD5 )
{
   //putLob 
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.putLob( filePath + fileName );
   } );

   //getLob  
   dbcl.getLob( lobID, filePath + "checkputlob22838" );
   var actMD5 = File.md5( filePath + "checkputlob22838" );
   assert.equal( fileMD5, actMD5 );

   //list Lob
   var rc = dbcl.listLobs();
   var lobNum = 0;
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      lobNum++;
   }
   rc.close();
   assert.equal( 1, lobNum );

   //truncateLob
   var size = 0;
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.truncateLob( lobID, size );
   } );

   //remove lob
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.deleteLob( lobID );
   } );

   deleteTmpFile( filePath + "checkputlob22838" );
}

