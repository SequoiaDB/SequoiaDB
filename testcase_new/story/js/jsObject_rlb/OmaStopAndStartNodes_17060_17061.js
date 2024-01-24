/************************************
*@Description: Oma.stopNodes()停止节点/Oma.startNodes()启动节点
*@author:      luweikang
*@createdate:  2019.3.18
*@testlinkCase:seqDB-17060 seqDB-17061
*@Info        ：用例执行机可能没有装sequoiadb，本地测试时new Oma()会失败,所以仅测试远程Oma
*               集群仅有1个catalog时，如catalog被停止，则其他节点会无法连接，因为连接节点时需要向catalog获取鉴权
**************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   //获取执行oma操作的主机名和操作的节点列表
   var remoteHost = toolGetLocalhost();
   var hostName = toolGetLocalhost().hostname;
   var oma = new Oma( COORDHOSTNAME, CMSVCNAME );
   var nodeList = getNodeList( db, hostName );

   try
   {
      //停止一个节点，启动一个节点
      var testNode1 = [nodeList[0]];
      oma.stopNodes( testNode1 );
      checkNodeStatus( hostName, testNode1, false );
      oma.startNodes( testNode1 );
      checkNodeStatus( hostName, nodeList, true );

      //停止多个节点，启动多个节点
      var testNode2 = [nodeList[1], nodeList[2]];
      oma.stopNodes( testNode2 );
      checkNodeStatus( hostName, testNode2, false );
      oma.startNodes( testNode2 );
      checkNodeStatus( hostName, nodeList, true );

      //停止所有节点，启动所有节点
      var testNode3 = nodeList;
      oma.stopNodes( testNode3 );
      checkNodeStatus( hostName, testNode3, false );
      oma.startNodes( testNode3 );
      checkNodeStatus( hostName, nodeList, true );

      //停止已停止或不存在的节点，启动多个节点包括不存在的节点
      var testNode4 = [nodeList[0]];
      oma.stopNodes( testNode4 );
      checkNodeStatus( hostName, [nodeList[0]], false );
      testNode4.push( RSRVPORTBEGIN );
      oma.stopNodes( testNode4 );
      checkNodeStatus( hostName, [nodeList[0]], false );
      try
      {
         oma.startNodes( testNode4 );
         throw new Error( "NODE_NOT_EXIST" );
      }
      catch( e )
      {
         if( e.message != SDB_COORD_NOT_ALL_DONE )
         {
            throw e;
         }
      }
      checkNodeStatus( hostName, nodeList, true );
   }
   catch( e )
   {
      oma.startAllNodes();
      throw e;
   }


}

function getNodeList ( db, hostName )
{
   var nodeList = [];
   var nodeCur = db.list( SDB_LIST_GROUPS, {}, { "Group": { "$elemMatch": { "HostName": hostName } } } );
   while( nodeCur.next() )
   {
      var node = JSON.parse( nodeCur.current() ).Group[0].Service[0].Name
      nodeList.push( node );
   }
   return nodeList;
}

function checkNodeStatus ( hostname, nodeList, isNormal )
{
   for( var i = 0; i < nodeList.length; i++ )
   {
      try
      {
         var conn = new Sdb( hostname, nodeList[i] );
         conn.close();
         if( !isNormal )
         {
            throw new Error( "NODE_SHOULD_DOWN" );
         }
      }
      catch( e )
      {
         if( isNormal )
         {
            throw new Error( "checkNodeStatus() fail,check node status: '" + hostname + ":" + nodeList[i] + "'" + "node normal" + "node error" );
         }
         else
         {
            if( e.message != SDB_NET_CANNOT_CONNECT )
            {
               throw new Error( "checkNodeStatus() fail,check node error: '" + hostname + ":" + nodeList[i] + "'" + -79 + e );
            }
         }
      }
   }
}


