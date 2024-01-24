/******************************************************************************
 * @Description   : seqDB-23133:使用数据源的集合为子表，主表上执行LOB操作
 * @Author        : Wu Yan
 * @CreateTime    : 2021.01.08
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc23133";
   var csName = "cs_23133";
   var clName = "cl_23133";
   var srcCSName = "cs_23133";
   var mainCLName = "mainCL_23133";
   var subCLName = "subCL_23133";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   var cs = db.createCS( csName );
   cs.createCL( subCLName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var maincl = createCLAndAttachCL( cs, csName, mainCLName, subCLName, clName );
   putLobAndCheckResult( maincl, dsMarjorVersion );

   db.dropCS( csName );
   db.dropDataSource( dataSrcName );
   datasrcDB.dropCS( srcCSName );
}

function createCLAndAttachCL ( cs, csName, mainCLName, subCLName, subCLName1 )
{
   //创建LobShardingKeyFormat为YYYYMMDD主表  
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { date: 1 }, "LobShardingKeyFormat": "YYYYMMDD", ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   var scope = 10;
   var beginBound = new Date().getFullYear() * 10000 + 101;

   mainCL.attachCL( csName + "." + subCLName, { LowBound: { "date": ( parseInt( beginBound ) ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + scope ) + '' } } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "date": ( parseInt( beginBound ) + scope ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + 2 * scope ) + '' } } );
   println( "-w---" )
   return mainCL;
}

function putLobAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob23133/";
   var fileName = "filelob_23133";
   var fileSize = 1024 * 1024;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );

   //putLobs
   var nameArr = dbcl.toString().split( "." );
   var mainCLFullName = nameArr[1] + "." + nameArr[2];
   var lobOids = insertLob( dbcl, filePath + fileName, "YYYYMMDD" );
   //get lobs and check md5
   checkLobMD5( dbcl, lobOids, fileMD5 );
   //list lobs
   var lobNum = 20;
   listLobs( dbcl, fileSize, lobNum );

   //truncate lob
   var size = 1024 * 20;
   var lobID = lobOids[0];
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
      dbcl.getLob( lobID, filePath + "checktruncateLob23133" );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob23133" );
      assert.equal( expMD5, actMD5 );
   }

   //deleteLob
   deleteLob( dbcl, lobOids );

   deleteTmpFile( filePath );
}

function listLobs ( dbcl, fileSize, lobNum )
{
   //list Lob
   var rc = dbcl.listLobs();
   var listlobNum = 0;
   while( rc.next() )
   {
      var obj = rc.current().toObj();
      var listSize = obj["Size"];
      assert.equal( fileSize, listSize );
      listlobNum++
   }
   rc.close();
   assert.equal( lobNum, listlobNum );
}