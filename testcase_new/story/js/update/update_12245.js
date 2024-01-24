/******************************************************************************
 * @Description   : seqDB-12245:set更新数组字段为嵌套对象类型
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12245";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var docs = [];
   docs.push( { a: [1, 2] } );
   varCL.insert( docs );
   varCL.update( { $set: { "a.c": 0 } } );
   var actual = varCL.find();
   commCompareResults( actual, docs );
}