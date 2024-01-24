/***************************************************************************
@Description : 13 bson type data inserted. Then create index .
@Modify list :
              2014-5-21  xiaojun Hu  Init
              2016-3-4   yan wu Modify(增加结果检测（查看访问计划是否走索引、走索引查询数据是否正确）)
****************************************************************************/
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false );

   // insert data to SDB
   idxCL.insert( { number: 24523453, longint: 2147483647000, floatNum: 12345.456 } );
   idxCL.insert( { string: "field_value", objectOID: { "$oid": "123abcd00ef12358902300ef" } } );
   idxCL.insert( { floatNum: 123e+50, bool: true, date: { "$date": "2014-5-21" } } );
   idxCL.insert( { timestamp: { "$timestamp": "2014-5-21-9.17.30.111111" } } );
   idxCL.insert( { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   idxCL.insert( { regex: { "$regex": "^张" }, regex: "张" } );
   idxCL.insert( { object: { "subobj": "can't" } } );
   idxCL.insert( { array: ["abc", 123, "def", "噆"], NULL: null } );
   var i = 0;
   do
   {
      var count = idxCL.count();
      ++i;
   } while( i < 15 )
   assert.equal( 8, count );

   //createIndex
   createIndex( idxCL, "numberIdx", { number: 1 } );
   createIndex( idxCL, "longinIdx", { longint: 1 } );
   createIndex( idxCL, "floatNumIdx", { floatNum: 1 } );
   createIndex( idxCL, "stringIdx", { string: 1 } );
   createIndex( idxCL, "objIDIdx", { "objectOID": -1 } );
   createIndex( idxCL, "boolIdx", { bool: -1 } );
   createIndex( idxCL, "dateIdx", { "date": -1 } );
   createIndex( idxCL, "timestampIdx", { "timestamp": -1 } );
   createIndex( idxCL, "binaryIdx", { "binary": -1 } );
   createIndex( idxCL, "objIdx", { "object": -1 } );
   createIndex( idxCL, "arrayIdx", { "array": -1 } );
   createIndex( idxCL, "nullIdx", { "NULL": -1 } );
   createIndex( idxCL, "regexIdx", { "regex": -1 } );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "numberIdx", "number", 1, false, false );
      inspecIndex( idxCL, "longinIdx", "longint", 1, false, false );
      inspecIndex( idxCL, "floatNumIdx", "floatNum", 1, false, false );
      inspecIndex( idxCL, "stringIdx", "string", 1, false, false );
      inspecIndex( idxCL, "objIDIdx", "objectOID", -1, false, false );
      inspecIndex( idxCL, "boolIdx", "bool", -1, false, false );
      inspecIndex( idxCL, "dateIdx", "date", -1, false, false );
      inspecIndex( idxCL, "timestampIdx", "timestamp", -1, false, false );
      inspecIndex( idxCL, "binaryIdx", "binary", -1, false, false );
      inspecIndex( idxCL, "objIdx", "object", -1, false, false );
      inspecIndex( idxCL, "arrayIdx", "array", -1, false, false );
      inspecIndex( idxCL, "nullIdx", "NULL", -1, false, false );
      inspecIndex( idxCL, "regexIdx", "regex", -1, false, false );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   //test find by index 
   checkExplain( idxCL, { number: 24523453 }, "ixscan", "numberIdx" );
   checkExplain( idxCL, { longint: 2147483647000 }, "ixscan", "longinIdx" );
   checkExplain( idxCL, { floatNum: 123e+50 }, "ixscan", "floatNumIdx" );
   checkExplain( idxCL, { string: "field_value" }, "ixscan", "stringIdx" );
   checkExplain( idxCL, { objectOID: { "$oid": "123abcd00ef12358902300ef" } }, "ixscan", "objIDIdx" );
   //checkExplain( idxCL, {bool:true} );
   checkExplain( idxCL, { date: { "$date": "2014-5-21" } }, "ixscan", "dateIdx" );
   checkExplain( idxCL, { timestamp: { "$timestamp": "2014-5-21-9.17.30.111111" } }, "ixscan", "timestampIdx" );
   checkExplain( idxCL, { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, "ixscan", "binaryIdx" );
   checkExplain( idxCL, { object: { "subobj": "can't" } }, "ixscan", "objIdx" );
   checkExplain( idxCL, { array: "abc" }, "ixscan", "arrayIdx" );
   //checkExplain( idxCL, {array:["abc",123,"def","噆"]} );
   checkExplain( idxCL, { NULL: null }, "ixscan", "nullIdx" );
   checkExplain( idxCL, { regex: { "$regex": "^张" } }, "ixscan", "regexIdx" );

   //check the result of find  
   checkResult( idxCL, { number: 24523453 } );
   checkResult( idxCL, { longint: 2147483647000 } );
   checkResult( idxCL, { floatNum: 123e+50 } );
   checkResult( idxCL, { string: "field_value" } );
   checkResult( idxCL, { objectOID: { "$oid": "123abcd00ef12358902300ef" } } );
   checkResult( idxCL, { bool: true } );
   checkResult( idxCL, { date: { "$date": "2014-05-21" } } );
   checkResult( idxCL, { timestamp: { "$timestamp": "2014-05-21-09.17.30.111111" } } );
   checkResult( idxCL, { binary: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   checkResult( idxCL, { array: ["abc", 123, "def", "噆"] } );
   checkResult( idxCL, { NULL: null } );

   //check the result of find by regex type
   var rc = idxCL.find( { regex: { "$regex": "^张" } } );
   var expRecs = [];
   expRecs.push( { regex: "张" } );
   checkRec( rc, expRecs );

   // drop collectionspace in clean
   commDropCL( db, csName, clName, false, false );
}
