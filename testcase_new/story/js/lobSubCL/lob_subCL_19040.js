/************************************
*@Description: seqDB-19040 创建不同lobpagesize的主表，进行lob操作
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

   var csName = "sub_cs_19040";
   var mainName1 = "main_19040_1";
   var mainName2 = "main_19040_2";
   var mainName3 = "main_19040_3";
   var subCLName1 = "sub_19040_1"
   var subCLName2 = "sub_19040_2"
   var subCLName3 = "sub_19040_3"

   var filePath = WORKDIR + "/lob19040/";
   var fileName = "file19040"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCS( db, csName );
   commDropCS( db, mainName1 );
   commDropCS( db, mainName2 );
   commDropCS( db, mainName3 );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   commCreateCS( db, mainName1, false, "", { LobPageSize: 4096 } );
   var mainCL1 = commCreateCL( db, mainName1, mainName1, options, true, false, "create main cl1" );

   commCreateCS( db, mainName2, false, "", { LobPageSize: 16384 } );
   var mainCL2 = commCreateCL( db, mainName2, mainName2, options, true, false, "create main cl2" );

   commCreateCS( db, mainName3, false, "", { LobPageSize: 262144 } );
   var mainCL3 = commCreateCL( db, mainName3, mainName3, options, true, false, "create main cl3" );

   commCreateCL( db, csName, subCLName1 );
   commCreateCL( db, csName, subCLName2 );
   commCreateCL( db, csName, subCLName3 );

   mainCL1.attachCL( csName + "." + subCLName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190805" } } );
   mainCL2.attachCL( csName + "." + subCLName2, { "LowBound": { "date": "20190805" }, "UpBound": { "date": "20190810" } } );
   mainCL3.attachCL( csName + "." + subCLName3, { "LowBound": { "date": "20190810" }, "UpBound": { "date": "20190815" } } );

   var lobOids1 = insertLob( mainCL1, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190801" );
   var lobOids2 = insertLob( mainCL2, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190805" );
   var lobOids3 = insertLob( mainCL3, fileFullPath, "YYYYMMDD", 5, 10, 1, "20190810" );

   checkLobMD5( mainCL1, lobOids1, fileMD5 );
   checkLobMD5( mainCL2, lobOids2, fileMD5 );
   checkLobMD5( mainCL3, lobOids3, fileMD5 );

   deleteLob( mainCL1, lobOids1 );
   deleteLob( mainCL2, lobOids2 );
   deleteLob( mainCL3, lobOids3 );

   deleteTmpFile( filePath );
   commDropCS( db, csName );
   commDropCS( db, mainName1 );
   commDropCS( db, mainName2 );
   commDropCS( db, mainName3 );
}
