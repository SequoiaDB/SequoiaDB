/************************************
*@Description: use cast
*@author:      zhaoyu
*@createdate:  2016.10.18
*@testlinkCase:cover all testcast in testlink
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, a: -2147483648, b: -2147483648 },
   { No: 2, a: { $numberLong: "-9223372036854775808" }, b: { $numberLong: "-9223372036854775808" } },
   { No: 3, a: { $decimal: "-92233720368547758089223372036854775808" }, b: { $decimal: "-92233720368547758089223372036854775808" } },
   { No: 4, a: -1.7E+308, b: -1.7E+308 },
   { No: 5, a: "string", b: "string" },
   { No: 6, a: { $oid: "573920accc332f037c000013" }, b: { $oid: "573920accc332f037c000013" } },
   { No: 7, a: false, b: false },
   { No: 8, a: true, b: true },
   { No: 9, a: { $date: "2016-05-16" }, b: { $date: "2016-05-16" } },
   { No: 10, a: { $timestamp: "2016-05-16-13.14.26.124233" }, b: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { No: 11, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { No: 12, a: { $regex: "^z", $options: "i" }, b: { $regex: "^z", $options: "i" } },
   { No: 13, a: { name: "hanmeimei" }, b: { name: "hanmeimei" } },
   { No: 14, a: ["b", 0], b: ["b", 0] },
   { No: 15, a: null, b: null },
   { No: 16, a: MaxKey(), b: MaxKey() },
   { No: 17, a: MinKey(), b: MinKey() }];
   dbcl.insert( doc );

   var expect1 = [{ No: 1, a: -2147483648, b: -2147483648 },
   { No: 2, a: { $numberLong: "-9223372036854775808" }, b: { $numberLong: "-9223372036854775808" } },
   { No: 3, a: { $decimal: "-92233720368547758089223372036854775808" }, b: { $decimal: "-92233720368547758089223372036854775808" } },
   { No: 4, a: -1.7E+308, b: -1.7E+308 },
   { No: 5, a: "string", b: "string" },
   { No: 6, a: { $oid: "573920accc332f037c000013" }, b: { $oid: "573920accc332f037c000013" } },
   { No: 7, a: false, b: false },
   { No: 8, a: true, b: true },
   { No: 9, a: { $date: "2016-05-16" }, b: { $date: "2016-05-16" } },
   { No: 10, a: { $timestamp: "2016-05-16-13.14.26.124233" }, b: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { No: 11, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { No: 12, a: { $regex: "^z", $options: "i" }, b: { $regex: "^z", $options: "i" } },
   { No: 13, a: { name: "hanmeimei" }, b: { name: "hanmeimei" } },
   { No: 14, a: ["b", 0], b: ["b", 0] },
   { No: 15, a: null, b: null },
   { No: 16, a: { $maxKey: 1 }, b: { $maxKey: 1 } },
   { No: 17, a: { $minKey: 1 }, b: { $minKey: 1 } }];
   var condition1 = { a: { $cast: "minkey", $et: { $minKey: 1 } }, b: { $cast: "minkey", $et: { $minKey: 1 } } };
   checkResult( dbcl, condition1, null, expect1, { _id: 1 } );

   var condition2 = { a: { $cast: -1, $et: { $minKey: 1 } }, b: { $cast: -1, $et: { $minKey: 1 } } };
   checkResult( dbcl, condition2, null, expect1, { _id: 1 } );

   var condition3 = { a: { $cast: "MINKEY", $et: { $minKey: 1 } }, b: { $cast: "MINKEY", $et: { $minKey: 1 } } };
   checkResult( dbcl, condition3, null, expect1, { _id: 1 } );

   expect4 = [];
   var condition4 = { a: { $cast: -1, $et: { $minKey: 1 } }, c: { $cast: -1, $et: { $minKey: 1 } } };
   checkResult( dbcl, condition4, null, expect4, { _id: 1 } );

   var condition5 = { a: { $cast: "maxkey", $et: { $maxKey: 1 } }, b: { $cast: "maxkey", $et: { $maxKey: 1 } } };
   checkResult( dbcl, condition5, null, expect1, { _id: 1 } );

   var condition6 = { a: { $cast: 127, $et: { $maxKey: 1 } }, b: { $cast: 127, $et: { $maxKey: 1 } } };
   checkResult( dbcl, condition6, null, expect1, { _id: 1 } );

   var condition7 = { a: { $cast: "MAXKEY", $et: { $maxKey: 1 } }, b: { $cast: "MAXKEY", $et: { $maxKey: 1 } } };
   checkResult( dbcl, condition7, null, expect1, { _id: 1 } );

   var condition8 = { a: { $cast: 127, $et: { $maxKey: 1 } }, c: { $cast: 127, $et: { $maxKey: 1 } } };
   checkResult( dbcl, condition8, null, expect4, { _id: 1 } );
}

