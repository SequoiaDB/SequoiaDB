/******************************************************************************
 * @Description   : seqDB-24367:事务中创建/删除本地索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2022.01.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24367";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexName = "index_24367";

   // 开启事务
   db.transBegin();

   // 插入数据后创建本地索引
   insertBulkData( dbcl, 1000 );
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );
   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 提交事务
   db.transCommit();

   // 校验索引
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );

   // 开启事务执行数据操作并删除索引
   db.transBegin();
   insertBulkData( dbcl, 1000 );
   dbcl.dropIndex( indexName );

   // 提交事务后校验索引
   db.transCommit();
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, false );
}