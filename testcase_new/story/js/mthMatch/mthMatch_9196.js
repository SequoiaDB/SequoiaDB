/************************************
*@Description: slice as function
*@author:      zhaoyu
*@createdate:  2016.11.1
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
   var doc = [{ No: 1, a: [1, 2, 3] },
   { No: 2, a: [1, 2] },
   { No: 3, a: [1, 2, 3, 4, 5] },
   { No: 4, a: [1, 2, 3, 4] },
   { No: 5, a: [] },
   { No: 6, a: 1 },
   { No: 7, a: { b: 1, a: true, c: 12.3 } },
   { No: 8, a: { "test": { name: "lily", age: 23 } } }];
   dbcl.insert( doc );

   //many field,some exists,some Non-exists,SEQUOIADBMAINSTREAM-2036
   var selector1 = { a: { $size: 1 }, b: { $size: 1 } };
   var expRecs1 = [{ No: 1, a: 3 },
   { No: 2, a: 2 },
   { No: 3, a: 5 },
   { No: 4, a: 4 },
   { No: 5, a: 0 },
   { No: 6, a: null },
   { No: 7, a: 3 },
   { No: 8, a: 1 }];
   checkResult( dbcl, null, selector1, expRecs1, { No: 1 } );

   //field is object
   var findCond3 = { No: 7 };
   var selector3 = { "a.b": { $size: 1 } };
   var expRecs3 = [{ No: 7, a: { b: null, a: true, c: 12.3 } }];
   checkResult( dbcl, findCond3, selector3, expRecs3, { No: 1 } );

   //field is nested object 
   var findCond4 = { No: 8 };
   var selector4 = { "a.test": { $size: 1 } };
   var expRecs4 = [{ No: 8, a: { "test": 2 } }];
   checkResult( dbcl, findCond4, selector4, expRecs4, { No: 1 } );

   var selector2 = { a: { $type: "a" } };
   InvalidArgCheck( dbcl, null, selector2, -6, { No: 1 } );


}
