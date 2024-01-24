/******************************************************************************
*@Description : seqDB-21854:指定ShowMainCLMode为main，设置主子表Attributes/CompressionType值不同，查询集合快照 
                seqDB-21855:指定ShowMainCLMode为main，子表的PageSize/LobPageSize值不同，查询集合快照信息
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5578

function test ()
{
   var mainCSName = "mainCS_21854_21855";
   var mainCLName = "mainCL_21854_21855";
   var subCSName1 = "subCS_21854_21855_1";
   var subCLName1 = "subCL_21854_21855_1";
   var subCSName2 = "subCS_21854_21855_2";
   var subCLName2 = "subCL_21854_21855_2";
   var groupName = commGetGroups( db )[0][0].GroupName;

   commDropCS( db, mainCSName );
   commDropCS( db, subCSName1 );
   commDropCS( db, subCSName2 );
   commCreateCS( db, mainCSName, undefined, undefined, { PageSize: 4096, LobPageSize: 4096 } );
   commCreateCS( db, subCSName1, undefined, undefined, { PageSize: 8192, LobPageSize: 8192 } );
   commCreateCS( db, subCSName2, undefined, undefined, { PageSize: 16384, LobPageSize: 16384 } );
   var mainCL = commCreateCL( db, mainCSName, mainCLName, {
      IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range",
      Compressed: true, CompressionType: "snappy", ReplSize: 0
   } );
   var subCL1 = commCreateCL( db, subCSName1, subCLName1, {
      ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true,
      CompressionType: "lzw", Group: groupName, ReplSize: 0
   } );
   var subCL2 = commCreateCL( db, subCSName2, subCLName2, {
      ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName,
      ReplSize: 0
   } );
   mainCL.attachCL( subCSName1 + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.attachCL( subCSName2 + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );

   var snapshotOption = "/*+use_option( ShowMainCLMode, main )*/";
   var cursor = db.exec( 'select * from $SNAPSHOT_CL where Name = "' + mainCSName + '.' + mainCLName + '" ' + snapshotOption );
   var expResult = { Attribute: "Compressed", CompressionType: "snappy", PageSize: 8192, LobPageSize: 8192 };
   checkParameters( cursor, expResult );

   commDropCS( db, mainCSName );
   commDropCS( db, subCSName1 );
   commDropCS( db, subCSName2 );
}

