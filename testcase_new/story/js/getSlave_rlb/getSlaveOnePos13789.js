/******************************************************************************
*@Description : test getSlave operation
*               seqDB-13789:指定一个位置获取备节点
*@auhor       : Liang XueWang
******************************************************************************/
var rgName = "testGetSlaveRg13789";
var logSourcePaths = [];
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   try
   {
      testOneNode();
      testTwoNode();
      testTwoNodeNoMaster();
   }
   catch( e )
   {
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/13789";
      File.mkdir( backupDir );
      for( var i = 0; i < logSourcePaths.length; i++ )
      {
         File.scp( logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
      }
      throw e;
   }
   finally
   {
      db.removeRG( rgName );
   }
}

// only one node in rg, test getSlave
function testOneNode ()
{
   var rg = db.createRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );

   for( var i = 1; i <= 7; i++ )
   {
      var slave = rg.getSlave( i );
      assert.equal( slave.toString(), master.toString() );
   }
}

// two nodes in rg, test getSlave
function testTwoNode ()
{
   var rg = db.getRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );

   var nodes = getGroupNodes( db, rgName );
   for( var i = 1; i <= 7; i++ )
   {
      var slave = rg.getSlave( i );
      var idx = ( i - 1 ) % 2;
      assert.equal( slave, nodes[idx] );
   }
}

// two nodes in rg( no master ), test getSlave
function testTwoNodeNoMaster ()
{
   var rg = db.getRG( rgName );

   var master = getMaster( rg );
   master.stop();
   var hasMaster = isMasterExist( db, rgName );
   assert.equal( hasMaster, false );

   var nodes = getGroupNodes( db, rgName );
   for( var i = 1; i <= 7; i++ )
   {
      var slave = rg.getSlave( i );
      var idx = ( i - 1 ) % 2;
      assert.equal( slave, nodes[idx] );
   }
}