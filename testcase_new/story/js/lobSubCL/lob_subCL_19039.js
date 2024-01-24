/************************************
*@Description: seqDB-19039 主表挂载不同lobpagesize的子表，进行lob操作
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
   var mainCLName = "mainCL_19039";
   var subName1 = "sub_19039_1";
   var subName2 = "sub_19039_2";
   var subName3 = "sub_19039_3";
   var filePath = WORKDIR + "/lob19039/";
   var fileName = "file19039"
   var fileFullPath = filePath + fileName;
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName );

   commDropCL( db, csName, mainCLName );
   commDropCS( db, subName1 );
   commDropCS( db, subName2 );
   commDropCS( db, subName3 );

   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl1" );

   commCreateCS( db, subName1, false, "", { LobPageSize: 4096 } );
   commCreateCL( db, subName1, subName1 );

   commCreateCS( db, subName2, false, "", { LobPageSize: 16384 } );
   commCreateCL( db, subName2, subName2 );

   commCreateCS( db, subName3, false, "", { LobPageSize: 262144 } );
   commCreateCL( db, subName3, subName3 );

   mainCL.attachCL( subName1 + "." + subName1, { "LowBound": { "date": "20190801" }, "UpBound": { "date": "20190805" } } );
   mainCL.attachCL( subName2 + "." + subName2, { "LowBound": { "date": "20190805" }, "UpBound": { "date": "20190810" } } );
   mainCL.attachCL( subName3 + "." + subName3, { "LowBound": { "date": "20190810" }, "UpBound": { "date": "20190815" } } );

   var lobOids = insertLob( mainCL, fileFullPath, "YYYYMMDD", 5, 10, 3, "20190801" );

   checkLobMD5( mainCL, lobOids, fileMD5 );

   for( i in lobOids )
   {
      mainCL.deleteLob( lobOids[i] );
      assert.tryThrow( SDB_FNE, function()
      {
         mainCL.getLob( lobOids[i], WORKDIR + "/checkLob19039_" + i );
      } );
   }

   deleteTmpFile( filePath );
   commDropCL( db, csName, mainCLName );
   commDropCS( db, subName1 );
   commDropCS( db, subName2 );
   commDropCS( db, subName3 );
}
