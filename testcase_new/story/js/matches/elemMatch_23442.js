/******************************************************************************
 * @Description   : seqDB-23442 :: $elemMatch条件以非$开头 
 *                  seqDB-23443 :: $elemMatch条件是以$开头的匹配符，匹配数组的非嵌套元素 
 *                  seqDB-23444 :: $elemMatch匹配字段为数组中的数组 
 *                  seqDB-23445 :: $elemMatch匹配多个字段，包含数组的非嵌套元素 
 *                  seqDB-23446 :: $elemMatch条件是以$开头的匹配符，匹配嵌套对象 
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.02.02
 * @LastEditors   : Yu Fan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23442";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 准备数据
   var docs = [{ a: 1, b: [] },
   { a: 2, b: [1, 2, 3] },
   { a: 3, b: [{ b1: 1, b2: 1 }, { b1: 2, b2: 2 }] },
   { a: 4, b: [1, [1, 2, 3]] },
   { a: 5, b: [1, 2, { b1: 1, b2: 2 }, [1, 2, 3]] },
   { a: 6, b: [{ b1: { b2: 1 } }, { b2: 2 }] },
   { a: 7, b: { b1: [1, 2, 3] } },
   { a: 8, b: { b1: 1, b2: { b3: 1 }, b3: [1, 2, 3] } },
   { a: 9, b: [{ "": 1 }] },
   { a: 10, b: [1, 2, 3], c: [1, 2, 3] },
   { a: 11, b: [1, 2, 3], c: [1, 2, 3], d: { e: [1, 2, 3] } }
   ]

   cl.insert( docs );

   // 普通字段
   var cursor = cl.find( { "b": { $elemMatch: { "b1": 1, "b2": 1 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 3, b: [{ b1: 1, b2: 1 }, { b1: 2, b2: 2 }] }] );


   // 空字符串字段
   cursor = cl.find( { "b": { $elemMatch: { "": 1 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 9, b: [{ "": 1 }] }] );

   // 以$开头匹配符
   cursor = cl.find( { b: { $elemMatch: { $et: 1 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 2, b: [1, 2, 3] },
   { a: 4, b: [1, [1, 2, 3]] },
   { a: 5, b: [1, 2, { b1: 1, b2: 2 }, [1, 2, 3]] },
   { "a": 10, "b": [1, 2, 3], "c": [1, 2, 3] },
   { "a": 11, "b": [1, 2, 3], "c": [1, 2, 3], "d": { "e": [1, 2, 3] } }] );

   cursor = cl.find( { b: { $elemMatch: { $in: [1, 2] } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 2, b: [1, 2, 3] }, { a: 4, b: [1, [1, 2, 3]] },
   { a: 5, b: [1, 2, { b1: 1, b2: 2 }, [1, 2, 3]] },
   { "a": 10, "b": [1, 2, 3], "c": [1, 2, 3] }, { "a": 11, "b": [1, 2, 3], "c": [1, 2, 3], "d": { "e": [1, 2, 3] } }] );

   cursor = cl.find( { b: { $elemMatch: { $gt: 1, $lt: 3 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ "a": 2, "b": [1, 2, 3] }, { "a": 4, "b": [1, [1, 2, 3]] }, { "a": 5, "b": [1, 2, { "b1": 1, "b2": 2 }, [1, 2, 3]] }, { "a": 10, "b": [1, 2, 3], "c": [1, 2, 3] }, { "a": 11, "b": [1, 2, 3], "c": [1, 2, 3], "d": { "e": [1, 2, 3] } }] );

   // $elemMatch匹配字段为数组中的数组 
   cursor = cl.find( { "b.3": { $elemMatch: { $gt: 1, $lt: 3 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 5, b: [1, 2, { b1: 1, b2: 2 }, [1, 2, 3]] }] );

   // $elemMatch匹配多个字段，包含数组的非嵌套元素 
   cursor = cl.find( { b: { $elemMatch: { $et: 1 } }, c: { $elemMatch: { $gt: 1, $lt: 3 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 10, b: [1, 2, 3], c: [1, 2, 3] }, { a: 11, b: [1, 2, 3], c: [1, 2, 3], d: { e: [1, 2, 3] } }] );
   //  $elemMatch条件是以$开头的匹配符，匹配嵌套对象 
   cursor = cl.find( { "d": { $elemMatch: { $gt: 1, $lt: 3 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [] );
}

