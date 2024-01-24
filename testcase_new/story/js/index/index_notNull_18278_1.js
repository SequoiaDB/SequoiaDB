/******************************************************************************
*@Description : seqDB-18278:创建复合索引，指定NotNull，索引键字段覆盖所有数据类型 
*               cover all type
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl_18278_1";
   var indexName = "idx";
   var indexKey = { num: 1, a: 1, b: -1, c: 1, d: -1, e: 1, f: -1, g: 1, h: -1, i: 1, j: -1, k: 1, l: -1, m: 1, n: -1, o: 1, p: -1 };
   var recs1 = [{ num: 1, a: 2147483647, b: 9223372036854775807, c: 1.7E+308, d: { $decimal: "123.456" }, e: "test", f: { obj: "" }, g: { $oid: "100000009010000000901000" }, h: true, i: { $date: "2019-01-01" }, j: { $timestamp: "2019-01-01-01.00.00.000000" }, k: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" }, l: { $regex: "^a", $options: "i" }, m: [1, 2], n: { $minKey: 1 }, o: { $maxKey: 1 }, p: "lastFieldExistAndNotNull" }];
   var recs2 =
      [{ num: 2, a: 2147483647, b: 9223372036854775807, c: 1.7E+308, d: { $decimal: "123.456" }, e: "test", f: { obj: "" }, g: { $oid: "100000009010000000901000" }, h: true, i: { $date: "2019-01-01" }, j: { $timestamp: "2019-01-01-01.00.00.000000" }, k: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" }, l: { $regex: "^a", $options: "i" }, m: [1, 2], n: { $minKey: 1 }, o: { $maxKey: 1 }, p: null },
      { num: 3, a: 2147483647, b: 9223372036854775807, c: 1.7E+308, d: { $decimal: "123.456" }, e: "test", f: { obj: "" }, g: { $oid: "100000009010000000901000" }, h: true, i: { $date: "2019-01-01" }, j: { $timestamp: "2019-01-01-01.00.00.000000" }, k: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" }, l: { $regex: "^a", $options: "i" }, m: [1, 2], n: { $minKey: 1 }, o: { $maxKey: 1 } }];

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );

   /**************************** test1, create composite index[ NotNull:true ] -> insert **********************/
   var NotNull = true;
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );

   cl.insert( recs1 );
   for( i = 0; i < recs2.length; i++ ) 
   {
      assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
      {
         cl.insert( recs2[i] );
      } );
   }

   checkIndex( cl, indexName, NotNull );
   checkRecords( cl, recs1 );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test2, create composite index[ NotNull:false ] -> insert **********************/
   var NotNull = false;
   cl.createIndex( indexName, indexKey, { NotNull: NotNull } );
   checkIndex( cl, indexName, NotNull );

   cl.insert( recs1 );
   checkRecords( cl, recs1 );

   cl.remove();

   cl.insert( recs2 );
   checkRecords( cl, recs2 );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /****************** test3, create composite index[ NotNull:true ] -> insert[ a:null/exist ] ******************/
   var NotNull = true;
   cl.createIndex( indexName, { a: 1, b: -1 }, { NotNull: NotNull } );

   var invRecs = [{ b: 1 }, { a: null, b: 2 }];
   for( i = 0; i < invRecs.length; i++ ) 
   {
      assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
      {
         cl.insert( invRecs[i] );
      } );
   }

   checkIndex( cl, indexName, NotNull );
   var cnt = cl.count();
   assert.equal( cnt, 0 );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false );
}

function checkIndex ( cl, indexName, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actNot = indexDef.NotNull;
   assert.equal( actNot, expNot );
}

function checkRecords ( cl, expRecs ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   assert.equal( expRecs, actRecs );
}