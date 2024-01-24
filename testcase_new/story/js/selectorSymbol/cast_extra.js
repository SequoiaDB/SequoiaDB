/************************************
*@Description: use cast:{field:<$cast:Value>}, 
               1.string of numberic type, cast double/int/long/decimal;
               2.int/int64/double/decimal,value is 0, cast bool;
	            3.string of date/timestamp type, cast data/timestamp;
	            4.double/int64/decimal in range int, cast int
               5.decimal/double in range numberLong, cast int64
               6.string of json format, cast object;
               7.string with 24 character,cast objectId
*@author:      zhaoyu
*@createdate:  2016.7.18
*@testlinkCase:
***************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ a: "-1.7E+308", b: "-2147483648", c: "-9223372036854775808", d: "-92233720368547758089223372036854775808", e: 0, f: "2016-7-18", g: "2016-7-18-13.14.26.124233" },
   { a: "1.7E+308", b: "2147483647", c: "9223372036854775807", d: "92233720368547758079223372036854775807", e: { $numberLong: "0" } },
   { a: "-4.9e-324", d: "-92233720368547758089223372036854775808.99", e: 0.0 },
   { a: "4.9e-324", d: "92233720368547758079223372036854775807.99", e: { $decimal: "0" } },
   { a: { $decimal: "-5.9e-324" }, b: { $numberLong: "-2147483648" } },
   { a: { $decimal: "5.9e-324" }, b: { $numberLong: "2147483647" } },
   { b: -2147483647.99, c: -922337203685477580.99 },
   { b: 2147483646.99, c: 922337203685477580.99 },
   { b: { $decimal: "-2147483648" }, c: { $decimal: "-9223372036854775808" }, h: "{name:{username:1}}" },
   { b: { $decimal: "2147483647" }, c: { $decimal: "9223372036854775807" }, i: "123abcd00ef12358902300ef" },
   { b: { $decimal: "-2147483647.99" }, c: { $decimal: "-9223372036854775807.99" }, f: "1889-12-31" },
   { b: { $decimal: "2147483646.99" }, c: { $decimal: "9223372036854775806.99" }, f: "1901-01-01" },
   { a: "-1.7E+309", b: "-2147483649", c: "-9223372036854775809", f: "0000-01-00" },
   { a: "1.7E+309", b: "2147483648", c: "9223372036854775808", f: "10000-01-01" }
   ];
   dbcl.insert( doc );

   //source type:all,destination type:all,for string express,lower letters
   var selectCondition1 = {
      a: { $cast: "double" },
      b: { $cast: "int32" },
      c: { $cast: "int64" },
      d: { $cast: "decimal" },
      e: { $cast: "bool" },
      f: { $cast: "date" },
      g: { $cast: "timestamp" },
      h: { $cast: "object" },
      i: { $cast: "oid" }
   };


   var expRecs1 = [{ a: -1.7E+308, b: -2147483648, c: { $numberLong: "-9223372036854775808" }, d: { $decimal: "-92233720368547758089223372036854775808" }, e: false, f: { $date: "2016-07-18" }, g: { $timestamp: "2016-07-18-13.14.26.124233" } },
   { a: 1.7E+308, b: 2147483647, c: { $numberLong: "9223372036854775807" }, d: { $decimal: "92233720368547758079223372036854775807" }, e: false },
   { a: -4.9e-324, d: { $decimal: "-92233720368547758089223372036854775808.99" }, e: false },
   { a: 4.9e-324, d: { $decimal: "92233720368547758079223372036854775807.99" }, e: false },
   { a: -5e-324, b: -2147483648 },
   { a: 5e-324, b: 2147483647 },
   { b: -2147483647, c: { $numberLong: "-922337203685477632" } },
   { b: 2147483646, c: { $numberLong: "922337203685477632" } },
   { b: -2147483648, c: { $numberLong: "-9223372036854775808" }, h: { name: { username: 1 } } },
   { b: 2147483647, c: { $numberLong: "9223372036854775807" }, i: { $oid: "123abcd00ef12358902300ef" } },
   { b: -2147483648, c: { $numberLong: "-9223372036854775808" }, f: { $date: "1889-12-31" } },
   { b: 2147483647, c: { $numberLong: "9223372036854775807" }, f: { $date: "1901-01-01" } },
   { a: 0, b: 0, c: 0, f: null },
   { a: 0, b: 0, c: 0, f: null }
   ];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );
}
