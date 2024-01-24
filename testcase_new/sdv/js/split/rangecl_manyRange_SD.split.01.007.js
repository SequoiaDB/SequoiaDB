
/************************************
*@Description：range分区表多次执行范围切分，在多个分区组之间执行split，如cl属于分区组A，
               将分区组A的数据指定范围切分到分区组B，在将分区组B的数据指定范围切分到分区组C,
               最后将分区组C中包含该字段的数据切回到分区组A
*@author：2015-10-21 wuyan  Init
**************************************/
var clName = CHANGEDPREFIX + "_splitcl007";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

//split 3 times from srcgroup to targetgrpup by different range
function splitManyByRange ( dbCL, csName, clName )
{
   var targetGroupNums = 2;
   var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );

   //groups are more than two,we split three times ,insert 1500 datas
   insertData( db, csName, clName, 1500 );

   println( "--begin split" );
   try
   {
      var catadb = new Sdb( COORDHOSTNAME, CATASVCNAME );
      var sleepInteval = 10;
      var sleepDuration = 0;
      var maxSleepDuration = 100;

      function split ( dbCL, srcGr, tarGr, endcondition )
      {
         dbCL.split( srcGr, tarGr, { no: 0 }, { no: endcondition } );
         while( catadb.SYSCAT.SYSTASKS.find().count() != 0 && sleepDuration < maxSleepDuration )
         {
            sleep( sleepInteval );
            sleepDuration += sleepInteval;
         }
      }
      //1 split
      println( "--1 split" )
      split( dbCL, groupsInfo[0].GroupName, groupsInfo[1].GroupName, 1000 );
      //2 split
      println( "--2 split" )
      split( dbCL, groupsInfo[1].GroupName, groupsInfo[2].GroupName, 500 );
      //3 split
      println( "--3 split" )
      split( dbCL, groupsInfo[2].GroupName, groupsInfo[0].GroupName, 300 );
   }
   catch( e )
   {
      throw buildException( "splitManyByRange", e );
   }
   finally
   {
      if( undefined !== catadb )
      {
         catadb.close();
      }
   }
   println( "--end split" )
   return groupsInfo;
}

function checkSplitResult ( dbCl, clName, splitGroupsInfo )
{
   //step1:
   var expectResult = new Array( 800, 500, 200 )
   checkSplitResultToEveryGr( clName, splitGroupsInfo, expectResult );
   //step2:
   insertDataAfterRangeClSplit( dbCl, clName, splitGroupsInfo )
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

      var allGroupName = getGroupName( db, true );
      if( 3 > allGroupName.length )
      {
         println( "--least three groups" );
         return;
      }

      var CL = createSplitCl( COMMCSNAME, clName );
      //split data ,and return nodeSvcNames for check split result   
      var splitGrInfo = splitManyByRange( CL, COMMCSNAME, clName );
      //check the datas of the splitgroups,find a data in splitgroups and check count	
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
