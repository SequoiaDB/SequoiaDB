/* *****************************************************************************
@Description: attach hashCL and insert-BoundData with index
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */

testConf.skipStandAlone = true;
main( test );
function test ()
{
   // Not Error, test mainCL'ShardingType is range and subCL's ShardingType is hash ,insert BoundData's result
   MainCL_Name = CHANGEDPREFIX + "year";
   subCl_Name = CHANGEDPREFIX + "month";
   commDropCL( db, COMMCSNAME, subCl_Name + "1", true, true );
   commDropCL( db, COMMCSNAME, subCl_Name + "2", true, true );
   commDropCL( db, COMMCSNAME, MainCL_Name, true, true );

   db.setSessionAttr( { PreferedInstance: "M" } );
   var cs = commCreateCS( db, COMMCSNAME, true, "create cs in the beginning" );

   var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1, b: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true } );
   var subCL1 = cs.createCL( subCl_Name + "1", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   var subCL2 = cs.createCL( subCl_Name + "2", { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "1", { LowBound: { a: 0 }, UpBound: { a: 10 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCl_Name + "2", { LowBound: { a: 10 }, UpBound: { a: 20 } } );

   //insert records
   for( var i = 0; i < 20; ++i )
   {
      mainCL.insert( { a: i } );
   }

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

   mainCL.createIndex( "aIndex", { a: -1, b: 1 } );

   for( var i = 0; i < 20; ++i )
   {
      var actCnt = mainCL.find( { a: i } ).count();
      var expCnt = 1;
      assert.equal( expCnt, actCnt );
   }

   mainCL.dropIndex( "aIndex" );

   mainCL.createIndex( "aIndex", { a: 1, b: 1 } );

   for( var i = 0; i < 20; ++i )
   {
      var actCnt = mainCL.find( { a: i } ).count();
      var expCnt = 1;
      assert.equal( expCnt, actCnt );
   }

   mainCL.dropIndex( "aIndex" );

   mainCL.createIndex( "aIndex", { a: 1, b: -1 } );

   for( var i = 0; i < 20; ++i )
   {
      actCnt = mainCL.find( { a: i } ).count();
      var expCnt = 1;
      assert.equal( expCnt, actCnt );
   }

   mainCL.dropIndex( "aIndex" );

   mainCL.createIndex( "aIndex", { a: -1, b: -1 } );

   for( var i = 0; i < 20; ++i )
   {
      actCnt = mainCL.find( { a: i } ).count();
      var expCnt = 1;
      assert.equal( expCnt, actCnt );
   }

   mainCL.dropIndex( "aIndex" );
   // clean
   commDropCL( db, COMMCSNAME, subCl_Name + "1", false, false );
   commDropCL( db, COMMCSNAME, subCl_Name + "2", false, false );
   commDropCL( db, COMMCSNAME, MainCL_Name, false, false );
}