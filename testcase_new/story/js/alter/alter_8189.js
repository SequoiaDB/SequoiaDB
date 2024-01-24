/******************************************************************************
*@Description : 1.hash-collection altered to partition-collection
*@Modify list :
*               2014-07-09  pusheng Ding  Init
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

   var clName = "alter8189";
   var srcGroup = groupName[0][0]["GroupName"];
   var tarGroup = groupName[1][0]["GroupName"];
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { id: 1 }, ShardingType: "hash", Group: srcGroup } );
   var data = [];
   for( var i = 0; i < 1000; i++ )
   {
      data.push( { "id": i, "text": "test alter " + i } );
   }
   cl.insert( data );
   cl.split( srcGroup, tarGroup, 50 );

   //alters shardingType
   try
   {
      cl.alter( { ShardingType: "hash" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }

   try
   {
      cl.alter( { ShardingType: "range" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }

   var num = cl.count();
   if( num != 1000 )
   {
      throw new Error( "check recordNum, \nexpect: 1000, \nbut found: " + num );
   }

   //clean test-env
   commDropCL( db, COMMCSNAME, clName );
}
