/***************************************************************************
@Description :seqDB-5574:在主子表中创建离线模式Id索引
@Modify list :
              2016-8-10  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_5574";

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, clName, true, true, "drop cl in the beginning" );
   commDropCL( db, COMMCSNAME, "subCL1", true, true, "drop cl in the beginning" );
   commDropCL( db, COMMCSNAME, "subCL2", true, true, "drop cl in the beginning" );
   //@ clean start:
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   // create collection
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true, IsMainCL: true }
   var mainCL = commCreateCL( db, COMMCSNAME, clName, options, true, true );
   var subCL1 = commCreateCL( db, COMMCSNAME, "subCL1", { ShardingKey: { a: 1 }, ShardingType: "hash", AutoIndexId: false, Compressed: true }, true, true );
   var subCL2 = commCreateCL( db, COMMCSNAME, "subCL2", { ShardingKey: { a: 1 }, ShardingType: "hash", AutoIndexId: false, Compressed: true }, true, true );

   mainCL.attachCL( COMMCSNAME + ".subCL1", { LowBound: { a: 0 }, UpBound: { a: 100 } } );
   mainCL.attachCL( COMMCSNAME + ".subCL2", { LowBound: { a: 100 }, UpBound: { a: 200 } } );
   // create Idindex
   mainCL.createIdIndex( { SortBufferSize: 128 } );

   // inspect the index
   inspecIndex( subCL1, "$id", "_id", 1 );
   inspecIndex( subCL2, "$id", "_id", 1 );

   //after create index, insert data
   var doc = [];
   for( var i = 0; i < 200; ++i )
   {
      doc.push( { _id: i, a: i, b: "test" + i } );
   }
   mainCL.insert( doc );

   //check the result of find  
   var expRecs = '[{"_id":170,"a":170,"b":"test170"}]';
   var rc = mainCL.find( { _id: 170 } ).hint( { "": "$id" } );
   checkCLData( expRecs, rc );

   //drop idIndex
   mainCL.dropIdIndex();
   // inspect the index
   commCheckIndexConsistency( subCL1, "$id", false );
   commCheckIndexConsistency( subCL2, "$id", false );

   // drop collection in clean   
   commDropCL( db, COMMCSNAME, "subCL1", false, false, "drop colleciton in the end" );
   commDropCL( db, COMMCSNAME, "subCL2", false, false, "drop colleciton in the end" );
   commDropCL( db, COMMCSNAME, clName, false, false, "drop colleciton in the end" );
}
