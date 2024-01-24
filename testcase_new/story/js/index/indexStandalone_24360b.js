/******************************************************************************
 * @Description   : seqDB-24360:主表创建/删除本地索引（指定多个子表所在节点）
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true
main( test );
function test ()
{
   var mainclName = "index_maincl_24360b";
   var subclName1 = "index_subcl1_24360b";
   var subclName2 = "index_subcl2_24360b";
   var indexName = "index_24360b";
   var recordNum = 2000;

   commDropCL( db, COMMCSNAME, mainclName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainclName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, ReplSize: 0 } );
   commCreateCL( db, COMMCSNAME, subclName1 );
   commCreateCL( db, COMMCSNAME, subclName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subclName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( COMMCSNAME + "." + subclName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   insertBulkData( maincl, recordNum );

   var groupName1 = commGetCLGroups( db, COMMCSNAME + "." + subclName1 );
   var groupName2 = commGetCLGroups( db, COMMCSNAME + "." + subclName2 );
   var nodeName1 = getCLOneNodeName( db, COMMCSNAME + "." + subclName1 );
   var nodeName2 = getCLOneNodeName( db, COMMCSNAME + "." + subclName2 );
   println( "----node1=" + nodeName1 + "---group1=" + groupName1[0] );
   println( "----node2=" + nodeName2 + "---group2=" + groupName2[0] );
   //创建本地索引
   maincl.createIndex( indexName, { no: 1, a: 1 }, { Standalone: true }, { NodeName: [nodeName1, nodeName2] } );

   if( groupName1[0] == groupName2[0] )
   {
      checkStandaloneIndexOnNodes( db, COMMCSNAME, subclName1, indexName, [nodeName1, nodeName2], true );
      checkStandaloneIndexOnNodes( db, COMMCSNAME, subclName2, indexName, [nodeName1, nodeName2], true );
      checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName1, nodeName1, indexName );
      checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName2, nodeName1, indexName );
      checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName1, nodeName2, indexName );
      checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName2, nodeName2, indexName );
      checkExplainUseStandAloneIndex( COMMCSNAME, subclName1, nodeName1, { a: 10, no: 10 }, "ixscan", indexName );
      checkExplainUseStandAloneIndex( COMMCSNAME, subclName1, nodeName2, { a: 10, no: 10 }, "ixscan", indexName );
      checkExplainUseStandAloneIndex( COMMCSNAME, subclName2, nodeName1, { a: 1000, no: 1000 }, "ixscan", indexName );
      checkExplainUseStandAloneIndex( COMMCSNAME, subclName2, nodeName2, { a: 1000, no: 1000 }, "ixscan", indexName );
   }
   else
   {
      //子表1和子表2不在一个组，则分别在各自组上指定节点创建索引
      checkStandaloneIndexOnNode( db, COMMCSNAME, subclName1, indexName, nodeName1, true );
      checkStandaloneIndexOnNode( db, COMMCSNAME, subclName2, indexName, nodeName2, true );
      checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName1, nodeName1, indexName );
      checkStandaloneIndexTask( "Create index", COMMCSNAME + "." + subclName2, nodeName2, indexName );
      checkExplainUseStandAloneIndex( COMMCSNAME, subclName1, nodeName1, { a: 10, no: 10 }, "ixscan", indexName );
      checkExplainUseStandAloneIndex( COMMCSNAME, subclName2, nodeName2, { a: 1000, no: 1000 }, "ixscan", indexName );
   }

   //删除本地索引
   maincl.dropIndex( indexName );
   checkStandaloneIndexTask( "Drop index", COMMCSNAME + "." + subclName1, nodeName1, indexName );
   checkStandaloneIndexTask( "Drop index", COMMCSNAME + "." + subclName2, nodeName2, indexName );
   commCheckIndexConsistent( db, COMMCSNAME, subclName1, indexName, false );
   commCheckIndexConsistent( db, COMMCSNAME, subclName2, indexName, false );
   checkExplainUseStandAloneIndex( COMMCSNAME, subclName1, nodeName1, { a: 10, no: 10 }, "tbscan", "" );
   commDropCL( db, COMMCSNAME, mainclName, true, true, "Fail to drop CL in the ending" );
}

