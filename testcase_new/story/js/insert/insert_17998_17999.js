/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-17998:单条插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据不冲突 
*               seqDB-17999:单条插入，指定flag为SDB_INSERT_REPLACEONDUP，插入数据冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_17998";
   var idxName = "idx";
   var cl = readyCL( clName );
   cl.createIndex( idxName, { a: 1, b: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );

   // key not conflict
   var recsArray = [{ c: 1 }, { a: 2, c: 2 }, { a: 3, b: 3, c: 3 }];
   keyNotConflict( cl, recsArray );
   var expRecs = [{ "c": 1 }, { "a": 1, "b": 1 }, { "a": 2, "c": 2 }, { "a": 3, "b": 3, "c": 3 }];
   checkRecords( cl, expRecs );

   // key conflict
   var recsArray = [{ c: 2 }, { a: 2, c: 3 }, { a: 3, b: 3, c: 4 }];
   keyConflict( cl, recsArray );

   // key conflict, set flag[SDB_INSERT_REPLACEONDUP]
   for( var i = 0; i < recsArray.length; i++ )
   {
      cl.insert( recsArray[i], SDB_INSERT_REPLACEONDUP );
   }
   var expRecs = [{ "c": 2 }, { "a": 1, "b": 1 }, { "a": 2, "c": 3 }, { "a": 3, "b": 3, "c": 4 }];
   checkRecords( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}

function keyNotConflict ( cl, recsArray )
{
   for( var i = 0; i < recsArray.length; i++ )
   {
      cl.insert( recsArray[i], SDB_INSERT_REPLACEONDUP );
   }
}
