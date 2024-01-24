/******************************************************************************
 * @Description   : seqDB-24035:直连data创建cl
 * @Author        : Yi Pan
 * @CreateTime    : 2021.05.11
 * @LastEditTime  : 2021.09.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_24035";
testConf.clName = COMMCLNAME + "_24035";
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var clName = "cl_24035";
   var csName = COMMCSNAME + "_24035";
   var group = testPara.srcGroupName;

   // 直连数据节点创建CL
   var data = db.getRG( group ).getMaster().connect();
   var dataCS = data.getCS( csName );
   dataCS.createCL( clName, { ShardingKey: { a: 1 } } );

   // 校验 $id和 $shard
   checkIndexExist( data, csName, clName, "$id", true );
   checkIndexExist( data, csName, clName, "$shard", true );
}