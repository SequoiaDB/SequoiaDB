/************************************
*@Description: seqDB-19038 反复挂载/去挂载子表，子表已有lob
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
   var mainCLName = "mainCL_19038";
   var subCLName = "subCL_19038";
   var filePath = WORKDIR + "/lob19038/";
   var fileName = "file19038";
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYY", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl1" );

   commCreateCL( db, csName, subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "2000" }, "UpBound": { "date": "2020" } } );
   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYY", 5, 10, 3, "20000101" );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "1990" }, "UpBound": { "date": "2000" } } );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "2020" }, "UpBound": { "date": "2030" } } );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "2010" }, "UpBound": { "date": "2015" } } );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "1990" }, "UpBound": { "date": "2030" } } );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "1990" }, "UpBound": { "date": "2010" } } );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "2010" }, "UpBound": { "date": "2030" } } );

   mainCL.detachCL( csName + "." + subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "2000" }, "UpBound": { "date": "2020" } } );

   var lobOids2 = insertLob( mainCL, fileFullPath, "YYYY", 4, 10, 3, "20100101" );
   checkLobMD5( mainCL, lobOids1, fileMD5 );
   checkLobMD5( mainCL, lobOids2, fileMD5 );

   for( i in lobOids1 )
   {
      mainCL.deleteLob( lobOids1[i] );
      assert.tryThrow( SDB_FNE, function()
      {
         mainCL.getLob( lobOids1[i], WORKDIR + "/checkLob19038_" + i );
      } );
   }

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );
}
