/******************************************************************************
@Description : 1. Test db.createDomain(<name>,[option]), have name and Groups.
               2. Test create CS on the domain.
               3. Test insert/update/find/remove operation.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var DOMCSNAME = CHANGEDPREFIX + "_8111";
   var domainName = CHANGEDPREFIX + "_8111";

   commDropCS( db, DOMCSNAME, true );
   clearDomain( db, domainName );

   // Get one group and create domain [Testing Point]
   var group = new Array();
   group = getGroup( db );
   db.createDomain( domainName, [group[0]] );

   // Create CS in domain and create collection [Testing Point]
   commCreateCS( db, DOMCSNAME, false, "create CS specify domain", { "Domain": domainName } );
   commCreateCL( db, DOMCSNAME, COMMCLNAME, {}, false, false );

   // Insert data
   insertData( db, DOMCSNAME, COMMCLNAME, 1000 )

   // Query data
   queryData( db, DOMCSNAME, COMMCLNAME )

   // Update data
   updateData( db, DOMCSNAME, COMMCLNAME )

   // Remove data
   removeData( db, DOMCSNAME, COMMCLNAME )

   // Drop domain int the end
   clearDomain( db, domainName );
}