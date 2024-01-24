/************************************
*@Description: use multiply:{field:{$multiply:xx}}
               1.one or two fields cover all type of numberic data,include numberic and Non-numberic;seqDB-8223
               2.multiply positive/negative int;seqDB-5661
               3.multiply positive/negative float;seqDB-5662/seqDB-5665
               4.cover all type of Non-numberic;seqDB-5666
               5.field is Non-existent;seqDB-5668
               6.multiply a number exceed the type of range,seqDB-5663/seqDB-5664/seqDB-5669
               7.multiply Non-numberic,seqDB-5667
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
   var doc = [{ a: 256, b: 256, c: -2147483648, d: 2147483647, e: 256, f: 256 },
   { a: 256.256, b: 256.256, c: { $numberLong: "-9223372036854775808" }, d: { $numberLong: "9223372036854775807" }, e: 256.256, f: 256.256 },
   { a: 0, b: 0, c: -1.7E+308, d: 1.7E+308, e: -1.7E+308, f: 1.7E+308 },
   { a: -256, b: -256, c: { $numberLong: "9223372036854775807" }, d: { $numberLong: "-9223372036854775808" }, e: -256, f: -256 },
   { a: -256.256, b: -256.256, e: -256.256, f: -256.256 },
   { a: { $decimal: "-1.7E+308" }, c: { $decimal: "-9223372036854775808" } },
   { a: { $decimal: "-4.9E-324" }, c: { $decimal: "9223372036854775807" } },
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

   /*1.one or two fields cover all type of numberic data,include numberic and Non-numberic;seqDB-8223
     2.multiply positive/negative int;seqDB-5661
     3.multiply positive/negative float;seqDB-5662/seqDB-5665
     4.cover all type of Non-numberic;seqDB-5666
     5.field is Non-existent;seqDB-5668
     7.multiply a number exceed the type of range,seqDB-5663/seqDB-5664/seqDB-5669*/
   var selectCondition1 = {
      a: { $multiply: 255 },
      b: { $multiply: 255.255 },
      c: { $multiply: 255 },
      d: { $multiply: 255.255 },
      e: { $multiply: -255 },
      f: { $multiply: -255.255 },
      g: { $multiply: 25 }
   };
   var expRecs1 = [{ a: 65280, b: 65345.28, c: -547608330240, d: 548155938314.985, e: -65280, f: -65345.28 },
   { a: 65345.27999999999, b: 65410.62527999999, c: { "$decimal": "-2351959869397967831040" }, d: 2.354311829267366e+21, e: -65345.27999999999, f: -65410.62527999999 },
   { a: 0, b: 0, c: -Infinity, d: Infinity, e: Infinity, f: -Infinity },
   { a: -65280, b: -65345.28, c: { "$decimal": "2351959869397967830785" }, d: -2.354311829267366e+21, e: 65280, f: 65345.28 },
   { a: -65345.27999999999, b: -65410.62527999999, e: 65345.27999999999, f: 65410.62527999999 },
   {
      a: { $decimal: "-43350000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" },
      c: { $decimal: "-2351959869397967831040" }
   },
   {
      a: { $decimal: "-0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000012495" },
      c: { $decimal: "2351959869397967830785" }
   },
   { a: { $decimal: "43350000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" } },
   { a: { $decimal: "0.0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000012495" } },
   { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: null }, { a: [null, 0, null] }, { a: null }, { a: null }, { a: null }, { a: null }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );

   //multiply Non-numberic,seqDB-5667
   var selectCondition4 = { a: { $multiply: "a" } };
   InvalidArgCheck( dbcl, null, selectCondition4, -6 );
}