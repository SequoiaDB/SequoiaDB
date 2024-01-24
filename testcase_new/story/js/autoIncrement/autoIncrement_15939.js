/***************************************************************************
@Description :seqDB-15939 :指定自增字段为任意类型插入记录
@Modify list :
              2018-10-16  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_15939";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: 1 } } );

   var doc = [{ id: -2147483648, a: "int" },
   { a: "numberLong", id: { $numberLong: "9223372036854775807" } },
   { id: 1.25, a: "float" },
   { id: { $decimal: "123" }, a: "decimal" },
   { id: "string", a: "string" },
   { id: { a: 1 }, a: "obj" },
   { id: { $date: "2012-01-01" }, a: "date" },
   { id: { $timestamp: "2012-01-01-13.14.26.124233" }, a: "timestamp" },
   { id: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, a: "binary" },
   { id: { $regex: "a", $options: "i" }, a: "regex" },
   { id: { $oid: "123abcd00ef12358902300ef" }, a: "oid" },
   { id: [1, 2, 3], a: "arr" },
   { id: null, a: "null" },
   { id: { $maxKey: 1 }, a: "maxKey" },
   { id: { $minKey: 1 }, a: "minkey" }];
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { _id: 1 } );
   checkRec( actR, doc );

   commDropCL( db, COMMCSNAME, clName, true, true );

}
