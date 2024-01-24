/******************************************************************************
 * @Description   : seqDB-22856b:修改数据源ErrorFilterMask属性 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// ErrorFilterMask 属性对 lob 操作不生效，校验修改数据源 ErrorFilterMask 属性不影响 lob 操作
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22856b";
   var csName = "cs_22856b";
   var clName = "cl_22856b";
   var srcCSName = "datasrcCS_22856b";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   //集合级使用数据源
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dataSource = db.getDataSource( dataSrcName );

   dataSource.alter( { ErrorFilterMask: "READ" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "READ|WRITE" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "WRITE" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "NONE" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "ALL" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   //集合空间级使用数据源
   commDropCS( db, csName );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   dataSource.alter( { ErrorFilterMask: "READ" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "READ|WRITE" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "WRITE" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "ALL" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   dataSource.alter( { ErrorFilterMask: "NONE" } );
   lobAndCheckResult( dbcl, dsMarjorVersion );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function lobAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22856b/";
   var fileName = "filelob_22856b";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22856b", true );
   var actMD5 = File.md5( filePath + "checkputlob22856b" );
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
      dbcl.getLob( lobID, filePath + "checktruncateLob22856b", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22856b" );
      assert.equal( expMD5, actMD5 );
   }

   dbcl.deleteLob( lobID );
   deleteTmpFile( filePath );
}
