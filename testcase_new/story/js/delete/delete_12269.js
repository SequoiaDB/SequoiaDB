/******************************************************************************
 * @Description   : seqDB-12269:lt匹配符购造条件，执行删除
 * @Author        : Zhang Yanan
 * @CreateTime    : 2015.01.27
 * @LastEditTime  : 2021.07.19
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12269";

main( test );

function test ( args )
{
   var varCL = args.testCL;
   var rowcnt = 100;
   var docs = [];
   for( var i = 0; i < rowcnt; i++ )
   {
      docs.push( { a: rowcnt - i, b: i, c: "abcdefghijkl" + i } );
   }
   varCL.insert( docs );

   varCL.remove( { b: { $lt: 50 } } );
   docs.splice( 0, 50 );
   var cursor = varCL.find();
   commCompareResults( cursor, docs );
}