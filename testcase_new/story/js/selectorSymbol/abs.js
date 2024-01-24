/************************************
*@Description: use abs:{field:{$abs:1}}
               1.one field cover all type of data,include numberic and Non-numberic;seqDB-5625/seqDB-5626
               2.two fields;seqDB-8217
               3.field is Non-existent;seqDB-5627
               4.placeholder set Non-1;seqDB-5628
*@author:      zhaoyu
*@createdate:  2016.7.15
*@testlinkCase:
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ a: -2147483648, b: -123 },
   { a: 2147483647, b: 123 },
   { a: -2147483649 },
   { a: 2147483648 },
   { a: { $numberLong: "-9223372036854775807" }, b: { $numberLong: "-2147483649" } },
   { a: { $numberLong: "9223372036854775807" }, b: { $numberLong: "2147483648" } },
   { a: { $numberLong: "9223372036854775808" } },
   { a: -1.7E+308, b: -17 },
   { a: 1.7E+308, b: 17 },
   { a: -1.8E+308 },
   { a: 1.8E+308 },
   { a: -4.9E-324, b: -4.9 },
   { a: 4.9E-324, b: 4.9 },
   { a: -5.9E-324 },
   { a: 5.9E-324 },
   { a: { $decimal: "-1.8E+308" }, b: { $decimal: "-1.8E+308" } },
   { a: { $decimal: "1.8E+308" }, b: { $decimal: "1.8E+308" } },
   { a: { $decimal: "-5.9E-324" }, b: { $decimal: "-5.9E-324" } },
   { a: { $decimal: "5.9E-324" }, b: { $decimal: "5.9E-324" } },
   { a: -0 }, { a: -0.0 }, { a: { $decimal: "-0" } }, { a: { $numberLong: "-0" } },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: { $date: "1900-01-01" } },
   { a: { $timestamp: "1902-01-01-00.00.00.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { a: { $regex: "^z", "$options": "i" } },
   { a: { subobj: "value" } },
   { a: ["abc", 0, "def"] },
   { a: null },
   { a: "abc" },
   { a: MinKey() },
   { a: MaxKey() }];
   dbcl.insert( doc );

   //one field cover all type of data,include numberic and Non-numberic;seqDB-5625/seqDB-5626
   var selectCondition1 = { a: { $abs: 1 } };
   var expRecs1 = [{ a: 2147483648, b: -123 },
   { a: 2147483647, b: 123 },
   { a: 2147483649 },
   { a: 2147483648 },
   { a: { $numberLong: "9223372036854775807" }, b: -2147483649 },
   { a: { $numberLong: "9223372036854775807" }, b: 2147483648 },
   { a: { $numberLong: "9223372036854775807" } },
   { a: 1.7E+308, b: -17 },
   { a: 1.7E+308, b: 17 },
   { a: 1.8E+308 },
   { a: 1.8E+308 },
   { a: 4.9E-324, b: -4.9 },
   { a: 4.9E-324, b: 4.9 },
   { a: 5.9E-324 },
   { a: 5.9E-324 },
   { a: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" }, b: { $decimal: "-0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" }, b: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" } },
   { a: 0 }, { a: 0.0 }, { a: { $decimal: "0" } }, { a: 0 },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 0, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

	/*1.two fields;seqDB-8217
	  2.field is Non-existent;seqDB-5627*/
   var selectCondition2 = { a: { $abs: 1 }, b: { $abs: 1 }, c: { $abs: 1 } };
   var expRecs2 = [{ a: 2147483648, b: 123 },
   { a: 2147483647, b: 123 },
   { a: 2147483649 },
   { a: 2147483648 },
   { a: { $numberLong: "9223372036854775807" }, b: 2147483649 },
   { a: { $numberLong: "9223372036854775807" }, b: 2147483648 },
   { a: { $numberLong: "9223372036854775807" } },
   { a: 1.7E+308, b: 17 },
   { a: 1.7E+308, b: 17 },
   { a: 1.8E+308 },
   { a: 1.8E+308 },
   { a: 4.9E-324, b: 4.9 },
   { a: 4.9E-324, b: 4.9 },
   { a: 5.9E-324 },
   { a: 5.9E-324 },
   { a: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" }, b: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" }, b: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000059" } },
   { a: 0 }, { a: 0.0 }, { a: { $decimal: "0" } }, { a: 0 },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 0, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { _id: 1 } );

   /*3.placeholder set Non-1;seqDB-5628*/
   selectCondition3 = { a: { $abs: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );

}