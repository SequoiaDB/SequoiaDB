/************************************************************************
*@Description: seqDB-1974:$aggregate使用数组查询
*@Author:  2015/10/21  huangxiaoni
************************************************************************/
testConf.clName = COMMCLNAME + "_1974";

main( test );

function test( testPara )
{
   testPara.testCL.insert( { a: { b: [1, 2] } } );

   var expResult = [ { "b": 1 } ];
   var cursor = testPara.testCL.aggregate( { $project: { b: "$a.b.$[0]" } } );
   commCompareResults ( cursor, expResult );
}
