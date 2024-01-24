/******************************************************************************
 * @Description   : seqDB-23822:主子表，写入lob，在主表做truncate，从主表恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.09
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var filePath = WORKDIR + "/lob23822/";
   var fileName = "filelob_23822";
   var fileSize = 1024 * 1024;
   var csName = "cs_23822";
   var clName = "cl_23822";
   var mainCLName = "mainCL_23822";
   var subCLName = "subCL_23822";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commCreateCS( db, csName );

   var shardingFormat = "YYYYMMDD";
   var beginBound = new Date().getFullYear() * 10000 + 101;
   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": shardingFormat, "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options );
   commCreateCL( db, csName, clName, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "AutoSplit": true } );
   commCreateCL( db, csName, subCLName, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "AutoSplit": true } );
   var lowBound = { "date": ( parseInt( beginBound ) ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": lowBound, "UpBound": upBound } );
   var lowBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 10 ) + '' };
   mainCL.attachCL( csName + "." + clName, { "LowBound": lowBound, "UpBound": upBound } );

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOids = insertLob( mainCL, filePath + fileName );

   // 主表执行truncate，从主表恢复
   mainCL.truncate();
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后进行校验
   for( var i = 0; i < lobOids.length; i++ )
   {
      mainCL.getLob( lobOids[i], filePath + "checkputlob23822" + "_" + i, true );
      var actMD5 = File.md5( filePath + "checkputlob23822" + "_" + i );
      assert.equal( fileMD5, actMD5 );
   }
   checkRecycleItem( recycleName );

   deleteTmpFile( filePath );
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function insertLob ( mainCL, filePath )
{
   var year = new Date().getFullYear();
   var month = 1;
   var day = 1;
   var lobOids = [];

   for( var j = 0; j < 10; j++ )
   {
      var timestamp = year + "-" + month + "-" + ( day + j ) + "-00.00.00.000000";
      var lobOid = mainCL.createLobID( timestamp );
      var lobOid = mainCL.putLob( filePath, lobOid );
      lobOids.push( lobOid );
   }
   return lobOids;
}