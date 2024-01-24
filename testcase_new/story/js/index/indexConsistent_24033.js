/******************************************************************************
 * @Description   : seqDB-24033:catalog上存在cl，data上不存在cl，执行写操作
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_24033";
testConf.clName = COMMCLNAME + "_24033";
testConf.clOpt = { ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var group = testPara.srcGroupName;
   var csName = COMMCSNAME + "_24033";
   var clName = COMMCLNAME + "_24033";

   // 直连data主节点删除集合
   var data = db.getRG( group ).getMaster().connect();
   var dataCS = data.getCS( csName );
   dataCS.dropCL( clName );

   // 连接coord执行写操作自动生成集合
   insertBulkData( dbcl, 100 );

   commCheckIndexConsistent( db, csName, clName, "$id", true );
   commCheckIndexConsistent( db, csName, clName, "$shard", true );
}