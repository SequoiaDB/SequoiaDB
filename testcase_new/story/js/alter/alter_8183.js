/******************************************************************************
*@Description : 1. normal collection with data altered to range-collection
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
   var groupName = commGetGroups( db );
   if( groupName.length < 2 )
   {
      return;
   }

   var clName = "alter8183";
   var srcGroup = groupName[0][0]["GroupName"];
   var tarGroup = groupName[1][0]["GroupName"];
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { Group: srcGroup } );

   var data = [];
   for( var i = 0; i < 1000; i++ )
   {
      data.push( { "id": i, "text": "test alter " + i } );
   }
   cl.insert( data );

   //alter cl to shardingCL
   cl.alter( { "ShardingKey": { "id": 1 }, "ShardingType": "range" } );
   cl.split( srcGroup, tarGroup, 50 );

   //check snapshot
   var snap = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var info = snap.current().toObj();
   var shardingKey = info['ShardingKey'];
   var shardingType = info['ShardingType'];
   if( JSON.stringify( shardingKey ) !== '{"id":1}' )
   {
      throw new Error( "check shardingKey, \nexpect: {\"id\": 1}, \nbut found: " + JSON.stringify( shardingKey ) );
   }
   if( shardingType !== "range" )
   {
      throw new Error( "check shardingType, \nexpect: range, \nbut found: " + shardingType );
   }

   var num = cl.count();
   if( num != 1000 )
   {
      throw new Error( "check recordNum, \nexpect: 1000, \nbut found: " + num );
   }

   commDropCL( db, COMMCSNAME, clName );
}