/***************************************************************************
@Description :First insert data ,and then create index. The index have
              unique ,but don't have enforced arguments.
@Modify list :
              2014-5-15  xiaojun Hu  Create
****************************************************************************/
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   // insert data to SDB
   idxCL.insert( { no: 001, name: "A", age: 2 } );
   idxCL.insert( { no: 002, name: "B", age: 2 } );
   idxCL.insert( { no: 003, name: "C", "姓名": "张" } );
   idxCL.insert( { no: 004, name: "C", "姓名": "张" } );
   count = idxCL.count();
   assert.equal( 4, count );

   // create index
   createIndex( idxCL, "noIndex", { no: 1 }, true, false );
   createIndex( idxCL, "nameIndex", { name: -1 }, true, false, SDB_IXM_DUP_KEY );
   createIndex( idxCL, "姓名索引", { "姓名": 1 }, true, false, SDB_IXM_DUP_KEY );
   createIndex( idxCL, "ageIndex", { "age": 1 }, true, false, SDB_IXM_DUP_KEY );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "noIndex", "no", 1, true, false );
      inspecIndex( idxCL, "nameIndex", "name", -1, true, false );
      inspecIndex( idxCL, "姓名索引", "姓名", 1, true, false );
      inspecIndex( idxCL, "ageIndex", "age", 1, true, false );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   //insert data after create index
   idxCL.insert( { no: 005, name: "D", "姓名": "汉", age: 8 } );
   var count = idxCL.count();
   assert.equal( 5, count );

   // drop collection in clean
   commDropCL( db, csName, clName, false, false );
}
