/******************************************************************************
 * @Description   : seqDB-22838:创建数据源，设置访问权限为WRITE
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22838b";
   var csName = "cs_22838b";
   var srcCSName = "datasrcCS_22838b";
   var clName = "cl_22838b";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var scs = datasrcDB.createCS( srcCSName );
   var sdbcl = scs.createCL( clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "WRITE" } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );
   //集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   crudAndCheckResult( dbcl, sdbcl );
   lobAndCheckResult( dbcl, sdbcl, dsMarjorVersion );

   //集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   crudAndCheckResult( dbcl, sdbcl );
   lobAndCheckResult( dbcl, sdbcl, dsMarjorVersion );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function crudAndCheckResult ( dbcl, sdbcl, dsMarjorVersion )
{
   dbcl.truncate();
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }, { a: 4, b: { b: 4 } }, { a: 5, b: { b: 5 } }];
   dbcl.insert( docs );
   dbcl.update( { $set: { c: "testcccc" } }, { a: 2 } );
   dbcl.remove( { a: 3 } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.find().toArray();
   } );
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.find( { a: 4 } ).remove().toArray();
   } );
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.find( { a: 5 } ).update( { $inc: { "b.b": 3 } } ).toArray();
   } );

   //源集群端查询数据操作结果
   var cursor = sdbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "testcccc" }, { a: 4, b: { b: 4 } }, { a: 5, b: { b: 5 } }];
   commCompareResults( cursor, expRecs );
}

function lobAndCheckResult ( dbcl, sdbcl, dsMarjorVersion )
{
   dbcl.truncate();
   var filePath = WORKDIR + "/lob22838b/";
   var fileName = "filelob_22838b";
   var fileSize = 1024 * 2;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   //putLob 
   var lobID = dbcl.putLob( filePath + fileName );
   //getLob
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.getLob( lobID, filePath + "checkputlob22838b" );
   } );

   //listLob
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.listLobs().toArray();
   } );

   //truncateLob    
   var size = 1;
   //只有3.0以上版本才支持truncateLob操作，2.8版本不支持报错-315（SDB_OPERATION_INCOMPATIBLE）
   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
      {
         dbcl.truncateLob( lobID, size );
      } );
   }
   else
   {
      dbcl.truncateLob( lobID, size );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      //没有读权限，在数据源集合上获取lob校验
      sdbcl.getLob( lobID, filePath + "checktruncateLob22838b" );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22838b" );
      assert.equal( expMD5, actMD5 );
   }

   //deleteLob
   dbcl.deleteLob( lobID );

   //源集群端查询数据操作结果
   var rc = sdbcl.listLobs();
   var lobnum = 0;
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      lobnum++;
   }
   rc.close();
   assert.equal( 0, lobnum );

   deleteTmpFile( filePath );
}
