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
   var doc = [{ No: 1, a: ["abcdefgh"], b: "", c: 123, e: ["uvwxyz"] },
   { No: 2, a: ["abceefgih"], b: "", c: 123, e: ["uvxxya"] },
   { No: 3, a: [{ 0: "abcdefgh" }], b: "", c: 123, e: [{ 0: "uvwxyz" }] },
   { No: 4, a: [{ 0: "abceefgih" }], b: "", c: 123, e: [{ 0: "uvxxya" }] },
   { No: 5, a: { 0: "abcdefgh" }, b: "", c: 123, e: { 0: "uvwxyz" } },
   { No: 6, a: { 0: "abceefgih" }, b: "", c: 123, e: { 0: "uvxxya" } }];
   dbcl.insert( doc );

   var condition1 = { a: { $substr: 4, $et: "abcd" } };
   var expRecs1 = [{ No: 1, a: ["abcdefgh"], b: "", c: 123, e: ["uvwxyz"] }];
   checkResult( dbcl, condition1, null, expRecs1, { No: 1 } );

   var condition2 = { "a.0": { $substr: 4, $et: "abcd" } };
   var expRecs2 = [{ No: 1, a: ["abcdefgh"], b: "", c: 123, e: ["uvwxyz"] },
   { No: 3, a: [{ 0: "abcdefgh" }], b: "", c: 123, e: [{ 0: "uvwxyz" }] },
   { No: 5, a: { 0: "abcdefgh" }, b: "", c: 123, e: { 0: "uvwxyz" } }];
   checkResult( dbcl, condition2, null, expRecs2, { No: 1 } );

   var condition3 = { a: { $substr: [2, 2], $et: "cd" } };
   var expRecs3 = [{ No: 1, a: ["abcdefgh"], b: "", c: 123, e: ["uvwxyz"] }];
   checkResult( dbcl, condition3, null, expRecs3, { No: 1 } );

   var condition4 = { "a.0": { $substr: [2, 2], $et: "cd" } };
   var expRecs4 = [{ No: 1, a: ["abcdefgh"], b: "", c: 123, e: ["uvwxyz"] },
   { No: 3, a: [{ 0: "abcdefgh" }], b: "", c: 123, e: [{ 0: "uvwxyz" }] },
   { No: 5, a: { 0: "abcdefgh" }, b: "", c: 123, e: { 0: "uvwxyz" } }];
   checkResult( dbcl, condition4, null, expRecs4, { No: 1 } );
}