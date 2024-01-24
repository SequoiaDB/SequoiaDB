/************************************
*@Description: index field,use gt/gte/lt/lte to comapare,
               value set boundary of date/timestamp
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

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //insert data 
   var doc = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   dbcl.insert( doc );

   //left boundary of date
   //$gt:1900-01-01
   var findCondition1 = { a: { $gt: { $date: "1900-01-01" } } };
   var expRecs1 = [{ No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //$gte:1900-01-01
   var findCondition2 = { a: { $gte: { $date: "1900-01-01" } } };
   var expRecs2 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );

   //$lt:1900-01-01
   var findCondition3 = { a: { $lt: { $date: "1900-01-01" } } };
   var expRecs3 = [];
   checkResult( dbcl, findCondition3, null, expRecs3, { No: 1 } );

   //$lte:1900-01-01
   var findCondition4 = { a: { $lte: { $date: "1900-01-01" } } };
   var expRecs4 = [{ No: 1, a: { $date: "1900-01-01" } }];
   checkResult( dbcl, findCondition4, null, expRecs4, { No: 1 } );

   //right boundary of date
   //$gt:9999-12-31
   var findCondition5 = { a: { $gt: { $date: "9999-12-31" } } };
   var expRecs5 = [];
   checkResult( dbcl, findCondition5, null, expRecs5, { No: 1 } );

   //$gte:9999-12-31
   var findCondition6 = { a: { $gte: { $date: "9999-12-31" } } };
   var expRecs6 = [{ No: 5, a: { $date: "9999-12-31" } }];
   checkResult( dbcl, findCondition6, null, expRecs6, { No: 1 } );

   //$lt:9999-12-31
   var findCondition7 = { a: { $lt: { $date: "9999-12-31" } } };
   var expRecs7 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition7, null, expRecs7, { No: 1 } );

   //$lte:9999-12-31
   var findCondition8 = { a: { $lte: { $date: "9999-12-31" } } };
   var expRecs8 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition8, null, expRecs8, { No: 1 } );

   //left boundary of timestamp
   //$gt:1902-01-01-00.00.00.000000
   var findCondition9 = { a: { $gt: { $timestamp: "1902-01-01-00.00.00.000000" } } };
   var expRecs9 = [{ No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition9, null, expRecs9, { No: 1 } );

   //$gte:1902-01-01-00.00.00.000000
   var findCondition10 = { a: { $gte: { $timestamp: "1902-01-01-00.00.00.000000" } } };
   var expRecs10 = [{ No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition10, null, expRecs10, { No: 1 } );

   //$lt:1902-01-01-00.00.00.000000
   var findCondition11 = { a: { $lt: { $timestamp: "1902-01-01-00.00.00.000000" } } };
   var expRecs11 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } }];
   checkResult( dbcl, findCondition11, null, expRecs11, { No: 1 } );

   //$lte:1902-01-01-00.00.00.000000
   var findCondition12 = { a: { $lte: { $timestamp: "1902-01-01-00.00.00.000000" } } };
   var expRecs12 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }];
   checkResult( dbcl, findCondition12, null, expRecs12, { No: 1 } );

   //right boundary of timestamp
   //$gt:2037-12-31-23.59.59.999999
   var findCondition13 = { a: { $gt: { $timestamp: "2037-12-31-23.59.59.999999" } } };
   var expRecs13 = [{ No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } }];
   checkResult( dbcl, findCondition13, null, expRecs13, { No: 1 } );

   //$gte:2037-12-31-23.59.59.999999
   var findCondition14 = { a: { $gte: { $timestamp: "2037-12-31-23.59.59.999999" } } };
   var expRecs14 = [{ No: 5, a: { $date: "9999-12-31" } }, { No: 6, a: { $date: "9999-12-30" } },
   { No: 7, a: { $date: "9999-11-30" } }, { No: 8, a: { $date: "9998-12-31" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition14, null, expRecs14, { No: 1 } );

   //$lt:2037-12-31-23.59.59.999999
   var findCondition15 = { a: { $lt: { $timestamp: "2037-12-31-23.59.59.999999" } } };
   var expRecs15 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition15, null, expRecs15, { No: 1 } );

   //$lte:2037-12-31-23.59.59.999999
   var findCondition16 = { a: { $lte: { $timestamp: "2037-12-31-23.59.59.999999" } } };
   var expRecs16 = [{ No: 1, a: { $date: "1900-01-01" } }, { No: 2, a: { $date: "1900-01-02" } },
   { No: 3, a: { $date: "1900-02-01" } }, { No: 4, a: { $date: "1901-01-01" } },
   { No: 9, a: { $timestamp: "1902-01-01-00.00.00.000000" } }, { No: 10, a: { $timestamp: "1902-01-01-00.00.00.000001" } },
   { No: 11, a: { $timestamp: "1902-01-01-00.00.01.000000" } }, { No: 12, a: { $timestamp: "1902-01-01-00.01.00.000000" } },
   { No: 13, a: { $timestamp: "1902-01-01-01.00.00.000000" } }, { No: 14, a: { $timestamp: "1902-01-02-00.00.00.000000" } },
   { No: 15, a: { $timestamp: "1902-02-01-00.00.00.000000" } }, { No: 16, a: { $timestamp: "1903-01-01-00.00.00.000000" } },
   { No: 17, a: { $timestamp: "2037-12-31-23.59.59.999999" } }, { No: 18, a: { $timestamp: "2037-12-31-23.59.59.999998" } },
   { No: 19, a: { $timestamp: "2037-12-31-23.59.58.999999" } }, { No: 20, a: { $timestamp: "2037-12-31-23.58.59.999999" } },
   { No: 21, a: { $timestamp: "2037-12-31-22.59.59.999999" } }, { No: 22, a: { $timestamp: "2037-12-30-23.59.59.999999" } },
   { No: 23, a: { $timestamp: "2037-11-30-23.59.59.999999" } }, { No: 24, a: { $timestamp: "2036-12-31-23.59.59.999999" } }];
   checkResult( dbcl, findCondition16, null, expRecs16, { No: 1 } );
}
