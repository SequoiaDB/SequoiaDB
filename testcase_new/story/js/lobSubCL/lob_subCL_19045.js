/************************************
*@Description: seqDB-19045 hash子表已有lob，执行切分
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
   var mainCLName = "mainCL_19045";
   var subCLName = "subCL_19045";
   var sourceGroup = groups[0][0].GroupName;
   var targetGroup = groups[1][0].GroupName;
   var filePath = WORKDIR + "/lob19045/";
   var fileName = "file19045";
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   var clOptions = { "ShardingKey": { "a": 1 }, ShardingType: "hash", Group: sourceGroup };
   var subCL = commCreateCL( db, csName, subCLName, clOptions, true, false, "create sub cl" );

   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190831" } } );
   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190801" );

   subCL.split( sourceGroup, targetGroup, 50 );

   var lobOids2 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190810" );
   checkLobMD5( mainCL, lobOids1, fileMD5 );
   checkLobMD5( mainCL, lobOids2, fileMD5 );
   deleteLob( mainCL, lobOids1 );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );
}
