/******************************************************************************
 * @Description   : seqDB-12246:unset存在的字段
 *                  seqDB-12247:inc更新不存在的字段
 *                  seqDB-12248:push往数组对象中追加指定值
 *                  seqDB-12249:pull清除指定数组对象中指定的值
 *                  seqDB-12250:push_all给数组字段增加多个值
 *                  seqDB-12251:pop删除指定数组字段的最后N个元素  
 *                  seqDB-12252:addtoset往指定数组字段中增加一个值        
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12246_12247_12248_12249_12250_12251_12252";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   varCL.insert( { a: 2, b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf"] }, c: "jkdi" } );

   varCL.update( { $unset: { c: "jkdi" } } );
   var actual = varCL.find( { c: "jkdi" } );
   commCompareResults( actual, [] );

   varCL.update( { $inc: { salary: 100 } } );
   var actual1 = varCL.find();
   commCompareResults( actual1, [{ a: 2, b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf"] }, salary: 100 }] );

   varCL.update( { $push: { "b.phone": 3 } } );
   var actual2 = varCL.find();
   commCompareResults( actual2, [{ a: 2, b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf", 3] }, salary: 100 }] )

   varCL.update( { $pull: { "b.phone": 3 } } );
   var actual3 = varCL.find( { "b.phone.3": 3 } );
   commCompareResults( actual3, [] );

   varCL.update( { $push_all: { array: [3, 4] } } );
   var actual4 = varCL.find();
   commCompareResults( actual4, [{ a: 2, array: [3, 4], b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf"] }, salary: 100 }] );

   varCL.update( { $pull_all: { array: [3, 4] } } );
   var actual5 = varCL.find( { array: [] } );
   commCompareResults( actual5, [{ a: 2, array: [], b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf"] }, salary: 100 }] );

   varCL.update( { $pop: { "b.phone": 2 } } );
   var actual6 = varCL.find( { "b.phone": [12] } );
   commCompareResults( actual6, [{ a: 2, array: [], b: { name: "YoYo", age: 23, phone: [12] }, salary: 100 }] );

   varCL.update( { $addtoset: { "b.phone": [6] } } );
   var actual7 = varCL.find();
   commCompareResults( actual7, [{ a: 2, array: [], b: { name: "YoYo", age: 23, phone: [12, 6] }, salary: 100 }] )
}