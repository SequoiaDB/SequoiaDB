/************************************
*@Description: use and/or/not,one layer combination,table scan.
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

   //and
   //match the left condition
   var findCondition1 = { $and: [{ name: "zhangsan0" }, { age: { $lt: 17 } }] };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //match the right condition
   var findCondition2 = { $and: [{ name: "zhangsan" }, { age: { $et: 18 } }] };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //match all condition
   var findCondition3 = { $and: [{ name: "zhangsan0" }, { age: { $gte: 19 } }] };
   var expRecs3 = [{ No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   expRecsExplain3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ name: { $et: "zhangsan0" } }, { age: { $gte: 19 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, { No: 1 }, expRecsExplain3 );

   //or
   //match the left condition
   var findCondition4 = { $or: [{ name: "zhangsan0" }, { age: { $lt: 17 } }] };
   var expRecs4 = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //match the right condition
   var findCondition5 = { $or: [{ name: "zhangsan" }, { age: { $et: 18 } }] };
   var expRecs5 = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 6, name: "zhangsan5", age: 18, major: "math5" }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //match all condition
   var findCondition6 = { $or: [{ name: "zhangsan0" }, { age: { $gte: 18 } }] };
   var expRecs6 = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" },
   { No: 4, name: "zhangsan3", age: 21, major: "math3" },
   { No: 5, name: "zhangsan4", age: 22, major: "math4" },
   { No: 6, name: "zhangsan5", age: 18, major: "math5" }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   expRecsExplain6 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $or: [{ name: { $et: "zhangsan0" } }, { age: { $gte: 18 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition6, null, { No: 1 }, expRecsExplain6 );

   //not
   //match the left condition
   var findCondition7 = { $not: [{ name: "zhangsan0" }, { age: { $lt: 17 } }] };
   var expRecs7 = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" },
   { No: 4, name: "zhangsan3", age: 21, major: "math3" },
   { No: 5, name: "zhangsan4", age: 22, major: "math4" },
   { No: 6, name: "zhangsan5", age: 18, major: "math5" }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   //match the right condition
   var findCondition8 = { $not: [{ name: "zhangsan" }, { age: { $et: 18 } }] };
   var expRecs8 = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 2, name: "zhangsan0", age: 19, major: "math1" },
   { No: 3, name: "zhangsan0", age: 20, major: "math2" },
   { No: 4, name: "zhangsan3", age: 21, major: "math3" },
   { No: 5, name: "zhangsan4", age: 22, major: "math4" },
   { No: 6, name: "zhangsan5", age: 18, major: "math5" }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );

   //match all condition
   var findCondition9 = { $not: [{ name: "zhangsan0" }, { age: { $gte: 19 } }] };
   var expRecs9 = [{ No: 1, name: "zhangsan0", age: 18, major: "math0" },
   { No: 4, name: "zhangsan3", age: 21, major: "math3" },
   { No: 5, name: "zhangsan4", age: 22, major: "math4" },
   { No: 6, name: "zhangsan5", age: 18, major: "math5" }];
   checkResult( dbcl, findCondition9, null, expRecs9, { No: 1 } );

   expRecsExplain9 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $not: [{ name: { $et: "zhangsan0" } }, { age: { $gte: 19 } }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition9, null, { No: 1 }, expRecsExplain9 );
}
