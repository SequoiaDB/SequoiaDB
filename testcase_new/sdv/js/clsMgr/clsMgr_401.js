/**********************************************
@Description: seqDB-401:删除单数据节点 
@author:      Zhao xiaoni
**********************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   var groupName = "rg_401";
   var hostName = getHostName();
   var dataPort = generatePort( RSRVPORTBEGIN );
   var dataRG = db.createRG( groupName )
   dataRG.createNode( hostName, dataPort, RSRVNODEDIR + "data/" + dataPort );
   dataRG.start();  
   try
   {
      clean( db, hostName, dataPort, groupName, "data_node" );
      throw "Remove single node should be failed!";
   }
   catch( e )
   {
      if( e !== -204 )
      {
         throw new Error( e );
      }
   }
   clean( db, undefined, undefined, groupName, "data_group" );
}

