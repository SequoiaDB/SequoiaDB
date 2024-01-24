/******************************************************************************
*@Description : seqDB-21853:指定ShowMainCLMode的值为非法值，查询集合快照信息 
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5578

function test ()
{
   var mainCLName = "mainCL_21853"
   var subCLName1 = "subCL_21853_1";
   var subCLName2 = "subCL_21853_2";

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } )
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } )

   var notContainedResult = [COMMCSNAME + "." + mainCLName];
   var containedResult = [COMMCSNAME + "." + subCLName1, COMMCSNAME + "." + subCLName2];
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowMainCLMode: "非法值" } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, sdbsnapshotOption );
   var actResult = getCursorResult( cursor );
   if( !isContained( actResult, containedResult ) || !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult +
         "]\ncontainedResult [" + containedResult + "]" );
   }

   commDropCL( db, COMMCSNAME, mainCLName, false, false );
}

