
testConf.skipStandAlone = true;
main( test );

function test ()
{
   //set priority from masterNode
   db.setSessionAttr( { PreferedInstance: "M" } );

   test_range_attach_hash_update_2();

}

function test_range_attach_hash_update_2 ()// NOT Error, test mainCL'ShardingType is range and subCL's ShardingType is hash , insert's result
{
   MainCL_Name = CHANGEDPREFIX + "year";
   subCl_Name = CHANGEDPREFIX + "month";
   commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true );
   commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true );
   commDropCL( db, COMMCSNAME, MainCL_Name, true, true );
   var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );
   var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1 }, ShardingType: "range", Partition: 4096, ReplSize: 0, Compressed: true, IsMainCL: true } );
   var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "1", { LowBound: { a: 0 }, UpBound: { a: 1 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "2", { LowBound: { a: 1 }, UpBound: { a: 2 } } );

   var subCL = [];
   subCL.push( subCL1 );
   subCL.push( subCL2 );
   var numberOfsubCl = 2;
   for( var i = 0; i < numberOfsubCl; ++i )
   {
      var sourceDataGroupName = getSourceGroupName_alone( COMMCSNAME, subCl_Name + ( i + 1 ) );

      var desDataGroupName = getOtherDataGroups( sourceDataGroupName );

      var Partition = getPartition( COMMCSNAME, subCl_Name + ( i + 1 ) );

      if( !subCL_split_hash( subCL[i], sourceDataGroupName, desDataGroupName, Partition ) )
      {
      }
   }

   for( var i = 0; i < 2; ++i )
   {
      mainCL.insert( { a: i, b: { name: "YoYo", age: 23, phone: [12, 56, "reqnf"] }, c: "jkdi" } );
   }

   mainCL.update( { $unset: { c: "jkdi" } } );

   var rc = mainCL.find( { c: "jkdi" } );

   assert.equal( mainCL.find( { c: "jkdi" } ).count(), 0 );

   mainCL.update( { $inc: { salary: 100 } } );
   var rc1 = mainCL.find( { salary: 100 } );

   assert.equal( mainCL.find( { salary: 100 } ).count(), 2 );

   mainCL.update( { $push: { "b.phone": 3 } } );
   var rc2 = mainCL.find( { "b.phone.3": 3 } );
   assert.equal( mainCL.find( { "b.phone.3": 3 } ).count(), 2 );

   mainCL.update( { $pull: { "b.phone": 3 } } );
   var rc3 = mainCL.find( { "b.phone.3": 3 } );

   assert.equal( 0, mainCL.find( { "b.phone.3": 3 } ).count() );

   mainCL.update( { $push_all: { array: [3, 4] } } );
   var rc4 = mainCL.find( { array: [3, 4] } );
   assert.equal( 2, mainCL.find( { array: [3, 4] } ).count() );

   mainCL.update( { $pull_all: { array: [3, 4] } } );
   var rc5 = mainCL.find( { array: [] } );
   assert.equal( 2, mainCL.find( { array: [] } ).count() );

   mainCL.update( { $pop: { "b.phone": 2 } } );
   var rc6 = mainCL.find( { "b.phone": [12] } );
   assert.equal( 2, mainCL.find( { "b.phone": [12] } ).count() );

   mainCL.update( { $addtoset: { "b.phone": [12] } } );
   var rc7 = mainCL.find( { "b.phone": [12] } );
   assert.equal( 2, mainCL.find( { "b.phone": [12] } ).count() );
   //	
   //	
}
