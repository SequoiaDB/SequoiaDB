/******************************************************************************
 * @Description   : seqDB-30255:批量插入，验证flag:SDB_INSERT_CONTONDUP_ID以及option:ContOnDupID
 * @Author        : Cheng Jingjing
 * @CreateTime    : 2023.02.20
 * @LastEditTime  : 2023.03.10
 * @LastEditors   : liuli
******************************************************************************/
testConf.clName = COMMCLNAME + "_30255";
main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   var idxName = "aIdx";
   cl.createIndex( idxName, { a: 1 }, true, true );
   var rec = { _id: 1, a: 1, b: 1 };
   cl.insert( rec );

   // test flag:SDB_INSERT_CONTONDUP_ID
   var recsArray = [{ _id: 11, a: 11 }, { _id: 22, a: 22, b: 22 }, { _id: 1, a: 1, b: 3 }];
   var res = cl.insert( recsArray, SDB_INSERT_CONTONDUP_ID ).toObj();
   var exp = { "InsertedNum": 2, "DuplicatedNum": 1, "ModifiedNum": 0 };
   assert.equal( res, exp );
   var expRecs = [{ _id: 1, a: 1, b: 1 }, { _id: 11, a: 11 }, { _id: 22, a: 22, b: 22 }];
   checkRecords( cl, expRecs, false );
   cl.truncate();

   cl.insert( rec );
   recsArray = [{ _id: 11, a: 11 }, { _id: 22, a: 22, b: 22 }, { _id: 33, a: 1, b: 33 }];
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( recsArray, SDB_INSERT_CONTONDUP_ID );
   } );
   expRecs = [{ _id: 1, a: 1, b: 1 }, { _id: 11, a: 11 }, { _id: 22, a: 22, b: 22 }];
   checkRecords( cl, expRecs, false );
   cl.truncate();

   // test option:ContOnDupID
   cl.insert( rec );
   recsArray = [{ _id: 1, a: 11 }, { _id: 22, a: 22, b: 22 }, { _id: 33, a: 33, b: 33 }];
   res = cl.insert( recsArray, { ContOnDupID: true } ).toObj();
   exp = { "InsertedNum": 2, "DuplicatedNum": 1, "ModifiedNum": 0 };
   assert.equal( res, exp );
   expRecs = [{ _id: 1, a: 1, b: 1 }, { _id: 22, a: 22, b: 22 }, { _id: 33, a: 33, b: 33 }];
   checkRecords( cl, expRecs, false );
   cl.truncate();

   cl.insert( rec );
   recsArray = [{ _id: 1, a: 11 }, { _id: 22, a: 1, b: 22 }, { _id: 33, a: 33, b: 33 }];
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( recsArray, { ContOnDupID: false } );
   } );
   expRecs = [{ _id: 1, a: 1, b: 1 }];
   checkRecords( cl, expRecs, false );
}
