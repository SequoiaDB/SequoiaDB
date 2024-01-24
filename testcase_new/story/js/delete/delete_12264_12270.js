/******************************************************************************
 * @Description   : seqDB-12264:空的集合执行删除操作
 *                  seqDB-12270:不带条件，删除所有记录
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.27
 * @LastEditTime  : 2021.07.19
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12264_12270";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   varCL.remove();
   var cursor = varCL.find();
   commCompareResults( cursor, [] );

   var docs = [];
   docs.push( { a: 1 } );
   docs.push( { b: [1, 2], salary: 10, name: "Tom" } );

   varCL.insert( docs );
   varCL.remove();
   var cursor = varCL.find();
   commCompareResults( cursor, [] );
}