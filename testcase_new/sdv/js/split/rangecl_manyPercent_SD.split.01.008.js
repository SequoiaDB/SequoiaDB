
/************************************
*@Description：range分区表往多个组进行百分比切分，执行多次split，设置百分比切分条件，如第一次切分到组a，第二次切分到组b
*@author：2015-10-20 wuyan  Init
**************************************/
var clName = CHANGEDPREFIX + "_rangesplitcl008";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "range", ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

//split data from srcgroup to targetgrpup by percent，1 split：20%；2 split；40%
function splitByPercent ( dbCL, csName, clName )
{
   var targetGroupNums = 2;
   var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
   var srcGrName = groupsInfo[0].GroupName;

   insertData( db, csName, clName, 1000 );

   //spliting 2 times 
   var splitTimes = 2;
   println( "--begin split" )
   try
   {
      var catadb = new Sdb( COORDHOSTNAME, CATASVCNAME );
      var sleepInteval = 10;
      var sleepDuration = 0;
      var maxSleepDuration = 100;
      for( var i = 1; i <= splitTimes; ++i )
      {
         println( "--splitTimes=" + i );
         dbCL.split( srcGrName, groupsInfo[i].GroupName, 20 * i );
         while( catadb.SYSCAT.SYSTASKS.find().count() != 0 && sleepDuration < maxSleepDuration )
         {
            sleep( sleepInteval );
            sleepDuration += sleepInteval;
         }
      }
   }
   catch( e )
   {
      throw buildException( "splitByPercent", e );
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
      //split data ,and return nodeSvcNames and nodeHostNames for check split result   
      var nodeInfos = splitByPercent( CL, COMMCSNAME, clName );
      //the data expect count of every group
      var expectResult = new Array( 480, 200, 320 )
      checkSplitResultToEveryGr( clName, nodeInfos, expectResult )

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
