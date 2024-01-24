/******************************************************************************
 * @Description   : seqDB-26374:存在一致性索引/本地索引，重复创建相同本地索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.04.14
 * @LastEditTime  : 2022.04.15
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 } };
testConf.clName = COMMCLNAME + "_indexConsistent_26374";

main( test );
function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;
   var indexName = "Index_26374";
   var standaloneIndexName = "standaloneIndex_26374";
   var recordNum = 200;
   insertBulkData( testPara.testCL, recordNum );

   var masteNode = db.getRG( srcGroupName ).getMaster().toString();
   dbcl.createIndex( indexName, { no: 1 } );
   dbcl.createIndex( standaloneIndexName, { b: 1 }, { Standalone: true }, { NodeName: masteNode } );
   var indexInfos1 = getIndexMetaInfo( dbcl, indexName, masteNode );
   var standaloneIndexInfos1 = getIndexMetaInfo( dbcl, standaloneIndexName, masteNode );

   //再次创建相同索引报错
   assert.tryThrow( SDB_IXM_REDEF, function()
   {
      dbcl.createIndex( indexName, { no: 1 }, { Standalone: true }, { NodeName: masteNode } );
   } );
   assert.tryThrow( SDB_IXM_REDEF, function()
   {
      dbcl.createIndex( standaloneIndexName, { b: 1 }, { Standalone: true }, { NodeName: masteNode } );
   } );

   var indexInfos2 = getIndexMetaInfo( dbcl, indexName, masteNode );
   var standaloneIndexInfos2 = getIndexMetaInfo( dbcl, standaloneIndexName, masteNode );
   checkResult( indexInfos2, indexInfos1 );
   checkResult( standaloneIndexInfos2, standaloneIndexInfos1 );
}

function getIndexMetaInfo ( dbcl, indexName, nodeName )
{
   var cursor = dbcl.snapshotIndexes( { RawData: true, "IndexDef.name": indexName, "NodeName": nodeName } );
   var indexInfos = [];
   while( cursor.next() )
   {
      var actIndexInfo = cursor.current().toObj();
      indexInfos.push( actIndexInfo );
   }
   cursor.close();
   return indexInfos;
}

function checkResult ( actRecs, expRecs )
{
   assert.equal( actRecs.length, expRecs.length, "---actRecs=" + JSON.stringify( actRec )
      + "\n---expRecs=" + JSON.stringify( expRecs ) );
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f], "---actRec=" + JSON.stringify( actRec )
            + "\n---expRec=" + JSON.stringify( expRec ) );
      }
   }
}
