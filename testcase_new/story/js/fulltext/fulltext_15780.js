/************************************
*@Description: 多键全文索引，更新为全部字段为数组，且数组元素均为string 
*@author:      liuxiaoxuan
*@createdate:  2018.10.10
*@testlinkCase: seqDB-15780
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_15780";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   // 创建全文索引前插入数据
   var doc = [{ a: "aaa", b: "bbb", c: "ccc" },
   { a: ["brr1"], b: "abc", c: "def" },
   { a: ["brr1", "brr2", "brr3"], b: "bac", c: "fed" },
   { a: ["arr1", 1], b: ["arr2", 1.001], c: ["arr3", 3000000000], d: "hjk" },
   { a: ["arr1", { "$oid": "123abcd00ef12358902300ef" }], b: "lmn", c: "opt" },
   { a: [{ 0: 1 }, { 1: 2 }, { 3: 4 }, 2, 2.222, -3000000000, { $decimal: "123.456" }], b: "rsv", c: "unw" },
   { a: { "$oid": "123abcd00ef1235890233456" }, b: { "$date": "2019-10-01" }, c: { "$timestamp": "2019-10-01-13.14.26.124233" }, d: null },
   { a: { "$binary": "qe91", "$type": "1" }, b: { "$regex": "^zzz", "$options": "i" }, c: { "a": "b" } }
   ];
   dbcl.insert( doc );

   var textIndexName = "textIndex_15780";
   dbcl.createIndex( textIndexName, { "a": "text", "b": "text", "c": "text", "d": "text" } );

   // 将全部记录更新为数组，且数组元素均为string
   try
   {
      dbcl.update( { $set: { a: ['updatea1', 'updatea2'], b: ['updateb1', 'updateb2'], c: ['updatec1', 'updatec2'], d: ['updated1', 'updated2'] } } );
      throw new Error( "update to all arrays of keys need fail." );
   }
   catch( e )
   {
      if( SDB_IXM_MULTIPLE_ARRAY != e.message )
      {
         throw e;
      }
   }

   // 更新后，检查ES数组是否同步 
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 5 );

   // 全文检索，检查结果
   var dbOpr = new DBOperator();
   var expectResult = [{ a: "aaa", b: "bbb", c: "ccc" },
   { a: ["brr1"], b: "abc", c: "def" },
   { a: ["brr1", "brr2", "brr3"], b: "bac", c: "fed" },
   { a: ["arr1", { "$oid": "123abcd00ef12358902300ef" }], b: "lmn", c: "opt" },
   { a: [{ 0: 1 }, { 1: 2 }, { 3: 4 }, 2, 2.222, -3000000000, { $decimal: "123.456" }], b: "rsv", c: "unw" }
   ];
   var actResult = dbOpr.findFromCL( dbcl, { "": { "$Text": { "query": { "match_all": {} } } } }, { _id: { "$include": 0 } }, { _id: 1 } );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
