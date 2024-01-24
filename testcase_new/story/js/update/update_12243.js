/******************************************************************************
 * @Description   : seqDB-12243:set存在和不存在的字段
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12243";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var docs = [];
   docs.push( { a: [1, 2], salary: 10, name: "Mike", age: 20 } );
   docs.push( { a: [1, 2], salary: 100 } );
   docs.push( { a: [1, 2], salary: 10, name: "Tom" } );
   varCL.insert( docs );

   varCL.update( { $set: { age: 25 } }, { age: { $exists: 1 } } );

   var rc = varCL.find( { age: 25 } );
   var docs1 = [];
   docs1.push( { a: [1, 2], salary: 10, name: "Mike", age: 25 } );
   commCompareResults( rc, docs1 );

   varCL.update( { $set: { age: 30 } }, null );
   var actual = varCL.find();
   commCompareResults( actual, [{ a: [1, 2], salary: 10, name: "Mike", age: 30 },
   { a: [1, 2], salary: 100, age: 30 },
   { a: [1, 2], salary: 10, name: "Tom", age: 30 }] );
}