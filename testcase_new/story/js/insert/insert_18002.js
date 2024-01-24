/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18002:主子表/分区表，插入数据索引键冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "mcl18002";
   var subCLName = "scl18002";

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Failed to drop maincl in the pre-condition." );
   commDropCL( db, COMMCSNAME, subCLName, true, true, "Failed to drop subcl in the pre-condition." );

   // ready cl, the subcl is sharding cl
   var cl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   commCreateCL( db, COMMCSNAME, subCLName, { ShardingKey: { a: 1 } } );
   cl.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { a: 1 }, UpBound: { a: 100 } } );
   cl.createIndex( "idx", { a: 1 }, true, true );

   // test
   cl.insert( { a: 1 } );
   // SDB_INSERT_CONTONDUP
   var recsArray = [{ a: 1, b: 1 }, { a: 2 }];
   cl.insert( recsArray, SDB_INSERT_CONTONDUP );
   var expRecs = [{ "a": 1 }, { "a": 2 }];
   checkRecords( cl, expRecs );

   // SDB_INSERT_REPLACEONDUP
   var recsArray = [{ a: 1, b: 2 }, { a: 3 }];
   cl.insert( recsArray, SDB_INSERT_REPLACEONDUP );
   var expRecs = [{ "a": 1, "b": 2 }, { "a": 2 }, { "a": 3 }];
   checkRecords( cl, expRecs );

   // SDB_INSERT_RETURN_ID
   var rc = cl.insert( { a: 1, b: 3 }, { ReturnOID: true, ReplaceOnDup: true } );
   assert.notEqual( rc, null );
   var expRecs = [{ "a": 1, "b": 3 }, { "a": 2 }, { "a": 3 }];
   checkRecords( cl, expRecs );

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName );
}
