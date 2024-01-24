/******************************************************************************
*@Description : 1. range collection alters replsize
*@Modify list :
*               2014-07-07  pusheng Ding  Init
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
   var groupName = commGetGroups( db );
   if( groupName.length < 2 )
   {
      return;
   }

   var clName = "alter8177";
   var srcGroup = groupName[0][0]["GroupName"];
   var tarGroup = groupName[1][0]["GroupName"];
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { id: 1 }, ShardingType: "range", Group: srcGroup, ReplSize: 1 } );

   //alters replsize
   cl.alter( { ReplSize: 2 } );

   //check snapshot
   var snap1 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var replsize = snap1.current().toObj()['ReplSize'];
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
   cl.split( srcGroup, tarGroup, 50 );
   cl.alter( { ReplSize: 3 } );

   //check snapshot
   var snap2 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var replsize = snap2.current().toObj()['ReplSize'];
   if( replsize !== 3 )
   {
      throw new Error( "check replsize, \nexpect: 3, \nbut found: " + replsize );
   }

   cl.insert( data );
   var num = cl.count();
   if( num != 2000 )
   {
      throw new Error( "check recordNum, \nexpect: 2000, \nbut found: " + num );
   }

   //clean test-env
   commDropCL( db, COMMCSNAME, clName );
}