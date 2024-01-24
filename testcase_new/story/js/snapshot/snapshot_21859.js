/******************************************************************************
*@Description : seqDB-21859:各子表含有不同个数的主表索引，指定ShowMainCLMode为main，查询集合快照
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5578

function test ()
{
   var mainCLName = "mainCL_21859";
   var subCLName1 = "subCL_21859_1";
   var subCLName2 = "subCL_21859_2";
   var groupName = commGetGroups( db )[0][0].GroupName;

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "range", Group: groupName } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupName } );
   commCreateIndex( mainCL, "index_1", { a: 1, b: 1 } );
   commCreateIndex( subCL2, "index_1", { a: 1, b: 1 } );
   commCreateIndex( subCL2, "index_2", { a: 1, c: 1 } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var expResult = { Indexes: 4 };
   var snapshotOption = "/*+use_option( ShowMainCLMode, main )*/";
   var cursor = db.exec( 'select * from $SNAPSHOT_CL where Name = "' + COMMCSNAME + '.' + mainCLName + '" ' + snapshotOption );
   checkParameters( cursor, expResult );

   commDropCL( db, COMMCSNAME, mainCLName, false, false );
}

