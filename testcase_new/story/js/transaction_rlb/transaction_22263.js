/******************************************************************************
@Description :   seqDB-22263:事务操作过程中执行reelect操作
@author :   2020-6-5    wuyan  Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range"};
testConf.clName = CHANGEDPREFIX + "_transaction22263";
main( test );

function test( testPara )
{
   var groupName = testPara.srcGroupName;
   db.transBegin();
   testPara.testCL.insert( { a: 1 } );
   
   try
   {
      var newdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );    
      var rg = newdb.getRG( groupName );
      var slaveNode = rg.getSlave();
      slaveHostName = slaveNode.getHostName();
      slaveServiceName = slaveNode.getServiceName();
      rg.reelect({Seconds: 300, HostName: slaveHostName, ServiceName: slaveServiceName });
   
      try
      {
         db.transCommit();
         throw new Error( "commit should be failed!");
      }
      catch( e )
      {
         if( e.message != SDB_COORD_REMOTE_DISC )
         {
            throw e;
         }
      }
      
      db.transRollback();
      checkReelect( groupName, slaveHostName, slaveServiceName );
      commCheckBusinessStatus( db, 180, true );      
      checkInsertResult(testConf.clName);
      
   }
   finally
   {
      db.transRollback(); 
      if( newdb !== undefined )
      {
         newdb.close();
      }
   }
}

function checkInsertResult(clName)
{
   //等待事务回滚结束后再查询记录比较结果，最长等待时间3分钟
   var cl = db.getCS(COMMCSNAME).getCL(clName);
   var expCount = 0;
   var waitMaxTime = 180;
   var doTime = 0;
   while(doTime < waitMaxTime)
   {
      var clCount = cl.count();
      if( Number(expCount) == Number(clCount))
      {  
         break;
      }
      else
      {
         sleep( 1000 );
         doTime++;
      }
   }
   var cursor = cl.find();
   commCompareResults( cursor, [] );
}

function checkReelect( groupName, hostName, svcName )
{
   var masterNode = db.getRG(groupName).getMaster();
   var masterNodeHostName = masterNode.getHostName();
   var masterNodeSvcName = masterNode.getServiceName();
   if( masterNodeHostName !== hostName || masterNodeSvcName !== svcName )
   {
      throw new Error( "Reelect failed! master node is " + JSON.stringify(masterNode) + "\n exp masternode is " + hostName + ":" + svcName );
   }
}
