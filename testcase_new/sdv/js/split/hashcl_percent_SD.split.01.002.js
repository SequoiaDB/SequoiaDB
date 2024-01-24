
/************************************
*@Description：对hash分区组进行百分比切分，输入1000条记录，按50%切分，验证切分基本功能
*@author：2015-10-14 wuyan  Init
**************************************/

var clName = CHANGEDPREFIX + "_rangesplitcl002";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

function checkSplitResult ( CL, clName, splitGroupsInfo )
{
   //step1:
   checkHashClSplitResult( clName, splitGroupsInfo )
   //step2:
   insertDataAfterHashClSplit( CL )
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
      var dataNums = 1000;
      insertData( db, COMMCSNAME, clName, dataNums );

      //split data from srcgroup to targetgrpup by percent 50                   
      var percent = 50;
      var splitGrInfo = ClSplitOneTimes( COMMCSNAME, clName, percent );

      //check the datas of the splitgroups,find datas count by split range in every splitgroups and check count 
      checkSplitResult( CL, clName, splitGrInfo );

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
