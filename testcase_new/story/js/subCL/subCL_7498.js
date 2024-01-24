/* *****************************************************************************
@Description: attach hashCL and insert and remove
@modify list:
   2014-07-30 pusheng Ding  Init
***************************************************************************** */
var csName = COMMCSNAME;
var MainCL_Name = CHANGEDPREFIX + "_year_7498";
var subCl_Name1 = CHANGEDPREFIX + "_month1_7498";
var subCl_Name2 = CHANGEDPREFIX + "_month2_7498";

main( test );

// Not Error, test mainCL'ShardingType is range and subCL's ShardingType is hash ,insert large number's result
function test () 
{

   if( commIsStandalone( db ) )
   {
      return;
   }
   commDropCL( db, csName, subCl_Name1, true, true );
   commDropCL( db, csName, subCl_Name2, true, true );
   commDropCL( db, csName, MainCL_Name, true, true );

   db.setSessionAttr( { PreferedInstance: "M" } );
   var cs = commCreateCS( db, csName, true, "create cs in the beginning" );

   var mainCL = cs.createCL( MainCL_Name, { ShardingKey: { a: 1, b: -1 }, ShardingType: "range", ReplSize: 0, Compressed: true, IsMainCL: true } );
   var subCL1 = cs.createCL( subCl_Name1, { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   var subCL2 = cs.createCL( subCl_Name2, { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0, Compressed: true, IsMainCL: false } );
   mainCL.attachCL( csName + "." + subCl_Name1, { LowBound: { a: 0, b: 1000 }, UpBound: { a: 1000, b: 0 } } );
   mainCL.attachCL( csName + "." + subCl_Name2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   mainCL.remove();
   for( var i = 0; i < 2000; ++i )
   {
      mainCL.insert( { a: i } );
   }
   mainCL.remove();
   //check results
   var mainCnt = mainCL.find().count();
   assert.equal( mainCnt, 0 );

   subCL1.remove();
   subCL2.remove();
   mainCL.remove();
   for( var i = 0; i < 2000; ++i )
   {
      mainCL.insert( { a: i } );
   }
   subCL1.remove();
   subCL2.remove();
   //check results
   var mainCnt = mainCL.count();
   assert.equal( mainCnt, 0 );

   mainCL.remove();
   for( var i = 0; i < 2000; ++i )
   {
      mainCL.insert( { a: i } );
   }
   subCL1.remove();
   //check results
   var subCnt1 = subCL1.count();
   var mainCnt1 = mainCL.find( { a: { $gte: 1000 } } ).count();
   var mainCnt2 = mainCL.count();
   if( 0 !== parseInt( subCnt1 ) || 1000 !== parseInt( mainCnt1 )
      || parseInt( mainCnt1 ) !== parseInt( mainCnt ) )
   {
      throw new Error( "result error" );
   }

   for( var i = 0; i < 2000; ++i )
   {
      mainCL.insert( { a: i } );
   }
   subCL2.remove();
   //check results
   var subCnt2 = subCL2.count();
   var mainCnt1 = mainCL.find( { a: { $lt: 1000 } } ).count();
   var mainCnt2 = mainCL.count();
   if( 0 !== parseInt( subCnt2 ) || 1000 !== parseInt( mainCnt1 )
      || parseInt( mainCnt1 ) !== parseInt( mainCnt ) )
   {
      throw new Error( "result error" );
   }

   mainCL.remove();
   subCL1.remove();
   subCL2.remove();
   //check results
   var mainCnt = mainCL.find().count();
   assert.equal( 0, mainCnt );

   commDropCL( db, csName, subCl_Name1, false, false );
   commDropCL( db, csName, subCl_Name2, false, false );
   commDropCL( db, csName, MainCL_Name, false, false );
}