/******************************************************************************
 * @Description   : seqDB-23563:多个子表使用相同数据源，主表上执行LOB操作
 * @Author        : Wu Yan
 * @CreateTime    : 2021.01.08
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc23563";
   var srcCSName = "cs_23563";
   var csName = "cs_23563";
   var clName = "cl_23563";
   var mainCLName = "mainCL_23563";
   var subCLName1 = "subCL_23563a";
   var subCLName2 = "subCL_23563b";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var dsMarjorVersion = getDSMajorVersion( dataSrcName );
   var cs = db.createCS( csName );
   cs.createCL( clName, { DataSource: dataSrcName } );
   cs.createCL( subCLName1 );
   cs.createCL( subCLName2, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var maincl = createCLAndAttachCL( cs, csName, mainCLName, subCLName1, subCLName2, clName );
   putLobAndCheckResult( maincl, dsMarjorVersion );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function createCLAndAttachCL ( cs, csName, mainCLName, subCLName1, subCLName2, clName )
{
   //创建LobShardingKeyFormat为YYYYMMDD主表  
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { date: 1 }, "LobShardingKeyFormat": "YYYYMMDD", ShardingType: "range", IsMainCL: true } );
   var scope = 10;
   var beginBound = new Date().getFullYear() * 10000 + 101;

   mainCL.attachCL( csName + "." + clName, { LowBound: { "date": ( parseInt( beginBound ) ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + scope ) + '' } } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { "date": ( parseInt( beginBound ) + scope ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + 2 * scope ) + '' } } );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      mainCL.attachCL( csName + "." + subCLName2, { LowBound: { "date": ( parseInt( beginBound ) + 2 * scope ) + '' }, UpBound: { "date": ( parseInt( beginBound ) + 3 * scope ) + '' } } );
   } );
   return mainCL;
}

function putLobAndCheckResult ( dbcl, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob23563/";
   var fileName = "filelob_23563";
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
      dbcl.getLob( lobID, filePath + "checktruncateLob23563" );
      var expMD5 = File.md5( filePath + fileName );
      var actMD5 = File.md5( filePath + "checktruncateLob23563" );
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