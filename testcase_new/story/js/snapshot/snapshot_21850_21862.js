/******************************************************************************
*@Description : seqDB-21850:不指定ShowMainCLMode，查询集合快照信息 
                seqDB-21862:不指定ShowMainCLMode，查询集合快照信息
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5578

function test ()
{
   var clName = "cl_21850_21860";
   var mainCLName = "mainCL_21850_21860"
   var subCLName1 = "subCL_21850_21860_1";
   var subCLName2 = "subCL_21850_21860_2";

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

   var notContainedResult = [COMMCSNAME + "." + mainCLName];
   var containedResult = [COMMCSNAME + "." + clName, COMMCSNAME + "." + subCLName1, COMMCSNAME + "." + subCLName2];
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS );
   var actResult = getCursorResult( cursor );
   if( !isContained( actResult, containedResult ) || !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult +
         "]\ncontainedResult [" + containedResult + "]" );
   }

   cursor = db.exec( 'select * from $SNAPSHOT_CL' );
   actResult = getCursorResult( cursor );
   if( !isContained( actResult, containedResult ) || !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult +
         "]\ncontainedResult [" + containedResult + "]" );
   }

   commDropCL( db, COMMCSNAME, clName, false, false );
   commDropCL( db, COMMCSNAME, mainCLName, false, false );
}

