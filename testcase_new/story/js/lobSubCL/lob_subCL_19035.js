/************************************
*@Description: seqDB-19035:主表去挂载子表
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
   var mainCLName = "mainCL_19035";

   var subCLName = "subCL_19035";
   var filePath = WORKDIR + "/lob19035/";
   var fileName = "file19035"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMM", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl1" );

   var subCL = commCreateCL( db, csName, subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": "201908" }, "UpBound": { "date": "201910" } } );
   var lobOids1 = insertLob( mainCL, fileFullPath, "YYYYMM", 1, 10, 1, "20190801" );

   //去挂载子表，对子表进行lob读增删
   mainCL.detachCL( csName + "." + subCLName );
   checkLobMD5( subCL, lobOids1, fileMD5 );
   var lobOids2 = insertLob( subCL, fileFullPath, "YYYYMM", 1, 10, 1, "20190901" );
   checkLobMD5( subCL, lobOids2, fileMD5 );
   deleteLob( subCL, lobOids1 );
   deleteLob( subCL, lobOids2 );

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );
}
