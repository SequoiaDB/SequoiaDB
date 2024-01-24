/******************************************************************************
 * @Description : test getSlave operation
 *                seqDB-13790:指定多个位置获取备节点
 * @auhor       : Liang XueWang
 ******************************************************************************/
var rgName = "testGetSlaveRg13790";
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
      //问题单：SEQUOIADBMAINSTREAM-3495
      testTwoNodeMasterPos();
      testThreeNode();
   }
   catch( e )
   {
      //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
      var backupDir = "/tmp/ci/rsrvnodelog/13790";
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

// only one node in rg, test getSlave with 1-7
function testOneNode ()
{
   var rg = db.createRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );
   var slave = rg.getSlave( 1, 2, 3, 4, 5, 6, 7 );
   assert.equal( slave.toString(), master.toString() );

}

// two nodes in rg, test getSlave with 1-7
function testTwoNode ()
{
   var rg = db.getRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );
   var slave = rg.getSlave( 1, 2, 3, 4, 5, 6, 7 );
   assert.notEqual( slave.toString(), master.toString() );
}

// two nodes in rg, getSlave with all master pos
function testTwoNodeMasterPos ()
{
   var rg = db.getRG( rgName );
   var master = getMaster( rg );

   var nodes = getGroupNodes( db, rgName );
   var masterIdx = nodes.indexOf( master.toString() );
   assert.notEqual( masterIdx, -1 );
   var masterPos = masterIdx + 1;

   var slave = rg.getSlave( masterPos, masterPos, masterPos );
   assert.equal( slave.toString(), master.toString() );
}

// three nodes in rg( master change ), getSlave with 1-7
function testThreeNode ()
{
   var rg = db.getRG( rgName );
   logSourcePaths = createNodes( rg, 1 );

   var master = getMaster( rg );
   master.stop();
   var newMaster = getMaster( rg );

   var slave = rg.getSlave( 1, 2, 3, 4, 5, 6, 7 );
   assert.notEqual( slave.toString(), newMaster.toString() );
}
