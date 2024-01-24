/******************************************************************************
*@Description : 1. sub collection alters replsize
*@Modify COORDHOSTNAME :
*               2014-07-08  pusheng Ding  Init
*               2015-03-28  xiaojun Hu    Changed
*               2017-02-08  zhaoyu   Changed
*               2019-10-21  luweikang modify
******************************************************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "alter8180_main";
   var subCLName1 = "alter8180_sub_1";
   var subCLName2 = "alter8180_sub_2";
   var subCLName3 = "alter8180_sub_3";
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   commDropCL( db, COMMCSNAME, subCLName3 );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { id: 1 }, ShardingType: "range", ReplSize: 1 } );
   var subcl1 = commCreateCL( db, COMMCSNAME, subCLName1, { ReplSize: 1 } );
   var subcl2 = commCreateCL( db, COMMCSNAME, subCLName2, { ReplSize: 1 } );
   var subcl3 = commCreateCL( db, COMMCSNAME, subCLName3, { ReplSize: 1 } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { id: 0 }, UpBound: { id: 300 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { id: 300 }, UpBound: { id: 700 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName3, { LowBound: { id: 700 }, UpBound: { id: 1100 } } );

   //alters replsize
   subcl1.alter( { ReplSize: 3 } );
   subcl2.alter( { ReplSize: 2 } );
   subcl3.alter( { ReplSize: 1 } );

   //check snapshot
   var snap1 = db.snapshot( 8, { Name: COMMCSNAME + "." + subCLName1 } );
   var replsize = snap1.current().toObj()['ReplSize'];
   if( replsize !== 3 )
   {
      throw new Error( "check replsize, \nexpect: 3, \nbut found: " + replsize );
   }

   var snap2 = db.snapshot( 8, { Name: COMMCSNAME + "." + subCLName2 } );
   var replsize = snap2.current().toObj()['ReplSize'];
   if( replsize !== 2 )
   {
      throw new Error( "check replsize, \nexpect: 2, \nbut found: " + replsize );
   }

   var snap3 = db.snapshot( 8, { Name: COMMCSNAME + "." + subCLName3 } );
   var replsize = snap3.current().toObj()['ReplSize'];
   if( replsize !== 1 )
   {
      throw new Error( "check replsize, \nexpect: 1, \nbut found: " + replsize );
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
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   commDropCL( db, COMMCSNAME, subCLName3 );
}