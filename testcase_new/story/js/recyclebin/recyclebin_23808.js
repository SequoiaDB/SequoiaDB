/******************************************************************************
 * @Description   : seqDB-23808:主子表，子表是hash分区表，写入lob，drop主表，从主表恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.13
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var filePath = WORKDIR + "/lob23808/";
   var fileName = "filelob_23808";
   var fileSize = 1024 * 1024;
   var csName = "cs_23808";
   var mainCLName = "mainCL_23808";
   var subCLName = "subCL_23808";
   var subCLNames = [];
   var shardRanges = [];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );

   // 创建主表和多个子表
   var shardingFormat = "YYYYMMDD";
   var beginBound = new Date().getFullYear() * 10000 + 101;
   var options = { IsMainCL: true, ShardingKey: { "date": 1 }, LobShardingKeyFormat: shardingFormat, ShardingType: "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options );
   for( var i = 0; i < 6; i++ )
   {
      commCreateCL( db, csName, subCLName + "_" + i, { ShardingKey: { "date": 1 }, ShardingType: "hash", AutoSplit: true } );
      var lowBound = { "date": ( parseInt( beginBound ) + i * 5 ) + '' };
      var upBound = { "date": ( parseInt( beginBound ) + ( i + 1 ) * 5 ) + '' };
      mainCL.attachCL( csName + "." + subCLName + "_" + i, { LowBound: lowBound, UpBound: upBound } );
      subCLNames.push( csName + "." + subCLName + "_" + i );
      shardRanges.push( ( parseInt( beginBound ) + i * 5 ) + '' );
   }
   shardRanges.push( ( parseInt( beginBound ) + 6 * 5 ) + '' );

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOids = insertLob( mainCL, filePath + fileName, 30 );

   // 删除主表后进行恢复
   dbcs.dropCL( mainCLName );
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后进行校验
   for( var i = 0; i < lobOids.length; i++ )
   {
      mainCL.getLob( lobOids[i], filePath + "checkputlob23808" + "_" + i, true );
      var actMD5 = File.md5( filePath + "checkputlob23808" + "_" + i );
      assert.equal( fileMD5, actMD5 );
   }
   checkRecycleItem( recycleName );

   // 校验主子表关联关系
   checkSubCL( csName, mainCLName, subCLNames, shardRanges )

   deleteTmpFile( filePath );
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function insertLob ( mainCL, filePath, lobNum )
{
   var year = new Date().getFullYear();
   var month = 1;
   var day = 1;
   var lobOids = [];

   for( var j = 0; j < lobNum; j++ )
   {
      var timestamp = year + "-" + month + "-" + ( day + j ) + "-00.00.00.000000";
      var lobOid = mainCL.createLobID( timestamp );
      var lobOid = mainCL.putLob( filePath, lobOid );
      lobOids.push( lobOid );
   }
   return lobOids;
}