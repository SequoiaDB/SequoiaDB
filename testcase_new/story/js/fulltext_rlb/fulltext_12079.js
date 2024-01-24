/************************************
*@Description: 集合中存在全文索引，修改普通集合的副本数
*@author:      liuxiaoxuan
*@createdate:  2019.08.21
*@testlinkCase: seqDB-12079
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;
// SEQUOIADBMAINSTREAM-5420
// main( test );

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

   var clName = COMMCLNAME + "_ES_12079";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );

   // 创建全文索引，插入数据
   var textIndexName = "textIndex_12079";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   var objs = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "test_12079 " + i, b: i } );
   }
   dbcl.insert( objs );

   // 修改普通集合的副本数为3
   db.getCS( COMMCSNAME ).getCL( clName ).alter( { ReplSize: 3 } );

   // 停止其中一个节点，插入记录
   var preSlave = db.getRG( groupName ).getSlave();
   try
   {
      preSlave.stop();
      dbcl.insert( { a: 'aaaaaaaaaaaaaaaa' } );
      throw new Error( "should insert fail" );
   }
   catch( e )
   {
      if( e.message != SDB_CLS_NODE_NOT_ENOUGH && e.message != SDB_CLS_WAIT_SYNC_FAILED )
      {
         throw e;
      }
   }
   finally
   {
      preSlave.start();
      // 节点起来后，检查数据组所有节点LSN是否一致
      checkGroupBusiness( 120, COMMCSNAME, clName );
   }

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
