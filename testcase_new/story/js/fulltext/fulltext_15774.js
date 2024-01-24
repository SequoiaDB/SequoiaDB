/************************************
*@Description: 更新全文索引字段为1个数组元素的记录 
*@author:      liuxiaoxuan
*@createdate:  2018.10.10
*@testlinkCase: seqDB-15774
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_15774";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // 创建全文索引前插入数据
   var doc = [{ a: ["arr1"] },
   { a: "arr1" },
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

   var textIndexName = "textIndex_15774";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // 更新至只有1个元素的数组，且元素类型为string
   dbcl.update( { "$set": { a: ["updated"] } }, { a: { "$isnull": 0 } } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 12 );

   // 检查全文检索结果
   expResult = [];
   for( var i = 0; i < dbcl.count(); i++ )  
   {
      expResult.push( { a: ["updated"] } );
   }
   actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { "_id": { "$include": 0 } }, { _id: 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
