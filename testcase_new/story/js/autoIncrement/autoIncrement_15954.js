/***************************************************************************
@Description :seqDB-15954 :Generated设置为strict，插入记录
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

   var clName = COMMCLNAME + "_15954";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 10;
   var acquireSize = 1;
   var fieldName = "id";
   var generated = "strict";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Generated: generated } } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var mainclSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: mainclSequenceName, Generated: generated }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expIncrementArr );

   var expR = [];
   var currentValue = 0;
   for( var i = 0; i < 100; i++ )
   {
      if( i % 2 === 0 )
      {
         currentValue = i + 10;
         var doc = { a: i, id: currentValue };
         dbcl.insert( doc );
         expR.push( doc );
      } else
      {
         dbcl.insert( { a: i } );
         expR.push( { a: i, id: currentValue + 1 } );
      }
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = { id: { $numberLong: "9223372036854775807" }, a: "numberLong" };
   expR.push( { id: { $numberLong: "9223372036854775807" }, a: "numberLong" } );
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { _id: 1 } );
   checkRec( actR, expR );

   var arr = [{ id: 1.25, a: "float" },
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
   insertOtherTypeDatas( dbcl, arr );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
