/************************************
*@Description: use divide:{field:{$divide:xx}}
               1.one or two fields cover all type of numberic data,include numberic and Non-numberic;seqDB-8224
               2.divide positive/negative int;seqDB-5670
               3.divide positive/negative float;seqDB-5671
               4.cover all type of Non-numberic;seqDB-5672
               5.field is Non-existent;seqDB-5678
               6.divide a number exceed the type of range,seqDB-5673/seqDB-5674
               7.can be divided,with no remainder,seqDB-5675
               8.can not be divided,with remainder,seqDB-5676
               9.divide Non-numberic,seqDB-5677
               10.divide 0
*@author:      zhaoyu
*@createdate:  2016.7.19
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
   var doc = [{ a: 2147483647, b: 2147483647, c: 2147483647, d: 2147483647, e: -2147483648, f: 2147483647, i: 125, j: 100 },
   { a: 2147483647.256, b: 2147483647.256, c: 2147483647.256, d: 2147483647.256, e: { $numberLong: "-9223372036854775808" }, f: { $numberLong: "9223372036854775807" } },
   { a: 0, b: 0, c: 0, d: 0, e: -1.7E+308, f: 1.7E+308, g: -4.9E-324, h: 4.9E-324 },
   { a: -2147483648, b: -2147483648, c: -2147483648, d: -2147483648 },
   { a: -2147483648.256, b: -2147483648.256, c: -2147483648.256, d: -2147483648.256 },
   { a: { $decimal: "-1.7E+308" }, d: { $decimal: "-9223372036854775808" } },
   { a: { $decimal: "-4.9E-324" }, d: { $decimal: "9223372036854775807" } },
   { a: { $decimal: "1.7E+308" } },
   { a: { $decimal: "4.9E-324" } },
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

   /*1.one or two fields cover all type of numberic data,include numberic and Non-numberic;seqDB-8224
     2.divide positive/negative int;seqDB-5670
     3.divide positive/negative float;seqDB-5671
     4.cover all type of Non-numberic;seqDB-5672
     5.field is Non-existent;seqDB-5678
     6.divide a number exceed the type of range,seqDB-5673/seqDB-5674
     7.can be divided,with no remainder,seqDB-5675
     8.can not be divided,with remainder,seqDB-5676 */
   var selectCondition1 = {
      a: { $divide: 255 },
      b: { $divide: -255 },
      c: { $divide: 255.255 },
      d: { $divide: -255.255 },
      e: { $divide: 0.0025 },
      f: { $divide: 0.0025 },
      g: { $divide: 25 },
      h: { $divide: 25 },
      i: { $divide: 5 },
      j: { $divide: 3 },
      k: { $divide: 3 }
   };
   var expRecs1 = [{ a: 8421504, b: -8421504, c: 8413091.406632584, d: -8413091.406632584, e: -858993459200, f: 858993458800, i: 25, j: 33 },
   { a: 8421504.499043137, b: -8421504.499043137, c: 8413091.407635503, d: -8413091.407635503, e: -3.68934881474191e+21, f: 3.68934881474191e+21 },
   { a: 0, b: 0, c: 0, d: 0, e: -Infinity, f: Infinity, g: 0, h: 0 },
   { a: -8421504, b: 8421504, c: -8413091.410550235, d: 8413091.410550235 },
   { a: -8421504.502964705, b: 8421504.502964705, c: -8413091.411553154, d: 8413091.411553154 },
   {
      a: { $decimal: "-666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666667" },
      d: { $decimal: "36133952466571764.737" }
   },
   {
      a: { $decimal: "-0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001921568627450980392" },
      d: { $decimal: "-36133952466571764.733" }
   },
   { a: { $decimal: "666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666667" } },
   { a: { $decimal: "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001921568627450980392" } },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 0, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

   //divide Non-numberic,seqDB-5677
   var selectCondition4 = { a: { $divide: "a" } };
   InvalidArgCheck( dbcl, null, selectCondition4, -6 );

   //divide 0
   var selectCondition4 = { a: { $divide: 0 } };
   InvalidArgCheck( dbcl, null, selectCondition4, -6 );
}