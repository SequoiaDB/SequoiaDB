/******************************************************************************
*@Description : insert, test flag and options                                
*               seqDB-18003:批量插入，多个options组合使用，插入数据冲突
*@Author      : 2019-3-13  XiaoNi Huang
******************************************************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_18003";
   var idxName = "idx";
   var cl = readyCL( clName );
   cl.createIndex( idxName, { a: 1, b: 1 }, true, true );

   // test
   setOptions_RC_true( cl );
   setOptions_RR_true( cl );
   setOptions_RC_false( cl );
   setOptions_RR_false( cl );
   setOptions_CR( cl );

   commDropCL( db, COMMCSNAME, clName );
}

function setOptions_RC_true ( cl )
{
   cl.insert( { _id: 1, a: 1, b: 1 } );
   var recsArray = [{ _id: 2, a: 1, b: 1, c: 1 }, { _id: 3, a: 2 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ContOnDup: true } );
   checkReturnOid( rc, [2, 3] );
   var expRecs = [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 3, "a": 2 }];
   checkRecords( cl, expRecs, false );
   cl.remove();
}

function setOptions_RR_true ( cl )
{
   cl.insert( { _id: 1, a: 1, b: 1 } );
   var recsArray = [{ _id: 2, a: 1, b: 1, c: 2 }, { _id: 3, a: 3 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ReplaceOnDup: true } );
   checkReturnOid( rc, [2, 3] );
   var expRecs = [{ "_id": 1, "a": 1, "b": 1, "c": 2 }, { "_id": 3, "a": 3 }];
   checkRecords( cl, expRecs, false );
   cl.remove();
}

function setOptions_RC_false ( cl )
{
   cl.insert( { _id: 1, a: 1, b: 1 } );
   var recsArray = [{ a: 1, b: 1, c: 3 }, { a: 3 }];
   try
   {
      cl.insert( recsArray, { ReturnOID: false, ContOnDup: false } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_IXM_DUP_KEY != e.message )
      {
         throw e;
      }
   }

   var expRecs = [{ "_id": 1, "a": 1, "b": 1 }];
   checkRecords( cl, expRecs, false );
   cl.remove();
}

function setOptions_RR_false ( cl )
{
   cl.insert( { _id: 1, a: 1, b: 1 } );
   var recsArray = [{ a: 1, b: 1, c: 4 }, { a: 2 }];
   try
   {
      cl.insert( recsArray, { ReturnOID: false, ReplaceOnDup: false } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_IXM_DUP_KEY != e.message )
      {
         throw e;
      }
   }

   var expRecs = [{ "_id": 1, "a": 1, "b": 1 }];
   checkRecords( cl, expRecs, false );

   cl.remove();
}

function setOptions_CR ( cl )
{
   cl.insert( { _id: 1, a: 1, b: 1 } );

   // ContOnDup:true,ReplaceOnDup:true
   var recsArray = [{ _id: 2, a: 1, b: 1, c: 5 }, { _id: 3, a: 5 }];
   try
   {
      cl.insert( recsArray, { ContOnDup: true, ReplaceOnDup: true } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )
      {
         throw e;
      }
   }
   var expRecs = [{ "_id": 1, "a": 1, "b": 1 }];
   checkRecords( cl, expRecs, false );

   // ContOnDup:true,ReplaceOnDup:false
   var recsArray = [{ _id: 4, a: 1, b: 1, c: 6 }, { _id: 5, a: 6 }];
   var rc = cl.insert( recsArray, { ReturnOID: true, ContOnDup: true, ReplaceOnDup: false } );
   checkReturnOid( rc, [4, 5] );
   var expRecs = [{ "_id": 1, "a": 1, "b": 1 }, { "_id": 5, "a": 6 }];
   checkRecords( cl, expRecs, false );

   // ContOnDup:false,ReplaceOnDup:true
   var recsArray = [{ _id: 6, a: 1, b: 1, c: 7 }, { _id: 7, a: 7 }];
   cl.insert( recsArray, { ContOnDup: false, ReplaceOnDup: true } );
   var expRecs = [{ "_id": 1, "a": 1, "b": 1, "c": 7 }, { _id: 5, "a": 6 }, { _id: 7, "a": 7 }];
   checkRecords( cl, expRecs, false );

   // ContOnDup:false,ReplaceOnDup:false
   var recsArray = [{ _id: 8, a: 1, b: 1, c: 8 }, { _id: 9, a: 8 }];
   try
   {
      cl.insert( recsArray, { ContOnDup: false, ReplaceOnDup: false } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_IXM_DUP_KEY != e.message )
      {
         throw e;
      }
   }
   checkRecords( cl, expRecs, false );

   cl.remove();
}
