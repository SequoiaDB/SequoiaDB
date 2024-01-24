/******************************************************************************
 * @Description   : seqDB-30253:单条插入，验证flag:SDB_INSERT_CONTONDUP_ID以及option:ContOnDupID
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.20
 * @LastEditTime  : 2023.03.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_30253";
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName = "aIdx";
   cl.createIndex( idxName, { a: 1 }, true, true );
   cl.insert( { _id: 1, a: 1, b: 1 } );

   // test flag:SDB_INSERT_CONTONDUP_ID
   var record = { _id: 1, a: 1, b: 2 };
   var res = cl.insert( record, SDB_INSERT_CONTONDUP_ID ).toObj();
   var exp = { "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 0 };
   assert.equal( res, exp );
   var expRecs = [{ _id: 1, a: 1, b: 1 }];
   checkRecords( cl, expRecs, false );

   record = { _id: 666, a: 1, b: 1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( record, SDB_INSERT_CONTONDUP_ID );
   } );

   // test option:ContOnDupID
   record = { _id: 1, a: 1, b: 3 };
   res = cl.insert( record, { ContOnDupID: true } ).toObj();
   exp = { "InsertedNum": 0, "DuplicatedNum": 1, "ModifiedNum": 0 };
   assert.equal( res, exp );
   var expRecs = [{ _id: 1, a: 1, b: 1 }];
   checkRecords( cl, expRecs, false );

   record = { _id: 1, a: 1, b: 1 };
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( record, { ContOnDupID: false } );
   } );

   // set options at same time
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( record, { ContOnDupID: true, ContOnDup: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( record, { ReplaceOnDup: true, ContOnDupID: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( record, { ContOnDupID: true, ReplaceOnDupID: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( record, { ReplaceOnDupID: true, ContOnDup: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( record, { ReplaceOnDupID: true, ReplaceOnDup: true } );
   } );
}