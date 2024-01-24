/************************************
*@Description: 正常停止两个编目节点后再次重启一个节点，并创建全文索引
*@author:      liuxiaoxuan
*@createdate:  2019.05.08
*@testlinkCase: seqDB-18266
**************************************/
// SEQUOIADBMAINSTREAM-4705
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_18266";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_18266 " + i, b: i } );
   }
   dbcl.insert( objs );

   // 停止两个编目节点
   var preCataMaster = db.getRG( "SYSCatalogGroup" ).getMaster();
   var preCataSlave = db.getRG( "SYSCatalogGroup" ).getSlave();
   var preCataMasterNodeName = preCataMaster.getHostName() + ":" + preCataMaster.getServiceName();
   var preCataSlaveNodeName = preCataSlave.getHostName() + ":" + preCataSlave.getServiceName();

   try
   {
      preCataMaster.stop();
      preCataSlave.stop();
      // 重启其中一个节点
      preCataSlave.start();
      // 等待切主，最长等待10min
      var doTimes = 1;
      for( ; doTimes <= 600; doTimes++ )
      {
         var curCataMaster = isMasterNodeExist( "SYSCatalogGroup" );
         var curCataMasterNodeName = curCataMaster.getHostName() + ":" + curCataMaster.getServiceName();
         // 切主后，则退出
         if( preCataMasterNodeName != curCataMasterNodeName ) 
         {
            break;
         }
         sleep( 1000 );
      }

      // 没有切主，则抛异常
      if( doTimes > 600 )
      {
         throw new Error( "changePrimary fail,change primary" + curCataMasterNodeName + preCataMasterNodeName );
      }
   }
   finally
   {
      // 拉起所有编目节点
      preCataSlave.start();
      preCataMaster.start();
   }

   // 检查主备节点LSN是否一致
   checkCatalogBusiness( 120 );

   // 创建全文索引，检查数据同步
   var textIndexName = "textIndex_18266";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // 全文检索
   var findConf = { "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_18266" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findConf, { 'a': '' } );
   var expResult = dbOpr.findFromCL( dbcl, { "b": { "$lt": 10000 } }, { 'a': '' } );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}