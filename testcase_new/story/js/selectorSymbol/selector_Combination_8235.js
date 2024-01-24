/************************************
*@Description: selector combination 
               the same field,different selector,the last expression valid only;seqDB-8235
*@author:      zhaoyu
*@createdate:  2016.7.19
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
   var doc = [{ a: 1000 }];
   dbcl.insert( doc );

   //the same field,different selector,the last expression valid only;seqDB-8235
   var selectCondition1 = { a: { $add: 100 }, a: { $subtract: 101 }, a: { $multiply: 102 }, a: { $divide: 50 } };
   var expRecs1 = [{ a: 20 }];
   checkResult( dbcl, null, selectCondition1, expRecs1, { _id: 1 } );
}
