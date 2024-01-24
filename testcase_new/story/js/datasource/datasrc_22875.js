/******************************************************************************
 * @Description   : seqDB-22875:源集群上cs执行修改/rename操作
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22875";
   var csName1 = "cs_22875_a";
   var csName2 = "cs_22875_b";
   var clName = "cl_22875";
   var srcCSName = "datasrcCS_22875";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName2 );
   clearDataSource( csName1, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db.createCS( csName1, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   // 修改源集群CS名后，进行数据操作并校验
   db.renameCS( csName1, csName2 );
   var dbcl = db.getCS( csName2 ).getCL( clName )
   operationAndCheckResult( dbcl, dsMarjorVersion );

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName2 );
   clearDataSource( csName1, dataSrcName );
   datasrcDB.close();
}

function operationAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22875/";
   var fileName = "filelob_22875";
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
   dbcl.getLob( lobID, filePath + "checkputlob22875", true );
   var actMD5 = File.md5( filePath + "checkputlob22875" );
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
      dbcl.getLob( lobID, filePath + "checktruncateLob22875", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22875" );
      assert.equal( expMD5, actMD5 );
   }

   dbcl.deleteLob( lobID );
   deleteTmpFile( filePath );
}