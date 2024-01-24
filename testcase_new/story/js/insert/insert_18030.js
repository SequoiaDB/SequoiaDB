/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18030:插入数据对多个索引冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_18030";
   var cl = readyCL( clName );
   cl.createIndex( "idx1", { a: 1, b: 1 }, true, true );
   cl.createIndex( "idx2", { a: 1, c: 1 }, true, true );
   cl.insert( { a: 1 } );

   // test
   // SDB_INSERT_CONTONDUP
   var recsArray = [{ a: 1, d: 1 }, { a: 2 }];
   var rc = cl.insert( recsArray, SDB_INSERT_CONTONDUP );
   var expRecs = [{ "a": 1 }, { "a": 2 }];
   checkRecords( cl, expRecs );

   // SDB_INSERT_REPLACEONDUP
   var recsArray = [{ a: 1, d: 2 }, { a: 3, b: 1, c: 1 }];
   var rc = cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
   var expRecs = [{ "a": 1, "d": 2 }, { "a": 2 }, { "a": 3, "b": 1, "c": 1 }];
   checkRecords( cl, expRecs );
   commDropCL( db, COMMCSNAME, clName );
}
