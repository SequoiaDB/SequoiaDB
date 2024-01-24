/************************************
*@Description: seqDB-19030 主表挂载range子表
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
   var mainCLName = "mainCL_19030";
   var subCLName1 = "subCL_19030_1";
   var subCLName2 = "subCL_19030_2";
   var targetGroup = groups[0][0].GroupName;
   var sourceGroup = groups[1][0].GroupName;
   var filePath = WORKDIR + "/lob19030/";
   var fileName = "file19030";
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName1 );
   var clOptions = { "ShardingKey": { "a": 1 }, ShardingType: "range", Group: targetGroup };
   var subcl2 = commCreateCL( db, csName, subCLName2, clOptions, true, false, "create sub cl2" );
   subcl2.insert( { a: 1 } );
   subcl2.insert( { a: 2 } );
   subcl2.split( targetGroup, sourceGroup, 50 );
   mainCL.attachCL( csName + "." + subCLName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190805" } } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      mainCL.attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190805" }, "UpBound": { "date": "20190810" } } );
   } );
   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190801" );
   checkLobMD5( mainCL, lobOids, fileMD5 );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );
}
