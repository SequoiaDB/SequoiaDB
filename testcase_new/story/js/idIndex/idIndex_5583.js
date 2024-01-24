/***************************************************************************
@Description :seqDB-5583:删除Id索引，执行插入/更新/删除数据操作
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5583";
main( test );

function test ()
{
   // drop collection in the beginning
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection in the beginning" );

   // create collection
   var idxCL = commCreateCL( db, COMMCSNAME, clName, { Compressed: true }, true, true );

   //drop idIndex
   idxCL.dropIdIndex();

   //insert data
   var doc = [];
   for( var i = 0; i < 100; ++i )
   {
      doc.push( { _id: i, a: "test" + i } );
   }
   idxCL.insert( doc );

   //update data
   idxCL.find( { _id: 3 } ).update( { $set: { a: "testa" } } );

   //remove data
   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      idxCL.remove( { _id: 3 } );
   } );

   // create Idindex
   idxCL.createIdIndex( { SortBufferSize: 256 } );

   //update data and remove data
   idxCL.find( { _id: 3 } ).update( { $set: { a: "testa" } } );
   idxCL.remove( { _id: 2 } );

   // inspect the index
   inspecIndex( idxCL, "$id", "_id", 1 );

   //check the result of find  
   var expRecs = '[{"_id":0,"a":"test0"},{"_id":1,"a":"test1"},{"_id":3,"a":"test3"}]';
   var rc = idxCL.find( { _id: { $lt: 4 } } ).sort( { _id: 1 } );
   checkCLData( expRecs, rc );

   // drop collection in clean
   commDropCL( db, COMMCSNAME, clName, false, false );
}
