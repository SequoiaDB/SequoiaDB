/*************************************************************
* @Description: seqDB-11722:reloadConf后新建编目节点
* @Jira: SEQUOIADBMAINSTREAM-2371
* @Author:      Liangxw Init
*************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groups = commGetGroups( db );
   var groupName = groups[0][0].GroupName;
   var nodeAttr = {};
   nodeAttr.hostName = groups[0][1].HostName;
   nodeAttr = addNode( groupName, nodeAttr );

   var oma = new Oma( nodeAttr.hostName, CMSVCNAME );
   var configs = oma.getNodeConfigs( nodeAttr.svcName ).toObj();
   configs["weight"] = 20;
   oma.setNodeConfigs( nodeAttr.svcName, configs );
   oma.close();
   db.reloadConf();
   db.getRG( groupName ).removeNode( nodeAttr.hostName, nodeAttr.svcName );

   groupName = "SYSCatalogGroup";
   nodeAttr = addNode( groupName, nodeAttr );
   db.getRG( groupName ).removeNode( nodeAttr.hostName, nodeAttr.svcName );

   checkNodeConfFile( groups );
}

function addNode ( groupName, nodeAttr )
{
   var doTimes = 0;
   var checkSucc = false;
   var maxRetryTimes = 10;
   while( !checkSucc && doTimes < maxRetryTimes )
   {
      nodeAttr.svcName = parseInt( RSRVPORTBEGIN ) + 10 * ( 2 + doTimes );
      nodeAttr.dbPath = RSRVNODEDIR + "data/" + nodeAttr.svcName;
      try
      {
         println( "add node: " + nodeAttr.hostName + ":" + nodeAttr.svcName + ", dbpath: " + nodeAttr.dbPath );
         db.getRG( groupName ).createNode( nodeAttr.hostName, nodeAttr.svcName, nodeAttr.dbPath );
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
   return nodeAttr;
}

function checkNodeConfFile ( groups )
{
   var sdbDir = commGetRemoteInstallPath( COORDHOSTNAME, CMSVCNAME );
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var cmd = remote.getCmd();

   for( var i = 0; i < groups.length; i++ )
   {
      var group = groups[i];
      for( var j = 1; j < group.length; j++ )
      {
         var confFile = sdbDir + "/conf/local/" + group[j].svcname + "/sdb.conf";
         var configs = cmd.run( "cat " + confFile ).split( "\n" ).sort();
         for( var k = 0; k < configs.length; k++ )
         {
            if( configs[k] === configs[k + 1] )
            {
               throw new Error( "The confFile has duplicate conf: " + configs[k] );
            }
         }
      }
   }
}

