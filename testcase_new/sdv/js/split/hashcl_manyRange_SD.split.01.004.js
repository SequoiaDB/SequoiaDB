
/************************************
*@Description：hash分区表往多个目标组进行范围切分，如cl属于分区组A，将cl的数据切分到组B和C,验证2次切分
*@author：2015-10-20 wuyan  Init
**************************************/
var clName = CHANGEDPREFIX + "_splitcl004";
function createSplitCl ( csName, clName )
{
   var options = { ShardingKey: { no: 1 }, ShardingType: "hash", Partition: 1024, ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, csName, clName, options, true, true );
   return varCL;
}

//split data from srcgroup to targetgrpup by percent 50 
function splitByHash ( dbCL, csName, clName )
{
   var targetGroupNums = 2;
   var groupsInfo = getSplitGroups( COMMCSNAME, clName, targetGroupNums );
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
      for( var i = 1; i != groupsInfo.length; ++i )
      {
         println( "--splitTimes=" + i );
         dbCL.split( srcGrName, groupsInfo[i].GroupName, { Partition: 500 * ( i - 1 ) }, { Partition: 500 * i } );
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

function checkHashSplitResult ( clName, dataNodeInfo )
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
         println( "--the find count of " + dataNodeInfo[i].HostName + ":" + dataNodeInfo[i].svcname + " is " + num )
         dataNum.push( num );
         total += num;
      }
      if( Number( total ) !== 1000 )
      {
         throw buildException( "checkHashSplitResult()", null, "the total datas of the cl is wrong", 1000, total )
      }

      // the expect split to the target data is splitNum
      var targetNum = 1000 * ( 500 / 1024 );
      var hashLimit = targetNum * 0.2;
      for( var i = 1; i != dataNum.length; ++i )
      {
         var diff = Math.abs( targetNum - dataNum[i] );
         if( diff > hashLimit )
         {
            println( "ERROR! the split data num is out of limit!" );
            throw "ERROR!";
         }
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

      var allGroupName = getGroupName( db, true );
      if( 3 > allGroupName.length )
      {
         println( "--least three groups" );
         return;
      }
      var CL = createSplitCl( COMMCSNAME, clName );
      //split data ,and return nodeSvcNames for check split result   
      var nodeInfos = splitByHash( CL, COMMCSNAME, clName );
      //check the datas of the splitgroups,find datas count by split range in every splitgroups and check count 
      checkHashSplitResult( clName, nodeInfos );


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
