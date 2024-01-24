/******************************************************************************
 * @Description   : seqDB-23125:数据源上已有数据，使用数据源的集合执行数据操作 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23125";
   var csName = "cs_23125";
   var clName = "cl_23125";
   var srcCSName = "datasrcCS_23125";
   var filePath = WORKDIR + "/lob23125/";
   var fileName = "filelob_23125";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var dataSrcCL = commCreateCL( datasrcDB, srcCSName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   // dataSrcCL 插入记录和 lob
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dataSrcCL.putLob( filePath + fileName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dataSrcCL.insert( docs );

   // 源集群端查询并校验记录和 lob
   dbcl.getLob( lobID, filePath + "checkputlob23125", true );
   var actMD5 = File.md5( filePath + "checkputlob23125" );
   assert.equal( fileMD5, actMD5 );
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   // 更新记录并校验
   dbcl.update( { $set: { b: 3 } }, { a: { $gt: 2 } } );
   dbcl.remove( { a: { $et: 2 } } );
   dbcl.insert( { a: 2, b: 2 } );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   // 更新 lob、删除并插入新 lob 
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
      dbcl.getLob( lobID, filePath + "checktruncateLob23125", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob23125" );
      assert.equal( expMD5, actMD5 );
   }

   dbcl.deleteLob( lobID );
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dataSrcCL.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob23125", true );
   var actMD5 = File.md5( filePath + "checkputlob23125" );
   assert.equal( fileMD5, actMD5 );

   deleteTmpFile( filePath );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}