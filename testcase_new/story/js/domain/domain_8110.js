/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]), only have domain name.
               2. Test "NULL domain" cannot be created collection space.
@Modify list :
               2014-6-17  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var DOMCSNAME = CHANGEDPREFIX + "_8110";
   var domainName = CHANGEDPREFIX + "_8110";

   // Drop CS in the beginning
   commDropCS( db, DOMCSNAME, true, "create CS in the beginning" );

   // Drop domain in the beginning
   clearDomain( db, domainName );

   // Create domain only have domainName [Testing Point]
   db.createDomain( domainName );

   assert.tryThrow( SDB_CAT_NO_GROUP_IN_DOMAIN, function()
   {
      db.createCS( csName, { "Domain": domainName } );
   } );

   // Drop domain int the end
   clearDomain( db, domainName );
}