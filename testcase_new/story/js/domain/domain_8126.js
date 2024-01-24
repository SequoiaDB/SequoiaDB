/******************************************************************************
@Description : 1. Test db.getDomain(<name>), specify name list.
               2. Test get domain , domain name don't exist.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8126";

   // Clear domain in the benignning
   clearDomain( db, domName );

   var group = getGroup( db );
   db.createDomain( domName, group );

   // Get domain that not exsit [Testing Point]
   assert.tryThrow( SDB_CAT_DOMAIN_NOT_EXIST, function()
   {
      db.getDomain( "SYSDOMAINGetNotExsitDomain" );
   } );

   // Get domain and list collection and collectionspace [Testing Point]
   var dom = db.getDomain( domName );
   if( dom != domName )
   {
      throw new Error( "ErrGetDom" );
   }

   // Drop domain int the end
   clearDomain( db, domName );
}