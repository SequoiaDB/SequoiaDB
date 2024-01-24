/************************************
*@Description: use elemMatch:{arr:{$elemMatch:{fieldName:xx}}}, 
               cover all type of data
*@author:      zhaoyu
*@createdate:  2016.7.14
*@testlinkCase: seqDB-5600
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, arr: [{ type: "int", value1: -2147483648 }, { type: "int", value2: 2147483647 }] },
   { No: 2, arr: [{ type: "int", value1: -2147483648 }, { type: "numberLong", value2: { $numberLong: "-9223372036854775808" } }] },
   { No: 3, arr: [{ type: "string", value1: "abc" }, { type: "numberLong", value2: { $numberLong: "-9223372036854775808" } }] },
   { No: 4, arr: [{ type: "string", value1: "abc" }, { type: "float", value2: -1.7E+308 }] },
   { No: 5, arr: [{ type: "decimal", value1: { $decimal: "-2147483648" } }, { type: "float", value2: -1.7E+308 }] },
   { No: 6, arr: [{ type: "decimal", value1: { $decimal: "-2147483648" } }, { type: "oid", value2: { $oid: "123abcd00ef12358902300ef" } }] },
   { No: 7, arr: [{ type: "bool", value1: true }, { type: "oid", value2: { $oid: "123abcd00ef12358902300ef" } }] },
   { No: 8, arr: [{ type: "bool", value1: true }, { type: "bool", value2: false }] },
   { No: 9, arr: [{ type: "date", value1: { $date: "2012-01-01" } }, { type: "bool", value2: false }] },
   { No: 10, arr: [{ type: "date", value1: { $date: "2012-01-01" } }, { type: "timestamp", value2: { $timestamp: "2012-01-01-13.14.26.124233" } }] },
   { No: 11, arr: [{ type: "bin", value1: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }, { type: "timestamp", value2: { $timestamp: "2012-01-01-13.14.26.124233" } }] },
   { No: 12, arr: [{ type: "bin", value1: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }, { type: "regex", value2: { $regex: "^z", $options: "i" } }] },
   { No: 13, arr: [{ type: "object", value1: { subobj: -2147483648 } }, { type: "regex", value2: { $regex: "^z", $options: "i" } }] },
   { No: 14, arr: [{ type: "object", value1: { subobj: -2147483648 } }, { type: "arr", value2: [1, 2, 3] }] },
   { No: 15, arr: [{ type: "null", value1: null }, { type: "arr", value2: [1, 2, 3] }] },
   { No: 16, arr: [{ type: "null", value1: null }, { type: "string", value2: "abc" }] }];
   dbcl.insert( doc );

   //field and value is existent,cover all type data,seqDB-5600
   var selectCondition1 = { arr: { $elemMatch: { value1: -2147483648 } } };
   var expRecs1 = [{ No: 1, arr: [{ type: "int", value1: -2147483648 }] },
   { No: 2, arr: [{ type: "int", value1: -2147483648 }] },
   { No: 3, arr: [] }, { No: 4, arr: [] },
   { No: 5, arr: [{ type: "decimal", value1: { $decimal: "-2147483648" } }] }, { No: 6, arr: [{ type: "decimal", value1: { $decimal: "-2147483648" } }] },
   { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { No: 1 } );

   var selectCondition2 = { arr: { $elemMatch: { value2: { $numberLong: "-9223372036854775808" } } } };
   var expRecs2 = [{ No: 1, arr: [] },
   { No: 2, arr: [{ type: "numberLong", value2: { $numberLong: "-9223372036854775808" } }] },
   { No: 3, arr: [{ type: "numberLong", value2: { $numberLong: "-9223372036854775808" } }] },
   { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition2, expRecs2, { No: 1 } );

   var selectCondition3 = { arr: { $elemMatch: { value1: "abc" } } };
   var expRecs3 = [{ No: 1, arr: [] },
   { No: 2, arr: [] },
   { No: 3, arr: [{ type: "string", value1: "abc" }] }, { No: 4, arr: [{ type: "string", value1: "abc" }] },
   { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition3, expRecs3, { No: 1 } );

   var selectCondition4 = { arr: { $elemMatch: { value1: { $decimal: "-2147483648" } } } };
   var expRecs4 = [{ No: 1, arr: [{ type: "int", value1: -2147483648 }] }, { No: 2, arr: [{ type: "int", value1: -2147483648 }] },
   { No: 3, arr: [] }, { No: 4, arr: [] },
   { No: 5, arr: [{ type: "decimal", value1: { $decimal: "-2147483648" } }] }, { No: 6, arr: [{ type: "decimal", value1: { $decimal: "-2147483648" } }] },
   { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition4, expRecs4, { No: 1 } );

   var selectCondition5 = { arr: { $elemMatch: { value2: { $oid: "123abcd00ef12358902300ef" } } } };
   var expRecs5 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] },
   { No: 6, arr: [{ type: "oid", value2: { $oid: "123abcd00ef12358902300ef" } }] }, { No: 7, arr: [{ type: "oid", value2: { $oid: "123abcd00ef12358902300ef" } }] },
   { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition5, expRecs5, { No: 1 } );

   var selectCondition6 = { arr: { $elemMatch: { value1: true } } };
   var expRecs6 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] },
   { No: 7, arr: [{ type: "bool", value1: true }] }, { No: 8, arr: [{ type: "bool", value1: true }] },
   { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition6, expRecs6, { No: 1 } );

   var selectCondition7 = { arr: { $elemMatch: { value2: false } } };
   var expRecs7 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] },
   { No: 8, arr: [{ type: "bool", value2: false }] }, { No: 9, arr: [{ type: "bool", value2: false }] },
   { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition7, expRecs7, { No: 1 } );

   var selectCondition8 = { arr: { $elemMatch: { value1: { $date: "2012-01-01" } } } };
   var expRecs8 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] },
   { No: 9, arr: [{ type: "date", value1: { $date: "2012-01-01" } }] }, { No: 10, arr: [{ type: "date", value1: { $date: "2012-01-01" } }] },
   { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition8, expRecs8, { No: 1 } );

   var selectCondition9 = { arr: { $elemMatch: { value2: { $timestamp: "2012-01-01-13.14.26.124233" } } } };
   var expRecs9 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] },
   { No: 10, arr: [{ type: "timestamp", value2: { $timestamp: "2012-01-01-13.14.26.124233" } }] }, { No: 11, arr: [{ type: "timestamp", value2: { $timestamp: "2012-01-01-13.14.26.124233" } }] },
   { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition9, expRecs9, { No: 1 } );

   var selectCondition10 = { arr: { $elemMatch: { value1: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } } };
   var expRecs10 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] },
   { No: 11, arr: [{ type: "bin", value1: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }] }, { No: 12, arr: [{ type: "bin", value1: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }] },
   { No: 13, arr: [] }, { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition10, expRecs10, { No: 1 } );

   var selectCondition11 = { arr: { $elemMatch: { value2: { $et: { $regex: "^z", $options: "i" } } } } };
   var expRecs11 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] },
   { No: 12, arr: [{ type: "regex", value2: { $regex: "^z", $options: "i" } }] }, { No: 13, arr: [{ type: "regex", value2: { $regex: "^z", $options: "i" } }] },
   { No: 14, arr: [] }, { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition11, expRecs11, { No: 1 } );

   var selectCondition12 = { arr: { $elemMatch: { value1: { subobj: -2147483648 } } } };
   var expRecs12 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] }, { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] },
   { No: 13, arr: [{ type: "object", value1: { subobj: -2147483648 } }] }, { No: 14, arr: [{ type: "object", value1: { subobj: -2147483648 } }] },
   { No: 15, arr: [] }, { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition12, expRecs12, { No: 1 } );

   var selectCondition13 = { arr: { $elemMatch: { value2: [1, 2, 3] } } };
   var expRecs13 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] },
   { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] },
   { No: 14, arr: [{ type: "arr", value2: [1, 2, 3] }] }, { No: 15, arr: [{ type: "arr", value2: [1, 2, 3] }] },
   { No: 16, arr: [] }];
   checkResult( dbcl, null, selectCondition13, expRecs13, { No: 1 } );

   var selectCondition14 = { arr: { $elemMatch: { value1: null } } };
   var expRecs14 = [{ No: 1, arr: [] }, { No: 2, arr: [] }, { No: 3, arr: [] }, { No: 4, arr: [] }, { No: 5, arr: [] }, { No: 6, arr: [] }, { No: 7, arr: [] }, { No: 8, arr: [] },
   { No: 9, arr: [] }, { No: 10, arr: [] }, { No: 11, arr: [] }, { No: 12, arr: [] }, { No: 13, arr: [] }, { No: 14, arr: [] },
   { No: 15, arr: [{ type: "null", value1: null }] }, { No: 16, arr: [{ type: "null", value1: null }] }];
   checkResult( dbcl, null, selectCondition14, expRecs14, { No: 1 } );

   //then find all data,check result
   checkResult( dbcl, null, null, doc, { No: 1 } );


}