/************************************
*@Description: use and/or/not, two layer combination,table scan.
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
   var doc = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   dbcl.insert( doc );

   //and-and
   var findCondition1 = { $and: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $et: 22 } }] };
   var expRecs1 = [{ No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   explainExpRecs1 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ age: { $et: 22 } }, { name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] },
      IXBound: null,
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
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ age: { $gte: 20 } }, { $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha2", "xiasha4", "xiasha1"] } }] }] },
      IXBound: null,
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
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $and: [{ age: { $gte: 20 } }, { $not: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );

   //or-or
   var findCondition1 = { $or: [{ $or: [{ name: { $elemMatch: { firstName: "zhang2" } } }, { "addr.2": { $in: ["xiasha5", "xiasha4", "xiasha6"] } }] }, { age: { $et: 22 } }] };
   var expRecs1 = [{ No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $or: [{ $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $et: 23 } }] };
   var expRecs2 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $or: [{ $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $gt: 20 } }] };
   var expRecs3 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   explainExpRecs3 = [{
      Name: COMMCSNAME + "." + COMMCLNAME,
      ScanType: "tbscan",
      IndexName: "",
      UseExtSort: false,
      Query: { $or: [{ age: { $gt: 20 } }, { $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }] },
      IXBound: null,
      NeedMatch: true
   }];
   checkExplainResult( dbcl, findCondition3, null, null, explainExpRecs3 );

   //or-and
   var findCondition1 = { $or: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha1", "xiasha4", "xiasha3"] } }] }, { age: { $et: 23 } }] };
   var expRecs1 = [{ No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $or: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha5", "xiasha4", "xiasha6"] } }] }, { age: { $et: 22 } }] };
   var expRecs2 = [{ No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $or: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $gt: 20 } }] };
   var expRecs3 = [{ No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //or-not
   var findCondition1 = { $or: [{ $not: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha1", "xiasha4", "xiasha3"] } }] }, { age: { $et: 23 } }] };
   var expRecs1 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $or: [{ $not: [{ age: { $gt: 17 } }, { "addr.0": { $in: ["guangdong"] } }] }, { age: { $et: 22 } }] };
   var expRecs2 = [{ No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $or: [{ $not: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $gt: 20 } }] };
   var expRecs3 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //not-and
   var findCondition1 = { $not: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha1", "xiasha4", "xiasha3"] } }] }, { age: { $et: 23 } }] };
   var expRecs1 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $not: [{ $and: [{ name: { $elemMatch: { firstName: "zhang2" } } }, { "addr.2": { $in: ["xiasha1", "xiasha4", "xiasha3"] } }] }, { age: { $et: 22 } }] };
   var expRecs2 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $not: [{ $and: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $gt: 20 } }] };
   var expRecs3 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //not-or
   var findCondition1 = { $not: [{ $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha1", "xiasha4", "xiasha3"] } }] }, { age: { $et: 23 } }] };
   var expRecs1 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $not: [{ $or: [{ name: { $elemMatch: { firstName: "zhang2" } } }, { "addr.2": { $in: ["xiasha5", "xiasha4", "xiasha6"] } }] }, { age: { $et: 22 } }] };
   var expRecs2 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $not: [{ $or: [{ name: { $elemMatch: { firstName: "zhang1" } } }, { "addr.2": { $in: ["xiasha3", "xiasha4", "xiasha1"] } }] }, { age: { $gt: 20 } }] };
   var expRecs3 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //not-not
   var findCondition1 = { $not: [{ $not: [{ age: { $gt: 17 } }, { "addr.0": { $in: ["guangdong"] } }] }, { age: { $et: 22 } }] };
   var expRecs1 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { $not: [{ $not: [{ age: { $et: 22 } }, { "addr.0": { $in: ["guangdong"] } }] }, { age: { $et: 23 } }] };
   var expRecs2 = [{ No: 1, name: { firstName: "zhang", lastName: "san" }, age: 18, addr: ["guangdong", "shenzhen1", "xiasha1", "yuehubuilding5", 1710] },
   { No: 2, name: { firstName: "zhang", lastName: "san1" }, age: 19, addr: ["guangdong", "shenzhen2", "xiasha1", "yuehubuilding5", 1711] },
   { No: 3, name: { firstName: "zhang", lastName: "san2" }, age: 20, addr: ["guangdong", "shenzhen3", "xiasha2", "yuehubuilding4", 1810] },
   { No: 4, name: { firstName: "zhang", lastName: "san" }, age: 21, addr: ["guangdong", "shenzhen4", "xiasha2", "yuehubuilding4", 1910] },
   { No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   var findCondition3 = { $not: [{ $not: [{ age: { $et: 22 } }, { "addr.0": { $in: ["guangdong"] } }] }, { name: { $elemMatch: { firstName: "zhang" } } }] };
   var expRecs3 = [{ No: 5, name: { firstName: "zhang", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen5", "xiasha3", "yuehubuilding1", 1712] },
   { No: 6, name: { firstName: "zhang1", lastName: "san3" }, age: 22, addr: ["guangdong", "shenzhen6", "xiasha3", "yuehubuilding2", 1715] }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );
}
