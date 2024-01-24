/************************************
*@Description: use subtract to match a field,
               value set boundary of int/double/numberLong/decimal
*@author:      zhaoyu
*@createdate:  2016.10.13
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   var doc1 = [{ No: 1, a: -1.7E+307 },
   { No: 2, a: -1.7E+308 },
   { No: 3, a: 1.7E+307 },
   { No: 4, a: 1.7E+308 }];
   dbcl.insert( doc1 );

   //$lt:1.7E+308
   var findCondition1 = { a: { $subtract: -1.7E+308, $gt: 0 } };
   var expRecs1 = [{ No: 1, a: -1.7E+307 },
   { No: 3, a: 1.7E+307 },
   { No: 4, a: 1.7E+308 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );

   //remove all data and insert numberLong data
   dbcl.remove();

   var doc2 = [{ No: 1, a: { $numberLong: "-9223372036854775808" } },
   { No: 2, a: { $numberLong: "-9223372036854775807" } }];
   dbcl.insert( doc2 );

   //test bundary value operation
   var findCondition2 = { a: { $subtract: 1, $et: { $decimal: "-9223372036854775809" } } };
   var expRecs2 = [{ No: 1, a: { $numberLong: "-9223372036854775808" } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { _id: 1 } );

}
