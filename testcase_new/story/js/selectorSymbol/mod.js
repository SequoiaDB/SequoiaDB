/************************************
*@Description: use mod:{field:{$mod:xx}}
               1.one or two fields cover all type of numberic data,include numberic and Non-numberic;seqDB-5639/seqDB-5640/seqDB-8220
               2.mod double;seqDB-5642
               3.cover all type of Non-numberic;seqDB-5643
               4.field is Non-existent;seqDB-5644
               5.mod 0,seqDB-5641
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
   var doc = [{ a: -2147483648, b: -2147483648, c: -2147483648, d: -2147483648 },
   { a: 2147483647, b: 2147483647, c: 2147483647, d: 2147483647 },
   { a: { $numberLong: "-9223372036854775808" }, b: { $numberLong: "-9223372036854775808" }, c: { $numberLong: "-9223372036854775808" }, d: { $numberLong: "-92233720368547" } },
   { a: { $numberLong: "9223372036854775807" }, b: { $numberLong: "9223372036854775807" }, c: { $numberLong: "9223372036854775807" }, d: { $numberLong: "92233720368547" } },
   { a: -2147483647.13, b: -2147483647.13, c: -2147483647.13, d: -2147483647.13 },
   { a: 2147483647.13, b: 2147483647.13, c: 2147483647.13, d: 2147483647.13 },
   { a: { $decimal: "-9223372036854775809922337203685" }, b: { $decimal: "-9223372036854775809922337203685" }, c: { $decimal: "-9223372036854775809922337203685" }, d: { $decimal: "-9223372036854775809922337203685" } },
   { a: { $decimal: "-9223372036854775809922337203685.13" }, b: { $decimal: "-9223372036854775809922337203685.13" }, c: { $decimal: "-9223372036854775809922337203685.13" }, d: { $decimal: "-9223372036854775809922337203685.13" } },
   { a: { $decimal: "9223372036854775809922337203685" }, b: { $decimal: "9223372036854775809922337203685" }, c: { $decimal: "9223372036854775809922337203685" }, d: { $decimal: "9223372036854775809922337203685" } },
   { a: { $decimal: "9223372036854775809922337203685.13" }, b: { $decimal: "9223372036854775809922337203685.13" }, c: { $decimal: "9223372036854775809922337203685.13" }, d: { $decimal: "9223372036854775809922337203685.13" } },
   { a: 0 },
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

   /*1.one or two fields cover all type of numberic data,include numberic and Non-numberic;seqDB-5639/seqDB-5640/seqDB-8220
     2.mod double;seqDB-5642
     3.cover all type of Non-numberic;seqDB-5643
     4.field is Non-existent;seqDB-5644*/
   var selectCondition1 = { a: { $mod: 256 }, b: { $mod: { $numberLong: "-922337203685477" } }, c: { $mod: { $decimal: "9223372036854775808" } }, d: { $mod: 3.256 }, e: { $mod: 2 } };
   var expRecs1 = [{ a: 0, b: -2147483648, c: { $decimal: "-2147483648" }, d: -3.056000232696533 },
   { a: 255, b: 2147483647, c: { $decimal: "2147483647" }, d: 2.056000232696533 },
   { a: 0, b: -5808, c: { $decimal: "0" }, d: -2.265625 },
   { a: 255, b: 5807, c: { $decimal: "9223372036854775807" }, d: 2.265625 },
   { a: -255.1300001144409, b: -2147483647.13, c: { $decimal: "-2147483647.13" }, d: -2.186000347137451 },
   { a: 255.1300001144409, b: 2147483647.13, c: { $decimal: "2147483647.13" }, d: 2.186000347137451 },
   { a: { $decimal: "-229" }, b: { $decimal: "-275899115090823" }, c: { $decimal: "-1922337203685" }, d: { $decimal: "-0.600" } },
   { a: { $decimal: "-229.13" }, b: { $decimal: "-275899115090823.13" }, c: { $decimal: "-1922337203685.13" }, d: { $decimal: "-0.730" } },
   { a: { $decimal: "229" }, b: { $decimal: "275899115090823" }, c: { $decimal: "1922337203685" }, d: { $decimal: "0.600" } },
   { a: { $decimal: "229.13" }, b: { $decimal: "275899115090823.13" }, c: { $decimal: "1922337203685.13" }, d: { $decimal: "0.730" } },
   { a: 0 },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 0, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

   //mod 0,seqDB-5641
   var selectCondition2 = { a: { $mod: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition2, -6 );
}