/******************************************************************************
 * @Description   : seqDB-12240:pop删除指定数组元素
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/

testConf.clName = COMMCLNAME + "_12240";

main( test );

function test ( args )
{
   var varCL = args.testCL;
   varCL.insert( { a: [1, 2], salary: 100 } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $pop: { a: [2] } } );
   } );
}