/* *****************************************************************************
@discretion: seqDB-20014:修改组内其中一个节点的权重为100，重新选主
@author：2018-11-06 zhaoxiaoni
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var nodesNum = 2;
   var groupName = "rg_20014";
   var hostName = commGetGroups( db )[0][1].HostName;
   createGroupAndNode( db, groupName, hostName, nodesNum );

   var clName = "cl_20014";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );

   var data = [];
   for( var i = 0; i < 10000; i++ )
   {
      data.push( { a: i } );
   }
   cl.insert( data );

   var masterNode = {};
   var node = db.getRG( groupName ).getMaster();
   masterNode.HostName = hostName;
   masterNode.svcname = node.getServiceName();
   var slaveNode = addNode( groupName, hostName );

   //节点创建成功启动后会有两个心跳窗口时间（一个心跳窗口时间是7s）的静默期，此时不会接受选主消息，因此这里停14s再执行选主
   sleep( 14000 );
   commCheckBusinessStatus( db );

   db.getRG( groupName ).reelect( { Seconds: 60 } );
   checkReelect( groupName, slaveNode.HostName, slaveNode.svcname );

   db.getRG( groupName ).reelect( { Seconds: 60 } );//由于内部实现影子权重的作用，再次reelect将切换主节点

   commDropCL( db, COMMCSNAME, clName, false, false );
   db.removeRG( groupName );
}

function addNode ( groupName, hostName )
{
   var dataNodeAttr = {};
   dataNodeAttr.HostName = hostName;
   dataNodeAttr.config = { weight: 100, diaglevel: 5 };
   var doTimes = 0;
   var checkSucc = false;
   var maxRetryTimes = 10;
   while( !checkSucc && doTimes < maxRetryTimes )
   {
      dataNodeAttr.svcname = parseInt( RSRVPORTBEGIN ) + 10 * ( 2 + doTimes );
      dataNodeAttr.dbPath = RSRVNODEDIR + "data/" + dataNodeAttr.svcname;
      try
      {
         db.getRG( groupName ).createNode( dataNodeAttr.HostName, dataNodeAttr.svcname, dataNodeAttr.dbPath, dataNodeAttr.config );
         checkSucc = true;
      }
      catch( e )
      {
         if( e.message == SDBCM_NODE_EXISTED || e.message == SDB_DIR_NOT_EMPTY )
         {
            doTimes++;
         }
         else
         {
            throw e;
         }
      }
   }
   db.getRG( groupName ).start();
   return dataNodeAttr;
}

