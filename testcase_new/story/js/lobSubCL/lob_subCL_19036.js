/************************************
*@Description: seqDB-19036 主表未去挂载子表，删除子表
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
   var mainCLName = "mainCL_19036";
   var subCLName1 = "subCL_19036_1";
   var subCLName2 = "subCL_19036_2";
   var filePath = WORKDIR + "/lob19036/";
   var fileName = "file19036";
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
   mainCL.attachCL( csName + "." + subCLName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190831" } } );
   mainCL.attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190901" }, "UpBound": { "date": "20190930" } } );
   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190801" );
   var lobOids2 = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190901" );

   db.getCS( csName ).dropCL( subCLName2 );

   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190901" );
   } );

   checkLobMD5( mainCL, lobOids1, fileMD5 );
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      checkLobMD5( mainCL, lobOids2, fileMD5 );
   } );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName1 );
   commDropCL( db, csName, subCLName2 );
}
