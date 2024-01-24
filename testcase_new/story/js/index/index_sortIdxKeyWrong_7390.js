/*******************************************************************************
@Description : When creating index , we specify the sort key :1/-1. If not equal
               1 and -1, there is a error.
@Modify list :
               2014-5-19  xiaojun Hu  Create
*******************************************************************************/
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, csName, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, csName, clName, {}, true, false, "create collection" );

   //insert data after create index
   idxCL.insert( { no: 001, name: "A", Des: "a" } );
   idxCL.insert( { no: 002, name: "B", Des: "b" } );
   idxCL.insert( { no: 003, name: "C", Des: "c" } );
   idxCL.insert( { no: 004, name: "D", Des: "d" } );
   idxCL.insert( { no: 005, name: "E", Des: "e" } );
   var count = idxCL.count();
   var i = 0;
   do
   {
      cnt = count;
      ++i;
   } while( i < 10 )
   assert.equal( 5, count );

   // create index
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      createIndex( idxCL, "noIndex", { no: 1 }, false, false );
      // create index. specify the sort key:-1
      createIndex( idxCL, "nameIndex", { name: -1 }, false, false );
      // create index. specify the sort key:5
      createIndex( idxCL, "desIndex", { Des: 5 }, false, false );
   } );

   // create index. specify the sort key:-5
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      idxCL.createIndex( "desIndex", { Des: -5 } );
   } );

   // create index. specify the sort key:0
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      idxCL.createIndex( "desIndex", { Des: 0 } );
   } );

   // inspect the index
   try
   {
      inspecIndex( idxCL, "noIndex", "no", 1, false, false );
      inspecIndex( idxCL, "nameIndex", "name", -1, false, false );
   }
   catch( e )
   {
      if( "ErrIdxName" != e.message )
      {
         throw e;
      }
   }

   // drop collection in clean
   commDropCL( db, csName, clName, false, false );
}
