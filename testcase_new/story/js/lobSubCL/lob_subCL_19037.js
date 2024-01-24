/************************************
*@Description: seqDB-19037 主表挂载多子表，分别设置LowBound为min，UpBound为max
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

   var csName = COMMCSNAME;
   var mainCLName = "mainCL_19037";
   var subCLName1 = "subCL_19037_1";
   var subCLName2 = "subCL_19037_2";
   var filePath = WORKDIR + "/lob19037/";
   var fileName = "file19037"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName1 );
   commCreateCL( db, csName, subCLName2 );
   mainCL.attachCL( csName + "." + subCLName1, { "LowBound": { "date": { "$minKey": 1 } }, "UpBound": { "date": "20190601" } } );
   mainCL.attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190601" }, "UpBound": { "date": { "$maxKey": 1 } } } );

   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190501" );
   var lobOids2 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190701" );
   checkLobMD5( mainCL, lobOids1, fileMD5 );
   checkLobMD5( mainCL, lobOids2, fileMD5 );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
}
