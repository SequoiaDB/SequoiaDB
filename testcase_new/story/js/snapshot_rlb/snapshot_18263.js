/******************************************************************************
 * @Description   : seqDB-18263:指定快照查询参数Mode为Run查询配置快照信息
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.24
 * @LastEditTime  : 2023.04.23
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
main( test );

function test ( testPara )
{
   var groups = testPara.groups;
   var groupName = groups[0][0].GroupName;
   var node = db.getRG( groupName ).getSlave();
   var hostName = node.getHostName();
   var svcname = node.getServiceName();
   var nodeName = hostName + ":" + svcname;

   changeConf( nodeName );
   node.stop();
   node.start();
   commCheckBusinessStatus( db );

   var expResult = { "transactionon": "FALSE" };
   var option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "run", "expand": false } );
   checkConfValue( db, option, expResult );

   try
   {
      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.deleteConf( { transactionon: 1 }, { 'NodeName': nodeName } );
      } );

      expResult = { "transactionon": "FALSE" };
      option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "run", "expand": false } );
      checkConfValue( db, option, expResult );

      var fieldName = "transactionon";
      option = new SdbSnapshotOption().cond( { NodeName: nodeName }, { transaction: "" } ).options( { "mode": "local", "expand": false } );
      checkConfNotKey( db, option, fieldName );
   }
   finally
   {
      try
      {
         db.deleteConf( { transactionon: 1 }, { 'NodeName': nodeName } );
      }
      catch( e )
      {
         if( e != SDB_COORD_NOT_ALL_DONE && e != SDB_RTN_CONF_NOT_TAKE_EFFECT )
         {
            throw new Error( e );
         }
      }
      node.stop();
      node.start();
      commCheckBusinessStatus( db );
   }
}

function changeConf ( nodeName )
{
   try
   {
      db.updateConf( { transactionon: false }, { NodeName: nodeName } );
   }
   catch( e )
   {
      if( e != SDB_RTN_CONF_NOT_TAKE_EFFECT )
      {
         throw new Error( e );
      }
   }
}

function checkConfValue ( db, option, expResult )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, option );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in expResult )
      {
         assert.equal( obj[key], expResult[key], "预期value值相等" );
      }
   }
   cursor.close();
}

function checkConfNotKey ( db, option, fieldName )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, option );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      assert.equal( obj[fieldName], undefined );
   }
   cursor.close();
}
