/******************************************************************************
 * @Description   : seqDB-22885:使用数据源创建cl，执行lob操作
 * @Author        : Wu Yan
 * @CreateTime    : 2021.01.08
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var srcCSName = "srcCS_22885";
   var srcCLName = "srcCL_22885";
   var csName = "cs_22885";
   var clName = "cl_22885";
   var srcDataName = "srcData22885";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( datasrcDB, csName );
   commDropCS( db, srcCSName );
   clearDataSource( csName, srcDataName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );
   commCreateCS( datasrcDB, csName );
   commCreateCL( datasrcDB, csName, clName );
   db.createDataSource( srcDataName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( srcDataName );

   //test a：指定mapping映射全名,数据源上存在不同名cs、cl
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: srcDataName, Mapping: srcCSName + "." + srcCLName } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //test a：指定mapping映射全名,数据源上存在同名cs、cl  
   cs.dropCL( clName );
   var dbcl = cs.createCL( clName, { DataSource: srcDataName, Mapping: csName + "." + clName } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //test b：指定mapping映射短名cl,数据源上存在同名cs、cl
   var cs1 = db.createCS( srcCSName );
   var dbcl = cs1.createCL( srcCLName, { DataSource: srcDataName, Mapping: srcCLName } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //test b：指定mapping映射短名cl,数据源上存在同名cs、不同名cl    
   var dbcl = cs.createCL( srcCLName, { DataSource: srcDataName, Mapping: clName } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //test c：不指定Mapping映射
   cs.dropCL( clName );
   var dbcl = cs.createCL( clName, { DataSource: srcDataName } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   db.dropCS( csName );
   db.dropCS( srcCSName );
   db.dropDataSource( srcDataName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.dropCS( csName );
   datasrcDB.close();
}

function lobAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22885/";
   var fileName = "filelob_22885";
   var fileSize = 1024 * 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   //putLob and getLob
   var lobID = dbcl.putLob( filePath + fileName );
   println( "---lobId=" + lobID )
   dbcl.getLob( lobID, filePath + "checkputlob22885" );
   var actMD5 = File.md5( filePath + "checkputlob22885" );
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
      dbcl.getLob( lobID, filePath + "checktruncateLob22885" );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22885" );
      assert.equal( expMD5, actMD5 );
   }

   //deleteLob
   dbcl.deleteLob( lobID );
   assert.tryThrow( SDB_FNE, function()
   {
      dbcl.getLob( lobID, filePath + "checkdeletelob22885" );
   } );

   dbcl.truncate();
   deleteTmpFile( filePath );
}


