/******************************************************************************
 * @Description   : seqDB-30258:验证flag:SDB_INSERT_REPLACEONDUP_ID以及option:ReplaceOnDupID
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.20
 * @LastEditTime  : 2023.03.10
 * @LastEditors   : liuli
******************************************************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_30258";
   var idxName = "aIdx";
   var cl = readyCL( clName );
   cl.createIndex( idxName, { a: 1 }, true, true );
   cl.insert( { _id: 1, a: 1, b: 1 } );

   // test flag:SDB_INSERT_REPLACEONDUP_ID
   var record = { _id: 1, a: 1, b: 2 };
   var res = cl.insert( record, SDB_INSERT_REPLACEONDUP_ID ).toObj();
   var exp = { "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 };
   assert.equal( res, exp );
   var expRecs = [{ _id: 1, a: 1, b: 2 }];
   checkRecords( cl, expRecs, false );

   record = { _id: 666, a: 1, b: 1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( record, SDB_INSERT_REPLACEONDUP_ID );
   } );

   // test option:ReplaceOnDupID
   record = { _id: 1, a: 1, b: 3 };
   res = cl.insert( record, { ReplaceOnDupID: true } ).toObj();
   exp = { "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 1 };
   assert.equal( res, exp );
   expRecs = [{ _id: 1, a: 1, b: 3 }];
   checkRecords( cl, expRecs, false );

   record = { _id: 1, a: 1, b: 1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( record, { ContOnDupID: false } );
   } );
   checkRecords( cl, expRecs, false );
}