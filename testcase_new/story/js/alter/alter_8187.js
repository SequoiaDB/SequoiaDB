/******************************************************************************
*@Description : 1. sub-collection with data altered to partition-collection
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

   var mainCLName = "alter8187_main";
   var subCLName1 = "alter8187_sub_1";
   var subCLName2 = "alter8187_sub_2";
   var subCLName3 = "alter8187_sub_3";
   var srcGroup = groupName[0][0]["GroupName"];
   var tarGroup = groupName[1][0]["GroupName"];
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   commDropCL( db, COMMCSNAME, subCLName3 );

   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { IsMainCL: true, ShardingKey: { id: 1 }, ShardingType: "range" } );
   var subcl1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { id: 1 }, ShardingType: 'hash', Group: srcGroup } );
   var subcl2 = commCreateCL( db, COMMCSNAME, subCLName2, { Group: srcGroup } );
   var subcl3 = commCreateCL( db, COMMCSNAME, subCLName3, { Group: srcGroup } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { id: 0 }, UpBound: { id: 300 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { id: 300 }, UpBound: { id: 700 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName3, { LowBound: { id: 700 }, UpBound: { id: 1100 } } );

   var data = [];
   for( var i = 0; i < 1000; i++ )
   {
      data.push( { "id": i, "text": "test alter " + i } );
   }
   maincl.insert( data );

   //alters shardingType
   subcl2.alter( { ShardingKey: { id: 1 }, ShardingType: 'range' } );
   subcl3.alter( { ShardingKey: { id: 1 }, ShardingType: 'hash' } );
   subcl2.split( srcGroup, tarGroup, 50 );
   subcl3.split( srcGroup, tarGroup, 50 );

   //check snapshot
   var snap2 = db.snapshot( 8, { Name: COMMCSNAME + "." + subCLName2 } );
   var info = snap2.current().toObj();
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

   var snap3 = db.snapshot( 8, { Name: COMMCSNAME + "." + subCLName3 } );
   var info = snap3.current().toObj();
   var shardingKey = info['ShardingKey'];
   var shardingType = info['ShardingType'];
   if( JSON.stringify( shardingKey ) !== '{"id":1}' )
   {
      throw new Error( "check shardingKey, \nexpect: {\"id\": 1}, \nbut found: " + JSON.stringify( shardingKey ) );
   }
   if( shardingType !== "hash" )
   {
      throw new Error( "check shardingType, \nexpect: hash, \nbut found: " + shardingType );
   }

   maincl.insert( data );
   var num = maincl.count();
   if( num != 2000 )
   {
      throw new Error( "check recordNum, \nexpect: 2000, \nbut found: " + num );
   }

   //clean test-env
   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   commDropCL( db, COMMCSNAME, subCLName3 );
}