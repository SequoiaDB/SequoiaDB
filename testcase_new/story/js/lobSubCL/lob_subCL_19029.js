/************************************
*@Description: seqDB-19029 主表挂载子表
*@author:      luweikang
*@createDate:  2019.8.12
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }

   var csName = COMMCSNAME;
   var mainCLName = "mainCL_19029";
   var subCLName1 = "subCL_19029_1";
   var subCLName2 = "subCL_19029_2";
   var targetGroup = groups[0][0].GroupName;
   var sourceGroup = groups[1][0].GroupName;
   var filePath = WORKDIR + "/lob19029/";
   var fileName = "file19029";
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName1 );
   var clOptions = { "ShardingKey": { "a": 1 }, ShardingType: "hash", Group: targetGroup };
   var subcl2 = commCreateCL( db, csName, subCLName2, clOptions, true, false, "create sub cl2" );
   subcl2.split( targetGroup, sourceGroup, 50 );

   mainCL.attachCL( csName + "." + subCLName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190805" } } );
   mainCL.attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190805" }, "UpBound": { "date": "20190810" } } );
   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 2, "20190801" );
   checkLobMD5( mainCL, lobOids, fileMD5 );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
}
