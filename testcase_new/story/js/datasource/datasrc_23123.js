/******************************************************************************
 * @Description   : seqDB-23123:使用数据源的集合执行truncate操作 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23123";
   var csName = "cs_23123";
   var clName = "cl_23123";
   var srcCSName = "datasrcCS_23123";
   var filePath = WORKDIR + "/lob23123/";
   var fileName = "filelob_23123";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   // 数据源端 cl 插入记录和 lob
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dbcl.insert( docs );

   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
      {
         dbcl.truncateLob( lobID, size );
      } );
   }
   else
   {
      // 源集群端查询并校验记录和 lob
      var cmd = new Cmd();
      dbcl.truncateLob( lobID, size );
      dbcl.getLob( lobID, filePath + "checktruncateLob23123", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob23123" );
      assert.equal( expMD5, actMD5 );
   }
   dbcl.truncate();
   var explainObj = dbcl.find();
   commCompareResults( explainObj, [] );

   deleteTmpFile( filePath );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
