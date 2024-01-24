/************************************
*@Description: size as function
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
   var doc = [{ No: 1, a: [[1, 2, 3], [2, 3], 1] },
   { No: 2, a: [[1, 2, 3, 4], [2], 1] }];
   dbcl.insert( doc );

   var findCondition1 = { "a.$0": { $size: 1, $et: 4 } };
   var expRecs1 = [{ No: 2, a: [[1, 2, 3, 4], [2], 1] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );
}
