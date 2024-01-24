/************************************
*@Description: use elemMatchOneOne:{arr:{$elemMatchOneOne:{fieldName:xx}}}, 
               cover all type of data,zero or one or two record match
*@author:      zhaoyu
*@createdate:  2016.7.14
*@testlinkCase: seqDB-5606/seqDB-5607/seqDB-5608
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{
      No: 1, arr: [{ type: "int", value: -2147483648 }, { type: "int", value: -2147483648 },
      { type: "numberLong", value: { $numberLong: "-9223372036854775808" } }, { type: "numberLong", value: { $numberLong: "-9223372036854775808" } },
      { type: "string", value: "abc" }, { type: "string", value: "abc" },
      { type: "float", value: -1.7E+308 }, { type: "float", value: -1.7E+308 },
      { type: "decimal", value: { $decimal: "-2147483648" } }, { type: "decimal", value: { $decimal: "-2147483648" } },
      { type: "oid", value: { $oid: "123abcd00ef12358902300ef" } }, { type: "oid", value: { $oid: "123abcd00ef12358902300ef" } }]
   },
   {
      No: 2, arr: [{ type: "bool", value: true }, { type: "bool", value: true },
      { type: "date", value: { $date: "2012-01-01" } }, { type: "date", value: { $date: "2012-01-01" } },
      { type: "timestamp", value: { $timestamp: "2012-01-01-13.14.26.124233" } }, { type: "timestamp", value: { $timestamp: "2012-01-01-13.14.26.124233" } },
      { type: "bin", value: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }, { type: "bin", value: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      { type: "regex", value: { $regex: "^z", $options: "i" } }, { type: "regex", value: { $regex: "^z", $options: "i" } },
      { type: "object", value: { subobj: -2147483648 } }, { type: "object", value: { subobj: -2147483648 } },
      { type: "arr", value: [1, 2, 3] }, { type: "arr", value: [1, 2, 3] },
      { type: "null", value: null }, { type: "null", value: null }]
   },
   {
      No: 3, arr: [{ type: "int", value: -2147483648 },
      { type: "numberLong", value: { $numberLong: "-9223372036854775808" } },
      { type: "string", value: "abc" },
      { type: "float", value: -1.7E+308 },
      { type: "decimal", value: { $decimal: "-2147483648" } },
      { type: "oid", value: { $oid: "123abcd00ef12358902300ef" } },
      { type: "bool", value: true },
      { type: "date", value: { $date: "2012-01-01" } },
      { type: "timestamp", value: { $timestamp: "2012-01-01-13.14.26.124233" } },
      { type: "bin", value: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      { type: "regex", value: { $regex: "^z", $options: "i" } },
      { type: "object", value: { subobj: -2147483648 } },
      { type: "arr", value: [1, 2, 3] },
      { type: "null", value: null }]
   }];
   dbcl.insert( doc );

   //field and value is existent,cover all type data,one or two record match,seqDB-5606/seqDB-5607
   var selectCondition1 = { arr: { $elemMatchOne: { value: -2147483648 } } };
   var expRecs1 = [{ No: 1, arr: [{ type: "int", value: -2147483648 }] },
   { No: 2, arr: [] },
   { No: 3, arr: [{ type: "int", value: -2147483648 }] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   var selectCondition2 = { arr: { $elemMatchOne: { value: { $numberLong: "-9223372036854775808" } } } };
   var expRecs2 = [{ No: 1, arr: [{ type: "numberLong", value: { $numberLong: "-9223372036854775808" } }] },
   { No: 2, arr: [] },
   { No: 3, arr: [{ type: "numberLong", value: { $numberLong: "-9223372036854775808" } }] }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { No: 1 } );

   var selectCondition3 = { arr: { $elemMatchOne: { value: "abc" } } };
   var expRecs3 = [{ No: 1, arr: [{ type: "string", value: "abc" }] },
   { No: 2, arr: [] },
   { No: 3, arr: [{ type: "string", value: "abc" }] }];
   checkResult( dbcl, null, selectCondition3, expRecs3, { No: 1 } );

   var selectCondition4 = { arr: { $elemMatchOne: { value: { $decimal: "-2147483648" } } } };
   var expRecs4 = [{ No: 1, arr: [{ type: "int", value: -2147483648 }] },
   { No: 2, arr: [] },
   { No: 3, arr: [{ type: "int", value: -2147483648 }] }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   var selectCondition5 = { arr: { $elemMatchOne: { value: { $oid: "123abcd00ef12358902300ef" } } } };
   var expRecs5 = [{ No: 1, arr: [{ type: "oid", value: { $oid: "123abcd00ef12358902300ef" } }] },
   { No: 2, arr: [] },
   { No: 3, arr: [{ type: "oid", value: { $oid: "123abcd00ef12358902300ef" } }] }];
   checkResult( dbcl, null, selectCondition5, expRecs5, { No: 1 } );

   var selectCondition6 = { arr: { $elemMatchOne: { value: true } } };
   var expRecs6 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "bool", value: true }] },
   { No: 3, arr: [{ type: "bool", value: true }] }];
   checkResult( dbcl, null, selectCondition6, expRecs6, { No: 1 } );

   var selectCondition8 = { arr: { $elemMatchOne: { value: { $date: "2012-01-01" } } } };
   var expRecs8 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "date", value: { $date: "2012-01-01" } }] },
   { No: 3, arr: [{ type: "date", value: { $date: "2012-01-01" } }] }];
   checkResult( dbcl, null, selectCondition8, expRecs8, { No: 1 } );

   var selectCondition9 = { arr: { $elemMatchOne: { value: { $timestamp: "2012-01-01-13.14.26.124233" } } } };
   var expRecs9 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "timestamp", value: { $timestamp: "2012-01-01-13.14.26.124233" } }] },
   { No: 3, arr: [{ type: "timestamp", value: { $timestamp: "2012-01-01-13.14.26.124233" } }] }];
   checkResult( dbcl, null, selectCondition9, expRecs9, { No: 1 } );

   var selectCondition10 = { arr: { $elemMatchOne: { value: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } } };
   var expRecs10 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "bin", value: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }] },
   { No: 3, arr: [{ type: "bin", value: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }] }];
   checkResult( dbcl, null, selectCondition10, expRecs10, { No: 1 } );

   var selectCondition11 = { arr: { $elemMatchOne: { value: { $et: { $regex: "^z", $options: "i" } } } } };
   var expRecs11 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "regex", value: { $regex: "^z", $options: "i" } }] },
   { No: 3, arr: [{ type: "regex", value: { $regex: "^z", $options: "i" } }] }];
   checkResult( dbcl, null, selectCondition11, expRecs11, { No: 1 } );

   var selectCondition12 = { arr: { $elemMatchOne: { value: { subobj: -2147483648 } } } };
   var expRecs12 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "object", value: { subobj: -2147483648 } }] },
   { No: 3, arr: [{ type: "object", value: { subobj: -2147483648 } }] }];
   checkResult( dbcl, null, selectCondition12, expRecs12, { No: 1 } );

   var selectCondition13 = { arr: { $elemMatchOne: { value: [1, 2, 3] } } };
   var expRecs13 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "arr", value: [1, 2, 3] }] },
   { No: 3, arr: [{ type: "arr", value: [1, 2, 3] }] }];
   checkResult( dbcl, null, selectCondition13, expRecs13, { No: 1 } );

   var selectCondition14 = { arr: { $elemMatchOne: { value: null } } };
   var expRecs14 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "null", value: null }] },
   { No: 3, arr: [{ type: "null", value: null }] }];
   checkResult( dbcl, null, selectCondition14, expRecs14, { No: 1 } );

   //no record match,seqDB-5608
   var selectCondition15 = { arr: { $elemMatchOne: { value: 20 } } };
   var expRecs15 = [{ No: 1, arr: [] },
   { No: 2, arr: [] },
   { No: 3, arr: [] }];
   checkResult( dbcl, null, selectCondition15, expRecs15, { No: 1 } );

   //then find all data,check result
   checkResult( dbcl, null, null, doc, { No: 1 } );
}
