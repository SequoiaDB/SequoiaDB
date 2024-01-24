/*******************************************************************************
@Description : When create composite index, there can be only one nest array as a
               index key.
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

   // insert data to SDB
   idxCL.insert( { no: 001, name: "A", score: [60, 70, 80], coutry: { china: { guangdong: "guanzhou" } }, age: 15, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.1" } } );
   idxCL.insert( { no: 002, name: "B", score: [60, 71, 80], coutry: { china: { guangdong: "shenzhen" } }, age: 17, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.2" } } );
   idxCL.insert( { no: 003, name: "C", score: [62, 70, 80], coutry: { china: { guangdong: "huizhou" } }, age: 18, major: ["English", "History", "Physics"], "class": { grade: "NO.3" } } );
   idxCL.insert( { no: 004, name: "D", score: [60, 70, 85], coutry: { china: { guangdong: "foshan" } }, age: 17, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.4" } } );
   idxCL.insert( { no: 005, name: "E", score: [65, 75, 85], coutry: { china: { guangdong: "zhuhai" } }, age: 18, major: ["English", "Chinese", "Physics"], "class": { grade: "NO.5" } } );
   var i = 0;
   do
   {
      var count = idxCL.count();
      ++i;
   } while( i < 10 )
   assert.equal( 5, count );

   // create index
   assert.tryThrow( SDB_IXM_MULTIPLE_ARRAY, function()
   {
      idxCL.createIndex( "comIndex1", { "score": 1, "coutry.china.guangdong": 1, age: 1 }, true, true );
      //create index for SDB, have two more array,failed
      idxCL.createIndex( "comIndex3", { "score": 1, "coutry.china.guangdong": 1, age: 1, "class.grade": 1 }, true, true );
      idxCL.createIndex( "comIndex2", { "score": 1, "coutry.china.guangdong": 1, age: 1, "major.1": 1 }, true, true );
   } )

   // inspect index
   try
   {
      inspecIndex( idxCL, "comIndex1", "score", 1, true, true );
      inspecIndex( idxCL, "comIndex3", "score", 1, true, true );
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
