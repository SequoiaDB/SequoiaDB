/************************************
*@Description: decimal data use $cast
*@author:      zhaoyu
*@createdate:  2016.5.21
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7777";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert all kind of data include decimal
   var doc1 = [{ a: -2147483648 },
   { a: 2147483647 },
   { a: { $numberLong: "-9223372036854775808" } },
   { a: { $numberLong: "9223372036854775807" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -4.9E-324 },
   { a: 4.9E-324 },
   { a: "string" },
   { a: { $oid: "573920accc332f037c000013" } },
   { a: false },
   { a: true },
   { a: { $date: "2016-05-16" } },
   { a: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: { name: "hanmeimei" } },
   { a: ["b", 0] },
   { a: null }];
   dbcl.insert( doc1 );

   //cast decimal
   var expRecs1 = [{ a: { $decimal: "-2147483648" } },
   { a: { $decimal: "2147483647" } },
   { a: { $decimal: "-9223372036854775808" } },
   { a: { $decimal: "9223372036854775807" } },
   { a: { $decimal: "-170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "-0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000494065645841247" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000494065645841247" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "1" } },
   { a: { $decimal: "1463328000000" } },
   { a: { $decimal: "1463375666124" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0" } },
   { a: { $decimal: "0" } },
   { a: [{ $decimal: "0" }, { $decimal: "0" }] },
   { a: { $decimal: "0" } }];
   checkResult( dbcl, null, { a: { $cast: 100 } }, expRecs1, { _id: 1 } );
   //delete data
   dbcl.remove();

   //insert decimal data
   var doc2 = [{ a: { $decimal: "123" } }, { a: { $decimal: "-9223372036854775809" } }, { a: { $decimal: "9223372036854775808", $precision: [100, 2] } }];
   dbcl.insert( doc2 );

   //cast int
   var expRecs2 = [{ a: 123 }, { a: 0 }, { a: 0 }];
   checkResult( dbcl, null, { a: { $cast: "int32" } }, expRecs2, { _id: 1 } );

   //delete data
   dbcl.remove();

   //insert decimal data
   var doc3 = [{ a: { $decimal: "9223372036854775807" } }, { a: { $decimal: "-9223372036854775809" } }, { a: { $decimal: "9223372036854775808", $precision: [100, 2] } }];
   dbcl.insert( doc3 );

   //cast numberLong
   var expRecs4 = [{ a: { $numberLong: "9223372036854775807" } }, { a: 0 }, { a: 0 }];
   checkResult( dbcl, null, { a: { $cast: "int64" } }, expRecs4, { _id: 1 } );

   //delete data
   dbcl.remove();

   //insert decimal data
   var doc5 = [{ a: { $decimal: "-1.7E+308" } }, { a: { $decimal: "1.7E+310" } }, { a: { $decimal: "1.7E+310", $precision: [1000, 2] } }];
   dbcl.insert( doc5 );

   //cast double
   var expRecs5 = [{ a: -1.7E+308 }, { a: 0 }, { a: 0 }];
   checkResult( dbcl, null, { a: { $cast: 1 } }, expRecs5, { _id: 1 } );

   dbcl.remove();
   testSpecialDecimal( dbcl );
   commDropCL( db, COMMCSNAME, clName );
}

function testSpecialDecimal ( dbcl )
{
   //insert specila decimal data
   var doc = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NAN" } },
   { a: { $decimal: "-INF" } },
   { a: { $decimal: "INF" } }];
   dbcl.insert( doc );

   //cast MinKey
   var expRecs = [{ a: { $minKey: 1 } },
   { a: { $minKey: 1 } },
   { a: { $minKey: 1 } },
   { a: { $minKey: 1 } },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, null, { a: { $cast: "minkey" } }, expRecs, { _id: 1 } );

   //cast Double
	/*
	expRecs = [{a:0},
	           {a:0},
	           {a:"nan"},
	           {a:0},
	           {a:0}];
	checkResult( dbcl, null,{a:{$cast:"double"}}, expRecs, {_id:1} );
	*/

   //cast String
   expRecs = [{ a: "MAX" },
   { a: "MIN" },
   { a: "NaN" },
   { a: "MIN" },
   { a: "MAX" }];
   checkResult( dbcl, null, { a: { $cast: "string" } }, expRecs, { _id: 1 } );

   //cast Bool
   expRecs = [{ a: true },
   { a: true },
   { a: true },
   { a: true },
   { a: true }];
   checkResult( dbcl, null, { a: { $cast: "bool" } }, expRecs, { _id: 1 } );

   //cast Date
   expRecs = [{ a: null },
   { a: null },
   { a: null },
   { a: null },
   { a: null }];
   checkResult( dbcl, null, { a: { $cast: "date" } }, expRecs, { _id: 1 } );

   //cast Null
   expRecs = [{ a: null },
   { a: null },
   { a: null },
   { a: null },
   { a: null }];
   checkResult( dbcl, null, { a: { $cast: "null" } }, expRecs, { _id: 1 } );

   //cast Int32
   expRecs = [{ a: 0 },
   { a: 0 },
   { a: 0 },
   { a: 0 },
   { a: 0 }];
   checkResult( dbcl, null, { a: { $cast: "int32" } }, expRecs, { _id: 1 } );

   //cast Timestamp
   expRecs = [{ a: null },
   { a: null },
   { a: null },
   { a: null },
   { a: null }];
   checkResult( dbcl, null, { a: { $cast: "timestamp" } }, expRecs, { _id: 1 } );

   //cast Int64
   expRecs = [{ a: 0 },
   { a: 0 },
   { a: 0 },
   { a: 0 },
   { a: 0 }];
   checkResult( dbcl, null, { a: { $cast: "int64" } }, expRecs, { _id: 1 } );

   //cast MaxKey
   expRecs = [{ a: { $maxKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, null, { a: { $cast: "maxkey" } }, expRecs, { _id: 1 } );
}

