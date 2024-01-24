/******************************************************************************
@Description : 1. Test dom.listCollections().create domain specify all groups.
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8122";

   clearDomain( db, domName );
   commDropCS( db, csName, true );

   var group = getGroup( db );
   commCreateDomain( db, domName, group, { AutoSplit: true } );

   // Create collection space and collection
   commCreateCS( db, csName, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csName, clName, { ShardingKey: { "No": -1 }, ShardingType: "hash", Partition: 1024 }, false, false, "create collection in domain" );

   // domain list collections and inspect [Testing Point]
   var dom = db.getDomain( domName );
   var domCLname = dom.listCollections().current().toObj()["Name"];
   var CsCl = csName + "." + clName;
   assert.equal( CsCl, domCLname );

   clearDomain( db, domName );
}