/* *****************************************************************************
@Description: seqDB-439:新增数据节点目录重复
              seqDB-441:创建节点时端口号冲突
@author:      Zhao Xiaoni
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test()
{
   var dataPort = generatePort( RSRVPORTBEGIN );  
   var object = db.listReplicaGroups().current().toObj();
   var groupName = object.GroupName;
   var group = object.Group;
   var path = group[0].dbpath;
   var hostName = group[0].HostName;
   try
   {
      db.getRG( groupName ).createNode( hostName, dataPort, path );
      throw "Create node should be failed!";
   }
   catch( e )
   {
      if( -290 !== e )
      {
         throw new Error( e );
      }
   }
 
   dataPort = group[0].Service[0].Name;
   try
   {
      db.getRG( groupName ).createNode( hostName, dataPort, path );
      throw "Create node should be failed!";
   }
   catch( e )
   {
      if( -145 !== e )
      {
         throw new Error( e );
      }
   }
}

