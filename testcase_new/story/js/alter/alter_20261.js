/******************************************************************************
*@Description: 修改有lob的普通表为range表
*@author:      luweikang
*@createdate:  2019.11.15
*@testlinkCase:seqDB-20261
******************************************************************************/
try
{
   var filePath = WORKDIR + "/" + "file20261";
   var isStandalone = commIsStandalone( db );
   main( test );
} finally
{
   if( !isStandalone )
   {
      File.remove( filePath );
   }
}

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var clName = "alter20261";
   commDropCL( db, COMMCSNAME, clName );

   var cl = commCreateCL( db, COMMCSNAME, clName );
   var file = new File( filePath );
   file.write( "test lob alter shardingType range" );
   file.close();
   var lobId = cl.putLob( filePath );

   //alters ShardingType
   try
   {
      cl.alter( { ShardingKey: { a: 1 }, ShardingType: "range" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }
   //check snapshot
   var snap1 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var clInfo = snap1.current().toObj();
   if( clInfo.hasOwnProperty( "ShardingType" ) )
   {
      throw new Error( "check snapshot error, \nexpect: not shardingType, \nbut found: " + JSON.stringify( clInfo ) );
   }

   try
   {
      cl.setAttributes( { ShardingKey: { a: 1 }, ShardingType: "range" } );
      throw new Error( "ERR_ALTER_CL" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }
   //check snapshot
   var snap2 = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var clInfo = snap1.current().toObj();
   if( clInfo.hasOwnProperty( "ShardingType" ) )
   {
      throw new Error( "check snapshot error, \nexpect: not shardingType, \nbut found: " + JSON.stringify( clInfo ) );
   }

   cl.deleteLob( lobId );
   cl.setAttributes( { ShardingKey: { a: 1 }, ShardingType: "range" } );
   checkSnapshot( db, SDB_SNAP_CATALOG, COMMCSNAME, clName, "ShardingType", "range" );

   //clean test-env
   commDropCL( db, COMMCSNAME, clName );
}
