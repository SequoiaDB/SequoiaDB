/************************************
*@Description: use cast:{field:<$cast:Value>}, 
               1.source type:all,destination type:all,for string express,lower letters
               2.source type:all,destination type:all,for number express
               3.source type:all,destination type:all,for string express,upper letters
               4.fields is Non-existent
               5.not include:
                 string of numberic type, cast double/int/long/decimal;
                 int/int64/double/decimal,value is 0, cast bool;
	              string of date/timestamp type, cast data/timestamp;
	              int64/double/decimal in range int, cast int
                 decimal/double in range numberLong, cast int64
                 string of json format, cast object;
                 string with 24 character,cast objectId
*@author:      zhaoyu
*@createdate:  2016.7.18
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
   var doc = [{ id: 1, a: -2147483648, b: -2147483648, c: -2147483648, d: -2147483648, e: -2147483648, f: -2147483648, g: -2147483648, h: -2147483648, i: -2147483648, k: -2147483648.48, l: -2147483648, m: -2147483648 },
   { id: 2, a: 2147483647, b: 2147483647, c: 2147483647, d: 2147483647, e: 2147483647, f: 2147483647, g: 2147483647, h: 2147483647, i: 2147483647, k: 2147483647.58, l: 2147483647, m: 2147483647 },
   { id: 3, a: { $numberLong: "-9223372036854775808" }, b: { $numberLong: "-9223372036854775808" }, c: { $numberLong: "-9223372036854775808" }, d: { $numberLong: "-9223372036854775808" }, e: { $numberLong: "-9223372036854775808" }, f: { $numberLong: "-9223372036854775808" }, g: { $numberLong: "-2147483648" }, h: { $numberLong: "-9223372036854775808" }, i: { $numberLong: "-9223372036854775808" }, j: { $numberLong: "-9223372036854775808" }, k: { $numberLong: "-9223372036854775808" }, l: { $numberLong: "-9223372036854775808" }, m: { $numberLong: "-9223372036854775808" } },
   { id: 4, a: { $numberLong: "9223372036854775807" }, b: { $numberLong: "9223372036854775807" }, c: { $numberLong: "9223372036854775807" }, d: { $numberLong: "9223372036854775807" }, e: { $numberLong: "9223372036854775807" }, f: { $numberLong: "9223372036854775807" }, g: { $numberLong: "2147483647" }, h: { $numberLong: "9223372036854775807" }, i: { $numberLong: "9223372036854775807" }, j: { $numberLong: "9223372036854775807" }, k: { $numberLong: "9223372036854775807" }, l: { $numberLong: "9223372036854775807" }, m: { $numberLong: "9223372036854775807" } },
   { id: 5, a: { $decimal: "-92233720368547758089223372036854775808" }, b: { $decimal: "-92233720368547758089223372036854775808" }, c: { $decimal: "-92233720368547758089223372036854775808" }, d: { $decimal: "-92233720368547758089223372036854775808" }, e: { $decimal: "-92233720368547758089223372036854775808" }, f: { $decimal: "-92233720368547758089223372036854775808" }, g: { $decimal: "-92233720368547758089223372036854775808" }, h: { $decimal: "-92233720368547758089223372036854775808" }, i: { $decimal: "-92233720368547758089223372036854775808" }, j: { $decimal: "-92233720368547758089223372036854775808" }, k: { $decimal: "-92233720368547758089223372036854775808" }, l: { $decimal: "-92233720368547758089223372036854775808" }, m: { $decimal: "-92233720368547758089223372036854775808" } },
   { id: 6, a: { $decimal: "92233720368547758079223372036854775807" }, b: { $decimal: "92233720368547758079223372036854775807" }, c: { $decimal: "92233720368547758079223372036854775807" }, d: { $decimal: "92233720368547758079223372036854775807" }, e: { $decimal: "92233720368547758079223372036854775807" }, f: { $decimal: "92233720368547758079223372036854775807" }, g: { $decimal: "92233720368547758079223372036854775807" }, h: { $decimal: "92233720368547758079223372036854775807" }, i: { $decimal: "92233720368547758079223372036854775807" }, j: { $decimal: "92233720368547758079223372036854775807" }, k: { $decimal: "92233720368547758079223372036854775807" }, l: { $decimal: "92233720368547758079223372036854775807" }, m: { $decimal: "92233720368547758079223372036854775807" } },
   { id: 7, a: -1.7E+308, b: -1.7E+308, c: -1.7E+308, d: -1.7E+308, e: -1.7E+308, f: -1.7E+308, g: -2147483648.23, h: -1.7E+308, i: -1.7E+308, j: -1.7E+308, k: -1.7E+308, l: -1.7E+308, m: -1.7E+308 },
   { id: 8, a: 1.7E+308, b: 1.7E+308, c: 1.7E+308, d: 1.7E+308, e: 1.7E+308, f: 1.7E+308, g: 2147483647.23, h: 1.7E+308, i: 1.7E+308, j: 1.7E+308, k: 1.7E+308, l: 1.7E+308, m: 1.7E+308 },
   { id: 9, a: -4.9E-324, b: -4.9E-324, c: -4.9E-324, d: -4.9E-324, e: -4.9E-324, f: -4.9E-324, h: -4.9E-324, i: -4.9E-324, k: -4.9E-324, l: -4.9E-324, m: -4.9E-324 },
   { id: 10, a: 4.9E-324, b: 4.9E-324, c: 4.9E-324, d: 4.9E-324, e: 4.9E-324, f: 4.9E-324, h: 4.9E-324, i: 4.9E-324, k: 4.9E-324, l: 4.9E-324, m: 4.9E-324 },
   { id: 11, a: "string", b: "string", c: "string", d: "string", e: "string", f: "string", g: "string", h: "string", i: "string", j: "string", k: "string", l: "string", m: "string" },
   { id: 12, a: { $oid: "573920accc332f037c000013" }, b: { $oid: "573920accc332f037c000013" }, c: { $oid: "573920accc332f037c000013" }, d: { $oid: "573920accc332f037c000013" }, e: { $oid: "573920accc332f037c000013" }, f: { $oid: "573920accc332f037c000013" }, g: { $oid: "573920accc332f037c000013" }, h: { $oid: "573920accc332f037c000013" }, i: { $oid: "573920accc332f037c000013" }, j: { $oid: "573920accc332f037c000013" }, k: { $oid: "573920accc332f037c000013" }, l: { $oid: "573920accc332f037c000013" }, m: { $oid: "573920accc332f037c000013" } },
   { id: 13, a: false, b: false, c: false, d: false, e: false, f: false, g: false, h: false, i: false, j: false, k: false, l: false, m: false },
   { id: 14, a: true, b: true, c: true, d: true, e: true, f: true, g: true, h: true, i: true, j: true, k: true, l: true, m: true },
   { id: 15, a: { $date: "2016-05-16" }, b: { $date: "2016-05-16" }, c: { $date: "2016-05-16" }, d: { $date: "2016-05-16" }, e: { $date: "2016-05-16" }, f: { $date: "2016-05-16" }, g: { $date: "2016-05-16" }, h: { $date: "2016-05-16" }, i: { $date: "2016-05-16" }, j: { $date: "2016-05-16" }, k: { $date: "2016-05-16" }, l: { $date: "2016-05-16" }, m: { $date: "2016-05-16" } },
   { id: 16, a: { $timestamp: "2016-05-16-13.14.26.124233" }, b: { $timestamp: "2016-05-16-13.14.26.124233" }, c: { $timestamp: "2016-05-16-13.14.26.124233" }, d: { $timestamp: "2016-05-16-13.14.26.124233" }, e: { $timestamp: "2016-05-16-13.14.26.124233" }, f: { $timestamp: "2016-05-16-13.14.26.124233" }, g: { $timestamp: "2016-05-16-13.14.26.124233" }, h: { $timestamp: "2016-05-16-13.14.26.124233" }, i: { $timestamp: "2016-05-16-13.14.26.124233" }, j: { $timestamp: "2016-05-16-13.14.26.124233" }, k: { $timestamp: "2016-05-16-13.14.26.124233" }, l: { $timestamp: "2016-05-16-13.14.26.124233" }, m: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { id: 17, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, d: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, e: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, f: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, g: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, h: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, i: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, j: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, k: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, l: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, m: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { id: 18, a: { $regex: "^z", $options: "i" }, b: { $regex: "^z", $options: "i" }, c: { $regex: "^z", $options: "i" }, d: { $regex: "^z", $options: "i" }, e: { $regex: "^z", $options: "i" }, f: { $regex: "^z", $options: "i" }, g: { $regex: "^z", $options: "i" }, h: { $regex: "^z", $options: "i" }, i: { $regex: "^z", $options: "i" }, j: { $regex: "^z", $options: "i" }, k: { $regex: "^z", $options: "i" }, l: { $regex: "^z", $options: "i" }, m: { $regex: "^z", $options: "i" } },
   { id: 19, a: { name: "hanmeimei" }, b: { name: "hanmeimei" }, c: { name: "hanmeimei" }, d: { name: "hanmeimei" }, e: { name: "hanmeimei" }, f: { name: "hanmeimei" }, g: { name: "hanmeimei" }, h: { name: "hanmeimei" }, i: { name: "hanmeimei" }, j: { name: "hanmeimei" }, k: { name: "hanmeimei" }, l: { name: "hanmeimei" }, m: { name: "hanmeimei" } },
   { id: 20, a: ["b", 0], b: ["b", 0], c: ["b", 0], d: ["b", 0], e: ["b", 0], f: ["b", 0], g: ["b", 0], h: ["b", 0], i: ["b", 0], j: ["b", 0], k: ["b", 0], l: ["b", 0], m: ["b", 0] },
   { id: 21, a: null, b: null, c: null, d: null, e: null, f: null, g: null, h: null, i: null, j: null, k: null, l: null, m: null },
   { id: 22, a: MaxKey(), b: MaxKey(), c: MaxKey(), d: MaxKey(), e: MaxKey(), f: MaxKey(), g: MaxKey(), h: MaxKey(), i: MaxKey(), j: MaxKey(), k: MaxKey(), l: MaxKey(), m: MaxKey() },
   { id: 23, a: MinKey(), b: MinKey(), c: MinKey(), d: MinKey(), e: MinKey(), f: MinKey(), g: MinKey(), h: MinKey(), i: MinKey(), j: MinKey(), k: MinKey(), l: MinKey(), m: MinKey() },
   { id: 24, j: { $date: "1900-01-01" } },
   { id: 25, j: { $date: "9999-12-31" } }
   ];
   dbcl.insert( doc );

   var expRecs = [
      { "a": { "$minKey": 1 }, "b": -2147483648, "c": "-2147483648", "d": null, "e": null, "f": true, "g": { "$date": "1901-12-14" }, "h": null, "i": -2147483648, "k": -2147483648, "l": { "$decimal": "-2147483648" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 2147483647, "c": "2147483647", "d": null, "e": null, "f": true, "g": { "$date": "2038-01-19" }, "h": null, "i": 2147483647, "k": 2147483647, "l": { "$decimal": "2147483647" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": -9223372036854776000, "c": "-9223372036854775808", "d": null, "e": null, "f": true, "g": { "$date": "1969-12-07" }, "h": null, "i": 0, "j": null, "k": { "$numberLong": "-9223372036854775808" }, "l": { "$decimal": "-9223372036854775808" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 9223372036854776000, "c": "9223372036854775807", "d": null, "e": null, "f": true, "g": { "$date": "1970-01-26" }, "h": null, "i": 0, "j": null, "k": { "$numberLong": "9223372036854775807" }, "l": { "$decimal": "9223372036854775807" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": -9.223372036854776e+37, "c": "-92233720368547758089223372036854775808", "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "-92233720368547758089223372036854775808" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 9.223372036854776e+37, "c": "92233720368547758079223372036854775807", "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "92233720368547758079223372036854775807" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": -1.7e+308, "c": "-1.7e+308", "d": null, "e": null, "f": true, "g": { "$date": "1969-12-07" }, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "-170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 1.7e+308, "c": "1.7e+308", "d": null, "e": null, "f": true, "g": { "$date": "1970-01-26" }, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": -5e-324, "c": "-4.94066e-324", "d": null, "e": null, "f": true, "h": null, "i": 0, "k": 0, "l": { "$decimal": "-0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000494065645841247" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 5e-324, "c": "4.94066e-324", "d": null, "e": null, "f": true, "h": null, "i": 0, "k": 0, "l": { "$decimal": "0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000494065645841247" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": "string", "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": "573920accc332f037c000013", "d": null, "e": { "$oid": "573920accc332f037c000013" }, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": "false", "d": null, "e": null, "f": false, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 1, "c": "true", "d": null, "e": null, "f": true, "g": null, "h": null, "i": 1, "j": null, "k": 1, "l": { "$decimal": "1" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 1463328000000, "c": "2016-05-16", "d": null, "e": null, "f": true, "g": { "$date": "2016-05-16" }, "h": null, "i": 1463328000, "j": { "$timestamp": "2016-05-16-00.00.00.000000" }, "k": 1463328000000, "l": { "$decimal": "1463328000000" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 1463375666124, "c": "2016-05-16-13.14.26.124233", "d": null, "e": null, "f": true, "g": { "$date": "2016-05-16" }, "h": null, "i": 1463375666, "j": { "$timestamp": "2016-05-16-13.14.26.124233" }, "k": 1463375666124, "l": { "$decimal": "1463375666124" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": null, "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": null, "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": "{ \"name\": \"hanmeimei\" }", "d": { "name": "hanmeimei" }, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": [{ "$minKey": 1 }, { "$minKey": 1 }], "b": [0, 0], "c": ["b", "0"], "d": [null, null], "e": [null, null], "f": [true, false], "g": [null, { "$date": "1970-01-01" }], "h": [null, null], "i": [0, 0], "j": [null, { "$timestamp": "1970-01-01-08.00.00.000000" }], "k": [0, 0], "l": [{ "$decimal": "0" }, { "$decimal": "0" }], "m": [{ "$maxKey": 1 }, { "$maxKey": 1 }] },
      { "a": { "$minKey": 1 }, "b": 0, "c": null, "d": null, "e": null, "f": false, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": null, "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { "a": { "$minKey": 1 }, "b": 0, "c": null, "d": null, "e": null, "f": true, "g": null, "h": null, "i": 0, "j": null, "k": 0, "l": { "$decimal": "0" }, "m": { "$maxKey": 1 } },
      { j: null },
      { j: null }
   ];

   //source type:all,destination type:all,for string express,lower letters
   var selectCondition1 = {
      _id: { $include: 0 },
      id: { $include: 0 },
      a: { $cast: "minkey" },
      b: { $cast: "double" },
      c: { $cast: "string" },
      d: { $cast: "object" },
      e: { $cast: "oid" },
      f: { $cast: "bool" },
      g: { $cast: "date" },
      h: { $cast: "null" },
      i: { $cast: "int32" },
      j: { $cast: "timestamp" },
      k: { $cast: "int64" },
      l: { $cast: "Decimal" },
      m: { $cast: "maxkey" },
      o: { $cast: "string" }
   };
   checkResult( dbcl, null, selectCondition1, expRecs, { id: 1 } );

   //source type:all,destination type:all,for number express
   var selectCondition2 = {
      _id: { $include: 0 },
      id: { $include: 0 },
      a: { $cast: -1 },
      b: { $cast: 1 },
      c: { $cast: 2 },
      d: { $cast: 3 },
      e: { $cast: 7 },
      f: { $cast: 8 },
      g: { $cast: 9 },
      h: { $cast: 10 },
      i: { $cast: 16 },
      j: { $cast: 17 },
      k: { $cast: 18 },
      l: { $cast: 100 },
      m: { $cast: 127 }
   };
   checkResult( dbcl, null, selectCondition2, expRecs, { id: 1 } );

   //source type:all,destination type:all,for string express,upper letters
   var selectCondition3 = {
      _id: { $include: 0 },
      id: { $include: 0 },
      a: { $cast: "MINKEY" },
      b: { $cast: "DOUBLE" },
      c: { $cast: "STRING" },
      d: { $cast: "OBJECT" },
      e: { $cast: "OID" },
      f: { $cast: "BOOL" },
      g: { $cast: "DATE" },
      h: { $cast: "NULL" },
      i: { $cast: "INT32" },
      j: { $cast: "TIMESTAMP" },
      k: { $cast: "INT64" },
      l: { $cast: "DECIMAL" },
      m: { $cast: "MAXKEY" }
   };
   checkResult( dbcl, null, selectCondition3, expRecs, { id: 1 } );
}

