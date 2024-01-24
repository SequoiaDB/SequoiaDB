/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18000:批量插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据不冲突 
*               seqDB-18001:批量插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_18000";
   var idxName = "idx";
   var cl = readyCL( clName );
   cl.createIndex( idxName, { a: 1, b: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );

   // key not conflict
   var recsArray = [{ c: 1 }, { a: 2, c: 1 }, { a: 2, b: 1, c: 1 }];
   cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
   var expRecs = [{ "c": 1 }, { "a": 1, "b": 1 }, { "a": 2, "c": 1 }, { "a": 2, "b": 1, "c": 1 }];
   checkRecords( cl, expRecs );

   // key conflict
   var firstRecord = { c: 2 };
   var middRecord = { a: 2, c: 3 };
   var lastRecord = { a: 2, b: 1, c: 5 };
   var recsArray = [firstRecord, middRecord, lastRecord];
   keyConflict( cl, recsArray );

   // first record conflict
   var recsArray = [firstRecord, { a: 3 }];
   cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );

   // middle record conflict
   var recsArray = [{ a: 4 }, middRecord, { a: 4, b: 1 }];
   cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );

   // last record conflict   
   var recsArray = [{ a: 5 }, lastRecord];
   cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );

   var expRecs = [{ "c": 2 }, { "a": 1, "b": 1 }, { "a": 2, "c": 3 }, { "a": 2, "b": 1, "c": 5 }, { "a": 3 }, { "a": 4 }, { "a": 4, "b": 1 }, { "a": 5 }];
   checkRecords( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
