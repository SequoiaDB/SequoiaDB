/******************************************************************************
 * @Description   : seqDB-24360:主表创建/删除本地索引（指定其中一个子表所在节点）
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2022.01.21
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var mainclName = "index_maincl_24360a";
   var subclName1 = "index_subcl1_24360a";
   var subclName2 = "index_subcl2_24360a";
   var indexName = "index_24360a";
   var recordNum = 2000;

   commDropCL( db, COMMCSNAME, mainclName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainclName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, ReplSize: 0 } );
   commCreateCL( db, COMMCSNAME, subclName1 );
   commCreateCL( db, COMMCSNAME, subclName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subclName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( COMMCSNAME + "." + subclName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   var expRecs = insertBulkData( maincl, recordNum );

   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + subclName1 );
   //创建本地索引
   maincl.createIndex( indexName, { no: 1, a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   checkStandaloneIndexOnNode( db, COMMCSNAME, subclName1, indexName, nodeName, true );
   checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName1, nodeName, indexName );
   checkExplainUseStandAloneIndex( COMMCSNAME, subclName1, nodeName, { a: 10, no: 10 }, "ixscan", indexName );

   //删除本地索引
   maincl.dropIndex( indexName );
   checkStandaloneIndexTask( "Drop index", COMMCSNAME + "." + subclName1, nodeName, indexName );
   checkStandaloneIndexOnNode( db, COMMCSNAME, subclName1, indexName, nodeName, false );
   checkExplainUseStandAloneIndex( COMMCSNAME, subclName1, nodeName, { a: 10, no: 10 }, "tbscan", "" );
   commDropCL( db, COMMCSNAME, mainclName, true, true, "Fail to drop CL in the ending" );
}

