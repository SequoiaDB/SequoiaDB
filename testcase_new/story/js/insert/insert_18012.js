/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18012:批量插入存在多个索引键冲突的记录
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_18012";
   var idxName = "idx";
   var cl = readyCL( clName );
   cl.createIndex( idxName, { a: 1 }, true, true );
   cl.insert( [{ a: 1 }, { a: 2 }] );

   // test
   var recsArray = [{ a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, c: 1 }, { a: 3 }];
   var rc = cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
   var expRecs = [{ "a": 1, "b": 2 }, { "a": 2, "c": 1 }, { "a": 3 }];
   checkRecords( cl, expRecs );
   commDropCL( db, COMMCSNAME, clName );
}
