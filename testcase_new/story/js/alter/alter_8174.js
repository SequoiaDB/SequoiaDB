/******************************************************************************
*@Description : 1. collection altered to main-collection, fail
*@Modify list :
*               2014-07-10 pusheng Ding  Init
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
   var clName1 = "alter8174_1";
   var clName2 = "alter8174_2";
   var clName3 = "alter8174_3";
   var clName4 = "alter8174_4";
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropCL( db, COMMCSNAME, clName3 );
   commDropCL( db, COMMCSNAME, clName4 );

   var cl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { id: 1 }, ShardingType: "hash" } );
   var cl3 = commCreateCL( db, COMMCSNAME, clName3, { ShardingKey: { id: 1 }, ShardingType: "range" } );
   var cl4 = commCreateCL( db, COMMCSNAME, clName4, { ShardingKey: { id: 1 }, ShardingType: "range", IsMainCL: true } );

   //alter cl
   try
   {
      cl1.alter( { "IsMainCL": true } );
      throw new Error( "ERR_ALTEL_ISMAINCL" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      cl2.alter( { "IsMainCL": true } );
      throw new Error( "ERR_ALTEL_ISMAINCL" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      cl3.alter( { "IsMainCL": true } );
      throw new Error( "ERR_ALTEL_ISMAINCL" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      cl4.alter( { "IsMainCL": false } );
      throw new Error( "ERR_ALTEL_ISMAINCL" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropCL( db, COMMCSNAME, clName3 );
   commDropCL( db, COMMCSNAME, clName4 );
}