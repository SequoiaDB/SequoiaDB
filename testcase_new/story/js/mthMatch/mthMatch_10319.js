/************************************
*@Description: one field use many functions,then use matches
*@author:      zhaoyu
*@createdate:  2016.10.25
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

   //insert data 
   var doc = [{ No: 1, a: " \n\r\t-123.1456789 \n\r\t" },
   { No: 2, a: " \n\r\t-124.1456789 \n\r\t" }];
   dbcl.insert( doc );

   var findCondition1 = {
      a: {
         $ltrim: 1, $rtrim: 1, $trim: 1, $substr: 8,
         $cast: 1,
         $floor: 1, $ceiling: 1, $abs: 1,
         $add: 1, $subtract: 2, $multiply: 3, $divide: 4, $mod: 5,
         $et: 2.25
      }
   };
   var expRecs1 = [{ No: 1, a: " \n\r\t-123.1456789 \n\r\t" }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );
}