/************************************
*@Description: use and,index scan.
*@author:      zhaoyu
*@createdate:  2016.8.11
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
   var doc = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" },
   { No: 4, name: "zhangsan3", age: 21, major: "math3" },
   { No: 5, name: "zhangsan4", age: 22, major: "math4" },
   { No: 6, name: "zhangsan5", age: 18, major: "math5" }];
   dbcl.insert( doc );

   //create index
   commCreateIndex( dbcl, "name", { name: -1 } );
   commCreateIndex( dbcl, "age", { age: 1 } );

   //and
   //match the left condition
   var findCondition1 = { $and: [{ name: "zhangsan0" }, { age: { $lt: 17 } }] };
   var expRecs1 = [];

   //match the right condition
   var findCondition2 = { $and: [{ name: "zhangsan" }, { age: { $et: 18 } }] };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //match all condition
   var findCondition3 = { $and: [{ name: "zhangsan0" }, { age: { $gte: 19 } }] };
   var expRecs3 = [{ No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" }];
   checkResult( dbcl, findCondition3, null, expRecs3, { _id: 1 } );

   expRecsExplain3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "name",
      UseExtSort: false,
      Query: { $and: [{ name: { $et: "zhangsan0" } }, { age: { $gte: 19 } }] },
      IXBound: { name: [["zhangsan0", "zhangsan0"]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, expRecsExplain3 );
}
