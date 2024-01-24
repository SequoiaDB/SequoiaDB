/******************************************************************************
 * @Description   : seqDB-22854:修改数据源访问权限 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 修改数据源访问权限并校验，验证 lob 操作
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22854b";
   var csName = "cs_22854b";
   var clName = "cl_22854b";
   var srcCSName = "datasrcCS_22854b";
   var filePath = WORKDIR + "/lob22854b/";
   var fileName = "filelob_22854b";
   var fileSize = 1024 * 1024;

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var datasrcCL = commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   // 集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dataSource = db.getDataSource( dataSrcName );

   // 修改访问权限为 READ
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );
   dataSource.alter( { AccessMode: "READ" } );
   readOnlyAndCheckResult( dbcl, lobID, fileMD5 );

   // 修改访问权限为 NONE
   dataSource.alter( { AccessMode: "NONE" } );
   nonePermAndCheckResult( dbcl, lobID );

   // 修改访问权限为 ALL
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   datasrcCL.deleteLob( lobID );
   dataSource.alter( { AccessMode: "ALL" } );
   allPermAndCheckResult( dbcl, fileMD5, dsMarjorVersion );

   // 修改访问权限为 WRITE
   dataSource.alter( { AccessMode: "WRITE" } );
   writeOnlyAndCheckResult( dbcl, dsMarjorVersion );

   // 集合空间级使用数据源
   commDropCS( db, csName );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   // 修改访问权限为 READ
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   dataSource.alter( { AccessMode: "READ" } );
   readOnlyAndCheckResult( dbcl, lobID, fileMD5 );

   // 修改访问权限为 NONE
   dataSource.alter( { AccessMode: "NONE" } );
   nonePermAndCheckResult( dbcl, lobID );

   // 修改访问权限为 ALL
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   datasrcCL.deleteLob( lobID );
   dataSource.alter( { AccessMode: "ALL" } );
   allPermAndCheckResult( dbcl, fileMD5, dsMarjorVersion );

   // 修改访问权限为 WRITE
   dataSource.alter( { AccessMode: "WRITE" } );
   writeOnlyAndCheckResult( dbcl, dsMarjorVersion );

   deleteTmpFile( filePath );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function readOnlyAndCheckResult ( dbcl, lobID, fileMD5 )
{
   var filePath = WORKDIR + "/lob22854b/";
   var fileName = "filelob_22854b";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.putLob( filePath + fileName );
   } );

   dbcl.getLob( lobID, filePath + "checkputlob22854b", true );
   var actMD5 = File.md5( filePath + "checkputlob22854b" );
   assert.equal( fileMD5, actMD5 );

   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];
      assert.equal( fileSize, listSize );
   }
   rc.close();

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.truncateLob( lobID, size );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.deleteLob( lobID );
   } );
}

function nonePermAndCheckResult ( dbcl, lobID )
{
   var filePath = WORKDIR + "/lob22854b/";
   var fileName = "filelob_22854b";
   var size = 1024 * 20;

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.putLob( filePath + fileName );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.getLob( lobID, filePath + "checkputlob22854b", true );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.listLobs();
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.truncateLob( lobID, size );

   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.deleteLob( lobID );
   } );
}

function writeOnlyAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22854b/";
   var fileName = "filelob_22854b";
   var size = 1024 * 20;

   var lobID = dbcl.putLob( filePath + fileName );
   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.getLob( lobID, filePath + "checkputlob22854b", true );
   } );

   assert.tryThrow( SDB_COORD_DATASOURCE_PERM_DENIED, function()
   {
      dbcl.listLobs();
   } );

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
   }
   dbcl.deleteLob( lobID );
}

function allPermAndCheckResult ( dbcl, fileMD5, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22854b/";
   var fileName = "filelob_22854b";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22854b", true );
   var actMD5 = File.md5( filePath + "checkputlob22854b" );
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
      dbcl.getLob( lobID, filePath + "checktruncateLob22854b", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22854b" );
      assert.equal( expMD5, actMD5 );
   }

   dbcl.deleteLob( lobID );
}