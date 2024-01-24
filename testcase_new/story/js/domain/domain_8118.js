/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]).
               2. Test db.createCS() specify domain, CS locate in domain
                  group is correct or not .
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = COMMCSNAME + "_8118";
   var clName = COMMCLNAME + "_8118";
   var domName = "domain_8118";
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; ++i )
   {
      var csRg = groups[i][0].GroupName;
      commDropCS( db, csName, true, "clear environmen in the beginning" );
      commDropDomain( db, domName );
      db.createDomain( domName, [csRg] );
      var varCS = db.createCS( csName, { "Domain": domName } );
      // need to create cl, because when cs has no cl, the cs don't create in data group
      varCS.createCL( clName );

      // Inspect the CS is located in
      db.invalidateCache();  // clean coord and data node cache
      var csGroups = commGetCSGroups( db, csName );
      if( csGroups.length > 1 || csGroups != csRg )
      {
         // sleep 10000ms, see the group that cs located in is changed or not
         sleep( 10000 );
      }
      db.invalidateCache();  // clean coord and data node cache
      csGroups = commGetCSGroups( db, csName );
      var retryTimes = 30;
      while( csGroups.length > 1 && 0 != retryTimes-- )
      {
         sleep( 3000 );
         csGroups = commGetCSGroups( db, csName );
      }
      if( csGroups.length > 1 || csGroups != csRg )
      {
         throw new Error( "error, create CS located in wrong group, csGroups:" + csGroups + ", csRg:" + csRg );
      }
   }
   // Clear the envioronment
   commDropCS( db, csName, false, "clear environmen in the end" );
   db.dropDomain( domName );
}