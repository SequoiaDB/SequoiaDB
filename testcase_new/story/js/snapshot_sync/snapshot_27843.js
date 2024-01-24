/******************************************************************************
 * @Description   : seqDB-27843:主表attachCL/detachCL后getDetail 
 * @Author        : liuli
 * @CreateTime    : 2022.09.27
 * @LastEditTime  : 2023.04.25
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var filePath = WORKDIR + "/lob27843/";
   var fileName = "filelob_27843";
   var fileSize = 1024 * 50;
   var size = 1024 * 32;
   var csName = "cs_27843";
   var mainCLName = "mainCL_27843";
   var subCLName1 = "subCL_27843_1";
   var subCLName2 = "subCL_27843_2";

   var groupNames = commGetDataGroupNames( db );
   commDropCS( db, csName );

   var shardingFormat = "YYYYMMDD";
   var beginBound = new Date().getFullYear() * 10000 + 101;
   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": shardingFormat, "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options );
   var subcl = commCreateCL( db, csName, subCLName1, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "Group": groupNames[0] } );
   commCreateCL( db, csName, subCLName2, { "ShardingKey": { "date": 1 }, "ShardingType": "hash", "Group": groupNames[0] } );
   var lowBound = { "date": ( parseInt( beginBound ) ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   mainCL.attachCL( csName + "." + subCLName1, { "LowBound": lowBound, "UpBound": upBound } );
   var lowBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 10 ) + '' };
   mainCL.attachCL( csName + "." + subCLName2, { "LowBound": lowBound, "UpBound": upBound } );

   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );

   // 执行putLob操作，每个子表5个lob
   var year = new Date().getFullYear();
   var month = 1;
   var day = 1;
   var lobOids = [];
   var lobs = 10;
   for( var j = 0; j < lobs; j++ )
   {
      var timestamp = year + "-" + month + "-" + ( day + j ) + "-00.00.00.000000";
      var lobOid = mainCL.createLobID( timestamp );
      lobOids.push( lobOid );
      mainCL.putLob( filePath + fileName, lobOid );
   }

   // 执行getLob操作
   var getLobFileName = "getlob27849_";
   for( var i in lobOids )
   {
      mainCL.getLob( lobOids[i], filePath + getLobFileName + i, true );
   }

   // 执行truncateLob操作
   mainCL.truncateLob( lobOids[0], size );

   // 执行deleteLob操作
   mainCL.deleteLob( lobOids[1] );

   // 获取主表getDetail信息
   var cursor = mainCL.getDetail();
   var mainDetail = getSnapshotLobStatToCL( cursor, true );

   var cursor = subcl.getDetail();
   var subDetail = getSnapshotLobStatToCL( cursor, true );

   // 卸载一个子表
   mainCL.detachCL( csName + "." + subCLName2 );

   // 校验主表getDetail信息
   var cursor = mainCL.getDetail();
   if( commIsArmArchitecture() == false )
   {
      checkSnapshotToCL( cursor, subDetail, true, lobs );
   }
   // 重新挂载子表
   mainCL.attachCL( csName + "." + subCLName2, { "LowBound": lowBound, "UpBound": upBound } );

   // 校验主表getDetail信息
   var cursor = mainCL.getDetail();
   if( commIsArmArchitecture() == false )
   {
      checkSnapshotToCL( cursor, mainDetail, true, lobs );
   }
   // 执行listLobs后校验getDetail信息
   mainDetail[0]["TotalLobList"] += 1 * 2;
   mainDetail[0]["TotalLobRead"] += lobs - 1;
   mainDetail[0]["TotalLobAddressing"] += lobs - 1;
   mainCL.listLobs().toArray();
   var cursor = mainCL.getDetail();
   if( commIsArmArchitecture() == false )
   {
      checkSnapshotToCL( cursor, mainDetail, true, lobs );
   }
   commDropCS( db, csName );
   deleteTmpFile( filePath + fileName )
}