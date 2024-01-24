/******************************************************************************
 * @Description   : seqDB-22888:使用数据源的集合执行rename操作
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22888";
   var csName = "cs_22888";
   var clName1 = "cl_22888a";
   var clName2 = "cl_22888b";

   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, csName, clName1 );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = commCreateCS( db, csName );
   dbcs.createCL( clName1, { DataSource: dataSrcName, Mapping: csName + "." + clName1 } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   dbcs.renameCL( clName1, clName2 );
   var dbcl = dbcs.getCL( clName2 );
   operationAndCheckResult( dbcl, dsMarjorVersion );

   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function operationAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22888/";
   var fileName = "filelob_22888";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   dbcl.insert( docs );
   dbcl.update( { $set: { b: 3 } }, { a: { $gt: 2 } } );
   dbcl.remove( { a: { $et: 2 } } );
   dbcl.insert( { a: 2, b: 2 } );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22888", true );
   var actMD5 = File.md5( filePath + "checkputlob22888" );
   assert.equal( fileMD5, actMD5 );

   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];
      assert.equal( fileSize, listSize );
   }
   rc.close();

   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
      {
         dbcl.truncateLob( lobID, size );
      } );
   }
   else
   {
      var cmd = new Cmd();
      dbcl.truncateLob( lobID, size );
      dbcl.getLob( lobID, filePath + "checktruncateLob22888", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22888" );
      assert.equal( expMD5, actMD5 );
   }

   dbcl.deleteLob( lobID );
   deleteTmpFile( filePath );
}