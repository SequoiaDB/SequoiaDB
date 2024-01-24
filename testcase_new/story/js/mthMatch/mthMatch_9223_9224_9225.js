/************************************
*@Description: use and/or/not, two layer combination,index scan.
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

   //create index
   commCreateIndex( dbcl, "age", { age: -1 } );

   hintCondition = { '': '' };

   //insert data 
   var doc = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   dbcl.insert( doc );

   //and-and
   var findCondition1 = {
      $and: [{
         $and: [{ name: { $elemMatch: { firstName: "zhang1" } } },
         { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }]
      }, { age: { $et: 22 } }]
   };
   var expRecs1 = [{ No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "age",
      UseExtSort: false,
      Query: { $and: [{ age: { $et: 22 } }, { name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] },
      IXBound: { age: [[22, 22]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition1, null, null, explainExpRecs1 );

   var findCondition2 = { $and: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $et: 23 } }] };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $and: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha2", "xiasha4", "xiasha1"] } }] }, { age: { $et: 22 } }] };
   var expRecs3 = [];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //and-or
   var findCondition1 = { $and: [{ $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $et: 23 } }] };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $and: [{ $or: [{ name: { $elemMatch: { firstName: "zhang2" } } }, { "addr.2": { $in: ["xiasha4"] } }] }, { age: { $et: 22 } }] };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $and: [{ $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha2", "xiasha4", "xiasha1"] } }] }, { age: { $gte: 20 } }] };
   var expRecs3 = [{ No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   explainExpRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "age",
      UseExtSort: false,
      Query: { $and: [{ age: { $gte: 20 } }, { $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha2", "xiasha4", "xiasha1"] } }] }] },
      IXBound: { age: [[{ "$decimal": "MAX" }, 20]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );

   //and-not
   var findCondition1 = { $and: [{ $not: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $et: 23 } }] };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $and: [{ age: { $et: 23 } }, { $not: [{ name: { $elemMatch: { firstName: "zhang2" } } }, { "addr.2": { $in: ["xiasha4"] } }] }] };
   var expRecs2 = [];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $and: [{ $not: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $gte: 20 } }] };
   var expRecs3 = [{ No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   explainExpRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "ixscan",
      IndexName: "age",
      UseExtSort: false,
      Query: { $and: [{ age: { $gte: 20 } }, { $not: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }] },
      IXBound: { age: [[{ "$decimal": "MAX" }, 20]] },
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );
}

