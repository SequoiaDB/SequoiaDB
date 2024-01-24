/************************************
*@Description: update object does not exist,use operator pull_all
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
main( test );
function test ()
{
   //clear environment
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data
   var doc1 = [{ object6: [10, -30, 20] },
   { object7: [200, [305, -299, 400], 400] }];
   dbcl.insert( doc1 );

   //update use pull_all object does not exist,no matches
   var updateCondition1 = {
      $pull_all: {
         object1: [10, 20],
         object2: [200, [305, -299], 400],
         object3: [false, "string", { $date: "2016-05-16" }],
         "object4.1": [400, [30, -10], 500],
         "object5.1": [200]
      }
   };
   dbcl.update( updateCondition1 );

   //check result
   var expRecs1 = [{ object6: [10, -30, 20] },
   { object7: [200, [305, -299, 400], 400] }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

   //update use pull_all object does not exist,with matches
   var updateCondition2 = {
      $pull_all: {
         object6: [10, 20],
         object7: [200, 400],
         object3: [false, "string", { $date: "2016-05-16" }],
         "object4.1": [400, [30, -10], 500],
         "object5.1": [200]
      }
   };
   var findCondition2 = { object7: { $exists: 1 } };
   dbcl.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ object6: [10, -30, 20] },
   { object7: [[305, -299, 400]] }];
   checkResult( dbcl, null, null, expRecs2, { _id: 1 } );
}

