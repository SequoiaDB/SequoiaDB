/******************************************************************************
 * @Description   : seqDB-23604:cl.alter，配置RepairCheck:false，做数据操作
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2021.03.05
 * @LastEditTime  : 2021.03.08
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_23604_2";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "ShardingType": "range" };

main( test );
function test ( testPara )
{
   var srcRgName = testPara.srcGroupName;
   var dstRgName = testPara.dstGroupNames[0];
   var cl = testPara.testCL;

   cl.insert( { "a": 1 } );
   cl.alter( { "RepairCheck": true } );

   cl.split( srcRgName, dstRgName, 50 );
   assert.equal( cl.count(), 1 );
}