/******************************************************************************
 * @Description   : seqDB-23530:创建数据源设置访问权限，主子表使用数据源执行Lob操作
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 设置数据源 AccessMode:"WRITE" 权限
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23530b";
   var csName = "cs_23530b";
   var clName = "cl_23530b";
   var srcCSName = "datasrcCS_23530b";
   var mainCLName = "mainCL_23530b";
   var subCLName = "subCL_23530b";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "AutoSplit": true } );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "WRITE" } );
   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   var shardingFormat = "YYYYMMDD";
   var beginBound = new Date().getFullYear() * 10000 + 101;
   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": shardingFormat, "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options );
   commCreateCL( db, csName, subCLName, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "AutoSplit": true } );
   var lowBound = { "date": ( parseInt( beginBound ) ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": lowBound, "UpBound": upBound } );
   var lowBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 10 ) + '' };
   mainCL.attachCL( csName + "." + clName, { "LowBound": lowBound, "UpBound": upBound } );

   // listLob 覆盖两个子表，其他操作在本地子表
   lobOperateLocalCL( mainCL );
   // listLob 覆盖两个子表，其他操作在数据源子表
   lobOperateDataSrcCL( mainCL, dsMarjorVersion );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function lobOperateDataSrcCL ( mainCL, dsMarjorVersion )
{
   var filePath = WORKDIR + "/lob23530b/";
   var fileName = "filelob_23530b";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = insertLob( mainCL, filePath + fileName, 1 );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.getLob( lobID, filePath + "checkputlob23530b", true );
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.listLobs();
   } );

   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
      {
         mainCL.truncateLob( lobID, size );
      } );
   }
   else
   {
      // 这里是 WRITE 权限，不能进行 getLob 操作，truncateLob 之后不进行校验
      mainCL.truncateLob( lobID, size );
   }

   mainCL.deleteLob( lobID );
   deleteTmpFile( filePath );
}

function lobOperateLocalCL ( mainCL )
{
   var filePath = WORKDIR + "/lob23530b/";
   var fileName = "filelob_23530b";
   var fileSize = 1024 * 1024;
   var size = 1024 * 20;

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = insertLob( mainCL, filePath + fileName, 0 );
   mainCL.getLob( lobID, filePath + "checkputlob23530b", true );
   var actMD5 = File.md5( filePath + "checkputlob23530b" );
   assert.equal( fileMD5, actMD5 );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.listLobs();
   } );

   var cmd = new Cmd();
   mainCL.truncateLob( lobID, size );
   mainCL.getLob( lobID, filePath + "checktruncateLob23530b", true );
   cmd.run( "truncate -s " + ( size ) + " " + filePath + fileName );
   var expMD5 = File.md5( filePath + fileName );
   var actMD5 = File.md5( filePath + "checktruncateLob23530b" );
   assert.equal( expMD5, actMD5 );

   mainCL.deleteLob( lobID );
   deleteTmpFile( filePath );
}

function insertLob ( mainCL, filePath, subCLRange )
{
   var year = new Date().getFullYear();
   var month = 1;
   var day = 1;

   for( var j = 0; j < 10; j++ )
   {
      var timestamp = year + "-" + month + "-" + ( day + subCLRange * 5 ) + "-00.00.00.000000";
      var lobOid = mainCL.createLobID( timestamp );
      var lobOid = mainCL.putLob( filePath, lobOid );
   }
   return lobOid;
}