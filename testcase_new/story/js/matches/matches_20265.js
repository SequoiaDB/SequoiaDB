/******************************************************************************
@Description: [seqDB-20265] Query by $elemMatch with index
              使用$elemMatch查询, 走索引查询
@Author: 2020/08/05 Zixian Yan
******************************************************************************/
testConf.clName = COMMCLNAME + "_20265";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_20265";

   var data = [{ "data": { "a": 1, "b": 2, "c": 2 } },
   { "data": { "a": 1, "b": 2, "c": 1 } },
   { "data": { "a": 5, "b": 2, "c": 1 } },
   { "data": { "a": 1, "b": 2, "c": 3 } },
   { "data": [{ "a": 1, "b": 2 }, { "age": 18, "name": "yzx" }] },
   { "data": [{ "a": 2, "b": 4 }, { "age": 23, "name": "wmt" }] }];

   cl.insert( data );

   cl.createIndex( indexName, { "data": 1 } );

   var rc = cl.find( { "data": { $elemMatch: { a: 1, b: 2 } } } ).hint( { "": indexName } );
   var expectation = [{ "data": [{ "a": 1, "b": 2 }, { "age": 18, "name": "yzx" }] },
   { "data": { "a": 1, "b": 2, "c": 1 } },
   { "data": { "a": 1, "b": 2, "c": 2 } },
   { "data": { "a": 1, "b": 2, "c": 3 } }];

   checkRec( rc, expectation );
}
