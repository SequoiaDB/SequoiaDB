/******************************************************************************
 * @Description   : seqDB-22839 :: 创建数据源，设置多个访问权限 (READ|ALL)
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22839b";
   var csName = "cs_22839b";
   var srcCSName = "datasrcCS_22839b";
   var clName = "cl_22839b";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } } );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "READ|ALL" } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );
   //集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   crudAndCheckResult( dbcl );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   crudAndCheckResult( dbcl );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function crudAndCheckResult ( dbcl )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }, { a: 4, b: 4 }, { a: 5, b: 1234.56 }, { a: 6, b: { $decimal: "100.01" } }];
   dbcl.insert( docs );
   dbcl.update( { $set: { c: "testcccc" } }, { a: 2 } );
   dbcl.remove( { a: 4 } );
   dbcl.find( { a: 3 } ).remove().toArray();
   dbcl.find( { a: 6 } ).update( { $inc: { b: 3 } } ).toArray();
   var expRecs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "testcccc" }, { a: 5, b: 1234.56 }, { a: 6, b: { $decimal: "103.01" } }];
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   docs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   dbcl.truncate();
}

function lobAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22839/";
   var fileName = "filelob_22839";
   var fileSize = 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   //putLob and getLob
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22839" );
   var actMD5 = File.md5( filePath + "checkputlob22839" );
   assert.equal( fileMD5, actMD5 );

   //list Lob
   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];
      assert.equal( fileSize, listSize );
   }

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
      dbcl.getLob( lobID, filePath + "checktruncateLob22839" );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22839" );
      assert.equal( expMD5, actMD5 );
   }

   //deleteLob
   dbcl.deleteLob( lobID );
   assert.tryThrow( SDB_FNE, function()
   {
      dbcl.getLob( lobID, filePath + "checkdeletelob22869" );
   } );

   deleteTmpFile( filePath );
}
