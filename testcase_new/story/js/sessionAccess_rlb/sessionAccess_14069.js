/* *****************************************************************************
@description:  seqDB-14069:createNode()接口中instanceid参数校验
@author：2018-11-20 wangkexin
***************************************************************************** */
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var instanceidList = [12, 0, 1, 255, "123"];
   var errInstanceidList = [12.234, -1, 256, "0x10"];

   var failedCount = 0;
   var groupName = "rg_14069";
   var rg = db.createRG( groupName );
   var hostName = db.listReplicaGroups().current().toObj().Group[0].HostName;
   for( var i = 0; i < instanceidList.length; i++ )
   {
      var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
      var dbpath = RSRVNODEDIR + "data/" + svc;
      var checkSucc = false;
      var times = 0;
      var maxRetryTimes = 10;
      var config = { instanceid: instanceidList[i], diaglevel: 5 };
      while( !checkSucc && times < maxRetryTimes )
      {
         try
         {
            rg.createNode( hostName, svc, dbpath, config );
            checkSucc = true;
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( e == SDBCM_NODE_EXISTED || e == SDB_DIR_NOT_EMPTY )
            {
               svc = svc + 10;
               dbpath = RSRVNODEDIR + "data/" + svc;
               failedCount++;
            }
            else
            {
               throw new Error( "create node failed! " + hostName + ":" + svc + " " + dbpath + " config: " + JSON.stringify( config ) + " errorCode: " + e );
            }
            times++;
         }
      }
   }

   for( var i = 0; i < errInstanceidList.length; i++ )
   {
      svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + 5 + failedCount );
      dbpath = RSRVNODEDIR + "data/" + svc;
      var config = { instanceid: errInstanceidList[i], diaglevel: 5 };
      try
      {
         rg.createNode( hostName, svc, dbpath, config );
         throw new Error( "Create node " + hostName + ":" + svc + " " + dbpath + " config: " + JSON.stringify( config ) + "  need error!" );
      }
      catch( e )
      {
         if( e.message != SDB_INVALIDARG )
         {
            throw e;
         }
      }
   }
   rg.start();

   checkResult( groupName, instanceidList );
   db.removeRG( groupName );
}

function checkResult ( groupName, instanceidList )
{
   var groupInfo = db.getRG( groupName ).getDetail().current().toObj();
   for( var i = 0; i < instanceidList.length; i++ )
   {
      if( instanceidList[i] != 0 && instanceidList[i] != groupInfo.Group[i].instanceid )
      {
         throw new Error( "instanceidList[" + i + "] = " + instanceidList[i] + ", groupInfo.Group[" + i + "].instanceid = " + groupInfo.Group[i].instanceid );
      }
   }
}
