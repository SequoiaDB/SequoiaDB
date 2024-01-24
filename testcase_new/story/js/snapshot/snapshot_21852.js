/******************************************************************************
*@Description : seqDB-21852:指定ShowMainCLMode为main，只存在主表，查询集合快照信息 
*@author      : Zhao xiaoni
*@Date        : 2020-02-21
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5578

function test ()
{
   var clName = "cl_21852";
   var mainCLName = "mainCL_21852"

   commDropCL( db, COMMCSNAME, mainCLName );
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );

   //指定ShowMainCLMode为main
   var notContainedResult = [COMMCSNAME + "." + mainCLName];
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowMainCLMode: "main" } );
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, sdbsnapshotOption );
   var actResult = getCursorResult( cursor );
   //判断快照信息中不含有主表信息
   if( !isNotContained( actResult, notContainedResult ) )
   {
      throw new Error( "\nactResult [" + actResult + "]\nnotContainedResult [" + notContainedResult + "]" );
   }

   commDropCL( db, COMMCSNAME, mainCLName, false, false );
}

