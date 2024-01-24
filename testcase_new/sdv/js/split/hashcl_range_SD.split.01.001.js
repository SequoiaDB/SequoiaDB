
/************************************
*@Description：对hash分区组进行范围切分，按范围进行切分，验证切分基本功能
*@author：2015-10-14 wuyan  Init
**************************************/
//var CHANGEDPREFIX = "local_test"
var clName = CHANGEDPREFIX + "_hashsplitcl01001";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

function checkSplitResult ( CL, clName, splitGrInfo )
{
   //step1:
   checkHashClSplitResult( clName, splitGrInfo )
   //step2:
   insertDataAfterHashClSplit( CL )
}

function main ()
{
   try
   {
      //@ clean start:
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

      var dataNums = 1000;
      insertData( db, COMMCSNAME, clName, dataNums );
      condition = { Partition: 512 }
      splitGrInfo = ClSplitOneTimes( COMMCSNAME, clName, condition )
      checkSplitResult( CL, clName, splitGrInfo )

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
