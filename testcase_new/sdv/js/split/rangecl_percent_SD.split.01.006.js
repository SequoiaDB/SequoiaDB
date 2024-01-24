
/************************************
*@Description：对range分区组进行百分比切分，输入1000条记录，按50%切分，验证切分基本功能
*@author：2015-10-14 wuyan  Init
**************************************/

var clName = CHANGEDPREFIX + "_rangesplitcl006";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

function checkSplitResult ( CL, clName, splitGroupsInfo )
{
   //step1:
   checkRangeClSplitResult( clName, splitGroupsInfo );
   //step2:   
   insertDataAfterRangeClSplit( CL, clName, splitGroupsInfo )
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
      var splitGroupsInfo = ClSplitOneTimes( COMMCSNAME, clName, condition )

      //check the datas of the splitgroups,find a data in splitgroups and check count	     
      checkSplitResult( CL, clName, splitGroupsInfo )

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
