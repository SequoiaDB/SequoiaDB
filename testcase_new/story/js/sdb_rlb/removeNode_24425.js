/******************************************************************************
 * @Description   : seqDB-24425:数据组存在集合，指定Enforced删除数据组最后一个节点
 * @Author        : liuli
 * @CreateTime    : 2021.10.15
 * @LastEditTime  : 2021.10.19
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupName = "group_24425";
   var csName = "cs_24425";
   var clName = "cl_24425";

   commDropCS( db, csName );
   var dbcs = commCreateCS( db, csName );
   var groupNames = commGetDataGroupNames( db );
   var groupsArray = commGetGroups( db );
   var hostName = groupsArray[0][1].HostName;
   var port = parseInt( RSRVPORTBEGIN ) + 10;
   var dbpath = RSRVNODEDIR + "data/" + port;

   try
   {
      var dataRG = db.createRG( groupName );
      dataRG.createNode( hostName, port, dbpath, { diaglevel: 5 } );
      dataRG.start();

      // 在group_24425上创建CL
      var dbcl = dbcs.createCL( clName, { Group: groupName } );

      // 指定Enforced为true强制删除节点
      assert.tryThrow( SDB_CATA_RM_NODE_FORBIDDEN, function()
      {
         dataRG.removeNode( hostName, port, { Enforced: true } );
      } );

      var docs = [];
      for( var i = 0; i < 20000; i++ )
      {
         docs.push( { a: i, b: i, c: i } );
      }
      dbcl.insert( docs );

      // 指定Enforced为true强制删除节点
      assert.tryThrow( SDB_CATA_RM_NODE_FORBIDDEN, function()
      {
         dataRG.removeNode( hostName, port, { Enforced: true } );
      } );

      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );

      dbcs.dropCL( clName );
      var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, Group: groupNames[0] } );
      dbcl.insert( docs );

      // 切分后指定Enforced为true强制删除节点
      var taskId = dbcl.splitAsync( groupNames[0], groupName, 50 );
      assert.tryThrow( SDB_CATA_RM_NODE_FORBIDDEN, function()
      {
         dataRG.removeNode( hostName, port, { Enforced: true } );
      } );

      // 任务结束后校验数据
      db.waitTasks( taskId );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   finally
   {
      commDropCS( db, csName );
      db.removeRG( groupName );
   }
}