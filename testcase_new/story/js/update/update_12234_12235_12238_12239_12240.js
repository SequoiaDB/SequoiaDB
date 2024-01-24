/******************************************************************************
 * @Description   : seqDB-12234:$addtoset更新非数组对象
 *                  seqDB-12235:$pull更新数组对象中非数组元素
 *                  seqDB-12238:$push更新非数组对象
 *                  seqDB-12239:push_all操作对象不为数组类型时清除数组中不存在的元素
 *                  seqDB-12240:pop删除指定数组元素
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.08.05
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12234_12235_12238_12239_12240";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   varCL.insert( { a: [1, 2], salary: 100 } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $addtoset: { b: 2 } } );
   } );

   var docs1 = [];
   docs1.push( { a: [1, 2], salary: 100 } );
   varCL.update( { $pull: { "a.0": 1 } } );
   var cursor1 = varCL.find();
   commCompareResults( cursor1, docs1 );

   varCL.update( { $push: { salary: 1 } } );
   var cursor2 = varCL.find();
   commCompareResults( cursor2, docs1 );



   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $pull_all: { a: 3 } } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $push_all: { a: 2 } } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.update( { $pop: { a: [2] } } );
   } );

   var cursor3 = varCL.find();
   commCompareResults( cursor3, docs1 );
}