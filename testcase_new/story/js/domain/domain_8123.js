/******************************************************************************
@Description : 1.Test dom.listCollectionSpace().create domain specify all groups.
               2.Test db.createCS("foo",{Domain:"domainName"})
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   domName = csName + "_8123";

   clearDomain( db, domName );
   commDropCS( db, csName, true );

   var group = getGroup( db );
   commCreateDomain( db, domName, group, { AutoSplit: true } );

   // Create collection space and collection[Testing Point]
   commCreateCS( db, csName, false, "create CS specify domain",
      { "Domain": domName } );
   commCreateCL( db, csName, clName, { ShardingKey: { "No": -1 }, ShardingType: "hash", Partition: 1024 }, false, false, "create collection in domain" );

   // domain list collections and inspect [Testing Point]
   var dom = db.getDomain( domName );
   var domCSname = dom.listCollectionSpaces().current().toObj()["Name"];
   assert.equal( csName, domCSname );

   clearDomain( db, domName );
}