/************************************
*@Description: use floor:{field:{$floor:1}}
               1.one field cover all type of data,include numberic and Non-numberic;seqDB-5629/seqDB-5630/seqDB-5631
               2.two fields;seqDB-8218
               3.field is Non-existent;seqDB-5632
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
   { a: { $numberLong: "-9223372036854775808" }, b: { $numberLong: "-2147483649" } },
   { a: { $numberLong: "9223372036854775807" }, b: { $numberLong: "2147483648" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -4.9E-324, b: -4.9 },
   { a: 4.9E-324, b: 4.1 },
   { a: { $decimal: "-1.8E+308" }, b: { $decimal: "-123" } },
   { a: { $decimal: "1.8E+308" }, b: { $decimal: "123" } },
   { a: { $decimal: "-5.9E-324" }, b: { $decimal: "-5.1" } },
   { a: { $decimal: "5.9E-324" }, b: { $decimal: "5.9" } },
   { a: -0 }, { a: -0.0 }, { a: { $decimal: "-0" } }, { a: { $numberLong: "-0" } },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: { $date: "1900-01-01" } },
   { a: { $timestamp: "1902-01-01-00.00.00.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { a: { $regex: "^z", "$options": "i" } },
   { a: { subobj: "value" } },
   { a: ["abc", 2.1, 2.5, "def"] },
   { a: null },
   { a: "abc" },
   { a: MinKey() },
   { a: MaxKey() }];
   dbcl.insert( doc );

   //one field cover all type of data,include numberic and Non-numberic;seqDB-5629/seqDB-5630/seqDB-5631
   var selectCondition1 = { a: { $floor: 1 } };
   var expRecs1 = [{ a: -2147483648, b: -123 },
   { a: 2147483647, b: 123 },
   { a: { $numberLong: "-9223372036854775808" }, b: -2147483649 },
   { a: { $numberLong: "9223372036854775807" }, b: 2147483648 },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -1, b: -4.9 },
   { a: 0, b: 4.1 },
   { a: { $decimal: "-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "-123" } },
   { a: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "123" } },
   { a: { $decimal: "-1" }, b: { $decimal: "-5.1" } },
   { a: { $decimal: "0" }, b: { $decimal: "5.9" } },
   { a: -0 }, { a: -0.0 }, { a: { $decimal: "0" } }, { a: 0 },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 2, 2, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

	/*1.two field;seqDB-8218
	  2.field is Non-existent;seqDB-5632*/
   var selectCondition2 = { a: { $floor: 1 }, b: { $floor: 1 }, c: { $floor: 1 } };
   var expRecs2 = [{ a: -2147483648, b: -123 },
   { a: 2147483647, b: 123 },
   { a: { $numberLong: "-9223372036854775808" }, b: -2147483649 },
   { a: { $numberLong: "9223372036854775807" }, b: 2147483648 },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -1, b: -5 },
   { a: 0, b: 4 },
   { a: { $decimal: "-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "-123" } },
   { a: { $decimal: "180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, b: { $decimal: "123" } },
   { a: { $decimal: "-1" }, b: { $decimal: "-6" } },
   { a: { $decimal: "0" }, b: { $decimal: "5" } },
   { a: -0 }, { a: -0.0 }, { a: { $decimal: "0" } }, { a: 0 },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 2, 2, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { _id: 1 } );

   /*3.placeholder set Non-1*/
   selectCondition3 = { a: { $ceiling: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition3, -6 );
}