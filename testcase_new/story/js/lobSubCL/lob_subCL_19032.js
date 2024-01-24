/************************************
*@Description: seqDB-19032 主表挂载单子表，设置LowBound和UpBound类型为min和max
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
   var mainCLName = "mainCL_19032";
   var subCLName = "subCL_19032";
   var filePath = WORKDIR + "/lob19032/";
   var fileName = "file19032"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": { "$minKey": 1 } }, "UpBound": { "date": { "$maxKey": 1 } } } );
   var lobOids = insertLob( mainCL, fileFullPath );
   checkLobMD5( mainCL, lobOids, fileMD5 );
   deleteLob( mainCL, lobOids );

   deleteTmpFile( filePath );
   cleanMainCL( db, csName, mainCLName );
}
