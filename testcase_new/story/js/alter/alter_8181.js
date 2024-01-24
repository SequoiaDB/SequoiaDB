/******************************************************************************
*@Description : 1. normal collection alters replsize
*@Modify list :
*               2014-07-08  pusheng Ding  Init
*               2015-03-28  xiaojun Hu    Changed
*               2019-10-21  luweikang modify
******************************************************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = "alter8181";
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 1 } );

   //alters replsize
   cl.alter( { ReplSize: 2 } );

   //check snapshot
   var snap = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
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
   cl.insert( data );

   //alters replsize
   cl.alter( { ReplSize: 3 } );

   //check snapshot
   var snap = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var replsize = snap.current().toObj()['ReplSize'];
   if( replsize !== 3 )
   {
      throw new Error( "check replsize, \nexpect: 3, \nbut found: " + replsize );
   }

   cl.remove();
   cl.insert( data );
   cl.insert( data );

   var num = cl.count();
   if( num != 2000 )
   {
      throw new Error( "check recordNum, \nexpect: 2000, \nbut found: " + num );
   }

   //clean test-env
   commDropCL( db, COMMCSNAME, clName );
}