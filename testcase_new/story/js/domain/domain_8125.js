/******************************************************************************
@Description : 1. Test db.dropDomains(<name>), specify not exist name .
               2. Test create four domains.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8125";
   commDropCS( db, csName, true );
   clearDomain( db, domName );

   var group = new Array();
   group = getGroup( db );
   db.createDomain( domName, group );

   commCreateCS( db, csName, false, "create CS specify domain", { "Domain": domName } );
   commCreateCL( db, csName, clName, {}, false, false );

   // Drop not exist domain [Testing Point]
   assert.tryThrow( SDB_CAT_DOMAIN_NOT_EXIST, function()
   {
      db.dropDomain( "SYSDOMAIN" );
   } );

   // Drop domain where CS/CL/data record in [Testing Point]
   assert.tryThrow( SDB_DOMAIN_IS_OCCUPIED, function()
   {
      db.dropDomain( domName );
   } );

   commDropCS( db, csName, true );
   clearDomain( db, domName );
}