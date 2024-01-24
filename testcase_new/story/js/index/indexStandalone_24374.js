/******************************************************************************
 * @Description   : seqDB-24374:直连data节点创建/删除本地索引
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2021.10.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24374";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexName = "index_24374";
   insertBulkData( dbcl, 1000 );

   // 直连数据节点创建本地索引
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );
   var data = new Sdb( nodeName );
   var dataCL = getCL( data, COMMCSNAME, testConf.clName );
   dataCL.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 校验索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );

   // 直连数据节点删除索引
   dataCL.dropIndex( indexName );

   // 校验索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, false );
   data.close();
}