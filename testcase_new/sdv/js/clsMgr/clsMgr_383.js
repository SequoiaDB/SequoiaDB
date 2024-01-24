/* *****************************************************************************
@Description: seqDB-383:手工部署集群（集群模式部署） 
@author:      Zhao Xiaoni
***************************************************************************** */
main( test );

function test()
{ 
   var hostName = getHostName();
   var tmpCoordPort = generatePort( COORDSVCNAME );
   var tmpDb = createDb( hostName, tmpCoordPort );
   var cataPorts = createCata( tmpDb, hostName );
   var dataPorts1 = createData( tmpDb, hostName, "rg_383_1" );
   var dataPorts2 = createData( tmpDb, hostName, "rg_383_2" );
   var coordPorts = createCoord( tmpDb, hostName );

   var clName = COMMCLNAME + "_383";     
   var db = new Sdb( hostName, coordPorts[0] );
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   insertData( cl, 1 );
   commDropCL( db, COMMCSNAME, clName, false, false );
 
   try
   {
      //删除数据主节点或备节点
      clean( tmpDb, hostName, dataPorts1[ Math.floor( Math.random()*2 ) ], "rg_383_1", "data_node" );
   }
   catch( e )
   {
      if( e !== -204 )
      {
         throw new Error( e );
      }
   }
   clean( tmpDb, hostName, undefined, "rg_383_1", "data_group" );
   clean( tmpDb, hostName, undefined, "rg_383_2", "data_group" );
   clean( tmpDb, hostName, undefined, undefined, "coord_group" );
   try
   {
      clean( tmpDb, hostName, cataPorts[ Math.floor( Math.random()*2 ) ], undefined, "cata_node" );
   }
   catch( e )
   {
      if( e !== -206 )
      {
         throw new Error( e );
      }
   }
   clean( tmpDb, hostName, undefined, undefined, "cata_group" );
   clean( tmpDb, hostName, tmpCoordPort, undefined, "tmp_coord" );
   tmpDb.close();
}


