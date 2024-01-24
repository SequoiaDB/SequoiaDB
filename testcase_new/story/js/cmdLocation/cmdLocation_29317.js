/******************************************************************************
 * @Description   : seqDB-29317:查看explain()中的CMDLocation参数返回值内容
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.13
 * @LastEditTime  : 2022.12.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_29317";
testConf.clOpt = { "ShardingKey": { "a": 1 }, "AutoSplit": true };
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;
   var docs = [];
   var recsNum = 1000;
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i } );
   }
   dbcl.insert( docs );
   var expResult = dbcl.find().explain( { Location: { GroupName: srcGroupName[0] } } );

   var actGroup = expResult.current().toObj()["PlanPath"]["ChildOperators"][0]["GroupName"];
   var actResult = dbcl.find( { a: 1 } ).explain( { CMDLocation: { GroupName: srcGroupName[0] } } );
   expResult.close();
   actResult.close();

   assert.equal( srcGroupName[0], actGroup );
   assert.equal( actResult, expResult );
} 