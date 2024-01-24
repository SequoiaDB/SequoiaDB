
/************************************
*@Description：hash分区表往同一个目标组多次进行百分比切分，设置百分比切分条件，如第一次切分20%，第二次切分40%
*@author：2015-10-20 wuyan  Init
**************************************/
var clName = CHANGEDPREFIX + "_SplitCl003";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

//split data from srcgroup to targetgrpup by percent，1 split：20%；2 split；40%
function splitByPercent ( dbCL, csName, clName )
{
   var targetGroupNums = 1;
   var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
   var srcGrName = groupsInfo[0].GroupName;

   //spliting 2 times 
   var splitTimes = 2;

   insertData( db, csName, clName, 1000 );
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
         dbCL.split( srcGrName, groupsInfo[1].GroupName, 20 * i );
         while( catadb.SYSCAT.SYSTASKS.find().count() != 0 && sleepDuration < maxSleepDuration )
         {
            sleep( sleepInteval );
            sleepDuration += sleepInteval;
         }
      }
   }
   catch( e )
   {
      throw buildException( "splitByRange", e );
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

function checkHashSplitResult ( clName, dataNodeInfo, percent )
{
   try
   {
      var dataNum = [];
      var total = 0;
      for( var i = 0; i != dataNodeInfo.length; ++i )
      {
         var gdb = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );
         var cl = gdb.getCS( COMMCSNAME ).getCL( clName )
         var num = cl.count();
         println( "--the find count of " + dataNodeInfo[i].svcname + " is " + num )
         dataNum.push( num );
         total += num;
      }
      if( Number( total ) !== 1000 )
      {
         throw buildException( "checkHashSplitResult()", null, "the total datas of the cl is wrong", 1000, total )
      }
      // the expect split to the target data is splitNum
      var splitNum = 1000 * ( 1 - 0.2 );
      var hashLimit = ( splitNum * percent ) * 0.2;
      var expectTargetDatas = 1000 * 0.2 + splitNum * percent;
      var diff = Math.abs( expectTargetDatas - dataNum[1] );
      if( diff > hashLimit )
      {
         println( "ERROR! the split data num is out of limit!" );
         throw "ERROR!";
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( gdb !== undefined )
      {
         gdb.close();
         gdb == undefined;
      }
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
      //split data ,and return nodeSvcNames and nodeHostNames for check split result   
      var nodeInfos = splitByPercent( CL, COMMCSNAME, clName );
      var splitNum = 1000 * ( 1 - 0.2 );
      checkHashSplitResult( clName, nodeInfos, 0.4 );

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
