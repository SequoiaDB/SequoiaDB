/******************************************************************************
 * @Description   : seqDB-23767:空主表创建/复制/删除索引  
 * @Author        : wu yan
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2021.04.02
 * @LastEditors   : wuyan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_maincl_index23767";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };

main( test );
function test ( testPara )
{
   var indexName = "Index_23767";
   var maincl = testPara.testCL;
   maincl.createIndex( indexName, { b: 1 } );

   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName] );

   assert.tryThrow( SDB_MAINCL_NOIDX_NOSUB, function()
   {
      maincl.copyIndex();
   } );
   checkNoTask( COMMCSNAME, testConf.clName, "Copy index" );

   maincl.dropIndex( indexName );
   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, [indexName] );
}
