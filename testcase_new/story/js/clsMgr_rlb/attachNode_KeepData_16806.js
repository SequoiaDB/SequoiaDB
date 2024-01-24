/* *****************************************************************************
@discretion: attachNode( )中KeepData参数校验
@author：2018-12-12 wangkexin
***************************************************************************** */
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   try
   {
      var groupList = getGroup( db );
      var groupName1 = groupList[0];
      var groupName2 = groupList[1];

      var hostname1 = db.getRG( groupName1 ).getDetail().next().toObj()["Group"][0]["HostName"];
      var port = parseInt( RSRVPORTBEGIN ) + 50;

      db.getRG( groupName1 ).createNode( hostname1, port, RSRVNODEDIR + port );
      db.getRG( groupName1 ).start();
      db.getRG( groupName1 ).detachNode( hostname1, port, { KeepData: true } );

      //test a : KeepData设为合法值
      db.getRG( groupName2 ).attachNode( hostname1, port, { KeepData: true } );
      db.getRG( groupName2 ).detachNode( hostname1, port, { KeepData: false } );
      //test b : KeepData设为空值
      attachNodeLawfulness( groupName2, hostname1, port, "" );
      //test c : KeepData设为非布尔值
      attachNodeLawfulness( groupName2, hostname1, port, "test" );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      try
      {
         db.getRG( groupName2 ).attachNode( hostname1, port, { KeepData: true } );
      }
      catch( e )
      {
         // -145:SDBCM_NODE_EXISTED  -155:SDB_CLS_NODE_NOT_EXIST
         if( e.message != SDBCM_NODE_EXISTED && e.message != SDB_CLS_NODE_NOT_EXIST )
         {
            throw e;
         }
      }
      db.getRG( groupName2 ).start();
      try
      {
         db.getRG( groupName2 ).removeNode( hostname1, port );
      }
      catch( e )
      {
         if( e.message == SDB_CLS_NODE_NOT_EXIST )
         {
            db.getRG( groupName1 ).removeNode( hostname1, port );
         }
         else
         {
            throw e;
         }
      }
      if( db !== undefined )
      {
         db.close();
      }
   }
}

function attachNodeLawfulness ( groupName, hostname, port, KeepDataMsg )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRG( groupName ).attachNode( hostname, port, { KeepData: KeepDataMsg } );
   } );
}
