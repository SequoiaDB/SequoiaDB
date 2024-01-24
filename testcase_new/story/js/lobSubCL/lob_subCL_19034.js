/************************************
*@Description: seqDB-19034 主表挂载其他主表去挂载的子表
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
   var mainCLName1 = "mainCL_19034_1";
   var mainCLName2 = "mainCL_19034_2";

   var subCLName = "subCL_19034";
   var filePath = WORKDIR + "/lob19034/";
   var fileName = "file19034"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName1 );
   commDropCL( db, csName, mainCLName2 );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMM", "ShardingType": "range" };
   var mainCL1 = commCreateCL( db, csName, mainCLName1, options, true, false, "create main cl1" );
   var mainCL2 = commCreateCL( db, csName, mainCLName2, options, true, false, "create main cl2" );

   var subCL = commCreateCL( db, csName, subCLName );
   mainCL1.attachCL( csName + "." + subCLName, { "LowBound": { "date": "201908" }, "UpBound": { "date": "201910" } } );

   var lobOids1 = insertLob( subCL, fileFullPath, "YYYYMM", 1, 10, 1, "20190801" );
   var lobOids2 = insertLob( subCL, fileFullPath, "YYYYMM", 1, 10, 1, "20190901" );
   mainCL1.detachCL( csName + "." + subCLName );
   mainCL2.attachCL( csName + "." + subCLName, { "LowBound": { "date": "201909" }, "UpBound": { "date": "201911" } } );

   checkLobMD5( mainCL2, lobOids2, fileMD5 );
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      checkLobMD5( mainCL2, lobOids1, fileMD5 );
   } );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName1 );
   commDropCL( db, csName, mainCLName2 );
   commDropCL( db, csName, subCLName );
}
