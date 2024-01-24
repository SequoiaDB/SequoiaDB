/******************************************************************************
 * @Description   : seqDB-23457 :: $elemMatch作为匹配符或选择符，同时匹配多种类型的数组元素或嵌套对象的多个字段 
 *                  seqDB-23458 :: $elemMatchOne作为选择符，同时匹配多种类型的数组元素或嵌套对象的多个字段 
 *                  seqDB-23459 :: $elemMatchOne作为选择符+$elemMatch作为匹配符+其他匹配符，在选择条件中组合嵌套使用  
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.20
 * @LastEditTime  : 2021.01.20
 * @LastEditors   : Yu Fan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23457";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 准备数据
   var docs = [{ a: 1, b: [] },
   { a: 2, b: [5, "test1"] },
   { a: 3, b: [{ "$date": "0000-01-01" }, { b1: 1, b2: 1 }, { b1: 2, b2: 2 }] },
   { a: 4, b: [1, [1, 2, 3], { "$timestamp": "2037-12-31-23.59.59.999999" }] },
   {
      "a": 5, "b": [{ "c": "1", "d": 18, e: [{ f: 1 }, { g: 0 }] },
      { "c": "2", "d": 19, e: [{ f: 1 }, { g: 1 }] },
      { "c": "3", "d": 18, e: [{ f: 0 }, { g: 0 }] }]
   },
   { a: 6, b: [1, 2, { b1: 1, b2: 2, b3: 3 }, [1, 2, 3]] },
   { a: 7, b: { b1: [1, 2, 3] } },
   { a: 8, b: { b1: 1, b2: [{ c1: 1, c2: 2 }, { c1: 1, c2: 2 }], b3: [1, 2, 3] } },
   { a: 9, b: [{ "": 1 }] },
   { a: 10, b: [{ "$date": "2021-01-01" }, { "$date": "2021-01-01" }, 2, 3], c: [1, 2, 2, 3] },
   { a: 11, b: [{ "$timestamp": "2036-12-31-23.59.59.999999" }, { "$timestamp": "2036-12-31-23.59.59.999999" }, 2, 3], c: [1, 2, 2, 3], d: { e: [1, 2, 3] } },
   { a: 12, b: ["test2", "test2"] }
   ]

   cl.insert( docs );

   // $elemMatch作为匹配符或选择符，同时匹配多种类型的数组元素或嵌套对象的多个字段 
   // 多种类型的数组元数
   var cursor = cl.find( { "b": { $elemMatch: { $in: ["test1", { "$date": "0000-01-01" }, { "$timestamp": "2037-12-31-23.59.59.999999" }] } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [
      { a: 2, b: [5, "test1"] },
      { a: 3, b: [{ "$date": "0000-01-01" }, { b1: 1, b2: 1 }, { b1: 2, b2: 2 }] },
      { a: 4, b: [1, [1, 2, 3], { "$timestamp": "2037-12-31-23.59.59.999999" }] }
   ] );
   // 嵌套对象多个字段
   cursor = cl.find( { "b": { $elemMatch: { b1: 1, b2: 2, b3: 3 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 6, b: [1, 2, { b1: 1, b2: 2, b3: 3 }, [1, 2, 3]] }] );

   // $elemMatchOne作为选择符，同时匹配多种类型的数组元素或嵌套对象的多个字段 
   // 多种类型数组元数
   cursor = cl.find( { a: { $gte: 10, $lte: 12 } }, { b: { $elemMatchOne: { $in: [{ "$date": "2021-01-01" }, { "$timestamp": "2036-12-31-23.59.59.999999" }, "test2"] } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [
      { a: 10, b: [{ "$date": "2021-01-01" }], c: [1, 2, 2, 3] },
      { a: 11, b: [{ "$timestamp": "2036-12-31-23.59.59.999999" }], c: [1, 2, 2, 3], d: { e: [1, 2, 3] } },
      { a: 12, b: ["test2"] }
   ] );
   // 嵌套对象多个字段
   cursor = cl.find( { a: 8 }, { "b.b2": { $elemMatchOne: { c1: 1, c2: 2 } } } ).sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 8, b: { b1: 1, b2: [{ c1: 1, c2: 2 }], b3: [1, 2, 3] } }] );

   // $elemMatchOne作为选择符+$elemMatch作为匹配符+其他匹配符，在选择条件中组合嵌套使用 
   cursor = cl.find( { a: 5 }, { b: { $elemMatchOne: { e: { $elemMatch: { f: 1 } } } } } )
   commCompareResults( cursor, [{ "a": 5, "b": [{ "c": "1", "d": 18, "e": [{ "f": 1 }, { "g": 0 }] }] }] );
}

