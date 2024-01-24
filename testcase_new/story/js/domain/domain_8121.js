/******************************************************************************
@Description : 1. Test dom.alter(<options>), specify {AutoSplit:true}.
               2. Before alter, the group have data
@Modify list :
               2014-6-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var domName = csName + "_8121";
   // Clear domain in the beginning
   clearDomain( db, domName );

   // Drop Collection space in the beginning
   commDropCS( db, csName, true );

   // Alter the group to domain [Testing Point]
   var group = new Array();
   group = getGroup( db );

   // Create domain without group and autosplit
   commCreateDomain( db, domName, group );
   // Create collection space and collection
   commCreateCS( db, csName, false, "create CS specify domain", { "Domain": domName } );
   commCreateCL( db, csName, clName, { ShardingKey: { "No": -1 }, ShardingType: "hash", Partition: 1024 }, false, false, "create collection in domain" );

   // Inspect data to SDB
   insertData( db, csName, clName, 1000 );
   inspectAutoSplit( db, csName, clName, domName );
   db.getCS( csName ).getCL( clName ).remove();
   dom = db.getDomain( domName );

   dom.alter( { AutoSplit: true } );

   // Inspect data to SDB
   insertData( db, csName, clName, 1000 );

   // inspect the AutoSplit is take effect or not
   inspectAutoSplit( db, csName, clName, domName );

   // Alter group and autosplit [Test Point]
   dom.alter( { "Groups": group, AutoSplit: true } );

   // Clear domain in the end
   clearDomain( db, domName );
}