/***************************************************************************
@Description :seqDB-17979 :编目节点组通过选举切主后，创建全文索引
@Modify list :
              2019-4-26  YinZhen  Create
****************************************************************************/
//SEQUOIADBMAINSTREAM-4575
//main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_17979";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // 插入数据
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );

   // 重新选主
   var rg = db.getCatalogRG();
   var masNode = rg.getMaster();
   var nodeName = masNode.getHostName() + ":" + masNode.getServiceName();
   var obj = db.snapshot( 13, { "NodeName": nodeName } ).current().toObj();
   var weight = obj["weight"];
   db.updateConf( { "weight": weight - 5 }, { "NodeName": nodeName } );

   var doTimes = 0;
   while( true )
   {
      println( "masNode: " + masNode.getNodeDetail() );
      rg.reelect( { Seconds: 60 } );
      println( "REELECTING... " + rg.getMaster().getNodeDetail() );
      doTimes++;
      if( ( rg.getMaster().getNodeDetail() != masNode.getNodeDetail() ) || doTimes > 10 )
      {
         break;
      }
   }

   commCreateIndex( dbcl, "fullIndex_17979", { b: "text" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_17979", 10000 );
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   db.updateConf( { "weight": weight }, { "NodeName": nodeName } );
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_17979" );
   commDropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
