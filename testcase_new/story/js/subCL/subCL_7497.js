/* *****************************************************************************
@Description: attach hashCL and insert-BoundData
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */
testConf.skipStandAlone = true;
main( test );
function test () // Not Error, test mainCL'ShardingType is range and subCL's ShardingType is hash ,insert BoundData's result
{
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

   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      mainCL.insert( { a: -1 } );
   } );

   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      mainCL.insert( { a: 2 } );
   } );

   for( var i = 0; i < 1000; ++i )
   {
      mainCL.insert( { a: 0 } );
      mainCL.insert( { a: 1, b: i } );
   }

   for( var i = 0; i < 1000; ++i )
   {
      mainCL.insert( { a: 1 } );
      mainCL.insert( { a: 2, b: i } );
   }

   //insert records
   var mainCnt = mainCL.count();
   var subCLCnt1 = subCL1.count();
   var subCLCnt2 = subCL2.count();
   //check results
   var sumCnt = 0;
   var sumCnt = subCLCnt1 + subCLCnt2;
   if( parseInt( mainCnt ) !== parseInt( sumCnt ) || parseInt( mainCnt ) !== 4000 )
   {
      throw new Error( "fail" );
   }
   commDropCL( db, COMMCSNAME, MainCL_Name, false, false );
}