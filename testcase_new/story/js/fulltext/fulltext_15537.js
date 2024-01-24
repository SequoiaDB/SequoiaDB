/************************************
*@Description: 单键全文索引，插入全文索引字段为数组类型的记录，全量同步/增量同步到ES 
*@author:      liuxiaoxuan
*@createdate:  2018.10.10
*@testlinkCase: seqDB-15537
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_15537";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // 创建全文索引前插入数据
   var doc = [{ a: ["arr1"] },
   { a: ["arr1", "arr2", "arr3"] },
   { a: [{ "subobj": "value" }, { "k": "v" }, { "a": "b" }] },
   { a: [1, 1.001, 3000000000, { $decimal: "123.456" }] },
   { a: [{ "$date": "2019-09-01" }, { "$timestamp": "2019-09-01-13.14.26.124233" }] },
   { a: [{ 2: "obj1" }, { 3: "obj2" }, { 4: "obj3" }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 3: 4 }] },
   { a: { 1: "obj1", 2: "obj2", 3: "obj3" } },
   { a: [{ "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, { "$regex": "^abc", "$options": "i" }] },
   { a: [true, null, { "$oid": "123abcd00ef12358902300ef" }, { "$minKey": 1 }, { "$maxKey": 1 }] },
   { a: ["abc", { "o": "m" }, { "p": "q" }, "def"] },
   { a: ["hjk", { "o": 1 }, { "p": 3.2 }, { "q": 3000000000 }, { "r": { $decimal: "123.456" } }, { "s": null }, "def"] },
   { a: ["lmn", { "w": { "$date": "2019-09-01" } }, { "x": null }, { "y": { "$binary": "qe91", "$type": "1" } }, { "z": true }, "opq"] },
   { a: [{ "abc": "abc" }, { "def": "def" }, 1, 3.2, 3000000000, { $decimal: "123.456" }, null, true] },
   { a: [{ "hjk": "hjk" }, { "$date": "2019-09-01" }, { "$timestamp": "2019-09-01-13.14.26.124233" }, { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }] },
   { a: [{ "lmn": "lmn" }, { "$regex": "^abc", "$options": "i" }, { "$oid": "123abcd00ef12358902300ef" }, { "$minKey": 1 }, { "$maxKey": 1 }] },
   { a: ["brr1", "123", { "a": "a" }, 1, 3.22] },
   { a: ["crr1", null, true] },
   { a: ["drr1", { "$oid": "123abcd00ef1235890233456" }, { "$regex": "^opq", "$options": "i" }, { $decimal: "567.089" }] },
   { a: ["err1", { "$date": "2019-10-01" }, { "$timestamp": "2019-10-01-13.14.26.124233" }] },
   { a: ["frr1", { "$minKey": 1 }, { "$maxKey": 1 }, { "$binary": "re81", "$type": "1" }] },
   { a: ["grr1", { "$minKey": 1 }, { "x": "y" }] },
   { a: ["hrr1", { "$regex": "^zzz", "$options": "i" }, null] },
   { a: [{ "$binary": "qe91", "$type": "1" }, { "$date": "2019-11-01" }, { "$timestamp": "2019-11-01-13.14.26.124233" }, { "a": "b" }, "abc", true, 10000000000] }
   ];

   dbcl.insert( doc );

   var textIndexName = "textIndex_15537";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // 全文检索，检查结果
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );
   var expectRecords = [{ a: ["arr1"] },
   { a: ["arr1", "arr2", "arr3"] }];
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   // 创建全文索引后再次插入同批数据
   dbcl.insert( doc );

   // 全文检索，检查结果
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );
   expectRecords = [{ a: ["arr1"] },
   { a: ["arr1", "arr2", "arr3"] },
   { a: ["arr1"] },
   { a: ["arr1", "arr2", "arr3"] }];
   actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   // 删除集合中的所有记录，全文检索返回0条
   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );
   actRecords = dbOpr.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   expectRecords = [];
   checkResult( expectRecords, actRecords );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
