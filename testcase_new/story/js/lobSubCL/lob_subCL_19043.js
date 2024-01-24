/************************************
*@Description: seqDB-19043 主表对lob执行truncate
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
   var mainCLName = "cl19043_main";
   var subCLName = "cl19043_sub";
   var filePath = WORKDIR + "/lob19043/";
   var fileName1 = "file19043_1";
   var fileName2 = "file19043_2";
   var fileName3 = "file19043_3";
   var fileSize = 1024 * 1024;
   deleteTmpFile( filePath );
   var file1MD5 = makeTmpFile( filePath, fileName1, fileSize );
   var file2MD5 = makeTmpFile( filePath, fileName2, fileSize );
   var file3MD5 = makeTmpFile( filePath, fileName3, fileSize );

   commDropCL( db, csName, mainCLName );
   commDropCL( db, csName, subCLName );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );
   commCreateCL( db, csName, subCLName );
   mainCL.attachCL( csName + "." + subCLName, { "LowBound": { "date": { "$minKey": 1 } }, "UpBound": { "date": { "$maxKey": 1 } } } );

   var lobID1 = mainCL.putLob( filePath + fileName1 );
   var lobID2 = mainCL.putLob( filePath + fileName2 );
   var lobID3 = mainCL.putLob( filePath + fileName3 );

   //truncate length = 0
   mainCL.truncateLob( lobID1, 0 );
   var lobInfo = mainCL.getLob( lobID1, filePath + "check19043_1" );
   var actFileSize = File.getSize( filePath + "check19043_1" );
   var actLobSize = lobInfo.toObj().LobSize;
   assert.equal( actLobSize, 0 );
   assert.equal( actFileSize, 0 );

   //truncate length = fileSize /2
   var length = fileSize / 2;
   var cmd = new Cmd();
   mainCL.truncateLob( lobID2, length );
   cmd.run( "truncate -s " + ( length ) + " " + filePath + fileName2 );
   var lobInfo = mainCL.getLob( lobID2, filePath + "check19043_2" );
   var actLobSize = lobInfo.toObj().LobSize;
   var expMD5 = File.md5( filePath + fileName2 );
   var actMD5 = File.md5( filePath + "check19043_2" );
   assert.equal( actLobSize, length );
   assert.equal( expMD5, actMD5 );


   //truncate length = fileSize
   mainCL.truncateLob( lobID3, fileSize );
   var lobInfo = mainCL.getLob( lobID3, filePath + "check19043_3" );
   var actLobSize = lobInfo.toObj().LobSize;
   var actMD5 = File.md5( filePath + "check19043_3" );
   assert.equal( actLobSize, fileSize );
   assert.equal( file3MD5, actMD5 );

   cleanMainCL( db, csName, mainCLName );
   deleteTmpFile( filePath );
}
