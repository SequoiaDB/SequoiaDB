/******************************************************************************
 * @Description   : seqDB-22880:源集群上创建cl关联数据源上分区集合
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.04.09
 * @LastEditors   : liuli
 ******************************************************************************/
// 分区表、切分表、主子表，执行lob操作
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22880";
   var csName = "cs_22880";
   var srcCSName = "datasrcCS_22880";
   var clName = "cl_22880";
   var mainCLName = "mainCL_22880";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   // 主子表，3.0以上版本才支持LobShardingKeyFormat，2.8版本不运行主子表部分
   if( dsMarjorVersion > 2 )
   {
      createMainCLAndAttachCL( datasrcDB, srcCSName, mainCLName, clName );
      var dbcs = commCreateCS( db, csName );
      var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + mainCLName } );
      lobOperateMainCL( dbcl );
   }

   // 分区表
   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   lobOperate( dbcl, dsMarjorVersion );

   // 切分表
   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, AutoSplit: true } );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   lobOperate( dbcl, dsMarjorVersion );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function lobOperate ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob22880/";
   var fileName = "filelob_22880";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );
   dbcl.getLob( lobID, filePath + "checkputlob22880", true );
   var actMD5 = File.md5( filePath + "checkputlob22880" );
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
      dbcl.getLob( lobID, filePath + "checktruncateLob22880", true );
      cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob22880" );
      assert.equal( expMD5, actMD5 );
   }

   dbcl.deleteLob( lobID );
   deleteTmpFile( filePath );
}


function lobOperateMainCL ( dbcl )
{
   var filePath = WORKDIR + "/lob22880/";
   var fileName = "filelob_22880";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = insertLob( dbcl, filePath + fileName );
   dbcl.getLob( lobID[0], filePath + "checkputlob22880", true );
   var actMD5 = File.md5( filePath + "checkputlob22880" );
   assert.equal( fileMD5, actMD5 );

   var rc = dbcl.listLobs();
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];
      assert.equal( fileSize, listSize );
   }
   rc.close();

   var cmd = new Cmd();
   dbcl.truncateLob( lobID[0], size );
   dbcl.getLob( lobID[0], filePath + "checktruncateLob22880", true );
   cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
   var expMD5 = File.md5( filePath + fileName );
   var actMD5 = File.md5( filePath + "checktruncateLob22880" );
   assert.equal( expMD5, actMD5 );

   dbcl.deleteLob( lobID[0] );
   deleteTmpFile( filePath );
}