/************************************
*@Description: use substr:{a:{$substr:value1, $et:'20'}},{a:{$substr:[value1,value2], $et:'20'}} 
               seqDB-10277/seqDB-10278/seqDB-10279/seqDB-10280/10281/seqDB-10282/seqDB-10283/seqDB-10284
*@author:      zhaoyu
*@createdate:  2016.10.14
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
   var doc = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" },
   { No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   dbcl.insert( doc );

   //seqDB-10277
   //value1>0,value1<str.length;
   var condition1 = { a: { $substr: 4, $et: "abcd" } };
   var expRecs1 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   //value1>0,value1>str.length
   var condition2 = { a: { $substr: 9, $et: "abcdefgh" } };
   var expRecs2 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   //value1<0,|value1|<str.length
   var condition3 = { e: { $substr: -4, $et: "wxyz" } };
   var expRecs3 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition3, null, expRecs3, { No: 1 } );

   //value1<0,|value1|>str.length
   var condition4 = { a: { $substr: -9, $et: "abceefgih" } };
   var expRecs4 = [{ No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   checkResult( dbcl, condition4, null, expRecs4, { No: 1 } );

   //value1=0
   var condition5 = { a: { $substr: 0, $et: "" } };
   var expRecs5 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" },
   { No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   checkResult( dbcl, condition5, null, expRecs5, { No: 1 } );

   //|value1|=str.length
   var condition6 = { a: { $substr: 8, $et: "abcdefgh" } };
   var expRecs6 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition6, null, expRecs6, { No: 1 } );

   var condition7 = { a: { $substr: -8, $et: "abcdefgh" } };
   var expRecs7 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition7, null, expRecs7, { No: 1 } );

   //seqDB-10281
   //value1<0,value2>=0,|value1|+value2<=arr.length 
   var condition8 = { a: { $substr: [-5, 4], $et: "defg" } };
   var expRecs8 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition8, null, expRecs8, { No: 1 } );

   //value1<0,value2>=0,|value1|+value2>arr.length
   var condition9 = { a: { $substr: [-5, 6], $et: "defgh" } };
   var expRecs9 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition9, null, expRecs9, { No: 1 } );

   //value1>=0,value2>=0,value1+value2<=arr.length
   var condition10 = { a: { $substr: [5, 3], $et: "fgh" } };
   var expRecs10 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition10, null, expRecs10, { No: 1 } );

   //value1>=0,value2>=0,value1+value2>arr.length
   var condition11 = { a: { $substr: [5, 4], $et: "fgh" } };
   var expRecs11 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition11, null, expRecs11, { No: 1 } );

   //value1<0,value2<0,|value1|+|value2|<=arr.length
   var condition12 = { a: { $substr: [-5, -4], $et: "defgh" } };
   var expRecs12 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition12, null, expRecs12, { No: 1 } );

   //value1<0,value2<0,|value1|+|value2|>arr.length
   var condition13 = { a: { $substr: [-5, -6], $et: "defgh" } };
   var expRecs13 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition13, null, expRecs13, { No: 1 } );

   //value1>=0,value2<0,value1+|value2|<=arr.length
   var condition14 = { a: { $substr: [5, -4], $et: "fgh" } };
   var expRecs14 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition14, null, expRecs14, { No: 1 } );

   //value1>=0,value2<0,value1+|value2|>arr.length
   var condition15 = { a: { $substr: [5, -6], $et: "fgh" } };
   var expRecs15 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition15, null, expRecs15, { No: 1 } );

   //value1>0,value1>arr.length
   var condition16 = { a: { $substr: [8, 4], $et: "h" } };
   var expRecs16 = [{ No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   checkResult( dbcl, condition16, null, expRecs16, { No: 1 } );

   //value1<0,|value1|>arr.length
   var condition17 = { a: { $substr: [-9, 4], $et: "abce" } };
   var expRecs17 = [{ No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   checkResult( dbcl, condition17, null, expRecs17, { No: 1 } );

   //value2>0,value2>arr.length
   var condition18 = { a: { $substr: [5, 10], $et: "fgh" } };
   var expRecs18 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition18, null, expRecs18, { No: 1 } );

   //value2<0,|value2|>arr.length
   var condition19 = { a: { $substr: [5, -10], $et: "fgh" } };
   var expRecs19 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition19, null, expRecs19, { No: 1 } );

   //empty string;
   var condition20 = { b: { $substr: [1, 5], $et: "" } };
   var expRecs20 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" },
   { No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   checkResult( dbcl, condition20, null, expRecs20, { No: 1 } );

   //field is Non-string;
   var condition21 = { c: { $substr: [1, 5], $et: null } };
   var expRecs21 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" },
   { No: 2, a: "abceefgih", b: "", c: 123, e: "uvxxya" }];
   checkResult( dbcl, condition21, null, expRecs21, { No: 1 } );

   //field is Non-existent;seqDB-10279
   var condition22 = { d: { $substr: [1, 5], $et: "" } };
   var expRecs22 = [];
   checkResult( dbcl, condition22, null, expRecs22, { No: 1 } );

   //two fields use substr;seqDB-10284
   var condition23 = { a: { $substr: [1, 5], $et: "bcdef" }, e: { $substr: [-2, 5], $et: "yz" } };
   var expRecs23 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition23, null, expRecs23, { No: 1 } );

   //value is illegal,seqDB-10278
   var condition24 = { a: { $substr: [1, 2] } };
   InvalidArgCheck( dbcl, condition24, null, SDB_INVALIDARG );

   var condition25 = { a: { $substr: ["a", 1], $et: "a" } };
   InvalidArgCheck( dbcl, condition25, null, SDB_INVALIDARG );

   var condition26 = { a: { $substr: [1, "a"] } };
   InvalidArgCheck( dbcl, condition26, null, SDB_INVALIDARG );

   var condition27 = { a: { $substr: 2 } };
   InvalidArgCheck( dbcl, condition27, null, SDB_INVALIDARG );

   var condition28 = { a: { $substr: "a", $et: "a" } };
   InvalidArgCheck( dbcl, condition28, null, SDB_INVALIDARG );

   //two fields use substr;seqDB-10280
   var condition29 = { a: { $substr: 5, $et: "abcde" }, e: { $substr: -5, $et: "vwxyz" } };
   var expRecs29 = [{ No: 1, a: "abcdefgh", b: "", c: 123, e: "uvwxyz" }];
   checkResult( dbcl, condition29, null, expRecs29, { No: 1 } );

}
