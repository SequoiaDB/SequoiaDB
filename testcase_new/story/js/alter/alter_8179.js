/******************************************************************************
*@Description : 1. main collection alters replsize
*@Modify list :
*               2014-07-04  pusheng Ding  Init
*               2015-03-28 xiaojun Hu    Changed
*               2019-10-21  luweikang modify
******************************************************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "alter8179_main";
   var subCLName = "alter8179_sub";

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { id: 1 }, ShardingType: "range", ReplSize: 1 } );
   commCreateCL( db, COMMCSNAME, subCLName, { ReplSize: 1 } );
   maincl.attachCL( COMMCSNAME + "." + subCLName, { LowBound: { id: MinKey() }, UpBound: { id: MaxKey() } } );

   //alters replsize
   maincl.alter( { ReplSize: 2 } );

   //check snapshot
   var snap = db.snapshot( 8, { Name: COMMCSNAME + "." + mainCLName } );
   var replsize = snap.current().toObj()['ReplSize'];
   if( replsize !== 2 )
   {
      throw new Error( "check replsize, \nexpect: 2, \nbut found: " + replsize );
   }

   var data = [];
   for( var i = 0; i < 1000; i++ )
   {
      data.push( { "id": i, "text": "test alter " + i } );
   }
   maincl.insert( data );

   var num = maincl.count();
   if( num != 1000 )
   {
      throw new Error( "check recordNum, \nexpect: 1000, \nbut found: " + num );
   }

   //clean test-env
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName );
}