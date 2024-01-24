
/************************************
*@Description：split后插入数据，然后执行update操作；验证split后，数据插入、更新功能
*@author：2015-10-14 wuyan  Init
**************************************/

var clName = CHANGEDPREFIX + "_rangesplitcl019";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

function insertAndUpsertResult ( CL, clName, splitGroupsInfo )
{
   try
   {
      println( "--begin upsert" );
      insertDataAfterRangeClSplit( CL, clName, splitGroupsInfo );
      CL.upsert( { $set: { conf: "testupsert" } }, { no: { $lte: 500 } } );
      var num = CL.count( { conf: "testupsert" } );
      var expertNum = 502;
      if( Number( num ) !== expertNum )			
      {
         throw buildException( "checkRangeClSplitResult()", "count wrong", "count()", expertNum, num );
      }
      println( "--end upsert" );
   }
   catch( e )
   {
      throw e;
   }
}

function main () 
{
   try
   {
      //@ clean before
      if( true == commIsStandalone( db ) )
      {
         println( "run mode is standalone" );
         return;
      }
      //less two groups no split
      var allGroupName = getGroupName( db, true );
      if( 1 === allGroupName.length )
      {
         println( "--least two groups" );
         return;
      }
      var CL = createSplitCl( COMMCSNAME, clName );

      //insert data 
      var dataNums = 1000;
      insertData( db, COMMCSNAME, clName, dataNums );
      //split data ,and return the split groupsInfo for check split result  
      var condition = 50;
      var splitGrInfo = ClSplitOneTimes( COMMCSNAME, clName, condition );

      //insert a data in splitgroups after split，then update the data
      insertAndUpsertResult( CL, clName, splitGrInfo );

      //@ clean end
      commDropCL( db, COMMCSNAME, clName, false, false, "drop CL in the beginning" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( undefined !== db )
      {
         db.close();
      }
   }
}

main();
