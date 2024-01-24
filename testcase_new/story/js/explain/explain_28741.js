/******************************************************************************
 * @Description   : seqDB-28741:查询指定sort执行访问计划，指定参数Expand为true
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.12.09
 * @LastEditTime  : 2022.12.14
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28741";
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   var docs = [];
   var recsNum = 1000;
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i } );
   }
   dbcl.insert( docs );
   var cursor = dbcl.find().sort( { a: 1 } ).explain( { Expand: true } );

   var expResult = ["TBSCAN", 1];
   var Operator = cursor.current().toObj()["PlanPath"]["ChildOperators"][0]["PlanPath"]["ChildOperators"][0]["Operator"];
   var Filter = cursor.current().toObj()["PlanPath"]["ChildOperators"][0]["PlanPath"]["ChildOperators"][0]["Estimate"]["Filter"]["MthSelectivity"];
   var actResult = [Operator, Filter];

   assert.equal( actResult, expResult );
}