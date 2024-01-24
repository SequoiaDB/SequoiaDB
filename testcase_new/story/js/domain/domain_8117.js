/******************************************************************************
@Description : Test create Domain specify group : "SYSCatalogGroup".
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domNameSYS = csName + "_DomGroupSYSCata";
   var domName = csName + "_DomAllGroupComprareSYS";

   clearDomain( db, domName );
   commDropCS( db, csName, true );
   var group = getGroup( db );
   db.createDomain( domName, group );

   assert.tryThrow( SDB_CAT_IS_NOT_DATAGROUP, function()
   {
      db.createDomain( domNameSYS, ["SYSCatalogGroup"] );
   } );

   // Create CS in domain and create collection
   commCreateCS( db, csName, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csName, clName, {}, false, false );

   // Insert data
   insertData( db, csName, clName, 1000 );

   // Query data
   queryData( db, csName, clName );

   // Update data
   updateData( db, csName, clName );

   // Remove data
   removeData( db, csName, clName );

   // Clear domain in the end
   clearDomain( db, domName );
}