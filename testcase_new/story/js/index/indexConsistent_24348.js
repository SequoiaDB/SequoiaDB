/******************************************************************************
 * @Description   : seqDB-24348:创建索引后执行切分操作 
 * @Author        : liuli
 * @CreateTime    : 2021.09.14
 * @LastEditTime  : 2022.01.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_24348";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash" };

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dstGroup = args.dstGroupNames;
   var indexName = "index_24348";
   var dbcl = args.testCL;
   if( dstGroup.length < 2 )
   {
      return;
   }

   // 插入大量数据
   insertBulkData( dbcl, 10000 );

   // 创建索引，校验索引任务和一致性
   dbcl.createIndex( indexName, { c: 1 } );
   checkIndexTask( "Create index", COMMCSNAME, COMMCLNAME + "_24348", indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24348", indexName, true );

   // 对cl进行100%切分
   dbcl.split( srcGroup, dstGroup[0], 100 );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24348", indexName, true );

   // 将cl切分到多个组
   dbcl.split( dstGroup[0], dstGroup[1], 50 );
   dbcl.split( dstGroup[0], srcGroup, 30 );
   commCheckIndexConsistent( db, COMMCSNAME, COMMCLNAME + "_24348", indexName, true );
}