/******************************************************************************
 * @Description   : seqDB-22869:使用数据源创建cs，关联数据源集合执行lob操作
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var srcCSName = "srcCS_22869";
   var srcCLName = "srcCL_22869";
   var csName = "cS_22869";
   var srcDataName = "srcData22869";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, srcCSName );
   clearDataSource( csName, srcDataName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );

   db.createDataSource( srcDataName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( srcDataName );
   //test a：不指定mapping映射cs
   var cs = db.createCS( srcCSName, { DataSource: srcDataName } );
   var dbcl = cs.getCL( srcCLName );
   createCSAndCheckResult( dbcl, dsMarjorVersion );

   //test b：指定mapping映射同名cs
   db.dropCS( srcCSName );
   var cs = db.createCS( srcCSName, { DataSource: srcDataName, Mapping: srcCSName } );
   var dbcl = cs.getCL( srcCLName );
   createCSAndCheckResult( dbcl, dsMarjorVersion );

   //test c：指定mapping映射不同名cs
   var cs = db.createCS( csName, { DataSource: srcDataName, Mapping: srcCSName } );
   var dbcl = cs.getCL( srcCLName );
   createCSAndCheckResult( dbcl, dsMarjorVersion );

   db.dropCS( csName );
   db.dropCS( srcCSName );
   db.dropDataSource( srcDataName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function createCSAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22869/";
   var fileName = "filelob_22869";
   var fileSize = 1024 * 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   //putLob and getLob
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22869" );
   var actMD5 = File.md5( filePath + "checkputlob22869" );
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
   var size = 1024 * 20;
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
      dbcl.getLob( lobID, filePath + "checktruncateLob22869" );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22869" );
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