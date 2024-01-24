/***************************************************************************
@Description :seqDB-15955:指定自增字段为嵌套字段，插入嵌套类型的记录 
@Modify list :
              2018-10-19  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15955";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 10;
   var acquireSize = 1;
   var fieldName = "id.a.b.c";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize } } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var mainclSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: mainclSequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expIncrementArr );

   var expR = [];
   var j = 1;
   for( var i = 0; i < 100; i++ )
   {
      var doc = { a: i };
      dbcl.insert( doc );
      expR.push( { a: i, id: { a: { b: { c: i + 1 } } } } );
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var arr = [{ id: { a: { b: { c: 1.25 } } }, a: "float" },
   { id: { a: { b: { c: { $decimal: "123" } } } }, a: "decimal" },
   { id: { a: { b: { c: "string" } } }, a: "string" },
   { id: { a: { b: { c: { a: 1 } } } }, a: "obj" },
   { id: { a: { b: { c: { $date: "2012-01-01" } } } }, a: "date" },
   { id: { a: { b: { c: { $timestamp: "2012-01-01-13.14.26.124233" } } } }, a: "timestamp" },
   { id: { a: { b: { c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } } }, a: "binary" },
   { id: { a: { b: { c: { $regex: "a", $options: "i" } } } }, a: "regex" },
   { id: { a: { b: { c: { $oid: "123abcd00ef12358902300ef" } } } }, a: "oid" },
   { id: { a: { b: { c: [1, 2, 3] } } }, a: "arr" },
   { id: { a: { b: { c: null } } }, a: "null" },
   { id: { a: { b: { c: { $maxKey: 1 } } } }, a: "maxKey" },
   { id: { a: { b: { c: { $minKey: 1 } } } }, a: "minkey" }];
   expR = expR.concat( arr );
   dbcl.insert( arr );
   var actR = dbcl.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   var arr = [{ id: { a: { b: { d: 1 } } } },
   { id: { a: { b: { c: { d: 1 } } } } },
   { id: { a: { b: { c: 1, d: 1 } } } },
   { id: { a: { b: { c: { d: 1, c: 1 } } } } }];
   dbcl.insert( arr );
   var expArr = [{ id: { a: { b: { d: 1, c: 101 } } } },
   { id: { a: { b: { c: { d: 1 } } } } },
   { id: { a: { b: { c: 1, d: 1 } } } },
   { id: { a: { b: { c: { d: 1, c: 1 } } } } }];
   expR = expR.concat( expArr );
   var actR = dbcl.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   insertOtherTypeDatas( dbcl, [{ id: { a: { b: 1 } } }] )

   commDropCL( db, COMMCSNAME, clName, true, true );
}
