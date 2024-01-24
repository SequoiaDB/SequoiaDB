/******************************************************************************
*@Description : seqDB-21851:指定ShowMainCLMode（覆盖both/sub/main），查询集合快照信息 
                seqDB-21863:指定ShowMainCLMode（覆盖both/sub/main），查询集合快照信息
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5593;SEQUOIADBMAINSTREAM-5578

function test ()
{
   var clName = "cl_21851_21863";
   var mainCLName = "mainCL_21851_21863"
   var subCLName1 = "subCL_21851_21863_1";
   var subCLName2 = "subCL_21851_21863_2";

   commDropCL( db, COMMCSNAME, clName );
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } )
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } )

   //指定ShowMainCLMode为both
   var actResult = [];
   var containedResult = [COMMCSNAME + "." + clName, COMMCSNAME + "." + mainCLName,
   COMMCSNAME + "." + subCLName1, COMMCSNAME + "." + subCLName2];
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowMainCLMode: "both" } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, sdbsnapshotOption );
   var actResult = getCursorResult( cursor );
   if( !isContained( actResult, containedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\ncontainedResult [" + containedResult + "]" );
   }

   //指定ShowMainCLMode为sub
   var actResult = [];
   var notContainedResult = [COMMCSNAME + "." + mainCLName];
   var containedResult = [COMMCSNAME + "." + clName, COMMCSNAME + "." + subCLName1, COMMCSNAME + "." + subCLName2];
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowMainCLMode: "sub" } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, sdbsnapshotOption );
   actResult = getCursorResult( cursor );
   //判断快照信息中含有子表信息且不含有主表信息
   if( !isContained( actResult, containedResult ) || !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult +
         "]\ncontainedResult [" + containedResult + "]" );
   }

   //指定ShowMainCLMode为main
   var actResult = [];
   var containedResult = [COMMCSNAME + "." + mainCLName];
   var notContainedResult = [COMMCSNAME + "." + subCLName1, COMMCSNAME + "." + subCLName2];
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowMainCLMode: "main" } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, sdbsnapshotOption );
   actResult = getCursorResult( cursor );
   //判断快照信息中含有主表信息且不含有子表信息
   if( !isContained( actResult, containedResult ) || !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult +
         "]\ncontainedResult [" + containedResult + "]" );
   }

   snapshotOption = "/*+use_option( ShowMainCLMode, main )*/";
   cursor = db.exec( 'select * from $SNAPSHOT_CL ' + snapshotOption );
   actResult = getCursorResult( cursor );
   //判断快照信息中含有主表信息且不含有子表信息
   if( !isContained( actResult, containedResult ) || !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult +
         "]\ncontainedResult [" + containedResult + "]" );
   }

   commDropCL( db, COMMCSNAME, clName, false, false );
   commDropCL( db, COMMCSNAME, mainCLName, false, false );
}
