/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18010:插入数据_id索引键冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_18010";
   var cl = readyCL( clName );
   cl.insert( { _id: 1 } );

   // test
   // ReturnOID:true,ContOnDup:true
   var recsArray = [{ _id: 1, c: 1 }, { _id: 2 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ContOnDup: true } );
   checkReturnOid( rc, [1, 2] );
   var expRecs = [{ "_id": 1 }, { "_id": 2 }];
   checkRecords( cl, expRecs, false );

   // ReturnOID:true,ReplaceOnDup:true
   var recsArray = [{ _id: 3 }, { _id: 1, c: 2 }, { _id: 4 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ReplaceOnDup: true } );
   checkReturnOid( rc, [3, 1, 4] );
   var expRecs = [{ "_id": 1, "c": 2 }, { "_id": 2 }, { "_id": 3 }, { "_id": 4 }];
   checkRecords( cl, expRecs, false );

   commDropCL( db, COMMCSNAME, clName );
}
