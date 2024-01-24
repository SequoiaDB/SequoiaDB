/************************************
*@Description: table scan,use gt/gte/lt/lte to comapare,
               value set date/timestamp
*@author:      zhaoyu
*@createdate:  2016.8.3
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
   var doc = [{ No: 1, a: 20170802 }, { No: 2, a: 20170803 }, { No: 3, a: 20170804 },
   { No: 4, a: 1501689600 }, { No: 5, a: 1501689601 }, { No: 6, a: 1501689599 },
   { No: 7, a: { $numberLong: "20170802" } }, { No: 8, a: { $numberLong: "20170803" } }, { No: 9, a: { $numberLong: "20170804" } },
   { No: 10, a: { $decimal: "20170802" } }, { No: 11, a: { $decimal: "20170803" } }, { No: 12, a: { $decimal: "20170804" } },
   { No: 13, a: 20170802.01 }, { No: 14, a: 20170803.01 }, { No: 15, a: 20170804.01 },
   { No: 16, a: { $date: "2017-08-02" } }, { No: 17, a: { $date: "2017-08-03" } }, { No: 18, a: { $date: "2017-08-04" } },
   { No: 19, a: { $date: "2017-07-02" } }, { No: 20, a: { $date: "2017-09-03" } },
   { No: 21, a: { $date: "2016-08-03" } }, { No: 22, a: { $date: "2018-08-03" } },
   { No: 23, a: { $date: "3016-08-03" } },
   { No: 24, a: { $timestamp: "2017-08-03-15.32.17.999999" } }, { No: 25, a: { $timestamp: "2017-08-03-15.32.18.000000" } },
   { No: 26, a: { $timestamp: "2017-08-03-15.32.18.000001" } },
   { No: 27, a: { $timestamp: "2017-08-03-15.33.18.000000" } }, { No: 28, a: { $timestamp: "2017-08-03-15.31.18.000000" } },
   { No: 29, a: { $timestamp: "2017-08-03-16.32.18.000000" } }, { No: 30, a: { $timestamp: "2017-08-03-14.32.18.000000" } },
   { No: 31, a: { $timestamp: "2017-08-02-15.32.18.000000" } }, { No: 32, a: { $timestamp: "2017-08-04-15.32.18.000000" } },
   { No: 33, a: { $timestamp: "2017-07-03-15.32.18.000000" } }, { No: 34, a: { $timestamp: "2017-09-03-15.32.18.000000" } },
   { No: 35, a: { $timestamp: "2016-08-03-15.32.18.000000" } }, { No: 36, a: { $timestamp: "2018-08-03-15.32.18.000000" } },
   { No: 37, a: { $timestamp: "2037-08-03-15.32.18.000000" } },
   { No: 38, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 39, a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { No: 40, a: { $regex: "^z", "$options": "i" } },
   { No: 41, a: null },
   { No: 42, a: "abc" },
   { No: 43, a: MinKey() },
   { No: 44, a: MaxKey() },
   { No: 45, a: 1501745537 }, { No: 46, a: 1501745538 }, { No: 47, a: 1501745539 },
   { No: 48, a: 1501745538.01 }, { No: 49, a: 1501745537.99 },
   { No: 50, a: 1501689600.01 }, { No: 51, a: 1501689599.99 },
   { No: 52, a: true }, { No: 53, a: false },
   { No: 54, a: { name: "zhang" } },
   { No: 55, a: [1, 2, 3] }];
   dbcl.insert( doc );

   //date
   //$gt:2017-08-03
   var findCondition1 = { a: { $gt: { $date: "2017-08-03" } } };
   var expRecs1 = [{ No: 18, a: { $date: "2017-08-04" } },
   { No: 20, a: { $date: "2017-09-03" } },
   { No: 22, a: { $date: "2018-08-03" } },
   { No: 23, a: { $date: "3016-08-03" } },
   { No: 24, a: { $timestamp: "2017-08-03-15.32.17.999999" } }, { No: 25, a: { $timestamp: "2017-08-03-15.32.18.000000" } },
   { No: 26, a: { $timestamp: "2017-08-03-15.32.18.000001" } },
   { No: 27, a: { $timestamp: "2017-08-03-15.33.18.000000" } }, { No: 28, a: { $timestamp: "2017-08-03-15.31.18.000000" } },
   { No: 29, a: { $timestamp: "2017-08-03-16.32.18.000000" } }, { No: 30, a: { $timestamp: "2017-08-03-14.32.18.000000" } },
   { No: 32, a: { $timestamp: "2017-08-04-15.32.18.000000" } },
   { No: 34, a: { $timestamp: "2017-09-03-15.32.18.000000" } },
   { No: 36, a: { $timestamp: "2018-08-03-15.32.18.000000" } },
   { No: 37, a: { $timestamp: "2037-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //$gte:2017-08-03
   var findCondition2 = { a: { $gte: { $date: "2017-08-03" } } };
   var expRecs2 = [{ No: 17, a: { $date: "2017-08-03" } }, { No: 18, a: { $date: "2017-08-04" } },
   { No: 20, a: { $date: "2017-09-03" } },
   { No: 22, a: { $date: "2018-08-03" } },
   { No: 23, a: { $date: "3016-08-03" } },
   { No: 24, a: { $timestamp: "2017-08-03-15.32.17.999999" } }, { No: 25, a: { $timestamp: "2017-08-03-15.32.18.000000" } },
   { No: 26, a: { $timestamp: "2017-08-03-15.32.18.000001" } },
   { No: 27, a: { $timestamp: "2017-08-03-15.33.18.000000" } }, { No: 28, a: { $timestamp: "2017-08-03-15.31.18.000000" } },
   { No: 29, a: { $timestamp: "2017-08-03-16.32.18.000000" } }, { No: 30, a: { $timestamp: "2017-08-03-14.32.18.000000" } },
   { No: 32, a: { $timestamp: "2017-08-04-15.32.18.000000" } },
   { No: 34, a: { $timestamp: "2017-09-03-15.32.18.000000" } },
   { No: 36, a: { $timestamp: "2018-08-03-15.32.18.000000" } },
   { No: 37, a: { $timestamp: "2037-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //$lt:2017-08-03
   var findCondition3 = { a: { $lt: { $date: "2017-08-03" } } };
   var expRecs3 = [{ No: 16, a: { $date: "2017-08-02" } },
   { No: 19, a: { $date: "2017-07-02" } },
   { No: 21, a: { $date: "2016-08-03" } },
   { No: 31, a: { $timestamp: "2017-08-02-15.32.18.000000" } },
   { No: 33, a: { $timestamp: "2017-07-03-15.32.18.000000" } },
   { No: 35, a: { $timestamp: "2016-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //$lte:2017-08-03
   var findCondition4 = { a: { $lte: { $date: "2017-08-03" } } };
   var expRecs4 = [{ No: 16, a: { $date: "2017-08-02" } }, { No: 17, a: { $date: "2017-08-03" } },
   { No: 19, a: { $date: "2017-07-02" } },
   { No: 21, a: { $date: "2016-08-03" } },
   { No: 31, a: { $timestamp: "2017-08-02-15.32.18.000000" } },
   { No: 33, a: { $timestamp: "2017-07-03-15.32.18.000000" } },
   { No: 35, a: { $timestamp: "2016-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //timestamp
   //$gt:2017-08-03-15.32.18.000000
   var findCondition5 = { a: { $gt: { $timestamp: "2017-08-03-15.32.18.000000" } } };
   var expRecs5 = [{ No: 18, a: { $date: "2017-08-04" } }, { No: 20, a: { $date: "2017-09-03" } },
   { No: 22, a: { $date: "2018-08-03" } },
   { No: 23, a: { $date: "3016-08-03" } },
   { No: 26, a: { $timestamp: "2017-08-03-15.32.18.000001" } },
   { No: 27, a: { $timestamp: "2017-08-03-15.33.18.000000" } },
   { No: 29, a: { $timestamp: "2017-08-03-16.32.18.000000" } },
   { No: 32, a: { $timestamp: "2017-08-04-15.32.18.000000" } },
   { No: 34, a: { $timestamp: "2017-09-03-15.32.18.000000" } },
   { No: 36, a: { $timestamp: "2018-08-03-15.32.18.000000" } },
   { No: 37, a: { $timestamp: "2037-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //$gte:2017-08-03-15.32.18.000000
   var findCondition6 = { a: { $gte: { $timestamp: "2017-08-03-15.32.18.000000" } } };
   var expRecs6 = [{ No: 18, a: { $date: "2017-08-04" } }, { No: 20, a: { $date: "2017-09-03" } },
   { No: 22, a: { $date: "2018-08-03" } },
   { No: 23, a: { $date: "3016-08-03" } },
   { No: 25, a: { $timestamp: "2017-08-03-15.32.18.000000" } },
   { No: 26, a: { $timestamp: "2017-08-03-15.32.18.000001" } },
   { No: 27, a: { $timestamp: "2017-08-03-15.33.18.000000" } },
   { No: 29, a: { $timestamp: "2017-08-03-16.32.18.000000" } },
   { No: 32, a: { $timestamp: "2017-08-04-15.32.18.000000" } },
   { No: 34, a: { $timestamp: "2017-09-03-15.32.18.000000" } },
   { No: 36, a: { $timestamp: "2018-08-03-15.32.18.000000" } },
   { No: 37, a: { $timestamp: "2037-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   //$lt:2017-08-03-15.32.18.000000
   var findCondition7 = { a: { $lt: { $timestamp: "2017-08-03-15.32.18.000000" } } };
   var expRecs7 = [{ No: 16, a: { $date: "2017-08-02" } }, { No: 17, a: { $date: "2017-08-03" } },
   { No: 19, a: { $date: "2017-07-02" } },
   { No: 21, a: { $date: "2016-08-03" } },
   { No: 24, a: { $timestamp: "2017-08-03-15.32.17.999999" } },
   { No: 28, a: { $timestamp: "2017-08-03-15.31.18.000000" } },
   { No: 30, a: { $timestamp: "2017-08-03-14.32.18.000000" } },
   { No: 31, a: { $timestamp: "2017-08-02-15.32.18.000000" } },
   { No: 33, a: { $timestamp: "2017-07-03-15.32.18.000000" } },
   { No: 35, a: { $timestamp: "2016-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   //$lte:2017-08-03-15.32.18.000000
   var findCondition8 = { a: { $lte: { $timestamp: "2017-08-03-15.32.18.000000" } } };
   var expRecs8 = [{ No: 16, a: { $date: "2017-08-02" } }, { No: 17, a: { $date: "2017-08-03" } },
   { No: 19, a: { $date: "2017-07-02" } },
   { No: 21, a: { $date: "2016-08-03" } },
   { No: 24, a: { $timestamp: "2017-08-03-15.32.17.999999" } }, { No: 25, a: { $timestamp: "2017-08-03-15.32.18.000000" } },
   { No: 28, a: { $timestamp: "2017-08-03-15.31.18.000000" } },
   { No: 30, a: { $timestamp: "2017-08-03-14.32.18.000000" } },
   { No: 31, a: { $timestamp: "2017-08-02-15.32.18.000000" } },
   { No: 33, a: { $timestamp: "2017-07-03-15.32.18.000000" } },
   { No: 35, a: { $timestamp: "2016-08-03-15.32.18.000000" } }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );
}
