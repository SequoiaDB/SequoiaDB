/************************************
*@Description: 单键全文索引，更新数组为数组元素分为非string的数组 
*@author:      liuxiaoxuan
*@createdate:  2019.08.15
*@testlinkCase: seqDB-19142
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_19142";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // 创建全文索引前插入数据
   var doc = [{ a: ["arr1"] },
   { a: ["arr1", "arr2", "arr3"] },
   { a: [{ "subobj": "value" }, { "k": "v" }, { "a": "b" }] },
   { a: [1, 1.001, 3000000000, { $decimal: "123.456" }] },
   { a: ["abc", { "o": "m" }, { "p": "q" }, "def"] },
   { a: ["hjk", { "o": 1 }, { "p": 3.2 }, { "q": 3000000000 }, { "r": { $decimal: "123.456" } }, { "s": null }, "def"] },
   { a: ["brr1", "123", { "a": "a" }, 1, 3.22] },
   { a: ["crr1", null, true] },
   { a: ["drr1", { "$oid": "123abcd00ef1235890233456" }, { "$regex": "^opq", "$options": "i" }, { $decimal: "567.089" }] },
   { a: ["err1", { "$date": "2019-10-01" }, { "$timestamp": "2019-10-01-13.14.26.124233" }] },
   { a: ["frr1", { "$minKey": 1 }, { "$maxKey": 1 }, { "$binary": "re81", "$type": "1" }] }
   ];
   dbcl.insert( doc );

   var textIndexName = "textIndex_19142";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // 更新全文索引字段为多个数组元素(部分为string)的记录
   dbcl.update( { "$set": { a: [9000000000, { "k": "v" }, { $decimal: "567.089" }, 3, null, true, { "$oid": "123abcd00ef1235890233456" }] } }, { a: { "$exists": 1 } } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   // 检查全文检索结果
   var dbOpr = new DBOperator();
   expResult = [];
   actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
