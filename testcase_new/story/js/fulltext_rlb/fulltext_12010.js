/************************************
*@Description: replSize设置不为1时插入/更新/删除记录
*@author:      liuxiaoxuan
*@createdate:  2019.08.21
*@testlinkCase: seqDB-12010
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ()
{
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; i++ )
   {
      var group = groups[i];
      if( group.length > 2 )
      {
         break;
      }
   }
   var groupName = group[0].GroupName;

   var clName = COMMCLNAME + "_ES_12010";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 7 } );

   // 创建全文索引，插入数据
   var textIndexName = "textIndex_12010";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   dbcl.insert( { a: 'insertBeforeNodeStop' } );

   // 检查数据同步
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   // 停止其中一个节点，插入记录
   var preSlave = db.getRG( groupName ).getSlave();
   try
   {
      preSlave.stop();

      assert.tryThrow( [SDB_CLS_NODE_NOT_ENOUGH, SDB_CLS_WAIT_SYNC_FAILED], function()
      {
         dbcl.insert( { a: 'insertAfterNodeStop' } );
      } );

      assert.tryThrow( [SDB_CLS_NODE_NOT_ENOUGH, SDB_CLS_WAIT_SYNC_FAILED], function()
      {
         dbcl.update( { $set: { a: 'updateAfterNodeStop' } } );
      } );

      assert.tryThrow( [SDB_CLS_NODE_NOT_ENOUGH, SDB_CLS_WAIT_SYNC_FAILED], function()
      {
         dbcl.remove();
      } );
   }
   finally
   {
      preSlave.start();
      // 节点起来后，检查数据组所有节点LSN是否一致
      checkGroupBusiness( 120, COMMCSNAME, clName );
   }

   // 节点起来后，再次插入一条记录
   dbcl.insert( { a: 'insertAfterNodeStart' } );

   // 检查数据同步
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, dbcl.count() );
   checkConsistency( COMMCSNAME, clName );

   var actCount = dbcl.find( { "": { "$Text": { "query": { "match_all": {} } } } } ).count();
   var expectCount = dbcl.count();
   if( parseInt( actCount ) != parseInt( expectCount ) )
   {
      throw new Error( "expect count: " + parseInt( expectCount ) + ", actual count: " + parseInt( actCount ) );
   }

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
